#!/usr/bin/env python3
"""Verify deletion qualification reaches only the selected disposable SQL port."""

import os
from pathlib import Path
import unittest
from unittest import mock

import run_mysql_deletion_journey as journey


class SQLCaptured(Exception):
    pass


class DeletionPortTests(unittest.TestCase):
    def probe(self, port):
        settings = {"TEST_DB_HOST": "127.0.0.1", "TEST_DB_USER": "fixture-only",
                    "TEST_DB_PASSWORD": "fixture-only"}
        if port is not None:
            settings["TEST_DB_PORT"] = port
        captured = {}

        def capture(arguments, **kwargs):
            captured.update(arguments=arguments, environment=kwargs["env"])
            raise SQLCaptured

        with mock.patch.dict(os.environ, settings, clear=True), mock.patch.object(
                journey.subprocess, "check_output", side_effect=capture):
            with self.assertRaises(SQLCaptured):
                journey.run(Path(__file__))
        return captured

    def test_selected_and_default_ports_reach_cli_and_server(self):
        for supplied, expected in (("34668", "34668"), (None, "3306")):
            with self.subTest(port=supplied):
                captured = self.probe(supplied)
                self.assertEqual(captured["environment"]["DB_PORT"], expected)
                self.assertIn("-P", captured["arguments"])
                self.assertEqual(captured["arguments"][captured["arguments"].index("-P") + 1], expected)

    def test_invalid_port_refuses_before_connecting(self):
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
