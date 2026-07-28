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
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

parent_table=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
ledger_table=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
state_table=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")

parent_columns=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards' AND (
 (ordinal_position=1 AND column_name='id' AND column_type='bigint unsigned' AND is_nullable='NO' AND extra LIKE '%auto_increment%') OR
 (ordinal_position=2 AND column_name='account_name' AND column_type='varchar(50)' AND is_nullable='NO' AND collation_name='utf8mb4_unicode_ci') OR
 (ordinal_position=3 AND column_name='reward_vnum' AND column_type='int' AND is_nullable='NO' AND column_default='36419') OR
 (ordinal_position=4 AND column_name='template_version' AND column_type='smallint unsigned' AND is_nullable='NO' AND column_default='0') OR
 (ordinal_position=5 AND column_name='template_json' AND data_type='longtext' AND is_nullable='YES' AND collation_name='utf8mb4_unicode_ci') OR
 (ordinal_position=6 AND column_name='display_name' AND column_type='varchar(512)' AND is_nullable='NO' AND column_default='' AND collation_name='utf8mb4_unicode_ci') OR
 (ordinal_position=7 AND column_name='granted_by' AND column_type='varchar(50)' AND is_nullable='NO' AND column_default='' AND collation_name='utf8mb4_unicode_ci') OR
 (ordinal_position=8 AND column_name='created_at' AND data_type='timestamp' AND is_nullable='YES' AND UPPER(column_default)='CURRENT_TIMESTAMP') OR
 (ordinal_position=9 AND column_name='updated_at' AND data_type='timestamp' AND is_nullable='YES' AND UPPER(column_default)='CURRENT_TIMESTAMP' AND LOWER(extra) LIKE '%on update current_timestamp%') OR
 (ordinal_position=10 AND column_name='expires_at' AND data_type='datetime' AND is_nullable='YES' AND column_default IS NULL) OR
 (ordinal_position=11 AND column_name='remaining_pwipes' AND column_type='int unsigned' AND is_nullable='YES' AND column_default IS NULL)
);")
parent_column_total=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_rewards';")

ledger_columns=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons' AND (
 (ordinal_position=1 AND column_name='grant_id' AND column_type='bigint unsigned' AND is_nullable='NO') OR
 (ordinal_position=2 AND column_name='pid' AND column_type='int unsigned' AND is_nullable='NO') OR
 (ordinal_position=3 AND column_name='last_summoned_at' AND data_type='datetime' AND is_nullable='NO' AND UPPER(column_default)='CURRENT_TIMESTAMP') OR
 (ordinal_position=4 AND column_name='recovery_ready' AND column_type='tinyint(1)' AND is_nullable='NO' AND column_default='0')
);")
ledger_column_total=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons';")

state_columns=$(read_scalar "
SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state' AND (
 (ordinal_position=1 AND column_name='id' AND column_type='tinyint unsigned' AND is_nullable='NO') OR
 (ordinal_position=2 AND column_name='last_processed_at' AND data_type='datetime' AND is_nullable='YES' AND column_default IS NULL)
);")
state_column_total=$(read_scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state';")

parent_indexes=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) sig
 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_rewards'
 GROUP BY index_name,non_unique
 HAVING (index_name='PRIMARY' AND non_unique=0 AND sig='id')
     OR (index_name='idx_account_bound_rewards_account' AND non_unique=1 AND sig='account_name')
     OR (index_name='idx_account_bound_rewards_vnum' AND non_unique=1 AND sig='reward_vnum')
     OR (index_name='idx_account_bound_rewards_expires' AND non_unique=1 AND sig='expires_at')
) x;")
ledger_indexes=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) sig
 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_reward_summons'
 GROUP BY index_name,non_unique
 HAVING (index_name='PRIMARY' AND non_unique=0 AND sig='grant_id,pid')
     OR (index_name='idx_account_bound_reward_summons_pid' AND non_unique=1 AND sig='pid')
) x;")
state_indexes=$(read_scalar "
SELECT COUNT(*) FROM (
 SELECT index_name,non_unique,GROUP_CONCAT(column_name ORDER BY seq_in_index) sig
 FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='account_bound_reward_pwipe_state'
 GROUP BY index_name,non_unique
 HAVING index_name='PRIMARY' AND non_unique=0 AND sig='id'
) x;")
state_rows=$(read_scalar "SELECT COUNT(*) FROM account_bound_reward_pwipe_state WHERE id=1;")
state_row_total=$(read_scalar "SELECT COUNT(*) FROM account_bound_reward_pwipe_state;")

fk_exact=$(read_scalar "
SELECT COUNT(*) FROM information_schema.key_column_usage kcu
JOIN information_schema.referential_constraints rc ON rc.constraint_schema=kcu.constraint_schema AND rc.table_name=kcu.table_name AND rc.constraint_name=kcu.constraint_name
WHERE kcu.constraint_schema=DATABASE() AND rc.update_rule='NO ACTION' AND rc.delete_rule='CASCADE' AND (
 (kcu.table_name='account_bound_rewards' AND kcu.constraint_name='account_bound_rewards_ibfk_1' AND kcu.column_name='account_name' AND kcu.referenced_table_name='accounts' AND kcu.referenced_column_name='account_name') OR
 (kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_grant_fk' AND kcu.column_name='grant_id' AND kcu.referenced_table_name='account_bound_rewards' AND kcu.referenced_column_name='id') OR
 (kcu.table_name='account_bound_reward_summons' AND kcu.constraint_name='account_bound_reward_summons_pid_fk' AND kcu.column_name='pid' AND kcu.referenced_table_name='player_data' AND kcu.referenced_column_name='pid')
);")

failed=0
[[ "$parent_table" == 1 && "$ledger_table" == 1 && "$state_table" == 1 ]] || { echo 'FAILED: account reward table engine/collation mismatch' >&2; failed=1; }
[[ "$parent_columns" == 11 && "$parent_column_total" == 11 ]] || { echo "FAILED: expected 11 exact account_bound_rewards columns; exact=$parent_columns total=$parent_column_total" >&2; failed=1; }
[[ "$ledger_columns" == 4 && "$ledger_column_total" == 4 ]] || { echo "FAILED: expected 4 exact summon columns; exact=$ledger_columns total=$ledger_column_total" >&2; failed=1; }
[[ "$state_columns" == 2 && "$state_column_total" == 2 ]] || { echo "FAILED: expected 2 exact pwipe state columns; exact=$state_columns total=$state_column_total" >&2; failed=1; }
[[ "$parent_indexes" == 4 ]] || { echo "FAILED: expected 4 exact reward indexes; found $parent_indexes" >&2; failed=1; }
[[ "$ledger_indexes" == 2 ]] || { echo "FAILED: expected 2 exact summon indexes; found $ledger_indexes" >&2; failed=1; }
[[ "$state_indexes" == 1 ]] || { echo "FAILED: expected 1 exact pwipe state index; found $state_indexes" >&2; failed=1; }
[[ "$state_rows" == 1 && "$state_row_total" == 1 ]] || { echo "FAILED: expected exactly singleton pwipe state row id=1; id1=$state_rows total=$state_row_total" >&2; failed=1; }
[[ "$fk_exact" == 3 ]] || { echo "FAILED: expected 3 exact cascade FKs; found $fk_exact" >&2; failed=1; }
[[ "$failed" == 0 ]] || exit 1
printf 'account reward schema verified: 3 tables, 17 columns, 7 indexes, 3 cascade FKs, 1 pwipe guard row\n'
