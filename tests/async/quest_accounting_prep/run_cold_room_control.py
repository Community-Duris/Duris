#!/usr/bin/env python3
"""Genuine full-world cold-room/later-drop control using existing fixture helpers.

Ordinary starter items only: no quest completion, fault injection, epoch or birth
proof. The separate original mini QP06 RED journey and assertions are unchanged.
"""

import argparse
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

from case_data import ROOT, digest
from prepare_fixture import prepare
from run_quest_execution import build_quest_inspector
import run_world_quest_dual_backend as world
import test_flatfile_combat_journey as journey


def execute(args):
    if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        raise ValueError("actual retained binary source commit required")
    binary = args.server.resolve(strict=True)
    if digest(binary) != args.server_sha256:
        raise ValueError("actual retained binary hash differs")
    args.evidence_dir.mkdir(parents=True, mode=0o700)  # Refuse overwrite.
    state_root = args.evidence_dir / "state"
    state_root.mkdir(mode=0o700)
    run_root = args.evidence_dir / "run"
    fixture = prepare("QP06", run_root, layout="world")
    runtime = fixture["runtime_paths"]
    # Exactly the maintained full-world fixture's local camp calibration;
    # retain its original30-second menu deadline, production areas/specials.
    properties_path = run_root / "lib/duris.properties"
    properties = properties_path.read_text()
    journey.require("camp.timer=9.000" in properties, "maintained camp fixture preimage changed")
    properties_path.write_text(properties.replace("camp.timer=9.000", "camp.timer=2.000"))
    inspector = build_quest_inspector()
    result = dict(binary_source_commit=args.source_commit, binary_sha256=digest(binary),
        backend="flatfile-primary", authority="genuine legacy full-world starter-item control",
        camp_fixture="maintained copied-lib camp.timer=2.000; original30-second menu deadline",
        runtime_properties_sha256=digest(properties_path),
        result="RUNNING", cuts={}, commands=[], boot_stages={},
        schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
        source_hashes={path: digest(ROOT / path) for path in (
            "src/world/db.c", "src/core/files.c", "src/flatfile/flatfile_corpse_restore.c",
            "src/item/item_ownership_runtime.c", "src/item/item_movement_transaction.c",
            "src/flatfile/flatfile_item_repository.c", "areas/AREA", "areas/obj/heavens.obj",
            "tests/async/run_world_quest_dual_backend.py",
            "tests/async/test_flatfile_full_world_boot.py",
            "tests/async/test_flatfile_combat_journey.py",
            "tests/async/quest_accounting_prep/run_cold_room_control.py")})
    started = time.monotonic()

    def capture(label):
        state = json.loads(subprocess.check_output(
            [str(inspector), str(state_root), "inspect", "1"], text=True, timeout=15))
        for name in ("player_items", "room_items"):
            ids = [row["uid"] for row in state[name]]
            journey.require(all(uid > 0 for uid in ids) and len(ids) == len(set(ids)),
                            "missing/duplicate genuine UID")
        journey.require(not ({row["uid"] for row in state["player_items"]} &
                             {row["uid"] for row in state["room_items"]}),
                        "a root appears in both owners")
        result["cuts"][label] = state
        return state

    def command(client, text, expected, timeout):
        result["commands"].append(text)
        client.send(text)
        client.expect(expected, timeout=timeout)

    def rows(state, owner):
        return {row["uid"]: row for row in state[owner + "_items"]}

    def check_move(before, after, uid, source, destination):
        old, new = rows(before, source), rows(after, source)
        journey.require(uid in old and uid not in new, "original source root did not move")
        journey.require({k: v for k, v in old.items() if k != uid} == new,
                        "unrelated source roots changed")
        old_to, new_to = rows(before, destination), rows(after, destination)
        journey.require(uid not in old_to and new_to.get(uid) == old[uid],
                        "destination changed UID/kind/root/parent")
        journey.require({k: v for k, v in new_to.items() if k != uid} == old_to,
                        "unrelated destination roots changed")
        journey.require(after["room_owner_revision"] == before["room_owner_revision"] + 1 and
                        after["player_owner_revision"] == before["player_owner_revision"] + 1,
                        "actual owner revisions did not advance exactly once")
        journey.require(after["wallet"] == before["wallet"] and
                        after["experience"] == before["experience"],
                        "ordinary move changed wallet/XP")

    def camp(client):
        command(client, "save", f"Save complete for {journey.CHARACTER}.", 45)
        command(client, "quit", "ACCOUNT MENU", 30)
        command(client, "0", "Thank you for playing!", 5)

    mace_uid = sword_uid = None
    try:
        for phase in ("initial", "cold"):
            port, tls, websocket = world.available_ports()
            environment = world.runtime_environment("flatfile", state_root,
                Path(runtime["player_journal"]), Path(runtime["critical_journal"]),
                port, tls, websocket, None)
            environment["CHAOS_MUD"] = "FALSE"
            for key in ("CHAOS_TEST_COMMANDS", "CHAOS_TEST_ACCOUNT", "CHAOS_STARTER_BONUSES"):
                environment.pop(key)
            if library_path := os.environ.get("LD_LIBRARY_PATH"):
                environment["LD_LIBRARY_PATH"] = library_path
            output_path = run_root / (phase + ".out")
            output = output_path.open("w", encoding="utf-8")
            process = subprocess.Popen([str(binary), "-d", str(run_root), str(port)],
                cwd=run_root, env=environment, stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                boot = world.wait_for_boot(process, output_path)  # Existing180-second helper.
                journey.require(Path(os.readlink(f"/proc/{process.pid}/exe")).resolve() == binary,
                                "running ELF differs from the pinned candidate")
                stages = {stage: stage in boot for stage in
                          ("-- Player corpses", "-- Shopkeepers", "Reloading SavedItems.", "Booting Ferries")}
                journey.require(all(stages.values()), "full-world restoration stage missing")
                journey.require("Skipping full-world state restoration in mini mode" not in boot,
                                "control unexpectedly booted mini mode")
                journey.require("no path found!" not in boot and
                                "can't find ferry ticket automat" not in boot,
                                "maintained full-world ferry prerequisite failed")
                result["boot_stages"][phase] = stages
                if phase == "initial":
                    client = journey.MudClient(port)
                    journey.create_character(client, expected_room=None)
                    command(client, "save", f"Save complete for {journey.CHARACTER}.", 45)
                    before = capture("before-first-drop")
                    # Both283 and1108 have sword aliases in the real starter
                    # graph. Explicit first-match inventory selection targets
                    #1108; retain its UID and preserve the separate kind283.
                    for vnum, name in ((677, "mace"), (1108, "sword")):
                        candidates = [row["uid"] for row in before["player_items"] if row["vnum"] == vnum]
                        journey.require(len(candidates) == 1, f"one genuine starter {name} required")
                        if name == "mace": mace_uid = candidates[0]
                        else: sword_uid = candidates[0]
                    command(client, "drop mace", "You drop a small wooden mace", 20)
                    after = capture("after-first-drop")
                    check_move(before, after, mace_uid, "player", "room")
                else:
                    restored = capture("cold-before-login")
                    previous = result["cuts"]["after-first-drop"]
                    for key in ("player_items", "room_items", "player_owner_revision",
                                "room_owner_revision", "wallet", "experience"):
                        journey.require(restored[key] == previous[key],
                                        f"cold restoration changed original {key}")
                    client = journey.reconnect_character(port,
                        return_message="You break camp and get ready to move on", expected_room=None)
                    before = capture("cold-before-later-drop")
                    journey.require(before["room_owner_revision"] > 0 and
                                    rows(before, "room").get(mace_uid) == rows(restored, "room")[mace_uid],
                                    "cold room lacks its original durable revision/root")
                    # First post-login item transfer: GET must not hydrate the room for this check.
                    command(client, "drop 1.sword", "You drop a long steel sword", 20)
                    after = capture("cold-after-later-drop")
                    check_move(before, after, sword_uid, "player", "room")
                    # The maintained full-world control releases the genuinely
                    # overfull starter inventory before GET. Do this only after
                    # the cold later-drop assertion, retaining exact custody.
                    command(client, "drop all", "You drop a steel long sword", 45)
                    released = capture("cold-after-stock-release")
                    journey.require(not released["player_items"] and
                        rows(released, "room") == {**rows(after, "room"), **rows(after, "player")},
                        "genuine bulk release changed/lost an original item identity")
                    journey.require(released["room_owner_revision"] == after["room_owner_revision"] + 1 and
                        released["player_owner_revision"] == after["player_owner_revision"] + 1 and
                        released["wallet"] == after["wallet"] and
                        released["experience"] == after["experience"],
                        "bulk release revisions/wallet/XP differ")
                    command(client, "get mace", "You get a small wooden mace", 20)
                    picked_up = capture("cold-after-original-pickup")
                    check_move(released, picked_up, mace_uid, "room", "player")
                camp(client)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                output.flush()
                journey.require(process.returncode == 0 and
                    "Normal termination of game." in output_path.read_text(errors="replace"),
                    "full-world process did not terminate normally")
            finally:
                if client is not None:
                    (args.evidence_dir / (phase + "-client.txt")).write_bytes(bytes(client.transcript))
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)
                output.close()
        result.update(result="PASS: genuine full-world cold restoration and first later drop",
                      mace_uid=mace_uid, sword_uid=sword_uid)
    except Exception as error:
        result.update(result="FAIL: full-world control", error=f"{type(error).__name__}: {error}")
        raise
    finally:
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        (args.evidence_dir / "result.json").write_text(json.dumps(result, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in result.items() if k != "cuts"}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    execute(parser.parse_args())
