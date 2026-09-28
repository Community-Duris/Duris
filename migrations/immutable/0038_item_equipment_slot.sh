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
shape=$("${MYSQL[@]}" -e "SELECT GROUP_CONCAT(CONCAT(table_name,'.',column_name,':',REPLACE(column_type,'smallint(5) unsigned','smallint unsigned'),':',is_nullable,':',column_default) ORDER BY table_name,column_name SEPARATOR '|') FROM information_schema.columns WHERE table_schema=DATABASE() AND ((table_name='item_current_owner' AND column_name='equipment_slot') OR (table_name='item_ownership_baseline' AND column_name='equipment_slot') OR (table_name='item_ownership_ledger' AND column_name IN ('from_equipment_slot','to_equipment_slot')));")
expected='item_current_owner.equipment_slot:smallint unsigned:NO:0|item_ownership_baseline.equipment_slot:smallint unsigned:NO:0|item_ownership_ledger.from_equipment_slot:smallint unsigned:NO:0|item_ownership_ledger.to_equipment_slot:smallint unsigned:NO:0'
[[ "$shape" == "$expected" ]] || {
    echo "FAILED: item equipment custody columns differ: $shape" >&2
    exit 1
}
printf 'item equipment custody schema verified\n'
