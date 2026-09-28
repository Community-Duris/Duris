#!/usr/bin/env python3
"""Behavioral disposable-MySQL regression for SQL lifecycle ownership."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tests/async/run_economic_sql_lifecycle_owner_mysql.sh"
result = subprocess.run(["bash", str(SCRIPT)], cwd=ROOT, check=False)
if result.returncode:
    raise SystemExit(result.returncode)
print("economic SQL lifecycle owner behavior passed")
