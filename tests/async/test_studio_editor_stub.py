#!/usr/bin/env python3
"""Private-editor handoff fixture uses the actual C++ server contract parser."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-editor-stub-") as directory:
    temporary = Path(directory)
    validator = temporary / "validator"
    subprocess.run(["g++", "-std=c++20", "-O1", "-I"+str(ROOT / "src"),
                    str(ROOT / "scripts/studio_ability_validator.cpp"),
                    str(ROOT / "src/item/studio_ability_model.c"), "-lcjson", "-o", str(validator)], check=True)
    source = ROOT / "docs/examples/studio-item-abilities.json"
    output, trg = temporary / "roundtrip.json", temporary / "roundtrip.trg"
    command = [sys.executable, str(ROOT / "scripts/studio_ability_editor_stub.py"), str(source),
               str(output), "--validator", str(validator), "--emit-trg", str(trg)]
    subprocess.run(command, check=True)
    assert json.loads(output.read_text()) == json.loads(source.read_text())
    assert trg.read_text().count("#22802 O")==1
    assert "T HIT\nitemability 1001\n~" in trg.read_text()
    assert "T CMD use\nitemability 1002\n~" in trg.read_text()
    previous = output.read_bytes()
    invalid = temporary / "invalid.json"
    invalid.write_text('{"schemaVersion":2,"abilities":[]}')
    command[2] = str(invalid)
    result = subprocess.run(command, capture_output=True)
    assert result.returncode != 0 and output.read_bytes() == previous
    print("Studio editor stub: canonical round-trip, .trg emission and invalid-input preservation passed")
