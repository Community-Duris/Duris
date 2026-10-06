#!/usr/bin/env python3
"""Real recovery-context/native-v12 pure values; no world or owner authority.

Run from the composed candidate. No database, Redis, game, world capture or live journal.
Retain all generated evidence on success/failure, including terminal compiler logs.
"""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/async/native_quest_recovery_context_test.cpp",
    "src/economy/item_transfer_accounting.c",
    "src/economy/shop_trade_recovery_manifest.c",
    "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c",
    "src/economy/economic_source_event.c",
    "src/economy/economic_accounting_types.c",
    "src/item/item_transfer_command.c",
    "src/world/quest_mobile_native_reference.c",
    "src/item/craft_pouch_mutation.c",
    "src/combat/chaos_pouch_ledger.c",
    "src/player/player_snapshot_codec.c",
    "src/persistence/critical_command.c",
    "src/world/native_quest_recovery_context.c",
    "src/world/quest_mobile_native.c",
]
FLAGS = [
    "-std=c++20", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
    "-pthread", "-Isrc", "-ffunction-sections", "-fdata-sections",
]
LINK = ["-lcrypto", "-lz", "-Wl,--gc-sections", "-Wl,--wrap=_Znwm"]
RUNTIME_SECONDS = 20  # Existing native journal fault-component deadline.


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parent = ROOT / "bin" / "tests"
    parent.mkdir(parents=True, exist_ok=True)
    if parent.is_symlink() or not parent.resolve().is_relative_to(ROOT.resolve()):
        raise ValueError("artifact parent must be owned below composed candidate/bin/tests")
    artifacts = Path(tempfile.mkdtemp(prefix="native-quest-recovery-context-", dir=parent))
    binary = artifacts / "native_quest_recovery_context"
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    if not compiler:
        raise ValueError("CXX must name the actual compiler")
    command = [*compiler, *FLAGS, *SOURCES, *LINK, "-o", str(binary)]
    receipt = {
        "classification": "native pure context component using actual providers",
        "external_services": [], "synthetic_values_only": True,
        "runtime_timeout_seconds": RUNTIME_SECONDS,
        "compile_command": command,
        "source_sha256": {name: sha(ROOT / name) for name in SOURCES},
        "compile_attempts": 0, "successful_compiles": 0,
        "runtime_attempts": 0, "status": "prepared",
        "coverage_boundary": "no coordinator/native lifetime/source authentication, SQL, ACK, crash recovery, activation or release qualification",
    }
    stage = "compile"
    started = time.monotonic()
    try:
        receipt["compile_attempts"] = 1
        with (artifacts / "compile.log").open("wb") as log:
            completed = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        receipt["compile_exit_code"] = completed.returncode
        receipt["compile_elapsed_seconds"] = time.monotonic() - started
        if completed.returncode:
            raise RuntimeError("actual fixture compiler failed; see retained compile.log")
        receipt["successful_compiles"] = 1
        receipt["binary_sha256"] = sha(binary)
        stage = "native_component"
        receipt["runtime_attempts"] = 1
        started = time.monotonic()
        with (artifacts / "native-component.log").open("wb") as log:
            completed = subprocess.run(
                [str(binary)], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT,
                timeout=RUNTIME_SECONDS, env=dict(
                    os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                    UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
        receipt["runtime_exit_code"] = completed.returncode
        receipt["runtime_elapsed_seconds"] = time.monotonic() - started
        if completed.returncode:
            raise RuntimeError("actual fixture failed; see retained native-component.log")
        receipt["status"] = "passed_component_only"
    except Exception as error:
        receipt["status"] = "failed"
        receipt["failure_stage"] = stage
        receipt["failure_type"] = type(error).__name__
        if isinstance(error, subprocess.TimeoutExpired):
            receipt["timeout_seconds"] = error.timeout
        raise
    finally:
        receipt["source_unchanged"] = all(
            sha(ROOT / name) == pin for name, pin in receipt["source_sha256"].items())
        receipt["artifacts_retained"] = True
        (artifacts / "receipt.json").write_text(
            json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print("Recovery context component artifacts:", artifacts.relative_to(ROOT))
    if not receipt["source_unchanged"]:
        raise RuntimeError("linked source changed during qualification")


if __name__ == "__main__":
    main()
