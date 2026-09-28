#!/usr/bin/env python3
"""Require and verify the parent-frozen integration-batch build artifact.

Set DURIS_ACCOUNTING_BASE_BUILD to the absolute path of the immutable
base-build.json before invoking one of the recovered SQL fixture families.
This helper never reads the checkout's .env.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat
import subprocess

BASE_BUILD_ENV = "DURIS_ACCOUNTING_BASE_BUILD"
PASS_STATUS = "BUILD_AND_LOCAL_CONTRACTS_PASS"
ROOT = Path(__file__).resolve().parents[2]
_SHA256 = re.compile(r"[0-9a-f]{64}\Z")
_GIT_SHA = re.compile(r"[0-9a-f]{40}\Z")


@dataclass(frozen=True)
class VerifiedBaseBuild:
    descriptor: Path
    head: str
    binary: Path
    binary_sha256: str
    source_manifest: Path


def _reject_private_path(path: Path, label: str) -> None:
    try:
        candidates = (path, path.resolve())
    except (OSError, RuntimeError) as exc:
        raise RuntimeError(f"cannot resolve {label}: {path}") from exc
    for candidate in candidates:
        if any(part == ".git" or part == ".env" or part.startswith(".env.")
               for part in candidate.parts):
            raise RuntimeError(f"{label} must not read private environment or Git metadata: {path}")


def _regular_absolute_file(value: object, label: str) -> Path:
    if not isinstance(value, str) or not value:
        raise RuntimeError(f"base-build descriptor omits {label}")
    path = Path(value).expanduser()
    _reject_private_path(path, label)
    if not path.is_absolute() or path.is_symlink() or not path.is_file():
        raise RuntimeError(f"{label} must be an absolute, regular, non-symlink file: {path}")
    return path


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as exc:
        raise RuntimeError(f"cannot read provenance input {path}: {exc}") from exc
    return digest.hexdigest()


def _require_immutable_metadata(path: Path, label: str) -> None:
    if path.stat().st_mode & (stat.S_IWGRP | stat.S_IWOTH):
        raise RuntimeError(f"{label} must not be group/world writable: {path}")


def _current_head(root: Path) -> str:
    result = subprocess.run(
        ["git", "rev-parse", "--verify", "HEAD"],
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    head = result.stdout.strip()
    if result.returncode or not _GIT_SHA.fullmatch(head):
        raise RuntimeError("cannot determine the repository's exact Git HEAD for artifact verification")
    return head


def load_base_build(root: Path = ROOT, *,
                    required_sources: tuple[str, ...] = ()) -> VerifiedBaseBuild:
    """Validate a frozen build and all declared and caller-required source inputs."""
    configured = os.environ.get(BASE_BUILD_ENV, "").strip()
    if not configured:
        raise RuntimeError(
            f"{BASE_BUILD_ENV} is required; set it to the absolute path of the "
            "parent-frozen integration-batch base-build.json (do not use an older rollout artifact)"
        )
    descriptor_path = Path(configured).expanduser()
    _reject_private_path(descriptor_path, "base-build descriptor")
    if not descriptor_path.is_absolute() or descriptor_path.is_symlink() or not descriptor_path.is_file():
        raise RuntimeError(
            f"{BASE_BUILD_ENV} must name an existing absolute, regular, non-symlink JSON file: "
            f"{descriptor_path}"
        )
    _require_immutable_metadata(descriptor_path, "base-build descriptor")
    try:
        descriptor = json.loads(descriptor_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"cannot read {BASE_BUILD_ENV} descriptor {descriptor_path}: {exc}") from exc
    if not isinstance(descriptor, dict):
        raise RuntimeError("base-build descriptor must be a JSON object")

    head = descriptor.get("head")
    if not isinstance(head, str) or not _GIT_SHA.fullmatch(head):
        raise RuntimeError("base-build descriptor must contain a lowercase 40-character Git head")
    current = _current_head(root.resolve())
    if head != current:
        raise RuntimeError(
            f"base-build head {head} does not match this checkout HEAD {current}; "
            "freeze and select an artifact for the exact integrated source batch"
        )
    if descriptor.get("status") != PASS_STATUS:
        raise RuntimeError(f"base-build status must be {PASS_STATUS!r}")
    checks = descriptor.get("checks")
    if not isinstance(checks, list) or not checks or any(
        not isinstance(check, dict) or type(check.get("exit")) is not int or check["exit"] != 0
        for check in checks
    ):
        raise RuntimeError("base-build descriptor must contain only recorded successful checks")
    if descriptor.get("backend") != "mariadb":
        raise RuntimeError("base-build artifact must target the MariaDB backend")
    if descriptor.get("profile") != "development/TEST_MUD":
        raise RuntimeError("base-build artifact must use the development/TEST_MUD profile")

    binary = _regular_absolute_file(descriptor.get("binary"), "binary path")
    mode = binary.stat().st_mode
    if not mode & 0o111 or mode & (stat.S_IWGRP | stat.S_IWOTH):
        raise RuntimeError("frozen DB binary must be executable and not group/world writable")
    expected_digest = descriptor.get("binary_sha256")
    if not isinstance(expected_digest, str) or not _SHA256.fullmatch(expected_digest):
        raise RuntimeError("base-build descriptor must contain a lowercase binary SHA-256")
    actual_digest = _sha256(binary)
    if actual_digest != expected_digest:
        raise RuntimeError(
            f"frozen DB binary SHA-256 mismatch: descriptor={expected_digest}, actual={actual_digest}"
        )

    manifest_path = _regular_absolute_file(descriptor.get("source_manifest"), "source manifest path")
    _require_immutable_metadata(manifest_path, "source manifest")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"cannot read source manifest {manifest_path}: {exc}") from exc
    if not isinstance(manifest, dict) or not manifest:
        raise RuntimeError("source manifest must be a non-empty path-to-SHA-256 JSON object")
    missing = [relative for relative in required_sources if relative not in manifest]
    if missing:
        raise RuntimeError("source manifest omits required source inputs: " + ", ".join(missing))

    repo_root = root.resolve()
    for relative, expected in manifest.items():
        if not isinstance(relative, str) or not isinstance(expected, str) or not _SHA256.fullmatch(expected):
            raise RuntimeError("source manifest contains a malformed path or SHA-256")
        relative_path = PurePosixPath(relative)
        if relative_path.is_absolute() or ".." in relative_path.parts or "\\" in relative:
            raise RuntimeError(f"source manifest path is not repository-relative: {relative!r}")
        source = repo_root.joinpath(*relative_path.parts)
        _reject_private_path(source, "source manifest input")
        if source.is_symlink() or not source.is_file():
            raise RuntimeError(f"source manifest input is absent or not a regular file: {relative}")
        try:
            source.resolve().relative_to(repo_root)
        except ValueError as exc:
            raise RuntimeError(f"source manifest input escapes the repository: {relative}") from exc
        if _sha256(source) != expected:
            raise RuntimeError(f"source manifest SHA-256 mismatch for {relative}")

    return VerifiedBaseBuild(
        descriptor=descriptor_path,
        head=head,
        binary=binary,
        binary_sha256=actual_digest,
        source_manifest=manifest_path,
    )


def report_base_build(build: VerifiedBaseBuild, *, scope: str) -> None:
    """Print artifact identity and make the execution boundary explicit."""
    print(
        f"PA_ACCOUNTING_BASE_BUILD head={build.head} sha256={build.binary_sha256} "
        f"scope={scope} binary={build.binary}",
        flush=True,
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--component", required=True, help="fixture family using this build descriptor")
    args = parser.parse_args()
    try:
        build = load_base_build()
    except RuntimeError as exc:
        parser.error(str(exc))
    report_base_build(build, scope=f"{args.component}:frozen-binary-not-executed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
