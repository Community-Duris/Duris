#!/usr/bin/env python3
"""Native literal checkpoint qualification against an externally supplied SQL fixture.

Compile-only never connects. Runtime creates synthetic rows and private journals
in an already migrated, explicitly disposable loopback schema. This runner never
creates a database, migrates a schema, starts a service, or reads .env. The native
fixture exercises real capture/pipeline/journal/worker/repository/pool functions;
its wrappers observe or delay actual SQL calls and hide one real COMMIT reply.
This is native component integration, not full-world or active accounting proof.
"""
from __future__ import annotations

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

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "tests/async/player_literal_checkpoint_mysql_harness.cpp",
    "src/player/player_snapshot_capture.c", "src/player/player_snapshot_codec.c",
    "src/player/pet_restore_state.c", "src/player/pet_restore_runtime.c",
    "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c",
    "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
    "src/persistence/critical_command.c", "src/player/player_snapshot_repository.c",
    "src/player/player_save_journal.c", "src/sql/item_extra_descr_codec.c",
    "src/persistence/persistence_observability.c", "src/sql/sql_pool.c",
    "src/player/player_save_worker.c", "src/player/player_revision_state.c",
    "src/player/player_save_pipeline.c", "src/account/character_identity.c",
    "src/economy/item_transfer_accounting.c", "src/economy/economic_accounting_intent.c",
    "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_types.c",
    "src/persistence/sql_room_item_payload.c",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def target_is_disposable(environment: dict[str, str]) -> bool:
    port = environment.get("DB_PORT", "")
    schema = environment.get("DB_NAME", "")
    return bool(environment.get("TEST_DB_DISPOSABLE") == "1" and
                environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1" and
                environment.get("DB_HOST") == "127.0.0.1" and
                not environment.get("DB_SOCKET") and not environment.get("TEST_DB_SOCKET") and
                re.fullmatch(r"economic_schema_test_(?:li_[0-9a-f]{8}|[0-9a-f]{12})", schema) and
                len("duris.player.death.restitution." + schema) <= 64 and
                environment.get("DB_ALLOWED_TARGETS") == "127.0.0.1/" + schema and
                port.isascii() and port.isdigit() and 1 <= int(port) <= 65535 and
                environment.get("DB_USER") and environment.get("DB_PASSWD"))


def execute(arguments: list[str], *, environment: dict[str, str], timeout: int) -> None:
    process = subprocess.Popen(arguments, cwd=ROOT, env=environment, start_new_session=True)
    try:
        code = process.wait(timeout=timeout)
        if code:
            raise RuntimeError(f"native qualification command failed with exit {code}")
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


def base_environment() -> dict[str, str]:
    result = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": "C"}
    for key in ("LIBRARY_PATH", "LD_LIBRARY_PATH", "TASK_LINKER_DIR"):
        if key in os.environ:
            result[key] = os.environ[key]
    return result


def compile_native(binary: Path) -> None:
    if binary.exists():
        raise RuntimeError("compile destination already exists; never overwrite a qualified executable")
    binary.parent.mkdir(parents=True, exist_ok=True)
    inputs = {name: sha256(ROOT / name) for name in SOURCES}
    inputs.update({str(path.relative_to(ROOT)): sha256(path) for path in (ROOT / "src").rglob("*.h")})
    flags = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-pthread", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
             "-ffunction-sections", "-fdata-sections", "-Isrc"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True, timeout=10))
    flags += list(SOURCES)
    flags += ["-Wl,--gc-sections", "-Wl,--wrap=mysql_real_query,--wrap=mysql_errno,--wrap=_Znwm",
              "-Wl,--wrap=sql_pool_acquire,--wrap=sql_pool_release,--wrap=sql_pool_replace_connection"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True, timeout=10))
    flags += ["-lcrypto", "-o", str(binary)]
    try:
        execute(flags, environment=base_environment(), timeout=600)
        if any(sha256(ROOT / name) != value for name, value in inputs.items()):
            raise RuntimeError("native compilation inputs changed during qualification")
    except BaseException:
        binary.unlink(missing_ok=True)
        raise
    metadata = dict(scope="native SQL component compile only; no services executed",
                    binary_sha256=sha256(binary), inputs=inputs, flags=flags,
                    timeout_seconds=600, sanitizers="address,undefined")
    binary.with_suffix(binary.suffix + ".qualification.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print("PASS: strict ASan/UBSan native literal checkpoint compile; SHA256", metadata["binary_sha256"])


def run_native(binary: Path, expected_sha256: str) -> None:
    if not target_is_disposable(os.environ):
        raise RuntimeError("explicit guarded disposable loopback schema, port and credentials required")
    if not re.fullmatch(r"[0-9a-f]{64}", expected_sha256) or sha256(binary) != expected_sha256:
        raise RuntimeError("native executable SHA256 pin mismatch")
    environment = base_environment()
    for key in ("TEST_DB_DISPOSABLE", "ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "DB_HOST", "DB_PORT",
                "DB_NAME", "DB_ALLOWED_TARGETS", "DB_USER", "DB_PASSWD"):
        environment[key] = os.environ[key]
    environment.update(PERSISTENCE_MODE="mariadb-primary", REDIS="FALSE", ENVIRONMENT="local",
                       DB_TLS="FALSE", ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    work = ROOT / "bin/tests/player-literal-checkpoint"
    work.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="fixture-", dir=work) as temporary:
        os.chmod(temporary, 0o700)
        execute(["bash", "-c", 'ulimit -s 65536 && exec "$1" "$2"', "literal-checkpoint",
                 str(binary.resolve()), str(Path(temporary).resolve())], environment=environment, timeout=120)
    if sha256(binary) != expected_sha256:
        raise RuntimeError("qualified native executable changed during runtime")


def self_test() -> None:
    valid = dict(TEST_DB_DISPOSABLE="1", ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1",
                 DB_HOST="127.0.0.1", DB_PORT="3307", DB_NAME="economic_schema_test_li_0123abcd",
                 DB_ALLOWED_TARGETS="127.0.0.1/economic_schema_test_li_0123abcd",
                 DB_USER="fixture", DB_PASSWD="synthetic-fixture-only")
    assert target_is_disposable(valid)
    matrix_schema = "economic_schema_test_012345abcdef"
    assert target_is_disposable(dict(valid, DB_NAME=matrix_schema,
                                    DB_ALLOWED_TARGETS="127.0.0.1/" + matrix_schema))
    assert len("duris.player.death.restitution." + valid["DB_NAME"]) == 63
    negatives = (("TEST_DB_DISPOSABLE", "0"), ("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "0"),
                 ("DB_HOST", "192.0.2.1"), ("DB_PORT", "0"), ("DB_PORT", "65536"),
                 ("DB_NAME", "economic_schema_test_literal_0123abcd"),
                 ("DB_NAME", "economic_schema_test_li_0123abcd;DROP DATABASE other"),
                 ("DB_NAME", "economic_schema_test_li_0123abcg"), ("DB_ALLOWED_TARGETS", ""),
                 ("DB_NAME", "economic_schema_test_012345abcde"),
                 ("DB_NAME", "economic_schema_test_012345abcdef0"),
                 ("DB_NAME", "economic_schema_test_012345abcdeF"),
                 ("DB_SOCKET", "/tmp/socket"), ("TEST_DB_SOCKET", "/tmp/socket"), ("DB_PASSWD", ""))
    for key, value in negatives:
        assert not target_is_disposable(dict(valid, **{key: value})), key
    print(f"PASS: supplied SQL fixture guard; {len(negatives)} negative cases; no compile or service execution")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--compile-only", type=Path, metavar="BINARY")
    mode.add_argument("--run-binary", type=Path, metavar="BINARY")
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--qualify", action="store_true",
                      help="compile and run against the matrix's guarded disposable schema")
    parser.add_argument("--binary-sha256")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.qualify:
        if not target_is_disposable(os.environ):
            parser.error("qualification requires the guarded disposable schema")
        work = ROOT / "bin/tests/player-literal-checkpoint"
        work.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="qualification-", dir=work) as temporary:
            binary = Path(temporary) / "checkpoint"
            compile_native(binary)
            run_native(binary, sha256(binary))
    elif args.compile_only:
        compile_native(args.compile_only.resolve())
    else:
        if not args.binary_sha256:
            parser.error("--run-binary requires --binary-sha256")
        run_native(args.run_binary.resolve(strict=True), args.binary_sha256)


if __name__ == "__main__":
    main()
