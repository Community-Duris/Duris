#!/usr/bin/env python3
"""Qualify actual recipe root and save receipts through the native restore decoder."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as native

with tempfile.TemporaryDirectory(prefix="duris-craft-restore-") as directory:
    base = Path(directory)
    qualifier = native.build(base / "qualify")
    fixture = base / "fixture"
    sources = []
    fixture_sources = list(dict.fromkeys(native.SOURCES + ["economic_accounting_intent", "economic_accounting_plan", "item_transfer_accounting", "flatfile_accounting_authority", "flatfile_accounting_store", "flatfile_collector_repository", "collector_command", "collector_codec", "collector_policy", "collector_accounting"]))
    for name in fixture_sources:
        found = list((ROOT / "src").rglob(name + ".c"))
        assert len(found) == 1
        sources.append(str(found[0]))
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-D__NO_MYSQL__", "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-DDURIS_FLATFILE_TRANSACTION_FAULT_TEST",
                    "-Isrc", "-Isrc/no_mysql", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    "tests/async/persistence_restore_fixture.cpp", *sources, "-lcrypto", "-lz", "-pthread",
                    "-o", str(fixture)], cwd=ROOT, check=True)
    seed = base / "seed"
    seed.mkdir(mode=0o700)
    (seed / "ISOLATED_RESTORE").write_text("synthetic disposable candidate\n")
    root = seed / "flatfile"
    subprocess.run([str(fixture), "seed-craft", str(root)], check=True)
    subprocess.run([str(qualifier), "--state-preflight", str(root)], check=True)
    subprocess.run([str(qualifier), str(root)], check=True)
    for fault in ("corrupt-applied", "corrupt-obligation", "missing-obligation", "missing-root", "future-revision", "renamed-applied"):
        candidate = base / fault
        shutil.copytree(seed, candidate)
        authority = candidate / "flatfile"
        applied = next((authority / "players").glob("*.craft"))
        obligation = next((authority / "players").glob("*.craft-obligation"))
        if fault.startswith("corrupt-"):
            path = applied if fault.endswith("applied") else obligation
            payload = bytearray(path.read_bytes())
            payload[-1] ^= 1
            path.write_bytes(payload)
        elif fault == "missing-obligation":
            obligation.unlink()
        elif fault == "missing-root":
            (authority / "domains" / "item_ownership").unlink()
        elif fault == "future-revision":
            subprocess.run([str(fixture), "craft-receipt-ahead", str(authority)], check=True)
        else:
            applied.rename(applied.with_name(applied.name.replace("42-", "043-")))
        for phase in ("--state-preflight", None):
            command = [str(qualifier)] + ([phase] if phase else []) + [str(authority)]
            result = subprocess.run(command, capture_output=True, text=True)
            assert result.returncode != 0, (fault, phase, result.stdout)
print("craft restore: committed root, exact save, checksum, obligation, root proof, revision and filename checks passed")
