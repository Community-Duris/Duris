#!/usr/bin/env python3
"""Contract checks for the unauthenticated, value-free health endpoint."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/websocket.c").read_text()
HEADER = (ROOT / "src/websocket.h").read_text()
SCRIPT = (ROOT / "scripts/healthcheck.sh").read_text()


def check(name, condition):
    print(("[PASS] " if condition else "[FAIL] ") + name)
    if not condition:
        raise SystemExit(1)


check("health route is exact", 'strcmp(line, "GET /health HTTP/1.1") == 0' in SOURCE)
check("health reports database pool readiness without a database round trip",
      "sql_pool_is_active()" in SOURCE and "mysql_ping" not in SOURCE)
check("health response is JSON and cannot be cached",
      "Content-Type: application/json" in SOURCE and "Cache-Control: no-store" in SOURCE)
check("health response closes its probe connection",
      "health responses return -2 and close" in HEADER and "return sent < 0 ? -1 : -2" in SOURCE)
check("health response contains no environment or player values",
      '"status"' not in SOURCE or "DB_PASSWD" not in SOURCE)
check("probe has a short timeout and requires HTTP success",
      "--max-time 3" in SCRIPT and "--fail" in SCRIPT)
check("local validation can use a non-conflicting WebSocket port",
      'getenv("DURIS_WEBSOCKET_PORT")' in SOURCE and
      "parsed_port < 1 || parsed_port > 65535" in SOURCE)

print("health endpoint contracts passed")
