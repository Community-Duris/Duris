#!/usr/bin/env python3
"""No-network command-spy tests for the new telemetry migration verifier TLS policy."""
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VERIFIER = ROOT / "migrations/immutable/0014_telemetry_storage.sh"


class TelemetryMigrationTlsTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.log = self.directory / "commands.jsonl"
        self.ca = self.directory / "test ca.pem"
        self.ca.write_text("synthetic command-spy CA; never used for a connection\n")
        mysql = self.directory / "mysql"
        mysql.write_text("#!" + sys.executable + "\n" + '''import json, os, sys
with open(os.environ["SPY_LOG"], "a") as output:
    output.write(json.dumps(sys.argv[1:]) + "\\n")
if sys.argv[1:] == ["--help"]:
    print(os.environ.get("SPY_HELP", ""))
    sys.exit(int(os.environ.get("SPY_HELP_EXIT", "0")))
# Stop before any SQL executes. The command arguments are the entire assertion.
sys.exit(88)
''')
        mysql.chmod(0o700)
        self.environment = {
            "PATH": str(self.directory) + os.pathsep + os.environ["PATH"],
            "ENVIRONMENT": "production", "DB_HOST": "telemetry.invalid",
            "DB_PORT": "3406", "DB_USER": "command_spy", "DB_PASSWD": "synthetic",
            "DB_NAME": "telemetry_test", "DB_TLS": "TRUE", "DB_SSL_CA": str(self.ca),
            "SPY_LOG": str(self.log), "SPY_HELP": "--ssl-mode --ssl-ca",
        }

    def invoke(self, changes=None, omit=()):
        environment = dict(self.environment)
        environment.update(changes or {})
        for key in omit:
            environment.pop(key, None)
        self.log.unlink(missing_ok=True)
        result = subprocess.run(["bash", str(VERIFIER)], env=environment,
                                text=True, capture_output=True)
        calls = [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []
        sql_calls = [args for args in calls if args != ["--help"]]
        return result, sql_calls, calls

    def assert_denied(self, changes=None, omit=()):
        result, sql, _ = self.invoke(changes, omit)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(sql, [])
        self.assertIn("FAILED: telemetry verifier", result.stderr)

    def test_remote_mysql_requires_identity_verification_and_ca(self):
        result, sql, calls = self.invoke()
        self.assertEqual(result.returncode, 88)
        self.assertEqual(calls[0], ["--help"])
        self.assertEqual(len(sql), 1)
        args = sql[0]
        self.assertIn("--protocol=tcp", args)
        self.assertIn("--ssl-mode=VERIFY_IDENTITY", args)
        self.assertIn("--ssl-ca=" + str(self.ca), args)
        self.assertNotIn("--skip-ssl", args)
        self.assertNotIn("--ssl-mode=PREFERRED", args)
        self.assertNotIn("synthetic", args)

    def test_remote_mariadb_requires_server_certificate_verification(self):
        result, sql, _ = self.invoke({"SPY_HELP": "--ssl-verify-server-cert --ssl-ca"})
        self.assertEqual(result.returncode, 88)
        self.assertIn("--ssl-verify-server-cert", sql[0])
        self.assertIn("--ssl-ca=" + str(self.ca), sql[0])
        self.assertFalse(any(value.startswith("--ssl-mode") for value in sql[0]))

    def test_remote_missing_tls_ca_and_unsupported_clients_fail_closed(self):
        for changes, omit in (
            ({}, ("DB_TLS",)), ({"DB_TLS": "FALSE"}, ()),
            ({}, ("DB_SSL_CA",)), ({"DB_SSL_CA": str(self.directory)}, ()),
            ({"DB_SSL_CA": str(self.directory / "missing.pem")}, ()),
            ({"SPY_HELP": "--ssl"}, ()), ({"SPY_HELP": "--ssl-mode"}, ()),
            ({"SPY_HELP": "--ssl-verify-server-cert"}, ()), ({"SPY_HELP_EXIT": "1"}, ()),
        ):
            with self.subTest(changes=changes, omit=omit):
                self.assert_denied(changes, omit)

    def test_nonproduction_requires_explicit_environment_and_loopback(self):
        self.assert_denied(omit=("ENVIRONMENT",))
        self.assert_denied({"ENVIRONMENT": "staging"})
        self.assert_denied({"ENVIRONMENT": "test"})
        self.assert_denied({"ENVIRONMENT": "test", "DB_HOST": "127.0.0.1", "DB_NAME": "game_production"})
        for host in ("127.0.0.1", "localhost", "::1"):
            with self.subTest(host=host):
                result, sql, calls = self.invoke({"ENVIRONMENT": "TeSt", "DB_HOST": host}, ("DB_TLS", "DB_SSL_CA"))
                self.assertEqual(result.returncode, 88)
                self.assertEqual(len(calls), 1)
                self.assertIn("--protocol=tcp", sql[0])
                self.assertEqual(sql[0][sql[0].index("-h") + 1], host)

    def test_socket_is_absolute_and_only_local_nonproduction(self):
        result, sql, calls = self.invoke({"ENVIRONMENT": "test", "DB_HOST": "localhost", "DB_SOCKET": "/tmp/telemetry test.sock"})
        self.assertEqual(result.returncode, 88)
        self.assertEqual(len(calls), 1)
        self.assertIn("--protocol=socket", sql[0])
        self.assertIn("--socket=/tmp/telemetry test.sock", sql[0])
        self.assertNotIn("-h", sql[0])
        self.assert_denied({"ENVIRONMENT": "test", "DB_HOST": "localhost", "DB_SOCKET": "relative.sock"})
        self.assert_denied({"DB_SOCKET": "/tmp/test.sock"})
        self.assert_denied({"DB_HOST": "localhost", "DB_SOCKET": "/tmp/test.sock"})
        self.assert_denied({"ENVIRONMENT": "test", "DB_SOCKET": "/tmp/test.sock"})

    def test_production_loopback_matches_runner_local_tcp_branch(self):
        result, sql, calls = self.invoke({"DB_HOST": "127.0.0.1"}, ("DB_TLS", "DB_SSL_CA"))
        self.assertEqual(result.returncode, 88)
        self.assertEqual(len(calls), 1)
        self.assertIn("--protocol=tcp", sql[0])


if __name__ == "__main__":
    unittest.main()
