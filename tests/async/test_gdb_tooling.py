"""Safety and dependency contracts for scripts/gdbdms."""

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/gdbdms"

assert SCRIPT.is_file(), "scripts/gdbdms is missing"
assert os.access(SCRIPT, os.X_OK), "scripts/gdbdms is not executable"

body = SCRIPT.read_text()
assert 'PORT="${1:-4000}"' in body, "GDB must default to development port 4000"
assert "PORT == 7777" in body, "GDB wrapper no longer refuses the production port"
assert "cp -f src/dms_new dms" in body, "GDB wrapper must preserve src/dms_new"
assert "mv src/dms_new dms" not in body, "GDB wrapper must not move the build output"
assert "command -v gdb" in body and "command -v nm" in body

subprocess.run(["bash", "-n", str(SCRIPT)], check=True)


def run(*args):
    return subprocess.run(
        ["bash", str(SCRIPT), *args], cwd=ROOT, capture_output=True, text=True
    )


helped = run("--help")
assert helped.returncode == 0 and "default port is 4000" in helped.stdout

for bad in ("7777", "abc", "80", "65536"):
    result = run(bad)
    assert result.returncode == 2, f"unsafe/invalid port {bad} was not rejected"

too_many = run("4000", "4001")
assert too_many.returncode == 2

print("GDB tooling contracts OK")
