#!/usr/bin/env python3
"""SQL lifecycle APIs must compile and refuse activation in client-free builds."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    with tempfile.TemporaryDirectory(prefix="duris-sql-lifecycle-no-mysql-") as temporary:
        directory = Path(temporary)
        harness = directory / "contract.cpp"
        executable = directory / "contract"
        harness.write_text(r'''
#include "persistence/economic_sql_accounting_lifecycle_transaction.h"
#include <cassert>
#include <cerrno>
int main()
{
    economic_sql_lifecycle_guard authority;
    economic_sql_currency_writer_guard writer;
    economic_sql_lifecycle_request request;
    economic_sql_lifecycle_receipt receipt;
    assert(!authority.is_maintenance_authority());
    assert(economic_sql_lifecycle_guard::acquire_runtime(nullptr, &authority) == ENOTSUP);
    assert(economic_sql_lifecycle_guard::acquire_maintenance(nullptr, &authority) == ENOTSUP);
    assert(economic_sql_currency_writer_guard::acquire(nullptr, &writer) == ENOTSUP);
    assert(economic_sql_accounting_lifecycle_transaction::install(
        nullptr, authority, request, &receipt) == ENOTSUP);
    assert(!authority.is_maintenance_authority());
    assert(receipt.wallets.empty() && receipt.banks.empty());
    assert(receipt.baseline_revision == 0);
}
''')
        command = shlex.split(os.environ.get("CXX", "g++")) + [
            "-std=c++20", "-D__NO_MYSQL__", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(ROOT / "src/no_mysql"), "-I", str(ROOT / "src"),
            str(harness),
            str(ROOT / "src/persistence/economic_sql_lifecycle_guard.c"),
            str(ROOT / "src/persistence/economic_sql_accounting_lifecycle_transaction.c"),
            "-o", str(executable),
        ]
        subprocess.run(command, check=True, timeout=90)
        subprocess.run([str(executable)], check=True, timeout=10)
    print("SQL lifecycle client-free refusal contract: PASS")


if __name__ == "__main__":
    main()
