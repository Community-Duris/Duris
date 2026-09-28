#!/usr/bin/env python3
"""Disposable SQL qualification for the SQL-first coin conversion slice."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tests/async/run_pa_coin_sql.sh"


class PaCoinSqlTest(unittest.TestCase):
    def test_legacy_coin_command_storage_matrix(self):
        completed = subprocess.run(
            [str(RUNNER)],
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
            timeout=600,
        )
        if completed.returncode:
            self.fail(
                f"coin SQL fixture failed ({completed.returncode})\n"
                f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
            )
        self.assertIn(
            "coin SQL: atomic conversion, rollback/replay, reload, retired custody",
            completed.stdout,
        )
        self.assertIn("Disposable SQL container absent after cleanup.", completed.stdout)


if __name__ == "__main__":
    unittest.main()
