#!/usr/bin/env python3
"""Client-free environment isolation checks; never start a database or server."""
import os
import unittest
from unittest.mock import patch

from pa_web_recovery_fixture import DisposableMariaDB, _clean_environment


class WebFixtureEnvironmentTests(unittest.TestCase):
    def test_ambient_database_socket_is_removed(self):
        for socket_path in ("/synthetic/not-a-real-db.sock", ""):
            with self.subTest(socket_path=socket_path), patch.dict(
                os.environ,
                {"DB_SOCKET": socket_path, "PATH": "/synthetic/tools"},
                clear=True,
            ):
                environment = _clean_environment()
                self.assertNotIn("DB_SOCKET", environment)
                self.assertEqual(environment["PATH"], "/synthetic/tools")

    def test_database_environment_preserves_only_its_disposable_endpoint(self):
        # Do not initialize or enter the fixture; exercise its real env method
        # with synthetic attributes, without containers, sockets or credentials.
        database = object.__new__(DisposableMariaDB)
        database.port = 43001
        database.database = "corpse_journey_test_synthetic"
        database.user = "synthetic_fixture_user"
        database.password = "synthetic-not-a-real-password"
        with patch.dict(os.environ, {
            "DB_SOCKET": "/synthetic/not-a-real-db.sock",
            "DB_HOST": "synthetic-invalid-host",
            "DB_NAME": "synthetic_wrong_database",
            "DB_PORT": "43002",
            "ENVIRONMENT": "production",
            "LD_LIBRARY_PATH": "/synthetic/libraries",
        }, clear=True):
            environment = database.env()
        self.assertNotIn("DB_SOCKET", environment)
        self.assertEqual(environment["DB_HOST"], "127.0.0.1")
        self.assertEqual(environment["DB_PORT"], "43001")
        self.assertEqual(environment["DB_NAME"], database.database)
        self.assertEqual(environment["DB_ALLOWED_TARGETS"], "127.0.0.1/" + database.database)
        self.assertEqual(environment["ENVIRONMENT"], "local")
        self.assertEqual(environment["TEST_DB_DISPOSABLE"], "1")
        self.assertEqual(environment["LD_LIBRARY_PATH"], "/synthetic/libraries")


if __name__ == "__main__":
    unittest.main()
