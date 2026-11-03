#!/usr/bin/env python3

import importlib.util
import json
import pathlib
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "zone_story_quest_catalog.py"
spec = importlib.util.spec_from_file_location("zone_story_quest_catalog", SCRIPT)
catalog_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog_module)

catalog = catalog_module.production_catalog(ROOT)
snapshot = ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json"
assert json.loads(snapshot.read_text(encoding="utf-8")) == catalog, "regenerate the production catalog after editing journal maps"
assert catalog["source"]["kind"] == "legacy_static_qst_and_scripted_outcomes"
assert catalog["source"]["area_list"] == "areas/AREA"
assert catalog["source"]["excludes"] == ["bartender_random_world_quests"]
assert sum(d["definition_id"].startswith("zone-story:qst:") for d in catalog["definitions"]) == 2668
scripted = [d for d in catalog["definitions"] if d["definition_id"].startswith("zone-story:scripted:")]
assert {d["giver_vnum"] for d in scripted} == {93602, 93603, 93611}
assert all(d["source_area"] == "bordello" and not d["daily_eligible"] for d in scripted)
assert all(not u["achievement"] and not u["daily_candidate"]
           for u in catalog_module.story_units(catalog) if u["zone_number"] == 936)
assert all(item["zone_number"] >= 0 for item in catalog["definitions"])
assert all(not item["eligible_for_zone_completion"] and not item["daily_eligible"]
           for item in catalog["definitions"] if item["zone_number"] == 0)
assert all(item["source_system"] == "zone_story" for item in catalog["definitions"])
assert all(item["repeatable"] or not item["daily_eligible"] for item in catalog["definitions"])
assert len({item["definition_id"] for item in catalog["definitions"]}) == len(catalog["definitions"])

with tempfile.TemporaryDirectory(prefix="duris-zone-story-production-catalog-") as temporary:
    output = pathlib.Path(temporary) / "catalog.json"
    subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            "--source-root",
            str(ROOT),
            "--production-output",
            str(output),
            "--check",
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    written = json.loads(output.read_text(encoding="utf-8"))
    assert written["source"]["fingerprint_sha256"] == catalog["source"]["fingerprint_sha256"]
    assert len(written["definitions"]) == len(catalog["definitions"])

print("zone-story production catalog coverage regression passed")
