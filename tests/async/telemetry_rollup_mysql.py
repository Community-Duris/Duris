#!/usr/bin/env python3
"""#268 real disposable MariaDB fixture/grant/scan-plan verification.

Only the task's explicit loopback *_test database is accepted. Administrative
setup and fixture cleanup belong to this test, never the rollup job.
"""
import json
import os
import re
import unittest

import pymysql

from telemetry_rollup_fixtures import ENUMS, FIXTURE_DIR, golden_rows


class RollupDatabaseFoundation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if os.environ.get("TELEMETRY_ROLLUP_TEST") != "1":
            raise RuntimeError("explicit TELEMETRY_ROLLUP_TEST=1 required")
        database = os.environ["ROLLUP_TEST_DATABASE"]
        if not re.fullmatch(r"duris_268_[a-z0-9_]*test", database):
            raise RuntimeError("requires task-owned duris_268_*test database")
        cls.database = database
        cls.connections = {}
        for role, user in (("root", "root"), ("rollup", "duris268_rollup"),
                           ("report", "duris268_report")):
            cls.connections[role] = pymysql.connect(
                host="127.0.0.1", port=3306, user=user, database=database,
                password=os.environ[f"ROLLUP_TEST_{role.upper()}_PASSWORD"],
                charset="utf8mb4", connect_timeout=2, read_timeout=2, write_timeout=2,
                autocommit=True, cursorclass=pymysql.cursors.DictCursor)
        root = cls.connections["root"]
        with root.cursor() as cur:
            # These tables and all rows were created exclusively for this task.
            cur.execute("TRUNCATE TABLE telemetry_interval")
            cur.execute("TRUNCATE TABLE telemetry_config")
            configs = {}
            rows = []
            for path in sorted(FIXTURE_DIR.glob("*.json")):
                fixture, facts = golden_rows(path)
                for config in fixture.get("configurations", []):
                    key = (config["environment_id"], config["config_id"])
                    if key in configs:
                        assert configs[key] == config
                    configs[key] = config
                for row in facts:
                    row["ingest_id"] = len(rows) * 3 + 1  # legitimate ID holes, not loss evidence
                    rows.append(row)
            for row in rows:
                columns = tuple(row)
                cur.execute("INSERT INTO telemetry_interval (" + ",".join(columns) +
                            ") VALUES (" + ",".join(["%s"] * len(columns)) + ")",
                            tuple(row[column] for column in columns))
            for original in configs.values():
                config = dict(original)
                config.pop("reserved", None)
                config["backend"] = ENUMS["backend"][config["backend"]]
                config["fingerprint"] = bytes.fromhex(config["fingerprint"])
                columns = tuple(config)
                cur.execute("INSERT INTO telemetry_config (" + ",".join(columns) +
                            ") VALUES (" + ",".join(["%s"] * len(columns)) + ")",
                            tuple(config[column] for column in columns))
            cls.expected_count = len(rows)
            assert cls.expected_count == 36

    @classmethod
    def tearDownClass(cls):
        for connection in cls.connections.values():
            connection.close()

    def test_golden_named_fields_survive_real_sql(self):
        with self.connections["rollup"].cursor() as cur:
            cur.execute("SELECT COUNT(*) AS n, COUNT(DISTINCT record_kind) AS kinds FROM telemetry_interval")
            self.assertEqual(cur.fetchone(), {"n": self.expected_count, "kinds": 5})

    def test_keyset_primary_range_plan_and_pagination(self):
        connection = self.connections["rollup"]
        with connection.cursor() as cur:
            statement = ("SELECT ingest_id,record_kind,duration_usec FROM telemetry_interval FORCE INDEX(PRIMARY) "
                         "WHERE ingest_id>%s AND ingest_id<=%s ORDER BY ingest_id LIMIT 5")
            cur.execute("EXPLAIN " + statement, (1, 500))
            plan = cur.fetchone()
            self.assertEqual(plan["key"], "PRIMARY")
            self.assertEqual(plan["type"], "range")
            ids = []
            cursor = 0
            for _ in range(20):
                cur.execute(statement, (cursor, 500))
                page = cur.fetchall()
                if not page:
                    break
                self.assertLessEqual(len(page), 5)
                ids.extend(row["ingest_id"] for row in page)
                cursor = ids[-1]
            self.assertEqual(len(ids), self.expected_count)
            self.assertEqual(len(set(ids)), len(ids))
            self.assertEqual(ids, sorted(ids))
            print("ISSUE268_REAL_PRIMARY_KEYSET_PLAN_OK", json.dumps(plan, default=str))

    def test_rollup_role_cannot_mutate_raw_or_schema(self):
        with self.connections["rollup"].cursor() as cur:
            for sql in ("UPDATE telemetry_interval SET record_seq=record_seq WHERE 0",
                        "DELETE FROM telemetry_interval WHERE 0",
                        "CREATE TABLE forbidden_rollup_ddl (id INT)"):
                with self.subTest(sql=sql), self.assertRaises(pymysql.err.OperationalError) as error:
                    cur.execute(sql)
                self.assertIn(error.exception.args[0], (1044, 1142))

    def test_report_role_is_aggregate_read_only(self):
        with self.connections["report"].cursor() as cur:
            cur.execute("SELECT input_watermark FROM telemetry_rollup_state LIMIT 1")
            for sql in ("SELECT ingest_id FROM telemetry_interval LIMIT 1",
                        "UPDATE telemetry_rollup_state SET input_watermark=0 WHERE 0"):
                with self.subTest(sql=sql), self.assertRaises(pymysql.err.OperationalError) as error:
                    cur.execute(sql)
                self.assertEqual(error.exception.args[0], 1142)


if __name__ == "__main__":
    unittest.main(verbosity=2)
