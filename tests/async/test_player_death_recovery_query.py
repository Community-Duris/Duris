#!/usr/bin/env python3
"""Focused contracts and pure runtime checks for the read-only death-recovery path."""

from pathlib import Path
import os
import shlex
import subprocess
import tempfile
import unittest
from _source_contract import function_bodies
from contract_text import index

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
LOAD_REPOSITORY = (SRC / "player/player_load_repository.c").read_text()
QUERY_SOURCE = (SRC / "player/player_death_recovery_query.c").read_text()
QUERY_HEADER = (SRC / "player/player_death_recovery_query.h").read_text()
ACCOUNT = (SRC / "account/account.c").read_text()
LOAD_EXECUTORS = function_bodies(LOAD_REPOSITORY, r"\bexecute_player_load\s*\(")
assert len(LOAD_EXECUTORS) == 1, "the shared load executor must have one definition"
LOAD_EXECUTION = LOAD_EXECUTORS[0]


class DeathRecoveryContracts(unittest.TestCase):
    def test_pure_query_identity_and_sanitization_harness(self):
        flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
        libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
        with tempfile.TemporaryDirectory(prefix="death-recovery-query-") as directory:
            binary = Path(directory) / "query-harness"
            subprocess.run(
                [
                    os.environ.get("CXX", "g++"),
                    "-std=c++20",
                    "-Wall",
                    "-Wextra",
                    "-Wpedantic",
                    "-Werror",
                    "-pthread",
                    "-Isrc",
                    *flags,
                    "tests/async/player_death_recovery_query_harness.cpp",
                    "src/player/player_death_recovery_query.c",
                    "src/persistence/persistence_observability.c",
                    *libs,
                    "-lcrypto",
                    "-o",
                    str(binary),
                ],
                cwd=ROOT,
                check=True,
            )
            subprocess.run([str(binary)], cwd=ROOT, check=True)

    def test_authenticated_self_scope_and_async_identity_checks(self):
        owner_check = QUERY_SOURCE[
            QUERY_SOURCE.index("bool account_owns_character") : QUERY_SOURCE.index(
                "std::string sanitize_item_label"
            )
        ]
        self.assertIn("FROM account_characters ac JOIN player_data pd ON pd.pid=ac.pid", owner_check)
        self.assertIn("ac.pid=", owner_check)
        self.assertIn("LOWER(ac.account_name)=LOWER('", owner_check)
        self.assertIn("LOWER(ac.char_name)=LOWER('", owner_check)
        self.assertIn("LOWER(pd.name)=LOWER('", owner_check)
        self.assertIn("ac.deleted_at IS NULL", owner_check)

        execution = QUERY_SOURCE[QUERY_SOURCE.index("player_death_recovery_query_execute(") :]
        self.assertLess(execution.index("account_owns_character("), execution.index("player_death_conflict_list("))
        self.assertLess(execution.index("account_owns_character("), execution.index("player_death_conflict_read("))
        self.assertIn("request.operation_id", execution)

        submit = ACCOUNT[
            ACCOUNT.index("bool account_death_recovery_submit") : ACCOUNT.index(
                "struct acct_chars *account_death_recovery_selection"
            )
        ]
        self.assertIn("account_recovery_character_by_name", submit)
        self.assertIn("request.pid = character->pid", submit)
        self.assertNotIn("strtol", submit)
        self.assertIn("player_load_pipeline_submit(request)", submit)
        self.assertNotIn("player_load_pipeline_wait", submit)
        self.assertNotIn("execute_sync", submit)

        completion = ACCOUNT[
            ACCOUNT.index("void account_death_recovery_query_complete") : ACCOUNT.index(
                "void account_player_load_complete"
            )
        ]
        for required in (
            "account_descriptor_is_live(descriptor)",
            "descriptor->player_load_request_id == result.request_id",
            "descriptor->player_load_pid == result.pid",
            "descriptor->account->acct_name",
            "descriptor->selected_char_name",
            "account_recovery_character_by_identity",
            "Recovery read discarded because the account or character context changed.",
            "if (STATE(descriptor) == CON_ACCT_SELECT_CHAR)",
        ):
            self.assertIn(required, completion)
        normal_completion = ACCOUNT[ACCOUNT.index("void account_player_load_complete") :]
        self.assertIn("if (!d || !account_descriptor_is_live(d))", normal_completion)
        self.assertLess(
            normal_completion.index("if (!d || !account_descriptor_is_live(d))"),
            normal_completion.index("d->player_load_mode"),
        )

    def test_gate_fails_closed_before_item_pet_reads_or_materialization(self):
        gate_start = LOAD_EXECUTION.index("std::vector<player_death_conflict_case> retained_cases;")
        gate = LOAD_EXECUTION[gate_start : LOAD_EXECUTION.index("load_components(connection")]
        self.assertNotIn("if (request.include_items)", gate)
        self.assertIn("player_death_conflict_list(connection, result.pid, 0", gate)
        self.assertIn("player_load_recovery_gate::retained_conflict", gate)
        self.assertIn("player_load_recovery_gate::unavailable", gate)
        self.assertIn("result.snapshot = {}", gate)
        self.assertIn("clear_items_and_pets(&result)", gate)
        self.assertIn('execute(connection, "ROLLBACK", &result)', gate)
        self.assertIn("return result", gate)
        gate_start = index(LOAD_EXECUTION, 'execute(connection, "START TRANSACTION", &result)')
        identity_lock = LOAD_EXECUTION.index('" LOCK IN SHARE MODE"', gate_start)
        first_snapshot_read = index(LOAD_EXECUTION, 'if (!load_status(connection, request, &result, recovery_inspection))', identity_lock)
        self.assertLess(gate_start, identity_lock)
        self.assertLess(identity_lock, first_snapshot_read)
        self.assertLess(
            first_snapshot_read,
            LOAD_EXECUTION.index("player_death_conflict_list(connection, result.pid, 0", gate_start),
        )
        self.assertLess(
            LOAD_REPOSITORY.index("player_death_conflict_list(connection, result.pid, 0"),
            LOAD_REPOSITORY.index("load_items(connection, &result)"),
        )
        self.assertLess(
            LOAD_REPOSITORY.index("player_death_conflict_list(connection, result.pid, 0"),
            LOAD_REPOSITORY.index("load_pets(connection, &result)"),
        )

        query_dispatch = LOAD_EXECUTION[
            LOAD_EXECUTION.index("if (request.death_recovery_query.kind !=") :
            index(LOAD_EXECUTION, 'execute(connection, "SET TRANSACTION ISOLATION LEVEL')
        ]
        self.assertIn("player_death_recovery_query_execute", query_dispatch)
        self.assertIn("return result", query_dispatch)
        self.assertIn("!request.include_items && !request.include_pets", LOAD_REPOSITORY)

        account_result = ACCOUNT[
            ACCOUNT.index("void account_player_load_complete") : ACCOUNT.index(
                "void account_new_char"
            )
        ]
        self.assertIn("account_recovery_gate_refused(d, result.recovery_gate)", account_result)
        self.assertLess(
            account_result.index("account_recovery_gate_refused(d, result.recovery_gate)"),
            account_result.index("ready_player_loads.emplace"),
        )

    def test_missing_schema_and_read_failure_are_not_treated_as_clear(self):
        gate = LOAD_REPOSITORY[
            LOAD_REPOSITORY.index("const player_death_conflict_result recovery_check") :
            LOAD_REPOSITORY.index("// Status and identity are the only mandatory player-load domain")
        ]
        self.assertIn("recovery_check.outcome == player_death_conflict_outcome::read", gate)
        self.assertIn("player_load_recovery_gate::unavailable", gate)
        self.assertIn("result.outcome = player_load_outcome::component_failure", gate)
        self.assertIn("clear_items_and_pets(&result)", gate)

        query_failure = QUERY_SOURCE[
            QUERY_SOURCE.index("const player_death_conflict_result listed") :
            QUERY_SOURCE.index("output.cases = std::move(cases)")
        ]
        self.assertIn("listed.outcome != player_death_conflict_outcome::read", query_failure)
        self.assertIn("rollback_error(listed.error_code", query_failure)
        self.assertIn("const player_death_conflict_result read", QUERY_SOURCE)
        self.assertIn("read.outcome != player_death_conflict_outcome::read", QUERY_SOURCE)
        self.assertIn("rollback_error(read.error_code", QUERY_SOURCE)

    def test_revision_pagination_and_detail_case_identity_are_bound(self):
        self.assertIn(
            "PLAYER_DEATH_CONFLICT_LIST_LIMIT = 25",
            (SRC / "player/player_death_conflict_repository.h").read_text(),
        )
        self.assertIn("request.after_revision", QUERY_SOURCE)
        self.assertIn("save_revision>", (SRC / "player/player_death_conflict_repository.c").read_text())
        self.assertIn("query.cases.size() == PLAYER_DEATH_CONFLICT_LIST_LIMIT", ACCOUNT)
        self.assertIn("next page: L %llu", ACCOUNT)
        self.assertIn("identity.operation_id.bytes == requested.bytes", QUERY_SOURCE)
        self.assertIn("snapshot.revision == identity.save_revision", QUERY_SOURCE)
        self.assertIn("identity.corpse_item_uid == snapshot.death->corpse.front().object_uid", QUERY_SOURCE)
        self.assertIn("player_death_conflict_read(", QUERY_SOURCE)

    def test_metadata_only_delete_confirmation_cannot_materialize_or_delete(self):
        materializer = (SRC / "player/player_load_materialize.c").read_text()
        valid_snapshot = materializer[
            materializer.index("bool valid_snapshot") : materializer.index("bool valid_snapshot") + 1800
        ]
        self.assertIn("result.outcome != player_load_outcome::applied && !degraded", valid_snapshot)
        self.assertIn("result.snapshot.schema_version != PLAYER_SNAPSHOT_SCHEMA_VERSION", valid_snapshot)

        load_char = ACCOUNT[
            ACCOUNT.index("P_char load_char_into_game") : ACCOUNT.index(
                "void account_death_recovery_query_complete"
            )
        ]
        self.assertIn("if (!player_load_materialize(player, loaded))", load_char)
        self.assertLess(
            load_char.index("if (!player_load_materialize(player, loaded))"),
            load_char.index("return player;"),
        )

        delete_flow = ACCOUNT[ACCOUNT.index("void account_delete_char(P_desc d, char *arg)") :]
        self.assertLess(delete_flow.index("ch = load_char_into_game(c, d)"), delete_flow.index("if (!ch)"))
        self.assertLess(delete_flow.index("if (!ch)"), delete_flow.index("Are you &+RABSOLUTELY SURE&n"))
        self.assertIn("STATE(d) = CON_DISPLAY_ACCT_MENU", delete_flow)

    def test_archive_evidence_is_unresolved_and_read_only(self):
        self.assertIn("unresolved archive evidence only", ACCOUNT)
        self.assertIn("not a completed terminal recovery", QUERY_SOURCE)
        for forbidden in ("INSERT INTO ", "UPDATE player_", "DELETE FROM "):
            self.assertNotIn(forbidden, QUERY_SOURCE)

    def test_recovery_path_never_enters_materializer_or_save_pipeline(self):
        submit = ACCOUNT[
            ACCOUNT.index("bool account_death_recovery_submit") : ACCOUNT.index(
                "struct acct_chars *account_death_recovery_selection"
            )
        ]
        self.assertIn("request.include_items = false", submit)
        self.assertIn("request.include_pets = false", submit)
        self.assertIn("player_load_pipeline_submit(request)", submit)

        query_dispatch = LOAD_EXECUTION[
            LOAD_EXECUTION.index("if (request.death_recovery_query.kind !=") :
            index(LOAD_EXECUTION, 'execute(connection, "SET TRANSACTION ISOLATION LEVEL')
        ]
        self.assertNotIn("load_items(", query_dispatch)
        self.assertNotIn("load_pets(", query_dispatch)
        self.assertNotIn("player_load_materialize", query_dispatch)

        query_complete = ACCOUNT[
            ACCOUNT.index("void account_death_recovery_query_complete") : ACCOUNT.index(
                "void account_player_load_complete"
            )
        ]
        self.assertNotIn("player_load_materialize", query_complete)
        self.assertNotIn("player_save_pipeline", query_complete)
        self.assertIn("if (d->player_load_mode == PLAYER_LOAD_MODE_ACCOUNT_DEATH_RECOVERY)", ACCOUNT)


if __name__ == "__main__":
    unittest.main()
