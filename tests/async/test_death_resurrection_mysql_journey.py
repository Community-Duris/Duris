#!/usr/bin/env python3
"""RED real-PC death/corpse/resurrection accounting journey on disposable MariaDB.

Requires the disposable Docker wrapper and explicit fixture DB credentials.
The wrapper's fresh MariaDB is forwarded through 127.0.0.1 so migration
policy remains intact. The run creates and drops its own schema, boots the real
server, creates two real characters, kills one in real combat, reconnects the
dead PC, and casts the real resurrection spell while retaining the corpse.
No checkout .env is read.

The default remains the strict accounting release acceptance. --legacy-persistence
qualifies the explicitly inactive fixture's durable gameplay contract and reports
its accounting gap without claiming release coverage.
"""
import argparse
import ipaddress
from contextlib import contextmanager
from pathlib import Path
import os
import select
import signal
import socket
import socketserver
import subprocess
import tempfile
import threading
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
CASTER_ACCOUNT = "Spellacct"
CASTER_NAME = "Veledran"
CASTER_EMAIL = "veleacct@example.invalid"
assert 2 <= len(CASTER_ACCOUNT) <= 12
assert 2 <= len(CASTER_NAME) <= 12


@contextmanager
def disposable_db_loopback(host: str, port: int):
    """Expose the wrapper-owned container on local loopback only."""
    class Forwarder(socketserver.BaseRequestHandler):
        def handle(self):
            try:
                with socket.create_connection((host, port), timeout=5) as upstream:
                    upstream.settimeout(None)
                    peers = (self.request, upstream)
                    while True:
                        readable, _, _ = select.select(peers, (), (), 30)
                        for source in readable:
                            packet = source.recv(65536)
                            if not packet:
                                return
                            peers[source is self.request].sendall(packet)
            except OSError:
                return

    class Listener(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True
        request_queue_size = 128

    listener = Listener(("127.0.0.1", 0), Forwarder)
    thread = threading.Thread(target=listener.serve_forever, daemon=True)
    thread.start()
    try:
        yield listener.server_address[1]
    finally:
        listener.shutdown()
        listener.server_close()
        thread.join()


def run(server: Path, *, legacy_persistence: bool = False) -> None:
    remote_host = os.environ["TEST_DB_HOST"]
    remote_port = int(os.environ.get("TEST_DB_PORT", "3306"))
    try:
        private_address = ipaddress.ip_address(remote_host).is_private
    except ValueError:
        private_address = False
    if (os.environ.get("TEST_DB_DISPOSABLE") != "1" or
            (remote_host not in ("127.0.0.1", "localhost", "host.docker.internal") and
             not private_address)):
        raise AssertionError("run through the disposable Docker MariaDB wrapper; refusing remote SQL")
    with disposable_db_loopback(remote_host, remote_port) as local_port:
        run_local(server, local_port, legacy_persistence=legacy_persistence)


def run_local(server: Path, local_port: int, *, legacy_persistence: bool = False) -> None:
    database = "death_resurrection_" + uuid.uuid4().hex[:12]
    host = "127.0.0.1"
    port = str(local_port)
    password = os.environ["TEST_DB_PASSWORD"]
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local", "DB_HOST": host, "DB_PORT": port,
        "DB_NAME": database, "DB_USER": os.environ["TEST_DB_USER"],
        "DB_PASSWD": password, "DB_ALLOWED_TARGETS": host + "/" + database,
        "MYSQL_PWD": password, "PERSISTENCE_MODE": "mariadb-primary",
        "DB_TLS": "FALSE", "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
        "DURIS_NEVENT_TRACE_PLAYER": "1", "LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
    }
    if "LD_LIBRARY_PATH" in os.environ:
        environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
    mysql_help = subprocess.check_output(["mysql", "--help"], text=True, env=environment)
    mysql_ssl = "--ssl-mode=PREFERRED" if "--ssl-mode" in mysql_help else "--skip-ssl"
    mysql = ["mysql", mysql_ssl, "--protocol=tcp", "--connect-timeout=5", "-h", host,
             "-P", port, "-u", environment["DB_USER"], "-N", "-B"]

    def sql(statement: str, selected: bool = True) -> str:
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=statement, text=True, env=environment).strip()

    def scalar(statement: str) -> int:
        return int(sql(statement))

    def require_inactive_accounting():
        for table in ('economic_sql_global_activation', 'economic_sql_activation_receipt',
                      'economic_sql_lifecycle_installation', 'economic_accounting_operation',
                      'economic_accounting_child', 'economic_accounting_coin_posting',
                      'economic_accounting_item_reference'):
            assert scalar('SELECT COUNT(*) FROM ' + table) == 0, \
                f'inactive persistence fixture unexpectedly contains accounting authority/evidence: {table}'

    sql(f"CREATE DATABASE {database} CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        subprocess.run(["python3", "scripts/migration_runner.py", "adopt", "--kind",
                        "fresh_bootstrap"], cwd=ROOT, env=environment, check=True)
        subprocess.run(["python3", "scripts/migration_runner.py", "run"],
                       cwd=ROOT, env=environment, check=True)
        if legacy_persistence:
            require_inactive_accounting()
        with tempfile.TemporaryDirectory(prefix="death-resurrection-") as runtime_tmp:
            runtime = Path(runtime_tmp)
            journey.make_fixture(runtime)
            journey.generate_certificate(runtime)
            (runtime / "Players").mkdir(mode=0o700)
            (runtime / "logs/log").mkdir(parents=True)
            for name in ("players", "critical"):
                (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
            plain, tls, websocket = journey.available_ports()
            environment.update(
                PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket))
            output_path = runtime / "server.out"
            process = None
            victim = None
            caster = None
            with output_path.open("w") as output:
                def boot():
                    offset = output_path.stat().st_size
                    proc = subprocess.Popen(
                        [str(server), "--minimal", "-s", "-d", str(runtime), str(plain)],
                        cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while b"Entering game loop." not in output_path.read_bytes()[offset:]:
                        if proc.poll() is not None or time.monotonic() > deadline:
                            if proc.poll() is None:
                                proc.terminate()
                            proc.wait(timeout=10)
                            raise AssertionError("MariaDB death/resurrection server did not boot")
                        time.sleep(.1)
                    return proc

                def stop():
                    nonlocal process
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    if process.returncode != 0:
                        raise AssertionError(f"server shutdown exit={process.returncode}")
                    process = None

                def logout(client):
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()

                try:
                    process = boot()
                    victim = journey.MudClient(plain)
                    journey.create_character(victim)
                    victim.send("toggle boon")
                    victim.expect("You will no longer be affected by boons.")
                    journey.complete_npc_combat_journey(victim)
                    victim.close()
                    victim = journey.reconnect_character(plain)
                    victim.send("get all")
                    victim.expect("You get", timeout=20)
                    victim.send("save")
                    victim.expect("Save complete for " + journey.CHARACTER + ".", timeout=30)
                    victim_pid = scalar("SELECT pid FROM player_data WHERE name='" +
                                        journey.CHARACTER + "'")
                    item_uids = sql(
                        f"SELECT item_uid FROM item_current_owner WHERE owner_type=1 "
                        f"AND owner_id={victim_pid} AND state=1 ORDER BY item_uid").splitlines()
                    if len(item_uids) < 3:
                        raise AssertionError(f"fixture needs 3+ real PC inventory items; got {item_uids}")
                    wallet_before = tuple(map(int, sql(
                        f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={victim_pid}"
                    ).split()))
                    if sum(amount * (10 ** index) for index, amount in enumerate(wallet_before)) <= 0:
                        raise AssertionError("real PC fixture needs nonzero carried currency before death")
                    currency_before = set(sql(
                        f"SELECT HEX(operation_id) FROM currency_ledger WHERE pid={victim_pid}").splitlines())
                    logout(victim)
                    victim = None

                    caster = journey.MudClient(plain)
                    journey.create_character(
                        caster, class_name="c", account=CASTER_ACCOUNT,
                        character=CASTER_NAME, email=CASTER_EMAIL)
                    caster.send("save")
                    caster.expect("Save complete for " + CASTER_NAME + ".", timeout=30)
                    logout(caster)
                    caster = None
                    caster_pid = scalar("SELECT pid FROM player_data WHERE name='" + CASTER_NAME + "'")
                    # Promotion is fixture-only, while the server is not running a session
                    # for this PC. Keep its real Cleric class and inventory unchanged.
                    sql(f"UPDATE player_data SET level=62,highest_level=62 WHERE pid={caster_pid}")
                    caster = journey.reconnect_character(
                        plain, account=CASTER_ACCOUNT, character=CASTER_NAME)
                    victim = journey.reconnect_character(plain)
                    victim.send("look")
                    victim.expect("The Regression Arena", timeout=10)
                    victim.send("hit executioner")
                    journey.attack_until_death(victim)
                    victim.expect("ACCOUNT MENU", timeout=45)
                    victim.send("0")
                    victim.close()
                    victim = None

                    # Re-enter the actual PC as the target while leaving the
                    # persisted PC corpse untouched; the trusted Cleric is the
                    # second live character needed by the game's real spell API.
                    victim = journey.reconnect_character(plain)
                    victim.send("look")
                    corpse_view = victim.expect("The corpse of a Human is lying here.", timeout=10)
                    if "Regression Arena" not in corpse_view:
                        # The room title may arrive in a preceding socket chunk.
                        victim.send("look")
                        victim.expect("The Regression Arena", timeout=10)
                        victim.expect("The corpse of a Human is lying here.", timeout=10)

                    deadline = time.monotonic() + 30
                    uid_list = ",".join(item_uids)
                    while scalar(
                        f"SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN ({uid_list}) "
                        "AND owner_type=4 AND state=1") != len(item_uids):
                        if time.monotonic() > deadline:
                            raise AssertionError("real PC death did not durably move all items to corpse")
                        time.sleep(.05)
                    coin_uids = sql(
                        "SELECT DISTINCT l.item_uid FROM item_ownership_ledger l "
                        "JOIN item_current_owner o ON o.item_uid=l.item_uid "
                        f"WHERE l.from_owner_type=1 AND l.from_owner_id={victim_pid} "
                        "AND l.reason_type=9 AND o.owner_type=4 AND o.vnum=3 AND o.state=1 "
                        "ORDER BY l.item_uid").splitlines()
                    if len(coin_uids) != 1 or set(coin_uids) & set(item_uids):
                        raise AssertionError(
                            f"death did not create one new durable coin pile: {coin_uids}")
                    coin_uid_list = ",".join(coin_uids)
                    if any(map(int, sql(
                            f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={victim_pid}"
                    ).split())):
                        raise AssertionError("real PC death did not publish the zero wallet")
                    death_currency = set(sql(
                        f"SELECT HEX(operation_id) FROM currency_ledger WHERE pid={victim_pid} "
                        "AND reason_type=16").splitlines()) - currency_before
                    if not death_currency:
                        raise AssertionError("death did not execute a new wallet-to-coin-pile ledger operation")

                    caster.send("cast 'resurrect' corpse")
                    first_result, _ = caster.expect_any((
                        "start chanting", "call forth", "comes to life again!",
                        "The resurrection cannot safely take hold",
                        "That person must consent",
                        "You can't find a soul to reunite",
                        "You can't resurrect your own corpse",
                        "You can only resurrect corpses", "You can't find",
                        "You don't know",
                    ), timeout=90)
                    if first_result == "comes to life again!":
                        result = first_result
                    else:
                        result, _ = caster.expect_any((
                            "comes to life again!",
                            "The resurrection cannot safely take hold",
                            "That person must consent",
                            "You can't find a soul to reunite",
                            "You can't resurrect your own corpse",
                            "You can only resurrect corpses",
                            "You can't find",
                            "You don't know",
                        ), timeout=90)
                    if result != "comes to life again!":
                        raise AssertionError("real resurrection spell did not complete: " + result)

                    deadline = time.monotonic() + 45
                    while scalar(
                        f"SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN ({uid_list}) "
                        f"AND owner_type=1 AND owner_id={victim_pid} AND state=1") != len(item_uids):
                        if time.monotonic() > deadline:
                            raise AssertionError("resurrection did not publish all original item roots")
                        time.sleep(.05)
                    deadline = time.monotonic() + 30
                    while tuple(map(int, sql(
                            f"SELECT copper,silver,gold,platinum FROM player_data WHERE pid={victim_pid}"
                    ).split())) != wallet_before:
                        if time.monotonic() > deadline:
                            raise AssertionError("resurrection did not restore the original PC wallet")
                        time.sleep(.05)
                    deadline = time.monotonic() + 30
                    while scalar(
                        f"SELECT COUNT(*) FROM item_current_owner WHERE item_uid IN ({coin_uid_list}) "
                        "AND owner_type=8 AND state=2") != len(coin_uids):
                        if time.monotonic() > deadline:
                            raise AssertionError("resurrection did not consume the death coin pile")
                        time.sleep(.05)

                    # RED acceptance: every immutable item/wallet event caused
                    # by death or resurrection needs a linked balanced economic
                    # operation plus its required item-reference lines.
                    currency_rows = [line.split() for line in sql(
                        f"SELECT HEX(operation_id),reason_type FROM currency_ledger "
                        f"WHERE pid={victim_pid} AND reason_type IN (16,18)").splitlines()]
                    new_currency = [row for row in currency_rows if row[0] not in currency_before]
                    new_currency_ops = [row[0] for row in new_currency]
                    reason_types = {int(row[1]) for row in new_currency}
                    if not {16, 18}.issubset(reason_types):
                        raise AssertionError(
                            f"journey missing death/resurrection wallet ledger reason(s): {reason_types}")
                    corpse_events = scalar(
                        f"SELECT COUNT(*) FROM item_ownership_ledger WHERE item_uid IN ({uid_list}) "
                        "AND reason_type=9")
                    resurrect_events = scalar(
                        f"SELECT COUNT(*) FROM item_ownership_ledger WHERE item_uid IN ({uid_list}) "
                        "AND reason_type=11")
                    if corpse_events != len(item_uids) or resurrect_events != len(item_uids):
                        raise AssertionError(
                            f"item movement evidence mismatch: expected={len(item_uids)} "
                            f"death={corpse_events} resurrection={resurrect_events}")
                    coin_events = {reason: scalar(
                        f"SELECT COUNT(*) FROM item_ownership_ledger "
                        f"WHERE item_uid IN ({coin_uid_list}) AND reason_type={reason}")
                        for reason in (2, 9, 3)}
                    if any(count != len(coin_uids) for count in coin_events.values()):
                        raise AssertionError(
                            f"coin pile must be created, moved to corpse, then destroyed: {coin_events}")
                    uncovered_items = scalar(
                        f"SELECT COUNT(*) FROM item_ownership_ledger l "
                        f"LEFT JOIN economic_accounting_item_reference r "
                        "ON r.legacy_operation_id=l.operation_id "
                        "AND r.legacy_event_index=l.event_index "
                        "AND r.item_uid=l.item_uid "
                        "AND r.after_revision=l.item_revision "
                        f"WHERE ((l.item_uid IN ({uid_list}) AND l.reason_type IN (9,11)) "
                        f"OR (l.item_uid IN ({coin_uid_list}) AND l.reason_type IN (2,3,9))) "
                        "AND r.operation_id IS NULL")
                    quoted_currency = ",".join(f"'{operation}'" for operation in new_currency_ops) or "''"
                    unaccounted_currency = scalar(
                        "SELECT COUNT(*) FROM currency_ledger c "
                        "LEFT JOIN economic_accounting_operation e ON e.operation_id=c.operation_id "
                        "LEFT JOIN economic_accounting_child child "
                        "ON child.child_operation_id=c.operation_id "
                        "LEFT JOIN economic_accounting_operation root "
                        "ON root.operation_id=child.operation_id "
                        f"WHERE c.pid={victim_pid} AND c.reason_type IN (16,18) "
                        f"AND HEX(c.operation_id) IN ({quoted_currency}) "
                        "AND e.operation_id IS NULL AND root.operation_id IS NULL") if new_currency_ops else 0
                    missing_legs = scalar(
                        "SELECT COUNT(*) FROM (SELECT DISTINCT HEX(c.operation_id) AS legacy_id, "
                        "COALESCE(e.operation_id,root.operation_id) AS root_id "
                        "FROM currency_ledger c "
                        "LEFT JOIN economic_accounting_operation e ON e.operation_id=c.operation_id "
                        "LEFT JOIN economic_accounting_child child "
                        "ON child.child_operation_id=c.operation_id "
                        "LEFT JOIN economic_accounting_operation root "
                        "ON root.operation_id=child.operation_id "
                        f"WHERE c.pid={victim_pid} AND c.reason_type IN (16,18) "
                        f"AND HEX(c.operation_id) IN ({quoted_currency})) candidate "
                        "WHERE candidate.root_id IS NULL OR "
                        "(SELECT COUNT(*) FROM economic_accounting_coin_posting p "
                        "WHERE p.operation_id=candidate.root_id)<2 OR "
                        "(SELECT COALESCE(SUM(p.copper_value),0) "
                        "FROM economic_accounting_coin_posting p "
                        "WHERE p.operation_id=candidate.root_id)<>0") if new_currency_ops else 0
                    if not new_currency:
                        raise AssertionError("journey produced no post-death/resurrection currency ledger events")
                    if legacy_persistence:
                        require_inactive_accounting()
                        assert uncovered_items == corpse_events + resurrect_events + sum(coin_events.values()), \
                            'inactive fixture has partial or mismatched item accounting coverage'
                        assert unaccounted_currency == missing_legs == len(new_currency), \
                            'inactive fixture has partial or mismatched currency accounting coverage'
                        print(
                            'PASS inactive-accounting death/resurrection persistence; '
                            f'item events death={corpse_events} resurrection={resurrect_events} '
                            f'coin_pile={coin_events}; currency operations={len(new_currency)}',
                            flush=True)
                        print(
                            'RELEASE ACCOUNTING COVERAGE BLOCKED: '
                            f'uncovered_item_events={uncovered_items} '
                            f'unaccounted_currency_operations={unaccounted_currency}', flush=True)
                    elif uncovered_items or unaccounted_currency or missing_legs:
                        raise AssertionError(
                            "RED double-entry gap after real PC death/resurrection: "
                            f"corpse_create_item_events={corpse_events} "
                            f"corpse_loot_item_events={resurrect_events} "
                            f"coin_pile_events={coin_events} "
                            f"uncovered_item_events={uncovered_items} "
                            f"currency_operations={len(new_currency)} "
                            f"unaccounted_currency_operations={unaccounted_currency} "
                            f"currency_operations_with_missing_root_or_postings={missing_legs}")
                    else:
                        print(
                            f"PASS death/resurrection gameplay; item events death={corpse_events} "
                            f"resurrection={resurrect_events} coin_pile={coin_events}; "
                            f"currency operations={len(new_currency)} "
                            "all double-entry-covered", flush=True)
                    victim.send("quit")
                    victim.expect("ACCOUNT MENU", timeout=30)
                    victim.send("0")
                    victim.close()
                    victim = None
                    caster.send("quit")
                    caster.expect("ACCOUNT MENU", timeout=30)
                    caster.send("0")
                    caster.close()
                    caster = None
                    stop()
                except Exception as error:
                    output.flush()
                    logs = journey.runtime_logs(runtime)
                    raise AssertionError(
                        f"{error}\n--- server output ---\n"
                        f"{output_path.read_text(errors='replace')[-12000:]}\n"
                        f"--- runtime logs ---\n{logs}") from error
                finally:
                    if victim:
                        victim.close()
                    if caster:
                        caster.close()
                    if process and process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=10)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=10)
    finally:
        sql("DROP DATABASE " + database, False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--legacy-persistence', action='store_true',
                        help='qualify the inactive durable gameplay contract; retain explicit release-gap evidence')
    args = parser.parse_args()
    if os.environ.get("TEST_DB_DISPOSABLE") != "1":
        raise SystemExit("run only through the disposable MariaDB wrapper")
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    frozen = os.environ.get("DURIS_MATRIX_SQL_BINARY")
    if frozen:
        from pa_accounting_batch_artifact import load_base_build
        build = load_base_build()
        if Path(frozen).resolve() != build.binary:
            raise SystemExit("death journey requires the attested matrix binary")
        run(build.binary, legacy_persistence=args.legacy_persistence)
    else:
        subprocess.run(["make", "-C", "src", "-j2", "PERSISTENCE_BACKEND=mariadb"],
                       cwd=ROOT, check=True)
        run((ROOT / "bin/server/dms_new").resolve(), legacy_persistence=args.legacy_persistence)
