"""Disposable SQL/WebSocket fixture helpers for the S10 integration journey.

Creates only a fresh, loopback-published MariaDB container with a random name,
random database selector, and generated credentials. No checkout .env is read.
"""
from __future__ import annotations

from contextlib import contextmanager
import hashlib
import hmac
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
import struct
import subprocess
import tempfile
import threading
import time
import uuid

from pa_accounting_batch_artifact import load_base_build, report_base_build

ROOT = Path(__file__).resolve().parents[2]
DB_IMAGE = "mariadb:10.11"
DB_PREFIX = "corpse_journey_test_"
WEB_RECOVERY_SOURCE_INPUTS = (
    "src/net/ws_handlers.c",
    "src/net/websocket.c",
    "src/sql/sql_player.c",
    "src/account/account.c",
    "src/core/files.c",
    "src/player/player_death_conflict_repository.c",
)


def _clean_environment() -> dict[str, str]:
    environment = os.environ.copy()
    for key in (
        "DB_HOST", "DB_PORT", "DB_SOCKET", "DB_NAME", "DB_USER", "DB_PASSWD", "MYSQL_PWD",
        "DB_ALLOWED_TARGETS", "TEST_DB_DISPOSABLE", "DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY",
        "TEST_DB_HOST", "TEST_DB_PORT", "TEST_DB_USER", "TEST_DB_PASSWORD",
        "DURIS_ACCOUNTING_BASE_BUILD", "DURIS_ACCOUNTING_RESOURCE_ROOT",
        "DURIS_ACCOUNTING_DOCKER_HEAVY_LOCK",
    ):
        environment.pop(key, None)
    return environment


def verify_frozen_binary() -> Path:
    """Require the exact parent-frozen integration-batch artifact and source state."""
    build = load_base_build(ROOT, required_sources=WEB_RECOVERY_SOURCE_INPUTS)
    report_base_build(build, scope="S10 frozen binary execution")
    return build.binary


class _LoopbackForwarder:
    """Reach Docker Desktop's host-published DB port through local 127.0.0.1."""
    class _Server(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True

    def __init__(self, host: str, port: int) -> None:
        owner = self
        self.host = host
        self.port = port

        class Handler(socketserver.BaseRequestHandler):
            def handle(self) -> None:
                try:
                    upstream = socket.create_connection((owner.host, owner.port), timeout=5)
                except OSError:
                    return
                peers = (self.request, upstream)
                try:
                    while True:
                        readable, _, _ = select.select(peers, (), (), 30)
                        if not readable:
                            continue
                        for source in readable:
                            packet = source.recv(65536)
                            if not packet:
                                return
                            peers[source is self.request].sendall(packet)
                except OSError:
                    return
                finally:
                    for peer in peers:
                        try:
                            peer.close()
                        except OSError:
                            pass

        self.server = self._Server(("127.0.0.1", 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.local_port = self.server.server_address[1]

    def close(self) -> None:
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)


class DisposableMariaDB:
    """Task-created database container; credentials stay in process environments."""

    def __init__(self) -> None:
        selector = uuid.uuid4().hex[:12]
        self.database = DB_PREFIX + selector
        self.container = "s10-pa-web-recovery-" + selector
        self.user = "s10_" + secrets.token_hex(8)
        self.password = secrets.token_hex(32)
        self.root_password = secrets.token_hex(32)
        self.port = 0
        self._forwarder: _LoopbackForwarder | None = None
        self._started = False
        self._tmp = None

    def __enter__(self) -> "DisposableMariaDB":
        if not re.fullmatch(r"corpse_journey_test_[0-9a-f]{12}", self.database):
            raise AssertionError("fixture produced a non-approved database selector")
        inspect = subprocess.run(
            ["docker", "image", "inspect", DB_IMAGE], stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE, text=True, check=False,
        )
        if inspect.returncode:
            raise RuntimeError(f"required local MariaDB image unavailable: {DB_IMAGE}")
        self._tmp = tempfile.TemporaryDirectory(prefix="s10-db-env-")
        env_path = Path(self._tmp.name) / "mariadb.env"
        env_path.write_text(
            "MARIADB_ROOT_PASSWORD=" + self.root_password + "\n" +
            "MARIADB_DATABASE=" + self.database + "\n" +
            "MARIADB_USER=" + self.user + "\n" +
            "MARIADB_PASSWORD=" + self.password + "\n",
            encoding="utf-8",
        )
        env_path.chmod(0o600)
        try:
            created = subprocess.run(
                ["docker", "run", "--detach", "--name", self.container,
                 "--restart=no", "--cpus=2", "--memory=2g", "--pids-limit=512",
                 "--publish", "127.0.0.1::3306", "--env-file", str(env_path), DB_IMAGE],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=False,
            )
            if created.returncode:
                raise RuntimeError("could not create the task's isolated MariaDB container")
            self._started = True
        finally:
            # The image has copied its initialization environment; remove the only
            # credential file before any migrations or runtime launch.
            try:
                env_path.unlink()
            except FileNotFoundError:
                pass
            self._tmp.cleanup()
            self._tmp = None

        try:
            mapped = subprocess.check_output(
                ["docker", "port", self.container, "3306/tcp"], text=True
            ).strip().splitlines()
            if len(mapped) != 1 or not mapped[0].startswith("127.0.0.1:"):
                raise RuntimeError("disposable SQL port was not published exclusively on loopback")
            self._forwarder = _LoopbackForwarder(
                "host.docker.internal", int(mapped[0].rsplit(":", 1)[1]))
            self.port = self._forwarder.local_port
            self._wait_ready()
            return self
        except BaseException:
            self._remove_container()
            raise

    def _remove_container(self) -> None:
        if self._forwarder:
            self._forwarder.close()
            self._forwarder = None
        if self._started:
            subprocess.run(["docker", "rm", "--force", self.container],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           check=False, timeout=30)
            listing = subprocess.run(
                ["docker", "ps", "-a", "--format", "{{.Names}}"],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                check=False, timeout=10)
            if listing.returncode:
                raise RuntimeError("could not verify disposable database cleanup")
            self._started = False
            if self.container in listing.stdout.splitlines():
                raise AssertionError("disposable database container survived cleanup")

    def __exit__(self, exc_type, exc, traceback) -> None:
        self._remove_container()
        if self._tmp:
            self._tmp.cleanup()
            self._tmp = None

    def env(self, *, port: int | None = None, secret: str | None = None) -> dict[str, str]:
        environment = _clean_environment()
        environment.update({
            "ENVIRONMENT": "local",
            "DB_HOST": "127.0.0.1",
            "DB_PORT": str(port or self.port),
            "DB_NAME": self.database,
            "DB_USER": self.user,
            "DB_PASSWD": self.password,
            "DB_ALLOWED_TARGETS": "127.0.0.1/" + self.database,
            "MYSQL_PWD": self.password,
            "TEST_DB_DISPOSABLE": "1",
            "DURIS_TEST_SQL_DEATH_CONFLICT_RECOVERY": "1",
            "PERSISTENCE_MODE": "mariadb-primary",
            "DB_TLS": "FALSE",
            "REDIS": "FALSE",
            "CHAOS_MUD": "FALSE",
        })
        if secret:
            environment["DURISWEB_SECRET"] = secret
        if "LD_LIBRARY_PATH" in os.environ:
            environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        return environment

    def mysql(self, statement: str, *, database: bool = True) -> str:
        command = ["mysql", "--protocol=tcp", "--connect-timeout=5", "-h", "127.0.0.1",
                   "-P", str(self.port), "-u", self.user, "--default-character-set=utf8mb4",
                   "-N", "-B"]
        if database:
            command.append(self.database)
        result = subprocess.run(command, input=statement, text=True, env=self.env(),
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                check=False, timeout=120)
        if result.returncode:
            raise RuntimeError("disposable SQL command failed: " + result.stderr[-2000:])
        return result.stdout.strip()

    def _wait_ready(self) -> None:
        deadline = time.monotonic() + 120
        last_error = "not ready"
        while time.monotonic() < deadline:
            result = subprocess.run(
                ["mysql", "--protocol=tcp", "--connect-timeout=2", "-h", "127.0.0.1",
                 "-P", str(self.port), "-u", self.user, "-N", "-B", self.database,
                 "-e", "SELECT 1"], env=self.env(), stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True, check=False, timeout=5,
            )
            if result.returncode == 0 and result.stdout.strip() == "1":
                return
            last_error = result.stderr[-500:]
            time.sleep(0.2)
        logs = subprocess.run(["docker", "logs", self.container],
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, check=False, timeout=10).stdout
        for secret in (self.password, self.root_password):
            logs = logs.replace(secret, "[REDACTED]")
        state = subprocess.run(
            ["docker", "inspect", "--format", "{{.State.Status}}/{{.State.ExitCode}}", self.container],
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, check=False, timeout=10,
        ).stdout.strip()
        raise RuntimeError("fresh MariaDB did not become ready (container=" + state + "): " +
                           last_error + "\\n" + logs[-3000:])

    def initialize_schema(self) -> None:
        # Use the ordinary project bootstrap and immutable migration runner only
        # against this fresh, loopback-only schema.
        bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        self.mysql(bootstrap)
        environment = self.env()
        commands = (
            (["python3", "scripts/migration_runner.py", "adopt", "--kind", "fresh_bootstrap"], 180),
            (["python3", "scripts/migration_runner.py", "run"], 300),
        )
        for command, timeout in commands:
            result: subprocess.CompletedProcess[str] | None = None
            attempts = 3 if command[-1] == "run" else 1
            for attempt in range(attempts):
                result = subprocess.run(command, cwd=ROOT, env=environment, check=False,
                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                        text=True, timeout=timeout)
                if not result.returncode:
                    break
                detail = result.stdout + result.stderr
                transient = "ERROR 2013" in detail or "ERROR 2003" in detail
                if not transient or attempt + 1 == attempts:
                    break
                time.sleep(1 << attempt)
            if result is None:
                raise RuntimeError("migration runner did not start")
            if result.returncode:
                detail = result.stdout + result.stderr
                for secret in (self.password, self.root_password, self.user):
                    detail = detail.replace(secret, "[REDACTED]")
                docker_logs = subprocess.run(
                    ["docker", "logs", self.container], stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT, text=True, check=False, timeout=10).stdout
                for secret in (self.password, self.root_password, self.user):
                    docker_logs = docker_logs.replace(secret, "[REDACTED]")
                raise RuntimeError("immutable migration runner failed (exit=" +
                                   str(result.returncode) + "): " + detail[-4000:] +
                                   "\\nMariaDB fixture log:\\n" + docker_logs[-4000:])


def container_present(name: str) -> bool:
    result = subprocess.run(["docker", "ps", "-a", "--format", "{{.Names}}"],
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                            check=False, timeout=10)
    if result.returncode:
        raise RuntimeError("could not verify disposable database container absence")
    return name in result.stdout.splitlines()


class CommitReplyDropProxy:
    """Forward MySQL TCP and lose one selected deletion COMMIT reply on purpose."""

    class _Server(socketserver.ThreadingTCPServer):
        allow_reuse_address = True
        daemon_threads = True

    def __init__(self, target_host: str, target_port: int) -> None:
        self.target_host = target_host
        self.target_port = target_port
        self.target_pid: int | None = None
        self.commit_sent = threading.Event()
        self.commit_reply_dropped = threading.Event()
        self.commit_reply_ok = False
        owner = self

        class Handler(socketserver.BaseRequestHandler):
            def handle(self) -> None:
                try:
                    upstream = socket.create_connection((owner.target_host, owner.target_port), timeout=5)
                except OSError:
                    return
                client = self.request
                upstream.settimeout(None)
                tagged = False
                drop_reply = threading.Event()
                close_once = threading.Lock()
                closed = False

                def close_peers() -> None:
                    nonlocal closed
                    with close_once:
                        if closed:
                            return
                        closed = True
                    for peer in (client, upstream):
                        try:
                            peer.shutdown(socket.SHUT_RDWR)
                        except OSError:
                            pass
                        try:
                            peer.close()
                        except OSError:
                            pass

                def client_to_server() -> None:
                    nonlocal tagged
                    pending = bytearray()
                    try:
                        while True:
                            chunk = client.recv(65536)
                            if not chunk:
                                break
                            pending.extend(chunk)
                            packets = []
                            while len(pending) >= 4:
                                size = pending[0] | (pending[1] << 8) | (pending[2] << 16)
                                packet_size = size + 4
                                if len(pending) < packet_size:
                                    break
                                packets.append(bytes(pending[:packet_size]))
                                del pending[:packet_size]
                            if not packets:
                                continue
                            for packet in packets:
                                payload = packet[4:]
                                if payload and payload[0] == 3:
                                    query = payload[1:].decode("utf-8", errors="ignore").strip().lower()
                                    if (owner.target_pid is not None and
                                            f"select pid from player_data where pid={owner.target_pid} for update" in query):
                                        tagged = True
                                    if tagged and query.rstrip("; ").strip() == "commit":
                                        owner.commit_sent.set()
                                        drop_reply.set()
                                upstream.sendall(packet)
                    except OSError:
                        pass
                    finally:
                        close_peers()

                def server_to_client() -> None:
                    pending = bytearray()
                    try:
                        while True:
                            chunk = upstream.recv(65536)
                            if not chunk:
                                break
                            if drop_reply.is_set():
                                pending.extend(chunk)
                                if len(pending) >= 4:
                                    size = pending[0] | (pending[1] << 8) | (pending[2] << 16)
                                    if len(pending) >= size + 4:
                                        self_ok = pending[4] == 0
                                        owner.commit_reply_ok = self_ok
                                        owner.commit_reply_dropped.set()
                                        break
                                continue
                            client.sendall(chunk)
                    except OSError:
                        pass
                    finally:
                        close_peers()

                down = threading.Thread(target=client_to_server, daemon=True)
                down.start()
                server_to_client()
                down.join(timeout=2)

        self.server = self._Server(("127.0.0.1", 0), Handler)
        self.thread = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.port = self.server.server_address[1]

    def arm(self, pid: int) -> None:
        self.target_pid = pid
        self.commit_sent.clear()
        self.commit_reply_dropped.clear()
        self.commit_reply_ok = False

    def wait_dropped_ok(self, timeout: float = 20) -> None:
        if not self.commit_reply_dropped.wait(timeout):
            raise AssertionError("proxy did not observe the selected deletion COMMIT reply")
        if not self.commit_reply_ok:
            raise AssertionError("selected deletion COMMIT returned a non-OK SQL outcome")

    def close(self) -> None:
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=5)


class WebSocketClient:
    """Small RFC 6455 client; exercises the actual service listener and frames."""
    def __init__(self, port: int) -> None:
        self.sock = socket.create_connection(("127.0.0.1", port), timeout=15)
        self.sock.settimeout(30)
        key = "dGhlIHNhbXBsZSBub25jZQ=="
        request = (
            "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Key: " + key +
            "\r\nSec-WebSocket-Version: 13\r\n\r\n"
        )
        self.sock.sendall(request.encode("ascii"))
        response = self._read_until(b"\r\n\r\n", 8192)
        if b" 101 " not in response or b"Upgrade: websocket" not in response:
            raise AssertionError("server did not complete the WebSocket upgrade")

    def _read_until(self, delimiter: bytes, maximum: int) -> bytes:
        data = bytearray()
        while delimiter not in data:
            chunk = self.sock.recv(1024)
            if not chunk:
                raise AssertionError("WebSocket closed before handshake completed")
            data.extend(chunk)
            if len(data) > maximum:
                raise AssertionError("WebSocket handshake exceeded bound")
        return bytes(data)

    def send(self, value: dict) -> None:
        payload = json.dumps(value, separators=(",", ":")).encode()
        mask = secrets.token_bytes(4)
        length = len(payload)
        if length < 126:
            header = bytes((0x81, 0x80 | length))
        elif length <= 0xFFFF:
            header = bytes((0x81, 0x80 | 126)) + struct.pack("!H", length)
        else:
            header = bytes((0x81, 0x80 | 127)) + struct.pack("!Q", length)
        masked = bytes(byte ^ mask[index % 4] for index, byte in enumerate(payload))
        self.sock.sendall(header + mask + masked)

    def receive(self) -> dict:
        first = self._read_exact(2)
        opcode = first[0] & 0x0F
        length = first[1] & 0x7F
        if length == 126:
            length = struct.unpack("!H", self._read_exact(2))[0]
        elif length == 127:
            length = struct.unpack("!Q", self._read_exact(8))[0]
        if first[1] & 0x80:
            mask = self._read_exact(4)
        else:
            mask = None
        payload = self._read_exact(length)
        if mask:
            payload = bytes(byte ^ mask[index % 4] for index, byte in enumerate(payload))
        if opcode == 8:
            raise AssertionError("server closed WebSocket while returning a result")
        if opcode != 1:
            return {"type": "non-text", "opcode": opcode}
        return json.loads(payload)

    def _read_exact(self, length: int) -> bytes:
        result = bytearray()
        while len(result) < length:
            chunk = self.sock.recv(length - len(result))
            if not chunk:
                raise AssertionError("WebSocket closed during frame")
            result.extend(chunk)
        return bytes(result)

    def authenticate(self, secret: str) -> None:
        self.send({"type": "cmd", "cmd": "durisweb_challenge"})
        deadline = time.monotonic() + 10
        challenge = {}
        while time.monotonic() < deadline:
            challenge = self.receive()
            if challenge.get("type") in ("durisweb_challenge", "durisweb_auth"):
                break
        if challenge.get("type") != "durisweb_challenge" or not challenge.get("nonce"):
            raise AssertionError("actual WebSocket challenge handler returned no nonce: " + repr(challenge))
        nonce = challenge["nonce"]
        minute = int(time.time() // 60)
        message = f"{minute}:{nonce}".encode()
        signature = hmac.new(secret.encode(), message, hashlib.sha256).hexdigest()
        self.send({"type": "cmd", "cmd": "durisweb_auth", "data": {"sig": signature}})
        auth = self.receive()
        if auth.get("type") != "durisweb_auth" or auth.get("success") is not True:
            raise AssertionError("real DurisWeb WebSocket authentication failed")

    def delete(self, *, request_id: str, account: str, name: str, pid: int) -> tuple[dict, list[dict]]:
        self.send({"type": "cmd", "cmd": "admin_delete_character", "data": {
            "requestId": request_id, "account": account, "name": name,
            "pid": pid, "deletedBy": "s10-disposable-fixture",
        }})
        progress: list[dict] = []
        deadline = time.monotonic() + 45
        while time.monotonic() < deadline:
            message = self.receive()
            if message.get("type") == "admin_delete_progress":
                progress.append(message)
            elif message.get("type") == "admin_delete_character":
                if message.get("requestId") != request_id:
                    continue
                return message, progress
        raise AssertionError("actual admin-delete WebSocket response timed out")

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass


@contextmanager
def server_fixture(database: DisposableMariaDB, runtime_dir: Path, *, server: Path,
                   database_port: int, secret: str, web_port: int, game_port: int,
                   tls_port: int):
    import test_flatfile_combat_journey as journey

    journey.make_fixture(runtime_dir)
    journey.generate_certificate(runtime_dir)
    (runtime_dir / "Players").mkdir(mode=0o700)
    (runtime_dir / "logs/log").mkdir(parents=True)
    for name in ("players", "critical"):
        (runtime_dir / "journals" / name).mkdir(parents=True, mode=0o700)
    environment = database.env(port=database_port, secret=secret)
    environment.update({
        "PLAYER_SAVE_JOURNAL_DIR": str(runtime_dir / "journals/players"),
        "CRITICAL_COMMAND_JOURNAL_DIR": str(runtime_dir / "journals/critical"),
        "DURIS_TLS_PORT": str(tls_port),
        "DURIS_WEBSOCKET_PORT": str(web_port),
        "LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
        "DURIS_PRODUCTION_PORT": str(game_port),
    })
    output_path = runtime_dir / "server.out"
    with output_path.open("w", encoding="utf-8") as output:
        process = subprocess.Popen([str(server), "--minimal", "-s", "-d", str(runtime_dir),
                                    str(game_port)], cwd=runtime_dir, env=environment,
                                   stdout=output, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                if process.poll() is not None:
                    log_text = ""
                    for log_file in sorted((runtime_dir / "logs/log").glob("*")):
                        if log_file.is_file() and log_file.stat().st_size <= 1_000_000:
                            log_text += "\\n" + log_file.read_text(errors="replace")[-5000:]
                    diagnostic = output_path.read_text(errors="replace")[-5000:] + log_text
                    for value in (database.password, database.user, secret):
                        diagnostic = diagnostic.replace(value, "[REDACTED]")
                    raise AssertionError("frozen server exited before entering the game loop:\\n" +
                                         diagnostic)
                if output_path.exists() and b"Entering game loop." in output_path.read_bytes():
                    break
                time.sleep(0.1)
            else:
                diagnostic = output_path.read_text(errors="replace")[-5000:]
                for value in (database.password, database.user, secret):
                    diagnostic = diagnostic.replace(value, "[REDACTED]")
                raise AssertionError("frozen server did not reach the game loop:\\n" + diagnostic)
            listener_deadline = time.monotonic() + 5
            last_error = "listener did not bind"
            while time.monotonic() < listener_deadline:
                probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                probe.settimeout(0.2)
                try:
                    probe.connect(("127.0.0.1", web_port))
                    break
                except OSError as exc:
                    last_error = str(exc)
                    time.sleep(0.1)
                finally:
                    probe.close()
            else:
                diagnostic = output_path.read_text(errors="replace")[-5000:]
                for log_file in sorted((runtime_dir / "logs/log").glob("*")):
                    if log_file.is_file() and log_file.stat().st_size <= 1_000_000:
                        diagnostic += "\\n" + log_file.read_text(errors="replace")[-5000:]
                for value in (database.password, database.user, secret):
                    diagnostic = diagnostic.replace(value, "[REDACTED]")
                raise AssertionError("WebSocket listener did not bind: " + last_error +
                                     "\\n" + diagnostic)
            yield process
        finally:
            if process.poll() is None:
                process.send_signal(signal.SIGTERM)
                try:
                    process.wait(timeout=30)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait(timeout=10)


def reserve_ports() -> tuple[int, int, int]:
    probes = [socket.socket() for _ in range(3)]
    try:
        for sock in probes:
            sock.bind(("127.0.0.1", 0))
        ports = [sock.getsockname()[1] for sock in probes]
        if len(set(ports)) != 3:
            raise AssertionError("listener port collision")
        return ports[0], ports[1], ports[2]
    finally:
        for sock in probes:
            sock.close()


def build_sql_harness(output: Path) -> None:
    flags = subprocess.check_output(["mysql_config", "--cflags"], text=True).split()
    libraries = subprocess.check_output(["mysql_config", "--libs"], text=True).split()
    compiler = os.environ.get("CXX", "g++-14")
    command = [compiler, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
               "-pthread", "-ffunction-sections", "-fdata-sections", "-Isrc", *flags,
               "tests/async/pa_web_recovery_harness.cpp",
               "src/player/player_death_conflict_repository.c",
               "src/player/player_snapshot_repository.c", "src/player/player_snapshot_codec.c",
               "src/player/player_save_journal.c",
               "src/sql/item_extra_descr_codec.c", "src/persistence/critical_command.c",
               "src/persistence/player_death_restitution_command.c",
               "src/persistence/persistence_observability.c",
               "src/persistence/economic_sql_lifecycle_guard.c",
               "-Wl,--gc-sections", *libraries, "-lcrypto", "-o", str(output)]
    subprocess.run(command, cwd=ROOT, check=True, stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE, text=True, timeout=180)


def run_sql_harness(binary: Path, database: DisposableMariaDB, mode: str) -> str:
    result = subprocess.run([str(binary), mode], cwd=ROOT, env=database.env(),
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            text=True, check=False, timeout=60)
    if result.returncode:
        raise AssertionError("SQL fixture harness failed: " + result.stderr[-3000:])
    return result.stdout.strip()
