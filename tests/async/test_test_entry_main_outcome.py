#!/usr/bin/env python3
"""Exercise real entry outcomes in isolated Python processes, without services."""
from __future__ import annotations

import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest

ROOT = Path(__file__).resolve().parents[2]
ADAPTER = ROOT / "tests/run_test_entry.py"
MATRIX = None


class MainOutcome(unittest.TestCase):
    def run_entry(self, text, mode="script"):
        with tempfile.TemporaryDirectory(prefix="duris-main-outcome-") as directory:
            fixture = Path(directory) / "fixture.py"
            events = Path(directory) / "events.json"
            fixture.write_text(text, encoding="utf-8")
            result = subprocess.run(
                [sys.executable, "-B", str(ADAPTER), str(fixture), mode, str(events), "1"],
                text=True, capture_output=True, timeout=15,
                env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"),
            )
            observed = json.loads(events.read_text(encoding="utf-8"))
        self.assertTrue(observed["complete"])
        self.assertEqual(len(observed["cases"]), len({row["id"] for row in observed["cases"]}))
        return result, observed

    def contract(self, process, observed, required):
        result = SimpleNamespace(
            cases=observed["cases"], returncode=process.returncode,
            status="passed" if process.returncode == 0 else "failed",
            output=process.stdout + process.stderr, skipped_checks=0,
        )
        return MATRIX.outcome_contract(
            dict(required_cases=required, required_markers=[]), result,
        )

    def test_main_success_satisfies_original_matrix_contract(self):
        process, observed = self.run_entry("def main():\n    return 0\nmain()\n")
        self.assertEqual(process.returncode, 0)
        self.assertEqual(observed["called"], ["main"])
        self.assertEqual({row["id"]: row["status"] for row in observed["cases"]},
                         {"script": "passed", "main": "passed"})
        self.assertEqual(self.contract(process, observed, ["script", "main"]), [])

    def test_main_exception_does_not_claim_pass(self):
        process, observed = self.run_entry("def main():\n    raise RuntimeError('fixture failure')\nmain()\n")
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual(observed["called"], ["main"])
        self.assertEqual({row["id"]: row["status"] for row in observed["cases"]},
                         {"script": "failed", "main": "unresolved"})
        self.assertTrue(self.contract(process, observed, ["script", "main"]))

    def test_failure_after_main_keeps_invocation_unresolved(self):
        process, observed = self.run_entry("def main():\n    return 0\nmain()\nraise RuntimeError('later failure')\n")
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual({row["id"]: row["status"] for row in observed["cases"]},
                         {"script": "failed", "main": "unresolved"})
        self.assertTrue(self.contract(process, observed, ["script", "main"]))

    def test_uncalled_main_is_absent_and_contract_refuses(self):
        process, observed = self.run_entry("def main():\n    raise AssertionError('must not execute')\nVALUE = 1\n")
        self.assertNotEqual(process.returncode, 0)
        self.assertEqual(observed["called"], [])
        self.assertEqual({row["id"]: row["status"] for row in observed["cases"]}, {"script": "failed"})
        failures = self.contract(process, observed, ["script", "main"])
        self.assertTrue(any("missing required cases: main" in failure for failure in failures))

    def test_system_exit_uses_original_script_status(self):
        for code, exit_code, status in (("0", 0, "passed"), ("None", 0, "passed"),
                                        ("3", 3, "unresolved"), ("'fixture exit'", 1, "unresolved")):
            with self.subTest(code=code):
                process, observed = self.run_entry(f"def main():\n    raise SystemExit({code})\nmain()\n")
                self.assertEqual(process.returncode, exit_code)
                statuses = {row["id"]: row["status"] for row in observed["cases"]}
                self.assertEqual(statuses["main"], status)
                self.assertEqual(statuses["script"], "passed" if exit_code == 0 else "failed")
                self.assertEqual(bool(self.contract(process, observed, ["script", "main"])), exit_code != 0)

    def test_no_main_retains_script_only_outcome(self):
        process, observed = self.run_entry("VALUE = 1\n")
        self.assertEqual(process.returncode, 0)
        self.assertEqual(observed["called"], [])
        self.assertEqual({row["id"]: row["status"] for row in observed["cases"]}, {"script": "passed"})
        self.assertEqual(self.contract(process, observed, ["script"]), [])

    def test_function_invocations_keep_existing_status_semantics(self):
        for failure in (False, True):
            with self.subTest(failure=failure):
                text = "def test_check():\n    assert 2 + 2 == 4\ndef main():\n    test_check()\nmain()\n"
                if failure:
                    text += "raise RuntimeError('later failure')\n"
                process, observed = self.run_entry(text)
                self.assertEqual(observed["called"], ["main", "test_check"])
                self.assertEqual({row["id"]: row["status"] for row in observed["cases"]},
                                 {"script": "failed" if failure else "passed",
                                  "main": "unresolved" if failure else "passed",
                                  "test_check": "unresolved" if failure else "passed"})
                self.assertEqual(bool(self.contract(process, observed, ["script", "main", "test_check"])), failure)

    def test_unittest_mode_keeps_collected_case_identity(self):
        for failure in (False, True):
            with self.subTest(failure=failure):
                expected = 5 if failure else 4
                process, observed = self.run_entry(
                    "import unittest\nclass Checks(unittest.TestCase):\n"
                    f"    def test_ok(self):\n        self.assertEqual(2 + 2, {expected})\n"
                    "def main():\n    unittest.main()\nmain()\n", mode="unittest",
                )
                self.assertEqual(process.returncode, 1 if failure else 0)
                self.assertEqual(observed["called"], [])
                self.assertEqual(observed["collected"], ["Checks.test_ok"])
                self.assertEqual({row["id"]: row["status"] for row in observed["cases"]},
                                 {"Checks.test_ok": "failed" if failure else "passed"})
                self.assertEqual(bool(self.contract(process, observed, ["Checks.test_ok"])), failure)


def main():
    global ADAPTER, MATRIX
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--adapter", type=Path, default=ADAPTER)
    args = parser.parse_args()
    ADAPTER = args.adapter.resolve()
    source_root = args.source_root.resolve()
    sys.path.insert(0, str(source_root / "tests"))
    spec = importlib.util.spec_from_file_location("duris_main_outcome_matrix", source_root / "tests/run_integration_matrix.py")
    MATRIX = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(MATRIX)
    unittest.main(argv=[sys.argv[0]], verbosity=2)


if __name__ == "__main__":
    main()
