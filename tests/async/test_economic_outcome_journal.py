#!/usr/bin/env python3
"""Focused MySQL-mode outcome codec and real journal fault composition.

The world-thread, domain publication and recovery boundary in this fixture are
controlled. This does not qualify actual gameplay, admission or a coupled base.
"""
import os
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    work = Path(os.environ.get("DURIS_TEST_SCRATCH", ROOT / "bin/tests")) / "economic-outcome-journal"
    work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="run-", dir=work) as temporary:
        scratch = Path(temporary)
        executable = scratch / "outcome-journal"
        sources = (
            "tests/async/economic_outcome_journal_test.cpp",
            "src/persistence/economic_outcome.c",
            "src/persistence/economic_outcome_journal.c",
            "src/persistence/economic_outcome_coordinator.c",
            "src/economy/economic_accounting_plan.c",
            "src/economy/economic_source_event.c",
            "src/economy/economic_accounting_types.c",
            "src/economy/economic_accounting_intent.c",
            "src/persistence/critical_command.c",
            "src/item/item_transfer_command.c",
            "src/item/lockpick_retirement_continuation.c",
            "src/economy/shop_trade_recovery_manifest.c",
            "src/economy/native_quest_cost.c",
            "src/economy/native_quest_coin_give.c",
            "src/world/quest_mobile_native_reference.c",
            "src/item/craft_pouch_mutation.c",
            "src/combat/chaos_pouch_ledger.c",
            "src/player/player_snapshot_codec.c",
        )
        # The canonical providers contain these helpers before the actual
        # source split. Include separate complete suppliers only in a tree
        # which contains that split; never import private files into a pin.
        sources += tuple(name for name in (
            "src/item/item_transfer_result_codec.c",
            "src/item/item_owner_identity.c",
            "src/player/player_item_snapshot_list_codec.c",
        ) if (ROOT / name).is_file())
        command = shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
            "-I" + str(ROOT / "src"),
        ] + [str(ROOT / name) for name in sources] + [
            "-Wl,--wrap=fsync", "-lcrypto", "-lz", "-pthread", "-o", str(executable)
        ]
        print(json.dumps({"command": command, "providers": sources}), flush=True)
        subprocess.run(command, check=True, timeout=300)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        subprocess.run([str(executable), str(scratch / "journal")],
                       check=True, env=environment, timeout=20)


if __name__ == "__main__":
    main()
