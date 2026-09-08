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

print("zone-story quest catalog contract passed")
