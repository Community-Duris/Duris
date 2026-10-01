#!/usr/bin/env bash
set -euo pipefail
: "$DB_HOST" "$DB_USER" "$DB_PASSWD" "$DB_NAME"
export MYSQL_PWD="$DB_PASSWD"
PORT="$(printenv DB_PORT || true)"
if [[ -z "$PORT" ]]; then PORT=3306; fi
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    MYSQL_SSL=--ssl-mode=PREFERRED
else
    MYSQL_SSL=--skip-ssl
fi
scalar() {
    mysql "$MYSQL_SSL" -h "$DB_HOST" -P "$PORT" -u "$DB_USER" -N -B "$DB_NAME" -e "$1"
}

old_unique=$(scalar "SELECT COUNT(*) FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name='saved_items'
  AND index_name='item_key' AND non_unique=0")
[[ "$old_unique" == 0 ]] || { echo 'FAILED: saved_items still has unique item_key' >&2; exit 1; }

lookup=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index)
FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name='saved_items'
  AND index_name='idx_saved_items_item_key' AND non_unique=1")
[[ "$lookup" == item_key ]] || { echo 'FAILED: saved_items item_key lookup index differs' >&2; exit 1; }

shape=$(scalar "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables
WHERE table_schema=DATABASE() AND table_name='saved_item_recovery_handoff'")
[[ "$shape" == InnoDB:utf8mb4_unicode_ci ]] ||
    { echo 'FAILED: saved-item handoff table engine/collation differs' >&2; exit 1; }

columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns
WHERE table_schema=DATABASE() AND table_name='saved_item_recovery_handoff'
  AND column_name IN ('season_epoch','source_root_id','source_key','source_uid',
    'source_room_vnum','source_row_count','source_id_digest',
    'destination_root_id','destination_key',
    'acknowledged_at','retired_at')")
[[ "$columns" == 11 ]] || { echo 'FAILED: saved-item handoff columns differ' >&2; exit 1; }

primary=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index)
FROM information_schema.statistics
WHERE table_schema=DATABASE() AND table_name='saved_item_recovery_handoff'
  AND index_name='PRIMARY'")
[[ "$primary" == season_epoch,source_root_id ]] ||
    { echo 'FAILED: saved-item handoff primary key differs' >&2; exit 1; }

echo 'saved-item recovery handoff schema verified'
