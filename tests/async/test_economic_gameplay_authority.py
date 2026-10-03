#!/usr/bin/env python3
"""Execute lifecycle-issued admission/cache contract (not native activation proof)."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "tests/async/economic_gameplay_authority_test.cpp",
    "src/economy/economic_gameplay_authority.c",
    "src/economy/economic_command_admission.c",
    "src/economy/economic_currency_adapter.c",
    "src/economy/coin_transfer_command.c",
    "src/economy/coin_transfer_accounting.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_accounting_types.c",
    "src/economy/currency_command.c",
    "src/economy/item_transfer_accounting.c",
    "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/player/player_snapshot_codec.c",
    "src/persistence/economic_accounting_repository.c",
    "src/persistence/critical_command.c",
)
with tempfile.TemporaryDirectory(prefix="duris-gameplay-authority-") as temporary:
    for mode in ("sql", "flatfile"):
        executable = Path(temporary) / mode
        command = shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
            "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
            "-pthread", "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST", "-I" + str(ROOT / "src"),
        ]
        if mode == "flatfile":
            command += ["-D__NO_MYSQL__", "-I" + str(ROOT / "src/no_mysql")]
        command += [str(ROOT / source) for source in SOURCES]
        if mode == "sql":
            command.append("-lmysqlclient")
        command += ["-lcrypto", "-o", str(executable)]
        subprocess.run(command, check=True)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        subprocess.run([str(executable)], env=environment, check=True, timeout=45)
        print(mode + " gameplay-authority admission passed", flush=True)
