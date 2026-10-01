#!/usr/bin/env python3

import importlib.util
import json
import pathlib
import re
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "zone_story_quest_catalog.py"
spec = importlib.util.spec_from_file_location("zone_story_quest_catalog", SCRIPT)
catalog_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalog_module)
native_header = (ROOT / "src/world/zone_story_quest_production.h").read_text()
native_limit = int(re.search(r"ZONE_STORY_QUEST_MAX_DURABLE_OFFERINGS\s*=\s*(\d+)", native_header)[1])
assert native_limit == catalog_module.MAX_DURABLE_ITEM_OFFERINGS

catalog = catalog_module.production_catalog(ROOT)
assert catalog["source"]["kind"] == "legacy_static_qst"
assert catalog["source"]["area_list"] == "areas/AREA"
assert catalog["source"]["excludes"] == ["bartender_random_world_quests"]
assert len(catalog["definitions"]) == 2668
assert all(item["zone_number"] >= 0 for item in catalog["definitions"])
assert all(item["source_system"] == "zone_story" for item in catalog["definitions"])
assert catalog["content_revision"] == 2
assert all(item["repeatable"] for item in catalog["definitions"] if item["daily_eligible"])
alatorin = [item for item in catalog["definitions"] if 83100 <= item["giver_vnum"] < 83600]
assert alatorin and all(item["zone_number"] == 831 for item in alatorin)
assert any(zone["discoverable"] and not any(item["zone_number"] == zone["zone_number"] for item in catalog["definitions"]) for zone in catalog["zones"])
assert any(item["daily_exclusion"] == "Item exchange" for item in catalog["definitions"])
assert any(item["daily_exclusion"] == "Unsupported durable offering" for item in catalog["definitions"])
for item in catalog["definitions"]:
    if item["daily_eligible"]:
        give = bytes.fromhex(item["completion_key"]).decode().split(";")[0][5:].split(",")
        assert len(give) <= catalog_module.MAX_DURABLE_ITEM_OFFERINGS
        assert all(goal.startswith("I:") and int(goal[2:]) > 0 for goal in give)
checked_in = json.loads((ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json").read_text())
assert checked_in == catalog
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
