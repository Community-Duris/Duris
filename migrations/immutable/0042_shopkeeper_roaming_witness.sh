#!/usr/bin/env bash
set -euo pipefail

: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" --protocol=tcp -h "$DB_HOST" -P "${DB_PORT:-3306}" \
    -u "$DB_USER" -N -B "$DB_NAME")
scalar() { "${MYSQL[@]}" -e "$1"; }

table=$(scalar "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='shopkeepers' AND engine='InnoDB'")
roaming=$(scalar "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='shopkeepers' AND column_name='keeper_roaming' AND data_type='tinyint' AND LOWER(column_type) LIKE '%unsigned%' AND is_nullable='YES' AND (column_default IS NULL OR UPPER(column_default)='NULL')")
if [[ "$table" != 1 || "$roaming" != 1 ]]; then
    echo 'shopkeeper roaming witness schema mismatch' >&2
    exit 1
fi

echo 'shopkeeper roaming witness schema verified'
