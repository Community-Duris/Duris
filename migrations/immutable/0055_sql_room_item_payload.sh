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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('bigint','smallint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='sql_room_item_payload' ORDER BY ordinal_position;" | paste -sd'|' -)
expected='item_uid:bigint unsigned:NO|item_revision:bigint unsigned:NO|payload_version:smallint unsigned:NO|operation_id:binary(16):NO|season_epoch:bigint unsigned:NO|payload:mediumblob:NO'
[[ "$shape" == "$expected" ]] || { echo 'room item payload columns differ' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='sql_room_item_payload';")
[[ "$engine" == InnoDB ]] || { echo 'room item payload requires InnoDB' >&2; exit 1; }
indexes=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='sql_room_item_payload' GROUP BY index_name,non_unique ORDER BY index_name;" | paste -sd'|' -)
[[ "$indexes" == 'idx_sql_room_item_operation:1:operation_id|PRIMARY:0:item_uid,item_revision' ]] || { echo 'room item payload indexes differ' >&2; exit 1; }
foreign_key=$("${MYSQL[@]}" -e "SELECT CONCAT(k.constraint_name,':',k.column_name,':',k.referenced_table_name,':',k.referenced_column_name,':',r.update_rule,':',r.delete_rule) FROM information_schema.key_column_usage k JOIN information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema AND r.constraint_name=k.constraint_name WHERE k.constraint_schema=DATABASE() AND k.table_name='sql_room_item_payload' AND k.referenced_table_name IS NOT NULL;")
[[ "$foreign_key" == 'fk_sql_room_item_operation:operation_id:critical_operation_inbox:operation_id:RESTRICT:RESTRICT' ]] || { echo 'room item payload operation foreign key differs' >&2; exit 1; }
check_clause=$("${MYSQL[@]}" -e "SELECT c.check_clause FROM information_schema.check_constraints c JOIN information_schema.table_constraints t ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name WHERE t.table_schema=DATABASE() AND t.table_name='sql_room_item_payload' AND t.constraint_type='CHECK';" | tr '[:upper:]' '[:lower:]' | tr -d '[:space:]`()' | sed 's/octet_length/length/g')
[[ "$check_clause" == 'item_uid>0anditem_revision>0andpayload_version=1andseason_epoch>0andlengthpayload>0andlengthpayload<=131072' ]] || { echo 'room item payload bounds constraint differs' >&2; exit 1; }
printf 'exact room item payload schema verified\n'
