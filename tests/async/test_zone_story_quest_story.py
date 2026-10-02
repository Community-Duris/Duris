#!/usr/bin/env python3

import copy
import importlib.util
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("catalog_tool", ROOT / "scripts/zone_story_quest_catalog.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
catalog = module.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "twin_towers_forest")
report = module.report_for(catalog)
assert report["valid"] and report["eligible_by_zone"]["135"] == 10
assert report["daily_unit_count"] == 2055

coverage_spec = importlib.util.spec_from_file_location("home_coverage", ROOT / "scripts/zone_story_quest_home_coverage.py")
coverage = importlib.util.module_from_spec(coverage_spec)
coverage_spec.loader.exec_module(coverage)
required = coverage.required_areas(ROOT)
assert len(required["areas"]) == 27 and required["excluded_empty_town_markers"] == ["end"]
assert all((ROOT / r["mapping"]).is_file() for r in required["areas"])
assert all(next(m for m in catalog["story_mappings"] if m["source_area"] == r["source_area"])["schema_version"] == 2 for r in required["areas"])

with tempfile.TemporaryDirectory(prefix="duris-authored-story-") as temporary:
    binary = pathlib.Path(temporary) / "story_test"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc",
        "tests/async/zone_story_quest_story_harness.cpp",
        "src/world/zone_story_quest_tracking.c", "src/world/zone_story_quest_catalog.c",
        "src/world/zone_story_quest_story.c", "src/world/zone_story_quest_feature.c",
        "-lcjson", "-o", str(binary),
    ], cwd=ROOT, check=True)
    native = ROOT / "docs/reference/ZONE_STORY_QUEST_PRODUCTION_CATALOG.json"
    subprocess.run([str(binary), str(native), str(ROOT / "areas/story/twin_towers_forest.story.json")], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(native), str(ROOT / "areas/story/twin_towers_forest.story.json"), "all"], cwd=ROOT, check=True)
    legacy_mapping = copy.deepcopy(mapping)
    legacy_mapping["schema_version"] = 1
    for key in ("introduction", "orientation", "contacts"):
        legacy_mapping.pop(key)
    legacy_path = pathlib.Path(temporary) / "schema-one.json"
    legacy_path.write_text(json.dumps(legacy_mapping), encoding="utf-8")
    subprocess.run([str(binary), str(native), str(legacy_path)], cwd=ROOT, check=True)

    partial = copy.deepcopy(mapping)
    partial["coverage"] = "partial"
    partial["stories"] = partial["stories"][:1]
    partial["exclusions"] = []
    for mode in ("partial", "service"):
        if mode == "service":
            partial["stories"][0]["category"] = "service"
        path = pathlib.Path(temporary) / f"{mode}.json"
        path.write_text(json.dumps(partial), encoding="utf-8")
        candidate = copy.deepcopy(catalog)
        candidate["story_mappings"] = [partial]
        assert module.report_for(candidate)["eligible_by_zone"]["135"] == (76 if mode == "service" else 77)
        subprocess.run([str(binary), str(native), str(path), mode], cwd=ROOT, check=True)

    def invalid(name, mutate):
        bad = copy.deepcopy(mapping)
        mutate(bad)
        candidate = copy.deepcopy(catalog)
        candidate["story_mappings"] = [bad]
        assert any(item["code"] == "invalid_story_mapping" for item in module.validate_catalog(candidate)), name
        path = pathlib.Path(temporary) / f"{name}.json"
        path.write_text(json.dumps(bad), encoding="utf-8")
        subprocess.run([str(binary), str(native), str(path), "invalid"], cwd=ROOT, check=True)

    invalid("unknown-contract", lambda m: m["stories"][0]["contracts"][0].update(giver_vnum=999999))
    invalid("duplicate-binding", lambda m: m["stories"][1]["contracts"].append(m["stories"][0]["contracts"][0]))
    invalid("incomplete-coverage", lambda m: m["exclusions"].pop())
    invalid("unknown-field", lambda m: m.update(rewards=10))
    invalid("untracked-event", lambda m: m["stories"][0]["steps"][0].update(kind="recover_item"))
    invalid("wrong-slot", lambda m: m["stories"][0]["steps"][0].update(slot=43))
    invalid("invalid-count", lambda m: m["stories"][0]["steps"][0].update(count=True))
    invalid("duplicate-item", lambda m: m["stories"][0]["steps"][0]["item_vnums"].append(13521))
    invalid("duplicate-step", lambda m: m["stories"][0]["steps"].append(m["stories"][0]["steps"][0]))
    invalid("control-text", lambda m: m["stories"][0].update(summary="bad\ntext"))
    invalid("wrong-area", lambda m: m.update(source_area="alatorin"))
    invalid("duplicate-contact", lambda m: m["contacts"].append(m["contacts"][0]))
    invalid("invalid-keyword", lambda m: m["contacts"][0].update(keyword="alvinar hello"))
    invalid("duplicate-topic", lambda m: m["contacts"][0]["topics"].append(m["contacts"][0]["topics"][0]))
    invalid("invalid-orientation", lambda m: m["orientation"].append("bad\ncommand"))
    unknown_item = copy.deepcopy(catalog)
    unknown_item["story_mappings"] = [copy.deepcopy(mapping)]
    unknown_item["story_mappings"][0]["stories"][0]["steps"][0]["item_vnums"] = [999999]
    try:
        module.story_units(unknown_item, {13521})
    except ValueError:
        pass
    else:
        raise AssertionError("unknown object prototype was accepted")
    duplicate = pathlib.Path(temporary) / "duplicate-field.json"
    duplicate.write_text(json.dumps(mapping).replace('"schema_version": 2', '"schema_version": 2, "schema_version": 2'), encoding="utf-8")
    subprocess.run([str(binary), str(native), str(duplicate), "invalid"], cwd=ROOT, check=True)

print("builder-authored zone story schema and projection regression passed")
