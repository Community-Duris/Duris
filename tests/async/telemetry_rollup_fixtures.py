"""Test-only bridge from frozen B golden fixtures to C named SQL fact columns.

This selects the oracle's durable records, not the attempted input stream: it
preserves stale checkpoints but excludes conflicts and rolled-back transactions.
No production ingestion or fixture expectations are changed.
"""
from __future__ import annotations

from copy import deepcopy
import json
from pathlib import Path

from test_telemetry_contract_fixtures import OracleState, validate_fixture, FIXTURE_DIR

ENUMS = {
    "record_kind": {"interval": 1, "session_lifecycle": 2, "session_checkpoint": 3,
                    "coverage_gap": 4, "configuration": 5},
    "category": {"unknown": 0, "connected_idle": 1, "connected_active": 2, "resident_linkdead": 3},
    "context": {"unknown": 0, "none": 1, "combat": 2, "travel": 3, "social": 4,
                "crafting": 5, "administration": 6, "other": 7, "overflow_unknown": 8},
    "context_quality": {"unknown": 0, "observed": 1, "partial": 2, "overflow": 3, "unavailable": 4},
    "lifecycle": {"unknown": 0, "session_entered": 1, "session_exited": 2,
                  "connection_attached": 3, "connection_detached": 4},
    "end_reason": {"unknown": 0, "logout": 1, "disconnect": 2, "shutdown": 3,
                   "copyover": 4, "process_restart": 5},
    "gap_reason": {"unknown": 0, "detail_queue_drop": 1, "control_queue_drop": 2,
                   "sequence_gap": 3, "telemetry_disabled": 4, "unclosed_tail": 5,
                   "clock_discontinuity": 6},
    "backend": {"sql": 1, "flatfile_disabled": 2},
}


def sql_record(record: dict, ingest_id: int) -> dict:
    header = record["header"]
    kind = header["kind"]
    row = dict(header["key"]["producer"])
    row.update(record_seq=header["key"]["record_seq"], ingest_id=ingest_id,
               schema_version=header["schema_version"], record_kind=ENUMS["record_kind"][kind],
               occurrence_utc_usec=header["occurrence_utc_usec"],
               ingested_utc_usec=max(0, header["occurrence_utc_usec"]))
    payload = deepcopy(record["payload"][kind])
    session = payload.pop("session", None)
    if session is not None:
        identity = session.pop("id")
        row.update(session)
        row.update(session_boot_id=identity["producer"]["boot_id"],
                   session_process_id=identity["producer"]["process_id"],
                   session_seq=identity["session_seq"])
    connection = payload.pop("connection", None)
    if connection is not None:
        row.update(connection_boot_id=connection["producer"]["boot_id"],
                   connection_process_id=connection["producer"]["process_id"],
                   connection_seq=connection["connection_seq"])
    for nested in ("window", "dimensions", "cumulative", "config"):
        row.update(payload.pop(nested, {}))
    if kind == "session_checkpoint":
        payload["checkpoint_revision"] = payload.pop("revision")
    if kind == "coverage_gap":
        payload["gap_reason"] = payload.pop("reason")
    row.update(payload)
    if kind == "configuration":
        row["config_revision"] = row.pop("revision")
    row.pop("reserved", None)
    for name, values in ENUMS.items():
        if isinstance(row.get(name), str):
            row[name] = values[row[name]]
    if "fingerprint" in row:
        row["fingerprint"] = bytes.fromhex(row["fingerprint"])
    if any(isinstance(value, (dict, list)) for value in row.values()):
        raise AssertionError("unmapped structured fixture field")
    return row


def golden_rows(path: Path) -> tuple[dict, list[dict]]:
    fixture = json.loads(path.read_text())
    errors = validate_fixture(fixture, path)
    if errors:
        raise AssertionError(errors)
    state = OracleState({c["config_id"]: c for c in fixture.get("configurations", [])})
    for operation in fixture["operations"]:
        if operation.get("server_effect") == "rolled_back":
            continue
        for index in operation["record_indexes"]:
            state.apply_record(fixture["records"][index])
    return fixture, [sql_record(record, index) for index, record in
                     enumerate(state.record_store.values(), 1)]


def verify_bridge() -> None:
    paths = sorted(FIXTURE_DIR.glob("*.json"))
    assert len(paths) == 10
    count = 0
    for path in paths:
        fixture, rows = golden_rows(path)
        values = {key: 0 for key in ("connected_usec", "active_usec", "idle_usec", "unknown_usec",
                                   "resident_usec", "linkdead_usec")}
        for row in rows:
            if row["record_kind"] != 1:
                continue
            duration, category = row["duration_usec"], row["category"]
            values["resident_usec"] += duration
            if category == 3:
                values["linkdead_usec"] += duration
            else:
                values["connected_usec"] += duration
                values[{0: "unknown_usec", 1: "idle_usec", 2: "active_usec"}[category]] += duration
        expected = fixture["expected"]["metrics"].get("conservation")
        if expected:
            assert all(values[key] == expected[key] for key in values), (path, values, expected)
        count += len(rows)
    print(f"ISSUE268_GOLDEN_SQL_BRIDGE_OK fixtures={len(paths)} durable_facts={count}")


if __name__ == "__main__":
    verify_bridge()
