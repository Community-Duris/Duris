#!/usr/bin/env python3
"""Source contract for read-only activation-receipt readback (no SQL execution)."""
from pathlib import Path
import ast
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/persistence/economic_sql_activation_receipt.h").read_text()
SOURCE = (ROOT / "src/persistence/economic_sql_activation_receipt.c").read_text()
CPP_TEST = (ROOT / "tests/async/economic_sql_activation_receipt_readback.cpp").read_text()


class ActivationReceiptReadbackContract(unittest.TestCase):
    def test_receipt_contract_is_versioned_and_data_only(self):
        for token in (
            "ECONOMIC_SQL_ACTIVATION_SCOPE_QUALIFICATION_WALLET_ROOT_V1 = 1",
            "ECONOMIC_SQL_ACTIVATION_COVERAGE_CONTRACT_VERSION_V1 = 1",
            "ECONOMIC_SQL_ACTIVATION_RECEIPT_VERSION_V1 = 1",
            "source_capture_digest",
            "native_boundary_digest",
            "coverage_evidence_digest",
            "activation_digest",
            "created_at",
            "economic_sql_activation_receipt_digest",
            "economic_sql_activation_receipt_validate",
        ):
            self.assertIn(token, HEADER)
        self.assertIn(
            "not an activation, gameplay, or coverage capability",
            HEADER.replace("\n// ", " "),
        )
        self.assertNotIn("economic_gameplay_authority", HEADER + SOURCE)
        self.assertNotIn("set_active_epoch", HEADER + SOURCE)

    def test_canonical_digest_binds_every_semantic_field(self):
        digest_start = SOURCE.index("unsigned int economic_sql_activation_receipt_digest(")
        digest_end = SOURCE.index(
            "unsigned int economic_sql_activation_receipt_validate_readback_row(",
            digest_start,
        )
        digest_body = SOURCE[digest_start:digest_end]
        for token in (
            "DURIS-SQL-ACTIVATION-RECEIPT-V1",
            "receipt.operation_id.bytes",
            "receipt.lineage.bytes",
            "receipt.epoch.bytes",
            "receipt.baseline_operation_id.bytes",
            "receipt.baseline_revision",
            "receipt.source_capture_digest",
            "receipt.native_boundary_digest",
            "receipt.activation_scope",
            "receipt.coverage_contract_version",
            "receipt.coverage_evidence_digest",
            "receipt.receipt_version",
        ):
            self.assertIn(token, digest_body)
        self.assertNotIn("receipt.created_at", digest_body)

    def test_readback_is_one_read_only_consistent_join(self):
        projection = SOURCE.split("constexpr const char *readback_projection =", 1)[1]
        projection = projection.split("economic_sql_activation_receipt_readback_row parse_record", 1)[0]
        for table in (
            "economic_sql_activation_receipt",
            "economic_sql_lifecycle_installation",
            "economic_epoch",
            "economic_lineage_state",
            "economic_baseline_control",
            "economic_baseline_witness",
            "critical_operation_inbox",
        ):
            self.assertIn(table, projection)
        self.assertGreaterEqual(projection.count("LEFT JOIN"), 8)
        self.assertIn("WHERE r.lineage=", projection)
        self.assertIn("fields.size() == 76", SOURCE)
        self.assertIn('i.wallet_count,i.bank_count,HEX(c.opening_account)', projection)
        self.assertIn("row.installation_wallet_count = integer<uint64_t>(fields, 72)", SOURCE)
        self.assertIn("row.installation_bank_count = integer<uint64_t>(fields, 73)", SOURCE)
        self.assertIn("row.baseline_control_opening_account = parse_account_key(fields, 74)", SOURCE)
        self.assertIn("receipt.created_at = text(fields, 75)", SOURCE)
        readback = SOURCE.split(
            "unsigned int economic_sql_activation_receipt_readback(\n", 1
        )[1]
        self.assertIn("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", readback)
        self.assertIn("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", readback)
        self.assertIn("query(connection, std::string(readback_projection) + binary(lineage), 76)", readback)
        self.assertIn("transaction.rollback()", readback)
        self.assertIn("authority.connection_ == connection", readback)
        self.assertIn("authority.session_ == mysql_thread_id(connection)", readback)
        self.assertIn("authority.is_maintenance_authority()", readback)
        self.assertIn("owns_maintenance_session(connection, session)", readback)
        self.assertIn("SERVER_STATUS_AUTOCOMMIT", SOURCE)
        self.assertIn("session as unusable", HEADER)
        self.assertNotRegex(projection.upper(), r"\b(INSERT|UPDATE|DELETE|REPLACE)\b")
        self.assertNotRegex(readback.upper(), r"\b(INSERT|UPDATE|DELETE|REPLACE)\b")

    def test_fail_closed_row_relations_and_regression_cases_exist(self):
        validator = SOURCE.split(
            "unsigned int economic_sql_activation_receipt_validate_readback_row(", 1
        )[1].split("unsigned int economic_sql_activation_receipt_readback(", 1)[0]
        for token in (
            "installation_phase == 2",
            "installation_revision == 1",
            "installation_source_capture_digest == receipt.source_capture_digest",
            "installation_native_boundary_digest == receipt.native_boundary_digest",
            "derived_baseline",
            "active_epoch",
            "baseline_control_revision == receipt.baseline_revision",
            "baseline_witness_revision == receipt.baseline_revision",
            "baseline_operation_outcome == 1",
            "baseline_inbox_durable_revision == receipt.baseline_revision",
            "economic_baseline_decode",
            "witness.preparation_id",
            "witness.boundary_digest == receipt.native_boundary_digest",
            "installation_wallet_count",
            "baseline_control_opening_account",
            "ECONOMIC_BASELINE_MAX_HOLDINGS",
        ):
            self.assertIn(token, validator)
        for token in (
            "malformed",
            "tampered",
            "orphan",
            "stale",
            "mismatched",
            "consistent_row",
            "economic_baseline_prepare",
            "economic_baseline_encode",
            "malformed_witness",
            "count_mismatch",
            "wrong_opening",
            "same_receipt",
            "before_failed_readback",
        ):
            self.assertIn(token, CPP_TEST)

    def test_readback_publication_and_cleanup_are_nonthrowing_or_explicit(self):
        self.assertIn("auto parsed = parse_record(rows.front())", SOURCE)
        self.assertIn("std::is_nothrow_move_assignable_v<economic_sql_activation_receipt>", SOURCE)
        self.assertIn("*output = std::move(parsed.receipt)", SOURCE)
        self.assertIn("unsigned int rollback() noexcept", SOURCE)
        self.assertIn("mysql_thread_id(connection) != session", SOURCE)
        self.assertIn("SERVER_STATUS_IN_TRANS", SOURCE)
        self.assertIn("authority.connection_ == connection", SOURCE)

    def test_this_contract_script_is_valid_python(self):
        ast.parse(Path(__file__).read_text())


if __name__ == "__main__":
    unittest.main(verbosity=2)
