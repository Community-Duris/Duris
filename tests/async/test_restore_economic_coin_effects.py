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


def build_coin_fixture(work, mode):
    from native_build_artifacts import build_native
    assert mode in ("sql", "flatfile")
    sources = ["tests/async/restore_coin_effects_fixture.cpp",
               "src/economy/economic_accounting_plan.c", "src/economy/economic_source_event.c", "src/economy/economic_accounting_types.c",
               "src/economy/economic_accounting_intent.c", "src/persistence/critical_command.c",
               "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/item/craft_pouch_mutation.c",
               "src/economy/shop_trade_recovery_manifest.c",
               "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c",
               "src/item/lockpick_retirement_continuation.c", "src/economy/native_quest_cost.c",
               "src/economy/native_quest_coin_give.c"]
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-Isrc"]
    target = work / ("fixture-" + mode)
    binary = build_native(target, sources,
                          flags + (["-D__NO_MYSQL__"] if mode == "flatfile" else []),
                          ["-no-pie", "-lcrypto"], name="plan5-restore-coin-" + mode)
    if binary.resolve() != target.resolve():
        shutil.copyfile(binary, target)
    target.chmod(0o700)
    return target


@unittest.skipUnless(sys.platform == "linux" and
                     os.environ.get("DURIS_RUN_RESTORE_COIN_INTEGRATION") == "1",
                     "requires explicit private Linux native/SQL restore invocation")
class RestoreCoinEffectsTests(unittest.TestCase):
    def test_native_coin_effects_both_modes_and_canonical_engines(self):
        sys.path.insert(0, str(ROOT / "scripts"))
        import persistence_restore as restore
        import pymysql

        canonical = os.environ.get("DURIS_PLAN5_CANONICAL_EVIDENCE") == "1"
        work = ROOT / ("bin/tests/plan5-retained-namespace-canonical" if canonical else
                       "bin/tests/plan5-retained-namespace-coins")
        work.mkdir(mode=0o700, parents=True, exist_ok=True)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        results = []
        for mode in ("sql", "flatfile"):
            target = build_coin_fixture(work, mode)
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
        histories = []
        for mode in ("sql", "flatfile"):
            history = subprocess.check_output([str(work / ("fixture-" + mode)), "--restore-history"],
                                              env=environment, timeout=60)
            histories.append(history)
            (work / ("history-" + mode + ".bin")).write_bytes(history)
        self.assertEqual(histories[0], histories[1])
        claim_histories = []
        for mode in ("sql", "flatfile"):
            history = subprocess.check_output([str(work / ("fixture-" + mode)), "--claim-history"],
                                              env=environment, timeout=60)
            claim_histories.append(history)
            (work / ("claims-" + mode + ".bin")).write_bytes(history)
        self.assertEqual(claim_histories[0], claim_histories[1])
        claim_blocks, claim_offset = [], 0
        while claim_offset < len(claim_histories[0]):
            size, = struct.unpack_from("<I", claim_histories[0], claim_offset)
            claim_offset += 4
            claim_blocks.append(claim_histories[0][claim_offset:claim_offset+size])
            claim_offset += size
        self.assertEqual(claim_offset, len(claim_histories[0]))
        self.assertEqual(len(claim_blocks), 10)
        from test_economic_sql_canonical_audit import ClaimProjectionFixture
        import economic_restore_evidence as evidence
        claim_pairs = list(zip(claim_blocks[::2], claim_blocks[1::2]))
        for mode in ("unspent", "partial", "consumed", "whole"):
            evidence.require_integrity(ClaimProjectionFixture(mode, claim_pairs))
        print("NATIVE_PENDING_CLAIM_CONTROLS " + json.dumps({"roots": 5, "modes": 2,
            "allocation_controls": 4, "fixture_sha256": hashlib.sha256(claim_histories[0]).hexdigest(),
            "producer_journey_qualified": False}, sort_keys=True), flush=True)
        maximum_intent = histories[0][-8192:]
        self.assertEqual(struct.unpack_from("<I", histories[0], len(histories[0]) - 8196)[0], 8192)
        self.assertEqual(maximum_intent[:4], b"EAI1")
        (work / "history-max-intent.bin").write_bytes(maximum_intent)
        # Keep the original decoder/coin oracle corpus. Generic SQL restore
        # history must have a valid transfer root, not a witness-less baseline.
        corpus_offset = sum(4 + len(block) for block in blocks[:5])
        sql_fixture = histories[0][:-8196] + results[0][corpus_offset:]
        fixture_path = work / "restore-history.bin"
        fixture_path.write_bytes(sql_fixture)
        history_plan_start = 8 + struct.unpack_from("<I", histories[0], 0)[0]
        print("NATIVE_RESTORE_HISTORY " + json.dumps({"modes": 2,
              "ordinary_reason": struct.unpack_from("<H", histories[0], history_plan_start + 96)[0],
              "history_sha256": hashlib.sha256(histories[0]).hexdigest(),
              "maximum_intent_sha256": hashlib.sha256(maximum_intent).hexdigest(),
              "sql_fixture_sha256": hashlib.sha256(sql_fixture).hexdigest()}, sort_keys=True), flush=True)
        if canonical:
            import economic_restore_evidence as evidence
            decoded = []
            for mode in ("sql", "flatfile"):
                output = subprocess.check_output([str(work / ("fixture-" + mode)), "--decode-corpus"],
                                                 env=environment, timeout=60)
                (work / ("native-decode-" + mode + ".jsonl")).write_bytes(output)
                decoded.append(output)
            self.assertEqual(decoded[0], decoded[1])
            cases = [json.loads(line) for line in decoded[0].splitlines()]
            self.assertEqual(len(cases), 3026)
            for case in cases:
                with self.subTest(native_case=case["name"], kind=case["kind"]):
                    try:
                        (evidence.decode_intent if case["kind"] == "intent" else evidence.decode_plan)(
                            bytes.fromhex(case["bytes"]))
                    except ValueError:
                        accepted = False
                    else:
                        accepted = True
                    self.assertEqual(accepted, case["accepted"])
            print("NATIVE_CANONICAL_DECODERS " + json.dumps({
                "cases": len(cases), "accepted": sum(row["accepted"] for row in cases),
                "modes": 2, "independent_agreement": True,
                "output_sha256": hashlib.sha256(decoded[0]).hexdigest()}, sort_keys=True), flush=True)
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
                                               DURIS_PLAN5_COIN_FIXTURE=str(fixture_path),
                                               DURIS_PLAN5_CLAIM_FIXTURE=str(work / "claims-sql.bin"),
                                               DURIS_PLAN5_CANONICAL_EVIDENCE=str(int(canonical)),
                                               DURIS_PLAN5_CANONICAL_RED=os.environ.get("DURIS_PLAN5_CANONICAL_RED", "0"))
                        # Preserve output as it arrives, including a timeout or
                        # failed assertion; a previous run must never masquerade
                        # as the current engine's diagnostics.
                        with (work / (engine + ".log")).open("w") as log:
                            result = subprocess.run([sys.executable, "-u", str(ROOT / "tests/async/run_restore_accounting_evidence_mysql.py")],
                                                    env=run_environment, stdout=log, stderr=subprocess.STDOUT, timeout=1800)
                        output = (work / (engine + ".log")).read_text()
                        print(output, end="", flush=True)
                        self.assertEqual(result.returncode, 0, output)
                        self.assertIn("restore_economic_intent_mismatch" if
                                      os.environ.get("DURIS_PLAN5_CANONICAL_RED") == "1" else
                                      "COIN_RESTORE_QUALIFIED", output)


if __name__ == "__main__":
    unittest.main()
