#!/usr/bin/env python3
"""Native worker retry/promotion scheduling under caller-thread allocation failures.

Actual worker/revision/codec/journal/observability; controlled repository apply.
Linux C++20 with ASan/UBSan, SQL client headers or flatfile. No SQL connection,
Redis, game, migrations, or service startup. Only fresh private journal files
are written. Declared budgets: 300s compile and 120s aggregate runtime.
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
HARNESS = Path(__file__).with_name("player_save_worker_scheduling_harness.cpp")
SOURCES = (
    "src/player/player_save_worker.c",
    "src/player/player_revision_state.c",
    "src/player/player_snapshot_codec.c",
    "src/player/player_save_journal.c",
    "src/persistence/persistence_observability.c",
)
CASES = (
    "retry_node", "retry_set_growth", "retry_deque_growth",
    "ambiguous_node", "ambiguous_set_growth", "ambiguous_deque_growth",
    "promote_quest_node", "promote_spell_node", "promote_craft_node",
    "promote_mixed_set_growth", "promote_mixed_deque_growth",
    "ordinary_newer_mask", "terminal_control", "stale_control", "exhaustion_control",
)
COMPILE_SECONDS = 300
RUNTIME_SECONDS = 120
SCOPE = "actual worker/revision/codec/journal; controlled repository apply"
EXCLUDED = [
    "actual SQL apply", "results-queue allocation", "initial/pending admission allocation",
    "restored PID integration", "startup replay gate",
    "receipt-bearing component narrowing identity", "stale frame release", "Redis", "gameplay",
]


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
    metadata_path = binary.with_suffix(binary.suffix + ".json")
    attempt_path = binary.with_suffix(binary.suffix + ".attempt.json")
    if binary.exists() or metadata_path.exists() or attempt_path.exists():
        raise ValueError("immutable artifact destination already exists")
    before = inputs(root)
    argv = [
        os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-pthread", "-O1", "-g0",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie",
        "-no-pie", "-ffunction-sections", "-fdata-sections", f"-I{root / 'src'}",
    ]
    if backend == "flat":
        argv.append("-D__NO_MYSQL__")
    else:
        argv.extend(shlex.split(subprocess.check_output(
            ["mysql_config", "--cflags"], text=True)))
    argv.append(str(HARNESS))
    argv.extend(str(root / name) for name in SOURCES)
    argv.extend(["-Wl,--gc-sections", "-Wl,--wrap=_Znwm"])
    if backend == "sql":
        argv.extend(shlex.split(subprocess.check_output(
            ["mysql_config", "--libs"], text=True)))
    argv.extend(["-o", str(binary)])
    binary.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    result = None
    timed_out = False
    try:
        result = subprocess.run(argv, cwd=root, timeout=COMPILE_SECONDS, check=False)
    except subprocess.TimeoutExpired:
        timed_out = True
    elapsed = time.monotonic() - started
    after = inputs(root)
    attempt = {
        "source_root": str(root), "backend": backend, "inputs": before,
        "compile_argv": argv, "compile_seconds": elapsed,
        "compile_budget_seconds": COMPILE_SECONDS,
        "runtime_budget_seconds": RUNTIME_SECONDS,
        "returncode": None if result is None else result.returncode,
        "timed_out": timed_out, "source_inputs_equal_before_after": before == after,
    }
    attempt_path.write_text(json.dumps(attempt, indent=2) + "\n")
    if before != after:
        raise RuntimeError("source or fixture changed during native compilation")
    if timed_out:
        raise RuntimeError("native compilation exceeded declared 300s budget")
    result.check_returncode()
    metadata = dict(attempt, scope=SCOPE, excluded=EXCLUDED,
                    binary_sha256=sha(binary), cases=CASES)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({"binary": str(binary), "sha256": metadata["binary_sha256"],
                      "compile_seconds": elapsed}), flush=True)
    return metadata


def verify(root, binary, backend, expected_sha):
    metadata = json.loads(binary.with_suffix(binary.suffix + ".json").read_text())
    if (metadata["backend"] != backend or metadata["source_root"] != str(root)
            or metadata["inputs"] != inputs(root)
            or metadata["binary_sha256"] != sha(binary)
            or sha(binary) != expected_sha or metadata["cases"] != list(CASES)
            or metadata["compile_budget_seconds"] != COMPILE_SECONDS
            or metadata["runtime_budget_seconds"] != RUNTIME_SECONDS
            or not metadata["source_inputs_equal_before_after"]):
        raise RuntimeError("supplied immutable artifact/backend/source/fixture identity mismatch")
    return metadata


def execute(root, binary, metadata, evidence):
    original = sha(binary)
    before = inputs(root)
    deadline = time.monotonic() + RUNTIME_SECONDS
    observations = []
    evidence.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    for name in CASES:
        started = time.monotonic()
        try:
            result = subprocess.run(
                [str(binary), name, str(evidence / (name + "-journal"))], cwd=root,
                env=environment, capture_output=True, text=True,
                timeout=max(0.001, deadline - started), check=False,
            )
            text = result.stdout + result.stderr
            code = result.returncode
        except subprocess.TimeoutExpired as error:
            text = ((error.stdout or b"") + (error.stderr or b"")).decode(
                errors="replace") + "\nAGGREGATE RUNTIME DEADLINE EXCEEDED\n"
            code = 124
        log = evidence / (name + ".log")
        log.write_text(text)
        observations.append({"case": name, "returncode": code,
                             "seconds": time.monotonic() - started,
                             "log_sha256": sha(log)})
        print(text, end="", flush=True)
        if code == 124:
            break
    unchanged = original == sha(binary) and before == inputs(root)
    declaration = {
        "binary_sha256": original,
        "metadata_sha256": sha(binary.with_suffix(binary.suffix + ".json")),
        "backend": metadata["backend"], "inputs": before,
        "observations": observations, "inputs_and_binary_unchanged": unchanged,
        "scope": SCOPE, "excluded": EXCLUDED,
    }
    (evidence / "results.json").write_text(json.dumps(declaration, indent=2) + "\n")
    if not unchanged:
        raise RuntimeError("immutable artifact or source changed during native runtime")
    return int(len(observations) != len(CASES)
               or any(item["returncode"] != 0 for item in observations))


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
    with tempfile.TemporaryDirectory(prefix="duris-save-worker-scheduling-") as temporary:
        binary = Path(temporary) / "native"
        metadata = build(root, binary, args.backend)
        return execute(root, binary, metadata, Path(temporary) / "evidence")


if __name__ == "__main__":
    raise SystemExit(main())
