#!/usr/bin/env python3
"""Regression contracts for durable container movement and nesting repair.

Nesting is ledger state.  `item_current_owner` carries each item's parent and
root, `capture()` refuses a subtree whose ledger nesting disagrees with the live
object tree, and player load rebuilds nesting from the ledger rather than from
the saved rows.  A put that moves an item into a container without submitting a
transfer therefore strands the container: every later give or drop of it fails
preflight, and the contents un-nest on the next login.  These contracts pin the
single-item put to the same durable path `put all` already uses, and pin the
refusal diagnostics that made the original report unattributable.
"""

import os
import re
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from _paths import ROOT, SRC


def function_body(text, signature, terminator):
    start = text.index(signature)
    return text[start:text.index(terminator, start)]


def write_executable(path, text):
    path.write_text(text)
    path.chmod(0o755)


def script_environment(host="127.0.0.1", extra=""):
    return (
        "ENVIRONMENT=test\n"
        "DB_NAME=duris_test\n"
        f"DB_HOST={host}\n"
        "DB_PORT=3306\n"
        "DB_USER=tester\n"
        "DB_PASSWD=test-password\n"
        f"{extra}"
    )


def fake_mysql():
    return """#!/usr/bin/env bash
if [[ "${1:-}" == "--help" ]]; then
  printf '%s\n' "${FAKE_MYSQL_HELP:---ssl-mode=name}"
  exit 0
fi
if [[ -n "${MYSQL_ARGS_LOG:-}" ]]; then
  printf '%s\n' "$@" >> "$MYSQL_ARGS_LOG"
fi
printf '%b' "${FAKE_MYSQL_OUTPUT:-0\\n}"
exit "${FAKE_MYSQL_STATUS:-0}"
"""


def run_copied_script(script_name, env_text, *, arguments=(), repair_body=None,
                      mysql_output="0\n"):
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        migrations = root / "migrations"
        binary = root / "bin"
        migrations.mkdir()
        binary.mkdir()
        shutil.copy2(ROOT / "migrations" / script_name, migrations / script_name)
        if repair_body is not None:
            write_executable(migrations / "repair_item_nesting.sh", repair_body)
        (root / ".env").write_text(env_text)
        write_executable(binary / "mysql", fake_mysql())
        process_env = os.environ.copy()
        process_env["PATH"] = f"{binary}:{process_env['PATH']}"
        process_env["FAKE_MYSQL_OUTPUT"] = mysql_output
        process_env["MYSQL_ARGS_LOG"] = str(root / "mysql-args")
        result = subprocess.run(
            [str(migrations / script_name), *arguments], cwd=root,
            env=process_env, text=True, capture_output=True, check=False)
        args_log = root / "mysql-args"
        return result, args_log.read_text() if args_log.exists() else ""


class DurableContainerPutContractTests(unittest.TestCase):
    def test_put_defers_to_the_ownership_pipeline_for_a_same_owner_container(self):
        actobj = (SRC / "actobj.c").read_text()
        self.assertIn("bool defer_durable_put(", actobj)
        self.assertNotIn("defer_cross_owner_put", actobj)
        body = function_body(actobj, "bool defer_durable_put(", "\nbool submit_player_drop(")
        # The locker branch keeps its same-owner shortcut: a chest is an owner,
        # not a parent, so an item already owned by the chest records nothing.
        locker = body[:body.index("if (!item_ownership_runtime_lookup(container->obj_uid")]
        container = body[body.index("if (!item_ownership_runtime_lookup(container->obj_uid"):]
        self.assertIn("item_owner_identity_equal(source, locker_destination)", locker)
        # The container branch must not short-circuit on a matching owner; the
        # parent linkage still has to reach the ledger.
        self.assertNotIn("item_owner_identity_equal(source, container_runtime.owner)",
                         container)
        self.assertIn("item_transfer_reason::player_put", container)
        # The container itself is the ownership target, which is what carries
        # target_parent_item_uid / target_root_item_uid into the payload.
        submit = container[container.index("item_movement_transaction_submit("):]
        self.assertIn("actor, object, container, source, destination", submit)

    def test_uid_bearing_container_without_runtime_authority_is_rejected(self):
        actobj = (SRC / "actobj.c").read_text()
        body = function_body(actobj, "bool defer_durable_put(",
                             "\nbool submit_player_drop(")
        failure_start = body.index(
            "if (!item_ownership_runtime_lookup(container->obj_uid")
        failure = body[failure_start:body.index(
            "const item_owner_identity source =", failure_start)]
        self.assertNotIn("OBJ_CARRIED_BY(container, actor)", failure)
        self.assertIn("That container lacks authoritative ownership", failure)
        self.assertIn("return true;", failure)

    def test_single_item_put_records_the_same_topology_as_bulk_put(self):
        actobj = (SRC / "actobj.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        bulk = function_body(actobj, "void start_bulk_put(", "\nvoid do_put(")
        self.assertIn("P_obj ownership_target = container;", bulk)
        # Both paths hand the container to the same payload field.
        self.assertIn(".target_parent_item_uid = target_container ? target_container->obj_uid : 0,",
                      movement)

    def test_capture_names_both_sides_of_a_topology_mismatch(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        capture = function_body(movement, "bool capture(P_obj object", "\nbool capture_absent(")
        self.assertIn("outcome=topology_mismatch", capture)
        self.assertIn("cause=missing_ledger_row", capture)
        self.assertIn("ledger(root=", capture)
        self.assertIn("live(root=", capture)

    def test_every_submission_refusal_carries_a_reason(self):
        header = (SRC / "item_movement_transaction.h").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        for reason in ("invalid_request", "queue_saturated", "pending_conflict",
                       "owner_mismatch", "missing_owner_revision", "topology_mismatch",
                       "snapshot_failure", "allocation_failure", "command_build_failure",
                       "coordinator_rejected"):
            self.assertIn(f"\t{reason},", header)
            self.assertIn(f"item_movement_reject::{reason}", movement)
        self.assertIn("item_movement_reject *reject = NULL", header)
        for signature, terminator in (
                ("bool item_movement_transaction_submit(P_char actor",
                 "\nconst char *item_movement_reject_name"),
                ("bool item_movement_transaction_submit_batch(",
                 "\nconst char *item_movement_reject_name")):
            body = function_body(movement, signature, terminator)
            body = body[:body.index("\n}\n")] if "\n}\n" in body else body
            self.assertNotIn("return false;", body,
                             f"{signature} still refuses without naming a reason")

    def test_busy_and_unreconciled_failures_read_differently(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        actobj = (SRC / "actobj.c").read_text()
        transient = function_body(movement, "bool item_movement_reject_is_transient",
                                  "\nbool item_creation_grant_submit_to_player")
        self.assertIn("item_movement_reject::pending_conflict", transient)
        self.assertIn("item_movement_reject::queue_saturated", transient)
        self.assertNotIn("item_movement_reject::topology_mismatch", transient)
        self.assertNotIn("item_movement_reject::owner_mismatch", transient)
        # The single collapsed sentence is gone from every command.
        self.assertNotIn("busy or lacks authoritative ownership", actobj)
        self.assertNotIn("busy or its ownership changed", actobj)
        report = function_body(actobj, "void report_movement_reject(",
                               "\nvoid report_batch_movement_reject(")
        self.assertIn("item_movement_reject_is_transient(reason)", report)
        self.assertIn("item_movement_reject_name(reason)", report)

    def test_audited_commands_report_the_reason_they_were_refused(self):
        actobj = (SRC / "actobj.c").read_text()
        for command in ("give", "get", "put", "drop"):
            self.assertRegex(actobj, re.compile(
                r'report_(batch_)?movement_reject\([^;]*"' + command + r'"', re.S),
                f"{command} does not report a refusal reason")
        # submit_player_drop hands its reason back rather than swallowing it.
        self.assertIn("bool submit_player_drop(P_char ch, P_obj object, "
                      "item_movement_reject *reject)", actobj)

    def test_overload_consent_rejection_precedes_any_give_submission(self):
        actobj = (SRC / "actobj.c").read_text()
        give = function_body(actobj, "void do_give(", "\nvoid do_drink(")
        consent_start = give.index("if (((((GET_OBJ_WEIGHT(obj)")
        consent_end = give.index("if (IS_ARTIFACT(obj)", consent_start)
        consent_gate = give[consent_start:consent_end]
        durable_start = give.index(
            "if (IS_PC(ch) && IS_PC(vict) && ch != vict && "
            "uses_generic_item_ownership(obj))")
        durable_give = give[durable_start:give.index("\n\tobj_from_char(obj);", durable_start)]
        self.assertIn("!is_linked_to(ch, vict, LNK_CONSENT)", consent_gate)
        self.assertIn("return;", consent_gate)
        self.assertNotIn("item_movement_transaction_submit", consent_gate)
        self.assertLess(consent_start, durable_start)
        self.assertEqual(durable_give.count("item_movement_transaction_submit("), 1)
        self.assertIn("item_transfer_reason::player_give", durable_give)
        self.assertIn("item_give_completion", durable_give)

    def test_populated_give_captures_and_publishes_the_tree_after_commit(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        actobj = (SRC / "actobj.c").read_text()
        capture = function_body(movement, "bool capture(P_obj object",
                                "\nbool capture_absent(")
        completion = function_body(actobj, "void item_give_completion(",
                                   "\nvoid item_put_completion(")
        self.assertIn("for (P_obj child = object->contains", capture)
        self.assertIn("capture(child, root_uid, object->obj_uid, items)", capture)
        self.assertLess(completion.index("!committed"),
                        completion.index("obj_from_char(object)"))
        self.assertLess(completion.index("obj_from_char(object)"),
                        completion.index("obj_to_char(object, recipient)"))

    def test_both_ownership_backends_cover_exactly_once_populated_give(self):
        mysql = (ROOT / "tests/async/item_transfer_mysql_harness.cpp").read_text()
        flatfile = (ROOT / "tests/async/flatfile_item_repository_harness.cpp").read_text()
        for backend in (mysql, flatfile):
            self.assertIn("item_transfer_reason::player_give", backend)
            self.assertIn("critical_apply_outcome::already_applied", backend)
        self.assertIn("created_result.item_count == 2", mysql)
        self.assertIn("move.item_count = 2", flatfile)
        self.assertIn("cross-owner give must apply exactly once", mysql)

    def test_nesting_repair_filters_ambiguous_quarantined_evidence(self):
        repair = (ROOT / "migrations/repair_item_nesting.sh").read_text()
        self.assertIn("CREATE TEMPORARY TABLE item_nesting_candidates", repair)
        self.assertNotIn("INSERT IGNORE INTO item_nesting_evidence", repair)
        self.assertIn("item_ownership_quarantine", repair)
        self.assertIn("repaired_at IS NULL", repair)
        self.assertIn(
            "HAVING COUNT(DISTINCT COALESCE(candidate.parent_uid,0))=1", repair)

    def test_nesting_repair_validates_proposed_chains_and_expected_roots(self):
        repair = (ROOT / "migrations/repair_item_nesting.sh").read_text()
        guard = repair.index("CREATE TEMPORARY TABLE item_nesting_repair_guard")
        update = repair.index("UPDATE item_current_owner current_item "
                              "JOIN item_nesting_drift drift")
        self.assertIn("item_nesting_evidence_invalid", repair)
        self.assertIn("item_nesting_proposed_invalid", repair)
        self.assertIn("cycle_detected", repair)
        self.assertIn("SUM(CASE WHEN parent_uid IS NULL THEN 1 ELSE 0 END)=1", repair)
        self.assertIn("current_item.root_item_uid<>roots.root_uid", repair)
        self.assertLess(guard, update)

    def test_remote_repair_uses_verified_tls_and_never_skip_ssl(self):
        with tempfile.NamedTemporaryFile() as certificate:
            env_text = script_environment(
                "db.example.test",
                f"DB_TLS=TRUE\nDB_SSL_CA={certificate.name}\n")
            result, mysql_args = run_copied_script(
                "repair_item_nesting.sh", env_text, arguments=("--check",),
                mysql_output="0\n0\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "nesting_mismatch=0\n")
        self.assertIn("--ssl-mode=VERIFY_IDENTITY", mysql_args)
        self.assertIn(f"--ssl-ca={certificate.name}", mysql_args)
        self.assertNotIn("--skip-ssl", mysql_args)
        repair = (ROOT / "migrations/repair_item_nesting.sh").read_text()
        self.assertNotIn("--ssl-mode=PREFERRED", repair)
        self.assertNotIn("--skip-ssl", repair)

    def test_reconcile_preserves_a_valid_nonzero_nesting_count(self):
        checker = """#!/usr/bin/env bash
printf 'nesting_mismatch=3\\n'
exit 1
"""
        result, _ = run_copied_script(
            "reconcile_item_ownership.sh", script_environment(),
            repair_body=checker)
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn("nesting_mismatch=3", result.stdout)

    def test_reconcile_propagates_checker_failure_instead_of_reporting_zero(self):
        checker = """#!/usr/bin/env bash
echo 'database unavailable' >&2
exit 42
"""
        result, _ = run_copied_script(
            "reconcile_item_ownership.sh", script_environment(),
            repair_body=checker)
        self.assertEqual(result.returncode, 42)
        self.assertIn("database unavailable", result.stderr)
        self.assertNotIn("nesting_mismatch=0", result.stdout)


if __name__ == "__main__":
    unittest.main()
