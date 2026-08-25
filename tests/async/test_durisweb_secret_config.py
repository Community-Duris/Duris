"""Source contracts for fail-closed DurisWeb secret configuration."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
WS = (ROOT / "src/ws_handlers.c").read_text()
GMCP = (ROOT / "src/gmcp.c").read_text()
SQL = (ROOT / "src/sql.c").read_text()
COMM = (ROOT / "src/comm.c").read_text()
EXAMPLE = (ROOT / ".env.example").read_text()


for source in (WS, GMCP):
    assert "DURISWEB_SECRET_DEFAULT" not in source
    assert 'getenv("DURISWEB_SECRET")' in source
    assert "if (!secret || !*secret)" in source
    assert "return NULL;" in source

    verifier = source.split("static int verify_durisweb_sig", 1)[1].split("\n}", 1)[0]
    assert "if (!secret)" in verifier
    assert verifier.index("if (!secret)") < verifier.index("strlen(secret)")

# The example contains a placeholder only; the runtime reads .env, never the
# template. The server also loads .env when launched without cycle_mud.sh.
assert "DURISWEB_SECRET=put-secret-here" in EXAMPLE
assert 'fopen(".env", "r")' in SQL
assert ".env.example" not in SQL
assert "load_env_file();" in COMM

print("DurisWeb secret configuration contracts passed")
