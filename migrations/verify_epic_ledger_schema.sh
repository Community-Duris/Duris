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

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('epic_balance_baseline','epic_ledger') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
columns=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='player_data' AND column_name='epic_revision' AND data_type='bigint' AND column_type LIKE '%unsigned') OR (table_name='epic_balance_baseline' AND column_name IN ('pid','opening_balance','opening_revision','captured_at')) OR (table_name='epic_ledger' AND column_name IN ('operation_id','pid','delta','balance_after','epic_revision','reason_type','reason_id','source_site','created_at')));")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) signature FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN ('epic_balance_baseline','epic_ledger') GROUP BY table_name,index_name,non_unique HAVING (table_name='epic_balance_baseline' AND index_name='PRIMARY' AND signature='pid') OR (table_name='epic_ledger' AND index_name='PRIMARY' AND signature='operation_id') OR (table_name='epic_ledger' AND index_name='uq_epic_ledger_pid_revision' AND non_unique=0 AND signature='pid,epic_revision') OR (table_name='epic_ledger' AND index_name='idx_epic_ledger_pid_created' AND signature='pid,created_at') OR (table_name='epic_ledger' AND index_name='idx_epic_ledger_reason_created' AND signature='reason_type,created_at')) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND table_name='epic_ledger' AND constraint_name='epic_ledger_operation_fk' AND update_rule='RESTRICT' AND delete_rule='RESTRICT';")

[[ "$tables" == 2 ]] || { echo "FAILED: expected 2 epic InnoDB tables; found $tables" >&2; exit 1; }
[[ "$columns" == 14 ]] || { echo "FAILED: expected epic revision plus 13 named columns; found $columns" >&2; exit 1; }
[[ "$indexes" == 5 ]] || { echo "FAILED: expected 5 exact epic indexes; found $indexes" >&2; exit 1; }
[[ "$foreign_keys" == 1 ]] || { echo "FAILED: expected epic operation foreign key; found $foreign_keys" >&2; exit 1; }
printf 'epic ledger schema verified: 2 tables, 14 columns, 5 indexes, 1 foreign key\n'
