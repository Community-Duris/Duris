#!/usr/bin/env python3
"""Run accounted item transactions on an explicitly disposable SQL schema."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class ItemProvenanceMysqlTest(unittest.TestCase):
    def test_accounted_item_transaction_and_replay(self):
        if os.environ.get("TEST_DB_DISPOSABLE") != "1":
            self.skipTest("explicit disposable SQL fixture required")
        if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
                os.environ.get("DB_HOST") != "127.0.0.1" or
                os.environ.get("DB_SOCKET") or
                not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+",
                                 os.environ.get("DB_NAME", ""))):
            self.fail("explicit disposable loopback schema is required")
        script = (ROOT / "tests/async/run_item_transfer_schema_mysql.sh").read_text()
        sources = list(dict.fromkeys(re.findall(r"src/[a-z0-9_/]+\.c", script)))
        self.assertIn("src/item/item_transfer_repository.c", sources)
        self.assertIn("src/persistence/economic_sql_item_transfer_transaction.c", sources)
        cflags = shlex.split(subprocess.check_output(
            ["mysql_config", "--cflags"], text=True))
        libs = shlex.split(subprocess.check_output(
            ["mysql_config", "--libs"], text=True))
        with tempfile.TemporaryDirectory(prefix="duris-item-provenance-mysql-") as directory:
            binary = Path(directory) / "item_provenance_mysql"
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall",
                "-Wextra", "-Wpedantic", "-Werror", "-pthread",
                "-ffunction-sections", "-fdata-sections", "-Isrc", *cflags,
                "tests/async/item_provenance_mysql_harness.cpp",
                "tests/async/item_extra_descr_codec_sql_escape_stub.cpp", *sources,
                "-Wl,--gc-sections", "-Wl,--wrap=mysql_real_query,--wrap=mysql_errno",
                *libs, "-lcrypto", "-lz", "-o", str(binary),
            ], cwd=ROOT, check=True, timeout=300)
            environment = dict(os.environ,
                               ITEM_TRANSFER_TEST_DB_NAME=os.environ["DB_NAME"])
            # Match the maintained native SQL driver: the bounded payloads in
            # this harness require more than the usual 8 MiB process stack.
            subprocess.run(["bash", "-c", 'ulimit -s 65536 && exec "$1"',
                            "item-provenance", str(binary)], cwd=ROOT, env=environment,
                           check=True, timeout=120)


if __name__ == "__main__":
    unittest.main()
