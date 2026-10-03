#!/usr/bin/env python3
"""Classify anomalous item custody history without changing native state."""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import stat
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from classify_item_topology import (
    ROOT, TopologyError, active_connections, run_mysql,
)
from import_legacy_dump import LegacyImportError, read_env_file


ARTIFACT_HEADER = "# duris-item-custody-history-v1"
MAX_RESULT_ROWS = 100000
CURRENT_FIELDS = (
    "item_uid", "root_item_uid", "parent_item_uid", "owner_type", "owner_id",
    "owner_context_id", "item_revision", "vnum", "state",
    "opening_item_revision", "ledger_event_count", "ledger_first_revision",
    "ledger_last_revision",
)
DEATH_FIELDS = (
    "pid", "save_revision", "item_uid", "root_item_uid", "parent_item_uid",
    "item_revision", "vnum", "state", "owner_type", "owner_id",
    "owner_context_id", "owner_revision", "disposition_operation_id",
    "disposition_payload_sha256",
)
ARTIFACT_COLUMNS = "\t".join(("evidence_id", "category", *CURRENT_FIELDS))
DEATH_ARTIFACT_COLUMNS = "\t".join(("death_evidence_id", *DEATH_FIELDS))


class HistoryError(Exception):
    """A bounded, aggregate-safe custody history error."""


@dataclass(frozen=True)
class Row:
    """One anomalous current custody row."""

    values: tuple[int | str | None, ...]

    @property
    def item_uid(self) -> int:
        return self.values[0]  # type: ignore[return-value]

    def current(self) -> tuple[int | str | None, ...]:
        return self.values

    def artifact_values(self) -> str:
        return "\t".join("NULL" if value is None else str(value)
                         for value in self.values)


@dataclass(frozen=True)
class Case:
    """One anomalous UID, classified against retained death rows."""

    category: str
    row: Row

    def evidence_id(self, snapshot_id: str) -> str:
        current = self.row.current()
        current_digest = hashlib.sha256("\t".join(
            "NULL" if value is None else str(value) for value in current
        ).encode()).hexdigest()
        return hashlib.sha256((ARTIFACT_HEADER + "\n" + snapshot_id + "\n" +
                               str(self.row.item_uid) + "\n" +
                               current_digest).encode()).hexdigest()


@dataclass(frozen=True)
class Snapshot:
    """Normalized evidence avoids repeating a death row for every child UID."""

    current_rows: tuple[Row, ...]
    death_rows: tuple[tuple[int | str | None, ...], ...]


def classification_sql() -> str:
    """Select only anomalous current rows; witness lookup is separately bounded."""
    return f"""
WITH ledger AS (
  SELECT item_uid,COUNT(*) event_count,MIN(item_revision) first_revision,
         MAX(item_revision) last_revision
  FROM item_ownership_ledger GROUP BY item_uid
)
SELECT c.item_uid,c.root_item_uid,c.parent_item_uid,c.owner_type,c.owner_id,
       c.owner_context_id,c.item_revision,c.vnum,c.state,
       b.opening_item_revision,l.event_count,l.first_revision,l.last_revision
FROM item_current_owner c
LEFT JOIN item_ownership_baseline b ON b.item_uid=c.item_uid
LEFT JOIN ledger l ON l.item_uid=c.item_uid
WHERE (b.item_uid IS NULL AND l.item_uid IS NULL)
   OR ((b.item_uid IS NOT NULL OR l.item_uid IS NOT NULL)
       AND c.item_revision<>COALESCE(b.opening_item_revision,0)+
           COALESCE(l.event_count,0))
ORDER BY c.item_uid
LIMIT {MAX_RESULT_ROWS + 1};
"""


def witness_sql(pids: list[int]) -> str:
    """Fetch retained death rows once per affected owner, without a root OR join."""
    if not pids or len(pids) > 100 or any(pid <= 0 for pid in pids):
        raise HistoryError("death witness batch is invalid")
    selected = ",".join(str(pid) for pid in pids)
    return f"""
SELECT d.pid,d.save_revision,d.item_uid,d.root_item_uid,d.parent_item_uid,
       d.item_revision,d.vnum,d.state,d.owner_type,d.owner_id,
       d.owner_context_id,d.owner_revision,HEX(p.operation_id),
       UPPER(SHA2(p.payload,256))
FROM player_death_custody d
LEFT JOIN player_death_disposition p
  ON p.pid=d.pid AND p.save_revision=d.save_revision
WHERE d.pid IN ({selected})
ORDER BY d.pid,d.save_revision,d.item_uid
LIMIT {MAX_RESULT_ROWS + 1};
"""


def _integer(value: str, optional: bool) -> int | None:
    if optional and value == "NULL":
        return None
    try:
        result = int(value)
    except ValueError as error:
        raise HistoryError("custody history query returned malformed data") from error
    return result


def load_rows(query: Callable[[str], str]) -> Snapshot:
    """Load bounded, normalized current and death evidence from a frozen clone."""
    output = query(classification_sql())
    lines = output.splitlines() if output else []
    if len(lines) > MAX_RESULT_ROWS:
        raise HistoryError("custody history exceeds the protected row limit")
    currents: list[Row] = []
    for line in lines:
        fields = line.split("\t")
        if len(fields) != 13:
            raise HistoryError("custody history query returned malformed data")
        values = tuple(_integer(field, index in (2, 9, 10, 11, 12))
                       for index, field in enumerate(fields))
        if values[0] is None or values[0] <= 0 or values[1] is None or \
                values[1] <= 0 or any(
                    value is None for value in values[3:9]) or any(
                        values[index] < 0 for index in (3, 4, 5, 6, 8)
                ):
            raise HistoryError("custody history query returned malformed current custody")
        currents.append(Row(values))
    pids = sorted({row.values[4] for row in currents
                   if row.values[3] == 1 and row.values[4] > 0})
    witnesses: list[tuple[int | str | None, ...]] = []
    for offset in range(0, len(pids), 100):
        output = query(witness_sql(pids[offset:offset + 100]))
        lines = output.splitlines() if output else []
        if len(witnesses) + len(lines) > MAX_RESULT_ROWS:
            raise HistoryError("death custody evidence exceeds the protected row limit")
        for line in lines:
            fields = line.split("\t")
            if len(fields) != len(DEATH_FIELDS):
                raise HistoryError("death witness query returned malformed data")
            values = tuple(_integer(field, False) for field in fields[:12]) + tuple(
                None if field == "NULL" else field for field in fields[12:])
            if values[0] <= 0 or any(values[index] < 0 for index in (
                    1, 2, 3, 4, 5, 7, 8, 9, 10, 11)):
                raise HistoryError("death witness query returned malformed custody")
            if (values[12] is None) != (values[13] is None) or \
                    (values[12] is not None and (
                        re.fullmatch(r"[0-9A-F]{32}", values[12]) is None or
                        re.fullmatch(r"[0-9A-F]{64}", values[13]) is None)):
                raise HistoryError("death witness returned malformed evidence hash")
            witnesses.append(values)
    return Snapshot(tuple(currents), tuple(witnesses))


def classify(snapshot: Snapshot) -> list[Case]:
    """Classify each UID while leaving every source witness in the snapshot."""
    by_pid: dict[int, list[tuple[int | str | None, ...]]] = defaultdict(list)
    for death in snapshot.death_rows:
        by_pid[death[0]].append(death)
    cases: list[Case] = []
    seen_uids: set[int] = set()
    for row in snapshot.current_rows:
        item_uid = row.item_uid
        if item_uid in seen_uids:
            raise HistoryError("custody query returned a duplicate current UID")
        seen_uids.add(item_uid)
        current = row.current()
        witnesses = by_pid.get(current[4], ()) if current[3] == 1 else ()
        origin = current[9] is not None or current[10] is not None
        expected = (current[9] or 0) + (current[10] or 0)
        if origin and current[6] == expected:
            raise HistoryError("custody query returned a reconciled row")
        if not origin:
            category = "missing_origin"
        elif any(death[2] == item_uid and death[5] + 1 == current[6] and
                 death[8:11] == current[3:6] and death[12] is not None
                 for death in witnesses):
            category = "revision_mismatch_exact_death_witness"
        elif any(death[12] is not None and
                 (death[2] == current[1] or death[3] == current[1])
                 for death in witnesses):
            category = "revision_mismatch_root_death_witness"
        elif any(death[2] == item_uid and death[12] is not None
                 for death in witnesses):
            category = "revision_mismatch_uid_death_witness"
        else:
            category = "revision_mismatch_unwitnessed"
        cases.append(Case(category, row))
    return cases


def summary(snapshot: Snapshot, cases: list[Case]) -> str:
    """Report only aggregate categories; never print item or player identities."""
    counts = Counter(case.category for case in cases)
    categories = ",".join(f"{name}:{counts[name]}" for name in sorted(counts)) or "none"
    return (f"cases={len(cases)} death_evidence_rows={len(snapshot.death_rows)} "
            f"categories={categories}")


def render_artifact(database: str, snapshot_id: str,
                    snapshot: Snapshot, cases: list[Case]) -> bytes:
    """Serialize normalized protected evidence deterministically."""
    if re.fullmatch(r"[0-9a-f]{64}", snapshot_id) is None:
        raise HistoryError("artifact requires a snapshot SHA-256")
    lines = [ARTIFACT_HEADER, f"# database={database}",
             f"# snapshot_sha256={snapshot_id}", ARTIFACT_COLUMNS]
    for case in cases:
        lines.append("\t".join((case.evidence_id(snapshot_id), case.category,
                                case.row.artifact_values())))
    lines.extend(("# death_custody_evidence", DEATH_ARTIFACT_COLUMNS))
    for death in snapshot.death_rows:
        values = "\t".join("NULL" if value is None else str(value)
                           for value in death)
        death_id = hashlib.sha256((ARTIFACT_HEADER + "\n" + snapshot_id +
                                   "\n" + values).encode()).hexdigest()
        lines.append(death_id + "\t" + values)
    return ("\n".join(lines) + "\n").encode()


def write_artifact(path: Path, database: str, snapshot_id: str,
                   snapshot: Snapshot, cases: list[Case]) -> str:
    """Create a new owner-only artifact with stable frozen-snapshot IDs."""
    if not path.is_absolute():
        raise HistoryError("artifact requires an absolute path")
    try:
        parent = path.parent.resolve(strict=True)
        metadata = parent.stat()
    except OSError as error:
        raise HistoryError(f"cannot inspect artifact directory: {error}") from error
    if parent != path.parent or metadata.st_uid != os.getuid() or \
            not stat.S_ISDIR(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise HistoryError("artifact directory must be owner-only without symlinks")
    payload = render_artifact(database, snapshot_id, snapshot, cases)
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(payload)
    except OSError as error:
        raise HistoryError(f"cannot create protected artifact: {error}") from error
    return hashlib.sha256(payload).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env")
    parser.add_argument("--artifact", type=Path)
    parser.add_argument("--snapshot-sha256")
    arguments = parser.parse_args()
    try:
        if bool(arguments.artifact) != bool(arguments.snapshot_sha256):
            raise HistoryError("artifact and snapshot SHA-256 must be provided together")
        config = read_env_file(arguments.env_file.resolve())
        if arguments.artifact and active_connections(config):
            raise HistoryError("protected evidence requires a quiesced database")
        snapshot = load_rows(lambda statement: run_mysql(config, statement))
        cases = classify(snapshot)
        rendered = summary(snapshot, cases)
        if arguments.artifact:
            digest = write_artifact(arguments.artifact, config["DB_NAME"],
                                    arguments.snapshot_sha256, snapshot, cases)
            rendered += f" artifact_sha256={digest}"
        print("item custody history: " + rendered)
        return 1 if cases else 0
    except (HistoryError, TopologyError, LegacyImportError, KeyError, OSError) as error:
        print(f"item custody history blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
