"""Contracts for the repository-level build and regression harness."""

import ast
import importlib.util
import os
import re
import subprocess
import sys
import json
import signal
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
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
    "test_persistent_transport_journey.py",
    "test_player_quarantine_restore.py",
    "test_static_quest_reward_journey.py",
    "test_account_recovery_journey.py",
    "test_creation_prompt_journey.py",
    "test_game_loop_session_journey.py",
    "test_network_readiness_journey.py",
    "test_area_coin_pickup.py",
    "test_coin_publication_ack_retention.py",
    "test_flatfile_auction_coin_put_journey.py",
    "test_flatfile_boot_preflight.py",
    "test_flatfile_chaos_new_character_kit.py",
    "test_flatfile_combat_journey.py",
    "test_flatfile_newbie_regrant_journey.py",
    "test_flatfile_first_session_currency.py",
    "test_flatfile_full_world_boot.py",
    "test_item_movement_prompt_runtime.py",
    "test_information_cache_journey.py",
    "test_mysql_combat_journey.py",
}
assert runner.RESOURCE_INTENSIVE_TEST_NAMES == expected_resource_intensive
assert runner.MANUAL_ONLY_TEST_NAMES == {
    "test_player_save_journal_quarantine.py",
    "test_staging_migration_fork_mysql.py",
    "test_quest_recovery_read_budget_mysql.py",
    "test_economic_accounting_schema_mysql.py",
    "test_economic_baseline_schema_mysql.py",
    "test_economic_accounting_item_reference_mysql.py",
    "test_player_save_item_reconcile_mysql.py",
    "test_player_spell_effect_receipt_mysql.py",
    "test_economic_sql_lifecycle_owner_contract.py",
    "test_mob_gold_dial_runtime.py",
    "test_mysql_playtime_journey.py",
    "test_pet_restart_journey.py",
    "test_playtime_mysql_repository.py",
    "test_issue331_player_journey.py",
    "test_issue331_staff_recovery_journey.py",
    "test_death_resurrection_mysql_journey.py",
    "test_pa_runtime_sql.py",
    "test_pa_copyover_sql.py",
    "test_pa_copyover_account_authority.py",
    "test_pa_necromancy_sql.py",
    "test_pa_item_creation_sql.py",
    "test_pa_item_flags_sql.py",
    "test_pa_atm_publication_sql.py",
    "test_pa_coin_sql.py",
    "test_pa_web_recovery_sql.py",
}
discovered = {path.name for path in runner.discover_tests(None)}
assert not (runner.MANUAL_ONLY_TEST_NAMES & discovered)
assert {
    "test_player_playtime_capture.py", "test_playtime_checkpoint.py",
    "test_playtime_flatfile.py", "test_playtime_legacy_sql.py",
} <= discovered
assert {"telemetry_rollup_engine_test.py", "telemetry_rollup_budget_test.py"} <= discovered, (
    "suffix-named telemetry behavioral tests must be in the normal gate"
)
assert {
    "test_audit_accounting_invariants.py", "test_economic_accounting_item_reference.py",
    "test_item_transfer_accounting_context.py", "test_flatfile_item_accounting_reference.py",
    "test_coin_transfer_accounting.py", "test_corpse_lifecycle_accounting_context.py",
    "test_universal_item_transfer_accounting.py", "test_collector_accounting_context.py",
    "test_shop_trade_accounting_context.py", "test_auction_accounting_context.py",
    "test_player_death_restitution_accounting.py", "test_coin_transfer_item_accounting.py",
    "test_artifact_guild_repository.py", "test_zone_touch_repository.py",
    "test_account_bank_dual_backend_baseline.py", "test_player_load_items.py",
    "test_economic_sql_lifecycle_no_mysql.py", "test_sql_pool_discard_recovery.py",
} <= discovered, "the consolidated suite must retain every independently executed regression"
sample_tests = [
    Path("test_fast.py"),
    Path("test_flatfile_combat_journey.py"),
    Path("test_flatfile_auction_coin_put_journey.py"),
    Path("test_account_recovery_journey.py"),
    Path("test_mysql_combat_journey.py"),
    Path("test_information_cache_journey.py"),
    Path("test_creation_prompt_journey.py"),
    Path("test_persistent_transport_journey.py"),
    Path("test_network_readiness_journey.py"),
]
parallel_tests, resource_intensive_tests = runner.partition_tests(sample_tests)
assert parallel_tests == [Path("test_fast.py")]
assert resource_intensive_tests == sample_tests[1:]
# Every explicitly expensive owner must stay serialized, in discovery order.
all_resource_tests = [Path(name) for name in sorted(expected_resource_intensive)]
parallel_tests, resource_intensive_tests = runner.partition_tests(
    [Path("test_fast.py"), *all_resource_tests]
)
assert parallel_tests == [Path("test_fast.py")]
assert resource_intensive_tests == all_resource_tests

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


class RunnerBehavior(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="regression-runner-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def script(self, name, body):
        path = self.root / name
        path.write_text(body)
        return path

    def test_discovers_both_names_once_and_keeps_manual_fixtures_explicit(self):
        for name in ("test_one.py", "other_test.py", "test_both_test.py", "helper.py"):
            self.script(name, "")
        manual = next(iter(runner.MANUAL_ONLY_TEST_NAMES))
        self.script(manual, "raise RuntimeError('must not execute')")
        with patch.object(runner, "TEST_DIRECTORY", self.root):
            self.assertEqual([p.name for p in runner.discover_tests(None)],
                             ["other_test.py", "test_both_test.py", "test_one.py"])
            self.assertEqual([p.name for p in runner.discover_tests("one")], ["test_one.py"])

    def test_function_contract_entrypoint_executes_its_assertions(self):
        # This script used to define pytest-style cases but silently execute
        # none when launched by the standalone gate. Remove their source input
        # in a private copy: its real entrypoint must run and reject the cases.
        path = ROOT / "tests/async/test_training_dummy_contract.py"
        tree = ast.parse(path.read_text())
        provider = next(node for node in tree.body
                        if isinstance(node, ast.FunctionDef) and node.name == "source")
        provider.body = [ast.Return(value=ast.Constant(value=""))]
        expected = sum(isinstance(node, ast.FunctionDef) and node.name.startswith("test_")
                       for node in tree.body)
        private = self.script("missing_contracts.py", ast.unparse(ast.fix_missing_locations(tree)))
        command = (
            "import runpy, sys; "
            f"sys.path.insert(0, {str(ROOT / 'tests/async')!r}); "
            f"runpy.run_path({str(private)!r}, run_name='__main__')"
        )
        completed = subprocess.run([sys.executable, "-c", command], cwd=ROOT,
                                   capture_output=True, text=True, timeout=10)
        self.assertNotEqual(completed.returncode, 0, "contract cases were silently omitted")
        self.assertIn("AssertionError", completed.stdout + completed.stderr)
        self.assertIn(f"Ran {expected} tests", completed.stdout + completed.stderr)

    def test_failure_signal_and_skip_are_distinct(self):
        cases = [("pass", "print('done')", 0, "passed"),
                 ("failure", "print('diagnostic', flush=True); raise AssertionError('broken')", 1, "failed"),
                 ("skip", "print('SKIP: explicitly disposable DB required')", 0, "skipped")]
        if os.name == "posix":
            cases.append(("terminated", "import os, signal; os.kill(os.getpid(), signal.SIGTERM)",
                          -signal.SIGTERM, "signal"))
        for name, body, code, status in cases:
            with self.subTest(name=name):
                result = runner.run_test(self.script(name + ".py", body), timeout=5)
                self.assertEqual((result.returncode, result.status), (code, status))
                if name == "failure":
                    self.assertIn("diagnostic", result.output)
                    self.assertIn("broken", result.output)
        self.assertEqual(runner.skip_count("Ran 3 tests in 0.01s\n\nOK (skipped=1)\n"), (False, 1))
        self.assertEqual(runner.skip_count("Ran 1 test in 0.01s\n\nOK (skipped=1)\n"), (True, 1))
        self.assertEqual(runner.skip_count("checking SKIP: message handling\nOK"), (False, 0))
        self.assertEqual(runner.skip_count("PASS: flatfile\nSKIP: SQL requires disposable fixture"), (False, 1))

    @unittest.skipUnless(os.name == "posix", "requires POSIX process groups")
    def test_timeout_kills_term_resistant_child_and_retains_output(self):
        # The child closes stdout: waiting for just the parent/pipes cannot
        # detect that it survived TERM. The entire process group must be killed.
        child = self.script("child.py", "import os, signal, time\n"
                            "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
                            "open(__file__ + '.pid', 'w').write(str(os.getpid()))\n"
                            "while True: time.sleep(1)\n")
        parent = self.script("parent.py", f"import subprocess, sys, time\n"
                             f"subprocess.Popen([sys.executable, {str(child)!r}], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)\n"
                             "print('started before hang', flush=True)\n"
                             "while True: time.sleep(1)\n")
        result = runner.run_test(parent, timeout=0.5)
        self.assertEqual((result.returncode, result.status), (124, "timeout"))
        self.assertIn("started before hang", result.output)
        pid = int(Path(str(child) + ".pid").read_text())
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            status = Path(f"/proc/{pid}/stat")
            if not status.exists() or status.read_text().split()[2] == "Z":
                break
            time.sleep(0.01)
        else:
            os.kill(pid, signal.SIGKILL)
            self.fail("a timed-out test left a live child")

    def test_cancellation_stops_running_and_never_starts_queued_test(self):
        stop = threading.Event()
        marker = self.root / "started"
        path = self.script("cancel.py", f"from pathlib import Path\nimport time\n"
                           f"Path({str(marker)!r}).write_text('started')\nwhile True: time.sleep(1)\n")
        result = []
        thread = threading.Thread(target=lambda: result.append(runner.run_test(path, 10, stop)))
        thread.start()
        try:
            deadline = time.monotonic() + 3
            while not marker.exists() and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertTrue(marker.exists(), "fixture did not start")
        finally:
            stop.set()
            thread.join(timeout=5)
        self.assertFalse(thread.is_alive())
        self.assertEqual(result[0].status, "cancelled")
        marker.unlink()
        self.assertEqual(runner.run_test(path, 10, stop).status, "cancelled")
        self.assertFalse(marker.exists())

    def test_cli_reports_failure_before_a_slow_test_and_writes_timings(self):
        self.script("test_bad.py", "print('immediate failure evidence', flush=True); raise AssertionError('broken')")
        self.script("test_slow.py", "import time; time.sleep(0.6); print('slow completed')")
        report = self.root / "result.json"
        # Use a private test directory, with the real runner and real child
        # interpreters. No mocks of execution or timing can satisfy this check.
        command = (
            "import runpy, pathlib, sys; "
            f"r=runpy.run_path({str(RUNNER)!r}, run_name='runner_fixture'); "
            "main=r['main']; "
            f"main.__globals__['ROOT']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['TEST_DIRECTORY']=pathlib.Path({str(self.root)!r}); "
            f"sys.argv=['runner', '--jobs', '2', '--timeout', '5', '--report', {str(report)!r}]; "
            "sys.exit(main())"
        )
        completed = subprocess.run([sys.executable, "-c", command], text=True,
                                   capture_output=True, timeout=10, cwd=ROOT)
        self.assertEqual(completed.returncode, 1, completed.stdout + completed.stderr)
        self.assertLess(completed.stdout.index("immediate failure evidence"),
                        completed.stdout.index("PASS test_slow.py"))
        payload = json.loads(report.read_text())
        self.assertFalse(payload["interrupted"])
        self.assertEqual({row["path"]: row["status"] for row in payload["results"]},
                         {"test_bad.py": "failed", "test_slow.py": "passed"})
        self.assertTrue(all(row["elapsed"] > 0 for row in payload["results"]))
        self.assertFalse(any("output" in row for row in payload["results"]))

    @unittest.skipUnless(os.name == "posix", "requires POSIX signals")
    def test_cli_interrupt_cancels_running_and_queued_work_and_writes_report(self):
        marker = self.root / "running.pid"
        queued_marker = self.root / "queued"
        self.script("test_first.py", "import os, time\nfrom pathlib import Path\n"
                    f"Path({str(marker)!r}).write_text(str(os.getpid()))\n"
                    "while True: time.sleep(1)\n")
        self.script("test_second.py", "from pathlib import Path\n"
                    f"Path({str(queued_marker)!r}).write_text('must not start')\n")
        self.script("test_flatfile_boot_preflight.py", "from pathlib import Path\n"
                    f"Path({str(queued_marker)!r}).write_text('serialized test must not start')\n")
        report = self.root / "interrupted.json"
        command = (
            "import runpy, pathlib, sys; "
            f"r=runpy.run_path({str(RUNNER)!r}, run_name='runner_fixture'); main=r['main']; "
            f"main.__globals__['ROOT']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['TEST_DIRECTORY']=pathlib.Path({str(self.root)!r}); "
            f"sys.argv=['runner', '--jobs', '1', '--timeout', '30', '--report', {str(report)!r}]; "
            "sys.exit(main())"
        )
        process = subprocess.Popen([sys.executable, "-c", command], text=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, cwd=ROOT)
        try:
            deadline = time.monotonic() + 5
            while not marker.exists() and time.monotonic() < deadline and process.poll() is None:
                time.sleep(0.01)
            self.assertTrue(marker.exists(), "running fixture did not start")
            process.send_signal(signal.SIGINT)
            output = process.communicate(timeout=8)[0]
            self.assertEqual(process.returncode, 130, output)
            self.assertFalse(queued_marker.exists())
            payload = json.loads(report.read_text())
            self.assertTrue(payload["interrupted"])
            self.assertEqual(len(payload["results"]), 3)
            self.assertTrue(all(row["status"] == "cancelled" for row in payload["results"]))
        finally:
            if process.poll() is None:
                process.kill()
                process.communicate(timeout=3)
            if marker.exists():
                try:
                    os.killpg(int(marker.read_text()), signal.SIGKILL)
                except ProcessLookupError:
                    pass


if __name__ == "__main__":
    unittest.main()
