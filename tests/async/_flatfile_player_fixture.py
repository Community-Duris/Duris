"""Shared production-linked player inspector build; no runtime fixture is shared."""

from pathlib import Path
from _paths import rel
from native_build_artifacts import build_native

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/async/flatfile_player_repository_harness.cpp",
    rel("flatfile_player_repository.c"),
    rel("player_load_topology.c"),
    rel("flatfile_identity_repository.c"),
    rel("flatfile_item_repository.c"),
    rel("flatfile_collector_repository.c"),
    rel("collector_command.c"),
    rel("collector_codec.c"),
    rel("collector_policy.c"),
    rel("coin_transfer_command.c"),
    rel("flatfile_player_snapshot_file.c"),
    rel("flatfile_corpse_repository.c"),
    rel("flatfile_locker_repository.c"),
    rel("flatfile_world_item_repository.c"),
    rel("flatfile_artifact_repository.c"),
    rel("flatfile_shop_trade_repository.c"),
    rel("flatfile_shop_trade_materialization.c"),
    rel("flatfile_shopkeeper_repository.c"),
    rel("flatfile_auction_repository.c"),
    rel("flatfile_boon_repository.c"),
    rel("flatfile_player_domain_repository.c"),
    rel("flatfile_authority_transaction.c"),
    rel("flatfile_item_accounting_reference.c"),
    rel("flatfile_accounting_authority.c"),
    rel("flatfile_accounting_store.c"),
    rel("economic_accounting_types.c"),
    rel("economic_accounting_plan.c"), rel("economic_source_event.c"),
    rel("auction_listing_accounting.c"),
    rel("auction_accounting.c"),
    rel("auction_settlement_accounting.c"),
    rel("auction_money_claim_accounting.c"),
    rel("auction_item_claim_accounting.c"),
    rel("collector_accounting.c"),
    rel("economic_accounting_intent.c"),
    rel("economic_accounting_item_reference.c"),
    rel("item_transfer_accounting.c"),
    rel("player_snapshot_codec.c"),
    rel("player_save_journal.c"),
    rel("player_quarantine_recovery.c"),
    rel("flatfile_store.c"),
    rel("item_transfer_command.c"),
    rel("craft_pouch_mutation.c"),
    rel("chaos_pouch_ledger.c"),
    rel("corpse_lifecycle_command.c"),
    rel("shop_trade_command.c"),
    rel("critical_command.c"),
    rel("epic_command.c"),
    rel("currency_command.c"),
    rel("auction_command.c"),
    rel("combat_outcome_command.c"),
    rel("boon_reward_command.c"),
    rel("boon_shop_command.c"),
    rel("persistence_observability.c"),
    rel("persistence_mode.c"),
    rel("flatfile_ip_activity_repository.c"),
]
FLAGS = [
    "-std=c++20",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Werror",
    "-D__NO_MYSQL__",
    "-ffunction-sections",
    "-fdata-sections",
    "-Wl,--gc-sections",
    "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
    "-DDURIS_FLATFILE_PLAYER_READ_FAULT_TEST",
    "-DDURIS_FLATFILE_ACCOUNTING_TEST",
    "-Isrc",
    "-Isrc/no_mysql",
    "-pthread",
]
LINK_FLAGS = [
    "-lcrypto",
    "-pthread",
    "-Wl,--wrap=openat",
]


def build_player_inspector(destination):
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    return build_native(destination, SOURCES, FLAGS, LINK_FLAGS, name="player-inspector")
