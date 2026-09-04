#!/usr/bin/env bash
# Rehearse account-locker conversion checks against an isolated MySQL database.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
container_name="duris-account-locker-check-$RANDOM-$$"
database_name="duris_account_locker_check"
password="duris-account-locker-check-$RANDOM-$$"
image="${ACCOUNT_LOCKER_CHECK_DB_IMAGE:-mysql:8.0}"

# Remove only the uniquely named disposable database container.
cleanup() {
	docker rm -f "$container_name" >/dev/null 2>&1 || true
}
trap cleanup EXIT HUP INT TERM

docker run --rm -d --name "$container_name" \
	-e MYSQL_ROOT_PASSWORD="$password" "$image" >/dev/null
ready=0
for _ in $(seq 1 90); do
	if docker exec -e MYSQL_PWD="$password" "$container_name" \
		mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
		ready=1
		break
	fi
	sleep 1
done
[[ "$ready" == 1 ]]

MYSQL=(docker exec -i -e MYSQL_PWD="$password" "$container_name" \
	mysql -h127.0.0.1 -uroot -N -B)
"${MYSQL[@]}" <<SQL
CREATE DATABASE $database_name CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $database_name;
CREATE TABLE accounts (
  account_name VARCHAR(50) NOT NULL PRIMARY KEY
) ENGINE=InnoDB;
CREATE TABLE lockers (
  id INT UNSIGNED NOT NULL PRIMARY KEY,
  locker_name VARCHAR(100) NOT NULL UNIQUE,
  owner_pid INT DEFAULT NULL,
  owner_assoc_id INT DEFAULT NULL,
  racewar TINYINT NOT NULL DEFAULT 0
) ENGINE=InnoDB;
CREATE TABLE private_chests (
  id INT UNSIGNED NOT NULL PRIMARY KEY,
  locker_id INT UNSIGNED NOT NULL,
  chest_name VARCHAR(32) NOT NULL,
  password_hash VARCHAR(64) DEFAULT NULL,
  is_public TINYINT(1) DEFAULT 0,
  sort_config TEXT
) ENGINE=InnoDB;
CREATE TABLE locker_items (
  id INT UNSIGNED NOT NULL PRIMARY KEY,
  locker_id INT UNSIGNED NOT NULL,
  chest_id INT UNSIGNED DEFAULT NULL,
  vnum INT NOT NULL,
  obj_uid BIGINT UNSIGNED DEFAULT NULL
) ENGINE=InnoDB;
CREATE TABLE item_current_owner (
  item_uid BIGINT UNSIGNED NOT NULL PRIMARY KEY,
  owner_type TINYINT UNSIGNED NOT NULL,
  owner_id BIGINT UNSIGNED NOT NULL,
  owner_context_id BIGINT UNSIGNED NOT NULL DEFAULT 0,
  vnum INT NOT NULL DEFAULT 0,
  state TINYINT UNSIGNED NOT NULL DEFAULT 1
) ENGINE=InnoDB;
CREATE TABLE locker_access (
  owner VARCHAR(255) NOT NULL,
  visitor VARCHAR(255) NOT NULL,
  PRIMARY KEY(owner,visitor)
) ENGINE=InnoDB;
INSERT INTO accounts(account_name) VALUES ('fixture');
INSERT INTO lockers(id,locker_name,racewar)
VALUES (10,'account.fixture.1.locker',1);
INSERT INTO private_chests(id,locker_id,chest_name,is_public)
VALUES (11,10,'public',1);
INSERT INTO locker_items(id,locker_id,chest_id,vnum,obj_uid)
VALUES (12,10,11,3900,900);
SQL

docker cp "$ROOT/migrations/check_flatfile_account_locker_conversion.sh" \
	"$container_name:/tmp/check_flatfile_account_locker_conversion.sh" >/dev/null
docker exec "$container_name" chmod +x /tmp/check_flatfile_account_locker_conversion.sh
CHECK=(docker exec \
	-e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
	-e DB_PASSWD="$password" -e DB_NAME="$database_name" \
	"$container_name" /tmp/check_flatfile_account_locker_conversion.sh --expect-count 1)

set +e
missing_output=$("${CHECK[@]}" 2>&1)
missing_status=$?
set -e
[[ "$missing_status" == 1 && "$missing_output" == *'invalid_item_shape=1'* ]]

"${MYSQL[@]}" "$database_name" -e "
INSERT INTO item_current_owner(item_uid,owner_type,owner_id,owner_context_id,vnum,state)
VALUES (900,5,10,11,3900,2);"
set +e
inactive_output=$("${CHECK[@]}" 2>&1)
inactive_status=$?
set -e
[[ "$inactive_status" == 1 && "$inactive_output" == *'invalid_item_shape=1'* ]]

"${MYSQL[@]}" "$database_name" -e \
	'UPDATE item_current_owner SET state=1,vnum=3901 WHERE item_uid=900;'
set +e
mismatch_output=$("${CHECK[@]}" 2>&1)
mismatch_status=$?
set -e
[[ "$mismatch_status" == 1 && "$mismatch_output" == *'invalid_item_shape=1'* ]]

"${MYSQL[@]}" "$database_name" -e \
	'UPDATE item_current_owner SET vnum=3900 WHERE item_uid=900;'
passing_output=$("${CHECK[@]}")
[[ "$passing_output" == *'account_lockers=1'* &&
	"$passing_output" == *'invalid_item_shape=0'* ]]

echo 'account locker conversion check: missing, inactive, and mismatched custody rejected'
