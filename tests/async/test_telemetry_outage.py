#!/usr/bin/env python3
"""Local bounded outage ledger: process replacement, kill and storage failures."""

from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-outage-", dir=artifacts) as directory:
        binary = Path(directory) / "telemetry-outage"
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-I", str(ROOT / "src"),
            str(ROOT / "src/telemetry/telemetry_outage.c"),
            str(ROOT / "tests/async/telemetry_outage_harness.cc"),
            "-Wl,--wrap=write", "-Wl,--wrap=fsync", "-Wl,--wrap=renameat", "-lcrypto",
            "-o", str(binary),
        ], cwd=ROOT, check=True, timeout=60)
        completed = subprocess.run([str(binary)], cwd=ROOT, text=True,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60)
        print(completed.stdout, end="")
        completed.check_returncode()
        assert "telemetry durable outage evidence qualification passed" in completed.stdout
        with tempfile.TemporaryDirectory(prefix="telemetry-evidence-") as evidence_path:
            evidence = Path(evidence_path)
            subprocess.run([str(binary), "--make-fixture", str(evidence)], check=True, timeout=10)
            before = (evidence / "outages.ledger").read_bytes()
            command = [sys.executable, str(ROOT / "scripts/telemetry/outage.py"), str(evidence)]
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            packet = json.loads(exported.stdout)
            assert packet["ledger_version"] == 6
            assert packet["producer_count"] == 2 and packet["generation"] == 4
            previous, current = packet["observations"]
            assert "ownership" in previous["record_families"]
            assert "battle" in previous["record_families"]
            assert "battle_contribution" in previous["record_families"]
            assert "battle_build" in previous["record_families"]
            assert "control" in previous["record_families"]
            assert "battle_result" in previous["record_families"]
            assert previous["phase"] == "unknown_tail" and previous["tail_end_utc_usec"] is None
            assert previous["tail_end_monotonic_usec"] is None and previous["observed_monotonic_usec"] == 400
            assert previous["rejected_detail_admissions"] == 3 and previous["unattempted_records"] == 4
            assert previous["known_abandoned_unattempted_records"] == 0
            assert current["phase"] == "clean_drained" and current["tail_end_monotonic_usec"] == 500
            assert (evidence / "outages.ledger").read_bytes() == before
            # Every earlier version retains its own family limit, including
            # both masks; a valid checksum cannot widen a sealed descriptor.
            for version, kind in ((1, 10), (1, 11), (2, 11), (1, 12), (2, 12), (3, 12),
                                  (1, 13), (2, 13), (3, 13), (4, 13),
                                  (1, 14), (2, 14), (3, 14), (4, 14), (5, 14)):
                for field in (9, 36):
                    incompatible = bytearray(before)
                    incompatible[7] = ord(str(version))
                    offset = 32 + field * 8
                    incompatible[offset:offset + 8] = (1 << kind).to_bytes(8, "big")
                    incompatible[-32:] = hashlib.sha256(incompatible[:-32]).digest()
                    (evidence / "outages.ledger").write_bytes(incompatible)
                    refused = subprocess.run(command, text=True, capture_output=True, timeout=10)
                    assert refused.returncode == 2
                    assert (evidence / "outages.ledger").read_bytes() == incompatible
            v5 = bytearray(before)
            v5[7] = ord("5")
            for index in range(packet["producer_count"]):
                for field in (9, 36):
                    offset = 32 + (index * 40 + field) * 8
                    mask = int.from_bytes(v5[offset:offset + 8], "big") & ~(1 << 14)
                    v5[offset:offset + 8] = mask.to_bytes(8, "big")
            v5[-32:] = hashlib.sha256(v5[:-32]).digest()
            (evidence / "outages.ledger").write_bytes(v5)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            older = json.loads(exported.stdout)
            assert older["ledger_version"] == 5 and older["producer_count"] == 2
            assert "control" in older["observations"][0]["record_families"]
            assert "battle_result" not in older["observations"][0]["record_families"]
            assert (evidence / "outages.ledger").read_bytes() == v5
            v4 = bytearray(v5)
            v4[7] = ord("4")
            for index in range(packet["producer_count"]):
                for field in (9, 36):
                    offset = 32 + (index * 40 + field) * 8
                    mask = int.from_bytes(v4[offset:offset + 8], "big") & ~(1 << 13)
                    v4[offset:offset + 8] = mask.to_bytes(8, "big")
            v4[-32:] = hashlib.sha256(v4[:-32]).digest()
            (evidence / "outages.ledger").write_bytes(v4)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            older = json.loads(exported.stdout)
            assert older["ledger_version"] == 4 and older["producer_count"] == 2
            assert "battle_build" in older["observations"][0]["record_families"]
            assert "control" not in older["observations"][0]["record_families"]
            assert (evidence / "outages.ledger").read_bytes() == v4
            v3 = bytearray(v4)
            v3[7] = ord("3")
            for index in range(packet["producer_count"]):
                for field in (9, 36):
                    offset = 32 + (index * 40 + field) * 8
                    mask = int.from_bytes(v3[offset:offset + 8], "big") & ~(1 << 12)
                    v3[offset:offset + 8] = mask.to_bytes(8, "big")
            v3[-32:] = hashlib.sha256(v3[:-32]).digest()
            (evidence / "outages.ledger").write_bytes(v3)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            older = json.loads(exported.stdout)
            assert older["ledger_version"] == 3 and older["producer_count"] == 2
            assert "battle_contribution" in older["observations"][0]["record_families"]
            assert "battle_build" not in older["observations"][0]["record_families"]
            assert (evidence / "outages.ledger").read_bytes() == v3
            v2 = bytearray(v4)
            v2[7] = ord("2")
            for index in range(packet["producer_count"]):
                for field in (9, 36):
                    offset = 32 + (index * 40 + field) * 8
                    mask = int.from_bytes(v2[offset:offset + 8], "big") & ~((1 << 11) | (1 << 12))
                    v2[offset:offset + 8] = mask.to_bytes(8, "big")
            v2[-32:] = hashlib.sha256(v2[:-32]).digest()
            (evidence / "outages.ledger").write_bytes(v2)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            older = json.loads(exported.stdout)
            assert older["ledger_version"] == 2 and older["producer_count"] == 2
            assert "battle" in older["observations"][0]["record_families"]
            assert "battle_contribution" not in older["observations"][0]["record_families"]
            assert (evidence / "outages.ledger").read_bytes() == v2
            legacy = bytearray(v4)
            legacy[7] = ord("1")
            for index in range(packet["producer_count"]):
                for field in (9, 36):
                    offset = 32 + (index * 40 + field) * 8
                    mask = int.from_bytes(legacy[offset:offset + 8], "big") & ~((1 << 10) | (1 << 11) | (1 << 12))
                    legacy[offset:offset + 8] = mask.to_bytes(8, "big")
            legacy[-32:] = hashlib.sha256(legacy[:-32]).digest()
            (evidence / "outages.ledger").write_bytes(legacy)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            old = json.loads(exported.stdout)
            assert old["ledger_version"] == 1 and old["producer_count"] == 2
            assert "ownership" in old["observations"][0]["record_families"]
            assert "battle" not in old["observations"][0]["record_families"]
            assert (evidence / "outages.ledger").read_bytes() == legacy
            subprocess.run([str(binary), "--upgrade-fixture", str(evidence)], check=True, timeout=10)
            exported = subprocess.run(command, check=True, text=True, capture_output=True, timeout=10)
            upgraded = json.loads(exported.stdout)
            assert upgraded["ledger_version"] == 6 and upgraded["producer_count"] == 3
            assert upgraded["observations"][:2] == old["observations"]
            pending = evidence / "outages.pending"
            pending.write_bytes(b"interrupted")
            pending.chmod(0o600)
            refused = subprocess.run(command, text=True, capture_output=True, timeout=10)
            assert refused.returncode == 2 and json.loads(refused.stdout)["reason"] == "pending_publication"
            assert pending.read_bytes() == b"interrupted"
            pending.unlink()
            (evidence / "outages.ledger").chmod(0o644)
            refused = subprocess.run(command, text=True, capture_output=True, timeout=10)
            assert refused.returncode == 2 and json.loads(refused.stdout)["reason"] == "unsafe_storage"
        with tempfile.TemporaryDirectory(prefix="telemetry-held-") as held:
            holder = subprocess.Popen([str(binary), "--hold-fixture", held], stdout=subprocess.PIPE,
                                      text=True)
            try:
                assert holder.stdout.readline().strip() == "held"
                refused = subprocess.run([sys.executable, str(ROOT / "scripts/telemetry/outage.py"), held],
                                         text=True, capture_output=True, timeout=10)
                assert refused.returncode == 2 and json.loads(refused.stdout)["reason"] == "owned_elsewhere"
            finally:
                holder.kill()
                holder.wait(timeout=10)
        print("outage offline bounded read-only evidence export passed")


if __name__ == "__main__":
    main()
