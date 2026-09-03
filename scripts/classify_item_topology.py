#!/usr/bin/env python3
"""Read-only classifier for saved-payload versus item-custody topology."""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import stat
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from import_legacy_dump import LegacyImportError, process_environment, read_env_file


ROOT = Path(__file__).resolve().parents[1]
LOOPBACK_HOSTS = {"127.0.0.1", "localhost", "::1"}
ARTIFACT_HEADER = "# duris-item-topology-classification-v1"
ARTIFACT_COLUMNS = (
    "category\tsource_table\tsource_row_id\titem_uid\tpayload_parent_uid\t"
    "expected_root_uid\tcurrent_parent_uid\tcurrent_root_uid\tpayload_owner_type\t"
    "payload_owner_id\tpayload_owner_context_id\tcurrent_owner_type\t"
    "current_owner_id\tcurrent_owner_context_id\tvnum\tcurrent_vnum\tstate\t"
    "item_revision\texpected_item_revision"
)
EXPECTED_CATEGORIES = {"expected_quarantine", "expected_inactive_state"}
REPAIRABLE_CATEGORIES = {"repairable_projection_lag"}


class TopologyError(Exception):
    """A fail-closed aggregate-safe classifier error."""


@dataclass(frozen=True)
class Row:
    source_table: str
    source_row_id: int
    item_uid: int
    payload_parent_uid: int | None
    payload_owner_type: int
    payload_owner_id: int
    payload_owner_context_id: int
    vnum: int
    broken_parent: bool = False
    quarantined: bool = False
    current_root_uid: int | None = None
    current_parent_uid: int | None = None
    current_owner_type: int | None = None
    current_owner_id: int | None = None
    current_owner_context_id: int | None = None
    item_revision: int | None = None
    current_vnum: int | None = None
    state: int | None = None
    expected_item_revision: int | None = None
    payload_parent_state: int | None = None
    payload_parent_owner_type: int | None = None
    payload_parent_owner_id: int | None = None
    payload_parent_owner_context_id: int | None = None


@dataclass(frozen=True)
class Finding:
    category: str
    row: Row
    expected_root_uid: int | None = None

    @property
    def expected(self) -> bool:
        return self.category in EXPECTED_CATEGORIES

    @property
    def repairable(self) -> bool:
        return self.category in REPAIRABLE_CATEGORIES

    def artifact_row(self) -> str:
        row = self.row
        values = (
            self.category, row.source_table, row.source_row_id, row.item_uid,
            row.payload_parent_uid, self.expected_root_uid, row.current_parent_uid,
            row.current_root_uid, row.payload_owner_type, row.payload_owner_id,
            row.payload_owner_context_id, row.current_owner_type, row.current_owner_id,
            row.current_owner_context_id, row.vnum, row.current_vnum, row.state,
            row.item_revision, row.expected_item_revision,
        )
        return "\t".join("NULL" if value is None else str(value) for value in values)


def connection_arguments(config: dict[str, str]) -> list[str]:
    socket_path = config.get("DB_SOCKET", "")
    if socket_path:
        return ["--protocol=socket", f"--socket={socket_path}"]
    target = ["--host", config["DB_HOST"], "--port", config.get("DB_PORT", "3306")]
    if config["DB_HOST"] in LOOPBACK_HOSTS:
        return ["--skip-ssl", *target]
    ca_path = Path(config.get("DB_SSL_CA", ""))
    if config.get("DB_TLS", "").upper() != "TRUE" or not ca_path.is_absolute() or \
            ca_path.is_symlink() or not ca_path.is_file():
        raise TopologyError("remote topology checks require TLS and a CA file")
    help_text = subprocess.run(
        ["mysql", "--help"], capture_output=True, text=True, check=False).stdout
    if "--ssl-mode" in help_text:
        return ["--ssl-mode=VERIFY_IDENTITY", f"--ssl-ca={ca_path}", *target]
    if "--ssl-verify-server-cert" in help_text:
        return [f"--ssl-ca={ca_path}", "--ssl-verify-server-cert", *target]
    raise TopologyError("database client cannot verify the remote server identity")


def run_mysql(config: dict[str, str], statement: str) -> str:
    command = [
        "mysql", *connection_arguments(config), "--user", config["DB_USER"],
        "--batch", "--skip-column-names", "--raw", config["DB_NAME"],
        "--execute", statement,
    ]
    result = subprocess.run(
        command, capture_output=True, text=True, env=process_environment(config),
        check=False)
    if result.returncode:
        lines = [line.strip() for line in result.stderr.splitlines() if line.strip()]
        raise TopologyError(
            "database command failed: " + (" | ".join(lines[:8]) or "no diagnostic"))
    return result.stdout.strip()


def active_connections(config: dict[str, str]) -> int:
    output = run_mysql(
        config, "SELECT COUNT(*) FROM information_schema.processlist "
        "WHERE db=DATABASE() AND id<>CONNECTION_ID();")
    try:
        return int(output)
    except ValueError as error:
        raise TopologyError("active connection check returned malformed data") from error


def classification_sql() -> str:
    return """
WITH candidate AS (
  SELECT 'player_items' source_table,i.id source_row_id,i.obj_uid item_uid,
         p.obj_uid payload_parent_uid,1 payload_owner_type,
         CAST(i.pid AS UNSIGNED) payload_owner_id,0 payload_owner_context_id,i.vnum,
         IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0) broken_parent
  FROM player_items i LEFT JOIN player_items p ON p.id=i.container_id WHERE i.obj_uid>0
  UNION ALL
  SELECT 'corpse_items',i.id,i.obj_uid,p.obj_uid,4,
         ((CAST(pd.pid AS UNSIGNED)<<32)|(CAST(c.save_id AS UNSIGNED)&4294967295)),0,
         i.vnum,IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
  FROM corpse_items i JOIN corpses c ON c.id=i.corpse_id
  JOIN player_data pd ON LOWER(pd.name)=LOWER(c.player_name)
  LEFT JOIN corpse_items p ON p.id=i.container_id WHERE i.obj_uid>0
  UNION ALL
  SELECT 'locker_items',i.id,i.obj_uid,p.obj_uid,5,CAST(i.locker_id AS UNSIGNED),
         CAST(COALESCE(i.chest_id,public_chest.id,0) AS UNSIGNED),i.vnum,
         IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
  FROM locker_items i LEFT JOIN locker_items p ON p.id=i.container_id
  LEFT JOIN private_chests public_chest
    ON public_chest.locker_id=i.locker_id AND public_chest.is_public=1
  WHERE i.obj_uid>0
  UNION ALL
  SELECT 'account_locker_items',i.id,i.obj_uid,p.obj_uid,5,
         CAST(i.chest_id AS UNSIGNED),0,i.vnum,
         IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
  FROM account_locker_items i LEFT JOIN account_locker_items p ON p.id=i.container_id
  WHERE i.obj_uid>0
  UNION ALL
  SELECT 'saved_items',i.id,i.obj_uid,p.obj_uid,3,CAST(i.room_vnum AS UNSIGNED),0,i.vnum,
         IF(i.container_id IS NOT NULL AND p.obj_uid IS NULL,1,0)
  FROM saved_items i LEFT JOIN saved_items p ON p.id=i.container_id WHERE i.obj_uid>0
), ledger AS (
  SELECT item_uid,COUNT(*) event_count FROM item_ownership_ledger GROUP BY item_uid
), open_quarantine AS (
  SELECT DISTINCT item_uid FROM item_ownership_quarantine
  WHERE repaired_at IS NULL AND item_uid>0
)
SELECT candidate.source_table,candidate.source_row_id,candidate.item_uid,
       candidate.payload_parent_uid,candidate.payload_owner_type,
       candidate.payload_owner_id,candidate.payload_owner_context_id,candidate.vnum,
       candidate.broken_parent,open_quarantine.item_uid IS NOT NULL,
       current_item.root_item_uid,current_item.parent_item_uid,current_item.owner_type,
       current_item.owner_id,current_item.owner_context_id,current_item.item_revision,
       current_item.vnum,current_item.state,
       CASE WHEN baseline.item_uid IS NULL THEN NULL
            ELSE baseline.opening_item_revision+COALESCE(ledger.event_count,0) END,
       payload_parent.state,payload_parent.owner_type,payload_parent.owner_id,
       payload_parent.owner_context_id
FROM candidate
LEFT JOIN item_current_owner current_item ON current_item.item_uid=candidate.item_uid
LEFT JOIN item_current_owner payload_parent
  ON payload_parent.item_uid=candidate.payload_parent_uid
LEFT JOIN item_ownership_baseline baseline ON baseline.item_uid=candidate.item_uid
LEFT JOIN ledger ON ledger.item_uid=candidate.item_uid
LEFT JOIN open_quarantine ON open_quarantine.item_uid=candidate.item_uid
ORDER BY candidate.item_uid,candidate.source_table,candidate.source_row_id;
"""


def _integer(value: str) -> int:
    try:
        return int(value)
    except ValueError as error:
        raise TopologyError("topology query returned malformed data") from error


def _optional(value: str) -> int | None:
    return None if value == "NULL" else _integer(value)


def load_rows(query: Callable[[str], str]) -> list[Row]:
    output = query(classification_sql())
    result: list[Row] = []
    for line in output.splitlines() if output else []:
        fields = line.split("\t")
        if len(fields) != 23 or re.fullmatch(r"[a-z_]+", fields[0]) is None:
            raise TopologyError("topology query returned malformed data")
        result.append(Row(
            fields[0], _integer(fields[1]), _integer(fields[2]), _optional(fields[3]),
            _integer(fields[4]), _integer(fields[5]), _integer(fields[6]),
            _integer(fields[7]), bool(_integer(fields[8])), bool(_integer(fields[9])),
            *(_optional(value) for value in fields[10:]),
        ))
    return result


def _canonical_rows(rows: list[Row]) -> tuple[dict[int, Row], dict[int, str]]:
    grouped: dict[int, list[Row]] = defaultdict(list)
    for row in rows:
        grouped[row.item_uid].append(row)
    canonical: dict[int, Row] = {}
    invalid: dict[int, str] = {}
    for item_uid, candidates in grouped.items():
        first = candidates[0]
        payloads = {
            (row.payload_parent_uid, row.payload_owner_type, row.payload_owner_id,
             row.payload_owner_context_id, row.vnum) for row in candidates
        }
        if any(row.broken_parent for row in candidates):
            invalid[item_uid] = "missing_payload_parent"
        elif len(payloads) != 1:
            invalid[item_uid] = "ambiguous_payload"
        canonical[item_uid] = first
    return canonical, invalid


def _roots(rows: dict[int, Row], invalid: dict[int, str], maximum_depth: int = 32) \
        -> tuple[dict[int, int], dict[int, str]]:
    roots: dict[int, int] = {}
    failures = dict(invalid)
    for item_uid in rows:
        if item_uid in failures:
            continue
        path: list[int] = []
        visited: set[int] = set()
        current = item_uid
        root: int | None = None
        category: str | None = None
        while True:
            if current in failures:
                category = failures[current]
                break
            if current in visited:
                category = "cycle"
                break
            row = rows.get(current)
            if row is None:
                category = "missing_ancestor"
                break
            visited.add(current)
            path.append(current)
            if len(path) > maximum_depth:
                category = "depth_exceeded"
                break
            if row.payload_parent_uid is None:
                root = current
                break
            current = row.payload_parent_uid
        if category:
            failures[item_uid] = category
        elif root is not None:
            roots[item_uid] = root
    return roots, failures


def classify(rows: list[Row], maximum_depth: int = 32) -> list[Finding]:
    canonical, initial = _canonical_rows(rows)
    roots, graph_failures = _roots(canonical, initial, maximum_depth)
    findings: list[Finding] = []
    for item_uid, row in canonical.items():
        root = roots.get(item_uid)
        if row.quarantined:
            category = "expected_quarantine"
        elif item_uid in graph_failures:
            category = graph_failures[item_uid]
        elif row.current_owner_type is None:
            category = "missing_current_owner"
        elif row.state != 1:
            category = "expected_inactive_state"
        elif row.current_vnum != row.vnum:
            category = "vnum_mismatch"
        elif row.expected_item_revision is None and row.item_revision == 0:
            category = "missing_baseline"
        elif row.expected_item_revision is not None and \
                row.item_revision != row.expected_item_revision:
            category = "item_revision_mismatch"
        elif (row.current_owner_type, row.current_owner_id,
              row.current_owner_context_id) != (
                  row.payload_owner_type, row.payload_owner_id,
                  row.payload_owner_context_id):
            category = "owner_disagreement"
        elif row.payload_parent_uid is not None and row.payload_parent_state is None:
            category = "missing_current_parent"
        elif row.payload_parent_uid is not None and (
                row.payload_parent_state != 1 or
                (row.payload_parent_owner_type, row.payload_parent_owner_id,
                 row.payload_parent_owner_context_id) != (
                     row.payload_owner_type, row.payload_owner_id,
                     row.payload_owner_context_id)):
            category = "foreign_or_inactive_parent"
        elif row.current_parent_uid != row.payload_parent_uid or \
                row.current_root_uid != root:
            category = "repairable_projection_lag"
        else:
            continue
        findings.append(Finding(category, row, root))
    return sorted(findings, key=lambda finding: (
        finding.category, finding.row.source_table, finding.row.source_row_id))


def summary(rows: list[Row], findings: list[Finding]) -> str:
    counts = Counter(finding.category for finding in findings)
    expected = sum(counts[category] for category in EXPECTED_CATEGORIES)
    repairable = sum(counts[category] for category in REPAIRABLE_CATEGORIES)
    corrupt = len(findings) - expected - repairable
    categories = ",".join(f"{name}:{counts[name]}" for name in sorted(counts)) or "none"
    return (
        f"payload_rows={len(rows)} unique_items={len({row.item_uid for row in rows})} "
        f"expected_transitions={expected} repairable_drift={repairable} "
        f"corruption={corrupt} categories={categories}"
    )


def write_artifact(path: Path, database: str, findings: list[Finding]) -> str:
    if not path.is_absolute() or not findings:
        raise TopologyError("artifact requires an absolute path and at least one finding")
    try:
        parent = path.parent.resolve(strict=True)
        metadata = parent.stat()
    except OSError as error:
        raise TopologyError(f"cannot inspect artifact directory: {error}") from error
    if parent != path.parent or metadata.st_uid != os.getuid() or \
            not stat.S_ISDIR(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise TopologyError("artifact directory must be owner-only without symlinks")
    payload = ("\n".join([
        ARTIFACT_HEADER, f"# database={database}", ARTIFACT_COLUMNS,
        *(finding.artifact_row() for finding in findings),
    ]) + "\n").encode()
    flags = os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0)
    try:
        descriptor = os.open(path, flags, 0o600)
        with os.fdopen(descriptor, "wb") as destination:
            destination.write(payload)
    except OSError as error:
        raise TopologyError(f"cannot create protected artifact: {error}") from error
    return hashlib.sha256(payload).hexdigest()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Classify saved item topology against authoritative custody.")
    parser.add_argument("--env-file", type=Path, default=ROOT / ".env")
    parser.add_argument("--artifact", type=Path)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    try:
        config = read_env_file(arguments.env_file.resolve())
        if arguments.artifact and active_connections(config):
            raise TopologyError(
                "protected classification requires a quiesced database save boundary")
        rows = load_rows(lambda statement: run_mysql(config, statement))
        findings = classify(rows)
        rendered = summary(rows, findings)
        if arguments.artifact:
            digest = write_artifact(arguments.artifact, config["DB_NAME"], findings)
            rendered += f" artifact_sha256={digest}"
        print("item topology classification: " + rendered)
        unsafe = any(not finding.expected for finding in findings)
        return 1 if unsafe else 0
    except (LegacyImportError, TopologyError, KeyError, OSError) as error:
        print(f"item topology classification blocked: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
