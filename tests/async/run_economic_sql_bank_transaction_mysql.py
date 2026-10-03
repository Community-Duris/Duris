#!/usr/bin/env python3
"""Compile/run the typed SQL bank journey in an explicitly disposable schema."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]


def target_is_disposable(environment):
    port = environment.get("DB_PORT", "")
    return (environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1" and
            environment.get("DB_HOST") == "127.0.0.1" and not environment.get("DB_SOCKET") and
            re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", environment.get("DB_NAME", "")) and
            port.isdigit() and 1 <= int(port) <= 65535)


def compile_bank(executable, real_pool=False):
    # Reuse the maintained repository link set and linked accounting adapters.
    script = (ROOT / "tests/async/run_currency_transaction_schema_mysql.sh").read_text()
    chunk = script.split("g++ -std=c++20", 1)[1].split(
        '"$ROOT/bin/tests/currency_transaction_mysql_harness"', 1)[0]
    files = re.findall(r"(?:tests|src)/[A-Za-z0-9_/.-]+\.(?:cpp|c)", chunk)[1:]
    files += ["tests/async/economic_sql_bank_transaction_mysql_harness.cpp",
              "src/persistence/economic_accounting_repository.c",
              "src/persistence/economic_sql_bank_transaction.c",
              "src/persistence/economic_sql_collector_transaction.c",
              "src/economy/collector_accounting.c",
              "src/economy/economic_gameplay_authority.c",
              "src/economy/economic_currency_adapter.c", "src/economy/economic_accounting_types.c",
              "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_intent.c",
              "src/sql/item_extra_descr_codec.c",
              "tests/async/item_extra_descr_codec_sql_escape_stub.cpp",
              "src/persistence/sql_room_item_payload.c"]
    if real_pool:
        files.append("src/sql/sql_pool.c")
    files = list(dict.fromkeys(files))
    flags = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-pthread", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
             "-Isrc", "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    flags += ["-Wl,--wrap=mysql_real_query,--wrap=mysql_errno"]
    if real_pool:
        flags += ["-DDURIS_ECONOMIC_SQL_REAL_POOL_TEST",
                  "-Wl,--wrap=sql_pool_acquire,--wrap=sql_pool_release",
                  "-Wl,--wrap=sql_pool_replace_connection,--wrap=sql_pool_discard_connection"]
    flags += files + shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(flags + ["-lcrypto", "-lz", "-o", str(executable)], cwd=ROOT,
                   check=True, timeout=600)


def run_bank(executable):
    subprocess.run(["bash", "-c", 'ulimit -s 65536 && exec "$1"', "economic-sql-bank", str(executable)],
                   cwd=ROOT, check=True, timeout=120,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--real-pool", action="store_true",
                        help="link the production SQL pool; keep the entire bank matrix")
    parser.add_argument("--compile-only", type=Path, metavar="BINARY",
                        help="compile a persistent native binary without connecting to a database")
    args = parser.parse_args()
    if args.compile_only:
        args.compile_only.parent.mkdir(parents=True, exist_ok=True)
        compile_bank(args.compile_only.resolve(), args.real_pool)
        return
    if (not target_is_disposable(os.environ) or
            (args.real_pool and os.environ.get("TEST_DB_DISPOSABLE") != "1")):
        raise SystemExit("explicit disposable loopback schema and port required")
    work = ROOT / "bin/tests/economic-sql-bank"
    work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="run-", dir=work) as temporary:
        executable = Path(temporary) / "bank"
        compile_bank(executable, args.real_pool)
        run_bank(executable)


if __name__ == "__main__":
    main()
