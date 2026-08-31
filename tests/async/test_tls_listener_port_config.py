#!/usr/bin/env python3
"""Contract checks for an independently configurable TLS telnet listener."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COMM = (SRC / "comm.c").read_text()
ENV_EXAMPLE = (ROOT / ".env.example").read_text()
CONFIGURATION = (ROOT / "docs/operations/CONFIGURATION.md").read_text()

assert 'getenv("DURIS_TLS_PORT")' in COMM
assert "strtol(configured_tls_port, &end, 10)" in COMM
assert "end == configured_tls_port" in COMM
assert "parsed_tls_port < 1" in COMM
assert "parsed_tls_port > 65535" in COMM
assert "parsed_tls_port == port" in COMM
assert 'fatal_boot_error("comm", "DURIS_TLS_PORT is invalid")' in COMM
assert "sslport = static_cast<int>(parsed_tls_port)" in COMM
assert "DURIS_TLS_PORT=" in ENV_EXAMPLE
assert "`DURIS_TLS_PORT`" in CONFIGURATION
