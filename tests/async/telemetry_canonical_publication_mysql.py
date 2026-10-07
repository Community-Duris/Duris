"""Native selected bank publication, invoked only by the guarded SQL runner."""
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import tracemalloc
import uuid
from unittest.mock import patch

from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_publication import (
    CanonicalRewardPublisher, CanonicalRewardReporter, GENERATION_TABLE, BINDING_TABLE,
    PRIVATE_EVENT_TABLE, COVERAGE_TABLE, EVENT_TABLE, PRIVATE_TABLES, PRIVATE_HEALTH_TABLE, HEALTH_TABLE,
)
from scripts.telemetry.canonical_reward_conflicts import CanonicalRewardConflictReader
from scripts.telemetry.canonical_reward_health import CanonicalRewardHealthReader
from scripts.telemetry.canonical_reward_reconciliation import reconcile_once
from scripts.telemetry.reward_projection import AmbiguousCommit, ProjectionBoundsExceeded


class MeasuredFactory:
    """Payload-free worker diagnostics; each private principal owns one connection."""
    def __init__(self, factory):
        self.factory, self.settings = factory, factory.settings
        self.active = set()
        self.metrics = dict(connections=0, max_active_connections=0, statements=0, selects=0,
            max_select_rows=0, writes=0, max_write_rows=0, max_write_payload_bytes=0)

    def connect(self):
        connection = self.factory.connect()
        self.active.add(id(connection))
        self.metrics["connections"] += 1
        self.metrics["max_active_connections"] = max(self.metrics["max_active_connections"], len(self.active))
        if len(self.active) > 1:
            raise AssertionError("canonical worker widened its connection budget")
        original_cursor = connection.cursor

        def cursor():
            result = original_cursor()
            original_execute = result.execute

            def execute(statement, parameters=()):
                self.metrics["statements"] += 1
                value = original_execute(statement, parameters)
                if statement.startswith("SELECT"):
                    self.metrics["selects"] += 1
                    self.metrics["max_select_rows"] = max(self.metrics["max_select_rows"], result.rowcount)
                elif statement.startswith(("INSERT", "UPDATE")):
                    self.metrics["writes"] += 1
                    self.metrics["max_write_rows"] = max(self.metrics["max_write_rows"], result.rowcount)
                    payload_bytes = sum(len(cell) for cell in parameters if type(cell) is bytes)
                    self.metrics["max_write_payload_bytes"] = max(self.metrics["max_write_payload_bytes"], payload_bytes)
                    if payload_bytes > 512 * 1024:
                        raise AssertionError("native canonical write widened its payload budget")
                return value

            result.execute = execute
            return result

        connection.cursor = cursor
        return connection

    def close(self, connection):
        self.factory.close(connection)
        self.active.discard(id(connection))


def qualify_capture_cli(source_factory, cut, route):
    """Capture actual held native rows using only the dedicated source role."""
    from scripts.telemetry.canonical_reward_retention import CanonicalRewardRetention
    settings = source_factory.settings
    environment = dict(os.environ, DB_HOST="invalid-game-host", DB_NAME="invalid-game-database",
        TELEMETRY_REWARD_SOURCE_DB_HOST=settings.host, TELEMETRY_REWARD_SOURCE_DB_PORT=str(settings.port),
        TELEMETRY_REWARD_SOURCE_DB_DATABASE=settings.database, TELEMETRY_REWARD_SOURCE_DB_USER=settings.user,
        TELEMETRY_REWARD_SOURCE_DB_PASSWD=settings.password)
    arguments = [sys.executable, "-m", "scripts.telemetry.canonical_reward", "capture", "--route", route]
    for operation in cut.selected_operations:
        arguments.extend(("--operation", operation.hex()))
    completed = subprocess.run(arguments, cwd=Path(__file__).resolve().parents[2], env=environment,
        capture_output=True, text=True, timeout=30)
    if completed.returncode:
        raise AssertionError("native capture CLI failed: " + completed.stderr[-2000:])
    result = json.loads(completed.stdout)
    restored = CanonicalRewardRetention(source_factory).load(bytes.fromhex(result["cut_id"]))
    if restored.sources != cut.sources or restored.events != cut.events or result["source_queries"] != list(cut.query_counts):
        raise AssertionError("native capture CLI changed exact source or authority")
    return result


def qualify_query_plans(factory, queries):
    """Payload-free optimizer diagnostics for the owned native fixture only."""
    if not 0 < len(queries) <= 4:
        raise AssertionError("native query-plan diagnostic count")
    connection = factory.connect()
    try:
        result = {}
        with connection.cursor() as cursor:
            for name, statement, parameters in queries:
                if not statement.startswith("SELECT ") or ";" in statement or name in result:
                    raise AssertionError("native query-plan diagnostic scope")
                cursor.execute("EXPLAIN " + statement, parameters)
                rows = cursor.fetchall()
                if not 0 < len(rows) <= 16:
                    raise AssertionError("native query-plan diagnostic row bound")
                result[name] = [dict(table=row.get("table"), access=row.get("type"),
                    possible_keys=row.get("possible_keys"), selected_key=row.get("key"),
                    estimated_rows=row.get("rows"), extra=row.get("Extra")) for row in rows]
        return result
    finally:
        connection.rollback()
        factory.close(connection)


def before_conflict(projection_factory, report_factory, admin, cut):
    generation_id = uuid.uuid4().bytes
    selection = (cut.source_digest,)

    class InterruptedStage(CanonicalRewardPublisher):
        def _bulk(self, table, columns, rows):
            super()._bulk(table, columns, rows)
            if table == PRIVATE_EVENT_TABLE:
                raise RuntimeError("injected generation staging interruption")

    try:
        InterruptedStage(projection_factory).stage(generation_id, selection)
    except RuntimeError as error:
        if str(error) != "injected generation staging interruption":
            raise
    else:
        raise AssertionError("native generation staging interruption not reached")
    with admin.cursor() as cursor:
        cursor.execute("SELECT COUNT(*) AS count FROM " + GENERATION_TABLE + " WHERE generation_id=%s", (generation_id,))
        if cursor.fetchone()["count"]:
            raise AssertionError("interrupted stage committed partial generation metadata")

    class LostCommitFactory:
        def connect(self):
            connection = projection_factory.connect()
            original = connection.commit

            def lose_reply():
                original()
                raise OSError("injected lost reward generation commit reply")

            connection.commit = lose_reply
            return connection

        def close(self, connection=None):
            projection_factory.close(connection)

    try:
        CanonicalRewardPublisher(LostCommitFactory()).stage(generation_id, selection)
    except AmbiguousCommit:
        pass
    else:
        raise AssertionError("lost staging commit reply was not surfaced")
    recovered = CanonicalRewardPublisher(projection_factory).load(generation_id)
    if contract.earned_totals(recovered.events) != {contract.Unit.CURRENCY: 100}:
        raise AssertionError("native sealed generation did not recover exact earned authority")
    if CanonicalRewardPublisher(projection_factory).stage(generation_id, selection) != recovered:
        raise AssertionError("native generation replay changed its source snapshot")
    try:
        CanonicalRewardReporter(report_factory).read(generation_id)
    except contract.EvidenceError as error:
        if str(error) != "canonical_public_generation_unavailable":
            raise
    else:
        raise AssertionError("report principal read a staged generation")

    class InterruptedPublication(CanonicalRewardPublisher):
        def _bulk(self, table, columns, rows):
            super()._bulk(table, columns, rows)
            if table == EVENT_TABLE:
                # A real independent report connection must still see no complete
                # generation while the publication transaction is in progress.
                try:
                    CanonicalRewardReporter(report_factory).read(generation_id)
                except contract.EvidenceError as error:
                    if str(error) != "canonical_public_generation_unavailable":
                        raise
                else:
                    raise AssertionError("reporter observed an incomplete publication")
                raise RuntimeError("injected publication interruption")

    try:
        InterruptedPublication(projection_factory).publish(generation_id)
    except RuntimeError as error:
        if str(error) != "injected publication interruption":
            raise
    else:
        raise AssertionError("native publication interruption not reached")
    with admin.cursor() as cursor:
        for table in (COVERAGE_TABLE, EVENT_TABLE):
            cursor.execute("SELECT COUNT(*) AS count FROM " + table + " WHERE generation_id=%s", (generation_id,))
            if cursor.fetchone()["count"]:
                raise AssertionError("publication interruption left partial public rows")
    started = time.monotonic()
    try:
        CanonicalRewardPublisher(LostCommitFactory()).publish(generation_id)
    except AmbiguousCommit:
        pass
    else:
        raise AssertionError("lost publication commit reply was not surfaced")
    report = CanonicalRewardReporter(report_factory, page_size=2).read(generation_id)
    if report["coverage"]["currency_earned_copper"] != 100 or len(report["events"]) != 5:
        raise AssertionError("restricted report did not recover the complete native publication")
    if CanonicalRewardPublisher(projection_factory).publish(generation_id) != report:
        raise AssertionError("native published replay changed earlier generation bytes")
    return dict(generation_id=generation_id, report=report, publication_elapsed_seconds=time.monotonic() - started,
                reserved_bytes=recovered.header["reserved_bytes"])


def after_conflict(projection_factory, report_factory, admin, cut, prior, repeated, quarantined):
    generation_id = uuid.uuid4().bytes
    selection = tuple(sorted((cut.source_digest, repeated.source_digest, quarantined.source_digest)))
    plan = CanonicalRewardPublisher(projection_factory).stage(generation_id, selection)
    if len(plan.witnesses) != 2 or contract.earned_totals(plan.events) or len(plan.events) != 5 or len(plan.cuts) != 3:
        raise AssertionError("new generation omitted an external exact retained conflict")
    current = CanonicalRewardPublisher(projection_factory).publish(generation_id)
    report = CanonicalRewardReporter(report_factory).read(generation_id)
    if report != current or report["coverage"]["currency_earned_copper"] is not None or report["coverage"]["unknown_count"] != 1:
        raise AssertionError("new canonical publication hid its conflicted native reward")
    if report["coverage"]["opening_count"] != 1 or report["coverage"]["transfer_count"] != 3:
        raise AssertionError("canonical publication promoted bank custody or starter supply to earnings")
    if any(row[name] is not None for row in report["events"] for name in
           ("award_utc_usec", "account_token", "controller_token", "identity_registry_version")):
        raise AssertionError("native bank publication invented dated account/controller attribution")
    if CanonicalRewardReporter(report_factory).read(prior["generation_id"]) != prior["report"]:
        raise AssertionError("later conflict rewrote an earlier published generation")
    if CanonicalRewardPublisher(projection_factory).stage(prior["generation_id"], (cut.source_digest,)).header["projection_digest"] != prior["report"]["projection_digest"]:
        raise AssertionError("staging replay silently rebuilt an earlier published generation")
    try:
        CanonicalRewardPublisher(projection_factory).stage(prior["generation_id"], ())
    except contract.EvidenceError:
        pass
    else:
        raise AssertionError("generation replay accepted changed source selection")
    with admin.cursor() as cursor:
        for table, statement in ((GENERATION_TABLE, "SET snapshot_utc_usec=snapshot_utc_usec+1"),
            (BINDING_TABLE, "SET payload_digest=payload_digest"), (PRIVATE_EVENT_TABLE, "SET payload=payload"),
            (COVERAGE_TABLE, "SET snapshot_utc_usec=snapshot_utc_usec+1"), (EVENT_TABLE, "SET payload=payload")):
            try:
                cursor.execute("UPDATE " + table + " " + statement + " WHERE generation_id=%s", (generation_id,))
            except Exception as error:
                if error.args[0] != 1644:
                    raise
            else:
                raise AssertionError("sealed generation/publication allowed mutation")
    connection = report_factory.connect()
    try:
        with connection.cursor() as cursor:
            for statement in ["SELECT * FROM " + table + " LIMIT 1" for table in (*PRIVATE_TABLES, "telemetry_reward_source_v2", "economic_accounting_operation")] + [
                "UPDATE " + EVENT_TABLE + " SET payload=payload WHERE 0"]:
                try:
                    cursor.execute(statement)
                except Exception as error:
                    if error.args[0] != 1142:
                        raise
                else:
                    raise AssertionError("report principal widened into private evidence or writes")
    finally:
        report_factory.close(connection)
    connection = projection_factory.connect()
    try:
        with connection.cursor() as cursor:
            try:
                cursor.execute("SELECT * FROM economic_accounting_operation LIMIT 1")
            except Exception as error:
                if error.args[0] != 1142:
                    raise
            else:
                raise AssertionError("projection principal can read native gameplay authority")
    finally:
        projection_factory.close(connection)
    for bounds in (dict(byte_limit=4096 + 5 * 12288 - 1), dict(clock=iter((0, 11)).__next__)):
        try:
            CanonicalRewardReporter(report_factory, **bounds).read(generation_id)
        except ProjectionBoundsExceeded:
            pass
        else:
            raise AssertionError("public reward report ignored its byte/deadline budget")
    output = command_round_trip(projection_factory, report_factory, selection)
    if output["coverage"]["event_count"] != 5 or output["coverage"]["unknown_count"] != 1:
        raise AssertionError("native report CLI changed exact generation data or used the game connection")
    return dict(native_bank_publication_qualified=True, publication_stage_rollback=True,
        publication_stage_lost_reply_recovered=True, publication_atomic_visibility=True,
        publication_interruption_rollback=True, publication_lost_reply_recovered=True,
        publication_exact_replay=True, publication_global_conflicts_applied=True,
        publication_earlier_generation_immutable=True, publication_permissions_qualified=True,
        publication_unknown_dated_ownership=True, publication_report_bounds_refused=True,
        publication_overlapping_native_cuts_deduplicated=True, publication_command_stage_publish_report=True,
        publication_selected_quarantine_keeps_one_root=True,
        publication_command_dedicated_roles=True,
        publication_elapsed_seconds=prior["publication_elapsed_seconds"], publication_reserved_bytes=prior["reserved_bytes"])


def command_round_trip(projection_factory, report_factory, selection, *, scan_id=None):
    environment = dict(os.environ, DB_HOST="unused-game-connection", DB_USER="unused-game-user", DB_PASSWD="unused-game-password")
    for prefix, factory in (("TELEMETRY_REWARD_PROJECT_DB_", projection_factory), ("TELEMETRY_REWARD_REPORT_DB_", report_factory)):
        settings = factory.settings
        for name, value in (("HOST", settings.host), ("PORT", str(settings.port)), ("DATABASE", settings.database),
                            ("USER", settings.user), ("PASSWORD", settings.password)):
            environment[prefix + name] = value
    cli_generation = uuid.uuid4().bytes
    stage = ["stage", "--generation", cli_generation.hex()]
    for cut_id in selection:
        stage.extend(("--cut", cut_id.hex()))
    if scan_id is not None:
        stage.extend(("--scan", scan_id.hex()))
    cli_commands = (
        stage,
        ["publish", "--generation", cli_generation.hex()],
        ["report", "--generation", cli_generation.hex()],
    )
    for arguments in cli_commands:
        result = subprocess.run([sys.executable, "-m", "scripts.telemetry.canonical_reward", *arguments],
            env=environment, capture_output=True, text=True, timeout=20)
        if result.returncode:
            raise AssertionError("native canonical reward CLI failed: " + result.stderr[-1000:])
        output = json.loads(result.stdout)
    expected = CanonicalRewardReporter(report_factory).read(cli_generation)
    expected = json.loads(json.dumps(expected, default=lambda value: value.hex()))
    if output != expected:
        raise AssertionError("native report CLI changed exact generation data or used the game connection")
    return output


def during_health_gap(projection_factory, report_factory, admin, scan_id, selection, source_factory, observed):
    """Freeze a real missing native source row and a concurrent journal commit."""
    generation_id = uuid.uuid4().bytes
    projection_factory, report_factory = MeasuredFactory(projection_factory), MeasuredFactory(report_factory)
    tracemalloc.start()
    started = time.monotonic()

    class InterruptedHealthStage(CanonicalRewardPublisher):
        def _bulk(self, table, columns, rows):
            super()._bulk(table, columns, rows)
            if table == PRIVATE_HEALTH_TABLE:
                raise RuntimeError("injected health staging interruption")

    try:
        InterruptedHealthStage(projection_factory).stage(generation_id, selection, scan_id=scan_id)
    except RuntimeError as error:
        if str(error) != "injected health staging interruption":
            raise
    else:
        raise AssertionError("private health staging interruption not reached")
    with admin.cursor() as cursor:
        for table in (GENERATION_TABLE, PRIVATE_HEALTH_TABLE):
            cursor.execute("SELECT COUNT(*) AS count FROM " + table + " WHERE generation_id=%s", (generation_id,))
            if cursor.fetchone()["count"]:
                raise AssertionError("interrupted health stage left private rows")

    advanced = []

    class ConcurrentHealthCut(CanonicalRewardConflictReader):
        def _resolve_in_transaction(self, *args):
            result = super()._resolve_in_transaction(*args)
            if not advanced:
                advanced.append(reconcile_once(scan_id, source_factory(), source_factory(), page_size=2))
            return result

    with patch("scripts.telemetry.canonical_reward_publication.CanonicalRewardConflictReader", ConcurrentHealthCut):
        plan = CanonicalRewardPublisher(projection_factory, page_size=31).stage(generation_id, selection, scan_id=scan_id)
    coverage = contract.decode_source_payload(plan.header["coverage_payload"])
    if len(advanced) != 1 or coverage["sweep_revision"] != observed.revision or \
            coverage["sweep_retained_metadata_gaps"] != 1 or coverage["sweep_visited_prefixes"] != 256 or \
            coverage["sweep_snapshot_utc_usec"] != plan.snapshot_utc_usec:
        raise AssertionError("generation health crossed its owning conflict/evidence snapshot")
    current = CanonicalRewardHealthReader(source_factory()).read(scan_id)
    if current.revision != observed.revision + 1:
        raise AssertionError("concurrent journal commit did not occur after the generation cut")

    class InterruptedHealthPublication(CanonicalRewardPublisher):
        def _bulk(self, table, columns, rows):
            super()._bulk(table, columns, rows)
            if table == HEALTH_TABLE:
                try:
                    CanonicalRewardReporter(report_factory).read(generation_id)
                except contract.EvidenceError as error:
                    if str(error) != "canonical_public_generation_unavailable":
                        raise
                else:
                    raise AssertionError("reader observed health before atomic completion")
                raise RuntimeError("injected health publication interruption")

    try:
        InterruptedHealthPublication(projection_factory).publish(generation_id)
    except RuntimeError as error:
        if str(error) != "injected health publication interruption":
            raise
    else:
        raise AssertionError("public health interruption not reached")
    with admin.cursor() as cursor:
        for table in (COVERAGE_TABLE, EVENT_TABLE, HEALTH_TABLE):
            cursor.execute("SELECT COUNT(*) AS count FROM " + table + " WHERE generation_id=%s", (generation_id,))
            if cursor.fetchone()["count"]:
                raise AssertionError("interrupted health publication left public rows")

    class LostHealthReply(CanonicalRewardPublisher):
        def _commit(self):
            self._ensure_connection().commit()
            self.close()
            raise AmbiguousCommit("canonical health publication reply lost")

    try:
        LostHealthReply(projection_factory).publish(generation_id)
    except AmbiguousCommit:
        pass
    else:
        raise AssertionError("lost health publication acknowledgement was not surfaced")
    report = CanonicalRewardReporter(report_factory, page_size=31).read(generation_id)
    prefix = report["health"]["prefixes"][0]
    if prefix["retained_metadata_gap"] != 1 or prefix["last_capture_utc_usec"] is not None or prefix["last_status"] != 6:
        raise AssertionError("public health hid the actual retained native source gap")
    if CanonicalRewardPublisher(projection_factory).publish(generation_id) != report:
        raise AssertionError("health acknowledgement recovery changed publication")
    if report["coverage"]["currency_earned_copper"] != 17 or report["coverage"]["unknown_count"] != 1 or \
            report["coverage"]["event_count"] != 6:
        raise AssertionError("native late reward failed exact conflicted-history publication/readback")
    traced_peak = tracemalloc.get_traced_memory()[1]
    tracemalloc.stop()
    if traced_peak > 32 * 1024 * 1024 or projection_factory.active or report_factory.active:
        raise AssertionError("native health exceeded traced allocation or connection lifetime budget")
    return dict(generation_id=generation_id, report=report, reserved_bytes=plan.header["reserved_bytes"],
                elapsed_seconds=time.monotonic() - started, traced_python_peak_bytes=traced_peak,
                projection_queries=dict(projection_factory.metrics), report_queries=dict(report_factory.metrics))


def after_health_recovery(projection_factory, report_factory, admin, scan_id, selection, prior):
    generation_id = uuid.uuid4().bytes
    recovered = CanonicalRewardPublisher(projection_factory).stage(generation_id, selection, scan_id=scan_id)
    current = CanonicalRewardPublisher(projection_factory).publish(generation_id)
    report = CanonicalRewardReporter(report_factory, page_size=31).read(generation_id)
    if report != current or report["coverage"]["sweep_retained_metadata_gaps"] != 0 or \
            report["health"]["prefixes"][0]["last_capture_utc_usec"] is None:
        raise AssertionError("new health generation failed to record native sweep recovery")
    if CanonicalRewardReporter(report_factory).read(prior["generation_id"]) != prior["report"]:
        raise AssertionError("repaired retention or newer journal steps changed frozen public health")
    original = CanonicalRewardPublisher(projection_factory).stage(prior["generation_id"], selection, scan_id=scan_id)
    if original.header["health_projection_digest"] != prior["report"]["health_projection_digest"]:
        raise AssertionError("health stage replay consulted mutable current sweep state")
    for changed_scan in (None, uuid.uuid4().bytes):
        try:
            CanonicalRewardPublisher(projection_factory).stage(prior["generation_id"], selection, scan_id=changed_scan)
        except contract.EvidenceError:
            pass
        else:
            raise AssertionError("health replay accepted a changed sweep selection")
    with admin.cursor() as cursor:
        for table in (PRIVATE_HEALTH_TABLE, HEALTH_TABLE):
            try:
                cursor.execute("UPDATE " + table + " SET payload=payload WHERE generation_id=%s", (generation_id,))
            except Exception as error:
                if error.args[0] != 1644:
                    raise
            else:
                raise AssertionError("sealed health evidence allowed mutation")
    for factory, statements in (
        (report_factory, ["SELECT * FROM " + table + " LIMIT 1" for table in
            (PRIVATE_HEALTH_TABLE, "telemetry_reward_sweep_v2", "telemetry_reward_sweep_bucket_v2", "telemetry_reward_sweep_step_v2")]),
        (projection_factory, ["UPDATE telemetry_reward_sweep_v2 SET revision=revision WHERE 0"])):
        connection = factory.connect()
        try:
            with connection.cursor() as cursor:
                for statement in statements:
                    try:
                        cursor.execute(statement)
                    except Exception as error:
                        if error.args[0] != 1142:
                            raise
                    else:
                        raise AssertionError("published health widened restricted principal authority")
        finally:
            factory.close(connection)
    try:
        CanonicalRewardReporter(report_factory, byte_limit=4096 + (len(report["events"]) + 257) * 12288 - 1).read(generation_id)
    except ProjectionBoundsExceeded:
        pass
    else:
        raise AssertionError("public health ignored its joint reward/coverage byte budget")
    if recovered.header["reserved_bytes"] > 32 * 1024 * 1024:
        raise AssertionError("native health exceeded generation reservation")
    cli_report = command_round_trip(projection_factory, report_factory, selection, scan_id=scan_id)
    if cli_report["coverage"]["health_count"] != 257 or cli_report["coverage"]["sweep_retained_metadata_gaps"] != 0 or \
            cli_report["coverage"]["sweep_scan_id"] != scan_id.hex() or len(cli_report["health"]["prefixes"]) != 256 or \
            cli_report["coverage"]["currency_earned_copper"] != 17:
        raise AssertionError("native stage --scan failed to carry restricted complete coverage readback")
    return dict(publication_sweep_health_qualified=True, publication_health_same_snapshot=True,
        publication_retention_gap_visible=True, publication_health_interruption_rollback=True,
        publication_health_atomic_visibility=True, publication_health_lost_reply_recovered=True,
        publication_health_prior_generation_stable_after_recovery=True, publication_health_permissions_qualified=True,
        publication_health_bounds_refused=True, publication_health_command_stage_publish_report=True,
        publication_late_lower_commit_reward_published=True, publication_late_reward_earned_copper=17,
        publication_health_reserved_bytes=prior["reserved_bytes"],
        publication_health_elapsed_seconds=prior["elapsed_seconds"],
        publication_health_traced_python_peak_bytes=prior["traced_python_peak_bytes"],
        publication_health_projection_queries=prior["projection_queries"], publication_health_report_queries=prior["report_queries"])
