#!/usr/bin/env python3
"""Test actual pure keeper ROOM parent delta preparation, whole providers with GC.

Constructed identity-only plans grant no locked authority, property AFTER, SQL
storage/publication/native/durable/Source or F002 closure. Section GC discards
unrelated repository/gameplay sections; real economic validator/topology run.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
sys.dont_write_bytecode = True
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROVIDERS = ("item/item_transfer_repository.c", "item/item_transfer_room_parents.c",
             "item/item_transfer_command.c", "economy/economic_accounting_types.c",
             "persistence/sql_room_item_payload.c", "player/player_snapshot_codec.c")


def main():
    preliminary = argparse.ArgumentParser(add_help=False)
    preliminary.add_argument("--source-root", type=Path, default=ROOT)
    selected, _ = preliminary.parse_known_args()
    source = selected.source_root.resolve()
    if not source.is_dir():
        preliminary.error("Source root must be an existing directory")
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--provider-root", type=Path, help="Exact frozen provider overlay")
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--output-dir", type=Path,
                        help="Task-specific evidence directory; use D: storage")
    parser.add_argument("--build-root", type=Path,
                        help="Task-specific binary scratch directory under D:/Dev/Builds")
    parser.add_argument("--compile-timeout", type=positive_seconds, default=300)
    parser.add_argument("--run-timeout", type=positive_seconds, default=60)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    proposal = args.proposal_root.resolve()
    overlay = args.provider_root.resolve() if args.provider_root else source
    for root in (source, proposal, overlay):
        if not root.is_dir():
            parser.error("Source/proposal/provider roots must be existing directories")
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE="1",
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    task_root = None
    if args.output_dir is None or args.build_root is None:
        temp_root = Path(tempfile.gettempdir()).resolve()
        if any(temp_root.is_relative_to(root) for root in (source, proposal, overlay)):
            parser.error("Temporary evidence/build paths must be outside the immutable input roots")
        task_root = Path(tempfile.mkdtemp(prefix="duris-room-parent-delta-", dir=temp_root))
    output = (args.output_dir if args.output_dir is not None else task_root / "evidence").resolve()
    build = (args.build_root if args.build_root is not None else task_root / "build").resolve()
    if any(destination.is_relative_to(root)
           for destination in (output, build) for root in (source, proposal, overlay)):
        parser.error("Evidence and build directories must be outside the immutable input roots")
    output.mkdir(parents=True, exist_ok=True)
    build.mkdir(parents=True, exist_ok=True)
    environment["TMPDIR"] = str(build)
    print("ARTIFACT_DIR " + str(output), flush=True)
    version = execute([args.compiler, "-dumpfullversion"], cwd=source,
                      timeout=min(30, args.compile_timeout), env=environment, capture_output=True).strip()
    harness = proposal / "tests/async/item_transfer_room_parent_delta_test.cpp"
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

    def provider(name):
        selected = overlay / "src" / name
        return selected if selected.is_file() else source / "src" / name
    inputs = [harness, *map(provider, PROVIDERS),
              *[provider(name) for name in (
                  "item/item_transfer_repository.h", "item/item_transfer_command.h",
                  "item/item_transfer_room_parents.h", "persistence/sql_room_item_payload.h",
                  "economy/economic_accounting_types.h", "player/player_load_repository.h")]]
    pins = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}
    (output / "inputs-before-compile.json").write_text(json.dumps(pins, indent=2) + "\n", encoding="utf-8")
    reports = []
    with tempfile.TemporaryDirectory(prefix="room-parent-delta-", dir=build) as temporary:
        work = Path(temporary)
        for backend in ("sql", "flatfile"):
            flags = (["-D__NO_MYSQL__", "-I" + str(source / "src/no_mysql")]
                     if backend == "flatfile" else shlex.split(execute(
                         ["mysql_config", "--cflags"], cwd=source,
                         timeout=min(30, args.compile_timeout), env=environment, capture_output=True)))
            binary = work / backend
            command = [args.compiler, "-std=c++20", "-O0", "-g1", "-Wall", "-Wextra",
                       "-Wpedantic", "-Werror", "-pthread", "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                       "-fno-pie", "-no-pie", "-I" + str(overlay / "src"), "-I" + str(source / "src"), *flags,
                       str(harness), *[str(provider(name)) for name in PROVIDERS],
                       "-Wl,--gc-sections", "-lcrypto", "-o", str(binary)]
            (output / (backend + "-command.json")).write_text(
                json.dumps(command, indent=2) + "\n", encoding="utf-8")
            logged(command, output / (backend + "-compile.log"), args.compile_timeout, work)
            result = logged([str(binary)], output / (backend + "-run.log"),
                            args.run_timeout, work)
            rows = [json.loads(line.split(" ", 1)[1]) for line in result.splitlines()
                    if line.startswith("ROOM_PARENT_DELTA ")]
            assert len(rows) == 1, result
            report = rows[0]
            assert report["healthy_cases"] == 4 and report["refusal_controls"] == 44
            assert report["allocation_cuts"] > 0 and report["strong_outputs"]
            assert report["pure_keeper_only"] is True
            composition_rows = [json.loads(line.split(" ", 1)[1]) for line in result.splitlines()
                                if line.startswith("ROOM_PARENT_REAL_COMPOSITION ")]
            assert len(composition_rows) == 1, result
            composition = composition_rows[0]
            assert composition["routes"] == 2 and composition["projected_plan_refusals"] == 8
            assert composition["projection_allocation_cuts"] > 0
            assert composition["keeper_allocation_cuts"] > 0
            assert composition["real_codec_projection_validator"] is True
            assert composition["pure_helpers_only"] is True
            reports.append(dict(backend=backend, command=command, report=report,
                                real_composition=composition))
            print(backend + ": " + result, end="", flush=True)
    assert all(hashlib.sha256(path.read_bytes()).hexdigest() == pins[str(path)] for path in inputs)
    (output / "receipt.json").write_text(json.dumps(dict(
        compiler_version=version, controls=reports,
        source_sha256=pins,
        scope="Pure keeper value delta; no SQL storage/native/property AFTER/Source/F002 qualification"),
        indent=2) + "\n", encoding="utf-8")
    print("PASS ROOM parent keeper delta (SQL-header/flat ASan/UBSan; component GC)")


if __name__ == "__main__":
    main()
