#!/usr/bin/env python3
"""Discover and run Duris' plain-Python regression tests."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from concurrent.futures import Future, ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TEST_DIRECTORY = ROOT / "tests" / "async"
MAX_AUTOMATIC_JOBS = 8
RESOURCE_INTENSIVE_TEST_NAMES = frozenset(
    {
        "test_account_recovery_journey.py",
        "test_area_coin_pickup.py",
        "test_flatfile_boot_preflight.py",
        "test_flatfile_chaos_new_character_kit.py",
        "test_flatfile_combat_journey.py",
        "test_flatfile_first_session_currency.py",
        "test_flatfile_full_world_boot.py",
        "test_item_movement_prompt_runtime.py",
    }
)


@dataclass(frozen=True)
class TestResult:
    path: Path
    returncode: int
    output: str
    elapsed: float


def discover_tests(match: str | None) -> list[Path]:
    tests = sorted(TEST_DIRECTORY.glob("test_*.py"))
    if match:
        tests = [path for path in tests if match in path.name]
    return tests


def automatic_jobs() -> int:
    return min(MAX_AUTOMATIC_JOBS, max(1, os.cpu_count() or 1))


def partition_tests(tests: list[Path]) -> tuple[list[Path], list[Path]]:
    parallel = [path for path in tests if path.name not in RESOURCE_INTENSIVE_TEST_NAMES]
    resource_intensive = [path for path in tests if path.name in RESOURCE_INTENSIVE_TEST_NAMES]
    return parallel, resource_intensive


def run_test(path: Path) -> TestResult:
    started = time.monotonic()
    completed = subprocess.run(
        [sys.executable, str(path)],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        check=False,
    )
    return TestResult(
        path=path,
        returncode=completed.returncode,
        output=completed.stdout,
        elapsed=time.monotonic() - started,
    )


def relative(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


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
    args = parser.parse_args()
    if args.jobs < 0:
        parser.error("--jobs must be zero or greater")
    return args


def main() -> int:
    args = parse_args()
    tests = discover_tests(args.match)

    if args.list:
        for path in tests:
            print(relative(path))
        print(f"{len(tests)} test(s)")
        return 0

    if not tests:
        print("error: no regression tests matched", file=sys.stderr)
        return 2

    os.environ.setdefault("DURIS_REGRESSION_BUILD_CACHE", str(ROOT / "bin/regression-artifacts"))
    jobs = args.jobs or automatic_jobs()
    parallel_tests, resource_intensive_tests = partition_tests(tests)
    started = time.monotonic()
    failures: list[TestResult] = []
    completed_count = 0
    print(
        f"Running {len(tests)} Python regression tests with {jobs} worker(s); "
        f"serializing {len(resource_intensive_tests)} resource-intensive test(s)"
    )

    def report(result: TestResult) -> None:
        nonlocal completed_count
        completed_count += 1
        status = "PASS" if result.returncode == 0 else "FAIL"
        print(
            f"[{completed_count:>{len(str(len(tests)))}}/{len(tests)}] "
            f"{status} {relative(result.path)} ({result.elapsed:.2f}s)",
            flush=True,
        )
        builds = re.findall(r"SERVER_BUILD (built|reused) build=([0-9.]+)s lookup=([0-9.]+)s", result.output)
        if builds:
            build_time = sum(float(build) for _, build, _ in builds)
            lookup_time = sum(float(lookup) for _, _, lookup in builds)
            print(f"    server artifacts: {', '.join(status for status, _, _ in builds)}; "
                  f"build {build_time:.3f}s; validation {lookup_time:.3f}s; "
                  f"journey/other {max(0, result.elapsed - build_time - lookup_time):.3f}s", flush=True)
        if result.returncode != 0:
            failures.append(result)

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        pending: dict[Future[TestResult], Path] = {
            executor.submit(run_test, path): path for path in parallel_tests
        }
        for future in as_completed(pending):
            report(future.result())

    for path in resource_intensive_tests:
        report(run_test(path))

    for result in failures:
        print(f"\n--- {relative(result.path)} output ---")
        print(result.output.rstrip() or "(no output)")

    elapsed = time.monotonic() - started
    passed = len(tests) - len(failures)
    print(f"\n{passed} passed, {len(failures)} failed in {elapsed:.2f}s")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
