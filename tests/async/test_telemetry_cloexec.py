#!/usr/bin/env python3
"""Compile the production telemetry descriptor contract against syscall stubs.

Unit coverage only: no database, sockets, live service, or timing dependency.
Requires a POSIX C++20 compiler for the descriptor flag constants.
"""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


HARNESS = r'''
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <fcntl.h>
#include <initializer_list>

static int fake_fcntl(int fd, int command, ...);
#define fcntl fake_fcntl
#include "sql/sql_telemetry_connection.h"
#undef fcntl

static MYSQL connection{SOCKET_INITIALIZER};
static int gets, sets;
static int original_flags, written_flags;
static bool fail_get, fail_set;
static constexpr int socket_fd = 31;

static int fake_fcntl(int fd, int command, ...)
{
    assert(fd == socket_fd);
    if (command == F_GETFD) {
        ++gets;
        if (fail_get) {
            errno = EBADF;
            return -1;
        }
        return original_flags;
    }
    assert(command == F_SETFD);
    ++sets;
    va_list args;
    va_start(args, command);
    written_flags = va_arg(args, int);
    va_end(args);
    if (fail_set) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static void reset()
{
    gets = sets = 0;
    written_flags = -1;
    original_flags = 0;
    fail_get = fail_set = false;
}

int main()
{
    // An unknown descriptor flag models any platform's additional flags.
    for (int flags : {0, FD_CLOEXEC, 0x400, 0x400 | FD_CLOEXEC}) {
        reset();
        original_flags = flags;
        assert(sql_telemetry_set_cloexec(&connection));
        assert(gets == 1 && sets == 1);
        assert(written_flags == (flags | FD_CLOEXEC));
    }
    reset();
    fail_get = true;
    assert(!sql_telemetry_set_cloexec(&connection));
    assert(gets == 1 && sets == 0);
    reset();
    fail_set = true;
    assert(!sql_telemetry_set_cloexec(&connection));
    assert(gets == 1 && sets == 1);
}
'''


CLIENTS = {
    "oracle": (
        "struct NET { int fd; };\n"
        "struct MYSQL { NET net; };\n",
        "31",
    ),
    "mariadb_base": (
        "#define MARIADB_BASE_VERSION 1\n"
        "struct MYSQL { int ignored; };\n"
        "inline int mysql_get_socket(MYSQL *) { return 31; }\n",
        "99",
    ),
    "mariadb_package": (
        "#define MARIADB_PACKAGE_VERSION 1\n"
        "struct MYSQL { int ignored; };\n"
        "inline int mysql_get_socket(MYSQL *) { return 31; }\n",
        "99",
    ),
}


with tempfile.TemporaryDirectory(prefix="telemetry-cloexec-") as directory:
    fixture = Path(directory)
    source = fixture / "contract.cc"
    executable = fixture / "contract"
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    for client, (mysql_h, initializer) in CLIENTS.items():
        (fixture / "mysql.h").write_text(mysql_h, encoding="utf-8")
        source.write_text(
            HARNESS.replace("SOCKET_INITIALIZER", initializer), encoding="utf-8"
        )
        subprocess.run(
            compiler
            + [
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{fixture}",
                f"-I{ROOT / 'src'}",
                str(source),
                "-o",
                str(executable),
            ],
            cwd=ROOT,
            check=True,
            timeout=60,
        )
        subprocess.run([str(executable)], cwd=ROOT, check=True, timeout=10)
        print(f"PASS: {client} telemetry descriptor contract")

print("PASS: telemetry descriptor flags preserve existing bits and fail closed")
