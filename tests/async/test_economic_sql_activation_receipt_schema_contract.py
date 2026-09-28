#!/usr/bin/env python3
"""Static source contract for the SQL activation-receipt schema prerequisite."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
MIGRATION = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sql"
MIRROR = ROOT / "migrations/economic_sql_activation_receipt.sql"
BOOTSTRAP = ROOT / "migrations/bootstrap_multithread_safe.sql"
VERIFIER = ROOT / "migrations/immutable/0036_economic_sql_activation_receipt.sh"


class EconomicSqlActivationReceiptSchemaContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.migration = MIGRATION.read_text()
        cls.bootstrap = BOOTSTRAP.read_text()
        cls.verifier = VERIFIER.read_text()

    def test_nonimmutable_mirror_matches_immutable_apply_source(self):
        self.assertEqual(MIGRATION.read_bytes(), MIRROR.read_bytes())

    def test_fixed_v1_receipt_columns_and_scope(self):
        expected = {
            "operation_id BINARY(16) NOT NULL",
            "lineage BINARY(16) NOT NULL",
            "epoch BINARY(16) NOT NULL",
            "baseline_operation_id BINARY(16) NOT NULL",
            "baseline_revision BIGINT UNSIGNED NOT NULL",
            "source_capture_digest BINARY(32) NOT NULL",
            "native_boundary_digest BINARY(32) NOT NULL",
            "activation_scope TINYINT UNSIGNED NOT NULL",
            "coverage_contract_version SMALLINT UNSIGNED NOT NULL",
            "coverage_evidence_digest BINARY(32) NOT NULL",
            "activation_digest BINARY(32) NOT NULL",
            "receipt_version SMALLINT UNSIGNED NOT NULL",
            "created_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)",
            "CHECK (activation_scope = 1)",
            "CHECK (coverage_contract_version = 1)",
            "CHECK (receipt_version = 1)",
        }
        for declaration in expected:
            with self.subTest(declaration=declaration):
                self.assertIn(declaration, self.migration)
                self.assertIn(declaration, self.bootstrap)
        self.assertIn("qualification_sql_wallet_root_v1", self.migration)
        self.assertIn("no global scope is supported", self.migration)

    def test_exact_installation_binding_and_composite_parent_key(self):
        normalized = re.sub(r"\s+", " ", self.migration)
        self.assertIn(
            "ADD UNIQUE KEY uq_economic_sql_lifecycle_activation_binding "
            "(operation_id, lineage, epoch, baseline_operation_id)",
            normalized,
        )
        self.assertIn(
            "FOREIGN KEY (operation_id, lineage, epoch, baseline_operation_id) "
            "REFERENCES economic_sql_lifecycle_installation "
            "(operation_id, lineage, epoch, baseline_operation_id)",
            normalized,
        )
        self.assertIn(
            "FOREIGN KEY (lineage, epoch) REFERENCES economic_epoch (lineage, epoch)",
            normalized,
        )
        self.assertIn(
            "FOREIGN KEY (lineage, epoch, baseline_revision) "
            "REFERENCES economic_baseline_witness (lineage, epoch, book_revision)",
            normalized,
        )
        self.assertIn(
            "FOREIGN KEY (operation_id) REFERENCES critical_operation_inbox (operation_id)",
            normalized,
        )
        self.assertIn(
            "FOREIGN KEY (baseline_operation_id) "
            "REFERENCES critical_operation_inbox (operation_id)",
            normalized,
        )

    def test_bootstrap_defers_only_the_immutable_0033_conflicting_edge(self):
        self.assertIn("CREATE TABLE IF NOT EXISTS economic_sql_activation_receipt", self.bootstrap)
        self.assertIn("fk_economic_sql_activation_epoch", self.bootstrap)
        self.assertIn("fk_economic_sql_activation_baseline_revision", self.bootstrap)
        self.assertIn("fk_economic_sql_activation_operation_inbox", self.bootstrap)
        self.assertIn("fk_economic_sql_activation_baseline_inbox", self.bootstrap)
        self.assertNotIn("uq_economic_sql_lifecycle_activation_binding", self.bootstrap)
        self.assertNotIn("fk_economic_sql_activation_installation", self.bootstrap)
        self.assertIn("deferred to immutable\n-- migration 0036", self.bootstrap)

    def test_migration_is_additive_and_never_seeds_runtime_state(self):
        lower = self.migration.lower()
        self.assertIn("create table if not exists economic_sql_activation_receipt", lower)
        self.assertIn("add unique key uq_economic_sql_lifecycle_activation_binding", lower)
        for forbidden in (
            "insert into economic_sql_activation_receipt",
            "update economic_lineage_state",
            "update economic_baseline_control",
            "update economic_sql_lifecycle_installation",
            "delete from economic_sql_activation_receipt",
            "truncate table economic_sql_activation_receipt",
            "drop table economic_sql_activation_receipt",
        ):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, lower)
        self.assertIn("does not seed or", lower)
        self.assertIn("active_epoch pointer", lower)

    def test_verifier_matches_measured_engine_metadata_exactly(self):
        for required_shape in (
            'if [[ "$engine" == mariadb ]]; then',
            "bigint_column_type='bigint(20) unsigned'",
            "tinyint_column_type='tinyint(3) unsigned'",
            "smallint_column_type='smallint(5) unsigned'",
            "bigint_column_type='bigint unsigned'",
            "tinyint_column_type='tinyint unsigned'",
            "smallint_column_type='smallint unsigned'",
            "created_at_extra=''",
            "created_at_extra='DEFAULT_GENERATED'",
            "column_type='${bigint_column_type}'",
            "column_type='${tinyint_column_type}'",
            "column_type='${smallint_column_type}'",
            "LOWER(COALESCE(column_default,''))='current_timestamp(6)' AND extra='${created_at_extra}'",
            "(index_name='fk_economic_sql_lifecycle_baseline' AND non_unique=1 AND seq_in_index=1 AND column_name='baseline_operation_id')",
            "expect_pair lifecycle-index-inventory \"$lifecycle_indexes\" $'9\\t9\\t9'",
            "operation_id<>0x00000000000000000000000000000000",
        ):
            with self.subTest(required_shape=required_shape):
                self.assertIn(required_shape, self.verifier)
        self.assertNotIn("LIKE 'current_timestamp%'", self.verifier)

    def test_verifier_check_normalization_does_not_invoke_shell(self):
        # A backtick inside the double-quoted SQL becomes Bash substitution,
        # even when SQL single quotes surround it. bash -n does not execute it.
        self.assertEqual(self.verifier.count("REPLACE(c.check_clause,CHAR(96),'')"), 8)
        self.assertNotIn("REPLACE(c.check_clause,'`','')", self.verifier)

    def test_verifier_checks_exact_metadata_read_only(self):
        for metadata_view in (
            "information_schema.tables",
            "information_schema.columns",
            "information_schema.statistics",
            "information_schema.key_column_usage",
            "information_schema.referential_constraints",
            "information_schema.table_constraints",
            "information_schema.check_constraints",
        ):
            with self.subTest(metadata_view=metadata_view):
                self.assertIn(metadata_view, self.verifier)
        for required_shape in (
            "table-engine-collation",
            "receipt-indexes",
            "lifecycle-composite-index",
            "lifecycle-index-inventory",
            "foreign-keys",
            "check-constraints",
            "baseline_revision>0",
        ):
            with self.subTest(required_shape=required_shape):
                self.assertIn(required_shape, self.verifier)
        lowered = self.verifier.lower()
        for forbidden in ("insert into", "update ", "delete from", "alter table", "drop table"):
            with self.subTest(forbidden=forbidden):
                self.assertNotIn(forbidden, lowered)


if __name__ == "__main__":
    unittest.main()
