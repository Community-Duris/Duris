#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
set -a
# shellcheck disable=SC1091
source "$ROOT/.env"
set +a
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || {
    echo 'refusing epic test: environment is not development/local/test' >&2
    exit 1
}
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || {
    echo 'refusing epic test: configured database name is not development/local/test' >&2
    exit 1
}
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B)
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/epic_ledger_balance.sql"
DB_NAME="$DB_NAME" "$ROOT/migrations/verify_epic_ledger_schema.sh"
EPIC_TEST_DB_NAME="$DB_NAME"
export EPIC_TEST_DB_NAME
mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/epic_transaction_mysql_harness.cpp \
    src/critical_command.c src/epic_command.c src/currency_command.c \
    src/item_transfer_command.c src/item_transfer_repository.c src/auction_command.c \
    src/auction_repository.c src/combat_outcome_command.c src/combat_outcome_repository.c \
    src/artifact_guild_command.c src/artifact_guild_repository.c \
    src/boon_reward_command.c src/boon_reward_repository.c \
    src/zone_touch_command.c src/zone_touch_repository.c \
    src/session_audit_command.c src/session_audit_repository.c \
    src/critical_command_repository.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o "$ROOT/bin/tests/epic_transaction_mysql_harness"
"$ROOT/bin/tests/epic_transaction_mysql_harness"
printf 'epic award, spend, rejection, duplicate, ledger, and baseline checks passed\n'
