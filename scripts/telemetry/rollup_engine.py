"""Pure semantics and bounded orchestration for external telemetry rollups.

No function in this module reads live game state or mutates the raw telemetry
stream.  The database boundary is deliberately structural so the pure page
builder can be tested with ordinary fakes; the production implementation lives
in :mod:`db_access`.
"""
from __future__ import annotations

from dataclasses import dataclass, field, replace
from datetime import date, timedelta
import time
import math
from typing import Any, Callable, Mapping, MutableMapping, Protocol, Sequence, cast

try:  # Running as a package.
    from .rollup_definitions import (
        COHORT_DIMENSION_FIELDS,
        COUNTER_FIELDS,
        CheckpointContribution,
        MembershipContribution,
        ReportSnapshot,
        RollupCoverage,
        RollupTarget,
        ROLLUP_QUALITY_CHECKPOINT_CONFLICT,
        ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_DIMENSION_INVALID,
        ROLLUP_QUALITY_LATE_INPUT,
        ROLLUP_QUALITY_KNOWN_MASK,
        ROLLUP_QUALITY_LIFECYCLE_CONFLICT,
        ROLLUP_QUALITY_PROCESS_GAP,
        ROLLUP_QUALITY_SESSION_GAP,
        ROLLUP_QUALITY_UTC_BACKWARD,
        ROLLUP_QUALITY_UTC_FANOUT,
        ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_UNKNOWN,
        UINT64_MAX,
        UNKNOWN_DAY,
        UTC_UNKNOWN,
        report_definition,
        unknown_dimensions,
    )
except ImportError:  # Running scripts/telemetry/rollup.py directly.
    from rollup_definitions import (  # type: ignore[no-redef]
        COHORT_DIMENSION_FIELDS,
        COUNTER_FIELDS,
        CheckpointContribution,
        MembershipContribution,
        ReportSnapshot,
        RollupCoverage,
        RollupTarget,
        ROLLUP_QUALITY_CHECKPOINT_CONFLICT,
        ROLLUP_QUALITY_CONTEXT_UNAVAILABLE,
        ROLLUP_QUALITY_DIMENSION_INVALID,
        ROLLUP_QUALITY_LATE_INPUT,
        ROLLUP_QUALITY_KNOWN_MASK,
        ROLLUP_QUALITY_LIFECYCLE_CONFLICT,
        ROLLUP_QUALITY_PROCESS_GAP,
        ROLLUP_QUALITY_SESSION_GAP,
        ROLLUP_QUALITY_UTC_BACKWARD,
        ROLLUP_QUALITY_UTC_FANOUT,
        ROLLUP_QUALITY_UTC_MISMATCH,
        ROLLUP_QUALITY_UTC_UNKNOWN,
        UINT64_MAX,
        UNKNOWN_DAY,
        UTC_UNKNOWN,
        report_definition,
        unknown_dimensions,
    )

DAY_USEC = 86_400_000_000
EPOCH = date(1970, 1, 1)
MIN_SQL_DAY = date(1000, 1, 1)
MAX_SQL_DAY = date(9999, 12, 31)
MIN_DAY_INDEX = (MIN_SQL_DAY - EPOCH).days
MAX_DAY_INDEX = (MAX_SQL_DAY - EPOCH).days

INTERVAL_KIND = 1
LIFECYCLE_KIND = 2
CHECKPOINT_KIND = 3
GAP_KIND = 4
CONFIGURATION_KIND = 5

CATEGORY_UNKNOWN = 0
CATEGORY_IDLE = 1
CATEGORY_ACTIVE = 2
CATEGORY_LINKDEAD = 3

CONTEXT_UNKNOWN = 0
CONTEXT_OVERFLOW = 8
CONTEXT_QUALITY_OBSERVED = 1

RAW_CLOCK_DISCONTINUITY = 1 << 7
RAW_LATE_INPUT = 1 << 8
MAX_UTC_FANOUT = 1_000
MAX_RETRIES_HARD_MAX = 8
REPORT_ROW_LIMIT_DEFAULT = 10_000
REPORT_BYTE_LIMIT_DEFAULT = 32 * 1024 * 1024
REPORT_RUNTIME_DEFAULT_S = 30.0


class RollupError(RuntimeError):
    """Base class for fail-closed rollup errors."""


class SemanticError(RollupError, ValueError):
    """A durable fact cannot be interpreted without guessing."""


class BoundsExceeded(RollupError):
    """A page or invocation exceeded an explicit safety bound."""


class CursorError(RollupError):
    """The source/cursor contract was not monotonic."""


class CheckpointConflict(SemanticError):
    """A same-revision checkpoint disagrees with the absolute projection."""


@dataclass(frozen=True, slots=True)
class RollupBounds:
    """Hard limits for one invocation and one page.

    The defaults are intentionally conservative.  They are code limits, not a
    claim that a production database has been load-qualified.
    """

    page_size: int = 100
    max_rows: int = 10_000
    max_page_bytes: int = 4 * 1024 * 1024
    max_total_bytes: int = 32 * 1024 * 1024
    max_output_fanout: int = 2_000
    max_transaction_statements: int = 5_000
    max_runtime_s: float = 30.0
    max_retries: int = 2
    statement_timeout_s: float = 2.0
    socket_timeout_s: float = 3.0
    lock_timeout_s: float = 2.0

    def validate(self) -> None:
        integer_fields = (
            "page_size",
            "max_rows",
            "max_page_bytes",
            "max_total_bytes",
            "max_output_fanout",
            "max_transaction_statements",
        )
        for name in integer_fields:
            value = getattr(self, name)
            if isinstance(value, bool) or not isinstance(value, int) or value <= 0:
                raise ValueError(f"{name} must be a positive integer")
        if self.page_size > 1_000:
            raise ValueError("page_size exceeds hard maximum 1000")
        if self.max_rows < self.page_size:
            raise ValueError("max_rows must be at least page_size")
        if self.max_page_bytes > 64 * 1024 * 1024:
            raise ValueError("max_page_bytes exceeds hard maximum")
        if self.max_total_bytes < self.max_page_bytes:
            raise ValueError("max_total_bytes must be at least max_page_bytes")
        if self.max_output_fanout > 100_000:
            raise ValueError("max_output_fanout exceeds hard maximum")
        if self.max_transaction_statements > 100_000:
            raise ValueError("max_transaction_statements exceeds hard maximum")
        for name in (
            "max_runtime_s",
            "statement_timeout_s",
            "socket_timeout_s",
            "lock_timeout_s",
        ):
            value = getattr(self, name)
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value) or value <= 0:
                raise ValueError(f"{name} must be a positive number")
        if self.max_runtime_s > 3_600:
            raise ValueError("max_runtime_s exceeds hard maximum")
        if isinstance(self.max_retries, bool) or not isinstance(self.max_retries, int) or not 0 <= self.max_retries <= MAX_RETRIES_HARD_MAX:
            raise ValueError(f"max_retries must be an integer in 0..{MAX_RETRIES_HARD_MAX}")


@dataclass(frozen=True, slots=True)
class UtcSlice:
    """One half-open UTC-day allocation, with explicit known/unknown status."""

    utc_day: date
    duration_usec: int
    known_utc: bool
    quality_flags: int = 0


@dataclass(slots=True)
class SessionDelta:
    target: RollupTarget
    subject_id: int
    pid: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    latest_checkpoint_revision: int = 0
    checkpoint_counters: dict[str, int] | None = None
    covered: dict[str, int] = field(
        default_factory=lambda: {"covered_" + name: 0 for name in COUNTER_FIELDS}
    )
    attributable_usec: int = 0
    observed_intervals: int = 0
    entered: int = 0
    exited: int = 0
    end_reason: int = 0
    quality_flags: int = 0
    input_watermark: int = 0


@dataclass(slots=True)
class PlayerDayDelta:
    target: RollupTarget
    utc_day: date
    subject_id: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    counters: dict[str, int] = field(default_factory=lambda: dict.fromkeys(COUNTER_FIELDS, 0))
    attributable_usec: int = 0
    observed_intervals: int = 0
    quality_flags: int = 0
    input_watermark: int = 0


@dataclass(slots=True)
class CohortDelta:
    target: RollupTarget
    utc_day: date
    level_band: int
    class_id: int
    race_id: int
    faction_id: int
    zone_vnum: int
    config_id: int
    category: int
    duration_usec: int = 0
    attributable_usec: int = 0
    observed_intervals: int = 0
    quality_flags: int = 0
    input_watermark: int = 0

    @property
    def key(self) -> tuple[Any, ...]:
        return (
            self.utc_day,
            self.level_band,
            self.class_id,
            self.race_id,
            self.faction_id,
            self.zone_vnum,
            self.config_id,
            self.category,
        )


@dataclass(slots=True)
class MemberDelta:
    target: RollupTarget
    utc_day: date
    level_band: int
    class_id: int
    race_id: int
    faction_id: int
    zone_vnum: int
    config_id: int
    category: int
    membership_kind: int
    subject_id: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    duration_usec: int = 0
    attributable_usec: int = 0
    observed_intervals: int = 0
    quality_flags: int = 0
    input_watermark: int = 0

    @property
    def key(self) -> tuple[Any, ...]:
        return (
            self.utc_day,
            self.level_band,
            self.class_id,
            self.race_id,
            self.faction_id,
            self.zone_vnum,
            self.config_id,
            self.category,
            self.membership_kind,
            self.subject_id,
            self.session_boot_id,
            self.session_process_id,
            self.session_seq,
        )


@dataclass(slots=True)
class PageContribution:
    target: RollupTarget
    start_cursor: int
    page_last_ingest_id: int
    fetched_rows: int
    estimated_bytes: int
    sessions: dict[tuple[int, int, int], SessionDelta] = field(default_factory=dict)
    player_days: dict[tuple[Any, ...], PlayerDayDelta] = field(default_factory=dict)
    cohorts: dict[tuple[Any, ...], CohortDelta] = field(default_factory=dict)
    members: dict[tuple[Any, ...], MemberDelta] = field(default_factory=dict)
    state_quality_flags: int = 0
    coverage_start_utc_usec: int | None = None
    coverage_end_utc_usec: int | None = None
    output_fanout: int = 0

    @property
    def cursor(self) -> int:
        return self.page_last_ingest_id


@dataclass(frozen=True, slots=True)
class PageBuildResult:
    contribution: PageContribution


@dataclass(frozen=True, slots=True)
class RollupPageResult:
    """Structural result returned by a database adapter after one commit."""

    status: str
    cursor: int
    start_cursor: int = 0
    page_last_ingest_id: int | None = None
    fetched_rows: int = 0
    estimated_bytes: int = 0
    output_fanout: int = 0
    quality_flags: int = 0


@dataclass(frozen=True, slots=True)
class RollupRunResult:
    target: RollupTarget
    snapshot_high_watermark: int
    initial_cursor: int
    final_cursor: int
    pages: int
    fetched_rows: int
    estimated_bytes: int
    output_fanout: int
    complete: bool
    elapsed_s: float
    quality_flags: int

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "snapshot_high_watermark": self.snapshot_high_watermark,
            "initial_cursor": self.initial_cursor,
            "final_cursor": self.final_cursor,
            "pages": self.pages,
            "fetched_rows": self.fetched_rows,
            "estimated_bytes": self.estimated_bytes,
            "output_fanout": self.output_fanout,
            "complete": self.complete,
            "elapsed_s": self.elapsed_s,
            "quality_flags": self.quality_flags,
        }


class RollupDatabase(Protocol):
    """Minimal adapter surface used by :class:`RollupEngine`."""

    def snapshot_high_watermark(self) -> int: ...

    def process_next_page(
        self,
        target: RollupTarget,
        through_ingest_id: int,
        bounds: RollupBounds,
        build_page: Callable[
            [Sequence[Mapping[str, Any]], RollupTarget, int, int | None], PageContribution
        ],
        max_rows_remaining: int,
        max_bytes_remaining: int,
        origin_ingest_id: int = 0,
    ) -> RollupPageResult: ...

    def publish_generation(
        self,
        target: RollupTarget,
        *,
        bounds: RollupBounds | None = None,
    ) -> Mapping[str, Any]: ...

    def read_report(
        self,
        target: RollupTarget,
        report_name: str,
        *,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> ReportSnapshot: ...



def _checked_add(left: int, right: int, field_name: str) -> int:
    if left < 0 or right < 0 or left > UINT64_MAX or right > UINT64_MAX:
        raise SemanticError(f"{field_name} is outside unsigned 64-bit range")
    value = left + right
    if value > UINT64_MAX:
        raise SemanticError(f"{field_name} overflows unsigned 64-bit range")
    return value


def _unsigned(value: Any, name: str, *, allow_zero: bool = True) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise SemanticError(f"{name} must be an integer")
    if value < (0 if allow_zero else 1) or value > UINT64_MAX:
        raise SemanticError(f"{name} is outside unsigned 64-bit range")
    return value


def _raw_quality(row: Mapping[str, Any]) -> int:
    value = row.get("quality_flags")
    if value is None:
        return 0
    if isinstance(value, bool) or not isinstance(value, int) or not 0 <= value <= UINT64_MAX:
        raise SemanticError("quality_flags is not an unsigned integer")
    if value > (1 << 32) - 1 or value & ~ROLLUP_QUALITY_KNOWN_MASK:
        raise SemanticError("quality_flags contains an unknown/reserved raw bit")
    return value


def _normalize_raw_quality(value: int) -> int:
    """Preserve raw quality and expose raw lateness in the rollup vocabulary."""

    if value & RAW_LATE_INPUT:
        return value | ROLLUP_QUALITY_LATE_INPUT
    return value


def _validate_optional_enums(row: Mapping[str, Any]) -> None:
    allowed = {
        "category": range(4),
        "context": range(9),
        "context_quality": range(5),
        "lifecycle": range(5),
        "end_reason": range(6),
        "gap_reason": range(1, 7),
        "backend": range(1, 3),
        "enabled": range(2),
    }
    for name, values in allowed.items():
        value = row.get(name)
        if value is None:
            continue
        if isinstance(value, bool) or not isinstance(value, int) or value not in values:
            raise SemanticError(f"invalid raw telemetry {name} value {value!r}")


def estimate_row_bytes(row: Mapping[str, Any]) -> int:
    """Conservative deterministic byte estimate; it never serializes payload blobs."""

    total = 32
    for name, value in row.items():
        total += len(str(name).encode("utf-8")) + 8
        if value is None:
            total += 1
        elif isinstance(value, bytes):
            total += len(value)
        else:
            total += len(str(value).encode("utf-8"))
    return total


def _unknown_slice(duration_usec: int, quality_flags: int) -> tuple[UtcSlice, ...]:
    return (UtcSlice(UNKNOWN_DAY, duration_usec, False, quality_flags),)


def _day_for_index(index: int) -> date | None:
    if index < MIN_DAY_INDEX or index > MAX_DAY_INDEX:
        return None
    return EPOCH + timedelta(days=index)


def split_utc_interval(
    start_utc_usec: int | None,
    end_utc_usec: int | None,
    duration_usec: int,
    *,
    max_fanout: int = 32,
    quality_flags: int = 0,
) -> tuple[UtcSlice, ...]:
    """Split a valid UTC interval conservatively and exactly.

    Unknown, backward, mismatched, non-representable, clock-discontinuous, and
    over-budget intervals retain their complete monotonic duration in the
    reserved unknown day.  No guessed date is emitted.
    """

    _unsigned(duration_usec, "duration_usec", allow_zero=False)
    if (
        isinstance(max_fanout, bool)
        or not isinstance(max_fanout, int)
        or not 0 < max_fanout <= MAX_UTC_FANOUT
    ):
        raise ValueError(f"max_fanout must be 1..{MAX_UTC_FANOUT}")
    if start_utc_usec is None or end_utc_usec is None:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_UNKNOWN)
    if not isinstance(start_utc_usec, int) or not isinstance(end_utc_usec, int):
        raise SemanticError("UTC endpoints must be signed integers or NULL")
    if start_utc_usec == UTC_UNKNOWN or end_utc_usec == UTC_UNKNOWN:
        if start_utc_usec == UTC_UNKNOWN and end_utc_usec == UTC_UNKNOWN:
            return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_UNKNOWN)
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_MISMATCH)
    if quality_flags & RAW_CLOCK_DISCONTINUITY:
        flag = ROLLUP_QUALITY_UTC_BACKWARD if end_utc_usec < start_utc_usec else ROLLUP_QUALITY_UTC_MISMATCH
        return _unknown_slice(duration_usec, flag)
    if end_utc_usec < start_utc_usec:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_BACKWARD)
    if end_utc_usec - start_utc_usec != duration_usec:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_MISMATCH)
    if end_utc_usec <= start_utc_usec:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_MISMATCH)

    start_day_index = start_utc_usec // DAY_USEC
    end_day_index = (end_utc_usec - 1) // DAY_USEC
    fanout = end_day_index - start_day_index + 1
    if fanout > max_fanout:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_FANOUT)
    if _day_for_index(start_day_index) is None or _day_for_index(end_day_index) is None:
        return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_MISMATCH)

    slices: list[UtcSlice] = []
    cursor = start_utc_usec
    for day_index in range(start_day_index, end_day_index + 1):
        day = _day_for_index(day_index)
        if day is None:  # Defensive; endpoints were checked above.
            return _unknown_slice(duration_usec, ROLLUP_QUALITY_UTC_MISMATCH)
        boundary = (day_index + 1) * DAY_USEC
        end = min(end_utc_usec, boundary)
        amount = end - cursor
        if amount <= 0:
            raise SemanticError("UTC splitter produced a non-positive slice")
        if day == UNKNOWN_DAY:
            slices.append(UtcSlice(UNKNOWN_DAY, amount, False, ROLLUP_QUALITY_UTC_UNKNOWN))
        else:
            slices.append(UtcSlice(day, amount, True, 0))
        cursor = end
    if cursor != end_utc_usec or sum(item.duration_usec for item in slices) != duration_usec:
        raise SemanticError("UTC split did not conserve duration")
    return tuple(slices)


def _session_key(row: Mapping[str, Any]) -> tuple[int, int, int] | None:
    names = ("session_boot_id", "session_process_id", "session_seq")
    values = tuple(row.get(name) for name in names)
    if all(value in (None, 0) for value in values):
        return None
    if any(value is None for value in values):
        raise SemanticError("session identity is partially NULL")
    parsed = tuple(_unsigned(value, name, allow_zero=False) for name, value in zip(names, values, strict=True))
    return parsed  # type: ignore[return-value]


def _row_scope(row: Mapping[str, Any]) -> tuple[int | None, int | None]:
    environment = row.get("environment_id")
    season = row.get("season_id")
    if environment is not None:
        _unsigned(environment, "environment_id", allow_zero=True)
    if season is not None:
        _unsigned(season, "season_id", allow_zero=True)
    return environment, season


def _validate_common_row(row: Mapping[str, Any], previous_ingest_id: int | None) -> int:
    if not isinstance(row, Mapping):
        raise SemanticError("raw page item must be a mapping")
    ingest_id = _unsigned(row.get("ingest_id"), "ingest_id", allow_zero=False)
    if previous_ingest_id is not None and ingest_id <= previous_ingest_id:
        raise CursorError("raw page ingest_id values must be strictly increasing")
    schema_version = row.get("schema_version")
    if schema_version != 1:
        raise SemanticError(f"unsupported raw telemetry schema_version {schema_version!r}")
    kind = row.get("record_kind")
    if isinstance(kind, bool) or not isinstance(kind, int) or kind not in range(1, 6):
        raise SemanticError(f"unsupported raw telemetry record_kind {kind!r}")
    return ingest_id


def _target_row(row: Mapping[str, Any], target: RollupTarget) -> bool:
    environment, season = _row_scope(row)
    return environment == target.environment_id and season == target.season_id


def _session_delta(
    sessions: MutableMapping[tuple[int, int, int], SessionDelta],
    row: Mapping[str, Any],
    target: RollupTarget,
    ingest_id: int,
) -> SessionDelta | None:
    key = _session_key(row)
    if key is None:
        return None
    subject_id = _unsigned(row.get("subject_id"), "subject_id", allow_zero=False)
    pid = row.get("pid")
    if isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0 or pid > (1 << 31) - 1:
        raise SemanticError("session pid is outside signed positive INT range")
    delta = sessions.get(key)
    if delta is None:
        delta = SessionDelta(
            target=target,
            subject_id=subject_id,
            pid=pid,
            session_boot_id=key[0],
            session_process_id=key[1],
            session_seq=key[2],
            input_watermark=ingest_id,
        )
        sessions[key] = delta
    elif (delta.subject_id, delta.pid) != (subject_id, pid):
        raise SemanticError("session subject/PID changed inside a rollup page")
    delta.input_watermark = max(delta.input_watermark, ingest_id)
    return delta


def _add_quality(delta: Any, value: int) -> None:
    delta.quality_flags |= value


def _category_counters(category: int, duration_usec: int) -> dict[str, int]:
    if isinstance(category, bool) or not isinstance(category, int) or category not in range(4):
        raise SemanticError(f"invalid interval category {category!r}")
    values = dict.fromkeys(COUNTER_FIELDS, 0)
    values["resident_usec"] = duration_usec
    if category == CATEGORY_LINKDEAD:
        values["linkdead_usec"] = duration_usec
    else:
        values["connected_usec"] = duration_usec
        bucket = {
            CATEGORY_UNKNOWN: "unknown_usec",
            CATEGORY_IDLE: "idle_usec",
            CATEGORY_ACTIVE: "active_usec",
        }[category]
        values[bucket] = duration_usec
    return values


def _dimensions(row: Mapping[str, Any]) -> tuple[int, int, int, int, int, int]:
    names = ("level_band", "class_id", "race_id", "faction_id", "zone_vnum", "group_size")
    values = tuple(row.get(name) for name in names)
    if any(value is None for value in values):
        raise SemanticError("interval attribution dimensions are incomplete")
    level, class_id, race, faction, zone, group = values
    if any(isinstance(value, bool) or not isinstance(value, int) for value in values):
        raise SemanticError("interval attribution dimensions must be integers")
    if not 0 <= level <= (1 << 16) - 1 or not 0 <= class_id <= (1 << 16) - 1:
        raise SemanticError("level/class dimension is outside SMALLINT UNSIGNED")
    if not 0 <= race <= (1 << 16) - 1 or not 0 <= faction <= (1 << 16) - 1:
        raise SemanticError("race/faction dimension is outside SMALLINT UNSIGNED")
    if not -(1 << 31) <= zone <= (1 << 31) - 1:
        raise SemanticError("zone dimension is outside signed INT")
    if not 0 <= group <= (1 << 32) - 1:
        raise SemanticError("group_size is outside INT UNSIGNED")
    return level, class_id, race, faction, zone, group


def _known_context(row: Mapping[str, Any]) -> tuple[bool, int]:
    context_quality = row.get("context_quality")
    context = row.get("context")
    dimensions = _dimensions(row)
    level, class_id, race, faction, zone, group = dimensions
    dimension_quality = (
        ROLLUP_QUALITY_DIMENSION_INVALID
        if level == 0 or class_id == 0 or race == 0 or faction == 0 or zone < 0 or group == 0
        else 0
    )
    if context_quality != CONTEXT_QUALITY_OBSERVED or context is None:
        return False, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE | dimension_quality
    if context == CONTEXT_UNKNOWN or context == CONTEXT_OVERFLOW:
        return False, ROLLUP_QUALITY_CONTEXT_UNAVAILABLE | dimension_quality
    if dimension_quality:
        return False, dimension_quality
    return True, 0


def _cohort_dimensions(row: Mapping[str, Any]) -> tuple[int, int, int, int, int]:
    """Return captured cohort dimensions without turning unavailable context into unknown.

    Context quality controls attribution, not whether the individually captured
    dimensions may be retained.  The C interval type uses 0/-1 sentinels for
    unknown dimensions; SQL NULL is rejected earlier by ``_dimensions``.
    """

    level, class_id, race, faction, zone, _group_size = _dimensions(row)
    unknown = unknown_dimensions()
    return (
        level if level != 0 else unknown["level_band"],
        class_id if class_id != 0 else unknown["class_id"],
        race if race != 0 else unknown["race_id"],
        faction if faction != 0 else unknown["faction_id"],
        zone if zone >= 0 else unknown["zone_vnum"],
    )


def _add_player_day(
    player_days: MutableMapping[tuple[Any, ...], PlayerDayDelta],
    row: Mapping[str, Any],
    target: RollupTarget,
    session_key: tuple[int, int, int],
    slice_value: UtcSlice,
    category_values: Mapping[str, int],
    attributable: int,
    quality_flags: int,
    ingest_id: int,
) -> None:
    key = (slice_value.utc_day, row["subject_id"], *session_key)
    delta = player_days.get(key)
    if delta is None:
        delta = PlayerDayDelta(
            target=target,
            utc_day=slice_value.utc_day,
            subject_id=int(row["subject_id"]),
            session_boot_id=session_key[0],
            session_process_id=session_key[1],
            session_seq=session_key[2],
            input_watermark=ingest_id,
        )
        player_days[key] = delta
    for name, value in category_values.items():
        delta.counters[name] = _checked_add(delta.counters[name], value, name)
    delta.attributable_usec = _checked_add(delta.attributable_usec, attributable, "attributable_usec")
    delta.observed_intervals = _checked_add(delta.observed_intervals, 1, "observed_intervals")
    delta.quality_flags |= quality_flags | slice_value.quality_flags
    delta.input_watermark = max(delta.input_watermark, ingest_id)


def _add_cohort_and_members(
    cohorts: MutableMapping[tuple[Any, ...], CohortDelta],
    members: MutableMapping[tuple[Any, ...], MemberDelta],
    row: Mapping[str, Any],
    target: RollupTarget,
    session_key: tuple[int, int, int],
    slice_value: UtcSlice,
    category: int,
    known_context: bool,
    context_quality_flag: int,
    attributable: int,
    quality_flags: int,
    ingest_id: int,
) -> None:
    # Context quality may make attribution zero while captured dimensions stay
    # useful for cohort grouping.  Only typed unknown sentinels are collapsed.
    level, class_id, race, faction, zone = _cohort_dimensions(row)
    config_id = _unsigned(row.get("config_id"), "config_id", allow_zero=False)
    cohort_key = (
        slice_value.utc_day,
        level,
        class_id,
        race,
        faction,
        zone,
        config_id,
        category,
    )
    effective_quality = quality_flags | context_quality_flag | slice_value.quality_flags
    cohort = cohorts.get(cohort_key)
    if cohort is None:
        cohort = CohortDelta(
            target=target,
            utc_day=slice_value.utc_day,
            level_band=level,
            class_id=class_id,
            race_id=race,
            faction_id=faction,
            zone_vnum=zone,
            config_id=config_id,
            category=category,
            input_watermark=ingest_id,
        )
        cohorts[cohort_key] = cohort
    cohort.duration_usec = _checked_add(cohort.duration_usec, slice_value.duration_usec, "duration_usec")
    cohort.attributable_usec = _checked_add(cohort.attributable_usec, attributable, "attributable_usec")
    cohort.observed_intervals = _checked_add(cohort.observed_intervals, 1, "observed_intervals")
    cohort.quality_flags |= effective_quality
    cohort.input_watermark = max(cohort.input_watermark, ingest_id)

    for membership_kind, member_session in ((1, (0, 0, 0)), (2, session_key)):
        member_key = cohort_key + (membership_kind, int(row["subject_id"]), *member_session)
        member = members.get(member_key)
        if member is None:
            member = MemberDelta(
                target=target,
                utc_day=slice_value.utc_day,
                level_band=level,
                class_id=class_id,
                race_id=race,
                faction_id=faction,
                zone_vnum=zone,
                config_id=config_id,
                category=category,
                membership_kind=membership_kind,
                subject_id=int(row["subject_id"]),
                session_boot_id=member_session[0],
                session_process_id=member_session[1],
                session_seq=member_session[2],
                input_watermark=ingest_id,
            )
            members[member_key] = member
        member.duration_usec = _checked_add(member.duration_usec, slice_value.duration_usec, "duration_usec")
        member.attributable_usec = _checked_add(member.attributable_usec, attributable, "attributable_usec")
        member.observed_intervals = _checked_add(member.observed_intervals, 1, "observed_intervals")
        member.quality_flags |= effective_quality
        member.input_watermark = max(member.input_watermark, ingest_id)


def _update_coverage(contribution: PageContribution, slices: Sequence[UtcSlice], row: Mapping[str, Any]) -> None:
    known = [item for item in slices if item.known_utc]
    if not known:
        return
    start = row.get("start_utc_usec")
    end = row.get("end_utc_usec")
    if not isinstance(start, int) or not isinstance(end, int):
        return
    contribution.coverage_start_utc_usec = (
        start
        if contribution.coverage_start_utc_usec is None
        else min(contribution.coverage_start_utc_usec, start)
    )
    contribution.coverage_end_utc_usec = (
        end
        if contribution.coverage_end_utc_usec is None
        else max(contribution.coverage_end_utc_usec, end)
    )


def _derived_late_input(row: Mapping[str, Any], prior_coverage_end_utc_usec: int | None) -> bool:
    """Conservatively label an event whose header occurrence predates coverage.

    ``start_utc_usec`` is an interval boundary and can overlap a concurrent
    interval legitimately.  The immutable header occurrence is the event label
    used for this review signal.  If that label is merely the interval start,
    do not infer elapsedness from overlap; only an explicit raw late bit can
    make that claim.  Unknown labels never become late by guesswork.
    """

    if prior_coverage_end_utc_usec is None:
        return False
    occurrence = row.get("occurrence_utc_usec")
    if occurrence is None or not isinstance(occurrence, int) or occurrence == UTC_UNKNOWN:
        return False
    if not isinstance(prior_coverage_end_utc_usec, int) or prior_coverage_end_utc_usec == UTC_UNKNOWN:
        return False
    if occurrence >= prior_coverage_end_utc_usec:
        return False
    # A header label equal to an interval's start is compatible with a
    # concurrent interval; treating overlap as lateness would be a false claim.
    return occurrence != row.get("start_utc_usec")


def _known_coverage_end(slices: Sequence[UtcSlice], row: Mapping[str, Any]) -> int | None:
    """Return the endpoint of the last known UTC slice, if any."""

    known_indexes = [index for index, item in enumerate(slices) if item.known_utc]
    if not known_indexes:
        return None
    start = row.get("start_utc_usec")
    if not isinstance(start, int) or start == UTC_UNKNOWN:
        return None
    return start + sum(item.duration_usec for item in slices[: known_indexes[-1] + 1])


def build_page_contributions(
    rows: Sequence[Mapping[str, Any]],
    target: RollupTarget,
    *,
    start_cursor: int = 0,
    max_page_bytes: int | None = None,
    max_output_fanout: int | None = None,
    max_utc_fanout: int = 32,
    prior_coverage_end_utc_usec: int | None = None,
) -> PageContribution:
    """Build page deltas without SQL and without mutating any input row."""

    target.__post_init__()
    _unsigned(start_cursor, "start_cursor")
    if max_page_bytes is not None and (not isinstance(max_page_bytes, int) or max_page_bytes <= 0):
        raise ValueError("max_page_bytes must be positive")
    if max_output_fanout is not None and (not isinstance(max_output_fanout, int) or max_output_fanout <= 0):
        raise ValueError("max_output_fanout must be positive")
    if prior_coverage_end_utc_usec is not None and not isinstance(prior_coverage_end_utc_usec, int):
        raise ValueError("prior_coverage_end_utc_usec must be an integer or None")
    if isinstance(max_utc_fanout, bool) or not isinstance(max_utc_fanout, int) or not 0 < max_utc_fanout <= MAX_UTC_FANOUT:
        raise ValueError(f"max_utc_fanout must be 1..{MAX_UTC_FANOUT}")
    contribution = PageContribution(
        target=target,
        start_cursor=start_cursor,
        page_last_ingest_id=start_cursor,
        fetched_rows=len(rows),
        estimated_bytes=0,
    )
    previous_ingest: int | None = None
    coverage_end_seen: int | None = prior_coverage_end_utc_usec
    seen_replay_keys: set[tuple[int, int, int]] = set()

    for row in rows:
        ingest_id = _validate_common_row(row, previous_ingest)
        previous_ingest = ingest_id
        contribution.page_last_ingest_id = ingest_id
        contribution.estimated_bytes += estimate_row_bytes(row)
        if max_page_bytes is not None and contribution.estimated_bytes > max_page_bytes:
            raise BoundsExceeded(
                f"raw page exceeds max_page_bytes={max_page_bytes}; cursor remains {start_cursor}"
            )
        kind = int(row["record_kind"])
        _validate_optional_enums(row)
        replay_key = (
            _unsigned(row.get("boot_id"), "boot_id", allow_zero=False),
            _unsigned(row.get("process_id"), "process_id", allow_zero=False),
            _unsigned(row.get("record_seq"), "record_seq", allow_zero=False),
        )
        if replay_key in seen_replay_keys:
            raise SemanticError("raw page contains a duplicate replay key")
        seen_replay_keys.add(replay_key)
        environment, season = _row_scope(row)
        if (
            environment in (None, 0)
            or season in (None, 0)
        ) and not (
            kind == GAP_KIND
            and _session_key(row) is None
            and environment in (None, 0)
            and season in (None, 0)
        ):
            raise SemanticError("non-gap telemetry facts require nonzero environment and season scope")
        is_target = environment == target.environment_id and season == target.season_id
        raw_quality = _normalize_raw_quality(_raw_quality(row))
        if kind == GAP_KIND:
            gap_reason = row.get("gap_reason")
            if isinstance(gap_reason, bool) or not isinstance(gap_reason, int) or gap_reason not in range(1, 7):
                raise SemanticError("coverage gap has no valid gap_reason")

        if kind == CONFIGURATION_KIND:
            # Configuration facts are intentionally scanned for the stable
            # global cursor but have no interval/session contribution.
            continue
        if not is_target:
            if kind == GAP_KIND and _session_key(row) is None:
                contribution.state_quality_flags |= raw_quality | ROLLUP_QUALITY_PROCESS_GAP
            continue

        contribution.state_quality_flags |= raw_quality
        session_key = _session_key(row)
        if kind == GAP_KIND:
            if session_key is not None:
                session = _session_delta(contribution.sessions, row, target, ingest_id)
                assert session is not None
                session.quality_flags |= raw_quality | ROLLUP_QUALITY_SESSION_GAP
                contribution.state_quality_flags |= ROLLUP_QUALITY_SESSION_GAP
            else:
                contribution.state_quality_flags |= ROLLUP_QUALITY_PROCESS_GAP
            continue
        if session_key is None:
            raise SemanticError(f"target record kind {kind} has no logical session identity")
        session = _session_delta(contribution.sessions, row, target, ingest_id)
        assert session is not None

        if kind == LIFECYCLE_KIND:
            lifecycle = row.get("lifecycle")
            end_reason = row.get("end_reason")
            if lifecycle not in (1, 2, 3, 4):
                raise SemanticError(f"invalid lifecycle value {lifecycle!r}")
            if end_reason is None or not isinstance(end_reason, int) or not 0 <= end_reason <= 5:
                raise SemanticError("invalid lifecycle end_reason")
            if lifecycle == 1:
                session.entered = 1
            elif lifecycle == 2:
                session.exited = 1
                if end_reason == 0:
                    raise SemanticError("session exit has no end_reason")
                if session.end_reason not in (0, end_reason):
                    session.end_reason = 0
                    session.quality_flags |= ROLLUP_QUALITY_LIFECYCLE_CONFLICT
                else:
                    session.end_reason = end_reason
            session.quality_flags |= raw_quality
            continue

        if kind == CHECKPOINT_KIND:
            revision = _unsigned(row.get("checkpoint_revision"), "checkpoint_revision", allow_zero=False)
            counters: dict[str, int] = {}
            for name in COUNTER_FIELDS:
                counters[name] = _unsigned(row.get(name), name)
            if counters["connected_usec"] != (
                counters["active_usec"] + counters["idle_usec"] + counters["unknown_usec"]
            ):
                raise SemanticError("checkpoint connected conservation identity failed")
            if counters["resident_usec"] != counters["connected_usec"] + counters["linkdead_usec"]:
                raise SemanticError("checkpoint resident conservation identity failed")
            if session.latest_checkpoint_revision == 0 or revision > session.latest_checkpoint_revision:
                session.latest_checkpoint_revision = revision
                session.checkpoint_counters = counters
            elif revision == session.latest_checkpoint_revision:
                if counters != session.checkpoint_counters:
                    raise CheckpointConflict("same-page checkpoint revision has conflicting totals")
            session.quality_flags |= raw_quality
            continue

        if kind != INTERVAL_KIND:
            raise SemanticError(f"unsupported target record kind {kind}")

        duration = _unsigned(row.get("duration_usec"), "duration_usec", allow_zero=False)
        start_monotonic = _unsigned(row.get("start_monotonic_usec"), "start_monotonic_usec")
        end_monotonic = _unsigned(row.get("end_monotonic_usec"), "end_monotonic_usec")
        if end_monotonic <= start_monotonic or end_monotonic - start_monotonic != duration:
            raise SemanticError("interval duration does not equal monotonic end-start")
        category = row.get("category")
        category_values = _category_counters(category, duration)
        for name in ("classifier_version", "policy_version"):
            version = _unsigned(row.get(name), name, allow_zero=False)
            if version > (1 << 32) - 1:
                raise SemanticError(f"{name} exceeds UINT32")
        _dimensions(row)
        for name, value in category_values.items():
            field_name = "covered_" + name
            session.covered[field_name] = _checked_add(session.covered[field_name], value, field_name)
        session.observed_intervals = _checked_add(session.observed_intervals, 1, "observed_intervals")

        context_known, context_quality_flag = _known_context(row)
        slices = split_utc_interval(
            row.get("start_utc_usec"),
            row.get("end_utc_usec"),
            duration,
            max_fanout=max_utc_fanout,
            quality_flags=raw_quality,
        )
        split_quality = 0
        for item in slices:
            split_quality |= item.quality_flags
        interval_quality = raw_quality | split_quality | context_quality_flag
        late_input = bool(raw_quality & RAW_LATE_INPUT) or _derived_late_input(row, coverage_end_seen)
        if late_input:
            interval_quality |= ROLLUP_QUALITY_LATE_INPUT
        session.quality_flags |= interval_quality
        contribution.state_quality_flags |= interval_quality
        for item in slices:
            attributable = item.duration_usec if item.known_utc and context_known else 0
            session.attributable_usec = _checked_add(
                session.attributable_usec, attributable, "attributable_usec"
            )
            _add_player_day(
                contribution.player_days,
                row,
                target,
                session_key,
                item,
                category_values={name: value * item.duration_usec // duration for name, value in category_values.items()},
                attributable=attributable,
                quality_flags=interval_quality,
                ingest_id=ingest_id,
            )
            _add_cohort_and_members(
                contribution.cohorts,
                contribution.members,
                row,
                target,
                session_key,
                item,
                int(category),
                context_known,
                context_quality_flag,
                attributable,
                interval_quality,
                ingest_id,
            )
        _update_coverage(contribution, slices, row)
        known_coverage_end = _known_coverage_end(slices, row)
        if known_coverage_end is not None:
            coverage_end_seen = (
                known_coverage_end
                if coverage_end_seen is None
                else max(coverage_end_seen, known_coverage_end)
            )

    contribution.output_fanout = (
        len(contribution.sessions)
        + len(contribution.player_days)
        + len(contribution.cohorts)
        + len(contribution.members)
    )
    if max_output_fanout is not None and contribution.output_fanout > max_output_fanout:
        raise BoundsExceeded(
            f"page output fanout {contribution.output_fanout} exceeds "
            f"max_output_fanout={max_output_fanout}; cursor remains {start_cursor}"
        )
    return contribution


def merge_session_projection(
    existing: Mapping[str, Any] | None,
    delta: SessionDelta,
) -> dict[str, Any]:
    """Merge one page's deltas with a locked support-session row."""

    target = delta.target
    output: dict[str, Any] = {
        "definition_version": target.definition_version,
        "generation": target.generation,
        "environment_id": target.environment_id,
        "season_id": target.season_id,
        "subject_id": delta.subject_id,
        "pid": delta.pid,
        "session_boot_id": delta.session_boot_id,
        "session_process_id": delta.session_process_id,
        "session_seq": delta.session_seq,
        "latest_checkpoint_revision": 0,
        "connected_usec": 0,
        "active_usec": 0,
        "idle_usec": 0,
        "unknown_usec": 0,
        "resident_usec": 0,
        "linkdead_usec": 0,
        **{field: 0 for field in ("covered_" + name for name in COUNTER_FIELDS)},
        "attributable_usec": 0,
        "observed_intervals": 0,
        "entered": 0,
        "exited": 0,
        "end_reason": 0,
        "quality_flags": 0,
        "input_watermark": 0,
        "provisional": 1,
    }
    if existing is not None:
        for name in output:
            if name in existing and name not in ("definition_version", "generation", "environment_id", "season_id"):
                output[name] = existing[name]
        expected_identity = (
            target.definition_version,
            target.generation,
            target.environment_id,
            target.season_id,
            delta.subject_id,
            delta.pid,
            delta.session_boot_id,
            delta.session_process_id,
            delta.session_seq,
        )
        actual_identity = tuple(existing.get(name) for name in (
            "definition_version", "generation", "environment_id", "season_id",
            "subject_id", "pid", "session_boot_id", "session_process_id", "session_seq",
        ))
        if actual_identity != expected_identity:
            raise SemanticError("locked rollup session identity/scope changed")

    old_revision = int(output["latest_checkpoint_revision"])
    old_counters = {name: int(output[name]) for name in COUNTER_FIELDS}
    if delta.latest_checkpoint_revision > old_revision:
        if delta.checkpoint_counters is None:
            raise SemanticError("checkpoint revision has no cumulative counters")
        for name in COUNTER_FIELDS:
            if delta.checkpoint_counters[name] < old_counters[name]:
                raise CheckpointConflict("newer checkpoint regresses an absolute counter")
        output["latest_checkpoint_revision"] = delta.latest_checkpoint_revision
        output.update(delta.checkpoint_counters)
    elif delta.latest_checkpoint_revision == old_revision and delta.latest_checkpoint_revision != 0:
        if delta.checkpoint_counters is not None and delta.checkpoint_counters != old_counters:
            raise CheckpointConflict("same checkpoint revision has conflicting absolute counters")

    for field_name, value in delta.covered.items():
        output[field_name] = _checked_add(int(output[field_name]), value, field_name)
    output["attributable_usec"] = _checked_add(
        int(output["attributable_usec"]), delta.attributable_usec, "attributable_usec"
    )
    output["observed_intervals"] = _checked_add(
        int(output["observed_intervals"]), delta.observed_intervals, "observed_intervals"
    )
    output["entered"] = max(int(output["entered"]), delta.entered)
    output["exited"] = max(int(output["exited"]), delta.exited)
    if delta.end_reason:
        if output["end_reason"] not in (0, delta.end_reason):
            output["end_reason"] = 0
            output["quality_flags"] |= ROLLUP_QUALITY_LIFECYCLE_CONFLICT
        else:
            output["end_reason"] = delta.end_reason
    output["quality_flags"] = int(output["quality_flags"]) | delta.quality_flags
    output["input_watermark"] = max(int(output["input_watermark"]), delta.input_watermark)
    output["provisional"] = 1
    return output


def checkpoint_contribution_from_row(target: RollupTarget, row: Mapping[str, Any]) -> CheckpointContribution:
    revision = int(row["latest_checkpoint_revision"])
    counters: tuple[int | None, int | None, int | None, int | None, int | None, int | None]
    if revision > 0:
        values = tuple(int(row[name]) for name in COUNTER_FIELDS)
        counters = cast(tuple[int | None, int | None, int | None, int | None, int | None, int | None], values)
    else:
        counters = (None, None, None, None, None, None)
    return CheckpointContribution(
        target=target,
        subject_id=int(row["subject_id"]),
        pid=int(row["pid"]),
        session_boot_id=int(row["session_boot_id"]),
        session_process_id=int(row["session_process_id"]),
        session_seq=int(row["session_seq"]),
        latest_checkpoint_revision=revision,
        counters=counters,
        quality_flags=int(row["quality_flags"]),
        input_watermark=int(row["input_watermark"]),
        provisional=bool(row["provisional"]),
    )


def membership_contribution_from_row(target: RollupTarget, row: Mapping[str, Any]) -> MembershipContribution:
    membership_kind = _unsigned(row.get("membership_kind"), "membership_kind", allow_zero=False)
    if membership_kind not in (1, 2):
        raise SemanticError("membership_kind must be 1 (subject) or 2 (session)")
    subject_id = _unsigned(row.get("subject_id"), "subject_id", allow_zero=False)
    session_ids = tuple(
        _unsigned(row.get(name), name, allow_zero=True)
        for name in ("session_boot_id", "session_process_id", "session_seq")
    )
    if membership_kind == 1 and any(session_ids):
        raise SemanticError("subject membership must have all session IDs zero")
    if membership_kind == 2 and not all(session_ids):
        raise SemanticError("session membership must have all session IDs nonzero")
    return MembershipContribution(
        target=target,
        utc_day=row["utc_day"],
        level_band=int(row["level_band"]),
        class_id=int(row["class_id"]),
        race_id=int(row["race_id"]),
        faction_id=int(row["faction_id"]),
        zone_vnum=int(row["zone_vnum"]),
        config_id=int(row["config_id"]),
        category=int(row["category"]),
        membership_kind=membership_kind,
        subject_id=subject_id,
        session_boot_id=session_ids[0],
        session_process_id=session_ids[1],
        session_seq=session_ids[2],
        duration_usec=int(row["duration_usec"]),
        attributable_usec=int(row["attributable_usec"]),
        observed_intervals=int(row["observed_intervals"]),
        quality_flags=int(row["quality_flags"]),
        input_watermark=int(row["input_watermark"]),
        provisional=bool(row.get("provisional", True)),
    )


def coverage_from_state_row(
    target: RollupTarget,
    row: Mapping[str, Any],
    *,
    snapshot_high_watermark: int | None = None,
) -> RollupCoverage:
    snapshot = (
        int(row.get("rebuild_through_ingest_id", 0))
        if snapshot_high_watermark is None
        else snapshot_high_watermark
    )
    return RollupCoverage(
        target=target,
        input_watermark=int(row["input_watermark"]),
        snapshot_high_watermark=snapshot,
        publication_status=int(row["publication_status"]),
        coverage_start_utc_usec=row.get("coverage_start_utc_usec"),
        coverage_end_utc_usec=row.get("coverage_end_utc_usec"),
        quality_flags=int(row["quality_flags"]),
        provisional=bool(row["provisional"]),
        rebuild_from_ingest_id=int(row["rebuild_from_ingest_id"]),
        rebuild_through_ingest_id=int(row["rebuild_through_ingest_id"]),
    )


class RollupEngine:
    """Bounded page coordinator over a single external database adapter."""

    def __init__(self, database: RollupDatabase, *, clock: Callable[[], float] = time.monotonic) -> None:
        self.database = database
        self.clock = clock

    def run(
        self,
        target: RollupTarget,
        *,
        through_ingest_id: int | None = None,
        origin_ingest_id: int = 0,
        bounds: RollupBounds | None = None,
    ) -> RollupRunResult:
        target.__post_init__()
        bounds = bounds or RollupBounds()
        bounds.validate()
        started = self.clock()
        if through_ingest_id is None:
            through_ingest_id = _unsigned(
                self.database.snapshot_high_watermark(),
                "snapshot_high_watermark",
            )
        else:
            through_ingest_id = _unsigned(through_ingest_id, "through_ingest_id")
        origin_ingest_id = _unsigned(origin_ingest_id, "origin_ingest_id")
        if through_ingest_id < origin_ingest_id:
            raise ValueError("through_ingest_id must be at least origin_ingest_id")
        deadline = started + float(bounds.max_runtime_s)
        initial_cursor: int | None = None
        cursor = 0
        pages = fetched_rows = estimated_bytes = output_fanout = 0
        quality_flags = 0

        while True:
            remaining_runtime = deadline - self.clock()
            if remaining_runtime <= 0:
                raise BoundsExceeded("rollup invocation exceeded max_runtime_s")
            page_bounds = replace(bounds, max_runtime_s=remaining_runtime)
            rows_remaining = bounds.max_rows - fetched_rows
            bytes_remaining = bounds.max_total_bytes - estimated_bytes
            if rows_remaining <= 0:
                raise BoundsExceeded("rollup invocation exceeded max_rows before reaching snapshot")
            if bytes_remaining < bounds.max_page_bytes:
                # A smaller final page is still valid; the adapter/builder will
                # reject any fact that cannot fit the remaining byte budget.
                page_bytes = bytes_remaining
            else:
                page_bytes = bounds.max_page_bytes

            def build_page(
                rows: Sequence[Mapping[str, Any]],
                page_target: RollupTarget,
                page_start_cursor: int,
                prior_coverage_end_utc_usec: int | None = None,
            ) -> PageContribution:
                return build_page_contributions(
                    rows,
                    page_target,
                    start_cursor=page_start_cursor,
                    max_page_bytes=page_bytes,
                    max_output_fanout=page_bounds.max_output_fanout,
                    prior_coverage_end_utc_usec=prior_coverage_end_utc_usec,
                )

            result = self.database.process_next_page(
                target,
                through_ingest_id,
                page_bounds,
                build_page,
                rows_remaining,
                bytes_remaining,
                origin_ingest_id,
            )
            # Adapters must either enforce the deadline during the operation or
            # return a failed result.  A successful result received after the
            # fixed deadline is never reported as a successful invocation.
            if self.clock() >= deadline:
                raise BoundsExceeded("rollup invocation exceeded max_runtime_s")
            if result.status == "complete":
                cursor = result.cursor
                quality_flags |= result.quality_flags
                if cursor != through_ingest_id:
                    raise CursorError("adapter reported completion before the fixed high-water mark")
                if initial_cursor is None:
                    initial_cursor = cursor
                break
            if result.status != "committed":
                raise RollupError(f"database adapter returned non-success status {result.status!r}")
            if initial_cursor is None:
                initial_cursor = result.start_cursor
            if result.fetched_rows <= 0 or result.page_last_ingest_id is None:
                raise CursorError("committed page did not acknowledge a nonempty keyset page")
            if result.cursor <= result.start_cursor or result.cursor != result.page_last_ingest_id:
                raise CursorError("committed page acknowledged an invalid keyset cursor")
            if result.cursor <= cursor and pages:
                raise CursorError("rollup cursor did not advance")
            cursor = result.cursor
            pages += 1
            fetched_rows += result.fetched_rows
            estimated_bytes += result.estimated_bytes
            output_fanout += result.output_fanout
            quality_flags |= result.quality_flags
            if fetched_rows > bounds.max_rows or estimated_bytes > bounds.max_total_bytes:
                raise BoundsExceeded("adapter acknowledged work beyond invocation bounds")
            if cursor >= through_ingest_id:
                break

        if initial_cursor is None:
            initial_cursor = cursor
        finished = self.clock()
        if finished >= deadline:
            raise BoundsExceeded("rollup invocation exceeded max_runtime_s")
        elapsed = max(0.0, finished - started)
        return RollupRunResult(
            target=target,
            snapshot_high_watermark=through_ingest_id,
            initial_cursor=initial_cursor,
            final_cursor=cursor,
            pages=pages,
            fetched_rows=fetched_rows,
            estimated_bytes=estimated_bytes,
            output_fanout=output_fanout,
            complete=cursor >= through_ingest_id,
            elapsed_s=elapsed,
            quality_flags=quality_flags,
        )

    def publish(
        self,
        target: RollupTarget,
        *,
        bounds: RollupBounds | None = None,
    ) -> Mapping[str, Any]:
        target.__post_init__()
        if bounds is None:
            return self.database.publish_generation(target)
        bounds.validate()
        return self.database.publish_generation(target, bounds=bounds)

    def report(
        self,
        target: RollupTarget,
        report_name: str,
        *,
        max_rows: int = REPORT_ROW_LIMIT_DEFAULT,
        max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT,
        max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S,
    ) -> ReportSnapshot:
        target.__post_init__()
        report_definition(report_name)
        return self.database.read_report(
            target,
            report_name,
            max_rows=max_rows,
            max_bytes=max_bytes,
            max_runtime_s=max_runtime_s,
        )


def run_rollup(
    database: RollupDatabase,
    target: RollupTarget,
    *,
    through_ingest_id: int | None = None,
    origin_ingest_id: int = 0,
    bounds: RollupBounds | None = None,
) -> RollupRunResult:
    return RollupEngine(database).run(
        target,
        through_ingest_id=through_ingest_id,
        origin_ingest_id=origin_ingest_id,
        bounds=bounds,
    )


__all__ = [
    "BoundsExceeded",
    "CATEGORY_ACTIVE",
    "CATEGORY_IDLE",
    "CATEGORY_LINKDEAD",
    "CATEGORY_UNKNOWN",
    "CheckpointContribution",
    "CheckpointConflict",
    "CohortDelta",
    "CursorError",
    "DAY_USEC",
    "INTERVAL_KIND",
    "LIFECYCLE_KIND",
    "MAX_RETRIES_HARD_MAX",
    "MAX_UTC_FANOUT",
    "MemberDelta",
    "MembershipContribution",
    "PageContribution",
    "PageBuildResult",
    "PlayerDayDelta",
    "RollupBounds",
    "RollupDatabase",
    "RollupEngine",
    "RollupError",
    "RollupPageResult",
    "RollupRunResult",
    "REPORT_BYTE_LIMIT_DEFAULT",
    "REPORT_ROW_LIMIT_DEFAULT",
    "REPORT_RUNTIME_DEFAULT_S",
    "SemanticError",
    "SessionDelta",
    "UtcSlice",
    "build_page_contributions",
    "checkpoint_contribution_from_row",
    "coverage_from_state_row",
    "estimate_row_bytes",
    "membership_contribution_from_row",
    "merge_session_projection",
    "run_rollup",
    "split_utc_interval",
]
