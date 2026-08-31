#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-legacy-migration-$RANDOM-$$"
PASSWORD="duris-legacy-test-$RANDOM-$$"
DB_IMAGE="${LEGACY_DB_IMAGE:-mysql:8.0}"
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
    -e MYSQL_ROOT_PASSWORD="$PASSWORD" "$DB_IMAGE" >/dev/null
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
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$MIGRATED_DB" -e "
CREATE TABLE imported_extension_probe (
    id INT NOT NULL PRIMARY KEY,
    note VARCHAR(32) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO imported_extension_probe VALUES (1, 'preserved');
CREATE TABLE server_reboots (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    boot_time INT NOT NULL,
    shutdown_time INT DEFAULT NULL,
    uptime_seconds INT DEFAULT NULL,
    shutdown_type VARCHAR(50) NOT NULL DEFAULT 'unknown',
    initiated_by VARCHAR(255) DEFAULT NULL,
    reason TEXT,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY idx_boot_time (boot_time),
    KEY idx_shutdown_time (shutdown_time),
    KEY idx_shutdown_type (shutdown_type),
    KEY idx_uptime (uptime_seconds)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO server_reboots
    (id,boot_time,shutdown_time,uptime_seconds,shutdown_type,initiated_by,reason)
VALUES
    (41,100,175,75,'reboot','operator','legacy row'),
    (42,200,NULL,NULL,'web_stop','operator','unfinished legacy row');"
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$BOOTSTRAP_DB" \
    < "$ROOT/migrations/bootstrap_multithread_safe.sql"
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$BOOTSTRAP_DB" -e "
SET FOREIGN_KEY_CHECKS=0;
INSERT INTO player_item_extra_descr (id,item_id,keyword,description) VALUES
    (9001,9001,'legacy-keyword','duplicate legacy metadata'),
    (9002,9001,'legacy-keyword','duplicate legacy metadata');
SET FOREIGN_KEY_CHECKS=1;"
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$BOOTSTRAP_DB" \
    < "$ROOT/migrations/legacy_schema_convergence.sql"

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
    DB_PASSWD="$PASSWORD" DB_NAME="$MIGRATED_DB" \
    python3 "$ROOT/scripts/migration_runner.py" run

extension_state=$(MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" -N -B "$MIGRATED_DB" -e \
    "SELECT CONCAT(COUNT(*), ':', MIN(note)) FROM imported_extension_probe;")
[[ "$extension_state" == "1:preserved" ]]
reboot_state=$(MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" -N -B "$MIGRATED_DB" -e \
    "SELECT CONCAT(COUNT(*), ':', MIN(record_id), ':', MIN(boot_time), ':', SUM(record_id=42 AND shutdown_time=200 AND uptime_seconds=0 AND shutdown_type='unknown')) FROM server_reboots;")
[[ "$reboot_state" == "2:41:100:1" ]]
archive_state=$(MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" -N -B "$MIGRATED_DB" -e \
    "SELECT CONCAT(COUNT(*), ':', MIN(source_id), ':', SUM(source_id=42 AND shutdown_time IS NULL AND uptime_seconds IS NULL AND shutdown_type='web_stop')) FROM legacy_import_server_reboots;")
[[ "$archive_state" == "2:41:1" ]]

ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="$DB_PORT" DB_USER=root \
    DB_PASSWD="$PASSWORD" DB_NAME="$BOOTSTRAP_DB" \
    python3 "$ROOT/scripts/migration_runner.py" adopt --kind fresh_bootstrap
ENVIRONMENT=test DB_HOST=127.0.0.1 DB_PORT="$DB_PORT" DB_USER=root \
    DB_PASSWD="$PASSWORD" DB_NAME="$BOOTSTRAP_DB" \
    python3 "$ROOT/scripts/migration_runner.py" run
metadata_state=$(MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" -N -B "$BOOTSTRAP_DB" -e \
    "SELECT CONCAT((SELECT COUNT(*) FROM player_item_extra_descr WHERE item_id=9001), ':', (SELECT COUNT(*) FROM legacy_import_player_item_extra_descr WHERE item_id=9001));")
[[ "$metadata_state" == "1:2" ]]
MYSQL_PWD="$PASSWORD" "${MYSQL[@]}" "$BOOTSTRAP_DB" -e \
    "DROP TABLE legacy_import_player_item_extra_descr;"

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
