#!/usr/bin/env python3
"""Keep the reviewed artifact corpus and effective source bindings reproducible."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[2]
subprocess.run([sys.executable, str(root/'scripts/artifact_source_inventory.py'), '--check'],
               cwd=root, check=True)
