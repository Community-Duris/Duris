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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type='int' THEN CONCAT('int',IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_spell_effect_receipt' ORDER BY ordinal_position;" | paste -sd'|' -)
expected='pid:int unsigned:NO|operation_id:binary(16):NO|effect_id:int unsigned:NO|created_at:timestamp(6):NO'
[[ "$shape" == "$expected" ]] || { echo "spell effect receipt columns differ: $shape" >&2; exit 1; }
index=$("${MYSQL[@]}" -e "SELECT CONCAT(index_name,':',non_unique,':',GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',')) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='player_spell_effect_receipt' GROUP BY index_name,non_unique ORDER BY index_name;" | paste -sd'|' -)
[[ "$index" == 'idx_player_spell_effect_receipt_age:1:pid,created_at|PRIMARY:0:pid,operation_id' ]] || { echo "spell effect receipt indexes differ: $index" >&2; exit 1; }
printf 'player spell effect receipt schema verified\n'
