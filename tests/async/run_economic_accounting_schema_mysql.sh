#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# Synthetic, disposable engine only; never source checkout .env.
# Inherited sockets must not redirect the migration runner away from this engine.
unset DB_SOCKET
NAME="duris-accounting-schema-$$-$RANDOM"
PASSWORD="accounting-schema-$$-$RANDOM"
IMAGE="${ECONOMIC_ACCOUNTING_DB_IMAGE:-mariadb:10.11}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then PASSWORD_ENV=MARIADB_ROOT_PASSWORD; else PASSWORD_ENV=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -p 127.0.0.1::3306 -e "$PASSWORD_ENV=$PASSWORD" "$IMAGE" >/dev/null
mapping="$(docker port "$NAME" 3306/tcp)"
export ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="${mapping##*:}"
export DB_USER=root DB_PASSWD="$PASSWORD" MYSQL_PWD="$PASSWORD"
export DB_NAME="economic_schema_test_$RANDOM"
export ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "$DB_PORT" -u "$DB_USER" -N -B)
ready=0
for _ in $(seq 1 90); do
    if "${MYSQL[@]}" -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    sleep 1
done
[[ "$ready" == 1 ]]
"${MYSQL[@]}" -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci"
"${MYSQL[@]}" "$DB_NAME" < migrations/bootstrap_multithread_safe.sql
python3 scripts/migration_runner.py adopt --kind fresh_bootstrap
python3 scripts/migration_runner.py run
python3 scripts/migration_runner.py run
bash migrations/verify_runtime_compatibility.sh
"${MYSQL[@]}" "$DB_NAME" < migrations/immutable/0031_economy_accounting.sql
bash migrations/immutable/0031_economy_accounting.sh
python3 tests/async/test_economic_accounting_schema_mysql.py -v
python3 migrations/verify_economic_baseline_schema.py
python3 tests/async/test_economic_baseline_schema_mysql.py -v

bash tests/async/run_economic_accounting_authority_mysql.sh

python3 tests/async/test_economic_sql_bank_transaction_flatfile.py
python3 tests/async/run_economic_sql_bank_transaction_mysql.py

python3 tests/async/run_economic_sql_baseline_transaction_mysql.py
python3 tests/async/run_economic_sql_source_snapshot_mysql.py
