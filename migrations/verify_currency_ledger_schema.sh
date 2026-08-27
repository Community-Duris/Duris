#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    if [[ -f "$PROJECT_ROOT/.env" ]]; then
        # shellcheck disable=SC1091
        source "$PROJECT_ROOT/.env"
    fi
fi
: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('currency_wallet_baseline','currency_bank_baseline','currency_ledger') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='player_data' AND column_name='wallet_revision' AND data_type='bigint' AND column_type LIKE '%unsigned') OR (table_name='account_banks' AND column_name='bank_revision' AND data_type='bigint' AND column_type LIKE '%unsigned') OR (table_name='currency_wallet_baseline' AND column_name IN ('pid','opening_copper','opening_silver','opening_gold','opening_platinum','opening_revision','captured_at')) OR (table_name='currency_bank_baseline' AND column_name IN ('bank_id','opening_copper','opening_silver','opening_gold','opening_platinum','opening_revision','captured_at')) OR (table_name='currency_ledger' AND column_name IN ('operation_id','pid','bank_id','wallet_delta_copper','wallet_delta_silver','wallet_delta_gold','wallet_delta_platinum','bank_delta_copper','bank_delta_silver','bank_delta_gold','bank_delta_platinum','wallet_after_copper','wallet_after_silver','wallet_after_gold','wallet_after_platinum','bank_after_copper','bank_after_silver','bank_after_gold','bank_after_platinum','wallet_revision','bank_revision','reason_type','reason_id','source_site','created_at'))); ")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN ('currency_wallet_baseline','currency_bank_baseline','currency_ledger') GROUP BY table_name,index_name,non_unique HAVING (table_name='currency_wallet_baseline' AND index_name='PRIMARY' AND signature='pid') OR (table_name='currency_bank_baseline' AND index_name='PRIMARY' AND signature='bank_id') OR (table_name='currency_ledger' AND index_name='PRIMARY' AND signature='operation_id') OR (table_name='currency_ledger' AND index_name='uq_currency_wallet_revision' AND non_unique=0 AND signature='pid,wallet_revision') OR (table_name='currency_ledger' AND index_name='uq_currency_bank_revision' AND non_unique=0 AND signature='bank_id,bank_revision') OR (table_name='currency_ledger' AND index_name='idx_currency_pid_created' AND signature='pid,created_at') OR (table_name='currency_ledger' AND index_name='idx_currency_bank_created' AND signature='bank_id,created_at') OR (table_name='currency_ledger' AND index_name='idx_currency_reason_created' AND signature='reason_type,created_at')) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND table_name='currency_ledger' AND constraint_name='currency_ledger_operation_fk' AND update_rule='RESTRICT' AND delete_rule='RESTRICT';")

[[ "$tables" == 3 ]] || { echo "FAILED: expected 3 currency InnoDB tables; found $tables" >&2; exit 1; }
[[ "$columns" == 41 ]] || { echo "FAILED: expected 41 currency revision/table columns; found $columns" >&2; exit 1; }
[[ "$indexes" == 8 ]] || { echo "FAILED: expected 8 exact currency indexes; found $indexes" >&2; exit 1; }
[[ "$foreign_keys" == 1 ]] || { echo "FAILED: expected currency operation foreign key; found $foreign_keys" >&2; exit 1; }
printf 'currency ledger schema verified: 3 tables, 41 columns, 8 indexes, 1 foreign key\n'
