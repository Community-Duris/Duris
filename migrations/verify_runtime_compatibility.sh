#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SCHEMA_ONLY=0
if [[ $# == 1 && "$1" == --schema-only ]]; then
    SCHEMA_ONLY=1
elif [[ $# != 0 ]]; then
    echo "usage: verify_runtime_compatibility.sh [--schema-only]" >&2
    exit 2
fi
MANIFEST="${RUNTIME_COMPATIBILITY_MANIFEST:-$SCRIPT_DIR/runtime_compatibility_manifest.json}"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if [[ -n "${DB_SOCKET:-}" ]]; then
    [[ "$DB_SOCKET" == /* ]] || { echo "DB_SOCKET must be an absolute path" >&2; exit 1; }
    MYSQL_CONNECTION=(--protocol=socket --socket="$DB_SOCKET")
else
    if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
    MYSQL_CONNECTION=("${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}")
fi
MYSQL=(mysql "${MYSQL_CONNECTION[@]}" -u "$DB_USER" -N -B --raw "$DB_NAME")
extract_string() { sed -n "s/.*\"$1\": \"\([^\"]*\)\".*/\1/p" "$MANIFEST" | head -1; }
extract_number() { sed -n "s/.*\"$1\": \([0-9][0-9]*\).*/\1/p" "$MANIFEST" | head -1; }
expected=(
    "$(extract_number current_table_count)"
    "$(extract_string mysql8)"
    "$(extract_string mariadb10_11)"
    "$(extract_string baseline_id)"
    "$(extract_string baseline_table_fingerprint)"
    "$(extract_string id)"
    "$(extract_number sequence)"
    "$(extract_string apply_checksum)"
    "$(extract_string verify_checksum)"
    "$(extract_string history_checksum)"
)
[[ ${#expected[@]} == 10 ]]
[[ "${expected[1]}" =~ ^[0-9a-f]{64}$ && "${expected[2]}" =~ ^[0-9a-f]{64}$ ]] || {
    echo "FAILED: 0061 runtime metadata fingerprints await actual engine measurement" >&2
    exit 1
}
# Scope alternate fields to their object; the unqualified extractor selects
# the canonical head. All histories are pinned by the offline validator.
extract_head() {
    sed -n "/\"$1\": {/,/}/p" "$MANIFEST" |
        sed -n "s/.*\"$2\": \([^,]*\).*/\1/p" | tr -d '"\r'
}
staging=("$(extract_head staging_0045_migration_head id)" "$(extract_head staging_0045_migration_head sequence)"
         "$(extract_head staging_0045_migration_head apply_checksum)" "$(extract_head staging_0045_migration_head verify_checksum)"
         "$(extract_head staging_0045_migration_head history_checksum)")
master=("$(extract_head master_0031_migration_head id)" "$(extract_head master_0031_migration_head sequence)"
        "$(extract_head master_0031_migration_head apply_checksum)" "$(extract_head master_0031_migration_head verify_checksum)"
        "$(extract_head master_0031_migration_head history_checksum)")
history_query=$(extract_string migration_history_sql)
[[ -n "$history_query" ]]
# Decode directly into the pipe: shell variables cannot preserve NUL bytes in
# the eight-byte length prefixes. Bound rows and total bytes before hashing.
history_digest=$("${MYSQL[@]}" -e "$history_query" |
    (
        rows=0; bytes=0
        while IFS= read -r record; do
            [[ "$record" =~ ^([0-9A-F]{2})+$ ]] || exit 1
            rows=$((rows + 1)); bytes=$((bytes + ${#record} / 2))
            [[ "$rows" -le "${expected[6]}" && "$bytes" -le 4194304 ]] || exit 1
            printf '%b' "$(printf '%s' "$record" | sed 's/../\\x&/g')"
        done
        [[ "$rows" == "${expected[6]}" ]]
    ) | sha256sum | cut -d' ' -f1)
if [[ "$history_digest" != "${expected[9]}" && "$history_digest" != "${staging[4]}" && "$history_digest" != "${master[4]}" ]]; then
    echo "FAILED: full immutable migration history mismatch" >&2
    exit 1
fi
runtime_tables=$(extract_string runtime_table_sql_list)
[[ -n "$runtime_tables" ]]
# The existing fingerprint's F row names the referenced table but not its
# schema. All accepted runtime foreign keys are same-schema; reject a redirect
# before hashing so a same-named external table cannot preserve the digest.
foreign_schema_count=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.key_column_usage k WHERE k.constraint_schema=DATABASE() AND k.referenced_table_name IS NOT NULL AND (k.table_name IN ($runtime_tables) OR k.referenced_table_name IN ($runtime_tables)) AND (k.referenced_table_schema IS NULL OR BINARY k.referenced_table_schema <> BINARY DATABASE());")
if [[ ! "$foreign_schema_count" =~ ^[0-9]+$ || "$foreign_schema_count" != 0 ]]; then
    printf 'FAILED: cross-schema runtime foreign key is not in the pinned contract\n' >&2
    exit 1
fi
# The archived website-owned user_profile_stats -> accounts CASCADE edge is an
# exact, pre-existing cross-boundary dependency. Exclude only that one row from
# the pinned MUD fingerprint; unknown or changed inbound FKs still fail closed.
query="SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($runtime_tables) UNION ALL SELECT CONCAT('C',CHAR(9),c.table_name,CHAR(9),c.column_name,CHAR(9),c.ordinal_position,CHAR(9),c.data_type,CHAR(9),c.is_nullable,CHAR(9),COALESCE(c.character_maximum_length,0),CHAR(9),COALESCE(c.numeric_precision,0),CHAR(9),COALESCE(c.numeric_scale,0),CHAR(9),COALESCE(c.datetime_precision,0),CHAR(9),CASE WHEN c.column_default IS NULL THEN '<NULL>' WHEN UPPER(c.column_default) LIKE 'CURRENT_TIMESTAMP%' THEN 'CURRENT_TIMESTAMP' ELSE TRIM(BOTH '\'' FROM c.column_default) END,CHAR(9),CONCAT(IF(LOWER(c.extra) LIKE '%auto_increment%','A',''),IF(LOWER(c.extra) LIKE '%on update%','U',''),IF(LOWER(c.extra) LIKE '%generated%','G',''))) FROM information_schema.columns c JOIN information_schema.tables t ON t.table_schema=c.table_schema AND t.table_name=c.table_name AND t.table_type='BASE TABLE' WHERE c.table_schema=DATABASE() AND c.table_name IN ($runtime_tables) UNION ALL SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0)) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name IN ($runtime_tables) UNION ALL SELECT CONCAT('F',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,CHAR(9),r.update_rule,CHAR(9),r.delete_rule) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND (k.table_name IN ($runtime_tables) OR k.referenced_table_name IN ($runtime_tables)) AND k.referenced_table_name IS NOT NULL AND NOT (BINARY k.table_name='user_profile_stats' AND BINARY k.constraint_name='user_profile_stats_ibfk_1' AND BINARY k.column_name='account_name' AND BINARY k.referenced_table_schema=BINARY DATABASE() AND BINARY k.referenced_table_name='accounts' AND BINARY k.referenced_column_name='account_name' AND k.ordinal_position=1 AND r.update_rule IN ('NO ACTION','RESTRICT') AND r.delete_rule='CASCADE')"
server_version=$("${MYSQL[@]}" -e "SELECT VERSION();")
query+=" UNION ALL SELECT CONCAT('X',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),column_type) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN ('economic_baseline_control','economic_baseline_reservation','economic_baseline_witness','economic_sql_lifecycle_installation','economic_sql_activation_receipt','economic_sql_global_activation','sql_room_item_payload','shopkeepers','shopkeeper_item_runtime_state','quest_mobile_native','item_owner_revision','item_current_owner','item_ownership_baseline') UNION ALL SELECT CONCAT('K',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.constraint_type='CHECK' AND t.table_name IN ('economic_baseline_control','economic_baseline_reservation','economic_baseline_witness','economic_sql_lifecycle_installation','economic_sql_activation_receipt','economic_sql_global_activation','sql_room_item_payload','shopkeepers','shopkeeper_item_runtime_state','quest_mobile_native','item_owner_revision','item_current_owner','item_ownership_baseline')"
if [[ "$server_version" != *MariaDB* ]]; then
    query+=" UNION ALL SELECT CONCAT('E',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),enforced) FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND constraint_type='CHECK' AND table_name IN ('economic_baseline_control','economic_baseline_reservation','economic_baseline_witness','economic_sql_lifecycle_installation','economic_sql_activation_receipt','economic_sql_global_activation','sql_room_item_payload','shopkeepers','shopkeeper_item_runtime_state','quest_mobile_native','item_owner_revision','item_current_owner','item_ownership_baseline')"
fi
query+=" ORDER BY 1;"
fingerprint=$("${MYSQL[@]}" -e "$query" | sha256sum | cut -d' ' -f1)
server_version=$("${MYSQL[@]}" -e "SELECT VERSION();")
if [[ "$server_version" == *MariaDB* ]]; then metadata_fingerprint="${expected[2]}"; else metadata_fingerprint="${expected[1]}"; fi
tables=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($runtime_tables);")
transactional=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($runtime_tables) AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
baseline=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_baselines WHERE baseline_id='${expected[3]}' AND LOWER(HEX(schema_fingerprint))='${expected[4]}' AND manifest_version=1 AND runner_version=1;")
head=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_history WHERE migration_id='${expected[5]}' AND sequence_number=${expected[6]} AND LOWER(HEX(apply_checksum))='${expected[7]}' AND LOWER(HEX(verify_checksum))='${expected[8]}' AND runner_version=1 OR migration_id='${staging[0]}' AND sequence_number=${staging[1]} AND LOWER(HEX(apply_checksum))='${staging[2]}' AND LOWER(HEX(verify_checksum))='${staging[3]}' AND runner_version=1 OR migration_id='${master[0]}' AND sequence_number=${master[1]} AND LOWER(HEX(apply_checksum))='${master[2]}' AND LOWER(HEX(verify_checksum))='${master[3]}' AND runner_version=1;")
state=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_migration_state WHERE state_id=1 AND applied_count=${expected[6]} AND LOWER(HEX(history_checksum))='$history_digest';")
description_columns=$("${MYSQL[@]}" -e "$(extract_string extra_description_generation_sql)")
level_cap=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM level_cap WHERE id=1 AND most_frags>=0 AND racewar_leader BETWEEN 0 AND 4 AND level BETWEEN 1 AND 56 AND next_update IS NOT NULL AND (SELECT COUNT(*) FROM level_cap)=1;")
failed=0
[[ "$description_columns" == 2 ]] || {
    echo "FAILED: item-description digest expression mismatch" >&2; failed=1;
}
[[ "$fingerprint" == "$metadata_fingerprint" ]] || {
    echo "FAILED: normalized metadata fingerprint mismatch: expected=$metadata_fingerprint actual=$fingerprint" >&2
    failed=1
}
[[ "$tables" == "${expected[0]}" ]] || {
    echo "FAILED: expected ${expected[0]} runtime tables; found $tables" >&2
    failed=1
}
[[ "$transactional" == "${expected[0]}" ]] || {
    echo "FAILED: expected ${expected[0]} InnoDB utf8mb4 tables; found $transactional" >&2
    failed=1
}
[[ "$baseline" == 1 ]] || { echo "FAILED: sealed baseline is absent or stale" >&2; failed=1; }
[[ "$head" == 1 ]] || { echo "FAILED: immutable migration head is absent or stale" >&2; failed=1; }
[[ "$state" == 1 ]] || { echo "FAILED: immutable migration state is absent or stale" >&2; failed=1; }
[[ "$level_cap" == 1 ]] || { echo "FAILED: required level-cap singleton is absent or invalid" >&2; failed=1; }
if [[ "$SCHEMA_ONLY" == 0 ]]; then
    character_baselines=$("${MYSQL[@]}" -e "SELECT COUNT(*),COALESCE(SUM(wallet.pid IS NULL),0),COALESCE(SUM(epic.pid IS NULL),0),COALESCE(SUM(combat.pid IS NULL),0) FROM (SELECT DISTINCT player.pid FROM player_data player JOIN account_characters mapping ON mapping.pid=player.pid WHERE player.active=1 AND mapping.deleted_at IS NULL AND mapping.blocked=0) eligible LEFT JOIN currency_wallet_baseline wallet ON wallet.pid=eligible.pid LEFT JOIN epic_balance_baseline epic ON epic.pid=eligible.pid LEFT JOIN combat_frag_baseline combat ON combat.pid=eligible.pid;")
    IFS=$'\t' read -r eligible_characters wallet_missing epic_missing combat_missing <<<"$character_baselines"
    if ! [[ "$eligible_characters" =~ ^[0-9]+$ && "$wallet_missing" =~ ^[0-9]+$ &&
            "$epic_missing" =~ ^[0-9]+$ && "$combat_missing" =~ ^[0-9]+$ ]]; then
        echo "FAILED: character baseline readiness returned malformed aggregate output" >&2
        failed=1
    elif [[ "$wallet_missing" != 0 || "$epic_missing" != 0 || "$combat_missing" != 0 ]]; then
        printf 'FAILED: character baseline readiness eligible=%s wallet_missing=%s epic_missing=%s combat_missing=%s\n' \
            "$eligible_characters" "$wallet_missing" "$epic_missing" "$combat_missing" >&2
        failed=1
    fi
fi
[[ "$failed" == 0 ]] || exit 1
if [[ "$SCHEMA_ONLY" == 1 ]]; then
    echo "immutable migration head, schema metadata, engine, collation, index, and FK compatibility verified; character baselines not checked"
else
    echo "runtime migration, schema metadata, baseline readiness, engine, collation, index, and FK compatibility verified"
fi
