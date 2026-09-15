#!/usr/bin/env python3
"""Compile the socket-descriptor boundary against supported client headers."""

from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HEADER = "sql/mysql_client_compat.h"


def compile_fixture(
    name: str,
    mysql_h: str,
    expected: int,
) -> None:
    with tempfile.TemporaryDirectory(prefix=f"mysql-client-{name}-") as directory:
        fixture = Path(directory)
        (fixture / "mysql.h").write_text(mysql_h, encoding="utf-8")
        source = fixture / "main.cc"
        source.write_text(
            f'''#include "{HEADER}"
int main()
{{
    MYSQL connection{{}};
    mysql_fixture_set_socket(&connection, {expected});
    return sql_mysql_socket_descriptor(&connection) == {expected} ? 0 : 1;
}}
''',
            encoding="utf-8",
        )
        executable = fixture / "probe"
        subprocess.run(
            [
                "g++",
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
        )
        subprocess.run([str(executable)], cwd=ROOT, check=True)


def compile_system_headers() -> None:
    flags = shlex.split(
        subprocess.check_output(["mysql_config", "--cflags"], text=True)
    )
    with tempfile.TemporaryDirectory(prefix="mysql-client-system-") as directory:
        fixture = Path(directory)
        source = fixture / "system.cc"
        source.write_text(
            f'''#include "{HEADER}"
int probe(MYSQL *connection)
{{
    return sql_mysql_socket_descriptor(connection);
}}
''',
            encoding="utf-8",
        )
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{ROOT / 'src'}",
                *flags,
                "-c",
                str(source),
                "-o",
                str(fixture / "system.o"),
            ],
            cwd=ROOT,
            check=True,
        )


compile_fixture(
    "mariadb",
    """#pragma once
#define MARIADB_BASE_VERSION \"3.3\"
struct MYSQL { int fixture_socket; };
using my_socket = int;
static inline void mysql_fixture_set_socket(MYSQL *connection, int value)
{ connection->fixture_socket = value; }
static inline my_socket mysql_get_socket(MYSQL *connection)
{ return connection->fixture_socket; }
""",
    17,
)
compile_fixture(
    "oracle",
    """#pragma once
using my_socket = int;
struct NET { my_socket fd; };
struct MYSQL { NET net; };
static inline void mysql_fixture_set_socket(MYSQL *connection, int value)
{ connection->net.fd = value; }
""",
    23,
)
compile_system_headers()
print("verified MySQL socket boundary: MariaDB, Oracle, and installed headers PASS")
