from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")

assert "desc >= FD_SETSIZE" in comm
assert "Accepted descriptor %d exceeds FD_SETSIZE" in comm
