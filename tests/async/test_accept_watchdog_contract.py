from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / "src" / "comm.c").read_text()

assert "static int drain_new_connections" in src
assert "return accepted_count;" in src
assert "/* Nonblocking accept is the authoritative readiness check. */" in src
for call in (
    'drain_new_connections(s, 0, "Telnet")',
    'drain_new_connections(S, 1, "SSL")',
    'drain_new_connections(WS, 2, "WebSocket")',
):
    assert call in src
assert "if (FD_ISSET(s, &input_set))\n\t\t\tdrain_new_connections" not in src
assert "if (FD_ISSET(S, &input_set))\n\t\t\tdrain_new_connections" not in src
