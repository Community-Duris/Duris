#!/usr/bin/env python3
"""End-to-end account password recovery by email regression.

Boots the real flat-file server twice against an isolated authority.  The first
boot enables recovery (MAIL_ENABLED=TRUE) with a wire-level fake SMTP server on
loopback: an account is created with an email address, the password prompt
offers '?', the reset code is read from the captured SMTP DATA, the password is
changed, the new password logs in, the old one is refused, and a second request
inside the cooldown produces the uniform text without a second mail.  The
second boot runs without MAIL_ENABLED and proves the byte-identical legacy
prompt plus the not-available reply to '?'.

Configuration reaches the server through the subprocess environment, the same
mechanism the sibling journeys use: no .env is written into the run root, and
load_env_file's setenv(name, value, 0) lets a real environment variable win.
"""

from __future__ import annotations

import os
import pathlib
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import threading
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]
ACCOUNT = "Recoveracct"
OLD_PASSWORD = "Qz7!mN4@"
NEW_PASSWORD = "Wk3#vB8%"
EMAIL = "recovery@example.invalid"
MAIL_FROM = "noreply@duris.test"
ANSI = re.compile(rb"\x1b\[[0-?]*[ -/]*[@-~]")

# Server-side literals this journey depends on.  Prompts carry their trailing space.
ENABLED_PROMPT = "Please enter your password (or ? to reset it by email): "
DISABLED_PROMPT = "Please enter your password: "
UNIFORM_FIRST_LINE = (
    "If that account has an email address on file, a reset code has been sent to it."
)
CODE_PROMPT = "Enter the reset code (or CANCEL): "
CODE_ACCEPTED = "Code accepted. Choose a new password."
NEW_PASSWORD_PROMPT = "New password (or CANCEL): "
VERIFY_PASSWORD_PROMPT = "Verify new password: "
PASSWORD_CHANGED = "Your password has been changed"
INVALID_PASSWORD = "Invalid Password"
NOT_AVAILABLE = "Password reset by email is not available on this server"
BOOT_ENABLED_LINE = "Account recovery enabled (smtp port="
BOOT_DISABLED_LINE = "MAIL_ENABLED is not TRUE"
MAIL_SUBJECT = "Subject: Duris account password reset"
CODE_PATTERN = re.compile(rb"Reset code: ([0-9a-f]{8}-[0-9a-f]{8}-[0-9a-f]{8}-[0-9a-f]{8})")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def available_ports() -> tuple[int, int, int]:
    """Reserve a non-privileged plain/TLS/WebSocket port set."""
    for _ in range(200):
        probes = [socket.socket() for _ in range(3)]
        try:
            probes[0].bind(("127.0.0.1", 0))
            plain = probes[0].getsockname()[1]
            if plain <= 1024 or plain >= 65533:
                continue
            probes[1].bind(("127.0.0.1", plain + 1))
            probes[2].bind(("127.0.0.1", plain + 2))
            return plain, plain + 1, plain + 2
        except OSError:
            continue
        finally:
            for probe in probes:
                probe.close()
    raise AssertionError("could not reserve isolated listener ports")


class FakeSmtpServer:
    """Plaintext SMTP on 127.0.0.1 that records connections and every DATA payload.

    Speaks 220 / multi-line 250 to EHLO (no STARTTLS advertised) / 250 / 250 / 354 /
    250 after CRLF.CRLF / 221.  The listener owns its port for the whole journey, so
    the server's MAIL_PORT cannot be stolen between reservation and use.
    """

    def __init__(self) -> None:
        self.listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(("127.0.0.1", 0))
        self.listener.listen(8)
        self.listener.settimeout(0.2)
        self.port = self.listener.getsockname()[1]
        self.lock = threading.Lock()
        self.connections = 0
        self.payloads: list[bytes] = []
        self.stop = threading.Event()
        self.workers: list[threading.Thread] = []
        self.thread = threading.Thread(target=self._accept_loop, name="fake-smtp", daemon=True)
        self.thread.start()

    def close(self) -> None:
        self.stop.set()
        self.thread.join(timeout=5)
        self.listener.close()
        for worker in self.workers:
            worker.join(timeout=5)

    def connection_count(self) -> int:
        with self.lock:
            return self.connections

    def wait_for_payloads(self, count: int, timeout: float) -> list[bytes]:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with self.lock:
                if len(self.payloads) >= count:
                    return list(self.payloads)
            time.sleep(0.05)
        with self.lock:
            captured = len(self.payloads)
        raise AssertionError(
            f"fake SMTP captured {captured} DATA payload(s); expected {count} within {timeout} s"
        )

    def _accept_loop(self) -> None:
        while not self.stop.is_set():
            try:
                connection, _ = self.listener.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            with self.lock:
                self.connections += 1
            worker = threading.Thread(target=self._serve, args=(connection,), daemon=True)
            worker.start()
            self.workers.append(worker)

    def _serve(self, connection: socket.socket) -> None:
        connection.settimeout(10)
        try:
            self._converse(connection)
        except (OSError, ValueError):
            pass
        finally:
            connection.close()

    @staticmethod
    def _read_line(connection: socket.socket, buffer: bytearray) -> bytes | None:
        while True:
            end = buffer.find(b"\r\n")
            if end >= 0:
                line = bytes(buffer[:end])
                del buffer[: end + 2]
                return line
            chunk = connection.recv(65536)
            if not chunk:
                return None
            buffer.extend(chunk)

    @staticmethod
    def _read_payload(connection: socket.socket, buffer: bytearray) -> bytes | None:
        while True:
            if buffer.startswith(b".\r\n"):
                del buffer[:3]
                return b""
            end = buffer.find(b"\r\n.\r\n")
            if end >= 0:
                payload = bytes(buffer[: end + 2])
                del buffer[: end + 5]
                # Undo SMTP dot-stuffing so assertions see the message as rendered.
                if payload.startswith(b".."):
                    payload = payload[1:]
                return payload.replace(b"\r\n..", b"\r\n.")
            chunk = connection.recv(65536)
            if not chunk:
                return None
            buffer.extend(chunk)

    def _converse(self, connection: socket.socket) -> None:
        buffer = bytearray()
        connection.sendall(b"220 fake ESMTP\r\n")
        while True:
            line = self._read_line(connection, buffer)
            if line is None:
                return
            verb = line.split(b" ", 1)[0].upper()
            if verb == b"EHLO":
                connection.sendall(b"250-fake\r\n250-SIZE 35882577\r\n250 8BITMIME\r\n")
            elif verb == b"HELO":
                connection.sendall(b"250 fake\r\n")
            elif verb in (b"MAIL", b"RCPT", b"RSET", b"NOOP"):
                connection.sendall(b"250 OK\r\n")
            elif verb == b"DATA":
                connection.sendall(b"354 go\r\n")
                payload = self._read_payload(connection, buffer)
                if payload is None:
                    return
                with self.lock:
                    self.payloads.append(payload)
                connection.sendall(b"250 queued\r\n")
            elif verb == b"QUIT":
                connection.sendall(b"221 bye\r\n")
                return
            else:
                connection.sendall(b"502 not implemented\r\n")


class MudClient:
    def __init__(self, port: int) -> None:
        deadline = time.monotonic() + 30
        while True:
            try:
                self.socket = socket.create_connection(("127.0.0.1", port), timeout=1)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)
        self.socket.settimeout(0.25)
        self.pending = bytearray()
        self.transcript = bytearray()

    def close(self) -> None:
        self.socket.close()

    def send(self, line: str) -> None:
        self.socket.sendall(line.encode("ascii") + b"\n")

    def _receive(self) -> bool:
        try:
            chunk = self.socket.recv(65536)
        except socket.timeout:
            return False
        if not chunk:
            raise AssertionError("server closed the gameplay connection")
        cleaned = ANSI.sub(b"", chunk)
        self.pending.extend(cleaned)
        self.transcript.extend(cleaned)
        return True

    def expect(self, needle: str, timeout: float = 15) -> str:
        matched, output = self.expect_any((needle,), timeout)
        require(matched == needle, f"unexpected match {matched!r}")
        return output

    def expect_any(self, needles: tuple[str, ...], timeout: float = 15) -> tuple[str, str]:
        targets = tuple((needle, needle.encode("ascii")) for needle in needles)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            cleaned = bytes(self.pending)
            for needle, target in targets:
                position = cleaned.find(target)
                if position >= 0:
                    # Login output is ASCII apart from Telnet negotiation. Consume
                    # through the last target byte while retaining a readable slice.
                    consumed = cleaned[: position + len(target)]
                    del self.pending[: position + len(target)]
                    return needle, consumed.decode("utf-8", errors="replace")
            self._receive()
        readable = bytes(self.pending).decode("utf-8", errors="replace")
        raise AssertionError(f"timed out waiting for {needles!r}; received:\n{readable[-6000:]}")


def make_fixture(run_root: pathlib.Path) -> None:
    for directory in ("areas", "docs"):
        (run_root / directory).symlink_to(ROOT / directory, target_is_directory=True)
    # lib and the mini world are copied, not linked, as the sibling journeys do: the
    # server must never write through a link into the checkout.
    shutil.copytree(ROOT / "lib", run_root / "lib")
    shutil.copytree(ROOT / "areas_mini", run_root / "areas_mini")


def generate_certificate(run_root: pathlib.Path) -> None:
    generated = subprocess.run(
        [
            "openssl",
            "req",
            "-x509",
            "-newkey",
            "rsa:2048",
            "-sha256",
            "-nodes",
            "-days",
            "1",
            "-subj",
            "/CN=localhost",
            "-keyout",
            str(run_root / "duris.key"),
            "-out",
            str(run_root / "duris.crt"),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=30,
    )
    require(generated.returncode == 0, "certificate generation failed:\n" + generated.stdout)
    (run_root / "duris.key").chmod(0o600)


def build_flatfile_server(build_root: pathlib.Path) -> pathlib.Path:
    """Build an isolated flat-file journey server under build_root."""
    binary = build_root / "server/dms_new"
    build = subprocess.run(
        [
            "make",
            "-C",
            "src",
            "PERSISTENCE_BACKEND=flatfile",
            f"BIN_ROOT={build_root}",
            f"OBJDIR={build_root / 'objects' / 'server'}",
            f"SERVER_BIN_DIR={binary.parent}",
            f"DMS_BINARY={binary}",
            "-j2",
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=600,
    )
    require(build.returncode == 0, "flat-file server build failed:\n" + build.stdout[-8000:])
    return binary


class IsolatedServer:
    """One booted flat-file server with its own state, run root, ports, and output file."""

    def __init__(self, binary: pathlib.Path, mail_environment: dict[str, str] | None) -> None:
        self.binary = binary
        self.mail_environment = mail_environment or {}
        self.state_tmp = tempfile.TemporaryDirectory(prefix="duris-recovery-state-")
        self.run_tmp = tempfile.TemporaryDirectory(prefix="duris-recovery-run-")
        self.state_root = pathlib.Path(self.state_tmp.name)
        self.run_root = pathlib.Path(self.run_tmp.name)
        self.output_path = self.run_root / "server.out"
        self.process: subprocess.Popen[str] | None = None
        self.output = None
        self.plain_port = 0

    def __enter__(self) -> "IsolatedServer":
        try:
            self._boot()
        except Exception:
            self.__exit__(None, None, None)
            raise
        return self

    def __exit__(self, *_: object) -> None:
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        if self.output is not None:
            self.output.close()
            self.output = None
        self.run_tmp.cleanup()
        self.state_tmp.cleanup()

    def _boot(self) -> None:
        self.state_root.chmod(0o700)
        (self.run_root / "logs/log").mkdir(parents=True)
        (self.run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        make_fixture(self.run_root)
        generate_certificate(self.run_root)

        journal_root = self.run_root / "journals"
        (journal_root / "players").mkdir(parents=True, mode=0o700)
        (journal_root / "critical").mkdir(mode=0o700)
        self.plain_port, tls_port, websocket_port = available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(self.state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(journal_root / "players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(journal_root / "critical"),
            "LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": str(websocket_port),
            "REDIS": "FALSE",
            "CHAOS_MUD": "FALSE",
        }
        environment.update(self.mail_environment)
        if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = runtime_library_path

        self.output = self.output_path.open("w", encoding="utf-8")
        self.process = subprocess.Popen(
            [str(self.binary), "--minimal", "-s", "-d", str(self.run_root), str(self.plain_port)],
            cwd=self.run_root,
            env=environment,
            text=True,
            stdout=self.output,
            stderr=subprocess.STDOUT,
        )
        deadline = time.monotonic() + 120
        boot_output = ""
        while time.monotonic() < deadline:
            boot_output = self.server_output()
            if "Entering game loop." in boot_output:
                break
            if self.process.poll() is not None:
                break
            time.sleep(0.1)
        require(
            "Entering game loop." in boot_output,
            "isolated recovery server did not boot:\n" + boot_output[-8000:],
        )

    def server_output(self) -> str:
        if self.output is not None:
            self.output.flush()
        return self.output_path.read_text(errors="replace")

    def status_log(self) -> str:
        path = self.run_root / "logs/log/status"
        if not path.is_file():
            return ""
        return path.read_text(encoding="utf-8", errors="replace")

    def wait_for_status_line(self, needle: str, timeout: float = 10) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if needle in self.status_log():
                return
            time.sleep(0.05)
        raise AssertionError(f"logs/log/status never carried {needle!r}:\n{self.status_log()[-4000:]}")

    def runtime_logs(self) -> str:
        """Collect bounded synthetic-server logs for actionable journey failures."""
        sections = []
        for path in sorted((self.run_root / "logs").rglob("*")):
            if not path.is_file() or path.name == ".gitignore":
                continue
            content = path.read_text(encoding="utf-8", errors="replace").strip()
            if content:
                sections.append(f"--- {path.relative_to(self.run_root)} ---\n{content[-8000:]}")
        return "\n".join(sections)[-30000:]

    def log_corpus(self) -> list[tuple[str, bytes]]:
        corpus = [("server.out", self.output_path.read_bytes())]
        for path in sorted((self.run_root / "logs").rglob("*")):
            if path.is_file():
                corpus.append((str(path.relative_to(self.run_root)), path.read_bytes()))
        return corpus

    def shutdown(self) -> None:
        assert self.process is not None
        self.process.send_signal(signal.SIGTERM)
        self.process.wait(timeout=30)
        server_output = self.server_output()
        require(
            self.process.returncode == 0,
            f"server shutdown failed ({self.process.returncode}):\n{server_output[-8000:]}",
        )
        require(
            "Normal termination of game." in server_output,
            "recovery journey did not reach normal server shutdown",
        )
        require(
            "FATAL:" not in server_output and "assert:" not in server_output,
            "recovery journey logged a fatal/assertion failure:\n" + server_output[-8000:],
        )
        require(
            "terminate called" not in server_output,
            "recovery journey terminated abnormally:\n" + server_output[-8000:],
        )


def enter_account_name(client: MudClient) -> None:
    entry, _ = client.expect_any(("term type", "account name"))
    if entry == "term type":
        client.send("9")
        client.expect("account name")
    client.send(ACCOUNT)


def create_account(client: MudClient, password: str) -> None:
    """Create ACCOUNT with EMAIL on file and stop at the account menu."""
    enter_account_name(client)
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send(EMAIL)
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send(password)
    client.expect("verify your password")
    client.send(password)
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")


def login_to_menu(client: MudClient, password: str) -> None:
    """From a password prompt already displayed: log in and reach the account menu."""
    client.send(password)
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")


def leave_account_menu(client: MudClient) -> None:
    client.send("0")
    client.close()


def request_reset(client: MudClient) -> None:
    """Name -> enabled prompt -> '?' -> uniform text -> code prompt."""
    enter_account_name(client)
    client.expect(ENABLED_PROMPT)
    client.send("?")
    client.expect(UNIFORM_FIRST_LINE)
    client.expect(CODE_PROMPT)


def header_present(message: str, name: str, address: str) -> bool:
    pattern = re.compile(rf"^{name}: <?{re.escape(address)}>?\r?$", re.MULTILINE)
    return pattern.search(message) is not None


def extract_reset_code(payload: bytes) -> str:
    message = payload.decode("ascii", errors="replace")
    require(MAIL_SUBJECT in message, "captured mail lacks the reset subject")
    require(header_present(message, "To", EMAIL), "captured mail is not addressed to the account email")
    require(header_present(message, "From", MAIL_FROM), "captured mail does not carry MAIL_FROM")
    match = CODE_PATTERN.search(payload)
    require(match is not None, "captured mail carries no 8-8-8-8 reset code line")
    assert match is not None
    return match.group(1).decode("ascii")


def require_no_new_connection(smtp: FakeSmtpServer, expected: int, window: float = 1.0) -> None:
    """A negative assertion needs a bounded window; fail on the first extra connection."""
    deadline = time.monotonic() + window
    while time.monotonic() < deadline:
        count = smtp.connection_count()
        require(count == expected, f"fake SMTP saw {count} connection(s); expected {expected}")
        time.sleep(0.05)


def require_no_secret_leak(server: IsolatedServer, secrets: tuple[str, ...]) -> None:
    corpus = server.log_corpus()
    by_label = dict(corpus)
    status = by_label.get("logs/log/status", b"")
    require(bool(status), "logs/log/status is missing from the sweep corpus")
    if b"Account recovery enabled" in status:
        # The one file the core writes per-request lines to must be in the sweep.
        player_log = by_label.get("logs/player-log/player", b"")
        require(
            b"account recovery request=" in player_log,
            "the recovery request lines are missing from the swept player log",
        )
    for label, content in corpus:
        for secret in secrets:
            # Never name the leaked value in the failure text.
            require(secret.encode("ascii") not in content, f"{label} leaked a recovery secret")


def run_enabled_journey(binary: pathlib.Path) -> None:
    smtp = FakeSmtpServer()
    try:
        mail_environment = {
            "MAIL_ENABLED": "TRUE",
            "MAIL_HOST": "127.0.0.1",
            "MAIL_PORT": str(smtp.port),
            "MAIL_TLS": "FALSE",
            "MAIL_FROM": MAIL_FROM,
        }
        with IsolatedServer(binary, mail_environment) as server:
            client: MudClient | None = None
            try:
                server.wait_for_status_line(BOOT_ENABLED_LINE)

                client = MudClient(server.plain_port)
                create_account(client, OLD_PASSWORD)
                leave_account_menu(client)
                client = None

                client = MudClient(server.plain_port)
                request_reset(client)
                payloads = smtp.wait_for_payloads(1, timeout=20)
                require(len(payloads) == 1, f"fake SMTP captured {len(payloads)} payloads; expected 1")
                dashed_code = extract_reset_code(payloads[0])
                plain_code = dashed_code.replace("-", "")

                client.send(dashed_code)
                client.expect(CODE_ACCEPTED)
                client.expect(NEW_PASSWORD_PROMPT)
                client.send(NEW_PASSWORD)
                client.expect(VERIFY_PASSWORD_PROMPT)
                client.send(NEW_PASSWORD)
                client.expect(PASSWORD_CHANGED)
                client.expect(ENABLED_PROMPT)
                login_to_menu(client, NEW_PASSWORD)
                leave_account_menu(client)
                client = None

                client = MudClient(server.plain_port)
                enter_account_name(client)
                client.expect(ENABLED_PROMPT)
                client.send(OLD_PASSWORD)
                client.expect(INVALID_PASSWORD)
                client.close()
                client = None

                client = MudClient(server.plain_port)
                request_reset(client)
                require_no_new_connection(smtp, expected=1)
                client.close()
                client = None

                server.shutdown()
                require(smtp.connection_count() == 1, "fake SMTP saw more than one connection")
                require_no_secret_leak(server, (dashed_code, plain_code, EMAIL))
            except Exception as error:
                raise AssertionError(
                    f"{error}\n\n--- isolated server output ---\n{server.server_output()[-12000:]}"
                    f"\n\n--- isolated runtime logs ---\n{server.runtime_logs()}"
                ) from error
            finally:
                if client is not None:
                    client.close()
    finally:
        smtp.close()


def run_disabled_journey(binary: pathlib.Path) -> None:
    with IsolatedServer(binary, None) as server:
        client: MudClient | None = None
        try:
            server.wait_for_status_line(BOOT_DISABLED_LINE)

            client = MudClient(server.plain_port)
            create_account(client, OLD_PASSWORD)
            leave_account_menu(client)
            client = None

            client = MudClient(server.plain_port)
            enter_account_name(client)
            prompt = client.expect(DISABLED_PROMPT)
            require("reset it by email" not in prompt, "disabled prompt advertised the reset path")
            client.send("?")
            client.expect(NOT_AVAILABLE)
            prompt = client.expect(DISABLED_PROMPT)
            require("reset it by email" not in prompt, "disabled re-prompt advertised the reset path")
            login_to_menu(client, OLD_PASSWORD)
            leave_account_menu(client)
            client = None

            server.shutdown()
        except Exception as error:
            raise AssertionError(
                f"{error}\n\n--- isolated server output ---\n{server.server_output()[-12000:]}"
                f"\n\n--- isolated runtime logs ---\n{server.runtime_logs()}"
            ) from error
        finally:
            if client is not None:
                client.close()


if __name__ == "__main__":
    # The root regression runner executes the flat-file journeys concurrently.
    # Distinct temporary build roots prevent linker/runtime races and are removed
    # after each journey, including on failure.
    with tempfile.TemporaryDirectory(prefix=f"flatfile-recovery-{os.getpid()}-") as build_tmp:
        server_binary = build_flatfile_server(pathlib.Path(build_tmp))
        run_enabled_journey(server_binary)
        run_disabled_journey(server_binary)
    print("account password recovery by email journey passed")
