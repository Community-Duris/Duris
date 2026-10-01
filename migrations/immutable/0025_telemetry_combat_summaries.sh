#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}" "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
MYSQL_BIN=${MYSQL_BIN:-mysql}
if "$MYSQL_BIN" --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=("$MYSQL_BIN" "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }
check() {
    local label=$1 expected=$2 sql=$3 actual
    actual=$(scalar "SET SESSION group_concat_max_len=32768; $sql")
    if [[ "$actual" != "$expected" ]]; then
        printf 'FAILED: %s differs\nexpected: %s\nactual: %s\n' "$label" "$expected" "$actual" >&2
        exit 1
    fi
}

check combat_columns '40' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name IN ('combat_encounter_boot_id','combat_encounter_process_id','combat_encounter_seq','combat_mode','combat_outcome','combat_revision','combat_environment_id','combat_season_id','combat_config_id','combat_classifier_version','combat_policy_version','combat_zone_vnum','combat_group_key','combat_actor_id','combat_actor_pid','combat_owner_subject_id','combat_actor_kind','combat_unique_player_count','combat_participant_count','combat_dropped_participant_count','combat_power_band','combat_opponent_power_band','combat_opponent_count','combat_modifier_flags','combat_start_monotonic_usec','combat_end_monotonic_usec','combat_start_utc_usec','combat_end_utc_usec','combat_damage_dealt','combat_damage_taken','combat_healing_attempted','combat_effective_healing','combat_overhealing','combat_control_applications','combat_casting_attempts','combat_casting_completions','combat_casting_aborts','combat_casting_elapsed_usec','combat_tanking_usec','combat_quality_flags');"
check combat_indexes 'uq_telemetry_combat_summary' \
    "SELECT GROUP_CONCAT(DISTINCT index_name ORDER BY BINARY index_name SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_combat_summary';"
check combat_index_columns 'combat_encounter_boot_id|combat_encounter_process_id|combat_encounter_seq|combat_actor_kind|combat_actor_id|combat_actor_pid|combat_revision' \
    "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_combat_summary';"

printf 'telemetry combat summary schema verified\n'
