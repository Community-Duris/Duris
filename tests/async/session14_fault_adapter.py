#!/usr/bin/env python3
"""Validated JSON-lines contract for deployment-owned Session 14 fault controls."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


MANIFEST = Path(__file__).with_name("session14_gate_manifest.json")
SAFE_ID = re.compile(r"^[A-Za-z0-9][A-Za-z0-9_.:-]{0,79}$")


def allowed_actions() -> set[str]:
    with MANIFEST.open(encoding="ascii") as handle:
        payload = json.load(handle)
    actions = payload.get("faults")
    if not isinstance(actions, list) or not all(isinstance(value, str) for value in actions):
        raise RuntimeError("fault manifest is invalid")
    return set(actions)


@dataclass(frozen=True)
class AdapterResult:
    action: str
    state: str
    detail_id: str


def validate_argv(argv: object) -> list[str]:
    if not isinstance(argv, list) or not argv or len(argv) > 32:
        raise ValueError("adapter argv must contain 1-32 arguments")
    if not all(isinstance(value, str) and value and "\x00" not in value for value in argv):
        raise ValueError("adapter argv contains an invalid argument")
    return list(argv)


def run_adapter(argv: object, action: str, phase: str, timeout_seconds: int = 30) -> AdapterResult:
    command = validate_argv(argv)
    if action not in allowed_actions():
        raise ValueError("fault action is not allow-listed")
    if phase not in {"preflight", "inject", "teardown", "verify"}:
        raise ValueError("invalid adapter phase")
    request = json.dumps({"schema_version": 1, "action": action, "phase": phase}) + "\n"
    completed = subprocess.run(
        command,
        input=request,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        timeout=timeout_seconds,
        check=False,
        shell=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(f"adapter failed: action={action} phase={phase}")
    lines = completed.stdout.splitlines()
    if len(lines) != 1:
        raise RuntimeError("adapter returned an invalid response count")
    payload = json.loads(lines[0])
    if set(payload) != {"schema_version", "action", "state", "detail_id"}:
        raise RuntimeError("adapter returned an invalid response schema")
    if payload["schema_version"] != 1 or payload["action"] != action:
        raise RuntimeError("adapter response identity mismatch")
    expected_state = {
        "preflight": "ready", "inject": "injected",
        "teardown": "restored", "verify": "verified",
    }[phase]
    if payload["state"] != expected_state:
        raise RuntimeError("adapter returned an invalid state")
    detail_id = payload["detail_id"]
    if not isinstance(detail_id, str) or SAFE_ID.fullmatch(detail_id) is None:
        raise RuntimeError("adapter returned an invalid detail ID")
    return AdapterResult(action, payload["state"], detail_id)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--adapter", type=Path, required=True)
    parser.add_argument("--action", required=True)
    parser.add_argument("--phase", required=True)
    args = parser.parse_args()
    result = run_adapter([str(args.adapter)], args.action, args.phase)
    json.dump(result.__dict__, sys.stdout, sort_keys=True)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
