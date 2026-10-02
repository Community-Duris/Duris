#!/usr/bin/env python3
"""Assemble read-only persistence evidence and explain recovery requirements."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
from collections import Counter

from classify_item_topology import TopologyError, connection_arguments
from import_legacy_dump import LegacyImportError, process_environment, read_env_file

MAX_BYTES = 8 * 1024 * 1024
MAX_ROWS = 8192
MAX_HISTORY = 256
DIAGNOSES = (
    "none", "invalid_snapshot_item", "invalid_snapshot_parent", "duplicate_snapshot_uid",
    "malformed_active_custody_row", "duplicate_equipment_slot",
    "active_custody_absent_from_snapshot", "custody_vnum_mismatch",
    "duplicate_custody_match", "snapshot_item_absent_from_custody",
    "invalid_custody_topology", "invalid_death_payload",
    "saved_item_absent_from_death_payload", "orphaned_saved_item", "orphaned_saved_pet_item",
)


class DoctorError(ValueError):
    """An aggregate-safe diagnostic refusal."""


def target(kind: str, value: str) -> dict:
    if kind == "operation":
        if not re.fullmatch(r"[0-9a-fA-F]{32}", value) or int(value, 16) == 0:
            raise DoctorError("invalid operation target")
        return {"kind": kind, "value": value.lower()}
    if kind not in ("player", "item") or not re.fullmatch(r"[0-9]+", value):
        raise DoctorError("invalid numeric target")
    limit = 2**31 - 1 if kind == "player" else 2**64 - 1
    if not 1 <= int(value) <= limit:
        raise DoctorError("target outside native bounds")
    return {"kind": kind, "value": str(int(value))}


def read_report(path: Path) -> dict:
    metadata = path.lstat()
    if not stat.S_ISREG(metadata.st_mode) or stat.S_IMODE(metadata.st_mode) & 0o077:
        raise DoctorError("diagnostic input must be an owner-only regular file")
    if metadata.st_size > MAX_BYTES:
        raise DoctorError("diagnostic input exceeds byte bound")
    report = json.loads(path.read_bytes())
    if not isinstance(report, dict) or report.get("schema_version") != 1 or report.get("report_type") != "duris-persistence-diagnostic":
        raise DoctorError("unsupported native diagnostic report")
    if not all(isinstance(report.get(key), dict) for key in ("target", "state", "history")):
        raise DoctorError("invalid native report structure")
    report["target"] = target(report["target"]["kind"], report["target"]["value"])
    history = report["history"]
    for collection, maximum in (("events", 64), ("incidents", 64)):
        if not isinstance(history[collection], list) or len(history[collection]) > maximum:
            raise DoctorError("native history exceeds bound")
        for event in history[collection]:
            if not isinstance(event, dict) or not isinstance(event.get("stage"), str):
                raise DoctorError("invalid native history")
            for field in ("sequence", "error", "diagnosis"):
                if not isinstance(event.get(field, 0), int) or not 0 <= event.get(field, 0) <= 2**64 - 1:
                    raise DoctorError("invalid native event scalar")
    return report


def sql_snapshot_statement(selected: dict) -> str:
    """All SELECTs share one read-only consistent view; no payload prose is read."""
    selected = target(selected["kind"], selected["value"])
    kind, value = selected["kind"], selected["value"]
    if kind == "player":
        owner = f"(c.owner_type=1 AND c.owner_id={value} AND c.owner_context_id=0) OR (c.owner_type=11 AND c.owner_context_id={value})"
        pi, pet = f"i.pid={value}", f"p.owner_pid={value}"
        owner += f" OR c.item_uid IN (SELECT obj_uid FROM player_items WHERE pid={value} UNION SELECT pi.obj_uid FROM player_pet_items pi JOIN player_pets pet ON pet.id=pi.pet_id WHERE pet.owner_pid={value})"
        ledger = f"(l.from_owner_type=1 AND l.from_owner_id={value}) OR (l.to_owner_type=1 AND l.to_owner_id={value}) OR (l.from_owner_type=11 AND l.from_owner_context_id={value}) OR (l.to_owner_type=11 AND l.to_owner_context_id={value})"
    elif kind == "item":
        owner = f"c.item_uid={value} OR c.root_item_uid=COALESCE((SELECT root_item_uid FROM item_current_owner WHERE item_uid={value}),{value})"
        scoped = f"SELECT c.item_uid FROM item_current_owner c WHERE {owner}"
        pi = pet = f"i.obj_uid={value} OR i.obj_uid IN ({scoped})"
        ledger = f"l.item_uid={value} OR l.item_uid IN ({scoped})"
    else:
        ledger = f"l.operation_id=UNHEX('{value}')"
        scoped = f"SELECT l.item_uid FROM item_ownership_ledger l WHERE {ledger}"
        owner = f"c.item_uid IN ({scoped})"
        pi = pet = f"i.obj_uid IN ({scoped})"
    statements = [
        "SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ",
        "START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY",
        "SELECT JSON_OBJECT('kind','capture','utc',UTC_TIMESTAMP(6))",
    ]
    if kind == "player":
        statements.append(f"SELECT JSON_OBJECT('kind','player','pid',pid,'save_revision',save_revision) FROM player_data WHERE pid={value}")
    statements.extend([
        f"SELECT JSON_OBJECT('kind','custody','uid',c.item_uid,'root',c.root_item_uid,'parent',COALESCE(c.parent_item_uid,0),'owner_type',c.owner_type,'owner_id',c.owner_id,'owner_context',c.owner_context_id,'revision',c.item_revision,'owner_revision',r.revision,'vnum',c.vnum,'slot',c.equipment_slot,'state',c.state,'inline_coin',c.coin_payload IS NOT NULL) FROM item_current_owner c LEFT JOIN item_owner_revision r ON r.owner_type=c.owner_type AND r.owner_id=c.owner_id AND r.owner_context_id=c.owner_context_id WHERE {owner} ORDER BY c.item_uid LIMIT {MAX_ROWS + 1}",
        f"SELECT JSON_OBJECT('kind','payload','source','player_items','row',i.id,'uid',i.obj_uid,'parent',COALESCE(parent.obj_uid,0),'broken_parent',i.container_id IS NOT NULL AND parent.id IS NULL,'owner_type',1,'owner_id',i.pid,'owner_context',0,'vnum',i.vnum,'slot',i.equip_slot) FROM player_items i LEFT JOIN player_items parent ON parent.id=i.container_id WHERE {pi} ORDER BY i.id LIMIT {MAX_ROWS + 1}",
        f"SELECT JSON_OBJECT('kind','payload','source','player_pet_items','row',i.id,'uid',i.obj_uid,'parent',COALESCE(parent.obj_uid,0),'broken_parent',i.container_id IS NOT NULL AND parent.id IS NULL,'owner_type',11,'owner_id',p.pet_uid,'owner_context',p.owner_pid,'vnum',i.vnum,'slot',i.equip_slot) FROM player_pet_items i JOIN player_pets p ON p.id=i.pet_id LEFT JOIN player_pet_items parent ON parent.id=i.container_id WHERE {pet} ORDER BY i.id LIMIT {MAX_ROWS + 1}",
        f"SELECT JSON_OBJECT('kind','ledger','uid',l.item_uid,'revision',l.item_revision,'operation',LOWER(HEX(l.operation_id)),'reason',l.reason_type,'source_site',l.source_site,'to_owner_type',l.to_owner_type,'to_owner_id',l.to_owner_id,'to_owner_context',l.to_owner_context_id) FROM item_ownership_ledger l WHERE {ledger} ORDER BY l.created_at DESC,l.operation_id,l.event_index LIMIT {MAX_HISTORY + 1}",
        f"WITH recent AS (SELECT l.operation_id FROM item_ownership_ledger l WHERE {ledger} ORDER BY l.created_at DESC,l.operation_id,l.event_index LIMIT {MAX_HISTORY + 1}), relevant AS (SELECT DISTINCT operation_id FROM recent) SELECT JSON_OBJECT('kind','receipt','operation',LOWER(HEX(i.operation_id)),'command_type',i.command_type,'status',i.status,'result_code',i.result_code,'failure_stage',i.failure_stage,'durable_revision',i.durable_revision,'command_hash',LOWER(HEX(i.command_hash)),'result_hash',LOWER(SHA2(i.result_payload,256))) FROM critical_operation_inbox i JOIN relevant r ON r.operation_id=i.operation_id LIMIT {MAX_HISTORY + 1}",
    ])
    if kind == "operation":
        statements.append(f"SELECT JSON_OBJECT('kind','receipt','operation',LOWER(HEX(operation_id)),'command_type',command_type,'status',status,'result_code',result_code,'failure_stage',failure_stage,'durable_revision',durable_revision,'command_hash',LOWER(HEX(command_hash)),'result_hash',LOWER(SHA2(result_payload,256))) FROM critical_operation_inbox WHERE operation_id=UNHEX('{value}')")
        statements.append(f"SELECT JSON_OBJECT('kind','outbox','operation',LOWER(HEX(operation_id)),'status',status,'event_index',event_index,'payload_hash',LOWER(SHA2(payload,256))) FROM critical_outbox WHERE operation_id=UNHEX('{value}') ORDER BY event_index LIMIT {MAX_HISTORY + 1}")
    statements.append("ROLLBACK")
    return ";\n".join(statements) + ";"


def capture_sql(config: dict, selected: dict) -> dict:
    port = config.get("DB_PORT", "")
    if not port.isdigit() or not 1 <= int(port) <= 65535:
        raise DoctorError("invalid database port")
    allowed = {entry.strip() for entry in config["DB_ALLOWED_TARGETS"].split(",")}
    if f"{config['DB_HOST']}/{config['DB_NAME']}" not in allowed:
        raise DoctorError("database target is not explicitly allow-listed")
    if not re.fullmatch(r"[A-Za-z0-9_]+", config["DB_NAME"]):
        raise DoctorError("invalid database name")
    try:
        # Keep the doctor's explicitly selected TCP target; a socket from the
        # environment file must not override its host/port or remote TLS policy.
        transport = connection_arguments({**config, "DB_SOCKET": ""})
    except TopologyError as error:
        raise DoctorError("remote SQL capture requires verified TLS and a valid CA file") from error
    command = ["mysql", "--no-defaults", "--protocol=tcp", *transport,
               "--user", config["DB_USER"], "--batch", "--skip-column-names",
               "--raw", "--connect-timeout=5", config["DB_NAME"]]
    completed = subprocess.run(command, input=sql_snapshot_statement(selected), text=True,
                               capture_output=True, timeout=30, env=process_environment(config))
    if completed.returncode:
        # MySQL prose can include database/user identity or SQL. Preserve numeric code only.
        code = re.search(r"ERROR ([0-9]+)", completed.stderr)
        raise DoctorError("read-only SQL capture failed" + (f" (code {code[1]})" if code else ""))
    if len(completed.stdout.encode()) > MAX_BYTES:
        raise DoctorError("SQL diagnostic exceeds byte bound")
    rows = [json.loads(line) for line in completed.stdout.splitlines() if line.strip()]
    counts = Counter(row["kind"] for row in rows)
    gaps = []
    for category in ("custody", "payload", "ledger", "receipt", "outbox"):
        maximum = MAX_ROWS if category in ("custody", "payload") else MAX_HISTORY
        # Each of the two payload collections has its own bound.
        if category == "payload":
            overflow = any(sum(row.get("source") == source for row in rows) > MAX_ROWS
                           for source in ("player_items", "player_pet_items"))
        else:
            overflow = counts[category] > maximum
        if overflow:
            gaps.append(f"{category}_collection_truncated")
    return {"consistent_read": True, "complete": False, "gaps": gaps, "rows": rows}


def findings(native: dict | None) -> list[dict]:
    if not native:
        return []
    currents = {row["uid"]: row for row in native["rows"] if row["kind"] == "custody"}
    payloads = [row for row in native["rows"] if row["kind"] == "payload"]
    by_uid = Counter(row["uid"] for row in payloads)
    result = []
    for row in payloads:
        current = currents.get(row["uid"])
        reason = None
        if not row["uid"]:
            reason = "invalid_payload_uid"
        elif by_uid[row["uid"]] > 1:
            reason = "duplicate_payload_uid"
        elif not current:
            reason = "payload_without_custody"
        elif current["state"] != 1:
            reason = "withheld_inactive_custody"
        elif any(row[key] != current[key] for key in ("owner_type", "owner_id", "owner_context")):
            reason = "payload_owner_mismatch"
        elif row["vnum"] != current["vnum"]:
            reason = "payload_vnum_mismatch"
        elif row["broken_parent"]:
            reason = "missing_payload_parent"
        elif row["parent"] != current["parent"]:
            reason = "payload_topology_lag"
        if reason:
            result.append({"category": reason, "uid": row["uid"], "source": row["source"]})
    topology = {}
    slots = Counter((row["owner_type"], row["owner_id"], row["owner_context"], row.get("slot", 0))
                    for row in currents.values() if row["state"] == 1 and row.get("slot", 0))
    for uid, current in currents.items():
        if current["state"] == 1 and current["owner_type"] in (1, 11) and not by_uid[uid] and not current["inline_coin"]:
            result.append({"category": "custody_without_payload", "uid": uid})
        slot_key = (current["owner_type"], current["owner_id"], current["owner_context"], current.get("slot", 0))
        if current["state"] == 1 and current.get("slot", 0) and slots[slot_key] > 1:
            result.append({"category": "duplicate_custody_equipment_slot", "uid": uid})
        seen, path, node, category = set(), [], current, None
        while node:
            if node["uid"] in topology:
                category = topology[node["uid"]]
                break
            if node["uid"] in seen:
                category = "custody_cycle"
                break
            seen.add(node["uid"])
            path.append(node["uid"])
            if not node["parent"]:
                if node["root"] != node["uid"]:
                    category = "custody_root_conflict"
                break
            parent = currents.get(node["parent"])
            if not parent:
                category = "parent_outside_capture"
                break
            if any(node[key] != parent[key] for key in ("owner_type", "owner_id", "owner_context", "root")):
                category = "custody_parent_conflict"
                break
            node = parent
        for visited in path:
            topology[visited] = category
        if category:
            result.append({"category": category, "uid": uid})
    return result


def assess(report: dict | None, native: dict | None) -> dict:
    state = report.get("state", {}) if report else {}
    history = report.get("history", {}) if report else {}
    gaps = list(native.get("gaps", [])) if native else ["native_authority_not_captured"]
    if native:
        gaps.append("native_payload_coverage_player_and_pet_only")
    if native and report:
        gaps.append("runtime_and_sql_are_separate_observations")
    if not report:
        gaps.append("runtime_and_journal_state_not_captured")
    elif not history.get("available"):
        gaps.append("runtime_recorder_unavailable")
    if report and report["target"]["kind"] == "player" and not state.get("archive_available"):
        gaps.append("runtime_archive_metadata_unavailable")
    if report and not state.get("journal_available"):
        gaps.append("runtime_journal_metadata_unavailable")
    if report and not state.get("pipeline_available"):
        gaps.append("runtime_save_metadata_unavailable")
    if history.get("overwritten") or history.get("dropped") or history.get("incidents_evicted"):
        gaps.append("runtime_history_loss")
    if history.get("matching_events", 0) > len(history.get("events", [])):
        gaps.append("runtime_history_window_truncated")
    events = history.get("events", []) + history.get("incidents", [])
    if any(event.get("keys_truncated") for event in events):
        gaps.append("command_entity_keys_truncated")
    failed_saves = [event for event in events if event["stage"] in
                    ("save_result", "save_fence", "save_replay_result", "save_replay_fence")
                    and (event.get("error") or event.get("outcome") in (3, 4, 5))]
    last = max(failed_saves, key=lambda event: event["sequence"], default=None)
    diagnosis = last.get("diagnosis", 0) if last else 0
    diagnosis_name = DIAGNOSES[diagnosis] if 0 <= diagnosis < len(DIAGNOSES) else "unknown"
    categories = findings(native)
    if state.get("global_fence"):
        action = "investigate_global_journal_fence"
    elif state.get("policy_fence"):
        action = "operator_policy_disposition_required"
    elif state.get("quarantined"):
        action = "stopped_native_recovery_evaluation_required"
    elif state.get("load_degraded") or categories:
        action = "investigate_item_graph_before_saving"
    elif state.get("save_pending"):
        action = "inspect_pending_durability_or_publication"
    else:
        action = "inspect_evidence_no_recovery_authorization"
    return {"action": action, "diagnosis": diagnosis_name, "last_failed_save": last,
            "findings": categories, "gaps": sorted(set(gaps)), "recovery_verified": False,
            "required_recovery_evidence": [
                "coherent stopped authority and journal generation",
                "all original quarantined frames and integrity checks",
                "original creation command envelopes, frozen payloads and exact native receipts",
                "complete native player projection and unchanged custody history",
                "separate exact proof for death, quest, spell and craft obligations",
            ]}


def save_private(path: Path, value: dict) -> None:
    encoded = (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()
    if len(encoded) > MAX_BYTES:
        raise DoctorError("combined diagnostic exceeds byte bound")
    parent = path.parent.lstat()
    if not stat.S_ISDIR(parent.st_mode) or stat.S_IMODE(parent.st_mode) & 0o077:
        raise DoctorError("output requires an existing private directory")
    descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | getattr(os, "O_NOFOLLOW", 0), 0o600)
    with os.fdopen(descriptor, "wb") as output:
        output.write(encoded)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    selected = parser.add_mutually_exclusive_group()
    selected.add_argument("--pid")
    selected.add_argument("--item")
    selected.add_argument("--operation")
    parser.add_argument("--report", type=Path, help="owner-only JSON from world persistence diagnose")
    parser.add_argument("--env-file", type=Path, help="explicit allow-listed SQL target; SELECTs only")
    parser.add_argument("--binary", type=Path, help="hash the executable used for the incident")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        report = read_report(args.report) if args.report else None
        requested = next((target(kind, value) for kind, value in
                          (("player", args.pid), ("item", args.item), ("operation", args.operation)) if value), None)
        chosen = requested or (report["target"] if report else None)
        if not chosen or (requested and report and requested != report["target"]):
            raise DoctorError("a single matching diagnostic target is required")
        config = read_env_file(args.env_file) if args.env_file else None
        if config and ((report and report.get("backend") != "mariadb-primary") or
                       config.get("PERSISTENCE_MODE", "mariadb-primary") != "mariadb-primary"):
            raise DoctorError("SQL capture cannot be combined with a different selected authority")
        native = capture_sql(config, chosen) if config else None
        result = {"schema_version": 1, "target": chosen, "complete": False,
                  "runtime": report, "native": native, "assessment": assess(report, native)}
        if args.binary:
            digest = hashlib.sha256()
            with args.binary.open("rb") as executable:
                for block in iter(lambda: executable.read(1024 * 1024), b""):
                    digest.update(block)
            result["binary_sha256"] = digest.hexdigest()
        save_private(args.output, result)
        print(f"diagnostic saved; findings={len(result['assessment']['findings'])} "
              f"evidence_gaps={len(result['assessment']['gaps'])} recovery_verified=0")
        return 0
    except (DoctorError, LegacyImportError, OSError, ValueError, KeyError, TypeError,
            subprocess.TimeoutExpired) as error:
        # Never print source rows, SQL, credentials, or arbitrary exception prose.
        reason = str(error) if isinstance(error, DoctorError) else "input, configuration, or capture unavailable"
        print(f"persistence diagnosis refused: {reason}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
