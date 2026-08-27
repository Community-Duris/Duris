#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
# shellcheck disable=SC1091
source "$PROJECT_ROOT/.env"
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing currency baseline: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing currency baseline: configured database name is not development/local/test' >&2; exit 1; }

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

advanced=$("${MYSQL[@]}" -e "SELECT (SELECT COUNT(*) FROM currency_ledger)+(SELECT COUNT(*) FROM player_data WHERE wallet_revision<>0)+(SELECT COUNT(*) FROM account_banks WHERE bank_revision<>0);")
[[ "$advanced" == 0 ]] || { echo 'refusing currency baseline after ledger/revision activity' >&2; exit 1; }
"${MYSQL[@]}" -e "START TRANSACTION; INSERT IGNORE INTO currency_wallet_baseline(pid,opening_copper,opening_silver,opening_gold,opening_platinum,opening_revision) SELECT pid,copper,silver,gold,platinum,wallet_revision FROM player_data; INSERT IGNORE INTO currency_bank_baseline(bank_id,opening_copper,opening_silver,opening_gold,opening_platinum,opening_revision) SELECT id,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM account_banks; COMMIT;"
printf 'currency baselines captured for local development rows\n'
