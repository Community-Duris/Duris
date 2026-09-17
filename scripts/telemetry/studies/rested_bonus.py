#!/usr/bin/env python3
"""Evaluate the bounded, fail-closed rested-bonus study export.

This module consumes a pre-redacted JSON export from approved published
telemetry interfaces.  It never opens a database, reads live game state, or
changes a balance.  The input is deliberately a study-shaped export rather
than a second report/rollup implementation: deployment code owns the mapping
from published reports to this schema.

The evaluator is descriptive only.  A complete, sufficiently sized synthetic
fixture is reported as ``fixture_qualified`` so the protocol can be tested;
only a non-synthetic export can receive ``qualified``.  Incomplete coverage,
small cells, contaminated rows, or missing repeat-group controls produce an
abstaining report instead of an inferred result.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import sys
from typing import Any, Iterable, Mapping, Sequence


SCHEMA_VERSION = 1
PROTOCOL_VERSION = 1
MAX_INPUT_BYTES = 8 * 1024 * 1024
MAX_ROWS = 50_000
MAX_TOKEN_LENGTH = 128
MIN_SUBJECTS_PER_CELL = 8
MIN_ACTIVE_USEC_PER_CELL = 900_000_000  # 15 active minutes
MIN_RETURN_OBSERVATIONS_PER_WINDOW = 4
CONFIDENCE_LEVEL = 0.95
Z_95 = 1.959963984540054
MICROSECONDS_PER_HOUR = 3_600_000_000

PARTICIPATION_COHORTS = ("solo", "group", "pvp", "group_pvp")
RESTED_TIERS = ("unrested", "rested", "well_rested")
APPROVED_SOURCE_INTERFACES = frozenset(
    {
        "telemetry.return.v1",
        "telemetry.playtime.v1",
        "telemetry.progression.v1",
    }
)
RETURN_WINDOWS = (
    ("under_1h", 0, 3_600_000_000),
    ("1_4h", 3_600_000_000, 14_400_000_000),
    ("4_24h", 14_400_000_000, 86_400_000_000),
    ("1_3d", 86_400_000_000, 259_200_000_000),
    ("3d_plus", 259_200_000_000, None),
)
RETURN_WINDOW_NAMES = frozenset(window[0] for window in RETURN_WINDOWS)
TOKEN_RE = re.compile(r"^[A-Za-z0-9_.:-]{1,128}$")

# Raw identity and operational fields are not valid in a study export.  The
# recursive check protects the evaluator if an upstream adapter accidentally
# passes through one of these names.
FORBIDDEN_FIELD_NAMES = frozenset(
    {
        "account_id",
        "account_name",
        "character_id",
        "character_name",
        "player_name",
        "ip",
        "ip_address",
        "chat",
        "message",
        "room_vnum",
        "live_location",
        "password",
        "credential",
        "raw_fact_id",
    }
)


class StudyInputError(ValueError):
    """The study export is malformed and cannot be evaluated."""


def _mapping(value: Any, name: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise StudyInputError(f"{name} must be an object")
    return value


def _required(mapping: Mapping[str, Any], name: str) -> Any:
    if name not in mapping:
        raise StudyInputError(f"missing required field: {name}")
    return mapping[name]


def _integer(value: Any, name: str, *, minimum: int = 0, maximum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise StudyInputError(f"{name} must be an integer")
    if value < minimum:
        raise StudyInputError(f"{name} must be >= {minimum}")
    if maximum is not None and value > maximum:
        raise StudyInputError(f"{name} must be <= {maximum}")
    return value


def _boolean(value: Any, name: str) -> bool:
    if not isinstance(value, bool):
        raise StudyInputError(f"{name} must be a boolean")
    return value


def _string(value: Any, name: str, *, max_length: int = 256) -> str:
    if not isinstance(value, str) or not value or len(value) > max_length:
        raise StudyInputError(f"{name} must be a non-empty string of <= {max_length} bytes")
    return value


def _token(value: Any, name: str) -> str:
    token = _string(value, name, max_length=MAX_TOKEN_LENGTH)
    if not TOKEN_RE.fullmatch(token):
        raise StudyInputError(f"{name} must be a redacted stable token")
    return token


def _parse_utc(value: Any, name: str) -> datetime:
    raw = _string(value, name, max_length=64)
    try:
        parsed = datetime.fromisoformat(raw.replace("Z", "+00:00"))
    except ValueError as exc:
        raise StudyInputError(f"{name} must be an ISO-8601 timestamp") from exc
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        raise StudyInputError(f"{name} must include a UTC offset")
    return parsed.astimezone(timezone.utc)


def _reject_forbidden_fields(value: Any, path: str = "input") -> None:
    if isinstance(value, Mapping):
        for key, child in value.items():
            normalized = str(key).strip().lower().replace("-", "_")
            if normalized in FORBIDDEN_FIELD_NAMES:
                raise StudyInputError(f"forbidden raw field at {path}.{key}")
            _reject_forbidden_fields(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_forbidden_fields(child, f"{path}[{index}]")


def _dedupe(values: Iterable[str]) -> list[str]:
    return list(dict.fromkeys(values))


def _window_definition() -> list[dict[str, Any]]:
    return [
        {
            "name": name,
            "lower_usec": lower,
            "upper_usec": upper,
            "upper_bound_inclusive": upper is None,
        }
        for name, lower, upper in RETURN_WINDOWS
    ]


def _wilson(successes: int, observations: int) -> dict[str, Any]:
    if observations <= 0:
        return {
            "value": None,
            "lower": None,
            "upper": None,
            "numerator": successes,
            "denominator": observations,
            "unit": "fraction",
            "zero_denominator_policy": "null",
            "confidence_level": CONFIDENCE_LEVEL,
            "method": "wilson_score_interval",
        }
    proportion = successes / observations
    z_squared = Z_95 * Z_95
    denominator = 1.0 + z_squared / observations
    center = (proportion + z_squared / (2.0 * observations)) / denominator
    margin = (
        Z_95
        * math.sqrt(
            proportion * (1.0 - proportion) / observations
            + z_squared / (4.0 * observations * observations)
        )
        / denominator
    )
    return {
        "value": proportion,
        "lower": max(0.0, center - margin),
        "upper": min(1.0, center + margin),
        "numerator": successes,
        "denominator": observations,
        "unit": "fraction",
        "zero_denominator_policy": "null",
        "confidence_level": CONFIDENCE_LEVEL,
        "method": "wilson_score_interval",
    }


def _xp_rate(rows: Sequence["Observation"]) -> tuple[dict[str, Any], dict[str, Any]]:
    total_xp = sum(row.xp_earned for row in rows)
    total_active = sum(row.active_usec for row in rows)
    if total_active <= 0:
        rate = {
            "value": None,
            "numerator": total_xp,
            "denominator_usec": total_active,
            "denominator_active_hours": 0.0,
            "unit": "xp_per_active_hour",
            "zero_denominator_policy": "null",
        }
        uncertainty = {
            "value": None,
            "lower": None,
            "upper": None,
            "n": 0,
            "method": "normal_95_percent_interval_across_subject_rates",
        }
        return rate, uncertainty

    active_hours = total_active / MICROSECONDS_PER_HOUR
    rate_value = total_xp / active_hours
    per_subject = [
        row.xp_earned / (row.active_usec / MICROSECONDS_PER_HOUR)
        for row in rows
        if row.active_usec > 0
    ]
    mean = sum(per_subject) / len(per_subject)
    if len(per_subject) > 1:
        variance = sum((value - mean) ** 2 for value in per_subject) / (len(per_subject) - 1)
        standard_error = math.sqrt(variance / len(per_subject))
    else:
        standard_error = 0.0
    margin = Z_95 * standard_error
    rate = {
        "value": rate_value,
        "numerator": total_xp,
        "denominator_usec": total_active,
        "denominator_active_hours": active_hours,
        "unit": "xp_per_active_hour",
        "zero_denominator_policy": "null",
    }
    uncertainty = {
        "value": mean,
        "lower": max(0.0, mean - margin),
        "upper": mean + margin,
        "n": len(per_subject),
        "method": "normal_95_percent_interval_across_subject_rates",
        "confidence_level": CONFIDENCE_LEVEL,
    }
    return rate, uncertainty


class Observation:
    __slots__ = (
        "subject_token",
        "repeat_group_token",
        "cohort",
        "rested_tier",
        "config_generation",
        "return_window",
        "returned",
        "censored",
        "active_usec",
        "connected_usec",
        "xp_earned",
    )

    def __init__(
        self,
        *,
        subject_token: str,
        repeat_group_token: str,
        cohort: str,
        rested_tier: str,
        config_generation: str,
        return_window: str,
        returned: bool,
        censored: bool,
        active_usec: int,
        connected_usec: int,
        xp_earned: int,
    ) -> None:
        self.subject_token = subject_token
        self.repeat_group_token = repeat_group_token
        self.cohort = cohort
        self.rested_tier = rested_tier
        self.config_generation = config_generation
        self.return_window = return_window
        self.returned = returned
        self.censored = censored
        self.active_usec = active_usec
        self.connected_usec = connected_usec
        self.xp_earned = xp_earned

    @property
    def cell_key(self) -> tuple[str, str, str]:
        return self.cohort, self.rested_tier, self.config_generation

    @property
    def key(self) -> tuple[str, str, str, str]:
        return self.subject_token, *self.cell_key


def _validate_source(source: Mapping[str, Any]) -> dict[str, Any]:
    kind = _string(_required(source, "kind"), "source.kind", max_length=64)
    if kind != "published_report_export":
        raise StudyInputError("source.kind must be published_report_export")
    interfaces = _required(source, "interfaces")
    if not isinstance(interfaces, list) or not interfaces or len(interfaces) > 16:
        raise StudyInputError("source.interfaces must be a bounded non-empty list")
    normalized_interfaces = [
        _string(interface, "source.interfaces[]", max_length=128) for interface in interfaces
    ]
    if any(interface not in APPROVED_SOURCE_INTERFACES for interface in normalized_interfaces):
        raise StudyInputError("source.interfaces contains an unapproved interface")
    if (
        len(normalized_interfaces) != len(APPROVED_SOURCE_INTERFACES)
        or set(normalized_interfaces) != APPROVED_SOURCE_INTERFACES
    ):
        raise StudyInputError("source.interfaces must include exactly the reviewed study interfaces")
    return {
        "kind": kind,
        "interfaces": sorted(APPROVED_SOURCE_INTERFACES),
        "redacted": _boolean(_required(source, "redacted"), "source.redacted"),
        "synthetic_fixture": _boolean(
            _required(source, "synthetic_fixture"), "source.synthetic_fixture"
        ),
    }


def _validate_scope(scope: Mapping[str, Any]) -> tuple[dict[str, Any], list[str]]:
    environment_id = _integer(_required(scope, "environment_id"), "scope.environment_id", minimum=0)
    season_id = _integer(_required(scope, "season_id"), "scope.season_id", minimum=0)
    generations = _required(scope, "config_generations")
    if not isinstance(generations, list) or not generations or len(generations) > 32:
        raise StudyInputError("scope.config_generations must be a bounded non-empty list")
    config_generations = sorted(
        {
            _token(generation, "scope.config_generations[]") for generation in generations
        }
    )
    coverage = _mapping(_required(scope, "coverage"), "scope.coverage")
    coverage_start = _parse_utc(_required(coverage, "start_utc"), "scope.coverage.start_utc")
    coverage_end = _parse_utc(_required(coverage, "end_utc"), "scope.coverage.end_utc")
    if coverage_end <= coverage_start:
        raise StudyInputError("scope.coverage.end_utc must be after start_utc")
    quality_flags = _required(coverage, "quality_flags")
    if not isinstance(quality_flags, list) or any(not isinstance(flag, str) for flag in quality_flags):
        raise StudyInputError("scope.coverage.quality_flags must be a list of strings")
    parsed_coverage = {
        "published": _boolean(_required(coverage, "published"), "scope.coverage.published"),
        "complete": _boolean(_required(coverage, "complete"), "scope.coverage.complete"),
        "published_generation": _integer(
            _required(coverage, "published_generation"),
            "scope.coverage.published_generation",
            minimum=0,
        ),
        "start_utc": coverage_start.isoformat().replace("+00:00", "Z"),
        "end_utc": coverage_end.isoformat().replace("+00:00", "Z"),
        "coverage_days": (coverage_end - coverage_start).total_seconds() / 86_400.0,
        "quality_flags": sorted(set(quality_flags)),
    }
    return {
        "environment_id": environment_id,
        "season_id": season_id,
        "config_generations": config_generations,
        "coverage": parsed_coverage,
    }, config_generations


def _row_issues(
    raw: Mapping[str, Any],
    *,
    config_generations: Sequence[str],
) -> tuple[list[str], Observation | None]:
    issues: list[str] = []

    def read_token(name: str) -> str | None:
        try:
            return _token(_required(raw, name), f"row.{name}")
        except StudyInputError as exc:
            issues.append(str(exc))
            return None

    subject_token = read_token("subject_token")
    repeat_group_token = read_token("repeat_group_token")
    cohort = raw.get("cohort")
    if cohort not in PARTICIPATION_COHORTS:
        issues.append("invalid_cohort")
    rested_tier = raw.get("rested_tier")
    if rested_tier not in RESTED_TIERS:
        issues.append("invalid_rested_tier")
    config_generation = raw.get("config_generation")
    if config_generation not in config_generations:
        issues.append("config_generation_outside_scope")
    return_window = raw.get("return_window")
    if return_window not in RETURN_WINDOW_NAMES:
        issues.append("invalid_return_window")

    mode_qualified = raw.get("mode_qualified")
    if not isinstance(mode_qualified, bool):
        issues.append("mode_qualification_missing")
    elif not mode_qualified:
        issues.append("mode_not_qualified")

    returned = raw.get("returned")
    censored = raw.get("censored")
    if not isinstance(returned, bool):
        issues.append("returned_must_be_boolean")
    if not isinstance(censored, bool):
        issues.append("censored_must_be_boolean")
    elif censored and returned is True:
        issues.append("censored_return_cannot_be_true")

    quality_flags = raw.get("quality_flags")
    if not isinstance(quality_flags, list) or any(not isinstance(flag, str) for flag in quality_flags):
        issues.append("quality_flags_must_be_string_list")
    elif quality_flags:
        issues.append("row_quality_flags_present")

    values: dict[str, int] = {}
    for name in ("active_usec", "connected_usec", "xp_earned"):
        value = raw.get(name)
        if isinstance(value, bool) or not isinstance(value, int) or value < 0 or value > 2**63 - 1:
            issues.append(f"invalid_{name}")
        else:
            values[name] = value
    if "active_usec" in values and "connected_usec" in values and values["active_usec"] > values["connected_usec"]:
        issues.append("active_exceeds_connected")

    if issues or None in (subject_token, repeat_group_token):
        return _dedupe(issues), None
    return [], Observation(
        subject_token=subject_token,
        repeat_group_token=repeat_group_token,
        cohort=cohort,
        rested_tier=rested_tier,
        config_generation=config_generation,
        return_window=return_window,
        returned=returned,
        censored=censored,
        active_usec=values["active_usec"],
        connected_usec=values["connected_usec"],
        xp_earned=values["xp_earned"],
    )


def _parse_rows(
    raw_rows: Any,
    *,
    config_generations: Sequence[str],
) -> tuple[list[Observation], list[dict[str, Any]]]:
    if not isinstance(raw_rows, list):
        raise StudyInputError("rows must be a list")
    if len(raw_rows) > MAX_ROWS:
        raise StudyInputError(f"rows exceeds hard bound of {MAX_ROWS}")
    observations: list[Observation] = []
    invalid: list[dict[str, Any]] = []
    seen: set[tuple[str, str, str, str]] = set()
    for index, raw in enumerate(raw_rows):
        if not isinstance(raw, Mapping):
            invalid.append({"row_index": index, "reasons": ["row_must_be_object"]})
            continue
        issues, observation = _row_issues(raw, config_generations=config_generations)
        if observation is not None and observation.key in seen:
            issues = ["duplicate_subject_cell"]
            observation = None
        if observation is None:
            invalid.append({"row_index": index, "reasons": _dedupe(issues)})
            continue
        seen.add(observation.key)
        observations.append(observation)
    observations.sort(key=lambda row: row.key)
    return observations, invalid


def _return_window_rows(rows: Sequence[Observation]) -> list[dict[str, Any]]:
    by_window: dict[str, list[Observation]] = defaultdict(list)
    for row in rows:
        by_window[row.return_window].append(row)
    results: list[dict[str, Any]] = []
    for window, _, _ in RETURN_WINDOWS:
        window_rows = sorted(by_window.get(window, []), key=lambda row: row.subject_token)
        uncensored = [row for row in window_rows if not row.censored]
        successes = sum(1 for row in uncensored if row.returned)
        sufficient = len(uncensored) >= MIN_RETURN_OBSERVATIONS_PER_WINDOW
        results.append(
            {
                "name": window,
                "status": "qualified" if sufficient else "abstain",
                "abstention_reasons": []
                if sufficient
                else ["empty_window" if not uncensored else "small_window_sample"],
                "observation_count": len(uncensored),
                "censored_count": len(window_rows) - len(uncensored),
                "return_success_count": successes,
                "return_fraction": _wilson(successes, len(uncensored)),
            }
        )
    return results


def _repeat_group_sensitivity(rows: Sequence[Observation]) -> dict[str, Any]:
    groups: dict[str, list[Observation]] = defaultdict(list)
    for row in rows:
        groups[row.repeat_group_token].append(row)
    capped = [
        sorted(group_rows, key=lambda row: row.subject_token)[0]
        for group_rows in groups.values()
    ]
    uncensored = [row for row in capped if not row.censored]
    successes = sum(1 for row in uncensored if row.returned)
    return {
        "available": True,
        "repeat_group_count": len(groups),
        "capped_subject_count": len(capped),
        "max_subjects_per_repeat_group": max((len(group) for group in groups.values()), default=0),
        "capped_return_fraction": _wilson(successes, len(uncensored)),
        "method": "one_deterministically_sorted_subject_per_redacted_repeat_group",
    }


def _cell_result(cell_key: tuple[str, str, str], rows: Sequence[Observation]) -> dict[str, Any]:
    cohort, rested_tier, config_generation = cell_key
    ordered_rows = sorted(rows, key=lambda row: row.subject_token)
    reasons: list[str] = []
    if len(ordered_rows) < MIN_SUBJECTS_PER_CELL:
        reasons.append("small_cell_sample")
    total_active = sum(row.active_usec for row in ordered_rows)
    if total_active < MIN_ACTIVE_USEC_PER_CELL:
        reasons.append("small_active_exposure")
    xp_rate, xp_uncertainty = _xp_rate(ordered_rows)
    return {
        "cohort": cohort,
        "rested_tier": rested_tier,
        "config_generation": config_generation,
        "status": "qualified" if not reasons else "abstain",
        "abstention_reasons": reasons,
        "sample_sufficiency": {
            "subject_count": len(ordered_rows),
            "minimum_subjects": MIN_SUBJECTS_PER_CELL,
            "active_usec": total_active,
            "minimum_active_usec": MIN_ACTIVE_USEC_PER_CELL,
            "qualified": not reasons,
        },
        "connected_usec": sum(row.connected_usec for row in ordered_rows),
        "active_usec": total_active,
        "xp_earned": sum(row.xp_earned for row in ordered_rows),
        "xp_rate_per_active_hour": xp_rate,
        "xp_rate_uncertainty": xp_uncertainty,
        "return_windows": _return_window_rows(ordered_rows),
        "repeat_group_sensitivity": _repeat_group_sensitivity(ordered_rows),
    }


def _coverage_reasons(coverage: Mapping[str, Any]) -> list[str]:
    reasons: list[str] = []
    if not coverage["published"]:
        reasons.append("source_generation_not_published")
    if not coverage["complete"]:
        reasons.append("coverage_incomplete")
    if coverage["quality_flags"]:
        reasons.append("coverage_quality_flags_present")
    return reasons


def build_report(payload: Mapping[str, Any]) -> dict[str, Any]:
    """Return a deterministic report for one bounded study export."""

    _reject_forbidden_fields(payload)
    if _integer(_required(payload, "schema_version"), "schema_version", minimum=0) != SCHEMA_VERSION:
        raise StudyInputError(f"schema_version must be {SCHEMA_VERSION}")
    if _integer(_required(payload, "protocol_version"), "protocol_version", minimum=0) != PROTOCOL_VERSION:
        raise StudyInputError(f"protocol_version must be {PROTOCOL_VERSION}")
    if _string(_required(payload, "study_id"), "study_id", max_length=64) != "rested_bonus":
        raise StudyInputError("study_id must be rested_bonus")

    source = _validate_source(_mapping(_required(payload, "source"), "source"))
    if not source["redacted"]:
        raise StudyInputError("source.redacted must be true")
    scope, config_generations = _validate_scope(_mapping(_required(payload, "scope"), "scope"))
    observations, invalid_rows = _parse_rows(
        _required(payload, "rows"), config_generations=config_generations
    )

    by_cell: dict[tuple[str, str, str], list[Observation]] = defaultdict(list)
    for observation in observations:
        by_cell[observation.cell_key].append(observation)
    cells = [
        _cell_result(cell_key, by_cell[cell_key])
        for cell_key in sorted(by_cell)
    ]
    coverage_reasons = _coverage_reasons(scope["coverage"])
    abstention_reasons = list(coverage_reasons)
    if invalid_rows:
        abstention_reasons.append("invalid_or_unqualified_rows")
    if not observations:
        abstention_reasons.append("empty_sample")
    for cell in cells:
        abstention_reasons.extend(cell["abstention_reasons"])
    abstention_reasons = _dedupe(abstention_reasons)

    if abstention_reasons:
        status = "abstain"
    elif source["synthetic_fixture"]:
        status = "fixture_qualified"
    else:
        status = "qualified"

    observed_generations = sorted({row.config_generation for row in observations})
    observed_cohorts = [
        {
            "cohort": cell["cohort"],
            "rested_tier": cell["rested_tier"],
            "config_generation": cell["config_generation"],
            "subject_count": cell["sample_sufficiency"]["subject_count"],
            "status": cell["status"],
        }
        for cell in cells
    ]
    cells_qualified = sum(cell["status"] == "qualified" for cell in cells)
    sample_sufficiency = {
        "qualified": bool(cells) and not abstention_reasons,
        "cells_observed": len(cells),
        "cells_qualified": cells_qualified,
        "cells_abstained": len(cells) - cells_qualified,
        "minimum_subjects_per_cell": MIN_SUBJECTS_PER_CELL,
        "minimum_active_usec_per_cell": MIN_ACTIVE_USEC_PER_CELL,
        "minimum_return_observations_per_window": MIN_RETURN_OBSERVATIONS_PER_WINDOW,
        "selection_rule": (
            "Each observed cohort/config/tier cell must meet the subject and active-exposure "
            "thresholds; each return window reports its own abstention when small."
        ),
    }
    coverage = {
        **scope["coverage"],
        "config_generations_in_scope": list(config_generations),
        "config_generations_observed": observed_generations,
        "observed_row_count": len(observations),
        "excluded_row_count": len(invalid_rows),
    }

    return {
        "schema_version": SCHEMA_VERSION,
        "protocol_version": PROTOCOL_VERSION,
        "study_id": "rested_bonus",
        "status": status,
        "qualified_for_real_observation": status == "qualified",
        "abstention_reasons": abstention_reasons,
        "provenance": {
            "kind": source["kind"],
            "interfaces": list(source["interfaces"]),
            "redacted": source["redacted"],
            "synthetic_fixture": source["synthetic_fixture"],
            "synthetic_results_are_not_real_observation": True,
        },
        "scope": {
            "environment_id": scope["environment_id"],
            "season_id": scope["season_id"],
            "config_generations": list(config_generations),
        },
        "coverage": coverage,
        "cohort_definition": {
            "participation_cohorts": list(PARTICIPATION_COHORTS),
            "rested_tiers": list(RESTED_TIERS),
            "return_windows": _window_definition(),
            "group_and_pvp_rule": "include only rows with mode_qualified=true",
        },
        "cohorts": observed_cohorts,
        "sample_sufficiency": sample_sufficiency,
        "uncertainty": {
            "confidence_level": CONFIDENCE_LEVEL,
            "return_fraction": "Wilson score interval; censored observations are excluded from its denominator.",
            "xp_rate": "Normal 95% interval across subject rates; aggregate numerator/active-time rate remains visible.",
            "association_not_causal": True,
            "causal_effect_proven": False,
        },
        "results": {
            "cells": cells,
            "excluded_rows": invalid_rows,
        },
        "future_change_protocol": {
            "target": "#254 rested-bonus rule review",
            "comparison": (
                "Pre-register either a complete before/after comparison or an experiment/holdout "
                "protocol using the same cohorts, return windows, config generations, coverage "
                "and censoring rules. Do not compare unlike seasons or silently mix generations."
            ),
            "rollback_evaluation_criteria": [
                "Rollback if a pre-registered primary guardrail is breached for two consecutive complete windows.",
                "Rollback if qualified coverage or sample sufficiency is lost; an incomplete report is not a pass.",
                "Rollback if an owner-approved adverse-outcome or XP-rate bound is crossed in a qualified stratum.",
                "Require reviewer sign-off and an expiry before any proposal can be considered for application.",
            ],
            "automatic_balance_mutation": False,
            "production_activation": False,
        },
    }


def _read_input(path: Path) -> Mapping[str, Any]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise StudyInputError(f"cannot read input: {path}") from exc
    if len(data) > MAX_INPUT_BYTES:
        raise StudyInputError(f"input exceeds hard bound of {MAX_INPUT_BYTES} bytes")
    try:
        parsed = json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise StudyInputError("input must be UTF-8 JSON") from exc
    return _mapping(parsed, "input")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="pre-redacted study export JSON")
    parser.add_argument("--output", type=Path, help="write the deterministic report JSON here")
    args = parser.parse_args(argv)
    try:
        report = build_report(_read_input(args.input))
    except (OSError, StudyInputError) as exc:
        print(f"rested-bonus study input error: {exc}", file=sys.stderr)
        return 2
    serialized = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        sys.stdout.write(serialized)
    else:
        try:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(serialized, encoding="utf-8", newline="\n")
        except OSError as exc:
            print(f"rested-bonus study output error: {exc}", file=sys.stderr)
            return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
