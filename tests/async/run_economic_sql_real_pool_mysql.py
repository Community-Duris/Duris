#!/usr/bin/env python3
"""Run actual typed coordinator/pool components on one disposable SQL schema."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]

def target_is_disposable(environment):
    return (environment.get("TEST_DB_DISPOSABLE") == "1" and
            environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1" and
            environment.get("DB_HOST") == "127.0.0.1" and not environment.get("DB_SOCKET") and
            re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", environment.get("DB_NAME", "")) and
            environment.get("DB_PORT", "").isdigit() and
            1 <= int(environment["DB_PORT"]) <= 65535)

def execute(arguments, *, environment=None, timeout):
    process = subprocess.Popen(arguments, cwd=ROOT, env=environment, start_new_session=True)
    try:
        code = process.wait(timeout=timeout)
        if code:
            raise RuntimeError(f"native fixture exited {code}")
    except BaseException:
        if process.poll() is None:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=5)
        raise


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compile_family(family, binary, *, timeout=300, retained_verifier=False):
    if binary.exists():
        raise RuntimeError("compile destination already exists; immutable executable required")
    binary.parent.mkdir(parents=True, exist_ok=True)
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
    inputs = {name: sha256(ROOT / name) for name in sources}
    # The room harness includes the maintained item harness, which in turn
    # includes the actual coordinator/pool/COMMIT observers. Pin them explicitly.
    dependencies = ["tests/async/run_economic_sql_real_pool_mysql.py",
                    "tests/async/run_item_transfer_schema_mysql.sh",
                    "tests/async/run_currency_transaction_schema_mysql.sh",
                    "tests/async/item_transfer_mysql_harness.cpp"]
    dependencies += [str(path.relative_to(ROOT)) for path in (ROOT / "src").rglob("*.h")]
    dependencies += [str(path.relative_to(ROOT)) for path in (ROOT / "tests/async").glob("*.h")]
    inputs.update({name: sha256(ROOT / name) for name in dependencies})
    arguments = [
        *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-pthread", "-g", "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-ffunction-sections",
        "-fdata-sections", "-DDURIS_ECONOMIC_SQL_REAL_POOL_TEST", "-Isrc", *flags,
        *(["-DDURIS_SQL_ROOM_ITEM_RETAINED_VERIFIER_TEST"] if retained_verifier else []),
        *sources, "-Wl,--gc-sections", "-Wl,--wrap=mysql_real_query,--wrap=mysql_errno",
        "-Wl,--wrap=sql_pool_acquire,--wrap=sql_pool_release",
        "-Wl,--wrap=sql_pool_replace_connection,--wrap=sql_pool_discard_connection",
        *libs, "-lcrypto", "-lz", "-o", str(binary),
    ]
    start = time.monotonic()
    try:
        execute(arguments, timeout=timeout)
        if any(sha256(ROOT / name) != expected for name, expected in inputs.items()):
            raise RuntimeError("compilation inputs changed during qualification")
    except BaseException:
        binary.unlink(missing_ok=True)
        raise
    metadata = dict(scope="native actual SQL pool component compile only; no services",
                    family=family, binary_sha256=sha256(binary), inputs=inputs,
                    flags=arguments, timeout_seconds=timeout,
                    elapsed_seconds=time.monotonic() - start,
                    retained_public_verifier=retained_verifier)
    binary.with_suffix(binary.suffix + ".qualification.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print("PASS: native pool component compile", family, metadata["binary_sha256"],
          "seconds", metadata["elapsed_seconds"], flush=True)


def run_binary(family, binary, expected_sha256=None):
    if not target_is_disposable(os.environ):
        raise RuntimeError("explicit disposable loopback schema and port required")
    if expected_sha256 and (not re.fullmatch(r"[0-9a-f]{64}", expected_sha256) or
                           sha256(binary) != expected_sha256):
        raise RuntimeError("native executable SHA256 pin mismatch")
    environment = dict(os.environ, CURRENCY_TEST_DB_NAME=os.environ["DB_NAME"],
                       ITEM_TRANSFER_TEST_DB_NAME=os.environ["DB_NAME"],
                       CURRENCY_TEST_COIN_ONLY="1",
                       ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    execute(["bash", "-c", 'ulimit -s 65536 && exec "$1"',
             "economic-real-pool", str(binary)], environment=environment, timeout=120)
    if expected_sha256 and sha256(binary) != expected_sha256:
        raise RuntimeError("qualified executable changed during runtime")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", choices=("coin", "item", "room"), required=True)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--compile-only", type=Path, metavar="BINARY")
    mode.add_argument("--run-binary", type=Path, metavar="BINARY")
    parser.add_argument("--binary-sha256")
    parser.add_argument("--compile-budget-seconds", type=int, choices=(300, 600), default=300)
    parser.add_argument("--retained-public-verifier", action="store_true",
                        help="include new public helper RR contract probe; room family only")
    args = parser.parse_args()
    if args.retained_public_verifier and args.family != "room":
        parser.error("public retained verifier belongs to the room fixture")
    if args.compile_only:
        compile_family(args.family, args.compile_only.resolve(),
                       timeout=args.compile_budget_seconds,
                       retained_verifier=args.retained_public_verifier)
        return
    if args.run_binary:
        if not args.binary_sha256:
            parser.error("--run-binary requires --binary-sha256")
        run_binary(args.family, args.run_binary.resolve(strict=True), args.binary_sha256)
        return
    if not target_is_disposable(os.environ):
        raise SystemExit("explicit disposable loopback schema and port required")
    with tempfile.TemporaryDirectory(prefix="economic-real-pool-") as directory:
        binary = Path(directory) / args.family
        compile_family(args.family, binary, timeout=args.compile_budget_seconds,
                       retained_verifier=args.retained_public_verifier)
        run_binary(args.family, binary, sha256(binary))

if __name__ == "__main__":
    main()
