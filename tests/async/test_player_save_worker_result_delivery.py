#!/usr/bin/env python3
"""Native completion delivery after real journal ACK under worker-thread OOM.

Actual worker/revision/codec/journal/observability, controlled repository apply.
SQL-header and flat modes do not connect to SQL. Private journals and immutable
artifacts only. New 300s compile/120s aggregate proposals are unmeasured until
execution. An abort is semantic RED only with real ACK, full original identity,
calibrated worker allocation and independent raw-frame removal evidence.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import re
import signal
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
HARNESS = Path(__file__).with_name("player_save_worker_result_delivery_harness.cpp")
SOURCES = (
    "src/player/player_save_worker.c",
    "src/player/player_revision_state.c",
    "src/player/player_snapshot_codec.c",
    "src/player/player_save_journal.c",
    "src/persistence/persistence_observability.c",
)
CASES = (
    "quest_block", "spell_block", "craft_block", "mixed_map", "unrelated_block",
    "retry_control", "ack_failure_control", "healthy_control", "shutdown_reopen_control",
    "wraparound_control", "partial_wrap_control", "full_shutdown_control",
)
COMPILE_SECONDS = 300
RUNTIME_SECONDS = 120
SCOPE = "actual worker/revision/codec/journal; controlled repository apply"
EXCLUDED = [
    "actual SQL apply", "full-results shutdown discard reachability", "initial/pending admission allocation",
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


def supervised(argv, root, environment, timeout, capture=False):
    process = subprocess.Popen(argv, cwd=root, env=environment, start_new_session=True,
                               stdout=subprocess.PIPE if capture else None,
                               stderr=subprocess.STDOUT if capture else None, text=True)
    try:
        output, _ = process.communicate(timeout=timeout)
        return subprocess.CompletedProcess(argv, process.returncode, output or "", "")
    except BaseException:
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            pass
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.communicate(timeout=5)
        raise


def parsed_frames(path):
    value = path.read_bytes()
    result = []
    offset = 0
    while offset < len(value):
        if len(value) - offset < 72:
            raise ValueError("incomplete native raw frame")
        header = value[offset:offset + 72]
        length = int.from_bytes(header[16:24], "little")
        if int.from_bytes(header[12:16], "little") != 72 or length < 72 or length > len(value) - offset:
            raise ValueError("invalid native raw frame length")
        result.append((int.from_bytes(header[40:44], "little"),
                       int.from_bytes(header[48:56], "little"), value[offset + 72:offset + length]))
        offset += length
    return result


def semantic_abort_proof(directory, output):
    result = {"classification": "setup_or_unqualified_process_failure"}
    try:
        payload = directory / "original-payload.bin"
        digest = sha(payload)
        markers = re.findall(r"REAL_ACK_EXACT pid=(\d+) revision=(\d+) canonical_sha256=([0-9a-f]{64})", output)
        if len(markers) != 1 or markers[0][2] != digest:
            return result
        pid, revision = int(markers[0][0]), int(markers[0][1])
        growth = re.search(r"CALIBRATED prefix=(\d+) allocation_size=([1-9]\d*)", output)
        fault = re.search(r"RESULT_ALLOCATION_FAULT after_real_ack=1 worker_thread=1 allocation_size=([1-9]\d*) ordinal=([1-9]\d*)", output)
        original = parsed_frames(directory / "original-journal.bin")
        current = parsed_frames(directory / "player-save.journal")
        if (sum(p == pid and r == revision and b == payload.read_bytes() for p, r, b in original) != 1
                or any(p == pid for p, _, _ in current)
                or growth is None or fault is None or growth[2] != fault[1]):
            return result
        return {"classification": "semantic_post_ack_delivery_failure", "pid": pid,
                "revision": revision, "canonical_sha256": digest,
                "original_journal_sha256": sha(directory / "original-journal.bin"),
                "remaining_journal_sha256": sha(directory / "player-save.journal"),
                "real_ack_exact": True, "worker_allocation_fault": True,
                "calibrated_prefix": int(growth[1]), "allocation_size": int(fault[1]),
                "allocation_ordinal": int(fault[2]),
                "original_raw_frame_removed": True}
    except (OSError, ValueError):
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
    argv.extend(["-Wl,--gc-sections", "-Wl,--wrap=_Znwm", "-Wl,--wrap=write", "-Wl,--wrap=fdatasync", "-lcrypto"])
    if backend == "sql":
        argv.extend(shlex.split(subprocess.check_output(
            ["mysql_config", "--libs"], text=True)))
    argv.extend(["-o", str(binary)])
    binary.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    result = None
    timed_out = False
    failure = None
    try:
        result = supervised(argv, root, os.environ.copy(), COMPILE_SECONDS, capture=False)
    except BaseException as error:
        timed_out = isinstance(error, subprocess.TimeoutExpired)
        failure = error
    elapsed = time.monotonic() - started
    after = inputs(root)
    attempt = {
        "source_root": str(root), "backend": backend, "inputs": before,
        "compile_argv": argv, "compile_seconds": elapsed,
        "compile_budget_seconds": COMPILE_SECONDS,
        "runtime_budget_seconds": RUNTIME_SECONDS,
        "returncode": None if result is None else result.returncode,
        "timed_out": timed_out, "source_inputs_equal_before_after": before == after,
        "interrupted": failure is not None and not timed_out,
    }
    attempt_path.write_text(json.dumps(attempt, indent=2) + "\n")
    if before != after:
        raise RuntimeError("source or fixture changed during native compilation")
    if timed_out:
        raise RuntimeError("native compilation exceeded declared 300s budget")
    if failure:
        raise failure
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
            or not metadata["source_inputs_equal_before_after"] or metadata["returncode"] != 0
            or metadata["timed_out"] or metadata["interrupted"]):
        raise RuntimeError("supplied immutable artifact/backend/source/fixture identity mismatch")
    return metadata


def execute(root, binary, metadata, evidence):
    original = sha(binary)
    before = inputs(root)
    deadline = time.monotonic() + RUNTIME_SECONDS
    observations = []
    evidence.mkdir(parents=True, exist_ok=False)
    os.chmod(evidence, 0o700)
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    for name in CASES:
        started = time.monotonic()
        try:
            result = supervised(
                [str(binary), name, str(evidence / (name + "-journal"))], root=root,
                environment=environment, capture=True,
                timeout=max(0.001, deadline - started),
            )
            text = result.stdout + result.stderr
            code = result.returncode
        except subprocess.TimeoutExpired as error:
            text = error.stdout or ""
            if isinstance(text, bytes):
                text = text.decode(errors="replace")
            text += "\nAGGREGATE RUNTIME DEADLINE EXCEEDED\n"
            code = 124
        log = evidence / (name + ".log")
        log.write_text(text)
        proof = semantic_abort_proof(evidence / (name + "-journal"), text) if code < 0 else {}
        observations.append({"case": name, "returncode": code, "abort_proof": proof,
                             "seconds": time.monotonic() - started,
                             "log_sha256": sha(log)})
        print(text, end="", flush=True)
        if code == 124:
            break
    for name in CASES[len(observations):]:
        observations.append({"case": name, "returncode": None, "attempted": False,
                             "reason": "aggregate runtime deadline; native case not executed"})
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
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    root = args.source_root.resolve()
    if args.self_test:
        with tempfile.TemporaryDirectory(prefix="result-delivery-guard-") as temporary:
            result = semantic_abort_proof(Path(temporary), "arbitrary process abort")
            assert result["classification"] == "setup_or_unqualified_process_failure"
        assert len(CASES) == 12 and len(set(CASES)) == 12
        print("PASS: abort-without-proof rejected; 12 distinct cases; no native/service execution")
        return 0
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
    with tempfile.TemporaryDirectory(prefix="duris-save-worker-result-delivery-") as temporary:
        binary = Path(temporary) / "native"
        metadata = build(root, binary, args.backend)
        return execute(root, binary, metadata, Path(temporary) / "evidence")


if __name__ == "__main__":
    raise SystemExit(main())
