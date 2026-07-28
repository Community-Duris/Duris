#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

usage() {
    printf 'usage: %s --confirm-db <database-name>\n' "$0" >&2
    printf 'Back up/quiesce the target first. Rehearse this command on an isolated clone before use.\n' >&2
    exit 2
}

[[ $# -eq 2 && "$1" == "--confirm-db" ]] || usage
confirmed_db="$2"

if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" || -z "${DB_NAME:-}" ]]; then
    if [[ -f "$SCRIPT_DIR/.env" ]]; then
        # shellcheck disable=SC1091
        source "$SCRIPT_DIR/.env"
    elif [[ -f "$PROJECT_ROOT/.env" ]]; then
        # shellcheck disable=SC1091
        source "$PROJECT_ROOT/.env"
    fi
fi

: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

if [[ "$confirmed_db" != "$DB_NAME" ]]; then
    printf 'refusing: --confirm-db %q does not match configured DB_NAME %q\n' "$confirmed_db" "$DB_NAME" >&2
    exit 2
fi

MYSQL_PWD="$DB_PASSWD"
export MYSQL_PWD
if mysql --help 2>&1 | grep -q -- '--ssl-mode'; then
    MYSQL_SSL=(--ssl-mode=PREFERRED)
else
    MYSQL_SSL=(--skip-ssl)
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")
selected_db=$("${MYSQL[@]}" -e 'SELECT DATABASE();')
if [[ "$selected_db" != "$DB_NAME" ]]; then
    printf 'refusing: server selected database %q, expected %q\n' "$selected_db" "$DB_NAME" >&2
    exit 2
fi

printf 'applying scoped persistence contract to %s on %s...\n' "$DB_NAME" "$DB_HOST"
"${MYSQL[@]}" < "$SCRIPT_DIR/persistence_contract.sql"

DB_HOST="$DB_HOST" DB_PORT="${DB_PORT:-3306}" DB_USER="$DB_USER" \
DB_PASSWD="$DB_PASSWD" DB_NAME="$DB_NAME" \
    "$SCRIPT_DIR/verify_persistence_contract.sh"
