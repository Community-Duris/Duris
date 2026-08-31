#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
set -a
# shellcheck disable=SC1091
source "$ROOT/.env"
set +a
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing auction test: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing auction test: configured database name is not development/local/test' >&2; exit 1; }
"$ROOT/migrations/apply_auction_transactional_cutover.sh"
export AUCTION_TEST_DB_NAME="$DB_NAME"
mkdir -p "$ROOT/bin/tests"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/auction_transaction_mysql_harness.cpp \
    src/persistence/critical_command.c src/world/epic_command.c src/economy/currency_command.c \
    src/item/item_transfer_command.c src/item/item_transfer_repository.c src/economy/auction_command.c \
    src/economy/auction_repository.c src/combat/combat_outcome_command.c src/combat/combat_outcome_repository.c \
    src/guild/artifact_guild_command.c src/guild/artifact_guild_repository.c \
    src/economy/boon_reward_command.c src/economy/boon_reward_repository.c \
    src/world/zone_touch_command.c src/world/zone_touch_repository.c \
    src/account/session_audit_command.c src/account/session_audit_repository.c \
    src/persistence/critical_command_repository.c \
    "${MYSQL_LIBS[@]}" -lcrypto -o "$ROOT/bin/tests/auction_transaction_mysql_harness"
"$ROOT/bin/tests/auction_transaction_mysql_harness"
printf 'auction listing, stale bid, replay, settlement, refund, money claim, and item claim checks passed\n'
