"""Fair bounded bank history progress over immutable retained source cuts.

Each dispatch visits one of 256 operation-key prefixes. A pass freezes its
visible key upper bound; it is not a commit watermark. Each page has its own
consistent cut, and all dispatches, including failures, advance the fair slot.
Failed source reads retain their cursor and uncertainty for a later retry.
"""
from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
import hashlib
import time

from . import canonical_reward_contract as contract
from .canonical_reward_source import BankPartitionPage, CanonicalRewardSnapshot, DiscoveryScope
from .canonical_reward_retention import CanonicalRewardRetention, CUT_TABLE
from .reward_projection import ProjectionBoundsExceeded
from .db_access import DatabaseAccessError

STATE_TABLE = "telemetry_reward_sweep_v2"
BUCKET_TABLE = "telemetry_reward_sweep_bucket_v2"
STEP_TABLE = "telemetry_reward_sweep_step_v2"
JOURNAL_TABLES = (STATE_TABLE, BUCKET_TABLE, STEP_TABLE)


class RetentionGap(contract.EvidenceError):
    """The current private progress depends on missing or invalid retained bytes."""


class AttemptStatus(IntEnum):
    CAPTURED = 1
    QUARANTINED = 2
    SOURCE_REFUSED = 3
    BOUND_EXCEEDED = 4
    SOURCE_UNAVAILABLE = 5
    RETENTION_GAP = 6


@dataclass(frozen=True, slots=True)
class SweepPosition:
    scan_id: bytes
    revision: int
    bucket: int
    cursor: bytes = contract.ZERO_ID
    pass_upper: bytes | None = None
    completed_passes: int = 0
    failures: int = 0

    def validate(self):
        contract.identifier(self.scan_id)
        contract.integer(self.revision)
        contract.integer(self.bucket, upper=255)
        contract.identifier(self.cursor, zero=True)
        contract.integer(self.completed_passes)
        contract.integer(self.failures)
        if self.cursor == contract.ZERO_ID:
            contract.require(self.pass_upper is None, "canonical_sweep_fresh_pass")
        else:
            contract.identifier(self.pass_upper)
            contract.require(self.cursor[0] == self.bucket == self.pass_upper[0] and
                             self.cursor <= self.pass_upper, "canonical_sweep_active_range")


@dataclass(frozen=True, slots=True)
class SweepStep:
    position: SweepPosition
    status: AttemptStatus
    attempt_utc_usec: int
    cut_id: bytes | None
    cursor_after: bytes
    upper_after: bytes | None
    completed_passes: int
    failures: int
    payload: bytes
    payload_digest: bytes

    @property
    def revision(self):
        return self.position.revision + 1

    @property
    def next_bucket(self):
        return (self.position.bucket + 1) % 256


def make_step(position, *, page: BankPartitionPage | None = None,
              failure: AttemptStatus | None = None, attempted_utc_usec=None):
    position.validate()
    contract.integer(position.revision + 1, lower=1)
    if page is not None:
        contract.require(failure is None and type(page) is BankPartitionPage and
                         page.bucket == position.bucket and page.cursor_before == position.cursor and
                         page.future_commits_provisional and page.cut.future_commits_provisional,
                         "canonical_sweep_page_request")
        contract.integer(page.page_limit, lower=1, upper=666)
        scope = page.cut.discovery
        contract.require(type(scope) is DiscoveryScope and scope.bucket == page.bucket and scope.cursor == page.cursor_before and
                         scope.page_limit == page.page_limit and scope.pass_upper == page.pass_upper and
                         scope.through == position.pass_upper, "canonical_sweep_discovery_request")
        contract.identifier(page.pass_upper, zero=True)
        selected = page.cut.selected_operations
        contract.require(len(selected) <= page.page_limit and selected == tuple(sorted(set(selected))) and
                         len(page.cut.events) == len(selected), "canonical_sweep_page_count")
        for operation in selected:
            contract.identifier(operation)
            contract.require(operation[0] == position.bucket and position.cursor < operation <= page.pass_upper,
                             "canonical_sweep_page_range")
        contract.require(position.pass_upper is None or page.pass_upper == position.pass_upper,
                         "canonical_sweep_changed_upper")
        wrapped = len(selected) < page.page_limit or selected[-1] == page.pass_upper
        expected_cursor = contract.ZERO_ID if wrapped else selected[-1]
        contract.require(type(page.wrapped) is bool and page.wrapped == wrapped and
                         page.cursor_after == expected_cursor, "canonical_sweep_page_cursor")
        unknown = sum(event.disposition in (contract.Disposition.UNKNOWN, contract.Disposition.CONFLICT)
                      for event in page.cut.events)
        status = AttemptStatus.QUARANTINED if unknown else AttemptStatus.CAPTURED
        cut_id, at = contract.digest(page.cut.source_digest), contract.integer(page.cut.captured_utc_usec, lower=1)
        upper_after = None if wrapped else page.pass_upper
        passes, failures = position.completed_passes + int(wrapped), position.failures
        detail = dict(page_limit=page.page_limit, pass_upper=page.pass_upper, wrapped=int(wrapped),
                      selected_count=len(selected), unknown_count=unknown,
                      first_operation=selected[0] if selected else None,
                      last_operation=selected[-1] if selected else None,
                      reserved_bytes=page.cut.reserved_bytes)
    else:
        contract.require(type(failure) is AttemptStatus and failure >= AttemptStatus.SOURCE_REFUSED,
                         "canonical_sweep_failure_status")
        status, cut_id, at = failure, None, contract.integer(attempted_utc_usec, lower=1)
        expected_cursor, upper_after = position.cursor, position.pass_upper
        passes, failures = position.completed_passes, position.failures + 1
        detail = dict(page_limit=0, pass_upper=position.pass_upper, wrapped=0, selected_count=None,
                      unknown_count=None, first_operation=None, last_operation=None, reserved_bytes=None)
    contract.integer(passes)
    contract.integer(failures)
    payload = contract.encode_source_payload(dict(definition_version=2, scan_id=position.scan_id,
        previous_revision=position.revision, revision=position.revision + 1, bucket=position.bucket,
        cursor_before=position.cursor, upper_before=position.pass_upper, cursor_after=expected_cursor,
        upper_after=upper_after, completed_passes=passes, failures=failures,
        status=int(status), attempted_utc_usec=at, cut_id=cut_id, future_commits_provisional=1, **detail))
    return SweepStep(position, status, at, cut_id, expected_cursor, upper_after, passes, failures,
                     payload, hashlib.sha256(payload).digest())


class CanonicalRewardJournal(CanonicalRewardRetention):
    """Private progress; never publish, repair balances, or lock native sources."""

    def _start(self, *, read_only):
        super()._start(read_only=read_only)
        rows = self._execute("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables "
            "WHERE table_schema=DATABASE() AND table_name IN (%s,%s,%s)", JOURNAL_TABLES)
        contract.require(len(rows) == 3 and {row["table_name"] for row in rows} == set(JOURNAL_TABLES) and
                         all(row["engine"] == "InnoDB" for row in rows), "canonical_sweep_snapshot_engines")

    def initialize(self, scan_id):
        contract.identifier(scan_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=False)
            self._statement_count = 0
            rows = self._execute("SELECT revision,next_bucket,provisional FROM " + STATE_TABLE +
                                 " WHERE scan_id=%s FOR UPDATE", (scan_id,))
            if not rows:
                self._execute("INSERT INTO " + STATE_TABLE +
                    " (scan_id,definition_version,revision,next_bucket,provisional) VALUES (%s,2,0,0,1)", (scan_id,))
                self._bulk(BUCKET_TABLE, ("scan_id", "bucket", "cursor_operation", "pass_upper", "completed_passes", "failures", "last_revision", "last_cut"),
                           [(scan_id, bucket, contract.ZERO_ID, None, 0, 0, 0, None) for bucket in range(256)])
            else:
                contract.require(len(rows) == 1 and rows[0]["provisional"] == 1, "canonical_sweep_state_identity")
            self._statement_count = 0
            counts = self._execute("SELECT COUNT(*) AS count FROM " + BUCKET_TABLE + " WHERE scan_id=%s", (scan_id,))
            contract.require(len(counts) == 1 and counts[0]["count"] == 256, "canonical_sweep_bucket_inventory")
            self._commit()
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def position(self, scan_id):
        contract.identifier(scan_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            self._statement_count = 0
            rows = self._execute("SELECT s.revision,s.next_bucket,b.cursor_operation,b.pass_upper,b.completed_passes,b.failures "
                "FROM " + STATE_TABLE + " s JOIN " + BUCKET_TABLE + " b ON b.scan_id=s.scan_id AND b.bucket=s.next_bucket "
                "WHERE s.scan_id=%s AND s.definition_version=2 AND s.provisional=1", (scan_id,))
            contract.require(len(rows) == 1, "canonical_sweep_position_missing")
            row = rows[0]
            result = SweepPosition(scan_id, row["revision"], row["next_bucket"], row["cursor_operation"],
                                   row["pass_upper"], row["completed_passes"], row["failures"])
            result.validate()
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def verify_progress_evidence(self, position):
        """Verify the current prefix's last retained cut before another dispatch.

        This bounds work to one known cut, never a history scan. Broken evidence
        must not be overwritten by a later successful source page. Failed attempts
        preserve last_cut, so the gap remains visible on subsequent fair visits.
        """
        position.validate()
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            self._statement_count = 0
            rows = self._execute("SELECT b.cursor_operation,b.pass_upper,b.completed_passes,b.failures,b.last_cut "
                "FROM " + STATE_TABLE + " s JOIN " + BUCKET_TABLE + " b ON b.scan_id=s.scan_id AND b.bucket=s.next_bucket "
                "WHERE s.scan_id=%s AND s.revision=%s AND s.next_bucket=%s",
                (position.scan_id, position.revision, position.bucket))
            contract.require(len(rows) == 1 and rows[0]["cursor_operation"] == position.cursor and
                rows[0]["pass_upper"] == position.pass_upper and rows[0]["completed_passes"] == position.completed_passes and
                rows[0]["failures"] == position.failures, "canonical_sweep_progress_changed")
            cut_id = rows[0]["last_cut"]
            if cut_id is not None:
                try:
                    contract.digest(cut_id)
                    header = self._header(cut_id)
                    contract.require(header is not None and header["sealed"] == 1,
                                     "canonical_retained_sealed_cut_missing")
                    retained = self._load_rows(header)
                    contract.require(retained.discovery is not None and retained.discovery.bucket == position.bucket,
                                     "canonical_sweep_retained_prefix")
                    scope, selected = retained.discovery, retained.selected_operations
                    wrapped = len(selected) < scope.page_limit or selected[-1] == scope.pass_upper
                    contract.require(position.cursor == (contract.ZERO_ID if wrapped else selected[-1]) and
                        position.pass_upper == (None if wrapped else scope.pass_upper),
                        "canonical_sweep_retained_cursor")
                except contract.EvidenceError as error:
                    raise RetentionGap("canonical_sweep_retention_gap") from error
            self._rollback()
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def advance(self, step):
        deadline = self.clock() + self.time_limit_s
        contract.require(type(step) is SweepStep and hashlib.sha256(step.payload).digest() == step.payload_digest,
                         "canonical_sweep_step_digest")
        # Validate all redundant SQL fields against the exact canonical payload.
        row = contract.decode_source_payload(step.payload)
        contract.require(row["scan_id"] == step.position.scan_id and row["previous_revision"] == step.position.revision and
            row["revision"] == step.revision and row["bucket"] == step.position.bucket and row["status"] == int(step.status) and
            row["cursor_after"] == step.cursor_after and row["upper_after"] == step.upper_after and row["cut_id"] == step.cut_id and
            row["completed_passes"] == step.completed_passes and row["failures"] == step.failures and
            row["attempted_utc_usec"] == step.attempt_utc_usec, "canonical_sweep_step_fields")
        step.position.validate()
        if step.cut_id is not None:
            remaining = deadline - self.clock()
            if remaining <= 0:
                raise ProjectionBoundsExceeded("canonical sweep derivation deadline")
            retained = CanonicalRewardRetention(self.connection_factory, byte_limit=self.byte_limit,
                                                time_limit_s=remaining, clock=self.clock).load(step.cut_id)
            scope = retained.discovery
            contract.require(scope is not None, "canonical_sweep_discovery_evidence_missing")
            page = BankPartitionPage(retained, scope.bucket, scope.cursor, step.cursor_after,
                                     bool(row["wrapped"]), scope.page_limit, scope.pass_upper)
            expected = make_step(step.position, page=page)
        else:
            expected = make_step(step.position, failure=step.status, attempted_utc_usec=step.attempt_utc_usec)
        contract.require(expected == step, "canonical_sweep_step_derivation")
        self._deadline = deadline
        try:
            self._start(read_only=False)
            self._statement_count = 0
            state = self._execute("SELECT revision,next_bucket FROM " + STATE_TABLE + " WHERE scan_id=%s FOR UPDATE", (step.position.scan_id,))
            contract.require(len(state) == 1, "canonical_sweep_state_missing")
            if state[0]["revision"] > step.position.revision:
                prior = self._execute("SELECT payload,payload_digest FROM " + STEP_TABLE + " WHERE scan_id=%s AND revision=%s",
                                      (step.position.scan_id, step.revision))
                contract.require(len(prior) == 1 and prior[0]["payload"] == step.payload and prior[0]["payload_digest"] == step.payload_digest,
                                 "canonical_sweep_replay_conflict")
                self._rollback()
                return True
            contract.require(state[0]["revision"] == step.position.revision and state[0]["next_bucket"] == step.position.bucket,
                             "canonical_sweep_stale_position")
            bucket = self._execute("SELECT cursor_operation,pass_upper,completed_passes,failures,last_cut FROM " + BUCKET_TABLE +
                " WHERE scan_id=%s AND bucket=%s FOR UPDATE", (step.position.scan_id, step.position.bucket))
            contract.require(len(bucket) == 1 and bucket[0]["cursor_operation"] == step.position.cursor and
                bucket[0]["pass_upper"] == step.position.pass_upper and bucket[0]["completed_passes"] == step.position.completed_passes and
                bucket[0]["failures"] == step.position.failures, "canonical_sweep_bucket_changed")
            if step.cut_id is not None:
                header = self._header(step.cut_id)
                contract.require(header is not None and header["sealed"] == 1 and header["captured_utc_usec"] == step.attempt_utc_usec and
                    header["selected_count"] == row["selected_count"] and header["reserved_bytes"] == row["reserved_bytes"],
                    "canonical_sweep_retained_cut_missing")
            self._statement_count = 0
            self._execute("INSERT INTO " + STEP_TABLE + " (scan_id,revision,bucket,status,attempted_utc_usec,cut_id,payload,payload_digest) "
                "VALUES (%s,%s,%s,%s,%s,%s,%s,%s)", (step.position.scan_id, step.revision, step.position.bucket, int(step.status),
                    step.attempt_utc_usec, step.cut_id, step.payload, step.payload_digest))
            last_cut = step.cut_id if step.cut_id is not None else bucket[0]["last_cut"]
            self._execute("UPDATE " + BUCKET_TABLE + " SET cursor_operation=%s,pass_upper=%s,completed_passes=%s,failures=%s,last_revision=%s,last_cut=%s "
                "WHERE scan_id=%s AND bucket=%s", (step.cursor_after, step.upper_after, step.completed_passes, step.failures,
                    step.revision, last_cut, step.position.scan_id, step.position.bucket))
            self._execute("UPDATE " + STATE_TABLE + " SET revision=%s,next_bucket=%s WHERE scan_id=%s AND revision=%s",
                (step.revision, step.next_bucket, step.position.scan_id, step.position.revision))
            self._commit()
            return False
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()


def reconcile_once(scan_id, source_factory, projection_factory, *, page_size=256, attempted_utc_usec=None,
                   time_limit_s=10.0, clock=time.monotonic):
    contract.require(type(time_limit_s) in (int, float) and 0 < time_limit_s <= 10, "canonical_sweep_time_budget")
    deadline = clock() + time_limit_s

    def remaining():
        value = deadline - clock()
        if value <= 0:
            raise ProjectionBoundsExceeded("canonical sweep invocation deadline")
        return value

    journal = CanonicalRewardJournal(projection_factory, time_limit_s=remaining(), clock=clock)
    position = journal.position(scan_id)
    journal.time_limit_s = remaining()
    try:
        journal.verify_progress_evidence(position)
    except RetentionGap:
        at = attempted_utc_usec if attempted_utc_usec is not None else time.time_ns() // 1000
        step = make_step(position, failure=AttemptStatus.RETENTION_GAP, attempted_utc_usec=at)
        journal.time_limit_s = remaining()
        journal.advance(step)
        return step
    try:
        page = CanonicalRewardSnapshot(source_factory, page_size=page_size, time_limit_s=min(6, remaining()), clock=clock).capture_bank_partition(
            position.bucket, position.cursor, through=position.pass_upper, preserve_refusals=True)
    except ProjectionBoundsExceeded:
        failure = AttemptStatus.BOUND_EXCEEDED
    except contract.EvidenceError:
        failure = AttemptStatus.SOURCE_REFUSED
    except Exception as error:
        if isinstance(error, DatabaseAccessError) or type(error).__module__.startswith("pymysql"):
            failure = AttemptStatus.SOURCE_UNAVAILABLE
        else:
            raise
    else:
        CanonicalRewardRetention(projection_factory, time_limit_s=remaining(), clock=clock).retain(page.cut)
        step = make_step(position, page=page)
        journal.time_limit_s = remaining()
        journal.advance(step)
        return step
    at = attempted_utc_usec if attempted_utc_usec is not None else time.time_ns() // 1000
    step = make_step(position, failure=failure, attempted_utc_usec=at)
    journal.time_limit_s = remaining()
    journal.advance(step)
    return step
