#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
read_scalar() { "${MYSQL[@]}" -e "$1"; }

tables=$(read_scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('boon_reward_outcome','boon_reward_outcome_entry','zone_touch_outcome','zone_touch_outcome_participant') AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
indexes=$(read_scalar "SELECT COUNT(*) FROM (SELECT table_name,index_name FROM information_schema.statistics WHERE table_schema=DATABASE() AND ((table_name='boon_reward_outcome' AND index_name='PRIMARY') OR (table_name='boon_reward_outcome_entry' AND index_name IN ('PRIMARY','uq_boon_reward_operation_boon')) OR (table_name='zone_touch_outcome' AND index_name='PRIMARY') OR (table_name='zone_touch_outcome_participant' AND index_name IN ('PRIMARY','uq_zone_touch_operation_pid'))) GROUP BY table_name,index_name) exact_indexes;")
foreign_keys=$(read_scalar "SELECT COUNT(*) FROM information_schema.referential_constraints WHERE constraint_schema=DATABASE() AND constraint_name IN ('boon_reward_operation_fk','boon_reward_entry_operation_fk','zone_touch_outcome_operation_fk','zone_touch_participant_operation_fk');")
[[ "$tables" == "4" && "$indexes" == "6" && "$foreign_keys" == "4" ]]
echo "boon reward and zone-touch outcome schema verified"
