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
