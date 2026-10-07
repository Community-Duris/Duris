#!/usr/bin/env python3
"""Execute the maintained crash journey and capture genuine private SQL cuts.

Legacy runs refuse any installed epoch. Native accounting assertion defaults
remain strict. No activation, UID, birth, task or receipt is seeded here.
"""

import argparse
import json
import os
from pathlib import Path
import re
import sys
import time

from case_data import ROOT, digest
import capture_quest_cut as reader
import quest_cut_checks as checks
import run_quest_reward_ack_crash as driver


def build_quest_inspector():
    """Add current maintained codec links in an owned build invocation.

    The shared inspector manifest remains with its owner. Keep every original
    source/flag/wrapper; no authority or fixture-state logic is replaced.
    """
    import _flatfile_player_fixture as fixture
    from native_build_artifacts import build_native
    sources = list(fixture.SOURCES)
    for source in ("src/economy/economic_baseline_codec.c", "src/economy/economic_baseline_adapter.c",
                   "src/economy/native_quest_cost.c", "src/economy/native_quest_coin_give.c",
                   "src/economy/auction_native_command_context.c",
                   "src/item/lockpick_retirement_continuation.c"):
        if source not in sources:
            sources.append(source)
    return build_native(ROOT / "bin/tests/quest-prep-player-inspector", sources,
                        fixture.FLAGS, fixture.LINK_FLAGS, name="quest-prep-player-inspector")


def verify_cuts(cuts, case_id, move_reward):
    """Same post-run predicates for a live run or its retained immutable cuts."""
    before, crash, recovered = (cuts[name] for name in ("before-offering", "at-crash", "recovered"))
    selected = [item["item_uid"] for item in before["items"] if
                item["owner_type"] == 1 and item["vnum"] in (29262, 29263, 29264)]
    rewards = [item["item_uid"] for item in recovered["items"] if
               item["owner_type"] == 1 and item["vnum"] == 29237 and item["state"] == 1]
    operation = checks.row(crash, "obligations")["offering_operation_id"]
    from legacy_xp import kord
    xp = kord(recovered, operation)
    checks.require(checks.row(crash, "obligations")["continuation"] == xp["continuation"],
                   "original frozen continuation changed during recovery")
    checks.static_complete(before, recovered, case_id, selected, rewards, [], 29237, xp["effective"], legacy=True)
    checks.require(checks.row(crash, "obligations")["acknowledged"] == 0, "crash lacks pending obligation")
    checks.acknowledged(recovered, operation)
    last = recovered
    if move_reward:
        last = cuts["after-move"]
        checks.later_move(recovered, last, rewards[0], legacy=True)
    checks.bind(last, cuts["second-restart"], case_id, legacy=True)
    checks.replay(last, cuts["second-restart"])
    try:
        checks.bind(before, recovered, case_id)
    except checks.CutError as error:
        native_refusal = str(error)
    else:
        raise checks.CutError("legacy cuts accidentally qualified native authority")
    return dict(selected_uids=selected, reward_uids=rewards, xp_evidence=xp,
                offering_operation=operation, native_accounting_refusal=native_refusal,
                captured_assertions="PASS: legacy custody/ledger/XP/ACK/replay" +
                                    ("/later move" if move_reward else ""))


def verify_existing(args):
    """Do not rewrite the original aggregate result or repeat native gameplay."""
    original_path = args.evidence_dir / "result.json"
    original = json.loads(original_path.read_text())
    checks.require(args.case == "QP06" and args.backend == "mariadb",
                   "retained verification requires the supported actual SQL Kord cuts")
    for name in ("source_commit", "backend", "case", "fault_phase", "move_reward"):
        checks.require(original[name] == getattr(args, name), f"retained run pin differs: {name}")
    checks.require(original["binary_sha256"] == args.server_sha256 == digest(args.server),
                   "retained server ELF differs")
    checks.require(original["schema_manifest_sha256"] == digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
                   "retained schema pin differs")
    expected = {"before-offering", "at-crash", "recovered", "second-restart"}
    if args.move_reward:
        expected.add("after-move")
    checks.require(len(original["cuts"]) == len(expected) and set(original["cuts"]) == expected,
                   "retained run lacks the exact complete cut set")
    paths = {name: args.evidence_dir / (name + ".json") for name in original["cuts"]}
    cuts = {name: json.loads(path.read_text()) for name, path in paths.items()}
    verified = verify_cuts(cuts, args.case, args.move_reward)
    verified.update(result="PASS: offline original post-run predicates on retained genuine legacy cuts",
                    original_aggregate_result=original["result"], original_result_sha256=digest(original_path),
                    source_commit=args.source_commit, binary_sha256=args.server_sha256,
                    schema_manifest_sha256=original["schema_manifest_sha256"],
                    cuts_sha256={name: digest(path) for name, path in paths.items()},
                    helper_sha256={name: digest(ROOT / "tests/async/quest_accounting_prep" / name) for name in
                        ("run_quest_execution.py", "legacy_xp.py", "read_quest_continuation.cpp", "quest_cut_checks.py")},
                    command=sys.argv, gameplay_repeated=False, sql_session_opened=False)
    # A second verification also refuses overwrite; preserve every terminal truth.
    with (args.evidence_dir / "verified-cuts.json").open("x", encoding="utf-8") as output:
        output.write(json.dumps(verified, indent=2) + "\n")
    print(json.dumps(verified, indent=2))


def execute(args):
    if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        raise ValueError("actual binary source commit required")
    binary = args.server.resolve(strict=True)
    if digest(binary) != args.server_sha256:
        raise ValueError("binary source pin differs")
    args.evidence_dir.mkdir(parents=True, mode=0o700)  # refuse overwrite
    summary = dict(source_commit=args.source_commit, binary_sha256=digest(binary),
        schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
        backend=args.backend, case=args.case, fault_phase=args.fault_phase,
        move_reward=args.move_reward, authority="legacy fixture gameplay",
        commands=sys.argv, cuts=[], source_files={path: digest(ROOT / path) for path in
            ("src/world/quest.c", "src/specs/specs.world_quest.c",
             "tests/async/run_quest_reward_ack_crash.py")})
    cuts = {}

    def observe(label, *, sql, environment, terms, **unused):
        if sql is None or args.case == "synthetic":
            return
        import pymysql
        connection = pymysql.connect(host="127.0.0.1", port=int(environment["DB_PORT"]),
            user=environment["DB_USER"], password=environment["DB_PASSWD"],
            database=environment["DB_NAME"], autocommit=True,
            cursorclass=pymysql.cursors.DictCursor, connect_timeout=5,
            read_timeout=10, write_timeout=5)
        try:
            cut = reader.capture(connection, args.case, 1, [], [],
                dict(source_commit=args.source_commit, binary_sha256=args.server_sha256,
                     schema_manifest_sha256=summary["schema_manifest_sha256"],
                     lineage=None, epoch=None), legacy_no_epoch=True)
            checks.require(cut["snapshot"][0]["in_transaction"] == 1,
                           "capture did not hold its actual SQL snapshot")
            # Session isolation can differ from next-transaction isolation on
            # MariaDB; START uses the explicit SET TRANSACTION above.
            with connection.cursor() as cursor:
                cursor.execute("SELECT @@in_transaction AS active")
                checks.require(cursor.fetchone()["active"] == 0, "capture leaked a transaction")
            if label == "before-offering":
                try:
                    reader.capture(connection, args.case, 2147483647, [], [], cut["meta"], legacy_no_epoch=True)
                except ValueError as error:
                    checks.require("player row missing" in str(error), "wrong missing-evidence refusal")
                    summary["missing_player_control"] = str(error)
                else:
                    raise checks.CutError("missing actual player evidence was accepted")
                previous = reader.MAX_ROWS
                try:
                    reader.MAX_ROWS = 1
                    with connection.cursor() as cursor:
                        try:
                            reader.selected(cursor, "SELECT item_uid FROM item_current_owner ORDER BY item_uid")
                        except ValueError as error:
                            checks.require("row limit" in str(error), "wrong row-limit refusal")
                            summary["row_limit_control"] = str(error)
                        else:
                            raise checks.CutError("real row-limit control did not refuse")
                finally:
                    reader.MAX_ROWS = previous
            if label == "at-crash":
                previous = reader.MAX_BYTES
                try:
                    reader.MAX_BYTES = 1
                    with connection.cursor() as cursor:
                        try:
                            reader.selected(cursor, "SELECT continuation FROM quest_reward_obligation WHERE player_pid=%s", (1,))
                        except ValueError as error:
                            checks.require("byte limit" in str(error), "wrong BLOB output-limit refusal")
                            summary["blob_output_limit_control"] = str(error)
                        else:
                            raise checks.CutError("real BLOB output-limit control did not refuse")
                finally:
                    reader.MAX_BYTES = previous
        finally:
            connection.close()
        cuts[label] = cut
        (args.evidence_dir / (label + ".json")).write_text(json.dumps(cut, indent=2) + "\n", encoding="utf-8")
        summary["cuts"].append(label)

    started = time.monotonic()
    try:
        if args.backend == "mariadb":
            driver.run_sql(binary, args.fault_phase, args.case, args.move_reward,
                           observer=observe, evidence_dir=args.evidence_dir)
        else:
            driver.journey.INSPECTOR = build_quest_inspector()
            driver.run(binary, True, fault_phase=args.fault_phase, quest_case=args.case,
                       move_reward=args.move_reward, evidence_dir=args.evidence_dir)
        if cuts:
            summary.update(verify_cuts(cuts, args.case, args.move_reward))
        summary["result"] = "PASS: maintained legacy gameplay journey"
    except Exception as error:
        summary.update(result="FAIL", error=f"{type(error).__name__}: {error}")
        if "recovered" in cuts and "recovery-failure" in cuts:
            try:
                checks.bind(cuts["recovered"], cuts["recovery-failure"], args.case, legacy=True)
                checks.unchanged(cuts["recovered"], cuts["recovery-failure"])
                summary["failed_move_refusal_control"] = "PASS: original reward/input custody, wallet, XP and obligation unchanged"
            except checks.CutError as control_error:
                summary["failed_move_refusal_control"] = str(control_error)
        raise
    finally:
        summary["elapsed_seconds"] = round(time.monotonic() - started, 3)
        (args.evidence_dir / "result.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--backend", choices=("mariadb", "flatfile"), required=True)
    parser.add_argument("--case", choices=("synthetic", "QP06"), default="QP06")
    parser.add_argument("--fault-phase", choices=("offering", "xp-ack"), default="offering")
    parser.add_argument("--move-reward", action="store_true")
    parser.add_argument("--evidence-dir", type=Path, required=True)
    parser.add_argument("--verify-existing", action="store_true",
                        help="run the same post-run predicates on retained cuts; write a separate verification")
    args = parser.parse_args()
    (verify_existing if args.verify_existing else execute)(args)
