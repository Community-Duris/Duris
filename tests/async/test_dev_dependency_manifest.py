"""Contracts for the Debian/Ubuntu developer dependency manifest."""

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "packaging/duris-build-deps.equivs"

body = MANIFEST.read_text()
depends_match = re.search(r"^Depends:\s*(.*?)(?=^[A-Z][A-Za-z-]*:)", body, re.MULTILINE | re.DOTALL)
assert depends_match, "the dependency manifest has no Depends field"
depends = depends_match.group(1)

# Keep the generated filename used by the README and CI deterministic.
assert re.search(r"^Version:\s*1\.0\s*$", body, re.MULTILINE)
assert re.search(r"^Architecture:\s*all\s*$", body, re.MULTILINE)

# These commands are invoked directly by the maintained developer workflow.
for package in (
    "build-essential",
    "git",
    "python3",
    "dos2unix",
    "gawk",
    "libncurses-dev",
    "valgrind",
    "gdb",
    "clang-format",
):
    assert re.search(rf"(^|[,|])\s*{re.escape(package)}\s*(?=[,|]|$)", depends), (
        f"{package} is not a direct developer dependency"
    )

# Preserve an already-installed database family while using the distribution
# defaults on a fresh host.
for alternatives in (
    "default-libmysqlclient-dev | libmariadb-dev-compat",
    "default-mysql-client | mariadb-client",
    "default-mysql-server | mariadb-server",
):
    assert alternatives in depends, f"missing database alternatives: {alternatives}"

# equivs builds this metapackage; depending on it would not solve bootstrap.
assert not re.search(r"(^|[,|])\s*equivs\s*(?=[,|]|$)", depends)

readme = (ROOT / "README.md").read_text()
assert "sudo apt install equivs" in readme
assert "sudo apt install equivs dos2unix" not in readme
assert "duris-build-deps_1.0_all.deb" in readme

workflow = (ROOT / ".github/workflows/build.yml").read_text()
assert "make build-deps-package" in workflow
assert "./bin/packages/duris-build-deps_1.0_all.deb" in workflow
assert "sudo apt install equivs dos2unix" not in workflow

assert "dos2unix" in (ROOT / "areas/make_lookup").read_text()
assert "python3" in (ROOT / "docs/TESTING.md").read_text()

print("developer dependency manifest contracts OK")
