"""Stable, executable definitions and typed output contracts for telemetry rollups.

The module deliberately has no database or PyMySQL dependency.  It is the small
contract that a report consumer (#269) can import without importing the worker.
"""
from __future__ import annotations

from dataclasses import dataclass
from datetime import date
import math
from types import MappingProxyType
from typing import Any, Mapping

DEFINITION_VERSION = 1
SUPPORTED_DEFINITION_VERSIONS = frozenset({DEFINITION_VERSION})

PUBLICATION_BUILDING = 0
PUBLICATION_PUBLISHED = 1
PUBLICATION_SUPERSEDED = 2
PUBLICATION_FAILED = 3
PUBLICATION_STATUSES = MappingProxyType(
    {
        "building": PUBLICATION_BUILDING,
        "published": PUBLICATION_PUBLISHED,
        "superseded": PUBLICATION_SUPERSEDED,
        "failed": PUBLICATION_FAILED,
    }
)

UNKNOWN_DAY = date(1000, 1, 1)
UTC_UNKNOWN = -(1 << 63)
UINT64_MAX = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
UINT16_MAX = (1 << 16) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1

COUNTER_FIELDS = (
    "connected_usec",
    "active_usec",
    "idle_usec",
    "unknown_usec",
    "resident_usec",
    "linkdead_usec",
)
COVERED_COUNTER_FIELDS = tuple("covered_" + name for name in COUNTER_FIELDS)
COHORT_DIMENSION_FIELDS = (
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
)
SESSION_ID_FIELDS = ("session_boot_id", "session_process_id", "session_seq")
SCOPE_FIELDS = ("definition_version", "generation", "environment_id", "season_id")

# Raw quality flags are bits 0..8 in telemetry_types.h.  Bits 16+ belong to the
# external rollup only and are intentionally documented rather than overloaded.
ROLLUP_QUALITY_UTC_UNKNOWN = 1 << 16
ROLLUP_QUALITY_UTC_MISMATCH = 1 << 17
ROLLUP_QUALITY_UTC_BACKWARD = 1 << 18
ROLLUP_QUALITY_UTC_FANOUT = 1 << 19
ROLLUP_QUALITY_PROCESS_GAP = 1 << 20
ROLLUP_QUALITY_CONTEXT_UNAVAILABLE = 1 << 21
ROLLUP_QUALITY_DIMENSION_INVALID = 1 << 22
ROLLUP_QUALITY_LIFECYCLE_CONFLICT = 1 << 23
ROLLUP_QUALITY_CHECKPOINT_CONFLICT = 1 << 24
ROLLUP_QUALITY_LATE_INPUT = 1 << 25
ROLLUP_QUALITY_SESSION_GAP = 1 << 26
ROLLUP_QUALITY_KNOWN_MASK = (1 << 9) - 1
ROLLUP_QUALITY_MASK = (
    ROLLUP_QUALITY_KNOWN_MASK
    | ROLLUP_QUALITY_UTC_UNKNOWN
    | ROLLUP_QUALITY_UTC_MISMATCH
    | ROLLUP_QUALITY_UTC_BACKWARD
    | ROLLUP_QUALITY_UTC_FANOUT
    | ROLLUP_QUALITY_PROCESS_GAP
    | ROLLUP_QUALITY_CONTEXT_UNAVAILABLE
    | ROLLUP_QUALITY_DIMENSION_INVALID
    | ROLLUP_QUALITY_LIFECYCLE_CONFLICT
    | ROLLUP_QUALITY_CHECKPOINT_CONFLICT
    | ROLLUP_QUALITY_LATE_INPUT
    | ROLLUP_QUALITY_SESSION_GAP
)


@dataclass(frozen=True, slots=True)
class RollupTarget:
    """Immutable rollup identity; all four fields are required on every call."""

    definition_version: int
    generation: int
    environment_id: int
    season_id: int

    def __post_init__(self) -> None:
        validate_target(self)

    @property
    def scope_tuple(self) -> tuple[int, int, int, int]:
        return (
            self.definition_version,
            self.generation,
            self.environment_id,
            self.season_id,
        )


@dataclass(frozen=True, slots=True)
class ReportDefinition:
    """Reviewable report metadata plus the aggregate table it reads."""

    name: str
    definition_version: int
    grain: str
    table: str
    dimensions: tuple[str, ...]
    metrics: tuple[str, ...]
    denominator: str
    distinct_semantics: str
    distribution_semantics: str
    account_metrics_available: bool = False
    unavailable_metrics: tuple[str, ...] = (
        "account_distinct_count",
        "account_duration_usec",
        "account_session_union_usec",
    )
    rate_numerator_metrics: tuple[str, ...] = ()
    rate_denominator_metrics: tuple[str, ...] = ()
    rate_unit: str = "fraction"
    zero_denominator_policy: str = "null"
    missing_input_policy: str = "null"

    def public_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "definition_version": self.definition_version,
            "grain": self.grain,
            "table": self.table,
            "dimensions": list(self.dimensions),
            "metrics": list(self.metrics),
            "denominator": self.denominator,
            "rate_numerator_metrics": list(self.rate_numerator_metrics),
            "rate_denominator_metrics": list(self.rate_denominator_metrics),
            "rate_unit": self.rate_unit,
            "zero_denominator_policy": self.zero_denominator_policy,
            "missing_input_policy": self.missing_input_policy,
            "rate": {
                "numerator_metrics": list(self.rate_numerator_metrics),
                "denominator_metrics": list(self.rate_denominator_metrics),
                "unit": self.rate_unit,
                "zero_denominator_policy": self.zero_denominator_policy,
                "missing_input_policy": self.missing_input_policy,
            },
            "distinct_semantics": self.distinct_semantics,
            "distribution_semantics": self.distribution_semantics,
            "account_metrics_available": self.account_metrics_available,
            "unavailable_metrics": list(self.unavailable_metrics),
        }


@dataclass(frozen=True, slots=True)
class RollupCoverage:
    """State/freshness metadata attached to every report read."""

    target: RollupTarget
    input_watermark: int
    snapshot_high_watermark: int
    publication_status: int
    coverage_start_utc_usec: int | None
    coverage_end_utc_usec: int | None
    quality_flags: int
    provisional: bool
    rebuild_from_ingest_id: int
    rebuild_through_ingest_id: int

    @property
    def input_complete_to_snapshot(self) -> bool:
        return self.input_watermark == self.snapshot_high_watermark

    @property
    def occurrence_bounds_known(self) -> bool:
        return (
            self.coverage_start_utc_usec is not None
            and self.coverage_end_utc_usec is not None
            and not self.quality_flags
            & (
                ROLLUP_QUALITY_UTC_UNKNOWN
                | ROLLUP_QUALITY_UTC_MISMATCH
                | ROLLUP_QUALITY_UTC_BACKWARD
                | ROLLUP_QUALITY_UTC_FANOUT
            )
        )

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "input_watermark": self.input_watermark,
            "snapshot_high_watermark": self.snapshot_high_watermark,
            "input_complete_to_snapshot": self.input_complete_to_snapshot,
            "publication_status": self.publication_status,
            "coverage_start_utc_usec": self.coverage_start_utc_usec,
            "coverage_end_utc_usec": self.coverage_end_utc_usec,
            "quality_flags": self.quality_flags,
            "provisional": self.provisional,
            "rebuild_from_ingest_id": self.rebuild_from_ingest_id,
            "rebuild_through_ingest_id": self.rebuild_through_ingest_id,
        }


@dataclass(frozen=True, slots=True)
class CheckpointContribution:
    """Latest absolute checkpoint plane for one logical session.

    The six totals are not interval deltas and must never be summed with one
    another or with covered interval totals.
    """

    target: RollupTarget
    subject_id: int
    pid: int
    session_boot_id: int
    session_process_id: int
    session_seq: int
    latest_checkpoint_revision: int
    counters: tuple[int | None, int | None, int | None, int | None, int | None, int | None]
    quality_flags: int
    input_watermark: int
    provisional: bool = True

    @property
    def checkpoint_available(self) -> bool:
        return self.latest_checkpoint_revision > 0

    def counter_dict(self) -> dict[str, int | None]:
        return dict(zip(COUNTER_FIELDS, self.counters, strict=True))

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "subject_id": self.subject_id,
            "pid": self.pid,
            "session_boot_id": self.session_boot_id,
            "session_process_id": self.session_process_id,
            "session_seq": self.session_seq,
            "latest_checkpoint_revision": self.latest_checkpoint_revision,
            "checkpoint_totals_available": self.checkpoint_available,
            "counters": self.counter_dict(),
            "quality_flags": self.quality_flags,
            "input_watermark": self.input_watermark,
            "provisional": self.provisional,
        }


@dataclass(frozen=True, slots=True)
class MembershipContribution:
    """One declared daily cohort member at subject or original-session grain."""

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
    duration_usec: int
    attributable_usec: int
    observed_intervals: int
    quality_flags: int
    input_watermark: int
    provisional: bool = True

    @property
    def cohort_key(self) -> tuple[Any, ...]:
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

    @property
    def identity_key(self) -> tuple[Any, ...]:
        return self.cohort_key + (
            self.membership_kind,
            self.subject_id,
            self.session_boot_id,
            self.session_process_id,
            self.session_seq,
        )

    @property
    def bucket_kind(self) -> str:
        return "unknown" if self.utc_day == UNKNOWN_DAY else "calendar"

    @property
    def public_utc_day(self) -> date | None:
        return None if self.utc_day == UNKNOWN_DAY else self.utc_day

    def public_dict(self) -> dict[str, Any]:
        return {
            "definition_version": self.target.definition_version,
            "generation": self.target.generation,
            "environment_id": self.target.environment_id,
            "season_id": self.target.season_id,
            "utc_day": self.public_utc_day,
            "bucket_kind": self.bucket_kind,
            "level_band": self.level_band,
            "class_id": self.class_id,
            "race_id": self.race_id,
            "faction_id": self.faction_id,
            "zone_vnum": self.zone_vnum,
            "config_id": self.config_id,
            "category": self.category,
            "membership_kind": self.membership_kind,
            "subject_id": self.subject_id,
            "session_boot_id": self.session_boot_id,
            "session_process_id": self.session_process_id,
            "session_seq": self.session_seq,
            "duration_usec": self.duration_usec,
            "attributable_usec": self.attributable_usec,
            "observed_intervals": self.observed_intervals,
            "quality_flags": self.quality_flags,
            "input_watermark": self.input_watermark,
            "provisional": self.provisional,
        }


@dataclass(frozen=True, slots=True)
class ReportSnapshot:
    """Typed handoff for #269: definition, coverage, and named rows."""

    definition: ReportDefinition
    coverage: RollupCoverage
    rows: tuple[Mapping[str, Any], ...]
    truncated: bool = False


def validate_target(target: RollupTarget) -> None:
    if not isinstance(target.definition_version, int) or isinstance(
        target.definition_version, bool
    ):
        raise ValueError("definition_version must be an integer")
    if target.definition_version not in SUPPORTED_DEFINITION_VERSIONS:
        raise ValueError(
            f"unsupported rollup definition version {target.definition_version}; "
            f"supported={sorted(SUPPORTED_DEFINITION_VERSIONS)}"
        )
    for name in ("generation", "environment_id", "season_id"):
        value = getattr(target, name)
        if not isinstance(value, int) or isinstance(value, bool) or not 1 <= value <= UINT64_MAX:
            raise ValueError(f"{name} must be an unsigned nonzero 64-bit integer")


SESSION_PLAYTIME = ReportDefinition(
    name="session_playtime",
    definition_version=DEFINITION_VERSION,
    grain="logical_session",
    table="telemetry_rollup_session",
    dimensions=("subject_id", "pid", *SESSION_ID_FIELDS),
    metrics=(
        *COUNTER_FIELDS,
        *COVERED_COUNTER_FIELDS,
        "attributable_usec",
        "observed_intervals",
        "entered",
        "exited",
        "end_reason",
        "quality_flags",
    ),
    denominator=(
        "Rates use an explicitly selected covered duration denominator; zero "
        "denominator yields null. Checkpoint totals are a separate absolute plane."
    ),
    rate_numerator_metrics=("attributable_usec",),
    rate_denominator_metrics=("covered_resident_usec",),
    rate_unit="fraction",
    zero_denominator_policy="null",
    missing_input_policy="null",
    distinct_semantics=(
        "Rows are one original logical session. Subject/account distinct counts "
        "are not additive across days or cohorts. No account linkage exists."
    ),
    distribution_semantics=(
        "Only per-session rows support a distribution; sums cannot produce a "
        "median or percentile."
    ),
)

COHORT_ACTIVITY = ReportDefinition(
    name="cohort_activity",
    definition_version=DEFINITION_VERSION,
    grain="utc_day_and_captured_context",
    table="telemetry_cohort_day",
    dimensions=COHORT_DIMENSION_FIELDS,
    metrics=(
        "duration_usec",
        "attributable_usec",
        "observed_intervals",
        "subject_count",
        "session_count",
        "quality_flags",
    ),
    denominator=(
        "Activity rates use duration_usec or attributable_usec selected by the "
        "caller; a zero denominator is null, never zero or infinity."
    ),
    rate_numerator_metrics=("attributable_usec",),
    rate_denominator_metrics=("duration_usec",),
    rate_unit="fraction",
    zero_denominator_policy="null",
    missing_input_policy="null",
    distinct_semantics=(
        "subject_count and session_count are daily cohort cardinalities backed by "
        "telemetry_cohort_member; do not sum them across days/cohorts."
    ),
    distribution_semantics=(
        "Member rows retain subject/session contributions for declared distributions; "
        "cohort sums alone cannot produce medians or percentiles."
    ),
)

REPORT_DEFINITIONS = MappingProxyType(
    {
        SESSION_PLAYTIME.name: SESSION_PLAYTIME,
        COHORT_ACTIVITY.name: COHORT_ACTIVITY,
    }
)

# Aliases are input conveniences only; emitted definition names remain stable.
REPORT_ALIASES = MappingProxyType(
    {
        "session": SESSION_PLAYTIME.name,
        "playtime": SESSION_PLAYTIME.name,
        "cohort": COHORT_ACTIVITY.name,
        "activity": COHORT_ACTIVITY.name,
    }
)


def report_definition(name: str) -> ReportDefinition:
    canonical = REPORT_ALIASES.get(name, name)
    try:
        return REPORT_DEFINITIONS[canonical]
    except KeyError as error:
        raise ValueError(
            f"unknown report definition {name!r}; "
            f"available={sorted(REPORT_DEFINITIONS)}"
        ) from error


def report_definitions() -> tuple[ReportDefinition, ...]:
    return tuple(REPORT_DEFINITIONS[name] for name in sorted(REPORT_DEFINITIONS))


def report_catalog() -> tuple[dict[str, Any], ...]:
    return tuple(definition.public_dict() for definition in report_definitions())


def unknown_dimensions() -> dict[str, int]:
    return {
        "level_band": 0,
        "class_id": 0,
        "race_id": 0,
        "faction_id": 0,
        "zone_vnum": -1,
    }


def counters_from_mapping(row: Mapping[str, Any], prefix: str = "") -> tuple[int, ...]:
    return tuple(int(row[prefix + name]) for name in COUNTER_FIELDS)


def safe_rate(numerator: int | float, denominator: int | float) -> float | None:
    """Return a rate or ``None`` when its explicitly selected denominator is zero."""
    for name, value in (("numerator", numerator), ("denominator", denominator)):
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            raise ValueError(f"{name} must be a finite number")
        if value < 0:
            raise ValueError(f"{name} must be nonnegative")
    if denominator == 0:
        return None
    return float(numerator) / float(denominator)


rate_or_none = safe_rate


__all__ = [
    "COHORT_ACTIVITY",
    "COHORT_DIMENSION_FIELDS",
    "COUNTER_FIELDS",
    "COVERED_COUNTER_FIELDS",
    "DEFINITION_VERSION",
    "CheckpointContribution",
    "INT32_MAX",
    "INT32_MIN",
    "MembershipContribution",
    "PUBLICATION_BUILDING",
    "PUBLICATION_FAILED",
    "PUBLICATION_PUBLISHED",
    "PUBLICATION_SUPERSEDED",
    "REPORT_DEFINITIONS",
    "REPORT_ALIASES",
    "ROLLUP_QUALITY_MASK",
    "ROLLUP_QUALITY_CHECKPOINT_CONFLICT",
    "ROLLUP_QUALITY_CONTEXT_UNAVAILABLE",
    "ROLLUP_QUALITY_DIMENSION_INVALID",
    "ROLLUP_QUALITY_KNOWN_MASK",
    "ROLLUP_QUALITY_LATE_INPUT",
    "ROLLUP_QUALITY_SESSION_GAP",
    "ROLLUP_QUALITY_LIFECYCLE_CONFLICT",
    "ROLLUP_QUALITY_PROCESS_GAP",
    "ROLLUP_QUALITY_UTC_BACKWARD",
    "ROLLUP_QUALITY_UTC_FANOUT",
    "ROLLUP_QUALITY_UTC_MISMATCH",
    "ROLLUP_QUALITY_UTC_UNKNOWN",
    "ReportDefinition",
    "ReportSnapshot",
    "RollupCoverage",
    "RollupTarget",
    "SESSION_ID_FIELDS",
    "SESSION_PLAYTIME",
    "SCOPE_FIELDS",
    "UINT16_MAX",
    "UINT32_MAX",
    "UINT64_MAX",
    "UNKNOWN_DAY",
    "UTC_UNKNOWN",
    "counters_from_mapping",
    "report_catalog",
    "report_definition",
    "report_definitions",
    "rate_or_none",
    "safe_rate",
    "unknown_dimensions",
    "validate_target",
]
