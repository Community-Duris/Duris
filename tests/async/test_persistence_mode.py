#!/usr/bin/env python3
"""Runtime and build-contract tests for persistence backend selection."""

from _paths import SRC
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = SRC / "persistence_mode.c"
HEADER_DIR = SRC
MAKEFILE = (SRC / "Makefile").read_text()

PROBE = r'''
#include "persistence/persistence_mode.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_ip_activity_repository.h"
#endif

#include <cstdlib>
#include <cstring>
#include <string>

#ifdef __NO_MYSQL__
flatfile_ip_activity_result flatfile_ip_activity_reset_active(const char *, int64_t,
                                                              std::string *)
{
    return flatfile_ip_activity_result::ok;
}
#endif

int main(int argc, char **argv)
{
    char error[2048] = {};
    const char *scenario = argc > 1 ? argv[1] : "default";

    unsetenv("PERSISTENCE_MODE");
    unsetenv("FLATFILE_STATE_DIR");

    if (!std::strcmp(scenario, "default")) {
#ifdef __NO_MYSQL__
        return !persistence_mode_configure(error, sizeof(error)) &&
               std::strstr(error, "requires a MariaDB client build") ? 0 : 1;
#else
        return persistence_mode_configure(error, sizeof(error)) &&
               persistence_mode_get() == PERSISTENCE_MODE_MARIADB_PRIMARY &&
               persistence_mode_requires_mysql() &&
               !std::strcmp(persistence_mode_name(), "mariadb-primary") ? 0 : 1;
#endif
    }

    if (!std::strcmp(scenario, "invalid")) {
        setenv("PERSISTENCE_MODE", "automatic", 1);
        return !persistence_mode_configure(error, sizeof(error)) &&
               std::strstr(error, "invalid PERSISTENCE_MODE") ? 0 : 1;
    }

    setenv("PERSISTENCE_MODE", scenario, 1);
    if (!std::strcmp(scenario, "mariadb-primary-flatfile-fallback"))
        return !persistence_mode_configure(error, sizeof(error)) &&
               std::strstr(error, "is not supported") ? 0 : 1;
    if (argc > 2)
        setenv("FLATFILE_STATE_DIR", argv[2], 1);
    const bool configured = persistence_mode_configure(error, sizeof(error));
#ifndef __NO_MYSQL__
    return !configured &&
           std::strstr(error, "requires a client-free flatfile build") ? 0 : 1;
#else
    if (argc == 2)
        return std::strstr(error, "FLATFILE_STATE_DIR is required") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "insecure"))
        return std::strstr(error, "permissions must be 0700 or stricter") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "relative"))
        return std::strstr(error, "must be an absolute path") ? 0 : 1;
    if (argc > 3 && !std::strcmp(argv[3], "symlink"))
        return std::strstr(error, "is not a directory") ? 0 : 1;
    return configured && persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
           !persistence_mode_requires_mysql() && persistence_mode_flatfile_root() ? 0 : 1;
#endif
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
    database_binary = temp / "database-probe"
    flat_binary = temp / "flat-probe"
    probe.write_text(PROBE)
    common_compile = [
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", f"-I{HEADER_DIR}",
        str(SOURCE), str(probe),
    ]
    subprocess.run([*common_compile, "-o", str(database_binary)], check=True)
    subprocess.run(
        [*common_compile, "-D__NO_MYSQL__", "-o", str(flat_binary)], check=True
    )
    for binary in (database_binary, flat_binary):
        subprocess.run([str(binary), "default"], check=True)
        subprocess.run([str(binary), "invalid"], check=True)
        subprocess.run([str(binary), "flatfile-primary"], check=True)
        subprocess.run(
            [str(binary), "mariadb-primary-flatfile-fallback"], check=True
        )
    subprocess.run(
        [str(flat_binary), "flatfile-primary", "relative", "relative"], check=True
    )

    state = temp / "flatfile-primary"
    subprocess.run([str(flat_binary), "flatfile-primary", str(state)], check=True)
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
        [str(flat_binary), "flatfile-primary", str(insecure), "insecure"],
        capture_output=True,
    )
    assert result.returncode == 0

    symlink_target = temp / "symlink-target"
    symlink_target.mkdir(mode=0o700)
    symlink = temp / "symlink"
    symlink.symlink_to(symlink_target, target_is_directory=True)
    subprocess.run(
        [str(flat_binary), "flatfile-primary", str(symlink), "symlink"], check=True
    )

    flat_build = temp / "flat-build"
    dry_run = subprocess.run(
        [
            "make",
            "-C",
            str(SRC),
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
    assert "-I./no_mysql" in dry_run
    assert "/usr/include/mysql" not in dry_run
    assert "-lmysqlclient" not in dry_run

print("persistence mode runtime and build contract passed")
