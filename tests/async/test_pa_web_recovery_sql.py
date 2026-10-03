#!/usr/bin/env python3
"""Real retained-recovery and admin-delete journey on isolated disposable SQL.

Opt in explicitly with PA_WEB_RECOVERY_DISPOSABLE=1. The fixture generates its
own DB/service secrets, uses the approved corpse_journey_test selector, boots the
attested frozen server, and drives its real WebSocket listener.
"""
from __future__ import annotations

import os
from pathlib import Path
import secrets
import shutil
import tempfile
import unittest

import pa_web_recovery_fixture as fixture

ROOT = Path(__file__).resolve().parents[2]
RECOVERY_PID = 74001
CLEAN_PID = 74002
ACCOUNT = "S10Test"


def reserve_ports() -> tuple[int, int, int]:
    return fixture.reserve_ports()


def assert_no_success(response: dict, progress: list[dict], request_id: str) -> None:
    if response.get("success") is not False or response.get("requestId") != request_id:
        raise AssertionError(f"delete response was not a correlated refusal: {response!r}")
    if any(item.get("message") in {
            "Character save file deleted", "Character deletion completed", "Account file updated"
    } for item in progress):
        raise AssertionError("failed deletion emitted success progress")


def run_journey() -> None:
    server = fixture.verify_frozen_binary()
    if os.environ.get("PA_WEB_RECOVERY_DISPOSABLE") != "1":
        raise RuntimeError(
            "set PA_WEB_RECOVERY_DISPOSABLE=1 to run this disposable SQL/WebSocket journey"
        )
    compiler = shutil.which(os.environ.get("CXX", "g++-14"))
    if not compiler:
        raise AssertionError("frozen harness compiler unavailable")

    bin_tests = ROOT / "bin/tests"
    bin_tests.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="s10-pa-web-recovery-", dir=bin_tests) as temp:
        temporary = Path(temp)
        sql_harness = temporary / "pa-web-recovery-sql"
        fixture.build_sql_harness(sql_harness)
        database_fixture = fixture.DisposableMariaDB()
        with database_fixture as database:
            print("fixture: generated loopback MariaDB and approved corpse_journey_test selector", flush=True)
            database.initialize_schema()
            print(fixture.run_sql_harness(sql_harness, database, "--seed"), flush=True)
            recovery_state = fixture.run_sql_harness(sql_harness, database, "--snapshot")
            if len(recovery_state) != 64 or any(c not in "0123456789abcdef" for c in recovery_state):
                raise AssertionError("fixture state digest malformed")

            proxy = fixture.CommitReplyDropProxy("127.0.0.1", database.port)
            secret = secrets.token_hex(32)
            game_port, tls_port, web_port = reserve_ports()
            try:
                runtime_dir = temporary / "runtime"
                runtime_dir.mkdir(mode=0o700)
                with fixture.server_fixture(
                        database, runtime_dir, server=server, database_port=proxy.port,
                        secret=secret, web_port=web_port, game_port=game_port, tls_port=tls_port):
                    client = fixture.WebSocketClient(web_port)
                    try:
                        client.authenticate(secret)
                        # Rename one load-only column in the disposable schema. The real
                        # restoreCharOnly SQL status query must fail, while account
                        # membership and retained evidence remain queryable.
                        columns = database.mysql("SHOW COLUMNS FROM player_data LIKE 'output_preferences'")
                        if not columns:
                            raise AssertionError("expected SQL restore column is absent from fixture schema")
                        database.mysql(
                            "ALTER TABLE player_data RENAME COLUMN output_preferences TO s10_hold_output_preferences")
                        try:
                            before = fixture.run_sql_harness(sql_harness, database, "--snapshot")
                            if before != recovery_state:
                                raise AssertionError("server setup changed target case/account/custody state")
                            response, progress = client.delete(
                                request_id="s10-restore-failure", account=ACCOUNT,
                                name="S10Recovery", pid=RECOVERY_PID)
                            assert_no_success(response, progress, "s10-restore-failure")
                            if response.get("error") != "Character save data could not be loaded; deletion was deferred":
                                raise AssertionError(f"restore failure was not surfaced accurately: {response!r}")
                            after = fixture.run_sql_harness(sql_harness, database, "--snapshot")
                            if before != after:
                                raise AssertionError("restore failure mutated account/player/item/custody/case SQL state")
                            print("PASS: actual WebSocket handler/transport reported restore failure; SQL state byte-stable", flush=True)
                        finally:
                            database.mysql(
                                "ALTER TABLE player_data RENAME COLUMN s10_hold_output_preferences TO output_preferences")

                        before_refusal = fixture.run_sql_harness(sql_harness, database, "--snapshot")
                        response, progress = client.delete(
                            request_id="s10-retained-refusal", account=ACCOUNT,
                            name="S10Recovery", pid=RECOVERY_PID)
                        assert_no_success(response, progress, "s10-retained-refusal")
                        if response.get("error") != "Character deletion was refused; account data was not changed":
                            raise AssertionError(f"typed retained-case refusal was not surfaced: {response!r}")
                        after_refusal = fixture.run_sql_harness(sql_harness, database, "--snapshot")
                        if before_refusal != after_refusal:
                            raise AssertionError("refused retained-case deletion mutated SQL account/custody/evidence")
                        print("PASS: real deletion guard refused retained evidence; account and item custody unchanged", flush=True)

                        # Drop only the COMMIT acknowledgement for the clean second
                        # character, after observing its real row-lock query. MariaDB
                        # commits; the handler must return reconciliation, not success
                        # or an extra account-file projection update.
                        proxy.arm(CLEAN_PID)
                        response, progress = client.delete(
                            request_id="s10-uncertain-commit", account=ACCOUNT,
                            name="S10Clean", pid=CLEAN_PID)
                        proxy.wait_dropped_ok()
                        assert_no_success(response, progress, "s10-uncertain-commit")
                        if "requires reconciliation" not in response.get("error", ""):
                            raise AssertionError(f"lost COMMIT reply was not reported as uncertain: {response!r}")
                        if "account data was not changed" in response.get("error", ""):
                            raise AssertionError("uncertain commit was mislabeled as a confirmed refusal")
                        if int(database.mysql("SELECT COUNT(*) FROM accounts WHERE account_name='S10Test'")) != 1:
                            raise AssertionError("uncertain delete removed the owning account row")
                        if int(database.mysql("SELECT COUNT(*) FROM account_characters WHERE account_name='S10Test' AND pid=74001 AND deleted_at IS NULL")) != 1:
                            raise AssertionError("uncertain delete disturbed the retained-case character membership")
                        if int(database.mysql("SELECT COUNT(*) FROM account_characters WHERE account_name='S10Test' AND pid=74002 AND deleted_at IS NOT NULL")) != 1:
                            raise AssertionError("selected COMMIT did not persist the clean character tombstone")
                        if int(database.mysql("SELECT COUNT(*) FROM player_data WHERE pid=74002")) != 0:
                            raise AssertionError("selected COMMIT acknowledgement was lost but player deletion did not land")
                        if int(database.mysql("SELECT COUNT(*) FROM player_death_conflict_evidence WHERE pid=74001")) != 1:
                            raise AssertionError("uncertain deletion altered the retained recovery case")
                        if int(database.mysql("SELECT COUNT(*) FROM item_current_owner WHERE item_uid=201 AND owner_type=1 AND owner_id=74001 AND item_revision=3")) != 1:
                            raise AssertionError("uncertain deletion altered retained-character custody")
                        print("PASS: real committed-but-unacknowledged delete returned reconciliation, no success/unrelated unlink", flush=True)
                    finally:
                        client.close()
            finally:
                proxy.close()
    if Path(temp).exists():
        raise AssertionError("temporary server/credential artifacts survived fixture cleanup")
    if fixture.container_present(database_fixture.container):
        raise AssertionError("disposable MariaDB container survived fixture cleanup")
    print("PASS: disposable DB container absent; temporary fixture files removed", flush=True)


class RetainedRecoveryWebDeletionSQL(unittest.TestCase):
    def test_live_websocket_and_disposable_sql_journey(self):
        run_journey()


if __name__ == "__main__":
    unittest.main(verbosity=2)
