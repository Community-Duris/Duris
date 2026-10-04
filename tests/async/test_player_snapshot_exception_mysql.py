#!/usr/bin/env python3
"""Ordinary SQL exception ownership against an externally supplied private schema.

Actual repository/pool/journal/codec/conflict/lifecycle modules; controlled native
client failure observers. Compile-only and self-test never connect. Runtime writes
synthetic rows in a fully migrated disposable schema and private journal files.
No service startup, schema creation, migrations, Redis, game, or .env access.
New declared budgets: 300s compilation and 120s aggregate runtime, unmeasured
until executed; these do not relabel the older literal owner's 600s compile gate.
"""

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
HARNESS = Path(__file__).with_name("player_snapshot_exception_mysql_harness.cpp")
SOURCES = (
    "src/player/player_snapshot_repository.c",
    "src/player/player_snapshot_codec.c",
    "src/player/player_save_journal.c",
    "src/sql/item_extra_descr_codec.c",
    "src/persistence/persistence_observability.c",
    "src/player/player_death_conflict_repository.c",
    "src/persistence/economic_sql_lifecycle_guard.c",
    "src/sql/sql_pool.c",
)
CASES = (
    "direct_start_oom", "pooled_start_oom", "direct_dml_oom", "pooled_dml_oom",
    "commit_readback_oom", "failed_rollback", "replacement_null", "borrowed_tx",
    "borrowed_autocommit", "borrowed_reconnect", "conflict_cleanup", "description_ownership", "pet_result_ownership",
)
WRAPPERS = (
    "mysql_real_query", "mysql_errno", "mysql_store_result", "mysql_free_result",
    "mysql_close", "malloc", "free", "_Znwm", "sql_pool_acquire", "sql_pool_release",
    "sql_pool_discard_connection", "sql_pool_replace_connection",
)
COMPILE_SECONDS = 300
RUNTIME_SECONDS = 120
SCOPE = "actual SQL repository/pool/journal/codec/conflict; controlled client failures"
EXCLUDED = [
    "worker/pipeline", "critical producer/coordinator admission", "live game",
    "active accounting", "Redis", "production data", "incoming schema 0056",
    "general allocator sweep", "death evidence transaction allocation sweep",
]
DB_KEYS = (
    "TEST_DB_DISPOSABLE", "ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "DB_HOST", "DB_PORT",
    "DB_NAME", "DB_ALLOWED_TARGETS", "DB_USER", "DB_PASSWD",
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def inputs(root):
    paths = [root / name for name in SOURCES]
    paths.extend(sorted((root / "src").rglob("*.h")))
    result = {str(path.relative_to(root)): sha(path) for path in paths}
    result["owner-runner"] = sha(__file__)
    result["owner-harness"] = sha(HARNESS)
    return result


def base_environment():
    result = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"), "LC_ALL": "C"}
    for key in ("LIBRARY_PATH", "LD_LIBRARY_PATH", "TASK_LINKER_DIR"):
        if key in os.environ:
            result[key] = os.environ[key]
    return result


def target_is_disposable(environment):
    port = environment.get("DB_PORT", "")
    schema = environment.get("DB_NAME", "")
    return bool(
        environment.get("TEST_DB_DISPOSABLE") == "1"
        and environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1"
        and environment.get("DB_HOST") == "127.0.0.1"
        and not environment.get("DB_SOCKET") and not environment.get("TEST_DB_SOCKET")
        and re.fullmatch(r"economic_schema_test_se_[0-9a-f]{8}", schema)
        and len("duris.player.death.restitution." + schema) <= 64
        and environment.get("DB_ALLOWED_TARGETS") == "127.0.0.1/" + schema
        and port.isascii() and port.isdigit() and 1 <= int(port) <= 65535
        and environment.get("DB_USER") and environment.get("DB_PASSWD")
    )


def stop_group(process):
    # The process may have exited while compiler children remain in its group.
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.communicate(timeout=5)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.communicate(timeout=5)


def invoke(argv, root, environment, timeout, capture=False):
    process = subprocess.Popen(
        argv, cwd=root, env=environment, start_new_session=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None, text=True,
    )
    try:
        output, _ = process.communicate(timeout=timeout)
        return process.returncode, output or ""
    except BaseException:
        stop_group(process)
        raise


def build(root, binary):
    metadata_path = binary.with_suffix(binary.suffix + ".json")
    attempt_path = binary.with_suffix(binary.suffix + ".attempt.json")
    if binary.exists() or metadata_path.exists() or attempt_path.exists():
        raise ValueError("immutable artifact destination already exists")
    before = inputs(root)
    environment = base_environment()
    argv = [
        *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
        "-Wpedantic", "-Werror", "-pthread", "-O1", "-g0",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie",
        "-no-pie", "-ffunction-sections", "-fdata-sections", f"-I{root / 'src'}",
    ]
    argv.extend(shlex.split(subprocess.check_output(
        ["mysql_config", "--cflags"], env=environment, text=True, timeout=10)))
    argv.append(str(HARNESS))
    argv.extend(str(root / name) for name in SOURCES)
    argv.append("-Wl,--gc-sections")
    argv.extend("-Wl,--wrap=" + symbol for symbol in WRAPPERS)
    argv.extend(shlex.split(subprocess.check_output(
        ["mysql_config", "--libs"], env=environment, text=True, timeout=10)))
    argv.extend(["-lcrypto", "-o", str(binary)])
    binary.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    code = None
    failure = None
    try:
        code, _ = invoke(argv, root, environment, COMPILE_SECONDS)
    except BaseException as error:
        failure = error
    after = inputs(root)
    attempt = dict(
        source_root=str(root), backend="sql", inputs=before, compile_argv=argv,
        compile_seconds=time.monotonic() - started, returncode=code,
        compile_budget_seconds=COMPILE_SECONDS, runtime_budget_seconds=RUNTIME_SECONDS,
        timed_out=isinstance(failure, subprocess.TimeoutExpired),
        interrupted=failure is not None and not isinstance(failure, subprocess.TimeoutExpired),
        source_inputs_equal_before_after=before == after,
    )
    attempt_path.write_text(json.dumps(attempt, indent=2) + "\n")
    if before != after:
        raise RuntimeError("native source or fixture changed during compilation")
    if failure:
        raise failure
    if code:
        raise RuntimeError(f"strict native compilation failed with exit {code}")
    metadata = dict(attempt, binary_sha256=sha(binary), cases=CASES,
                    scope=SCOPE, excluded=EXCLUDED)
    metadata_path.write_text(json.dumps(metadata, indent=2) + "\n")
    print(json.dumps({"binary": str(binary), "sha256": metadata["binary_sha256"],
                      "compile_seconds": metadata["compile_seconds"]}), flush=True)
    return metadata


def verify(root, binary, expected_sha, backend="sql"):
    metadata = json.loads(binary.with_suffix(binary.suffix + ".json").read_text())
    if (backend != "sql" or metadata["backend"] != backend
            or metadata["source_root"] != str(root) or metadata["inputs"] != inputs(root)
            or not re.fullmatch(r"[0-9a-f]{64}", expected_sha)
            or metadata["binary_sha256"] != sha(binary) or sha(binary) != expected_sha
            or metadata["cases"] != list(CASES)
            or metadata["compile_budget_seconds"] != COMPILE_SECONDS
            or metadata["runtime_budget_seconds"] != RUNTIME_SECONDS
            or not metadata["source_inputs_equal_before_after"] or metadata["returncode"] != 0
            or metadata["timed_out"] or metadata["interrupted"]):
        raise RuntimeError("supplied immutable artifact/backend/source/fixture identity mismatch")
    return metadata


def execute(root, binary, metadata, evidence):
    if not target_is_disposable(os.environ):
        raise RuntimeError("explicit guarded disposable loopback SQL target required")
    before = inputs(root)
    original = sha(binary)
    environment = base_environment()
    environment.update({key: os.environ[key] for key in DB_KEYS})
    environment.update(PERSISTENCE_MODE="mariadb-primary", REDIS="FALSE", ENVIRONMENT="local",
                       DB_TLS="FALSE", ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    evidence.mkdir(parents=True, exist_ok=False)
    os.chmod(evidence, 0o700)
    deadline = time.monotonic() + RUNTIME_SECONDS
    observations = []
    for name in CASES:
        started = time.monotonic()
        try:
            code, output = invoke(
                [str(binary), name, str(evidence / (name + "-journal"))], root,
                environment, max(0.001, deadline - started), capture=True,
            )
        except subprocess.TimeoutExpired as error:
            output = error.output or ""
            if isinstance(output, bytes):
                output = output.decode(errors="replace")
            output += "\nAGGREGATE RUNTIME DEADLINE EXCEEDED\n"
            code = 124
        log = evidence / (name + ".log")
        log.write_text(output)
        observations.append(dict(case=name, attempted=True, returncode=code, seconds=time.monotonic() - started,
                                 log_sha256=sha(log)))
        print(output, end="", flush=True)
        if code == 124:
            break
    for name in CASES[len(observations):]:
        observations.append(dict(case=name, attempted=False, returncode=None, seconds=0,
                                 reason="aggregate runtime deadline; native case not executed"))
    unchanged = before == inputs(root) and original == sha(binary)
    declaration = dict(
        binary_sha256=original, metadata_sha256=sha(binary.with_suffix(binary.suffix + ".json")),
        backend=metadata["backend"], inputs=before, observations=observations,
        inputs_and_binary_unchanged=unchanged, runtime_budget_seconds=RUNTIME_SECONDS,
        scope=SCOPE, excluded=EXCLUDED,
    )
    (evidence / "results.json").write_text(json.dumps(declaration, indent=2) + "\n")
    if not unchanged:
        raise RuntimeError("immutable artifact or source changed during native runtime")
    return int(len(observations) != len(CASES)
               or any(item["returncode"] != 0 for item in observations))


def self_test():
    valid = dict(TEST_DB_DISPOSABLE="1", ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1",
                 DB_HOST="127.0.0.1", DB_PORT="3307", DB_NAME="economic_schema_test_se_0123abcd",
                 DB_ALLOWED_TARGETS="127.0.0.1/economic_schema_test_se_0123abcd",
                 DB_USER="fixture", DB_PASSWD="synthetic-fixture-only")
    assert target_is_disposable(valid)
    assert len("duris.player.death.restitution." + valid["DB_NAME"]) == 63
    negatives = (
        ("TEST_DB_DISPOSABLE", "0"), ("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA", "0"),
        ("DB_HOST", "192.0.2.1"), ("DB_PORT", "0"), ("DB_PORT", "65536"), ("DB_PORT", "١٢٣"),
        ("DB_NAME", "economic_schema_test_se_0123abcg"), ("DB_NAME", "other"),
        ("DB_NAME", "economic_schema_test_se_0123abcd;DROP DATABASE other"),
        ("DB_ALLOWED_TARGETS", ""), ("DB_SOCKET", "/tmp/socket"),
        ("TEST_DB_SOCKET", "/tmp/socket"), ("DB_USER", ""), ("DB_PASSWD", ""),
    )
    for key, value in negatives:
        assert not target_is_disposable(dict(valid, **{key: value})), key
    print(f"PASS: supplied SQL target guard; {len(negatives)} negatives; no native or service execution")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--compile-only", type=Path, metavar="BINARY")
    modes.add_argument("--run-binary", type=Path, metavar="BINARY")
    modes.add_argument("--self-test", action="store_true")
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--binary-sha256")
    parser.add_argument("--evidence", type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    if args.self_test:
        self_test()
        return 0
    if args.compile_only:
        build(root, args.compile_only.resolve())
        return 0
    if not args.binary_sha256 or not args.evidence:
        parser.error("supplied runtime requires binary SHA and new evidence directory")
    binary = args.run_binary.resolve(strict=True)
    metadata = verify(root, binary, args.binary_sha256)
    return execute(root, binary, metadata, args.evidence.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
