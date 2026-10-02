#!/usr/bin/env python3
"""Verify the disposable journey routes every SQL client to its selected port."""

import os
from pathlib import Path
import unittest
from unittest import mock

import test_mysql_playtime_journey as journey


class SQLCaptured(Exception):
    pass


class FixturePortTests(unittest.TestCase):
    def probe(self, port):
        settings = {"TEST_DB_HOST": "127.0.0.1", "TEST_DB_USER": "fixture-only",
                    "TEST_DB_PASSWORD": "fixture-only"}
        if port is not None:
            settings["TEST_DB_PORT"] = port
        captured = {}

        def capture(arguments, **kwargs):
            captured.update(arguments=arguments, environment=kwargs["env"])
            raise SQLCaptured

        # Stop at the first SQL call: no connection, schema or server process
        # exists, including when testing the pre-fix wrong-port behavior.
        with mock.patch.dict(os.environ, settings, clear=True), mock.patch.object(
                journey.subprocess, "check_output", side_effect=capture):
            with self.assertRaises(SQLCaptured):
                journey.run(Path(__file__))
        return captured

    def test_explicit_port_reaches_cli_and_native_environment(self):
        captured = self.probe("34667")
        self.assertEqual(captured["environment"]["DB_PORT"], "34667")
        self.assertIn("-P", captured["arguments"])
        self.assertEqual(captured["arguments"][captured["arguments"].index("-P") + 1], "34667")

    def test_default_port_is_explicit_for_every_client(self):
        captured = self.probe(None)
        self.assertEqual(captured["environment"]["DB_PORT"], "3306")
        self.assertIn("-P", captured["arguments"])
        self.assertEqual(captured["arguments"][captured["arguments"].index("-P") + 1], "3306")

    def test_invalid_port_refuses_before_any_connection(self):
        for port in ("0", "65536", "-1", "", "3306suffix", "٣٣٠٦", "9" * 100):
            with self.subTest(port=port), mock.patch.dict(os.environ, {
                    "TEST_DB_HOST": "127.0.0.1", "TEST_DB_PORT": port,
                    "TEST_DB_USER": "fixture-only", "TEST_DB_PASSWORD": "fixture-only"},
                    clear=True), mock.patch.object(journey.subprocess, "check_output",
                                                  side_effect=SQLCaptured) as sql:
                with self.assertRaisesRegex(RuntimeError, "TEST_DB_PORT"):
                    journey.run(Path(__file__))
                sql.assert_not_called()


if __name__ == "__main__":
    unittest.main()
