#!/usr/bin/env bash
# Disposable real-FK proof: historical affect uniqueness must not collapse snapshots.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
[[ ! -e "$ROOT/.env" && ! -e "$ROOT/migrations/.env" ]]
NAME="duris-accounting-schema-affect-index-$$-$RANDOM"
PASSWORD="affect-index-$$-$RANDOM"
IMAGE="${AFFECT_INDEX_DB_IMAGE:-mysql:8.0}"
[[ "$IMAGE" =~ ^(mysql:8\.0(\.[0-9]+)?|mariadb:10\.11(\.[0-9]+)?)(@sha256:[0-9a-f]{64})?$ ]] || { echo 'unsupported test image' >&2; exit 2; }
SQL_FIXTURE_CONTAINER_ID=
cleanup() { [[ "${SQL_FIXTURE_CONTAINER_ID:-}" =~ ^[0-9a-f]{64}$ ]] && docker rm -f "$SQL_FIXTURE_CONTAINER_ID" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then ROOT_ENV=MARIADB_ROOT_PASSWORD; else ROOT_ENV=MYSQL_ROOT_PASSWORD; fi
SQL_FIXTURE_CONTAINER_ID=$(docker run -d --name "$NAME" -e "$ROOT_ENV=$PASSWORD" "$IMAGE" --innodb-use-native-aio=OFF)
# TCP readiness excludes MySQL's temporary, socket-only initialization server.
mysql_clone() {
    docker exec -i "$NAME" sh -c 'MYSQL_PWD="${MYSQL_ROOT_PASSWORD:-$MARIADB_ROOT_PASSWORD}" exec mysql --protocol=tcp -h127.0.0.1 -u root -N -B "$@"' sh "$@"
}
ready=0
for _ in $(seq 1 90); do
    if mysql_clone -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo 'disposable affect-index database not ready' >&2; exit 1; }
printf 'affect-index fixture: authenticated disposable database ready\n'
create_db() {
    mysql_clone -e "CREATE DATABASE $1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
    mysql_clone "$1" < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
}
key_rows() {
    mysql_clone "$1" -e "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_affects' AND index_name='uk_pid_type_dur_flags_mod_loc_lvl'"
}
fixture() {
    mysql_clone "$1" -e "INSERT INTO accounts(account_name,password) VALUES('affect-probe','fixture-only'); INSERT INTO player_data(pid,name,account_name,racewar,level,race,last_room) VALUES(9101,'AffectProbe','affect-probe',1,25,1,100); INSERT INTO player_affects(pid,type,duration,flags,modifier,location,level,bitvector1,custom_msg_char) VALUES(9101,12,4,0,2,3,6,17,'first');" >/dev/null
}
second_row() {
    mysql_clone "$1" -e "INSERT INTO player_affects(pid,type,duration,flags,modifier,location,level,bitvector1,custom_msg_char) VALUES(9101,12,4,0,2,3,6,99,'second');" >/dev/null
}
rows() {
    mysql_clone "$1" -e "SELECT COUNT(*),SUM(bitvector1=17 AND custom_msg_char='first'),SUM(bitvector1=99 AND custom_msg_char='second') FROM player_affects WHERE pid=9101"
}
DB=affect_index_legacy
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE player_affects ADD UNIQUE KEY uk_pid_type_dur_flags_mod_loc_lvl (pid,type,duration,flags,modifier,location,level)' >/dev/null
fixture "$DB"
printf 'affect-index fixture: keyed source and parent/child row seeded\n'
[[ "$(key_rows "$DB")" == 7 ]]
if second_row "$DB" 2>/dev/null; then echo 'FAILED: legacy index did not reject the duplicate signature' >&2; exit 1; fi
[[ "$(rows "$DB")" == $'1\t1\t0' ]]
printf 'affect-index fixture: historical key rejected second payload as expected\n'
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null
[[ "$(key_rows "$DB")" == 0 ]] || { echo 'FAILED: historical affect index remains after reconciliation' >&2; exit 1; }
second_row "$DB"
[[ "$(rows "$DB")" == $'2\t1\t1' ]] || { echo 'FAILED: distinct affect payloads were lost' >&2; exit 1; }
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null
[[ "$(rows "$DB")" == $'2\t1\t1' ]]
[[ "$(mysql_clone "$DB" -e "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name='player_affects' AND constraint_name='fk_player_affects' AND referenced_table_name='player_data'")" == 1 ]]
[[ "$(mysql_clone "$DB" -e "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_affects' AND index_name='idx_pid'")" == 1 ]]
printf 'affect-index fixture: keyed refusal, retained FK, two payloads, replay PASS\n'

DB=affect_index_bootstrap
create_db "$DB"
fixture "$DB"
second_row "$DB"
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null
[[ "$(rows "$DB")" == $'2\t1\t1' ]]
printf 'affect-index fixture: fresh canonical schema and rows retained PASS\n'

DB=affect_index_wrong
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE player_affects ADD KEY uk_pid_type_dur_flags_mod_loc_lvl (pid,type)' >/dev/null
fixture "$DB"
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null 2>&1; then echo 'FAILED: wrong same-named index accepted' >&2; exit 1; fi
[[ "$(key_rows "$DB")" == 2 && "$(rows "$DB")" == $'1\t1\t0' ]]
printf 'affect-index fixture: wrong same-named index refused before mutation PASS\n'

DB=affect_index_no_support
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE player_affects ADD UNIQUE KEY uk_pid_type_dur_flags_mod_loc_lvl (pid,type,duration,flags,modifier,location,level), DROP INDEX idx_pid' >/dev/null
fixture "$DB"
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null 2>&1; then echo 'FAILED: missing independent FK support accepted' >&2; exit 1; fi
[[ "$(key_rows "$DB")" == 7 && "$(rows "$DB")" == $'1\t1\t0' ]]
printf 'affect-index fixture: missing FK support refused before mutation PASS\n'

DB=affect_index_inbound_fk
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE player_affects ADD UNIQUE KEY uk_pid_type_dur_flags_mod_loc_lvl (pid,type,duration,flags,modifier,location,level)' >/dev/null
fixture "$DB"
mysql_clone "$DB" -e "CREATE TABLE affect_reference (id INT UNSIGNED PRIMARY KEY, affect_id INT UNSIGNED NOT NULL, CONSTRAINT fk_affect_reference FOREIGN KEY (affect_id) REFERENCES player_affects(id)) ENGINE=InnoDB; INSERT INTO affect_reference(id,affect_id) SELECT 1,id FROM player_affects WHERE pid=9101;" >/dev/null
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_affect_index.sql" >/dev/null 2>&1; then echo 'FAILED: inbound FK was accepted' >&2; exit 1; fi
[[ "$(key_rows "$DB")" == 7 && "$(rows "$DB")" == $'1\t1\t0' ]]
[[ "$(mysql_clone "$DB" -e 'SELECT COUNT(*) FROM affect_reference WHERE id=1')" == 1 ]]
printf 'affect-index fixture: inbound FK refused with parent and child intact PASS\n'
