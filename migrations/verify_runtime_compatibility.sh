#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
MANIFEST="${RUNTIME_COMPATIBILITY_MANIFEST:-$SCRIPT_DIR/runtime_compatibility_manifest.json}"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
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
query="SELECT CONCAT('T',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' UNION ALL SELECT CONCAT('C',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,CHAR(9),data_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),CHAR(9),COALESCE(datetime_precision,0),CHAR(9),CASE WHEN column_default IS NULL THEN '<NULL>' WHEN UPPER(column_default) LIKE 'CURRENT_TIMESTAMP%' THEN 'CURRENT_TIMESTAMP' ELSE TRIM(BOTH '\'' FROM column_default) END,CHAR(9),CONCAT(IF(LOWER(extra) LIKE '%auto_increment%','A',''),IF(LOWER(extra) LIKE '%on update%','U',''),IF(LOWER(extra) LIKE '%generated%','G',''))) FROM information_schema.columns WHERE table_schema=DATABASE() UNION ALL SELECT CONCAT('I',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0)) FROM information_schema.statistics WHERE table_schema=DATABASE() UNION ALL SELECT CONCAT('F',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,CHAR(9),r.update_rule,CHAR(9),r.delete_rule) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.referenced_table_name IS NOT NULL ORDER BY 1;"
fingerprint=$("${MYSQL[@]}" -e "$query" | sha256sum | cut -d' ' -f1)
server_version=$("${MYSQL[@]}" -e "SELECT VERSION();")
if [[ "$server_version" == *MariaDB* ]]; then metadata_fingerprint="${expected[2]}"; else metadata_fingerprint="${expected[1]}"; fi
tables=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE';")
transactional=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND engine='InnoDB' AND table_collation='utf8mb4_unicode_ci';")
baseline=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_baselines WHERE baseline_id='${expected[3]}' AND LOWER(HEX(schema_fingerprint))='${expected[4]}' AND manifest_version=1 AND runner_version=1;")
head=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_history WHERE migration_id='${expected[5]}' AND sequence_number=${expected[6]} AND LOWER(HEX(apply_checksum))='${expected[7]}' AND LOWER(HEX(verify_checksum))='${expected[8]}' AND runner_version=1;")
state=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM mud_schema_migration_state WHERE state_id=1 AND applied_count=${expected[6]} AND LOWER(HEX(history_checksum))='${expected[9]}';")
[[ "$fingerprint" == "$metadata_fingerprint" && "$tables" == "${expected[0]}" && \
   "$transactional" == "${expected[0]}" && "$baseline" == 1 && "$head" == 1 && \
   "$state" == 1 ]]
echo "runtime migration, schema metadata, engine, collation, index, and FK compatibility verified"
