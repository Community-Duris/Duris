#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-runtime-contract-$$"
PASSWORD=$(printf 'runtime-contract-%s-%s' "$$" "$RANDOM")
DB_NAME="runtime_contract_test"
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
for file in bootstrap_multithread_safe.sql immutable/0001_lookup_dataset_state.sql immutable/0001_lookup_dataset_state.sh immutable/0002_player_item_metadata_uniqueness.sql immutable/0002_player_item_metadata_uniqueness.sh immutable/0003_season_reset_state.sql immutable/0003_season_reset_state.sh runtime_compatibility_manifest.json verify_runtime_compatibility.sh; do
    docker cp "$ROOT/migrations/$file" "$NAME:/tmp/$(basename "$file")" >/dev/null
done
docker exec "$NAME" chmod +x /tmp/0001_lookup_dataset_state.sh /tmp/0002_player_item_metadata_uniqueness.sh /tmp/0003_season_reset_state.sh /tmp/verify_runtime_compatibility.sh
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/bootstrap_multithread_safe.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0001_lookup_dataset_state.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0002_player_item_metadata_uniqueness.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0003_season_reset_state.sql"
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0001_lookup_dataset_state.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0002_player_item_metadata_uniqueness.sh >/dev/null
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0003_season_reset_state.sh >/dev/null
history_checksum=$(PYTHONPATH="$ROOT/scripts" python3 - <<'PY'
import migration_runner as m
x=m.load_manifest()
rows=[m.AppliedMigration(a.migration_id,a.sequence,a.description,a.apply_checksum,a.verify_checksum,a.compatibility,x.runner_version) for a in x.migrations]
print(m.history_checksum(rows))
PY
)
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
"${MYSQL[@]}" -e "INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES('duris-schema-2026-08-27-session11','fresh_bootstrap',UNHEX('db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd'),1,1); INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0001_lookup_dataset_state',1,'Add atomic race and class lookup dataset state',UNHEX('e39db8df5bd7a8d71d5cc9c177919b8117d6fed77a9318f79610ed9413de4ccb'),UNHEX('90fc6de3aa449ef9b5b77fc96032314e129dfdd631e9760af86dc40329d7f0ca'),'mysql8-mariadb10',1),('0002_player_item_metadata_uniqueness',2,'Deduplicate and guard player item descriptions and affects',UNHEX('00e86dc65e6d5e935a50cd731d010675ade6da7fbbfdd18c4fe6fb17f88addba'),UNHEX('312aa0aa354439e15bcef68403f00b7158c96627518d8be0bf85f59890ea1a90'),'mysql8-mariadb10',1),('0003_season_reset_state',3,'Add durable season reset epoch and fence state',UNHEX('82390d1302e9a0bec3a0111fc22fd428b83ef3c0954a0904cfb9215a8ea14cc7'),UNHEX('75aeeae3f6bbfd486f4c596b33c742d23cff85d319f1a833c8503587f7d751a9'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=3,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
verify() { docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" -e RUNTIME_COMPATIBILITY_MANIFEST=/tmp/runtime_compatibility_manifest.json "$NAME" /tmp/verify_runtime_compatibility.sh; }
expect_reject() { if verify >/dev/null 2>&1; then echo "runtime drift was accepted: $1" >&2; exit 1; fi; }
verify >/dev/null
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
printf 'runtime full-schema and season-fence pass plus history/table/engine/collation/index/column drift rejection (%s): ok\n' "$DB_IMAGE"
