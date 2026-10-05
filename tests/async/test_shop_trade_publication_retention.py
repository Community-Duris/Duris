#!/usr/bin/env python3
"""Native shop projection ownership faults; no SQL services or gameplay proof."""
from pathlib import Path
import os
import hashlib
import json
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CASES = ("success", "preflight", "balances", "custody", "revision", "exception",
         "reentry", "missing_player", "malformed", "ambiguous", "changed_duplicate",
         "notification_reentry", "wrong_action", "wrong_uid", "invalid_disposition",
         "notification_exception", "notification_reentry_exception", "many_transient_failures",
         "duplicate_burst", "equivalent_readback", "fresh_body")
SOURCES = ["tests/async/shop_trade_publication_retention_harness.cpp",
           "src/economy/shop_trade_transaction.c", "src/economy/shop_trade_command.c",
           "src/item/item_transfer_command.c", "src/item/craft_pouch_mutation.c",
           "src/combat/chaos_pouch_ledger.c", "src/player/player_snapshot_codec.c",
           "src/economy/currency_command.c", "src/persistence/critical_command.c"]
PHYSICAL_CASES = ("success", "destroy_success", "missing_object", "missing_keeper",
                  "payload_conflict", "placement_refused", "placement_exception",
                  "container_refused", "destruction_exception", "store_success",
                  "cleanup_success", "tree_success", "restored_success", "byte_changed_retry",
                  "nesting_exception", "detachment_exception")
PHYSICAL_SOURCES = ["tests/async/shop_trade_physical_retention_harness.cpp",
                    *SOURCES[1:], "src/core/safe_format.c", "src/player/player_snapshot_capture.c"]

failed = []
with tempfile.TemporaryDirectory(prefix="duris-shop-retention-") as temporary:
    for policy, defines, sources, cases in (
            (f"{backend}_{scope}", defines, sources, cases)
            for backend, defines in (("sql_header", []), ("flatfile", ["-D__NO_MYSQL__"]))
            for scope, sources, cases in (("owner", SOURCES, CASES),
                                          ("physical", PHYSICAL_SOURCES, PHYSICAL_CASES))):
        binary = Path(temporary) / policy
        compiled = subprocess.run(
            [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
             "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
             "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc",
             "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
             *defines, *sources, "-lcrypto", "-o", str(binary)],
            cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            timeout=300)
        if compiled.returncode:
            raise SystemExit(compiled.stdout)
        print(json.dumps({"policy": policy, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                          "source_sha256": {s: hashlib.sha256((ROOT / s).read_bytes()).hexdigest()
                                            for s in sources}, "compile_seconds_limit": 300,
                          "runtime_case_seconds_limit": 30}), flush=True)
        for case in cases:
            result = subprocess.run([str(binary), case], cwd=ROOT, text=True,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=30)
            print(f"policy={policy} case={case} exit={result.returncode}\n{result.stdout}", flush=True)
            if result.returncode:
                failed.append((policy, case))
if failed:
    raise SystemExit(f"Shop retention failures: {failed}")
print("PASS shop retained original-result projection policies; actual physical/backend/restart gates remain separate")
