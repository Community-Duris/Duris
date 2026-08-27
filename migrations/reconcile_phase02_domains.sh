#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing Phase 02 reconciliation: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing Phase 02 reconciliation: database name is not development/local/test' >&2; exit 1; }

for check in \
    reconcile_epic_balances.sh \
    reconcile_currency_balances.sh \
    reconcile_item_ownership.sh \
    reconcile_combat_frags.sh \
    reconcile_artifact_guild_outcomes.sh \
    reconcile_boon_reward_zone.sh
do
    "$SCRIPT_DIR/$check"
done
echo "Phase 02 authoritative domain reconciliation passed"
