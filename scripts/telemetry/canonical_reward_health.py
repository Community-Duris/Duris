"""Private bounded bank-sweep coverage and freshness, with unknown backlog.

This reads the 256 current prefix checkpoints and their latest exact steps. It
does not enumerate source history, acknowledge pruning, or turn an empty source
page into a complete population. Full retained payload verification occurs on
dispatch and explicit cut replay; this view checks retained metadata/counts.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
import hashlib

from . import canonical_reward_contract as contract
from .canonical_reward_reconciliation import CanonicalRewardJournal, STATE_TABLE, BUCKET_TABLE, STEP_TABLE, AttemptStatus
from .canonical_reward_retention import CUT_TABLE, SELECTION_TABLE, SOURCE_TABLE
from .canonical_reward_source import HEADER_BYTE_BOUND, INPUT_ROW_BYTE_BOUND, DEFAULT_BYTE_LIMIT, MAX_OPERATIONS
from .reward_projection import ProjectionBoundsExceeded


@dataclass(frozen=True, slots=True)
class PrefixHealth:
    bucket: int
    cursor: bytes
    pass_upper: bytes | None
    completed_passes: int
    failures: int
    last_revision: int
    last_status: AttemptStatus | None
    last_attempt_utc_usec: int | None
    last_capture_utc_usec: int | None
    retained_metadata_gap: bool


@dataclass(frozen=True, slots=True)
class SweepHealth:
    scan_id: bytes
    revision: int
    next_bucket: int
    read_snapshot_utc_usec: int
    prefixes: tuple[PrefixHealth, ...]
    reserved_bytes: int
    query_count: int
    coverage: str = "native_bank_owner_prefix_checkpoints"
    known_source_backlog: int | None = None
    source_retention_floor: bytes | None = None
    retention_acknowledged: bool = False
    future_commits_provisional: bool = True

    @property
    def visited_prefixes(self):
        return sum(prefix.last_revision > 0 for prefix in self.prefixes)

    @property
    def active_passes(self):
        return sum(prefix.cursor != contract.ZERO_ID for prefix in self.prefixes)

    @property
    def latest_failed_prefixes(self):
        return sum(prefix.last_status is not None and prefix.last_status >= AttemptStatus.SOURCE_REFUSED
                   for prefix in self.prefixes)

    @property
    def retained_metadata_gaps(self):
        return sum(prefix.retained_metadata_gap for prefix in self.prefixes)


@dataclass(frozen=True, slots=True)
class RetainedHealth:
    """Exact checkpoint/step/count observations, frozen with their SQL cut."""
    payloads: tuple[bytes, ...]
    health: SweepHealth


def replay_health(payloads, *, snapshot, byte_limit=DEFAULT_BYTE_LIMIT):
    contract.require(type(payloads) is tuple and len(payloads) == 257, "canonical_health_evidence_inventory")
    state = contract.decode_source_payload(payloads[0])
    contract.require(set(state) == {"scan_id", "revision", "next_bucket", "definition_version", "provisional", "read_utc_usec"},
                     "canonical_health_state_columns")
    scan_id = contract.identifier(state["scan_id"])
    revision = contract.integer(state["revision"])
    next_bucket = contract.integer(state["next_bucket"], upper=255)
    contract.require(state["definition_version"] == 2 and state["provisional"] == 1 and
        next_bucket == revision % 256 and state["read_utc_usec"] == snapshot,
        "canonical_health_retained_state")
    contract.integer(snapshot, lower=1)
    reserved = HEADER_BYTE_BOUND + 257 * 2 * INPUT_ROW_BYTE_BOUND
    if reserved > byte_limit:
        raise ProjectionBoundsExceeded("canonical health evidence byte budget")
    prefixes = []
    for bucket, payload in enumerate(payloads[1:]):
        row = contract.decode_source_payload(payload)
        contract.require(row["bucket"] == bucket and row["scan_id"] == scan_id, "canonical_health_prefix_order")
        expected_revision = revision - (revision - 1 - bucket) % 256 if revision > bucket else 0
        contract.require(row["last_revision"] == expected_revision, "canonical_health_prefix_fair_revision")
        prefixes.append(prefix_health(row, revision=revision))
    health = SweepHealth(scan_id, revision, next_bucket, snapshot, tuple(prefixes), reserved, 0)
    return RetainedHealth(payloads, health)


def public_health_rows(retained):
    """Expose validated coverage; raw step payloads remain private."""
    health = retained.health
    rows = [dict(scan_id=health.scan_id, revision=health.revision, next_bucket=health.next_bucket,
        snapshot_utc_usec=health.read_snapshot_utc_usec, definition_version=2,
        coverage=health.coverage, source_backlog=None, source_retention_floor=None,
        retention_acknowledged=0, future_commits_provisional=1)]
    for prefix in health.prefixes:
        rows.append(dict(bucket=prefix.bucket, cursor=prefix.cursor, pass_upper=prefix.pass_upper,
            completed_passes=prefix.completed_passes, failures=prefix.failures,
            last_revision=prefix.last_revision, last_status=None if prefix.last_status is None else int(prefix.last_status),
            last_attempt_utc_usec=prefix.last_attempt_utc_usec, last_capture_utc_usec=prefix.last_capture_utc_usec,
            retained_metadata_gap=int(prefix.retained_metadata_gap)))
    return tuple(contract.encode_source_payload(row) for row in rows)


def decode_public_health(payloads, *, snapshot):
    contract.require(type(payloads) is tuple and len(payloads) == 257, "canonical_public_health_inventory")
    state = contract.decode_source_payload(payloads[0])
    contract.require(set(state) == {"scan_id", "revision", "next_bucket", "snapshot_utc_usec", "definition_version",
        "coverage", "source_backlog", "source_retention_floor", "retention_acknowledged", "future_commits_provisional"},
        "canonical_public_health_state_columns")
    contract.identifier(state["scan_id"])
    revision = contract.integer(state["revision"])
    contract.require(state["next_bucket"] == revision % 256 and state["snapshot_utc_usec"] == snapshot and
        state["definition_version"] == 2 and state["coverage"] == "native_bank_owner_prefix_checkpoints" and
        state["source_backlog"] is None and state["source_retention_floor"] is None and
        state["retention_acknowledged"] == 0 and state["future_commits_provisional"] == 1,
        "canonical_public_health_provisional_state")
    prefixes = []
    fields = {"bucket", "cursor", "pass_upper", "completed_passes", "failures", "last_revision", "last_status",
              "last_attempt_utc_usec", "last_capture_utc_usec", "retained_metadata_gap"}
    for bucket, payload in enumerate(payloads[1:]):
        row = contract.decode_source_payload(payload)
        contract.require(set(row) == fields and row["bucket"] == bucket, "canonical_public_health_prefix_columns")
        cursor = contract.identifier(row["cursor"], zero=True)
        upper = row["pass_upper"]
        contract.require(upper is None if cursor == contract.ZERO_ID else
            contract.identifier(upper)[0] == cursor[0] == bucket and cursor <= upper,
            "canonical_public_health_pass_range")
        last_revision = contract.integer(row["last_revision"], upper=revision)
        expected_revision = revision - (revision - 1 - bucket) % 256 if revision > bucket else 0
        contract.require(last_revision == expected_revision, "canonical_public_health_fair_revision")
        for name in ("completed_passes", "failures"):
            contract.integer(row[name])
        gap = contract.integer(row["retained_metadata_gap"], upper=1)
        status = None if row["last_status"] is None else AttemptStatus(contract.integer(row["last_status"], lower=1, upper=6))
        for name in ("last_attempt_utc_usec", "last_capture_utc_usec"):
            if row[name] is not None:
                contract.integer(row[name], lower=1)
        contract.require((status is None) == (row["last_attempt_utc_usec"] is None) and
            (last_revision > 0 or (cursor == contract.ZERO_ID and row["completed_passes"] == row["failures"] == 0 and
                status is None and row["last_capture_utc_usec"] is None and gap == 0)) and
            (status != AttemptStatus.RETENTION_GAP or gap == 1) and
            (last_revision == 0 or status is not None or gap == 1), "canonical_public_health_prefix_status")
        prefixes.append(row)
    return dict(state=state, prefixes=tuple(prefixes))


def health_summary(public):
    if public is None:
        return dict(sweep_scan_id=None, sweep_revision=None, sweep_next_bucket=None,
            sweep_visited_prefixes=None, sweep_unobserved_prefixes=None, sweep_active_passes=None,
            sweep_latest_failed_prefixes=None, sweep_retained_metadata_gaps=None, sweep_snapshot_utc_usec=None)
    state, prefixes = public["state"], public["prefixes"]
    visited = sum(row["last_revision"] > 0 for row in prefixes)
    return dict(sweep_scan_id=state["scan_id"], sweep_revision=state["revision"], sweep_next_bucket=state["next_bucket"],
        sweep_visited_prefixes=visited, sweep_unobserved_prefixes=256 - visited,
        sweep_active_passes=sum(row["cursor"] != contract.ZERO_ID for row in prefixes),
        sweep_latest_failed_prefixes=sum(row["last_status"] is not None and row["last_status"] >= 3 for row in prefixes),
        sweep_retained_metadata_gaps=sum(row["retained_metadata_gap"] for row in prefixes),
        sweep_snapshot_utc_usec=state["snapshot_utc_usec"])


def capture_health_in_transaction(reader, scan_id, *, snapshot=None, prior_bytes=0):
    """Use the caller's owning transaction and deadline; never open a connection."""
    contract.identifier(scan_id)
    reader._statement_count = 0
    state = reader._execute("SELECT revision,next_bucket,definition_version,provisional,"
        "TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00',UTC_TIMESTAMP(6)) AS read_utc_usec "
        "FROM " + STATE_TABLE + " WHERE scan_id=%s", (scan_id,))
    contract.require(len(state) == 1, "canonical_health_state_missing")
    state = dict(state[0], scan_id=scan_id)
    if snapshot is not None:
        state["read_utc_usec"] = snapshot
    payloads, queries = [contract.encode_source_payload(state)], 1
    reserved = HEADER_BYTE_BOUND + 257 * 2 * INPUT_ROW_BYTE_BOUND
    if reserved + prior_bytes > reader.byte_limit:
        raise ProjectionBoundsExceeded("canonical health evidence byte budget")
    for start in range(0, 256, reader.page_size):
        reader._statement_count = 0
        rows = reader._execute("SELECT b.*,p.status,p.attempted_utc_usec,p.payload,p.payload_digest,"
            "c.definition_version,c.sealed,c.captured_utc_usec,c.discovery_bucket,c.selected_count,c.source_count,c.reserved_bytes,"
            "(SELECT COUNT(*) FROM " + SELECTION_TABLE + " x WHERE x.cut_id=b.last_cut) AS actual_selected,"
            "(SELECT COUNT(*) FROM " + SOURCE_TABLE + " x WHERE x.cut_id=b.last_cut) AS actual_sources "
            "FROM " + BUCKET_TABLE + " b LEFT JOIN " + STEP_TABLE + " p ON p.scan_id=b.scan_id AND p.revision=b.last_revision "
            "LEFT JOIN " + CUT_TABLE + " c ON c.cut_id=b.last_cut "
            "WHERE b.scan_id=%s AND b.bucket>=%s ORDER BY b.bucket LIMIT %s",
            (scan_id, start, min(reader.page_size, 256 - start)))
        queries += 1
        contract.require(len(rows) == min(reader.page_size, 256 - start), "canonical_health_prefix_inventory")
        payloads.extend(contract.encode_source_payload(row) for row in rows)
    replay = replay_health(tuple(payloads), snapshot=state["read_utc_usec"], byte_limit=reader.byte_limit)
    reader._check_deadline()
    return RetainedHealth(replay.payloads, replace(replay.health, query_count=queries))


def prefix_health(row, *, revision):
    """Validate an exact latest step; metadata loss remains a visible gap."""
    bucket = contract.integer(row["bucket"], upper=255)
    cursor = contract.identifier(row["cursor_operation"], zero=True)
    upper = row["pass_upper"]
    if cursor == contract.ZERO_ID:
        contract.require(upper is None, "canonical_health_fresh_pass")
    else:
        contract.identifier(upper)
        contract.require(cursor[0] == upper[0] == bucket and cursor <= upper, "canonical_health_pass_range")
    passes, failures = contract.integer(row["completed_passes"]), contract.integer(row["failures"])
    last_revision = contract.integer(row["last_revision"], upper=revision)
    status, attempted = None, None
    gap = False
    if last_revision:
        try:
            payload = row["payload"]
            contract.require(type(payload) is bytes and hashlib.sha256(payload).digest() == row["payload_digest"],
                             "canonical_health_step_digest")
            step = contract.decode_source_payload(payload)
            status = AttemptStatus(contract.integer(row["status"], lower=1, upper=6))
            attempted = contract.integer(row["attempted_utc_usec"], lower=1)
            contract.require(step["scan_id"] == row["scan_id"] and step["revision"] == last_revision and
                step["definition_version"] == 2 and step["bucket"] == bucket and step["status"] == int(status) and
                step["cursor_after"] == cursor and step["upper_after"] == upper and
                step["completed_passes"] == passes and step["failures"] == failures and
                step["attempted_utc_usec"] == attempted and step["future_commits_provisional"] == 1,
                "canonical_health_step_fields")
            contract.require(step["cut_id"] == row["last_cut"] if status <= AttemptStatus.QUARANTINED
                             else step["cut_id"] is None, "canonical_health_step_cut")
        except (contract.EvidenceError, KeyError, TypeError):
            status, attempted, gap = None, None, True
    else:
        contract.require(cursor == contract.ZERO_ID and passes == failures == 0 and row["last_cut"] is None,
                         "canonical_health_initial_prefix")
    capture = None
    if row["last_cut"] is not None:
        try:
            contract.digest(row["last_cut"])
            capture = contract.integer(row["captured_utc_usec"], lower=1)
            contract.require(row["sealed"] == 1 and row["definition_version"] == 2 and
                row["discovery_bucket"] == bucket, "canonical_health_retained_cut")
            selected = contract.integer(row["selected_count"], upper=MAX_OPERATIONS)
            sources = contract.integer(row["source_count"], upper=contract.MAX_REFERENCES)
            reserved = contract.integer(row["reserved_bytes"], lower=HEADER_BYTE_BOUND, upper=DEFAULT_BYTE_LIMIT)
            contract.require(reserved >= HEADER_BYTE_BOUND + selected * 32 + sources * INPUT_ROW_BYTE_BOUND and
                row["actual_selected"] == selected and row["actual_sources"] == sources,
                "canonical_health_retained_counts")
        except contract.EvidenceError:
            capture, gap = None, True
    if status == AttemptStatus.RETENTION_GAP:
        gap = True
    return PrefixHealth(bucket, cursor, upper, passes, failures, last_revision, status, attempted, capture, gap)


class CanonicalRewardHealthReader(CanonicalRewardJournal):
    def read(self, scan_id):
        contract.identifier(scan_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            result = capture_health_in_transaction(self, scan_id).health
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()
