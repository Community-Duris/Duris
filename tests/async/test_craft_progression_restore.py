#!/usr/bin/env python3
"""Qualify actual recipe root and save receipts through the native restore decoder."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as native
from _restore_fixture import build as build_fixture

with tempfile.TemporaryDirectory(prefix="duris-craft-restore-") as directory:
    base = Path(directory)
    qualifier = native.build(base / "qualify")
    fixture = build_fixture(base / "fixture")
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
