"""Contracts for the repository-level build and regression harness."""

import importlib.util
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

runner_spec = importlib.util.spec_from_file_location("duris_regression_runner", RUNNER)
assert runner_spec is not None and runner_spec.loader is not None
runner = importlib.util.module_from_spec(runner_spec)
sys.modules[runner_spec.name] = runner
runner_spec.loader.exec_module(runner)
expected_resource_intensive = {
    "test_account_recovery_journey.py",
    "test_area_coin_pickup.py",
    "test_flatfile_boot_preflight.py",
    "test_flatfile_chaos_new_character_kit.py",
    "test_flatfile_combat_journey.py",
    "test_flatfile_newbie_regrant_journey.py",
    "test_flatfile_first_session_currency.py",
    "test_flatfile_full_world_boot.py",
    "test_item_movement_prompt_runtime.py",
    "test_mysql_combat_journey.py",
}
assert runner.RESOURCE_INTENSIVE_TEST_NAMES == expected_resource_intensive
sample_tests = [
    Path("test_fast.py"),
    Path("test_flatfile_combat_journey.py"),
    Path("test_account_recovery_journey.py"),
    Path("test_mysql_combat_journey.py"),
]
parallel_tests, resource_intensive_tests = runner.partition_tests(sample_tests)
assert parallel_tests == [Path("test_fast.py")]
assert resource_intensive_tests == sample_tests[1:]

editor_makefile = (ROOT / "areas" / "de" / "src" / "Makefile").read_text()
assert re.search(r"^CXX_STANDARD\s*=\s*-std=c\+\+20$", editor_makefile, re.MULTILINE)
for flags in ("CCFLAGS", "CFLAGS"):
    assert re.search(rf"^{flags}\s*=.*\$\(CXX_STANDARD\)", editor_makefile, re.MULTILINE), (
        f"the area editor {flags} must compile shared server headers as C++20"
    )
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

testing_doc = (ROOT / "docs" / "guides" / "TESTING.md").read_text()
assert "make test-all" in testing_doc
assert "TEST_MATCH" in testing_doc

for wrapper in (ROOT / "tests" / "async").glob("run_*.sh"):
    assert os.access(wrapper, os.X_OK), f"test wrapper is not executable: {wrapper.name}"
    source = wrapper.read_text()
    if "docker rm -f" in source:
        assert "docker run --rm -d" not in source, (
            f"{wrapper.name} races Docker auto-removal against its cleanup trap"
        )

for script in ("m_slow", "m_quick", "make_all", "moveall", "make_lookup"):
    lines = (ROOT / "areas" / script).read_text().splitlines()
    assert lines[:2] == ["#!/bin/sh", "set -eu"], (
        f"areas/{script} must stop when a generation step fails"
    )

print("root build and test harness contracts passed")
