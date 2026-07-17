from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")
websocket = (ROOT / "src" / "websocket.c").read_text(encoding="utf-8", errors="replace")
readme = (ROOT / "README.md").read_text(encoding="utf-8", errors="replace")

assert "resolve_descriptor_hostname_async" in comm
assert "resolve_descriptor_hostname_async" in websocket
assert "host %s | sed" not in comm
assert "host %s | sed" not in websocket
assert "system(Gbuf1)" not in comm
assert "system(cmd)" not in websocket
assert "bind9-host" not in readme
assert "getnameinfo" in comm
assert "pthread_create" in comm
assert '"lib/etc/hosts/%d.%s"' in comm
assert '"lib/etc/hosts/%d.%s"' in (ROOT / "src" / "actinf.c").read_text(encoding="utf-8", errors="replace")
