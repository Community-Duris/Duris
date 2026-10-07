"""Controlled native bank sweep qualification, called only by the guarded runner."""
import os
import select
import time
import uuid

from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_source import CanonicalRewardSnapshot
from scripts.telemetry.canonical_reward_retention import CanonicalRewardRetention, SOURCE_TABLE, SOURCE_COLUMNS
from scripts.telemetry.canonical_reward_conflicts import CanonicalRewardConflictReader
from scripts.telemetry.canonical_reward_health import CanonicalRewardHealthReader
from scripts.telemetry.canonical_reward_reconciliation import (
    CanonicalRewardJournal, SweepPosition, AttemptStatus, make_step, reconcile_once,
)
from scripts.telemetry.db_access import PyMySQLConnectionFactory
from scripts.telemetry.reward_projection import AmbiguousCommit


def qualify_native_sweep(settings, process, producer_output, admin, *, publication_factories=None, publication_cut=None):
    scan_id = uuid.uuid4().bytes
    factory = lambda: PyMySQLConnectionFactory(settings)
    CanonicalRewardJournal(factory()).initialize(scan_id)
    initial = CanonicalRewardJournal(factory()).position(scan_id)
    if initial != SweepPosition(scan_id, 0, 0):
        raise AssertionError("new native sweep initial position")
    late_id = None
    diagnostic_queries = 0

    def commit_late():
        nonlocal late_id
        process.stdin.write(b"late\n")
        process.stdin.flush()
        deadline, buffer = time.monotonic() + 30, bytearray()
        while time.monotonic() < deadline:
            readable, _, _ = select.select([process.stdout], [], [], 1)
            if not readable:
                continue
            chunk = os.read(process.stdout.fileno(), 65536)
            if not chunk:
                raise AssertionError("native late bank producer ended")
            producer_output.extend(chunk)
            buffer.extend(chunk)
            while b"\n" in buffer:
                line, _, remainder = buffer.partition(b"\n")
                buffer = bytearray(remainder)
                if line.startswith(b"LATE_BANK_OP "):
                    late_id = bytes.fromhex(line[13:].decode("ascii"))
                    if late_id != bytes(15) + b"\x01":
                        raise AssertionError("native late root did not use the proven lower key")
                    return
        raise AssertionError("native late root commit deadline")

    class ConcurrentSnapshot(CanonicalRewardSnapshot):
        def _execute(self, statement, parameters=()):
            nonlocal diagnostic_queries
            rows = super()._execute(statement, parameters)
            if "AS pass_upper" in statement and late_id is None:
                commit_late()
                # Supplemental isolation diagnostic, separate from the normal
                # eight-query page. It uses the same sole private connection.
                with self._connection.cursor() as cursor:
                    cursor.execute("SELECT COUNT(*) AS count FROM economic_accounting_operation WHERE operation_id=%s", (late_id,))
                    if cursor.fetchone()["count"] != 0:
                        raise AssertionError("owning native source snapshot observed a later commit")
                diagnostic_queries += 1
            return rows

    first = ConcurrentSnapshot(factory(), page_size=2).capture_bank_partition(0, preserve_refusals=True)
    if late_id in first.cut.selected_operations or not first.future_commits_provisional:
        raise AssertionError("late lower commit entered an earlier source cut")
    CanonicalRewardRetention(factory()).retain(first.cut)
    first_step = make_step(initial, page=first)

    class InterruptedJournal(CanonicalRewardJournal):
        def _execute(self, statement, parameters=()):
            result = super()._execute(statement, parameters)
            if statement.startswith("UPDATE telemetry_reward_sweep_bucket_v2"):
                raise RuntimeError("injected interruption after sweep bucket write")
            return result

    try:
        InterruptedJournal(factory()).advance(first_step)
    except RuntimeError as error:
        if str(error) != "injected interruption after sweep bucket write":
            raise
    else:
        raise AssertionError("native sweep interruption was not injected")
    if CanonicalRewardJournal(factory()).position(scan_id) != initial:
        raise AssertionError("interrupted native sweep advanced durable progress")

    class LostCommitFactory(PyMySQLConnectionFactory):
        def connect(self):
            connection = super().connect()
            original = connection.commit

            def lose_reply():
                original()
                raise OSError("injected lost sweep commit reply")

            connection.commit = lose_reply
            return connection

    try:
        CanonicalRewardJournal(LostCommitFactory(settings)).advance(first_step)
    except AmbiguousCommit:
        pass
    else:
        raise AssertionError("native sweep lost reply was not surfaced")
    if not CanonicalRewardJournal(factory()).advance(first_step):
        raise AssertionError("native sweep lost reply did not recover exact replay")
    changed = make_step(initial, failure=AttemptStatus.SOURCE_REFUSED, attempted_utc_usec=first_step.attempt_utc_usec)
    try:
        CanonicalRewardJournal(factory()).advance(changed)
    except contract.EvidenceError:
        pass
    else:
        raise AssertionError("native stale sweep overwrote a different committed step")

    collected, prefixes = list(first.cut.events), {0}
    nonempty_cuts = {first.cut.source_digest} if first.cut.events else set()
    started = time.monotonic()
    for expected_bucket in range(1, 256):
        current = CanonicalRewardJournal(factory()).position(scan_id)
        if current.bucket != expected_bucket:
            raise AssertionError("native sweep failed to rotate a fair prefix")
        step = reconcile_once(scan_id, factory(), factory(), page_size=2)
        prefixes.add(step.position.bucket)
        retained = CanonicalRewardRetention(factory()).load(step.cut_id)
        collected.extend(retained.events)
        if retained.events:
            nonempty_cuts.add(step.cut_id)
    after_round = CanonicalRewardJournal(factory()).position(scan_id)
    if after_round.bucket != 0 or after_round.revision != 256 or len(prefixes) != 256:
        raise AssertionError("native sweep did not retain a full fair dispatch round")
    # Complete the finite prefix pass if it had more than two old roots. Every
    # further dispatch still rotates all prefixes rather than starving them.
    for _ in range(8):
        for _ in range(256):
            step = reconcile_once(scan_id, factory(), factory(), page_size=2)
            retained = CanonicalRewardRetention(factory()).load(step.cut_id)
            collected.extend(retained.events)
            if retained.events:
                nonempty_cuts.add(step.cut_id)
            if any(event.identity.operation_id == late_id for event in retained.events):
                break
        else:
            continue
        break
    else:
        raise AssertionError("native lower-sorting late issuance was never revisited")
    canonical = contract.reconcile_events(collected)
    late = [event for event in canonical if event.identity.operation_id == late_id]
    if len(late) != 1 or late[0].disposition != contract.Disposition.EARNED or late[0].amount != 17:
        raise AssertionError("native late issuance was duplicated or lacked exact authority")
    if contract.earned_totals(canonical) != {contract.Unit.CURRENCY: 117}:
        raise AssertionError("native full-prefix sweep inflated issuance through custody or opening supply")
    globally_resolved = []
    for cut_id in sorted(nonempty_cuts):
        globally_resolved.extend(CanonicalRewardConflictReader(factory()).resolve(cut_id).events)
    if contract.earned_totals(globally_resolved) != {contract.Unit.CURRENCY: 17}:
        raise AssertionError("sweep ignored the external retained conflict of the original 100-copper issuance")
    publication_selection = None
    if publication_factories is not None:
        # Keep an independent exact late-reward cut available while a different
        # historical discovery cut undergoes the restore-loss injection below.
        late_cut = CanonicalRewardSnapshot(factory()).capture_bank_operations((late_id,))
        if len(late_cut.events) != 1 or contract.earned_totals(late_cut.events) != {contract.Unit.CURRENCY: 17}:
            raise AssertionError("recovered lower-sorting native reward lost its issuance authority")
        CanonicalRewardRetention(factory()).retain(late_cut)
        publication_selection = tuple(sorted((publication_cut, late_cut.source_digest)))

    class ClosedReaderFactory(PyMySQLConnectionFactory):
        def connect(self):
            connection = super().connect()
            original_cursor = connection.cursor

            def closing_cursor():
                cursor = original_cursor()
                original_execute = cursor.execute

                def execute(statement, parameters=()):
                    result = original_execute(statement, parameters)
                    if "AS pass_upper" in statement:
                        connection.close()
                    return result

                cursor.execute = execute
                return cursor

            connection.cursor = closing_cursor
            return connection

    before_outage = CanonicalRewardJournal(factory()).position(scan_id)
    failed = reconcile_once(scan_id, ClosedReaderFactory(settings), factory(), page_size=2)
    after_outage = CanonicalRewardJournal(factory()).position(scan_id)
    if failed.status != AttemptStatus.SOURCE_UNAVAILABLE or failed.cursor_after != before_outage.cursor or \
            after_outage.bucket != (before_outage.bucket + 1) % 256:
        raise AssertionError("native interrupted source lost its cursor or blocked fair dispatch")
    health = CanonicalRewardHealthReader(factory(), page_size=31).read(scan_id)
    if health.visited_prefixes != 256 or health.latest_failed_prefixes != 1 or health.retained_metadata_gaps or \
            health.known_source_backlog is not None or health.retention_acknowledged or not health.future_commits_provisional:
        raise AssertionError("native health view hid failed coverage or invented backlog/completeness")
    # Reach the known native late-root prefix through ordinary fair dispatch.
    # No administrative change jumps the journal to the target prefix.
    for _ in range(256):
        before_gap = CanonicalRewardJournal(factory()).position(scan_id)
        if before_gap.bucket == 0:
            break
        reconcile_once(scan_id, factory(), factory(), page_size=2)
    else:
        raise AssertionError("native gap qualification never reached its retained prefix")
    with admin.cursor() as cursor:
        cursor.execute("SELECT last_cut FROM telemetry_reward_sweep_bucket_v2 WHERE scan_id=%s AND bucket=0", (scan_id,))
        gap_cut_id = cursor.fetchone()["last_cut"]
        cursor.execute("SELECT cut_id," + ",".join(SOURCE_COLUMNS) + " FROM " + SOURCE_TABLE +
                       " WHERE cut_id=%s ORDER BY source_index LIMIT 1", (gap_cut_id,))
        saved_source = cursor.fetchone()
        if saved_source is None:
            raise AssertionError("native retained prefix lacked a physical source for gap qualification")
        original_triggers = {}
        for trigger in ("telemetry_reward2_source_delete", "telemetry_reward2_source_insert"):
            cursor.execute("SHOW CREATE TRIGGER " + trigger)
            original_triggers[trigger] = cursor.fetchone()["SQL Original Statement"]
    deleted = False
    try:
        # Explicit owned-schema restore-loss injection. Normal worker permissions
        # and guards have already proved that they cannot delete these bytes.
        with admin.cursor() as cursor:
            cursor.execute("DROP TRIGGER telemetry_reward2_source_delete")
            cursor.execute("DELETE FROM " + SOURCE_TABLE + " WHERE cut_id=%s AND source_index=%s",
                           (gap_cut_id, saved_source["source_index"]))
            if cursor.rowcount != 1:
                raise AssertionError("native retention-gap injection did not remove exactly one source")
            deleted = True

        class MustNotReadSource:
            def connect(self):
                raise AssertionError("retention gap attempted a new native source cut")

        gap_step = reconcile_once(scan_id, MustNotReadSource(), factory(), page_size=2)
        if gap_step.status != AttemptStatus.RETENTION_GAP or gap_step.cursor_after != before_gap.cursor or \
                gap_step.upper_after != before_gap.pass_upper or gap_step.next_bucket != 1:
            raise AssertionError("retention loss advanced source progress or stopped fair rotation")
        gap_health = CanonicalRewardHealthReader(factory()).read(scan_id)
        if gap_health.retained_metadata_gaps != 1 or gap_health.prefixes[0].last_status != AttemptStatus.RETENTION_GAP or \
                gap_health.prefixes[0].last_capture_utc_usec is not None:
            raise AssertionError("native retained source loss disappeared from bounded health")
        if publication_factories is not None:
            from telemetry_canonical_publication_mysql import during_health_gap
            published_gap = during_health_gap(*publication_factories, admin, scan_id, publication_selection,
                                             factory, gap_health)
    finally:
        with admin.cursor() as cursor:
            if deleted:
                cursor.execute("DROP TRIGGER telemetry_reward2_source_insert")
                columns = ("cut_id", *SOURCE_COLUMNS)
                cursor.execute("INSERT INTO " + SOURCE_TABLE + " (" + ",".join(columns) + ") VALUES (" +
                               ",".join("%s" for _ in columns) + ")", tuple(saved_source[name] for name in columns))
                cursor.execute(original_triggers["telemetry_reward2_source_insert"])
            cursor.execute(original_triggers["telemetry_reward2_source_delete"])
    CanonicalRewardRetention(factory()).load(gap_cut_id)
    publication_health = {}
    if publication_factories is not None:
        from telemetry_canonical_publication_mysql import after_health_recovery
        for _ in range(256):
            position = CanonicalRewardJournal(factory()).position(scan_id)
            reconcile_once(scan_id, factory(), factory(), page_size=2)
            if position.bucket == 0:
                break
        else:
            raise AssertionError("native health recovery never revisited its failed prefix")
        publication_health = after_health_recovery(*publication_factories, admin, scan_id, publication_selection, published_gap)
    return dict(native_bank_sweep_qualified=True, native_bank_sweep_prefixes=256,
        native_late_lower_commit_recovered=True, native_snapshot_later_commit_excluded=True,
        qualification_snapshot_probe_queries=diagnostic_queries, normal_page_query_limit=8,
        sweep_interruption_rollback=True, sweep_lost_reply_recovered=True, sweep_replay_conflict_refused=True,
        sweep_source_disconnect_preserved_cursor=True, sweep_earned_currency_copper=117,
        sweep_global_conflict_applied=True, sweep_after_retained_conflicts_copper=17,
        sweep_retention_gap_detected=True, sweep_retention_gap_source_cursor_preserved=True,
        sweep_retention_gap_exact_bytes_restored=True, sweep_health_qualified=True,
        sweep_health_query_count=health.query_count, sweep_health_reserved_bytes=health.reserved_bytes,
        sweep_source_backlog_unknown=True, sweep_source_retention_unacknowledged=True,
        sweep_elapsed_seconds=time.monotonic() - started, sweep_future_commits_provisional=True, **publication_health)
