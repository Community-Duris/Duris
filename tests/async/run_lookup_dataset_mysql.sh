#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-lookup-dataset-$$"
PASSWORD=$(printf 'lookup-dataset-%s-%s' "$$" "$RANDOM")
DB_NAME="lookup_dataset_test"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
docker run --rm -d --name "$NAME" -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
for _ in $(seq 1 60); do docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1 && break; sleep 1; done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
"${MYSQL[@]}" -e "CREATE TABLE races(id INT UNSIGNED PRIMARY KEY,name VARCHAR(64) NOT NULL,short_name VARCHAR(32),ansi_name VARCHAR(128),abbrev VARCHAR(8),racewar INT,playable TINYINT) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci; CREATE TABLE classes(id INT UNSIGNED PRIMARY KEY,name VARCHAR(64) NOT NULL,ansi_name VARCHAR(128),short_name VARCHAR(8),menu_char CHAR(1)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;"
docker cp "$ROOT/migrations/immutable/0001_lookup_dataset_state.sql" "$NAME:/tmp/lookup.sql" >/dev/null
docker cp "$ROOT/migrations/immutable/0001_lookup_dataset_state.sh" "$NAME:/tmp/verify.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/verify.sh
for _ in 1 2; do docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/lookup.sql"; done
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/verify.sh >/dev/null
"${MYSQL[@]}" -e "INSERT INTO races VALUES(1,'old','old','old','o',1,1); INSERT INTO classes VALUES(1,'old','old','old','o'); INSERT INTO lookup_dataset_state VALUES('race_class',1,UNHEX(REPEAT('01',32)),1,1,CURRENT_TIMESTAMP(6));"
if "${MYSQL[@]}" -e "START TRANSACTION; UPDATE races SET name='partial' WHERE id=1; INSERT INTO classes VALUES(1,'duplicate','x','x','x'); COMMIT;" >/dev/null 2>&1; then echo 'synthetic lookup failure unexpectedly committed' >&2; exit 1; fi
[[ $("${MYSQL[@]}" -e "SELECT name FROM races WHERE id=1;") == old ]]
"${MYSQL[@]}" -e "START TRANSACTION; INSERT INTO races VALUES(1,'new','new','new','n',1,1) ON DUPLICATE KEY UPDATE name=VALUES(name),short_name=VALUES(short_name),ansi_name=VALUES(ansi_name),abbrev=VALUES(abbrev),racewar=VALUES(racewar),playable=VALUES(playable); INSERT INTO classes VALUES(1,'new','new','new','n') ON DUPLICATE KEY UPDATE name=VALUES(name),ansi_name=VALUES(ansi_name),short_name=VALUES(short_name),menu_char=VALUES(menu_char); UPDATE lookup_dataset_state SET dataset_version=2,dataset_checksum=UNHEX(REPEAT('02',32)),race_count=1,class_count=1 WHERE dataset_name='race_class'; COMMIT;"
[[ $("${MYSQL[@]}" -e "SELECT CONCAT(r.name,':',c.name,':',s.dataset_version,':',LOWER(HEX(s.dataset_checksum))) FROM races r JOIN classes c ON c.id=r.id JOIN lookup_dataset_state s ON s.dataset_name='race_class';") == "new:new:2:$(printf '02%.0s' {1..32})" ]]
printf 'lookup dataset schema replay, rollback, and atomic state publication: ok\n'
