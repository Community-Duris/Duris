#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-runtime-contract-$$"
PASSWORD=$(printf 'runtime-contract-%s-%s' "$$" "$RANDOM")
DB_NAME="runtime_contract_test"
LEGACY_DB_NAME="runtime_contract_legacy_test"
DB_IMAGE="${RUNTIME_DB_IMAGE:-mysql:8.0}"
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT
if [[ "$DB_IMAGE" == mariadb:* ]]; then
    ROOT_PASSWORD_ENV="MARIADB_ROOT_PASSWORD"
else
    ROOT_PASSWORD_ENV="MYSQL_ROOT_PASSWORD"
fi
docker run -d --name "$NAME" -e "$ROOT_PASSWORD_ENV=$PASSWORD" "$DB_IMAGE" >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        sleep 2
        if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then ready=1; break; fi
    fi
    sleep 1
done
[[ "$ready" == 1 ]]
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -e "CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
migration_files=$(PYTHONPATH="$ROOT/scripts" python3 - <<'PYTHON'
import migration_runner as m
for step in m.load_manifest().migrations:
    print(step.apply_path.relative_to(m.ROOT / "migrations"))
    print(step.verify_path.relative_to(m.ROOT / "migrations"))
PYTHON
)
mapfile -t MIGRATION_FILES <<< "$migration_files"
for file in bootstrap_multithread_safe.sql "${MIGRATION_FILES[@]}" runtime_compatibility_manifest.json verify_runtime_compatibility.sh; do
    docker cp "$ROOT/migrations/$file" "$NAME:/tmp/$(basename "$file")" >/dev/null
    if [[ "$file" == *.sh ]]; then
        docker exec "$NAME" chmod +x "/tmp/$(basename "$file")"
    fi
done

# The pre-b029 launcher created server_reboots outside the migration system.
# Prove that 0004 converts that exact shape, preserves every row, removes its
# legacy-only metadata/indexes, and remains safe to replay on both engines.
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -e "
CREATE DATABASE $LEGACY_DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $LEGACY_DB_NAME;
CREATE TABLE server_reboots (
  id INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  boot_time INT NOT NULL,
  shutdown_time INT NOT NULL,
  uptime_seconds INT NOT NULL,
  shutdown_type VARCHAR(50) NOT NULL DEFAULT 'unknown',
  initiated_by VARCHAR(255) NULL,
  reason TEXT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_boot_time (boot_time),
  INDEX idx_shutdown_time (shutdown_time),
  INDEX idx_created_at (created_at),
  INDEX idx_shutdown_type (shutdown_type)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
INSERT INTO server_reboots
  (id,boot_time,shutdown_time,uptime_seconds,shutdown_type,initiated_by,reason)
VALUES
  (2,1700000000,1700000060,60,'shutdown','operator','planned stop'),
  (7,1700001000,1700001120,120,'autoreboot_copyover',NULL,NULL),
  (11,1700002000,1700002005,5,'legacy_custom','legacy-admin','old value');
"
for _ in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$LEGACY_DB_NAME' < /tmp/0004_server_reboots.sql"
    docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
        -e DB_PASSWD="$PASSWORD" -e DB_NAME="$LEGACY_DB_NAME" \
        "$NAME" /tmp/0004_server_reboots.sh >/dev/null
done
LEGACY_MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$LEGACY_DB_NAME")
legacy_rows=$("${LEGACY_MYSQL[@]}" -e "
SELECT GROUP_CONCAT(
  CONCAT_WS(':',record_id,boot_time,shutdown_time,uptime_seconds,shutdown_type,
            COALESCE(initiated_by,'<null>'),COALESCE(reason,'<null>'))
  ORDER BY record_id SEPARATOR '|')
FROM server_reboots;")
[[ "$legacy_rows" == "2:1700000000:1700000060:60:shutdown:operator:planned stop|7:1700001000:1700001120:120:autoreboot_copyover:<null>:<null>|11:1700002000:1700002005:5:unknown:legacy-admin:old value" ]]
legacy_scratch=$("${LEGACY_MYSQL[@]}" -e "
SELECT COUNT(*) FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_name LIKE 'server_reboots_0004%';")
[[ "$legacy_scratch" == 0 ]]
"${LEGACY_MYSQL[@]}" -e "
INSERT INTO server_reboots
  (boot_time,shutdown_time,uptime_seconds,shutdown_type,initiated_by,reason)
VALUES (1700003000,1700003001,1,'reboot',NULL,NULL);"
legacy_next_id=$("${LEGACY_MYSQL[@]}" -e "SELECT MAX(record_id) FROM server_reboots;")
[[ "$legacy_next_id" == 12 ]]

# The kingdom code's pre-registration migrations/kingdom_realms.sql created this
# table with "default charset=utf8mb4" and no COLLATE clause, so it carries the
# character set's default collation rather than utf8mb4_unicode_ci. That default
# differs per engine, so the CONVERT TO below pins the drifted state to the same
# collation on both and the pre-state assertion cannot pass vacuously. Prove that
# 0006 converges it, preserves the row, and stays exactly re-runnable.
"${LEGACY_MYSQL[@]}" -e "
CREATE TABLE kingdom_realms (
  assoc_id INT NOT NULL,
  realm_id INT NOT NULL DEFAULT 0,
  hall_vnum INT NOT NULL DEFAULT 0,
  highest_claim INT NOT NULL DEFAULT 0,
  res_mineral BIGINT NOT NULL DEFAULT 0,
  res_wood BIGINT NOT NULL DEFAULT 0,
  res_fibre BIGINT NOT NULL DEFAULT 0,
  res_water BIGINT NOT NULL DEFAULT 0,
  upkeep_paid_through BIGINT NOT NULL DEFAULT 0,
  arrears INT NOT NULL DEFAULT 0,
  missed_cycles INT NOT NULL DEFAULT 0,
  PRIMARY KEY (assoc_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
ALTER TABLE kingdom_realms CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
INSERT INTO kingdom_realms
  (assoc_id,realm_id,hall_vnum,highest_claim,res_mineral,res_wood,res_fibre,
   res_water,upkeep_paid_through,arrears,missed_cycles)
VALUES (3,1,7801,24,5,6,7,8,1700000000,1,2);"
legacy_realm_collation=$("${LEGACY_MYSQL[@]}" -e "
SELECT table_collation FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_name='kingdom_realms';")
[[ "$legacy_realm_collation" == utf8mb4_general_ci ]]
for _ in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$LEGACY_DB_NAME' < /tmp/0006_kingdom_realms.sql"
    docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
        -e DB_PASSWD="$PASSWORD" -e DB_NAME="$LEGACY_DB_NAME" \
        "$NAME" /tmp/0006_kingdom_realms.sh >/dev/null
done
legacy_realm_row=$("${LEGACY_MYSQL[@]}" -e "
SELECT CONCAT_WS(':',assoc_id,realm_id,hall_vnum,highest_claim,res_mineral,
                 res_wood,res_fibre,res_water,upkeep_paid_through,arrears,
                 missed_cycles)
FROM kingdom_realms;")
[[ "$legacy_realm_row" == "3:1:7801:24:5:6:7:8:1700000000:1:2" ]]

# The garrison migration must preserve purchased guards while converging a
# pre-existing table's collation, including on a second application.
"${LEGACY_MYSQL[@]}" -e "
CREATE TABLE kingdom_garrison (
  assoc_id INT NOT NULL,
  slot INT NOT NULL,
  guard_class INT NOT NULL DEFAULT 0,
  level INT NOT NULL DEFAULT 0,
  PRIMARY KEY (assoc_id,slot)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;
INSERT INTO kingdom_garrison VALUES (3,0,1,12),(3,16,2,20);"
for _ in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
        "mysql -h127.0.0.1 -uroot '$LEGACY_DB_NAME' < /tmp/0009_kingdom_garrison.sql"
    docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
        -e DB_PASSWD="$PASSWORD" -e DB_NAME="$LEGACY_DB_NAME" \
        "$NAME" /tmp/0009_kingdom_garrison.sh >/dev/null
done
legacy_garrison_rows=$("${LEGACY_MYSQL[@]}" -e "
SELECT GROUP_CONCAT(CONCAT_WS(':',assoc_id,slot,guard_class,level)
                    ORDER BY assoc_id,slot SEPARATOR '|') FROM kingdom_garrison;")
[[ "$legacy_garrison_rows" == "3:0:1:12|3:16:2:20" ]]

docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/bootstrap_multithread_safe.sql"
# Apply every registered step and its verifier, including an exact replay.
for replay in 1 2; do
    for file in "${MIGRATION_FILES[@]}"; do
        if [[ "$file" == *.sql ]]; then
            docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c \
                "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/$(basename "$file")"
        else
            docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
                -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" \
                "$NAME" "/tmp/$(basename "$file")" >/dev/null
        fi
    done
done
history_sql=$(PYTHONPATH="$ROOT/scripts" python3 - <<'PYTHON'
import migration_runner as m
manifest = m.load_manifest()
def quote(value):
    # Hex literals keep fixture descriptions independent of SQL escaping modes.
    return "CONVERT(UNHEX('" + value.encode().hex() + "') USING utf8mb4)"
print("INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES(" +
      quote(manifest.baseline_id) + ",'fresh_bootstrap',UNHEX('" + manifest.required_table_fingerprint +
      "')," + str(manifest.version) + "," + str(manifest.runner_version) + ");")
rows = []
for step in manifest.migrations:
    rows.append(m.AppliedMigration(step.migration_id, step.sequence, step.description,
                                  step.apply_checksum, step.verify_checksum,
                                  step.compatibility, manifest.runner_version))
    values = [quote(step.migration_id), str(step.sequence), quote(step.description),
              "UNHEX('" + step.apply_checksum + "')", "UNHEX('" + step.verify_checksum + "')",
              quote(step.compatibility), str(manifest.runner_version)]
    print("INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES(" +
          ",".join(values) + ");")
print("UPDATE mud_schema_migration_state SET applied_count=" + str(len(rows)) +
      ",history_checksum=UNHEX('" + m.history_checksum(rows) + "') WHERE state_id=1;")
PYTHON
)
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
"${MYSQL[@]}" -e "$history_sql"
history_checksum=$("${MYSQL[@]}" -e "SELECT LOWER(HEX(history_checksum)) FROM mud_schema_migration_state WHERE state_id=1;")
"${MYSQL[@]}" -e "CREATE TABLE imported_extension_probe (id INT PRIMARY KEY, note VARCHAR(32)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci; INSERT INTO imported_extension_probe VALUES (1, 'preserved');"

verify() { docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" -e RUNTIME_COMPATIBILITY_MANIFEST=/tmp/runtime_compatibility_manifest.json "$NAME" /tmp/verify_runtime_compatibility.sh; }
expect_reject() { if verify >/dev/null 2>&1; then echo "runtime drift was accepted: $1" >&2; exit 1; fi; }
verify >/dev/null
for table in player_death_disposition player_death_custody kingdom_garrison epic_stone_claim; do
    "${MYSQL[@]}" -e "RENAME TABLE $table TO ${table}_drift;"
    expect_reject "missing-$table"
    "${MYSQL[@]}" -e "RENAME TABLE ${table}_drift TO $table;"
    verify >/dev/null
done
"${MYSQL[@]}" -e "ALTER TABLE imported_extension_probe ADD COLUMN pid INT UNSIGNED NULL, ADD CONSTRAINT imported_extension_probe_pid_fk FOREIGN KEY (pid) REFERENCES player_data(pid);"
expect_reject inbound-foreign-key
"${MYSQL[@]}" -e "ALTER TABLE imported_extension_probe DROP FOREIGN KEY imported_extension_probe_pid_fk, DROP COLUMN pid;"
"${MYSQL[@]}" -e "START TRANSACTION; SELECT season_epoch FROM season_reset_state WHERE state_id=1 FOR UPDATE; UPDATE season_reset_state SET season_epoch=season_epoch+1,reset_status='resetting',reset_started_at=UTC_TIMESTAMP(6),reset_completed_at=NULL WHERE state_id=1 AND reset_status='active'; COMMIT;" >/dev/null
season_fenced=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM season_reset_state WHERE state_id=1 AND season_epoch=2 AND reset_status='resetting' AND reset_started_at IS NOT NULL AND reset_completed_at IS NULL;")
[[ "$season_fenced" == 1 ]]
"${MYSQL[@]}" -e "UPDATE season_reset_state SET reset_status='active',reset_completed_at=UTC_TIMESTAMP(6) WHERE state_id=1 AND season_epoch=2 AND reset_status='resetting';"
season_active=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM season_reset_state WHERE state_id=1 AND season_epoch=2 AND reset_status='active' AND reset_completed_at IS NOT NULL;")
[[ "$season_active" == 1 ]]
"${MYSQL[@]}" -e "UPDATE mud_schema_migration_state SET history_checksum=UNHEX(REPEAT('00',32)) WHERE state_id=1;"
expect_reject history
"${MYSQL[@]}" -e "UPDATE mud_schema_migration_state SET history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
"${MYSQL[@]}" -e "RENAME TABLE lookup_dataset_state TO lookup_dataset_state_drift;"
expect_reject missing-table
"${MYSQL[@]}" -e "RENAME TABLE lookup_dataset_state_drift TO lookup_dataset_state;"
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state ENGINE=MyISAM;"
expect_reject engine
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state ENGINE=InnoDB;"
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;"
expect_reject collation
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state DROP PRIMARY KEY;"
expect_reject index
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state ADD PRIMARY KEY(dataset_name);"
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state MODIFY dataset_version BIGINT UNSIGNED NOT NULL;"
expect_reject column
"${MYSQL[@]}" -e "ALTER TABLE lookup_dataset_state MODIFY dataset_version INT UNSIGNED NOT NULL; DELETE FROM level_cap WHERE id=1;"
expect_reject level-cap-singleton
printf 'legacy server-reboot and kingdom-realm convergence, runtime full-schema, season fence, and history/table/engine/collation/index/column drift rejection (%s): ok\n' "$DB_IMAGE"
