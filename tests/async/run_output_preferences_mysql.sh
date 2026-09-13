#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Only a new disposable database; never source the checkout's .env.
NAME="duris-color-$$-$RANDOM"
PASSWORD="color-$$-$RANDOM"
IMAGE="${COLOR_DB_IMAGE:-mariadb:11.4}"
cleanup() { docker rm -f -v "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -p 127.0.0.1::3306 -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export DB_PORT="${mapping##*:}" DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_USER=root DB_NAME=colorization_test
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER")
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
for migration in migrations/immutable/*.sql; do "${MYSQL[@]}" "$DB_NAME" < "$migration"; done
mkdir -p bin/tests
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -ffunction-sections -fdata-sections -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/output_preferences_mysql_harness.cpp \
    src/player/player_snapshot_repository.c src/player/player_load_repository.c \
    src/player/player_load_topology.c src/player/player_snapshot_codec.c \
    src/persistence/persistence_observability.c -Wl,--gc-sections "${MYSQL_LIBS[@]}" \
    -o bin/tests/output_preferences_mysql_harness
bin/tests/output_preferences_mysql_harness