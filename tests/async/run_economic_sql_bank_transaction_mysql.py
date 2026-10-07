#!/usr/bin/env python3
"""Compile/run the typed SQL bank journey in an explicitly disposable schema."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import time
import uuid
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))


def target_is_disposable(environment):
    port = environment.get("DB_PORT", "")
    return (environment.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") == "1" and
            environment.get("DB_HOST") == "127.0.0.1" and not environment.get("DB_SOCKET") and
            re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+", environment.get("DB_NAME", "")) and
            port.isdigit() and 1 <= int(port) <= 65535)


def compile_bank(executable, real_pool=False, *, harness="bank"):
    # Reuse the maintained repository link set and linked accounting adapters.
    script = (ROOT / "tests/async/run_currency_transaction_schema_mysql.sh").read_text()
    chunk = script.split("g++ -std=c++20", 1)[1].split(
        '"$ROOT/bin/tests/currency_transaction_mysql_harness"', 1)[0]
    files = re.findall(r"(?:tests|src)/[A-Za-z0-9_/.-]+\.(?:cpp|c)", chunk)[1:]
    owners = {"bank": "tests/async/economic_sql_bank_transaction_mysql_harness.cpp",
              "coin": "tests/async/currency_transaction_mysql_harness.cpp"}
    if harness not in owners:
        raise ValueError("unsupported native qualification owner")
    files += [owners[harness],
              "src/persistence/economic_accounting_repository.c",
              "src/persistence/economic_sql_bank_transaction.c",
              "src/persistence/economic_sql_collector_transaction.c",
              "src/economy/collector_accounting.c",
              "src/economy/economic_gameplay_authority.c",
              "src/economy/economic_currency_adapter.c", "src/economy/economic_accounting_types.c",
              "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_intent.c",
              "src/sql/item_extra_descr_codec.c",
              "tests/async/item_extra_descr_codec_sql_escape_stub.cpp",
              "src/persistence/sql_room_item_payload.c"]
    if real_pool:
        files.append("src/sql/sql_pool.c")
    files = list(dict.fromkeys(files))
    flags = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-pthread", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
             "-Isrc", "-DDURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST",
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
    flags += ["-Wl,--wrap=mysql_real_query,--wrap=mysql_errno"]
    if harness == "coin":
        flags.append("-DDURIS_TELEMETRY_COIN_QUALIFICATION")
    if real_pool:
        flags += ["-DDURIS_ECONOMIC_SQL_REAL_POOL_TEST",
                  "-Wl,--wrap=sql_pool_acquire,--wrap=sql_pool_release",
                  "-Wl,--wrap=sql_pool_replace_connection,--wrap=sql_pool_discard_connection"]
    flags += files + shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    subprocess.run(flags + ["-lcrypto", "-lz", "-o", str(executable)], cwd=ROOT,
                   check=True, timeout=600)


def run_bank(executable):
    subprocess.run(["bash", "-c", 'ulimit -s 65536 && exec "$1"', "economic-sql-bank", str(executable)],
                   cwd=ROOT, check=True, timeout=120,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))


def qualify_telemetry_source(executable, report_path=None, *, retain_sources=False, reconcile_history=False, publish_rewards=False):
    """Read actual native committed rows before the maintained producer cleans up.

    Only dedicated fixture principals are provisioned in the explicitly owned
    disposable instance. Source capture has SELECT only; the report principal
    cannot read native accounting evidence. This is not a publication gate.
    """
    import select
    import pymysql
    from scripts.telemetry import canonical_reward_contract as canonical
    from scripts.telemetry.canonical_reward_source import (
        CanonicalRewardSnapshot, SOURCE_TABLES, ProjectionBoundsExceeded,
    )
    from scripts.telemetry.canonical_reward_retention import (
        CanonicalRewardRetention, RETENTION_TABLES, CUT_TABLE, SOURCE_TABLE,
    )
    from scripts.telemetry.canonical_reward_conflicts import CanonicalRewardConflictReader
    from scripts.telemetry.canonical_reward_publication import PRIVATE_TABLES, PUBLIC_TABLES, GENERATION_TABLE, COVERAGE_TABLE, CanonicalRewardPublisher
    from scripts.telemetry.reward_projection import AmbiguousCommit
    from scripts.telemetry.canonical_reward_reconciliation import JOURNAL_TABLES, STEP_TABLE
    from scripts.telemetry.db_access import ConnectionSettings, PyMySQLConnectionFactory
    if os.name != "posix" or not target_is_disposable(os.environ):
        raise RuntimeError("canonical native source qualification requires owned disposable Linux loopback")
    database = os.environ["DB_NAME"]
    common = dict(host="127.0.0.1", port=int(os.environ["DB_PORT"]), database=database,
                  connect_timeout=3, read_timeout=3, write_timeout=3, autocommit=True,
                  charset="utf8mb4", cursorclass=pymysql.cursors.DictCursor)
    admin = pymysql.connect(user=os.environ["DB_USER"], password=os.environ["DB_PASSWD"], **common)
    suffix = uuid.uuid4().hex[:12]
    worker, reporter = "canonical_source_" + suffix, "canonical_report_" + suffix
    publisher = "reward_projection_" + suffix
    synthetic_password = "synthetic_" + uuid.uuid4().hex
    process = None
    producer_output = bytearray()
    roles = []
    try:
        with admin.cursor() as cursor:
            for principal in ((worker, reporter, publisher) if publish_rewards else (worker, reporter)):
                cursor.execute("CREATE USER %s@'%%' IDENTIFIED BY %s", (principal, synthetic_password))
                roles.append(principal)
            for table in SOURCE_TABLES:
                cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (worker,))
            if retain_sources:
                for table in RETENTION_TABLES:
                    cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (worker,))
                cursor.execute("GRANT UPDATE(sealed) ON `" + database + "`.`" + CUT_TABLE + "` TO %s@'%%'", (worker,))
            if reconcile_history:
                for table in JOURNAL_TABLES:
                    privileges = "SELECT,INSERT" if table == STEP_TABLE else "SELECT,INSERT,UPDATE"
                    cursor.execute("GRANT " + privileges + " ON `" + database + "`.`" + table + "` TO %s@'%%'", (worker,))
            cursor.execute("GRANT SELECT ON `" + database + "`.telemetry_reward_projection TO %s@'%%'", (reporter,))
            if publish_rewards:
                for table in (*RETENTION_TABLES, *(JOURNAL_TABLES if reconcile_history else ())):
                    cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (publisher,))
                for table in (*PRIVATE_TABLES, *PUBLIC_TABLES):
                    cursor.execute("GRANT SELECT,INSERT ON `" + database + "`.`" + table + "` TO %s@'%%'", (publisher,))
                cursor.execute("GRANT UPDATE(phase) ON `" + database + "`.`" + GENERATION_TABLE + "` TO %s@'%%'", (publisher,))
                cursor.execute("GRANT UPDATE(complete) ON `" + database + "`.`" + COVERAGE_TABLE + "` TO %s@'%%'", (publisher,))
                for table in PUBLIC_TABLES:
                    cursor.execute("GRANT SELECT ON `" + database + "`.`" + table + "` TO %s@'%%'", (reporter,))
        environment = dict(os.environ, DURIS_TELEMETRY_BANK_CHECKPOINT="1",
                           ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        process = subprocess.Popen(["bash", "-c", 'ulimit -s 65536 && exec "$1"',
                                    "economic-sql-bank", str(executable)], cwd=ROOT, env=environment,
                                   stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 120
        buffer, operations, ready = bytearray(), [], False
        while not ready:
            if time.monotonic() >= deadline:
                raise RuntimeError("native source checkpoint deadline")
            readable, _, _ = select.select([process.stdout], [], [], min(1, max(0, deadline - time.monotonic())))
            if not readable:
                continue
            chunk = os.read(process.stdout.fileno(), 65536)
            if not chunk:
                raise RuntimeError("native bank producer ended before source checkpoint")
            producer_output.extend(chunk)
            buffer.extend(chunk)
            while b"\n" in buffer:
                line, _, rest = buffer.partition(b"\n")
                buffer = bytearray(rest)
                if line.startswith(b"BANK_OP "):
                    operations.append(bytes.fromhex(line[8:].decode("ascii")))
                elif line == b"TELEMETRY_BANK_READY":
                    ready = True
        if len(operations) != 5 or len(set(operations)) != 5:
            raise AssertionError("actual native bank checkpoint operation inventory")
        settings = ConnectionSettings(host="127.0.0.1", port=common["port"], database=database,
                                      user=worker, password=synthetic_password, read_timeout_s=3, write_timeout_s=3)
        cut = CanonicalRewardSnapshot(PyMySQLConnectionFactory(settings)).capture_bank_operations(operations)
        repeated = CanonicalRewardSnapshot(PyMySQLConnectionFactory(settings)).capture_bank_operations(operations)
        expected = {canonical.Disposition.EARNED: 1, canonical.Disposition.OPENING: 1,
                    canonical.Disposition.TRANSFER: 3}
        counts = {kind: sum(event.disposition == kind for event in cut.events) for kind in expected}
        if counts != expected or canonical.earned_totals(cut.events) != {canonical.Unit.CURRENCY: 100}:
            raise AssertionError("actual native quest/bank/starter canonical classification")
        replay = canonical.reconcile_events((*cut.events, *repeated.events))
        if len(replay) != 5 or canonical.earned_totals(replay) != {canonical.Unit.CURRENCY: 100}:
            raise AssertionError("actual native source replay inflated earnings")
        if cut.query_counts != (8,) or len(cut.sources) != 39 or not cut.future_commits_provisional:
            raise AssertionError("native selected cut coverage/query contract")
        from telemetry_canonical_publication_mysql import qualify_query_plans
        from scripts.telemetry.canonical_reward_source import EFFECT_COLUMNS
        quest = next(event for event in cut.events if event.disposition == canonical.Disposition.EARNED)
        bucket, partition_cursor = quest.identity.operation_id[0], canonical.ZERO_ID
        lower = bytes((bucket,)) + bytes(15)
        scope, upper_scope = "operation_id>=%s AND operation_id>%s", "operation_id>=%s"
        values, upper_values = [lower, partition_cursor], [lower]
        if bucket < 255:
            upper = bytes((bucket + 1,)) + bytes(15)
            scope += " AND operation_id<%s"
            upper_scope += " AND operation_id<%s"
            values.append(upper)
            upper_values.append(upper)
        supported = " AND outcome=1 AND result_code=0 AND writer_id IN (1,2,3,4) AND reason IN (1,5,8) "
        discovery_query = "SELECT operation_id,writer_id,reason," + ",".join(canonical.COUNTS) + \
            ",OCTET_LENGTH(canonical_plan) AS plan_bytes,OCTET_LENGTH(canonical_intent) AS intent_bytes," + \
            "(SELECT MAX(operation_id) FROM economic_accounting_operation WHERE " + upper_scope + supported + \
            ") AS pass_upper FROM economic_accounting_operation WHERE " + scope + supported + \
            "ORDER BY operation_id LIMIT %s"
        markers = ",".join("%s" for _ in operations)
        query_plans = qualify_query_plans(PyMySQLConnectionFactory(settings), (
            ("bank_discovery", discovery_query, (*upper_values, *values, 2)),
            ("bank_effects", "SELECT " + ",".join(EFFECT_COLUMNS) +
                " FROM economic_accounting_account_effect WHERE operation_id IN (" + markers +
                ") ORDER BY operation_id,account_index LIMIT %s", (*operations, 3 * len(operations) + 1)),
        ))
        discovered, discovery_queries = {}, []
        for _ in range(64):
            partition = CanonicalRewardSnapshot(PyMySQLConnectionFactory(settings), page_size=2).capture_bank_partition(
                bucket, partition_cursor)
            if not partition.future_commits_provisional or any(count > 8 for count in partition.cut.query_counts):
                raise AssertionError("native partition discovery silently cleared future/query uncertainty")
            discovery_queries.extend(partition.cut.query_counts)
            for event in partition.cut.events:
                if event.identity.operation_id in discovered:
                    raise AssertionError("native partition page duplicated an operation before wrap")
                discovered[event.identity.operation_id] = event
            if partition.wrapped:
                break
            if partition.cursor_after <= partition_cursor:
                raise AssertionError("native partition discovery failed to advance its bounded cursor")
            partition_cursor = partition.cursor_after
        else:
            raise AssertionError("native fixture partition exceeded bounded discovery invocations")
        if discovered.get(quest.identity.operation_id) != quest:
            raise AssertionError("native indexed partition discovery did not recover the exact quest reward")
        with pymysql.connect(user=worker, password=synthetic_password, **common) as restricted:
            try:
                with restricted.cursor() as cursor:
                    cursor.execute("UPDATE currency_ledger SET pid=pid WHERE 0")
            except pymysql.MySQLError as error:
                if error.args[0] != 1142:
                    raise
            else:
                raise AssertionError("source principal can mutate native authority")
        report_settings = ConnectionSettings(host="127.0.0.1", port=common["port"], database=database,
                                             user=reporter, password=synthetic_password)
        projection_settings = ConnectionSettings(host="127.0.0.1", port=common["port"], database=database,
            user=publisher, password=synthetic_password, read_timeout_s=3, write_timeout_s=3)
        try:
            CanonicalRewardSnapshot(PyMySQLConnectionFactory(report_settings)).capture_bank_operations(operations)
        except canonical.EvidenceError:
            pass
        else:
            raise AssertionError("report principal obtained native source evidence")
        with admin.cursor() as cursor:
            cursor.execute("UPDATE economic_accounting_coin_posting SET copper_value=copper_value+1 "
                           "WHERE operation_id=%s AND line_index=0", (quest.identity.operation_id,))
        quarantined = None
        try:
            try:
                CanonicalRewardSnapshot(PyMySQLConnectionFactory(settings)).capture_bank_operations(operations)
            except canonical.EvidenceError:
                pass
            else:
                raise AssertionError("native source payload disagreement was silently accepted")
            if retain_sources:
                quarantined = CanonicalRewardSnapshot(PyMySQLConnectionFactory(settings)).capture_bank_operations(
                    operations, preserve_refusals=True)
                unknown = [event for event in quarantined.events if event.disposition == canonical.Disposition.UNKNOWN]
                if len(unknown) != 1 or unknown[0].identity.operation_id != quest.identity.operation_id or unknown[0].amount is not None:
                    raise AssertionError("native disagreement acquired an earned amount instead of exact quarantine")
        finally:
            with admin.cursor() as cursor:
                cursor.execute("UPDATE economic_accounting_coin_posting SET copper_value=copper_value-1 "
                               "WHERE operation_id=%s AND line_index=0", (quest.identity.operation_id,))
        if canonical.earned_totals(cut.events) != {canonical.Unit.CURRENCY: 100}:
            raise AssertionError("retained cut changed after later native source mutation")
        retention_report = dict(durable_retention_qualified=False)
        sweep_report = dict(native_bank_sweep_qualified=False)
        publication_report = dict(native_bank_publication_qualified=False)
        if retain_sources:
            class InterruptedRetention(CanonicalRewardRetention):
                def _bulk(self, table, columns, rows):
                    super()._bulk(table, columns, rows)
                    if table == SOURCE_TABLE:
                        raise RuntimeError("injected interruption after retained inputs")

            try:
                InterruptedRetention(PyMySQLConnectionFactory(settings)).retain(cut)
            except RuntimeError as error:
                if str(error) != "injected interruption after retained inputs":
                    raise
            else:
                raise AssertionError("retention interruption was not injected")
            with admin.cursor() as cursor:
                cursor.execute("SELECT COUNT(*) AS count FROM " + CUT_TABLE + " WHERE cut_id=%s", (cut.source_digest,))
                if cursor.fetchone()["count"]:
                    raise AssertionError("interrupted retention left a partial durable cut")

            class LostCommitFactory(PyMySQLConnectionFactory):
                def connect(self):
                    connection = super().connect()
                    original_commit = connection.commit

                    def lose_reply():
                        original_commit()
                        raise OSError("injected lost retention commit reply")

                    connection.commit = lose_reply
                    return connection

            started = time.monotonic()
            try:
                CanonicalRewardRetention(LostCommitFactory(settings)).retain(cut)
            except AmbiguousCommit:
                pass
            else:
                raise AssertionError("retention lost acknowledgement was not surfaced")
            elapsed = time.monotonic() - started
            restored = CanonicalRewardRetention(PyMySQLConnectionFactory(settings), page_size=7).load(cut.source_digest)
            if restored.events != cut.events or restored.sources != cut.sources:
                raise AssertionError("exact retained cut could not recover after lost commit reply")
            if CanonicalRewardRetention(PyMySQLConnectionFactory(settings)).retain(cut) != cut.source_digest:
                raise AssertionError("retained cut replay did not retain exact identity")
            from telemetry_canonical_publication_mysql import qualify_capture_cli
            qualify_capture_cli(PyMySQLConnectionFactory(settings), cut, "bank")
            retention_report["actual_capture_cli"] = True
            if publish_rewards:
                CanonicalRewardRetention(PyMySQLConnectionFactory(settings)).retain(repeated)
            if publish_rewards:
                from telemetry_canonical_publication_mysql import before_conflict, after_conflict
                prior_publication = before_conflict(PyMySQLConnectionFactory(projection_settings),
                    PyMySQLConnectionFactory(report_settings), admin, cut)
            class ConcurrentConflictReader(CanonicalRewardConflictReader):
                inserted = False

                def _execute(self, statement, parameters=()):
                    if "idx_reward2_source_payloads" in statement and not self.inserted:
                        self.inserted = True
                        # Qualification-only second connection: a real sealed
                        # quarantine commits after the owning reader cut began.
                        CanonicalRewardRetention(PyMySQLConnectionFactory(settings)).retain(quarantined)
                    return super()._execute(statement, parameters)

            before_conflict = ConcurrentConflictReader(PyMySQLConnectionFactory(settings)).resolve(cut.source_digest)
            if before_conflict.witnesses or canonical.earned_totals(before_conflict.events) != {canonical.Unit.CURRENCY: 100}:
                raise AssertionError("retained conflict lookup mixed a later commit into its owning snapshot")
            global_conflict = CanonicalRewardConflictReader(PyMySQLConnectionFactory(settings)).resolve(cut.source_digest)
            if len(global_conflict.witnesses) != 1 or global_conflict.witnesses[0].cut_id != quarantined.source_digest:
                raise AssertionError("selected valid cut ignored an external retained quarantine")
            witness = global_conflict.witnesses[0]
            if witness.source not in quarantined.sources or canonical.earned_totals(global_conflict.events):
                raise AssertionError("global retained conflict lost exact bytes or retained an earned amount")
            conflicted = next(event for event in global_conflict.events if event.identity == quest.identity)
            if conflicted.disposition != canonical.Disposition.CONFLICT or conflicted.amount is not None or witness.source.reference not in conflicted.references:
                raise AssertionError("global physical conflict did not invalidate the original canonical event")
            if global_conflict.conflict_query_count != len(cut.sources) + 1 or not global_conflict.future_evidence_provisional:
                raise AssertionError("global retained conflict query/coverage contract")
            for bounds in (dict(conflict_query_limit=1), dict(byte_limit=cut.reserved_bytes)):
                try:
                    CanonicalRewardConflictReader(PyMySQLConnectionFactory(settings), **bounds).resolve(cut.source_digest)
                except ProjectionBoundsExceeded:
                    pass
                else:
                    raise AssertionError("global conflict lookup returned partial authority after a bound")
            refused = CanonicalRewardRetention(PyMySQLConnectionFactory(settings)).load(quarantined.source_digest)
            if refused.sources != quarantined.sources or refused.events != quarantined.events:
                raise AssertionError("native conflicting payloads did not survive exact durable quarantine")
            reconciled = canonical.reconcile_events((*restored.events, *refused.events))
            if len(reconciled) != 5 or any(event.identity.participant_pid != 0 for event in reconciled):
                raise AssertionError("native bank quarantine duplicated its economic root as a participant event")
            if canonical.earned_totals(reconciled).get(canonical.Unit.CURRENCY, 0) != 0:
                raise AssertionError("native conflicting physical posting retained a qualified earned amount")
            with admin.cursor() as cursor:
                cursor.execute("SELECT COUNT(*) AS count FROM " + CUT_TABLE + " WHERE cut_id=%s", (cut.source_digest,))
                if cursor.fetchone()["count"] != 1:
                    raise AssertionError("retention replay duplicated the cut")
                for statement in (
                    "UPDATE " + SOURCE_TABLE + " SET payload=payload WHERE cut_id=%s",
                    "DELETE FROM " + SOURCE_TABLE + " WHERE cut_id=%s",
                    "UPDATE " + CUT_TABLE + " SET captured_utc_usec=captured_utc_usec+1 WHERE cut_id=%s",
                ):
                    try:
                        cursor.execute(statement, (cut.source_digest,))
                    except pymysql.MySQLError as error:
                        if error.args[0] != 1644:
                            raise
                    else:
                        raise AssertionError("sealed retained evidence allowed mutation")
            try:
                CanonicalRewardRetention(PyMySQLConnectionFactory(report_settings)).load(cut.source_digest)
            except canonical.EvidenceError as error:
                if str(error) != "canonical_retention_snapshot_engines":
                    raise
            except pymysql.MySQLError as error:
                if error.args[0] != 1142:
                    raise
            else:
                raise AssertionError("report principal obtained private retained native evidence")
            for bounds in (dict(byte_limit=cut.reserved_bytes - 1), dict(clock=iter((0, 11)).__next__)):
                try:
                    CanonicalRewardRetention(PyMySQLConnectionFactory(settings), **bounds).load(cut.source_digest)
                except ProjectionBoundsExceeded:
                    pass
                else:
                    raise AssertionError("retained read exceeded its byte/deadline guard")
            retention_report.update(durable_retention_qualified=True, retention_elapsed_seconds=elapsed,
                                    retention_interruption_rollback=True, retention_lost_reply_recovered=True,
                                    retention_exact_replay=True, retention_sealed_mutations_refused=True,
                                    retention_report_access_refused=True, retention_byte_and_deadline_refused=True)
            retention_report.update(quarantine_exact_payloads_retained=True, quarantine_prior_amount_invalidated=True)
            retention_report.update(global_retained_conflict_qualified=True,
                global_conflict_later_commit_excluded=True, global_conflict_exact_witnesses=1,
                global_conflict_query_count=global_conflict.conflict_query_count,
                global_conflict_reserved_bytes=global_conflict.reserved_bytes,
                global_conflict_elapsed_seconds=global_conflict.elapsed_seconds,
                global_conflict_bounds_refused=True)
            if publish_rewards:
                publication_report = after_conflict(PyMySQLConnectionFactory(projection_settings),
                    PyMySQLConnectionFactory(report_settings), admin, cut, prior_publication, repeated, quarantined)
        if reconcile_history:
            from telemetry_canonical_reconciliation_mysql import qualify_native_sweep
            sweep_report = qualify_native_sweep(settings, process, producer_output, admin,
                publication_factories=(PyMySQLConnectionFactory(projection_settings), PyMySQLConnectionFactory(report_settings))
                    if publish_rewards else None, publication_cut=cut.source_digest)
        process.stdin.write(b"continue\n")
        process.stdin.flush()
        remaining, _ = process.communicate(timeout=120)
        producer_output.extend(remaining)
        if process.returncode:
            raise RuntimeError("native bank producer failed after source checkpoint")
        if retain_sources:
            after_cleanup = CanonicalRewardRetention(PyMySQLConnectionFactory(settings)).load(cut.source_digest)
            if after_cleanup.events != cut.events or after_cleanup.sources != cut.sources:
                raise AssertionError("durable authority replay depended on removed native source rows")
            retention_report["retention_replayed_after_native_source_cleanup"] = True
            if publish_rewards:
                retained_generation = CanonicalRewardPublisher(PyMySQLConnectionFactory(projection_settings)).load(prior_publication["generation_id"])
                if contract_totals := canonical.earned_totals(retained_generation.events):
                    if contract_totals != {canonical.Unit.CURRENCY: 100}:
                        raise AssertionError("published retained reward replay changed after native cleanup")
                else:
                    raise AssertionError("published retained authority depended on removed native rows")
                publication_report["publication_replayed_after_native_cleanup"] = True
        print(producer_output.decode("utf-8", errors="replace"), end="")
        report = dict(scope="actual native SQL bank owner; selected operations only",
                      binary_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                      operations=5, source_rows=39, query_counts=cut.query_counts,
                      reserved_bytes=cut.reserved_bytes, elapsed_seconds=cut.elapsed_seconds,
                      earned_currency_copper=100, transfers=3, opening_supply_copper=1_000_000_000,
                      source_digest=cut.source_digest.hex(), future_commits_provisional=True,
                      source_query_plans=query_plans,
                      native_partition_discovery_qualified=True, discovery_scope="one native fixture bank prefix",
                      discovery_query_counts=discovery_queries, discovery_operations=len(discovered),
                      source_principal_select_only=True, report_source_access_refused=True,
                      replay_and_payload_conflict=True, production_or_staging_access=False,
                      publication_qualified=False, historical_reconciliation_qualified=False, **retention_report, **sweep_report, **publication_report)
        if report_path:
            report_path.parent.mkdir(parents=True, exist_ok=True)
            report_path.write_bytes((json.dumps(report, sort_keys=True, indent=2) + "\n").encode("utf-8"))
        print("canonical native SQL source qualification: " + json.dumps(report, sort_keys=True))
    except Exception:
        if producer_output:
            print(producer_output.decode("utf-8", errors="replace"), end="", file=sys.stderr)
        raise
    finally:
        if process is not None and process.poll() is None:
            process.kill()
            process.wait(timeout=10)
        try:
            with admin.cursor() as cursor:
                for principal in roles:
                    cursor.execute("DROP USER IF EXISTS %s@'%%'", (principal,))
        finally:
            admin.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--real-pool", action="store_true",
                        help="link the production SQL pool; keep the entire bank matrix")
    parser.add_argument("--compile-only", type=Path, metavar="BINARY",
                        help="compile a persistent native binary without connecting to a database")
    parser.add_argument("--compiled-bank", type=Path, metavar="BINARY",
                        help="reuse an explicitly prepared binary under bin/tests")
    parser.add_argument("--telemetry-source", action="store_true",
                        help="qualify selected canonical receipts through the actual native SQL bank owner")
    parser.add_argument("--telemetry-retention", action="store_true",
                        help="also qualify the prepared private definition-2 retained source schema")
    parser.add_argument("--telemetry-reconciliation", action="store_true",
                        help="also qualify private fair bank sweeps and a real late native issuance")
    parser.add_argument("--telemetry-publication", action="store_true",
                        help="also qualify independent canonical reward generations and restricted reports")
    parser.add_argument("--telemetry-source-report", type=Path,
                        help="save a payload-free qualification report under bin/tests")
    args = parser.parse_args()
    if args.telemetry_retention and not args.telemetry_source:
        parser.error("--telemetry-retention requires --telemetry-source")
    if args.telemetry_reconciliation and not args.telemetry_retention:
        parser.error("--telemetry-reconciliation requires --telemetry-retention")
    if args.telemetry_publication and not args.telemetry_retention:
        parser.error("--telemetry-publication requires --telemetry-retention")
    if args.compile_only:
        args.compile_only.parent.mkdir(parents=True, exist_ok=True)
        compile_bank(args.compile_only.resolve(), args.real_pool)
        return
    if (not target_is_disposable(os.environ) or
            (args.real_pool and os.environ.get("TEST_DB_DISPOSABLE") != "1")):
        raise SystemExit("explicit disposable loopback schema and port required")
    work = ROOT / "bin/tests/economic-sql-bank"
    work.mkdir(parents=True, exist_ok=True)
    if args.telemetry_source_report:
        args.telemetry_source_report.resolve().relative_to((ROOT / "bin/tests").resolve())
    if args.compiled_bank:
        executable = args.compiled_bank.resolve()
        executable.relative_to((ROOT / "bin/tests").resolve())
        if not executable.is_file():
            raise SystemExit("prepared native bank binary missing")
        if args.telemetry_source:
            qualify_telemetry_source(executable, args.telemetry_source_report, retain_sources=args.telemetry_retention,
                                     reconcile_history=args.telemetry_reconciliation, publish_rewards=args.telemetry_publication)
        else:
            run_bank(executable)
        return
    with tempfile.TemporaryDirectory(prefix="run-", dir=work) as temporary:
        executable = Path(temporary) / "bank"
        compile_bank(executable, args.real_pool)
        if args.telemetry_source:
            qualify_telemetry_source(executable, args.telemetry_source_report, retain_sources=args.telemetry_retention,
                                     reconcile_history=args.telemetry_reconciliation, publish_rewards=args.telemetry_publication)
        else:
            run_bank(executable)


if __name__ == "__main__":
    main()
