#!/usr/bin/env python3
"""Prepared Plan 2 components; native execution is deferred to the plan milestone.

Actual coin owner/publisher/capture/codec/custody with explicit world/coordinator
seams. This does not qualify gameplay, persistence, or callback-free cold replay.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import shlex
import signal
import subprocess
import tempfile

from _paths import ROOT, rel

OWNER_PATH = Path(__file__).with_name("test_coin_publication_ack_retention.py")
PHYSICAL_CASES = ("drop", "missing_uid", "duplicate_uid", "conflicting_bytes",
                  "conflicting_custody", "water", "falling", "offline_actor",
                  "placement_exception", "materialize_exception", "changed_receipt",
                  "pickup_partial", "pickup_consumed", "pickup_missing",
                  "amount_exception", "extraction_exception", "wrong_actor",
                  "wrong_placement", "stale_item", "masked_blob", "malformed_result",
                  "literal_conflict", "object_cycle", "reentry")
PHYSICAL_SOURCES = ("tests/async/coin_physical_publication_harness.cpp",
                    "src/economy/coin_physical_publication.c",
                    "src/economy/coin_transfer_command.c", "src/economy/currency_command.c",
                    "src/item/item_transfer_command.c", "src/item/item_ownership_runtime.c",
                    "src/item/craft_pouch_mutation.c", "src/item/chaos_pouch_ledger.c",
                    "src/player/player_snapshot_capture.c", "src/player/player_snapshot_codec.c",
                    "src/persistence/critical_command.c")


def bounded(command, seconds, **kwargs):
    process = subprocess.Popen(command, start_new_session=True, **kwargs)
    try:
        out, _ = process.communicate(timeout=seconds)
    except BaseException:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
        raise
    return process.returncode, out


def owner_fixture():
    spec = importlib.util.spec_from_file_location("coin_ack_fixture", OWNER_PATH)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    harness = module.HARNESS
    before = '''check(physical_attempts == CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS,
\t\t\t      "physical attempts bounded");'''
    after = '''check(physical_attempts == CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS + 3,
\t\t\t      "one attempt per pulse remains eligible beyond old lifetime cap");'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture retry anchor changed")
    harness = harness.replace(before, after)
    before = '''"exhaustion cannot finalize or abandon committed publication");'''
    after = before + '''
            physical_available = true;
            throw_physical = false;
            currency_transaction_handle_completions(nullptr, 0);
            check(acks == 1 && !held && notifications == 1 && !notification_before_ack,
                  "repaired dependency ACKs original receipt after many transient failures");'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture recovery anchor changed")
    harness = harness.replace(before, after)
    # ACK retry now verifies the completed physical stage before another ACK.
    before = '''check(physical_successes == 1 && physical_attempts == 1,
\t\t      "ACK retry cannot repeat physical publication");'''
    after = '''check(physical_attempts >= 1,
\t\t      "ACK retry asks publisher to verify the same original operation");'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture ACK anchor changed")
    harness = harness.replace(before, after)
    before = '''if (scenario == "ack_retry" || scenario == "changed_after_physical")'''
    after = '''if (scenario == "ack_retry" || scenario == "fresh_body" || scenario == "changed_after_physical")'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture reconnect setup anchor changed")
    harness = harness.replace(before, after)
    before = '''else if (scenario == "ack_retry")'''
    after = '''else if (scenario == "ack_retry" || scenario == "fresh_body")'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture reconnect branch anchor changed")
    harness = harness.replace(before, after)
    before = '''auto duplicate = completed;
\t\tduplicate.outcome = critical_apply_outcome::already_applied;'''
    after = '''pc_only_data replacement_pc = {};
        char_data replacement_body = {};
        if (scenario == "fresh_body")
        {
            replacement_pc.pid = 42;
            replacement_pc.wallet_revision = replacement_pc.bank_revision = 1;
            replacement_body.only.pc = &replacement_pc;
            replacement_body.player.racewar = 1;
            replacement_body.runtime_id = 999;
            GET_COPPER(&replacement_body) = 5;
            online = &replacement_body;
        }
        auto duplicate = completed;
\t\tduplicate.outcome = critical_apply_outcome::already_applied;'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture fresh body anchor changed")
    harness = harness.replace(before, after)
    before = '''"ACK retry retires domain entry");'''
    after = before + '''
        if (scenario == "fresh_body")
            check(GET_COPPER(online) == 4 && replacement_pc.wallet_revision == 2 &&
                  replacement_pc.bank_revision == 2,
                  "replacement body receives original exact projection before ACK");
        online = &actor;'''
    if harness.count(before) != 1:
        raise RuntimeError("maintained owner fixture fresh body result anchor changed")
    harness = harness.replace(before, after)
    return harness, [rel(name) for name in module.SOURCES], (*module.SCENARIOS, "fresh_body")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--run-artifacts", type=Path)
    parser.add_argument("--scope", choices=("all", "owner", "physical"), default="all")
    args = parser.parse_args()
    if args.compile_only and args.run_artifacts:
        parser.error("select one artifact mode")
    with tempfile.TemporaryDirectory(prefix="coin-physical-") as temporary:
        directory = args.run_artifacts or args.compile_only or Path(temporary)
        if args.compile_only:
            directory.mkdir(parents=True, exist_ok=False)
        owner, owner_sources, owner_cases = owner_fixture()
        owner_cpp = Path(temporary) / "owner.cpp"
        owner_cpp.write_text(owner, encoding="utf-8")
        sources_pin = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                       for p in sorted((ROOT / "src").rglob("*"))
                       if p.is_file() and p.suffix in (".c", ".h", ".cpp", ".hpp")}
        fixture_paths = (Path(__file__), OWNER_PATH, ROOT / PHYSICAL_SOURCES[0],
                         Path(__file__).with_name("_paths.py"))
        fixture_pins = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                        for p in fixture_paths}
        failures = []
        for policy, defines in (("sql_header", []), ("flatfile", ["-D__NO_MYSQL__", "-Isrc/no_mysql"])):
            for scope, sources, cases in (("physical", list(PHYSICAL_SOURCES), PHYSICAL_CASES),
                                           ("owner", [str(owner_cpp), *owner_sources], owner_cases)):
                if args.scope != "all" and args.scope != scope:
                    continue
                name = f"{policy}_{scope}"
                binary = directory / name
                receipt_path = directory / f"{name}.json"
                if not args.run_artifacts:
                    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                             "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                             "-fno-pie", "-no-pie", "-ffunction-sections", "-fdata-sections",
                             "-Isrc", *defines]
                    if scope == "owner":
                        flags += ["-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST", "-DDURIS_COIN_SPLIT_PUBLICATION_TEST"]
                    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
                    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
                    command = [*shlex.split(os.environ.get("CXX", "g++")), *flags,
                               *cflags, *sources, "-Wl,--gc-sections", "-lcrypto", *libs,
                               "-o", str(binary)]
                    rc, out = bounded(command, 300, cwd=ROOT, text=True,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                    if rc:
                        raise RuntimeError(f"{name} compile failed:\n{out}")
                    receipt = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                               "source_pins": sources_pin, "fixture_pins": fixture_pins,
                               "owner_harness_sha256": hashlib.sha256(owner.encode()).hexdigest(),
                               "flags": flags, "compile_command": command,
                               "cases": cases, "compile_budget_seconds": 300,
                               "case_budget_seconds": 30, "scope": __doc__}
                    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
                else:
                    receipt = json.loads(receipt_path.read_text())
                    if receipt["source_pins"] != sources_pin or receipt["fixture_pins"] != fixture_pins:
                        raise RuntimeError("artifact source/fixture pin mismatch")
                    if receipt["binary_sha256"] != hashlib.sha256(binary.read_bytes()).hexdigest():
                        raise RuntimeError("artifact binary pin mismatch")
                if args.compile_only:
                    continue
                for case in cases:
                    rc, out = bounded([str(binary), case], 30, cwd=ROOT, text=True,
                                      stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                    print(f"policy={policy} scope={scope} case={case} exit={rc}\n{out}", flush=True)
                    if rc:
                        failures.append((name, case))
        if failures:
            raise AssertionError(f"Coin physical publication failures: {failures}")
        after_pins = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in sorted((ROOT / "src").rglob("*"))
                      if p.is_file() and p.suffix in (".c", ".h", ".cpp", ".hpp")}
        after_fixtures = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in fixture_paths}
        if after_pins != sources_pin or after_fixtures != fixture_pins:
            raise RuntimeError("source/fixture inputs changed during native qualification")


if __name__ == "__main__":
    main()
