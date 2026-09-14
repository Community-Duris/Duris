#!/usr/bin/env python3
"""Bounded external telemetry rollup CLI.

Examples and the credential boundary are documented in docs/telemetry/ROLLUPS.md.
The CLI has no migration, reset, delete, password, or raw-mutation command.
"""
from __future__ import annotations

import argparse
from datetime import date
import json
import math
import multiprocessing
import os
import sys
import time
from typing import Any, Callable

try:  # Running as a package.
    from .db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase
    from .rollup_definitions import RollupTarget, report_catalog
    from .rollup_engine import (
        BoundsExceeded,
        REPORT_BYTE_LIMIT_DEFAULT,
        REPORT_ROW_LIMIT_DEFAULT,
        REPORT_RUNTIME_DEFAULT_S,
        RollupBounds,
        RollupEngine,
    )
except ImportError:  # Running scripts/telemetry/rollup.py directly.
    from db_access import ConnectionSettings, PyMySQLConnectionFactory, PyMySQLRollupDatabase  # type: ignore[no-redef]
    from rollup_definitions import RollupTarget, report_catalog  # type: ignore[no-redef]
    from rollup_engine import (  # type: ignore[no-redef]
        BoundsExceeded,
        REPORT_BYTE_LIMIT_DEFAULT,
        REPORT_ROW_LIMIT_DEFAULT,
        REPORT_RUNTIME_DEFAULT_S,
        RollupBounds,
        RollupEngine,
    )


PREFIX = "TELEMETRY_ROLLUP_DB_"


def _run_killable_process(
    target: Callable[..., Any],
    args: tuple[Any, ...],
    *,
    timeout_s: float,
) -> int:
    """Run one command in a process that the parent can kill at its deadline."""

    if isinstance(timeout_s, bool) or not isinstance(timeout_s, (int, float)) or not math.isfinite(timeout_s) or timeout_s <= 0:
        raise ValueError("worker timeout_s must be a positive finite number")
    context = multiprocessing.get_context("spawn")
    process = context.Process(target=target, args=args)
    deadline = time.monotonic() + float(timeout_s)
    process.start()
    try:
        while process.is_alive():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            process.join(remaining)
        if process.is_alive():
            killer = getattr(process, "kill", process.terminate)
            killer()
            process.join()
            raise BoundsExceeded("rollup worker was killed at its max_runtime_s deadline")
        if process.exitcode is None:
            raise RuntimeError("rollup worker exited without a status")
        return int(process.exitcode)
    finally:
        if process.is_alive():
            killer = getattr(process, "kill", process.terminate)
            killer()
            process.join()
        process.close()


def _worker_entry(command: str, payload: dict[str, Any]) -> None:
    args = argparse.Namespace(**payload)
    handler = {"run": _run, "publish": _publish, "report": _report}[command]
    try:
        status = int(handler(args))
    except (ValueError, RuntimeError) as error:
        print(f"telemetry rollup failed: {error}", file=sys.stderr, flush=True)
        raise SystemExit(2)
    except BaseException as error:
        print(f"telemetry rollup failed: {type(error).__name__}", file=sys.stderr, flush=True)
        raise SystemExit(1)
    raise SystemExit(status)


def _command_runtime(args: argparse.Namespace) -> float:
    if args.command == "run":
        bounds = _bounds(args)
        bounds.validate()
        return float(bounds.max_runtime_s)
    if args.command == "publish":
        bounds = RollupBounds(max_runtime_s=args.max_runtime_s, max_retries=args.max_retries)
        bounds.validate()
        return float(bounds.max_runtime_s)
    if args.command == "report":
        bounds = RollupBounds(max_runtime_s=args.max_runtime_s)
        bounds.validate()
        return float(bounds.max_runtime_s)
    raise ValueError(f"command {args.command!r} has no worker runtime budget")


def _run_killable_command(args: argparse.Namespace) -> int:
    payload = vars(args).copy()
    payload.pop("handler", None)
    return _run_killable_process(
        _worker_entry,
        (args.command, payload),
        timeout_s=_command_runtime(args),
    )


def _bool_env(name: str, default: bool) -> bool:
    value = os.environ.get(PREFIX + name)
    if value is None:
        return default
    normalized = value.strip().lower()
    if normalized not in {"0", "1", "false", "true", "no", "yes"}:
        raise ValueError(f"{PREFIX + name} must be true/false")
    return normalized in {"1", "true", "yes"}


def _settings(args: argparse.Namespace) -> ConnectionSettings:
    password = os.environ.get(PREFIX + "PASSWORD")
    if password is None:
        password = os.environ.get(PREFIX + "PASSWD")
    missing: list[str] = []
    host = args.host or os.environ.get(PREFIX + "HOST")
    database = args.database or os.environ.get(PREFIX + "DATABASE")
    user = args.user or os.environ.get(PREFIX + "USER")
    if not host:
        missing.append("host/" + PREFIX + "HOST")
    if not database:
        missing.append("database/" + PREFIX + "DATABASE")
    if not user:
        missing.append("user/" + PREFIX + "USER")
    if password is None:
        missing.append(PREFIX + "PASSWORD")
    if missing:
        raise ValueError("missing explicit rollup connection values: " + ", ".join(missing))
    settings = ConnectionSettings(
        host=host,
        database=database,
        user=user,
        password=password,
        port=args.port if args.port is not None else int(os.environ.get(PREFIX + "PORT", "3306")),
        tls_ca=args.tls_ca if args.tls_ca is not None else os.environ.get(PREFIX + "SSL_CA"),
        tls_verify=args.tls_verify if args.tls_verify is not None else _bool_env("TLS_VERIFY", False),
        secure_tunnel=args.secure_tunnel if args.secure_tunnel is not None else _bool_env("SECURE_TUNNEL", False),
        connect_timeout_s=float(os.environ.get(PREFIX + "CONNECT_TIMEOUT_S", "2")),
        read_timeout_s=float(os.environ.get(PREFIX + "READ_TIMEOUT_S", "3")),
        write_timeout_s=float(os.environ.get(PREFIX + "WRITE_TIMEOUT_S", "3")),
        statement_timeout_s=float(os.environ.get(PREFIX + "STATEMENT_TIMEOUT_S", "2")),
        lock_timeout_s=float(os.environ.get(PREFIX + "LOCK_TIMEOUT_S", "2")),
    )
    settings.validate()
    return settings


def _target(args: argparse.Namespace) -> RollupTarget:
    return RollupTarget(
        definition_version=args.definition_version,
        generation=args.generation,
        environment_id=args.environment_id,
        season_id=args.season_id,
    )


def _bounds(args: argparse.Namespace) -> RollupBounds:
    return RollupBounds(
        page_size=args.page_size,
        max_rows=args.max_rows,
        max_page_bytes=args.max_page_bytes,
        max_total_bytes=args.max_total_bytes,
        max_output_fanout=args.max_output_fanout,
        max_transaction_statements=args.max_transaction_statements,
        max_runtime_s=args.max_runtime_s,
        max_retries=args.max_retries,
        statement_timeout_s=float(os.environ.get(PREFIX + "STATEMENT_TIMEOUT_S", "2")),
        socket_timeout_s=float(os.environ.get(PREFIX + "READ_TIMEOUT_S", "3")),
        lock_timeout_s=float(os.environ.get(PREFIX + "LOCK_TIMEOUT_S", "2")),
    )


def _json_default(value: Any) -> Any:
    if isinstance(value, (date,)):
        return value.isoformat()
    if isinstance(value, bytes):
        return value.hex()
    raise TypeError(f"not JSON serializable: {type(value).__name__}")


def _add_connection_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--host", help="explicit database host; otherwise TELEMETRY_ROLLUP_DB_HOST")
    parser.add_argument("--port", type=int, help="database TCP port")
    parser.add_argument("--database", help="explicit database name; otherwise TELEMETRY_ROLLUP_DB_DATABASE")
    parser.add_argument("--user", help="explicit rollup user; otherwise TELEMETRY_ROLLUP_DB_USER")
    parser.add_argument("--tls-ca", help="CA file for verified non-loopback TLS")
    tls = parser.add_mutually_exclusive_group()
    tls.add_argument("--tls-verify", dest="tls_verify", action="store_true")
    tls.add_argument("--no-tls-verify", dest="tls_verify", action="store_false")
    tunnel = parser.add_mutually_exclusive_group()
    tunnel.add_argument("--secure-tunnel", dest="secure_tunnel", action="store_true")
    tunnel.add_argument("--no-secure-tunnel", dest="secure_tunnel", action="store_false")
    parser.set_defaults(tls_verify=None, secure_tunnel=None)


def _add_target_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--definition-version", type=int, required=True)
    parser.add_argument("--generation", type=int, required=True)
    parser.add_argument("--environment-id", type=int, required=True)
    parser.add_argument("--season-id", type=int, required=True)


def _add_run_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--through-ingest-id", type=int)
    parser.add_argument("--origin-ingest-id", type=int, default=0)
    parser.add_argument("--page-size", type=int, default=100)
    parser.add_argument("--max-rows", type=int, default=10_000)
    parser.add_argument("--max-page-bytes", type=int, default=4 * 1024 * 1024)
    parser.add_argument("--max-total-bytes", type=int, default=32 * 1024 * 1024)
    parser.add_argument("--max-output-fanout", type=int, default=2_000)
    parser.add_argument("--max-transaction-statements", type=int, default=5_000)
    parser.add_argument("--max-runtime-s", type=float, default=30.0)
    parser.add_argument("--max-retries", type=int, default=2)


def _add_publication_budget_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--max-runtime-s", type=float, default=10.0)
    parser.add_argument("--max-retries", type=int, default=2)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="bounded replay-safe telemetry rollup")
    subparsers = parser.add_subparsers(dest="command", required=True)

    definitions = subparsers.add_parser("definitions", help="print stable report definitions without SQL")
    definitions.set_defaults(handler=_definitions)

    run = subparsers.add_parser("run", help="process one explicit generation through one fixed raw high-water mark")
    _add_target_arguments(run)
    _add_connection_arguments(run)
    _add_run_arguments(run)
    run.set_defaults(handler=_run)

    publish = subparsers.add_parser("publish", help="atomically publish one complete explicit generation")
    _add_target_arguments(publish)
    _add_connection_arguments(publish)
    _add_publication_budget_arguments(publish)
    publish.set_defaults(handler=_publish)

    report = subparsers.add_parser("report", help="read one named aggregate report and its coverage metadata")
    _add_target_arguments(report)
    _add_connection_arguments(report)
    report.add_argument("--name", required=True, help="session_playtime or cohort_activity")
    report.add_argument("--max-rows", type=int, default=REPORT_ROW_LIMIT_DEFAULT)
    report.add_argument("--max-bytes", type=int, default=REPORT_BYTE_LIMIT_DEFAULT)
    report.add_argument("--max-runtime-s", type=float, default=REPORT_RUNTIME_DEFAULT_S)
    report.set_defaults(handler=_report)
    return parser


def _definitions(_args: argparse.Namespace) -> int:
    print(json.dumps({"definitions": report_catalog()}, sort_keys=True))
    return 0


def _database(args: argparse.Namespace) -> PyMySQLRollupDatabase:
    settings = _settings(args)
    return PyMySQLRollupDatabase(PyMySQLConnectionFactory(settings))


def _run(args: argparse.Namespace) -> int:
    target = _target(args)
    database = _database(args)
    try:
        result = RollupEngine(database).run(
            target,
            through_ingest_id=args.through_ingest_id,
            origin_ingest_id=args.origin_ingest_id,
            bounds=_bounds(args),
        )
        print(json.dumps(result.public_dict(), default=_json_default, sort_keys=True))
    finally:
        database.close()
    return 0


def _publish(args: argparse.Namespace) -> int:
    target = _target(args)
    database = _database(args)
    try:
        bounds = RollupBounds(max_runtime_s=args.max_runtime_s, max_retries=args.max_retries)
        print(json.dumps(dict(RollupEngine(database).publish(target, bounds=bounds)), sort_keys=True))
    finally:
        database.close()
    return 0


def _report(args: argparse.Namespace) -> int:
    target = _target(args)
    database = _database(args)
    try:
        snapshot = RollupEngine(database).report(
            target,
            args.name,
            max_rows=args.max_rows,
            max_bytes=args.max_bytes,
            max_runtime_s=args.max_runtime_s,
        )
        print(
            json.dumps(
                {
                    "definition": snapshot.definition.public_dict(),
                    "coverage": snapshot.coverage.public_dict(),
                    "truncated": snapshot.truncated,
                    "rows": list(snapshot.rows),
                },
                default=_json_default,
                sort_keys=True,
            )
        )
    finally:
        database.close()
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        if args.command == "definitions":
            return int(args.handler(args))
        return _run_killable_command(args)
    except (ValueError, RuntimeError) as error:
        print(f"telemetry rollup failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
