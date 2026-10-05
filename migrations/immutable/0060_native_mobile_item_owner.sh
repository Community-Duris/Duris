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
# Metadata-only verifier; no credentials are sourced and no rows are changed.
query=$(cat <<'DURIS_NATIVE_MOBILE_OWNER_SQL'
SET @duris_native_owner_enforced = NULL;
SET @duris_native_owner_enforcement_sql = IF(LOCATE('MariaDB', VERSION()) > 0,
    'SELECT @@SESSION.check_constraint_checks INTO @duris_native_owner_enforced',
    'SELECT (COUNT(*) = 3) INTO @duris_native_owner_enforced FROM information_schema.table_constraints t WHERE t.table_schema=DATABASE() AND t.constraint_schema=DATABASE() AND t.constraint_type=''CHECK'' AND t.enforced=''YES'' AND ((t.table_name=''item_owner_revision'' AND t.constraint_name=''chk_item_owner_revision_type'') OR (t.table_name=''item_current_owner'' AND t.constraint_name=''chk_item_current_owner_type'') OR (t.table_name=''item_ownership_baseline'' AND t.constraint_name=''chk_item_baseline_owner_type''))');
PREPARE native_owner_enforcement_stmt FROM @duris_native_owner_enforcement_sql;
EXECUTE native_owner_enforcement_stmt;
DEALLOCATE PREPARE native_owner_enforcement_stmt;
SELECT (@duris_native_owner_enforced = 1 AND (
        (SELECT COUNT(*) FROM information_schema.columns
            WHERE table_schema=DATABASE()
              AND table_name IN ('item_owner_revision', 'item_current_owner', 'item_ownership_baseline')
              AND column_name='owner_type' AND data_type='tinyint'
              AND column_type LIKE '%unsigned%' AND is_nullable='NO') = 3
        AND (SELECT COUNT(*) FROM information_schema.table_constraints t
            JOIN information_schema.check_constraints c
              ON c.constraint_schema=t.constraint_schema AND c.constraint_name=t.constraint_name
            WHERE t.table_schema=DATABASE() AND t.constraint_schema=DATABASE()
              AND t.constraint_type='CHECK'
              AND ((t.table_name='item_owner_revision' AND t.constraint_name='chk_item_owner_revision_type')
                OR (t.table_name='item_current_owner' AND t.constraint_name='chk_item_current_owner_type')
                OR (t.table_name='item_ownership_baseline' AND t.constraint_name='chk_item_baseline_owner_type'))
              AND LOWER(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(c.check_clause,
                CHAR(96), ''), '(', ''), ')', ''), ' ', ''), CHAR(9), ''), CHAR(10), ''), CHAR(13), '')) = 'owner_typebetween1and12') = 3
));
DURIS_NATIVE_MOBILE_OWNER_SQL
)
shape=$("${MYSQL[@]}" -e "$query")
[[ "$shape" == 1 ]] || { echo 'native mobile item owner constraints differ or are not enforced' >&2; exit 1; }
printf 'exact native mobile item owner range 1..12 verified\n'
