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
for file in bootstrap_multithread_safe.sql immutable/0001_lookup_dataset_state.sql immutable/0001_lookup_dataset_state.sh runtime_compatibility_manifest.json verify_runtime_compatibility.sh; do
    docker cp "$ROOT/migrations/$file" "$NAME:/tmp/$(basename "$file")" >/dev/null
done
docker exec "$NAME" chmod +x /tmp/0001_lookup_dataset_state.sh /tmp/verify_runtime_compatibility.sh
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/bootstrap_multithread_safe.sql && mysql -h127.0.0.1 -uroot '$DB_NAME' < /tmp/0001_lookup_dataset_state.sql"
docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" "$NAME" /tmp/0001_lookup_dataset_state.sh >/dev/null
history_checksum=$(PYTHONPATH="$ROOT/scripts" python3 - <<'PY'
import migration_runner as m
x=m.load_manifest(); a=x.migrations[0]
row=m.AppliedMigration(a.migration_id,a.sequence,a.description,a.apply_checksum,a.verify_checksum,a.compatibility,x.runner_version)
print(m.history_checksum([row]))
PY
)
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B "$DB_NAME")
"${MYSQL[@]}" -e "INSERT INTO mud_schema_baselines(baseline_id,baseline_kind,schema_fingerprint,manifest_version,runner_version) VALUES('duris-schema-2026-08-27-session11','fresh_bootstrap',UNHEX('db13d7a42bf82bcbd32bac8d83224913c755fefd000ade6d4e798b1bd4f494dd'),1,1); INSERT INTO mud_schema_history(migration_id,sequence_number,description,apply_checksum,verify_checksum,compatibility,runner_version) VALUES('0001_lookup_dataset_state',1,'Add atomic race and class lookup dataset state',UNHEX('e39db8df5bd7a8d71d5cc9c177919b8117d6fed77a9318f79610ed9413de4ccb'),UNHEX('90fc6de3aa449ef9b5b77fc96032314e129dfdd631e9760af86dc40329d7f0ca'),'mysql8-mariadb10',1); UPDATE mud_schema_migration_state SET applied_count=1,history_checksum=UNHEX('$history_checksum') WHERE state_id=1;"
verify() { docker exec -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" -e RUNTIME_COMPATIBILITY_MANIFEST=/tmp/runtime_compatibility_manifest.json "$NAME" /tmp/verify_runtime_compatibility.sh; }
expect_reject() { if verify >/dev/null 2>&1; then echo "runtime drift was accepted: $1" >&2; exit 1; fi; }
verify >/dev/null
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
printf 'runtime full-schema pass and history/table/engine/collation/index/column drift rejection (%s): ok\n' "$DB_IMAGE"
