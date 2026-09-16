#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?}" "${DB_USER:?}" "${DB_PASSWD:?}" "${DB_NAME:?}"
export MYSQL_PWD="$DB_PASSWD"
fail_connection() { echo "FAILED: telemetry rollup verifier $1" >&2; exit 1; }
environment="${ENVIRONMENT:-}"
environment="${environment,,}"
local_host=0
case "$DB_HOST" in 127.0.0.1|localhost|::1) local_host=1 ;; esac
if [[ "$environment" != production ]]; then
    case "$environment" in local|development|dev|test) ;; *) fail_connection "requires an explicit migration environment" ;; esac
    [[ "$local_host" == 1 ]] || fail_connection "non-production target must be loopback"
    [[ ! "${DB_NAME,,}" =~ (^|[_-])prod(uction)?($|[_-]) ]] || fail_connection "non-production target cannot name a production database"
fi
if [[ -n "${DB_SOCKET:-}" ]]; then
    [[ "$environment" != production && "$local_host" == 1 ]] || fail_connection "socket requires a local non-production target"
    [[ "$DB_SOCKET" == /* ]] || fail_connection "DB_SOCKET must be an absolute path"
    MYSQL_CONNECTION=(--protocol=socket "--socket=$DB_SOCKET")
else
    MYSQL_CONNECTION=(--protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}")
    if [[ "$local_host" != 1 ]]; then
        [[ "${DB_TLS:-}" == TRUE && -f "${DB_SSL_CA:-}" ]] || fail_connection "remote target requires TLS and a regular CA file"
        mysql_help=$(mysql --help) || fail_connection "cannot inspect database client TLS capabilities"
        [[ "$mysql_help" == *--ssl-ca* ]] || fail_connection "database client cannot configure a remote CA file"
        if [[ "$mysql_help" == *--ssl-mode* ]]; then
            MYSQL_CONNECTION+=(--ssl-mode=VERIFY_IDENTITY "--ssl-ca=$DB_SSL_CA")
        elif [[ "$mysql_help" == *--ssl-verify-server-cert* ]]; then
            MYSQL_CONNECTION+=("--ssl-ca=$DB_SSL_CA" --ssl-verify-server-cert)
        else
            fail_connection "database client cannot verify remote server identity"
        fi
    fi
fi
MYSQL=(mysql "${MYSQL_CONNECTION[@]}" -u "$DB_USER" -N -B "$DB_NAME")
NEW_TABLES=(telemetry_cohort_member telemetry_rollup_session)
NEW_TABLE_COUNT=${#NEW_TABLES[@]}
NEW_TABLE_SQL=$(printf "'%s'," "${NEW_TABLES[@]}")
NEW_TABLE_SQL=${NEW_TABLE_SQL%,}
NEW_TABLE_NAMES=$(printf "%s|" "${NEW_TABLES[@]}")
NEW_TABLE_NAMES=${NEW_TABLE_NAMES%|}
check() { local actual; actual=$("${MYSQL[@]}" -e "SET SESSION group_concat_max_len=65536; $2"); [[ "$actual" == "$1" ]] || { echo "FAILED: telemetry rollup $3 differs" >&2; exit 1; }; }
normalize_check_clause() {
    local clause="$1"
    clause=$(printf '%s' "$clause" | sed 's/`//g' | tr '\r\n\t' '   ' | tr -s ' ')
    clause="${clause,,}"
    clause="${clause# }"
    clause="${clause% }"
    printf '%s' "$clause"
}
KIND_CHECK_MYSQL='(((membership_kind = 1) and (session_boot_id = 0) and (session_process_id = 0) and (session_seq = 0)) or ((membership_kind = 2) and (session_boot_id <> 0) and (session_process_id <> 0) and (session_seq <> 0)))'
KIND_CHECK_MARIADB='membership_kind = 1 and session_boot_id = 0 and session_process_id = 0 and session_seq = 0 or membership_kind = 2 and session_boot_id <> 0 and session_process_id <> 0 and session_seq <> 0'
KIND_CHECK_CANONICAL='(membership_kind = 1 and session_boot_id = 0 and session_process_id = 0 and session_seq = 0) or (membership_kind = 2 and session_boot_id <> 0 and session_process_id <> 0 and session_seq <> 0)'
SUBJECT_CHECK_MYSQL='(subject_id <> 0)'
SUBJECT_CHECK_MARIADB='subject_id <> 0'
SUBJECT_CHECK_CANONICAL='subject_id <> 0'
canonical_check_clause() {
    local name="$1" clause="$2"
    case "$name" in
        chk_telemetry_cohort_member_kind)
            if [[ "$clause" == "$KIND_CHECK_MYSQL" || "$clause" == "$KIND_CHECK_MARIADB" ]]; then
                printf '%s' "$KIND_CHECK_CANONICAL"
                return 0
            fi
            ;;
        chk_telemetry_cohort_member_subject)
            if [[ "$clause" == "$SUBJECT_CHECK_MYSQL" || "$clause" == "$SUBJECT_CHECK_MARIADB" ]]; then
                printf '%s' "$SUBJECT_CHECK_CANONICAL"
                return 0
            fi
            ;;
    esac
    return 1
}
check_check_constraints() {
    local server_version is_mariadb=0 query actual table name clause enforced normalized canonical
    local row_count=0 kind_canonical='' subject_canonical='' kind_enforced='' subject_enforced=''
    server_version=$("${MYSQL[@]}" -e 'SELECT VERSION();') || { echo 'FAILED: telemetry rollup server version lookup' >&2; exit 1; }
    [[ "$server_version" == *MariaDB* ]] && is_mariadb=1
    if [[ "$is_mariadb" == 1 ]]; then
        query="SELECT c.table_name,c.constraint_name,c.check_clause,'UNAVAILABLE' FROM information_schema.check_constraints c JOIN information_schema.table_constraints t ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name WHERE c.constraint_schema=DATABASE() AND c.table_name='telemetry_cohort_member' AND t.constraint_type='CHECK' ORDER BY BINARY c.constraint_name;"
    else
        query="SELECT t.table_name,c.constraint_name,c.check_clause,t.enforced FROM information_schema.check_constraints c JOIN information_schema.table_constraints t ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name WHERE c.constraint_schema=DATABASE() AND t.table_name='telemetry_cohort_member' AND t.constraint_type='CHECK' ORDER BY BINARY c.constraint_name;"
    fi
    if ! actual=$("${MYSQL[@]}" -e "$query"); then
        echo 'FAILED: telemetry rollup check expression metadata query' >&2
        exit 1
    fi
    while IFS=$'\t' read -r table name clause enforced; do
        [[ -n "$table" ]] || continue
        row_count=$((row_count + 1))
        [[ "$table" == telemetry_cohort_member ]] || { echo "FAILED: telemetry rollup unexpected CHECK table $table" >&2; exit 1; }
        normalized=$(normalize_check_clause "$clause")
        if ! canonical=$(canonical_check_clause "$name" "$normalized"); then
            echo "FAILED: telemetry rollup CHECK expression $name differs" >&2
            exit 1
        fi
        if [[ "$name" == chk_telemetry_cohort_member_kind ]]; then
            kind_canonical="$canonical"
            kind_enforced="$enforced"
        elif [[ "$name" == chk_telemetry_cohort_member_subject ]]; then
            subject_canonical="$canonical"
            subject_enforced="$enforced"
        else
            echo "FAILED: telemetry rollup unexpected CHECK constraint $name" >&2
            exit 1
        fi
    done <<< "$actual"
    [[ "$row_count" == 2 ]] || { echo "FAILED: telemetry rollup CHECK inventory differs" >&2; exit 1; }
    [[ "$kind_canonical" == "$KIND_CHECK_CANONICAL" && "$subject_canonical" == "$SUBJECT_CHECK_CANONICAL" ]] || { echo 'FAILED: telemetry rollup normalized CHECK expressions differ' >&2; exit 1; }
    if [[ "$is_mariadb" == 0 ]]; then
        [[ "$kind_enforced" == YES && "$subject_enforced" == YES ]] || { echo 'FAILED: telemetry rollup MySQL CHECK constraints are not enforced' >&2; exit 1; }
    fi
}
check "$NEW_TABLE_COUNT" "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($NEW_TABLE_SQL);" "table inventory"
check "$NEW_TABLE_NAMES" "SELECT GROUP_CONCAT(table_name ORDER BY table_name SEPARATOR '|') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($NEW_TABLE_SQL);" "table names"
check "telemetry_cohort_member:InnoDB:utf8mb4_unicode_ci|telemetry_rollup_session:InnoDB:utf8mb4_unicode_ci" "SELECT GROUP_CONCAT(CONCAT(table_name,':',engine,':',table_collation) ORDER BY table_name SEPARATOR '|') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND table_name IN ($NEW_TABLE_SQL);" "table engines/collations"
check "definition_version:int:unsigned:NO:<NULL>|generation:bigint:unsigned:NO:<NULL>|environment_id:bigint:unsigned:NO:<NULL>|season_id:bigint:unsigned:NO:<NULL>|utc_day:date:plain:NO:<NULL>|level_band:smallint:unsigned:NO:<NULL>|class_id:smallint:unsigned:NO:<NULL>|race_id:smallint:unsigned:NO:<NULL>|faction_id:smallint:unsigned:NO:<NULL>|zone_vnum:int:signed:NO:<NULL>|config_id:bigint:unsigned:NO:<NULL>|category:tinyint:unsigned:NO:<NULL>|membership_kind:tinyint:unsigned:NO:<NULL>|subject_id:bigint:unsigned:NO:<NULL>|session_boot_id:bigint:unsigned:NO:<NULL>|session_process_id:bigint:unsigned:NO:<NULL>|session_seq:bigint:unsigned:NO:<NULL>|duration_usec:bigint:unsigned:NO:0|attributable_usec:bigint:unsigned:NO:0|observed_intervals:bigint:unsigned:NO:0|quality_flags:int:unsigned:NO:0|input_watermark:bigint:unsigned:NO:0" "SELECT GROUP_CONCAT(CONCAT(column_name,':',LOWER(data_type),':',CASE WHEN data_type IN ('tinyint','smallint','mediumint','int','bigint') THEN IF(LOCATE('unsigned',LOWER(column_type))>0,'unsigned','signed') ELSE 'plain' END,':',is_nullable,':',COALESCE(column_default,'<NULL>')) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_cohort_member';" "telemetry_cohort_member columns/defaults"
check "definition_version:int:unsigned:NO:<NULL>|generation:bigint:unsigned:NO:<NULL>|environment_id:bigint:unsigned:NO:<NULL>|season_id:bigint:unsigned:NO:<NULL>|session_boot_id:bigint:unsigned:NO:<NULL>|session_process_id:bigint:unsigned:NO:<NULL>|session_seq:bigint:unsigned:NO:<NULL>|subject_id:bigint:unsigned:NO:<NULL>|pid:int:signed:NO:<NULL>|latest_checkpoint_revision:bigint:unsigned:NO:0|connected_usec:bigint:unsigned:NO:0|active_usec:bigint:unsigned:NO:0|idle_usec:bigint:unsigned:NO:0|unknown_usec:bigint:unsigned:NO:0|resident_usec:bigint:unsigned:NO:0|linkdead_usec:bigint:unsigned:NO:0|covered_connected_usec:bigint:unsigned:NO:0|covered_active_usec:bigint:unsigned:NO:0|covered_idle_usec:bigint:unsigned:NO:0|covered_unknown_usec:bigint:unsigned:NO:0|covered_resident_usec:bigint:unsigned:NO:0|covered_linkdead_usec:bigint:unsigned:NO:0|attributable_usec:bigint:unsigned:NO:0|observed_intervals:bigint:unsigned:NO:0|entered:tinyint:unsigned:NO:0|exited:tinyint:unsigned:NO:0|end_reason:tinyint:unsigned:NO:0|quality_flags:int:unsigned:NO:0|input_watermark:bigint:unsigned:NO:0|provisional:tinyint:unsigned:NO:1" "SELECT GROUP_CONCAT(CONCAT(column_name,':',LOWER(data_type),':',CASE WHEN data_type IN ('tinyint','smallint','mediumint','int','bigint') THEN IF(LOCATE('unsigned',LOWER(column_type))>0,'unsigned','signed') ELSE 'plain' END,':',is_nullable,':',COALESCE(column_default,'<NULL>')) ORDER BY ordinal_position SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_rollup_session';" "telemetry_rollup_session columns/defaults"
check "PRIMARY:0:1:definition_version:<NULL>|PRIMARY:0:2:generation:<NULL>|PRIMARY:0:3:environment_id:<NULL>|PRIMARY:0:4:season_id:<NULL>|PRIMARY:0:5:utc_day:<NULL>|PRIMARY:0:6:level_band:<NULL>|PRIMARY:0:7:class_id:<NULL>|PRIMARY:0:8:race_id:<NULL>|PRIMARY:0:9:faction_id:<NULL>|PRIMARY:0:10:zone_vnum:<NULL>|PRIMARY:0:11:config_id:<NULL>|PRIMARY:0:12:category:<NULL>|PRIMARY:0:13:subject_id:<NULL>|PRIMARY:0:14:session_boot_id:<NULL>|PRIMARY:0:15:session_process_id:<NULL>|PRIMARY:0:16:session_seq:<NULL>" "SELECT GROUP_CONCAT(CONCAT(index_name,':',non_unique,':',seq_in_index,':',column_name,':',COALESCE(sub_part,'<NULL>')) ORDER BY BINARY index_name,seq_in_index SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_cohort_member';" "telemetry_cohort_member indexes"
check "PRIMARY:0:1:definition_version:<NULL>|PRIMARY:0:2:generation:<NULL>|PRIMARY:0:3:environment_id:<NULL>|PRIMARY:0:4:season_id:<NULL>|PRIMARY:0:5:session_boot_id:<NULL>|PRIMARY:0:6:session_process_id:<NULL>|PRIMARY:0:7:session_seq:<NULL>|idx_rollup_session_subject:1:1:definition_version:<NULL>|idx_rollup_session_subject:1:2:generation:<NULL>|idx_rollup_session_subject:1:3:environment_id:<NULL>|idx_rollup_session_subject:1:4:season_id:<NULL>|idx_rollup_session_subject:1:5:subject_id:<NULL>|idx_rollup_session_subject:1:6:session_boot_id:<NULL>|idx_rollup_session_subject:1:7:session_process_id:<NULL>|idx_rollup_session_subject:1:8:session_seq:<NULL>" "SELECT GROUP_CONCAT(CONCAT(index_name,':',non_unique,':',seq_in_index,':',column_name,':',COALESCE(sub_part,'<NULL>')) ORDER BY BINARY index_name,seq_in_index SEPARATOR '|') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='telemetry_rollup_session';" "telemetry_rollup_session indexes"
check "chk_telemetry_cohort_member_kind|chk_telemetry_cohort_member_subject" "SELECT GROUP_CONCAT(constraint_name ORDER BY BINARY constraint_name SEPARATOR '|') FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_cohort_member' AND constraint_type='CHECK';" "telemetry_cohort_member checks"
check_check_constraints
check 0 "SELECT COUNT(*) FROM information_schema.key_column_usage WHERE table_schema=DATABASE() AND table_name IN ('telemetry_rollup_session','telemetry_cohort_member') AND referenced_table_name IS NOT NULL;" "foreign keys"
echo 'telemetry rollup support schema verified'
