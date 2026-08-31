from _paths import SRC
from pathlib import Path

source = (SRC / "utility.c").read_text()
start = source.index("int persistence_replay_fallback_events")
active = source[start:].split("#if 0", 1)[0]

assert "raw_execution_disabled" in active
assert "persistence_quarantine_fallback_events" in active
assert "sql_persistence_write" not in active
assert "fopen" not in active

print("fallback replay retirement checks passed")
