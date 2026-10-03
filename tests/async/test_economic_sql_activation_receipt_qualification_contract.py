#!/usr/bin/env python3
"""Dependency-free source contract for the manual C05 qualification harness."""

from pathlib import Path
import ast
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
HARNESS = ROOT / "tests/async/economic_sql_activation_receipt_qualification.py"
FIXTURE = ROOT / "tests/async/economic_sql_activation_receipt_upgrade_fixture.cpp"
MIGRATION = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sql"
LIFECYCLE_OWNER = ROOT / "src/persistence/economic_sql_accounting_lifecycle_transaction.c"
BASELINE_OWNER = ROOT / "src/persistence/economic_sql_baseline_transaction.c"

EXPECTED_SNAPSHOT_TABLES = (
    "accounts",
    "account_banks",
    "player_data",
    "critical_operation_inbox",
    "economic_lineage_state",
    "economic_epoch",
    "economic_account_mapping",
    "economic_sql_lifecycle_installation",
    "economic_baseline_control",
    "economic_baseline_witness",
    "economic_baseline_reservation",
    "economic_accounting_operation",
    "economic_accounting_account_effect",
    "economic_accounting_coin_posting",
    "economic_accounting_source_claim",
)

EXPECTED_FKS = {
    "fk_economic_sql_activation_installation",
    "fk_economic_sql_activation_epoch",
    "fk_economic_sql_activation_baseline_revision",
    "fk_economic_sql_activation_operation_inbox",
    "fk_economic_sql_activation_baseline_inbox",
}
EXPECTED_CHECKS = {
    "ck_economic_sql_activation_operation_nonzero",
    "ck_economic_sql_activation_lineage_nonzero",
    "ck_economic_sql_activation_epoch_nonzero",
    "ck_economic_sql_activation_baseline_operation_nonzero",
    "ck_economic_sql_activation_baseline_revision",
    "ck_economic_sql_activation_scope",
    "ck_economic_sql_activation_coverage_version",
    "ck_economic_sql_activation_receipt_version",
}


class EconomicSqlActivationReceiptQualificationContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.harness = HARNESS.read_text()
        cls.fixture = FIXTURE.read_text()
        cls.migration = MIGRATION.read_text()
        cls.lifecycle_owner = LIFECYCLE_OWNER.read_text()
        cls.baseline_owner = BASELINE_OWNER.read_text()

    def test_db_qualification_is_manual_only_and_dependency_free(self):
        self.assertFalse(HARNESS.name.startswith("test_"))
        ast.parse(self.harness, filename=str(HARNESS))
        self.assertIn('choices=("populated", "constraints", "all")', self.harness)
        self.assertIn('"--staged-fixture-ready"', self.harness)
        self.assertIn('if __name__ == "__main__":', self.harness)
        for dependency in ("mysql.connector", "pymysql", "sqlalchemy"):
            with self.subTest(dependency=dependency):
                self.assertNotIn(dependency, self.harness)

    def test_connection_is_explicit_local_disposable_bounded_and_redacted(self):
        for requirement in (
            'os.environ.get("ENVIRONMENT") != "test"',
            'os.environ.get(DISPOSABLE_MARKER) != "1"',
            '"DB_SOCKET" in os.environ',
            're.fullmatch(r"economic_receipt_test_[A-Za-z0-9_]{1,48}", database)',
            '{"127.0.0.1", "::1", "localhost"}',
            '"MYSQL_PWD": password',
            '"ECONOMIC_SQL_ACTIVATION_RECEIPT_TRANSPORT", "tcp"',
            '"-e", "MYSQL_PWD", "-e", "DB_HOST"',
            '"-e", "DB_PASSWD", "-e", "DB_NAME"',
            '"bash", "-s"',
            '"--no-defaults"',
            'timeout=20',
            'timeout=45',
            'timeout=90',
            'timeout=120',
            'def _safe_detail',
        ):
            with self.subTest(requirement=requirement):
                self.assertIn(requirement, self.harness)
        for forbidden in ("load_dotenv", "shell=True", "docker run", "Path('.env')", "Path(\".env\")"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, self.harness)
        self.assertNotIn("host.docker.internal", self.harness + self.fixture)

    def test_populated_upgrade_orders_0033_then_owner_fixture_then_0036_replay(self):
        start = self.harness.index("def run_populated(")
        end = self.harness.index("def receipt_insert_sql(", start)
        populated = self.harness[start:end]
        steps = (
            "verify_history_0035(target)",
            "target.run_script(HISTORICAL_VERIFIER",
            "target.run_fixture_helper()",
            "before = snapshot(target)",
            "target.sql_file(MIGRATION)",
            'target.run_script(RECEIPT_VERIFIER, "0036 canonical")',
            "after_first_apply = snapshot(target)",
            "target.sql_file(MIGRATION)",
            'target.run_script(RECEIPT_VERIFIER, "0036 direct replay")',
            "after_replay = snapshot(target)",
        )
        positions = []
        cursor = 0
        for step in steps:
            position = populated.find(step, cursor)
            self.assertGreaterEqual(position, 0, f"missing ordered step: {step}")
            positions.append(position)
            cursor = position + len(step)
        self.assertEqual(positions, sorted(positions))
        self.assertIn("before != after_first_apply", populated)
        self.assertIn("before != after_replay", populated)
        self.assertIn("assert_no_runtime_authority(target)", populated)

    def test_snapshot_covers_full_fixture_and_owner_rows_deterministically(self):
        tree = ast.parse(self.harness, filename=str(HARNESS))
        snapshot_tables = next(
            ast.literal_eval(node.value)
            for node in tree.body
            if isinstance(node, ast.Assign)
            and any(isinstance(target, ast.Name) and target.id == "SNAPSHOT_TABLES"
                    for target in node.targets)
        )
        self.assertEqual(snapshot_tables, EXPECTED_SNAPSHOT_TABLES)
        snapshot_node = next(
            node for node in tree.body
            if isinstance(node, ast.FunctionDef) and node.name == "snapshot"
        )
        snapshot_source = ast.get_source_segment(self.harness, snapshot_node) or ""
        for contract in (
            "information_schema.columns",
            "information_schema.statistics",
            "ORDER BY TABLE_NAME,ORDINAL_POSITION",
            "index_name='PRIMARY' ORDER BY TABLE_NAME,SEQ_IN_INDEX",
            "JSON_ARRAY(",
            "IF(`{column}` IS NULL,NULL,CONCAT('V',HEX(CAST(`{column}` AS BINARY))))",
            "ORDER BY {order_by}",
            "for column in columns",
        ):
            with self.subTest(snapshot_encoding_contract=contract):
                self.assertIn(contract, snapshot_source)

        fixture_writes = {
            "accounts": r"INSERT INTO accounts\(",
            "account_banks": r"INSERT INTO account_banks\(",
            "player_data": r"INSERT INTO player_data\(",
        }
        for table, pattern in fixture_writes.items():
            with self.subTest(fixture_table=table):
                self.assertRegex(self.fixture, pattern)
        owner_writes = {
            "critical_operation_inbox": (self.lifecycle_owner, r'INSERT INTO critical_operation_inbox\('),
            "economic_lineage_state": (self.lifecycle_owner, r'INSERT INTO economic_lineage_state\('),
            "economic_epoch": (self.lifecycle_owner, r'INSERT INTO economic_epoch\('),
            "economic_account_mapping": (self.lifecycle_owner, r'INSERT INTO economic_account_mapping\('),
            "economic_sql_lifecycle_installation": (
                self.lifecycle_owner, r'INSERT INTO economic_sql_lifecycle_installation\('
            ),
            "economic_baseline_control": (self.baseline_owner, r'insert\(connection, "economic_baseline_control"'),
            "economic_baseline_witness": (self.baseline_owner, r'insert\(connection, "economic_baseline_witness"'),
            "economic_baseline_reservation": (self.baseline_owner, r'rows\(connection, "economic_baseline_reservation"'),
            "economic_accounting_operation": (self.baseline_owner, r'insert\(connection, "economic_accounting_operation"'),
            "economic_accounting_account_effect": (
                self.baseline_owner, r'rows\(connection, "economic_accounting_account_effect"'
            ),
            "economic_accounting_coin_posting": (
                self.baseline_owner, r'rows\(connection, "economic_accounting_coin_posting"'
            ),
            "economic_accounting_source_claim": (
                self.baseline_owner, r'insert\(connection, "economic_accounting_source_claim"'
            ),
        }
        self.assertEqual(set(fixture_writes) | set(owner_writes), set(snapshot_tables))
        for table, (source, pattern) in owner_writes.items():
            with self.subTest(owner_table=table):
                self.assertRegex(source, pattern)

    def test_receipt_fk_and_all_eight_check_refusals_are_pinned(self):
        migration_fks = set(re.findall(
            r"CONSTRAINT\s+(fk_economic_sql_activation_[a-z_]+)\s+FOREIGN KEY", self.migration
        ))
        migration_checks = set(re.findall(
            r"CONSTRAINT\s+(ck_economic_sql_activation_[a-z_]+)\s+CHECK", self.migration
        ))
        harness_fks = set(re.findall(
            r'"(fk_economic_sql_activation_[a-z_]+)"\s*:', self.harness
        ))
        harness_checks = set(re.findall(
            r'"(ck_economic_sql_activation_[a-z_]+)"', self.harness
        ))
        self.assertEqual(migration_fks, EXPECTED_FKS)
        self.assertEqual(harness_fks, EXPECTED_FKS)
        self.assertEqual(migration_checks, EXPECTED_CHECKS)
        self.assertEqual(harness_checks & EXPECTED_CHECKS, EXPECTED_CHECKS)
        self.assertIn("target.run_script(RECEIPT_VERIFIER, \"0036 pre-negative canonical\")", self.harness)
        self.assertIn("target.run_script(RECEIPT_VERIFIER, \"0036 post-negative canonical restoration\")", self.harness)
        self.assertIn("target.sql_file(MIGRATION)", self.harness)

    def test_owner_fixture_uses_real_lifecycle_transaction_and_never_activates(self):
        self.assertIn("economic_sql_lifecycle_guard::acquire_maintenance", self.fixture)
        self.assertIn("economic_sql_accounting_lifecycle_transaction::install", self.fixture)
        self.assertIn("economic_sql_lifecycle_installation", self.fixture)
        self.assertIn("economic_baseline_control", self.fixture)
        self.assertIn("economic_baseline_witness", self.fixture)
        self.assertIn("economic_baseline_reservation", self.fixture)
        self.assertIn("active_epoch IS NOT NULL", self.fixture)
        self.assertIn("active_epoch IS NOT NULL", self.harness)
        for source, name in ((self.fixture, "fixture"), (self.harness, "harness")):
            with self.subTest(source=name):
                self.assertIsNone(re.search(r"(?i)\bUPDATE\s+economic_lineage_state\b", source))
                self.assertIsNone(re.search(r"(?i)\bSET\s+foreign_key_checks\s*=\s*0", source))
        self.assertNotIn("INSERT INTO economic_sql_activation_receipt", self.fixture)

    def test_constraint_synthetic_rows_are_disposable_and_never_retained(self):
        self.assertIn("DISPOSABLE_MARKER", self.harness)
        self.assertIn("expect_constraint_rejection", self.harness)
        self.assertIn("DELETE FROM economic_sql_activation_receipt WHERE operation_id=", self.harness)
        self.assertIn("receipt table empty after probes", self.harness)
        self.assertIn("active epoch remains NULL", self.harness)
        self.assertIn(
            "SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL",
            self.harness,
        )
        self.assertNotIn("SET FOREIGN_KEY_CHECKS=0", self.harness.upper())
        self.assertNotIn("UPDATE economic_lineage_state SET active_epoch", self.harness)

    def test_schema_verifier_negatives_restore_then_recheck_canonical_shape(self):
        self.assertIn("c05_receipt_qualification_probe", self.harness)
        self.assertIn('expect_rejection="receipt-indexes"', self.harness)
        self.assertIn('expect_rejection="foreign-keys"', self.harness)
        self.assertIn("0036 after extra-index restoration", self.harness)
        self.assertIn("0036 after FK replay restoration", self.harness)
        self.assertIn("docker exec", self.harness)
        self.assertIn("docker transport requires --staged-fixture-ready", self.harness)
        self.assertIn("do not prove target", self.harness)
        self.assertIn("SQL transport", self.harness)
        self.assertIn("exact PASS output", self.harness)
        self.assertNotIn("docker rm", self.harness)
        self.assertNotIn("docker run", self.harness)
        self.assertIn("SQL/verifier credentials are forwarded", self.harness)


if __name__ == "__main__":
    unittest.main()
