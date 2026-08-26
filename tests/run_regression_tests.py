#!/usr/bin/env python3
"""Discover and run Duris' plain-Python regression tests."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from concurrent.futures import Future, ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TEST_DIRECTORY = ROOT / "tests" / "async"
MAX_AUTOMATIC_JOBS = 8


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

    jobs = args.jobs or automatic_jobs()
    started = time.monotonic()
    failures: list[TestResult] = []
    print(f"Running {len(tests)} Python regression tests with {jobs} worker(s)")

    with ThreadPoolExecutor(max_workers=jobs) as executor:
        pending: dict[Future[TestResult], Path] = {
            executor.submit(run_test, path): path for path in tests
        }
        for completed_count, future in enumerate(as_completed(pending), start=1):
            result = future.result()
            status = "PASS" if result.returncode == 0 else "FAIL"
            print(
                f"[{completed_count:>{len(str(len(tests)))}}/{len(tests)}] "
                f"{status} {relative(result.path)} ({result.elapsed:.2f}s)",
                flush=True,
            )
            if result.returncode != 0:
                failures.append(result)

    for result in failures:
        print(f"\n--- {relative(result.path)} output ---")
        print(result.output.rstrip() or "(no output)")

    elapsed = time.monotonic() - started
    passed = len(tests) - len(failures)
    print(f"\n{passed} passed, {len(failures)} failed in {elapsed:.2f}s")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
