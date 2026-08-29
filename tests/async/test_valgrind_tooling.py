"""Contracts for the Valgrind tooling.

Valgrind is a declared development dependency, and `scripts/valgrind_mud.sh`
is the only supported way to run the game under it.  The checks below pin the
parts that are dangerous or easy to lose: the refusal to bind the production
port, the suppression file being applied and staying free of Duris frames,
and the dependency being declared where a fresh clone will see it.
"""

import os
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/valgrind_mud.sh"
SUPP = ROOT / "scripts/valgrind.supp"
DOC = ROOT / "docs/guides/valgrind.md"

# --------------------------------------------------------------------------
# 1. The runner exists, is executable, and defaults to a development port.
# --------------------------------------------------------------------------
assert SCRIPT.is_file(), "scripts/valgrind_mud.sh is missing"
assert os.access(SCRIPT, os.X_OK), "scripts/valgrind_mud.sh is not executable"

body = SCRIPT.read_text()
assert 'PORT="4000"' in body, "the default port is no longer the development port 4000"
assert "--suppressions=" in body and "scripts/valgrind.supp" in body, (
    "the runner no longer applies scripts/valgrind.supp"
)
assert "logs/valgrind" in body, "reports must be written under logs/valgrind/"
assert '--minimal)          SERVER_ARGS+=("--minimal")' in body
assert '--server-arg=*)     SERVER_ARGS+=("${1#*=}")' in body
assert '${SERVER_ARGS[@]+"${SERVER_ARGS[@]}"} "$PORT"' in body

subprocess.run(["bash", "-n", str(SCRIPT)], check=True)

# --------------------------------------------------------------------------
# 2. Guard rails.  Port 7777 selects the production database in src/sql.c, and
#    a Valgrind run is expected to be slow and killed mid-session, so it must
#    never be allowed to bind it.
# --------------------------------------------------------------------------
def run(*args):
    return subprocess.run(
        ["bash", str(SCRIPT), *args], cwd=ROOT, capture_output=True, text=True
    )

prod = run("--port", "7777")
assert prod.returncode == 2, f"--port 7777 exited {prod.returncode}, expected 2"
assert "production" in prod.stderr, f"unhelpful refusal: {prod.stderr!r}"

for bad, why in (("--tool=bogus", "unknown tool"), ("--port=abc", "non-numeric port"),
                 ("--nonsense", "unknown option")):
    r = run(bad)
    assert r.returncode == 2, f"{why} ({bad}) exited {r.returncode}, expected 2"

helped = run("--help")
assert helped.returncode == 0 and "--tool" in helped.stdout

# src/sql.c redirects to the development database only when RUNNING_PORT
# differs from DFLT_PORT, so the refused port must stay in step with config.h.
dflt_port = re.search(r"#define DFLT_PORT\s+(\d+)", (ROOT / "src/config.h").read_text())
assert dflt_port and dflt_port.group(1) == "7777", (
    "DFLT_PORT changed -- update the production-port guard in scripts/valgrind_mud.sh"
)
assert re.search(r"RUNNING_PORT != DFLT_PORT", (ROOT / "src/sql.c").read_text()), (
    "src/sql.c no longer picks the database by port -- recheck the runner's guard"
)

# --------------------------------------------------------------------------
# 3. Suppressions: third-party only, and syntactically valid.
# --------------------------------------------------------------------------
assert SUPP.is_file(), "scripts/valgrind.supp is missing"
supp = SUPP.read_text()
assert supp.count("{") >= 5 and supp.count("{") == supp.count("}")

for line in supp.splitlines():
    stripped = line.strip()
    if stripped.startswith("#") or not stripped:
        continue
    assert "/dms" not in stripped, f"suppressing a Duris frame is forbidden: {line!r}"
    if stripped.startswith("obj:"):
        assert ".so" in stripped, f"only shared libraries may be suppressed: {line!r}"

valgrind = shutil.which("valgrind")
if valgrind:
    # valgrind rejects a malformed suppression file at startup.
    probe = subprocess.run(
        [valgrind, "-q", f"--suppressions={SUPP}", "/bin/true"],
        capture_output=True, text=True,
    )
    assert probe.returncode == 0, f"valgrind rejected the suppression file:\n{probe.stderr}"

# --------------------------------------------------------------------------
# 4. The diagnostic dependencies and docs are declared where a fresh clone
#    will see them. GDB is direct rather than relying on Valgrind's optional
#    package recommendation.
# --------------------------------------------------------------------------
equivs = (ROOT / "packaging/duris-build-deps.equivs").read_text()
depends = equivs.split("Depends:", 1)[1].split("Suggests:", 1)[0]
assert "valgrind" in depends, "valgrind is not a declared build dependency"
assert re.search(r"^\s*gdb,?$", depends, re.MULTILINE), (
    "gdb is not a direct developer dependency"
)

readme = (ROOT / "README.md").read_text()
assert "valgrind" in readme.lower(), "README.md does not mention valgrind"
assert "docs/guides/valgrind.md" in readme, "README.md does not link docs/guides/valgrind.md"

assert DOC.is_file(), "docs/guides/valgrind.md is missing"
doc = DOC.read_text()
assert "scripts/valgrind_mud.sh" in doc and "scripts/valgrind.supp" in doc

makefile = (ROOT / "src/Makefile").read_text()
assert "\nvalgrind: $(PROGS)" in makefile, "src/Makefile lost its valgrind target"
assert "./scripts/valgrind_mud.sh" in makefile

gitignore = (ROOT / ".gitignore").read_text().splitlines()
assert "logs/valgrind/" in gitignore, "logs/valgrind/ must stay untracked"

print("valgrind tooling contracts OK")
