#!/usr/bin/env python3
"""Source contracts for staged activation; executable refusal lives in test_economic_sql_lifecycle_no_mysql."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


class TestEconomicSqlLifecycleActivationContract(unittest.TestCase):

    def test_header_declarations(self):
        header = (ROOT / "src/persistence/economic_sql_accounting_lifecycle_transaction.h").read_text(encoding="utf-8")
        self.assertIn("static unsigned int activate(", header)
        self.assertIn("static unsigned int activate_verified(", header)
        self.assertIn("static unsigned int pause(", header)
        self.assertIn("static unsigned int recover_runtime(", header)
        self.assertIn("economic_sql_cutover_transaction_owner &owner", header)
        self.assertIn("const critical_operation_id &lineage", header)
        self.assertIn("uint64_t *new_lineage_revision = nullptr", header)

    def test_guard_friendship(self):
        guard_header = (ROOT / "src/persistence/economic_sql_lifecycle_guard.h").read_text(encoding="utf-8")
        cutover_match = re.search(
            r"class economic_sql_cutover_transaction_owner.*?friend class economic_sql_accounting_lifecycle_transaction;",
            guard_header,
            re.DOTALL,
        )
        self.assertIsNotNone(cutover_match, "economic_sql_cutover_transaction_owner must grant friendship to economic_sql_accounting_lifecycle_transaction")

    def test_implementation_safety_guards(self):
        impl = (ROOT / "src/persistence/economic_sql_accounting_lifecycle_transaction.c").read_text(encoding="utf-8")
        self.assertIn("economic_sql_accounting_lifecycle_transaction::activate", impl)
        # Check preconditions
        self.assertIn("owner.is_valid()", impl)
        self.assertIn("owner.connection_ == connection", impl)
        self.assertIn("mysql_thread_id(connection) == owner.session_", impl)
        self.assertIn("stored.phase == 2", impl)
        self.assertIn("decision.exists && decision.state == 1", impl)
        self.assertIn("verify(connection, evidence, snapshot)", impl)
        self.assertIn("economic_sql_global_activation", impl)
        # Check transition statement
        self.assertIn("UPDATE economic_lineage_state SET active_epoch=", impl)
        self.assertIn("revision=revision+1", impl)
        self.assertIn("AND active_epoch IS NULL", impl)
        # Exact retry accepts only the already selected epoch.
        self.assertIn("parse_id(state[0]).bytes == stored.epoch.bytes", impl)

if __name__ == "__main__":
    unittest.main()
