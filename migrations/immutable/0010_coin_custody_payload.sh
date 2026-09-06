#!/usr/bin/env bash
set -euo pipefail
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
column=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='item_current_owner' AND column_name='coin_payload' AND data_type='mediumblob' AND is_nullable='YES';")
[[ "$column" == 1 ]] || { echo 'FAILED: authoritative coin payload column missing or incompatible' >&2; exit 1; }
echo 'authoritative coin payload schema verified'
