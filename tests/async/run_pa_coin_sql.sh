#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"

# Slot 0 owns this bounded disposable SQL fixture. The shared heavy lock is a
# read lease; validate configured lock files rather than creating new lock names.
RESOURCE_ROOT="${DURIS_ACCOUNTING_RESOURCE_ROOT:-}"
HEAVY_LOCK="${DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK:-}"
if [[ -z "$RESOURCE_ROOT" || "$RESOURCE_ROOT" != /* || -z "$HEAVY_LOCK" || "$HEAVY_LOCK" != /* ]]; then
    printf 'Set DURIS_ACCOUNTING_RESOURCE_ROOT and DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK to the assigned absolute lock paths.\n' >&2
    exit 2
fi
for lock in "$RESOURCE_ROOT/test-slot-0.lock" "$HEAVY_LOCK"; do
    if [[ ! -f "$lock" || -L "$lock" ]]; then
        printf 'Required assigned resource lock is absent or unsafe: %s\n' "$lock" >&2
        exit 2
    fi
done
python3 "$ROOT/tests/async/pa_accounting_batch_artifact.py" --component S07-coin

exec 9>>"$RESOURCE_ROOT/test-slot-0.lock"
flock -w 900 -x 9
exec 8>>"$HEAVY_LOCK"
flock -w 900 -s 8
export DURIS_PARALLEL_DB_TEST_SLOT=0

NAME="duris-pa-coin-$$-${RANDOM}"
PASSWORD="pa-coin-$$-${RANDOM}"
IMAGE="${DURIS_TEST_DB_IMAGE:-mariadb:10.11}"
TMPDIR="$(mktemp -d -t pa-coin-sql.XXXXXX)"
cleanup() {
    local status=$?
    if [[ -n "${NAME:-}" ]]; then
        docker logs "$NAME" 2>&1 | PASSWORD="$PASSWORD" python3 -c \
            'import os,sys; print(sys.stdin.read().replace(os.environ["PASSWORD"], "<fixture-password>"), end="")' || true
        docker rm -fv "$NAME" >/dev/null 2>&1 || true
        if docker container inspect "$NAME" >/dev/null 2>&1; then
            printf 'Disposable SQL container cleanup failed: %s\n' "$NAME" >&2
            status=1
        else
            printf 'Disposable SQL container absent after cleanup.\n'
        fi
    fi
    rm -rf "$TMPDIR"
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# Pass the generated fixture password in the environment, not in Docker's argv.
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
export "$PASSWORD_ENV=$PASSWORD"
source "$ROOT/tests/async/_sql_fixture_network.sh"
sql_fixture_network
docker run -d --name "$NAME" --cpus=2 --memory=2g \
    "${SQL_FIXTURE_NETWORK[@]}" -e "$PASSWORD_ENV" "$IMAGE" "${SQL_FIXTURE_SERVER[@]}" >/dev/null
unset "$PASSWORD_ENV"
mapping="$(sql_fixture_mapping "$NAME")"
published_port="${mapping##*:}"
container_host="$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$NAME")"
export ENVIRONMENT=test DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME=pa_coin_sql_test CURRENCY_TEST_DB_NAME=pa_coin_sql_test
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
ready=0
CANDIDATES=("127.0.0.1:$published_port" "host.docker.internal:$published_port" "$container_host:3306")
if [[ -n "$SQL_FIXTURE_PRIVATE_PORT" ]]; then
    CANDIDATES=("127.0.0.1:$published_port")
fi
for candidate in "${CANDIDATES[@]}"; do
    [[ "$candidate" == :3306 ]] && continue
    export DB_HOST="${candidate%:*}" DB_PORT="${candidate##*:}"
    MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp --connect-timeout=3 -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
    deadline=$((SECONDS + 90))
    while ((SECONDS < deadline)); do
        if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break 2; fi
        sleep 1
    done
done
if [[ "$ready" != 1 ]]; then
    printf 'Disposable coin SQL readiness failed on bounded loopback/bridge endpoints.\n' >&2
    "${MYSQL[@]}" --connect-timeout=3 -e 'SELECT 1' >/dev/null
    exit 1
fi

"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
"${MYSQL[@]}" "$DB_NAME" < migrations/critical_command_inbox_outbox.sql
for migration in migrations/immutable/*.sql; do
    "${MYSQL[@]}" "$DB_NAME" < "$migration"
done
"${MYSQL[@]}" "$DB_NAME" < migrations/currency_ledger.sql

SQL_DISPATCH_SOURCES_TEXT="$(python3 tests/async/_sql_dispatch_sources.py)"
read -r -a SQL_DISPATCH_SOURCES <<< "$SQL_DISPATCH_SOURCES_TEXT"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
read -r -a CXX_CMD <<< "${CXX:-g++}"
"${CXX_CMD[@]}" -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread \
    "${SQL_DISPATCH_SOURCES[@]}" \
    -ffunction-sections -fdata-sections -Wl,--gc-sections -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/pa_coin_sql_harness.cpp \
    src/persistence/critical_command.c src/economy/currency_command.c src/world/epic_command.c \
    src/combat/combat_outcome_command.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o "$TMPDIR/pa_coin_sql_harness"
"$TMPDIR/pa_coin_sql_harness"
