#!/usr/bin/env python3
"""Execute legacy-only flatfile authority gates under ASan and UBSan."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import source

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "flatfile_item_repository.c", "flatfile_collector_repository.c",
    "collector_command.c", "collector_codec.c", "collector_policy.c",
    "coin_transfer_command.c", "flatfile_player_snapshot_file.c",
    "flatfile_corpse_repository.c", "flatfile_shop_trade_repository.c",
    "flatfile_shop_trade_materialization.c", "flatfile_locker_repository.c",
    "flatfile_world_item_repository.c", "flatfile_artifact_repository.c",
    "flatfile_shopkeeper_repository.c", "flatfile_auction_repository.c",
    "flatfile_boon_repository.c", "flatfile_player_domain_repository.c",
    "flatfile_ip_activity_repository.c", "flatfile_authority_transaction.c",
    "flatfile_accounting_authority.c", "flatfile_accounting_store.c", "flatfile_item_accounting_reference.c",
    "flatfile_store.c", "player_snapshot_codec.c", "item_transfer_command.c", "craft_pouch_mutation.c", "chaos_pouch_ledger.c",
    "economic_accounting_item_reference.c",
    "corpse_lifecycle_command.c", "shop_trade_command.c", "critical_command.c",
    "epic_command.c", "currency_command.c", "auction_command.c",
    "combat_outcome_command.c", "boon_reward_command.c", "boon_shop_command.c",
    "persistence_mode.c", "economic_accounting_intent.c",
    "item_transfer_accounting.c",
    "economic_command_admission.c", "economic_currency_adapter.c",
    "coin_transfer_accounting.c", "critical_command_journal.c",
    "critical_command_coordinator.c",
    "economic_accounting_plan.c", "economic_source_event.c", "economic_accounting_types.c",
    "economic_baseline_codec.c", "economic_baseline_adapter.c",
    "auction_item_claim_accounting.c", "auction_accounting.c",
    "auction_settlement_accounting.c", "auction_money_claim_accounting.c",
    "auction_listing_accounting.c",
    "collector_accounting.c",
)
with tempfile.TemporaryDirectory(prefix="duris-accounting-flatfile-gate-") as temporary:
    executable = Path(temporary) / "gate"
    # Keep the existing flatfile harness unoptimized: GCC 13 at -O1 diagnoses
    # an unrelated boon result copy even on the unchanged phase-1 foundation.
    # Sanitizers and warnings-as-errors remain enabled for this gate harness.
    command = shlex.split(os.environ.get("CXX", "g++")) + [
        "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O0", "-g",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-ffunction-sections", "-fdata-sections",
        "-D__NO_MYSQL__", "-DDURIS_FLATFILE_ACCOUNTING_TEST",
        "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-I" + str(ROOT / "src"),
        "-I" + str(ROOT / "src/no_mysql"),
        str(ROOT / "tests/async/economic_accounting_flatfile_gate_test.cpp"),
    ]
    command += [str(source(name)) for name in SOURCES]
    command += ["-lcrypto", "-lz", "-pthread", "-Wl,--gc-sections", "-o", str(executable)]
    subprocess.run(command, cwd=ROOT, check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    subprocess.run([str(executable), str(Path(temporary) / "state")],
                   env=environment, cwd=ROOT, check=True, timeout=45)
