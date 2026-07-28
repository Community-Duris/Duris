from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")
mccp = (ROOT / "src" / "mccp.c").read_text(encoding="utf-8", errors="replace")

# Fragmented Telnet commands must remain buffered until the next socket read.
assert "memmove(bp, buf + i, len - i);" in comm
assert "bp += len - i;" in comm

# The parser must not inspect the command or option byte until present.
assert "if (buflen < 2)" in mccp
assert mccp.count("if (buflen < 3)") >= 5
