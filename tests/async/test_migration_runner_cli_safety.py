#!/usr/bin/env python3
"""Legacy migration runner CLI and local Redis target safety contracts."""

from pathlib import Path
import os
import subprocess


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "migrations/run_migration.sh"
SOURCE = RUNNER.read_text()
CLEAR_REDIS = (ROOT / "scripts/clear-redis.sh").read_text()


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
assert ': "${REDIS_HOST:?REDIS_HOST is required}"' in SOURCE
assert ': "${REDIS_PORT:?REDIS_PORT is required}"' in SOURCE
assert 'redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT"' in SOURCE
assert "FAILED=$((FAILED + 1))" in SOURCE
assert "redis-cli FLUSHDB" not in SOURCE
assert '[[ -L "$ENV_FILE" ]]' in CLEAR_REDIS
assert "stat -c '%a'" in CLEAR_REDIS
assert ': "${REDIS_HOST:?REDIS_HOST is required}"' in CLEAR_REDIS
assert ': "${REDIS_PORT:?REDIS_PORT is required}"' in CLEAR_REDIS
assert 'redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" FLUSHDB' in CLEAR_REDIS
assert "redis-cli FLUSHDB" not in CLEAR_REDIS

print("legacy migration CLI and configured Redis target safety: ok")
