#!/usr/bin/env python3
"""Prepare isolated real-term mini data; does not boot or seed any authority."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re

from case_data import CASES, ROOT, blocks, digest, facts, prototype, record, reset_family
import test_flatfile_combat_journey as journey


def prepare(case_id, output):
    case = CASES[case_id]
    output = Path(output).resolve()
    if output.exists() and any(output.iterdir()):
        raise ValueError("fixture output must be a new or empty directory")
    output.mkdir(parents=True, exist_ok=True)
    if case.get("dynamic"):
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
        metadata["commands"] = ["ask one-eyed quest"] if case_id == "QP04" else [
            "ask one-eyed map", "ask one-eyed abandon confirm"]
        metadata["integration_hooks"] = [
            "Disposable SQL authority and authentic accounting epoch from primary lifecycle owner",
            "Configured mortal level/quoted fees; maintained Chaos creator uses level56, not level11",
            "Real arrival at room1734/giver1709; maintained driver defaults to Woodseer16633/16553",
            "Fault seam after original debit and before callback; no-target or replacement-task control",
            "Native wallet/task/history/receipt/root captures and two cold boots on integrated candidate",
        ]
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
    counts = Counter()
    for block in selected:
        counts |= Counter(number for kind, number in block["give"] if kind == "I")
    # Include one spare of each kind to test selected-root precision. The native
    # production caps are preserved in provenance, not asserted by synthetic O.
    resets = [f"M 0 {case['giver']} 1 22800 100 0 0 0 * relocated actual recipient"] + stock
    for vnum, count in sorted(counts.items()):
        resets += [f"O 0 {vnum} {count + 1} 22800 100 0 0 0 * supplied fixture root"] * (count + 1)
    path = mini / "mini.zon"
    text = re.sub(r"(?m)^[MGEOP] .*\n", "", path.read_text())
    assert text.count("\nS\n") == 1
    path.write_text(text.replace("\nS\n", "\n" + "\n".join(resets) + "\nS\n"))
    metadata["fixture_input_counts"] = dict(counts)
    metadata["fixture_hashes"] = {p.name: digest(p) for p in sorted(mini.iterdir()) if p.is_file()}
    (output / "quest-prep-provenance.json").write_text(json.dumps(metadata, indent=2) + "\n")
    return metadata


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=list(CASES), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prepared = prepare(args.case, args.output)
    print(json.dumps(dict(case=args.case, output=str(args.output.resolve()),
                         scope=prepared["fixture_scope"]), indent=2))
