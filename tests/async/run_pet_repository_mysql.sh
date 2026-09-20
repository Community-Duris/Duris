#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../.."
cd "$ROOT"
NAME="duris-pet-custody-${RANDOM}-$$"
PASSWORD="pet-custody-${RANDOM}-$$"
IMAGE="${PET_CUSTODY_DB_IMAGE:-mariadb:11.4}"
cleanup() { docker rm -f -v "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -p 127.0.0.1::3306 -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export TEST_DB_HOST=127.0.0.1 TEST_DB_PORT="${mapping##*:}"
export TEST_DB_USER=root TEST_DB_PASSWORD="$PASSWORD" TEST_DB_NAME=pet_state_test
export MYSQL_PWD="$PASSWORD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$TEST_DB_HOST" -P "$TEST_DB_PORT" -u "$TEST_DB_USER")
ready=0
for _ in $(seq 1 90); do
	if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
	sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $TEST_DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$TEST_DB_NAME" < migrations/bootstrap_multithread_safe.sql
for migration in migrations/immutable/*.sql; do
	"${MYSQL[@]}" "$TEST_DB_NAME" < "$migration"
done
mkdir -p bin/tests
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -ffunction-sections -fdata-sections -Isrc \
	"${MYSQL_CFLAGS[@]}" tests/async/pet_repository_mysql_harness.cpp \
	src/player/player_snapshot_repository.c src/player/player_load_repository.c \
	src/player/player_load_topology.c src/player/player_snapshot_codec.c \
	src/player/pet_restore_state.c src/persistence/persistence_observability.c \
	src/persistence/player_death_restitution_command.c src/sql/item_extra_descr_codec.c \
	src/sql/sql_player.c src/sql/sql_pool.c \
	-Wl,--gc-sections "${MYSQL_LIBS[@]}" -lcrypto -o bin/tests/pet_repository_mysql_harness
TEST_DB_HOST="$TEST_DB_HOST" TEST_DB_PORT="$TEST_DB_PORT" \
TEST_DB_USER="$TEST_DB_USER" TEST_DB_PASSWORD="$TEST_DB_PASSWORD" \
TEST_DB_NAME="$TEST_DB_NAME" bin/tests/pet_repository_mysql_harness
