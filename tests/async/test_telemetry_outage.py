#!/usr/bin/env python3
"""Local bounded outage ledger: process replacement, kill and storage failures."""

from pathlib import Path
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
            assert packet["producer_count"] == 2 and packet["generation"] == 4
            previous, current = packet["observations"]
            assert "ownership" in previous["record_families"]
            assert previous["phase"] == "unknown_tail" and previous["tail_end_utc_usec"] is None
            assert previous["tail_end_monotonic_usec"] is None and previous["observed_monotonic_usec"] == 400
            assert previous["rejected_detail_admissions"] == 3 and previous["unattempted_records"] == 4
            assert previous["known_abandoned_unattempted_records"] == 0
            assert current["phase"] == "clean_drained" and current["tail_end_monotonic_usec"] == 500
            assert (evidence / "outages.ledger").read_bytes() == before
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
