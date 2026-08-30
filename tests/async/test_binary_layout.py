"""Contracts for keeping every compiled repository artifact under bin/."""

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "bin"

assert (BIN / ".gitignore").read_text().splitlines()[-2:] == ["*", "!.gitignore"]

expected_routes = {
    "src/Makefile": (
        "$(BIN_ROOT)/objects/server/$(PERSISTENCE_BACKEND)",
        "$(SERVER_BIN_DIR)/dms_new",
        "$(TOOL_BIN_DIR)/pfile",
    ),
    "scripts/build-san.sh": (
        "bin/objects/server-san",
        "bin/server/dms_san",
    ),
    "scripts/cycle_mud.sh": (
        'SERVER_BIN_DIR="bin/server"',
        'BINARY_HISTORY_DIR="$SERVER_BIN_DIR/history"',
    ),
    "areas/src/Makefile": ("$(BIN_ROOT)/areas/tools", "$(BIN_ROOT)/objects/areas/tools"),
    "areas/de/src/Makefile": ("$(BIN_ROOT)/areas/editor", "$(BIN_ROOT)/objects/areas/editor"),
    "src-migrate/Makefile": ("$(BIN_ROOT)/migrations", "$(BIN_ROOT)/objects/migrations"),
    "Makefile": ("bin/packages", "bin/areas/tools"),
    "tests/async/run_signal_handlers.sh": ("bin/tests/test_signal_handlers",),
}

for relative, needles in expected_routes.items():
    body = (ROOT / relative).read_text()
    for needle in needles:
        assert needle in body, f"{relative} does not route through {needle}"

# No compiled artifact may be checked into a source or documentation tree.
tracked = subprocess.run(
    ["git", "ls-files", "-z"],
    cwd=ROOT,
    check=True,
    capture_output=True,
).stdout.split(b"\0")
offenders = []
for raw_path in tracked:
    if not raw_path:
        continue
    relative = Path(raw_path.decode())
    path = ROOT / relative
    if not path.is_file():
        continue
    magic = path.read_bytes()[:8]
    if magic.startswith(b"\x7fELF") or magic == b"!<arch>\n":
        offenders.append(str(relative))

assert not offenders, "tracked compiled artifacts outside ignored bin/: " + ", ".join(offenders)

# Dry runs prove the maintained build entry points emit under bin/ even from a
# clean checkout where no output directories exist yet.
for command, required in (
    (["make", "-C", "src", "-B", "-n", "dms_new"], "bin/server/dms_new"),
    (["make", "-C", "areas/src", "-B", "-n"], "bin/areas/tools/make_mob"),
    (["make", "-C", "areas/de/src", "-B", "-n"], "bin/areas/editor/de"),
    (
        ["make", "-C", "src-migrate", "-B", "-n", "pfile_converter"],
        "bin/migrations/pfile_converter",
    ),
):
    output = subprocess.run(
        command,
        cwd=ROOT,
        check=True,
        text=True,
        capture_output=True,
    ).stdout
    assert required in output, f"{' '.join(command)} does not emit {required}"

# MariaDB and client-free builds use incompatible compile definitions and link
# dependencies. Their objects must remain isolated even though both publish the
# maintained server path, and switching modes must invalidate that shared binary.
makefile = (ROOT / "src/Makefile").read_text()
assert "BACKEND_STAMP" in makefile
for backend in ("mariadb", "flatfile"):
    output = subprocess.run(
        [
            "make",
            "-C",
            "src",
            "-B",
            "-n",
            "dms_new",
            f"PERSISTENCE_BACKEND={backend}",
        ],
        cwd=ROOT,
        check=True,
        text=True,
        capture_output=True,
    ).stdout
    assert f"bin/objects/server/{backend}" in output
    assert f"backend='{backend}'" in output

# Cleaning the isolated sanitizer build must not remove normal utility or
# migration binaries.
san_clean = subprocess.run(
    [
        "make",
        "-C",
        "src",
        "-n",
        "clean-server",
        "OBJDIR=../bin/objects/server-san",
        "DMS_BINARY=../bin/server/dms_san",
    ],
    cwd=ROOT,
    check=True,
    text=True,
    capture_output=True,
).stdout
assert "bin/server/dms_san" in san_clean
assert "bin/tools/pfile" not in san_clean
assert "bin/migrations" not in san_clean

print("binary layout contracts OK")
