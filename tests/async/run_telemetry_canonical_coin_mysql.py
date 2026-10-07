#!/usr/bin/env python3
"""Actual typed coin custody through exact retention and restricted publication.

This executable test requires an explicitly owned disposable Linux loopback
schema. It never loads .env or creates a database/server. The outer maintained
qualification command owns those resources and schema preparation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import subprocess
import sys
import time
import tracemalloc
import uuid

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from run_economic_sql_bank_transaction_mysql import compile_bank, target_is_disposable


def qualify_native_target_guard(executable):
    """The compiled positive-journey binary must refuse unsafe targets at entry."""
    base = {"PATH": os.environ.get("PATH", ""), "TEST_DB_DISPOSABLE": "1",
        "ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA": "1", "DB_HOST": "127.0.0.1", "DB_PORT": "3306",
        "DB_USER": "synthetic_guard", "DB_PASSWD": "synthetic_guard", "DB_NAME": "economic_schema_test_guard",
        "CURRENCY_TEST_DB_NAME": "economic_schema_test_guard", "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1"}
    for changed in ({"TEST_DB_DISPOSABLE": "0"}, {"DB_HOST": "192.0.2.1"},
                    {"DB_SOCKET": "/tmp/unowned-socket"}, {"DB_NAME": "unowned_database"},
                    {"CURRENCY_TEST_DB_NAME": "economic_schema_test_other"}, {"DB_PORT": "0"}):
        result = subprocess.run(["bash", "-c", 'ulimit -c 0 && ulimit -s 65536 && exec "$1"', "coin-target-guard", str(executable)],
            cwd=ROOT, env=dict(base, **changed), capture_output=True, timeout=10)
        if result.returncode == 0 or b"require_telemetry_disposable_target" not in result.stderr:
            raise AssertionError("native coin target guard did not refuse before a connection")


def qualify(executable, destination):
    import pymysql
    from scripts.telemetry import canonical_reward_contract as c
    from scripts.telemetry.canonical_reward_coin_source import CanonicalCoinSnapshot
    from scripts.telemetry.canonical_reward_source import COIN_SOURCE_TABLES, replay_retained_bank_cut
    from scripts.telemetry.canonical_reward_retention import CanonicalRewardRetention, RETENTION_TABLES, CUT_TABLE
    from scripts.telemetry.canonical_reward_publication import (
        CanonicalRewardPublisher, CanonicalRewardReporter, PRIVATE_TABLES, PUBLIC_TABLES,
        GENERATION_TABLE, COVERAGE_TABLE, PRIVATE_EVENT_TABLE, EVENT_TABLE,
    )
    from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory
    from scripts.telemetry.reward_projection import AmbiguousCommit
    from telemetry_canonical_publication_mysql import MeasuredFactory

    if os.name != "posix" or not target_is_disposable(os.environ) or os.environ.get("TEST_DB_DISPOSABLE") != "1":
        raise RuntimeError("native coin qualification requires an explicit disposable Linux loopback target")
    qualify_native_target_guard(executable)
    database = os.environ["DB_NAME"]
    common = dict(host="127.0.0.1", port=int(os.environ["DB_PORT"]), database=database,
        connect_timeout=3, read_timeout=3, write_timeout=3, autocommit=True,
        charset="utf8mb4", cursorclass=pymysql.cursors.DictCursor)
    admin = pymysql.connect(user=os.environ["DB_USER"], password=os.environ["DB_PASSWD"], **common)
    suffix, password = uuid.uuid4().hex[:12], "synthetic_" + uuid.uuid4().hex
    source_user, project_user, report_user = (prefix + suffix for prefix in ("coin_source_", "coin_project_", "coin_report_"))
    roles, process, output, cuts, families = [], None, bytearray(), [], []
    started = time.monotonic()
    tracemalloc.start()
    try:
        with admin.cursor() as cursor:
            for user in (source_user, project_user, report_user):
                cursor.execute("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password))
                roles.append(user)
            for table in COIN_SOURCE_TABLES:
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (source_user,))
            for table in RETENTION_TABLES:
                cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (source_user,))
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(sealed) ON `" + database + "`.`" + CUT_TABLE + "` TO %s@'%%'", (source_user,))
            for table in (*PRIVATE_TABLES, *PUBLIC_TABLES):
                cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(phase) ON `" + database + "`.`" + GENERATION_TABLE + "` TO %s@'%%'", (project_user,))
            cursor.execute("GRANT UPDATE(complete) ON `" + database + "`.`" + COVERAGE_TABLE + "` TO %s@'%%'", (project_user,))
            for table in PUBLIC_TABLES:
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (report_user,))

        def factory(user):
            settings = ConnectionSettings(host="127.0.0.1", port=common["port"], database=database,
                user=user, password=password, read_timeout_s=3, write_timeout_s=3)
            return MeasuredFactory(PyMySQLConnectionFactory(settings))

        source, project, reporter = factory(source_user), factory(project_user), factory(report_user)
        env = dict(os.environ, CURRENCY_TEST_DB_NAME=database, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        process = subprocess.Popen(["bash", "-c", 'ulimit -s 65536 && exec "$1"', "native-coin", str(executable)],
            cwd=ROOT, env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        buffer = bytearray()
        expected = {"drop_pickup": (2, 2), "change": (1, 10), "pile_split_merge": (2, 8),
                    "peer": (1, 1), "split_recipients": (2, 6)}
        for family in expected:
            deadline, checkpoint = time.monotonic() + 120, None
            while checkpoint is None:
                if time.monotonic() >= deadline:
                    raise AssertionError("native coin checkpoint deadline: " + family)
                readable, _, _ = select.select([process.stdout], [], [], 1)
                if not readable:
                    continue
                chunk = os.read(process.stdout.fileno(), 65536)
                if not chunk:
                    raise AssertionError("native coin ended before " + family + ": " + output[-5000:].decode(errors="replace"))
                output.extend(chunk); buffer.extend(chunk)
                while b"\n" in buffer:
                    line, _, rest = buffer.partition(b"\n")
                    buffer = bytearray(rest)
                    if line.startswith(b"TELEMETRY_COIN_READY "):
                        parts = line.decode("ascii").split()
                        if parts[1] != family:
                            raise AssertionError("native coin checkpoint order")
                        checkpoint = tuple(bytes.fromhex(value) for value in parts[2:])
                        break
            count, volume = expected[family]
            if len(checkpoint) != count:
                raise AssertionError("native coin checkpoint inventory")
            cut = CanonicalCoinSnapshot(source).capture_coin_operations(checkpoint)
            repeated = CanonicalCoinSnapshot(source, page_size=1).capture_coin_operations(checkpoint)
            if len(cut.events) != count or sum(event.amount for event in cut.events) != volume or \
                any(event.disposition != c.Disposition.TRANSFER or event.identity.participant_pid != 0 for event in cut.events) or \
                c.earned_totals(cut.events) or cut.query_counts != (8,) or repeated.query_counts != (8,) * count:
                raise AssertionError("native coin root/child canonical transfer contract: " + family)
            if len(c.reconcile_events((*cut.events, *repeated.events))) != count:
                raise AssertionError("native coin overlap duplicated custody")
            from telemetry_canonical_publication_mysql import qualify_capture_cli, qualify_query_plans
            from scripts.telemetry.canonical_reward_coin_source import (
                _columns, ROOT_COLUMNS, INBOX_COLUMNS, CHILD_COLUMNS, LEDGER_COLUMNS, ITEM_LEDGER_COLUMNS,
            )
            qualify_capture_cli(source, cut, "wallet-pile")
            markers = ",".join("%s" for _ in cut.selected_operations)
            query_plans = qualify_query_plans(source, (
                ("coin_root", "SELECT " + _columns("r", "root_", ROOT_COLUMNS, "recorded_at") + "," +
                    _columns("i", "inbox_", INBOX_COLUMNS, "committed_at") +
                    " FROM economic_accounting_operation r LEFT JOIN critical_operation_inbox i "
                    "ON i.operation_id=r.operation_id WHERE r.operation_id IN (" + markers +
                    ") ORDER BY r.operation_id LIMIT %s", (*cut.selected_operations, len(cut.selected_operations) + 1)),
                ("coin_children", "SELECT " + _columns("ch", "child_", CHILD_COLUMNS) + "," +
                    _columns("i", "inbox_", INBOX_COLUMNS, "committed_at") + "," +
                    _columns("cu", "currency_", LEDGER_COLUMNS, "created_at") + "," +
                    _columns("it", "item_", ITEM_LEDGER_COLUMNS, "created_at") +
                    " FROM economic_accounting_child ch LEFT JOIN critical_operation_inbox i ON i.operation_id=ch.child_operation_id "
                    "LEFT JOIN currency_ledger cu ON cu.operation_id=ch.child_operation_id "
                    "LEFT JOIN item_ownership_ledger it ON it.operation_id=ch.child_operation_id "
                    "WHERE ch.operation_id IN (" + markers + ") ORDER BY ch.operation_id,ch.child_index,it.event_index LIMIT %s",
                    (*cut.selected_operations, 2 * len(cut.selected_operations) + 1)),
            ))
            for selected_cut in (cut, repeated):
                CanonicalRewardRetention(source).retain(selected_cut)
                restored = CanonicalRewardRetention(source).load(selected_cut.source_digest)
                if restored.events != selected_cut.events or restored.sources != selected_cut.sources:
                    raise AssertionError("native coin exact retained replay")
                cuts.append(selected_cut)
            gid = uuid.uuid4().bytes
            selection = tuple(sorted({cut.source_digest, repeated.source_digest}))
            generation = CanonicalRewardPublisher(project).stage(gid, selection)
            if len(generation.events) != count or generation.header["health_count"] != 0:
                raise AssertionError("native coin selected publication identity")
            CanonicalRewardPublisher(project).publish(gid)
            report = CanonicalRewardReporter(reporter).read(gid)
            if report["coverage"]["currency_earned_copper"] is not None or report["coverage"]["transfer_count"] != count or \
                any(row["amount"] <= 0 or row["award_utc_usec"] is not None for row in report["events"]):
                raise AssertionError("native coin custody/unknown attribution publication: " +
                    json.dumps(dict(family=family, earned=report["coverage"]["currency_earned_copper"],
                        transfers=report["coverage"]["transfer_count"], events=[dict(disposition=row["disposition"],
                        amount=row["amount"], award_utc_usec=row["award_utc_usec"]) for row in report["events"]]), sort_keys=True))
            families.append(dict(family=family, events=count, custody_copper=volume,
                source_query_plans=query_plans,
                source_rows=len(cut.sources), queries=list(cut.query_counts), reserved_bytes=cut.reserved_bytes,
                elapsed_seconds=cut.elapsed_seconds))
            print("PASS native canonical coin " + family, flush=True)
            if family == "peer":
                # A changed compatibility child must invalidate its parent even
                # when the original parent source cut is the selected generation.
                witness = next(row for row in cut.sources if row.reference.table == "currency_ledger")
                physical = c.decode_source_payload(witness.payload)
                child_op, prior = physical["operation_id"], physical["wallet_delta_copper"]
                with admin.cursor() as cursor:
                    cursor.execute("UPDATE currency_ledger SET wallet_delta_copper=wallet_delta_copper+1 WHERE operation_id=%s", (child_op,))
                try:
                    try:
                        CanonicalCoinSnapshot(source).capture_coin_operations(checkpoint)
                    except c.EvidenceError:
                        pass
                    else:
                        raise AssertionError("changed native compatibility child retained authority")
                    refused = CanonicalCoinSnapshot(source).capture_coin_operations(checkpoint, preserve_refusals=True)
                    if refused.events[0].amount is not None or refused.events[0].disposition != c.Disposition.UNKNOWN:
                        raise AssertionError("native coin disagreement quarantine")
                    CanonicalRewardRetention(source).retain(refused)
                    conflict_gid = uuid.uuid4().bytes
                    plan = CanonicalRewardPublisher(project).stage(conflict_gid, (cut.source_digest,))
                    if plan.events[0].disposition != c.Disposition.CONFLICT or plan.events[0].amount is not None:
                        raise AssertionError("native compatibility conflict did not reach its parent")
                    CanonicalRewardPublisher(project).publish(conflict_gid)
                    if CanonicalRewardReporter(reporter).read(gid) != report:
                        raise AssertionError("later child conflict changed an old generation")
                finally:
                    with admin.cursor() as cursor:
                        cursor.execute("UPDATE currency_ledger SET wallet_delta_copper=%s WHERE operation_id=%s", (prior, child_op))
            process.stdin.write(b"continue\n"); process.stdin.flush()
        tail, _ = process.communicate(timeout=120)
        output.extend(tail)
        if process.returncode:
            raise AssertionError("native coin fixture failed: " + output[-5000:].decode(errors="replace"))
        if b"PASS: activated SQL split children" not in output or b"AddressSanitizer" in output or b"runtime error:" in output:
            raise AssertionError("native coin sanitizer/native journey evidence missing")
        # The native owner has deleted its fixture receipts. Publication below
        # reconstructs only retained evidence, including the known child conflict.
        operations = tuple(event.identity.operation_id for cut in cuts for event in cut.events)
        with admin.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) AS count FROM economic_accounting_operation WHERE operation_id IN (" +
                ",".join("%s" for _ in operations) + ")", operations)
            if cursor.fetchone()["count"]:
                raise AssertionError("native coin cleanup did not remove source roots")
        selection = tuple(sorted({cut.source_digest for cut in cuts}))
        final_id = uuid.uuid4().bytes

        class InterruptedStage(CanonicalRewardPublisher):
            def _bulk(self, table, columns, rows):
                super()._bulk(table, columns, rows)
                if table == PRIVATE_EVENT_TABLE:
                    raise RuntimeError("injected coin staging interruption")

        try:
            InterruptedStage(project).stage(final_id, selection)
        except RuntimeError as error:
            if str(error) != "injected coin staging interruption":
                raise
        else:
            raise AssertionError("coin staging interruption not reached")
        with admin.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) AS count FROM " + GENERATION_TABLE + " WHERE generation_id=%s", (final_id,))
            if cursor.fetchone()["count"]:
                raise AssertionError("coin interrupted staging left a partial generation")

        class LostReplyFactory:
            def connect(self):
                connection = project.connect()
                original = connection.commit

                def commit():
                    original()
                    raise OSError("injected coin lost commit reply")

                connection.commit = commit
                return connection

            def close(self, connection=None):
                project.close(connection)

        try:
            CanonicalRewardPublisher(LostReplyFactory()).stage(final_id, selection)
        except AmbiguousCommit:
            pass
        else:
            raise AssertionError("coin lost stage reply did not require reconciliation")
        final_plan = CanonicalRewardPublisher(project).stage(final_id, selection)
        if len(final_plan.events) != 8 or sum(event.disposition == c.Disposition.CONFLICT for event in final_plan.events) != 1:
            raise AssertionError("retained native custody overlap/conflict conservation")

        class InterruptedPublish(CanonicalRewardPublisher):
            def _bulk(self, table, columns, rows):
                super()._bulk(table, columns, rows)
                if table == EVENT_TABLE:
                    try:
                        CanonicalRewardReporter(reporter).read(final_id)
                    except c.EvidenceError:
                        pass
                    else:
                        raise AssertionError("independent reader saw partial coin publication")
                    raise RuntimeError("injected coin publication interruption")

        try:
            InterruptedPublish(project).publish(final_id)
        except RuntimeError as error:
            if str(error) != "injected coin publication interruption":
                raise
        else:
            raise AssertionError("coin publication interruption not reached")
        try:
            CanonicalRewardPublisher(LostReplyFactory()).publish(final_id)
        except AmbiguousCommit:
            pass
        else:
            raise AssertionError("coin lost publication reply did not require reconciliation")
        CanonicalRewardPublisher(project).publish(final_id)
        report = CanonicalRewardReporter(reporter).read(final_id)
        if report["coverage"]["transfer_count"] != 7 or report["coverage"]["unknown_count"] != 1 or \
            report["coverage"]["earned_count"] != 0 or report["coverage"]["currency_earned_copper"] is not None:
            raise AssertionError("retained native custody publication counts")
        for user, sql in ((source_user, "UPDATE currency_ledger SET pid=pid WHERE 0"),
                          (project_user, "SELECT operation_id FROM economic_accounting_operation LIMIT 1"),
                          (report_user, "SELECT cut_id FROM " + CUT_TABLE + " LIMIT 1"),
                          (report_user, "UPDATE " + EVENT_TABLE + " SET event_index=event_index WHERE 0")):
            restricted = pymysql.connect(user=user, password=password, **common)
            try:
                with restricted.cursor() as cursor:
                    cursor.execute(sql)
            except pymysql.MySQLError as error:
                if error.args[0] != 1142:
                    raise
            else:
                raise AssertionError("native coin principal exceeded its authority")
            finally:
                restricted.close()
        cli_environment = dict(os.environ, DB_HOST="invalid-game-host", DB_NAME="invalid-game-database")
        for prefix, user in (("TELEMETRY_REWARD_PROJECT_DB_", project_user), ("TELEMETRY_REWARD_REPORT_DB_", report_user)):
            cli_environment.update({prefix + "HOST": "127.0.0.1", prefix + "PORT": os.environ["DB_PORT"],
                prefix + "USER": user, prefix + "PASSWD": password, prefix + "DATABASE": database})
        for mode in ("stage", "publish", "report"):
            arguments = [sys.executable, "-m", "scripts.telemetry.canonical_reward", mode, "--generation", final_id.hex()]
            if mode == "stage":
                for cut_id in selection:
                    arguments.extend(("--cut", cut_id.hex()))
            subprocess.run(arguments, cwd=ROOT, env=cli_environment, check=True, timeout=30,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        _, peak = tracemalloc.get_traced_memory()
        result = dict(status="passed", native_owner="typed_coin_transfer", families=families,
            events=8, earned_count=0, transfer_count=7, conflict_count=1,
            native_source_removed=True, old_generation_immutable=True,
            snapshot_and_exact_retention=True, stage_publication_faults_recovered=True,
            native_target_guard_refusals=6,
            actual_cli_and_restricted_roles=True, future_commits_provisional=True,
            actual_capture_cli=True,
            historical_backlog="unknown", award_dates="unknown", account_controller_attribution="unknown",
            native_binary_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
            maximum_generation_reserved_bytes=final_plan.header["reserved_bytes"],
            peak_traced_python_bytes=peak, elapsed_seconds=time.monotonic() - started,
            source_metrics=source.metrics, projection_metrics=project.metrics, report_metrics=reporter.metrics,
            production_or_staging_access=False)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps(result, sort_keys=True))
    finally:
        tracemalloc.stop()
        if process is not None and process.poll() is None:
            process.kill(); process.communicate(timeout=10)
        with admin.cursor() as cursor:
            for user in roles:
                cursor.execute("DROP USER %s@'%%'", (user,))
        admin.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compile-only", type=Path)
    parser.add_argument("--compiled-coin", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    binary = args.compile_only or args.compiled_coin
    if binary is None or not binary.resolve().is_relative_to((ROOT / "bin/tests").resolve()):
        parser.error("native coin binary must be explicitly inside bin/tests")
    if args.compile_only:
        if args.compiled_coin or args.report:
            parser.error("compile-only is independent of SQL qualification")
        binary.parent.mkdir(parents=True, exist_ok=True)
        compile_bank(binary.resolve(), real_pool=True, harness="coin")
        qualify_native_target_guard(binary.resolve())
    else:
        if args.report is None or not args.report.resolve().is_relative_to((ROOT / "bin/tests").resolve()):
            parser.error("qualification report must be explicitly inside bin/tests")
        qualify(binary.resolve(), args.report.resolve())


if __name__ == "__main__":
    main()
