#!/usr/bin/env python3
"""Run Wind Blade creation, retry, save and reconnect against disposable MariaDB."""

from __future__ import annotations

# Exact source hashes captured from the untouched S04 worktree before recovery:
# pa_item_creation_fixture.py: a1fa8e36615ba768d95fdf6f7187882fd0afef8894a5a02be4ef565fcf4bae22
# test_pa_item_creation_sql.py: b940da77924e9125ee28937836414f65ad328ff52b8d758a1148666f14d3b481

import argparse
import os
from pathlib import Path

from pa_item_creation_fixture import run_wind_blade_sql_journey


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path,
                        help="immutable parent DB server binary recorded in --base-build")
    parser.add_argument("--base-build", required=True, type=Path,
                        help="parent base-build.json matching the binary and assigned base")
    parser.add_argument("--expected-head", required=True,
                        help="exact source HEAD recorded by the parent batch build")
    parser.add_argument("--expected-binary-sha256", required=True,
                        help="exact SHA-256 recorded by the parent batch build")
    parser.add_argument("--evidence", required=True, type=Path,
                        help="write sanitized result/failure evidence here")
    args = parser.parse_args()
    if os.environ.get("DURIS_PARALLEL_DB_TEST_SLOT") != "1":
        raise SystemExit("run under the assigned slot-1 and shared Docker-heavy locks")
    evidence = run_wind_blade_sql_journey(
        args.binary, args.base_build, args.expected_head,
        args.expected_binary_sha256, args.evidence,
    )
    print("[PASS] Wind Blade SQL creation/retry/reconnect journey")
    print("base binary SHA-256:", evidence["binary_sha256"])
    for phase in evidence["phases"]:
        print("-", phase)
    print("cleanup:", evidence["cleanup"])
    print("evidence:", args.evidence)


if __name__ == "__main__":
    main()
