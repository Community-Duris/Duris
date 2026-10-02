"""Disposable SQL/game fixture for the frozen S02 copyover qualification.

No checkout .env, shared service, fake lifecycle authority, or server build.
The caller holds the rollout's assigned slot and shared heavy-resource lease.
"""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import select
import shutil
import signal
import socket
import socketserver
import subprocess
import tempfile
import threading
import time

import test_flatfile_combat_journey as journey

ROOT = Path(__file__).resolve().parents[2]
LIFECYCLE_LOCK = "duris:economic_sql_boot_maintenance"
PLAYER_LOCK_PREFIX = "duris.player.death.restitution."


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def environment():
    result = {"PATH": os.environ.get("PATH", "/usr/bin:/bin"),
              "HOME": os.environ.get("HOME", "/tmp"), "LC_ALL": "C"}
    if "LD_LIBRARY_PATH" in os.environ:
        result["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
    return result


def record(phase, **values):
    print("PA_COPYOVER_PHASE " + json.dumps(dict(phase=phase, **values), sort_keys=True),
          flush=True)


class LoopbackTcpForward:
    """Docker Desktop publishes on its host, not this client container.

    Keep BOTH published and client-side listeners loopback-only. The forward is
    byte-transparent; real MySQL sessions/IDs/locks are never emulated.
    """
    def __init__(self, remote_host, remote_port):
        self.connections = set()
        self.lock = threading.Lock()
        owner = self

        class Handler(socketserver.BaseRequestHandler):
            def handle(self):
                upstream = None
                pair = None
                try:
                    upstream = socket.create_connection((remote_host, remote_port), timeout=10)
                    upstream.settimeout(None)
                    self.request.settimeout(None)
                    pair = (self.request, upstream)
                    with owner.lock:
                        owner.connections.add(pair)
                    readable_sockets = set(pair)
                    while readable_sockets:
                        ready, _, _ = select.select(list(readable_sockets), [], [], 10)
                        for source in ready:
                            destination = upstream if source is self.request else self.request
                            data = source.recv(65536)
                            if data:
                                destination.sendall(data)
                            else:
                                readable_sockets.remove(source)
                                try:
                                    destination.shutdown(socket.SHUT_WR)
                                except OSError:
                                    pass
                except (OSError, ValueError):
                    pass
                finally:
                    if upstream is not None:
                        upstream.close()
                    if pair is not None:
                        with owner.lock:
                            owner.connections.discard(pair)

        class Server(socketserver.ThreadingTCPServer):
            allow_reuse_address = True
            daemon_threads = True
            request_queue_size = 32

        self.server = Server(("127.0.0.1", 0), Handler)
        self.port = int(self.server.server_address[1])
        self.thread = threading.Thread(target=self.server.serve_forever,
                                       name="pa-copyover-db-forward", daemon=True)
        self.thread.start()
        record("db-forward", listener=f"127.0.0.1:{self.port}",
               docker_host=remote_host, docker_host_port=remote_port)

    def close(self):
        self.server.shutdown()
        self.server.server_close()
        with self.lock:
            active = tuple(self.connections)
        for pair in active:
            for connection in pair:
                try:
                    connection.shutdown(socket.SHUT_RDWR)
                except OSError:
                    pass
                connection.close()
        self.thread.join(timeout=10)
        require(not self.thread.is_alive(), "database forward listener did not stop")


class DisposableMariaDB:
    def __init__(self):
        token = secrets.token_hex(6)
        self.container = f"duris-pa-copyover-{token}"
        self.database = f"pa_copyover_{token}"
        self.user = "pa_copyover"
        self.root_password = secrets.token_hex(32)
        self.password = secrets.token_hex(32)
        self.image = os.environ.get("PA_COPYOVER_DB_IMAGE", "mariadb:10.11")
        require(self.image.startswith(("mariadb:", "mysql:")),
                "PA_COPYOVER_DB_IMAGE must be mariadb:* or mysql:*")
        self.prefix = "MARIADB" if self.image.startswith("mariadb:") else "MYSQL"
        docker = shutil.which("docker")
        client = shutil.which("mysql") or shutil.which("mariadb")
        if docker is None or client is None:
            raise RuntimeError("Docker and mysql/mariadb client are required")
        self.docker, self.client = docker, client
        self.started = False
        self.port = None
        self.forward = None
        self.version = ""

    def redact(self, text):
        for value in (self.root_password, self.password, journey.PASSWORD):
            text = text.replace(value, "<redacted>")
        return text

    def sql(self, statement, *, selected=True, root=True, timeout=60):
        require(self.port is not None, "disposable DB endpoint is unavailable")
        env = environment()
        env["MYSQL_PWD"] = self.root_password if root else self.password
        command = [self.client, "--protocol=tcp", "--host=127.0.0.1",
                   f"--port={self.port}", "--user=" + ("root" if root else self.user),
                   "--batch", "--skip-column-names", "--raw"]
        if selected:
            command.append(self.database)
        result = subprocess.run(command, input=statement, text=True, capture_output=True,
                                env=env, timeout=timeout)
        require(result.returncode == 0,
                "disposable SQL query failed: " + self.redact(result.stderr[-5000:]))
        return result.stdout.strip()

    def start(self):
        from disposable_sql_fixture import private_network
        network = private_network()
        server_arguments = ["--innodb-use-native-aio=OFF"]
        env = environment()
        variables = {self.prefix + "_ROOT_PASSWORD": self.root_password,
                     self.prefix + "_USER": self.user,
                     self.prefix + "_PASSWORD": self.password,
                     self.prefix + "_DATABASE": self.database}
        env.update(variables)
        command = [self.docker, "run", "--pull=never", "--rm", "-d", "--name", self.container,
                   "--cpus", "2", "--memory", "2g"]
        if network:
            with socket.socket() as probe:
                probe.bind(("127.0.0.1", 0))
                self.port = probe.getsockname()[1]
            command += ["--network", network]
            server_arguments += ["--port=" + str(self.port), "--bind-address=127.0.0.1"]
        else:
            command += ["-p", "127.0.0.1::3306"]
        for key in variables:
            command.extend(("-e", key))
        result = subprocess.run(command + [self.image, *server_arguments], env=env, text=True,
                                capture_output=True, timeout=60)
        require(result.returncode == 0, "disposable DB start failed: " +
                self.redact(result.stderr[-5000:]))
        self.started = True
        mapping = "127.0.0.1:" + str(self.port) if network else subprocess.check_output(
            [self.docker, "port", self.container, "3306/tcp"], text=True, timeout=30).strip()
        require(re.fullmatch(r"127\.0\.0\.1:\d+", mapping),
                f"DB publication was not exclusively loopback: {mapping!r}")
        published_port = int(mapping.rsplit(":", 1)[1])
        docker_host = "127.0.0.1"
        if docker_host == "127.0.0.1":
            self.port = published_port
        else:
            self.forward = LoopbackTcpForward(docker_host, published_port)
            self.port = self.forward.port
        deadline, last_error = time.monotonic() + 90, "no query attempted"
        while True:
            try:
                if self.sql("SELECT 1", selected=False, timeout=10) == "1":
                    break
            except (AssertionError, subprocess.TimeoutExpired) as error:
                last_error = str(error)
            if time.monotonic() >= deadline:
                logs = subprocess.run([self.docker, "logs", self.container], text=True,
                                      capture_output=True, timeout=30)
                raise AssertionError("DB readiness failed: " + self.redact(last_error) +
                                     "\n" + self.redact((logs.stdout + logs.stderr)[-5000:]))
            time.sleep(0.25)
        self.version = self.sql("SELECT VERSION()", selected=False)
        require(("MariaDB" in self.version) == self.image.startswith("mariadb:"),
                f"unexpected DB version: {self.version}")
        record("db-ready", image=self.image, version=self.version, published=mapping,
               cpus=2, memory="2g", container=self.container)

    def db_environment(self):
        return dict(environment(), ENVIRONMENT="local", DB_HOST="127.0.0.1",
                    DB_PORT=str(self.port), DB_NAME=self.database, DB_USER=self.user,
                    DB_PASSWD=self.password, DB_ALLOWED_TARGETS=f"127.0.0.1/{self.database}",
                    DB_TLS="FALSE")

    def prepare_schema(self):
        self.sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text(),
                 root=False, timeout=180)
        env = self.db_environment()
        for arguments in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            result = subprocess.run(["python3", "scripts/migration_runner.py", *arguments],
                                    cwd=ROOT, env=env, text=True, capture_output=True, timeout=240)
            require(result.returncode == 0, "schema setup failed: " +
                    self.redact((result.stdout + result.stderr)[-6000:]))
        require(self.sql("SELECT COUNT(*) FROM economic_lineage_state "
                         "WHERE active_epoch IS NOT NULL") == "0", "fixture activated an epoch")
        record("schema-ready", accounting_scope="inactive-legacy-SQL; no epoch activation")

    def owners(self):
        result = self.sql("SELECT COALESCE(IS_USED_LOCK(CONCAT('" + PLAYER_LOCK_PREFIX +
                          "',DATABASE())),0),COALESCE(IS_USED_LOCK('" + LIFECYCLE_LOCK + "'),0)")
        return tuple(int(value) for value in result.split("\t"))

    def sessions(self):
        rows = self.sql("SELECT ID FROM information_schema.PROCESSLIST WHERE USER='" +
                        self.user + "' AND DB=DATABASE() ORDER BY ID")
        return tuple(int(row) for row in rows.splitlines() if row)

    def owner_sessions(self, previous=None):
        deadline = time.monotonic() + 45
        while True:
            owners, sessions = self.owners(), self.sessions()
            if (len(owners) == 2 and owners[0] > 0 and owners[1] > 0 and
                    owners[0] != owners[1] and set(owners) <= set(sessions) and
                    (previous is None or set(owners).isdisjoint(previous))):
                return owners
            require(time.monotonic() < deadline,
                    f"authority unavailable: owners={owners}, sessions={sessions}, previous={previous}")
            time.sleep(0.1)

    def old_sessions(self, ids):
        result = self.sql("SELECT ID FROM information_schema.PROCESSLIST WHERE ID IN (" +
                          ",".join(str(i) for i in ids) + ") ORDER BY ID")
        return tuple(int(row) for row in result.splitlines() if row)

    def no_game_sessions(self):
        deadline = time.monotonic() + 10
        while True:
            owners, sessions = self.owners(), self.sessions()
            if owners == (0, 0) and not sessions:
                return
            require(time.monotonic() < deadline,
                    f"SQL sessions survived game stop: owners={owners}, sessions={sessions}")
            time.sleep(0.1)

    def close(self):
        try:
            if self.forward is not None:
                self.forward.close()
        finally:
            if self.started:
                subprocess.run([self.docker, "rm", "-f", self.container], text=True,
                               capture_output=True, timeout=60)
                result = subprocess.run([self.docker, "inspect", self.container], text=True,
                                        capture_output=True, timeout=30)
                require(result.returncode != 0 and "No such" in result.stderr + result.stdout,
                        f"disposable container removal not verified: {self.container}")
                record("cleanup", container=self.container, absent=True)
                self.started = False


def character_state(db, pid) -> dict:
    def rows(query):
        return tuple(tuple(line.split("\t")) for line in db.sql(query).splitlines() if line)
    wallet = tuple(int(value) for value in db.sql(
        f"SELECT copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid={pid}").split("\t"))
    require(len(wallet) == 5, "player wallet row missing")
    return dict(
        wallet=wallet,
        custody=rows("SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,"
                     "owner_id,owner_context_id,item_revision,vnum,state,"
                     "COALESCE(HEX(coin_payload),'NULL') FROM item_current_owner "
                     f"WHERE owner_type=1 AND owner_id={pid} AND owner_context_id=0 "
                     "AND state=1 ORDER BY item_uid"),
        items=rows("SELECT obj_uid,vnum,equip_slot,COALESCE(container_id,0),quantity "
                   f"FROM player_items WHERE pid={pid} ORDER BY obj_uid"),
        owner_revision=db.sql("SELECT COALESCE(MAX(revision),0) FROM item_owner_revision "
                              f"WHERE owner_type=1 AND owner_id={pid} AND owner_context_id=0"),
        currency_ledger=rows("SELECT HEX(operation_id),wallet_revision,wallet_after_copper,"
                             "wallet_after_silver,wallet_after_gold,wallet_after_platinum "
                             f"FROM currency_ledger WHERE pid={pid} ORDER BY wallet_revision"),
        item_ledger=rows("SELECT HEX(operation_id),event_index,item_uid,item_revision "
                        f"FROM item_ownership_ledger WHERE to_owner_type=1 AND to_owner_id={pid} "
                        "ORDER BY operation_id,event_index"),
        active_epochs=db.sql("SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL"))


def state_digest(state):
    return hashlib.sha256(json.dumps(state, sort_keys=True).encode()).hexdigest()


def visible_state(client, state):
    client.send("inventory")
    client.expect("a banana", timeout=20)
    start = len(client.transcript)
    client.send("score")
    client.expect("Coins carried:", timeout=20)
    client.expect("Coins in bank:", timeout=20)
    text = client.transcript[start:].decode(errors="replace")
    match = re.search(r"Coins carried:\s*(\d+) platinum\s+(\d+) gold\s+(\d+) silver\s+(\d+) copper", text)
    require(match and tuple(map(int, match.groups())) == state["wallet"][:4][::-1],
            f"live wallet differs from SQL: expected={state['wallet'][:4]}, output={text[-3000:]}")


def make_runtime(runtime, binary, db):
    journey.make_fixture(runtime, reset_coins=True)
    zone = runtime / "areas_mini/mini.zon"
    content = zone.read_text()
    header = "29999 0 0 6 11 1"
    require(content.count(header) == 1 and content.count("\nS\n") == 1, "world fixture drift")
    end = content.index(header) + len(header)
    # No NPCs/scavengers; ordinary room pickups establish real currency/custody.
    zone.write_text(content[:end] + "\nO 0 3 1 22800 100 0 0 0 * wallet pickup\n"
                    "O 0 15 1 22800 100 0 0 0 * durable item pickup\nS\n$~\n")
    journey.generate_certificate(runtime)
    (runtime / "logs/log").mkdir(parents=True)
    (runtime / "logs/log/.gitignore").write_text("*\n")
    for name in ("players", "critical"):
        (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
    (runtime / "bin/server").mkdir(parents=True)
    for name in ("dms", "dms_new"):
        destination = runtime / "bin/server" / name
        shutil.copy2(binary, destination)
        destination.chmod(0o755)
        require(sha256_file(destination) == sha256_file(binary), "staged binary hash differs")
    (runtime / "copyover-state").mkdir(mode=0o700)
    port, tls, websocket = journey.available_ports()
    env = dict(db.db_environment(), PERSISTENCE_MODE="mariadb-primary", REDIS="FALSE",
               CHAOS_MUD="FALSE", LISTEN_ADDRESS="127.0.0.1", DURIS_TLS_PORT=str(tls),
               DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", DURIS_WEBSOCKET_PORT=str(websocket),
               PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
               CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
               COPYOVER_STATE_FILE=str(runtime / "copyover-state/copyover.dat"))
    return port, env


def run(binary, expected_sha256):
    binary = Path(binary).resolve(strict=True)
    require(sha256_file(binary) == expected_sha256, "frozen DB binary hash mismatch")
    db = DisposableMariaDB()
    with tempfile.TemporaryDirectory(prefix="duris-pa-copyover-") as temporary:
        runtime = Path(temporary) / "runtime"
        process = client = output = None
        pid = None
        phase = "db-start"
        try:
            db.start()
            db.prepare_schema()
            runtime.mkdir()
            port, env = make_runtime(runtime, binary, db)
            output_path = runtime / "server.out"
            output = output_path.open("w")

            def logs():
                return output_path.read_text(errors="replace") + "\n" + journey.runtime_logs(runtime)

            def boot():
                nonlocal process
                offset = output_path.stat().st_size
                process = subprocess.Popen(
                    [str(runtime / "bin/server/dms"), "--minimal", "-s", "-d", str(runtime), str(port)],
                    cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
                deadline = time.monotonic() + 120
                while "Entering game loop." not in output_path.read_text(errors="replace")[offset:]:
                    require(process.poll() is None and time.monotonic() < deadline,
                            "game boot failed:\n" + logs()[-10000:])
                    time.sleep(0.1)
                require(sha256_file(Path(f"/proc/{process.pid}/exe")) == expected_sha256,
                        "executing game is not the frozen binary")
                return db.owner_sessions()

            def stop():
                nonlocal process
                if process is not None and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=15)
                        raise AssertionError("game did not stop gracefully")
                process = None

            def save():
                assert client is not None
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=45)

            def logout():
                nonlocal client
                assert client is not None
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=30)
                client.send("0")
                client.close()
                client = None

            phase = "initial-boot"
            first_owners = boot()
            client = journey.MudClient(port)
            journey.create_character(client)
            client.send("wield mace")
            client.expect("You wield", timeout=15)
            client.send("drop all")
            client.expect("You drop a steel long sword.", timeout=20)
            client.send("get coins")
            client.expect("You get 3s.", timeout=30)
            client.send("get banana")
            client.expect("get a banana", timeout=30)
            save()
            pid = int(db.sql("SELECT pid FROM player_data WHERE name='" + journey.CHARACTER +
                            "' AND account_name='" + journey.ACCOUNT + "'"))
            baseline = character_state(db, pid)
            require(baseline["wallet"][:4] == (0, 3, 0, 0), "real pickup did not persist 3 silver")
            require(baseline["wallet"][4] > 0 and baseline["currency_ledger"], "missing coin ledger")
            banana = [row for row in baseline["custody"] if row[7] == "15"]
            require(len(banana) == 1 and any(row[0] == banana[0][0] and row[1] == "15"
                                           for row in baseline["items"]),
                    "banana was not durably projected with matching active custody")
            require(baseline["active_epochs"] == "0", "active accounting is outside this fixture")
            visible_state(client, baseline)
            digest = state_digest(baseline)
            record("prepared", authority=first_owners, state_sha256=digest,
                   wallet=baseline["wallet"], banana_uid=banana[0][0], items=len(baseline["items"]))

            # Prove the independent cold restart before the deliberately risky exec.
            phase = "cold-restart"
            logout()
            stop()
            db.no_game_sessions()
            restart_owners = boot()
            require(set(first_owners).isdisjoint(restart_owners), "cold restart reused SQL sessions")
            client = journey.reconnect_character(port)
            visible_state(client, baseline)
            save()
            require(character_state(db, pid) == baseline, "cold restart changed item/currency/custody")
            record(phase, status="PASS", authority=restart_owners, state_sha256=digest,
                   old_sessions_absent=not db.old_sessions(first_owners), fresh_login=True)

            phase = "failed-exec"
            staged = runtime / "bin/server/dms_new"
            staged.write_bytes(b"not an executable image\n")
            staged.chmod(0o755)
            assert process is not None
            process.send_signal(signal.SIGUSR1)
            client.expect("Copyover FAILED!", timeout=90)
            require(process.poll() is None, "old process exited after failed exec")
            require("copyover: execl failed:" in logs(), "failure did not reach actual execl")
            require(db.owners() == restart_owners and db.old_sessions(restart_owners) == tuple(sorted(restart_owners)),
                    "failed exec surrendered old SQL authority")
            client.send("look")
            client.expect("The Regression Arena", timeout=20)
            visible_state(client, baseline)
            save()
            require(character_state(db, pid) == baseline, "failed exec changed item/currency/custody")
            record(phase, status="PASS", authority=db.owners(), state_sha256=digest,
                   old_process_live=True, old_sessions_live=True, save_acknowledged=True)

            phase = "successful-copyover"
            shutil.copy2(binary, staged)
            staged.chmod(0o755)
            require(sha256_file(staged) == expected_sha256, "replacement stage differs from frozen artifact")
            socket_identity = (client.socket.fileno(), client.socket.getsockname(), client.socket.getpeername())
            process_id = process.pid
            process.send_signal(signal.SIGUSR1)
            client.expect("Copyover complete!", timeout=120)
            require(process.poll() is None and process.pid == process_id, "exec lost process continuity")
            require(sha256_file(Path(f"/proc/{process.pid}/exe")) == expected_sha256,
                    "replacement is not executing the frozen binary")
            owners = db.owner_sessions(previous=restart_owners)
            require(not db.old_sessions(restart_owners), "successful exec retained old SQL owner sessions")
            require(socket_identity == (client.socket.fileno(), client.socket.getsockname(), client.socket.getpeername()),
                    "game TCP socket was replaced rather than inherited")
            require(not Path(env["COPYOVER_STATE_FILE"]).exists(), "copyover state file was not consumed")
            client.send("look")
            client.expect("The Regression Arena", timeout=20)
            visible_state(client, baseline)
            save()
            require(character_state(db, pid) == baseline, "copyover changed item/currency/custody")
            record(phase, status="PASS", authority_before=restart_owners, authority_after=owners,
                   state_sha256=digest, same_game_socket=True, old_sql_sessions_closed=True,
                   binary_sha256=expected_sha256, save_acknowledged=True)
            # The gameplay socket stays live through copyover and the read/save
            # assertions above cover the user-visible journey.  Close the test
            # client directly so teardown does not require account-menu state,
            # which copyover currently restores from character_name (see handoff).
            assert client is not None
            client.close()
            client = None
            stop()
            db.no_game_sessions()
        except Exception as error:
            record("failure", failed_phase=phase, error=db.redact(str(error)))
            if db.started and db.port:
                try:
                    record("failure-db", authority=db.owners(), sessions=db.sessions(),
                           state=character_state(db, pid) if pid is not None else None)
                except Exception as probe_error:
                    print("failure DB probe: " + db.redact(str(probe_error)), flush=True)
            if client is not None:
                print("--- synthetic client output ---\n" + db.redact(
                    client.transcript.decode(errors="replace")[-6000:]), flush=True)
            if (runtime / "server.out").exists():
                print("--- server output ---\n" + db.redact(
                    (runtime / "server.out").read_text(errors="replace")[-12000:]), flush=True)
                print("--- runtime logs ---\n" + db.redact(journey.runtime_logs(runtime)), flush=True)
            raise
        finally:
            if client is not None:
                client.close()
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=15)
            if output is not None:
                output.close()
            db.close()
    require(not Path(temporary).exists(), "disposable runtime workspace survived cleanup")
    record("cleanup", runtime_workspace_absent=True)
