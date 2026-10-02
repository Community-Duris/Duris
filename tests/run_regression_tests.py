#!/usr/bin/env python3
"""Discover and run Duris' plain-Python regression tests."""

from __future__ import annotations

import argparse
import ast
import json
import os
import re
import signal
import subprocess
import sys
import tempfile
import threading
import time
from concurrent.futures import FIRST_COMPLETED, Future, ThreadPoolExecutor, wait
from dataclasses import asdict, dataclass, field
from pathlib import Path
import xml.etree.ElementTree as ET

sys.path.insert(0, str(Path(__file__).resolve().parent))
from regression_inventory import PROFILES, TestSpec, inventory, select


ROOT = Path(__file__).resolve().parent.parent
TEST_DIRECTORY = ROOT / "tests" / "async"
MANIFEST = Path(__file__).with_name("regression_manifest.json")
ENTRY_ADAPTER = Path(__file__).with_name("run_test_entry.py")
MAX_AUTOMATIC_JOBS = 8
DEFAULT_TIMEOUT = 900.0
HEARTBEAT_SECONDS = 30.0
TERMINATE_GRACE_SECONDS = 2.0


@dataclass(frozen=True)
class TestResult:
    path: Path
    returncode: int
    output: str
    elapsed: float
    status: str = "passed"
    skipped_checks: int = 0
    cases: tuple[dict, ...] = ()
    profile: str = "core"
    phases: dict[str, float] = field(default_factory=dict)


def discover_tests(match: str | None) -> list[Path]:
    return [spec.path for spec in select(inventory(TEST_DIRECTORY, MANIFEST), "core", match)]


def automatic_jobs() -> int:
    return min(MAX_AUTOMATIC_JOBS, max(1, os.cpu_count() or 1))


def terminate_test(process: subprocess.Popen) -> str:
    """Stop the whole test group, even when its leader exits before its children."""
    if os.name == "posix":
        def send(sig: int) -> None:
            try:
                os.killpg(process.pid, sig)
            except ProcessLookupError:
                pass

        send(signal.SIGTERM)
        try:
            process.communicate(timeout=TERMINATE_GRACE_SECONDS)
        except subprocess.TimeoutExpired:
            pass
        # A descendant can close stdout and ignore TERM after the leader exits.
        # Always kill the group; waiting only for the leader would leak it.
        send(signal.SIGKILL)
    else:
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)
        if process.poll() is None:
            process.kill()
    try:
        return process.communicate(timeout=TERMINATE_GRACE_SECONDS)[0] or ""
    except subprocess.TimeoutExpired as error:
        # A process that deliberately detached from the group may still hold
        # the pipe. The runner's deadline must also bound draining that pipe.
        output = error.output or ""
        if process.stdout is not None:
            process.stdout.close()
        process.wait(timeout=TERMINATE_GRACE_SECONDS)
        return output.decode(errors="replace") if isinstance(output, bytes) else output


def skip_count(output: str) -> tuple[bool, int]:
    """Report explicit whole-script and unittest skips without treating prose as a skip."""
    lines = output.strip().splitlines()
    explicit = sum(line.startswith("SKIP:") for line in lines)
    if explicit:
        return explicit == len(lines), explicit
    skipped = re.search(r"^OK \(skipped=(\d+)\)$", output, re.MULTILINE)
    ran = re.search(r"^Ran (\d+) tests? in ", output, re.MULTILINE)
    if skipped and ran:
        count = int(skipped[1])
        return count == int(ran[1]), count
    return False, 0


def run_test(path: Path, timeout: float = DEFAULT_TIMEOUT,
             stop: threading.Event | None = None, spec: TestSpec | None = None,
             *, arguments=(), environment=None, prefix=(), observer_uid=None) -> TestResult:
    started = time.monotonic()
    stop = stop if stop is not None else threading.Event()
    if stop.is_set():
        return TestResult(path, 130, "cancelled before starting", 0.0, "cancelled")
    if spec is None:
        tree = ast.parse(path.read_text())
        unit = any(isinstance(node, ast.ClassDef) and any(
            isinstance(base, ast.Attribute) and base.attr == "TestCase" for base in node.bases)
                   for node in ast.walk(tree))
        spec = TestSpec(path, mode="unittest" if unit else "script")
    events = tempfile.TemporaryDirectory(prefix="regression-cases-")
    if observer_uid is not None:
        os.chown(events.name, observer_uid, observer_uid)
    event_path = Path(events.name) / "cases.json"
    try:
        process = subprocess.Popen(
            [*prefix, sys.executable, str(ENTRY_ADAPTER), str(path.resolve()), spec.mode,
             str(event_path), str(spec.minimum_cases), *arguments], cwd=ROOT, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, errors="replace",
            env=dict(os.environ if environment is None else environment,
                     DURIS_NATIVE_BUILD_JOBS=str(min(2, spec.cpu))),
            start_new_session=os.name == "posix",
        )
    except OSError as error:
        events.cleanup()
        return TestResult(path, 127, str(error), time.monotonic() - started, "error")
    def finish(code, output, status, skipped=0):
        try:
            observation = json.loads(event_path.read_text())
            cases = observation["cases"]
        except (OSError, ValueError, KeyError):
            observation = {}
            cases = []
        if not code and observation.get("complete") is not True:
            code, status = 1, "error"
            output += "\ncase execution contract failed: entry ended without complete observation\n"
        for row in cases:
            row["id"] = relative(path) + "::" + row["id"]
            if row["status"] == "running":
                row["status"] = status if code else "error"
            if status == "skipped" and row["id"].endswith("::script"):
                row["status"] = "skipped"
                row["reason"] = "explicit entry-level prerequisite skip"
        observed_skips = sum(row["status"] == "skipped" for row in cases)
        skipped = max(skipped, observed_skips)
        if not code and spec.mode == "unittest" and cases and observed_skips == len(cases):
            status = "skipped"
        events.cleanup()
        elapsed = time.monotonic() - started
        phases = {name: 0.0 for name in ("native_compile", "native_link", "artifact_lookup", "server_build")}
        for match in re.finditer(r"NATIVE_BUILD \w+ name=\S+ compile=([0-9.]+)s link=([0-9.]+)s lookup=([0-9.]+)s", output):
            for name, value in zip(("native_compile", "native_link", "artifact_lookup"), match.groups()):
                phases[name] += float(value)
        for match in re.finditer(r"SERVER_BUILD \w+ build=([0-9.]+)s lookup=([0-9.]+)s", output):
            phases["server_build"] += float(match[1])
            phases["artifact_lookup"] += float(match[2])
        phases["entry_other"] = max(0.0, elapsed - sum(phases.values()))
        return TestResult(path, code, output, elapsed,
                          status, skipped, tuple(cases), spec.profile, phases)
    while True:
        remaining = timeout - (time.monotonic() - started)
        if stop.is_set() or remaining <= 0:
            cancelled = stop.is_set()
            output = terminate_test(process)
            reason = "cancelled" if cancelled else f"timed out after {timeout:g}s"
            return finish(130 if cancelled else 124, output + f"\n{reason}\n",
                          "cancelled" if cancelled else "timeout")
        try:
            output = process.communicate(timeout=min(0.25, remaining))[0]
            break
        except subprocess.TimeoutExpired:
            continue
    entirely_skipped, skipped = skip_count(output)
    status = ("signal" if process.returncode < 0 else "failed") if process.returncode else (
        "skipped" if entirely_skipped else "passed")
    return finish(process.returncode, output, status, skipped)


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--jobs",
        type=int,
        default=0,
        help="parallel workers (0: automatic, capped at 8)",
    )
    parser.add_argument(
        "--match",
        metavar="TEXT",
        help="only run tests whose filename contains TEXT",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="list discovered tests without running them",
    )
    parser.add_argument("--timeout", type=float, default=None, metavar="SECONDS",
                        help="override the entry's preserved 900s/1800s deadline")
    parser.add_argument("--report", type=Path, default=ROOT / "bin/test-results.json",
                        help="write timing/outcome JSON (default: bin/test-results.json)")
    parser.add_argument("--profile", choices=PROFILES, default="core",
                        help="core retains every automatic entry; other profiles select explicit metadata")
    parser.add_argument("--cpu-budget", type=int, default=None, help="CPU reservations available to tests")
    parser.add_argument("--memory-mb", type=int, default=4096, help="test memory reservation budget")
    parser.add_argument("--durations", type=Path, help="previous JSON report for longest-first scheduling")
    parser.add_argument("--junit", type=Path, help="write case-level JUnit XML")
    args = parser.parse_args()
    if args.jobs < 0:
        parser.error("--jobs must be zero or greater")
    if args.timeout is not None and (not 0 < args.timeout < float("inf")):
        parser.error("--timeout must be finite and greater than zero")
    if args.memory_mb < 1 or (args.cpu_budget is not None and args.cpu_budget < 1):
        parser.error("resource budgets must be positive")
    return args


def write_report(path: Path, results: list[TestResult], elapsed: float, interrupted: bool,
                 *, profile="core", planned: list[Path] | None = None) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    completed = {result.path for result in results}
    payload = {"version": 2, "profile": profile, "elapsed": elapsed, "interrupted": interrupted,
               "pending": [relative(item) for item in (planned or []) if item not in completed],
               "results": [
        {**{key: value for key, value in asdict(result).items() if key not in {"path", "output"}},
         "path": relative(result.path)} for result in sorted(results, key=lambda result: result.path)
    ]}
    temporary = path.with_name(path.name + f".{os.getpid()}.tmp")
    try:
        temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def write_junit(path: Path, results: list[TestResult]) -> None:
    suite = ET.Element("testsuite", name="duris.regressions")
    for result in results:
        rows = result.cases or ({"id": relative(result.path) + "::entry",
                                "status": result.status, "elapsed": result.elapsed},)
        for row in rows:
            node = ET.SubElement(suite, "testcase", name=row["id"], classname=result.profile,
                                 time=str(row.get("elapsed", 0)))
            status = row["status"]
            if status in {"skipped", "expected_failure", "cancelled"}:
                ET.SubElement(node, "skipped", message=row.get("reason", status))
            elif status not in {"passed"}:
                ET.SubElement(node, "failure", message=status)
        # A post-case/module failure must remain visible even if every observed
        # unittest case passed before the entry failed.
        if result.returncode and all(row["status"] in {"passed", "skipped", "expected_failure"} for row in rows):
            ET.SubElement(ET.SubElement(suite, "testcase", name=relative(result.path) + "::entry"),
                          "failure", message=result.status)
    suite.set("tests", str(len(suite)))
    suite.set("failures", str(sum(node.find("failure") is not None for node in suite)))
    suite.set("skipped", str(sum(node.find("skipped") is not None for node in suite)))
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + f".{os.getpid()}.tmp")
    ET.ElementTree(suite).write(temporary, encoding="utf-8", xml_declaration=True)
    os.replace(temporary, path)


def cpu_capacity() -> int:
    cpus = automatic_jobs()
    try:
        quota, period = Path("/sys/fs/cgroup/cpu.max").read_text().split()
        if quota != "max":
            cpus = min(cpus, max(1, int(quota) // int(period)))
    except (OSError, ValueError):
        pass
    # A two-worker compiler can still run on a one-core host; reservations
    # limit overlap rather than promising physical cores to a child.
    return max(2, cpus)


def schedule(specs: list[TestSpec], jobs: int, cpu_budget: int, memory_mb: int,
             stop: threading.Event, report, *, timeout=None, durations=None) -> None:
    estimates = durations or {}
    queued = sorted(specs, key=lambda spec: (-estimates.get(relative(spec.path), spec.seconds),
                                             spec.path.name))
    running: dict[Future[TestResult], TestSpec] = {}
    started = time.monotonic()
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        while queued or running:
            if stop.is_set():
                for spec in queued:
                    report(TestResult(spec.path, 130, "cancelled before starting", 0.0, "cancelled",
                                      profile=spec.profile))
                queued.clear()
            used_cpu = sum(spec.cpu for spec in running.values())
            used_memory = sum(spec.memory_mb for spec in running.values())
            locks = {lock for spec in running.values() for lock in spec.locks}
            for spec in list(queued):
                if len(running) >= jobs:
                    break
                if (used_cpu + spec.cpu > cpu_budget or used_memory + spec.memory_mb > memory_mb
                        or locks.intersection(spec.locks)):
                    continue
                queued.remove(spec)
                deadline = timeout or spec.timeout_seconds
                running[executor.submit(run_test, spec.path, deadline, stop, spec)] = spec
                used_cpu += spec.cpu
                used_memory += spec.memory_mb
                locks.update(spec.locks)
            if not running:
                if queued:
                    raise ValueError("queued test cannot fit the configured resource budget")
                break
            done, _ = wait(running, timeout=HEARTBEAT_SECONDS, return_when=FIRST_COMPLETED)
            if not done:
                print(f"Still running ({time.monotonic()-started:.1f}s elapsed): "
                      + ", ".join(relative(spec.path) for spec in running.values()), flush=True)
            for future in done:
                running.pop(future)
                report(future.result())


def main() -> int:
    args = parse_args()
    try:
        specs = select(inventory(TEST_DIRECTORY, MANIFEST), args.profile, args.match)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    tests = [spec.path for spec in specs]
    if args.list:
        for path in tests:
            print(relative(path))
        print(f"{len(tests)} test(s)")
        return 0
    if not tests:
        print("error: no regression tests matched", file=sys.stderr)
        return 2
    manual = [spec for spec in specs if spec.manual]
    if manual:
        for spec in manual:
            print(f"error: {relative(spec.path)} requires its documented manual fixture: "
                  + spec.reason, file=sys.stderr)
        return 2
    cpus = args.cpu_budget or cpu_capacity()
    too_large = [spec for spec in specs if spec.cpu > cpus or spec.memory_mb > args.memory_mb]
    if too_large:
        print("error: resource budget cannot fit: "
              + ", ".join(relative(spec.path) for spec in too_large), file=sys.stderr)
        return 2
    durations = {}
    history = args.durations or args.report
    if history.is_file():
        try:
            durations = {row["path"]: float(row["elapsed"])
                         for row in json.loads(history.read_text())["results"]
                         if row["status"] == "passed" and float(row["elapsed"]) >= 0}
        except (OSError, ValueError, KeyError, TypeError) as error:
            print(f"error: invalid duration report: {error}", file=sys.stderr)
            return 2
    os.environ.setdefault("DURIS_REGRESSION_BUILD_CACHE", str(ROOT / "bin/regression-artifacts"))
    jobs = args.jobs or cpus
    started = time.monotonic()
    results: list[TestResult] = []
    stop = threading.Event()
    interrupted = False
    print(f"Running {len(tests)} Python regression tests with {jobs} worker(s); "
          f"profile={args.profile}, CPU budget={cpus}, memory budget={args.memory_mb} MiB", flush=True)

    def snapshot():
        write_report(args.report, results, time.monotonic()-started, interrupted,
                     profile=args.profile, planned=tests)

    def report(result: TestResult) -> None:
        results.append(result)
        status = {"passed": "PASS", "skipped": "SKIP"}.get(result.status, result.status.upper())
        print(f"[{len(results):>{len(str(len(tests)))}}/{len(tests)}] "
              f"{status} {relative(result.path)} ({result.elapsed:.2f}s; "
              f"{len(result.cases)} observed case(s))", flush=True)
        builds = re.findall(r"(?:SERVER_BUILD|NATIVE_BUILD) (built|reused)[^\n]*", result.output)
        if builds:
            for line in result.output.splitlines():
                if line.startswith(("SERVER_BUILD ", "NATIVE_BUILD ")):
                    print("    " + line, flush=True)
        if result.returncode:
            print(f"\n--- {relative(result.path)} output ---", flush=True)
            print(result.output.rstrip() or "(no output)", flush=True)
        elif result.skipped_checks:
            print(f"    {result.skipped_checks} check(s) explicitly skipped", flush=True)
        snapshot()

    def cancel(signum, frame) -> None:
        nonlocal interrupted
        if not interrupted:
            print("\nCancelling running tests and their children...", flush=True)
        interrupted = True
        stop.set()

    snapshot()
    previous_handler = signal.signal(signal.SIGINT, cancel)
    try:
        schedule(specs, jobs, cpus, args.memory_mb, stop, report,
                 timeout=args.timeout, durations=durations)
    finally:
        signal.signal(signal.SIGINT, previous_handler)
        snapshot()
        if args.junit:
            write_junit(args.junit, results)
    elapsed = time.monotonic() - started
    passed = sum(result.status == "passed" for result in results)
    skipped = sum(result.status == "skipped" for result in results)
    failed = sum(result.returncode != 0 for result in results)
    incomplete = args.profile in {"database", "recovery"} and any(
        result.status == "skipped" or result.skipped_checks for result in results)
    print(f"\n{passed} passed, {skipped} skipped, {failed} failed in {elapsed:.2f}s")
    if incomplete:
        print("error: requested integration profile has skipped required checks", file=sys.stderr)
    print("Slowest tests:")
    for result in sorted(results, key=lambda result: result.elapsed, reverse=True)[:10]:
        print(f"  {result.elapsed:8.2f}s {relative(result.path)} ({result.status})")
    print(f"Results: {args.report}")
    return 130 if interrupted else 1 if failed or incomplete else 0


if __name__ == "__main__":
    raise SystemExit(main())
