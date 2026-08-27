#!/usr/bin/env python3
"""Bounded line-protocol client used by the qualified Session 14 load adapter."""

from __future__ import annotations

import socket
import time
from dataclasses import dataclass


MAX_LINE_BYTES = 256
MAX_COMMANDS = 128


@dataclass(frozen=True)
class ClientIdentity:
    account: str
    password: str
    character: str


def _line(value: str) -> bytes:
    if not value or "\n" in value or "\r" in value or len(value.encode("utf-8")) > MAX_LINE_BYTES:
        raise ValueError("invalid client line")
    return value.encode("utf-8") + b"\n"


def run_client(host: str, port: int, identity: ClientIdentity, commands: list[str],
               dwell_seconds: float, timeout_seconds: float = 5.0) -> dict[str, int | str]:
    if len(commands) > MAX_COMMANDS or dwell_seconds < 0:
        raise ValueError("invalid client schedule")
    started = time.monotonic()
    sent = 0
    received = 0
    with socket.create_connection((host, port), timeout=timeout_seconds) as connection:
        connection.settimeout(timeout_seconds)
        for value in (identity.account, identity.password, identity.character):
            connection.sendall(_line(value))
            sent += 1
        for command in commands:
            connection.sendall(_line(command))
            sent += 1
            if dwell_seconds:
                time.sleep(dwell_seconds)
        connection.sendall(b"quit\n")
        sent += 1
        try:
            while connection.recv(4096):
                received += 1
                if received >= 64:
                    break
        except (TimeoutError, socket.timeout):
            pass
    return {
        "state": "completed",
        "sent_lines": sent,
        "receive_chunks": received,
        "elapsed_msec": int((time.monotonic() - started) * 1000),
    }
