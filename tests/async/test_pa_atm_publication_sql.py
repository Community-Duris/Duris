#!/usr/bin/env python3
"""Run the ATM producer/publication path against the real disposable SQL owner."""

import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from _paths import ROOT
from pa_accounting_batch_artifact import load_base_build, report_base_build

RUNNER = ROOT / "tests/async/run_pa_atm_publication_sql.sh"
HARNESS = ROOT / "tests/async/pa_atm_publication_harness.cpp"


def verify_base_build() -> str:
    build = load_base_build(ROOT)
    report_base_build(build, scope="S06 ATM component SQL harness; frozen binary not executed")
    return build.binary_sha256


def run_component_harness() -> None:
    if (
        os.environ.get("ATM_PUBLICATION_DISPOSABLE_SCHEMA") != "1"
        or os.environ.get("DB_HOST") != "127.0.0.1"
        or os.environ.get("DB_SOCKET")
        or not re.fullmatch(r"atm_pub_test_[A-Za-z0-9_]+", os.environ.get("DB_NAME", ""))
    ):
        raise SystemExit("explicit disposable loopback ATM schema required")
    verify_base_build()

    # Reuse the maintained link set for the production SQL command dispatcher;
    # add only the real producer, coordinator/journal and existing test seam.
    schema_runner = (ROOT / "tests/async/run_currency_transaction_schema_mysql.sh").read_text(
        encoding="utf-8"
    )
    command_block = schema_runner.split("g++ -std=c++20", 1)[1].split(
        '"$ROOT/bin/tests/currency_transaction_mysql_harness"', 1
    )[0]
    sources = [
        path
        for path in re.findall(r"(?:tests|src)/[A-Za-z0-9_./-]+\.(?:cpp|c)", command_block)
        if path.startswith("src/")
    ]
    sources.extend(
        [
            "src/economy/currency_transaction.c",
            "src/economy/economic_gameplay_authority.c",
            "src/economy/economic_command_admission.c",
            "src/persistence/critical_command_coordinator.c",
            "src/persistence/critical_command_journal.c",
            "src/core/utility.c",
        ]
    )
    sources = list(dict.fromkeys(sources))

    work = ROOT / "bin/tests/atm-publication-sql"
    work.mkdir(parents=True, exist_ok=True)
    cflags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="run-", dir=work) as temporary:
        executable = Path(temporary) / "atm-publication"
        command = compiler + [
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-pthread",
            "-O1",
            "-g",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-fno-pie",
            "-no-pie",
            "-ffunction-sections",
            "-fdata-sections",
            "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
            "-Isrc",
            *cflags,
            str(HARNESS.relative_to(ROOT)),
            *sources,
            "-Wl,--gc-sections",
            *libs,
            "-lcrypto",
            "-lz",
            "-o",
            str(executable),
        ]
        subprocess.run(command, cwd=ROOT, check=True)
        environment = dict(
            os.environ,
            ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
            ATM_TEST_JOURNAL_PARENT=temporary,
        )
        subprocess.run([str(executable)], cwd=ROOT, env=environment, check=True, timeout=90)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--execute",
        action="store_true",
        help="compile and run inside the wrapper's explicitly disposable SQL schema",
    )
    args = parser.parse_args()
    if args.execute:
        run_component_harness()
        return
    subprocess.run(["bash", str(RUNNER)], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
