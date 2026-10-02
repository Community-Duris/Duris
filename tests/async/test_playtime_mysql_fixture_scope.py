#!/usr/bin/env python3
"""Keep the permanent-table probe inside its fresh disposable journey schema."""

import os
from pathlib import Path
import runpy
import subprocess
import unittest
from unittest import mock

PROBE = Path(__file__).with_name("test_playtime_mysql_repository.py")
DATABASE = "playtime_test_012345abcdef"


class CompilerCaptured(Exception):
    pass


class FixtureScopeTests(unittest.TestCase):
    def settings(self):
        return {"DB_HOST": "127.0.0.1", "DB_NAME": DATABASE,
                "DB_ALLOWED_TARGETS": "127.0.0.1/" + DATABASE}

    def test_unowned_schema_refuses_before_compilation_or_connection(self):
        for database in ("", "duris", "playtime_test_existing", DATABASE + "suffix"):
            with self.subTest(database=database):
                settings = self.settings()
                settings.update(DB_NAME=database, DB_ALLOWED_TARGETS="127.0.0.1/" + database)
                with mock.patch.dict(os.environ, settings, clear=True), mock.patch.object(
                        subprocess, "run", side_effect=CompilerCaptured) as compiler:
                    with self.assertRaisesRegex(RuntimeError, "fresh disposable"):
                        runpy.run_path(str(PROBE))
                    compiler.assert_not_called()

    def test_remote_or_unapproved_target_refuses_before_compilation(self):
        for override in ({"DB_HOST": "localhost"}, {"DB_HOST": "192.0.2.1"},
                         {"DB_ALLOWED_TARGETS": ""}, {"DB_ALLOWED_TARGETS": "127.0.0.1/other"}):
            with self.subTest(override=override):
                settings = self.settings()
                settings.update(override)
                with mock.patch.dict(os.environ, settings, clear=True), mock.patch.object(
                        subprocess, "run", side_effect=CompilerCaptured) as compiler:
                    with self.assertRaisesRegex(RuntimeError, "fresh disposable"):
                        runpy.run_path(str(PROBE))
                    compiler.assert_not_called()

    def test_selected_disposable_schema_reaches_compilation_without_running_it(self):
        with mock.patch.dict(os.environ, self.settings(), clear=True), mock.patch.object(
                subprocess, "run", side_effect=CompilerCaptured) as compiler:
            with self.assertRaises(CompilerCaptured):
                runpy.run_path(str(PROBE))
            compiler.assert_called_once()
            self.assertEqual(compiler.call_args.args[0][0], "g++")


if __name__ == "__main__":
    unittest.main()
