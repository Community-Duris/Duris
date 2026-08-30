#!/usr/bin/env python3
"""Source and schema contracts for transactional player/account currency."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


class CurrencyTransactionContractTests(unittest.TestCase):
    def test_schema_baselines_and_operator_tools_are_wired(self):
        migration = (ROOT / "migrations/currency_ledger.sql").read_text()
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        runner = (ROOT / "migrations/run_migration.sh").read_text()
        for token in (
            "wallet_revision",
            "bank_revision",
            "currency_wallet_baseline",
            "currency_bank_baseline",
            "currency_ledger",
            "uq_currency_wallet_revision",
            "uq_currency_bank_revision",
            "currency_ledger_operation_fk",
        ):
            self.assertIn(token, migration)
            self.assertIn(token, bootstrap)
        self.assertIn("currency_ledger.sql", runner)
        self.assertIn("verify_currency_ledger_schema.sh", runner)
        for script in (
            "baseline_currency_balances.sh",
            "reconcile_currency_balances.sh",
            "verify_currency_ledger_schema.sh",
        ):
            self.assertTrue((ROOT / "migrations" / script).stat().st_mode & 0o111)

    def test_command_is_fixed_typed_bounded_and_revisioned(self):
        header = (SRC / "currency_command.h").read_text()
        implementation = (SRC / "currency_command.c").read_text()
        self.assertIn("CURRENCY_COMMAND_PAYLOAD_BYTES = 136", header)
        self.assertIn("CURRENCY_RESULT_PAYLOAD_BYTES = 80", header)
        self.assertIn("std::array<int64_t, CURRENCY_DENOMINATION_COUNT>", header)
        self.assertIn("atm_deposit", header)
        self.assertIn("auction_pickup", header)
        self.assertIn("ship_insurance", header)
        self.assertIn("std::numeric_limits<int64_t>::min()", implementation)
        self.assertIn("command.expected_revisions.size() == 2", implementation)

    def test_repository_commits_both_states_ledger_result_and_outbox(self):
        repository = (SRC / "critical_command_repository.c").read_text()
        start = repository.index("bool execute_currency_state")
        apply = repository.index("critical_apply_result critical_command_repository_apply")
        state = repository[start:apply]
        for token in (
            "FOR UPDATE",
            "INSERT IGNORE INTO account_banks(account_name,racewar) VALUES(?,?)",
            "UPDATE player_data SET copper=?,silver=?,gold=?,platinum=?,wallet_revision=?",
            "UPDATE account_banks SET bank_copper=?,bank_silver=?,bank_gold=?,bank_platinum=?,",
            "INSERT INTO currency_ledger",
            "currency_wallet_baseline",
            "currency_bank_baseline",
        ):
            self.assertIn(token, state)
        bank_ensure = state.index(
            "INSERT IGNORE INTO account_banks(account_name,racewar) VALUES(?,?)"
        )
        bank_lock = state.index(
            "FROM account_banks WHERE account_name=? AND racewar=? FOR UPDATE"
        )
        self.assertLess(bank_ensure, bank_lock)
        branch = repository[apply:]
        currency = branch[branch.index("if (currency_command)") :]
        commit = currency.index('execute(connection, "COMMIT")')
        self.assertLess(currency.index("insert_outbox"), commit)
        self.assertLess(currency.index("finish_inbox"), commit)

    def test_atm_and_audited_producers_use_the_ack_boundary(self):
        atm = (SRC / "actoth.c").read_text()
        utility = (SRC / "utility.c").read_text()
        auction = (SRC / "auction_houses.c").read_text()
        boon = (SRC / "boon.c").read_text()
        ship = (SRC / "ships/ship_base.c").read_text()
        self.assertNotIn("deposit_carried_coin", atm)
        self.assertIn("currency_reason_type::atm_deposit", atm)
        self.assertIn("currency_reason_type::atm_withdraw", atm)
        deposit_all = atm[atm.index('if (strstr("all", argument))') : atm.index("half_chop", atm.index('if (strstr("all", argument))'))]
        self.assertEqual(1, deposit_all.count("currency_transaction_submit("))
        self.assertIn("currency_transaction_submit_bank_payment", utility)
        self.assertIn("currency_transaction_submit_wallet_value", utility)
        self.assertIn("currency_reason_type::auction_pickup", auction)
        self.assertIn("currency_reason_type::boon_reward", boon)
        self.assertIn("currency_reason_type::ship_insurance", ship)

    def test_positive_wallet_rewards_serialize_and_rebase(self):
        command = (SRC / "currency_command.c").read_text()
        transaction = (SRC / "currency_transaction.c").read_text()
        repository = (SRC / "critical_command_repository.c").read_text()
        helper = "currency_command_is_rebasable_wallet_reward"
        self.assertIn("payload.reason != currency_reason_type::wallet_reward", command)
        self.assertIn("payload.wallet_delta.amount[index] < 0", command)
        self.assertIn("payload.bank_delta.amount[index]", command)
        self.assertIn("!rebasable_reward &&", transaction)
        self.assertIn(helper, transaction)
        self.assertIn("!rebasable_reward &&", repository)
        self.assertIn(helper, repository)

    def test_checkpoint_handoff_captures_but_cannot_overwrite_currency(self):
        capture = (SRC / "player_snapshot_capture.c").read_text()
        replay = (SRC / "player_snapshot_repository.c").read_text()
        sql_player = (SRC / "sql_player.c").read_text()
        for field in ("copper", "silver", "gold", "platinum"):
            # A complete flat baseline needs the immutable opening value. Both SQL
            # replay and load materialization still treat the transaction domain as
            # authoritative after that handoff.
            self.assertIn(f"ADD_STATUS({field},", capture)
            self.assertIn(f"row.field == player_status_field::{field}", replay)
        self.assertIn("copper=copper, silver=silver, gold=gold, platinum=platinum", sql_player)
        self.assertIn("INSERT INTO currency_wallet_baseline", sql_player)
        self.assertIn("currency_bank_baseline", sql_player)

    def test_no_legacy_bank_delta_helper_remains_in_gameplay(self):
        call = re.compile(r"sql_account_bank_(?:deposit|withdraw)(?:_balances|_value)?\s*\(")
        violations = []
        for path in SRC.rglob("*.c"):
            if path.name == "sql_player.c":
                continue
            for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                if call.search(line):
                    violations.append(f"{path.relative_to(ROOT)}:{number}:{line.strip()}")
        self.assertEqual([], violations)

    def test_player_currency_mutations_are_centralized(self):
        assignment = re.compile(
            r"GET_(?:COPPER|SILVER|GOLD|PLATINUM)\([^)]*\)\s*(?:[+\-]=|=(?!=))"
        )
        allowed = {
            "currency_transaction.c",  # game-thread ACK publication
            "files.c",                 # format-compatible import parsing
            "nanny.c",                 # new-character initialization
            "sql_player.c",            # authoritative hydration
            "player_load_materialize.c",  # authoritative worker snapshot hydration
            "utility.c",               # NPC fallback in central adapters
            "mobconv.c",               # NPC construction
            "db.c",                    # NPC construction
            "copyover.c",              # NPC restoration
            "smagic.c",                # summoned NPC setup
            "necromancy.c",            # summoned NPC setup
            "random.mob.c",            # NPC construction
            "actnew.c",                # NPC construction
            "nexus_stones.c",           # NPC construction
            "guildhall_rooms.c",       # NPC construction
            "specs.mobile.c",           # NPC vendor/undead balances
        }
        violations = []
        for path in SRC.rglob("*.c"):
            if path.name in allowed:
                continue
            for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                if line.lstrip().startswith("//"):
                    continue
                if assignment.search(line):
                    violations.append(f"{path.relative_to(ROOT)}:{number}:{line.strip()}")
        self.assertEqual([], violations)

    def test_completion_publication_and_lifecycle_are_game_thread_owned(self):
        transaction = (SRC / "currency_transaction.c").read_text()
        utility = (SRC / "utility.c").read_text()
        comm = (SRC / "comm.c").read_text()
        nanny = (SRC / "nanny.c").read_text()
        self.assertIn("find_player_by_pid", transaction)
        self.assertIn("publish_account_bank_balances_revision", transaction)
        self.assertIn("for (P_desc desc = descriptor_list", utility)
        self.assertIn("currency_transaction_handle_completions", comm)
        self.assertIn("currency_transaction_player_ready", nanny)
        self.assertIn("critical_command_coordinator_is_fenced", transaction)


if __name__ == "__main__":
    unittest.main()
