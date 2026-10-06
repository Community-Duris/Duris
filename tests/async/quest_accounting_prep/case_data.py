"""Production fixture inputs; no runtime, SQL or accounting authority is modeled."""

from pathlib import Path
import hashlib
import json
import re
import sys
from functools import lru_cache

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts"))
sys.path.insert(0, str(ROOT / "tests/async"))
import zone_story_quest_catalog as catalog

ACCOUNTING_PIN = "17c033d69316b21da8598791fc95cae79baa8dc2"
RESEARCH_PIN = "55905eac1906cf59405764407f9d22497cccfff3"
CATALOG_PIN = "04d687493b02ed27a3b32d070a0a71e9d10c1643406d5fb95fe6b1538b4bb314"
CASES = {
    "QP01": dict(area="tikitt", giver=44101, room=44313, rewards=[44192],
                 give=[("I", 43703), ("I", 43752), ("I", 43753), ("I", 44164)],
                 receive=[("I", 44192)], disappear=False,
                 dossier="docs/design/zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md"),
    "QP02": dict(area="goblincave", giver=19005, room=19008,
                 rewards=[19007, 19008, 19009, 19010],
                 contracts=[([("I", 19006), ("C", 1000)], [("I", 19007)]),
                            ([("I", 19006)] * 2 + [("C", 2000)], [("I", 19008)]),
                            ([("I", 19006)] * 3, [("I", 19009)]),
                            ([("I", 19006)] * 4 + [("C", 10000)], [("I", 19010)])],
                 disappear=False,
                 dossier="docs/design/zone-stories/GAGGA_JOBO_CAVE_SYSTEM.md"),
    "QP03": dict(area="pineholl", giver=16006, room=16077, rewards=[16015, 16075],
                 give=[("I", 16013), ("I", 16014), ("I", 16080)],
                 receive=[("I", 16015), ("I", 16075)], disappear=True,
                 dossier="docs/design/zone-stories/PINE_HOLLOW.md"),
    "QP04": dict(area="quietus", giver=1709, room=1734, dynamic=True,
                 level=11, fee_copper=220, rewards=[],
                 dossier="docs/design/zone-stories/QUIETUS_QUAY.md"),
    "QP05": dict(area="pineholl", giver=16080, room=16110, rewards=[16048, 16050],
                 contracts=[([("I", 16019)] * 3, [("I", 16048)]),
                            ([("I", 16021)] * 2, [("I", 16050)])],
                 disappear=False,
                 dossier="docs/design/zone-stories/PINE_HOLLOW.md"),
    "QP06": dict(area="newbie", giver=29257, room=29217, rewards=[29237],
                 give=[("I", 29262), ("I", 29263), ("I", 29264)],
                 receive=[("E", 2500), ("C", 3000), ("I", 29237)], disappear=False,
                 dossier="docs/reference/zone-story-audits/newbie.md"),
    "QP07": dict(area="quietus", giver=1709, room=1734, dynamic=True,
                 level=11, map_fee_copper=110, rewards=[],
                 dossier="docs/design/zone-stories/QUIETUS_QUAY.md"),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path, vnum):
    """Return a complete native # record and one-based source line."""
    text = path.read_text(encoding="utf-8", errors="strict")
    match = re.search(rf"(?m)^#{vnum}\s*$", text)
    if not match:
        raise ValueError(f"missing #{vnum}: {path}")
    end = re.search(r"(?m)^(?:#-?\d+\s*|\$~?)$", text[match.end():])
    if not end:
        # Production per-area records may end at EOF; the area compiler appends
        # the combined world's terminal marker. Preserve that final record too.
        return text[match.start():], text[:match.start()].count("\n") + 1
    return text[match.start():match.end() + end.start()], text[:match.start()].count("\n") + 1


def blocks(case_id):
    case = CASES[case_id]
    if case.get("dynamic"):
        return []
    path = ROOT / f"areas/qst/{case['area']}.qst"
    text, first_line = record(path, case["giver"])
    markers = list(re.finditer(r"(?m)^(?:Q|QA|M|MA|S)\s*$", text))
    found = []
    for index, marker in enumerate(markers):
        if marker.group().strip() not in ("Q", "QA"):
            continue
        end = markers[index + 1].start() if index + 1 < len(markers) else len(text)
        body = text[marker.start():end]
        give, receive = [], []
        for group, kind, number in re.findall(r"(?m)^([GR])\s+([ITCSE])\s+(-?\d+)\s*$", body):
            (give if group == "G" else receive).append((kind, int(number)))
        if any(kind == "I" and number in case["rewards"] for kind, number in receive):
            found.append(dict(text=body, give=give, receive=receive,
                              disappear=bool(re.search(r"(?m)^D\s*$", body)),
                              line=first_line + text[:marker.start()].count("\n")))
    return found


@lru_cache(maxsize=2)
def prototype_index(kind):
    index = {}
    for raw in (ROOT / "areas/AREA").read_text().splitlines():
        if not raw.strip() or raw.lstrip().startswith("*"):
            continue
        area = raw.split()[0]
        path = ROOT / f"areas/{kind}/{area}.{kind}"
        if not path.is_file():
            continue
        for match in re.finditer(r"(?m)^#(-?\d+)\s*$", path.read_text(encoding="utf-8")):
            index.setdefault(int(match[1]), []).append(path)
    return index


def prototype(kind, vnum):
    """Resolve against production AREA; selected duplicates are refused."""
    matches = prototype_index(kind).get(vnum, [])
    if len(matches) != 1:
        raise ValueError(f"expected one production {kind}#{vnum}, found {len(matches)}")
    body, line = record(matches[0], vnum)
    return matches[0], body, line


def reset_family(case_id):
    case = CASES[case_id]
    path = ROOT / f"areas/zon/{case['area']}.zon"
    lines = path.read_text().splitlines()
    families = []
    for index, line in enumerate(lines):
        parts = line.split()
        if len(parts) < 6 or parts[0] != "M" or int(parts[2]) != case["giver"]:
            continue
        family = [(index + 1, line)]
        for following in range(index + 1, len(lines)):
            parts = lines[following].split()
            if parts and parts[0] in ("M", "S"):
                break
            if parts and parts[0] in ("G", "E", "P"):
                family.append((following + 1, lines[following]))
        families.append(family)
    return families


def facts(case_id):
    case = CASES[case_id]
    selected = blocks(case_id)
    paths = [ROOT / "areas/AREA", ROOT / f"areas/zon/{case['area']}.zon",
             ROOT / "src/world/db.c", ROOT / "src/world/quest.c",
             ROOT / "src/specs/specs.assign.c", ROOT / "src/cmd/interp.c"]
    if case.get("dynamic"):
        paths += [ROOT / "src/specs/specs.world_quest.c", ROOT / "src/world/world_quest.c",
                  ROOT / "src/world/world_quest_policy.c", ROOT / "src/economy/shop.c",
                  ROOT / "src/core/utility.c", ROOT / "areas/shp/quietus.shp"]
    else:
        paths.append(ROOT / f"areas/qst/{case['area']}.qst")
    source_areas = [case["area"]] + (["tikit"] if case_id == "QP01" else [])
    needed = {number for block in selected for group in ("give", "receive")
              for kind, number in block[group] if kind == "I"}
    sources = []
    for area in source_areas:
        path = ROOT / f"areas/zon/{area}.zon"
        if path not in paths:
            paths.append(path)
        previous_mobile = None
        for line_number, raw in enumerate(path.read_text().splitlines(), 1):
            words = raw.split()
            if len(words) < 6:
                continue
            if words[0] == "M":
                previous_mobile = dict(line=line_number, raw=raw)
            if words[0] in ("G", "E", "O", "P") and int(words[2]) in needed:
                sources.append(dict(path=str(path.relative_to(ROOT)).replace("\\", "/"),
                                    line=line_number, raw=raw,
                                    preceding_mobile=previous_mobile if words[0] in ("G", "E") else None))
    prototype_records = []
    for kind, vnums in (("mob", [case["giver"]]), ("obj", sorted(needed))):
        for vnum in vnums:
            path, _, line = prototype(kind, vnum)
            if path not in paths:
                paths.append(path)
            prototype_records.append(dict(kind=kind, vnum=vnum, line=line,
                path=str(path.relative_to(ROOT)).replace("\\", "/")))
    return dict(case_id=case_id, accounting_pin=ACCOUNTING_PIN, research_pin=RESEARCH_PIN,
                candidate_scope="working-tree source hashes below; base pin is not an integrated-candidate claim",
                config=case, blocks=selected, reset_families=reset_family(case_id),
                prototype_records=prototype_records, ingredient_reward_reset_declarations=sources,
                source_hashes={str(p.relative_to(ROOT)).replace("\\", "/"): digest(p) for p in paths},
                evidence="source verification only; fixture/component is not a native journey")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    rendered = json.dumps([facts(key) for key in CASES], indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
