#!/usr/bin/env bash
# Disposable auction transactional cutover fixture. Never reads .env or an existing game DB.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${AUCTION_TRANSACTION_DB_IMAGE:-mariadb:10.11}"
NAME="duris-auction-test-$$-$RANDOM"
PASSWORD="auction-test-$$-$RANDOM"
DATABASE=auction_schema_test_$RANDOM

DOCKER="docker"
if ! docker info >/dev/null 2>&1 && which docker.exe >/dev/null 2>&1; then
    DOCKER="docker.exe"
fi

cleanup() { "$DOCKER" rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM

if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi

"$DOCKER" run -d --name "$NAME" -p 127.0.0.1::3306 \
    -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$("$DOCKER" port "$NAME" 3306/tcp)"
port="${mapping##*:}"
export ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1
export ENVIRONMENT=test
export DB_HOST=127.0.0.1 DB_PORT="$port" DB_USER=root DB_PASSWD="$PASSWORD"
export DB_NAME="$DATABASE" AUCTION_TEST_DB_NAME="$DATABASE" MYSQL_PWD="$PASSWORD"
unset DB_SOCKET

if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    mysql_ssl=(--ssl-mode=PREFERRED)
else
    mysql_ssl=(--skip-ssl)
fi
mysql_client=(mysql "${mysql_ssl[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u root)
ready=0
for _ in $(seq 1 90); do
    if "${mysql_client[@]}" -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
if [[ "$ready" != 1 ]]; then
    printf 'Database did not become ready: %s\n' "$IMAGE" >&2
    exit 1
fi

"${mysql_client[@]}" -e \
    "CREATE DATABASE $DATABASE CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${mysql_client[@]}" "$DATABASE" < "$ROOT/migrations/bootstrap_multithread_safe.sql"
python3 "$ROOT/scripts/migration_runner.py" adopt --kind fresh_bootstrap
python3 "$ROOT/scripts/migration_runner.py" run
"${mysql_client[@]}" "$DATABASE" < "$ROOT/migrations/auction_transactional_cutover.sql"

cd "$ROOT"
python3 tests/async/run_auction_transaction_disposable_mysql.py
printf 'Auction transaction disposable schema fixture (%s): ok\n' "$IMAGE"
