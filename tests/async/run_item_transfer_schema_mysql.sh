#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
if [[ -z "${DB_HOST:-}" || -z "${DB_USER:-}" || -z "${DB_PASSWD:-}" ||
      -z "${DB_NAME:-}" || -z "${ENVIRONMENT:-${APP_ENV:-}}" ]]; then
    set -a
    # shellcheck disable=SC1091
    source "$ROOT/.env"
    set +a
fi
environment_name="${ENVIRONMENT:-${APP_ENV:-}}"
[[ "${environment_name,,}" =~ (dev|local|test) ]] || { echo 'refusing item transfer test: environment is not development/local/test' >&2; exit 1; }
[[ "${DB_NAME,,}" =~ (dev|local|test) ]] || { echo 'refusing item transfer test: database name is not development/local/test' >&2; exit 1; }
export MYSQL_PWD="$DB_PASSWD"
if mysql --help 2>&1 | grep -- '--ssl-mode' >/dev/null; then MYSQL_SSL=(--ssl-mode=PREFERRED); else MYSQL_SSL=(--skip-ssl); fi
MYSQL=(mysql "${MYSQL_SSL[@]}" -h "$DB_HOST" -P "${DB_PORT:-3306}" -u "$DB_USER" -N -B)
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/item_ownership_ledger.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/shopkeeper_item_owner.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/collector_item_owner.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0045_quest_reward_obligation.sql"
"${MYSQL[@]}" "$DB_NAME" < "$ROOT/migrations/immutable/0051_player_item_runtime_state.sql"
DB_NAME="$DB_NAME" "$ROOT/migrations/verify_collector_item_owner.sh"
# The full accounting schema has later provenance columns. Its sealed runtime
# contract supersedes the older isolated item-ledger column census.
if [[ $("${MYSQL[@]}" "$DB_NAME" -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='mud_schema_history'") == 1 ]]; then
    DB_NAME="$DB_NAME" bash "$ROOT/migrations/verify_runtime_compatibility.sh" --schema-only
else
    DB_NAME="$DB_NAME" "$ROOT/migrations/verify_item_ownership_schema.sh"
fi
export ITEM_TRANSFER_TEST_DB_NAME="$DB_NAME"
mkdir -p "$ROOT/bin/tests"
TEST_BINARY="$ROOT/bin/tests/item_transfer_mysql_harness"
if [[ "${PLAYER_QUARANTINE_RECOVERY_TEST:-}" == 1 ]]; then
    [[ "$DB_NAME" =~ ^economic_schema_test_recovery_[a-f0-9]{16}$ ]] || { echo 'refusing recovery fixture outside its generated disposable schema' >&2; exit 1; }
    TEST_BINARY="$ROOT/bin/tests/${DB_NAME}_item_transfer_harness"
fi
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -ffunction-sections -fdata-sections -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/item_transfer_mysql_harness.cpp \
    tests/async/item_extra_descr_codec_sql_escape_stub.cpp \
    src/persistence/critical_command.c src/world/epic_command.c src/economy/currency_command.c \
    src/item/item_transfer_command.c src/item/craft_pouch_mutation.c src/combat/chaos_pouch_ledger.c src/item/item_transfer_repository.c src/persistence/sql_room_item_payload.c \
    src/item/economic_accounting_item_reference.c \
    src/sql/item_extra_descr_codec.c \
	 src/economy/auction_command.c src/economy/auction_repository.c \
    src/combat/combat_outcome_command.c src/combat/combat_outcome_repository.c \
	 src/guild/artifact_guild_command.c src/guild/artifact_guild_repository.c \
    src/economy/boon_reward_command.c src/economy/boon_reward_repository.c \
    src/world/zone_touch_command.c src/world/zone_touch_repository.c \
    src/account/session_audit_command.c src/account/session_audit_repository.c \
    src/item/item_uid_allocator.c src/flatfile/flatfile_item_uid_allocator.c src/flatfile/flatfile_store.c \
    src/persistence/persistence_mode.c \
    src/economy/coin_transfer_command.c src/player/player_snapshot_codec.c \
    src/economy/collector_command.c src/economy/collector_codec.c \
    src/economy/collector_policy.c src/economy/collector_repository.c \
    src/economy/collector_accounting.c \
    src/economy/shop_trade_command.c src/economy/shop_trade_recovery_manifest.c src/economy/shop_trade_accounting.c \
    src/persistence/corpse_lifecycle_command.c src/persistence/corpse_lifecycle_repository.c \
    src/persistence/player_death_restitution_command.c \
    src/persistence/player_death_restitution_repository.c \
    src/player/player_snapshot_repository.c src/player/player_load_repository.c \
    src/player/player_death_recovery_query.c src/player/player_death_conflict_repository.c \
    src/player/player_save_journal.c \
    src/player/player_quarantine_recovery.c \
    src/player/player_load_topology.c src/persistence/persistence_observability.c \
    src/persistence/economic_accounting_repository.c \
    src/persistence/economic_sql_bank_transaction.c \
    src/persistence/economic_sql_item_transfer_transaction.c \
    src/persistence/economic_sql_collector_transaction.c \
    src/persistence/economic_sql_shop_trade_transaction.c src/persistence/shop_item_runtime_payload.c src/economy/shop_trade_recovery_image.c \
    src/economy/economic_currency_adapter.c \
    src/economy/item_transfer_accounting.c \
    src/economy/coin_transfer_accounting.c \
    src/economy/economic_accounting_types.c \
    src/economy/economic_accounting_plan.c src/economy/economic_source_event.c \
    src/economy/economic_accounting_intent.c src/economy/economic_command_admission.c \
    src/persistence/economic_sql_lifecycle_guard.c src/persistence/critical_command_repository.c \
    src/persistence/quest_reward_obligation_repository.c \
    src/persistence/critical_command_journal.c src/persistence/critical_command_coordinator.c \
    -Wl,--gc-sections -Wl,--wrap=mysql_real_query,--wrap=mysql_errno "${MYSQL_LIBS[@]}" -lcrypto -lz \
    -o "$TEST_BINARY"
# The accounted lifecycle fixture retains many bounded item payloads in one
# test frame. The usual 8 MiB shell stack can overflow before its SQL checks.
if ! ulimit -s 65536; then
    echo 'item transfer SQL harness requires a 64 MiB stack' >&2
    exit 1
fi
"$TEST_BINARY"
printf 'item creation, sourced creation claims, subtree, stale, incomplete, replay, transfer, destruction, ledger, and outbox checks passed\n'
