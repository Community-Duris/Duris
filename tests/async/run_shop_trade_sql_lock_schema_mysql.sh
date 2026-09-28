#!/usr/bin/env bash
# Disposable inactive shop SQL lock fixture. Never reads .env or an existing game DB.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${SHOP_TRADE_LOCK_DB_IMAGE:-mariadb:10.11}"
NAME="duris-shop-lock-$$-$RANDOM"
PASSWORD="shop-lock-$$-$RANDOM"
DATABASE=economic_schema_test_shop_lock

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
"${mysql_client[@]}" "$DATABASE" < "$ROOT/migrations/immutable/0038_item_equipment_slot.sql"
"${mysql_client[@]}" "$DATABASE" < "$ROOT/migrations/immutable/0041_shopkeeper_cash_identity.sql"
cd "$ROOT"
python3 tests/async/run_shop_trade_sql_lock_mysql.py
printf 'Inactive shop SQL lock fixture (%s): ok\n' "$IMAGE"
