#!/usr/bin/env python3
"""Behavioral disposable-MySQL regression for SQL lifecycle ownership."""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tests/async/run_economic_sql_lifecycle_owner_mysql.sh"
command = ([sys.executable, str(ROOT / "tests/async/run_economic_sql_lifecycle_owner_loopback.py")]
           if os.environ.get("ECONOMIC_SQL_LIFECYCLE_DISPOSABLE_SERVER") == "1"
           else ["bash", str(SCRIPT)])
result = subprocess.run(command, cwd=ROOT, check=False)
if result.returncode:
    raise SystemExit(result.returncode)
print("economic SQL lifecycle owner behavior passed")
