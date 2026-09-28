#!/usr/bin/env python3
"""SQL copyover + cold-restart journey using a parent-frozen DB binary.

Required resource lease (slot 1, then shared docker-heavy lock):
  flock -w 900 -x PLAN_ROOT/resources/test-slot-1.lock \
    flock -w 900 -s /opt/data/workspaces/duris-persistence-tools/docker-heavy.lock \
    python3 tests/async/test_pa_copyover_sql.py \
      --descriptor /absolute/path/to/batch-build.json --source-sha SOURCE_COMMIT

The descriptor and expected source commit are supplied together by the parent
batch. With no arguments, the original S02 base-build descriptor/source pair
remains available for compatibility. The loader never builds or selects a
different binary and always checks the descriptor's source, PASS evidence,
backend/profile, file permissions, and actual binary SHA-256.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import stat
import sys

import pa_copyover_fixture

ROOT = Path(__file__).resolve().parents[2]
PLAN_ROOT = Path("/opt/data/workspaces/duris-persistence-tools/parallel-db-rollout-23d086e3")
DESCRIPTOR = PLAN_ROOT / "base-build.json"
BASE_SHA = "23d086e3a76f2bd424aa4034c5879ceba0d5d594"
PASS_STATUS = "BUILD_AND_LOCAL_CONTRACTS_PASS"
SOURCE_SHA_PATTERN = re.compile(r"[0-9a-f]{40}")
BINARY_SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")


def load_frozen_binary(
        descriptor_path: str | Path = DESCRIPTOR,
        expected_source_sha: str = BASE_SHA,
) -> tuple[Path, str, dict]:
    if not isinstance(expected_source_sha, str) or not SOURCE_SHA_PATTERN.fullmatch(expected_source_sha):
        raise AssertionError("expected source SHA must be a 40-character lowercase Git SHA")
    descriptor_file = Path(descriptor_path)
    try:
        descriptor = json.loads(descriptor_file.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise AssertionError(f"cannot read frozen build descriptor {descriptor_file}: {error}") from error
    if not isinstance(descriptor, dict):
        raise AssertionError("frozen build descriptor must be a JSON object")
    if descriptor.get("head") != expected_source_sha:
        raise AssertionError(
            f"frozen build source SHA mismatch: {descriptor.get('head')!r} "
            f"!= expected {expected_source_sha}"
        )
    if descriptor.get("status") != PASS_STATUS:
        raise AssertionError(f"frozen build did not PASS: {descriptor.get('status')!r}")
    if descriptor.get("backend") != "mariadb":
        raise AssertionError(f"frozen build is not a MariaDB DB binary: {descriptor.get('backend')!r}")
    if descriptor.get("profile") != "development/TEST_MUD":
        raise AssertionError(f"unexpected frozen build profile: {descriptor.get('profile')!r}")
    checks = descriptor.get("checks")
    if not isinstance(checks, list) or not checks or any(
            not isinstance(check, dict) or type(check.get("exit")) is not int or
            check.get("exit") != 0 for check in checks):
        raise AssertionError("frozen build descriptor contains a failed or missing check")
    binary_value = descriptor.get("binary")
    expected_sha = descriptor.get("binary_sha256")
    if not isinstance(binary_value, str) or not isinstance(expected_sha, str):
        raise AssertionError("frozen build descriptor omits binary path or SHA-256")
    if not BINARY_SHA256_PATTERN.fullmatch(expected_sha):
        raise AssertionError("frozen build descriptor contains an invalid binary SHA-256")
    binary = Path(binary_value)
    if not binary.is_absolute() or binary.is_symlink() or not binary.is_file():
        raise AssertionError(f"frozen DB binary is missing or not a regular file: {binary}")
    metadata = binary.stat()
    if metadata.st_mode & (stat.S_IWGRP | stat.S_IWOTH) or not metadata.st_mode & 0o111:
        raise AssertionError("frozen DB binary must be executable and not group/world writable")
    digest = hashlib.sha256(binary.read_bytes()).hexdigest()
    if digest != expected_sha:
        raise AssertionError(f"frozen DB binary hash differs from descriptor: {digest}")
    print(f"PA_COPYOVER_FROZEN status={PASS_STATUS} checks={len(checks)} "
          f"base={expected_source_sha} sha256={digest} binary={binary}", flush=True)
    return binary, digest, descriptor


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--descriptor",
        type=Path,
        help="parent-frozen JSON build descriptor (requires --source-sha)",
    )
    parser.add_argument(
        "--source-sha",
        help="expected 40-character source commit recorded in descriptor.head",
    )
    args = parser.parse_args(argv)
    if (args.descriptor is None) != (args.source_sha is None):
        parser.error("--descriptor and --source-sha must be supplied together")
    return args


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if Path.cwd().resolve() != ROOT:
        raise SystemExit(f"run from repository root {ROOT}")
    descriptor = args.descriptor if args.descriptor is not None else DESCRIPTOR
    source_sha = args.source_sha if args.source_sha is not None else BASE_SHA
    binary, digest, _ = load_frozen_binary(descriptor, source_sha)
    pa_copyover_fixture.run(binary, digest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
