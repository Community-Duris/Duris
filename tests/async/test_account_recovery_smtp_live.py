#!/usr/bin/env python3
"""Exercise the REAL libcurl SMTP path of src/net/mail_sender.c against a fake relay.

A hand-rolled socket server on 127.0.0.1 speaks just enough ESMTP (no STARTTLS
advertised, plaintext on loopback as the config rule permits) to record what
libcurl sends and to answer by mode: ok, rcpt550 (RCPT refused), data451 (the
message refused after DATA) and stall (accept, never greet).  The compiled
harness tests/async/mail_sender_live_harness.cpp links mail_sender.c ALONE, so
the wire behaviour under test is the module's, with no engine stub in between.

Python 3.14 ships no smtpd/aiosmtpd, hence the raw-socket relay; the free-port
and stall-server shapes follow test_redis_fault_recovery_live.py.  A failing
compile IS the hard failure (its output is the message): there is no fixed-path
header pre-check, because Debian multiarch keeps curl.h under the triplet.
"""

from _paths import rel
import pathlib
import re
import shutil
import socket
import subprocess
import tempfile
import threading
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]
FROM = "noreply@duris.test"
TO = "player@duris.test"
HARNESS_TIMEOUT_S = 60


class FakeSmtpServer:
    """One-connection-at-a-time ESMTP relay that records every command line and
    every DATA payload it receives, across all connections."""

    def __init__(self, mode: str) -> None:
        self.mode = mode
        self.connections = 0
        self.transcript: list[bytes] = []
        self.data: list[bytes] = []
        self._stop = threading.Event()
        self._listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._listener.bind(("127.0.0.1", 0))
        self._listener.listen(4)
        self._listener.settimeout(0.2)
        self.port = int(self._listener.getsockname()[1])
        self._thread = threading.Thread(target=self._run, daemon=True)

    def __enter__(self) -> "FakeSmtpServer":
        self._thread.start()
        return self

    def __exit__(self, *unused: object) -> None:
        self._stop.set()
        self._thread.join(timeout=20)
        self._listener.close()

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                client, _ = self._listener.accept()
            except socket.timeout:
                continue
            except OSError:
                return
            self.connections += 1
            with client:
                try:
                    self._serve(client)
                except (OSError, TimeoutError):
                    pass

    def _serve(self, client: socket.socket) -> None:
        client.settimeout(10)
        if self.mode == "stall":
            # Accept and stay silent: the client must give up on its own clock.
            try:
                while client.recv(4096):
                    pass
            except (OSError, TimeoutError):
                pass
            return
        client.sendall(b"220 fake ESMTP\r\n")
        buffer = b""
        while True:
            line, buffer = self._read_until(client, buffer, b"\r\n")
            if line is None:
                return
            self.transcript.append(line)
            verb = line.split(b" ", 1)[0].split(b":", 1)[0].upper()
            if verb in (b"EHLO", b"HELO"):
                client.sendall(b"250-fake\r\n250-SIZE 35882577\r\n250 8BITMIME\r\n")
            elif verb == b"MAIL":
                client.sendall(b"250 OK\r\n")
            elif verb == b"RCPT":
                client.sendall(b"550 no\r\n" if self.mode == "rcpt550" else b"250 OK\r\n")
            elif verb == b"DATA":
                client.sendall(b"354 go\r\n")
                payload, buffer = self._read_until(client, buffer, b"\r\n.\r\n")
                if payload is None:
                    return
                self.data.append(payload)
                client.sendall(b"451 later\r\n" if self.mode == "data451" else b"250 queued\r\n")
            elif verb == b"QUIT":
                client.sendall(b"221 bye\r\n")
                return
            elif verb in (b"RSET", b"NOOP"):
                client.sendall(b"250 OK\r\n")
            else:
                client.sendall(b"500 unknown\r\n")

    @staticmethod
    def _read_until(client: socket.socket, buffer: bytes, terminator: bytes):
        """(chunk before terminator, remainder) or (None, buffer) on a closed peer."""
        while terminator not in buffer:
            received = client.recv(65536)
            if not received:
                return None, buffer
            buffer += received
        chunk, _, rest = buffer.partition(terminator)
        return chunk, rest


def compile_harness(binary: pathlib.Path) -> None:
    result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "-Isrc/ships",
            "-I/usr/include/libxml2",
            "tests/async/mail_sender_live_harness.cpp",
            rel("mail_sender.c"),
            "-Wl,--gc-sections",
            "-lcurl",
            "-lcrypto",
            "-lssl",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.returncode:
        hint = ""
        if shutil.which("curl-config") is None:
            hint = "\n(curl-config is not on PATH: is the libcurl development package installed?)"
        raise SystemExit(result.stdout + hint)


def run_harness(binary: pathlib.Path, port: int, code_file: pathlib.Path, section: str,
                total_timeout_ms: int | None = None):
    """(completed process, wall seconds); a non-zero exit or a hang is the failure."""
    argv = [str(binary), "127.0.0.1", str(port), FROM, TO, str(code_file), section]
    if total_timeout_ms is not None:
        argv.append(str(total_timeout_ms))
    started = time.monotonic()
    try:
        completed = subprocess.run(
            argv,
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=HARNESS_TIMEOUT_S,
        )
    except subprocess.TimeoutExpired as error:
        raise SystemExit(f"{section}: live harness exceeded {HARNESS_TIMEOUT_S} s") from error
    elapsed = time.monotonic() - started
    if completed.returncode:
        raise SystemExit(
            f"{section}: live harness exit {completed.returncode}\n"
            f"{completed.stdout}\n{completed.stderr}"
        )
    if f"ok: {section}\n" not in completed.stdout:
        raise SystemExit(f"{section}: harness did not report the section:\n{completed.stdout}")
    return completed, elapsed


def parse_result(stdout: str) -> tuple[int, int, int]:
    match = re.search(r"^outcome=(\d+) curl=(-?\d+) smtp=(-?\d+)$", stdout, re.MULTILINE)
    if not match:
        raise SystemExit(f"harness printed no result line:\n{stdout}")
    return int(match.group(1)), int(match.group(2)), int(match.group(3))


def read_code(code_file: pathlib.Path) -> str:
    code = code_file.read_text().strip()
    if not re.fullmatch(r"[0-9a-f]{32}", code):
        raise SystemExit("the harness did not write a 32-hex marker code")
    return code


def dashed(code: str) -> str:
    return "-".join(code[index : index + 8] for index in range(0, 32, 8))


def expect(condition: bool, label: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {label}")


SENT, RETRYABLE, TERMINAL = 0, 1, 2
CURLE_OPERATION_TIMEDOUT = 28

with tempfile.TemporaryDirectory(prefix="duris-smtp-live-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "mail_sender_live"
    code_file = temporary_path / "code.txt"
    compile_harness(binary)

    # ---- mode ok, direct send: the wire and the message ----
    with FakeSmtpServer("ok") as fake:
        completed, _ = run_harness(binary, fake.port, code_file, "direct")
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect((outcome, curl_code, smtp_code) == (SENT, 0, 250), "ok: outcome sent with 250")
    expect(fake.connections == 1, "ok: exactly one connection")
    transcript = fake.transcript
    expect(any(line.startswith(b"EHLO") for line in transcript), "ok: EHLO was sent")
    expect(
        any(line.startswith(b"MAIL FROM:<" + FROM.encode() + b">") for line in transcript),
        "ok: MAIL FROM names the configured sender",
    )
    expect(
        any(line.startswith(b"RCPT TO:<" + TO.encode() + b">") for line in transcript),
        "ok: RCPT TO names the recipient",
    )
    expect(b"DATA" in transcript and b"QUIT" in transcript, "ok: DATA and QUIT were sent")
    expect(len(fake.data) == 1, "ok: one message body was captured")
    data = fake.data[0]
    for header in (
        b"From: " + FROM.encode() + b"\r\n",
        b"To: " + TO.encode() + b"\r\n",
        b"Subject: Duris account password reset\r\n",
        b"MIME-Version: 1.0\r\n",
        b"Content-Type: text/plain; charset=utf-8\r\n",
        b"Content-Transfer-Encoding: 7bit\r\n",
        b"Date: ",
    ):
        expect(header in data, f"ok: header present: {header[:24]!r}")
    expect(
        re.search(rb"^Message-ID: <[0-9a-f]{32}@duris\.test>\r$", data, re.MULTILINE) is not None,
        "ok: Message-ID is 32 hex at the sender domain",
    )
    expect(data.count(b"\n") == data.count(b"\r\n"), "ok: every line is CRLF-terminated")
    code = read_code(code_file)
    expect(dashed(code).encode() in data, "ok: the marker code reached the relay in 8-8-8-8 form")
    printed = completed.stdout + completed.stderr
    expect(code not in printed and dashed(code) not in printed, "ok: the code was never printed")

    # ---- mode ok, worker round trip ----
    with FakeSmtpServer("ok") as fake:
        completed, _ = run_harness(binary, fake.port, code_file, "worker")
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect((outcome, smtp_code) == (SENT, 250), "worker: completion is sent with 250")
    expect("sent=1 " in completed.stdout, "worker: health counts one sent mail")
    expect("terminate called" not in completed.stdout + completed.stderr, "worker: clean exit")
    expect(fake.connections == 1 and len(fake.data) == 1, "worker: one connection, one message")
    code = read_code(code_file)
    expect(dashed(code).encode() in fake.data[0], "worker: the marker code reached the relay")
    expect(code not in completed.stdout + completed.stderr, "worker: the code was never printed")

    # ---- RCPT refused: terminal, one connection, no retry ----
    with FakeSmtpServer("rcpt550") as fake:
        completed, _ = run_harness(binary, fake.port, code_file, "direct")
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect((outcome, smtp_code) == (TERMINAL, 550), "rcpt550: terminal failure with 550")
    expect(fake.connections == 1, "rcpt550: exactly one connection (no retry)")
    expect(not fake.data, "rcpt550: no message body was transmitted")

    # ---- message refused after DATA: retryable, one connection, no retry ----
    with FakeSmtpServer("data451") as fake:
        completed, _ = run_harness(binary, fake.port, code_file, "direct")
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect((outcome, smtp_code) == (RETRYABLE, 451), "data451: retryable failure with 451")
    expect(fake.connections == 1, "data451: exactly one connection (no retry)")
    expect(len(fake.data) == 1, "data451: the body was transmitted once")

    # ---- silent relay: the total timeout bounds the send ----
    with FakeSmtpServer("stall") as fake:
        completed, elapsed = run_harness(binary, fake.port, code_file, "direct", total_timeout_ms=3000)
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect(
        (outcome, curl_code, smtp_code) == (RETRYABLE, CURLE_OPERATION_TIMEDOUT, 0),
        "stall: retryable with CURLE_OPERATION_TIMEDOUT",
    )
    expect(elapsed < 3.0 + 3.0, f"stall: wall time {elapsed:.1f} s stays within timeout + 3 s")
    expect(fake.connections == 1, "stall: exactly one connection")

    # ---- CRLF in the recipient never reaches the wire ----
    with FakeSmtpServer("ok") as fake:
        completed, _ = run_harness(binary, fake.port, code_file, "crlf")
    outcome, curl_code, smtp_code = parse_result(completed.stdout)
    expect((outcome, curl_code) == (TERMINAL, 0), "crlf: refused before libcurl with curl_code 0")
    expect(fake.connections == 0, "crlf: the relay saw no connection")

print("account recovery live SMTP regression passed")
