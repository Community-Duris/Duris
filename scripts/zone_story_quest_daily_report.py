#!/usr/bin/env python3
"""Build an evidence-only suitability report for zone-story daily quests.

The input is newline-delimited JSON exported from the authoritative quest
telemetry stream.  This tool does not assign quests, mutate server state, or
invent a population denominator.  It reports only observed attempts and the
explicit policy checks that make an active production definition eligible.
"""

from __future__ import annotations

import argparse
import collections
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from zone_story_quest_catalog import validate_catalog


OUTCOMES = {"success", "failure", "abandoned", "inaccessible", "stale_revision"}
REQUIRED = {
    "observation_id",
    "quest_definition_id",
    "content_revision",
    "observed_at",
    "pid",
    "level",
    "racewar",
    "party_size",
    "strongest_party_level",
    "duration_seconds",
    "outcome",
    "accessible",
}


class ReportError(ValueError):
    """The evidence stream cannot be trusted as a stable telemetry input."""


def read_observations(path: Path) -> list[dict]:
    observations: dict[str, dict] = {}
    with path.open(encoding="utf-8") as source:
        for line_number, raw in enumerate(source, 1):
            if not raw.strip():
                continue
            try:
                value = json.loads(raw)
            except json.JSONDecodeError as error:
                raise ReportError(f"line {line_number}: invalid JSON: {error}") from error
            if not isinstance(value, dict) or not REQUIRED <= set(value):
                missing = sorted(REQUIRED - set(value)) if isinstance(value, dict) else sorted(REQUIRED)
                raise ReportError(f"line {line_number}: missing telemetry fields {missing}")
            observation_id = value["observation_id"]
            if not isinstance(observation_id, str) or not observation_id:
                raise ReportError(f"line {line_number}: observation_id must be non-empty")
            if value["outcome"] not in OUTCOMES:
                raise ReportError(f"line {line_number}: unsupported outcome")
            if any(not isinstance(value[field], int) for field in (
                "content_revision", "observed_at", "pid", "level", "racewar",
                "party_size", "strongest_party_level", "duration_seconds",
            )) or value["content_revision"] <= 0 or value["observed_at"] <= 0 or \
                    value["pid"] <= 0 or value["level"] < 0 or value["party_size"] < 0 or \
                    value["strongest_party_level"] < 0 or value["duration_seconds"] < 0:
                raise ReportError(f"line {line_number}: telemetry numeric bounds are invalid")
            if not isinstance(value["accessible"], bool):
                raise ReportError(f"line {line_number}: accessible must be boolean")
            previous = observations.get(observation_id)
            if previous is not None and previous != value:
                raise ReportError(f"line {line_number}: observation ID was reused with different data")
            observations[observation_id] = value
    return list(observations.values())


def policy_from_args(args: argparse.Namespace) -> dict:
    return {
        "minimum_attempts": args.minimum_attempts,
        "minimum_distinct_pids": args.minimum_distinct_pids,
        "minimum_successes": args.minimum_successes,
        "minimum_level": args.minimum_level,
        "maximum_level": args.maximum_level,
        "minimum_racewar": args.minimum_racewar,
        "maximum_racewar": args.maximum_racewar,
        "maximum_party_level_delta": args.maximum_party_level_delta,
        "require_accessible_evidence": args.require_accessible_evidence,
        "require_known_party_context": args.require_known_party_context,
    }


def quest_report(definition: dict, observations: list[dict], policy: dict) -> dict:
    outcomes = collections.Counter(observation["outcome"] for observation in observations)
    pids = {observation["pid"] for observation in observations}
    levels = [observation["level"] for observation in observations]
    racewars = [observation["racewar"] for observation in observations]
    inaccessible = outcomes["inaccessible"]
    stale = outcomes["stale_revision"]
    unknown_party = sum(
        observation["party_size"] == 0 or observation["strongest_party_level"] == 0
        for observation in observations
    )
    carried = sum(
        observation["party_size"] > 0 and observation["strongest_party_level"] > 0 and
        observation["strongest_party_level"] >
        observation["level"] + policy["maximum_party_level_delta"]
        for observation in observations
    )
    all_accessible = bool(observations) and all(
        observation["accessible"] for observation in observations
    )
    level_ok = bool(levels) and min(levels) >= policy["minimum_level"] and (
        policy["maximum_level"] == 0 or max(levels) <= policy["maximum_level"]
    )
    racewar_ok = not racewars or (
        (policy["minimum_racewar"] == 0 or min(racewars) >= policy["minimum_racewar"]) and
        (policy["maximum_racewar"] == 0 or max(racewars) <= policy["maximum_racewar"])
    )
    accessible_ok = not policy["require_accessible_evidence"] or all_accessible
    party_ok = unknown_party == 0 and carried == 0 and (
        not policy["require_known_party_context"] or unknown_party == 0
    )
    current = definition["content_revision"] == definition["_catalog_revision"]
    checks = {
        "active": definition["active"],
        "eligible_for_zone_completion": definition["eligible_for_zone_completion"],
        "current_catalog_revision": current,
        "minimum_attempts": len(observations) >= policy["minimum_attempts"],
        "minimum_distinct_pids": len(pids) >= policy["minimum_distinct_pids"],
        "minimum_successes": outcomes["success"] >= policy["minimum_successes"],
        "level_range": level_ok,
        "racewar_range": racewar_ok,
        "accessible_evidence": accessible_ok and inaccessible == 0,
        "known_party_context": party_ok,
        "no_stale_revision": stale == 0,
    }
    suitable = all(checks.values())
    reason = (
        "observed evidence satisfies the configured policy"
        if suitable else
        "; ".join(name + " is not satisfied" for name, passed in checks.items() if not passed)
    )
    return {
        "quest_definition_id": definition["definition_id"],
        "content_revision": definition["content_revision"],
        "zone_number": definition["zone_number"],
        "observed_attempts": len(observations),
        "distinct_pids": len(pids),
        "successful_attempts": outcomes["success"],
        "outcomes": {name: outcomes[name] for name in sorted(OUTCOMES)},
        "minimum_level_observed": min(levels) if levels else None,
        "maximum_level_observed": max(levels) if levels else None,
        "minimum_racewar_observed": min(racewars) if racewars else None,
        "maximum_racewar_observed": max(racewars) if racewars else None,
        "inaccessible_attempts": inaccessible,
        "stale_revision_attempts": stale,
        "unknown_party_context_attempts": unknown_party,
        "carried_attempts": carried,
        "all_observed_accessible": all_accessible,
        "checks": checks,
        "suitable": suitable,
        "explanation": reason,
    }


def build_report(catalog: dict, observations: list[dict], policy: dict) -> dict:
    diagnostics = validate_catalog(catalog)
    if diagnostics:
        raise ReportError("catalog is invalid: " + diagnostics[0]["message"])
    revision = catalog["content_revision"]
    definitions = []
    for definition in catalog["definitions"]:
        item = dict(definition)
        item["_catalog_revision"] = revision
        definitions.append(item)
    by_key: dict[tuple[str, int], list[dict]] = collections.defaultdict(list)
    for observation in observations:
        by_key[(observation["quest_definition_id"], observation["content_revision"])].append(observation)
    known_keys = {(definition["definition_id"], definition["content_revision"])
                  for definition in definitions}
    unmatched = [observation for observation in observations
                 if (observation["quest_definition_id"], observation["content_revision"])
                 not in known_keys]
    reports = [
        quest_report(
            definition,
            by_key[(definition["definition_id"], definition["content_revision"])],
            policy,
        )
        for definition in definitions
    ]
    return {
        "schema_version": 1,
        "catalog_revision": revision,
        "observed_input_count": len(observations),
        "unmatched_observation_count": len(unmatched),
        "unmatched_observation_definition_ids": sorted({
            observation["quest_definition_id"] for observation in unmatched
        }),
        "policy": policy,
        "no_invented_denominator": True,
        "assignments_not_mutated": True,
        "quests": reports,
        "suitable_quest_definition_ids": [
            item["quest_definition_id"] for item in reports if item["suitable"]
        ],
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--observations", type=Path, required=True)
    parser.add_argument(
        "--catalog", type=Path,
        default=Path(__file__).resolve().parents[1] /
        "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json",
    )
    parser.add_argument("--format", choices=("json", "text"), default="json")
    parser.add_argument("--minimum-attempts", type=int, default=20)
    parser.add_argument("--minimum-distinct-pids", type=int, default=5)
    parser.add_argument("--minimum-successes", type=int, default=1)
    parser.add_argument("--minimum-level", type=int, default=1)
    parser.add_argument("--maximum-level", type=int, default=0)
    parser.add_argument("--minimum-racewar", type=int, default=0)
    parser.add_argument("--maximum-racewar", type=int, default=0)
    parser.add_argument("--maximum-party-level-delta", type=int, default=10)
    parser.add_argument("--require-accessible-evidence", action=argparse.BooleanOptionalAction,
                        default=True)
    parser.add_argument("--require-known-party-context", action=argparse.BooleanOptionalAction,
                        default=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
        report = build_report(catalog, read_observations(args.observations), policy_from_args(args))
    except (OSError, json.JSONDecodeError, ReportError) as error:
        print(f"daily evidence report failed: {error}", file=sys.stderr)
        return 2
    if args.format == "json":
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(f"catalog revision {report['catalog_revision']}; observed attempts {report['observed_input_count']}")
        for item in report["quests"]:
            if item["observed_attempts"]:
                print(f"{item['quest_definition_id']}: {item['observed_attempts']} attempts, "
                      f"{item['distinct_pids']} PIDs, suitable={str(item['suitable']).lower()} "
                      f"({item['explanation']})")
        print(f"suitable candidates: {len(report['suitable_quest_definition_ids'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
