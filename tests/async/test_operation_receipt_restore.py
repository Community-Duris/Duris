#!/usr/bin/env python3
"""Validate operation receipts through the production native restore qualifier."""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from _paths import ROOT

sys.path.insert(0, str(ROOT / "scripts"))
from build_restore_qualifier import build

operation_id = bytes(range(1, 17))
payload = b"\x01\x02"
prefix = (b"DOR1" + operation_id + hashlib.sha256(b"synthetic request").digest()
          + struct.pack("<QH", 7, len(payload)) + payload)
encoded = prefix + hashlib.sha256(prefix).digest()
outputs = ROOT / "bin/tests"
outputs.mkdir(parents=True, exist_ok=True)

with tempfile.TemporaryDirectory(prefix="receipt-qualifier-", dir=outputs) as binaries, \
        tempfile.TemporaryDirectory(prefix="receipt-restore-") as scratch:
    qualifier = build(Path(binaries) / "qualifier")
    for case in ("valid", "valid-rejection", "invalid-rejection", "checksum", "truncated", "wrong-id", "oversized",
                 "unsafe-mode", "symlink", "hardlink"):
        candidate = Path(scratch) / case
        receipts = candidate / "journals/critical/receipts"
        receipts.mkdir(parents=True, mode=0o700)
        (candidate / "journals/players").mkdir(mode=0o700)
        (candidate / "ISOLATED_RESTORE").write_bytes(b"")
        for directory in (candidate, candidate / "journals", candidate / "journals/critical"):
            directory.chmod(0o700)
        receipt = receipts / operation_id.hex()
        receipt.write_bytes(encoded)
        receipt.chmod(0o600)
        if case in ("valid-rejection", "invalid-rejection"):
            rejection = (b"DOR2" + operation_id + hashlib.sha256(b"synthetic request").digest()
                         + struct.pack("<QIH", 0, 13 if case == "valid-rejection" else 0, 0))
            receipt.write_bytes(rejection + hashlib.sha256(rejection).digest())
        elif case == "checksum":
            receipt.write_bytes(encoded[:-1] + bytes([encoded[-1] ^ 1]))
        elif case == "truncated":
            receipt.write_bytes(encoded[:-1])
        elif case == "wrong-id":
            receipt.rename(receipts / ("ab" * 16))
        elif case == "oversized":
            receipt.write_bytes(encoded + b"x" * 4096)
        elif case == "unsafe-mode":
            receipt.chmod(0o644)
        elif case in ("symlink", "hardlink"):
            target = candidate / "alias"
            if case == "symlink":
                receipt.rename(target)
                receipt.symlink_to(target)
            else:
                os.link(receipt, target)
        for phase in ("--journals-preflight", "--journals-drained"):
            result = subprocess.run([str(qualifier), phase, str(candidate)],
                                    capture_output=True, text=True, timeout=10)
            assert (result.returncode == 0) == (case in ("valid", "valid-rejection")), (case, phase, result.stdout, result.stderr)
print("operation receipt restore checksum, identity, bounds and filesystem checks passed")
