#!/usr/bin/env python3
"""Source and fixture checks, explicitly separate from native journeys."""

import argparse
from collections import Counter
import json
from pathlib import Path
import tempfile

from case_data import CASES, ROOT, CATALOG_PIN, blocks, catalog, facts, prototype
from prepare_fixture import prepare


def check(case_id):
    case = CASES[case_id]
    catalog_data = catalog.production_catalog(ROOT)
    assert not catalog.validate_catalog(catalog_data)
    assert len(catalog_data["definitions"]) == 2668
    assert catalog_data["source"]["fingerprint_sha256"] == CATALOG_PIN, "reverify changed production catalog"
    evidence = facts(case_id)
    assert any(int(family[0][1].split()[4]) == case["room"] for family in evidence["reset_families"])
    if case.get("dynamic"):
        assert not blocks(case_id)
        assignments = (ROOT / "src/specs/specs.assign.c").read_text()
        assert "mob_index[real_mobile0(1709)].func.mob = world_quest" in assignments
        assert "bartender_random_world_quests" in catalog_data["source"]["excludes"]
        with tempfile.TemporaryDirectory(prefix=f"quest-prep-{case_id}-world-") as directory:
            metadata = prepare(case_id, Path(directory))
            assert not (Path(directory) / "areas_mini").exists()
            assert (Path(directory) / "areas").resolve() == (ROOT / "areas").resolve()
            assert Path(metadata["runtime_paths"]["critical_journal"]).is_dir()
            assert metadata["commands"][0].startswith("ask one-eyed ")
        return dict(case=case_id, result="source and isolated full-world run-root passed; no native journey")
    selected = blocks(case_id)
    expected = case.get("contracts", [(case.get("give"), case.get("receive"))])
    assert len(selected) == len(expected)
    for block, (give, receive) in zip(selected, expected):
        assert Counter(block["give"]) == Counter(give), (case_id, block["give"])
        assert Counter(block["receive"]) == Counter(receive), (case_id, block["receive"])
        assert block["disappear"] == case["disappear"]
        key = catalog.completion_key(block["give"], block["receive"], block["disappear"])
        assert any(d["giver_vnum"] == case["giver"] and d["source_area"] == case["area"]
                   and bytes.fromhex(d["completion_key"]).decode() == key for d in catalog_data["definitions"])
    if case_id == "QP01":
        names = [prototype("obj", vnum)[1].splitlines()[2] for vnum in (43703, 43752, 43753)]
        assert len(set(names)) == 1, "reverify same-looking sapphire case"
        assert any(row["raw"].startswith("P 1 44164 1 44165 ") for row in
                   evidence["ingredient_reward_reset_declarations"])
    if case_id == "QP05":
        huge = [row for row in evidence["ingredient_reward_reset_declarations"]
                if row["raw"].startswith("G 1 16021 ")]
        assert len(huge) == 1 and huge[0]["raw"].startswith("G 1 16021 1 ")
        assert huge[0]["preceding_mobile"]["raw"].startswith("M 0 16021 1 16040 ")
    with tempfile.TemporaryDirectory(prefix=f"quest-prep-{case_id}-") as directory:
        generated = prepare(case_id, Path(directory))
        mini = Path(directory) / "areas_mini"
        assert (mini / "mini.qst").read_text().count("\nQ\n") == len(selected)
        for row in generated["prototype_records"]:
            _, body, _ = prototype(row["kind"], row["vnum"])
            assert body.rstrip() in (mini / f"mini.{row['kind']}").read_text()
        try:
            prepare(case_id, Path(directory))
        except ValueError:
            pass
        else:
            raise AssertionError("fixture overwrote existing output")
    return dict(case=case_id, result="production terms and isolated fixture passed; no native journey")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=list(CASES))
    args = parser.parse_args()
    print(json.dumps([check(k) for k in ([args.case] if args.case else CASES)], indent=2))
