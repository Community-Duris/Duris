#!/usr/bin/env python3
"""Execute the real extracted policy with typed boundary fixtures in both modes.

This is a narrow policy regression, not SQL/journal/ACK-lock qualification.
Root runs it only after reviewing and applying the complete private proposal.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    proposal_root = args.proposal_root.resolve()
    with tempfile.TemporaryDirectory(prefix="duris-held-policy-") as directory:
        for mode in ("sql", "flat"):
            executable = Path(directory) / mode
            command = [
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pthread",
                "-ffunction-sections", "-fdata-sections", "-I", str(proposal_root / "src"),
                "-I", str(source_root / "src"),
                "-x", "c++", str(proposal_root / "src/item/held_retirement_publication.c"),
                str(proposal_root / "tests/async/held_retirement_publication_policy_harness.cpp"),
                "-Wl,--gc-sections", "-o", str(executable),
            ]
            if mode == "flat":
                command[1:1] = ["-D__NO_MYSQL__", "-I", str(source_root / "src/no_mysql")]
            subprocess.run(command, check=True, cwd=source_root)
            subprocess.run([str(executable)], check=True, cwd=source_root)
            print(f"held-retirement publication policy {mode}: PASS")


if __name__ == "__main__":
    main()
