#!/usr/bin/env bash
#
# Rehearse or apply the explicit visitor grant for one legacy personal locker.
# The target is intentionally supplied at runtime so private production
# identifiers never enter the repository.
#
# Required environment:
#   LOCKER_REPAIR_NAME            Exact legacy "<player>.locker" name.
#   LOCKER_REPAIR_OWNER_PID       Expected current owner PID.
#   LOCKER_REPAIR_EXPECTED_ITEMS  Exact payload count (21 for issue #122).
#
# Apply mode also requires an unused absolute LOCKER_REPAIR_BACKUP path. The
# script dumps every existing access row for the target before inserting the
# derived current account as an idempotent visitor grant.
#
# Usage: repair_legacy_personal_locker_access.sh --check|--apply
set -euo pipefail

usage() {
	echo 'usage: repair_legacy_personal_locker_access.sh --check|--apply' >&2
}

case "${1:-}" in
	--check | --apply)
		mode="$1"
		;;
	*)
		usage
		exit 2
		;;
esac

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
environment_file="${MIGRATION_ENV_FILE:-$PROJECT_ROOT/.env}"
[[ -f "$environment_file" && ! -L "$environment_file" ]] || {
	echo 'locker repair configuration must be a regular, non-symlink file' >&2
	exit 1
}
set -a
# shellcheck disable=SC1090
source "$environment_file"
set +a

: "${DB_HOST:?missing DB_HOST}"
: "${DB_USER:?missing DB_USER}"
: "${DB_PASSWD:?missing DB_PASSWD}"
: "${DB_NAME:?missing DB_NAME}"

locker_name="${LOCKER_REPAIR_NAME:-}"
owner_pid="${LOCKER_REPAIR_OWNER_PID:-}"
expected_items="${LOCKER_REPAIR_EXPECTED_ITEMS:-}"
backup_path="${LOCKER_REPAIR_BACKUP:-}"
db_port="${DB_PORT:-3306}"

[[ "$locker_name" =~ ^[A-Za-z][A-Za-z0-9_-]{0,63}\.locker$ ]] || {
	echo 'LOCKER_REPAIR_NAME must be an exact legacy personal-locker name' >&2
	exit 1
}
[[ "$owner_pid" =~ ^[1-9][0-9]*$ ]] || {
	echo 'LOCKER_REPAIR_OWNER_PID must be a positive integer' >&2
	exit 1
}
[[ "$expected_items" =~ ^[1-9][0-9]*$ ]] || {
	echo 'LOCKER_REPAIR_EXPECTED_ITEMS must be a positive integer' >&2
	exit 1
}
[[ "$db_port" =~ ^[1-9][0-9]*$ ]] || {
	echo 'DB_PORT must be a positive integer' >&2
	exit 1
}

environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
if [[ ! "${environment_name,,}" =~ ^(dev|development|local|test)$ ]]; then
	expected_ack="${mode#--}:${DB_HOST}:${db_port}/${DB_NAME}:${locker_name}:${owner_pid}:${expected_items}"
	[[ "${LOCKER_REPAIR_PRODUCTION_ACK:-}" == "$expected_ack" ]] || {
		echo "refusing non-development locker repair; set the exact confirmation: $expected_ack" >&2
		exit 1
	}
fi

MYSQL_SSL=()
DUMP_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
	[[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
		echo 'remote locker repair requires TLS and a CA file' >&2
		exit 1
	}
	if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
		MYSQL_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
	elif mysql --help 2>&1 | grep -q -- '--ssl-verify-server-cert'; then
		MYSQL_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
	else
		echo 'database client cannot verify the remote server identity' >&2
		exit 1
	fi
	if mysqldump --help 2>&1 | grep -q -- '--ssl-mode'; then
		DUMP_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
	elif mysqldump --help 2>&1 | grep -q -- '--ssl-verify-server-cert'; then
		DUMP_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
	else
		echo 'database dump client cannot verify the remote server identity' >&2
		exit 1
	fi
else
	if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
		MYSQL_SSL=(--ssl-mode=PREFERRED)
	else
		MYSQL_SSL=(--skip-ssl)
	fi
	if mysqldump --help 2>&1 | grep -q -- '--ssl-mode'; then
		DUMP_SSL=(--ssl-mode=PREFERRED)
	else
		DUMP_SSL=(--skip-ssl)
	fi
fi

export MYSQL_PWD="$DB_PASSWD"
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "$db_port" -u "$DB_USER" -N -B)
MYSQLDUMP=(mysqldump "${DUMP_SSL[@]}" -h "$DB_HOST" -P "$db_port" -u "$DB_USER")

# Mirror sql_locker_owner_can_access() and additionally pin the private audit's
# expected payload count. Multiple current mappings deliberately fail closed.
preflight=$("${MYSQL[@]}" "$DB_NAME" -e "
SELECT l.id,ac.account_name,COUNT(li.id),
       IF(EXISTS(SELECT 1 FROM locker_access la
                 WHERE LOWER(la.owner)=LOWER(l.locker_name)
                   AND LOWER(la.visitor)=LOWER(ac.account_name)),1,0)
FROM lockers l
JOIN account_characters ac
  ON ac.pid=l.owner_pid AND ac.racewar=l.racewar
 AND ac.blocked=0 AND ac.deleted_at IS NULL
LEFT JOIN locker_items li ON li.locker_id=l.id
WHERE LOWER(l.locker_name)=LOWER('$locker_name')
  AND l.owner_pid=$owner_pid AND l.owner_assoc_id IS NULL
GROUP BY l.id,l.locker_name,ac.account_name
HAVING COUNT(li.id)=$expected_items;")

mapfile -t candidates <<< "$preflight"
if [[ "${#candidates[@]}" != 1 || -z "${candidates[0]}" ]]; then
	echo 'locker repair preflight did not find exactly one current owner mapping with the expected payload count' >&2
	exit 1
fi
IFS=$'\t' read -r locker_id visitor_account item_count grant_ready extra <<< "${candidates[0]}"
[[ "$locker_id" =~ ^[1-9][0-9]*$ && "$item_count" == "$expected_items" &&
   "$grant_ready" =~ ^[01]$ && -z "${extra:-}" ]] || {
	echo 'locker repair preflight returned malformed database evidence' >&2
	exit 1
}
[[ "$visitor_account" =~ ^[A-Za-z0-9][A-Za-z0-9_.@+-]{0,63}$ ]] || {
	echo 'derived visitor account is not safe for the legacy grant' >&2
	exit 1
}

printf 'candidate_count=1 item_count=%s owner_path_ready=1 visitor_grant_ready=%s\n' \
	"$item_count" "$grant_ready"
[[ "$mode" == "--check" ]] && exit 0

[[ "$backup_path" == /* ]] || {
	echo 'LOCKER_REPAIR_BACKUP must be an absolute path in apply mode' >&2
	exit 1
}
[[ -d "$(dirname "$backup_path")" && ! -e "$backup_path" ]] || {
	echo 'LOCKER_REPAIR_BACKUP parent must exist and the backup path must be unused' >&2
	exit 1
}

umask 077
"${MYSQLDUMP[@]}" --no-create-info --skip-triggers --compact "$DB_NAME" locker_access \
	--where="LOWER(owner)=LOWER('$locker_name')" > "$backup_path"
[[ -s "$backup_path" ]] || {
	echo 'locker access backup was not created; refusing repair' >&2
	exit 1
}

"${MYSQL[@]}" "$DB_NAME" -e "
START TRANSACTION;
INSERT IGNORE INTO locker_access(owner,visitor)
VALUES ('$locker_name','$visitor_account');
COMMIT;"

post_grant=$("${MYSQL[@]}" "$DB_NAME" -e "
SELECT COUNT(*) FROM locker_access
WHERE LOWER(owner)=LOWER('$locker_name')
  AND LOWER(visitor)=LOWER('$visitor_account');")
post_items=$("${MYSQL[@]}" "$DB_NAME" -e \
	"SELECT COUNT(*) FROM locker_items WHERE locker_id=$locker_id;")
[[ "$post_grant" == 1 && "$post_items" == "$expected_items" ]] || {
	echo "locker repair verification failed; preserve and inspect $backup_path" >&2
	exit 1
}
printf 'repair_applied=1 item_count=%s backup=%s\n' "$post_items" "$backup_path"
