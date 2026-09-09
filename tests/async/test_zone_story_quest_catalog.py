#!/usr/bin/env python3

import json
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "zone_story_quest_catalog.py"
FIXTURE = ROOT / "tests" / "fixtures" / "zone_story_quest_catalog.json"

result = subprocess.run(
    ["python3", str(SCRIPT), "--catalog", str(FIXTURE), "--json"],
    cwd=ROOT,
    check=True,
    capture_output=True,
    text=True,
)
report = json.loads(result.stdout)
assert report["valid"] is True
assert report["definition_count"] == 3
assert report["eligible_by_zone"] == {"900": 2, "901": 1}
assert report["repeatable_definition_count"] == 3
assert report["diagnostics"] == []

with tempfile.TemporaryDirectory(prefix="duris-zone-story-quest-catalog-") as temporary:
    invalid = pathlib.Path(temporary) / "duplicate.json"
    catalog = json.loads(FIXTURE.read_text())
    catalog["definitions"].append(dict(catalog["definitions"][0]))
    invalid.write_text(json.dumps(catalog))
    invalid_result = subprocess.run(
        ["python3", str(SCRIPT), "--catalog", str(invalid), "--json", "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert invalid_result.returncode == 1
    invalid_report = json.loads(invalid_result.stdout)
    assert invalid_report["valid"] is False
    assert any(item["code"] == "duplicate_definition_id" for item in invalid_report["diagnostics"])

    # Finding 1: Non-object definition must produce clean invalid_definition diagnostic
    non_object_catalog = json.loads(FIXTURE.read_text())
    non_object_catalog["definitions"].append("oops")
    non_object_path = pathlib.Path(temporary) / "non_object.json"
    non_object_path.write_text(json.dumps(non_object_catalog))
    non_object_result = subprocess.run(
        ["python3", str(SCRIPT), "--catalog", str(non_object_path), "--json", "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert non_object_result.returncode == 1
    non_object_report = json.loads(non_object_result.stdout)
    assert non_object_report["valid"] is False
    assert any(item["code"] == "invalid_definition" for item in non_object_report["diagnostics"])

    # Finding 2: Revision mismatch must produce revision_mismatch diagnostic
    mismatch_catalog = json.loads(FIXTURE.read_text())
    mismatch_catalog["definitions"][0]["content_revision"] = 99
    mismatch_path = pathlib.Path(temporary) / "revision_mismatch.json"
    mismatch_path.write_text(json.dumps(mismatch_catalog))
    mismatch_result = subprocess.run(
        ["python3", str(SCRIPT), "--catalog", str(mismatch_path), "--json", "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert mismatch_result.returncode == 1
    mismatch_report = json.loads(mismatch_result.stdout)
    assert mismatch_report["valid"] is False
    assert any(item["code"] == "revision_mismatch" for item in mismatch_report["diagnostics"])

    # Finding 3: giver_vnum must be a positive integer
    for bad_vnum in (0, "abc"):
        bad_vnum_catalog = json.loads(FIXTURE.read_text())
        bad_vnum_catalog["definitions"][0]["giver_vnum"] = bad_vnum
        bad_vnum_path = pathlib.Path(temporary) / "bad_vnum.json"
        bad_vnum_path.write_text(json.dumps(bad_vnum_catalog))
        bad_vnum_result = subprocess.run(
            ["python3", str(SCRIPT), "--catalog", str(bad_vnum_path), "--json", "--check"],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        assert bad_vnum_result.returncode == 1
        bad_vnum_report = json.loads(bad_vnum_result.stdout)
        assert bad_vnum_report["valid"] is False
        assert any(item["code"] == "invalid_giver_vnum" for item in bad_vnum_report["diagnostics"])

    # Finding 4: unhashable definition_id must not crash with TypeError
    unhashable_catalog = json.loads(FIXTURE.read_text())
    unhashable_catalog["definitions"][0]["definition_id"] = ["invalid"]
    unhashable_path = pathlib.Path(temporary) / "unhashable.json"
    unhashable_path.write_text(json.dumps(unhashable_catalog))
    unhashable_result = subprocess.run(
        ["python3", str(SCRIPT), "--catalog", str(unhashable_path), "--json", "--check"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    assert unhashable_result.returncode == 1
    unhashable_report = json.loads(unhashable_result.stdout)
    assert unhashable_report["valid"] is False
    assert any(item["code"] == "invalid_definition_id" for item in unhashable_report["diagnostics"])

print("zone-story quest catalog contract passed")
