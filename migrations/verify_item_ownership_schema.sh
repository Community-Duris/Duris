#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('item_uid_allocator','item_owner_revision','item_current_owner','item_ownership_baseline','item_ownership_quarantine','item_ownership_ledger') AND engine='InnoDB';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='item_uid_allocator' AND column_name IN ('allocator_id','next_uid','updated_at')) OR (table_name='item_owner_revision' AND column_name IN ('owner_type','owner_id','owner_context_id','revision','updated_at')) OR (table_name='item_current_owner' AND column_name IN ('item_uid','root_item_uid','parent_item_uid','owner_type','owner_id','owner_context_id','item_revision','vnum','state','coin_payload','updated_at')) OR (table_name='item_ownership_baseline' AND column_name IN ('item_uid','root_item_uid','parent_item_uid','owner_type','owner_id','owner_context_id','opening_item_revision','vnum','source_table','source_row_id','captured_at')) OR (table_name='item_ownership_quarantine' AND column_name IN ('quarantine_id','item_uid','source_table','source_row_id','conflict_code','evidence','detected_at','repaired_at')) OR (table_name='item_ownership_ledger' AND column_name IN ('operation_id','event_index','item_uid','root_item_uid','parent_item_uid','from_owner_type','from_owner_id','from_owner_context_id','to_owner_type','to_owner_id','to_owner_context_id','item_revision','from_owner_revision','to_owner_revision','reason_type','reason_id','source_site','created_at')));")
total_columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ('item_uid_allocator','item_owner_revision','item_current_owner','item_ownership_baseline','item_ownership_quarantine','item_ownership_ledger');")
[[ "$total_columns" == 56 ]] || { echo 'FAILED: unexpected item ownership columns' >&2; exit 1; }
singleton=$(read_scalar "SELECT COUNT(*) FROM item_uid_allocator WHERE allocator_id=1 AND next_uid>0;")
primary_keys=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,GROUP_CONCAT(column_name ORDER BY seq_in_index) signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND index_name='PRIMARY' AND table_name IN ('item_uid_allocator','item_owner_revision','item_current_owner','item_ownership_baseline','item_ownership_quarantine','item_ownership_ledger') GROUP BY table_name HAVING signature IN ('allocator_id','owner_type,owner_id,owner_context_id','item_uid','quarantine_id','operation_id,event_index')) exact_pk;")
secondary_indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) signature FROM information_schema.statistics WHERE table_schema=DATABASE() GROUP BY table_name,index_name,non_unique HAVING (table_name='item_owner_revision' AND index_name='idx_item_owner_revision_updated' AND signature='updated_at') OR (table_name='item_current_owner' AND index_name='idx_item_current_root_uid' AND signature='root_item_uid,item_uid') OR (table_name='item_current_owner' AND index_name='idx_item_current_owner' AND signature='owner_type,owner_id,owner_context_id,item_uid') OR (table_name='item_current_owner' AND index_name='idx_item_current_parent' AND signature='parent_item_uid') OR (table_name='item_ownership_baseline' AND index_name='uq_item_baseline_source' AND non_unique=0 AND signature='source_table,source_row_id') OR (table_name='item_ownership_baseline' AND index_name='idx_item_baseline_owner' AND signature='owner_type,owner_id,owner_context_id') OR (table_name='item_ownership_quarantine' AND index_name='uq_item_quarantine_evidence' AND non_unique=0 AND signature='item_uid,source_table,source_row_id,conflict_code') OR (table_name='item_ownership_quarantine' AND index_name='idx_item_quarantine_open' AND signature='repaired_at,item_uid') OR (table_name='item_ownership_ledger' AND index_name='uq_item_ledger_item_revision' AND non_unique=0 AND signature='item_uid,item_revision') OR (table_name='item_ownership_ledger' AND index_name='idx_item_ledger_item_created' AND signature='item_uid,created_at') OR (table_name='item_ownership_ledger' AND index_name='idx_item_ledger_from_owner' AND signature='from_owner_type,from_owner_id,from_owner_context_id,created_at') OR (table_name='item_ownership_ledger' AND index_name='idx_item_ledger_to_owner' AND signature='to_owner_type,to_owner_id,to_owner_context_id,created_at')) exact_secondary;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('item_current_parent_fk','item_ownership_operation_fk') AND update_rule='RESTRICT' AND delete_rule='RESTRICT';")

[[ "$tables" == 6 ]] || { echo "FAILED: expected 6 item ownership tables; found $tables" >&2; exit 1; }
coin_payload=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='item_current_owner' AND column_name='coin_payload' AND data_type='mediumblob' AND is_nullable='YES' AND ordinal_position=10 AND (column_default IS NULL OR column_default='NULL');")
[[ "$coin_payload" == 1 ]] || { echo 'FAILED: authoritative coin payload column missing' >&2; exit 1; }
[[ "$columns" == 56 ]] || { echo "FAILED: expected 56 item ownership columns; found $columns" >&2; exit 1; }
[[ "$singleton" == 1 ]] || { echo 'FAILED: item UID allocator singleton missing' >&2; exit 1; }
[[ "$primary_keys" == 6 ]] || { echo "FAILED: expected 6 exact primary keys; found $primary_keys" >&2; exit 1; }
[[ "$secondary_indexes" == 12 ]] || { echo "FAILED: expected 12 exact secondary indexes; found $secondary_indexes" >&2; exit 1; }
[[ "$foreign_keys" == 2 ]] || { echo "FAILED: expected 2 restrictive foreign keys; found $foreign_keys" >&2; exit 1; }
printf 'item ownership schema verified: 6 tables, 56 columns, allocator, 18 indexes, and 2 foreign keys\n'
