#!/usr/bin/env python3
"""Native restore qualification validates retained mana pools before service boot."""
import hashlib
from pathlib import Path
import os
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as native


def frame(uid=81, reserve=2):
    payload = b"DURMANA\x01" + struct.pack("<8Q", uid, 7, 1, 2, 100000, 1, reserve, 100)
    return payload + hashlib.sha256(payload).digest()


with tempfile.TemporaryDirectory(prefix="duris-mana-restore-") as directory:
    base = Path(directory)
    qualifier = native.build(base / "qualify")
    root = base / "state"
    for relative in ("domains", "identities/accounts", "players"):
        (root / relative).mkdir(mode=0o700, parents=True, exist_ok=True)
    for path in (base, root, root / "identities"):
        os.chmod(path, 0o700)
    (base / "ISOLATED_RESTORE").write_text('{"synthetic":true}')
    pool = root / "domains/artifact-mana-81"
    pool.write_bytes(frame())
    os.chmod(pool, 0o600)
    def qualifies():
        return subprocess.run([str(qualifier), str(root)], capture_output=True, timeout=15).returncode == 0
    assert qualifies(), "valid retained mana pool did not qualify"
    for invalid in (frame()[:-1], b"bad-checksum" + frame()[12:], frame(uid=82), frame(reserve=100001)):
        pool.write_bytes(invalid)
        assert not qualifies(), "invalid native mana authority qualified"
    pool.write_bytes(frame())
    pool.rename(pool.with_name("artifact-mana-081"))
    assert not qualifies(), "noncanonical mana identity qualified"
print("native mana restore accepts retained depletion and rejects corrupt or relabeled state")
