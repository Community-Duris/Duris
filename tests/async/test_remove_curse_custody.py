#!/usr/bin/env python3
"""Native production REMOVE-CURSE custody regression owner.

Compile the actual spell unit; retained native RED and control invocations may
be selected independently. This owner does not qualify persistence or replay.
"""
from pathlib import Path
import argparse
from contextlib import nullcontext
import hashlib
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CASES = ("inactive_selection", "no_consent", "object_uncurse", "active_cursed_selection")
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--case", choices=CASES, action="append")
parser.add_argument("--artifact-dir", type=Path, help="Fresh private directory retaining RED or AFTER binary and receipt")
args = parser.parse_args()
if args.artifact_dir:
    args.artifact_dir.mkdir(parents=True, exist_ok=False)
    directory_owner = nullcontext(str(args.artifact_dir.resolve()))
else:
    directory_owner = tempfile.TemporaryDirectory(prefix="duris-remove-curse-")
with directory_owner as directory:
    binary = Path(directory) / "remove_curse_custody"
    subprocess.run([
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-ffunction-sections", "-fdata-sections", "-Isrc",
        "tests/async/remove_curse_custody_harness.cpp",
        "src/magic/spell_item_enhancement.c", "-Wl,--gc-sections", "-o", str(binary),
    ], cwd=ROOT, check=True, timeout=120)
    if args.artifact_dir:
        receipt = {
            "status": "compiled_case_results_follow_in_native_log",
            "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
            "inputs": {
                name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                for name in (
                    "src/magic/spell_item_enhancement.c",
                    "tests/async/test_remove_curse_custody.py",
                    "tests/async/remove_curse_custody_harness.cpp",
                )
            },
            "cases": args.case or list(CASES),
            "limits": {"compile_seconds": 120, "case_seconds": 10},
            "scope": "production spell unit with controlled native doubles; no backend or player journey qualification",
        }
        (Path(directory) / "compiled-owner.json").write_text(
            json.dumps(receipt, indent=2) + "\n", encoding="utf-8"
        )
    for case in args.case or CASES:
        subprocess.run([str(binary), case], cwd=ROOT, check=True, timeout=10)
        print(f"REMOVE-CURSE native case passed: {case}", flush=True)
