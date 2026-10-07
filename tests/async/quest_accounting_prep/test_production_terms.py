#!/usr/bin/env python3
"""Source and fixture checks, explicitly separate from native journeys."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re
import tempfile

from case_data import CASES, ROOT, CATALOG_PIN, blocks, catalog, facts, prototype
from prepare_fixture import prepare


def check(case_id, variants=False):
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
        if case_id == "QP06":
            import run_quest_reward_ack_crash as crash
            terms = crash.prepare_quest_fixture(Path(directory), case_id)
            assert terms == dict(offering_vnums=(29262, 29263, 29264),
                                 offering_names=("ear", "scalp", "toe"), giver="kord",
                                 reward_vnum=29237, reward_name="dagger", coin_reward=3000)
            assert crash.fixture_xp_mask(Path(directory), case_id) == 4
            generated = json.loads((Path(directory) / "quest-prep-provenance.json").read_text())
        else:
            generated = prepare(case_id, Path(directory))
        mini = Path(directory) / "areas_mini"
        assert (mini / "mini.qst").read_text().count("\nQ\n") == len(selected)
        for row in generated["prototype_records"]:
            _, body, _ = prototype(row["kind"], row["vnum"])
            assert body.rstrip() in (mini / f"mini.{row['kind']}").read_text()
        for kind in ("mob", "obj"):
            indexed = [int(value) for value in re.findall(r"(?m)^#(\d+)\s*$", (mini / f"mini.{kind}").read_text())]
            assert indexed == sorted(set(indexed)), "native binary lookup needs ordered unique prototype indices"
        try:
            prepare(case_id, Path(directory))
        except ValueError:
            pass
        else:
            raise AssertionError("fixture overwrote existing output")
    if variants:
        with tempfile.TemporaryDirectory(prefix=f"quest-prep-{case_id}-native-world-") as directory:
            generated = prepare(case_id, Path(directory), layout="world")
            assert generated["fixture_layout"] == "world"
            assert (Path(directory) / "areas").resolve() == (ROOT / "areas").resolve()
            assert not (Path(directory) / "areas_mini").exists()
            assert generated["reset_families"] == evidence["reset_families"]
            assert generated["commands"] == []
        for contract in selected:
            reward = next(number for kind, number in contract["receive"] if kind == "I")
            required = Counter(number for kind, number in contract["give"] if kind == "I")
            supplies = ("exact", "shortage", "spares") + (("wrong-kind",) if case_id in ("QP01", "QP05") else ())
            for supply in supplies:
                with tempfile.TemporaryDirectory(prefix=f"quest-prep-{case_id}-{supply}-") as directory:
                    generated = prepare(case_id, Path(directory), reward_vnum=reward, supply=supply)
                    mini = Path(directory) / "areas_mini"
                    assert (mini / "mini.qst").read_text() == (f"#{case['giver']}\n" +
                        "".join(block["text"] for block in selected) + "S\n$~\n")
                    declarations = [line.split() for line in (mini / "mini.zon").read_text().splitlines() if line.startswith("O ")]
                    actual = Counter(int(line[2]) for line in declarations)
                    assert actual == Counter({int(k): v for k, v in generated["fixture_supplied_counts"].items()})
                    assert all(int(line[3]) == actual[int(line[2])] for line in declarations)
                    assert generated["selected_contract"]["give"] == contract["give"]
                    assert generated["reset_families"] == evidence["reset_families"]
                    if supply == "exact":
                        assert actual == required
                    elif supply == "spares":
                        assert actual == required + Counter({k: 1 for k in required})
                    else:
                        assert not actual >= required, "negative supply accidentally satisfies selected contract"
                    if case_id == "QP02":
                        assert (mini / "mini.qst").read_text().count("\nQ\n") == 4
        # Reject invalid recipe selection before creating any output.
        with tempfile.TemporaryDirectory(prefix="quest-prep-invalid-") as directory:
            output = Path(directory) / "must-not-exist"
            try:
                prepare(case_id, output, reward_vnum=-1, supply="exact")
            except ValueError:
                assert not output.exists()
            else:
                raise AssertionError("invalid contract accepted")
    return dict(case=case_id, result="production terms and isolated fixture passed; no native journey")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=list(CASES))
    parser.add_argument("--variants", action="store_true", help="check exact, shortage, spares and supported wrong-kind supplies")
    args = parser.parse_args()
    print(json.dumps([check(k, args.variants) for k in ([args.case] if args.case else CASES)], indent=2))
