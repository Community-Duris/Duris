#!/usr/bin/env python3
"""Regression tests for sanitizer result classification, not transport proofs."""
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch
import test_telemetry_transport as runner


def result(code=0, output=""):
    return subprocess.CompletedProcess(["test-process"], code, output)


class SanitizerClassification(unittest.TestCase):
    def test_signal_in_actual_harness_fails(self):
        with patch.object(runner, "compile_harness"), patch.object(
            runner.subprocess, "run", side_effect=[result(), result(), result(-11)]
        ), self.assertRaises(subprocess.CalledProcessError):
            runner.try_tsan(Path("/unused"))

    def test_known_failure_in_trivial_runtime_probe_is_unsupported(self):
        with patch.object(runner, "compile_harness") as compile_real, patch.object(
            runner.subprocess, "run", side_effect=[result(), result(66, "FATAL: ThreadSanitizer: unexpected memory mapping\n")]
        ):
            runner.try_tsan(Path("/unused"))
            compile_real.assert_not_called()

    def test_ordinary_compile_error_is_not_unsupported(self):
        with patch.object(runner.subprocess, "run", return_value=result(1, "syntax error")), \
             self.assertRaises(subprocess.CalledProcessError):
            runner.try_tsan(Path("/unused"))


if __name__ == "__main__":
    unittest.main()
