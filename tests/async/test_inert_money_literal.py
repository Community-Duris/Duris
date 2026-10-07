#!/usr/bin/env python3
"""Actual inert money staging with a read-only prototype registry seam.

This qualifies literal zero/one cardinality, refusal and unpublished cleanup;
it does not qualify a SQL receipt, original boot, gameplay or release readiness.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import signal
import subprocess
import tempfile

from _paths import ROOT

CASES = ("zero", "one", "two", "malformed", "spellbook", "identity", "currency")
SOURCES = ("tests/async/inert_money_literal_harness.cpp", "src/player/inert_item_stage.c",
           "src/core/mm.c", "src/core/memory.c")


def bounded(command, seconds):
    process = subprocess.Popen(command, cwd=ROOT, text=True, start_new_session=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        output, _ = process.communicate(timeout=seconds)
    except BaseException:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
        raise
    return process.returncode, output


def pins(paths):
    return {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(paths)}


def source_paths():
    return [p for p in (ROOT / "src").rglob("*")
            if p.is_file() and p.suffix in (".c", ".h", ".cpp", ".hpp")]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--run-artifacts", type=Path)
    args = parser.parse_args()
    if args.compile_only and args.run_artifacts:
        parser.error("select one artifact mode")
    fixtures = [Path(__file__), ROOT / SOURCES[0], Path(__file__).with_name("_paths.py"),
                Path(__file__).with_name("contract_text.py")]
    source_pins, fixture_pins = pins(source_paths()), pins(fixtures)
    with tempfile.TemporaryDirectory(prefix="inert-money-literal-") as temporary:
        directory = args.run_artifacts or args.compile_only or Path(temporary)
        if args.compile_only:
            directory.mkdir(parents=True, exist_ok=False)
        for policy, defines in (("sql_header", []),
                                ("flatfile", ["-D__NO_MYSQL__", "-Isrc/no_mysql"])):
            binary = directory / policy
            receipt_path = directory / (policy + ".json")
            if not args.run_artifacts:
                flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                         "-O1", "-g0", "-fsanitize=address,undefined",
                         "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                         "-ffunction-sections", "-fdata-sections", "-Isrc", *defines]
                if policy == "sql_header":
                    flags += shlex.split(subprocess.check_output(
                        ["mysql_config", "--cflags"], text=True))
                command = [*shlex.split(os.environ.get("CXX", "g++")), *flags,
                           *SOURCES, "-Wl,--gc-sections", "-o", str(binary)]
                rc, output = bounded(command, 300)
                (directory / (policy + "-compile.log")).write_text(output)
                if rc:
                    raise RuntimeError(f"{policy} compile failed:\n{output}")
                receipt = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                           "source_pins": source_pins, "fixture_pins": fixture_pins,
                           "compile_command": command, "compile_budget_seconds": 300,
                           "case_budget_seconds": 30, "cases": CASES, "scope": __doc__}
                receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
            else:
                receipt = json.loads(receipt_path.read_text())
                if receipt["source_pins"] != source_pins or receipt["fixture_pins"] != fixture_pins:
                    raise RuntimeError("artifact source/fixture pin mismatch")
                if receipt["binary_sha256"] != hashlib.sha256(binary.read_bytes()).hexdigest():
                    raise RuntimeError("artifact binary pin mismatch")
            if args.compile_only:
                continue
            for case in CASES:
                rc, output = bounded([str(binary), case], 30)
                (directory / f"{policy}-{case}.log").write_text(output)
                print(f"policy={policy} case={case} exit={rc}\n{output}", flush=True)
                if rc:
                    raise AssertionError(f"{policy}/{case} failed")
        if pins(source_paths()) != source_pins or pins(fixtures) != fixture_pins:
            raise RuntimeError("source/fixture inputs changed during qualification")


if __name__ == "__main__":
    main()
