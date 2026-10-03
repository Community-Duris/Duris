#!/usr/bin/env python3
"""Native unresolved journal replay with real codec/journal and controlled apply.

Linux C++20, SQL header mode or flatfile, ASan/UBSan. No database connection,
Redis, game, migrations, or service startup. Only fresh private journal files are
written. The declared budgets are 300s compile and 120s aggregate runtime.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
HARNESS = Path(__file__).with_name("player_save_journal_unresolved_harness.cpp")
SOURCES = (
    "src/player/player_snapshot_codec.c",
    "src/player/player_save_journal.c",
    "src/persistence/persistence_observability.c",
)
CASES = (
    'first_badalloc',
    'late_ordinary_badalloc',
    'late_ordinary_retry',
    'late_ordinary_ambiguous',
    'late_death_badalloc',
    'late_death_retry',
    'late_death_ambiguous',
    'late_quest_badalloc',
    'late_quest_retry',
    'late_quest_ambiguous',
    'late_spell_badalloc',
    'late_spell_retry',
    'late_spell_ambiguous',
    'late_craft_badalloc',
    'late_craft_retry',
    'late_craft_ambiguous',
    'first_badalloc_after_effect',
    'checkpoint_badalloc_repair',
    'checkpoint_retry_repair',
    'runtime_control',
    'custody_control',
    'death_control',
    'enomem_control',
)
COMPILE_SECONDS = 300
RUNTIME_SECONDS = 120


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inputs(root):
    paths = [root / name for name in SOURCES]
    paths.extend(sorted((root / "src").rglob("*.h")))
    result = {str(path.relative_to(root)): sha(path) for path in paths}
    result["owner-runner"] = sha(__file__)
    result["owner-harness"] = sha(HARNESS)
    return result


def build(root, binary, backend):
    if binary.exists() or binary.with_suffix(binary.suffix + ".json").exists():
        raise ValueError("immutable artifact destination already exists")
    before = inputs(root)
    argv = [
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-pthread", "-O1", "-g0",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie",
        "-no-pie", "-ffunction-sections", "-fdata-sections",
        f"-I{root / 'src'}",
    ]
    if backend == "flat":
        argv.append("-D__NO_MYSQL__")
    else:
        argv.extend(shlex.split(subprocess.check_output(
            ["mysql_config", "--cflags"], text=True)))
    argv.append(str(HARNESS))
    argv.extend(str(root / name) for name in SOURCES)
    argv.extend(["-Wl,--gc-sections", "-Wl,--wrap=fdatasync"])
    if backend == "sql":
        argv.extend(shlex.split(subprocess.check_output(
            ["mysql_config", "--libs"], text=True)))
    argv.extend(["-o", str(binary)])
    binary.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    result = subprocess.run(argv, cwd=root, timeout=COMPILE_SECONDS, check=False)
    elapsed = time.monotonic() - started
    after = inputs(root)
    if before != after:
        raise RuntimeError("source or fixture changed during native compilation")
    result.check_returncode()
    metadata = {
        "scope": "actual journal/codec/observability; controlled repository apply",
        "backend": backend,
        "source_root": str(root),
        "inputs": before,
        "compile_argv": argv,
        "compile_seconds": elapsed,
        "compile_budget_seconds": COMPILE_SECONDS,
        "runtime_budget_seconds": RUNTIME_SECONDS,
        "contract": "unresolved_same_pid",
        "binary_sha256": sha(binary),
        "cases": CASES,
        "source_inputs_equal_before_after": True,
        "excluded": ["actual SQL apply", "worker parking integration", "restored PID admission/ACK bypass",
                     "startup replay gate", "stale frame release", "general allocation safety", "actual SQL transaction/lease cleanup", "Redis", "gameplay"],
    }
    binary.with_suffix(binary.suffix + ".json").write_text(
        json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({"binary": str(binary), "sha256": metadata["binary_sha256"],
                      "compile_seconds": elapsed, "contract": "unresolved_same_pid"}), flush=True)
    return metadata


def verify(root, binary, backend, expected_sha):
    metadata = json.loads(binary.with_suffix(binary.suffix + ".json").read_text())
    if (metadata["backend"] != backend or metadata["inputs"] != inputs(root)
            or metadata["binary_sha256"] != sha(binary)
            or (expected_sha is not None and sha(binary) != expected_sha)
            or metadata["compile_budget_seconds"] != COMPILE_SECONDS
            or metadata["runtime_budget_seconds"] != RUNTIME_SECONDS):
        raise RuntimeError("supplied immutable artifact/backend/source/fixture identity mismatch")
    return metadata


def execute(root, binary, metadata, evidence):
    original = sha(binary)
    before = inputs(root)
    deadline = time.monotonic() + RUNTIME_SECONDS
    observations = []
    # The path is fresh and task-owned. No service is contacted and no existing
    # journal can be overwritten. Failed case directories remain in the evidence.
    evidence.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    for name in CASES:
        started = time.monotonic()
        result = subprocess.run(
            [str(binary), name, str(evidence / (name + "-journal"))], cwd=root,
            env=environment, capture_output=True, text=True,
            timeout=max(0.001, deadline - started), check=False,
        )
        log = evidence / (name + ".log")
        log.write_text(result.stdout + result.stderr)
        observations.append({"case": name, "returncode": result.returncode,
                             "seconds": time.monotonic() - started,
                             "log_sha256": sha(log)})
        print(result.stdout + result.stderr, end="", flush=True)
    if original != sha(binary) or before != inputs(root):
        raise RuntimeError("immutable artifact or source changed during native runtime")
    declaration = {
        "binary_sha256": original,
        "metadata_sha256": sha(binary.with_suffix(binary.suffix + ".json")),
        "backend": metadata["backend"], "contract": metadata["contract"],
        "inputs": before, "observations": observations,
        "inputs_and_binary_unchanged": True,
        "scope": metadata["scope"], "excluded": metadata["excluded"],
    }
    (evidence / "results.json").write_text(json.dumps(declaration, indent=2) + "\n")
    return int(any(item["returncode"] != 0 for item in observations))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--backend", choices=("sql", "flat"), default="sql")
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--binary-sha256")
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--run-supplied", action="store_true")
    parser.add_argument("--evidence", type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    if args.compile_only and args.run_supplied:
        parser.error("compile-only and run-supplied are mutually exclusive")
    if args.run_supplied and (args.binary is None or args.binary_sha256 is None):
        parser.error("supplied runtime requires explicit immutable binary and SHA")
    if args.compile_only and args.binary is None:
        parser.error("compile-only requires unique immutable binary path")
    if args.binary is not None:
        binary = args.binary.resolve()
        metadata = (verify(root, binary, args.backend, args.binary_sha256)
                    if args.run_supplied else build(root, binary, args.backend))
        if args.compile_only:
            return 0
        if args.evidence is None:
            parser.error("persistent binary execution requires a new evidence directory")
        return execute(root, binary, metadata, args.evidence.resolve())
    with tempfile.TemporaryDirectory(prefix="duris-save-journal-unresolved-") as temporary:
        binary = Path(temporary) / "native"
        metadata = build(root, binary, args.backend)
        return execute(root, binary, metadata, Path(temporary) / "evidence")


if __name__ == "__main__":
    raise SystemExit(main())
