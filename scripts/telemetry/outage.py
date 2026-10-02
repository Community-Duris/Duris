#!/usr/bin/env python3
"""Bounded, offline, read-only export of worker outage evidence (wire version 1)."""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import sys

MAGIC = b"DMSTLJ01"
MAX_PRODUCERS = 256
WORDS = 40
MAX_BYTES = 64 + MAX_PRODUCERS * WORDS * 8
UNKNOWN_UTC = -(1 << 63)
KNOWN_KINDS = (1 << 9) - 2
PHASES = {1: "running", 2: "clean_drained", 3: "abandoned", 4: "unknown_tail"}
FAMILIES = {1: "interval", 2: "session_lifecycle", 3: "session_checkpoint",
            4: "coverage_gap", 5: "configuration", 6: "progression",
            7: "encounter", 8: "combat_summary"}
FIELDS = (
    "boot_id", "process_id", "environment_id", "season_id",
    "registered_monotonic_usec", "registered_utc_usec",
    "observed_monotonic_usec", "observed_utc_usec", "phase_code", "record_kind_mask",
    "admitted_detail", "admitted_control", "last_admitted_record_seq", "applied_records",
    "duplicate_records", "stale_checkpoint_records", "quarantined_records", "invalid_records",
    "conflict_records", "rejected_detail_admissions", "rejected_control_admissions",
    "last_committed_record_seq", "inflight_first_record_seq", "inflight_last_record_seq",
    "inflight_records", "unattempted_records", "queue_depth", "inflight_active",
    "retryable_failures", "ambiguous_commits", "sequence_gap_count", "unclosed_tail_count",
    "last_failure_class", "last_error_code", "last_failure_first_record_seq",
    "last_failure_last_record_seq", "last_failure_record_kind_mask",
    "last_success_monotonic_usec", "last_failure_monotonic_usec", "circuit_open_count",
)


class EvidenceError(Exception):
    """Sanitized failure classification; no path, payload or credential text."""


def decode(data: bytes) -> dict:
    if not 384 <= len(data) <= MAX_BYTES or data[:8] != MAGIC:
        raise EvidenceError("corrupt")
    if hashlib.sha256(data[:-32]).digest() != data[-32:]:
        raise EvidenceError("corrupt")
    generation, count, reserved = struct.unpack_from(">3Q", data, 8)
    if not generation or not 1 <= count <= MAX_PRODUCERS or reserved or len(data) != 64 + count * WORDS * 8:
        raise EvidenceError("corrupt")
    observations = []
    producers = set()
    for index in range(count):
        w = struct.unpack_from(f">{WORDS}Q", data, 32 + index * WORDS * 8)
        producer = w[:2]
        accepted = w[10] + w[11]
        acknowledged = w[13] + w[14] + w[15]
        clean = (accepted == acknowledged and accepted < (1 << 64) - 1 and
                 w[26] == 0 and w[24] == 0 and w[16] == w[17] == w[18] == 0)
        if (not all(w[:4]) or producer in producers or w[6] < w[4] or w[8] not in PHASES or
            (index + 1 < count and w[8] == 1) or w[9] & ~KNOWN_KINDS or w[36] & ~KNOWN_KINDS or
            w[21] > w[12] or w[24] > 128 or w[26] > 8192 or w[24] + w[25] != w[26] or
            w[27] not in (0, 1) or bool(w[27]) != bool(w[24]) or w[32] > 8 or w[33] >= (1 << 32) or
            (w[27] and not 0 < w[22] <= w[23] <= w[12]) or (w[8] == 2 and not clean) or
            (w[8] == 3 and (w[24] or not w[25]))):
            raise EvidenceError("corrupt")
        producers.add(producer)
        row = dict(zip(FIELDS, w))
        for name in ("registered_utc_usec", "observed_utc_usec"):
            value = row[name]
            signed = value - (1 << 64) if value >= (1 << 63) else value
            row[name] = None if signed == UNKNOWN_UTC else signed
        row["phase"] = PHASES[w[8]]
        row["record_families"] = [name for kind, name in FAMILIES.items() if w[9] & (1 << kind)]
        row["tail_end_utc_usec"] = row["observed_utc_usec"] if w[8] in (2, 3) else None
        row["tail_end_monotonic_usec"] = w[6] if w[8] in (2, 3) else None
        row["known_abandoned_unattempted_records"] = w[25] if w[8] == 3 else 0
        row["unknown_after_last_sample"] = w[8] in (1, 4)
        observations.append(row)
    return {"ledger_version": 1, "generation": generation, "producer_count": count,
            "max_producers": MAX_PRODUCERS, "observations": observations}


def safe_file(fd: int) -> os.stat_result:
    status = os.fstat(fd)
    if (not stat.S_ISREG(status.st_mode) or status.st_uid != os.geteuid() or
        stat.S_IMODE(status.st_mode) != 0o600 or status.st_nlink != 1):
        raise EvidenceError("unsafe_storage")
    return status


def read_evidence(directory: Path) -> dict:
    if not directory.is_absolute():
        raise EvidenceError("invalid_directory")
    root = os.open(directory, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC)
    owner = ledger = None
    try:
        status = os.fstat(root)
        if status.st_uid != os.geteuid() or stat.S_IMODE(status.st_mode) != 0o700:
            raise EvidenceError("unsafe_storage")
        owner = os.open("outages.owner", os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC, dir_fd=root)
        if safe_file(owner).st_size != 0:
            raise EvidenceError("unsafe_storage")
        try:
            fcntl.flock(owner, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise EvidenceError("owned_elsewhere") from None
        try:
            os.stat("outages.pending", dir_fd=root, follow_symlinks=False)
        except FileNotFoundError:
            pass
        else:
            # This reader never repairs or deletes interrupted evidence.
            raise EvidenceError("pending_publication")
        ledger = os.open("outages.ledger", os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC, dir_fd=root)
        size = safe_file(ledger).st_size
        if not 384 <= size <= MAX_BYTES:
            raise EvidenceError("corrupt")
        with os.fdopen(ledger, "rb", closefd=False) as stream:
            data = stream.read(MAX_BYTES + 1)
        return decode(data)
    finally:
        for fd in (ledger, owner, root):
            if fd is not None:
                os.close(fd)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    try:
        evidence = read_evidence(args.directory)
    except EvidenceError as error:
        print(json.dumps({"status": "refused", "reason": str(error)}))
        return 2
    except OSError as error:
        print(json.dumps({"status": "refused", "reason": "storage_io", "error_code": error.errno}))
        return 2
    print(json.dumps(evidence, sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
