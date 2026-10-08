#!/usr/bin/env python3
"""Recover frozen daily credit alongside item, cash, and XP reward receipts."""
import argparse
from pathlib import Path
import run_quest_reward_ack_crash as crash

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--server", type=Path, required=True)
parser.add_argument("--phase", choices=("offering", "xp-ack"), default="offering")
parser.add_argument("--backend", choices=("flatfile", "mariadb"), default="flatfile")
args = parser.parse_args()
if not crash.journey.INSPECTOR.is_file():
    crash.journey.build_inspector()
if args.backend == "mariadb":
    crash.run_sql(args.server.resolve(strict=True), args.phase, daily=True)
else:
    crash.run(args.server.resolve(strict=True), True, fault_phase=args.phase, daily=True)
print(f"daily {args.phase} crash recovery and two cold restarts passed")
