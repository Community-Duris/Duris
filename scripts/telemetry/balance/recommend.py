#!/usr/bin/env python3
"""Build one deterministic, shadow-only balance recommendation.

The evaluator consumes a bounded, pre-redacted export from one reviewed
published report generation.  It has no database or gameplay connection and
cannot apply the recommendation.  The policy intentionally owns one narrow
target only; future targets require a separately reviewed policy version.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from datetime import datetime, timedelta, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import sys
from typing import Any, Iterable, Mapping, Sequence


SCHEMA_VERSION = 1
POLICY_VERSION = "balance-shadow-v1"
APPROVED_REPORT_NAME = "balance_shadow_outcome_v1"
APPROVED_REPORT_DEFINITION_VERSION = 1
TARGET_PARAMETER = "payout.epic.zone.alignmentMod"
TARGET_UNIT = "fraction"
TARGET_MIN_MILLI = 0
TARGET_MAX_MILLI = 1_000
TARGET_STEP_MILLI = 50
TARGET_SUCCESS_MILLI = 600
TARGET_DEADBAND_MILLI = 50
MIN_SAMPLES = 20
MIN_DISTINCT_SUBJECTS = 8
MIN_DISTINCT_REPEAT_GROUPS = 8
MIN_STRATUM_SAMPLES = 5
MAX_REPEAT_GROUP_OBSERVATIONS = 4
MAX_REPEAT_GROUP_SHARE_MILLI = 250
MAX_REPORT_AGE = timedelta(days=14)
COOLDOWN = timedelta(days=14)
HYSTERESIS_MILLI = 50
MAX_INPUT_BYTES = 16 * 1024 * 1024
MAX_ROWS = 100_000
MAX_TOKEN_LENGTH = 128
MAX_REWARD_AMOUNT = 2**63 - 1
CONFIDENCE_LEVEL = 0.95
Z_95 = 1.959963984540054

COHORTS = ("solo", "group", "pvp", "group_pvp")
ADJUSTMENT_CONTEXTS = ("none", "trophy", "alignment", "trophy_alignment")
TOKEN_RE = re.compile(r"^[A-Za-z0-9_.:-]{1,128}$")
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


class RecommendationInputError(ValueError):
    """The shadow input cannot be parsed as the versioned contract."""


def _mapping(value: Any, name: str) -> Mapping[str, Any]:
    if not isinstance(value, Mapping):
        raise RecommendationInputError(f"{name} must be an object")
    return value


def _required(mapping: Mapping[str, Any], name: str) -> Any:
    if name not in mapping:
        raise RecommendationInputError(f"missing required field: {name}")
    return mapping[name]


def _integer(value: Any, name: str, *, minimum: int = 0, maximum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise RecommendationInputError(f"{name} must be an integer")
    if value < minimum:
        raise RecommendationInputError(f"{name} must be >= {minimum}")
    if maximum is not None and value > maximum:
        raise RecommendationInputError(f"{name} must be <= {maximum}")
    return value


def _boolean(value: Any, name: str) -> bool:
    if not isinstance(value, bool):
        raise RecommendationInputError(f"{name} must be a boolean")
    return value


def _string(value: Any, name: str, *, max_length: int = 256) -> str:
    if not isinstance(value, str) or not value or len(value) > max_length:
        raise RecommendationInputError(f"{name} must be a non-empty string of <= {max_length} characters")
    return value


def _token(value: Any, name: str) -> str:
    token = _string(value, name, max_length=MAX_TOKEN_LENGTH)
    if not TOKEN_RE.fullmatch(token):
        raise RecommendationInputError(f"{name} must be a redacted stable token")
    return token


def _parse_utc(value: Any, name: str) -> datetime:
    raw = _string(value, name, max_length=64)
    try:
        parsed = datetime.fromisoformat(raw.replace("Z", "+00:00"))
    except ValueError as exc:
        raise RecommendationInputError(f"{name} must be an ISO-8601 timestamp") from exc
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        raise RecommendationInputError(f"{name} must include a UTC offset")
    return parsed.astimezone(timezone.utc)


def _reject_forbidden_fields(value: Any, path: str = "input") -> None:
    if isinstance(value, Mapping):
        for key, child in value.items():
            normalized = str(key).strip().lower().replace("-", "_")
            if normalized in FORBIDDEN_FIELD_NAMES:
                raise RecommendationInputError(f"forbidden raw field at {path}.{key}")
            _reject_forbidden_fields(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            _reject_forbidden_fields(child, f"{path}[{index}]")


def _dedupe(values: Iterable[str]) -> list[str]:
    return list(dict.fromkeys(values))


def _format_milli(value: int | None) -> str | None:
    if value is None:
        return None
    return f"{value / 1000.0:.3f}"


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


class ShadowObservation:
    __slots__ = (
        "event_token",
        "subject_token",
        "repeat_group_token",
        "cohort",
        "adjustment_context",
        "reward_success",
        "reward_amount",
    )

    def __init__(
        self,
        *,
        event_token: str,
        subject_token: str,
        repeat_group_token: str,
        cohort: str,
        adjustment_context: str,
        reward_success: bool,
        reward_amount: int,
    ) -> None:
        self.event_token = event_token
        self.subject_token = subject_token
        self.repeat_group_token = repeat_group_token
        self.cohort = cohort
        self.adjustment_context = adjustment_context
        self.reward_success = reward_success
        self.reward_amount = reward_amount

    @property
    def stratum_key(self) -> tuple[str, str]:
        return self.cohort, self.adjustment_context


def _read_row_token(raw: Mapping[str, Any], name: str, issues: list[str]) -> str | None:
    value = raw.get(name)
    if not isinstance(value, str) or not TOKEN_RE.fullmatch(value):
        issues.append(f"invalid_{name}")
        return None
    return value


def _parse_report(
    raw_report: Any,
    *,
    as_of: datetime,
) -> tuple[dict[str, Any], list[str]]:
    report = _mapping(raw_report, "report")
    reasons: list[str] = []
    name = report.get("name")
    if name != APPROVED_REPORT_NAME:
        reasons.append("unapproved_report_name")
    definition_version = _integer(
        _required(report, "definition_version"),
        "report.definition_version",
        minimum=0,
    )
    if definition_version != APPROVED_REPORT_DEFINITION_VERSION:
        reasons.append("unsupported_report_definition")
    generation = _integer(_required(report, "generation"), "report.generation", minimum=1)
    config_generation = _token(
        _required(report, "config_generation"), "report.config_generation"
    )
    environment_id = _integer(_required(report, "environment_id"), "report.environment_id", minimum=0)
    season_id = _integer(_required(report, "season_id"), "report.season_id", minimum=0)
    coverage = _mapping(_required(report, "coverage"), "report.coverage")
    coverage_start = _parse_utc(_required(coverage, "start_utc"), "report.coverage.start_utc")
    coverage_end = _parse_utc(_required(coverage, "end_utc"), "report.coverage.end_utc")
    expires_at = _parse_utc(_required(report, "expires_at_utc"), "report.expires_at_utc")
    if coverage_end <= coverage_start:
        reasons.append("invalid_coverage_window")
    if coverage_end > as_of:
        reasons.append("coverage_end_in_future")
    if expires_at <= as_of:
        reasons.append("report_expired")
    if expires_at <= coverage_end:
        reasons.append("invalid_report_expiry")
    if as_of - coverage_end > MAX_REPORT_AGE:
        reasons.append("report_stale")
    coverage_quality_flags = _required(coverage, "quality_flags")
    if not isinstance(coverage_quality_flags, list) or any(
        not isinstance(flag, str) for flag in coverage_quality_flags
    ):
        raise RecommendationInputError("report.coverage.quality_flags must be a list of strings")
    if coverage_quality_flags:
        reasons.append("coverage_quality_flags_present")
    if not _boolean(_required(coverage, "published"), "report.coverage.published"):
        reasons.append("source_generation_not_published")
    if not _boolean(_required(coverage, "complete"), "report.coverage.complete"):
        reasons.append("coverage_incomplete")
    return (
        {
            "name": name,
            "definition_version": definition_version,
            "generation": generation,
            "config_generation": config_generation,
            "environment_id": environment_id,
            "season_id": season_id,
            "coverage": {
                "start_utc": coverage_start.isoformat().replace("+00:00", "Z"),
                "end_utc": coverage_end.isoformat().replace("+00:00", "Z"),
                "published": bool(coverage.get("published")),
                "complete": bool(coverage.get("complete")),
                "quality_flags": sorted(set(coverage_quality_flags)),
                "coverage_days": max(0.0, (coverage_end - coverage_start).total_seconds() / 86_400.0),
            },
            "expires_at_utc": expires_at.isoformat().replace("+00:00", "Z"),
        },
        _dedupe(reasons),
    )


def _parse_target(
    raw_target: Any,
    *,
    expected_config_generation: str,
) -> tuple[dict[str, Any], list[str]]:
    target = _mapping(raw_target, "target")
    reasons: list[str] = []
    parameter = target.get("parameter")
    if parameter != TARGET_PARAMETER:
        reasons.append("target_not_policy_owned")
    if target.get("unit") != TARGET_UNIT:
        reasons.append("target_unit_mismatch")
    if target.get("min_value_milli") != TARGET_MIN_MILLI:
        reasons.append("target_minimum_mismatch")
    if target.get("max_value_milli") != TARGET_MAX_MILLI:
        reasons.append("target_maximum_mismatch")
    if target.get("step_milli") != TARGET_STEP_MILLI:
        reasons.append("target_step_mismatch")
    if target.get("config_generation") != expected_config_generation:
        reasons.append("target_config_generation_mismatch")
    current_value = target.get("current_value_milli")
    if (
        isinstance(current_value, bool)
        or not isinstance(current_value, int)
        or not TARGET_MIN_MILLI <= current_value <= TARGET_MAX_MILLI
    ):
        reasons.append("target_value_out_of_bounds")
        current_value = None
    return (
        {
            "parameter": TARGET_PARAMETER,
            "unit": TARGET_UNIT,
            "config_generation": expected_config_generation,
            "current_value_milli": current_value,
            "current_value": _format_milli(current_value),
            "bounds_milli": {"min": TARGET_MIN_MILLI, "max": TARGET_MAX_MILLI},
            "step_milli": TARGET_STEP_MILLI,
        },
        reasons,
    )


def _parse_history(raw_history: Any, *, as_of: datetime) -> tuple[dict[str, Any], list[str]]:
    history = _mapping(raw_history, "history")
    reasons: list[str] = []
    applied_mutations = _integer(
        _required(history, "applied_mutations"),
        "history.applied_mutations",
        minimum=0,
        maximum=MAX_ROWS,
    )
    if applied_mutations:
        reasons.append("feedback_loop_detected")
    raw_last = history.get("last_recommendation")
    if raw_last is None:
        return {"applied_mutations": applied_mutations, "last_recommendation": None}, reasons
    last = _mapping(raw_last, "history.last_recommendation")
    status = last.get("status")
    if status != "shadow_only":
        reasons.append("feedback_loop_detected")
    target_parameter = last.get("target_parameter")
    if target_parameter != TARGET_PARAMETER:
        reasons.append("history_target_mismatch")
    proposed = last.get("proposed_value_milli")
    if (
        isinstance(proposed, bool)
        or not isinstance(proposed, int)
        or not TARGET_MIN_MILLI <= proposed <= TARGET_MAX_MILLI
    ):
        reasons.append("history_value_out_of_bounds")
        proposed = None
    issued_at = _parse_utc(_required(last, "issued_at_utc"), "history.last_recommendation.issued_at_utc")
    if issued_at > as_of:
        reasons.append("history_timestamp_in_future")
    normalized = {
        "status": status,
        "target_parameter": TARGET_PARAMETER,
        "proposed_value_milli": proposed,
        "proposed_value": _format_milli(proposed),
        "issued_at_utc": issued_at.isoformat().replace("+00:00", "Z"),
    }
    return {"applied_mutations": applied_mutations, "last_recommendation": normalized}, reasons


def _parse_rows(
    raw_rows: Any,
    *,
    report_generation: int,
    config_generation: str,
) -> tuple[list[ShadowObservation], list[dict[str, Any]]]:
    if not isinstance(raw_rows, list):
        raise RecommendationInputError("rows must be a list")
    if len(raw_rows) > MAX_ROWS:
        raise RecommendationInputError(f"rows exceeds hard bound of {MAX_ROWS}")
    valid: list[ShadowObservation] = []
    invalid: list[dict[str, Any]] = []
    seen_events: set[str] = set()
    for index, raw in enumerate(raw_rows):
        if not isinstance(raw, Mapping):
            invalid.append({"row_index": index, "reasons": ["row_must_be_object"]})
            continue
        issues: list[str] = []
        event_token = _read_row_token(raw, "event_token", issues)
        subject_token = _read_row_token(raw, "subject_token", issues)
        repeat_group_token = _read_row_token(raw, "repeat_group_token", issues)
        if raw.get("source_generation") != report_generation:
            issues.append("source_generation_mismatch")
        if raw.get("config_generation") != config_generation:
            issues.append("config_generation_mismatch")
        if raw.get("cohort") not in COHORTS:
            issues.append("invalid_cohort")
        if raw.get("adjustment_context") not in ADJUSTMENT_CONTEXTS:
            issues.append("invalid_adjustment_context")
        if raw.get("eligible") is not True:
            issues.append("ineligible_observation")
        reward_success = raw.get("reward_success")
        if not isinstance(reward_success, bool):
            issues.append("reward_success_must_be_boolean")
        reward_amount = raw.get("reward_amount")
        if (
            isinstance(reward_amount, bool)
            or not isinstance(reward_amount, int)
            or not 0 <= reward_amount <= MAX_REWARD_AMOUNT
        ):
            issues.append("invalid_reward_amount")
        quality_flags = raw.get("quality_flags")
        if not isinstance(quality_flags, list) or any(
            not isinstance(flag, str) for flag in quality_flags
        ):
            issues.append("quality_flags_must_be_string_list")
        elif quality_flags:
            issues.append("row_quality_flags_present")
        if event_token is not None and event_token in seen_events:
            issues.append("duplicate_event_token")
        if issues or None in (event_token, subject_token, repeat_group_token):
            invalid.append({"row_index": index, "reasons": _dedupe(issues)})
            continue
        seen_events.add(event_token)
        valid.append(
            ShadowObservation(
                event_token=event_token,
                subject_token=subject_token,
                repeat_group_token=repeat_group_token,
                cohort=raw["cohort"],
                adjustment_context=raw["adjustment_context"],
                reward_success=reward_success,
                reward_amount=reward_amount,
            )
        )
    valid.sort(key=lambda row: (row.repeat_group_token, row.event_token))
    return valid, invalid


def _cap_repeat_groups(rows: Sequence[ShadowObservation]) -> tuple[list[ShadowObservation], bool]:
    by_group: dict[str, list[ShadowObservation]] = defaultdict(list)
    for row in rows:
        by_group[row.repeat_group_token].append(row)
    capped: list[ShadowObservation] = []
    cap_applied = False
    for group in sorted(by_group):
        group_rows = sorted(by_group[group], key=lambda row: row.event_token)
        if len(group_rows) > MAX_REPEAT_GROUP_OBSERVATIONS:
            cap_applied = True
        capped.extend(group_rows[:MAX_REPEAT_GROUP_OBSERVATIONS])
    capped.sort(key=lambda row: (row.cohort, row.adjustment_context, row.event_token))
    return capped, cap_applied


def _stratum_results(rows: Sequence[ShadowObservation]) -> tuple[list[dict[str, Any]], list[str]]:
    by_stratum: dict[tuple[str, str], list[ShadowObservation]] = defaultdict(list)
    for row in rows:
        by_stratum[row.stratum_key].append(row)
    results: list[dict[str, Any]] = []
    reasons: list[str] = []
    for (cohort, adjustment_context), stratum_rows in sorted(by_stratum.items()):
        success_count = sum(row.reward_success for row in stratum_rows)
        stratum_reasons: list[str] = []
        if len(stratum_rows) < MIN_STRATUM_SAMPLES:
            stratum_reasons.append("small_stratum_sample")
            reasons.append("small_stratum_sample")
        results.append(
            {
                "cohort": cohort,
                "adjustment_context": adjustment_context,
                "status": "qualified" if not stratum_reasons else "abstain",
                "abstention_reasons": stratum_reasons,
                "sample_count": len(stratum_rows),
                "distinct_subject_count": len({row.subject_token for row in stratum_rows}),
                "success_count": success_count,
                "success_fraction": _wilson(success_count, len(stratum_rows)),
                "reward_amount_total": sum(row.reward_amount for row in stratum_rows),
            }
        )
    return results, _dedupe(reasons)


def _digest(value: Any) -> str:
    canonical = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def _decision(
    *,
    current_value: int | None,
    rows: Sequence[ShadowObservation],
    history: Mapping[str, Any],
    as_of: datetime,
    reasons: list[str],
) -> tuple[str, int | None, list[str], dict[str, Any]]:
    decision_reasons = list(reasons)
    sample_count = len(rows)
    distinct_subjects = len({row.subject_token for row in rows})
    repeat_groups: dict[str, int] = defaultdict(int)
    for row in rows:
        repeat_groups[row.repeat_group_token] += 1
    distinct_repeat_groups = len(repeat_groups)
    if sample_count < MIN_SAMPLES:
        decision_reasons.append("sparse_sample")
    if distinct_subjects < MIN_DISTINCT_SUBJECTS:
        decision_reasons.append("insufficient_distinct_subjects")
    if distinct_repeat_groups < MIN_DISTINCT_REPEAT_GROUPS:
        decision_reasons.append("insufficient_distinct_repeat_groups")
    if sample_count:
        max_share_milli = max(repeat_groups.values()) * 1000 // sample_count
        if max_share_milli > MAX_REPEAT_GROUP_SHARE_MILLI:
            decision_reasons.append("repeat_group_dominates")
    else:
        max_share_milli = 0
        decision_reasons.append("empty_sample")

    success_count = sum(row.reward_success for row in rows)
    observed_success_milli = (
        (success_count * 1000 + sample_count // 2) // sample_count if sample_count else None
    )
    rule = {
        "target_success_milli": TARGET_SUCCESS_MILLI,
        "target_success": _format_milli(TARGET_SUCCESS_MILLI),
        "deadband_milli": TARGET_DEADBAND_MILLI,
        "deadband": _format_milli(TARGET_DEADBAND_MILLI),
        "observed_success_milli": observed_success_milli,
        "observed_success": _format_milli(observed_success_milli),
        "max_repeat_group_share_milli": max_share_milli,
        "min_samples": MIN_SAMPLES,
        "min_distinct_subjects": MIN_DISTINCT_SUBJECTS,
        "min_distinct_repeat_groups": MIN_DISTINCT_REPEAT_GROUPS,
    }
    if decision_reasons or current_value is None or observed_success_milli is None:
        return "abstain", None, _dedupe(decision_reasons), rule

    proposed = current_value
    direction = "hold"
    if observed_success_milli < TARGET_SUCCESS_MILLI - TARGET_DEADBAND_MILLI:
        direction = "increase"
        proposed = min(TARGET_MAX_MILLI, current_value + TARGET_STEP_MILLI)
    elif observed_success_milli > TARGET_SUCCESS_MILLI + TARGET_DEADBAND_MILLI:
        direction = "decrease"
        proposed = max(TARGET_MIN_MILLI, current_value - TARGET_STEP_MILLI)
    else:
        decision_reasons.append("inside_hysteresis_band")

    last = history.get("last_recommendation")
    if last is not None and proposed != current_value:
        issued_at = datetime.fromisoformat(last["issued_at_utc"].replace("Z", "+00:00"))
        if as_of < issued_at + COOLDOWN:
            decision_reasons.append("cooldown_active")
            return "cooldown_hold", current_value, _dedupe(decision_reasons), rule
        last_proposed = last.get("proposed_value_milli")
        if last_proposed is not None and abs(proposed - last_proposed) < HYSTERESIS_MILLI:
            decision_reasons.append("hysteresis_suppressed")
            return "hysteresis_hold", current_value, _dedupe(decision_reasons), rule

    if proposed == current_value:
        if direction != "hold":
            decision_reasons.append("target_bound_reached")
        return "hold", current_value, _dedupe(decision_reasons), rule
    return "recommend", proposed, _dedupe(decision_reasons), {**rule, "direction": direction}


def build_recommendation(payload: Mapping[str, Any]) -> dict[str, Any]:
    """Return one deterministic shadow record for a validated input export."""

    _reject_forbidden_fields(payload)
    schema_version = _integer(_required(payload, "schema_version"), "schema_version", minimum=0)
    if schema_version != SCHEMA_VERSION:
        raise RecommendationInputError(f"schema_version must be {SCHEMA_VERSION}")
    policy_version = _string(_required(payload, "policy_version"), "policy_version", max_length=64)
    policy_reasons = [] if policy_version == POLICY_VERSION else ["unsupported_policy_version"]
    as_of = _parse_utc(_required(payload, "as_of_utc"), "as_of_utc")
    report, report_reasons = _parse_report(_required(payload, "report"), as_of=as_of)
    target, target_reasons = _parse_target(
        _required(payload, "target"),
        expected_config_generation=report["config_generation"],
    )
    history, history_reasons = _parse_history(_required(payload, "history"), as_of=as_of)
    rows, invalid_rows = _parse_rows(
        _required(payload, "rows"),
        report_generation=report["generation"],
        config_generation=report["config_generation"],
    )
    capped_rows, repeat_cap_applied = _cap_repeat_groups(rows)
    strata, stratum_reasons = _stratum_results(capped_rows)
    reasons = _dedupe(
        [
            *policy_reasons,
            *report_reasons,
            *target_reasons,
            *history_reasons,
            *stratum_reasons,
            *( ["invalid_or_manipulated_rows"] if invalid_rows else []),
        ]
    )

    status, proposed_value, decision_reasons, decision_rule = _decision(
        current_value=target["current_value_milli"],
        rows=capped_rows,
        history=history,
        as_of=as_of,
        reasons=reasons,
    )
    reasons = _dedupe([*reasons, *decision_reasons])
    success_count = sum(row.reward_success for row in capped_rows)
    evidence_rows = [
        {
            "event_token": row.event_token,
            "subject_token": row.subject_token,
            "repeat_group_token": row.repeat_group_token,
            "cohort": row.cohort,
            "adjustment_context": row.adjustment_context,
            "reward_success": row.reward_success,
            "reward_amount": row.reward_amount,
        }
        for row in capped_rows
    ]
    evidence_fingerprint = _digest(
        {
            "report": report,
            "policy_version": policy_version,
            "target": target,
            "rows": evidence_rows,
        }
    )
    recommendation_id = "shadow-" + _digest(
        {
            "policy_version": policy_version,
            "report": report,
            "target": target,
            "history": history,
            "evidence_fingerprint": evidence_fingerprint,
            "status": status,
            "proposed_value_milli": proposed_value,
            "reasons": reasons,
        }
    )[:24]
    distinct_repeat_groups = len({row.repeat_group_token for row in capped_rows})
    return {
        "schema_version": SCHEMA_VERSION,
        "policy_version": POLICY_VERSION,
        "status": status,
        "recommendation_id": recommendation_id,
        "as_of_utc": as_of.isoformat().replace("+00:00", "Z"),
        "abstention_reasons": reasons if status == "abstain" else [],
        "input_reference": {
            "report_name": report["name"],
            "definition_version": report["definition_version"],
            "generation": report["generation"],
            "config_generation": report["config_generation"],
            "environment_id": report["environment_id"],
            "season_id": report["season_id"],
            "coverage": report["coverage"],
            "expires_at_utc": report["expires_at_utc"],
        },
        "target": {
            **target,
            "proposed_value_milli": proposed_value,
            "proposed_value": _format_milli(proposed_value),
        },
        "evidence": {
            "coverage_window": {
                "start_utc": report["coverage"]["start_utc"],
                "end_utc": report["coverage"]["end_utc"],
            },
            "sample_count_raw": len(rows),
            "sample_count_capped": len(capped_rows),
            "distinct_subject_count": len({row.subject_token for row in capped_rows}),
            "distinct_repeat_group_count": distinct_repeat_groups,
            "success_count": success_count,
            "success_fraction": _wilson(success_count, len(capped_rows)),
            "repeat_group_cap_applied": repeat_cap_applied,
            "max_repeat_group_observations": MAX_REPEAT_GROUP_OBSERVATIONS,
            "strata": strata,
            "excluded_rows": invalid_rows,
            "fingerprint": evidence_fingerprint,
        },
        "decision": {
            **decision_rule,
            "reason_codes": reasons,
            "cooldown_days": COOLDOWN.days,
            "hysteresis_milli": HYSTERESIS_MILLI,
        },
        "constraints": {
            "shadow_only": True,
            "can_apply": False,
            "gameplay_mutation": False,
            "automatic_balance_mutation": False,
            "write_privilege": False,
            "feedback_history": history,
        },
    }


def _read_input(path: Path) -> Mapping[str, Any]:
    try:
        data = path.read_bytes()
    except OSError as exc:
        raise RecommendationInputError(f"cannot read input: {path}") from exc
    if len(data) > MAX_INPUT_BYTES:
        raise RecommendationInputError(f"input exceeds hard bound of {MAX_INPUT_BYTES} bytes")
    try:
        parsed = json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise RecommendationInputError("input must be UTF-8 JSON") from exc
    return _mapping(parsed, "input")


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="pre-redacted shadow export JSON")
    parser.add_argument("--output", type=Path, help="write the recommendation JSON here")
    args = parser.parse_args(argv)
    try:
        recommendation = build_recommendation(_read_input(args.input))
    except (OSError, RecommendationInputError) as exc:
        print(f"balance-shadow input error: {exc}", file=sys.stderr)
        return 2
    serialized = json.dumps(recommendation, indent=2, sort_keys=True) + "\n"
    if args.output is None:
        sys.stdout.write(serialized)
    else:
        try:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(serialized, encoding="utf-8", newline="\n")
        except OSError as exc:
            print(f"balance-shadow output error: {exc}", file=sys.stderr)
            return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
