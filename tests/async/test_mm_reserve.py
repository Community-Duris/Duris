#!/usr/bin/env python3
"""Run actual pool controls with both original production provider closures.

Linux build/native unit only. No SQL/Redis/game startup, migrations or runtime
data access. mmap is fault-injected only in the unit link. Artifacts stay in bin.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shlex
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
HARNESS = Path(__file__).with_name("mm_reserve_harness.cpp")
EXPECTED_OUTPUT = (
    "ACTUAL_MM_RESERVE_PASS existing_slot empty_chunk acquire_release refill "
    "geometry stats_overflow mmap_refusal"
)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def invoke(command, destination, *, timeout=None):
    started = time.monotonic()
    timed_out = False
    exit_code = None
    with destination.with_suffix(".stdout.log").open("wb") as stdout, \
            destination.with_suffix(".stderr.log").open("wb") as stderr:
        try:
            result = subprocess.run(command, cwd=ROOT / "src", stdout=stdout,
                                    stderr=stderr, timeout=timeout, check=False)
            exit_code = result.returncode
        except subprocess.TimeoutExpired:
            timed_out = True
    row = {
        "command": command, "exit_code": exit_code,
        "seconds": round(time.monotonic() - started, 3),
        "timeout_seconds": timeout, "timed_out": timed_out,
        "stdout_sha256": digest(destination.with_suffix(".stdout.log")),
        "stderr_sha256": digest(destination.with_suffix(".stderr.log")),
    }
    destination.with_suffix(".json").write_text(json.dumps(row, indent=2) + "\n")
    if timed_out:
        raise RuntimeError("Original provider/unit step timed out; evidence: " + str(destination))
    if exit_code:
        raise RuntimeError("Original provider/unit step refused; evidence: " + str(destination))
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path,
                        help="Owned original production build root; Make validates dependencies.")
    args = parser.parse_args()
    artifact_root = ROOT / "bin/tests/mm-reserve"
    artifact_root.mkdir(parents=True, exist_ok=True)
    attempt = Path(tempfile.mkdtemp(prefix="attempt-", dir=artifact_root))
    build_root = args.build_root.resolve() if args.build_root else attempt / "build"
    if not build_root.is_relative_to((ROOT / "bin").resolve()):
        raise ValueError("Build artifacts must remain in this checkout's bin directory")
    initial = {name: digest(ROOT / name) for name in (
        "src/Makefile", "src/core/mm.c", "src/core/mm.h",
        "src/player/player_load_items.c", "src/player/inert_item_stage.c",
        "src/net/comm.c", "tests/async/mm_reserve_harness.cpp",
    )}
    reports = []
    for backend in ("mariadb", "flatfile"):
        output = attempt / backend
        output.mkdir()
        server = build_root / "server" / backend / "dms_new"
        variables = ["BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                     "BIN_ROOT=" + str(build_root), "DMS_BINARY=" + str(server),
                     "PFILE_BINARY=" + str(build_root / "tools" / backend / "pfile")]
        # The original Makefile supplies every compile flag and provider/link
        # argument. The dry recipes are captured before building, not recreated.
        invoke(["make", "--no-print-directory", "-n", "-B", *variables, str(server)],
               output / "original-make-dry", timeout=55)
        recipes = []
        for line in (output / "original-make-dry.stdout.log").read_text().splitlines():
            if not line.startswith("g++ "):
                continue
            tokens = shlex.split(line)
            if tokens and tokens[0] == "g++" and "-o" in tokens:
                recipes.append(tokens)
        compiles = [argv for argv in recipes if "-c" in argv]
        comm = [argv for argv in compiles if argv[argv.index("-c") + 1] == "net/comm.c"]
        links = [argv for argv in recipes if "-c" not in argv and
                 argv[argv.index("-o") + 1] == str(server)]
        if len(comm) != 1 or len(links) != 1:
            raise RuntimeError("Original full-server compile/link recipe is ambiguous")
        providers = [value for value in links[0] if value.endswith(".o")]
        if not providers or len(providers) != len(set(providers)) or len(providers) != len(compiles):
            raise RuntimeError("Original full-provider closure differs")
        # An existing owned build root may be supplied after the normal full
        # build gate; original Make .d/stamp/flags rules still run here. With no
        # argument each profile gets a fresh complete provider directory.
        invoke(["make", "--no-print-directory", "-j2", *variables, str(server)],
               output / "original-production-make")
        unit_object = output / "mm-reserve.o"
        unit_binary = output / "mm-reserve"
        compile_command = list(comm[0])
        original_comm_object = compile_command[compile_command.index("-o") + 1]
        compile_command[compile_command.index("-c") + 1] = str(HARNESS)
        compile_command[compile_command.index("-o") + 1] = str(unit_object)
        invoke(compile_command, output / "unit-compile", timeout=55)
        provider_pins = []
        for name in providers:
            path = Path(name)
            dependency = path.with_suffix(".d")
            provider_pins.append({"object": name, "sha256": digest(path),
                                  "dependency_sha256": digest(dependency)})
        link = list(links[0])
        if link.count(original_comm_object) != 1:
            raise RuntimeError("Original comm provider is not present exactly once")
        link[link.index(original_comm_object)] = str(unit_object)
        link[link.index("-o") + 1] = str(unit_binary)
        link.append("-Wl,--wrap=mmap")
        # Full production-provider links have a separate bounded build budget.
        invoke(link, output / "unit-original-provider-link", timeout=900)
        result = invoke([str(unit_binary)], output / "actual-mm-unit", timeout=15)
        if (output / "actual-mm-unit.stdout.log").read_text().strip() != EXPECTED_OUTPUT:
            raise RuntimeError("Actual-mm control completion marker differs")
        for name, pin in initial.items():
            if digest(ROOT / name) != pin:
                raise RuntimeError("Source changed during unit qualification: " + name)
        reports.append({"backend": backend, "check_class": "native pool unit; mmap-only fault injection",
                        "provider_count": len(providers), "providers": provider_pins,
                        "original_unit_compile_argv": compile_command, "original_link_argv": links[0],
                        "unit_link_argv": link, "binary_sha256": digest(unit_binary),
                        "actual_result": result, "source_pins": initial,
                        "SQL_Redis_game_or_migration_services_started": False,
                        "full_native_world_or_current_HEAD_runtime_qualification": False})
        (attempt / "RECEIPT.json").write_text(json.dumps(reports, indent=2) + "\n")
    print("BOTH_PROFILE_ACTUAL_MM_RESERVE_PASS")


if __name__ == "__main__":
    main()
