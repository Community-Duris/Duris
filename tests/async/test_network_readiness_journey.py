#!/usr/bin/env python3
"""Measure real listener wakeups with the existing disposable server fixture.

--measure-only permits the same probe to run against the pre-change executable.
The fixture owns its synthetic account/character; no real account, production
configuration, database, or player state is used.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import random
import socket
import ssl
import tempfile
import time

from test_account_recovery_journey import (
    IsolatedServer, MudClient, OLD_PASSWORD, build_flatfile_server, enter_account_name,
)
from test_game_loop_session_journey import CHARACTER, create_character
from test_creation_prompt_journey import require_prompt_framed


def summary(values):
    ordered = sorted(values)
    return {"samples": len(values), "p50_ms": round(ordered[len(values) // 2], 3),
            "p95_ms": round(ordered[min(len(values) - 1, int(len(values) * .95))], 3),
            "max_ms": round(max(values), 3)}


def cpu_seconds(pid):
    fields = Path(f"/proc/{pid}/stat").read_text().split(")", 1)[1].split()
    return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


class TlsMudClient(MudClient):
    def __init__(self, port, context):
        self.socket = context.wrap_socket(
            socket.create_connection(("127.0.0.1", port), timeout=5), server_hostname="localhost")
        self.socket.settimeout(.25)
        self.pending = bytearray()
        self.transcript = bytearray()


def command_spacing(client):
    require_prompt_framed(client, "<> ")
    for _ in range(8):
        client.send("score")
    responses = []
    for _ in range(8):
        # Identify each score response before its prompt so unrelated world
        # notifications cannot masquerade as an executed queued command.
        client.expect(CHARACTER)
        client.expect("Pos: standing >")
        responses.append(time.monotonic())
        require_prompt_framed(client, "<> ")
    return summary([(later - earlier) * 1000
                    for earlier, later in zip(responses, responses[1:])])


def measure(binary, enforce_capacity):
    random_source = random.Random(20261002)
    with IsolatedServer(binary, None) as server:
        ports = server.environment
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        measurements = {}
        for kind in ("telnet_first_byte", "tls_handshake", "websocket_upgrade"):
            values = []
            for _ in range(24):
                time.sleep(random_source.uniform(.015, .17))
                port = (server.plain_port if kind == "telnet_first_byte" else
                        int(ports["DURIS_TLS_PORT"] if kind == "tls_handshake" else
                            ports["DURIS_WEBSOCKET_PORT"]))
                started = time.monotonic()
                with socket.create_connection(("127.0.0.1", port), timeout=5) as client:
                    if kind == "tls_handshake":
                        with context.wrap_socket(client, server_hostname="localhost"):
                            pass
                    elif kind == "websocket_upgrade":
                        client.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                                       b"Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
                                       b"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n")
                        response = client.recv(4096)
                        assert b"101" in response, response
                    else:
                        assert client.recv(1)
                values.append((time.monotonic() - started) * 1000)
            measurements[kind] = summary(values)

        # More than the accept budget, with every connection below the explicit cap.
        clients = []
        try:
            started = time.monotonic()
            for _ in range(96):
                clients.append(socket.create_connection(("127.0.0.1", server.plain_port), timeout=5))
            for client in clients:
                assert client.recv(1)
            measurements["burst_96_first_bytes_ms"] = round((time.monotonic() - started) * 1000, 3)
        finally:
            for client in clients:
                client.close()

        # Let disconnects settle before measuring the idle process, including workers.
        time.sleep(.6)
        before_cpu = cpu_seconds(server.process.pid)
        started = time.monotonic()
        time.sleep(3)
        elapsed = time.monotonic() - started
        measurements["idle_cpu_percent"] = round(
            100 * (cpu_seconds(server.process.pid) - before_cpu) / elapsed, 3)
        measurements["idle_sample_seconds"] = round(elapsed, 3)

        if enforce_capacity:
            clients = []
            try:
                for _ in range(256):
                    clients.append(socket.create_connection(
                        ("127.0.0.1", server.plain_port), timeout=5))
                for connection in clients:
                    assert connection.recv(1), "connection below the declared capacity was refused"
                with socket.create_connection(("127.0.0.1", server.plain_port), timeout=5) as extra:
                    try:
                        refused = not extra.recv(1)
                    except ConnectionResetError:
                        refused = True
                    assert refused, "explicit connection capacity was not enforced"
                measurements["capacity_accepted"] = len(clients)
                measurements["capacity_refused_extra"] = refused
            finally:
                for connection in clients:
                    connection.close()
            time.sleep(.6)

        # Gameplay execution remains one eligible queued command per pulse.
        # Synthetic character creation crosses the real password worker and
        # persistence pipeline before this wire-level cadence measurement.
        client = MudClient(server.plain_port)
        try:
            create_character(client)
            measurements["queued_command_spacing"] = command_spacing(client)
        finally:
            client.close()
        time.sleep(.6)
        client = TlsMudClient(int(ports["DURIS_TLS_PORT"]), context)
        try:
            enter_account_name(client)
            client.expect("enter your password")
            client.send(OLD_PASSWORD)
            client.expect("PRESS RETURN")
            client.send("")
            client.expect("Please select an option")
            client.send("1")
            client.expect(CHARACTER)
            client.send("1")
            # Link-dead bodies reconnect directly; a cold restore confirms entry.
            matched, _ = client.expect_any(("Play as", "Pos: standing >"), timeout=30)
            if matched == "Play as":
                client.send("y")
                client.expect("Pos: standing >", timeout=30)
            measurements["tls_command_spacing"] = command_spacing(client)
        finally:
            client.close()
        return measurements


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path)
    parser.add_argument("--measure-only", action="store_true")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="network-readiness-build-") as directory:
        binary = args.binary.resolve() if args.binary else build_flatfile_server(Path(directory))
        result = measure(binary, not args.measure_only)
    print(json.dumps(result, indent=2), flush=True)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + "\n")
    if not args.measure_only:
        assert result["telnet_first_byte"]["p95_ms"] < 100, result
        assert result["tls_handshake"]["p95_ms"] < 150, result
        assert result["websocket_upgrade"]["p95_ms"] < 100, result
        assert result["burst_96_first_bytes_ms"] < 350, result
        assert result["idle_cpu_percent"] < 10, result
        assert 200 < result["queued_command_spacing"]["p50_ms"] < 350, result
        assert 200 < result["tls_command_spacing"]["p50_ms"] < 350, result
    print("network readiness listener journey passed", flush=True)


if __name__ == "__main__":
    main()
