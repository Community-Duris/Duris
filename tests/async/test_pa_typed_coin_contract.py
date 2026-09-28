#!/usr/bin/env python3
"""Cheap source-contract checks for the typed wallet coin root."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
ACCOUNTING = (ROOT / "src/economy/coin_transfer_accounting.c").read_text()
AUTHORITY = (ROOT / "src/economy/economic_gameplay_authority.c").read_text()
REPOSITORY = (ROOT / "src/persistence/critical_command_repository.c").read_text()
COMMAND = (ROOT / "src/economy/coin_transfer_command.c").read_text()
TRANSACTION = (ROOT / "src/economy/currency_transaction.c").read_text()
ACCOUNTING_FLAT = re.sub(r"\s+", " ", ACCOUNTING)


class TypedCoinRootContract(unittest.TestCase):
    def test_typed_root_is_wallet_only_and_uses_retained_lifetimes(self):
        self.assertIn("source.type == critical_command_type::account_bank", ACCOUNTING)
        self.assertIn("destination.type == critical_command_type::account_bank", ACCOUNTING)
        self.assertIn("economic_sql_lock_authority", ACCOUNTING)
        self.assertIn("PLAYER_LOCATOR", ACCOUNTING)
        self.assertIn("SELECT copper,silver,gold,platinum,wallet_revision FROM player_data", ACCOUNTING)
        self.assertIn(" FOR UPDATE", ACCOUNTING)
        self.assertIn(
            "result.bank_revision == endpoint.change.expected_revisions[1].revision + 1",
            ACCOUNTING_FLAT,
        )

    def test_no_lifetime_or_epoch_bootstrap_in_coin_writer(self):
        self.assertNotIn("START TRANSACTION", ACCOUNTING)
        self.assertNotIn("COMMIT", ACCOUNTING)
        self.assertNotIn("INSERT INTO economic_account_mapping", ACCOUNTING)
        self.assertNotIn("INSERT INTO economic_lineage_state", ACCOUNTING)
        self.assertNotIn("INSERT INTO economic_epoch", ACCOUNTING)
        self.assertIn("connection->server_status & SERVER_STATUS_IN_TRANS", ACCOUNTING)

    def test_balanced_plan_and_retained_exact_id_evidence(self):
        self.assertIn("economic_coin_effects_validate", ACCOUNTING)
        self.assertIn('"economic_accounting_coin_posting"', ACCOUNTING)
        self.assertIn('"economic_accounting_account_effect"', ACCOUNTING)
        self.assertIn("coin_transfer_accounting_verify_retained", REPOSITORY)
        self.assertIn("identity_matches(stored, command, command_hash, keys_hash)", REPOSITORY)
        self.assertIn("economic_sql_lock_authority", ACCOUNTING)

    def test_root_transaction_orders_native_and_accounting_effects_atomically(self):
        apply_body = REPOSITORY[REPOSITORY.index("critical_apply_result critical_command_repository_apply("):]
        self.assertLess(apply_body.index("START TRANSACTION"), apply_body.index("coin_transfer_accounting_lock"))
        self.assertLess(apply_body.index("coin_transfer_accounting_lock"), apply_body.index("if (coin_command)"))
        self.assertLess(apply_body.index("if (coin_command)"), apply_body.index("coin_transfer_accounting_record"))
        self.assertLess(apply_body.index("coin_transfer_accounting_record"), apply_body.index('"COMMIT"'))
        self.assertIn("ROLLBACK TO SAVEPOINT coin_endpoints", apply_body)
        self.assertIn("rollback(connection);", apply_body)

    def test_inactive_legacy_and_active_pile_refusal(self):
        self.assertIn("if (!selected)\n\t\t\treturn error::ok;", AUTHORITY)
        self.assertIn("return error::incomplete_coverage;", AUTHORITY)
        self.assertIn("separate authenticated custody/account adapter", ACCOUNTING)
        self.assertIn("COIN_TRANSFER_OPERATION_DOMAIN", COMMAND)

    def test_typed_coin_publication_and_restart_replay_are_retained(self):
        self.assertIn("command.type == critical_command_type::coin_transfer", TRANSACTION)
        self.assertIn("coin_transfer_command_decode_payload(command, &coin)", TRANSACTION)
        self.assertIn(".coin = coin,", TRANSACTION)
        self.assertIn(
            "critical_command_coordinator_acknowledge_publication(completed.operation_id)",
            TRANSACTION,
        )


if __name__ == "__main__":
    unittest.main()
