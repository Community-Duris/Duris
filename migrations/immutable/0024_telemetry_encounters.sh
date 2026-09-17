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

check encounter_columns '25' \
    "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name IN ('encounter_boot_id','encounter_process_id','encounter_seq','encounter_event','encounter_mode','encounter_outcome','encounter_revision','encounter_environment_id','encounter_season_id','encounter_config_id','encounter_classifier_version','encounter_policy_version','encounter_zone_vnum','encounter_group_key','encounter_participant_subject_id','encounter_participant_pid','at_monotonic_usec','at_utc_usec','encounter_start_monotonic_usec','encounter_start_utc_usec','elapsed_usec','participant_usec','participant_count','expected_credit_count','encounter_quality_flags');"
check encounter_indexes 'uq_telemetry_encounter_event' \
    "SELECT GROUP_CONCAT(DISTINCT index_name ORDER BY BINARY index_name SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_encounter_event';"
check encounter_index_columns 'encounter_boot_id|encounter_process_id|encounter_seq|encounter_event|encounter_revision|encounter_participant_pid' \
    "SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND index_name='uq_telemetry_encounter_event';"

printf 'telemetry encounter schema verified\n'
