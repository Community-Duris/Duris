#!/usr/bin/env bash
set -euo pipefail
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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_craft_progression' ORDER BY ordinal_position;" | paste -sd'|' -)
expected='operation_id:binary(16):NO|pid:int unsigned:NO|discipline:int unsigned:NO|experience:int unsigned:NO|applied_revision:bigint unsigned:NO|created_at:timestamp(6):NO'
[[ "$shape" == "$expected" ]] || { echo "craft progression columns differ: $shape" >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_craft_progression' GROUP BY index_name,non_unique ORDER BY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'idx_player_craft_progression_owner:1:pid,applied_revision,operation_id|PRIMARY:0:operation_id' ]] || { echo 'craft progression indexes differ' >&2; exit 1; }
foreign_key=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='player_craft_progression' AND k.referenced_table_name IS NOT NULL;")
[[ "$foreign_key" == 'fk_player_craft_progression_operation:operation_id:critical_operation_inbox:operation_id:RESTRICT:RESTRICT' ]] || { echo 'craft progression operation foreign key differs' >&2; exit 1; }
printf 'craft progression schema verified\n'
