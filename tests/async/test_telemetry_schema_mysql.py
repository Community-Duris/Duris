#!/usr/bin/env python3
"""Run against a caller-provisioned disposable database containing migration 0014.

Example: TELEMETRY_SCHEMA_TEST=1 DB_HOST=127.0.0.1 DB_PORT=3306 DB_USER=root
DB_NAME=duris_telemetry_schema_test python3 tests/async/test_telemetry_schema_mysql.py
Only explicitly named *_test schemas and loopback endpoints are accepted. Existing
credentials are supplied by the caller; this harness never creates users/grants.
"""
import os
import re
import subprocess
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(os.environ.get("TELEMETRY_SCHEMA_TEST") == "1", "requires disposable SQL test opt-in")
class TelemetrySchemaMysqlTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        host = os.environ.get("DB_HOST", "")
        name = os.environ.get("DB_NAME", "")
        if host not in ("127.0.0.1", "localhost", "::1") or not re.fullmatch(r"[a-z][a-z0-9_]*_test", name):
            raise RuntimeError("requires explicit loopback disposable *_test schema")
        cls.connection = ["mysql", "-h" + host, "-P" + os.environ.get("DB_PORT", "3306"),
                          "-u" + os.environ["DB_USER"], "-N", "-B", name]
        cls.environment = dict(os.environ)
        cls.environment["ENVIRONMENT"] = "test"
        if "DB_PASSWD" in os.environ:
            cls.environment["MYSQL_PWD"] = os.environ["DB_PASSWD"]
        cls.fact = ("INSERT INTO telemetry_interval "
                    "(boot_id,process_id,record_seq,schema_version,record_kind,occurrence_utc_usec,ingested_utc_usec) "
                    "VALUES (18446744073709551614,18446744073709551614,1,1,4,0,0);")
        cls.session = ("INSERT INTO telemetry_session "
                       "(environment_id,season_id,session_boot_id,session_process_id,session_seq,subject_id,pid) "
                       "VALUES (1,1,18446744073709551614,18446744073709551614,1,1,1);")

    def sql(self, statement, expected=0):
        result = subprocess.run(self.connection, input=statement, text=True,
                                env=self.environment, capture_output=True)
        if expected == 0:
            self.assertEqual(result.returncode, 0, result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(str(expected), result.stderr)
        return result.stdout.strip()

    def verifier(self, expected=0):
        # The existing verifier requires nonempty DB_PASSWD. For a caller's
        # passwordless disposable server adapt environment plumbing in memory;
        # every SQL shape/index check remains unchanged.
        source = (ROOT / "migrations/immutable/0014_telemetry_storage.sh").read_text()
        environment = dict(self.environment)
        if not environment.get("DB_PASSWD"):
            environment["DB_PASSWD"] = "unused-test-placeholder"
            source = source.replace('export MYSQL_PWD="$DB_PASSWD"', 'unset MYSQL_PWD')
        result = subprocess.run(["bash"], input=source, text=True,
                                env=environment, capture_output=True)
        self.assertEqual(result.returncode == 0, expected == 0, result.stdout + result.stderr)

    def test_apply_replay_and_verified_shape(self):
        sql = (ROOT / "migrations/immutable/0014_telemetry_storage.sql").read_text()
        self.sql(sql)
        self.sql(sql)
        self.verifier()

    def test_global_replay_and_session_identity_unique(self):
        self.sql("START TRANSACTION;" + self.fact + self.fact + "ROLLBACK;", 1062)
        changed_scope = self.session.replace("VALUES (1,1,", "VALUES (2,1,")
        self.sql("START TRANSACTION;" + self.session + changed_scope + "ROLLBACK;", 1062)

    def test_unsigned_endpoints_signed_utc_and_nonunique_checkpoint_revision(self):
        insert = self.fact.replace("record_kind,occurrence", "record_kind,checkpoint_revision,duration_usec,occurrence")
        insert = insert.replace(",1,1,4,0,0);", ",1,1,3,18446744073709551615,18446744073709551615,-9223372036854775808,0);")
        second = insert.replace(",1,1,3,18446744073709551615", ",2,1,3,18446744073709551615")
        output = self.sql("START TRANSACTION;" + insert + second +
                          "SELECT COUNT(*),MIN(duration_usec),MIN(occurrence_utc_usec) FROM telemetry_interval "
                          "WHERE boot_id=18446744073709551614 AND process_id=18446744073709551614;ROLLBACK;")
        self.assertEqual(output, "2\t18446744073709551615\t-9223372036854775808")

    def test_verifier_rejects_unsigned_type_and_unique_index_damage(self):
        changes = (
            ("ALTER TABLE telemetry_interval MODIFY duration_usec BIGINT NULL;",
             "ALTER TABLE telemetry_interval MODIFY duration_usec BIGINT UNSIGNED NULL;"),
            ("ALTER TABLE telemetry_interval DROP INDEX uq_telemetry_replay;",
             "ALTER TABLE telemetry_interval ADD UNIQUE KEY uq_telemetry_replay (boot_id,process_id,record_seq);"),
            ("ALTER TABLE telemetry_session DROP INDEX uq_telemetry_session_identity;",
             "ALTER TABLE telemetry_session ADD UNIQUE KEY uq_telemetry_session_identity (session_boot_id,session_process_id,session_seq);"),
        )
        for damage, restore in changes:
            with self.subTest(damage=damage):
                self.sql(damage)
                try:
                    self.verifier(expected=1)
                finally:
                    self.sql(restore)
                self.verifier()


if __name__ == "__main__":
    unittest.main()
