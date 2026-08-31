#!/usr/bin/env python3
"""Execute the production account-projection repair against temporary tables."""

from _paths import SRC
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

from contract_text import index

ROOT = Path(__file__).resolve().parents[2]
source_text = (SRC / "sql_player.c").read_text(
    encoding="utf-8", errors="replace"
)
mysql_source_text = source_text[source_text.index("\n#else\n\n// globals") :]


def body(text, signature):
    start = index(text, signature)
    while ";" in text[start + len(signature) : text.index("{", start)]:
        start = index(text, signature, start + len(signature))
    opening = text.index("{", start)
    depth = 0
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated definition: {signature}")


repair = body(
    mysql_source_text,
    "int sql_repair_account_character_projection(const char *account_name)",
)

harness = f'''\
#include <mysql/mysql.h>

#include <cassert>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

MYSQL *DB = nullptr;

char *sql_escape_string(const char *value)
{{
    if (!DB || !value)
        return nullptr;
    const size_t length = std::strlen(value);
    char *escaped = static_cast<char *>(std::malloc(length * 2 + 1));
    if (!escaped)
        return nullptr;
    mysql_real_escape_string(DB, escaped, value, length);
    return escaped;
}}

bool sql_run_query(const char *query)
{{
    if (mysql_query(DB, query) == 0)
        return true;
    std::cerr << "repair query failed: " << mysql_error(DB) << '\\n';
    return false;
}}

{repair}

static const char *required_env(const char *name)
{{
    const char *value = std::getenv(name);
    assert(value && *value);
    return value;
}}

static void execute(const char *query)
{{
    if (mysql_query(DB, query) != 0)
    {{
        std::cerr << "fixture query failed: " << mysql_error(DB) << '\\n';
        std::abort();
    }}
}}

static long scalar(const char *query)
{{
    execute(query);
    MYSQL_RES *result = mysql_store_result(DB);
    assert(result);
    MYSQL_ROW row = mysql_fetch_row(result);
    assert(row && row[0]);
    const long value = std::strtol(row[0], nullptr, 10);
    mysql_free_result(result);
    return value;
}}

int main()
{{
    DB = mysql_init(nullptr);
    assert(DB);
    const char *port_text = std::getenv("DB_PORT");
    const unsigned int port = port_text ? std::strtoul(port_text, nullptr, 10) : 3306;
    assert(mysql_real_connect(DB, required_env("DB_HOST"), required_env("DB_USER"),
                              required_env("DB_PASSWD"), required_env("DB_NAME"),
                              port, nullptr, 0));

    execute("CREATE TEMPORARY TABLE player_data ("
            "pid INT NOT NULL PRIMARY KEY, name VARCHAR(80) NOT NULL, "
            "account_name VARCHAR(255) NOT NULL, active TINYINT NOT NULL)");
    execute("CREATE TEMPORARY TABLE account_characters ("
            "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY, "
            "account_name VARCHAR(255) NOT NULL, pid INT NOT NULL, "
            "char_name VARCHAR(80) NOT NULL, created_at DATETIME NOT NULL, "
            "deleted_at DATETIME NULL, UNIQUE KEY account_character_pid (pid), "
            "UNIQUE KEY account_character_name (char_name))");
    execute("INSERT INTO player_data VALUES "
            "(101,'RepairHero','RepairAcct',1),"
            "(102,'MovedHero','RepairAcct',1),"
            "(103,'DeletedHero','RepairAcct',1),"
            "(104,'InactiveHero','RepairAcct',0)");
    execute("INSERT INTO account_characters "
            "(account_name,pid,char_name,created_at,deleted_at) VALUES "
            "('WrongAcct',102,'MovedHero',NOW(),NULL),"
            "('RepairAcct',103,'DeletedHero',NOW(),NOW())");

    assert(sql_repair_account_character_projection("repairacct") > 0);
    assert(mysql_commit(DB) == 0);

    // These reload queries have no process-local account/character state. They
    // prove the missing and misassigned projections were written to the database.
    assert(scalar("SELECT COUNT(*) FROM account_characters "
                  "WHERE account_name='RepairAcct' AND deleted_at IS NULL") == 2);
    assert(scalar("SELECT COUNT(*) FROM account_characters "
                  "WHERE pid=101 AND account_name='RepairAcct' AND deleted_at IS NULL") == 1);
    assert(scalar("SELECT COUNT(*) FROM account_characters "
                  "WHERE pid=102 AND account_name='RepairAcct' AND deleted_at IS NULL") == 1);
    assert(scalar("SELECT COUNT(*) FROM account_characters "
                  "WHERE pid=103 AND deleted_at IS NULL") == 0);
    assert(scalar("SELECT COUNT(*) FROM account_characters "
                  "WHERE pid=103 AND deleted_at IS NOT NULL") == 1);
    assert(scalar("SELECT COUNT(*) FROM account_characters WHERE pid=104") == 0);
    assert(sql_repair_account_character_projection("RepairAcct") == 0);
    assert(sql_repair_account_character_projection("EmptyAcct") == 0);

    mysql_close(DB);
    DB = nullptr;
    std::cout << "account character projection MariaDB behavior passed\\n";
    return 0;
}}
'''

with tempfile.TemporaryDirectory(prefix="duris-account-projection-mysql-") as directory:
    temp = Path(directory)
    source = temp / "projection_repair.cpp"
    binary = temp / "projection_repair"
    source.write_text(harness, encoding="utf-8")
    cflags = shlex.split(
        subprocess.check_output(["mysql_config", "--cflags"], text=True)
    )
    libs = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            *cflags,
            str(source),
            *libs,
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert compile_result.returncode == 0, compile_result.stderr
    subprocess.run([str(binary)], check=True, env=os.environ.copy())
