#!/usr/bin/env python3
"""Contracts for the guarded legacy personal-locker access repair."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
repair = (ROOT / "migrations/repair_legacy_personal_locker_access.sh").read_text()
rehearsal = (
    ROOT / "tests/async/run_legacy_personal_locker_access_repair_mysql.sh"
).read_text()

checks = {
    "repair has distinct read-only and mutation modes": "--check | --apply" in repair,
    "target identity is supplied at runtime": (
        "LOCKER_REPAIR_NAME" in repair
        and "LOCKER_REPAIR_OWNER_PID" in repair
        and "LOCKER_REPAIR_EXPECTED_ITEMS" in repair
    ),
    "personal owner preflight uses the runtime identity contract": (
        "ac.pid=l.owner_pid" in repair
        and "ac.racewar=l.racewar" in repair
        and "ac.blocked=0" in repair
        and "ac.deleted_at IS NULL" in repair
        and "l.owner_assoc_id IS NULL" in repair
    ),
    "payload count is an exact precondition": "HAVING COUNT(li.id)=$expected_items" in repair,
    "backup happens before mutation": (
        repair.index('MYSQLDUMP=(mysqldump')
        < repair.index('INSERT IGNORE INTO locker_access(owner,visitor)')
    ),
    "grant is additive and idempotent": "INSERT IGNORE INTO locker_access" in repair,
    "production use requires an exact target acknowledgment": (
        "LOCKER_REPAIR_PRODUCTION_ACK" in repair and "expected_ack=" in repair
    ),
    "remote database identity is verified": (
        "remote locker repair requires TLS and a CA file" in repair
        and "--ssl-mode=VERIFY_IDENTITY" in repair
    ),
    "rehearsal models the audited 21-item locker": (
        "LOCKER_REPAIR_EXPECTED_ITEMS=21" in rehearsal
        and rehearsal.count("(7,10") == 21
    ),
    "rehearsal proves payload preservation and replay safety": (
        '"$after" == "$before"' in rehearsal
        and "access-before-second.sql" in rehearsal
        and '[[ "$grant" == 1' in rehearsal
    ),
}

for name, ok in checks.items():
    print(("PASS" if ok else "FAIL") + ": " + name)

raise SystemExit(0 if all(checks.values()) else 1)
