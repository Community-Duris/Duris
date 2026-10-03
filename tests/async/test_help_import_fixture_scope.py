#!/usr/bin/env python3
"""Check help import fixture routing and refusal without any SQL connection."""

import os
import unittest
from unittest import mock

import test_help_import_atomic as fixture


class SQLCaptured(Exception):
    pass


class HelpImportFixtureScope(unittest.TestCase):
    def settings(self):
        return {"TEST_DB_HOST": "127.0.0.1", "TEST_DB_USER": "fixture-only",
                "TEST_DB_PASSWORD": "fixture-only", "TEST_DB_DISPOSABLE": "1"}

    def invoke(self):
        fixture.AtomicImport("test_rollback_and_consistent_publication").test_rollback_and_consistent_publication()

    def test_selected_and_default_connection(self):
        for supplied, expected in (("34671", "34671"), (None, "3306")):
            settings = self.settings()
            if supplied is not None:
                settings["TEST_DB_PORT"] = supplied
            captured = {}

            def capture(arguments, **kwargs):
                captured.update(arguments=arguments, environment=kwargs["env"])
                raise SQLCaptured

            with self.subTest(port=supplied), mock.patch.dict(os.environ, settings, clear=True), \
                    mock.patch.object(fixture.subprocess, "run", side_effect=capture):
                with self.assertRaises(SQLCaptured):
                    self.invoke()
                args, env = captured["arguments"], captured["environment"]
                self.assertIn("-P", args)
                self.assertEqual(args[args.index("-P") + 1], expected)
                self.assertEqual(args[args.index("-u") + 1], settings["TEST_DB_USER"])
                self.assertEqual(env["DB_PORT"], expected)
                self.assertEqual(env["DB_PASSWD"], settings["TEST_DB_PASSWORD"])
                self.assertEqual(env["MYSQL_PWD"], settings["TEST_DB_PASSWORD"])
                self.assertRegex(env["DB_NAME"], r"^help_import_test_[0-9a-f]{12}$")

    def test_invalid_ports_refuse_before_sql(self):
        for port in ("0", "65536", "-1", "", "3306suffix", "٣٣٠٦", "9" * 100):
            settings = dict(self.settings(), TEST_DB_PORT=port)
            with self.subTest(port=port), mock.patch.dict(os.environ, settings, clear=True), \
                    mock.patch.object(fixture.subprocess, "run") as sql:
                with self.assertRaisesRegex(RuntimeError, "TEST_DB_PORT"):
                    self.invoke()
                sql.assert_not_called()

    def test_non_disposable_or_remote_targets_refuse(self):
        for key, value in (("TEST_DB_DISPOSABLE", "0"), ("TEST_DB_HOST", "example.invalid")):
            settings = dict(self.settings(), **{key: value})
            with self.subTest(key=key), mock.patch.dict(os.environ, settings, clear=True), \
                    mock.patch.object(fixture.subprocess, "run") as sql:
                with self.assertRaisesRegex(RuntimeError, "disposable loopback"):
                    self.invoke()
                sql.assert_not_called()

    def test_missing_credentials_refuse_before_sql(self):
        for key in ("TEST_DB_USER", "TEST_DB_PASSWORD"):
            settings = self.settings()
            settings.pop(key)
            with self.subTest(key=key), mock.patch.dict(os.environ, settings, clear=True), \
                    mock.patch.object(fixture.subprocess, "run") as sql:
                with self.assertRaisesRegex(RuntimeError, key):
                    self.invoke()
                sql.assert_not_called()


if __name__ == "__main__":
    unittest.main()
