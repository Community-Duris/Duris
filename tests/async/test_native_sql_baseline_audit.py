#!/usr/bin/env python3
"""Native baseline persistence evidence on private canonical SQL audit cuts."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(sys.platform == "linux" and
                     os.environ.get("DURIS_RUN_NATIVE_BASELINE_AUDIT") == "1",
                     "requires explicit private Linux native baseline audit invocation")
class NativeBaselineAuditTests(unittest.TestCase):
    def test_native_baseline_claims_both_canonical_engines(self):
        sys.path.insert(0, str(ROOT / "scripts"))
        import persistence_restore as restore
        from native_build_artifacts import build_native
        import pymysql

        work = ROOT / "bin/tests/plan5-baseline-zero-effects"
        work.mkdir(mode=0o700, parents=True, exist_ok=True)
        sources = ["tests/async/plan5_sql_baseline_audit_fixture.cpp",
                   "src/persistence/economic_sql_baseline_transaction.c",
                   "src/economy/economic_baseline_command.c", "src/economy/economic_baseline_adapter.c",
                   "src/economy/economic_baseline_codec.c", "src/economy/economic_accounting_intent.c",
                   "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_types.c",
                   "src/persistence/critical_command.c", "src/item/item_transfer_command.c",
                   "src/item/craft_pouch_mutation.c", "src/combat/chaos_pouch_ledger.c",
                   "src/player/player_snapshot_codec.c"]
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                 "-DDURIS_ECONOMIC_SQL_BASELINE_TEST", "-fsanitize=address,undefined",
                 "-fno-omit-frame-pointer", "-fno-pie", "-Isrc"]
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        for mode in ("sql", "client-free"):
            target = work / ("fixture-" + mode)
            if mode == "sql":
                compile_flags = flags + shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
                link_flags = shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
            else:
                compile_flags = flags + ["-D__NO_MYSQL__", "-Isrc/no_mysql"]
                link_flags = []
            binary = build_native(target, sources, compile_flags,
                                  ["-no-pie", *link_flags, "-lcrypto"], name="plan5-native-baseline-" + mode)
            if binary.resolve() != target.resolve():
                shutil.copyfile(binary, target)
            target.chmod(0o700)
            print("NATIVE_BASELINE_BINARY " + mode + " " + hashlib.sha256(target.read_bytes()).hexdigest(), flush=True)
        client_free = subprocess.check_output([str(work / "fixture-client-free")], env=environment)
        (work / "client-free.json").write_bytes(client_free)
        print("NATIVE_BASELINE_CLIENT_FREE " + client_free.decode().strip(), flush=True)
        with tempfile.TemporaryDirectory(prefix="plan5-restore-baseline-", dir="/") as directory:
            for engine in ("mariadb", "mysql"):
                with self.subTest(engine=engine):
                    candidate = Path(directory) / engine
                    candidate.mkdir(mode=0o700)
                    with restore.private_database(candidate, engine) as private:
                        owner = pymysql.connect(unix_socket=private["DB_SOCKET"], user="root", autocommit=True)
                        try:
                            with owner.cursor() as cursor:
                                cursor.execute("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='duris_restore'")
                                self.assertEqual(cursor.fetchone()[0], 0)
                                cursor.execute("ALTER USER 'root'@'localhost' IDENTIFIED BY 'plan5-private-baseline-root'")
                        finally:
                            owner.close()
                        run_environment = dict(private, DB_USER="root", DB_PASSWD="plan5-private-baseline-root",
                                               MYSQL_PWD="plan5-private-baseline-root", DB_HOST="127.0.0.1",
                                               ENVIRONMENT="test", TEST_DB_DISPOSABLE="1",
                                               DURIS_PLAN5_BASELINE_FIXTURE=str(work / "fixture-sql"),
                                               DURIS_PLAN5_BASELINE_RED=os.environ.get("DURIS_PLAN5_BASELINE_RED", "0"),
                                               ASAN_OPTIONS=environment["ASAN_OPTIONS"], UBSAN_OPTIONS=environment["UBSAN_OPTIONS"])
                        with (work / (engine + ".log")).open("w") as log:
                            result = subprocess.run([sys.executable, "-u", str(ROOT / "tests/async/run_native_sql_baseline_audit.py")],
                                                    env=run_environment, stdout=log, stderr=subprocess.STDOUT, timeout=1200)
                        output = (work / (engine + ".log")).read_text()
                        print(output, end="", flush=True)
                        self.assertEqual(result.returncode, 0, output)
                        self.assertIn("NATIVE_BASELINE_AUDIT_QUALIFIED", output)


if __name__ == "__main__":
    unittest.main()
