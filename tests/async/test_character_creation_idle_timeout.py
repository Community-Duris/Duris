#!/usr/bin/env python3
"""Character-creation decisions must not inherit the one-minute login timeout."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
comm = (SRC / "comm.c").read_text()
config = (SRC / "config.h").read_text()

short_start = comm.index("short protocol/login transitions retain a 60 second timeout")
creation_start = comm.index("slightly more involved, 10 minute timeout", short_start)
default_start = comm.index("for remaining states, 15 minutes", creation_start)
short_bucket = comm[short_start:creation_start]
creation_bucket = comm[creation_start:default_start]

assert "#define WAIT_SEC 4" in config
assert "point->wait > 240" in short_bucket
assert "point->wait > 2400" in creation_bucket

for state in ("CON_APPROPRIATE_NAME", "CON_NAME_CONF", "CON_GET_SEX"):
    assert state not in short_bucket, f"{state} still times out after one minute"
    assert state in creation_bucket, f"{state} is not covered by the ten-minute timeout"

print("character-creation idle timeout contract passed")
