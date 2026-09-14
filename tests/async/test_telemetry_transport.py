#!/usr/bin/env python3
"""Focused bounded telemetry transport/queue tests.

The harness uses only an injected fixed-size repository and clock.  It never
opens SQL, the filesystem, or a network endpoint.  Sanitizer variants exercise
the same producer/worker stress case; TSAN is attempted and reports when the
host runtime cannot start it.
"""
from __future__ import annotations

import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
HARNESS = ROOT / "tests/async/telemetry_transport_harness.cc"
REVIEW = ROOT / "tests/async/telemetry_transport_review.cc"
REVIEW_CASES = ("worker-init", "control-reserve", "immutable-rejected-key",
                "producer-order", "ambiguous-invalid-barrier", "missing-clock-flush",
                "stop-init-race", "bounded-loss", "duplicate-results", "stop-worker-init", "mixed-retry-barrier",
                "stop-invalid-init", "cancelled-empty-start", "disabled-init-loss")
SOURCES = [SRC / "telemetry/telemetry_queue.c", SRC / "telemetry/telemetry_transport.c"]


def run(command: list[str], *, timeout: int = 60, env: dict[str, str] | None = None,
        check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(command, cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=timeout, check=check)


def compile_harness(output: Path, extra: list[str], source: Path = HARNESS) -> None:
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    command = compiler + [
        "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic", "-pthread",
        "-I", str(SRC),
        *extra,
        *(str(item) for item in SOURCES), str(source), "-o", str(output),
    ]
    run(command)


def source_contract() -> None:
    transport = (SRC / "telemetry/telemetry_transport.c").read_text()
    queue = (SRC / "telemetry/telemetry_queue.c").read_text()
    queue_private = (SRC / "telemetry/telemetry_queue_private.h").read_text()
    private = (SRC / "telemetry/telemetry_transport_private.h").read_text()
    combined = transport + queue
    for forbidden in ("std::mutex", "std::thread", "std::condition_variable", "std::vector",
                      "malloc(", "calloc(", "realloc(", "new ", "fopen(", "mysql_"):
        assert forbidden not in combined, f"producer/transport source contains {forbidden}"
    for marker in ("telemetry_transport_bind_for_tests", "quiesce_for_tests",
                   "resume_for_tests", "telemetry_transport_clock_binding"):
        assert marker in private, f"private seam missing: {marker}"
    assert "std::atomic" in queue_private and "producer_head" in queue_private and "consumer_tail" in queue_private
    assert "control_reserve" in queue and "inflight" in transport
    print("Transport source contract: PASS")


def run_variant(name: str, flags: list[str], directory: Path) -> None:
    binary = directory / name
    compile_harness(binary, flags)
    result = run([str(binary)], timeout=90,
                 env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1"})
    print(result.stdout, end="")
    print(f"{name}: PASS")
    review_binary = directory / (name + "_review")
    compile_harness(review_binary, flags, REVIEW)
    for case in REVIEW_CASES:
        reviewed = run([str(review_binary), case], timeout=90,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0:abort_on_error=1"})
        print(reviewed.stdout, end="")


def try_tsan(directory: Path) -> None:
    # A runtime capability probe is not a pass for the actual transport tests.
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    probe = directory / "tsan-runtime-probe"
    command = compiler + ["-std=c++20", "-fsanitize=thread", "-x", "c++", "-",
                          "-o", str(probe)]
    built = subprocess.run(command, input="int main() { return 0; }\n", cwd=ROOT,
                           text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           timeout=60)
    if built.returncode:
        if re.search(r"unrecognized.*fsanitize=thread|cannot find.*(?:tsan|clang_rt.tsan)",
                     built.stdout, re.IGNORECASE):
            print("TSAN: UNSUPPORTED (compiler runtime unavailable)")
            print(built.stdout, end="")
            return
        built.check_returncode()
    control = run([str(probe)], timeout=15, check=False)
    if control.returncode:
        print(control.stdout, end="")
        if control.returncode < 0 or re.search(
                r"unexpected memory mapping|cannot.*shadow", control.stdout, re.IGNORECASE):
            print(f"TSAN: UNSUPPORTED (trivial runtime probe exit {control.returncode}; harness not run)")
            return
        control.check_returncode()
    binary = directory / "telemetry_transport_tsan"
    compile_harness(binary, ["-O1", "-g", "-fsanitize=thread"])
    result = run([str(binary)], timeout=120, env=os.environ.copy(), check=False)
    print(result.stdout, end="")
    if (result.returncode != 0 and
            "FATAL: ThreadSanitizer: unexpected memory mapping" in result.stdout and
            "WARNING: ThreadSanitizer" not in result.stdout):
        print("TSAN: UNSUPPORTED (runtime address-map initialization failure)")
        return
    # Races, unexplained signals, timeouts and other harness errors fail.
    result.check_returncode()
    print("TSAN: PASS")


def main() -> None:
    source_contract()
    output_root = ROOT / "bin/tests"
    output_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-transport-",
                                      dir=output_root) as temporary:
        directory = Path(temporary)
        run_variant("telemetry_transport_normal", ["-O2"], directory)
        run_variant("telemetry_transport_asan", ["-O1", "-g", "-fsanitize=address,undefined",
                                                   "-fno-omit-frame-pointer"], directory)
        run_variant("telemetry_transport_ubsan", ["-O1", "-g", "-fsanitize=undefined"], directory)
        try_tsan(directory)


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
        print(getattr(error, "stdout", "") or "", file=sys.stderr)
        print(f"telemetry transport tests: FAIL: {error}", file=sys.stderr)
        raise
