#!/usr/bin/env python3
"""Whole canonical runtime allocation-fault regression; original provider union."""
from _paths import rel
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-atomic-cache-") as temp_dir:
    binary=Path(temp_dir)/"atomic_item_cache_transfer"
    command=[
		"g++",
			"-std=c++20",
			"-Wall",
			"-Wextra",
			"-Wpedantic",
			"-Werror",
			"-Isrc",
			str(ROOT / "tests/async/atomic_item_cache_transfer_test.cpp"),
			rel("item_ownership_runtime.c"), rel("economic_sql_runtime_cache_correspondence.c"),
			rel("item_transfer_command.c"), rel("quest_mobile_native_reference.c"), rel("economic_source_event.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"),
			rel("player_snapshot_codec.c"),
			rel("critical_command.c"),
			rel("shop_trade_recovery_manifest.c"), rel("lockpick_retirement_continuation.c"),
			rel("native_quest_cost.c"), rel("native_quest_coin_give.c"),
			rel("event_thread_owner.c"), rel("fatal_error.c"), rel("logging.c"),
			"-lcrypto",
			"-o",
			str(binary),
    ]
    subprocess.run(command,cwd=ROOT,check=True,capture_output=True,text=True)
    subprocess.run([str(binary)],cwd=ROOT,check=True)
