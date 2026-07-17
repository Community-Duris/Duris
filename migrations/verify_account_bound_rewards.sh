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

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

read_scalar() {
    "${MYSQL[@]}" -e "$1"
}

table_exact=$(read_scalar "
SELECT COUNT(*) FROM information_schema.tables
WHERE table_schema=DATABASE()
  AND table_name='account_bound_rewards'
  AND engine='InnoDB'
  AND table_collation='utf8mb4_unicode_ci';")

columns_exact=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns
WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND (
 (ordinal_position=1 AND column_name='account_name' AND column_type='varchar(50)'
  AND is_nullable='NO' AND character_set_name='utf8mb4' AND collation_name='utf8mb4_unicode_ci')
 OR
 (ordinal_position=2 AND column_name='reward_vnum' AND column_type='int'
  AND is_nullable='NO' AND column_default='36419')
 OR
 (ordinal_position=3 AND column_name='granted_by' AND column_type='varchar(50)'
  AND is_nullable='NO' AND column_default='' AND character_set_name='utf8mb4'
  AND collation_name='utf8mb4_unicode_ci')
 OR
 (ordinal_position=4 AND column_name='created_at' AND data_type='timestamp'
  AND is_nullable='YES' AND UPPER(column_default)='CURRENT_TIMESTAMP')
 OR
 (ordinal_position=5 AND column_name='updated_at' AND data_type='timestamp'
  AND is_nullable='YES' AND UPPER(column_default)='CURRENT_TIMESTAMP'
  AND LOWER(extra) LIKE '%on update current_timestamp%')
);")

indexes_exact=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT index_name, non_unique,
        GROUP_CONCAT(column_name ORDER BY seq_in_index) AS columns_signature
 FROM information_schema.statistics
 WHERE table_schema=DATABASE() AND table_name='account_bound_rewards'
 GROUP BY index_name, non_unique
 HAVING (index_name='PRIMARY' AND non_unique=0 AND columns_signature='account_name,reward_vnum')
     OR (index_name='idx_account_bound_rewards_vnum' AND non_unique=1
         AND columns_signature='reward_vnum')
) AS exact_account_reward_indexes;")

fk_exact=$(read_scalar "
SELECT COUNT(*)
FROM information_schema.key_column_usage kcu
JOIN information_schema.referential_constraints rc
  ON rc.constraint_schema=kcu.constraint_schema
 AND rc.table_name=kcu.table_name
 AND rc.constraint_name=kcu.constraint_name
WHERE kcu.constraint_schema=DATABASE()
  AND kcu.table_name='account_bound_rewards'
  AND kcu.constraint_name='account_bound_rewards_ibfk_1'
  AND kcu.column_name='account_name'
  AND kcu.referenced_table_name='accounts'
  AND kcu.referenced_column_name='account_name'
  AND rc.update_rule='NO ACTION'
  AND rc.delete_rule='CASCADE';")

failed=0
[[ "$table_exact" == "1" ]] || { printf 'FAILED: account_bound_rewards table engine/collation mismatch\n' >&2; failed=1; }
[[ "$columns_exact" == "5" ]] || { printf 'FAILED: expected 5 exact account_bound_rewards columns; found %s\n' "$columns_exact" >&2; failed=1; }
[[ "$indexes_exact" == "2" ]] || { printf 'FAILED: expected 2 exact account_bound_rewards indexes; found %s\n' "$indexes_exact" >&2; failed=1; }
[[ "$fk_exact" == "1" ]] || { printf 'FAILED: account_bound_rewards cascade FK mismatch\n' >&2; failed=1; }

[[ "$failed" == "0" ]] || exit 1
printf 'account reward schema verified: 1 table, 5 columns, 2 indexes, cascade FK\n'
