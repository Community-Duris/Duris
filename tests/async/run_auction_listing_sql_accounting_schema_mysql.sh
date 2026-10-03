#!/usr/bin/env bash
# Disposable auction listing EAP1 journey. Never reads .env or an existing game DB.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${AUCTION_ACCOUNTING_DB_IMAGE:-mariadb:10.11}"
NAME="duris-auction-listing-$$-$RANDOM"
PASSWORD="auction-listing-$$-$RANDOM"
DATABASE=economic_schema_test_auction

cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM

if [[ "$IMAGE" == mariadb:* ]]; then
    PASSWORD_ENV=MARIADB_ROOT_PASSWORD
else
    PASSWORD_ENV=MYSQL_ROOT_PASSWORD
fi

docker run -d --name "$NAME" -p 127.0.0.1::3306 \
    -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
port="${mapping##*:}"
export ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1
export DB_HOST=127.0.0.1 DB_PORT="$port" DB_USER=root DB_PASSWD="$PASSWORD"
export DB_NAME="$DATABASE" MYSQL_PWD="$PASSWORD"

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
cd "$ROOT"
python3 tests/async/run_auction_listing_sql_accounting_mysql.py
printf 'Auction listing EAP1 journey (%s): ok\n' "$IMAGE"
