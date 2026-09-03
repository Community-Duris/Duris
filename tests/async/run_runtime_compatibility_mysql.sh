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
docker run --rm -d --name "$NAME" -e "$ROOT_PASSWORD_ENV=$PASSWORD" "$DB_IMAGE" >/dev/null
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
for file in bootstrap_multithread_safe.sql immutable/0001_lookup_dataset_state.sql immutable/0001_lookup_dataset_state.sh immutable/0002_player_item_metadata_uniqueness.sql immutable/0002_player_item_metadata_uniqueness.sh immutable/0003_season_reset_state.sql immutable/0003_season_reset_state.sh immutable/0004_server_reboots.sql immutable/0004_server_reboots.sh immutable/0005_level_cap_singleton.sql immutable/0005_level_cap_singleton.sh immutable/0006_kingdom_realms.sql immutable/0006_kingdom_realms.sh immutable/0007_pkill_event_stamp_contract.sql immutable/0007_pkill_event_stamp_contract.sh immutable/0008_statistics_date_index.sql immutable/0008_statistics_date_index.sh runtime_compatibility_manifest.json verify_runtime_compatibility.sh; do
    docker cp "$ROOT/migrations/$file" "$NAME:/tmp/$(basename "$file")" >/dev/null
done
docker exec "$NAME" chmod +x /tmp/0001_lookup_dataset_state.sh /tmp/0002_player_item_metadata_uniqueness.sh /tmp/0003_season_reset_state.sh /tmp/0004_server_reboots.sh /tmp/0005_level_cap_singleton.sh /tmp/0006_kingdom_realms.sh /tmp/0007_pkill_event_stamp_contract.sh /tmp/0008_statistics_date_index.sh /tmp/verify_runtime_compatibility.sh

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

docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/bootstrap_multithread_safe.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0001_lookup_dataset_state.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0002_player_item_metadata_uniqueness.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0003_season_reset_state.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0004_server_reboots.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0005_level_cap_singleton.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0006_kingdom_realms.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0007_pkill_event_stamp_contract.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0008_statistics_date_index.sql"
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0001_lookup_dataset_state.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0002_player_item_metadata_uniqueness.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0003_season_reset_state.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0004_server_reboots.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0005_level_cap_singleton.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0006_kingdom_realms.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0007_pkill_event_stamp_contract.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0008_statistics_date_index.sh >/dev/null
history_checksum=$(PYTHONPATH="$ROOT/scripts" python3 - <<'PY'
import migration_runner as m
x=m.load_manifest()
rows=[m.AppliedMigration(a.migration_id,a.sequence,a.description,a.apply_checksum,a.verify_checksum,a.compatibility,x.runner_version) for a in x.migrations]
print(m.history_checksum(rows))
PY
)
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
"${MYSQL[@]}" -e "INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES('duris-schema-2026-08-27-session11','fresh_bootstrap',UNHEX('db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd'),1,1); INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0001_lookup_dataset_state',1,'Add atomic race and class lookup dataset state',UNHEX('e39db8df5bd7a8d71d5cc9c177919b8117d6fed77a9318f79610ed9413de4ccb'),UNHEX('90fc6de3aa449ef9b5b77fc96032314e129dfdd631e9760af86dc40329d7f0ca'),'mysql8-mariadb10',1),('0002_player_item_metadata_uniqueness',2,'Deduplicate and guard player item descriptions and affects',UNHEX('00e86dc65e6d5e935a50cd731d010675ade6da7fbbfdd18c4fe6fb17f88addba'),UNHEX('312aa0aa354439e15bcef68403f00b7158c96627518d8be0bf85f59890ea1a90'),'mysql8-mariadb10',1),('0003_season_reset_state',3,'Add durable season reset epoch and fence state',UNHEX('82390d1302e9a0bec3a0111fc22fd428b83ef3c0954a0904cfb9215a8ea14cc7'),UNHEX('75aeeae3f6bbfd486f4c596b33c742d23cff85d319f1a833c8503587f7d751a9'),'mysql8-mariadb10',1),('0004_server_reboots',4,'Add durable server reboot lifecycle records',UNHEX('4756002529e0e55eedf8de2391c333f37a579e30415da28c340162e1b030834f'),UNHEX('15c4e45ce796d854df1e0d6e1257023969829e8e17322c41d081c0a3e27f1e1b'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=4,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
"${MYSQL[@]}" -e "CREATE TABLE imported_extension_probe (id INT PRIMARY KEY, note VARCHAR(32)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci; INSERT INTO imported_extension_probe VALUES (1, 'preserved');"
"${MYSQL[@]}" -e "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0005_level_cap_singleton',5,'Restore the required level cap singleton state',UNHEX('a83264e2f9241e328bcf76eefa192a0d10b0b4122917e56ef12ad39a35bc6132'),UNHEX('4974eb5bf494251d83fc9c3f0c381a61f3afbd80e4eae6f9f291ea5b5c77b77f'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=5,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
"${MYSQL[@]}" -e "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0006_kingdom_realms',6,'Add the guild kingdom realm territory and upkeep table',UNHEX('f146b2236b7c982bc909ec981f2d2d06831f21fc224824f670607811506e5c93'),UNHEX('c9d3ac9c6f4c43fc85940c6f012fbe6c29aa6fbb3d6daf80652aee5363599697'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=6,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
"${MYSQL[@]}" -e "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0007_pkill_event_stamp_contract',7,'Replace the pkill_event stamp zero-date default with a portable contract',UNHEX('9afca6945d9d1524dea06a404d18a168b70f2b2c1840913f0561d45378ad0608'),UNHEX('8fe8e260c77d5dfa45a1efb4bff3c44030f344fec0d93e47fa046204f008ac43'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=7,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
"${MYSQL[@]}" -e "INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0008_statistics_date_index',8,'Index the statistics population time series on its epoch date column',UNHEX('ca99318a723dc4c54d78d74027967299abb64f8b48426f2feff3a28b693f909d'),UNHEX('9d345e93f4528944c63d71d75082b8208f7222330c3b45c6f062de7c6c5255de'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=8,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
verify() { docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" -e RUNTIME_COMPATIBILITY_MANIFEST=/tmp/runtime_compatibility_manifest.json "$NAME" /tmp/verify_runtime_compatibility.sh; }
expect_reject() { if verify >/dev/null 2>&1; then echo "runtime drift was accepted: $1" >&2; exit 1; fi; }
verify >/dev/null
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
