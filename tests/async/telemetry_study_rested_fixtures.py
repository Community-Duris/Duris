"""Deterministic, redacted fixtures for the #273 rested-bonus study."""
from __future__ import annotations

from copy import deepcopy


WINDOWS = ("under_1h", "1_4h", "4_24h", "1_3d", "3d_plus")


def qualified_export(*, synthetic_fixture: bool = True) -> dict:
    rows = []
    cell_index = 0
    for cohort in ("solo", "group"):
        for rested_tier in ("unrested", "rested"):
            for config_generation in ("cfg-v1", "cfg-v2"):
                for index in range(20):
                    window = WINDOWS[index // 4]
                    active_usec = 120_000_000 + index * 1_000_000
                    rows.append(
                        {
                            "subject_token": f"subject-{cell_index:02d}-{index:02d}",
                            "repeat_group_token": f"repeat-{index % 10:02d}",
                            "cohort": cohort,
                            "rested_tier": rested_tier,
                            "config_generation": config_generation,
                            "return_window": window,
                            "returned": index % 3 != 0,
                            "censored": False,
                            "mode_qualified": True,
                            "active_usec": active_usec,
                            "connected_usec": active_usec + 30_000_000,
                            "xp_earned": 100 + index * 7 + (20 if rested_tier != "unrested" else 0),
                            "quality_flags": [],
                        }
                    )
                cell_index += 1
    return {
        "schema_version": 1,
        "protocol_version": 1,
        "study_id": "rested_bonus",
        "source": {
            "kind": "published_report_export",
            "interfaces": [
                "telemetry.return.v1",
                "telemetry.playtime.v1",
                "telemetry.progression.v1",
            ],
            "redacted": True,
            "synthetic_fixture": synthetic_fixture,
        },
        "scope": {
            "environment_id": 8,
            "season_id": 9,
            "config_generations": ["cfg-v1", "cfg-v2"],
            "coverage": {
                "published": True,
                "complete": True,
                "published_generation": 101,
                "start_utc": "2026-01-01T00:00:00Z",
                "end_utc": "2026-01-15T00:00:00Z",
                "quality_flags": [],
            },
        },
        "rows": rows,
    }


def empty_export() -> dict:
    payload = qualified_export()
    payload["rows"] = []
    return payload


def small_export() -> dict:
    payload = qualified_export()
    payload["rows"] = payload["rows"][:3]
    return payload


def copy_export() -> dict:
    return deepcopy(qualified_export())
