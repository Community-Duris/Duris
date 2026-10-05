#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile
import os
import shutil

ROOT = pathlib.Path(__file__).resolve().parents[2]

sources = [
  "tests/async/flatfile_craft_conservation_harness.cpp",
  rel("flatfile_item_repository.c"),
  rel("flatfile_item_accounting_reference.c"),
  rel("flatfile_accounting_authority.c"),
  rel("flatfile_accounting_store.c"),
  rel("economic_accounting_types.c"),
  rel("economic_accounting_plan.c"), rel("economic_source_event.c"),
  rel("economic_accounting_intent.c"),
  rel("economic_accounting_item_reference.c"),
  rel("item_transfer_accounting.c"),
  rel("auction_listing_accounting.c"),
  rel("auction_accounting.c"),
  rel("auction_settlement_accounting.c"),
  rel("auction_money_claim_accounting.c"),
  rel("auction_item_claim_accounting.c"),
  rel("collector_accounting.c"),
  rel("flatfile_collector_repository.c"),
  rel("collector_command.c"),
  rel("collector_codec.c"),
  rel("collector_policy.c"),
  rel("coin_transfer_command.c"),
  rel("flatfile_player_snapshot_file.c"),
  rel("flatfile_corpse_repository.c"),
  rel("flatfile_shop_trade_repository.c"),
  rel("flatfile_shop_trade_materialization.c"),
  rel("flatfile_locker_repository.c"),
  rel("flatfile_world_item_repository.c"),
  rel("flatfile_artifact_repository.c"),
  rel("flatfile_shopkeeper_repository.c"),
  rel("flatfile_auction_repository.c"),
  rel("flatfile_boon_repository.c"),
  rel("flatfile_player_domain_repository.c"),
  rel("flatfile_ip_activity_repository.c"),
  rel("flatfile_authority_transaction.c"),
  rel("flatfile_store.c"),
  rel("player_snapshot_codec.c"),
  rel("item_transfer_command.c"), rel("quest_mobile_native_reference.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"),
  rel("corpse_lifecycle_command.c"),
  rel("shop_trade_command.c"), rel("shop_trade_recovery_manifest.c"),
  rel("critical_command.c"),
  rel("epic_command.c"),
  rel("currency_command.c"),
  rel("auction_command.c"),
  rel("combat_outcome_command.c"),
  rel("boon_reward_command.c"),
  rel("boon_shop_command.c"),
  rel("persistence_mode.c"),
]

with tempfile.TemporaryDirectory(prefix="duris-flat-craft-test-") as temporary:
  temporary_path = pathlib.Path(temporary)
  binary = temporary_path / "flatfile_craft_conservation"
  compile_result = subprocess.run(
    [
      "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
      "-D__NO_MYSQL__", "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
      "-DDURIS_FLATFILE_ACCOUNTING_TEST", "-Isrc",
      "-Isrc/no_mysql", *sources, "-lcrypto", "-pthread", "-o", str(binary),
    ], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
  )
  if compile_result.returncode:
    raise SystemExit(compile_result.stdout)
  if os.environ.get("ISSUE551_RETAIN_FIXTURE_BINARY") == "1":
    retained = ROOT / "bin/tests/issue551-flatfile-fixture"
    retained.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(binary, retained)
  state_root = temporary_path / "state"
  run_result = subprocess.run(
    [str(binary), str(state_root)], cwd=ROOT, text=True,
    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
  )
  if run_result.returncode:
    raise SystemExit(run_result.stdout)
  print(run_result.stdout.strip())
