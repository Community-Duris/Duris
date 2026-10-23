#!/usr/bin/env python3
"""Preserve registered source-file case in Python and native journal loading."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import zone_story_quest_catalog as catalog_module
import zone_story_quest_zone_inventory as inventory_module

catalog = catalog_module.production_catalog(ROOT)
catalog["story_mappings"] = []
evidence = inventory_module.area_evidence(ROOT, "Voluntown")
assert evidence["zone"]["source_area"] == "Voluntown"
mapping = {
    "schema_version": 3,
    "revision": 1,
    "source_area": "Voluntown",
    "coverage": "complete",
    "introduction": "A journal for an existing capitalized area filename.",
    "orientation": ["Ask the caretaker about the royal families."],
    "contacts": [{"mob_vnum": 142400, "name": "The caretaker", "keyword": "caretaker",
                  "description": "Explains the crypts.", "topics": ["drakenstone"]}],
    "stories": [],
    "exclusions": [],
}
for index, request in enumerate(evidence["requests"]):
    contract = request["block"]["binding"]
    mapping["stories"].append({
        "id": f"return-{index}",
        "title": f"Accepted return {index}",
        "category": "story",
        "summary": "An exact native return from the registered area.",
        "contracts": [contract],
        "steps": [{"id": "turn-in", "text": "Complete the return", "kind": "completion",
                   "contracts": [contract], "hint": "Requires the native receipt."}],
    })

cases = [("registered-case", mapping, True)]
for name, mutate in (
    ("wrong-case", lambda m: m.update(source_area="voluntown")),
    ("traversal", lambda m: m.update(source_area="Voluntown/../Voluntown")),
    ("uppercase-story-id", lambda m: m["stories"][0].update(id="Return-0")),
    ("uppercase-step-id", lambda m: m["stories"][0]["steps"][0].update(id="Turn-in")),
    ("uppercase-keyword", lambda m: m["contacts"][0].update(keyword="Caretaker")),
    ("uppercase-topic", lambda m: m["contacts"][0].update(topics=["Drakenstone"])),
):
    candidate = copy.deepcopy(mapping)
    mutate(candidate)
    cases.append((name, candidate, False))

with tempfile.TemporaryDirectory(prefix="duris-source-area-") as temporary:
    binary = Path(temporary) / "story_test"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "tests/async/zone_story_quest_story_harness.cpp",
        "src/world/zone_story_quest_tracking.c", "src/world/zone_story_quest_catalog.c",
        "src/world/zone_story_quest_story.c", "src/world/zone_story_quest_feature.c",
        "-lcjson", "-o", str(binary),
    ], cwd=ROOT, check=True)
    native = Path(temporary) / "catalog.json"
    native.write_text(json.dumps(catalog), encoding="utf-8")
    for name, candidate_mapping, accepted in cases:
        candidate = copy.deepcopy(catalog)
        candidate["story_mappings"] = [candidate_mapping]
        try:
            units = catalog_module.story_units(candidate)
        except ValueError:
            assert not accepted, name
        else:
            assert accepted, name
            local = [unit for unit in units if unit["zone_number"] == 1424]
            assert len(local) == 2 and all("Voluntown" in unit["id"] for unit in local), name
        directory = Path(temporary) / name
        directory.mkdir()
        path = directory / "Voluntown.story.json"
        path.write_text(json.dumps(candidate_mapping), encoding="utf-8")
        subprocess.run([str(binary), str(native), str(path),
                        "source-area" if accepted else "invalid-source-area"],
                       cwd=ROOT, check=True)

print("registered source area case and strict story identifiers regression passed")
