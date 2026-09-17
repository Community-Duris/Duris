"""Deterministic redacted fixtures for the #274 shadow policy."""
from __future__ import annotations

from copy import deepcopy


def shadow_export() -> dict:
    rows = []
    contexts = (
        ("solo", "none"),
        ("group", "trophy"),
        ("pvp", "alignment"),
        ("group_pvp", "trophy_alignment"),
    )
    for index in range(100):
        cohort, adjustment_context = contexts[index // 25]
        rows.append(
            {
                "event_token": f"event-{index:03d}",
                "subject_token": f"subject-{index:03d}",
                "repeat_group_token": f"repeat-{index // 25}-{index % 5:02d}",
                "source_generation": 42,
                "config_generation": "cfg-v1",
                "cohort": cohort,
                "adjustment_context": adjustment_context,
                "eligible": True,
                "reward_success": index % 5 == 0,
                "reward_amount": 100 + index if index % 5 == 0 else 0,
                "quality_flags": [],
            }
        )
    return {
        "schema_version": 1,
        "policy_version": "balance-shadow-v1",
        "as_of_utc": "2026-02-01T00:00:00Z",
        "report": {
            "name": "balance_shadow_outcome_v1",
            "definition_version": 1,
            "generation": 42,
            "config_generation": "cfg-v1",
            "environment_id": 8,
            "season_id": 9,
            "expires_at_utc": "2026-02-14T00:00:00Z",
            "coverage": {
                "start_utc": "2026-01-10T00:00:00Z",
                "end_utc": "2026-01-31T00:00:00Z",
                "published": True,
                "complete": True,
                "quality_flags": [],
            },
        },
        "target": {
            "parameter": "payout.epic.zone.alignmentMod",
            "unit": "fraction",
            "current_value_milli": 200,
            "min_value_milli": 0,
            "max_value_milli": 1000,
            "step_milli": 50,
        },
        "history": {"applied_mutations": 0, "last_recommendation": None},
        "rows": rows,
    }


def copy_export() -> dict:
    return deepcopy(shadow_export())
