#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing locker ownership cutover: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing locker ownership cutover: database name is not development/local/test' >&2; exit 1; }
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME" \
  < "$SCRIPT_DIR/locker_ownership_cutover.sql"
