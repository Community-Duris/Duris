#!/usr/bin/env python3
"""Regression coverage for the evidence-only daily suitability report."""

from __future__ import annotations

import importlib.util
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/zone_story_quest_daily_report.py"
spec = importlib.util.spec_from_file_location("zone_story_quest_daily_report", SCRIPT)
report = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(report)


def main() -> None:
    catalog = json.loads(
        (ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json").read_text()
    )
    definition = catalog["definitions"][0]
    observations = []
    for pid in (101, 102):
        observations.append({
            "observation_id": f"obs-{pid}",
            "quest_definition_id": definition["definition_id"],
            "content_revision": catalog["content_revision"],
            "observed_at": 1_700_000_000 + pid,
            "pid": pid,
            "level": 10,
            "racewar": 1,
            "credit_mask": 1,
            "party_size": 2,
            "strongest_party_level": 10,
            "duration_seconds": 30,
            "outcome": "success",
            "accessible": True,
        })
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "observations.jsonl"
        path.write_text("\n".join(json.dumps(item) for item in observations) + "\n")
        loaded = report.read_observations(path)
        policy = {
            "minimum_attempts": 2,
            "minimum_distinct_pids": 2,
            "minimum_successes": 2,
            "minimum_level": 1,
            "maximum_level": 0,
            "minimum_racewar": 0,
            "maximum_racewar": 0,
            "maximum_party_level_delta": 10,
            "require_accessible_evidence": True,
            "require_known_party_context": True,
        }
        result = report.build_report(catalog, loaded, policy)
    assert result["no_invented_denominator"] is True
    assert result["assignments_not_mutated"] is True
    assert result["unmatched_observation_count"] == 0
    assert result["suitable_quest_definition_ids"] == [definition["definition_id"]]
    item = next(item for item in result["quests"]
                if item["quest_definition_id"] == definition["definition_id"])
    assert item["observed_attempts"] == 2
    assert item["distinct_pids"] == 2
    assert item["unknown_party_context_attempts"] == 0
    assert item["carried_attempts"] == 0
    assert item["suitable"] is True

    carried = dict(observations[0])
    carried.update({
        "observation_id": "carried",
        "pid": 103,
        "level": 1,
        "party_size": 2,
        "strongest_party_level": 30,
    })
    carried_result = report.build_report(catalog, observations + [carried], policy)
    carried_item = next(item for item in carried_result["quests"]
                        if item["quest_definition_id"] == definition["definition_id"])
    assert carried_item["carried_attempts"] == 1
    assert carried_item["suitable"] is False
    print("zone-story daily evidence report regression passed")


if __name__ == "__main__":
    main()
