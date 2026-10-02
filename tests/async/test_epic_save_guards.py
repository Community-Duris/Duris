#!/usr/bin/env python3
"""Execute epic refund callers with refused transactions and failed checkpoints.

The whole production translation unit is compiled. External persistence and
terminal output are fixture boundaries; no save or refund is simulated as durable.
"""
import subprocess
import tempfile
from pathlib import Path
import unittest

from native_build_artifacts import build_native

ROOT = Path(__file__).resolve().parents[2]


class EpicSaveGuards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        cls.directory = tempfile.TemporaryDirectory(prefix="epic-refund-", dir=ROOT / "bin/tests")
        cls.addClassCleanup(cls.directory.cleanup)
        cls.binary = build_native(
            Path(cls.directory.name) / "refund",
            ["tests/async/epic_save_guards_harness.cpp"],
            ["-std=c++20", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
             "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql"],
            ["-Wl,--gc-sections"], name="epic-save-guards",
        )
        print("EPIC-NATIVE compiled", flush=True)

    def test_refused_refund_preserves_skills_and_never_attempts_save(self):
        subprocess.run([str(self.binary), "refused"], cwd=ROOT, check=True, timeout=20)

    def test_committed_refund_checks_save_result_and_reports_failed_checkpoint(self):
        subprocess.run([str(self.binary), "checkpoint"], cwd=ROOT, check=True, timeout=20)


if __name__ == "__main__":
    unittest.main()
