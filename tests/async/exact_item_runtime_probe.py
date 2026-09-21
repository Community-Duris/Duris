"""Compile verbatim runtime metadata/session functions against a real SQL client.

This probes those production functions, not the complete server boot sequence.
The caller owns the temporary directory and enforces disposable test targets.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def build_probe(directory):
    text = (ROOT / 'src/sql/sql.c').read_text()
    def section(first, following):
        start = text.index(first + '\n{')
        return text[start:text.index(following, start)]
    source = r'''#include "core/runtime_compatibility_contract.h"
#include <mysql/mysql.h>
#include <openssl/sha.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <string>
MYSQL *DB = nullptr;
'''
    source += section('static bool sql_connection_execute(MYSQL *conn, const char *statement)',
                      'static bool sql_connection_execute_affected')
    source += section('static bool sql_mode_has(const char *mode, const char *required)',
                      'static MYSQL *sql_open_verified_connection')
    source += section('static bool sql_verify_metadata_fingerprint(void)',
                      '/* Same as above, but won')
    source += r'''
int main(int argc, char **argv)
{
    assert(argc == 2);
    const char *host = getenv("DB_HOST"), *database = getenv("DB_NAME");
    const char *socket = getenv("DB_SOCKET");
    const char *baseline_guard = getenv("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA");
    constexpr char baseline_prefix[] = "economic_schema_test_";
    const bool baseline_target = baseline_guard && !strcmp(baseline_guard, "1") && database &&
        !strncmp(database, baseline_prefix, sizeof(baseline_prefix) - 1);
    assert(host && !strcmp(host, "127.0.0.1") && baseline_target);
    assert(!socket || !*socket);
    const char *user = getenv("DB_USER"), *password = getenv("DB_PASSWD");
    const char *port = getenv("DB_PORT");
    assert(user && password && port);
    DB = mysql_init(nullptr);
    assert(DB);
    unsigned timeout = 10;
    assert(!mysql_options(DB, MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
    assert(mysql_real_connect(DB, host, user, password, database,
                              static_cast<unsigned>(strtoul(port, nullptr, 10)), nullptr, 0));
    assert(sql_apply_session_contract(DB));
    int result = 0;
    if (!strcmp(argv[1], "metadata"))
        result = sql_verify_metadata_fingerprint() ? 0 : 1;
    else
    {
        assert(!strcmp(argv[1], "session"));
        const char *server = mysql_get_server_info(DB);
        assert(server);
        if (strstr(server, "MariaDB"))
        {
            assert(sql_connection_execute(DB, "SET SESSION check_constraint_checks=0"));
            assert(!sql_verify_session_contract(DB));
            assert(sql_apply_session_contract(DB));
            assert(sql_verify_session_contract(DB));
            assert(sql_connection_execute(DB, "SET SESSION check_constraint_checks=0"));
            assert(sql_connection_execute(DB, "CREATE TEMPORARY TABLE exact_item_probe(value INT CHECK(value>0))"));
            assert(sql_connection_execute(DB, "INSERT INTO exact_item_probe VALUES(-1)"));
            assert(sql_apply_session_contract(DB));
            assert(!sql_connection_execute(DB, "INSERT INTO exact_item_probe VALUES(-2)"));
            assert(sql_connection_execute(DB, "DROP TEMPORARY TABLE exact_item_probe"));
        }
        else
            assert(sql_verify_session_contract(DB));
    }
    mysql_close(DB);
    return result;
}
'''
    path = Path(directory) / 'probe.cpp'
    binary = Path(directory) / 'probe'
    path.write_text(source)
    subprocess.run(['g++', '-std=c++20', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-Isrc', '-I/usr/include/mysql', str(path), '-lmysqlclient',
                    '-lcrypto', '-o', str(binary)], cwd=ROOT, check=True, timeout=120)
    return binary
