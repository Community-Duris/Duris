#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('collector_catalog_state','collector_deaths','collector_listings','collector_ledger','collector_reconciliation_quarantine') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
[[ "$tables" == 5 ]] || { echo "FAILED: expected 5 InnoDB utf8mb4 collector tables; found $tables" >&2; exit 1; }

singleton=$(scalar "SELECT COUNT(*) FROM collector_catalog_state WHERE state_id=1 AND next_listing>0;")
singleton_rows=$(scalar "SELECT COUNT(*) FROM collector_catalog_state;")
[[ "$singleton" == 1 && "$singleton_rows" == 1 ]] || { echo "FAILED: collector catalog singleton is absent or invalid" >&2; exit 1; }

death_columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='collector_deaths' AND column_name IN ('death_operation_id','beneficiary_pid','death_time','collection_delay','sale_delay','holding_duration','price_percent','minimum_value','hint_state','hint_revision','created_at','updated_at');")
[[ "$death_columns" == 12 ]] || { echo "FAILED: collector death contract has $death_columns of 12 columns" >&2; exit 1; }

listing_columns=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='collector_listings' AND column_name IN ('listing_id','death_operation_id','beneficiary_pid','item_uid','status','holding_paused','due_at','listing_revision','item_revision','price_value','record_blob','item_blob','created_at','updated_at');")
[[ "$listing_columns" == 14 ]] || { echo "FAILED: collector listing contract has $listing_columns of 14 columns" >&2; exit 1; }

death_indexes=$(scalar "SELECT COUNT(DISTINCT index_name) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='collector_deaths' AND index_name IN ('PRIMARY','uq_collector_death_identity') AND non_unique=0;")
[[ "$death_indexes" == 2 ]] || { echo "FAILED: collector death identity is not uniquely indexed" >&2; exit 1; }

indexes=$(scalar "SELECT COUNT(DISTINCT index_name) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='collector_listings' AND index_name IN ('PRIMARY','uq_collector_death_item','idx_collector_item_history','idx_collector_beneficiary','idx_collector_due');")
[[ "$indexes" == 5 ]] || { echo "FAILED: collector listing indexes differ; found $indexes of 5" >&2; exit 1; }

ledger_primary=$(scalar "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='collector_ledger' AND index_name='PRIMARY';")
[[ "$ledger_primary" == "operation_id,listing_id" ]] || { echo "FAILED: collector ledger primary key is $ledger_primary, expected operation_id,listing_id" >&2; exit 1; }

foreign_keys=$(scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name='collector_listing_death_fk';")
inbox_foreign_keys=$(scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('collector_death_operation_fk','collector_ledger_operation_fk');")
[[ "$foreign_keys" == 1 && "$inbox_foreign_keys" == 0 ]] || { echo "FAILED: collector foreign keys retain inbox history or omit listing/death integrity" >&2; exit 1; }

checks=$(scalar "SELECT COUNT(*) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND constraint_type='CHECK' AND constraint_name IN ('chk_collector_catalog_singleton','chk_collector_catalog_next_listing','chk_collector_death_beneficiary','chk_collector_death_time','chk_collector_death_collection_delay','chk_collector_death_sale_delay','chk_collector_death_holding_duration','chk_collector_death_price_percent','chk_collector_death_minimum_value','chk_collector_hint_state','chk_collector_listing_id','chk_collector_listing_beneficiary','chk_collector_listing_item','chk_collector_listing_status','chk_collector_listing_paused','chk_collector_listing_revision','chk_collector_listing_item_revision','chk_collector_record_size','chk_collector_item_blob_size','chk_collector_ledger_listing','chk_collector_ledger_action','chk_collector_ledger_catalog_revision','chk_collector_ledger_listing_revision','chk_collector_ledger_item','chk_collector_ledger_reason','chk_collector_quarantine_listing','chk_collector_quarantine_conflict','chk_collector_quarantine_evidence');")
[[ "$checks" == 28 ]] || { echo "FAILED: collector checks differ; found $checks of 28" >&2; exit 1; }

echo 'collector catalog schema verified'
