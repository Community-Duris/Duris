#!/usr/bin/env python3
"""Strict server catalog contract and editor serialization fixture under sanitizers."""
import copy
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "item/studio_ability_model.h"
#include <cassert>
#include <iostream>
#include <iterator>
int main() {
    const std::string input((std::istreambuf_iterator<char>(std::cin)),{});
    studio_ability_catalog catalog;
    studio_ability_definition sentinel; sentinel.id=99; catalog[99]=sentinel;
    std::string error;
    if(!parse_studio_ability_catalog(input,catalog,error)) {
        assert(catalog.size()==1 && catalog.at(99)==sentinel);
        std::cerr << error << '\n'; return 1;
    }
    assert(!catalog.contains(99));
    if(catalog.size()==2) {
        assert(catalog.at(1001).cost==2000 && catalog.at(1002).cost==5000);
        assert(catalog.at(1001).mana==catalog.at(1002).mana);
        assert(catalog.at(1002).effects[0].call==studio_ability_call::wand);
    }
}
'''

with tempfile.TemporaryDirectory(prefix="duris-studio-model-") as directory:
    source = Path(directory) / "harness.cpp"; binary = Path(directory) / "harness"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-O1", "-g", "-fsanitize=address,undefined", "-no-pie",
                    "-I"+str(ROOT / "src"), str(source), str(ROOT / "src/item/studio_ability_model.c"),
                    "-lcjson", "-o", str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1")
    sample = json.loads((ROOT / "docs/examples/studio-item-abilities.json").read_text())
    for text in (json.dumps(sample), json.dumps(sample, sort_keys=True),
                 '{"schemaVersion":1,"abilities":[]}'):
        subprocess.run([str(binary)], input=text, text=True, check=True, env=env)
    failures = []
    for field, value, diagnostic in (
        ("mode", "active", ".mode"), ("source", "carried", ".source"),
        ("trigger", "damaged", ".trigger"), ("windupPulses", 0, ".windupPulses"),
        ("progressPulses", 8, ".progressPulses"), ("cost", 20001, ".cost"),
        ("cost", -1, ".cost"), ("cost", .5, ".cost"), ("cost", 1e309, "JSON"),
        ("mana", None, ".cost"), ("cooldownMs", 3600001, ".cooldownMs"),
        ("concurrency", "unlimited", ".concurrency"), ("ownership", "replace-native", ".ownership"),
        ("effects", [], ".effects"), ("id", 0, ".id"), ("revision", 0, ".revision"),
        ("surprise", 1, "unknown")):
        changed = copy.deepcopy(sample); changed["abilities"][0][field] = value
        failures.append((json.dumps(changed), diagnostic))
    for field, value in (("spell", 2000), ("power", 61), ("target", "world"), ("call", "native"), ("type", "summon")):
        changed = copy.deepcopy(sample); changed["abilities"][0]["effects"][0][field] = value
        failures.append((json.dumps(changed), ".effects[0]."+field))
    for changed, diagnostic in ((dict(sample, schemaVersion=2), "schemaVersion"),
        (dict(sample, abilities=[sample["abilities"][0]]*2), "duplicate")):
        failures.append((json.dumps(changed), diagnostic))
    changed = copy.deepcopy(sample); changed["abilities"][1]["mana"]["capacity"] += 1
    failures.append((json.dumps(changed), "conflicting"))
    changed = copy.deepcopy(sample); changed["abilities"][0]["presentation"]["begin"] = "$n attacks"
    failures.append((json.dumps(changed), "substitution"))
    failures += [('{"schemaVersion":1,"schemaVersion":1,"abilities":[]}', "duplicate"),
                 (json.dumps(sample)+" trailing", "JSON"), (" "*1048577, "MiB")]
    for text, diagnostic in failures:
        result = subprocess.run([str(binary)], input=text, text=True, capture_output=True, env=env)
        assert result.returncode == 1 and diagnostic in result.stderr, (diagnostic, result.returncode, result.stderr)
    print(f"Studio catalog: editor round-trip and {len(failures)} strict rejection cases passed")
