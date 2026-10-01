#!/usr/bin/env bash
#
# Reconcile one custody_pending pet whose saved room no longer matches its
# owner's saved room. The target is supplied at runtime so private production
# identifiers never enter the repository.
#
# Required environment:
#   PET_CUSTODY_REPAIR_OWNER_PID  Exact player_data.pid.
#   PET_CUSTODY_REPAIR_UID        Exact player_pets.pet_uid.
#
# Apply mode also requires an unused absolute PET_CUSTODY_REPAIR_BACKUP path.
# The script dumps the exact player_pets row before changing room_vnum. Outside
# development it additionally requires PET_CUSTODY_REPAIR_PRODUCTION_ACK to
# equal the target-pinned confirmation printed by the failed check.
#
# Usage: repair_stale_pet_custody.sh --check|--apply
set -euo pipefail

usage() {
	echo 'usage: repair_stale_pet_custody.sh --check|--apply' >&2
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
	echo 'pet custody repair configuration must be a regular, non-symlink file' >&2
	exit 1
}
set -a
# shellcheck disable=SC1090
source "$environment_file"
set +a

: "${DB_HOST:?missing DB_HOST}" "${DB_USER:?missing DB_USER}"
: "${DB_PASSWD:?missing DB_PASSWD}" "${DB_NAME:?missing DB_NAME}"

owner_pid="${PET_CUSTODY_REPAIR_OWNER_PID:-}"
pet_uid="${PET_CUSTODY_REPAIR_UID:-}"
backup_path="${PET_CUSTODY_REPAIR_BACKUP:-}"
db_port="${DB_PORT:-3306}"
[[ "$owner_pid" =~ ^[1-9][0-9]*$ && "$owner_pid" -le 4294967295 ]] || {
	echo 'PET_CUSTODY_REPAIR_OWNER_PID must be a positive player PID' >&2
	exit 1
}
[[ "$pet_uid" =~ ^[1-9][0-9]*$ ]] || {
	echo 'PET_CUSTODY_REPAIR_UID must be a positive durable pet UID' >&2
	exit 1
}
[[ "$db_port" =~ ^[1-9][0-9]*$ ]] || {
	echo 'DB_PORT must be a positive integer' >&2
	exit 1
}

environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
if [[ ! "${environment_name,,}" =~ ^(dev|development|local|test)$ ]]; then
	expected_ack="${mode#--}:${DB_HOST}:${db_port}/${DB_NAME}:${owner_pid}:${pet_uid}"
	[[ "${PET_CUSTODY_REPAIR_PRODUCTION_ACK:-}" == "$expected_ack" ]] || {
		echo "refusing non-development pet custody repair; set the exact confirmation: $expected_ack" >&2
		exit 1
	}
fi

MYSQL_SSL=()
DUMP_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
	[[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
		echo 'remote pet custody repair requires TLS and a CA file' >&2
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

# Require one exact held row whose owner still exists and whose room is stale.
# The custody count is evidence only: an empty set is repaired here as well,
# then the normal full save deletes it under the tightened repository predicate.
preflight=$("${MYSQL[@]}" "$DB_NAME" -e "
SELECT pp.id,pp.room_vnum,pd.last_room,
       (SELECT COUNT(*) FROM item_current_owner own
        WHERE own.owner_type=11 AND own.owner_id=pp.pet_uid
          AND own.owner_context_id=pp.owner_pid AND own.state IN (1,3)),
       (SELECT COUNT(*) FROM player_pet_items ppi WHERE ppi.pet_id=pp.id)
FROM player_pets pp JOIN player_data pd ON pd.pid=pp.owner_pid
WHERE pp.id IS NOT NULL AND pp.owner_pid=$owner_pid AND pp.pet_uid=$pet_uid
  AND pp.hold_reason=6 AND pp.room_vnum<>pd.last_room;")
mapfile -t candidates <<< "$preflight"
if [[ "${#candidates[@]}" != 1 || -z "${candidates[0]}" ]]; then
	echo 'pet custody repair preflight did not find exactly one stale custody_pending row' >&2
	exit 1
fi
IFS=$'\t' read -r pet_id room_before owner_room custody_rows payload_rows extra <<< "${candidates[0]}"
[[ "$pet_id" =~ ^[1-9][0-9]*$ && "$room_before" =~ ^[1-9][0-9]*$ &&
	"$owner_room" =~ ^[1-9][0-9]*$ && "$room_before" != "$owner_room" &&
	"$custody_rows" =~ ^[0-9]+$ && "$payload_rows" =~ ^[0-9]+$ &&
	-z "${extra:-}" ]] || {
	echo 'pet custody repair preflight returned malformed database evidence' >&2
	exit 1
}

printf 'candidate_count=1 room_before=%s room_after=%s custody_rows=%s payload_rows=%s\n' \
	"$room_before" "$owner_room" "$custody_rows" "$payload_rows"
[[ "$mode" == '--check' ]] && exit 0

[[ "$backup_path" == /* ]] || {
	echo 'PET_CUSTODY_REPAIR_BACKUP must be an absolute path in apply mode' >&2
	exit 1
}
[[ -d "$(dirname "$backup_path")" && ! -e "$backup_path" ]] || {
	echo 'PET_CUSTODY_REPAIR_BACKUP parent must exist and the backup path must be unused' >&2
	exit 1
}

umask 077
{
	printf '%s\n' '-- Duris stale pet custody backup v1'
	"${MYSQLDUMP[@]}" --no-create-info --skip-triggers --compact "$DB_NAME" player_pets \
		--where="id=$pet_id AND owner_pid=$owner_pid AND pet_uid=$pet_uid AND hold_reason=6 AND room_vnum=$room_before"
} > "$backup_path"
[[ -s "$backup_path" ]] || {
	echo 'pet custody backup was not created; refusing repair' >&2
	exit 1
}

changed=$("${MYSQL[@]}" "$DB_NAME" -e "
START TRANSACTION;
UPDATE player_pets pp JOIN player_data pd ON pd.pid=pp.owner_pid
SET pp.room_vnum=pd.last_room
WHERE pp.id=$pet_id AND pp.owner_pid=$owner_pid AND pp.pet_uid=$pet_uid
  AND pp.hold_reason=6 AND pp.room_vnum=$room_before
  AND pd.last_room=$owner_room
  AND pp.room_vnum<>pd.last_room;
SELECT ROW_COUNT();
COMMIT;")
[[ "$changed" == '1' ]] || {
	echo "pet custody repair changed $changed rows; preserve and inspect $backup_path" >&2
	exit 1
}

remaining=$("${MYSQL[@]}" "$DB_NAME" -e "
SELECT COUNT(*) FROM player_pets pp JOIN player_data pd ON pd.pid=pp.owner_pid
WHERE pp.id=$pet_id AND pp.owner_pid=$owner_pid AND pp.pet_uid=$pet_uid
  AND pp.hold_reason=6 AND pp.room_vnum<>pd.last_room;")
[[ "$remaining" == '0' ]] || {
	echo "pet custody repair verification failed; preserve and inspect $backup_path" >&2
	exit 1
}
printf 'repair_applied=1 room_after=%s backup=%s\n' "$owner_room" "$backup_path"
