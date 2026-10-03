#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import hashlib
import shutil
import struct
import sys
import subprocess
import tempfile
from _flatfile_player_fixture import build_player_inspector


ROOT = pathlib.Path(__file__).resolve().parents[2]

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="flat-player-test-", dir=ROOT / "bin/tests") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_player_test"
    binary = build_player_inspector(binary)

    if len(sys.argv) == 3 and sys.argv[1] == "--build-inspector":
        destination = pathlib.Path(sys.argv[2]).resolve()
        if not destination.is_relative_to((ROOT / "bin").resolve()):
            raise SystemExit("inspector must be placed below bin/")
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, destination)
        raise SystemExit(0)

    # The authority lock requires native POSIX ownership/mode metadata. Keep
    # runtime state on the local filesystem when the checkout is on DrvFS.
    with tempfile.TemporaryDirectory(prefix="flat-player-state-") as state_temporary:
        run_result = subprocess.run(
            [str(binary), state_temporary],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        if run_result.returncode:
            raise SystemExit(run_result.stdout)

        # The native checksum-refusal case deliberately damages this synthetic
        # snapshot last. Undo that exact fault before exercising its observer.
        state = pathlib.Path(state_temporary)
        snapshot = state / "players/42.snapshot"
        data = bytearray(snapshot.read_bytes())
        data[-1] ^= 0x5a
        snapshot.write_bytes(data)
        inspection = [str(binary), str(state), "inspect", "42"]
        before = {p.relative_to(state): p.read_bytes() for p in state.rglob("*") if p.is_file()}
        subprocess.run(inspection, cwd=ROOT, check=True, capture_output=True)
        assert before == {p.relative_to(state): p.read_bytes() for p in state.rglob("*") if p.is_file()}, \
            "ordinary inspection changed authority files"

        # This is a valid native v2 after-image journal, not an invalid marker.
        # The old observer replayed it, installed the image, and reported success.
        name, image = b"inspection_should_not_write", b"observer side effect"
        payload = struct.pack("<HBBH", 1, 1, 1, len(name)) + name + struct.pack("<I", len(image)) + image
        pending = b"DURAUTH\0" + struct.pack("<II", 2, len(payload)) + hashlib.sha256(payload).digest() + payload
        journal = state / "domains/.critical-authority-transaction"
        journal.write_bytes(pending)
        journal.chmod(0o600)
        refused = subprocess.run(inspection, cwd=ROOT, capture_output=True, text=True)
        assert refused.returncode != 0 and "inspect refuses pending recovery" in refused.stderr, \
            "observer accepted authority that requires recovery"
        assert journal.read_bytes() == pending and not (state / "domains" / name.decode()).exists(), \
            "inspection replayed a pending after-image"
        print("PASS: native authority inspection preserves files and refuses a valid pending after-image")

    with tempfile.TemporaryDirectory(prefix="flat-recovery-state-") as recovery_temporary:
        recovery_result = subprocess.run([str(binary), recovery_temporary, "quarantine-recovery"],
                                         cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if recovery_result.returncode:
            raise SystemExit(recovery_result.stdout)
        print(recovery_result.stdout.strip())

    domain_source = (SRC / "flatfile_player_domain_repository.c").read_text()
    player_source = (SRC / "flatfile_player_repository.c").read_text()
    materialize_source = (SRC / "player_load_materialize.c").read_text()
    for token in (
        "constexpr uint32_t domain_format_version = 4",
        "base_stat_revision",
        "record.domains.base_stats",
        "format_version >= 3",
    ):
        if token not in domain_source:
            raise SystemExit(f"flat player stat authority is missing {token}")
    if "record.domains.base_stat_revision = 1" not in player_source:
        raise SystemExit("first player baseline does not establish stat authority")
    for token in (
        "result.domains.base_stat_revision",
        "ch->base_stats.Str = result.domains.base_stats[0]",
        "ch->base_stats.Luk = result.domains.base_stats[9]",
    ):
        if token not in materialize_source:
            raise SystemExit(f"player load does not publish authoritative stats: {token}")
    print(run_result.stdout.strip())
