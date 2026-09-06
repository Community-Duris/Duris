#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Synthetic data only, in a disposable server. Never source the checkout's .env:
# it may point at the live game.
NAME="duris-death-disposition-$$-$RANDOM"
PASSWORD="death-$$-$RANDOM"
IMAGE="${DEATH_DISPOSITION_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
docker run --rm -d --name "$NAME" -p 127.0.0.1::3306 -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=death_disposition_test
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/bootstrap_multithread_safe.sql"
for migration in "$ROOT"/migrations/immutable/*.sql; do
    "${MYSQL[@]}" "$DB_NAME" < "$migration"
done
# Exercise the upgrade from a pre-death schema, then verify rerunning it is safe.
"${MYSQL[@]}" "$DB_NAME" -e 'DROP TABLE player_death_custody, player_death_disposition'
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0011_player_death_disposition.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0011_player_death_disposition.sql"
"$ROOT/migrations/immutable/0011_player_death_disposition.sh"
# CREATE IF NOT EXISTS must not bless damaged existing tables.
for damage in \
    'ALTER TABLE player_death_disposition MODIFY wallet_copper BIGINT NOT NULL DEFAULT 0' \
    'ALTER TABLE player_death_disposition MODIFY wallet_copper INT NULL DEFAULT 0' \
    'ALTER TABLE player_death_disposition ADD unexpected INT' \
    'ALTER TABLE player_death_disposition DROP INDEX idx_player_death_disposition_operation' \
    'ALTER TABLE player_death_disposition DROP PRIMARY KEY' \
    'ALTER TABLE player_death_custody DROP INDEX idx_player_death_custody_item' \
    'ALTER TABLE player_death_custody ENGINE=MyISAM'; do
    "${MYSQL[@]}" "$DB_NAME" -e "$damage"
    "${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0011_player_death_disposition.sql"
    if "$ROOT/migrations/immutable/0011_player_death_disposition.sh" >/dev/null 2>&1; then
        echo "Damaged death schema passed verification: $damage" >&2
        exit 1
    fi
    "${MYSQL[@]}" "$DB_NAME" -e 'DROP TABLE player_death_custody, player_death_disposition'
    "${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0011_player_death_disposition.sql"
done
"$ROOT/migrations/immutable/0011_player_death_disposition.sh"
"${MYSQL[@]}" "$DB_NAME" -e "INSERT INTO accounts(account_name) VALUES ('death_probe'); INSERT INTO player_data(pid,name,account_name) VALUES (1,'Probe','death_probe');"

mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/player_death_disposition_mysql_harness.cpp \
    src/player/player_snapshot_repository.c src/player/player_snapshot_codec.c \
    src/persistence/persistence_observability.c \
    "${MYSQL_LIBS[@]}" -o "$ROOT/bin/tests/player_death_disposition_mysql_harness"
"$ROOT/bin/tests/player_death_disposition_mysql_harness"
