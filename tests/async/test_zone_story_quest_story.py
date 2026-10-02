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
assert report["daily_unit_count"] == 1962
assert mapping["schema_version"] == 3 and mapping["stories"][0]["steps"][0]["optional"]
assert sum(s["category"] == "service" for s in mapping["stories"]) == 12
assert len(mapping["exclusions"]) == 1 and len(mapping["exclusions"][0]["contracts"]) == 40
assert sum(len(s["contracts"]) for s in mapping["stories"]) == 44
assert all(s["category"] == "service" for s in mapping["stories"] if s["id"].startswith(("prepare-animal-", "archer-trade-")))
assert "arrows" in next(c for c in mapping["contacts"] if c["mob_vnum"] == 13501)["topics"]
assert "backpack" in next(c for c in mapping["contacts"] if c["mob_vnum"] == 13503)["topics"]

new_areas = {"breale", "elvish", "krimman", "bastine", "pineholl", "quietus", "torg", "solonar"}
new_mappings = {m["source_area"]: m for m in catalog["story_mappings"] if m["source_area"] in new_areas}
assert set(new_mappings) == new_areas and all(m["coverage"] == "complete" for m in new_mappings.values())
assert sum(d["source_area"] in new_areas for d in catalog["definitions"]) == 86
assert sum(len(m["stories"]) for m in new_mappings.values()) == 80
drider = next(s for s in new_mappings["elvish"]["stories"] if s["title"] == "Release the Cursed Drider")
assert len(drider["contracts"]) == 1 and drider["steps"][-2]["item_vnums"] == [35813]
assert len(drider["steps"]) == 6 and all(t["optional"] for t in drider["steps"][:-2])
assert drider["steps"][0]["item_vnums"] == [35824]  # Exact access key, distinct from the reward key.
breale = new_mappings["breale"]
assert breale["schema_version"] == new_mappings["elvish"]["schema_version"] == 3
assert breale["revision"] == new_mappings["elvish"]["revision"] == 2
triad = next(s for s in breale["stories"] if s["id"] == "finish-the-triad-mixture-2602")
assert len(triad["contracts"]) == 1 and sum(t.get("optional", False) for t in triad["steps"]) == 5
assert all(len(t["contracts"]) == 1 for t in triad["steps"] if t["kind"] == "completion")
assert report["eligible_by_zone"]["26"] == 6 and report["eligible_by_zone"]["358"] == 2
promotions = [s for s in new_mappings["bastine"]["stories"] if s["title"].startswith("The Bastine Road:")]
assert len(promotions) == 12 and all(len(s["contracts"]) == 1 for s in promotions)
assert new_mappings["bastine"]["schema_version"] == new_mappings["krimman"]["schema_version"] == 3
assert new_mappings["bastine"]["revision"] == new_mappings["krimman"]["revision"] == 2
assert [next(t["item_vnums"][0] for t in s["steps"] if t["kind"] == "carried_item" and not t.get("optional")) for s in promotions] == [
    41388, 41407, 12802, 41327, 41924, 2607, 41408, 41922, 41920, 41375, 41411, 70970]
assert sum(t.get("optional", False) for t in promotions[-1]["steps"]) == 11
assert report["eligible_by_zone"]["164"] == 8 and report["eligible_by_zone"]["76"] == 14
pineholl = new_mappings["pineholl"]
assert pineholl["schema_version"] == 2 and pineholl["revision"] == 2
assert report["eligible_by_zone"]["160"] == 7 and len(pineholl["stories"]) == 7
clothing = [s for s in pineholl["stories"] if s["contracts"][0]["giver_vnum"] == 16080]
assert [(s["steps"][0]["item_vnums"], s["steps"][0]["count"]) for s in clothing] == [
    ([16019], 3), ([16020], 3), ([16021], 2), ([16024], 3), ([16025], 3)]
assert all(s["category"] == "request" and len(s["contracts"]) == 1 and len(s["steps"]) == 2 for s in clothing)
assert all(len(s["contracts"]) == 1 and s["steps"][-1]["contracts"] == s["contracts"] for s in pineholl["stories"])
assert all(s["category"] == "service" for s in new_mappings["quietus"]["stories"] if "Credentials" in s["title"] or s["title"].startswith("Hear ") or s["title"].startswith("Obtain "))
credentials = [s for s in new_mappings["quietus"]["stories"] if s["steps"][0].get("item_vnums") == [1701, 80808]]
assert len(credentials) == 5 and all("badge" in s["steps"][0]["text"] and "longsword" in s["steps"][0]["text"] for s in credentials)
quietus = new_mappings["quietus"]
assert quietus["schema_version"] == 3 and quietus["revision"] == 2
assert report["eligible_by_zone"]["17"] == 4
assert len(quietus["stories"]) == 11 and sum(s["category"] == "service" for s in quietus["stories"]) == 7
assert sum(len(s["contracts"]) for s in quietus["stories"]) == 16 and not quietus["exclusions"]
quietus_missions = [s for s in quietus["stories"] if s["category"] == "story"]
assert len(quietus_missions) == 4 and all(len(s["contracts"]) == 1 for s in quietus_missions)
assert [sum(t.get("optional", False) for t in s["steps"]) for s in quietus_missions] == [1, 1, 1, 2]
assert [t["item_vnums"] for t in quietus_missions[0]["steps"] if t["kind"] == "carried_item"] == [[1732], [1746]]
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in quietus_missions)
secret_briefing = quietus_missions[-1]["steps"][1]
assert secret_briefing["optional"] and len(secret_briefing["contracts"]) == 2
assert {c["completion_key"] for c in secret_briefing["contracts"]} == {
    "give=I:16429;receive=;disappear=0", "give=I:1747;receive=I:1747;disappear=0"}
assert len(quietus["contacts"]) == 10
assert next(c for c in quietus["contacts"] if c["mob_vnum"] == 1709)["topics"] == ["aresliean", "quest"]
assert next(c for c in quietus["contacts"] if c["mob_vnum"] == 1751)["topics"] == ["name", "aresliean", "darvanu"]
chisel = next(s for s in new_mappings["torg"]["stories"] if s["id"] == "a-fine-chisel-for-the-craftsman")
assert {c["giver_vnum"] for c in chisel["contracts"]} == {29023, 29024}
assert len(chisel["steps"]) == 2 and chisel["steps"][0]["count"] == 1
torg = new_mappings["torg"]
assert torg["schema_version"] == 3 and torg["revision"] == 2
assert report["eligible_by_zone"]["289"] == 12 and len(torg["stories"]) == 14
assert sum(s["category"] == "service" for s in torg["stories"]) == 2
assert sum(len(s["contracts"]) for s in torg["stories"]) == 15 and not torg["exclusions"]
assert len(torg["contacts"]) == 12
assert next(c for c in torg["contacts"] if c["mob_vnum"] == 28975)["topics"] == []
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in torg["stories"])
for prefix, optional in (("a-dracolich-hide-", 1), ("a-secret-rose-delivery-", 2), ("an-obsidian-buckle-", 1)):
    story = next(s for s in torg["stories"] if s["id"].startswith(prefix))
    assert sum(t.get("optional", False) for t in story["steps"]) == optional
rings = next(s for s in torg["stories"] if s["id"] == "evidence-of-a-secret-affair-28932")
assert [t["item_vnums"] for t in rings["steps"] if t["kind"] == "carried_item"] == [[28916], [28938]]
legends = next(s for s in torg["stories"] if s["id"] == "relics-of-the-eight-legends-28964")
assert [t["item_vnums"][0] for t in legends["steps"][:-1]] == list(range(28944, 28952))
assert all(t["count"] == 1 for t in legends["steps"][:-1])
buckle = next(s for s in torg["stories"] if s["id"] == "an-obsidian-buckle-29024")
assert {c["giver_vnum"] for c in buckle["contracts"]} == {29024}
assert [t["item_vnums"] for t in buckle["steps"] if t["kind"] == "carried_item"] == [[28962], [28982]]
solonar = new_mappings["solonar"]
assert solonar["schema_version"] == 3 and solonar["revision"] == 2
assert report["eligible_by_zone"]["306"] == 5 and len(solonar["stories"]) == 15
assert sum(s["category"] == "service" for s in solonar["stories"]) == 10
assert sum(len(s["contracts"]) for s in solonar["stories"]) == 15 and not solonar["exclusions"]
assert len(solonar["contacts"]) == 14
assert all(s["steps"][-1]["contracts"] == s["contracts"] for s in solonar["stories"])
for prefix, optional in (("robes-of-", 5), ("a-piwafwi-", 2), ("forge-mage-", 2), ("prepare-an-ancient-", 2)):
    story = next(s for s in solonar["stories"] if s["id"].startswith(prefix))
    assert sum(t.get("optional", False) for t in story["steps"]) == optional
mage_bane = next(s for s in solonar["stories"] if s["id"] == "forge-mage-bane-30638")
assert [t["item_vnums"] for t in mage_bane["steps"] if t["kind"] == "carried_item"] == [[30659], [30662], [30664]]
piwafwi = next(s for s in solonar["stories"] if s["id"] == "a-piwafwi-of-power-30604")
assert [t["item_vnums"] for t in piwafwi["steps"] if t["kind"] == "carried_item"] == [[30634], [30635], [30636], [30649], [30666]]
scroll = next(s for s in solonar["stories"] if s["id"] == "prepare-an-ancient-scroll-30617")
assert scroll["contracts"][0]["completion_key"].endswith("receive=I:30666,I:30666;disappear=0")
grove_definitions = [d for d in catalog["definitions"] if d["source_area"] == "solonar"]
assert {d["giver_vnum"] for d in grove_definitions if d["daily_exclusion"] == "Unsupported durable offering"} == {30600, 30603, 30604}
assert next(c for c in solonar["contacts"] if c["mob_vnum"] == 30638)["keyword"] == "valin"
family = next(s for s in new_mappings["krimman"]["stories"] if s["title"] == "Release the Haunted Family")
assert len(family["contracts"]) == 1 and len(family["steps"]) == 9
assert all(t["optional"] for t in family["steps"][:-4])
assert [t["item_vnums"] for t in family["steps"][-4:-1]] == [[16452], [16453], [16454]]

ailvio = next(m for m in catalog["story_mappings"] if m["source_area"] == "newbie")
assert len(ailvio["stories"]) == 39 and report["eligible_by_zone"]["292"] == 22
feeding = next(s for s in ailvio["stories"] if s["id"] == "feed-the-ailing-family")
# Every valid native pair is covered, including two fish of the same kind.
import itertools
fishes = [293, 294, 295, 318, 319, 330, 332, 333, 334, 335, 355, 356]
pairs = {tuple(int(g[2:]) for g in c["completion_key"].split(";")[0][5:].split(",")) for c in feeding["contracts"]}
assert len(feeding["contracts"]) == 78 and pairs == set(itertools.combinations_with_replacement(fishes, 2))
assert feeding["steps"][0]["item_vnums"] == fishes and feeding["steps"][0]["count"] == 2
assert all(c["completion_key"].endswith("receive=I:29223;disappear=1") for c in feeding["contracts"])
assert sum(s["category"] == "service" for s in ailvio["stories"]) == 17
cleric = next(s for s in ailvio["stories"] if s["id"] == "request-29238-c2cf98d3f50e")
assert all(t["optional"] for t in cleric["steps"] if t["kind"] == "completion" and t["id"] != "turn-in")
assert next(c for c in ailvio["contacts"] if c["mob_vnum"] == 29233)["topics"] == ["quest"]
assert "item" in next(c for c in ailvio["contacts"] if c["mob_vnum"] == 29237)["topics"]
mansion = next(m for m in catalog["story_mappings"] if m["source_area"] == "braddistock")
assert report["eligible_by_zone"]["13"] == 1 and not mansion["exclusions"]
assert [(s["id"], s["category"]) for s in mansion["stories"]] == [("release-slippers", "service"), ("quiet-the-mansion", "story")]
assert all(t["optional"] for t in mansion["stories"][1]["steps"][:2])

coverage_spec = importlib.util.spec_from_file_location("home_coverage", ROOT / "scripts/zone_story_quest_home_coverage.py")
coverage = importlib.util.module_from_spec(coverage_spec)
coverage_spec.loader.exec_module(coverage)
required = coverage.required_areas(ROOT)
assert len(required["areas"]) == 27 and required["excluded_empty_town_markers"] == ["end"]
assert all((ROOT / r["mapping"]).is_file() for r in required["areas"])
assert all(next(m for m in catalog["story_mappings"] if m["source_area"] == r["source_area"])["schema_version"] in (2, 3) for r in required["areas"])

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
    boundary_dir = pathlib.Path(temporary) / "sidecar-boundary"
    boundary_dir.mkdir()
    boundary_path = boundary_dir / "twin_towers_forest.story.json"
    encoded = json.dumps(mapping).encode("utf-8")
    boundary_path.write_bytes(encoded + b" " * (module.MAX_STORY_MAPPING_BYTES - len(encoded)))
    subprocess.run([str(binary), str(native), str(boundary_path), "boundary"], cwd=ROOT, check=True)
    with boundary_path.open("ab") as output:
        output.write(b" ")
    subprocess.run([str(binary), str(native), str(boundary_path), "oversized"], cwd=ROOT, check=True)
    for version in (1, 2):
        legacy_mapping = copy.deepcopy(mapping)
        legacy_mapping["schema_version"] = version
        for story in legacy_mapping["stories"]:
            for step in story["steps"]:
                step.pop("optional", None)
        if version == 1:
            for key in ("introduction", "orientation", "contacts"):
                legacy_mapping.pop(key)
        legacy_path = pathlib.Path(temporary) / f"schema-{version}.json"
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
    invalid("invalid-optional", lambda m: m["stories"][0]["steps"][0].update(optional=1))
    invalid("old-schema-optional", lambda m: m.update(schema_version=2))
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
    duplicate.write_text(json.dumps(mapping).replace('"schema_version": 3', '"schema_version": 3, "schema_version": 3'), encoding="utf-8")
    subprocess.run([str(binary), str(native), str(duplicate), "invalid"], cwd=ROOT, check=True)

print("builder-authored zone story schema and projection regression passed")
