#!/usr/bin/env python3
"""Discover and run Duris' plain-Python regression tests."""

from __future__ import annotations

import argparse
import json
import os
import re
import signal
import subprocess
import sys
import threading
import time
from concurrent.futures import FIRST_COMPLETED, Future, ThreadPoolExecutor, wait
from dataclasses import asdict, dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TEST_DIRECTORY = ROOT / "tests" / "async"
MAX_AUTOMATIC_JOBS = 8
DEFAULT_TIMEOUT = 900.0
RESOURCE_TIMEOUT = 1800.0
HEARTBEAT_SECONDS = 30.0
TERMINATE_GRACE_SECONDS = 2.0
RESOURCE_INTENSIVE_TEST_NAMES = frozenset(
    {
        "test_persistent_transport_journey.py",
        "test_player_quarantine_restore.py",
        "test_account_recovery_journey.py",
        "test_creation_prompt_journey.py",
        "test_game_loop_session_journey.py",
        "test_network_readiness_journey.py",
        "test_area_coin_pickup.py",
        "test_flatfile_auction_coin_put_journey.py",
        "test_flatfile_boot_preflight.py",
        "test_flatfile_chaos_new_character_kit.py",
        "test_flatfile_combat_journey.py",
        "test_flatfile_newbie_regrant_journey.py",
        "test_flatfile_first_session_currency.py",
        "test_flatfile_full_world_boot.py",
        "test_item_movement_prompt_runtime.py",
        "test_information_cache_journey.py",
        "test_mysql_combat_journey.py",
        "test_static_quest_reward_journey.py",
    }
)

# These real-runtime probes require explicitly supplied artifacts or helpers
# (test_pet_restart_journey.py a flat-file server; test_mob_gold_dial_runtime.py a
# server and a level promotion helper). The MySQL playtime journey needs a
# disposable database and --server, and invokes its repository probe with that
# database's environment. They are run explicitly, not by the generic test-all
# runner, which invokes every discovered script with no arguments.
MANUAL_ONLY_TEST_NAMES = frozenset(
    {
        # Requires a private copied staging journal and its custody manifests.
        "test_player_save_journal_quarantine.py",
        # Owns disposable Docker databases and measures the staging schema fork.
        "test_staging_migration_fork_mysql.py",
        # Requires an explicitly disposable loopback database on each SQL engine.
        "test_quest_recovery_read_budget_mysql.py",
        # These require a migrated, explicitly disposable loopback schema.
        "test_economic_accounting_schema_mysql.py",
        "test_economic_baseline_schema_mysql.py",
        "test_economic_accounting_item_reference_mysql.py",
        "test_player_save_item_reconcile_mysql.py",
        "test_player_spell_effect_receipt_mysql.py",
        # Owns a disposable SQL fixture and runs sanitizer cutover harnesses.
        "test_economic_sql_lifecycle_owner_contract.py",
        "test_mob_gold_dial_runtime.py",
        "test_mysql_playtime_journey.py",
        "test_pet_restart_journey.py",
        "test_playtime_mysql_repository.py",
        "test_issue331_player_journey.py",
        "test_issue331_staff_recovery_journey.py",
        "test_death_resurrection_mysql_journey.py",
        # Frozen-artifact SQL journeys are leased by the central batch runner.
        "test_pa_runtime_sql.py",
        "test_pa_copyover_sql.py",
        "test_pa_copyover_account_authority.py",
        "test_pa_necromancy_sql.py",
        "test_pa_item_creation_sql.py",
        "test_pa_item_flags_sql.py",
        "test_pa_atm_publication_sql.py",
        "test_pa_coin_sql.py",
        "test_pa_web_recovery_sql.py",
    }
)


@dataclass(frozen=True)
class TestResult:
    path: Path
    returncode: int
    output: str
    elapsed: float
    status: str = "passed"
    skipped_checks: int = 0


def discover_tests(match: str | None) -> list[Path]:
    tests = sorted(
        path for path in set(TEST_DIRECTORY.glob("test_*.py")) | set(TEST_DIRECTORY.glob("*_test.py"))
        if path.name not in MANUAL_ONLY_TEST_NAMES
    )
    if match:
        tests = [path for path in tests if match in path.name]
    return tests


def automatic_jobs() -> int:
    return min(MAX_AUTOMATIC_JOBS, max(1, os.cpu_count() or 1))


def partition_tests(tests: list[Path]) -> tuple[list[Path], list[Path]]:
    parallel = [path for path in tests if path.name not in RESOURCE_INTENSIVE_TEST_NAMES]
    resource_intensive = [path for path in tests if path.name in RESOURCE_INTENSIVE_TEST_NAMES]
    return parallel, resource_intensive


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
             stop: threading.Event | None = None) -> TestResult:
    started = time.monotonic()
    stop = stop if stop is not None else threading.Event()
    if stop.is_set():
        return TestResult(path, 130, "cancelled before starting", 0.0, "cancelled")
    try:
        process = subprocess.Popen(
            [sys.executable, str(path)], cwd=ROOT, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, errors="replace",
            start_new_session=os.name == "posix",
        )
    except OSError as error:
        return TestResult(path, 127, str(error), time.monotonic() - started, "error")
    while True:
        remaining = timeout - (time.monotonic() - started)
        if stop.is_set() or remaining <= 0:
            cancelled = stop.is_set()
            output = terminate_test(process)
            reason = "cancelled" if cancelled else f"timed out after {timeout:g}s"
            return TestResult(path, 130 if cancelled else 124, output + f"\n{reason}\n",
                              time.monotonic() - started, "cancelled" if cancelled else "timeout")
        try:
            output = process.communicate(timeout=min(0.25, remaining))[0]
            break
        except subprocess.TimeoutExpired:
            continue
    entirely_skipped, skipped = skip_count(output)
    status = ("signal" if process.returncode < 0 else "failed") if process.returncode else (
        "skipped" if entirely_skipped else "passed")
    return TestResult(path, process.returncode, output, time.monotonic() - started, status, skipped)


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
    parser.add_argument("--timeout", type=float, default=None, metavar="SECONDS",
                        help="per-test deadline (default: 900s; resource-intensive: 1800s)")
    parser.add_argument("--report", type=Path, default=ROOT / "bin/test-results.json",
                        help="write timing/outcome JSON (default: bin/test-results.json)")
    args = parser.parse_args()
    if args.jobs < 0:
        parser.error("--jobs must be zero or greater")
    if args.timeout is not None and (not 0 < args.timeout < float("inf")):
        parser.error("--timeout must be finite and greater than zero")
    return args


def write_report(path: Path, results: list[TestResult], elapsed: float, interrupted: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {"elapsed": elapsed, "interrupted": interrupted, "results": [
        {**{key: value for key, value in asdict(result).items() if key not in {"path", "output"}},
         "path": relative(result.path)} for result in sorted(results, key=lambda result: result.path)
    ]}
    temporary = path.with_name(path.name + f".{os.getpid()}.tmp")
    try:
        temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


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
    results: list[TestResult] = []
    stop = threading.Event()
    interrupted = False
    completed_count = 0
    print(
        f"Running {len(tests)} Python regression tests with {jobs} worker(s); "
        f"serializing {len(resource_intensive_tests)} resource-intensive test(s)"
    )

    def report(result: TestResult) -> None:
        nonlocal completed_count
        completed_count += 1
        results.append(result)
        status = {"passed": "PASS", "skipped": "SKIP"}.get(result.status, result.status.upper())
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
            print(f"\n--- {relative(result.path)} output ---", flush=True)
            print(result.output.rstrip() or "(no output)", flush=True)
        elif result.skipped_checks:
            print(f"    {result.skipped_checks} check(s) explicitly skipped", flush=True)

    def run_group(paths: list[Path], workers: int, default_timeout: float) -> None:
        with ThreadPoolExecutor(max_workers=workers) as executor:
            pending: dict[Future[TestResult], Path] = {
                executor.submit(run_test, path, args.timeout or default_timeout, stop): path for path in paths
            }
            while pending:
                done, _ = wait(pending, timeout=HEARTBEAT_SECONDS, return_when=FIRST_COMPLETED)
                if not done:
                    active = [relative(path) for future, path in pending.items() if future.running()]
                    print(f"Still running ({time.monotonic() - started:.1f}s elapsed): "
                          + ", ".join(active), flush=True)
                for future in done:
                    pending.pop(future)
                    report(future.result())

    def cancel(signum, frame) -> None:
        nonlocal interrupted
        if not interrupted:
            print("\nCancelling running tests and their children...", flush=True)
        interrupted = True
        stop.set()

    previous_handler = signal.signal(signal.SIGINT, cancel)
    try:
        run_group(parallel_tests, jobs, DEFAULT_TIMEOUT)
        if not interrupted:
            run_group(resource_intensive_tests, 1, RESOURCE_TIMEOUT)
        else:
            for path in resource_intensive_tests:
                report(run_test(path, args.timeout or RESOURCE_TIMEOUT, stop))
    finally:
        signal.signal(signal.SIGINT, previous_handler)

    elapsed = time.monotonic() - started
    write_report(args.report, results, elapsed, interrupted)
    passed = sum(result.status == "passed" for result in results)
    skipped = sum(result.status == "skipped" for result in results)
    failed = sum(result.returncode != 0 for result in results)
    print(f"\n{passed} passed, {skipped} skipped, {failed} failed in {elapsed:.2f}s")
    print("Slowest tests:")
    for result in sorted(results, key=lambda result: result.elapsed, reverse=True)[:10]:
        print(f"  {result.elapsed:8.2f}s {relative(result.path)} ({result.status})")
    print(f"Results: {args.report}")
    return 130 if interrupted else 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
