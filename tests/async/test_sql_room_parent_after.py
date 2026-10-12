#!/usr/bin/env python3
"""Owned ROOM parent value projection; no SQL, runtime or durability qualification."""
import argparse
import hashlib
import json
import os
import sys
sys.dont_write_bytecode = True
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=ROOT)
parser.add_argument("--proposal-root", type=Path, default=ROOT)
parser.add_argument("--dependency-root", type=Path, default=ROOT)
parser.add_argument("--parent-provider-root", type=Path, default=ROOT)
parser.add_argument("--artifact-dir", type=Path)
args = parser.parse_args()
source_root = args.source_root.resolve()
proposal = args.proposal_root.resolve()
dependency = args.dependency_root.resolve()
parent_provider = args.parent_provider_root.resolve()
for root in (source_root, proposal, dependency, parent_provider):
    if not root.is_dir():
        parser.error("Source/proposal/dependency/provider roots must be existing directories")
if args.artifact_dir is None:
    temp_root = Path(tempfile.gettempdir()).resolve()
    if any(temp_root.is_relative_to(root) for root in (source_root, proposal, dependency, parent_provider)):
        parser.error("Temporary artifacts must be outside the immutable input roots")
    task_root = Path(tempfile.mkdtemp(prefix="duris-sql-room-parent-", dir=temp_root))
    output = task_root / "evidence"
    build_root = task_root / "build"
else:
    output = args.artifact_dir.resolve()
    build_root = output
if any(output.is_relative_to(root) for root in (source_root, proposal, dependency, parent_provider)):
    parser.error("Artifacts must be outside the immutable input roots")
output.mkdir(parents=True, exist_ok=False)
if build_root != output:
    build_root.mkdir(parents=True, exist_ok=False)
print("ARTIFACT_DIR " + str(output), flush=True)
sources = [source_root / "src/persistence/sql_room_item_payload.c",
           dependency / "src/player/player_snapshot_codec.c",
           dependency / "src/item/item_transfer_command.c",
           parent_provider / "src/item/item_transfer_room_parents.c",
           proposal / "tests/async/sql_room_parent_after_test.cpp"]
binary = build_root / "sql_room_parent_after"
command = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
           "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
           "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
           "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
           "-I" + str(parent_provider / "src"), "-I" + str(source_root / "src"),
           "-I" + str(dependency / "src"), *map(str, sources), "-lcrypto", "-o", str(binary)]
build = subprocess.run(command, cwd=source_root, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=180)
(output / "build.log").write_text(build.stdout)
receipt = {"scope": __doc__, "argv": command, "compile_exit": build.returncode,
           "link_scope": "Complete translation units; unused SQL/gameplay sections discarded",
           "inputs_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                             for p in sources + [source_root / "src/persistence/sql_room_item_payload.h",
                               parent_provider / "src/item/item_transfer_room_parents.h",
                               Path(__file__).resolve()]},
           "limits_seconds": {"compile": 180, "run": 30}}
if build.returncode == 0:
    run = subprocess.run([str(binary)], cwd=source_root, text=True, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, timeout=30)
    receipt.update(run_exit=run.returncode,
                   binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
    (output / "runtime.log").write_text(run.stdout)
    print(run.stdout.strip())
else:
    print(build.stdout)
(output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
raise SystemExit(receipt.get("run_exit", build.returncode))
