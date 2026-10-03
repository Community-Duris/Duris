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
shape=$("${MYSQL[@]}" -e "SELECT CONCAT(column_name,':',data_type,IF(column_type LIKE '%unsigned%',' unsigned',''),':',is_nullable,':',COALESCE(column_default,'NULL'),':',extra) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='telemetry_interval' AND column_name IN ('ownership_account_token','ownership_source') ORDER BY ordinal_position;" | paste -sd'|' -)
[[ "$shape" == 'ownership_account_token:bigint unsigned:YES:NULL:|ownership_source:tinyint unsigned:YES:NULL:' ]] || { echo 'telemetry ownership column shape differs' >&2; exit 1; }
ordering=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns a JOIN information_schema.columns b ON b.table_schema=a.table_schema AND b.table_name=a.table_name JOIN information_schema.columns c ON c.table_schema=a.table_schema AND c.table_name=a.table_name WHERE a.table_schema=DATABASE() AND a.table_name='telemetry_interval' AND a.column_name='combat_quality_flags' AND b.column_name='ownership_account_token' AND c.column_name='ownership_source' AND b.ordinal_position=a.ordinal_position+1 AND c.ordinal_position=b.ordinal_position+1;")
[[ "$ordering" == 1 ]] || { echo 'telemetry ownership column order differs' >&2; exit 1; }
checks=$("${MYSQL[@]}" -e "SELECT LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,' ',''),CHAR(96),''),'(',''),')',''),CHAR(10),'')) FROM information_schema.table_constraints t JOIN information_schema.check_constraints c ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name WHERE t.constraint_schema=DATABASE() AND t.table_name='telemetry_interval' AND t.constraint_type='CHECK' AND t.constraint_name='chk_telemetry_ownership_payload';")
[[ "$checks" == 'record_kind=9andownership_account_tokenisnotnullandownership_sourceisnotnullandownership_sourcebetween1and4andownership_account_token<>0orownership_source=5andownership_account_token=0orrecord_kind<>9andownership_account_tokenisnullandownership_sourceisnull' ]] || { echo 'telemetry ownership payload check differs' >&2; exit 1; }
engine=$("${MYSQL[@]}" -e "SELECT CONCAT(engine,':',table_collation) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='telemetry_interval';")
[[ "$engine" == InnoDB:utf8mb4_unicode_ci ]] || { echo 'telemetry ownership engine or collation differs' >&2; exit 1; }
if [[ $("${MYSQL[@]}" -e 'SELECT VERSION();') != *MariaDB* ]]; then
    enforced=$("${MYSQL[@]}" -e "SELECT enforced FROM information_schema.table_constraints WHERE constraint_schema=DATABASE() AND table_name='telemetry_interval' AND constraint_name='chk_telemetry_ownership_payload';")
    [[ "$enforced" == YES ]] || { echo 'telemetry ownership payload check is not enforced' >&2; exit 1; }
fi
printf 'telemetry ownership observation schema verified\n'
