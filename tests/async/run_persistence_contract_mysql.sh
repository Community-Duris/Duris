#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
NAME="duris-persistence-contract-$$"
PASSWORD="contract-test-only"
DB_NAME="persistence_contract_test"
REF_DB="persistence_contract_bootstrap_reference"

cleanup() {
    docker rm -f "$NAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

docker run --rm -d --name "$NAME" \
    -e MYSQL_ROOT_PASSWORD="$PASSWORD" mysql:8.0 >/dev/null

for _ in $(seq 1 60); do
    if docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null 2>&1; then
        break
    fi
    sleep 1
done
docker exec -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -e 'SELECT 1' >/dev/null

MYSQL=(docker exec -i -e MYSQL_PWD="$PASSWORD" "$NAME" mysql -h127.0.0.1 -uroot -N -B)

verify_contract() {
    local columns indexes engines exact_columns item_index scalar_index
    columns=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='persistence_item_events' AND column_name IN ('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at')) OR (table_name='persistence_scalar_events' AND column_name IN ('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')));")
    indexes=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM (SELECT DISTINCT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='persistence_item_events' AND index_name IN ('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe')) OR (table_name='persistence_scalar_events' AND index_name IN ('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))) x;")
    engines=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(DISTINCT table_name) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions');")
    exact_columns=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='persistence_item_events' AND column_name='event_type' AND column_type='varchar(64)' AND is_nullable='NO' AND column_default='') OR (table_name='persistence_item_events' AND column_name='item_uid' AND column_type='bigint unsigned' AND is_nullable='NO' AND column_default='0') OR (table_name='persistence_scalar_events' AND column_name='event_type' AND column_type='varchar(64)' AND is_nullable='NO' AND column_default=''));")
    item_index=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_item_events' AND index_name='idx_item_uid_ts';")
    scalar_index=$("${MYSQL[@]}" "$DB_NAME" -e "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='persistence_scalar_events' AND index_name='idx_scalar_zone_time';")
    [[ "$columns" == "25" ]]
    [[ "$indexes" == "8" ]]
    [[ "$engines" == "4" ]]
    [[ "$exact_columns" == "3" ]]
    [[ "$item_index" == "item_uid,ts_usec,id" ]]
    [[ "$scalar_index" == "zone_number,touched_at" ]]
}

"${MYSQL[@]}" <<SQL
CREATE DATABASE $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE DATABASE $REF_DB CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE $DB_NAME;
CREATE TABLE persistence_item_events (
    ts_usec BIGINT UNSIGNED NOT NULL DEFAULT 0,
    event_type VARCHAR(16) NOT NULL DEFAULT '',
    item_uid BIGINT NOT NULL DEFAULT 0,
    PRIMARY KEY (event_type),
    KEY idx_item_uid_ts (item_uid)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
INSERT INTO persistence_item_events (ts_usec,event_type,item_uid)
VALUES (9,'legacy_no_id',4);
CREATE TABLE persistence_scalar_events (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
    event_type VARCHAR(16) NULL DEFAULT NULL,
    KEY idx_scalar_zone_time (id)
) ENGINE=InnoDB;
CREATE TABLE auction_bid_history (id INT NOT NULL PRIMARY KEY) ENGINE=MyISAM;
CREATE TABLE auction_item_pickups (id INT NOT NULL PRIMARY KEY) ENGINE=MyISAM;
CREATE TABLE auction_money_pickups (id INT NOT NULL PRIMARY KEY) ENGINE=MyISAM;
CREATE TABLE auctions (id INT NOT NULL PRIMARY KEY) ENGINE=MyISAM;
SQL

python3 - "$ROOT/migrations/bootstrap_multithread_safe.sql" <<'PY' | "${MYSQL[@]}" "$REF_DB"
from pathlib import Path
import sys
text = Path(sys.argv[1]).read_text()
for table in ("persistence_item_events", "persistence_scalar_events"):
    start = text.index(f"CREATE TABLE `{table}`")
    end = text.index(";", start) + 1
    print(text[start:end])
PY

docker exec "$NAME" mkdir -p /tmp/persistence-contract
docker cp "$ROOT/migrations/persistence_contract.sql" "$NAME:/tmp/persistence-contract/persistence_contract.sql" >/dev/null
docker cp "$ROOT/migrations/verify_persistence_contract.sh" "$NAME:/tmp/persistence-contract/verify_persistence_contract.sh" >/dev/null
docker cp "$ROOT/migrations/apply_persistence_contract.sh" "$NAME:/tmp/persistence-contract/apply_persistence_contract.sh" >/dev/null

apply_contract() {
    docker exec \
        -e DB_HOST=127.0.0.1 -e DB_PORT=3306 -e DB_USER=root \
        -e DB_PASSWD="$PASSWORD" -e DB_NAME="$DB_NAME" \
        "$NAME" /tmp/persistence-contract/apply_persistence_contract.sh --confirm-db "$DB_NAME"
}

apply_contract
verify_contract
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM persistence_item_events WHERE event_type='legacy_no_id' AND id > 0;") == "1" ]]

"${MYSQL[@]}" "$DB_NAME" -e "INSERT INTO persistence_item_events (ts_usec,event_type,item_uid) VALUES (1,'replay_probe',7);"

schema_signature() {
    local database="${1:-$DB_NAME}"
    "${MYSQL[@]}" "$database" -e "
SELECT CONCAT('C|',table_name,'|',ordinal_position,'|',column_name,'|',column_type,'|',is_nullable,'|',COALESCE(column_default,'<NULL>'),'|',extra,'|',COALESCE(character_set_name,'<NULL>'),'|',COALESCE(collation_name,'<NULL>'))
FROM information_schema.columns
WHERE table_schema=DATABASE() AND table_name IN ('persistence_item_events','persistence_scalar_events')
UNION ALL
SELECT CONCAT('I|',table_name,'|',index_name,'|',seq_in_index,'|',column_name,'|',non_unique)
FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name IN ('persistence_item_events','persistence_scalar_events')
ORDER BY 1;"
}

before_replay=$(schema_signature)
bootstrap_signature=$(schema_signature "$REF_DB")
[[ "$before_replay" == "$bootstrap_signature" ]]
apply_contract
after_replay=$(schema_signature)
[[ "$before_replay" == "$after_replay" ]]
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM persistence_item_events WHERE event_type='replay_probe';") == "1" ]]

"${MYSQL[@]}" "$DB_NAME" -e "
ALTER TABLE persistence_item_events DROP COLUMN target;
ALTER TABLE persistence_scalar_events DROP INDEX idx_scalar_zone_time;"
apply_contract
verify_contract
[[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM persistence_item_events WHERE event_type='replay_probe';") == "1" ]]
[[ "$before_replay" == "$(schema_signature)" ]]

printf 'persistence contract MySQL convergence/replay test passed\n'
