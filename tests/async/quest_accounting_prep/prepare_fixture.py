#!/usr/bin/env python3
"""Prepare isolated real-term mini data; does not boot or seed any authority."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re

from case_data import CASES, ROOT, blocks, digest, facts, prototype, record, reset_family
import test_flatfile_combat_journey as journey


def prepare(case_id, output, *, reward_vnum=None, supply="spares", layout=None):
    if layout not in (None, "mini", "world"):
        raise ValueError("unknown fixture layout")
    if CASES[case_id].get("dynamic") and layout == "mini":
        raise ValueError("dynamic producer requires production world layout")
    if supply not in ("spares", "exact", "shortage", "wrong-kind"):
        raise ValueError("unknown fixture supply")
    if CASES[case_id].get("dynamic") and (reward_vnum is not None or supply != "spares"):
        raise ValueError("dynamic producers have no static recipe supply")
    selected_contract = None
    required = Counter()
    if not CASES[case_id].get("dynamic"):
        choices = blocks(case_id)
        if reward_vnum is not None:
            choices = [term for term in choices if ("I", reward_vnum) in term["receive"]]
            if len(choices) != 1:
                raise ValueError("reward VNUM must select one actual production contract")
            selected_contract = choices[0]
        elif supply != "spares" and len(choices) != 1:
            raise ValueError("select --reward-vnum for competing recipes")
        for term in choices:
            required |= Counter(number for kind, number in term["give"] if kind == "I")
    counts = required.copy()
    if supply == "spares":
        counts.update({vnum: 1 for vnum in required})
    elif supply == "shortage":
        counts[max(counts)] -= 1
    elif supply == "wrong-kind":
        if case_id == "QP01":
            counts = Counter({43703: 3, 44164: 1})
        elif case_id == "QP05":
            original = next(iter(required))
            counts[original] -= 1
            counts[16021 if original == 16019 else 16019] += 1
        else:
            raise ValueError("wrong-kind fixture is defined only for QP01 and QP05")
    counts = +counts
    case = CASES[case_id]
    if layout == "world" and (reward_vnum is not None or supply != "spares"):
        raise ValueError("production world layout does not modify ingredient supply")
    output = Path(output).resolve()
    if output.exists() and any(output.iterdir()):
        raise ValueError("fixture output must be a new or empty directory")
    output.mkdir(parents=True, exist_ok=True)
    if case.get("dynamic") or layout == "world":
        if reward_vnum is not None or supply != "spares":
            raise ValueError("production world layout does not modify ingredient supply")
        # Reuse the maintained world's isolated run-root, journals, library and
        # local TLS setup. Do not substitute a static QST record for this producer.
        import run_world_quest_dual_backend as world_journey
        player_journal, critical_journal, boot_output = world_journey.setup_run_root(output)
        metadata = facts(case_id)
        metadata["fixture_scope"] = (
            "Full production areas link, isolated copied lib/journals/local TLS from "
            "run_world_quest_dual_backend.setup_run_root; no server, database, player, "
            "accounting baseline, task or receipt is seeded. make world and the actual "
            "SQL candidate are required at primary qualification."
        )
        metadata["runtime_paths"] = dict(player_journal=str(player_journal),
            critical_journal=str(critical_journal), boot_output=str(boot_output))
        metadata["commands"] = (["ask one-eyed quest"] if case_id == "QP04" else
            ["ask one-eyed map", "ask one-eyed abandon confirm"] if case_id == "QP07" else [])
        metadata["fixture_layout"] = "world"
        metadata["integration_hooks"] = ([
            "Disposable SQL authority and authentic accounting epoch from primary lifecycle owner",
            "Configured mortal level/quoted fees; maintained Chaos creator uses level56, not level11",
            "Real arrival at room1734/giver1709; maintained driver defaults to Woodseer16633/16553",
            "Fault seam after original debit and before callback; no-target or replacement-task control",
            "Native wallet/task/history/receipt/root captures and two cold boots on integrated candidate",
        ] if case.get("dynamic") else [
            "Authentic active SQL fresh-world full boot/reset and original quest recipient birth/source",
            "Acquire real production reset stock and transfer through genuine native GIVE; no synthetic O",
            "Actual original/reference UID, full forest/cash, receipt, cold adoption and historical ACK/pair observations",
            "Primary journey must arrive at actual case room/giver, preserving production caps and quest family",
        ])
        (output / "quest-prep-provenance.json").write_text(json.dumps(metadata, indent=2) + "\n")
        return metadata
    selected = blocks(case_id)
    if not selected:
        raise ValueError("no production Q blocks found")
    journey.make_fixture(output)
    mini = output / "areas_mini"
    metadata = facts(case_id)
    metadata["fixture_scope"] = (
        "Production mobile/object/Q bytes; selected Q subset except complete QP02 family. "
        "Recipient relocated to synthetic room22800 and ingredients supplied by synthetic O. "
        "No full-world source, accounting baseline, birth binding, UID or receipt is supplied."
    )
    metadata["fixture_layout"] = "mini"
    metadata["prototype_records"] = []
    stock = []
    for family in reset_family(case_id):
        if int(family[0][1].split()[4]) == case["room"]:
            stock = [line for _, line in family[1:] if line.split()[0] in ("G", "E")]
            break
    item_vnums = {number for block in selected for group in ("give", "receive")
                  for kind, number in block[group] if kind == "I"}
    item_vnums.update(int(line.split()[2]) for line in stock)
    for kind, vnums in (("mob", [case["giver"]]), ("obj", sorted(item_vnums))):
        path = mini / f"mini.{kind}"
        text = path.read_text()
        for vnum in vnums:
            origin, body, line = prototype(kind, vnum)
            if re.search(rf"(?m)^#{vnum}\s*$", text):
                previous, _ = record(path, vnum)
                text = text.replace(previous, body.rstrip() + "\n", 1)
            else:
                assert text.count("$~") == 1
                text = text.replace("$~", body.rstrip() + "\n$~")
            metadata["prototype_records"].append(dict(kind=kind, vnum=vnum,
                path=str(origin.relative_to(ROOT)).replace("\\", "/"), line=line,
                file_sha256=digest(origin)))
        path.write_text(text, encoding="utf-8")
    (mini / "mini.qst").write_text(
        f"#{case['giver']}\n" + "".join(block["text"] for block in selected) + "S\n$~\n",
        encoding="utf-8")
    # Preserve the complete competing Q family; change supplied O roots only.
    # Synthetic stock establishes no genuine availability or native birth.
    resets = [f"M 0 {case['giver']} 1 22800 100 0 0 0 * relocated actual recipient"] + stock
    for vnum, count in sorted(counts.items()):
        resets += [f"O 0 {vnum} {count} 22800 100 0 0 0 * supplied fixture root"] * count
    path = mini / "mini.zon"
    text = re.sub(r"(?m)^[MGEOP] .*\n", "", path.read_text())
    assert text.count("\nS\n") == 1
    path.write_text(text.replace("\nS\n", "\n" + "\n".join(resets) + "\nS\n"))
    metadata["fixture_input_counts"] = dict(required)  # retained required-count interface
    metadata["fixture_supplied_counts"] = dict(counts)
    metadata["fixture_variant"] = supply
    metadata["selected_contract"] = None if selected_contract is None else dict(
        reward_vnum=reward_vnum, give=selected_contract["give"], receive=selected_contract["receive"],
        disappear=selected_contract["disappear"], line=selected_contract["line"])
    metadata["integration_hooks"] = [
        "Primary integrated SQL-first candidate and authentic reset birth/source/binding; synthetic O is supplied stock only",
        "Native GIVE first transfers to the original NPC, then prepare_original selects from NPC inventory",
        "Exact original child/source/custody/receipt observations and actual reward ACK/paired retirement faults",
        "Maintained crash driver fixture/alias/VNUM selection remains primary-owned; no native journey is run here",
    ]
    metadata["fixture_hashes"] = {p.name: digest(p) for p in sorted(mini.iterdir()) if p.is_file()}
    (output / "quest-prep-provenance.json").write_text(json.dumps(metadata, indent=2) + "\n")
    return metadata


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=list(CASES), required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--reward-vnum", type=int, help="select supplied counts, preserving the full competing Q family")
    parser.add_argument("--supply", choices=("spares", "exact", "shortage", "wrong-kind"), default="spares")
    parser.add_argument("--layout", choices=("mini", "world"), help="defaults to mini static / full world dynamic; world never supplies O stock")
    args = parser.parse_args()
    prepared = prepare(args.case, args.output, reward_vnum=args.reward_vnum, supply=args.supply, layout=args.layout)
    print(json.dumps(dict(case=args.case, output=str(args.output.resolve()),
                         scope=prepared["fixture_scope"]), indent=2))
