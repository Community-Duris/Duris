#!/usr/bin/env bash
set -euo pipefail
# Self-contained0061 verifier: full exact baseline metadata, including0058 and both EAB formats.
# Engine POST fingerprints are measured; never source checkout .env.
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
if [[ -n "${DB_SOCKET:-}" ]]; then
    [[ "$DB_SOCKET" == /* ]] || { echo 'database socket must be absolute' >&2; exit 1; }
    CONNECTION=(--protocol=socket --socket="$DB_SOCKET")
else
    CONNECTION=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}")
    help=$(mysql --no-defaults --help)
    if [[ "$DB_HOST" == 127.0.0.1 || "$DB_HOST" == localhost || "$DB_HOST" == ::1 ]]; then
        if [[ "$help" == *--ssl-mode* ]]; then CONNECTION+=(--ssl-mode=PREFERRED); else CONNECTION+=(--skip-ssl); fi
    else
        [[ "${DB_TLS:-}" == TRUE && -f "${DB_SSL_CA:-}" ]] || { echo 'remote verification requires TLS and a CA file' >&2; exit 1; }
        if [[ "$help" == *--ssl-mode* ]]; then
            CONNECTION+=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
        elif [[ "$help" == *--ssl-verify-server-cert* ]]; then
            CONNECTION+=(--ssl-verify-server-cert --ssl-ca="$DB_SSL_CA")
        else
            echo 'database client cannot verify remote identity' >&2; exit 1
        fi
    fi
fi
MYSQL=(timeout 30 mysql --no-defaults --connect-timeout=10 "${CONNECTION[@]}" -u "$DB_USER" -N -B --raw "$DB_NAME")
version=$("${MYSQL[@]}" -e 'SELECT VERSION();')
if [[ "$version" == 10.11.*MariaDB* ]]; then
    expected=31f31065ee4e95d08049748eb49372a71ab7569f2f775aaaa9b1dbebf0978272
    checks=$("${MYSQL[@]}" -e 'SELECT @@SESSION.check_constraint_checks;')
    [[ "$checks" == 1 ]] || { echo 'baseline equipment schema metadata fingerprint mismatch: disabled checks' >&2; exit 1; }
elif [[ "$version" == 8.0.* && "$version" != *MariaDB* ]]; then
    expected=858f37fb10734428601d4a10bacb1892154d6fb77cd656fe8a90a004ef00016d
else
    echo 'unsupported database engine for baseline equipment schema' >&2; exit 1
fi
[[ "$expected" =~ ^[0-9a-f]{64}$ ]] || {
    echo '0061 baseline metadata awaits actual engine measurement' >&2; exit 1
}
query=$(cat <<'DURIS_BASELINE_V2_METADATA_SQL'
SET @baseline_v2_engine = CASE WHEN VERSION() LIKE '10.11.%MariaDB%' THEN 'mariadb'
    WHEN VERSION() LIKE '8.0.%' AND LOCATE('MariaDB',VERSION())=0 THEN 'mysql' ELSE NULL END;
SET @baseline_v2_previous_concat = @@SESSION.group_concat_max_len;
SET SESSION group_concat_max_len=65536;
SET @baseline_v2_read = IF(@baseline_v2_engine='mysql',
'SELECT COUNT(*),COUNT(metadata_row),COALESCE(SUM(OCTET_LENGTH(metadata_row)+1),0),COALESCE(OCTET_LENGTH(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10))),0),SHA2(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10)),256) INTO @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,@baseline_v2_actual_bytes,@baseline_v2_actual FROM (SELECT 0 AS phase,metadata_row FROM (SELECT CONCAT(''T'',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) AS metadata_row
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type=''BASE TABLE'' AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''C'',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,''<NULL>''),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''I'',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation''))
UNION ALL
SELECT CONCAT(''F'',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT(''K'',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND t.constraint_type=''CHECK'') AS ordinary
UNION ALL SELECT 1 AS phase,metadata_row FROM (SELECT CONCAT(''E'',CHAR(9),table_name,CHAR(9),constraint_name,CHAR(9),enforced) AS metadata_row FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND constraint_type=''CHECK'') AS enforcement) AS canonical_metadata',
'SELECT COUNT(*),COUNT(metadata_row),COALESCE(SUM(OCTET_LENGTH(metadata_row)+1),0),COALESCE(OCTET_LENGTH(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10))),0),SHA2(CONCAT(GROUP_CONCAT(metadata_row ORDER BY phase,metadata_row SEPARATOR ''
''),CHAR(10)),256) INTO @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,@baseline_v2_actual_bytes,@baseline_v2_actual FROM (SELECT 0 AS phase,metadata_row FROM (SELECT CONCAT(''T'',CHAR(9),table_name,CHAR(9),engine,CHAR(9),table_collation) AS metadata_row
FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type=''BASE TABLE'' AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''C'',CHAR(9),table_name,CHAR(9),column_name,CHAR(9),ordinal_position,
 CHAR(9),column_type,CHAR(9),is_nullable,CHAR(9),COALESCE(character_maximum_length,0),
 CHAR(9),COALESCE(numeric_precision,0),CHAR(9),COALESCE(numeric_scale,0),
 CHAR(9),COALESCE(datetime_precision,0),CHAR(9),COALESCE(column_default,''<NULL>''),CHAR(9),extra)
FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'')
UNION ALL
SELECT CONCAT(''I'',CHAR(9),table_name,CHAR(9),index_name,CHAR(9),non_unique,CHAR(9),seq_in_index,
 CHAR(9),column_name,CHAR(9),COALESCE(sub_part,0),CHAR(9),index_type)
FROM information_schema.statistics WHERE table_schema=DATABASE() AND (table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation''))
UNION ALL
SELECT CONCAT(''F'',CHAR(9),k.table_name,CHAR(9),k.constraint_name,CHAR(9),k.column_name,
 CHAR(9),k.referenced_table_name,CHAR(9),k.referenced_column_name,CHAR(9),k.ordinal_position,
 CHAR(9),r.update_rule,CHAR(9),r.delete_rule)
FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r
 ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name
WHERE k.constraint_schema=DATABASE() AND k.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND k.referenced_table_name IS NOT NULL
UNION ALL
SELECT CONCAT(''K'',CHAR(9),t.table_name,CHAR(9),t.constraint_name,CHAR(9),c.check_clause)
FROM information_schema.table_constraints t JOIN information_schema.check_constraints c
 ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
WHERE t.constraint_schema=DATABASE() AND t.table_name IN (''economic_baseline_control'',''economic_baseline_witness'',''economic_baseline_reservation'') AND t.constraint_type=''CHECK'') AS ordinary) AS canonical_metadata');
PREPARE baseline_v2_stmt FROM @baseline_v2_read;
EXECUTE baseline_v2_stmt;
DEALLOCATE PREPARE baseline_v2_stmt;
SET SESSION group_concat_max_len=@baseline_v2_previous_concat;
SELECT @baseline_v2_row_count,@baseline_v2_nonnull_rows,@baseline_v2_expected_bytes,
       @baseline_v2_actual_bytes,@baseline_v2_actual;
DURIS_BASELINE_V2_METADATA_SQL
)
metadata=$("${MYSQL[@]}" -e "$query")
pattern=$'^[0-9]+\t[0-9]+\t[0-9]+\t[0-9]+\t[0-9a-f]{64}$'
[[ "$metadata" =~ $pattern ]] || { echo 'baseline equipment schema metadata fingerprint mismatch: invalid aggregate' >&2; exit 1; }
IFS=$'\t' read -r rows nonnull expected_bytes actual_bytes actual <<< "$metadata"
# Counts come from bounded metadata, not canonical witness fixtures. Compare
# decimal strings via regex first, then bounded base10 arithmetic (no octal).
[[ ${#rows} -le 4 && ${#nonnull} -le 4 && ${#expected_bytes} -le 5 && ${#actual_bytes} -le 5 ]] || {
    echo 'baseline equipment schema metadata fingerprint mismatch: unbounded aggregate' >&2; exit 1
}
(( 10#$rows >= 1 && 10#$rows <= 4096 && 10#$nonnull == 10#$rows &&
   10#$expected_bytes >= 1 && 10#$expected_bytes <= 65536 &&
   10#$expected_bytes == 10#$actual_bytes )) || {
    echo 'baseline equipment schema metadata fingerprint mismatch: NULL or truncated aggregate' >&2; exit 1
}
[[ "$actual" == "$expected" ]] || { echo 'baseline equipment schema metadata fingerprint mismatch' >&2; exit 1; }
printf 'baseline equipment schema verified: exact EAB1/EAB2 CHECK and original baseline metadata\n'
