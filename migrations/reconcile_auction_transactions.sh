#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing auction reconciliation: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing auction reconciliation: database name is not development/local/test' >&2; exit 1; }
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -N -B -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" "$DB_NAME")
"${MYSQL[@]}" <<'SQL'
SELECT CONCAT('authoritative_missing_custody=',COUNT(*)) FROM auctions a
WHERE a.custody_state=1 AND NOT EXISTS (SELECT 1 FROM auction_item_custody c WHERE c.auction_id=a.id);
SELECT CONCAT('custody_owner_mismatch=',COUNT(*)) FROM auction_item_custody c
JOIN item_current_owner o ON o.item_uid=c.item_uid
WHERE c.claimed_at IS NULL AND (o.owner_type<>6 OR o.owner_id<>c.auction_id);
SELECT CONCAT('custody_revision_mismatch=',COUNT(*)) FROM auction_item_custody c
JOIN item_current_owner o ON o.item_uid=c.item_uid WHERE o.item_revision<>c.item_revision;
SELECT CONCAT('open_quarantine=',COUNT(*)) FROM auction_reconciliation_quarantine WHERE repaired_at IS NULL;
SQL
