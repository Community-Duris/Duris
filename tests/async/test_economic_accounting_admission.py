#!/usr/bin/env python3
"""Schema-2 economic transport admission and durable replay without SQL."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "tests/async/economic_accounting_admission_test.cpp",
    "src/economy/economic_command_admission.c",
    "src/economy/economic_currency_adapter.c",
    "src/economy/currency_command.c",
    "src/economy/coin_transfer_command.c",
    "src/economy/coin_transfer_accounting.c",
    "src/economy/item_transfer_accounting.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c", "src/economy/economic_source_event.c",
    "src/economy/economic_accounting_types.c",
    "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/player/player_snapshot_codec.c",
    "src/persistence/critical_command.c",
    "src/persistence/critical_command_journal.c",
    "src/persistence/critical_command_coordinator.c",
)
# Native temporary storage preserves the private directory modes used by journals.
with tempfile.TemporaryDirectory(prefix="duris-economic-admission-") as temporary:
    work = Path(temporary)
    for mode in ("sql", "client-free"):
        executable = work / mode
        command = shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
            "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
            "-pthread", "-I" + str(ROOT / "src"),
        ]
        if mode == "client-free":
            command += ["-D__NO_MYSQL__", "-I" + str(ROOT / "src/no_mysql")]
        command += [str(ROOT / source) for source in SOURCES]
        command += ["-lcrypto", "-lz", "-o", str(executable)]
        subprocess.run(command, check=True)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        subprocess.run([str(executable), str(work / (mode + "-journal"))],
                       env=environment, check=True, timeout=45)
        print(mode + " economic transport passed", flush=True)
