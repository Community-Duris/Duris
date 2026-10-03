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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',CASE WHEN data_type IN ('int','bigint','tinyint') THEN CONCAT(data_type,IF(column_type LIKE '%unsigned%',' unsigned','')) ELSE column_type END,':',is_nullable,':',COALESCE(column_default,'<null>')) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='player_affects' AND column_name LIKE 'ward_%' ORDER BY ordinal_position;" | paste -sd'|' -)
expected='ward_source_uid:bigint unsigned:NO:0|ward_full_duration:int:NO:0|ward_capacity:bigint unsigned:NO:0|ward_capacity_max:bigint unsigned:NO:0|ward_refresh_remaining:int:NO:0|ward_source_type:tinyint unsigned:NO:0|ward_source_worn:tinyint unsigned:NO:0|ward_active:tinyint unsigned:NO:0'
[[ "$shape" == "$expected" ]] || { echo "spell ward columns differ: $shape" >&2; exit 1; }
printf 'spell ward durability schema verified\n'
