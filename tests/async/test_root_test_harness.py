"""Contracts for the repository-level build and regression harness."""

import ast
import importlib.util
import os
import re
import subprocess
import sys
import json
import io
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
specs = runner.inventory(runner.TEST_DIRECTORY, runner.MANIFEST)
manual_names = {spec.path.name for spec in specs if spec.manual}

# Preserve the target branch's bounded server-journey concurrency.
server_journeys = (
    "test_account_recovery_journey.py", "test_area_coin_pickup.py",
    "test_creation_prompt_journey.py", "test_flatfile_auction_coin_put_journey.py",
    "test_flatfile_boot_preflight.py", "test_flatfile_chaos_new_character_kit.py",
    "test_flatfile_combat_journey.py", "test_flatfile_first_session_currency.py",
    "test_flatfile_full_world_boot.py", "test_flatfile_newbie_regrant_journey.py",
    "test_game_loop_session_journey.py", "test_information_cache_journey.py",
    "test_item_movement_prompt_runtime.py", "test_mysql_combat_journey.py",
    "test_network_readiness_journey.py", "test_persistent_transport_journey.py",
    "test_player_quarantine_restore.py", "test_static_quest_reward_journey.py",
)
by_name = {spec.path.name: spec for spec in specs}
assert all("server-journey" in by_name[name].locks for name in server_journeys)
discovered = {path.name for path in runner.discover_tests(None)}
assert not (manual_names & discovered)
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
    def test_explicit_arguments_and_fixture_environment_reach_observed_child(self):
        path = self.script("test_arguments.py", "import sys, os\n"
                           "assert sys.argv[1:] == ['--fixture', 'literal space']\n"
                           "assert os.environ['OWNED_FIXTURE'] == 'private'\n"
                           "assert 'INHERITED_DB' not in os.environ\n")
        environment = dict(os.environ, OWNED_FIXTURE="private")
        environment.pop("INHERITED_DB", None)
        with patch.object(runner, "ROOT", self.root):
            result = runner.run_test(path, 5, arguments=("--fixture", "literal space"),
                                     environment=environment)
        self.assertEqual(result.returncode, 0, result.output)
        self.assertEqual(result.cases[0]["status"], "passed")

    def integration_module(self):
        spec = importlib.util.spec_from_file_location("matrix_contract", ROOT / "tests/run_integration_matrix.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_matrix_refuses_missing_identity_hidden_skip_and_missing_native_witness(self):
        matrix = self.integration_module()
        path = self.script("test_named.py", "import io, unittest\n"
                           "class Native(unittest.TestCase):\n"
                           " def test_real(self): self.skipTest('fixture absent')\n"
                           "unittest.main(testRunner=unittest.TextTestRunner(stream=io.StringIO()))\n")
        spec = runner.TestSpec(path, mode="unittest")
        with patch.object(runner, "ROOT", self.root):
            result = runner.run_test(path, 5, spec=spec)
        row = dict(required_cases=["Native.test_real", "Native.test_removed"],
                   required_markers=["PASS: actual native witness"])
        failures = matrix.outcome_contract(row, result)
        self.assertTrue(any("missing required cases" in text for text in failures))
        self.assertTrue(any("skipped" in text for text in failures))
        self.assertTrue(any("missing requirement evidence" in text for text in failures))

    def test_matrix_validates_required_owner_coverage_before_filters(self):
        matrix = self.integration_module()
        document = json.loads((ROOT / "tests/integration_manifest.json").read_text())
        matrix.workload(document, specs)
        required = next(spec for spec in specs if spec.manual)
        for row in document["rows"]:
            row["covers"] = [name for name in row["covers"] if name != required.path.name]
        with self.assertRaisesRegex(ValueError, "omits required owners"):
            matrix.workload(document, specs)

    def test_once_matrix_preserves_both_pinned_database_images(self):
        matrix = self.integration_module()
        document = json.loads((ROOT / "tests/integration_manifest.json").read_text())
        observed = []

        def capture(row, tokens, environment, spec_map):
            observed.append(environment)
            return dict(row=row["id"], engine=tokens["engine"], status="passed")

        tokens = dict(descriptor="unused", sql_binary="unused", flat_binary="unused",
                      head="a" * 40, binary_sha256="b" * 64)
        with (patch.object(matrix, "freeze_build", return_value=tokens),
              patch.object(matrix, "image_identity", return_value={"id": "sha256:" + "c" * 64}),
              patch.object(matrix.subprocess, "check_output", return_value="a" * 40 + "\n"),
              patch.object(matrix, "run_row", side_effect=capture)):
            self.assertEqual(matrix.main(["--engine", "once", "--match", "pa_runtime_sql",
                                          "--evidence-root", str(self.root / "evidence")]), 0)
        self.assertEqual(len(observed), 1)
        self.assertEqual(observed[0]["PA_RUNTIME_SQL_MARIADB_IMAGE"], document["engines"]["mariadb"])
        self.assertEqual(observed[0]["PA_RUNTIME_SQL_MYSQL_IMAGE"], document["engines"]["mysql"])

    def test_matrix_repeated_filters_select_each_requested_owner_once(self):
        matrix = self.integration_module()
        with patch("sys.stdout", new_callable=io.StringIO) as output:
            self.assertEqual(matrix.main(["--engine", "once", "--match", "epic_save_guards",
                                          "--match", "flatfile_accounting_bank", "--list"]), 0)
        selected = [line.split()[1] for line in output.getvalue().splitlines()]
        self.assertCountEqual(selected, ["epic_save_guards", "flatfile_accounting_bank"])

    def test_matrix_reports_pending_required_rows_as_incomplete_in_json_and_junit(self):
        matrix = self.integration_module()
        report = dict(status="incomplete", attempts=[], pending=["mysql/required_native"])
        path = self.root / "matrix.json"
        matrix.write_report(path, report)
        self.assertEqual(json.loads(path.read_text())["pending"], report["pending"])
        xml = runner.ET.parse(path.with_suffix(".xml")).getroot()
        self.assertEqual(xml.attrib["failures"], "1")
        self.assertEqual(xml.find("testcase/failure").attrib["type"], "incomplete")

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="regression-runner-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def script(self, name, body):
        path = self.root / name
        path.write_text(body)
        return path

    def fixture_inventory(self):
        entries = {}
        for path in self.root.glob("*.py"):
            if path.name.startswith("test_") or path.name.endswith("_test.py"):
                entries[path.name] = {"profile": "fast", "purpose": "isolated runner fixture",
                                      "seconds": 0 if "boot_preflight" in path.name else 1}
        manifest = self.root / "inventory.json"
        manifest.write_text(json.dumps({"version": 1, "tests": entries}))
        return manifest

    def test_discovers_both_names_once_and_keeps_manual_fixtures_explicit(self):
        for name in ("test_one.py", "other_test.py", "test_both_test.py", "helper.py"):
            self.script(name, "")
        manual = next(iter(manual_names))
        self.script(manual, "raise RuntimeError('must not execute')")
        manifest = self.fixture_inventory()
        rows = json.loads(manifest.read_text())
        rows["tests"][manual].update(manual=True, profile="database", reason="explicit fixture")
        manifest.write_text(json.dumps(rows))
        with (patch.object(runner, "TEST_DIRECTORY", self.root),
              patch.object(runner, "MANIFEST", manifest)):
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

    @unittest.skipUnless(sys.platform == "linux", "requires Linux process groups")
    def test_fault_control_timeout_retains_redacted_log_and_stops_descendants(self):
        import qualify_behavioral_faults as qualification

        child = self.script("fault_child.py", "import os, signal, time\n"
                            "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
                            "open(__file__ + '.pid', 'w').write(str(os.getpid()))\n"
                            "while True: time.sleep(1)\n")
        parent = self.script("fault_owner.py", "import subprocess, sys, time\n"
                             f"subprocess.Popen([sys.executable, {str(child)!r}], stdout=subprocess.DEVNULL)\n"
                             "print('partial diagnostic: synthetic-secret', flush=True)\n"
                             "while True: time.sleep(1)\n")
        log = self.root / "fault-before.log"
        with self.assertRaises(subprocess.TimeoutExpired):
            qualification.run_owner([sys.executable, str(parent)], self.root, dict(os.environ),
                                    log, lambda text: text.replace("synthetic-secret", "<redacted>"),
                                    timeout=0.5)
        self.assertIn("partial diagnostic: <redacted>", log.read_text())
        self.assertNotIn("synthetic-secret", log.read_text())
        pid = int(Path(str(child) + ".pid").read_text())
        deadline = time.monotonic() + 2
        while time.monotonic() < deadline:
            status = Path(f"/proc/{pid}/stat")
            if not status.exists() or status.read_text().split()[2] == "Z":
                break
            time.sleep(0.01)
        else:
            os.kill(pid, signal.SIGKILL)
            self.fail("fault qualification left a live descendant")

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
        self.script("test_chosen_bad.py", "print('immediate failure evidence', flush=True); raise AssertionError('broken')")
        self.script("test_chosen_slow.py", "import time; time.sleep(0.6); print('slow completed')")
        self.script("test_excluded.py", "raise AssertionError('filtered entry must not run')")
        report = self.root / "result.json"
        # Use a private test directory, with the real runner and real child
        # interpreters. No mocks of execution or timing can satisfy this check.
        manifest = self.fixture_inventory()
        command = (
            "import runpy, pathlib, sys; "
            f"r=runpy.run_path({str(RUNNER)!r}, run_name='runner_fixture'); "
            "main=r['main']; "
            f"main.__globals__['ROOT']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['TEST_DIRECTORY']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['MANIFEST']=pathlib.Path({str(manifest)!r}); "
            f"sys.argv=['runner', '--jobs', '2', '--timeout', '5', '--match', 'chosen_', '--report', {str(report)!r}]; "
            "sys.exit(main())"
        )
        completed = subprocess.run([sys.executable, "-c", command], text=True,
                                   capture_output=True, timeout=10, cwd=ROOT)
        self.assertEqual(completed.returncode, 1, completed.stdout + completed.stderr)
        self.assertLess(completed.stdout.index("immediate failure evidence"),
                        completed.stdout.index("PASS test_chosen_slow.py"))
        payload = json.loads(report.read_text())
        self.assertFalse(payload["interrupted"])
        self.assertEqual({row["path"]: row["status"] for row in payload["results"]},
                         {"test_chosen_bad.py": "failed", "test_chosen_slow.py": "passed"})
        self.assertCountEqual(payload["selected"], ["test_chosen_bad.py", "test_chosen_slow.py"])
        self.assertEqual(payload["excluded"], ["test_excluded.py"])
        self.assertFalse(payload["pending"])
        self.assertEqual(payload["inventory_sha256"], runner.hashlib.sha256(manifest.read_bytes()).hexdigest())
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
        manifest = self.fixture_inventory()
        command = (
            "import runpy, pathlib, sys; "
            f"r=runpy.run_path({str(RUNNER)!r}, run_name='runner_fixture'); main=r['main']; "
            f"main.__globals__['ROOT']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['TEST_DIRECTORY']=pathlib.Path({str(self.root)!r}); "
            f"main.__globals__['MANIFEST']=pathlib.Path({str(manifest)!r}); "
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


    def test_rejects_unclassified_entries_and_stale_inventory(self):
        self.script("test_one.py", "assert True")
        manifest = self.fixture_inventory()
        self.script("test_omitted.py", "assert True")
        with self.assertRaisesRegex(ValueError, "unclassified: test_omitted.py"):
            runner.inventory(self.root, manifest)
        manifest = self.fixture_inventory()
        (self.root / "test_one.py").unlink()
        with self.assertRaisesRegex(ValueError, "missing files: test_one.py"):
            runner.inventory(self.root, manifest)

    def test_zero_unittest_or_disconnected_function_entrypoint_cannot_pass(self):
        fixtures = {
            "empty_unit.py": "import unittest\nclass Checks(unittest.TestCase):\n def test_real(self): assert False\n",
            "empty_main.py": "def main():\n raise AssertionError('must execute')\n",
            "omitted_function.py": "def test_real():\n raise AssertionError('must execute')\n",
            "ignored_unit_failure.py": "import unittest\nclass Checks(unittest.TestCase):\n"
                                       " def test_real(self): assert False\nunittest.main(exit=False)\n",
            "ignored_class_cleanup.py": "import io, unittest\nclass Checks(unittest.TestCase):\n"
                                        " def test_real(self): pass\n"
                                        " @classmethod\n def tearDownClass(cls): raise RuntimeError('cleanup failed')\n"
                                        "unittest.main(exit=False, testRunner=unittest.TextTestRunner(stream=io.StringIO()))\n",
            "ignored_module_cleanup.py": "import io, unittest\nclass Checks(unittest.TestCase):\n"
                                         " def test_real(self): pass\n"
                                         "def tearDownModule(): raise RuntimeError('cleanup failed')\n"
                                         "unittest.main(exit=False, testRunner=unittest.TextTestRunner(stream=io.StringIO()))\n",
            "early_script_exit.py": "import os\nos._exit(0)\n",
            "early_unit_exit.py": "import os, unittest\nclass Checks(unittest.TestCase):\n"
                                  " def test_real(self): os._exit(0)\nunittest.main()\n",
            "exit_after_unit_cases.py": "import os, unittest\nclass Checks(unittest.TestCase):\n"
                                       " def test_real(self): self.assertTrue(True)\n"
                                       "unittest.main(exit=False)\nos._exit(0)\n",
        }
        for name, body in fixtures.items():
            with self.subTest(name=name):
                result = runner.run_test(self.script(name, body), timeout=5)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("case execution contract failed", result.output)

    def test_named_case_results_and_junit_preserve_failure_and_partial_skip(self):
        path = self.script("unit.py", "import unittest\n"
                           "class Cases(unittest.TestCase):\n"
                           " def test_pass(self): self.assertTrue(True)\n"
                           " def test_skip(self): self.skipTest('explicit fixture')\n"
                           " def test_failure(self): self.assertEqual(1, 2)\n"
                           "if __name__ == '__main__': unittest.main()\n")
        result = runner.run_test(path, timeout=5)
        outcomes = {row["id"].split("::")[-1]: row["status"] for row in result.cases}
        self.assertEqual(outcomes, {"Cases.test_pass": "passed", "Cases.test_skip": "skipped",
                                    "Cases.test_failure": "failed"})
        xml = self.root / "cases.xml"
        runner.write_junit(xml, [result])
        document = runner.ET.parse(xml).getroot()
        self.assertEqual(document.attrib["failures"], "1")
        self.assertEqual(document.attrib["skipped"], "1")

    def timed_script(self, name, pause):
        begin, end = self.root / (name + ".begin"), self.root / (name + ".end")
        path = self.script("test_" + name + ".py", "from pathlib import Path\nimport time\n"
                           f"Path({str(begin)!r}).write_text(str(time.monotonic()))\n"
                           f"time.sleep({pause})\n"
                           f"Path({str(end)!r}).write_text(str(time.monotonic()))\n")
        return path, begin, end

    def test_resource_scheduler_overlaps_light_work_and_bounds_heavy_work(self):
        a = self.timed_script("heavy_a", 0.3)
        b = self.timed_script("heavy_b", 0.1)
        light = self.timed_script("light", 0.05)
        specs = [runner.TestSpec(a[0], cpu=2, memory_mb=768, seconds=3),
                 runner.TestSpec(b[0], cpu=2, memory_mb=768, seconds=2),
                 runner.TestSpec(light[0], cpu=1, memory_mb=256, seconds=1)]
        results = []
        runner.schedule(specs, 3, 3, 1024, threading.Event(), results.append)
        self.assertTrue(all(row.returncode == 0 for row in results))
        self.assertLess(float(light[1].read_text()), float(a[2].read_text()))
        self.assertGreaterEqual(float(b[1].read_text()), float(a[2].read_text()))

    def test_resource_locks_and_duration_history_control_actual_start_order(self):
        a = self.timed_script("first", 0.1)
        b = self.timed_script("second", 0.2)
        light = self.timed_script("independent", 0.05)
        specs = [runner.TestSpec(a[0], locks=("exclusive-fixture",)),
                 runner.TestSpec(b[0], locks=("exclusive-fixture",)),
                 runner.TestSpec(light[0])]
        estimates = {runner.relative(a[0]): 1, runner.relative(b[0]): 10,
                     runner.relative(light[0]): 0.1}
        results = []
        runner.schedule(specs, 3, 4, 4096, threading.Event(), results.append, durations=estimates)
        self.assertLess(float(light[1].read_text()), float(b[2].read_text()))
        self.assertGreaterEqual(float(a[1].read_text()), float(b[2].read_text()))
        self.assertEqual(len(results), 3)

    def test_required_integration_profile_rejects_a_successful_skip(self):
        path = self.script("test_db.py", "print('SKIP: explicitly disposable fixture required')")
        manifest = self.fixture_inventory()
        row = json.loads(manifest.read_text())
        row["tests"][path.name]["profile"] = "database"
        manifest.write_text(json.dumps(row))
        report = self.root / "database.json"
        with (patch.object(runner, "ROOT", self.root), patch.object(runner, "TEST_DIRECTORY", self.root),
              patch.object(runner, "MANIFEST", manifest), patch.object(sys, "argv", [
                  "runner", "--profile", "database", "--report", str(report)])):
            self.assertEqual(runner.main(), 1)
        self.assertEqual(json.loads(report.read_text())["results"][0]["status"], "skipped")

    def test_unittest_skips_do_not_depend_on_console_summary(self):
        body = ("import io, unittest\n"
                "class Checks(unittest.TestCase):\n"
                " def test_pass(self): self.assertTrue(True)\n"
                " def test_skip(self): self.skipTest('owned fixture missing')\n"
                "if __name__ == '__main__':\n"
                " unittest.main(testRunner=unittest.TextTestRunner(stream=io.StringIO()))\n")
        path = self.script("test_db_cases.py", body)
        spec = runner.TestSpec(path, mode="unittest", minimum_cases=2)
        with patch.object(runner, "ROOT", self.root):
            result = runner.run_test(path, 2, spec=spec)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.status, "passed")
        self.assertEqual(result.skipped_checks, 1)
        skipped = [case for case in result.cases if case["status"] == "skipped"]
        self.assertEqual(skipped[0]["reason"], "owned fixture missing")

        manifest = self.fixture_inventory()
        rows = json.loads(manifest.read_text())
        rows["tests"][path.name].update(profile="database", mode="unittest", minimum_cases=2)
        manifest.write_text(json.dumps(rows))
        report = self.root / "database-cases.json"
        with (patch.object(runner, "ROOT", self.root), patch.object(runner, "TEST_DIRECTORY", self.root),
              patch.object(runner, "MANIFEST", manifest), patch.object(sys, "argv", [
                  "runner", "--profile", "database", "--report", str(report)])):
            self.assertEqual(runner.main(), 1)

        path.write_text(body.replace("self.assertTrue(True)", "self.skipTest('all unavailable')"))
        with patch.object(runner, "ROOT", self.root):
            result = runner.run_test(path, 2, spec=spec)
        self.assertEqual(result.status, "skipped")
        self.assertEqual(result.skipped_checks, 2)

    def test_hidden_subtest_skip_remains_required_coverage_failure(self):
        path = self.script("test_subtest.py", "import io, unittest\n"
                           "class Checks(unittest.TestCase):\n"
                           " def test_database(self):\n"
                           "  with self.subTest(engine='mysql'): self.skipTest('owned fixture missing')\n"
                           "  with self.subTest(engine='mariadb'): self.assertTrue(True)\n"
                           "unittest.main(testRunner=unittest.TextTestRunner(stream=io.StringIO()))\n")
        result = runner.run_test(path, 5, spec=runner.TestSpec(path, mode="unittest"))
        self.assertEqual(result.returncode, 0, result.output)
        self.assertEqual(result.skipped_checks, 1)
        skipped = [row for row in result.cases if row["status"] == "skipped"]
        self.assertEqual(skipped[0]["reason"], "owned fixture missing")
        self.assertIn("engine='mysql'", skipped[0]["id"])
        matrix = self.integration_module()
        failures = matrix.outcome_contract(dict(required_cases=["Checks.test_database"],
                                                required_markers=[]), result)
        self.assertIn("required integration checks skipped", failures)
        xml = self.root / "subtest.xml"
        runner.write_junit(xml, [result])
        self.assertEqual(runner.ET.parse(xml).getroot().attrib["skipped"], "1")


if __name__ == "__main__":
    unittest.main()
