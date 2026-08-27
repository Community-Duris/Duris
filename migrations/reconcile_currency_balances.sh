#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
if [[ -f "$PROJECT_ROOT/.env" ]]; then
    # shellcheck disable=SC1091
    source "$PROJECT_ROOT/.env"
fi
: "${DB_HOST:?DB_HOST is required}"
: "${DB_USER:?DB_USER is required}"
: "${DB_PASSWD:?DB_PASSWD is required}"
: "${DB_NAME:?DB_NAME is required}"

export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B "$DB_NAME")

missing_wallet=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM player_data p LEFT JOIN currency_wallet_baseline b ON b.pid=p.pid WHERE b.pid IS NULL;")
missing_bank=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM account_banks a LEFT JOIN currency_bank_baseline b ON b.bank_id=a.id WHERE b.bank_id IS NULL;")
wallet_mismatch=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM player_data p JOIN currency_wallet_baseline b ON b.pid=p.pid LEFT JOIN (SELECT pid,SUM(wallet_delta_copper) c,SUM(wallet_delta_silver) s,SUM(wallet_delta_gold) g,SUM(wallet_delta_platinum) pl FROM currency_ledger GROUP BY pid) l ON l.pid=p.pid WHERE b.opening_copper+COALESCE(l.c,0)<>p.copper OR b.opening_silver+COALESCE(l.s,0)<>p.silver OR b.opening_gold+COALESCE(l.g,0)<>p.gold OR b.opening_platinum+COALESCE(l.pl,0)<>p.platinum;")
bank_mismatch=$("${MYSQL[@]}" -e "SELECT COUNT(*) FROM account_banks a JOIN currency_bank_baseline b ON b.bank_id=a.id LEFT JOIN (SELECT bank_id,SUM(bank_delta_copper) c,SUM(bank_delta_silver) s,SUM(bank_delta_gold) g,SUM(bank_delta_platinum) pl FROM currency_ledger GROUP BY bank_id) l ON l.bank_id=a.id WHERE b.opening_copper+COALESCE(l.c,0)<>a.bank_copper OR b.opening_silver+COALESCE(l.s,0)<>a.bank_silver OR b.opening_gold+COALESCE(l.g,0)<>a.bank_gold OR b.opening_platinum+COALESCE(l.pl,0)<>a.bank_platinum;")
printf 'currency reconciliation missing_wallet=%s missing_bank=%s wallet_mismatch=%s bank_mismatch=%s\n' "$missing_wallet" "$missing_bank" "$wallet_mismatch" "$bank_mismatch"
[[ "$missing_wallet" == 0 && "$missing_bank" == 0 && "$wallet_mismatch" == 0 && "$bank_mismatch" == 0 ]]
