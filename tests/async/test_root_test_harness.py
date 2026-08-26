"""Contracts for the repository-level build and regression harness."""

import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAKEFILE = ROOT / "Makefile"
RUNNER = ROOT / "tests" / "run_regression_tests.py"

assert MAKEFILE.is_file(), "the repository root Makefile is missing"
makefile = MAKEFILE.read_text()
for target in (
    "build",
    "build-server",
    "build-editor",
    "build-area-tools",
    "build-deps-package",
    "world",
    "test",
    "test-all",
    "test-python",
    "test-native",
    "test-list",
    "test-db",
    "clean-all",
):
    assert re.search(rf"^{re.escape(target)}(?:\s*:|:)", makefile, re.MULTILINE), (
        f"root Makefile target is missing: {target}"
    )

assert "tests/run_regression_tests.py" in makefile
assert "tests/async/run_signal_handlers.sh" in makefile
assert re.search(r"^test-all:\s*build\s*$", makefile, re.MULTILINE)
assert "$(MAKE) test" in makefile

editor_makefile = (ROOT / "areas" / "de" / "src" / "Makefile").read_text()
assert re.search(r"^de:\s*\$\(DE_BINARY\)$", editor_makefile, re.MULTILINE)
assert re.search(
    r"^\$\(DE_BINARY\):\s*\$\(OBJS\)\s+\$\(C_OBJS\)\s+\|\s+message$",
    editor_makefile,
    re.MULTILINE,
), (
    "the area editor status target must not force an unchanged relink"
)

listed = subprocess.run(
    [sys.executable, str(RUNNER), "--list", "--match", Path(__file__).name],
    cwd=ROOT,
    check=True,
    stdout=subprocess.PIPE,
    text=True,
).stdout.splitlines()
assert listed == ["tests/async/test_root_test_harness.py", "1 test(s)"]

dry_run = subprocess.run(
    ["make", "-n", "test-list", "TEST_MATCH=root_test_harness"],
    cwd=ROOT,
    check=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
).stdout
assert "run_regression_tests.py --list" in dry_run

workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text()
assert "make test-all" in workflow, "CI does not exercise the root test gate"
assert "make -j`nproc` -C src" not in workflow, "CI duplicates the root build harness"

testing_doc = (ROOT / "docs" / "TESTING.md").read_text()
assert "make test-all" in testing_doc
assert "TEST_MATCH" in testing_doc

for wrapper in (ROOT / "tests" / "async").glob("run_*.sh"):
    assert os.access(wrapper, os.X_OK), f"test wrapper is not executable: {wrapper.name}"

for script in ("m_slow", "m_quick", "make_all", "moveall", "make_lookup"):
    lines = (ROOT / "areas" / script).read_text().splitlines()
    assert lines[:2] == ["#!/bin/sh", "set -eu"], (
        f"areas/{script} must stop when a generation step fails"
    )

print("root build and test harness contracts passed")
