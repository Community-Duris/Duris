#!/usr/bin/env python3
"""Real authenticated transports, world exec, gameplay and failure reconciliation.

Every world, account, key and persistence directory is disposable. The supplied
binary must support flatfile-primary mode; no operator credentials are read.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import socket
import ssl
import struct
import subprocess
import sys
import tempfile
import time
import zlib

import test_flatfile_combat_journey as journey
from run_copyover_runtime_journey import Client as CompressedClient

ROOT = Path(__file__).resolve().parents[2]


def copy_binary(source, destination):
    # DrvFS sendfile can reserve the entire debug executable under memory
    # pressure; keep fixture staging bounded as well as the transport queues.
    with Path(source).open("rb") as reader, Path(destination).open("wb") as writer:
        shutil.copyfileobj(reader, writer, length=1024 * 1024)
    shutil.copystat(source, destination)


class TelnetClient(CompressedClient):
    def __init__(self, port):
        super().__init__(port)
        self.telnet_wire = bytearray()
        self.gmcp_messages = []

    def _receive(self):
        pending_at, transcript_at = len(self.pending), len(self.transcript)
        received = super()._receive()
        self.telnet_wire.extend(self.transcript[transcript_at:])
        del self.pending[pending_at:]; del self.transcript[transcript_at:]
        plain = bytearray(); at = 0
        while at < len(self.telnet_wire):
            if self.telnet_wire[at] != 255:
                plain.append(self.telnet_wire[at]); at += 1; continue
            if len(self.telnet_wire) - at < 2: break
            command = self.telnet_wire[at + 1]
            if command in (251, 252, 253, 254):
                if len(self.telnet_wire) - at < 3: break
                at += 3
            elif command == 250:
                end = self.telnet_wire.find(b"\xff\xf0", at + 2)
                if end < 0: break
                if self.telnet_wire[at + 2] == 201:
                    self.gmcp_messages.append(bytes(self.telnet_wire[at + 3:end]).decode())
                at = end + 2
            else:
                if command == 255: plain.append(255)
                at += 2
        del self.telnet_wire[:at]
        self.pending.extend(plain); self.transcript.extend(plain)
        return received


class TLSClient(TelnetClient):
    def __init__(self, port):
        super().__init__(port)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE  # Disposable, generated local keypair.
        self.socket = context.wrap_socket(self.socket, server_hostname="localhost")
        self.socket.settimeout(0.25)


class WebClient(journey.MudClient):
    def __init__(self, port):
        super().__init__(port)
        self.wire = bytearray()
        self.compressed_frames = 0
        self.received = []
        self.socket.sendall(
            b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n"
            b"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n"
            b"Sec-WebSocket-Extensions: permessage-deflate\r\n\r\n")
        deadline = time.monotonic() + 15
        while b"\r\n\r\n" not in self.wire:
            assert time.monotonic() < deadline
            try: self.wire.extend(self.socket.recv(65536))
            except socket.timeout: continue
        end = self.wire.index(b"\r\n\r\n") + 4
        response = bytes(self.wire[:end]); del self.wire[:end]
        assert b" 101 " in response and b"permessage-deflate" in response

    def frame(self, payload, opcode=1, compressed=False, fin=True):
        mask = os.urandom(4)
        first = (0x80 if fin else 0) | (0x40 if compressed else 0) | opcode
        n = len(payload)
        header = bytes((first, 0x80 | n)) if n < 126 else bytes((first, 0xFE)) + struct.pack("!H", n) if n <= 65535 else bytes((first, 0xFF)) + struct.pack("!Q", n)
        return header + mask + bytes(value ^ mask[i % 4] for i, value in enumerate(payload))

    def message(self, cmd, data):
        payload = json.dumps(dict(type="cmd", cmd=cmd, data=data), separators=(",", ":")).encode()
        deflater = zlib.compressobj(wbits=-15)
        compressed = deflater.compress(payload) + deflater.flush(zlib.Z_SYNC_FLUSH)
        self.socket.sendall(self.frame(compressed[:-4], compressed=True))

    def send(self, line):
        self.message("game", dict(command=line))

    def _receive(self):
        try: chunk = self.socket.recv(65536)
        except socket.timeout: chunk = b""
        else: assert chunk, "original WebSocket connection closed"
        self.wire.extend(chunk)
        consumed = False
        while len(self.wire) >= 2:
            first, second = self.wire[:2]; size = second & 127; offset = 2
            if size == 126:
                if len(self.wire) < 4: break
                size = struct.unpack("!H", self.wire[2:4])[0]; offset = 4
            elif size == 127:
                if len(self.wire) < 10: break
                size = struct.unpack("!Q", self.wire[2:10])[0]; offset = 10
            assert not second & 0x80 and size <= 4 * 1024 * 1024
            if len(self.wire) < offset + size: break
            payload = bytes(self.wire[offset:offset + size]); del self.wire[:offset + size]
            opcode = first & 15; consumed = True
            if opcode == 9:
                self.socket.sendall(self.frame(payload, 10)); continue
            if opcode == 10:
                self.received.append(dict(pong=payload.decode())); continue
            assert opcode != 8, "original WebSocket received close"
            if opcode != 1: continue
            if first & 0x40:
                payload = zlib.decompressobj(-15).decompress(payload + b"\x00\x00\xff\xff")
                self.compressed_frames += 1
            value = json.loads(payload); self.received.append(value)
            text = value.get("data", "") if value.get("type") in ("text", "system") else ""
            if isinstance(text, dict): text = text.get("message", "")
            if not isinstance(text, str): text = ""
            clean = journey.ANSI.sub(b"", text.encode())
            self.pending.extend(clean); self.transcript.extend(clean)
        return consumed or bool(chunk)

    def expect_type(self, kind, status=None, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for i, value in enumerate(self.received):
                if value.get("type") == kind and (status is None or value.get("status", value.get("action")) == status):
                    return self.received.pop(i)
            self._receive()
        raise AssertionError(f"missing WebSocket response {kind}")


class Fixture:
    def __init__(self, binary, injector):
        self.temporary = tempfile.TemporaryDirectory(prefix="duris-transport-")
        self.root = Path(self.temporary.name)
        self.state, self.runtime = self.root / "state", self.root / "runtime"
        self.state.mkdir(mode=0o700); (self.state / "domains").mkdir(mode=0o700)
        self.runtime.mkdir(); (self.runtime / "logs/log").mkdir(parents=True)
        (self.runtime / "logs/log/.gitignore").write_text("*\n")
        journey.make_fixture(self.runtime); journey.generate_certificate(self.runtime)
        for name in ("players", "critical"):
            (self.runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        (self.runtime / "bin/server").mkdir(parents=True)
        for name in ("dms", "dms_new"): copy_binary(binary, self.runtime / "bin/server" / name)
        self.copyover = self.root / "handoff/copyover.dat"
        self.ports = journey.available_ports()
        self.env = dict(PATH=os.environ.get("PATH", "/usr/bin:/bin"), ENVIRONMENT="local",
                        PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(self.state),
                        PLAYER_SAVE_JOURNAL_DIR=str(self.runtime / "journals/players"),
                        CRITICAL_COMMAND_JOURNAL_DIR=str(self.runtime / "journals/critical"),
                        COPYOVER_STATE_FILE=str(self.copyover), LISTEN_ADDRESS="127.0.0.1",
                        DURIS_TLS_PORT=str(self.ports[1]), DURIS_WEBSOCKET_PORT=str(self.ports[2]),
                        DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1", REDIS="FALSE", CHAOS_MUD="FALSE",
                        LD_PRELOAD=str(injector), DURIS_TEST_TRANSPORT_FAULT="delay,duplicate")
        if "LD_LIBRARY_PATH" in os.environ: self.env["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        subprocess.run([str(journey.INSPECTOR), str(self.state), "seed-combat"], check=True)
        self.process = None; self.output = None; self.clients = []

    def boot(self, relative_directory=False, wait=True, watchdog=False):
        self.output = (self.runtime / "server.out").open("w")
        arguments = [str(self.runtime / "bin/server/dms"), "--persistent-transport", "--minimal", "-s"]
        if relative_directory: arguments.extend(("-d", "runtime"))
        arguments.append(str(self.ports[0]))
        self.watchdog = watchdog
        if watchdog:
            arguments = [sys.executable, str(ROOT / "scripts/game_loop_watchdog.py"), "--", *arguments]
        self.process = subprocess.Popen(arguments,
                                        cwd=self.root if relative_directory else self.runtime,
                                        env=self.env, stdout=self.output,
                                        stderr=subprocess.STDOUT)
        if wait: self.wait_boots(1)

    def wait_boots(self, count):
        deadline = time.monotonic() + 90
        while (self.runtime / "server.out").read_text(errors="replace").count("Entering game loop.") < count:
            assert self.process.poll() is None and time.monotonic() < deadline, "world failed to boot"
            time.sleep(0.05)

    def frontend_pid(self):
        if not self.watchdog:
            return self.process.pid
        children = Path(f"/proc/{self.process.pid}/task/{self.process.pid}/children").read_text().split()
        assert len(children) == 1, "watchdog must own exactly one transport supervisor"
        return int(children[0])

    def world_pid(self):
        frontend = self.frontend_pid()
        children = Path(f"/proc/{frontend}/task/{frontend}/children").read_text().split()
        assert len(children) == 1, "frontend must own exactly one world child"
        return int(children[0])

    def wait_healthy(self, timeout=90):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            assert self.process.poll() is None, "frontend exited during cold recovery"
            try:
                with socket.create_connection(("127.0.0.1", self.ports[2]), timeout=1) as probe:
                    probe.sendall(b"GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n")
                    if b"200 OK" in probe.recv(4096): return
            except OSError: pass
            time.sleep(0.1)
        raise AssertionError("cold world did not become healthy")

    def stop(self):
        for client in self.clients: client.close()
        self.clients.clear()
        if self.process and self.process.poll() is None:
            self.process.terminate(); self.process.wait(timeout=45)
        if self.output: self.output.close()

    def close(self):
        self.stop(); self.temporary.cleanup()

    def seed_player(self):
        self.boot(); client = TelnetClient(self.ports[0]); self.clients.append(client)
        journey.create_character(client)
        client.send("save"); client.expect(f"Save complete for {journey.CHARACTER}.")
        client.send("quit"); client.expect("ACCOUNT MENU", timeout=30)
        self.stop(); self.boot()

    def login(self, protocol, account=journey.ACCOUNT, character=journey.CHARACTER):
        if protocol == "websocket":
            client = WebClient(self.ports[2]); self.clients.append(client)
            client.expect("Welcome")
            client.message("login", dict(account=account, password=journey.PASSWORD))
            client.expect_type("auth", "success")
            client.message("enter", dict(character=character))
            client.expect("The Regression Arena", timeout=30)
        else:
            cls = TLSClient if protocol == "tls" else TelnetClient
            port = self.ports[1] if protocol == "tls" else self.ports[0]
            original = journey.MudClient; journey.MudClient = cls
            try: client = journey.reconnect_character(port, account=account, character=character)
            finally: journey.MudClient = original
            self.clients.append(client)
            client.enable_compression()
        return client

    def diagnostics(self, client=None):
        print((self.runtime / "server.out").read_text(errors="replace")[-8000:])
        print(journey.runtime_logs(self.runtime))
        status = self.runtime / "logs/log/status"
        if status.exists():
            print("\n".join(line for line in status.read_text(errors="replace").splitlines()
                            if "Persistent" in line or "copyover:" in line))
        if client: print(client.transcript.decode(errors="replace")[-6000:])


def run_protocol(binary, injector, protocol):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.seed_player(); client = fixture.login(protocol)
        original_socket = client.socket.fileno(); original_world = fixture.world_pid()
        if protocol != "websocket":
            client.socket.sendall(b"\xff\xfd\xc9")
        client.send("inventory"); client.expect("a small wooden mace")
        client.send("wield mace"); client.expect("You wield")
        client.send("say beforeboundary"); client.expect("beforeboundary")
        before = client.transcript.count(b"beforeboundary")
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover FAILED", timeout=30)  # Missing handoff directory.
        assert fixture.world_pid() == original_world and not fixture.copyover.exists()
        client.send("look"); client.expect("The Regression Arena")
        if protocol != "websocket": assert client.inflater and not client.inflater.eof
        fixture.copyover.parent.mkdir(mode=0o700)
        idle = TelnetClient(fixture.ports[0]); fixture.clients.append(idle)
        entry, _ = idle.expect_any(("account name", "term type"))
        if entry == "term type":
            idle.send("9"); idle.expect("account name")
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("a connection cannot survive this handoff", timeout=30)
        idle.send(journey.ACCOUNT); idle.expect("enter your password", timeout=10)
        idle.close(); fixture.clients.remove(idle)
        time.sleep(0.2)
        # A rejected exec must roll the quiescence barrier back onto the live world.
        staged = fixture.runtime / "bin/server/dms_new"
        staged.write_bytes(b"invalid executable\n"); staged.chmod(0o755)
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover FAILED!", timeout=30)
        client.send("look"); client.expect("The Regression Arena")
        copy_binary(binary, fixture.runtime / "bin/server/dms")
        copy_binary(binary, staged)
        client.send("rest"); client.expect("You sit down and relax.")
        if protocol == "websocket":
            payload = json.dumps(dict(type="cmd", cmd="game", data=dict(command="say fragmentedboundary"))).encode()
            deflater = zlib.compressobj(wbits=-15)
            fragment = (deflater.compress(payload) + deflater.flush(zlib.Z_SYNC_FLUSH))[:-4]
            midpoint = len(fragment) // 2
            client.socket.sendall(client.frame(fragment[:midpoint], compressed=True, fin=False))
        else:
            # A partial GMCP subnegotiation remains in the parent's Telnet parser across exec.
            client.socket.sendall(b'\xff\xfa\xc9Core.Hello {"client":"DurisTransport","version":"1"')
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover in progress", timeout=30)
        if protocol == "websocket":
            client.socket.sendall(client.frame(fragment[midpoint:], opcode=0))
        else:
            client.socket.sendall(b'}\xff\xf0')
        compressed_before = getattr(client, "compressed_frames", 0)
        for index in range(12): client.send(f"say heldsequence{index:02d}")
        if protocol == "websocket":
            # Framing and control handling continue while the world HELLO is delayed.
            client.socket.sendall(client.frame(b"duringreplacement", 9))
            fresh = WebClient(fixture.ports[2]); fixture.clients.append(fresh)
            fresh.expect("Welcome")
            fresh.message("login", dict(account=journey.ACCOUNT, password="invalid-test-password"))
        client.expect("Copyover complete!", timeout=90)
        if protocol == "websocket": client.expect("fragmentedboundary", timeout=30)
        for index in range(12): client.expect(f"heldsequence{index:02d}", timeout=30)
        client.send("look"); client.expect("The Regression Arena")
        client.send("equipment"); client.expect("a small wooden mace")
        client.send("rest"); client.expect("You are already resting.")
        client.send("protocol")
        client.expect("WebSocket" if protocol == "websocket" else "SSL/TLS" if protocol == "tls" else "Telnet")
        if protocol != "websocket": client.expect("Enabled (v2)")
        client.send("save"); client.expect(f"Save complete for {journey.CHARACTER}.", timeout=30)
        assert client.socket.fileno() == original_socket and fixture.world_pid() == original_world
        fixture.wait_boots(2)
        assert not fixture.copyover.exists()
        assert client.transcript.count(b"beforeboundary") == before == 1
        positions = []
        for index in range(12):
            marker = f"heldsequence{index:02d}".encode()
            assert client.transcript.count(marker) == 1, "duplicate execution or duplicate output"
            positions.append(client.transcript.index(marker))
        assert positions == sorted(positions), "held commands reordered"
        if protocol == "websocket":
            assert client.compressed_frames > compressed_before
            assert any(value.get("pong") == "duringreplacement" for value in client.received)
            # A fresh login from the same IP must not retire the authenticated old socket.
            fresh.expect_type("auth", "failed")
            client.send("look"); client.expect("The Regression Arena")
            fresh.message("login", dict(account=journey.ACCOUNT, password=journey.PASSWORD))
            fresh.expect_type("auth", "reconnected")
            fresh.message("enter", dict(character=journey.CHARACTER))
            fresh.send("look"); fresh.expect("The Regression Arena")
            client = fresh
        else:
            assert client.inflater is not None and not client.inflater.eof and client.compressed_bytes > 0
            assert client.gmcp_messages, "GMCP application framing was not preserved"
        # An ordinary backend crash cannot replay an uncertain arbitrary command.
        os.kill(fixture.world_pid(), signal.SIGKILL)
        deadline = time.monotonic() + 15
        closed = False
        while time.monotonic() < deadline:
            try: data = client.socket.recv(65536)
            except (ConnectionError, ssl.SSLError): closed = True; break
            except socket.timeout: continue
            if not data: closed = True; break
        assert closed, "backend failure left an unauthenticated zombie session"
        fixture.wait_healthy()
        assert fixture.world_pid() != original_world
        recovered = fixture.login(protocol)
        recovered.send("equipment"); recovered.expect("a small wooden mace")
        recovered.send("save"); recovered.expect(f"Save complete for {journey.CHARACTER}.")
        print(f"{protocol}: authenticated exec, compression, framing, order, deduplication, state and failure passed", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_frontend_failure(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.seed_player(); fixture.stop()
        fixture.env["DURIS_WATCHDOG_SHUTDOWN_SECONDS"] = "30"
        fixture.boot(watchdog=True)
        client = fixture.login("tls"); world_pid = fixture.world_pid()
        client.send("wield mace"); client.expect("You wield")
        os.kill(fixture.frontend_pid(), signal.SIGKILL)
        assert fixture.process.wait(timeout=40) == 137
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            status = Path(f"/proc/{world_pid}/stat")
            if not status.exists() or status.read_text().split()[2] == "Z": break
            time.sleep(0.05)
        else: raise AssertionError("world failed to stop and save after frontend death")
        fixture.stop(); fixture.boot()
        client = fixture.login("tls")
        client.send("equipment"); client.expect("a small wooden mace")
        client.send("save"); client.expect(f"Save complete for {journey.CHARACTER}.")
        print("frontend failure: real launcher allowed IPC-loss persistence shutdown and authenticated recovery", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_disconnect_race(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.seed_player(); client = fixture.login("tls")
        fixture.copyover.parent.mkdir(mode=0o700)
        client.send("wield mace"); client.expect("You wield")
        world = fixture.world_pid()
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover in progress", timeout=30)
        client.close(); fixture.clients.remove(client)
        fresh = WebClient(fixture.ports[2]); fixture.clients.append(fresh)
        fresh.expect("Welcome")
        fresh.message("login", dict(account=journey.ACCOUNT, password=journey.PASSWORD))
        auth = fresh.expect_type("auth", timeout=90)
        assert auth["status"] in ("success", "reconnected")
        if auth["status"] == "success":
            fresh.message("enter", dict(character=journey.CHARACTER))
        fresh.send("look"); fresh.expect("The Regression Arena")
        fresh.send("equipment"); fresh.expect("a small wooden mace")
        fresh.send("save"); fresh.expect(f"Save complete for {journey.CHARACTER}.")
        fixture.wait_boots(2)
        assert fixture.world_pid() == world
        print("disconnect/reconnect race: restored body reconciled with fresh authenticated transport", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_mixed_sessions(binary, injector):
    fixture = Fixture(binary, injector); clients = []
    players = (("telnet", journey.ACCOUNT, journey.CHARACTER),
               ("tls", "Transporttls", "Talerek"),
               ("websocket", "Transportweb", "Valerek"))
    try:
        fixture.boot()
        for _, account, character in players:
            seed = TelnetClient(fixture.ports[0]); fixture.clients.append(seed)
            journey.create_character(seed, account=account, character=character,
                                     email=account.lower() + "@example.invalid")
            seed.send("save"); seed.expect(f"Save complete for {character}.")
            seed.send("quit"); seed.expect("ACCOUNT MENU", timeout=30)
            seed.close(); fixture.clients.remove(seed)
        fixture.stop(); fixture.boot()
        fixture.copyover.parent.mkdir(mode=0o700)
        clients = [fixture.login(protocol, account, character) for protocol, account, character in players]
        world_pid = fixture.world_pid()
        sockets = [client.socket.fileno() for client in clients]
        for client in clients:
            client.send("wield mace"); client.expect("You wield")
        for replacement in range(2):
            fixture.process.send_signal(signal.SIGUSR1)
            for client in clients: client.expect("Copyover in progress", timeout=30)
            for index, client in enumerate(clients): client.send(f"say mixed{replacement}_{index}")
            for index, client in enumerate(clients):
                client.expect("Copyover complete!", timeout=90)
                marker = f"mixed{replacement}_{index}"
                client.expect(marker, timeout=30)
                assert client.transcript.count(marker.encode()) == 1
                client.send("equipment"); client.expect("a small wooden mace")
                client.send("save"); client.expect(f"Save complete for {players[index][2]}.")
                assert client.socket.fileno() == sockets[index]
            assert fixture.world_pid() == world_pid
        assert all(client.inflater is not None and not client.inflater.eof for client in clients[:2])
        assert clients[2].compressed_frames > 0
        fixture.wait_boots(3)
        print("mixed sessions: three distinct authenticated players survived two execs with identity, equipment and sequences intact", flush=True)
    except Exception:
        fixture.diagnostics(clients[0] if clients else None); raise
    finally: fixture.close()


def run_invalid_handoff(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.env["DURIS_TEST_TRANSPORT_FAULT"] = "corrupt-handoff"
        fixture.copyover.parent.mkdir(mode=0o700)
        fixture.seed_player(); client = fixture.login("tls")
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover in progress", timeout=30)
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            try:
                if not client.socket.recv(65536): break
            except (ConnectionError, ssl.SSLError): break
            except socket.timeout: continue
        else: raise AssertionError("untrusted handoff did not retire its sessions")
        fixture.wait_healthy()
        assert "authenticated transport handoff digest rejected" in journey.runtime_logs(fixture.runtime)
        assert b"Copyover complete!" not in client.transcript
        restored = fixture.login("tls")
        restored.send("inventory"); restored.expect("a small wooden mace")
        restored.send("save"); restored.expect(f"Save complete for {journey.CHARACTER}.")
        print("modified handoff: restoration rejected, clients retired and cold authenticated recovery passed", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_application_barrier(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.env["DURIS_TEST_TRANSPORT_FAULT"] += ",delay-pause"
        fixture.copyover.parent.mkdir(mode=0o700)
        fixture.seed_player(); client = fixture.login("websocket")
        fixture.process.send_signal(signal.SIGUSR1)
        deadline = time.monotonic() + 30
        while "transport-test: pause barrier delayed" not in (fixture.runtime / "server.out").read_text(errors="replace"):
            assert time.monotonic() < deadline
            time.sleep(0.01)
        # This account mutation completes in the IPC pump before PAUSED, without
        # creating a queued game command. It must be ACKed before committing.
        client.message("change_email", dict(newEmail="barrier@example.invalid"))
        client.expect("Copyover complete!", timeout=90)
        client.send("look"); client.expect("The Regression Arena")
        assert sum(value.get("type") == "account" and value.get("action") == "email_changed"
                   for value in client.received) == 1, "completed application mutation replayed"
        client.message("account_info", {})
        info = client.expect_type("account", "info")
        assert info["data"]["email"] == "barrier@example.invalid"
        print("application barrier: a completed account mutation was durable and not re-executed after exec", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_bounds(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.seed_player()
        # Large valid room text makes output pressure observable with ordinary look commands.
        fixture.stop()
        # Kernel autotuning varies by host; make real TCP backpressure repeatable.
        fixture.env["DURIS_TEST_TRANSPORT_FAULT"] += ",slow-output"
        world_file = fixture.runtime / "areas_mini/mini.wld"
        world_text = world_file.read_text()
        description = "This quiet stone arena exists to prove the complete combat journey."
        assert description in world_text
        world_file.write_text(world_text.replace(description,
                             "Bounded transport output " + os.urandom(16384).hex()))
        fixture.boot()
        client = journey.reconnect_character(fixture.ports[0]); fixture.clients.append(client)
        client.send("toggle brief off"); client.expect("Brief mode off.")
        client.send("toggle paging off"); client.expect("Paging mode off.")
        client.send("look room"); client.expect("Bounded transport output"); client.expect("<> ")
        client.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        def resident():
            return int(Path(f"/proc/{fixture.process.pid}/status").read_text().split("VmRSS:")[1].split()[0])
        before = resident()
        # Stop the backend while flooding commands: the frontend must stop reading at its watermark.
        backend = fixture.world_pid(); os.kill(backend, signal.SIGSTOP)
        client.socket.settimeout(2)
        try: client.socket.sendall(b"look room\n" * 100000)
        except socket.timeout: pass
        time.sleep(0.5)
        input_growth = resident() - before
        assert input_growth < 32768, "input flood grew frontend memory beyond its bounded queues"
        os.kill(backend, signal.SIGCONT)
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            try: data = client.socket.recv(65536)
            except ConnectionError: break
            except socket.timeout: continue
            if not data: break
        else: raise AssertionError("over-limit input flood was not retired")
        client.close(); fixture.clients.remove(client)
        # The flood leaves an existing linkdead body. Account selection performs
        # direct reconciliation, rather than the fresh-player confirmation path.
        client = TelnetClient(fixture.ports[0]); fixture.clients.append(client)
        entry, _ = client.expect_any(("term type", "account name"))
        if entry == "term type":
            client.send("9"); client.expect("account name")
        client.send(journey.ACCOUNT); client.expect("enter your password")
        client.send(journey.PASSWORD); client.expect("PRESS RETURN")
        client.send(""); client.expect("Please select an option")
        client.send("1"); client.expect(journey.CHARACTER)
        client.send("1")
        selected, _ = client.expect_any(("Play as", "Reconnecting"))
        if selected == "Play as": client.send("y")
        client.send("look"); client.expect("The Regression Arena")
        client.send("toggle brief off"); client.expect("Brief mode off.")
        client.send("toggle paging off"); client.expect("Paging mode off.")
        received_before = len(client.transcript)
        client.send("look room"); client.expect("Bounded transport output"); client.expect("<> ")
        assert len(client.transcript) - received_before >= 32768, "slow-reader fixture did not produce large output"
        client.socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        time.sleep(0.2)  # Let the toggle event's world ACK reach the frontend.
        client.socket.sendall(b"look room\n" * 64)
        # A slow reader is retired at the output cap while a health probe stays responsive.
        deadline = time.monotonic() + 40
        retired = False
        peak_growth = 0
        while time.monotonic() < deadline:
            assert fixture.process.poll() is None and fixture.world_pid() == backend
            peak_growth = max(peak_growth, resident() - before)
            assert peak_growth < 65536, "slow reader exceeded the global memory budget"
            status = (fixture.runtime / "logs/log/status").read_text(errors="replace")
            comm = fixture.runtime / "logs/log/comm"
            if comm.exists(): status += comm.read_text(errors="replace")
            if "Telnet output queue exceeded" in status:
                retired = True; break
            time.sleep(0.1)
        assert retired, "slow Telnet reader did not hit the output cap"
        with socket.create_connection(("127.0.0.1", fixture.ports[2]), timeout=3) as probe:
            probe.sendall(b"GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n")
            assert b"200 OK" in probe.recv(4096), "slow client stalled healthy sessions"
        print(f"slow client/input flood: bounded memory, backpressure and independent frontend health passed "
              f"(input growth {input_growth} KiB, output peak growth {peak_growth} KiB)", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def run_startup(binary, injector):
    fixture = Fixture(binary, injector)
    try:
        fixture.boot(relative_directory=True); fixture.wait_healthy()
        assert fixture.world_pid() != fixture.process.pid
        print("relative data directory: private world startup and clean shutdown passed", flush=True)
    except Exception:
        fixture.diagnostics(); raise
    finally: fixture.close()


def run_lifecycle_exit(binary, injector):
    fixture = Fixture(binary, injector)
    try:
        fixture.env["DURIS_TEST_TRANSPORT_FAULT"] = "lifecycle-exit"
        fixture.boot(wait=False)
        assert fixture.process.wait(timeout=30) == 55, "intentional lifecycle exit was restarted or lost"
        print("lifecycle exit: supervisor preserved the world's stop status without invoking wipe operations", flush=True)
    except Exception:
        fixture.diagnostics(); raise
    finally: fixture.close()


def run_lifecycle_failure(binary, injector):
    fixture = Fixture(binary, injector)
    try:
        fixture.env["DURIS_TEST_TRANSPORT_FAULT"] = "lifecycle-fail"
        fixture.boot(wait=False)
        assert fixture.process.wait(timeout=30) == 1, "cold startup failure was unbounded or reported as success"
        status = (fixture.runtime / "logs/log/status").read_text(errors="replace")
        assert status.count("Persistent transport cold world restart") == 3
        print("lifecycle failure: three cold-start retries exhausted with a failure status", flush=True)
    except Exception:
        fixture.diagnostics(); raise
    finally: fixture.close()


def run_watchdog(binary, injector):
    fixture = Fixture(binary, injector); client = None
    try:
        fixture.seed_player(); fixture.stop()
        for key, value in dict(startup=60, stall=2, copyover=15, shutdown=15, abort=2).items():
            fixture.env[f"DURIS_WATCHDOG_{key.upper()}_SECONDS"] = str(value)
        fixture.copyover.parent.mkdir(mode=0o700)
        fixture.boot(watchdog=True)
        frontend, world = fixture.frontend_pid(), fixture.world_pid()
        client = fixture.login("tls")
        original_socket = client.socket.fileno()
        client.send("wield mace"); client.expect("You wield")
        time.sleep(3)  # A transport heartbeat cannot substitute for completed world loops.
        assert fixture.process.poll() is None
        fixture.process.send_signal(signal.SIGUSR1)
        client.expect("Copyover complete!", timeout=90)
        client.send("equipment"); client.expect("a small wooden mace")
        client.send("save"); client.expect(f"Save complete for {journey.CHARACTER}.")
        assert client.socket.fileno() == original_socket and not client.inflater.eof
        assert (fixture.frontend_pid(), fixture.world_pid()) == (frontend, world)
        fixture.wait_boots(2)
        time.sleep(3)
        assert (fixture.runtime / "server.out").read_text().count("Entering game loop.") == 2, "lifecycle signal delivered twice"
        fixture.process.send_signal(signal.SIGRTMIN)
        deadline = time.monotonic() + 60
        while True:
            assert fixture.process.poll() is None and time.monotonic() < deadline
            try:
                if fixture.world_pid() != world:
                    break
            except (FileNotFoundError, AssertionError):
                pass  # The old child can exit before its cold successor is registered.
            time.sleep(0.05)
        fixture.wait_healthy()
        recovered = fixture.login("tls")
        recovered.send("equipment"); recovered.expect("a small wooden mace")
        time.sleep(3)
        assert fixture.process.poll() is None and fixture.frontend_pid() == frontend
        assert "deadline expired" not in (fixture.runtime / "server.out").read_text()
        stalled_world = fixture.world_pid()
        os.kill(stalled_world, signal.SIGSTOP)
        assert fixture.process.wait(timeout=20) == 56, "live frontend concealed a stalled world"
        evidence = (fixture.runtime / "server.out").read_text()
        assert f"pid={stalled_world} supervisor={frontend} phase=running deadline expired" in evidence
        print("launcher watchdog: world delegation, single lifecycle signals, exec, cold restart and stalled-world detection passed", flush=True)
    except Exception:
        fixture.diagnostics(client); raise
    finally: fixture.close()


def main():
    cases = dict(startup=run_startup, lifecycle_exit=run_lifecycle_exit,
                 lifecycle_failure=run_lifecycle_failure, mixed=run_mixed_sessions,
                 disconnect_race=run_disconnect_race, frontend_failure=run_frontend_failure,
                 invalid_handoff=run_invalid_handoff, application_barrier=run_application_barrier,
                 bounds=run_bounds, watchdog=run_watchdog)
    parser = argparse.ArgumentParser(); parser.add_argument("--server", type=Path)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--protocol", choices=("telnet", "tls", "websocket"))
    selection.add_argument("--case", choices=cases)
    args = parser.parse_args()
    build = ROOT / "bin/tests"; build.mkdir(parents=True, exist_ok=True)
    injector = build / "transport-faults.so"
    subprocess.run(["g++", "-std=c++20", "-shared", "-fPIC", "-Wall", "-Wextra", "-Werror",
                    "tests/async/transport_fault_injector.cpp", "-ldl", "-o", str(injector)], cwd=ROOT, check=True)
    if not journey.INSPECTOR.exists():
        journey.build_inspector()
    with tempfile.TemporaryDirectory(prefix="transport-server-", dir=build) as temporary:
        source = args.server.resolve() if args.server else journey.build_flatfile_server(Path(temporary))
        binary = Path(temporary) / "transport-server"
        copy_binary(source, binary)  # Freeze one build for the entire replacement matrix.
        protocols = [] if args.case else [args.protocol] if args.protocol else ("telnet", "tls", "websocket")
        for protocol in protocols:
            run_protocol(binary, injector, protocol)
        if args.case:
            cases[args.case](binary, injector)
        elif not args.protocol:
            for case in cases.values(): case(binary, injector)


if __name__ == "__main__": main()
