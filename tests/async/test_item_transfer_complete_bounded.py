#!/usr/bin/env python3
"""Run complete Item codec value/refusal regressions in both header backends.

Uses the real production Make object rules. Ordinary success requires the
supported Linux x86_64 GNU 13/libstdc++ 13/OpenSSL 3.0.13 profile. Genuine ASan
has its own explicit refusal lane; it is never treated as a supported success
profile. These fixtures do not start a server or contact SQL/Redis services.
The executable gets a 64 MiB fixture stack ceiling, preserving the hard limit;
this is not a change to accounting admission limits. Optional --library-path
arguments select caller-reviewed installed linker scripts/libraries only.
Each build/link and fixture run has a positive deadline; failed commands have
their new process group terminated before temporary artifacts are removed.
"""
import argparse
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "item/item_transfer_command", "item/lockpick_retirement_continuation",
    "item/craft_pouch_mutation", "combat/chaos_pouch_ledger",
    "player/player_snapshot_codec", "world/quest_mobile_native_reference",
    "persistence/critical_command", "economy/native_quest_cost",
    "economy/native_quest_coin_give", "economy/shop_trade_recovery_manifest",
    "economy/item_transfer_accounting", "economy/economic_accounting_intent",
    "economy/economic_accounting_plan", "economy/economic_source_event",
    "economy/economic_accounting_types",
)


def positive_seconds(value):
    seconds = int(value)
    if seconds <= 0:
        raise argparse.ArgumentTypeError("The deadline must be a positive number of seconds")
    return seconds


def terminate_on_signal(signum, _frame):
    # Let the outer runner's termination use the same descendant cleanup path.
    raise SystemExit(128 + signum)


def stop_process_group(process):
    # Repeated termination must not interrupt the owned group's kill/reap.
    original_mask = signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGTERM})
    try:
        # A leader can exit while a compiler or fixture descendant is still alive.
        # Signal the owned group even when process.poll() says the leader has exited.
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            pass
        finally:
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.communicate(timeout=5)
    finally:
        signal.pthread_sigmask(signal.SIG_SETMASK, original_mask)


def execute(arguments, *, cwd, timeout, env=None, preexec_fn=None, capture_output=False):
    # Defer termination until Popen returns and its child is inside our guard.
    original_mask = signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGTERM})
    process = None

    def child_setup():
        # The child inherited the temporary mask, not a requested fixture policy.
        signal.pthread_sigmask(signal.SIG_SETMASK, original_mask)
        if preexec_fn is not None:
            preexec_fn()

    try:
        process = subprocess.Popen(
            arguments, cwd=cwd, env=env, preexec_fn=child_setup, text=True,
            stdout=subprocess.PIPE if capture_output else None,
            stderr=subprocess.STDOUT if capture_output else None, start_new_session=True,
        )
        # Delivery of a deferred SIGTERM now raises inside the ownership guard.
        signal.pthread_sigmask(signal.SIG_SETMASK, original_mask)
        output, _ = process.communicate(timeout=timeout)
        if process.returncode:
            raise subprocess.CalledProcessError(process.returncode, arguments, output=output)
        return output
    except BaseException:
        if process is not None:
            stop_process_group(process)
        raise
    finally:
        signal.pthread_sigmask(signal.SIG_SETMASK, original_mask)


def main():
    signal.signal(signal.SIGTERM, terminate_on_signal)
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--library-path", type=Path, action="append", default=[])
    parser.add_argument("--sanitizer-refusal", action="store_true")
    parser.add_argument("--compile-timeout", type=positive_seconds, default=1200)
    parser.add_argument("--run-timeout", type=positive_seconds, default=180)
    args = parser.parse_args()
    fixture_stack_bytes = 64 * 1024 * 1024
    _, stack_hard_limit = resource.getrlimit(resource.RLIMIT_STACK)
    if stack_hard_limit != resource.RLIM_INFINITY and stack_hard_limit < fixture_stack_bytes:
        parser.error("The fixture requires a hard stack limit of at least 64 MiB")
    library_paths = [path.resolve() for path in args.library_path]
    if any(not path.is_dir() for path in library_paths):
        parser.error("Each --library-path must name an existing reviewed directory")
    linker_flags = [flag for path in library_paths for flag in ("-L", str(path))]

    def fixture_stack_limit():
        resource.setrlimit(resource.RLIMIT_STACK, (fixture_stack_bytes, stack_hard_limit))

    source_root = args.source_root.resolve()
    harness = args.proposal_root.resolve() / "tests/async/item_transfer_complete_bounded_test.cpp"
    bin_root = source_root / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    sanitizer_flags = ["-fsanitize=address,undefined"] if args.sanitizer_refusal else []
    run_environment = os.environ.copy()
    if args.sanitizer_refusal:
        run_environment.update(ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    marker = ("ITEM_TRANSFER_COMPLETE_SANITIZER_REFUSAL" if args.sanitizer_refusal
              else "ITEM_TRANSFER_COMPLETE_CASES")
    for backend, label in (("mariadb", "sql"), ("flatfile", "flat")):
        with tempfile.TemporaryDirectory(prefix=f"item-complete-{label}-", dir=bin_root) as directory:
            mode_root = Path(directory)
            objects = mode_root / "objects"
            targets = [objects / (name + ".o") for name in SOURCES]
            execute([
                "make", "-C", str(source_root / "src"), "-j1",
                "CC=" + args.compiler, "BUILD_PROFILE=production",
                "PERSISTENCE_BACKEND=" + backend, "OBJDIR=" + str(objects),
                "DMS_BINARY=" + str(mode_root / "unused-server"),
                "EXTRA_CFLAGS=" + " ".join(sanitizer_flags),
                *(str(target) for target in targets),
            ], cwd=source_root, timeout=args.compile_timeout)
            executable = mode_root / "item-transfer-complete"
            flags = [
                "-std=c++20", "-Og", "-g", "-Wall", "-Wextra", "-Wpedantic",
                "-Werror", "-D__NO_TESTS__", "-DDURIS_PERSISTENT_TRANSPORT",
                "-I", str(source_root / "src"), *sanitizer_flags,
            ]
            if backend == "flatfile":
                flags += ["-D__NO_MYSQL__", "-I", str(source_root / "src/no_mysql")]
            else:
                flags += ["-I/usr/include/mysql"]
            execute([
                args.compiler, *flags, str(harness), *(str(target) for target in targets),
                "-pthread", *linker_flags, "-lcrypto", "-o", str(executable),
            ], cwd=source_root, timeout=args.compile_timeout)
            output = execute([str(executable)], cwd=source_root, timeout=args.run_timeout,
                             env=run_environment, preexec_fn=fixture_stack_limit,
                             capture_output=True)
            reports = [json.loads(line[len(marker) + 1:]) for line in output.splitlines()
                       if line.startswith(marker + " ")]
            assert len(reports) == 1, output
            if args.sanitizer_refusal:
                assert all(reports[0][key] for key in
                           ("legacy_success", "source_refusal", "no_writes")), output
            else:
                assert reports[0]["successful_cases"] == 15, output
                assert reports[0]["refusal_points"] > 0, output
            print(output, end="")
            lane = "sanitizer refusal" if args.sanitizer_refusal else "ordinary"
            print(f"item-transfer complete bounded {label} {lane}: PASS")


if __name__ == "__main__":
    main()
