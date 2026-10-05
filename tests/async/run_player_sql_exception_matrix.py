#!/usr/bin/env python3
"""Compose existing immutable SQL exception owners on a matrix-owned schema."""
import argparse
import importlib.util
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OWNERS = {
    "snapshot": "test_player_snapshot_exception_mysql.py",
    "retained": "test_player_death_conflict_exception_mysql.py",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--owner", choices=OWNERS, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
    if not artifacts.is_relative_to((ROOT / "bin").resolve()) or artifacts.exists():
        parser.error("requires a new matrix-owned artifact directory below bin")
    source = Path(__file__).with_name(OWNERS[args.owner])
    spec = importlib.util.spec_from_file_location("matrix_sql_exception_owner", source)
    owner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(owner)
    if not owner.target_is_disposable(os.environ):
        raise RuntimeError("existing owner refuses the supplied disposable SQL target")
    artifacts.mkdir(parents=True, mode=0o700, exist_ok=False)
    binary = artifacts / "native-probe"
    built = owner.build(ROOT, binary)
    metadata = owner.verify(ROOT, binary, built["binary_sha256"])
    return owner.execute(ROOT, binary, metadata, artifacts / "runtime")


if __name__ == "__main__":
    raise SystemExit(main())
