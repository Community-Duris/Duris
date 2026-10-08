#!/usr/bin/env python3
"""Exact builder ownership corrections, fallback, and invalid metadata rejection."""
import copy
import json
from pathlib import Path
import sys
import tempfile
from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import zone_story_quest_catalog as catalog_tool
import zone_story_quest_zone_inventory as inventory_tool

with tempfile.TemporaryDirectory(prefix="duris-quest-owners-") as temporary:
    root = Path(temporary)
    for kind in ("zon", "qst"):
        (root / "areas" / kind).mkdir(parents=True)
    (root / "areas/AREA").write_text("original\nreviewed\n")
    (root / "areas/zon/original.zon").write_text("#1\nOriginal~\n100 0 0 0\nS\n$\n")
    (root / "areas/zon/reviewed.zon").write_text("#2\nReviewed~\n200 2 0 0\nS\n$\n")
    (root / "areas/qst/original.qst").write_text("#17\nQ\nG I 42\nR I 43\nD\nQ\nG I 44\nR I 43\nS\n$\n")
    before = catalog_tool.production_catalog(root)
    key = "give=I:42;receive=I:43;disappear=1"
    valid = {"schema_version": 1, "owners": [{"giver_vnum": 17, "completion_key": key,
             "previous_zone_number": 1, "previous_source_area": "original", "zone_number": 2,
             "source_area": "reviewed", "content_revision": 2}]}
    path = root / "areas/quest_owners.json"
    path.write_text(json.dumps(valid))
    after = catalog_tool.production_catalog(root)
    by_id = {d["definition_id"]: d for d in after["definitions"]}
    for original in before["definitions"]:
        current = by_id[original["definition_id"]]
        if bytes.fromhex(original["completion_key"]).decode() == key:
            expected = dict(original, zone_number=2, source_area="reviewed", previous_zone_number=1,
                            repeatable=True, daily_eligible=True, daily_exclusion="")
            assert current == expected and not original["repeatable"]
        else:
            assert current == original
    assert after["zones"] == before["zones"] and catalog_tool.report_for(after)["valid"]
    whole_numbers = copy.deepcopy(valid)
    whole_numbers["schema_version"] = 1.0
    for field in ("giver_vnum", "previous_zone_number", "zone_number", "content_revision"):
        whole_numbers["owners"][0][field] = float(whole_numbers["owners"][0][field])
    path.write_text(json.dumps(whole_numbers))
    assert catalog_tool.production_catalog(root) == after
    invalid = []
    for field, values in {
        "giver_vnum": [True, 0, 17.5, "17", 2**31],
        "zone_number": [1, 3], "previous_zone_number": [2],
        "source_area": ["wrong", "reviewed\x00"], "previous_source_area": ["wrong"],
        "completion_key": [key + "-unknown", "", "x" * 1025], "content_revision": [1, True]
    }.items():
        for value in values:
            candidate = copy.deepcopy(valid)
            candidate["owners"][0][field] = value
            invalid.append(json.dumps(candidate))
    candidate = copy.deepcopy(valid)
    candidate["owners"].append(dict(candidate["owners"][0]))
    invalid.append(json.dumps(candidate))
    invalid += ["{}", "null", json.dumps(valid).replace('"schema_version": 1', '"schema_version": 1, "schema_version": 1'),
                json.dumps(valid).replace('"giver_vnum": 17', '"giver_vnum": 17, "extra": 1'), " " * (64 * 1024 + 1)]
    for raw in invalid:
        path.write_text(raw)
        try:
            catalog_tool.production_catalog(root)
        except (ValueError, KeyError, TypeError):
            pass
        else:
            raise AssertionError("invalid owner metadata silently fell back")
    path.unlink()
    assert catalog_tool.production_catalog(root) == before
    path.mkdir()
    try:
        catalog_tool.production_catalog(root)
    except ValueError:
        pass
    else:
        raise AssertionError("present directory treated as absent metadata")

catalog = catalog_tool.production_catalog(ROOT)
corrections = [d for d in catalog["definitions"] if "previous_zone_number" in d]
assert len(corrections) == 2 and {d["giver_vnum"] for d in corrections} == {87860, 87869}
assert all(d["zone_number"] == 878 and d["previous_zone_number"] == 879 and d["source_area"] == "church" for d in corrections)
rows, _, _ = inventory_tool.inventory(ROOT)
church = next(r for r in rows if r["zone"]["source_area"] == "church")
kelek = next(r for r in rows if r["zone"]["source_area"] == "kelek")
assert len(church["requests"]) == 4 and len(kelek["requests"]) == 1
assert [b["body"][0] for b in church["dialogue"]] == ["paladin paladins~", "hello hi~", "initiation~"]
assert [b["body"][0] for b in kelek["dialogue"]] == ["mithril~"]
assert catalog_tool.report_for(catalog)["eligible_by_zone"]["878"] == 4
capture = " ".join(extract_function("quest.c", "static bool capture_quest_offering_continuation(").split())
assert "original_definition ? original_definition : zone_story_quest_production::definition_id_for(completion)" in capture
assert "zone_story_quest_production::runtime_catalog().definitions" in capture
assert "if (definition.definition_id == *definition_id) { zone_number = definition.zone_number; break; }" in capture
assert "zone_number < 0" in capture
runtime = (ROOT / "src/world/zone_story_quest_runtime.c").read_text()
for name in ("record_authoritative_completion", "record_legacy_completion"):
    body = runtime.split("bool " + name + "(", 1)[1].split("\n}", 1)[0]
    assert "transaction_id.empty() && !economic_gameplay_authority::active()" in body
print("exact reviewed quest ownership, stable identities, target reset eligibility, dialogue and fail-closed metadata passed")
