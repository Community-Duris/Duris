#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -z "${DB_HOST:-}" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}" "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}" "${DB_NAME:?DB_NAME is required}"
export MYSQL_PWD="$DB_PASSWD"
MYSQL_SSL=()
if [[ "$DB_HOST" != "localhost" && "$DB_HOST" != "127.0.0.1" && "$DB_HOST" != "::1" ]]; then
    [[ "${DB_TLS:-}" == "TRUE" && -f "${DB_SSL_CA:-}" ]] || {
        echo 'remote character baseline readiness requires TLS and a CA file' >&2
        exit 2
    }
    mysql_help=$(mysql --help 2>&1)
    if grep -q -- '--ssl-mode' <<<"$mysql_help"; then
        MYSQL_SSL=(--ssl-mode=VERIFY_IDENTITY --ssl-ca="$DB_SSL_CA")
    elif grep -q -- '--ssl-verify-server-cert' <<<"$mysql_help"; then
        MYSQL_SSL=(--ssl-ca="$DB_SSL_CA" --ssl-verify-server-cert)
    else
        echo 'database client cannot verify the remote server identity' >&2
        exit 2
    fi
fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

query="SELECT COUNT(*),
COALESCE(SUM(wallet.pid IS NULL),0),
COALESCE(SUM(epic.pid IS NULL),0),
COALESCE(SUM(combat.pid IS NULL),0)
FROM (
    SELECT DISTINCT player.pid
    FROM player_data player
    JOIN account_characters mapping ON mapping.pid=player.pid
    WHERE player.active=1 AND mapping.deleted_at IS NULL AND mapping.blocked=0
) eligible
LEFT JOIN currency_wallet_baseline wallet ON wallet.pid=eligible.pid
LEFT JOIN epic_balance_baseline epic ON epic.pid=eligible.pid
LEFT JOIN combat_frag_baseline combat ON combat.pid=eligible.pid;"
if ! result=$("${MYSQL[@]}" -e "$query"); then
    echo 'character baseline readiness query failed' >&2
    exit 2
fi
IFS=$'\t' read -r eligible wallet_missing epic_missing combat_missing <<<"$result"
for value in "$eligible" "$wallet_missing" "$epic_missing" "$combat_missing"; do
    [[ "$value" =~ ^[0-9]+$ ]] || {
        echo 'character baseline readiness returned malformed aggregate output' >&2
        exit 2
    }
done
printf 'eligible_characters=%s\nwallet_baseline_missing=%s\nepic_baseline_missing=%s\n' \
    "$eligible" "$wallet_missing" "$epic_missing"
printf 'combat_frag_baseline_missing=%s\n' "$combat_missing"
[[ "$wallet_missing" == 0 && "$epic_missing" == 0 && "$combat_missing" == 0 ]]
