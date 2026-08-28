#!/usr/bin/env python3
"""Legacy migration runner CLI and local Redis target safety contracts."""

from pathlib import Path
import os
import subprocess


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "migrations/run_migration.sh"
SOURCE = RUNNER.read_text()
CLEAR_REDIS = (ROOT / "scripts/clear-redis.sh").read_text()
SCOPED_CLEAR = (ROOT / "scripts/clear-duris-redis-keys.sh").read_text()


helped = subprocess.run(
    ["bash", str(RUNNER), "--help"],
    cwd=ROOT,
    env={"PATH": os.environ["PATH"]},
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    check=False,
)
assert helped.returncode == 0
assert "usage:" in helped.stdout
assert "missing migration configuration" not in helped.stderr

unknown = subprocess.run(
    ["bash", str(RUNNER), "--not-a-real-option"],
    cwd=ROOT,
    env={"PATH": os.environ["PATH"]},
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    check=False,
)
assert unknown.returncode == 2
assert "unknown argument" in unknown.stderr

assert "ALTER TABLE players_core" not in SOURCE
assert '[[ "${REDIS:-FALSE}" != "TRUE" ]]' in SOURCE
assert '"$PROJECT_ROOT/scripts/clear-duris-redis-keys.sh"' in SOURCE
assert "FAILED=$((FAILED + 1))" in SOURCE
assert "FLUSHDB" not in SOURCE
assert '[[ -L "$ENV_FILE" ]]' in CLEAR_REDIS
assert "stat -c '%a'" in CLEAR_REDIS
assert "--confirm <host:port/database|unix:/absolute/socket/database>" in CLEAR_REDIS
assert "REDIS_ALLOWED_TARGETS" in CLEAR_REDIS
assert "clear-duris-redis-keys.sh" in CLEAR_REDIS
assert 'PATTERNS=("$REDIS_NAMESPACE:*" \'mud:*\' \'ship:snapshot:*\')' in SCOPED_CLEAR
assert "ENVIRONMENT must be local" in SCOPED_CLEAR
assert "REDIS_DB must be an integer" in SCOPED_CLEAR
assert "non-loopback Redis requires REDIS_TLS=TRUE" in SCOPED_CLEAR
assert "REDIS_SOCKET is mutually exclusive" in SCOPED_CLEAR
assert "REDISCLI_AUTH" in SCOPED_CLEAR
assert "redis-cli is required" in SCOPED_CLEAR
assert "FLUSHDB" not in CLEAR_REDIS and "FLUSHDB" not in SCOPED_CLEAR

print("legacy migration CLI and configured Redis target safety: ok")
