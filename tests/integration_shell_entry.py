#!/usr/bin/env python3
"""Run a maintained shell fixture through the existing entry observer."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def main():
    path = (ROOT / sys.argv[1]).resolve()
    if not path.is_relative_to(ROOT / "tests/async") or path.suffix != ".sh":
        raise SystemExit("matrix shell entry must be a maintained tests/async wrapper")
    subprocess.run(["bash", str(path), *sys.argv[2:]], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
