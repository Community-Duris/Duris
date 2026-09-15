#!/usr/bin/env python3
"""Bounded administrator reports over the published #268 aggregates.

The report path is deliberately separate from the rollup writer.  It selects
only published aggregate rows through a dedicated report connection, applies
all scope and cohort predicates in SQL, and exposes a cache of the last
published page for rebuild or database-outage use.  It never reads the
immutable fact stream or live game tables.
"""
from __future__ import annotations

import argparse
import base64
from copy import deepcopy
from dataclasses import dataclass, field
from datetime import date
from decimal import Decimal
import hashlib
import json
import math
import multiprocessing
import os
from pathlib import Path
import sys
import tempfile
import time
from typing import Any, Callable, Mapping, Sequence

try:  # Running as a package.
    from .db_access import ConnectionSettings, PyMySQLConnectionFactory
    from .rollup_definitions import (
        COVERED_COUNTER_FIELDS,
        COUNTER_FIELDS,
        PUBLICATION_PUBLISHED,
        RollupTarget,
        UNKNOWN_DAY,
    )
    from .rollup_engine import (
        BoundsExceeded,
        coverage_from_state_row,
        estimate_row_bytes,
    )
except ImportError:  # Running scripts/telemetry/report.py directly.
    from db_access import ConnectionSettings, PyMySQLConnectionFactory  # type: ignore[no-redef]
    from rollup_definitions import (  # type: ignore[no-redef]
        COVERED_COUNTER_FIELDS,
        COUNTER_FIELDS,
        PUBLICATION_PUBLISHED,
        RollupTarget,
        UNKNOWN_DAY,
    )
    from rollup_engine import (  # type: ignore[no-redef]
        BoundsExceeded,
        coverage_from_state_row,
        estimate_row_bytes,
    )


REPORT_SCHEMA_VERSION = 1
REPORT_PREFIX = "TELEMETRY_REPORT_DB_"
REPORT_ROW_LIMIT_DEFAULT = 10_000
REPORT_ROW_LIMIT_HARD_MAX = 100_000
REPORT_BYTE_LIMIT_DEFAULT = 32 * 1024 * 1024
REPORT_BYTE_LIMIT_HARD_MAX = 256 * 1024 * 1024
REPORT_RUNTIME_DEFAULT_S = 30.0
REPORT_RUNTIME_HARD_MAX = 3_600.0
REPORT_ROW_BYTE_BOUND = 2_048
REPORT_MIN_FETCH_ROWS = 2
CACHE_SCHEMA_VERSION = 1
MICROSECONDS_PER_HOUR = 3_600_000_000
UINT64_MAX = (1 << 64) - 1
UINT32_MAX = (1 << 32) - 1
INT32_MIN = -(1 << 31)
INT32_MAX = (1 << 31) - 1

CATEGORY_NAMES = {
    0: "unknown",
    1: "idle",
    2: "active",
    3: "linkdead",
}

# These are the only aggregate columns the report path may select.  Keeping
# the projection here, rather than accepting caller SQL, prevents a report
# request from widening into raw or private data.
SESSION_REPORT_COLUMNS = (
    "subject_id",
    "pid",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    "latest_checkpoint_revision",
    *COUNTER_FIELDS,
    *COVERED_COUNTER_FIELDS,
    "attributable_usec",
    "observed_intervals",
    "entered",
    "exited",
    "end_reason",
    "quality_flags",
    "input_watermark",
    "provisional",
)
COHORT_REPORT_COLUMNS = (
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
    "duration_usec",
    "attributable_usec",
    "observed_intervals",
    "subject_count",
    "session_count",
    "quality_flags",
    "input_watermark",
    "provisional",
)
MEMBER_REPORT_COLUMNS = (
    "utc_day",
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
    "membership_kind",
    "subject_id",
    "session_boot_id",
    "session_process_id",
    "session_seq",
    "duration_usec",
    "attributable_usec",
    "observed_intervals",
    "quality_flags",
    "input_watermark",
)
STATE_REPORT_COLUMNS = (
    "definition_version",
    "generation",
    "environment_id",
    "season_id",
    "input_watermark",
    "publication_status",
    "coverage_start_utc_usec",
    "coverage_end_utc_usec",
    "quality_flags",
    "provisional",
    "rebuild_from_ingest_id",
    "rebuild_through_ingest_id",
)

SCOPE_FIELDS = ("definition_version", "generation", "environment_id", "season_id")
COHORT_FILTER_FIELDS = (
    "level_band",
    "class_id",
    "race_id",
    "faction_id",
    "zone_vnum",
    "config_id",
    "category",
)


class ReportError(RuntimeError):
    """Base class for report-path failures."""


class ReportDatabaseError(ReportError):
    """The dedicated report connection could not complete a read."""


class ReportNotPublished(ReportDatabaseError):
    """No published generation exists for the requested scope."""


class ReportCacheError(ReportError):
    """A cache operation failed closed without affecting fresh reads."""


@dataclass(frozen=True, slots=True)
class ReportDefinition:
    """Administrator-facing report metadata and executable query kind."""

    name: str
    source_table: str
    source_report: str
    grain: str
    dimensions: tuple[str, ...]
    metrics: tuple[str, ...]
    supported_filters: tuple[str, ...]
    units: Mapping[str, str]
    denominator: str
    count_semantics: str
    unavailable_metrics: tuple[str, ...]
    query_kind: str
    description: str

    def public_dict(self) -> dict[str, Any]:
        return {
            "name": self.name,
            "schema_version": REPORT_SCHEMA_VERSION,
            "definition_version": REPORT_SCHEMA_VERSION,
            "source_table": self.source_table,
            "source_report": self.source_report,
            "grain": self.grain,
            "dimensions": list(self.dimensions),
            "metrics": list(self.metrics),
            "supported_filters": list(self.supported_filters),
            "units": dict(self.units),
            "denominator": self.denominator,
            "count_semantics": self.count_semantics,
            "unavailable_metrics": list(self.unavailable_metrics),
            "account_metrics_available": False,
            "description": self.description,
        }


UNAVAILABLE = (
    "account_distinct_count",
    "account_duration_usec",
    "xp_per_hour",
    "progression_metrics",
    "reward_metrics",
    "raw_fact_drilldown",
    "chat_or_ip_metrics",
    "live_character_location",
)

REPORT_DEFINITIONS = {
    "playtime": ReportDefinition(
        name="playtime",
        source_table="telemetry_rollup_session",
        source_report="session_playtime",
        grain="original_logical_session",
        dimensions=("subject_id", "pid", "session_boot_id", "session_process_id", "session_seq"),
        metrics=(
            "covered_connected_usec",
            "covered_active_usec",
            "covered_idle_usec",
            "covered_unknown_usec",
            "covered_resident_usec",
            "covered_linkdead_usec",
            "attributable_usec",
            "observed_intervals",
            "entered",
            "exited",
            "quality_flags",
        ),
        supported_filters=("subject_id",),
        units={
            "*_usec": "microseconds",
            "*_hours": "hours (display conversion; *_usec is authoritative)",
            "active_fraction": "fraction",
            "attributable_fraction": "fraction",
            "observed_intervals": "count",
        },
        denominator=(
            "active_fraction uses covered_active_usec / covered_resident_usec; "
            "attributable_fraction uses attributable_usec / covered_resident_usec. "
            "Checkpoint counters are a separate absolute plane and are not summed."
        ),
        count_semantics=(
            "session_count is logical-session rows; subject_count is distinct "
            "telemetry subjects in the page, never an account count."
        ),
        unavailable_metrics=UNAVAILABLE,
        query_kind="playtime",
        description="Connected, active, idle, linkdead, resident and attributable time from sealed session coverage.",
    ),
    "cohort": ReportDefinition(
        name="cohort",
        source_table="telemetry_cohort_day",
        source_report="cohort_activity",
        grain="utc_day_and_captured_context_and_activity_category",
        dimensions=(
            "utc_day",
            "level_band",
            "class_id",
            "race_id",
            "faction_id",
            "zone_vnum",
            "config_id",
            "category",
        ),
        metrics=("duration_usec", "attributable_usec", "observed_intervals", "subject_count", "session_count", "quality_flags"),
        supported_filters=("date_from", "date_to", *COHORT_FILTER_FIELDS),
        units={
            "duration_usec": "microseconds",
            "attributable_usec": "microseconds",
            "*_hours": "hours (display conversion)",
            "attributable_fraction": "fraction",
            "subject_count": "daily cohort members (count)",
            "session_count": "daily cohort logical sessions (count)",
        },
        denominator="attributable_fraction uses attributable_usec / duration_usec; zero denominator is null.",
        count_semantics=(
            "subject_count and session_count are per-row daily cohort cardinalities. "
            "They are not additive across days, dimensions or categories."
        ),
        unavailable_metrics=UNAVAILABLE,
        query_kind="cohort",
        description="Published day/context/activity aggregate rows with captured cohort dimensions.",
    ),
    "return": ReportDefinition(
        name="return",
        source_table="telemetry_cohort_member",
        source_report="cohort_activity membership support",
        grain="subject_within_filtered_member_population",
        dimensions=("subject_id",),
        metrics=("logical_session_count", "returning_subject", "return_fraction", "quality_flags"),
        supported_filters=("date_from", "date_to", *COHORT_FILTER_FIELDS, "subject_id"),
        units={"logical_session_count": "count", "returning_subject": "boolean", "return_fraction": "fraction"},
        denominator=(
            "return_fraction uses returning subjects / subjects with at least one "
            "matching logical-session member row. It is not account retention."
        ),
        count_semantics=(
            "A returning subject has at least two distinct original logical session "
            "IDs in the filtered member population. The report does not infer login "
            "dates or account identity."
        ),
        unavailable_metrics=UNAVAILABLE,
        query_kind="return",
        description="Bounded distinct-subject multi-session indicator from declared cohort membership.",
    ),
    "time": ReportDefinition(
        name="time",
        source_table="telemetry_cohort_day",
        source_report="cohort_activity",
        grain="utc_day_and_activity_category",
        dimensions=("utc_day", "category"),
        metrics=("duration_usec", "attributable_usec", "observed_intervals", "cohort_bucket_count", "quality_flags"),
        supported_filters=("date_from", "date_to", *COHORT_FILTER_FIELDS),
        units={
            "duration_usec": "microseconds",
            "attributable_usec": "microseconds",
            "*_hours": "hours (display conversion)",
            "attributable_fraction": "fraction",
            "cohort_bucket_count": "count",
        },
        denominator="attributable_fraction uses attributable_usec / duration_usec; zero denominator is null.",
        count_semantics=(
            "cohort_bucket_count counts aggregate buckets. Daily subject/session "
            "counts remain per-cohort-row values and are not summed here."
        ),
        unavailable_metrics=UNAVAILABLE,
        query_kind="time",
        description=(
            "Activity-band summary by UTC day. category is active/idle/unknown/linkdead, "
            "not a local-clock hour band."
        ),
    ),
    "faction": ReportDefinition(
        name="faction",
        source_table="telemetry_cohort_day",
        source_report="cohort_activity",
        grain="captured_faction",
        dimensions=("faction_id",),
        metrics=("duration_usec", "attributable_usec", "observed_intervals", "cohort_bucket_count", "quality_flags"),
        supported_filters=("date_from", "date_to", *COHORT_FILTER_FIELDS),
        units={
            "duration_usec": "microseconds",
            "attributable_usec": "microseconds",
            "*_hours": "hours (display conversion)",
            "attributable_fraction": "fraction",
            "cohort_bucket_count": "count",
        },
        denominator="attributable_fraction uses attributable_usec / duration_usec; zero denominator is null.",
        count_semantics=(
            "cohort_bucket_count is an aggregate-row count. Subject/session counts "
            "are not globally distinct at this grouping and are intentionally omitted."
        ),
        unavailable_metrics=UNAVAILABLE,
        query_kind="faction",
        description="Activity duration and coverage grouped by captured faction ID.",
    ),
}

REPORT_ALIASES = {
    "playtime": "playtime",
    "session": "playtime",
    "session_playtime": "playtime",
    "cohort": "cohort",
    "activity": "cohort",
    "cohort_activity": "cohort",
    "return": "return",
    "returns": "return",
    "time": "time",
    "time_band": "time",
    "time-band": "time",
    "timeband": "time",
    "faction": "faction",
}


@dataclass(frozen=True, slots=True)
class ReportFilters:
    """Exact filters shared by aggregate and member queries."""

    date_from: date | None = None
    date_to: date | None = None
    level_band: int | None = None
    class_id: int | None = None
    race_id: int | None = None
    faction_id: int | None = None
    zone_vnum: int | None = None
    config_id: int | None = None
    category: int | None = None
    subject_id: int | None = None

    def validate(self, definition: ReportDefinition) -> None:
        if self.date_from is not None and not isinstance(self.date_from, date):
            raise ValueError("date_from must be a UTC date")
        if self.date_to is not None and not isinstance(self.date_to, date):
            raise ValueError("date_to must be a UTC date")
        if self.date_from is not None and self.date_to is not None and self.date_from >= self.date_to:
            raise ValueError("date_from must be before date_to; the end is exclusive")
        values = {
            "level_band": self.level_band,
            "class_id": self.class_id,
            "race_id": self.race_id,
            "faction_id": self.faction_id,
            "zone_vnum": self.zone_vnum,
            "config_id": self.config_id,
            "category": self.category,
            "subject_id": self.subject_id,
        }
        for name, value in values.items():
            if value is None:
                continue
            if name not in definition.supported_filters:
                raise ValueError(f"{name} is not a supported filter for {definition.name}")
            if isinstance(value, bool) or not isinstance(value, int):
                raise ValueError(f"{name} must be an integer")
            if name in {"level_band", "class_id", "race_id", "faction_id"} and not 0 <= value <= 65_535:
                raise ValueError(f"{name} must be in 0..65535")
            if name == "zone_vnum" and not INT32_MIN <= value <= INT32_MAX:
                raise ValueError("zone_vnum is outside signed INT range")
            if name == "config_id" and not 1 <= value <= UINT64_MAX:
                raise ValueError("config_id must be a nonzero unsigned 64-bit integer")
            if name == "category" and value not in CATEGORY_NAMES:
                raise ValueError("category must be 0 (unknown), 1 (idle), 2 (active), or 3 (linkdead)")
            if name == "subject_id" and not 1 <= value <= UINT64_MAX:
                raise ValueError("subject_id must be a nonzero unsigned 64-bit integer")

    def public_dict(self) -> dict[str, Any]:
        return {
            "date_from": self.date_from.isoformat() if self.date_from is not None else None,
            "date_to_exclusive": self.date_to.isoformat() if self.date_to is not None else None,
            "level_band": self.level_band,
            "class_id": self.class_id,
            "race_id": self.race_id,
            "faction_id": self.faction_id,
            "zone_vnum": self.zone_vnum,
            "config_id": self.config_id,
            "category": self.category,
            "subject_id": self.subject_id,
        }


@dataclass(frozen=True, slots=True)
class ReportRequest:
    """A fully scoped, bounded administrator report request."""

    report_name: str
    definition_version: int
    environment_id: int
    season_id: int
    generation: int | None = None
    filters: ReportFilters = field(default_factory=ReportFilters)
    after: str | None = None
    max_rows: int = REPORT_ROW_LIMIT_DEFAULT
    max_bytes: int = REPORT_BYTE_LIMIT_DEFAULT
    max_runtime_s: float = REPORT_RUNTIME_DEFAULT_S

    @property
    def canonical_name(self) -> str:
        return canonical_report_name(self.report_name)

    def validate(self) -> None:
        definition = report_definition(self.report_name)
        if isinstance(self.definition_version, bool) or not isinstance(self.definition_version, int) or self.definition_version != REPORT_SCHEMA_VERSION:
            raise ValueError("definition_version must be 1")
        for name, value in (("environment_id", self.environment_id), ("season_id", self.season_id)):
            if isinstance(value, bool) or not isinstance(value, int) or not 1 <= value <= UINT64_MAX:
                raise ValueError(f"{name} must be a nonzero unsigned 64-bit integer")
        if self.generation is not None and (
            isinstance(self.generation, bool)
            or not isinstance(self.generation, int)
            or not 1 <= self.generation <= UINT64_MAX
        ):
            raise ValueError("generation must be a nonzero unsigned 64-bit integer when supplied")
        if isinstance(self.max_rows, bool) or not isinstance(self.max_rows, int) or self.max_rows <= 0:
            raise ValueError("max_rows must be a positive integer")
        if self.max_rows > REPORT_ROW_LIMIT_HARD_MAX:
            raise ValueError(f"max_rows exceeds hard maximum {REPORT_ROW_LIMIT_HARD_MAX}")
        if isinstance(self.max_bytes, bool) or not isinstance(self.max_bytes, int) or self.max_bytes <= 0:
            raise ValueError("max_bytes must be a positive integer")
        if self.max_bytes > REPORT_BYTE_LIMIT_HARD_MAX:
            raise ValueError(f"max_bytes exceeds hard maximum {REPORT_BYTE_LIMIT_HARD_MAX}")
        if self.max_bytes // REPORT_ROW_BYTE_BOUND < REPORT_MIN_FETCH_ROWS:
            raise BoundsExceeded("report byte budget cannot reserve one row and a truncation sentinel")
        if (
            isinstance(self.max_runtime_s, bool)
            or not isinstance(self.max_runtime_s, (int, float))
            or not math.isfinite(self.max_runtime_s)
            or self.max_runtime_s <= 0
        ):
            raise ValueError("max_runtime_s must be a positive finite number")
        if self.max_runtime_s > REPORT_RUNTIME_HARD_MAX:
            raise ValueError("max_runtime_s exceeds hard maximum")
        self.filters.validate(definition)

    def cache_identity(self) -> dict[str, Any]:
        return {
            "schema_version": REPORT_SCHEMA_VERSION,
            "report": self.canonical_name,
            "definition_version": self.definition_version,
            "generation": self.generation,
            "environment_id": self.environment_id,
            "season_id": self.season_id,
            "filters": self.filters.public_dict(),
            "after": self.after,
            "max_rows": self.max_rows,
            "max_bytes": self.max_bytes,
        }


def canonical_report_name(name: str) -> str:
    if not isinstance(name, str) or name not in REPORT_ALIASES:
        raise ValueError(
            f"unknown report {name!r}; available={sorted(REPORT_DEFINITIONS)}"
        )
    return REPORT_ALIASES[name]


def report_definition(name: str) -> ReportDefinition:
    return REPORT_DEFINITIONS[canonical_report_name(name)]


def report_catalog() -> tuple[dict[str, Any], ...]:
    return tuple(REPORT_DEFINITIONS[name].public_dict() for name in sorted(REPORT_DEFINITIONS))


def _json_default(value: Any) -> Any:
    if isinstance(value, date):
        return value.isoformat()
    if isinstance(value, Decimal):
        # PyMySQL returns SUM(BIGINT) and some COUNT expressions as Decimal.
        # Preserve integral counters as JSON integers; never round a fractional
        # value silently if a future aggregate adds one.
        if value == value.to_integral_value():
            return int(value)
        return str(value)
    if isinstance(value, bytes):
        return value.hex()
    raise TypeError(f"not JSON serializable: {type(value).__name__}")


def _parse_date(value: str | None, name: str) -> date | None:
    if value is None:
        return None
    try:
        return date.fromisoformat(value)
    except (TypeError, ValueError) as error:
        raise ValueError(f"{name} must be an ISO UTC date YYYY-MM-DD") from error


def _hours(value: int) -> float:
    return round(int(value) / MICROSECONDS_PER_HOUR, 6)


def _rate(numerator: int, denominator: int, *, name: str) -> dict[str, Any]:
    if numerator < 0 or denominator < 0:
        raise ReportError(f"{name} rate inputs must be nonnegative")
    return {
        "value": None if denominator == 0 else round(numerator / denominator, 10),
        "numerator": numerator,
        "denominator": denominator,
        "unit": "fraction",
        "zero_denominator_policy": "null",
    }


def _quality_or(rows: Sequence[Mapping[str, Any]]) -> int:
    value = 0
    for row in rows:
        value |= int(row.get("quality_flags", 0) or 0)
    return value


def _config_ids(rows: Sequence[Mapping[str, Any]]) -> list[int]:
    return sorted({int(row["config_id"]) for row in rows if row.get("config_id") is not None})


def _summarize_playtime(rows: Sequence[Mapping[str, Any]], complete: bool) -> dict[str, Any]:
    sums = {
        name: sum(int(row.get(name, 0) or 0) for row in rows)
        for name in COVERED_COUNTER_FIELDS
    }
    attributable = sum(int(row.get("attributable_usec", 0) or 0) for row in rows)
    subjects = {int(row["subject_id"]) for row in rows}
    available = sum(int(row.get("latest_checkpoint_revision", 0) or 0) > 0 for row in rows)
    connected = sums["covered_connected_usec"]
    active = sums["covered_active_usec"]
    idle = sums["covered_idle_usec"]
    unknown = sums["covered_unknown_usec"]
    resident = sums["covered_resident_usec"]
    linkdead = sums["covered_linkdead_usec"]
    return {
        "complete": complete,
        "session_count": len(rows),
        "subject_count": len(subjects),
        "checkpoint_totals_available_session_count": available,
        "checkpoint_totals_unavailable_session_count": len(rows) - available,
        "checkpoint_totals_summed": False,
        "observed_intervals": sum(int(row.get("observed_intervals", 0) or 0) for row in rows),
        "entered_sessions": sum(int(row.get("entered", 0) or 0) for row in rows),
        "exited_sessions": sum(int(row.get("exited", 0) or 0) for row in rows),
        "covered_connected_usec": connected,
        "covered_active_usec": active,
        "covered_idle_usec": idle,
        "covered_unknown_usec": unknown,
        "covered_resident_usec": resident,
        "covered_linkdead_usec": linkdead,
        "attributable_usec": attributable,
        "covered_connected_hours": _hours(connected),
        "covered_active_hours": _hours(active),
        "covered_idle_hours": _hours(idle),
        "covered_unknown_hours": _hours(unknown),
        "covered_resident_hours": _hours(resident),
        "covered_linkdead_hours": _hours(linkdead),
        "attributable_hours": _hours(attributable),
        "active_fraction": _rate(active, resident, name="active_fraction"),
        "attributable_fraction": _rate(attributable, resident, name="attributable_fraction"),
        "quality_flags": _quality_or(rows),
    }


def _summarize_cohort(rows: Sequence[Mapping[str, Any]], complete: bool, *, grouping: str) -> dict[str, Any]:
    duration = sum(int(row.get("duration_usec", 0) or 0) for row in rows)
    attributable = sum(int(row.get("attributable_usec", 0) or 0) for row in rows)
    buckets = sum(int(row.get("cohort_bucket_count", 1) or 0) for row in rows)
    return {
        "complete": complete,
        "grouping": grouping,
        "cohort_row_count": len(rows),
        "cohort_bucket_count": buckets,
        "duration_usec": duration,
        "attributable_usec": attributable,
        "duration_hours": _hours(duration),
        "attributable_hours": _hours(attributable),
        "observed_intervals": sum(int(row.get("observed_intervals", 0) or 0) for row in rows),
        "attributable_fraction": _rate(attributable, duration, name="attributable_fraction"),
        "quality_flags": _quality_or(rows),
        "distinct_counts_additive": False,
    }


def _return_rows(
    rows: Sequence[Mapping[str, Any]],
) -> tuple[list[dict[str, Any]], dict[int, set[tuple[int, int, int]],], bool]:
    sessions: dict[int, set[tuple[int, int, int]]] = {}
    quality: dict[int, int] = {}
    observed: dict[int, int] = {}
    contribution_duration: dict[int, int] = {}
    contribution_attributable: dict[int, int] = {}
    for row in rows:
        subject = int(row["subject_id"])
        identity = (
            int(row["session_boot_id"]),
            int(row["session_process_id"]),
            int(row["session_seq"]),
        )
        sessions.setdefault(subject, set()).add(identity)
        quality[subject] = quality.get(subject, 0) | int(row.get("quality_flags", 0) or 0)
        observed[subject] = observed.get(subject, 0) + int(row.get("observed_intervals", 0) or 0)
        contribution_duration[subject] = contribution_duration.get(subject, 0) + int(row.get("duration_usec", 0) or 0)
        contribution_attributable[subject] = contribution_attributable.get(subject, 0) + int(row.get("attributable_usec", 0) or 0)
    output = [
        {
            "subject_id": subject,
            "logical_session_count": len(sessions[subject]),
            "logical_session_ids": [list(identity) for identity in sorted(sessions[subject])],
            "returning_subject": len(sessions[subject]) >= 2,
            "member_observed_intervals": observed[subject],
            "member_duration_usec": contribution_duration[subject],
            "member_attributable_usec": contribution_attributable[subject],
            "quality_flags": quality[subject],
        }
        for subject in sorted(sessions)
    ]
    return output, sessions, False


def _summarize_return(
    rows: Sequence[Mapping[str, Any]],
    complete: bool,
    partial_subject: bool,
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    output, sessions, _ = _return_rows(rows)
    subjects = len(sessions)
    returning = sum(len(values) >= 2 for values in sessions.values())
    logical_sessions = len({identity for values in sessions.values() for identity in values})
    complete = complete and not partial_subject
    for row in output:
        row["complete"] = complete
        if not complete and not row["returning_subject"]:
            row["returning_subject"] = None
    fraction = _rate(returning, subjects, name="return_fraction")
    if not complete:
        fraction["value"] = None
        fraction["unavailable_reason"] = "incomplete_population"
    return output, {
        "complete": complete,
        "count_scope": "filtered_population" if complete else "page_only_not_additive",
        "member_row_count": len(rows),
        "subject_count": subjects,
        "returning_subject_count": returning,
        "logical_session_count": logical_sessions,
        "return_fraction": fraction,
        "quality_flags": _quality_or(rows),
        "denominator": "subjects with at least one matching logical-session member row",
        "partial_subject_at_boundary": partial_subject,
        "distinct_account_count": None,
    }


def _public_session(row: Mapping[str, Any]) -> dict[str, Any]:
    """Expose checkpoint totals only when the stored revision is available."""
    output = dict(row)
    for field_name, value in tuple(output.items()):
        if isinstance(value, Decimal):
            output[field_name] = int(value) if value == value.to_integral_value() else str(value)
    available = int(output.get("latest_checkpoint_revision", 0) or 0) > 0
    output["checkpoint_totals_available"] = available
    if not available:
        for field_name in COUNTER_FIELDS:
            output[field_name] = None
    return output


def _public_day(row: Mapping[str, Any]) -> dict[str, Any]:
    output = dict(row)
    for field_name, value in tuple(output.items()):
        if isinstance(value, Decimal):
            output[field_name] = int(value) if value == value.to_integral_value() else str(value)
    if "utc_day" in output:
        if output.get("utc_day") == UNKNOWN_DAY:
            output["utc_day"] = None
            output["bucket_kind"] = "unknown"
        else:
            if isinstance(output.get("utc_day"), date):
                output["utc_day"] = output["utc_day"].isoformat()
            output["bucket_kind"] = "calendar"
    if "category" in output:
        output["category_name"] = CATEGORY_NAMES.get(int(output["category"]), "unknown")
    return output


def _cursor_value(value: Any) -> Any:
    return value.isoformat() if isinstance(value, date) else value


def _encode_cursor(
    request: ReportRequest,
    target: RollupTarget,
    order_fields: Sequence[str],
    row: Mapping[str, Any],
) -> str:
    payload = {
        "version": 1,
        "report": request.canonical_name,
        "scope": list(target.scope_tuple),
        "filters": request.filters.public_dict(),
        "order": list(order_fields),
        "values": [_cursor_value(row[field]) for field in order_fields],
    }
    raw = json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    return base64.urlsafe_b64encode(raw).rstrip(b"=").decode("ascii")


def _decode_cursor(
    token: str,
    request: ReportRequest,
    target: RollupTarget,
    order_fields: Sequence[str],
) -> tuple[Any, ...]:
    if not isinstance(token, str) or not token or len(token) > 4_096:
        raise ValueError("after cursor must be a nonempty bounded token")
    try:
        padded = token + "=" * (-len(token) % 4)
        raw = base64.b64decode(padded.encode("ascii"), altchars=b"-_", validate=True)
        payload = json.loads(raw.decode("utf-8"))
    except (ValueError, UnicodeError, json.JSONDecodeError) as error:
        raise ValueError("after cursor is not valid report cursor data") from error
    if not isinstance(payload, dict) or payload.get("version") != 1:
        raise ValueError("after cursor version is unsupported")
    if payload.get("report") != request.canonical_name:
        raise ValueError("after cursor belongs to a different report")
    if payload.get("scope") != list(target.scope_tuple):
        raise ValueError("after cursor belongs to a different report scope")
    if payload.get("filters") != request.filters.public_dict():
        raise ValueError("after cursor belongs to different filters")
    if payload.get("order") != list(order_fields):
        raise ValueError("after cursor order is incompatible with this report")
    values = payload.get("values")
    if not isinstance(values, list) or len(values) != len(order_fields):
        raise ValueError("after cursor has the wrong key shape")
    output: list[Any] = []
    for field_name, value in zip(order_fields, values, strict=True):
        if field_name == "utc_day":
            if not isinstance(value, str):
                raise ValueError("after cursor UTC key is invalid")
            output.append(_parse_date(value, "after UTC key"))
        else:
            if isinstance(value, bool) or not isinstance(value, int):
                raise ValueError("after cursor numeric key is invalid")
            output.append(value)
    return tuple(output)


def _seek_predicate(order_fields: Sequence[str]) -> str:
    terms: list[str] = []
    for index, field_name in enumerate(order_fields):
        equal = " AND ".join(f"{previous}=%s" for previous in order_fields[:index])
        terms.append(f"({equal + ' AND ' if equal else ''}{field_name}>%s)")
    return "(" + " OR ".join(terms) + ")"


def _seek_parameters(values: Sequence[Any]) -> tuple[Any, ...]:
    output: list[Any] = []
    for index, value in enumerate(values):
        output.extend(values[:index])
        output.append(value)
    return tuple(output)


def _append_cohort_filters(
    where: list[str],
    parameters: list[Any],
    filters: ReportFilters,
) -> None:
    if filters.date_from is not None:
        where.append("utc_day >= %s")
        parameters.append(filters.date_from)
    if filters.date_to is not None:
        where.append("utc_day < %s")
        parameters.append(filters.date_to)
    for field_name in COHORT_FILTER_FIELDS:
        value = getattr(filters, field_name)
        if value is not None:
            where.append(f"{field_name}=%s")
            parameters.append(value)


def _scope_where(target: RollupTarget) -> tuple[list[str], list[Any]]:
    return [
        "definition_version=%s",
        "generation=%s",
        "environment_id=%s",
        "season_id=%s",
    ], list(target.scope_tuple)


def _build_query(
    request: ReportRequest,
    target: RollupTarget,
    after_values: Sequence[Any] | None,
    limit: int,
) -> tuple[str, tuple[Any, ...], tuple[str, ...]]:
    definition = report_definition(request.report_name)
    where, parameters = _scope_where(target)
    filters = request.filters
    kind = definition.query_kind
    group_by = ""
    if kind == "playtime":
        if filters.subject_id is not None:
            where.append("subject_id=%s")
            parameters.append(filters.subject_id)
        order_fields = ("subject_id", "session_boot_id", "session_process_id", "session_seq")
        statement = (
            "SELECT " + ",".join(SESSION_REPORT_COLUMNS)
            + " FROM telemetry_rollup_session FORCE INDEX (idx_rollup_session_subject) WHERE "
            + " AND ".join(where)
        )
    elif kind == "return":
        where.append("membership_kind=%s")
        parameters.append(2)
        if filters.subject_id is not None:
            where.append("subject_id=%s")
            parameters.append(filters.subject_id)
        _append_cohort_filters(where, parameters, filters)
        order_fields = (
            "utc_day",
            "level_band",
            "class_id",
            "race_id",
            "faction_id",
            "zone_vnum",
            "config_id",
            "category",
            "subject_id",
            "session_boot_id",
            "session_process_id",
            "session_seq",
        )
        statement = (
            "SELECT " + ",".join(MEMBER_REPORT_COLUMNS)
            + " FROM telemetry_cohort_member FORCE INDEX (PRIMARY) WHERE "
            + " AND ".join(where)
        )
    elif kind == "cohort":
        _append_cohort_filters(where, parameters, filters)
        order_fields = (
            "utc_day",
            "level_band",
            "class_id",
            "race_id",
            "faction_id",
            "zone_vnum",
            "config_id",
            "category",
        )
        statement = (
            "SELECT " + ",".join(COHORT_REPORT_COLUMNS)
            + " FROM telemetry_cohort_day FORCE INDEX (PRIMARY) WHERE "
            + " AND ".join(where)
        )
    elif kind == "time":
        _append_cohort_filters(where, parameters, filters)
        order_fields = ("utc_day", "category")
        statement = (
            "SELECT utc_day,category,SUM(duration_usec) AS duration_usec,"
            "SUM(attributable_usec) AS attributable_usec,"
            "SUM(observed_intervals) AS observed_intervals,COUNT(*) AS cohort_bucket_count,"
            "BIT_OR(quality_flags) AS quality_flags,MAX(input_watermark) AS input_watermark,"
            "MAX(provisional) AS provisional FROM telemetry_cohort_day FORCE INDEX (PRIMARY) WHERE "
            + " AND ".join(where)
        )
        group_by = " GROUP BY utc_day,category"
    elif kind == "faction":
        _append_cohort_filters(where, parameters, filters)
        order_fields = ("faction_id",)
        statement = (
            "SELECT faction_id,SUM(duration_usec) AS duration_usec,"
            "SUM(attributable_usec) AS attributable_usec,"
            "SUM(observed_intervals) AS observed_intervals,COUNT(*) AS cohort_bucket_count,"
            "BIT_OR(quality_flags) AS quality_flags,MAX(input_watermark) AS input_watermark,"
            "MAX(provisional) AS provisional FROM telemetry_cohort_day FORCE INDEX (PRIMARY) WHERE "
            + " AND ".join(where)
        )
        group_by = " GROUP BY faction_id"
    else:  # pragma: no cover - definitions are immutable in this module.
        raise ValueError(f"report has no query implementation: {kind}")
    if after_values is not None:
        statement += " AND " + _seek_predicate(order_fields)
        parameters.extend(_seek_parameters(after_values))
    statement += group_by + " ORDER BY " + ",".join(order_fields) + " LIMIT %s"
    parameters.append(limit)
    return statement, tuple(parameters), tuple(order_fields)


@dataclass(frozen=True, slots=True)
class _QueryPage:
    rows: tuple[Mapping[str, Any], ...]
    sentinel: Mapping[str, Any] | None
    truncated: bool
    next_cursor: str | None
    source_rows_fetched: int


def _enforce_response_budget(response: Mapping[str, Any], request: ReportRequest) -> None:
    # Include definitions, coverage, summaries, cursors, and expanded identities,
    # not just estimated source rows. Match the JSON CLI wire representation.
    encoded = (json.dumps(response, default=_json_default, sort_keys=True) + "\n").encode("utf-8")
    if len(encoded) > request.max_bytes:
        raise BoundsExceeded("serialized report including metadata exceeds max_bytes; increase budget or reduce max_rows")


class ReportDatabase:
    """One dedicated SELECT-only connection for one report read."""

    def __init__(self, connection_factory: PyMySQLConnectionFactory, *, clock: Callable[[], float] = time.monotonic) -> None:
        self.connection_factory = connection_factory
        self.clock = clock
        self.connection: Any | None = None
        self.deadline: float | None = None

    def close(self) -> None:
        if self.connection is not None:
            connection = self.connection
            self.connection = None
            try:
                try:
                    connection.rollback()
                except Exception:
                    pass
                self.connection_factory.close(connection)
            except Exception:
                try:
                    connection.close()
                except Exception:
                    pass

    def _check_deadline(self) -> None:
        if self.deadline is not None and self.clock() >= self.deadline:
            raise BoundsExceeded("report operation exceeded max_runtime_s")

    def _ensure_connection(self) -> Any:
        if self.connection is None:
            try:
                self.connection = self.connection_factory.connect()
            except Exception as error:
                raise ReportDatabaseError("dedicated report connection could not be opened") from error
        return self.connection

    def _execute(self, statement: str, parameters: Sequence[Any] = ()) -> list[Mapping[str, Any]]:
        self._check_deadline()
        connection = self._ensure_connection()
        cursor = connection.cursor()
        try:
            cursor.execute(statement, tuple(parameters))
            rows = list(cursor.fetchall()) if getattr(cursor, "description", None) else []
            self._check_deadline()
            return rows
        except BoundsExceeded:
            raise
        except Exception as error:
            raise ReportDatabaseError("dedicated report SQL read failed") from error
        finally:
            try:
                cursor.close()
            except Exception:
                pass

    def _begin_snapshot(self) -> None:
        self._execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        self._execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")

    def _read_published_state(self, request: ReportRequest) -> Mapping[str, Any]:
        columns = ",".join(STATE_REPORT_COLUMNS)
        if request.generation is not None:
            statement = (
                "SELECT " + columns + " FROM telemetry_rollup_state FORCE INDEX (PRIMARY) WHERE "
                "definition_version=%s AND generation=%s AND environment_id=%s AND season_id=%s "
                "AND publication_status=%s LIMIT 1"
            )
            parameters = (*(
                request.definition_version,
                request.generation,
                request.environment_id,
                request.season_id,
                PUBLICATION_PUBLISHED,
            ),)
        else:
            statement = (
                "SELECT " + columns + " FROM telemetry_rollup_state FORCE INDEX (PRIMARY) WHERE "
                "definition_version=%s AND environment_id=%s AND season_id=%s "
                "AND publication_status=%s ORDER BY generation DESC LIMIT 1"
            )
            parameters = (
                request.definition_version,
                request.environment_id,
                request.season_id,
                PUBLICATION_PUBLISHED,
            )
        rows = self._execute(statement, parameters)
        if not rows:
            raise ReportNotPublished("requested scope has no published generation")
        return rows[0]

    def _read_page(self, request: ReportRequest, target: RollupTarget) -> _QueryPage:
        definition = report_definition(request.report_name)
        order_fields: tuple[str, ...]
        # The order shape is fixed by the report kind.  Decode only after the
        # published generation is known so a cursor cannot cross generations.
        after_values = None
        if request.after is not None:
            _, _, order_fields = _build_query(request, target, None, 1)
            after_values = _decode_cursor(request.after, request, target, order_fields)
        result_capacity = min(request.max_rows, request.max_bytes // REPORT_ROW_BYTE_BOUND - 1)
        if result_capacity <= 0:
            raise BoundsExceeded("report byte budget cannot reserve one row and a truncation sentinel")
        statement, parameters, order_fields = _build_query(
            request,
            target,
            after_values,
            result_capacity + 1,
        )
        rows = self._execute(statement, parameters)
        estimated = sum(estimate_row_bytes(row) for row in rows)
        if estimated > request.max_bytes:
            raise BoundsExceeded("report rows exceed max_bytes")
        if len(rows) > result_capacity + 1:
            raise ReportDatabaseError("report SQL returned more rows than its LIMIT")
        truncated = len(rows) > result_capacity
        materialized = tuple(rows[:result_capacity])
        next_cursor = (
            _encode_cursor(request, target, order_fields, materialized[-1])
            if truncated and materialized
            else None
        )
        sentinel = rows[result_capacity] if truncated else None
        return _QueryPage(
            rows=materialized,
            sentinel=sentinel,
            truncated=truncated,
            next_cursor=next_cursor,
            source_rows_fetched=len(rows),
        )

    def read(self, request: ReportRequest) -> dict[str, Any]:
        request.validate()
        self.deadline = self.clock() + float(request.max_runtime_s)
        try:
            self._begin_snapshot()
            state = self._read_published_state(request)
            target = RollupTarget(
                request.definition_version,
                int(state["generation"]),
                request.environment_id,
                request.season_id,
            )
            page = self._read_page(request, target)
            definition = report_definition(request.report_name)
            source_rows = page.rows
            public_rows: Sequence[Mapping[str, Any]] = source_rows
            if definition.query_kind in {"cohort", "time", "faction"}:
                public_rows = [_public_day(row) for row in source_rows]
            elif definition.query_kind == "playtime":
                public_rows = [_public_session(row) for row in source_rows]
            complete = not page.truncated and request.after is None
            # Date/dimension order does not keep a subject's rows contiguous.
            # Any bounded page may omit another session of any shown subject.
            partial_subject = definition.query_kind == "return" and not complete
            if definition.query_kind == "return":
                public_rows, summary = _summarize_return(source_rows, complete, partial_subject)
            elif definition.query_kind == "playtime":
                summary = _summarize_playtime(source_rows, complete)
            else:
                grouping = "utc_day_and_category" if definition.query_kind == "time" else (
                    "faction_id" if definition.query_kind == "faction" else "cohort_bucket"
                )
                summary = _summarize_cohort(source_rows, complete, grouping=grouping)
            coverage = coverage_from_state_row(
                target,
                state,
                snapshot_high_watermark=int(state["rebuild_through_ingest_id"]),
            )
            coverage_public = coverage.public_dict()
            coverage_public["occurrence_bounds_known"] = coverage.occurrence_bounds_known
            coverage_public["input_watermark"] = coverage.input_watermark
            coverage_public["as_of_watermark"] = coverage.input_watermark
            versions = {
                "report_schema_version": REPORT_SCHEMA_VERSION,
                "definition_version": request.definition_version,
                "config_ids": _config_ids(source_rows),
                "classifier_version": None,
                "policy_version": None,
            }
            response = {
                "report_schema_version": REPORT_SCHEMA_VERSION,
                "report": definition.name,
                "definition": definition.public_dict(),
                "scope": {
                    "definition_version": request.definition_version,
                    "generation": target.generation,
                    "environment_id": target.environment_id,
                    "season_id": target.season_id,
                },
                "versions": versions,
                "filters": request.filters.public_dict(),
                "coverage": coverage_public,
                "watermark": coverage_public["as_of_watermark"],
                "provisional": bool(coverage.provisional),
                "summary": summary,
                "page": {
                    "max_rows": request.max_rows,
                    "max_bytes": request.max_bytes,
                    "source_rows_fetched": page.source_rows_fetched,
                    "rows_returned": len(public_rows),
                    "truncated": page.truncated,
                    "has_more": page.truncated,
                    "next_cursor": page.next_cursor,
                },
                "rows": public_rows,
                "truncated": page.truncated,
                "has_more": page.truncated,
                "next_cursor": page.next_cursor,
                "served_from_cache": False,
                "cache": {"served": False, "status": "fresh_published"},
            }
            _enforce_response_budget(response, request)
            self._check_deadline()
            return response
        finally:
            self.close()
            self.deadline = None


class ReportCache:
    """Atomic mode-0600 cache for exact request pages."""

    def __init__(self, directory: str | os.PathLike[str]) -> None:
        self.directory = Path(directory)

    @staticmethod
    def _key(request: ReportRequest, namespace: str = "") -> str:
        raw = json.dumps({"source": namespace, "request": request.cache_identity()}, sort_keys=True, separators=(",", ":")).encode("utf-8")
        return hashlib.sha256(raw).hexdigest()

    def _path(self, request: ReportRequest, namespace: str = "") -> Path:
        return self.directory / (self._key(request, namespace) + ".json")

    def write(self, request: ReportRequest, payload: Mapping[str, Any], *, namespace: str = "") -> None:
        try:
            self.directory.mkdir(parents=True, exist_ok=True, mode=0o700)
            path = self._path(request, namespace)
            body = {
                "cache_schema_version": CACHE_SCHEMA_VERSION,
                "request_key": self._key(request, namespace),
                "payload": payload,
            }
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                dir=self.directory,
                prefix=".report-cache-",
                suffix=".tmp",
                delete=False,
            ) as stream:
                temporary = Path(stream.name)
                os.chmod(temporary, 0o600)
                json.dump(body, stream, default=_json_default, sort_keys=True, separators=(",", ":"))
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, path)
            os.chmod(path, 0o600)
        except (OSError, TypeError, ValueError) as error:
            raise ReportCacheError("report cache write failed") from error

    def read(self, request: ReportRequest, *, namespace: str = "") -> dict[str, Any] | None:
        path = self._path(request, namespace)
        try:
            # The small cache wrapper is outside the emitted-response budget.
            with path.open("rb") as stream:
                raw = stream.read(request.max_bytes + 1025)
            if len(raw) > request.max_bytes + 1024:
                return None
            body = json.loads(raw)
        except (FileNotFoundError, OSError, json.JSONDecodeError, UnicodeDecodeError):
            return None
        if not isinstance(body, dict) or body.get("cache_schema_version") != CACHE_SCHEMA_VERSION:
            return None
        if body.get("request_key") != self._key(request, namespace) or not isinstance(body.get("payload"), dict):
            return None
        return deepcopy(body["payload"])


class AdministratorReportService:
    """Read fresh data, then fall back to the last exact published page."""

    def __init__(self, connection_factory: PyMySQLConnectionFactory, *, cache: ReportCache | None = None) -> None:
        self.connection_factory = connection_factory
        self.cache = cache
        self.cache_namespace = ""
        if cache is not None:
            settings = connection_factory.settings
            identity = {"host": settings.host, "port": settings.port,
                        "database": settings.database, "user": settings.user,
                        "schema": REPORT_SCHEMA_VERSION}
            self.cache_namespace = hashlib.sha256(json.dumps(
                identity, sort_keys=True, separators=(",", ":")
            ).encode("utf-8")).hexdigest()

    def read(self, request: ReportRequest) -> dict[str, Any]:
        request.validate()
        try:
            response = ReportDatabase(self.connection_factory).read(request)
        except (ReportDatabaseError, ReportNotPublished):
            if self.cache is None:
                raise
            cached = self.cache.read(request, namespace=self.cache_namespace)
            if cached is None:
                raise
            cached["served_from_cache"] = True
            cached["cache"] = {
                "served": True,
                "status": "last_published",
            }
            _enforce_response_budget(cached, request)
            return cached
        if self.cache is not None:
            try:
                self.cache.write(request, response, namespace=self.cache_namespace)
            except ReportCacheError:
                # Fresh published output is still valid when the optional cache
                # filesystem is unavailable.  The response says it was fresh.
                response["cache"] = {"served": False, "status": "write_failed"}
        _enforce_response_budget(response, request)
        return response


def _settings_from_args(args: argparse.Namespace) -> ConnectionSettings:
    prefix = REPORT_PREFIX
    password = os.environ.get(prefix + "PASSWORD")
    if password is None:
        password = os.environ.get(prefix + "PASSWD")
    host = args.host or os.environ.get(prefix + "HOST")
    database = args.database or os.environ.get(prefix + "DATABASE")
    user = args.user or os.environ.get(prefix + "USER")
    missing = []
    if not host:
        missing.append(prefix + "HOST")
    if not database:
        missing.append(prefix + "DATABASE")
    if not user:
        missing.append(prefix + "USER")
    if password is None:
        missing.append(prefix + "PASSWORD")
    if missing:
        raise ValueError("missing explicit report connection values: " + ", ".join(missing))

    def env_bool(name: str, default: bool) -> bool:
        value = os.environ.get(prefix + name)
        if value is None:
            return default
        normalized = value.strip().lower()
        if normalized not in {"0", "1", "false", "true", "no", "yes"}:
            raise ValueError(f"{prefix + name} must be true/false")
        return normalized in {"1", "true", "yes"}

    settings = ConnectionSettings(
        host=host,
        database=database,
        user=user,
        password=password,
        port=args.port if args.port is not None else int(os.environ.get(prefix + "PORT", "3306")),
        tls_ca=args.tls_ca if args.tls_ca is not None else os.environ.get(prefix + "SSL_CA"),
        tls_verify=args.tls_verify if args.tls_verify is not None else env_bool("TLS_VERIFY", False),
        secure_tunnel=args.secure_tunnel if args.secure_tunnel is not None else env_bool("SECURE_TUNNEL", False),
        connect_timeout_s=float(os.environ.get(prefix + "CONNECT_TIMEOUT_S", "2")),
        read_timeout_s=float(os.environ.get(prefix + "READ_TIMEOUT_S", "3")),
        write_timeout_s=float(os.environ.get(prefix + "WRITE_TIMEOUT_S", "3")),
        statement_timeout_s=float(os.environ.get(prefix + "STATEMENT_TIMEOUT_S", "2")),
        lock_timeout_s=float(os.environ.get(prefix + "LOCK_TIMEOUT_S", "2")),
    )
    settings.validate()
    return settings


def _filters_from_args(args: argparse.Namespace) -> ReportFilters:
    return ReportFilters(
        date_from=_parse_date(args.date_from, "date_from"),
        date_to=_parse_date(args.date_to, "date_to"),
        level_band=args.level_band,
        class_id=args.class_id,
        race_id=args.race_id,
        faction_id=args.faction_id,
        zone_vnum=args.zone_vnum,
        config_id=args.config_id,
        category=args.category,
        subject_id=args.subject_id,
    )


def _request_from_args(args: argparse.Namespace) -> ReportRequest:
    return ReportRequest(
        report_name=args.name,
        definition_version=args.definition_version,
        environment_id=args.environment_id,
        season_id=args.season_id,
        generation=args.generation,
        filters=_filters_from_args(args),
        after=args.after,
        max_rows=args.max_rows,
        max_bytes=args.max_bytes,
        max_runtime_s=args.max_runtime_s,
    )


def render_text(payload: Mapping[str, Any]) -> str:
    """Stable, human-readable table without dropping machine metadata."""
    lines = [
        f"report: {payload['report']}",
        f"scope: {json.dumps(payload['scope'], sort_keys=True)}",
        f"watermark: {payload['watermark']}",
        f"provisional: {payload['provisional']}",
        f"served_from_cache: {payload['served_from_cache']}",
        f"coverage: {json.dumps(payload['coverage'], sort_keys=True, default=_json_default)}",
        f"summary: {json.dumps(payload['summary'], sort_keys=True, default=_json_default)}",
        f"page: {json.dumps(payload['page'], sort_keys=True, default=_json_default)}",
        "rows:",
    ]
    for row in payload.get("rows", []):
        lines.append(json.dumps(row, sort_keys=True, default=_json_default))
    return "\n".join(lines)


def render_catalog_text() -> str:
    lines = [f"report_schema_version: {REPORT_SCHEMA_VERSION}", "reports:"]
    for definition in (REPORT_DEFINITIONS[name] for name in sorted(REPORT_DEFINITIONS)):
        lines.append(f"- {definition.name}: {definition.description}")
        lines.append(f"  source: {definition.source_table} ({definition.source_report})")
        lines.append(f"  grain: {definition.grain}")
        lines.append(f"  metrics: {', '.join(definition.metrics)}")
        lines.append(f"  denominator: {definition.denominator}")
        lines.append(f"  counts: {definition.count_semantics}")
        lines.append(f"  unavailable: {', '.join(definition.unavailable_metrics)}")
    return "\n".join(lines)


def _add_connection_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--host", help="explicit report database host; otherwise TELEMETRY_REPORT_DB_HOST")
    parser.add_argument("--port", type=int, help="report database TCP port")
    parser.add_argument("--database", help="explicit report database name; otherwise TELEMETRY_REPORT_DB_DATABASE")
    parser.add_argument("--user", help="explicit report role; otherwise TELEMETRY_REPORT_DB_USER")
    parser.add_argument("--tls-ca", help="CA file for verified non-loopback TLS")
    tls = parser.add_mutually_exclusive_group()
    tls.add_argument("--tls-verify", dest="tls_verify", action="store_true")
    tls.add_argument("--no-tls-verify", dest="tls_verify", action="store_false")
    tunnel = parser.add_mutually_exclusive_group()
    tunnel.add_argument("--secure-tunnel", dest="secure_tunnel", action="store_true")
    tunnel.add_argument("--no-secure-tunnel", dest="secure_tunnel", action="store_false")
    parser.set_defaults(tls_verify=None, secure_tunnel=None)


def _add_request_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--name", required=True, help="playtime, cohort, return, time, or faction")
    parser.add_argument("--definition-version", type=int, required=True)
    parser.add_argument("--generation", type=int, help="exact published generation; omit for newest published")
    parser.add_argument("--environment-id", type=int, required=True)
    parser.add_argument("--season-id", type=int, required=True)
    parser.add_argument("--date-from", "--from-date", dest="date_from", help="inclusive UTC date YYYY-MM-DD")
    parser.add_argument("--date-to", "--to-date", dest="date_to", help="exclusive UTC date YYYY-MM-DD")
    parser.add_argument("--level-band", type=int)
    parser.add_argument("--class-id", type=int)
    parser.add_argument("--race-id", type=int)
    parser.add_argument("--faction-id", type=int)
    parser.add_argument("--zone-vnum", type=int)
    parser.add_argument("--config-id", type=int)
    parser.add_argument("--category", type=int, help="0 unknown, 1 idle, 2 active, 3 linkdead")
    parser.add_argument("--subject-id", type=int)
    parser.add_argument("--after", help="opaque keyset cursor from a prior page")
    parser.add_argument("--max-rows", type=int, default=REPORT_ROW_LIMIT_DEFAULT)
    parser.add_argument("--max-bytes", type=int, default=REPORT_BYTE_LIMIT_DEFAULT)
    parser.add_argument("--max-runtime-s", type=float, default=REPORT_RUNTIME_DEFAULT_S)
    parser.add_argument("--cache-dir", help="optional mode-0700 cache directory; otherwise TELEMETRY_REPORT_CACHE_DIR")
    parser.add_argument("--format", choices=("json", "text"), default="json")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="bounded SELECT-only administrator telemetry reports")
    subparsers = parser.add_subparsers(dest="command", required=True)
    catalog = subparsers.add_parser("catalog", aliases=("definitions",), help="print report catalog without SQL")
    catalog.add_argument("--format", choices=("json", "text"), default="json")
    catalog.set_defaults(handler="catalog")
    report = subparsers.add_parser("report", help="read one published aggregate report")
    _add_connection_arguments(report)
    _add_request_arguments(report)
    report.set_defaults(handler="report")
    return parser


def _run_report_worker(payload: Mapping[str, Any]) -> None:
    try:
        args = argparse.Namespace(**payload)
        request = _request_from_args(args)
        settings = _settings_from_args(args)
        factory = PyMySQLConnectionFactory(settings)
        cache_dir = args.cache_dir or os.environ.get("TELEMETRY_REPORT_CACHE_DIR")
        service = AdministratorReportService(factory, cache=ReportCache(cache_dir) if cache_dir else None)
        response = service.read(request)
        rendered = render_text(response) if args.format == "text" else json.dumps(
            response, default=_json_default, sort_keys=True)
        if len((rendered + "\n").encode("utf-8")) > request.max_bytes:
            raise BoundsExceeded("rendered report exceeds max_bytes")
        print(rendered, flush=True)
    except (ValueError, ReportError, BoundsExceeded) as error:
        print(f"telemetry report failed: {error}", file=sys.stderr, flush=True)
        raise SystemExit(2)
    except BaseException as error:
        print(f"telemetry report failed: {type(error).__name__}", file=sys.stderr, flush=True)
        raise SystemExit(1)


def _run_killable_process(target: Callable[..., Any], args: tuple[Any, ...], *, timeout_s: float) -> int:
    if isinstance(timeout_s, bool) or not isinstance(timeout_s, (int, float)) or not math.isfinite(timeout_s) or timeout_s <= 0:
        raise ValueError("worker timeout_s must be a positive finite number")
    context = multiprocessing.get_context("spawn")
    process = context.Process(target=target, args=args)
    deadline = time.monotonic() + float(timeout_s)
    process.start()
    try:
        while process.is_alive():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            process.join(remaining)
        if process.is_alive():
            killer = getattr(process, "kill", process.terminate)
            killer()
            process.join()
            raise BoundsExceeded("report worker was killed at its max_runtime_s deadline")
        if process.exitcode is None:
            raise ReportError("report worker exited without a status")
        return int(process.exitcode)
    finally:
        if process.is_alive():
            killer = getattr(process, "kill", process.terminate)
            killer()
            process.join()
        process.close()


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.command in {"catalog", "definitions"}:
        if args.format == "text":
            print(render_catalog_text())
        else:
            print(json.dumps({"schema_version": REPORT_SCHEMA_VERSION, "reports": list(report_catalog())}, sort_keys=True))
        return 0
    payload = vars(args).copy()
    payload.pop("handler", None)
    try:
        # Validate locally so malformed requests fail before a child is
        # launched; the child remains the hard boundary for database work.
        _request_from_args(args).validate()
        return _run_killable_process(
            _run_report_worker,
            (payload,),
            timeout_s=float(args.max_runtime_s),
        )
    except (ValueError, ReportError, BoundsExceeded) as error:
        print(f"telemetry report failed: {error}", file=sys.stderr)
        return 2


__all__ = [
    "AdministratorReportService",
    "BoundsExceeded",
    "CATEGORY_NAMES",
    "ReportCache",
    "ReportDatabase",
    "ReportDefinition",
    "ReportFilters",
    "ReportRequest",
    "ReportDatabaseError",
    "ReportError",
    "ReportNotPublished",
    "REPORT_ALIASES",
    "REPORT_DEFINITIONS",
    "REPORT_SCHEMA_VERSION",
    "_build_query",
    "_decode_cursor",
    "_encode_cursor",
    "_run_killable_process",
    "build_parser",
    "canonical_report_name",
    "main",
    "render_catalog_text",
    "render_text",
    "report_catalog",
    "report_definition",
]


if __name__ == "__main__":
    raise SystemExit(main())
