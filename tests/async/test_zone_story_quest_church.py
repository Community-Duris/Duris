#!/usr/bin/env python3
"""Church's four exact returns, proof sources, conditional access and narrative limits."""
import collections
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import zone_story_quest_catalog as catalog_tool
import zone_story_quest_zone_inventory as inventory_tool

def bodies(kind):
    source = (ROOT / "areas" / kind / f"church.{kind}").read_text()
    return {int(m[1]): m[2] for m in re.finditer(r"^#(\d+)\s*\n([\s\S]*?)(?=^#\d+\s*$|\Z)", source, re.M)}

evidence = inventory_tool.area_evidence(ROOT, "church")
catalog = catalog_tool.production_catalog(ROOT)
mapping = next(m for m in catalog["story_mappings"] if m["source_area"] == "church")
assert (mapping["schema_version"], mapping["revision"], mapping["coverage"]) == (3, 1, "complete")
assert len(mapping["stories"]) == 4 and len(mapping["contacts"]) == 12
expected = {14: (87821, [87846], [("E", 100000)]), 24: (87847, [87873], [("I", 87874)]),
            47: (87860, [87869], [("C", 250000), ("E", 100000)]),
            62: (87869, [87870, 87871, 87872], [("I", 87873)])}
assert len(evidence["requests"]) == 4
for request in evidence["requests"]:
    block = request["block"]
    giver, inputs, reward = expected[block["line"]]
    assert block["source"] == "areas/qst/church.qst" and block["kind"] == "Q"
    assert (block["giver_vnum"], block["give"], block["receive"], block["disappear"]) == (giver, [("I", v) for v in inputs], reward, False)
    assert request["definition"]["zone_number"] == 878 and not request["definition"]["prerequisites"]
    story = next(s for s in mapping["stories"] if s["contracts"] == [block["binding"]])
    assert story["category"] == "request" and story["steps"][-1]["contracts"] == story["contracts"]
    material = [t for t in story["steps"] if t["kind"] == "carried_item"]
    assert [(t["item_vnums"], t["count"], t["optional"]) for t in material] == [([v], 1, True) for v in inputs]
    assert all(t.get("optional") for t in story["steps"][:-1])
assert [b["body"][0] for b in evidence["dialogue"]] == ["paladin paladins~", "hello hi~", "initiation~"]
assert evidence["candidate_link_items"] == [87873]
assert not evidence["special_assignments"]
rooms, objects, mobs = bodies("wld"), bodies("obj"), bodies("mob")
assert (len(rooms), len(objects), len(mobs)) == (33, 55, 55)
assert set(rooms) == set(range(87820, 87853))
assert len((ROOT / "areas/qst/church.qst").read_text().splitlines()) == 71
assert len((ROOT / "areas/zon/church.zon").read_text().splitlines()) == 250
resets = evidence["reset_commands"]
assert len(resets) == 161 and collections.Counter(r["command"] for r in resets) == {"D": 26, "O": 4, "P": 1, "M": 62, "E": 52, "F": 5, "G": 11}
parent, stock = None, []
for reset in resets:
    args, command = reset["arguments"], reset["command"]
    if command in {"M", "F"}:
        parent = args[1], args[3]
    if command in {"G", "E"}:
        stock.append((command, args[1], parent, reset["line"]))
assert {("E", 87846, (87848, 87847), 232), ("G", 87869, (87821, 87840), 201),
        ("G", 87870, (87866, 87852), 244), ("G", 87871, (87867, 87852), 246),
        ("G", 87872, (87868, 87852), 248), ("G", 87868, (87840, 87849), 236),
        ("G", 87824, (87820, 87826), 160)} <= set(stock)
assert not any(r["command"] in {"G", "E", "O", "P"} and r["arguments"][1] == 87873 for r in resets)
assert "Relxis" in objects[87873] and "rescue" in objects[87873].lower()
edges = [(v, int(m[1]), int(m[4]), int(m[5]), int(m[6])) for v, body in rooms.items()
         for m in re.finditer(r"\bD(\d+)\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)", body, re.S)]
assert len(edges) == 69 and {(87842, 4, 7, -2, 87848), (87848, 5, 3, -2, 87842)} <= set(edges)
assert {(v, dest) for v, d, raw, key, dest in edges if dest not in rooms} == {(87847, 527483)}
assert "door blasphemy~" in rooms[87842] and "door blasphemy~" in rooms[87848]
states = {(r["arguments"][1], r["arguments"][2]): r["arguments"][3] for r in resets if r["command"] == "D"}
def reach(password=False, key=False):
    seen = {87847}
    while True:
        before = len(seen)
        for v, direction, raw, lock, target in edges:
            if v not in seen or target not in rooms:
                continue
            if states.get((v, direction), 0) & 3 in (2, 3):
                if (lock == -2 and not password) or (lock == 87868 and not key):
                    continue
            seen.add(target)
        if len(seen) == before:
            return seen
assert [len(reach(p, k)) for p, k in [(False, False), (True, False), (True, True)]] == [28, 30, 33]
assert len([e for e in edges if e[3] == 87868 and e[2] == 2]) == 6
case_values = objects[87823].split("~", 4)[4].splitlines()
assert any(re.fullmatch(r"\s*180\s+29\s+87824\s+180\s+0\s+0\s+0\s+0\s*", line) for line in case_values)
assert any(r["command"] == "P" and r["line"] == 117 and r["arguments"][1] == 87825 for r in resets)
speech = (ROOT / "src/cmd/actcomm.c").read_text().split("void check_magic_doors(", 1)[1].split("void do_petition(", 1)[0]
assert "EXIT(ch, door)->key == -2" in speech and "REMOVE_BIT(back->exit_info, EX_LOCKED)" in speech and "EX_CLOSED" not in speech
assert "check_magic_doors(ch, argument + i);" in (ROOT / "src/cmd/actcomm.c").read_text()
assert re.search(r"state\s*&=\s*3;", (ROOT / "src/world/db.c").read_text())
print("Church full selected source contracts, exact proof stock, optional narratives and conditional access passed")
