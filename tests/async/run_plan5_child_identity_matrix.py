#!/usr/bin/env python3
"""Supply exact historical inputs to the existing Plan 5 native qualification."""
import hashlib
import os
from pathlib import Path
import runpy
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
BASE = 'd90381b549960dce652a588d14422eeac0e90dca'
PREIMAGES = {'reconcile_economy_accounting': '5ab01420abdb9e7e6cd766cec7ae58373bf360cbae5bf08705e1e76b849b4d8d', 'economic_sql_audit_snapshot': 'dbf6999c965c2cbfac436bd91b638618924b1a223c49fce16e3dbb4dec7a2e17'}


def main():
    if os.environ.get("DURIS_PLAN5_CHILD_IDENTITY_NATIVE") != "1":
        raise RuntimeError("requires the existing explicit native/private SQL opt-in")
    artifacts = Path(os.environ["DURIS_PLAN5_CHILD_IDENTITY_ARTIFACTS"]).resolve()
    if artifacts.exists() or not artifacts.is_relative_to((ROOT / "bin").resolve()):
        raise RuntimeError("requires a fresh owned artifact directory below bin")
    # The unchanged owner creates artifacts itself and reads its preimages from
    # the parent. Verify both immutable Git blobs before writing either one.
    originals = {name: subprocess.check_output(["git", "show", BASE + ":scripts/" + name + ".py"], cwd=ROOT)
                 for name in PREIMAGES}
    if any(hashlib.sha256(originals[name]).hexdigest() != expected
           for name, expected in PREIMAGES.items()):
        raise RuntimeError("historical qualification preimage mismatch")
    # Keep database sockets below the native Unix path bound. The matrix uses
    # a fresh short nonce namespace directly below bin rather than its long
    # report directory; preserve its location in the original row transcript.
    if artifacts.parent.exists():
        raise RuntimeError("historical qualification namespace already exists")
    artifacts.parent.mkdir(parents=True, mode=0o700, exist_ok=False)
    for name, original in originals.items():
        with (artifacts.parent / (name + ".py")).open("xb") as output:
            output.write(original)
    print("Plan 5 child native artifacts: " + str(artifacts), flush=True)
    owner = ROOT / "tests/async/test_plan5_child_identity.py"
    sys.argv[0] = str(owner)
    runpy.run_path(str(owner), run_name="__main__")


if __name__ == "__main__":
    main()
