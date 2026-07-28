from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
comm = (ROOT / "src" / "comm.c").read_text(encoding="utf-8", errors="replace")

# Optional reverse DNS must be bounded and fail open to the numeric address.
assert "MAX_HOSTNAME_LOOKUP_WORKERS" in comm
assert "hostname_lookup_workers" in comm
assert "hostname_lookup_workers >= MAX_HOSTNAME_LOOKUP_WORKERS" in comm

# SSL setup failure must close the accepted descriptor before returning.
ssl_failure = comm[comm.index("/* SSL connection - initialize TLS */"):comm.index("used_descs++;", comm.index("/* SSL connection - initialize TLS */"))]
assert "shutdown(desc, 2);" in ssl_failure
assert "close(desc);" in ssl_failure

# Optional worker failures must never deliberately crash the whole MUD.
resolver = comm[comm.index("struct hostname_lookup_request"):comm.index("int new_descriptor")]
assert "SIGSEGV" not in resolver
assert "panic_corruption" not in resolver
