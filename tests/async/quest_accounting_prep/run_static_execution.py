#!/usr/bin/env python3
"""Real legacy mini-world commands using production static quest terms.

Relocated reset/O stock is calibration, not authentic native reset birth.
"""

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import time

from case_data import CASES, ROOT, blocks, digest, prototype
from prepare_fixture import prepare
import capture_quest_cut as reader
import quest_cut_checks as checks
import run_quest_reward_ack_crash as crash
import test_static_quest_reward_journey as quest
from run_quest_execution import build_quest_inspector


def execute(args):
    binary = args.server.resolve(strict=True)
    if digest(binary) != args.server_sha256:
        raise ValueError("actual binary hash differs")
    args.evidence_dir.mkdir(parents=True, mode=0o700)
    terms = next(term for term in blocks(args.case) if ("I", args.reward_vnum) in term["receive"])
    if any(kind in ("C", "E", "T") for kind, _ in terms["give"] + terms["receive"]):
        raise ValueError("use maintained mixed-reward crash journey for economic/XP terms")
    result = dict(case=args.case, backend=args.backend, source_commit=args.source_commit,
                  binary_sha256=args.server_sha256, authority="legacy relocated mini fixture",
                  commands=[], result="RUNNING")
    started = time.monotonic()

    def run(binary, unused, *, sql=None, sql_environment=None, **kwargs):
        with crash.retained_directory("quest-prep-static-run-", args.evidence_dir, "run") as run_tmp, \
             crash.retained_directory("quest-prep-static-state-", args.evidence_dir, "state") as state_tmp:
            run_root, state_root = Path(run_tmp), Path(state_tmp)
            (state_root / "domains").mkdir(mode=0o700)
            if sql is None:
                import subprocess
                subprocess.run([str(crash.journey.INSPECTOR), str(state_root), "seed-combat"], check=True)
            fixture = prepare(args.case, run_root, reward_vnum=args.reward_vnum, supply=args.supply)
            (run_root / "logs/log").mkdir(parents=True)
            crash.journey.generate_certificate(run_root)
            journals = run_root / "journals"
            (journals / "players").mkdir(parents=True, mode=0o700)
            (journals / "critical").mkdir(mode=0o700)
            port, tls, websocket = crash.journey.available_ports()
            environment = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(state_root),
                PLAYER_SAVE_JOURNAL_DIR=str(journals / "players"),
                CRITICAL_COMMAND_JOURNAL_DIR=str(journals / "critical"), LISTEN_ADDRESS="127.0.0.1",
                DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket),
                DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", REDIS="FALSE", CHAOS_MUD="FALSE")
            if sql_environment:
                environment.update(sql_environment)
                environment.pop("FLATFILE_STATE_DIR")
                (run_root / "Players").mkdir(mode=0o700)

            def capture(label):
                if sql is None:
                    state = crash.journey.inspect_authority(state_root)
                else:
                    import pymysql
                    connection = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
                        user=environment["DB_USER"], password=environment["DB_PASSWD"], database=environment["DB_NAME"],
                        autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                    try:
                        state = reader.capture(connection, args.case, 1, [], [], dict(
                            source_commit=args.source_commit, binary_sha256=args.server_sha256,
                            schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
                            lineage=None, epoch=None), legacy_no_epoch=True)
                    finally:
                        connection.close()
                (args.evidence_dir / (label + ".json")).write_text(json.dumps(state, indent=2) + "\n", encoding="utf-8")
                return state

            def player_items(state):
                if sql is None:
                    return state["player_items"]
                return [dict(uid=item["item_uid"], vnum=item["vnum"]) for item in state["items"]
                        if item["owner_type"] == 1 and item["state"] == 1]

            reward_ids = None
            for phase in ("initial", "restart"):
                process, output = quest.boot(binary, run_root, environment, port, run_root / (phase + ".out"))
                client = None
                try:
                    if phase == "initial":
                        client = crash.journey.MudClient(port)
                        crash.journey.create_character(client, expected_room=None, class_name="d", hometown="p")
                        client.send("drop all")
                        client.expect("You drop", timeout=20)
                        for vnum, count in fixture["fixture_supplied_counts"].items():
                            alias = prototype("obj", int(vnum))[1].splitlines()[1].split()[0].rstrip("~")
                            for _ in range(count):
                                command = "get " + alias
                                result["commands"].append(command)
                                fields = prototype("obj", int(vnum))[1].split("~")[4].split()
                                secret = bool(int(fields[6]) & 4096)
                                crash.get_quest_input(client, alias, secret=secret)
                        client.send("save")
                        client.expect("Save complete for Taverek.", timeout=30)
                        before = capture("before")
                        observed = Counter(item["vnum"] for item in player_items(before))
                        checks.require(observed == Counter({int(k): v for k, v in fixture["fixture_supplied_counts"].items()}),
                                       "actual acquired kinds/counts differ from fixture")
                        alias = prototype("obj", next(iter(fixture["fixture_supplied_counts"])))[1].splitlines()[1].split()[0].rstrip("~")
                        giver = prototype("mob", CASES[args.case]["giver"])[1].splitlines()[1].split()[0].rstrip("~")
                        command = f"give {alias} {giver}"
                        result["commands"].append(command)
                        client.send(command)
                        if args.expect_refused:
                            expected = ("This quest cannot accept that durable item safely.",)
                            if args.supply == "shortage":
                                expected = ("Bring all the requested items together before offering them.",)
                            if args.case == "QP03":
                                expected = ("No one by that name around here.",)
                            response = client.expect_any(expected, timeout=15)
                            result["refusal_response"] = response
                        else:
                            client.expect("Your quest offering is being accepted.", timeout=15)
                            client.expect("Your committed quest reward is being recovered.", timeout=30)
                    else:
                        client = crash.journey.reconnect_character(port)
                    client.send("save")
                    client.expect("Save complete for Taverek.", timeout=30)
                    after = capture(phase)
                    items = player_items(after)
                    expected_rewards = Counter(n for k, n in terms["receive"] if k == "I")
                    actual_rewards = [item for item in items if item["vnum"] in expected_rewards]
                    checks.require(Counter(item["vnum"] for item in actual_rewards) ==
                                   (Counter() if args.expect_refused else expected_rewards),
                                   "actual reward kinds/counts differ")
                    if phase == "initial":
                        reward_ids = [item["uid"] for item in actual_rewards]
                        if args.expect_refused:
                            checks.require(items == player_items(before), "refusal changed original player input UIDs")
                            if sql is not None:
                                checks.bind(before, after, args.case, legacy=True)
                                checks.unchanged(before, after)
                        if sql is not None and not args.expect_refused:
                            terminal = {item["item_uid"] for item in after["items"] if
                                        item["owner_type"] == 8 and item["state"] == 2}
                            selected = [item["uid"] for item in player_items(before) if item["uid"] in terminal]
                            spare = [item["uid"] for item in player_items(before) if item["uid"] not in terminal]
                            checks.static_complete(before, after, args.case, selected, reward_ids, spare,
                                                   args.reward_vnum, legacy=True)
                        initial = after
                    else:
                        checks.require([item["uid"] for item in actual_rewards] == reward_ids,
                                       "cold load changed original reward UID")
                        if args.expect_refused:
                            checks.require(items == player_items(before), "cold load changed refused input UIDs")
                        if sql is not None:
                            checks.bind(initial, after, args.case, legacy=True)
                            checks.replay(initial, after)
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None
                    quest.stop(process, output)
                finally:
                    if client is not None:
                        (args.evidence_dir / (phase + "-client.txt")).write_bytes(bytes(client.transcript))
                        client.close()
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except Exception:
                            process.kill()
                            process.wait(timeout=5)
                    output.close()
            result["reward_uids"] = reward_ids

    try:
        if args.backend == "mariadb":
            crash.run_sql(binary, "offering", evidence_dir=args.evidence_dir, journey_callback=run)
        else:
            crash.journey.INSPECTOR = build_quest_inspector()
            run(binary, True)
        result["result"] = ("PASS: genuine legacy refusal, original input UIDs unchanged through cold load"
                            if args.expect_refused else "PASS: actual legacy command, save and cold reward UID")
    except Exception as error:
        result.update(result="FAIL", error=f"{type(error).__name__}: {error}")
        raise
    finally:
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        (args.evidence_dir / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", choices=("QP01", "QP02", "QP03", "QP05"), required=True)
    parser.add_argument("--reward-vnum", type=int, required=True)
    parser.add_argument("--supply", choices=("exact", "spares", "shortage", "wrong-kind"), default="exact")
    parser.add_argument("--expect-refused", action="store_true",
                        help="require the genuine legacy durable-item refusal and unchanged original inputs")
    parser.add_argument("--backend", choices=("mariadb", "flatfile"), required=True)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    execute(parser.parse_args())
