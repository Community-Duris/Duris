#!/usr/bin/env bash
# Rehearse issue #122 against an isolated production-shaped MySQL database.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
container_name="duris-locker-repair-$RANDOM-$$"
database_name="duris_locker_repair_test"
password="duris-locker-repair-$RANDOM-$$"
image="${LOCKER_REPAIR_DB_IMAGE:-mysql:8.0}"
temporary_root="$(mktemp -d)"
config="$temporary_root/test.env"

# Remove only the uniquely named disposable container and temporary directory.
cleanup() {
	docker rm -f "$container_name" >/dev/null 2>&1 || true
	rm -rf "$temporary_root"
}
trap cleanup EXIT HUP INT TERM

docker run --rm -d --name "$container_name" -p 127.0.0.1::3306 \
	-e MYSQL_ROOT_PASSWORD="$password" "$image" >/dev/null
mapping="$(docker port "$container_name" 3306/tcp)"
db_port="${mapping##*:}"

ready=0
for _ in $(seq 1 90); do
	if MYSQL_PWD="$password" mysql --skip-ssl -h127.0.0.1 -P"$db_port" \
		-uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
		ready=1
		break
	fi
	sleep 1
done
[[ "$ready" == 1 ]]

MYSQL=(mysql --skip-ssl -h127.0.0.1 -P"$db_port" -uroot -N -B)
MYSQL_PWD="$password" "${MYSQL[@]}" -e "CREATE DATABASE $database_name;"
MYSQL_PWD="$password" "${MYSQL[@]}" "$database_name" <<'SQL'
CREATE TABLE account_characters (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  account_name VARCHAR(50) NOT NULL,
  char_name VARCHAR(64) NOT NULL,
  pid INT UNSIGNED DEFAULT NULL,
  blocked TINYINT NOT NULL DEFAULT 0,
  racewar TINYINT NOT NULL DEFAULT 0,
  deleted_at DATETIME DEFAULT NULL
) ENGINE=InnoDB;
CREATE TABLE lockers (
  id INT UNSIGNED NOT NULL PRIMARY KEY,
  locker_name VARCHAR(100) NOT NULL UNIQUE,
  owner_pid INT DEFAULT NULL,
  owner_assoc_id INT DEFAULT NULL,
  racewar TINYINT NOT NULL DEFAULT 0
) ENGINE=InnoDB;
CREATE TABLE locker_items (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  locker_id INT UNSIGNED NOT NULL,
  vnum INT NOT NULL
) ENGINE=InnoDB;
CREATE TABLE locker_access (
  owner VARCHAR(255) NOT NULL,
  visitor VARCHAR(255) NOT NULL,
  PRIMARY KEY(owner,visitor)
) ENGINE=InnoDB;
INSERT INTO account_characters(account_name,char_name,pid,blocked,racewar,deleted_at)
VALUES ('legacy_account','Legacyowner',424242,0,1,NULL);
INSERT INTO lockers(id,locker_name,owner_pid,owner_assoc_id,racewar)
VALUES (7,'Legacyowner.locker',424242,NULL,1);
INSERT INTO locker_items(locker_id,vnum) VALUES
  (7,1001),(7,1002),(7,1003),(7,1004),(7,1005),(7,1006),(7,1007),
  (7,1008),(7,1009),(7,1010),(7,1011),(7,1012),(7,1013),(7,1014),
  (7,1015),(7,1016),(7,1017),(7,1018),(7,1019),(7,1020),(7,1021);
SQL

: > "$config"
chmod 600 "$config"
printf '%s\n' \
	'ENVIRONMENT=test' \
	'DB_HOST=127.0.0.1' \
	"DB_PORT=$db_port" \
	'DB_USER=root' \
	"DB_PASSWD=$password" \
	"DB_NAME=$database_name" > "$config"

repair=("$ROOT/migrations/repair_legacy_personal_locker_access.sh")
common_env=(
	"MIGRATION_ENV_FILE=$config"
	'LOCKER_REPAIR_NAME=Legacyowner.locker'
	'LOCKER_REPAIR_OWNER_PID=424242'
	'LOCKER_REPAIR_EXPECTED_ITEMS=21'
)

env "${common_env[@]}" "${repair[@]}" --check |
	grep -q 'candidate_count=1 item_count=21 owner_path_ready=1 visitor_grant_ready=0'
if env "${common_env[@]}" LOCKER_REPAIR_EXPECTED_ITEMS=20 "${repair[@]}" --check \
	>/dev/null 2>&1; then
	echo 'repair accepted the wrong payload count' >&2
	exit 1
fi

before=$(MYSQL_PWD="$password" "${MYSQL[@]}" "$database_name" -e \
	'SELECT CONCAT(COUNT(*),":",SUM(vnum)) FROM locker_items WHERE locker_id=7;')
env "${common_env[@]}" LOCKER_REPAIR_BACKUP="$temporary_root/access-before.sql" \
	"${repair[@]}" --apply >/dev/null
[[ -s "$temporary_root/access-before.sql" ]]

after=$(MYSQL_PWD="$password" "${MYSQL[@]}" "$database_name" -e \
	'SELECT CONCAT(COUNT(*),":",SUM(vnum)) FROM locker_items WHERE locker_id=7;')
grant=$(MYSQL_PWD="$password" "${MYSQL[@]}" "$database_name" -e \
	"SELECT COUNT(*) FROM locker_access WHERE owner='Legacyowner.locker' AND visitor='legacy_account';")
[[ "$before" == '21:21231' && "$after" == "$before" && "$grant" == 1 ]]

env "${common_env[@]}" LOCKER_REPAIR_BACKUP="$temporary_root/access-before-second.sql" \
	"${repair[@]}" --apply >/dev/null
grant=$(MYSQL_PWD="$password" "${MYSQL[@]}" "$database_name" -e \
	"SELECT COUNT(*) FROM locker_access WHERE owner='Legacyowner.locker' AND visitor='legacy_account';")
[[ "$grant" == 1 && -s "$temporary_root/access-before-second.sql" ]]

echo 'legacy personal locker repair rehearsal: 21 items preserved and grant idempotent'
