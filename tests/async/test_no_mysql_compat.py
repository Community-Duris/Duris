#!/usr/bin/env python3
"""The flat build's MySQL compatibility surface must always fail closed."""

from _paths import SRC
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

PROBE = r'''
#define __NO_MYSQL__
#include <mysql.h>
#include <mysql/mysql.h>

#include <cerrno>
#include <cstring>

int main()
{
    MYSQL connection = {};
    char escaped[16] = {};

    if (mysql_init(nullptr) != nullptr)
        return 1;
    if (mysql_real_connect(&connection, "host", "user", "pass", "db", 3306,
                           nullptr, 0) != nullptr)
        return 2;
    if (mysql_real_query(&connection, "SELECT 1", 8) == 0)
        return 3;
    if (mysql_store_result(&connection) != nullptr)
        return 4;
    if (mysql_errno(&connection) != ENOTSUP)
        return 5;
    if (std::strcmp(mysql_sqlstate(&connection), "HY000"))
        return 6;
    if (mysql_stmt_init(&connection) != nullptr)
        return 7;
    if (mysql_thread_init() == 0 || mysql_library_init(0, nullptr, nullptr) == 0)
        return 8;
    if (mysql_real_escape_string(&connection, escaped, "value", 5) != 5 ||
        std::strcmp(escaped, "value"))
        return 9;
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="no-mysql-compat-") as directory:
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
            f"-I{SRC / "no_mysql"}",
            str(probe),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("client-free MySQL compatibility contract passed")
