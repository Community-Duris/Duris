#!/usr/bin/env python3
"""Regression test for the WebSocket listener's address default.

websocket_listener_address() required DURIS_WEBSOCKET_LISTEN_ADDRESS or
LISTEN_ADDRESS to be set.  A deployment that configured only a port logged
"WebSocket listener address is invalid" / "WARNING: WebSocket server failed to
start on port 4050" and healthcheck.sh failed with "Failed to connect to
127.0.0.1 port 4050", while the telnet listener started fine because
runtime_listener_address() falls back to in6addr_any.

The listener now falls back to loopback, which is strictly narrower than the
telnet default and still satisfies the production loopback requirement, so an
unset address can no longer silently cost a deployment its WebSocket listener.
"""

from _paths import SRC
from pathlib import Path
import sys

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
websocket = (SRC / "websocket.c").read_text(encoding="utf-8", errors="replace")
example = (ROOT / ".env.example").read_text(encoding="utf-8", errors="replace")
configuration = (ROOT / "docs" / "operations" / "CONFIGURATION.md").read_text(
    encoding="utf-8", errors="replace")

start = index(websocket, "static int websocket_listener_address(")
resolver = websocket[start:index(websocket, "\n}", start)]

checks = [
    ("the default listener address is loopback",
     contains(websocket, '#define WEBSOCKET_DEFAULT_LISTEN_ADDRESS "127.0.0.1"')),
    ("the resolver falls back to it instead of failing",
     contains(resolver, "value = WEBSOCKET_DEFAULT_LISTEN_ADDRESS;") and
     not contains(resolver, "if (!value || !*value || !address)\n\t\treturn 0;")),
    ("the configured environment variables still win",
     index(resolver, 'getenv("DURIS_WEBSOCKET_LISTEN_ADDRESS")') <
     index(resolver, 'getenv("LISTEN_ADDRESS")') <
     index(resolver, "WEBSOCKET_DEFAULT_LISTEN_ADDRESS;")),
    ("a non-numeric address is still rejected",
     contains(resolver, "if (inet_pton(AF_INET, value, &ipv4) != 1)\n\t\t\treturn 0;")),
    ("production still demands an explicit loopback bind and a trusted proxy",
     contains(websocket, "!websocket_address_is_loopback(listen_address) || "
                         '!getenv("DURIS_TRUSTED_PROXY_IP")')),
    ("the override remains documented",
     "DURIS_WEBSOCKET_LISTEN_ADDRESS" in example and
     "DURIS_WEBSOCKET_LISTEN_ADDRESS" in configuration and
     "127.0.0.1" in configuration),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll WebSocket listener default checks passed successfully.")
