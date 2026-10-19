#!/usr/bin/env python3
"""Verify coordinator retention of the original fence on uncertain journal append."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel


with tempfile.TemporaryDirectory(prefix="duris-coordinator-uncertain-") as temporary:
    root = Path(temporary)
    binary = root / "coordinator_uncertain"
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread", "-Isrc",
            "tests/async/ram_coordinator_test.cpp", rel("critical_command.c"),
            rel("critical_command_journal.c"), rel("critical_command_coordinator.c"),
            "src/persistence/operation_log.c", "-lz", "-lcrypto",
            "-Wl,--wrap=fsync", "-Wl,--wrap=ftruncate", "-o", str(binary),
        ],
        cwd=ROOT, check=True,
    )
    subprocess.run([str(binary), str(root / "journal")], check=True, timeout=20)

for relative in (
    "src/world/zone_touch_transaction.c",
    "src/world/epic_transaction.c",
    "src/economy/shop_trade_transaction.c",
    "src/economy/auction_transaction.c",
    "src/economy/boon_shop_transaction.c",
    "src/economy/boon_reward_transaction.c",
    "src/economy/currency_transaction.c",
    "src/guild/artifact_guild_transaction.c",
    "src/combat/combat_outcome_transaction.c",
    "src/persistence/corpse_lifecycle_transaction.c",
    "src/item/item_transfer_synthetic.c",
    "src/account/session_audit_transaction.c",
):
    assert "critical_submit_result_keeps_operation" in (ROOT / relative).read_text()

print("critical coordinator uncertain-journal fence checks passed")
