#!/usr/bin/env python3
"""Native coin-effect parity on private canonical MySQL/MariaDB restore cuts."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


@unittest.skipUnless(sys.platform == "linux" and
                     os.environ.get("DURIS_RUN_RESTORE_COIN_INTEGRATION") == "1",
                     "requires explicit private Linux native/SQL restore invocation")
class RestoreCoinEffectsTests(unittest.TestCase):
    def test_native_coin_effects_both_modes_and_canonical_engines(self):
        sys.path.insert(0, str(ROOT / "scripts"))
        import persistence_restore as restore
        from native_build_artifacts import build_native
        import pymysql

        work = ROOT / "bin/tests/plan5-sql-coin-effects"
        work.mkdir(mode=0o700, parents=True, exist_ok=True)
        sources = ["tests/async/restore_coin_effects_fixture.cpp",
                   "src/economy/economic_accounting_plan.c", "src/economy/economic_accounting_types.c",
                   "src/economy/economic_accounting_intent.c", "src/persistence/critical_command.c",
                   "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c",
                   "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c"]
        flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                 "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-Isrc"]
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        results = []
        for mode in ("sql", "flatfile"):
            target = work / ("fixture-" + mode)
            binary = build_native(target, sources,
                                  flags + (["-D__NO_MYSQL__"] if mode == "flatfile" else []),
                                  ["-no-pie", "-lcrypto"], name="plan5-restore-coin-" + mode)
            if binary.resolve() != target.resolve():
                shutil.copyfile(binary, target)
            target.chmod(0o700)
            output = subprocess.check_output([str(target)], env=environment)
            results.append(output)
            (work / ("native-" + mode + ".bin")).write_bytes(output)
            print("NATIVE_RESTORE_COIN " + mode + " " + json.dumps({
                "binary": hashlib.sha256(target.read_bytes()).hexdigest(),
                "output": hashlib.sha256(output).hexdigest()}, sort_keys=True), flush=True)
        self.assertEqual(results[0], results[1])
        blocks, offset = [], 0
        while offset < len(results[0]):
            size, = struct.unpack_from("<I", results[0], offset)
            offset += 4
            blocks.append(results[0][offset:offset + size])
            offset += size
        self.assertEqual(offset, len(results[0]))
        self.assertEqual(len(blocks), 37)
        cases = [json.loads(block) for block in blocks[5:]]
        self.assertEqual(len(cases), 32)
        print("NATIVE_RESTORE_COIN_CASES " + json.dumps({
            "cases": len(cases), "accepted": sum(row["accepted"] for row in cases),
            "account_kinds": 11, "modes": 2, "wire_plans": 3, "wire_intents": 2}, sort_keys=True), flush=True)
        # The helper creates one empty private schema. Verify it is empty before
        # replacing it so the existing runner can prove fresh-schema creation.
        with tempfile.TemporaryDirectory(prefix="plan5-restore-coins-", dir="/") as directory:
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
                                cursor.execute("DROP DATABASE duris_restore")
                                cursor.execute("ALTER USER 'root'@'localhost' IDENTIFIED BY 'plan5-private-coin-root'")
                        finally:
                            owner.close()
                        run_environment = dict(private, DB_USER="root", DB_PASSWD="plan5-private-coin-root", MYSQL_PWD="plan5-private-coin-root",
                                               DB_HOST="127.0.0.1", ENVIRONMENT="test", TEST_DB_DISPOSABLE="1",
                                               DURIS_PLAN5_COIN_FIXTURE=str(work / "native-sql.bin"))
                        # Preserve output as it arrives, including a timeout or
                        # failed assertion; a previous run must never masquerade
                        # as the current engine's diagnostics.
                        with (work / (engine + ".log")).open("w") as log:
                            result = subprocess.run([sys.executable, "-u", str(ROOT / "tests/async/run_restore_accounting_evidence_mysql.py")],
                                                    env=run_environment, stdout=log, stderr=subprocess.STDOUT, timeout=900)
                        output = (work / (engine + ".log")).read_text()
                        print(output, end="", flush=True)
                        self.assertEqual(result.returncode, 0, output)
                        self.assertIn("COIN_RESTORE_QUALIFIED", output)


if __name__ == "__main__":
    unittest.main()
