#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing item reconciliation: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing item reconciliation: database name is not development/local/test' >&2; exit 1; }
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

missing_baseline=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM item_current_owner current_item LEFT JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid WHERE baseline.item_uid IS NULL AND current_item.item_revision=0;")
item_revision_mismatch=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM item_current_owner current_item JOIN item_ownership_baseline baseline ON baseline.item_uid=current_item.item_uid LEFT JOIN (SELECT item_uid,COUNT(*) event_count FROM item_ownership_ledger GROUP BY item_uid) ledger ON ledger.item_uid=current_item.item_uid WHERE current_item.item_revision<>baseline.opening_item_revision+COALESCE(ledger.event_count,0);")
owner_revision_mismatch=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM item_owner_revision owner LEFT JOIN (SELECT owner_type,owner_id,owner_context_id,COUNT(DISTINCT operation_id) event_count FROM (SELECT operation_id,from_owner_type owner_type,from_owner_id owner_id,from_owner_context_id owner_context_id FROM item_ownership_ledger UNION ALL SELECT operation_id,to_owner_type,to_owner_id,to_owner_context_id FROM item_ownership_ledger) touched GROUP BY owner_type,owner_id,owner_context_id) ledger ON ledger.owner_type=owner.owner_type AND ledger.owner_id=owner.owner_id AND ledger.owner_context_id=owner.owner_context_id WHERE owner.revision<>COALESCE(ledger.event_count,0);")
latest_owner_mismatch=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM item_current_owner current_item JOIN item_ownership_ledger ledger ON ledger.item_uid=current_item.item_uid AND ledger.item_revision=current_item.item_revision WHERE current_item.owner_type<>ledger.to_owner_type OR current_item.owner_id<>ledger.to_owner_id OR current_item.owner_context_id<>ledger.to_owner_context_id;")

printf 'missing_baseline=%s\nitem_revision_mismatch=%s\nowner_revision_mismatch=%s\nlatest_owner_mismatch=%s\n' \
    "$missing_baseline" "$item_revision_mismatch" "$owner_revision_mismatch" "$latest_owner_mismatch"
[[ "$missing_baseline" == 0 && "$item_revision_mismatch" == 0 && "$owner_revision_mismatch" == 0 && "$latest_owner_mismatch" == 0 ]]
