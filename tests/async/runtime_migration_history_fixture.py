#!/usr/bin/env python3
"""Exercise the actual compiled boot history/schema predicates on a test DB."""
from __future__ import annotations

import hashlib
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    if os.environ.get("ENVIRONMENT") != "test" or \
            os.environ.get("TEST_DB_DISPOSABLE") != "1" or \
            not os.environ.get("DB_NAME", "").startswith("duris_268_"):
        raise RuntimeError("runtime fixture requires an explicitly disposable test DB")
    port = os.environ.get("DB_PORT", "3306")
    if not port.isdigit() or not 1 <= int(port) <= 65535:
        raise RuntimeError("invalid disposable database port")
    # Exercise the restore's exact complete-history selector against the same
    # native rows as the compiled boot predicate, including its tamper cases.
    sys.path.insert(0, str(ROOT / "scripts"))
    import migration_runner as migrations
    import qualify_database_restore as restore
    executor = migrations.MysqlExecutor(migrations.load_manifest())
    restored_history = False
    try:
        executor.require_baseline(executor.manifest)
        restore.require_completed_history(executor.applied())
        restored_history = True
    except (migrations.MigrationContractError, RuntimeError):
        pass
    finally:
        executor.release_lock()
    source = (ROOT / "src/sql/sql.c").read_text()
    first = source.index("static bool sql_verify_boot_database(void)\n{")
    last = source.index("\tif (!sql_verify_migration_history())", first)
    # Include the real initial baseline/head/state/table predicate as well as
    # the complete-history and metadata predicates below. This exercises the
    # alternate head's SQL formatting and acceptance before any boot mutation.
    identity = source[first:last] + "\treturn true;\n}\n"
    first = source.index("static bool sql_verify_migration_history(void)\n{")
    last = source.index("/* Same as above, but won't log failed queries", first)
    predicates = source[first:last]
    harness = r'''
#include <mysql/mysql.h>
#include <openssl/sha.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "core/runtime_compatibility_contract.h"
static MYSQL *DB;
static constexpr int LOG_STATUS = 0;
static void logit(int, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
}
static MYSQL_RES *db_query(const char *format, ...) {
    char statement[32768];
    va_list args;
    va_start(args, format);
    int size = vsnprintf(statement, sizeof(statement), format, args);
    va_end(args);
    if (size < 0 || static_cast<size_t>(size) >= sizeof(statement) ||
        mysql_real_query(DB, statement, size)) return nullptr;
    return mysql_store_result(DB);
}
''' + identity + predicates + r'''
int main() {
    DB = mysql_init(nullptr);
    if (!DB || !mysql_real_connect(DB, getenv("DB_HOST"), getenv("DB_USER"),
        getenv("DB_PASSWD"), getenv("DB_NAME"),
        getenv("DB_PORT") ? static_cast<unsigned int>(std::strtoul(getenv("DB_PORT"), nullptr, 10)) : 3306,
        nullptr, 0) ||
        mysql_set_character_set(DB, "utf8mb4")) return 2;
    bool identity = sql_verify_boot_database();
    bool history = sql_verify_migration_history();
    bool metadata = sql_verify_metadata_fingerprint();
    MYSQL_RES *result = db_query("%s", RUNTIME_EXTRA_DESCRIPTION_GENERATION_SQL);
    MYSQL_ROW row = result ? mysql_fetch_row(result) : nullptr;
    bool descriptions = row && row[0] && !strcmp(row[0], "2");
    if (result) mysql_free_result(result);
    mysql_close(DB);
    printf("identity=%u history=%u metadata=%u description_expression=%u\n",
           identity, history, metadata, descriptions);
    return identity && history && metadata && descriptions ? 0 : 1;
}
'''
    digest = hashlib.sha256((harness + (ROOT / "src/core/runtime_compatibility_contract.h")
                            .read_text()).encode()).hexdigest()
    directory = Path(tempfile.gettempdir()) / "duris-runtime-history-fixture"
    directory.mkdir(mode=0o700, exist_ok=True)
    binary = directory / digest
    if not binary.is_file():
        cpp = directory / (digest + ".cpp")
        cpp.write_text(harness)
        flags = subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)
        subprocess.run(["g++", "-std=c++20", "-I", str(ROOT / "src"), str(cpp),
                        "-o", str(binary), *shlex.split(flags), "-lcrypto"], check=True)
    compiled = subprocess.run([str(binary)], check=False).returncode
    print(f"restore_history={int(restored_history)}")
    return compiled if compiled else (0 if restored_history else 1)


if __name__ == "__main__":
    raise SystemExit(main())
