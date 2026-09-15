#!/usr/bin/env python3
"""Compile the production telemetry factory against deterministic syscall stubs.

Unit coverage only: no database, sockets, live service, or timing dependency.
Requires a POSIX C++20 compiler for the descriptor flag constants.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/sql/sql.c").read_text()
start = source.index("MYSQL *sql_open_telemetry_connection(void)")
end = source.index("\n/* Escapes a string. */", start)
factory = source[start:end]
harness = r'''
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <cerrno>
#include <fcntl.h>
struct MYSQL {};
static MYSQL connection;
static int closes, gets, sets, executes;
static int original_flags, written_flags;
static bool fail_connect, fail_get, fail_set, fail_execute;
static constexpr int socket_fd = 31;
static MYSQL *sql_open_verified_connection(unsigned long, const char *, const char *, unsigned int)
{ return fail_connect ? nullptr : &connection; }
static int mysql_get_socket(MYSQL *conn) { assert(conn == &connection); return socket_fd; }
static void mysql_close(MYSQL *conn) { assert(conn == &connection); ++closes; }
static bool sql_connection_execute(MYSQL *conn, const char *)
{ assert(conn == &connection); ++executes; return !fail_execute; }
static int fake_fcntl(int fd, int command, ...)
{
    assert(fd == socket_fd);
    if (command == F_GETFD) {
        ++gets;
        if (fail_get) { errno = EBADF; return -1; }
        return original_flags;
    }
    assert(command == F_SETFD);
    ++sets;
    va_list args;
    va_start(args, command);
    written_flags = va_arg(args, int);
    va_end(args);
    if (fail_set) { errno = EIO; return -1; }
    return 0;
}
#define fcntl fake_fcntl
'''
harness += factory
harness += r'''
static void reset()
{
    closes = gets = sets = executes = 0;
    written_flags = -1;
    original_flags = 0;
    fail_connect = fail_get = fail_set = fail_execute = false;
}
int main()
{
    // An unknown descriptor flag models any platform's additional flags.
    for (int flags : {0, FD_CLOEXEC, 0x400, 0x400 | FD_CLOEXEC}) {
        reset(); original_flags = flags;
        assert(sql_open_telemetry_connection() == &connection);
        assert(gets == 1 && sets == 1 && closes == 0 && executes == 1);
        assert(written_flags == (flags | FD_CLOEXEC));
    }
    reset(); fail_connect = true;
    assert(sql_open_telemetry_connection() == nullptr);
    assert(gets == 0 && sets == 0 && closes == 0 && executes == 0);
    reset(); fail_get = true;
    assert(sql_open_telemetry_connection() == nullptr);
    assert(gets == 1 && sets == 0 && closes == 1 && executes == 0);
    reset(); fail_set = true;
    assert(sql_open_telemetry_connection() == nullptr);
    assert(gets == 1 && sets == 1 && closes == 1 && executes == 0);
    reset(); fail_execute = true;
    assert(sql_open_telemetry_connection() == nullptr && closes == 1);
}
'''
harness = "#include <initializer_list>\n" + harness
with tempfile.TemporaryDirectory(prefix="telemetry-cloexec-") as directory:
    cpp = Path(directory) / "factory.cc"
    exe = Path(directory) / "factory"
    cpp.write_text(harness)
    subprocess.run([*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20",
                    "-Wall", "-Wextra", "-Werror",
                    str(cpp), "-o", str(exe)], check=True, timeout=60)
    subprocess.run([str(exe)], check=True, timeout=10)
print("PASS: telemetry descriptor flags preserved; get/set failures close the connection")
