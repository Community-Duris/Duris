#!/usr/bin/env bash
# Always uses disposable databases in a disposable container; never reads .env.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE="${COLLECTOR_CATALOG_DB_IMAGE:-mariadb:10.11}"
NAME="duris-collector-catalog-$$-$RANDOM"
PASSWORD="collector-catalog-$$-$RANDOM"
FRESH_DB=collector_catalog_fresh
UPGRADE_DB=collector_catalog_upgrade
cleanup() { docker rm -f "$NAME" >/dev/null 2>&1 || true; }
trap cleanup EXIT HUP INT TERM
if [[ "$IMAGE" == mariadb:* ]]; then SECRET=MARIADB_ROOT_PASSWORD; else SECRET=MYSQL_ROOT_PASSWORD; fi
docker run -d --name "$NAME" -e "$SECRET=$PASSWORD" "$IMAGE" --innodb-use-native-aio=OFF >/dev/null
ready=0
for _ in $(seq 1 90); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
[[ "$ready" == 1 ]] || { echo "FAILED: $IMAGE did not accept local connections" >&2; exit 1; }

docker cp "$ROOT/migrations/bootstrap_multithread_safe.sql" "$NAME:/tmp/bootstrap.sql" >/dev/null
docker cp "$ROOT/migrations/immutable/0018_collector_catalog.sql" "$NAME:/tmp/collector.sql" >/dev/null
docker cp "$ROOT/migrations/immutable/0018_collector_catalog.sh" "$NAME:/tmp/collector.sh" >/dev/null
docker exec "$NAME" chmod +x /tmp/collector.sh
MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B --raw)
"${MYSQL[@]}" -e "CREATE DATABASE $FRESH_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci; CREATE DATABASE $UPGRADE_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

# Fresh bootstrap and post-baseline upgrade must converge to the same authority.
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$FRESH_DB' < /tmp/bootstrap.sql"
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$UPGRADE_DB' < /tmp/bootstrap.sql"
"${MYSQL[@]}" "$UPGRADE_DB" -e "DROP TABLE collector_ledger,collector_listings,collector_deaths,collector_reconciliation_quarantine,collector_catalog_state;"
for replay in 1 2; do
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$FRESH_DB' < /tmp/collector.sql"
    docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$UPGRADE_DB' < /tmp/collector.sql"
    for database in "$FRESH_DB" "$UPGRADE_DB"; do
        docker exec -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_USER=root \
            -e DB_PASSWD="$PASSWORD" -e DB_NAME="$database" "$NAME" /tmp/collector.sh >/dev/null
    done
done
initial=$("${MYSQL[@]}" "$FRESH_DB" -e "SELECT CONCAT(state_id,':',catalog_revision,':',next_listing) FROM collector_catalog_state;")
[[ "$initial" == 1:0:1 ]] || { echo "FAILED: fresh collector catalog cursor differs: $initial" >&2; exit 1; }

# Exercise valid rows, then prove verifier replay remains valid after normal revision growth.
"${MYSQL[@]}" "$UPGRADE_DB" -e "
INSERT INTO critical_operation_inbox
  (operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_payload)
VALUES
  (UNHEX(REPEAT('01',16)),UNHEX(REPEAT('02',32)),UNHEX(REPEAT('03',32)),17,1,1,2,X''),
  (UNHEX(REPEAT('04',16)),UNHEX(REPEAT('05',32)),UNHEX(REPEAT('06',32)),17,1,1,2,X'');
INSERT INTO collector_deaths
  (death_operation_id,beneficiary_pid,death_time,hint_state,hint_revision)
VALUES (UNHEX(REPEAT('01',16)),42,1700000000,1,1);
INSERT INTO collector_listings
  (listing_id,death_operation_id,beneficiary_pid,item_uid,status,holding_paused,due_at,
   listing_revision,item_revision,price_value,record_blob,item_blob)
VALUES
  (41,UNHEX(REPEAT('01',16)),42,9001,2,0,1700043200,2,7,200,
   UNHEX(REPEAT('00',154)),X'01');
INSERT INTO collector_ledger
  (operation_id,listing_id,action,catalog_revision,listing_revision,actor_pid,item_uid,
   value_delta,closed_reason,source_site)
VALUES (UNHEX(REPEAT('04',16)),41,1,9,2,0,9001,0,0,6);
INSERT INTO collector_reconciliation_quarantine
  (listing_id,item_uid,conflict_code,evidence)
VALUES (41,9001,1,'fixture conflict');
UPDATE collector_catalog_state SET catalog_revision=9,next_listing=42 WHERE state_id=1;"
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" sh -c "mysql -h127.0.0.1 -uroot '$UPGRADE_DB' < /tmp/collector.sql"
docker exec -e ENVIRONMENT=test -e DB_HOST=127.0.0.1 -e DB_USER=root \
    -e DB_PASSWD="$PASSWORD" -e DB_NAME="$UPGRADE_DB" "$NAME" /tmp/collector.sh >/dev/null
advanced=$("${MYSQL[@]}" "$UPGRADE_DB" -e "SELECT CONCAT(state_id,':',catalog_revision,':',next_listing) FROM collector_catalog_state;")
[[ "$advanced" == 1:9:42 ]] || { echo "FAILED: migration replay rewound collector catalog: $advanced" >&2; exit 1; }

# Database guards must reject corrupt projections and undersized canonical records.
expect_reject() {
    local label=$1 statement=$2
    if "${MYSQL[@]}" "$UPGRADE_DB" -e "$statement" >/dev/null 2>&1; then
        echo "FAILED: collector schema accepted $label" >&2
        exit 1
    fi
}
expect_reject short-record "INSERT INTO collector_listings (listing_id,death_operation_id,beneficiary_pid,item_uid,status,listing_revision,item_revision,record_blob) VALUES (42,UNHEX(REPEAT('01',16)),42,9002,1,1,1,X'00');"
expect_reject invalid-status "INSERT INTO collector_listings (listing_id,death_operation_id,beneficiary_pid,item_uid,status,listing_revision,item_revision,record_blob) VALUES (42,UNHEX(REPEAT('01',16)),42,9002,7,1,1,UNHEX(REPEAT('00',154)));"
expect_reject blank-evidence "INSERT INTO collector_reconciliation_quarantine (listing_id,item_uid,conflict_code,evidence) VALUES (41,9002,2,'');"
expect_reject zero-next-listing "UPDATE collector_catalog_state SET next_listing=0 WHERE state_id=1;"

metadata_query="SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name LIKE 'collector_%' UNION ALL SELECT CONCAT('C',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),CHAR(9),COALESCE(column_default,'<NULL>'),CHAR(9),extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name LIKE 'collector_%' UNION ALL SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,CHAR(9),column_name) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name LIKE 'collector_%' UNION ALL SELECT CONCAT('F',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),column_name,CHAR(9),referenced_table_name,CHAR(9),referenced_column_name) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name LIKE 'collector_%' AND referenced_table_name IS NOT NULL ORDER BY 1;"
fresh_shape=$("${MYSQL[@]}" "$FRESH_DB" -e "$metadata_query")
upgrade_shape=$("${MYSQL[@]}" "$UPGRADE_DB" -e "$metadata_query")
[[ "$fresh_shape" == "$upgrade_shape" ]] || { echo 'FAILED: fresh and upgraded collector metadata differ' >&2; exit 1; }

# The dedicated runtime compatibility owner qualifies the complete migration
# history and fingerprint on both engines. This fixture keeps its collector
# fresh/upgrade/replay and corruption assertions independent of that history.

printf 'collector catalog fresh/upgrade/replay and corruption guards (%s): ok\n' "$IMAGE"
