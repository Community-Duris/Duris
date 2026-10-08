#!/usr/bin/env python3
"""Prove quest items, cash and XP survive offering/XP-ACK crashes and two restarts."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import uuid

import test_flatfile_combat_journey as journey
import test_static_quest_reward_journey as quest


def prepare_quest_fixture(run_root: Path, case_id: str, daily: bool = False) -> dict:
    """Retain the original calibration and expose the actual Kord contract.

    QP06 copies production prototypes/Q terms. Its relocated NPC and supplied
    O stock are a journey fixture, not authentic birth or activation evidence.
    """
    if case_id == "synthetic":
        quest.quest_fixture(run_root, xp_reward=100, daily=daily)
        return dict(offering_vnums=(22802, 22803, 22804),
                    offering_names=("acorn", "branch", "feather"), giver="lapney",
                    reward_vnum=quest.REWARD_VNUM, reward_name="blade", coin_reward=1000)
    if case_id != "QP06":
        raise ValueError("unsupported quest crash fixture")
    subprocess.run([sys.executable, "-B", str(quest.ROOT /
                   "tests/async/quest_accounting_prep/prepare_fixture.py"),
                    "--case", case_id, "--output", str(run_root)], check=True)
    evidence = json.loads((run_root / "quest-prep-provenance.json").read_text(encoding="utf-8"))
    blocks = evidence["blocks"]
    journey.require(len(blocks) == 1, "Kord's production completion changed")
    terms = blocks[0]
    journey.require(terms["give"] == [["I", 29262], ["I", 29263], ["I", 29264]] and
                    terms["receive"] == [["E", 2500], ["C", 3000], ["I", 29237]] and
                    not terms["disappear"] and evidence["config"]["giver"] == 29257,
                    "reverify the actual Kord recipe, money, XP and recipient")
    return dict(offering_vnums=(29262, 29263, 29264),
                offering_names=("ear", "scalp", "toe"), giver="kord",
                reward_vnum=29237, reward_name="dagger", coin_reward=3000)


def fault_boot(binary: Path, run_root: Path, environment: dict[str, str],
               port: int, output_path: Path, fault_phase: str = "offering"):
    output = output_path.open("w", encoding="utf-8")
    command = [
        "gdb", "-q", "-batch", "-ex", "set confirm off",
        "-ex", "set debuginfod enabled off",
        "-ex", ("break complete_quest_offering" if fault_phase == "offering" else
                "break quest_reward_recovery_save_acknowledged if receipt_count > 0"),
        "-ex", "run",
        "-ex", "kill", "--args", str(binary), "--minimal",
        "-d", str(run_root), str(port),
    ]
    process = subprocess.Popen(command, cwd=run_root, env=environment,
                               stdout=output, stderr=subprocess.STDOUT)
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        output.flush()
        if "Entering game loop." in output_path.read_text(errors="replace"):
            return process, output
        if process.poll() is not None:
            break
        time.sleep(0.1)
    raise AssertionError("fault server did not boot:\n" +
                         output_path.read_text(errors="replace")[-8000:])


def pending_quest_reward_count(state_root: Path, player_pid: int) -> int:
    output = subprocess.check_output(
        [str(journey.INSPECTOR), str(state_root), "pending-quest-rewards",
         str(player_pid)], text=True, timeout=15)
    return int(output.strip())


def run(binary: Path, expect_recovered: bool, *, sql=None,
        sql_environment=None, fault_phase: str = "offering", daily: bool = False,
        quest_case: str = "synthetic", move_reward: bool = False) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-quest-crash-state-") as state_tmp, \
         tempfile.TemporaryDirectory(prefix="duris-quest-crash-run-") as run_tmp:
        state_root, run_root = Path(state_tmp), Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        if sql is None:
            subprocess.run([str(journey.INSPECTOR), str(state_root), "seed-combat"],
                           check=True)
        terms = prepare_quest_fixture(run_root, quest_case, daily=daily)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        if daily:
            quest.seed_daily_evidence(state_root, xp_reward=100)
            if sql is not None:
                evidence = (state_root / "domains/zone-story-quests.state").read_bytes()[56:].decode("ascii")
                evidence = evidence.replace("\\", "\\\\").replace("'", "\\'").replace("\n", "\\n")
                sql("INSERT INTO zone_story_quest_state (state_id,state_version,catalog_revision,state_blob) "
                    "VALUES (1,2,2,'" + evidence + "')")
        journey.generate_certificate(run_root)
        journals = run_root / "journals"
        (journals / "players").mkdir(parents=True, mode=0o700)
        (journals / "critical").mkdir(mode=0o700)
        port, tls_port, websocket_port = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "ENVIRONMENT": "local", "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(journals / "players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(journals / "critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": str(websocket_port), "REDIS": "FALSE",
            "CHAOS_MUD": "FALSE",
            # Bound synthetic diagnostics identify the exact failed revision
            # and native error without recording connection credentials.
            "DURIS_NEVENT_TRACE_PLAYER": "1",
        }
        if daily:
            environment["ZONE_STORY_DAILY_ENABLED"] = "true"
        if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = runtime_library_path
        if sql_environment is not None:
            environment.update(sql_environment)
            environment.pop("FLATFILE_STATE_DIR")
            (run_root / "Players").mkdir(mode=0o700)

        def authority():
            if sql is None:
                state = journey.inspect_authority(state_root)
                return {key: state[key] for key in ("wallet", "experience", "player_items")}
            rows = sql("SELECT obj_uid,vnum FROM player_items WHERE pid=1 ORDER BY obj_uid")
            wallet = sql("SELECT copper,silver,gold,platinum,exp FROM player_data WHERE pid=1")
            values = [int(value) for value in wallet.split('\t')]
            return {
                "wallet": values[:4], "experience": values[4],
                "player_items": [{"uid": int(uid), "vnum": int(vnum)}
                                 for uid, vnum in (row.split('\t') for row in rows.splitlines())],
            }

        def pending_rewards():
            if sql is None:
                return pending_quest_reward_count(state_root, 1)
            return int(sql("SELECT COUNT(*) FROM quest_reward_obligation WHERE player_pid=1 "
                           "AND acknowledged_at IS NULL"))

        def wallet_value():
            return sum(amount * denomination for amount, denomination in
                       zip(authority()["wallet"], (1, 10, 100, 1000)))

        def reward_custody(uid):
            if sql is None:
                return json.loads(subprocess.check_output(
                    [str(journey.INSPECTOR), str(state_root), "inspect-item", str(uid)],
                    text=True, timeout=15))
            rows = sql("SELECT item_uid,vnum,item_revision,owner_type,state,root_item_uid,"
                       "COALESCE(parent_item_uid,0) FROM item_current_owner WHERE item_uid=" + str(uid))
            values = rows.split('\t')
            journey.require(len(values) == 7, "reward custody must retain exactly one original UID")
            return dict(zip(("uid", "vnum", "revision", "owner_type", "state", "root", "parent"),
                            (int(value) for value in values)))

        crash_output = run_root / "ack-crash.out"
        process, output = fault_boot(binary, run_root, environment, port,
                                     crash_output, fault_phase)
        client = journey.MudClient(port)
        try:
            journey.create_character(client, expected_room=None, class_name="d",
                                     hometown="p")
            client.send("drop all")
            client.expect("You drop", timeout=20)
            for name in terms["offering_names"]:
                client.send(f"get {name}")
                client.expect("You get", timeout=15)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            before = authority()
            wallet_baseline = wallet_value()
            offering_uids = {row["uid"] for row in before["player_items"]
                             if row["vnum"] in terms["offering_vnums"]}
            journey.require(len(offering_uids) == 3, "offering fixture lost original UIDs")
            journey.require(all(sum(row["vnum"] == vnum for row in before["player_items"]) == 1
                                for vnum in terms["offering_vnums"]),
                            "offering fixture must hold exactly one original root of each kind")
            client.send(f"give {terms['offering_names'][0]} {terms['giver']}")
            client.expect("Your quest offering is being accepted.", timeout=15)
            process.wait(timeout=30)
            output.flush()
            fault_text = crash_output.read_text(errors="replace")
            breakpoint = ("complete_quest_offering" if fault_phase == "offering" else
                          "quest_reward_recovery_save_acknowledged")
            journey.require("Breakpoint 1," in fault_text and breakpoint in fault_text,
                            "fault did not reach the requested quest boundary")
            at_crash = authority()
            journey.require(pending_rewards() == 1, "crash lost the reward obligation")
            if sql is not None:
                count = int(sql("SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN (" +
                                ','.join(str(uid) for uid in sorted(offering_uids)) +
                                ") AND owner_type=8 AND state=2"))
                journey.require(count == 3, "offering crash lacks original UID tombstones")
                applied = int(sql("SELECT xp_applied_mask FROM quest_reward_obligation WHERE player_pid=1"))
                journey.require(applied == int(fault_phase == "xp-ack"),
                                "XP crash did not reach the intended durable boundary")
        finally:
            client.close()
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            output.close()

        restart_output = run_root / "restart.out"
        process, output = quest.boot(binary, run_root, environment, port,
                                     restart_output)
        client = None
        try:
            client = journey.reconnect_character(port)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            pending_after_load = pending_rewards()
            if expect_recovered:
                deadline = time.monotonic() + 10
                while pending_after_load and time.monotonic() < deadline:
                    time.sleep(0.1)
                    pending_after_load = pending_rewards()
            recovered = authority()
            items = recovered["player_items"]
            rewards = [item for item in items if item["vnum"] == terms["reward_vnum"]]
            offerings = [item for item in items if item["vnum"] in terms["offering_vnums"]]
            journey.require(not offerings, f"consumed offerings returned: {offerings}")
            expected = 1 if expect_recovered else 0
            diagnostic_lines = []
            for line in (restart_output.read_text(errors="replace") + "\n" +
                         journey.runtime_logs(run_root)).splitlines():
                if any(term in line.lower() for term in
                       ("quest", "reward", "currency", "materialize", "obligation")):
                    diagnostic_lines.append(line)
            journey.require(len(rewards) == expected,
                            f"expected {expected} reward after ack crash, found {rewards}; "
                            f"pending={pending_after_load}\n"
                            f"--- recovery diagnostics ---\n" +
                            "\n".join(diagnostic_lines[-80:]))
            expected_wallet = (wallet_baseline + terms["coin_reward"]
                               if expect_recovered else wallet_baseline)
            journey.require(wallet_value() == expected_wallet,
                            f"expected wallet value {expected_wallet} after recovery, "
                            f"found {wallet_value()}")
            if expect_recovered:
                journey.require(recovered["experience"] > before["experience"],
                                "quest recovery lost XP")
                if fault_phase == "xp-ack":
                    journey.require(recovered["experience"] == at_crash["experience"],
                                    "lost XP completion paid the reward twice")
                if sql is not None:
                    journey.require(int(sql("SELECT xp_applied_mask FROM quest_reward_obligation "
                                            "WHERE player_pid=1")) == 1,
                                    "quest XP lacks its durable application marker")
            if expect_recovered:
                journey.require(pending_after_load == 0,
                                f"recovered reward obligation remains pending: "
                                f"{pending_after_load}")
            if daily:
                client.send("quest daily")
                client.expect("Completed today: 1; renown: 1", timeout=15)
            moved_custody = None
            if expect_recovered and move_reward:
                original_uid = rewards[0]["uid"]
                original_custody = reward_custody(original_uid)
                client.send(f"drop {terms['reward_name']}")
                client.expect("You drop", timeout=20)
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
                moved = authority()
                moved_custody = reward_custody(original_uid)
                journey.require(not any(item["vnum"] == terms["reward_vnum"]
                                        for item in moved["player_items"]) and
                                moved_custody["uid"] == original_uid and
                                moved_custody["vnum"] == terms["reward_vnum"] and
                                moved_custody["owner_type"] == 3 and moved_custody["state"] == 1 and
                                moved_custody["revision"] > original_custody["revision"] and
                                moved["wallet"] == recovered["wallet"] and
                                moved["experience"] == recovered["experience"],
                                "legitimate reward drop must preserve UID, cash and XP")
                recovered = moved
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            client.send("0")
            client.close()
            client = None
            quest.stop(process, output)
        except Exception as error:
            output.flush()
            raise AssertionError(
                f"recovery: {error}\n--- server ---\n{restart_output.read_text(errors='replace')[-8000:]}"
                f"\n--- logs ---\n{journey.runtime_logs(run_root)}"
            ) from error
        finally:
            if client is not None:
                client.close()
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            if not output.closed:
                output.close()

        if not expect_recovered:
            return
        # A second cold load must retain the same reward UID, XP and cash.
        process, output = quest.boot(binary, run_root, environment, port,
                                     run_root / "second-restart.out")
        client = None
        try:
            client = journey.reconnect_character(port)
            client.send("save")
            client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
            again = authority()
            if daily:
                client.send("quest daily")
                client.expect("Completed today: 1; renown: 1", timeout=15)
            journey.require(again == recovered, "second restart changed reward UID, XP or cash")
            journey.require(pending_rewards() == 0, "second restart reopened the obligation")
            if moved_custody is not None:
                journey.require(reward_custody(moved_custody["uid"]) == moved_custody,
                                "second restart restored or rewrote the already-moved reward")
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            client.send("0")
            client.close()
            client = None
            quest.stop(process, output)
        except Exception as error:
            output.flush()
            raise AssertionError(
                f"second restart: {error}\n--- server ---\n{(run_root / 'second-restart.out').read_text(errors='replace')[-8000:]}"
                f"\n--- logs ---\n{journey.runtime_logs(run_root)}"
            ) from error
        finally:
            if client is not None:
                client.close()
            if process.poll() is None:
                process.kill()
                process.wait(timeout=5)
            if not output.closed:
                output.close()


def run_sql(binary: Path, fault_phase: str, quest_case: str = "synthetic",
            move_reward: bool = False, daily: bool = False) -> None:
    if os.environ.get("TEST_DB_DISPOSABLE") != "1" or os.environ.get("TEST_DB_HOST") != "127.0.0.1":
        raise RuntimeError("TEST_DB_DISPOSABLE=1 and a loopback disposable database are required")
    database = "quest_journey_test_" + uuid.uuid4().hex[:12]
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
        "DB_HOST": "127.0.0.1", "DB_PORT": os.environ.get("TEST_DB_PORT", "3306"),
        "DB_NAME": database, "DB_USER": os.environ["TEST_DB_USER"],
        "DB_PASSWD": os.environ["TEST_DB_PASSWORD"], "MYSQL_PWD": os.environ["TEST_DB_PASSWORD"],
        "DB_ALLOWED_TARGETS": "127.0.0.1/" + database,
        "PERSISTENCE_MODE": "mariadb-primary", "DB_TLS": "FALSE",
    }
    if library_path := os.environ.get("LD_LIBRARY_PATH"):
        environment["LD_LIBRARY_PATH"] = library_path
    mysql = ["mysql", "--protocol=tcp", "-h", "127.0.0.1", "-P", environment["DB_PORT"],
             "-u", environment["DB_USER"], "-N", "-B"]

    def sql(query, selected=True):
        return subprocess.check_output(mysql + ([database] if selected else []), input=query,
                                       text=True, env=environment).strip()

    sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((quest.ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for command in (["adopt", "--kind", "fresh_bootstrap"], ["run"]):
            subprocess.run(["python3", "scripts/migration_runner.py", *command],
                           cwd=quest.ROOT, env=environment, check=True, timeout=600)
        run(binary, True, sql=sql, sql_environment=environment, fault_phase=fault_phase,
            quest_case=quest_case, move_reward=move_reward, daily=daily)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--confirm-loss", action="store_true",
                        help="record the known failing baseline before recovery is implemented")
    parser.add_argument("--backend", choices=("flatfile", "mariadb"), default="flatfile")
    parser.add_argument("--fault-phase", choices=("offering", "xp-ack"), default="offering")
    parser.add_argument("--quest-case", choices=("synthetic", "QP06"), default="synthetic",
                        help="retain original calibration or use production Kord terms/prototypes")
    parser.add_argument("--move-reward", action="store_true",
                        help="after recovery, drop the original reward before the second cold boot")
    parser.add_argument("--daily", action="store_true",
                        help="qualify frozen daily credit with the synthetic fixture")
    args = parser.parse_args()
    if args.daily and args.quest_case != "synthetic":
        parser.error("daily qualification uses the synthetic fixture")
    if args.confirm_loss and (args.quest_case != "synthetic" or args.move_reward):
        parser.error("historical loss calibration requires its original synthetic fixture")
    if args.backend == "mariadb":
        if args.confirm_loss:
            parser.error("--confirm-loss is only a historical flatfile baseline")
        run_sql(args.server.resolve(strict=True), args.fault_phase, args.quest_case, args.move_reward, args.daily)
    else:
        subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                        "--build-inspector", str(journey.INSPECTOR)],
                       cwd=quest.ROOT, check=True, timeout=180)
        run(args.server.resolve(strict=True), not args.confirm_loss, fault_phase=args.fault_phase,
            quest_case=args.quest_case, move_reward=args.move_reward, daily=args.daily)
    print("post-ack quest reward crash journey passed")
