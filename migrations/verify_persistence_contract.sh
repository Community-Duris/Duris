#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    if [[ -f "$SCRIPT_DIR/.env" ]]; then
        # shellcheck disable=SC1091
        source "$SCRIPT_DIR/.env"
    elif [[ -f "$PROJECT_ROOT/.env" ]]; then
        # shellcheck disable=SC1091
        source "$PROJECT_ROOT/.env"
    fi
fi

: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

MYSQL_PWD="$DB_PASSWD"
export MYSQL_PWD
if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

read_scalar() {
    "${MYSQL[@]}" -e "$1"
}

required_columns=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns
WHERE table_schema=DATABASE() AND
 ((table_name='persistence_item_events' AND column_name IN
 ('id','ts_usec','event_type','item_uid','vnum','item','actor','actor_id','source','target','note','dedupe_key','created_at'))
 OR (table_name='persistence_scalar_events' AND column_name IN
 ('id','event_type','event_key','boot_time','touched_at','zone_number','toucher_pid','group_size','epic_value','alignment_delta','dedupe_key','created_at')));")


exact_columns=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns
WHERE table_schema=DATABASE() AND (
  (table_name='persistence_item_events' AND ordinal_position=1 AND column_name='id' AND column_type='bigint unsigned' AND is_nullable='NO' AND column_default IS NULL AND extra LIKE '%auto_increment%')
  OR (table_name='persistence_item_events' AND ordinal_position=2 AND column_name='ts_usec' AND column_type='bigint unsigned' AND is_nullable='NO' AND column_default IS NULL)
  OR (table_name='persistence_item_events' AND ordinal_position=3 AND column_name='event_type' AND column_type='varchar(64)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=4 AND column_name='item_uid' AND column_type='bigint unsigned' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_item_events' AND ordinal_position=5 AND column_name='vnum' AND column_type='int' AND is_nullable='NO' AND column_default='-1')
  OR (table_name='persistence_item_events' AND ordinal_position=6 AND column_name='item' AND column_type='varchar(255)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=7 AND column_name='actor' AND column_type='varchar(128)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=8 AND column_name='actor_id' AND column_type='int' AND is_nullable='NO' AND column_default='-1')
  OR (table_name='persistence_item_events' AND ordinal_position=9 AND column_name='source' AND column_type='varchar(255)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=10 AND column_name='target' AND column_type='varchar(255)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=11 AND column_name='note' AND column_type='varchar(255)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_item_events' AND ordinal_position=12 AND column_name='created_at' AND column_type='datetime' AND is_nullable='NO' AND UPPER(column_default)='CURRENT_TIMESTAMP')
  OR (table_name='persistence_item_events' AND ordinal_position=13 AND column_name='dedupe_key' AND column_type='varchar(64)' AND is_nullable='YES' AND column_default IS NULL)
  OR (table_name='persistence_scalar_events' AND ordinal_position=1 AND column_name='id' AND column_type='bigint unsigned' AND is_nullable='NO' AND column_default IS NULL AND extra LIKE '%auto_increment%')
  OR (table_name='persistence_scalar_events' AND ordinal_position=2 AND column_name='event_type' AND column_type='varchar(64)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_scalar_events' AND ordinal_position=3 AND column_name='event_key' AND column_type='varchar(255)' AND is_nullable='NO' AND column_default='')
  OR (table_name='persistence_scalar_events' AND ordinal_position=4 AND column_name='boot_time' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=5 AND column_name='touched_at' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=6 AND column_name='zone_number' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=7 AND column_name='toucher_pid' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=8 AND column_name='group_size' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=9 AND column_name='epic_value' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=10 AND column_name='alignment_delta' AND column_type='int' AND is_nullable='NO' AND column_default='0')
  OR (table_name='persistence_scalar_events' AND ordinal_position=11 AND column_name='dedupe_key' AND column_type='varchar(64)' AND is_nullable='YES' AND column_default IS NULL)
  OR (table_name='persistence_scalar_events' AND ordinal_position=12 AND column_name='created_at' AND column_type='datetime' AND is_nullable='NO' AND UPPER(column_default)='CURRENT_TIMESTAMP')
);" )

required_indexes=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT DISTINCT table_name, index_name
 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND
  ((table_name='persistence_item_events' AND index_name IN
    ('PRIMARY','idx_item_uid_ts','idx_event_type_created','uq_item_dedupe'))
   OR (table_name='persistence_scalar_events' AND index_name IN
    ('PRIMARY','idx_scalar_event_key','idx_scalar_zone_time','uq_scalar_dedupe')))
) AS required_indexes;")

exact_indexes=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT table_name, index_name,
        GROUP_CONCAT(CONCAT(column_name, ':', non_unique) ORDER BY seq_in_index) AS signature
 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name IN ('persistence_item_events','persistence_scalar_events')
 GROUP BY table_name, index_name
 HAVING (table_name='persistence_item_events' AND index_name='PRIMARY' AND signature='id:0')
     OR (table_name='persistence_item_events' AND index_name='idx_item_uid_ts' AND signature='item_uid:1,ts_usec:1,id:1')
     OR (table_name='persistence_item_events' AND index_name='idx_event_type_created' AND signature='event_type:1,created_at:1')
     OR (table_name='persistence_item_events' AND index_name='uq_item_dedupe' AND signature='dedupe_key:0')
     OR (table_name='persistence_scalar_events' AND index_name='PRIMARY' AND signature='id:0')
     OR (table_name='persistence_scalar_events' AND index_name='idx_scalar_event_key' AND signature='event_type:1,event_key:1')
     OR (table_name='persistence_scalar_events' AND index_name='idx_scalar_zone_time' AND signature='zone_number:1,touched_at:1')
     OR (table_name='persistence_scalar_events' AND index_name='uq_scalar_dedupe' AND signature='dedupe_key:0')
) AS exact_indexes;")

event_tables_exact=$(read_scalar "
SELECT COUNT(*) FROM information_schema.tables
WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci'
  AND table_name IN ('persistence_item_events','persistence_scalar_events');")

auction_innodb=$(read_scalar "
SELECT COUNT(DISTINCT table_name)
FROM information_schema.tables
WHERE table_schema=DATABASE() AND engine='InnoDB'
  AND table_name IN
  ('auction_bid_history','auction_item_pickups','auction_money_pickups','auctions');")

failed=0
if [[ "$required_columns" != "25" ]]; then
    printf 'FAILED: expected 25 required columns; found %s\n' "$required_columns" >&2
    failed=1
elif [[ "$exact_columns" != "25" ]]; then
    printf 'FAILED: expected 25 exact column definitions; found %s\n' "$exact_columns" >&2
    failed=1
fi
if [[ "$required_indexes" != "8" ]]; then
    printf 'FAILED: expected 8 required indexes; found %s\n' "$required_indexes" >&2
    failed=1
elif [[ "$exact_indexes" != "8" ]]; then
    printf 'FAILED: expected 8 exact index definitions; found %s\n' "$exact_indexes" >&2
    failed=1
fi
if [[ "$event_tables_exact" != "2" ]]; then
    printf 'FAILED: expected 2 exact InnoDB/utf8mb4 persistence event tables; found %s\n' "$event_tables_exact" >&2
    failed=1
fi
if [[ "$auction_innodb" != "4" ]]; then
    printf 'FAILED: expected 4 InnoDB auction tables; found %s\n' "$auction_innodb" >&2
    failed=1
fi

if [[ "$failed" -ne 0 ]]; then
    exit 1
fi

printf 'persistence contract verified: 25 exact columns, 8 exact indexes, 2 exact event tables, 4 InnoDB auction tables\n'
