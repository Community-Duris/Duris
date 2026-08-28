#!/usr/bin/env python3
"""Runtime and build-contract tests for persistence backend selection."""

import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src/persistence_mode.c"
HEADER_DIR = ROOT / "src"
MAKEFILE = (ROOT / "src/Makefile").read_text()

PROBE = r'''
#include "persistence_mode.h"

#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char **argv)
{
    char error[2048] = {};
    const char *scenario = argc > 1 ? argv[1] : "default";

    unsetenv("PERSISTENCE_MODE");
    unsetenv("FLATFILE_STATE_DIR");

    if (!std::strcmp(scenario, "default"))
        return persistence_mode_configure(error, sizeof(error)) &&
               persistence_mode_get() == PERSISTENCE_MODE_MARIADB_PRIMARY &&
               persistence_mode_requires_mysql() &&
               !std::strcmp(persistence_mode_name(), "mariadb-primary") ? 0 : 1;

    if (!std::strcmp(scenario, "invalid")) {
        setenv("PERSISTENCE_MODE", "automatic", 1);
        return !persistence_mode_configure(error, sizeof(error)) &&
               std::strstr(error, "invalid PERSISTENCE_MODE") ? 0 : 1;
    }

    setenv("PERSISTENCE_MODE", scenario, 1);
    if (argc > 2)
        setenv("FLATFILE_STATE_DIR", argv[2], 1);
    if (persistence_mode_configure(error, sizeof(error)))
        return 1;
    if (argc == 2)
        return std::strstr(error, "FLATFILE_STATE_DIR is required") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "insecure"))
        return std::strstr(error, "permissions must be 0700 or stricter") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "relative"))
        return std::strstr(error, "must be an absolute path") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "symlink"))
        return std::strstr(error, "is not a directory") ? 0 : 1;
    const bool flatfile_primary = !std::strcmp(scenario, "flatfile-primary");
    return std::strstr(error, "unimplemented durable domains") &&
           persistence_mode_requires_mysql() == !flatfile_primary ? 0 : 1;
}
'''

assert "PERSISTENCE_BACKEND ?= mariadb" in MAKEFILE
assert "CFLAGS += -D__NO_MYSQL__" in MAKEFILE
assert "MYSQL_INCLUDES =" in MAKEFILE
assert "MYSQL_LIBS =" in MAKEFILE
assert "$(MYSQL_INCLUDES)" in MAKEFILE
assert "$(MYSQL_LIBS)" in MAKEFILE

with tempfile.TemporaryDirectory(prefix="persistence-mode-") as directory:
    temp = Path(directory)
    probe = temp / "probe.cpp"
    binary = temp / "probe"
    probe.write_text(PROBE)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{HEADER_DIR}",
            str(SOURCE),
            str(probe),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary), "default"], check=True)
    subprocess.run([str(binary), "invalid"], check=True)
    subprocess.run([str(binary), "flatfile-primary"], check=True)
    subprocess.run([str(binary), "mariadb-primary-flatfile-fallback"], check=True)
    subprocess.run(
        [str(binary), "flatfile-primary", "relative", "relative"], check=True
    )

    for mode in ("flatfile-primary", "mariadb-primary-flatfile-fallback"):
        state = temp / mode
        subprocess.run([str(binary), mode, str(state)], check=True)
        expected = {
            state / "metadata",
            state / "identities/accounts",
            state / "identities/names",
            state / "players",
            state / "operations/wal",
            state / "domains",
            state / "manifests",
        }
        assert all(path.is_dir() for path in expected)
        assert all((path.stat().st_mode & 0o777) == 0o700 for path in expected)

    insecure = temp / "insecure"
    insecure.mkdir(mode=0o755)
    os.chmod(insecure, 0o755)
    result = subprocess.run(
        [str(binary), "flatfile-primary", str(insecure), "insecure"], capture_output=True
    )
    assert result.returncode == 0

    symlink_target = temp / "symlink-target"
    symlink_target.mkdir(mode=0o700)
    symlink = temp / "symlink"
    symlink.symlink_to(symlink_target, target_is_directory=True)
    subprocess.run(
        [str(binary), "flatfile-primary", str(symlink), "symlink"], check=True
    )

    flat_build = temp / "flat-build"
    dry_run = subprocess.run(
        [
            "make",
            "-C",
            str(ROOT / "src"),
            "PERSISTENCE_BACKEND=flatfile",
            f"BIN_ROOT={flat_build}",
            f"OBJDIR={flat_build / 'objects/server'}",
            f"DMS_BINARY={flat_build / 'server/dms_new'}",
            "-n",
        ],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    assert "-D__NO_MYSQL__" in dry_run
    assert "/usr/include/mysql" not in dry_run
    assert "-lmysqlclient" not in dry_run

print("persistence mode runtime and build contract passed")
