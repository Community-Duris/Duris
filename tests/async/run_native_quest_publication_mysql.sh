#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$ROOT"
# No .env loading, migrations, schema creation, deletion, or live-game connection.
# The orchestrator owns creation/recovery/removal of the isolated migrated schema.
: "${DB_HOST:?disposable service required}" "${DB_USER:?disposable user required}"
: "${DB_PASSWD:?disposable credential required}"
: "${NATIVE_QUEST_PUBLICATION_TEST_DB_NAME:?fresh generated schema required}"
[[ "${NATIVE_QUEST_PUBLICATION_DISPOSABLE:-}" == 1 ]] || {
    echo 'refusing native publication fixture without disposable authorization' >&2; exit 1;
}
[[ "$NATIVE_QUEST_PUBLICATION_TEST_DB_NAME" =~ ^native_quest_publication_test_[a-f0-9]{16}$ ]] || {
    echo 'refusing native publication fixture outside generated disposable schema' >&2; exit 1;
}
export DB_NAME="$NATIVE_QUEST_PUBLICATION_TEST_DB_NAME" ENVIRONMENT=test
export MYSQL_PWD="$DB_PASSWD"
bash "$ROOT/migrations/verify_runtime_compatibility.sh" --schema-only
mkdir -p "$ROOT/bin/tests"
TEST_BINARY="$ROOT/bin/tests/${DB_NAME}_native_quest_publication_harness"
read -r -a MYSQL_CFLAGS <<< "$(mysql_config --cflags)"
read -r -a MYSQL_LIBS <<< "$(mysql_config --libs)"
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -ffunction-sections -fdata-sections -Isrc \
    "${MYSQL_CFLAGS[@]}" tests/async/native_quest_publication_mysql_harness.cpp \
    tests/async/item_extra_descr_codec_sql_escape_stub.cpp \
    src/persistence/critical_command.c src/world/db.c src/world/epic_command.c src/economy/currency_command.c \
    src/item/item_transfer_command.c src/world/quest_mobile_native_reference.c src/item/craft_pouch_mutation.c src/combat/chaos_pouch_ledger.c src/item/item_transfer_repository.c src/world/quest_mobile_native.c src/persistence/quest_mobile_native_sql.c src/persistence/sql_room_item_payload.c \
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
    src/persistence/economic_sql_source_snapshot.c \
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
    -Wl,--gc-sections "${MYSQL_LIBS[@]}" -lcrypto -lz \
    -o "$TEST_BINARY"
# Preserve the original item SQL fixture's 64 MiB stack policy.
ulimit -s 65536 || { echo 'native publication fixture requires a 64 MiB stack' >&2; exit 1; }
# One sequential bounded fixture, 3s SQL transport and 1s lock waits; no concurrency.
timeout --signal=TERM --kill-after=5s 60s "$TEST_BINARY"
