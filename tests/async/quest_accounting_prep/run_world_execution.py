#!/usr/bin/env python3
"""Run maintained genuine Woodseer bartender calibration on a pinned binary.

This exercises the same payment callback owner as Quietus. It records the
actual level56/giver16553; it does not claim room1734/giver1709 or active birth.
"""

import argparse
import json
from pathlib import Path
import time

from case_data import ROOT, digest
import capture_quest_cut as reader
import quest_cut_checks as checks
import run_world_quest_dual_backend as world
import run_quest_reward_ack_crash as crash


def execute(args):
    args.evidence_dir.mkdir(parents=True, mode=0o700)
    if digest(args.server) != args.server_sha256:
        raise ValueError("actual binary hash differs")
    result = dict(source_commit=args.source_commit, binary_sha256=args.server_sha256,
                  schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
                  backend=args.backend, authority="legacy full-world Woodseer calibration",
                  cases="QP04 creation and QP07 legitimate map/abandon; no stale debit injection",
                  actual_room=16633, actual_giver=16553, actual_level=56, commands=vars(args).copy())
    result["commands"] = {k: str(v) if isinstance(v, Path) else v for k, v in result["commands"].items()}
    started = time.monotonic()

    def run(binary, unused, *, sql_environment=None, **kwargs):
        with crash.retained_directory("quest-prep-world-state-", args.evidence_dir, "state") as state_tmp:
            state_root = Path(state_tmp)
            state_root.chmod(0o700)
            environment = None
            if sql_environment:
                environment = dict(sql_environment, DB_SOCKET="")
            result["journey"] = world.perform_quest_journey(binary, args.backend, state_root,
                environment, evidence_dir=args.evidence_dir, buy_map=True)
            transcript = (args.evidence_dir / "client.txt").read_text(errors="replace")
            result["actual_fee_quotes"] = [line for line in transcript.splitlines()
                if "It'll cost you" in line or "toss me" in line]
            if environment:
                import pymysql
                connection = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
                    user=environment["DB_USER"], password=environment["DB_PASSWD"],
                    database=environment["DB_NAME"], autocommit=True,
                    cursorclass=pymysql.cursors.DictCursor, connect_timeout=5,
                    read_timeout=10, write_timeout=5)
                try:
                    with connection.cursor() as cursor:
                        cursor.execute("SELECT pid FROM player_data WHERE name=%s", ("Taverek",))
                        players = cursor.fetchall()
                    checks.require(len(players) == 1, "actual bartender player identity missing")
                    cut = reader.capture(connection, "QP04", players[0]["pid"], [], [],
                        dict(source_commit=args.source_commit, binary_sha256=args.server_sha256,
                             schema_manifest_sha256=result["schema_manifest_sha256"],
                             lineage=None, epoch=None), legacy_no_epoch=True)
                finally:
                    connection.close()
                (args.evidence_dir / "terminal-cut.json").write_text(json.dumps(cut, indent=2) + "\n")
                fees = [entry for entry in cut["currency"] if entry["reason_id"] == 16553]
                amounts = [checks.value(entry, "wallet_delta_") for entry in fees]
                mapped = result["journey"]["map_result"] == "will now show you additional information"
                checks.require(len(fees) == 4 + int(mapped), "missing or extra legitimate bartender charge")
                checks.require(len({entry["operation_id"] for entry in fees}) == len(fees),
                               "bartender repeated a fee operation")
                checks.require(all(entry["reason_type"] == 6 and amount < 0
                                   for entry, amount in zip(fees, amounts)), "unexpected service credit/reason")
                checks.require(amounts.count(-1120) == 2, "two level56 creation fees must be exact")
                if mapped:
                    checks.require(amounts.count(-560) == 1, "level56 map fee must be exact")
                abandon = list(amounts)
                for fixed in [-1120, -1120] + ([-560] if mapped else []):
                    abandon.remove(fixed)
                checks.require(len(abandon) == 2 and all(0 < -fee <= 43904 for fee in abandon),
                               "abandon fees exceed original configured level56 quote bound")
                checks.require(checks.row(cut, "player")["quest_active"] == 0,
                               "final paid abandonment did not persist retired task")
                checks.require(cut["obligations"] == [] and cut["xp_entitlements"] == [],
                               "legitimate service unexpectedly issued item/XP reward obligation")
                # Full forest is captured. These services should have no item
                # event under any of their actual committed fee operations.
                fee_ids = {entry["operation_id"] for entry in fees}
                checks.require(not any(event["operation_id"] in fee_ids for event in cut["ownership_events"]),
                               "bartender service changed an item UID")
                result["terminal_capture"] = dict(pid=players[0]["pid"], fee_copper=amounts,
                    fee_operations=sorted(fee_ids), map_information_bought=mapped,
                    player_item_uids=[item["item_uid"] for item in cut["items"]],
                    final_task_active=0, authority="legacy ledger/task only; no native settlement or refund")

    try:
        if args.backend == "mariadb":
            crash.run_sql(args.server, "offering", journey_callback=run, evidence_dir=args.evidence_dir)
        else:
            run(args.server, True)
        result["result"] = "PASS: maintained full-world bartender calibration"
    except Exception as error:
        result.update(result="FAIL", error=f"{type(error).__name__}: {error}")
        raise
    finally:
        result["elapsed_seconds"] = round(time.monotonic() - started, 3)
        (args.evidence_dir / "result.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", choices=("mariadb", "flatfile"), required=True)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    execute(parser.parse_args())
