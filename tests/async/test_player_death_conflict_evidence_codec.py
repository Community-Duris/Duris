#!/usr/bin/env python3
"""Execute the real codec, including a frozen pre-extension wire contract."""
import hashlib
import os
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
# Filled from the actual pre-extension encoder, not regenerated from new code.
LEGACY_SHA256 = {
    "legacy-normal": "f7dd31fe4115d52db04b11dc03655f6fd2dd716cf852d9af8fbb59eb555d63fc",
    "legacy-death": "6b140185a8101ceffcdc268c05c5afd015dbfe599501f35adfd011b92908da62",
}

with tempfile.TemporaryDirectory(prefix="duris-death-evidence-codec-") as temporary:
    binary = pathlib.Path(temporary) / "death_evidence_codec_test"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Isrc"]
    if os.environ.get("DEATH_EVIDENCE_SANITIZERS") == "1":
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"]
    subprocess.run(
        ["g++", *flags, "tests/async/player_death_conflict_evidence_codec_harness.cpp",
         "src/player/player_snapshot_codec.c", "-o", str(binary)],
        cwd=ROOT, check=True,
    )
    for mode in ("legacy-normal", "legacy-death"):
        original = subprocess.run([str(binary), mode], check=True, stdout=subprocess.PIPE)
        digest = hashlib.sha256(original.stdout).hexdigest()
        print(f"{mode} sha256={digest}", flush=True)
        if mode in LEGACY_SHA256:
            assert digest == LEGACY_SHA256[mode], f"{mode} bytes changed"
    subprocess.run([str(binary)], check=True, cwd=ROOT)
