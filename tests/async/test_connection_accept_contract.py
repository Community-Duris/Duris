from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (SRC / "comm.c").read_text(encoding="utf-8", errors="replace")
proto = (SRC / "prototypes.h").read_text(encoding="utf-8", errors="replace")

assert "listen(s, SOMAXCONN)" in comm
assert "nonblock(s);" in comm
assert "MAX_ACCEPTS_PER_PULSE" in comm
assert "drain_new_connections" in comm
assert "for (int attempt = 0; attempt < MAX_ACCEPTS_PER_PULSE; attempt++)" in comm
assert "accepted_count++;" in comm
assert "int new_connection(int s);" in proto
assert "int new_connection(int s, bool ssl);" not in proto
