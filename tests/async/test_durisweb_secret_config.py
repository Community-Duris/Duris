"""Source contracts for fail-closed DurisWeb secret configuration."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WS = (ROOT / "src/ws_handlers.c").read_text()
GMCP = (ROOT / "src/gmcp.c").read_text()
AUTH = (ROOT / "src/ws_auth.h").read_text()
SQL = (ROOT / "src/sql.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
EXAMPLE = (ROOT / ".env.example").read_text()


for source in (WS, GMCP, AUTH):
    assert "DURISWEB_SECRET_DEFAULT" not in source

assert '#include "ws_auth.h"' in WS
assert '#include "ws_auth.h"' in GMCP
assert 'getenv("DURISWEB_SECRET")' in AUTH
assert "if (!secret || !*secret || !sig || strlen(sig) != 64 || !challenge" in AUTH
assert "char expected[65];" in AUTH
assert "expected[64] = '\\0';" in AUTH
assert "CRYPTO_memcmp(sig, expected, 64)" in AUTH
assert 'getenv("DURISWEB_SECRET_PREVIOUS")' in AUTH
assert "RAND_bytes" in AUTH
assert '"%ld:%s", minute, challenge' in AUTH

# The example contains a placeholder only; the runtime reads .env, never the
# template. The server also loads .env when launched without cycle_mud.sh.
assert "DURISWEB_SECRET=put-secret-here" in EXAMPLE
assert 'open(".env", O_RDONLY | O_CLOEXEC | O_NOFOLLOW)' in SQL
assert ".env.example" not in SQL
assert "if (load_env_file() < 0)" in COMM

print("DurisWeb secret configuration contracts passed")
