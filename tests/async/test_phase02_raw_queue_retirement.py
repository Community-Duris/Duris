#!/usr/bin/env python3
"""Typed session audit and fail-closed legacy raw queue retirement contracts."""

from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "session_audit_command.h"
#include <cassert>

int main()
{
    critical_operation_id operation = {};
    assert(critical_operation_id_generate(&operation));
    const session_audit_payload payload = {42, session_audit_event::login, 1700000000};
    critical_command command = {};
    assert(session_audit_command_build(&command, operation, payload));
    session_audit_payload decoded = {};
    assert(session_audit_command_decode_payload(command, &decoded));
    assert(decoded.pid == 42 && decoded.event == session_audit_event::login);
    std::array<uint8_t, SESSION_AUDIT_RESULT_BYTES> result = {};
    assert(session_audit_command_encode_result(payload, &result));
    assert(session_audit_command_decode_result(result.data(), result.size(), &decoded));
    command.keys[0].id = 43;
    assert(!session_audit_command_decode_payload(command, &decoded));
}
'''


class Phase02RawQueueRetirementTests(unittest.TestCase):
    def test_session_audit_codec_is_bounded_and_reconstructs_key(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "session_audit.cpp"
            binary = Path(directory) / "session_audit"
            source.write_text(HARNESS)
            subprocess.run([
                "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{SRC}",
                str(source), str(SRC / "critical_command.c"),
                str(SRC / "session_audit_command.c"), "-lcrypto", "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)

    def test_login_logout_use_typed_command_without_private_payload(self):
        sql = (SRC / "sql.c").read_text()
        start = sql.index("void sql_log_player_login")
        end = sql.index("/* ---- Persistence DB connection ----", start)
        body = sql[start:end]
        self.assertIn("session_audit_transaction_submit", body)
        for forbidden in ("qry(", "db_query", "persistence_scalar_event_queue_enqueue",
                          "host", "account", "client_name"):
            self.assertNotIn(forbidden, body)

    def test_boot_and_shutdown_do_not_activate_legacy_raw_workers(self):
        comm = (SRC / "comm.c").read_text()
        run = comm[comm.index("void run_the_game"):]
        for forbidden in ("persistence_replay_fallback_events();",
                          "persistence_start_item_event_worker();",
                          "persistence_start_scalar_event_worker();",
                          "persistence_start_large_event_worker();"):
            self.assertNotIn(forbidden, run)
        raw = (SRC / "sql_persistence_raw.c").read_text()
        function = raw[raw.index("bool sql_persistence_execute_raw"):]
        self.assertIn("return false;", function)
        self.assertNotIn("sql_observed_execute_at", function)

    def test_legacy_replay_only_quarantines(self):
        utility = (SRC / "utility.c").read_text()
        start = utility.index("int persistence_replay_fallback_events")
        end = utility.index("static int persistence_item_event_log_writer", start)
        active = utility[start:end].split("#if 0", 1)[0]
        self.assertIn("persistence_quarantine_fallback_events", active)
        self.assertNotIn("sql_persistence_write", active)
        self.assertNotIn("fopen", active)

    def test_inspector_hashes_counts_and_requires_explicit_quarantine(self):
        script = ROOT / "scripts/inspect_legacy_persistence_fallback.sh"
        with tempfile.TemporaryDirectory() as directory:
            fallback = Path(directory) / "events.log"
            fallback.write_text(
                "PERSISTENCE_ITEM_EVENT|ts=1|event=x\n"
                "PERSISTENCE_SCALAR_EVENT|INSERT INTO x\nordinary log\n"
            )
            inspected = subprocess.run([str(script), str(fallback)], check=True,
                                       text=True, capture_output=True).stdout
            self.assertIn("item=1 scalar=1 large=0 other=1", inspected)
            self.assertTrue(fallback.exists())
            quarantined = subprocess.run([str(script), "--quarantine", str(fallback)],
                                         check=True, text=True,
                                         capture_output=True).stdout
            self.assertIn("quarantined=", quarantined)
            self.assertFalse(fallback.exists())

    def test_schema_reconciliation_and_operator_contract_are_wired(self):
        for path, token in (
            ("migrations/session_audit_outcome.sql", "session_audit_outcome"),
            ("migrations/bootstrap_multithread_safe.sql", "session_audit_outcome"),
            ("migrations/run_migration.sh", "verify_session_audit_schema.sh"),
            ("migrations/reconcile_phase02_domains.sh", "reconcile_item_ownership.sh"),
            ("docs/gates/PHASE02_DOMAIN_GATE.md", "Never execute a legacy fallback record"),
        ):
            self.assertIn(token, (ROOT / path).read_text())


if __name__ == "__main__":
    unittest.main()
