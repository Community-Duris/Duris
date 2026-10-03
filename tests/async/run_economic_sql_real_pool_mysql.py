#!/usr/bin/env python3
"""Run actual typed coordinator/pool components on one disposable SQL schema."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def target_is_disposable(environment):
    return (environment.get("TEST_DB_DISPOSABLE") == "1" and
            environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1" and
            environment.get("DB_HOST") == "127.0.0.1" and not environment.get("DB_SOCKET") and
            re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", environment.get("DB_NAME", "")) and
            environment.get("DB_PORT", "").isdigit() and
            1 <= int(environment["DB_PORT"]) <= 65535)

def compile_family(family, binary):
    if family == "coin":
        script = (ROOT / "tests/async/run_currency_transaction_schema_mysql.sh").read_text()
        chunk = script.split("g++ -std=c++20", 1)[1].split(
            '"$ROOT/bin/tests/currency_transaction_mysql_harness"', 1)[0]
    else:
        chunk = (ROOT / "tests/async/run_item_transfer_schema_mysql.sh").read_text()
    sources = list(dict.fromkeys(re.findall(r"(?:tests|src)/[A-Za-z0-9_/.-]+\.(?:cpp|c)", chunk)))
    if family == "room":
        sources = [name.replace("tests/async/item_transfer_mysql_harness.cpp",
                    "tests/async/sql_room_item_payload_mysql_harness.cpp") for name in sources]
    if family == "room":
        sources.append("src/account/account_load.c")
    sources.append("src/sql/sql_pool.c")
    flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run([
        *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-pthread", "-g", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-ffunction-sections",
        "-fdata-sections", "-DDURIS_ECONOMIC_SQL_REAL_POOL_TEST", "-Isrc", *flags,
        *sources, "-Wl,--gc-sections", "-Wl,--wrap=mysql_real_query,--wrap=mysql_errno",
        "-Wl,--wrap=sql_pool_acquire,--wrap=sql_pool_release",
        "-Wl,--wrap=sql_pool_replace_connection,--wrap=sql_pool_discard_connection",
        *libs, "-lcrypto", "-lz", "-o", str(binary),
    ], cwd=ROOT, check=True, timeout=300)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", choices=("coin", "item", "room"), required=True)
    parser.add_argument("--server", type=Path,
                        help="also run the room recovery journey using this frozen SQL server")
    parser.add_argument("--server-sha256")
    args = parser.parse_args()
    if bool(args.server) != bool(args.server_sha256) or (args.server and args.family != "room"):
        parser.error("the room recovery journey requires both --server and --server-sha256")
    if not target_is_disposable(os.environ):
        raise SystemExit("explicit disposable loopback schema and port required")
    with tempfile.TemporaryDirectory(prefix="economic-real-pool-") as directory:
        binary = Path(directory) / args.family
        compile_family(args.family, binary)
        environment = dict(os.environ, CURRENCY_TEST_DB_NAME=os.environ["DB_NAME"],
                           ITEM_TRANSFER_TEST_DB_NAME=os.environ["DB_NAME"],
                           CURRENCY_TEST_COIN_ONLY="1",
                           ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        subprocess.run(["bash", "-c", 'ulimit -s 65536 && exec "$1"',
                        "economic-real-pool", str(binary)], cwd=ROOT, env=environment,
                       check=True, timeout=120)
        if args.server:
            subprocess.run([
                "python3", str(ROOT / "tests/async/run_sql_room_item_payload_recovery_journey.py"),
                "--seed-binary", str(binary), "--seed-sha256", hashlib.sha256(binary.read_bytes()).hexdigest(),
                "--server-binary", str(args.server), "--server-sha256", args.server_sha256,
            ], cwd=ROOT, env=environment, check=True, timeout=600)
    print(f"PASS: real SQL pool {args.family} qualification completed")

if __name__ == "__main__":
    main()
