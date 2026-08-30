#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-legacy-migration-$RANDOM-$$"
PASSWORD="duris-legacy-test-$RANDOM-$$"
MIGRATED_DB="duris_legacy_migration_test"
BOOTSTRAP_DB="duris_fresh_bootstrap_test"
CONFIG=$(mktemp)

cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
    rm -f "$CONFIG"
}
trap cleanup EXIT HUP INT TERM

chmod 600 "$CONFIG"
docker run --rm -d --name "$NAME" -p 127.0.0.1::3306 \
    -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null
mapping=$(docker port "$NAME" 3306/tcp)
DB_PORT=${mapping##*:}

ready=0
for _ in $(seq 1 90); do
    if MYSQL_PWD="$PASSWORD" mysql --skip-ssl -h127.0.0.1 -P"$DB_PORT" \
        -uroot -N -B -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]]

MYSQL=(mysql --skip-ssl -h127.0.0.1 -P"$DB_PORT" -uroot)
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" -e "
CREATE DATABASE $MIGRATED_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE $BOOTSTRAP_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$MIGRATED_DB" \
    < "$ROOT/migrations/bootstrap_legacy_baseline.sql"
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$BOOTSTRAP_DB" \
    < "$ROOT/migrations/bootstrap_multithread_safe.sql"

printf '%s\n' \
    'ENVIRONMENT=test' \
    'DB_HOST=127.0.0.1' \
    "DB_PORT=$DB_PORT" \
    'DB_USER=root' \
    "DB_PASSWD=$PASSWORD" \
    "DB_NAME=$MIGRATED_DB" \
    "DB_ALLOWED_TARGETS=127.0.0.1/$MIGRATED_DB" \
    'REDIS=FALSE' > "$CONFIG"

MIGRATION_ENV_FILE="$CONFIG" "$ROOT/migrations/run_migration.sh"

ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="$DB_PORT" DB_USER=root \
    DB_PASSWD="$PASSWORD" DB_NAME="$BOOTSTRAP_DB" \
    python3 "$ROOT/scripts/migration_runner.py" adopt --kind fresh_bootstrap
ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="$DB_PORT" DB_USER=root \
    DB_PASSWD="$PASSWORD" DB_NAME="$BOOTSTRAP_DB" \
    python3 "$ROOT/scripts/migration_runner.py" run

MYSQL_PWD="$PASSWORD" MYSQL_HOST=127.0.0.1 MYSQL_PORT="$DB_PORT" MYSQL_USER=root \
    MIGRATION_TEST_DB="$MIGRATED_DB" MIGRATION_SCRIPT="$ROOT/migrations/run_migration.sh" \
    MIGRATION_ENV_FILE="$CONFIG" sh "$ROOT/tests/test_migration_replay_safety.sh"
MYSQL_PWD="$PASSWORD" MYSQL_HOST=127.0.0.1 MYSQL_PORT="$DB_PORT" MYSQL_USER=root \
    MIGRATION_TEST_DB="$MIGRATED_DB" sh "$ROOT/tests/test_run_migration_persistence_schema.sh"
MYSQL_PWD="$PASSWORD" MYSQL_HOST=127.0.0.1 MYSQL_PORT="$DB_PORT" MYSQL_USER=root \
    MIGRATED_DB="$MIGRATED_DB" BOOTSTRAP_DB="$BOOTSTRAP_DB" \
    sh "$ROOT/tests/compare_bootstrap_mud_schema.sh"
ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="$DB_PORT" DB_USER=root \
    DB_PASSWD="$PASSWORD" DB_NAME="$MIGRATED_DB" \
    "$ROOT/migrations/verify_runtime_compatibility.sh"

echo 'legacy migration, replay, bootstrap equivalence, and runtime compatibility: ok'
