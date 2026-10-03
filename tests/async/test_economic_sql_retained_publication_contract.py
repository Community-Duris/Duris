#!/usr/bin/env python3
"""Source contract for retained cutover publication (no SQL/native execution)."""
from pathlib import Path
import ast
import unittest

ROOT = Path(__file__).resolve().parents[2]
HEADER = (ROOT / "src/persistence/economic_sql_lifecycle_guard.h").read_text()
SOURCE = (ROOT / "src/persistence/economic_sql_cutover_capability.c").read_text()
CPP_TEST = (ROOT / "tests/async/economic_sql_owned_cutover_capability.cpp").read_text()


def function_body(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for offset in range(opening, len(source)):
        if source[offset] == "{":
            depth += 1
        elif source[offset] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : offset]
    raise AssertionError(f"unterminated function: {signature}")


class EconomicSqlRetainedPublicationContract(unittest.TestCase):
    def test_api_is_opt_in_and_preserves_c18_readback_friend(self):
        for token in (
            "commit_and_retain_publication() noexcept",
            "is_valid_for_publication() noexcept",
            "finish_publication(economic_sql_lifecycle_guard *lifetime_guard) noexcept",
            "publication_pending() const noexcept",
            "bool maintenance_ = false;",
            "bool publication_pending_ = false;",
            "economic_sql_activation_receipt_readback(MYSQL *, const economic_sql_lifecycle_guard &",
        ):
            self.assertIn(token, HEADER)
        self.assertNotIn("acquire_qualification", HEADER)
        self.assertNotIn("activation_receipt_write", SOURCE)

    def test_known_commit_and_publication_pending_are_distinct(self):
        commit_retained = function_body(
            SOURCE,
            "bool economic_sql_cutover_transaction_owner::commit_and_retain_publication()",
        )
        self.assertIn("!maintenance_", commit_retained)
        self.assertIn("mysql_commit(connection_)", commit_retained)
        self.assertIn("terminal_outcome_ = economic_sql_cutover_terminal_outcome::committed", commit_retained)
        self.assertIn("publication_pending_ = true", commit_retained)
        self.assertIn("return is_valid_for_publication()", commit_retained)
        self.assertNotIn("release_after_terminal()", commit_retained)

        publication_valid = function_body(
            SOURCE,
            "bool economic_sql_cutover_transaction_owner::is_valid_for_publication()",
        )
        for token in (
            "terminal_outcome_ != economic_sql_cutover_terminal_outcome::committed",
            "!publication_pending_",
            "!maintenance_",
            "sql_resources_released_",
            "mysql_thread_id(connection_) != session_",
            "reconnect_disabled(connection_)",
            "SERVER_STATUS_IN_TRANS",
            "SERVER_STATUS_AUTOCOMMIT",
            "critical_command_coordinator_owner::validate_cutover_transaction",
            "ECONOMIC_SQL_BOOT_MAINTENANCE_LOCK_NAME",
            '"duris:economic_sql_currency_writers"',
        ):
            self.assertIn(token, publication_valid)
        self.assertNotIn("outcome_uncertain_ = true", publication_valid)

    def test_handoff_preflights_empty_guard_and_holds_fences_through_release(self):
        handoff = function_body(
            SOURCE,
            "bool economic_sql_cutover_transaction_owner::finish_publication(",
        )
        for field in (
            "connection_",
            "session_",
            "runtime_lock_",
            "writer_lock_",
            "maintenance_",
            "local_runtime_",
            "local_maintenance_",
            "coordinator_release_",
            "authority_id_",
            "coordinator_generation_",
            "coordinator_lease_id_",
            "local_exclusive_.owns_lock()",
            "local_exclusive_.mutex()",
        ):
            self.assertIn(f"lifetime_guard->{field}", handoff)
        release = handoff.index("finish_cutover_transaction(")
        transfer = handoff.index("lifetime_guard->local_exclusive_ = std::move(local_exclusive_)")
        self.assertLess(release, transfer)
        self.assertLess(handoff.index("if (!lifetime_guard"), release)
        self.assertIn("if (!critical_command_coordinator_owner::finish_cutover_transaction(", handoff)
        self.assertNotIn("release_lock_and_verify", handoff)

    def test_ordinary_cleanup_cannot_release_retained_publication(self):
        release = function_body(
            SOURCE,
            "bool economic_sql_cutover_transaction_owner::release_after_terminal()",
        )
        commit = function_body(SOURCE, "bool economic_sql_cutover_transaction_owner::commit()")
        retry = function_body(SOURCE, "bool economic_sql_cutover_transaction_owner::retry_cleanup()")
        self.assertIn("publication_pending_", release)
        self.assertIn("if (publication_pending_)", commit)
        self.assertIn("publication_pending_", retry)
        self.assertIn("return release_after_terminal()", commit)
        self.assertIn("return release_after_terminal()", retry)

    def test_regression_source_covers_retention_refusal_handoff_and_legacy_path(self):
        for token in (
            "test_retained_publication_handoff",
            "commit_and_retain_publication()",
            "transaction.publication_pending()",
            "!held.accepting",
            "finish_publication(nullptr)",
            "wrong_thread_finish",
            "finish_publication(&guard)",
            "guard.is_valid_authority()",
            "test_retained_commit_requires_maintenance",
            "run_retained_session_loss_child",
            "run_retained_destructor_child",
            "terminal_reopen_order",
            "terminal_cleanup_retry",
            "OWNED-CUTOVER-SOURCE",
        ):
            self.assertIn(token, CPP_TEST)
        self.assertIn("--retained-session-loss-child", CPP_TEST)
        self.assertIn("--retained-destructor-child", CPP_TEST)

    def test_session_loss_counts_only_additional_terminal_attempts(self):
        child = function_body(CPP_TEST, "int run_retained_session_loss_child(")
        fault = child.index('execute(killer, "KILL CONNECTION "')
        for counter, baseline in (
            ("commit_attempts", "committed_attempts"),
            ("rollback_attempts", "rollback_attempts_before_loss"),
        ):
            capture = f"const auto {baseline} = {counter}.load();"
            self.assertIn(capture, child)
            self.assertLess(child.index(capture), fault)
            self.assertIn(f"{counter} == {baseline}", child)

    def test_this_contract_script_is_valid_python(self):
        ast.parse(Path(__file__).read_text())


if __name__ == "__main__":
    unittest.main(verbosity=2)
