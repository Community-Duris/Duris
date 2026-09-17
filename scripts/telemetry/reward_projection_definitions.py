"""Pure contracts for committed reward projection.

The projection is deliberately downstream of the authoritative ledgers.  A
source row may describe a reward outcome or a participant, but only a row
marked as an authority contributes an amount to an analytics report.  This is
what prevents an epic/currency ledger row from being counted again when the
same operation also appears in a PvP, zone, boon, or quest outcome table.

This module has no database dependency.  The SQL adapter imports these values
and the focused tests use the pure functions directly.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from enum import IntEnum
import hashlib
import json
import re
from types import MappingProxyType
from typing import Iterable, Mapping, Sequence


SCHEMA_VERSION = 1
PROJECTION_DEFINITION_VERSION = 1
DEFAULT_FAST_WINDOW_USEC = 15 * 60 * 1_000_000
DEFAULT_RECONCILIATION_OVERLAP_USEC = 5 * 60 * 1_000_000
DEFAULT_PAGE_SIZE = 256
MAX_PAGE_SIZE = 2_000
MAX_RECONCILIATION_CONNECTIONS = 1
MAX_QUERY_STATEMENTS_PER_PAGE = 8
MAX_REPORT_WINDOW_USEC = 31 * 24 * 60 * 60 * 1_000_000

REWARD_REPORT_DEFINITION = MappingProxyType({
    "name": "committed_reward_projection",
    "version": PROJECTION_DEFINITION_VERSION,
    "grain": "source_kind, reward_kind, status",
    "table": "telemetry_reward_projection",
    "metrics": (
        "gross_amount",
        "net_amount",
        "transfer_amount",
        "projected_source_count",
        "duplicate_source_count",
        "conflict_source_count",
        "unknown_context_count",
    ),
    "coverage": (
        "cycle_id",
        "cycle_high_water",
        "retention_floor",
        "acknowledged_through",
        "backlog_rows",
        "quality_flags",
        "provisional",
    ),
})


class SourceKind(IntEnum):
    CURRENCY_LEDGER = 1
    EPIC_LEDGER = 2
    ZONE_OUTCOME = 3
    ZONE_OUTCOME_PARTICIPANT = 4
    PVP_OUTCOME = 5
    PVP_OUTCOME_PARTICIPANT = 6
    BOON_OUTCOME = 7
    BOON_OUTCOME_ENTRY = 8
    QUEST_OUTCOME = 9
    COMBAT_FRAG_LEDGER = 10


class AuthorityKind(IntEnum):
    UNKNOWN = 0
    COMMITTED_LEDGER = 1
    OUTCOME_CONTEXT = 2


class RewardKind(IntEnum):
    NONE = 0
    CURRENCY = 1
    EPIC = 2
    FRAGS = 3


class ProjectionStatus(IntEnum):
    PROJECTED = 1
    DUPLICATE = 2
    CONFLICT = 3
    CONTEXT_ONLY = 4
    UNKNOWN_CONTEXT = 5


class ProjectionQuality(IntEnum):
    NONE = 0
    DUPLICATE_IDENTITY = 1 << 0
    CONFLICTING_IDENTITY = 1 << 1
    LATE_ARRIVAL = 1 << 2
    RETENTION_WARNING = 1 << 3
    INCOMPLETE_SOURCE_RANGE = 1 << 4
    UNKNOWN_CONTEXT = 1 << 5
    TRANSFER_NOT_CREATION = 1 << 6


_IDENTIFIER = re.compile(r"^[a-z][a-z0-9_]{1,63}$")
ZERO_OPERATION_ID = b"\x00" * 16


def normalize_operation_id(value: bytes | bytearray | memoryview | str) -> bytes:
    """Normalize a binary or hexadecimal operation id without accepting junk."""

    if isinstance(value, str):
        if len(value) != 32 or not re.fullmatch(r"[0-9a-fA-F]{32}", value):
            raise ValueError("operation_id must be 16 bytes or 32 hexadecimal characters")
        value = bytes.fromhex(value)
    elif isinstance(value, (bytearray, memoryview)):
        value = bytes(value)
    if not isinstance(value, bytes) or len(value) != 16 or value == ZERO_OPERATION_ID:
        raise ValueError("operation_id must be a nonzero 16-byte value")
    return value


@dataclass(frozen=True, order=True, slots=True)
class SourceCursor:
    """Keyset position; timestamps are a scan key, never commit order."""

    created_at_usec: int
    operation_id: bytes
    entry_index: int = 0
    participant_pid: int = 0

    def validate(self, *, allow_zero: bool = True) -> None:
        if self.created_at_usec < 0:
            raise ValueError("cursor timestamp cannot be negative")
        if allow_zero and self.operation_id == ZERO_OPERATION_ID:
            if self.entry_index == 0 and self.participant_pid == 0:
                # A timestamp plus the zero operation id is the inclusive
                # lower-bound sentinel used by reconciliation.  It is not a
                # gameplay operation and must never be persisted as one.
                return
            raise ValueError("zero operation cursor must have zero entry and participant")
        normalize_operation_id(self.operation_id)
        if not 0 <= self.entry_index <= 65_535:
            raise ValueError("cursor entry_index must fit SMALLINT UNSIGNED")
        if not 0 <= self.participant_pid <= 2**32 - 1:
            raise ValueError("cursor participant_pid must fit INT UNSIGNED")


@dataclass(frozen=True, slots=True)
class SourceObservation:
    """One immutable source observation selected by a read-only adapter."""

    source_kind: SourceKind
    operation_id: bytes
    entry_index: int
    participant_pid: int
    source_table: str
    created_at_usec: int
    authority_kind: AuthorityKind
    reward_kind: RewardKind = RewardKind.NONE
    gross_amount: int | None = None
    net_amount: int | None = None
    transfer_amount: int | None = None
    reason_type: int = 0
    reason_id: int = 0
    source_site: int = 0
    parent_operation_id: bytes | None = None
    ledger_operation_id: bytes | None = None
    is_transfer: bool = False
    is_creation: bool = False
    context_complete: bool = True

    def validate(self) -> None:
        try:
            source_kind = SourceKind(self.source_kind)
            authority_kind = AuthorityKind(self.authority_kind)
            reward_kind = RewardKind(self.reward_kind)
        except ValueError as error:
            raise ValueError("unknown source, authority, or reward kind") from error
        normalize_operation_id(self.operation_id)
        if not _IDENTIFIER.fullmatch(self.source_table):
            raise ValueError("source_table is not a safe identifier")
        if self.created_at_usec < 0:
            raise ValueError("created_at_usec cannot be negative")
        if not 0 <= self.entry_index <= 65_535:
            raise ValueError("entry_index must fit SMALLINT UNSIGNED")
        if not 0 <= self.participant_pid <= 2**32 - 1:
            raise ValueError("participant_pid must fit INT UNSIGNED")
        if self.reason_type < 0 or self.reason_type > 65_535:
            raise ValueError("reason_type must fit SMALLINT UNSIGNED")
        if self.source_site < 0 or self.source_site > 65_535:
            raise ValueError("source_site must fit SMALLINT UNSIGNED")
        for name, value in (
            ("parent_operation_id", self.parent_operation_id),
            ("ledger_operation_id", self.ledger_operation_id),
        ):
            if value is not None:
                normalize_operation_id(value)
                if value == self.operation_id and name == "parent_operation_id":
                    raise ValueError("parent operation cannot equal child operation")
        if authority_kind == AuthorityKind.COMMITTED_LEDGER:
            if reward_kind == RewardKind.NONE or self.gross_amount is None or self.net_amount is None:
                raise ValueError("committed ledger observations need a reward and amounts")
            if self.ledger_operation_id is not None:
                raise ValueError("a ledger authority is already its own ledger operation")
        elif self.gross_amount is not None or self.net_amount is not None:
            raise ValueError("outcome context cannot claim an authoritative amount")
        if self.transfer_amount is not None and not self.is_transfer:
            raise ValueError("transfer_amount requires is_transfer")
        if self.transfer_amount is not None and self.transfer_amount < 0:
            raise ValueError("transfer_amount cannot be negative")
        if self.is_transfer and self.is_creation:
            raise ValueError("a transfer is not creation")
        if self.is_creation and self.gross_amount is not None and self.gross_amount < 0:
            raise ValueError("creation amount cannot be negative")
        if source_kind in (
            SourceKind.CURRENCY_LEDGER, SourceKind.EPIC_LEDGER, SourceKind.COMBAT_FRAG_LEDGER,
        ) and authority_kind not in (
            AuthorityKind.COMMITTED_LEDGER,
            AuthorityKind.UNKNOWN,
        ):
            raise ValueError("ledger source kinds must be committed authorities")

    @property
    def identity(self) -> tuple[int, bytes, int, int]:
        self.validate()
        return (int(self.source_kind), normalize_operation_id(self.operation_id), self.entry_index, self.participant_pid)

    @property
    def economic_identity(self) -> tuple[int, bytes, int, int]:
        """Identity on which amounts are summed, excluding descriptive context rows."""

        operation = self.ledger_operation_id or self.operation_id
        return (int(self.reward_kind), normalize_operation_id(operation), self.entry_index, self.participant_pid)


@dataclass(frozen=True, slots=True)
class ProjectionRecord:
    source_kind: SourceKind
    operation_id: bytes
    entry_index: int
    participant_pid: int
    source_table: str
    created_at_usec: int
    authority_kind: AuthorityKind
    reward_kind: RewardKind
    gross_amount: int | None
    net_amount: int | None
    transfer_amount: int | None
    is_transfer: bool
    is_creation: bool
    status: ProjectionStatus
    quality_flags: int
    economic_operation_id: bytes
    parent_operation_id: bytes | None
    reason_type: int
    reason_id: int
    source_site: int
    context_complete: bool
    payload_digest: bytes

    @property
    def identity(self) -> tuple[int, bytes, int, int]:
        return (int(self.source_kind), self.operation_id, self.entry_index, self.participant_pid)

    @property
    def contributes_amount(self) -> bool:
        return (
            self.authority_kind == AuthorityKind.COMMITTED_LEDGER
            and self.status == ProjectionStatus.PROJECTED
            and self.gross_amount is not None
            and self.net_amount is not None
        )


@dataclass(frozen=True, slots=True)
class ProjectionBatch:
    records: tuple[ProjectionRecord, ...]
    duplicate_count: int
    conflict_count: int
    context_count: int
    authoritative_count: int

    @property
    def gross_total(self) -> int:
        return sum(record.gross_amount or 0 for record in self.records if record.contributes_amount)

    @property
    def net_total(self) -> int:
        return sum(record.net_amount or 0 for record in self.records if record.contributes_amount)

    @property
    def transfer_total(self) -> int:
        return sum(record.transfer_amount or 0 for record in self.records if record.contributes_amount)


def _record_from_observation(observation: SourceObservation, *, status: ProjectionStatus,
                             quality_flags: int = int(ProjectionQuality.NONE)) -> ProjectionRecord:
    operation_id = normalize_operation_id(observation.operation_id)
    economic_operation_id = normalize_operation_id(observation.ledger_operation_id or operation_id)
    return ProjectionRecord(
        source_kind=SourceKind(observation.source_kind),
        operation_id=operation_id,
        entry_index=observation.entry_index,
        participant_pid=observation.participant_pid,
        source_table=observation.source_table,
        created_at_usec=observation.created_at_usec,
        authority_kind=AuthorityKind(observation.authority_kind),
        reward_kind=RewardKind(observation.reward_kind),
        gross_amount=observation.gross_amount if status == ProjectionStatus.PROJECTED else None,
        net_amount=observation.net_amount if status == ProjectionStatus.PROJECTED else None,
        transfer_amount=observation.transfer_amount if status == ProjectionStatus.PROJECTED else None,
        is_transfer=observation.is_transfer,
        is_creation=observation.is_creation,
        status=status,
        quality_flags=quality_flags,
        economic_operation_id=economic_operation_id,
        parent_operation_id=(
            normalize_operation_id(observation.parent_operation_id)
            if observation.parent_operation_id is not None else None
        ),
        reason_type=observation.reason_type,
        reason_id=observation.reason_id,
        source_site=observation.source_site,
        context_complete=observation.context_complete,
        payload_digest=observation_payload_digest(observation),
    )


def _observation_payload(observation: SourceObservation) -> tuple[object, ...]:
    return (
        int(observation.source_kind), observation.operation_id, observation.entry_index,
        observation.participant_pid, observation.created_at_usec,
        int(observation.authority_kind), int(observation.reward_kind),
        observation.gross_amount, observation.net_amount, observation.transfer_amount,
        observation.reason_type, observation.reason_id, observation.source_site,
        observation.parent_operation_id, observation.ledger_operation_id,
        observation.is_transfer, observation.is_creation, observation.context_complete,
    )


def observation_payload_digest(observation: SourceObservation) -> bytes:
    """Return a stable digest for the non-key source payload.

    The digest is stored with the projection row so a replay can use one
    bounded upsert: equal payloads are harmless replays, while a different
    payload for the same source identity becomes a durable conflict instead
    of silently replacing the first observation.
    """

    observation.validate()
    payload = {
        "source_kind": int(observation.source_kind),
        "operation_id": normalize_operation_id(observation.operation_id).hex(),
        "entry_index": observation.entry_index,
        "participant_pid": observation.participant_pid,
        "source_table": observation.source_table,
        "created_at_usec": observation.created_at_usec,
        "authority_kind": int(observation.authority_kind),
        "reward_kind": int(observation.reward_kind),
        "gross_amount": observation.gross_amount,
        "net_amount": observation.net_amount,
        "transfer_amount": observation.transfer_amount,
        "reason_type": observation.reason_type,
        "reason_id": observation.reason_id,
        "source_site": observation.source_site,
        "parent_operation_id": (
            normalize_operation_id(observation.parent_operation_id).hex()
            if observation.parent_operation_id is not None else None
        ),
        "ledger_operation_id": (
            normalize_operation_id(observation.ledger_operation_id).hex()
            if observation.ledger_operation_id is not None else None
        ),
        "is_transfer": observation.is_transfer,
        "is_creation": observation.is_creation,
        "context_complete": observation.context_complete,
    }
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode("ascii")
    return hashlib.sha256(encoded).digest()


def project_observations(observations: Iterable[SourceObservation]) -> ProjectionBatch:
    """Deduplicate source identities and keep amount authority separate.

    Exact duplicate rows are restart/replay evidence and remain represented by
    one row.  Conflicting payloads fail closed as a conflict row with no amount.
    Context rows are retained for coverage and lineage, but never contribute to
    gross/net totals.  This is the parent/child and participant de-duplication
    boundary required by #270.
    """

    by_identity: dict[tuple[int, bytes, int, int], SourceObservation] = {}
    conflicted_identities: set[tuple[int, bytes, int, int]] = set()
    duplicate_count = 0
    conflict_count = 0
    for observation in observations:
        observation.validate()
        identity = observation.identity
        prior = by_identity.get(identity)
        if prior is None:
            by_identity[identity] = observation
            continue
        if _observation_payload(prior) == _observation_payload(observation):
            duplicate_count += 1
            continue
        conflict_count += 1
        conflicted_identities.add(identity)
        # Preserve the first stable identity, but make the amount unusable.
        by_identity[identity] = replace(
            prior,
            authority_kind=AuthorityKind.UNKNOWN,
            reward_kind=RewardKind.NONE,
            gross_amount=None,
            net_amount=None,
            transfer_amount=None,
            context_complete=False,
        )

    records: list[ProjectionRecord] = []
    context_count = 0
    authoritative_count = 0
    for observation in sorted(
        by_identity.values(),
        key=lambda item: (item.created_at_usec, item.operation_id, item.entry_index, item.participant_pid),
    ):
        if observation.identity in conflicted_identities:
            status = ProjectionStatus.CONFLICT
            quality = int(ProjectionQuality.CONFLICTING_IDENTITY)
        elif observation.authority_kind == AuthorityKind.COMMITTED_LEDGER:
            status = ProjectionStatus.CONFLICT if observation.reward_kind == RewardKind.NONE else ProjectionStatus.PROJECTED
            quality = int(ProjectionQuality.CONFLICTING_IDENTITY) if status == ProjectionStatus.CONFLICT else int(ProjectionQuality.NONE)
            authoritative_count += status == ProjectionStatus.PROJECTED
        else:
            context_count += 1
            status = (
                ProjectionStatus.CONTEXT_ONLY
                if observation.ledger_operation_id and observation.context_complete
                else ProjectionStatus.UNKNOWN_CONTEXT
            )
            quality = int(
                ProjectionQuality.NONE
                if status == ProjectionStatus.CONTEXT_ONLY
                else ProjectionQuality.UNKNOWN_CONTEXT
            )
        if observation.is_transfer:
            quality |= int(ProjectionQuality.TRANSFER_NOT_CREATION)
        records.append(_record_from_observation(observation, status=status, quality_flags=quality))
    return ProjectionBatch(tuple(records), duplicate_count, conflict_count, context_count, authoritative_count)


@dataclass(frozen=True, slots=True)
class ProjectionState:
    """Restartable state for one source kind and one reconciliation cycle."""

    source_kind: SourceKind
    cycle_id: int = 0
    fast_cursor: SourceCursor = SourceCursor(0, ZERO_OPERATION_ID)
    reconcile_cursor: SourceCursor = SourceCursor(0, ZERO_OPERATION_ID)
    cycle_high_water_usec: int = 0
    retention_floor_usec: int = 0
    acknowledged_through_usec: int = 0
    backlog_rows: int = 0
    quality_flags: int = int(ProjectionQuality.NONE)
    provisional: bool = True
    fast_started_at_usec: int = 0
    fast_completed_at_usec: int = 0
    reconcile_started_at_usec: int = 0
    reconcile_completed_at_usec: int = 0

    def validate(self) -> None:
        SourceKind(self.source_kind)
        if self.cycle_id < 0 or self.cycle_high_water_usec < 0 or self.retention_floor_usec < 0:
            raise ValueError("projection state counters/timestamps cannot be negative")
        if self.acknowledged_through_usec < 0 or self.backlog_rows < 0:
            raise ValueError("projection state acknowledgement/backlog cannot be negative")
        if any(value < 0 for value in (
            self.fast_started_at_usec, self.fast_completed_at_usec,
            self.reconcile_started_at_usec, self.reconcile_completed_at_usec,
        )):
            raise ValueError("projection state timestamps cannot be negative")
        if self.retention_floor_usec > self.acknowledged_through_usec and self.acknowledged_through_usec:
            raise ValueError("retention floor cannot pass the acknowledged reconciliation boundary")
        if self.quality_flags & ~sum(int(flag) for flag in ProjectionQuality):
            raise ValueError("unknown projection quality flag")
        self.fast_cursor.validate()
        self.reconcile_cursor.validate()

    def start_reconciliation(self, *, cycle_id: int, retention_floor_usec: int,
                             high_water_usec: int) -> "ProjectionState":
        if cycle_id <= self.cycle_id:
            raise ValueError("reconciliation cycle must advance monotonically")
        if retention_floor_usec < 0 or high_water_usec < retention_floor_usec:
            raise ValueError("reconciliation range is invalid")
        next_state = replace(
            self,
            cycle_id=cycle_id,
            reconcile_cursor=SourceCursor(retention_floor_usec, ZERO_OPERATION_ID),
            cycle_high_water_usec=high_water_usec,
            retention_floor_usec=retention_floor_usec,
            provisional=True,
            quality_flags=self.quality_flags,
            reconcile_started_at_usec=0,
            reconcile_completed_at_usec=0,
        )
        next_state.validate()
        return next_state

    def acknowledge_page(self, *, cursor: SourceCursor, rows_seen: int,
                         reconciliation: bool, page_complete: bool) -> "ProjectionState":
        if rows_seen < 0 or rows_seen > MAX_PAGE_SIZE:
            raise ValueError("rows_seen exceeds the bounded page contract")
        cursor.validate()
        if reconciliation:
            if page_complete:
                next_state = replace(
                    self,
                    reconcile_cursor=SourceCursor(self.cycle_high_water_usec, ZERO_OPERATION_ID),
                    acknowledged_through_usec=self.cycle_high_water_usec,
                    backlog_rows=0,
                    provisional=False,
                )
            else:
                next_state = replace(self, reconcile_cursor=cursor,
                                     backlog_rows=max(0, self.backlog_rows + rows_seen),
                                     provisional=True)
        else:
            next_state = replace(self, fast_cursor=cursor)
        next_state.validate()
        return next_state


def retention_warnings(state: ProjectionState, *, now_usec: int,
                       retention_horizon_usec: int) -> tuple[str, ...]:
    """Return explicit warnings before a source pruner may remove history."""

    state.validate()
    if retention_horizon_usec <= 0 or now_usec < retention_horizon_usec:
        raise ValueError("retention horizon must be a positive past timestamp")
    warnings: list[str] = []
    if state.acknowledged_through_usec < retention_horizon_usec:
        warnings.append("reconciliation_acknowledgement_behind_retention_floor")
    if state.provisional:
        warnings.append("projection_cycle_provisional")
    if state.backlog_rows:
        warnings.append("reconciliation_backlog_nonzero")
    if state.quality_flags & int(ProjectionQuality.INCOMPLETE_SOURCE_RANGE):
        warnings.append("source_range_incomplete")
    return tuple(warnings)


def currency_value(denominations: Sequence[int]) -> int:
    """Convert copper/silver/gold/platinum using the game's canonical weights."""

    if len(denominations) != 4:
        raise ValueError("currency requires four denominations")
    values = (1, 10, 100, 1000)
    total = 0
    for amount, weight in zip(denominations, values, strict=True):
        if not isinstance(amount, int):
            raise ValueError("currency denominations must be integers")
        total += amount * weight
    return total


__all__ = [
    "AuthorityKind", "DEFAULT_FAST_WINDOW_USEC", "DEFAULT_PAGE_SIZE",
    "DEFAULT_RECONCILIATION_OVERLAP_USEC", "MAX_PAGE_SIZE",
    "MAX_QUERY_STATEMENTS_PER_PAGE", "MAX_RECONCILIATION_CONNECTIONS",
    "MAX_REPORT_WINDOW_USEC", "ProjectionBatch", "ProjectionQuality",
    "ProjectionRecord", "ProjectionState", "ProjectionStatus",
    "PROJECTION_DEFINITION_VERSION", "REWARD_REPORT_DEFINITION", "RewardKind", "SCHEMA_VERSION",
    "SourceCursor", "SourceKind", "SourceObservation", "currency_value",
    "normalize_operation_id", "observation_payload_digest", "project_observations",
    "retention_warnings",
]
