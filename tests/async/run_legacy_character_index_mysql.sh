#!/usr/bin/env bash
# Disposable account-character index transition; no player data or live DB.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
[[ ! -e "$ROOT/.env" && ! -e "$ROOT/migrations/.env" ]]
NAME="duris-accounting-schema-character-index-$$-$RANDOM"
PASSWORD="character-index-$$-$RANDOM"
IMAGE="${CHARACTER_INDEX_DB_IMAGE:-mysql:8.0}"
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
[[ "$ready" == 1 ]] || { echo 'disposable character-index DB not ready' >&2; exit 1; }
create_db() {
    mysql_clone -e "CREATE DATABASE $1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" >/dev/null
    mysql_clone "$1" < "$ROOT/migrations/bootstrap_multithread_safe.sql" >/dev/null
}
index_rows() {
    mysql_clone "$1" -e "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_characters' AND index_name='acct_char'"
}
fixture() {
    mysql_clone "$1" -e "INSERT INTO account_characters(account_name,pid,char_name) VALUES('account-probe',9101,'AProbe');" >/dev/null
}
DB=character_index_legacy
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE account_characters ADD UNIQUE KEY acct_char (account_name,char_name)' >/dev/null
fixture "$DB"
[[ "$(index_rows "$DB")" == 2 ]]
if mysql_clone "$DB" -e "INSERT INTO account_characters(account_name,pid,char_name) VALUES('other-account',9102,'AProbe')" >/dev/null 2>&1; then echo 'FAILED: canonical unique-name key did not protect character identity' >&2; exit 1; fi
printf 'character-index fixture: stronger canonical UNIQUE(char_name) protects identity\n'
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null
[[ "$(index_rows "$DB")" == 0 ]] || { echo 'FAILED: legacy account-character key remained' >&2; exit 1; }
mysql_clone "$DB" -e "INSERT INTO account_characters(account_name,pid,char_name) VALUES('account-probe',9103,'BProbe');" >/dev/null
[[ "$(mysql_clone "$DB" -e "SELECT COUNT(*),COUNT(DISTINCT char_name) FROM account_characters WHERE pid IN (9101,9103)")" == $'2\t2' ]]
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null
[[ "$(index_rows "$DB")" == 0 ]]
printf 'character-index fixture: row and unique-name preservation, replay PASS\n'

DB=character_index_bootstrap
create_db "$DB"
fixture "$DB"
mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null
[[ "$(index_rows "$DB")" == 0 && "$(mysql_clone "$DB" -e 'SELECT COUNT(*) FROM account_characters WHERE pid=9101')" == 1 ]]
printf 'character-index fixture: canonical fresh schema PASS\n'

DB=character_index_wrong
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE account_characters ADD KEY acct_char (char_name)' >/dev/null
fixture "$DB"
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null 2>&1; then echo 'FAILED: wrong same-named legacy key accepted' >&2; exit 1; fi
[[ "$(index_rows "$DB")" == 1 && "$(mysql_clone "$DB" -e 'SELECT COUNT(*) FROM account_characters WHERE pid=9101')" == 1 ]]
printf 'character-index fixture: wrong same-named key refused before mutation PASS\n'

DB=character_index_no_unique
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE account_characters ADD UNIQUE KEY acct_char (account_name,char_name), DROP INDEX idx_char_name_unique' >/dev/null
fixture "$DB"
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null 2>&1; then echo 'FAILED: missing canonical unique-name key accepted' >&2; exit 1; fi
[[ "$(index_rows "$DB")" == 2 && "$(mysql_clone "$DB" -e 'SELECT COUNT(*) FROM account_characters WHERE pid=9101')" == 1 ]]
printf 'character-index fixture: missing stronger uniqueness refused before mutation PASS\n'

DB=character_index_inbound_fk
create_db "$DB"
mysql_clone "$DB" -e 'ALTER TABLE account_characters ADD UNIQUE KEY acct_char (account_name,char_name)' >/dev/null
fixture "$DB"
mysql_clone "$DB" -e "CREATE TABLE character_reference (id INT UNSIGNED PRIMARY KEY, mapping_id INT NOT NULL, CONSTRAINT fk_character_reference FOREIGN KEY (mapping_id) REFERENCES account_characters(id)) ENGINE=InnoDB; INSERT INTO character_reference(id,mapping_id) SELECT 1,id FROM account_characters WHERE pid=9101;" >/dev/null
if mysql_clone "$DB" < "$ROOT/migrations/legacy_archive_character_index.sql" >/dev/null 2>&1; then echo 'FAILED: inbound FK was accepted' >&2; exit 1; fi
[[ "$(index_rows "$DB")" == 2 && "$(mysql_clone "$DB" -e 'SELECT COUNT(*) FROM character_reference WHERE id=1')" == 1 ]]
printf 'character-index fixture: inbound FK refused, parent and child intact PASS\n'
