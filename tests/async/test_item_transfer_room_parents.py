#!/usr/bin/env python3
"""Link the genuine ROOM ancestor value leaf and owner identity without GC/stubs.

SQL-header and flat-file ASan/UBSan controls enumerate BEFORE identities only.
No SQL transaction, owner authority, property AFTER, native/durable or Source
qualification is inferred. Compiler, exact argv and source hashes are retained.
"""
import sys
sys.dont_write_bytecode = True
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROVIDERS = ("item/item_transfer_room_parents.c",)


def main():
    preliminary = argparse.ArgumentParser(add_help=False)
    preliminary.add_argument("--source-root", type=Path, default=ROOT)
    selected, _ = preliminary.parse_known_args()
    source = selected.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--output-dir", type=Path,
                        help="Task-specific evidence directory; use D: storage")
    parser.add_argument("--build-root", type=Path,
                        help="Task-specific binary scratch directory under D:/Dev/Builds")
    parser.add_argument("--compile-timeout", type=positive_seconds, default=300)
    parser.add_argument("--run-timeout", type=positive_seconds, default=60)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1",
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    task_root = None
    if args.output_dir is None or args.build_root is None:
        temp_root = Path(tempfile.gettempdir()).resolve()
        if any(temp_root.is_relative_to(root) for root in (source, args.proposal_root.resolve())):
            parser.error("Temporary evidence/build paths must be outside the immutable input roots")
        task_root = Path(tempfile.mkdtemp(prefix="duris-room-parent-identities-", dir=temp_root))
    output = (args.output_dir if args.output_dir is not None else task_root / "evidence").resolve()
    output.mkdir(parents=True, exist_ok=True)
    build = (args.build_root if args.build_root is not None else task_root / "build").resolve()
    build.mkdir(parents=True, exist_ok=True)
    environment["TMPDIR"] = str(build)
    print("ARTIFACT_DIR " + str(output), flush=True)
    version = execute([args.compiler, "-dumpfullversion"], cwd=source,
                      timeout=min(30, args.compile_timeout), env=environment, capture_output=True).strip()
    harness = args.proposal_root.resolve() / "tests/async/item_transfer_room_parents_test.cpp"
    assert harness.is_file(), harness
    def logged(arguments, destination, timeout, cwd):
        try:
            result = execute(arguments, cwd=cwd, timeout=timeout,
                             env=environment, capture_output=True)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            detail = error.output or ""
            if isinstance(detail, bytes):
                detail = detail.decode("utf-8", errors="replace")
            destination.write_text(detail, encoding="utf-8")
            if detail:
                sys.stderr.write(detail)
            raise
        destination.write_text(result, encoding="utf-8")
        return result

    reports = []
    with tempfile.TemporaryDirectory(prefix="room-parent-identities-", dir=build) as temporary:
        work = Path(temporary)
        for backend in ("sql", "flatfile"):
            flags = (["-D__NO_MYSQL__", "-I" + str(source / "src/no_mysql")]
                     if backend == "flatfile" else shlex.split(execute(
                         ["mysql_config", "--cflags"], cwd=source,
                         timeout=min(30, args.compile_timeout), env=environment, capture_output=True)))
            binary = work / backend
            command = [args.compiler, "-std=c++20", "-O1", "-g", "-Wall", "-Wextra",
                       "-Wpedantic", "-Werror", "-pthread", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                       "-fno-pie", "-no-pie", "-I" + str(source / "src"), *flags,
                       str(harness), *[str(source / "src" / name) for name in PROVIDERS],
                       "-o", str(binary)]
            (output / (backend + "-command.json")).write_text(
                json.dumps(command, indent=2) + "\n", encoding="utf-8")
            logged(command, output / (backend + "-compile.log"), args.compile_timeout, work)
            result = logged([str(binary)], output / (backend + "-run.log"),
                            args.run_timeout, work)
            rows = [json.loads(line.split(" ", 1)[1]) for line in result.splitlines()
                    if line.startswith("ITEM_TRANSFER_ROOM_PARENTS ")]
            assert len(rows) == 1, result
            report = rows[0]
            assert report["healthy_cases"] == 7 and report["negative_controls"] == 45
            assert report["allocation_cuts"] > 0 and report["strong_outputs"]
            assert report["before_identities_only"] is True
            reports.append(dict(backend=backend, command=command, report=report))
            print(backend + ": " + result, end="", flush=True)
    (output / "receipt.json").write_text(json.dumps(dict(
        compiler_version=version, controls=reports,
        source_sha256={str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in
                       [harness, *[source / "src" / name for name in PROVIDERS],
                        source / "src/item/item_transfer_room_parents.h",
                        source / "src/item/item_transfer_command.h",
                        source / "src/player/player_load_repository.h"]},
        scope="BEFORE identities only; no SQL/native/durable/property AFTER/Source qualification"),
        indent=2) + "\n", encoding="utf-8")
    print("PASS ROOM ancestor identities (SQL-header/flat ASan/UBSan; no GC or stubs)")


if __name__ == "__main__":
    main()
