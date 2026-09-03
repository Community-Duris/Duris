#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
# A loopback target needs no transport protection; anything else must present a
# CA-verified server identity before the password is sent, matching the runtime
# connection contract's remote_tls_required.
MYSQL_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
    [[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
        echo 'remote verification requires TLS and a CA file' >&2
        exit 1
    }
    if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
        MYSQL_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
    elif mysql --help 2>&1 | grep -q -- '--ssl-verify-server-cert'; then
        MYSQL_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
    else
        echo 'database client cannot verify the remote server identity' >&2
        exit 1
    fi
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

# MySQL reports the converged default as CURRENT_TIMESTAMP and MariaDB as
# current_timestamp(); both satisfy the prefix match.
column_contract=$("${MYSQL[@]}" -e "
SELECT COUNT(*)
FROM information_schema.columns
WHERE table_schema=DATABASE()
  AND table_name='pkill_event'
  AND column_name='stamp'
  AND data_type='datetime'
  AND is_nullable='NO'
  AND UPPER(column_default) LIKE 'CURRENT_TIMESTAMP%';")
zero_rows=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM pkill_event WHERE YEAR(stamp)=0;")

[[ "$column_contract" == 1 && "$zero_rows" == 0 ]]
echo "pkill event stamp contract verified"
