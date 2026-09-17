"""Pure encounter report semantics for the #271 telemetry fact plane.

This module contains no database connection and never reads the raw fact
stream. The aggregate/report owners can consume the definition while keeping
run attempts, roster effort, and expected reward credit distinct.
"""
from __future__ import annotations

from types import MappingProxyType

DEFINITION_VERSION = 1

ENCOUNTER_EVENT_KINDS = MappingProxyType(
    {
        "start": 1,
        "participant_join": 2,
        "participant_leave": 3,
        "close": 4,
        "participant_summary": 5,
    }
)
ENCOUNTER_OUTCOMES = MappingProxyType(
    {
        "unknown": 0,
        "success": 1,
        "failure": 2,
        "death": 3,
        "flee": 4,
        "withdrawal": 5,
        "abandonment": 6,
        "timeout": 7,
        "copyover": 8,
        "shutdown": 9,
        "unknown_close": 10,
    }
)
TERMINAL_OUTCOMES = frozenset(
    name for name in ENCOUNTER_OUTCOMES if name not in {"unknown"}
)
ATTEMPT_DENOMINATOR_OUTCOMES = TERMINAL_OUTCOMES

ENCOUNTER_REPORT_DEFINITION = MappingProxyType(
    {
        "name": "encounter_runs",
        "definition_version": DEFINITION_VERSION,
        "grain": "encounter_run",
        "source_table": "telemetry_interval",
        "event_kind": "encounter_event",
        "metrics": (
            "run_elapsed_usec",
            "participant_usec",
            "distinct_participant_count",
            "expected_credit_count",
            "quality_flags",
        ),
        "denominators": {
            "attempted_runs": "one per closed encounter, including every terminal outcome",
            "successful_runs": "closed encounters with outcome=success",
            "participant_effort": "sum participant_summary.participant_usec",
            "expected_credit": "sum close.expected_credit_count; never participation",
        },
        "unclosed_tail_policy": "classify as unclosed_tail and keep duration unknown",
    }
)


def outcome_name(value: int) -> str:
    """Return the stable outcome name or reject an unknown enum value."""

    for name, number in ENCOUNTER_OUTCOMES.items():
        if number == value:
            return name
    raise ValueError(f"unknown encounter outcome {value!r}")


def denominator_bucket(value: int | None) -> str:
    """Classify a close for denominator accounting without inventing tails."""

    if value is None:
        return "unclosed_tail"
    name = outcome_name(value)
    if name not in ATTEMPT_DENOMINATOR_OUTCOMES:
        raise ValueError("unknown outcome cannot be a terminal denominator")
    return "successful" if name == "success" else "attempted_non_success"


__all__ = [
    "ATTEMPT_DENOMINATOR_OUTCOMES",
    "DEFINITION_VERSION",
    "ENCOUNTER_EVENT_KINDS",
    "ENCOUNTER_OUTCOMES",
    "ENCOUNTER_REPORT_DEFINITION",
    "TERMINAL_OUTCOMES",
    "denominator_bucket",
    "outcome_name",
]
