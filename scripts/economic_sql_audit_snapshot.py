#!/usr/bin/env python3
"""Export a bounded, explicitly incomplete SQL accounting audit cut.

The same read-only transaction captures baseline origins, retained accounting
rows, mapped native holding balances, and current UID positions. The result is
diagnostic input for the reconciler. It always says complete=false because
native lifecycle semantics and untracked UID history are not proven.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys

from economic_sql_audit_origins import (ITEM_STATES, OriginError, baseline_projection_bound,
                                       identity, read_origins_in_transaction)
from reconcile_economy_accounting import (MAX_INPUT_BYTES, MAX_ROWS, ORPHAN_EVIDENCE_SOURCES,
                                          REGISTRY_PATH, TABLES)
from economic_item_payload_audit import PROOF_NUMBERS, PayloadError, decode_single_item

MAX_ITEM_PAYLOAD_BYTES = 4 * 1024 * 1024
MAX_ITEM_ROWS = 8192
COIN_VNUM = 3
ITEM_MONEY = 20


class ExportError(ValueError):
    pass


def hex_id(value: bytes | None) -> str | None:
    return value.hex() if value is not None else None


def previous_item_revision(revision: int) -> int:
    """Native custody increments each UID once, independently of owner counters."""
    if type(revision) is not int or not 0 < revision < 2**64:
        raise ExportError("invalid native item ledger revision")
    return revision - 1


def account_key(lineage: bytes, kind: int, lifetime: int, context: int) -> str:
    return (lineage + struct.pack("<HHQQ4x", 1, kind, lifetime, context)).hex()


def bounded(cursor, sql: str, params: tuple = ()) -> list[dict]:
    cursor.execute(sql + " LIMIT %s", params + (MAX_ROWS + 1,))
    rows = cursor.fetchall()
    if len(rows) > MAX_ROWS:
        raise ExportError("SQL audit collection exceeds row limit")
    return rows


def coin_source_sql(alias: str = "") -> str:
    # The native codec stores ITEM_MONEY at byte31, including area prototypes.
    # This only selects candidates; full bounded decoding authenticates values.
    prefix = alias + "." if alias else ""
    return (f"({prefix}vnum={COIN_VNUM} OR "
            f"SUBSTRING({prefix}coin_payload,31,1)=X'14')")


def decode_coin_payload(blob: bytes, expected_uid: int, expected_vnum: int = COIN_VNUM) -> list[int]:
    """Decode denomination values using the independent bounded item reader."""
    try:
        item = decode_single_item(blob, "coin-pile")
    except PayloadError as error:
        raise ExportError(str(error)) from error
    if (item["uid"] != expected_uid or item["parent"] != -1 or
            type(expected_vnum) is not int or not 0 < expected_vnum <= 2**31 - 1 or
            item["vnum"] != expected_vnum or item["item_type"] != ITEM_MONEY or
            any(amount < 0 for amount in item["values"][:4])):
        raise ExportError("coin-pile payload identity or values are invalid")
    return item["values"][:4]


def realized_price_column_available(cursor) -> bool:
    cursor.execute(
        "SELECT COUNT(*) AS column_count FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND table_name='economic_accounting_operation' "
        "AND column_name='realized_price_copper'")
    return cursor.fetchone()["column_count"] == 1


def operation_rows(cursor, lineage: bytes, epoch: bytes,
                   has_realized_price: bool) -> tuple[list[dict], list[bytes]]:
    # EAP1 contains fixed-width non-personal IDs and arithmetic, unlike opaque
    # EAI1 facts. Bound the source before fetching/hex-encoding retained plans.
    from economic_restore_evidence import MAX_PLAN
    cursor.execute(
        "SELECT COUNT(*) AS root_count,COALESCE(SUM(OCTET_LENGTH(canonical_plan)),0) AS plan_bytes,"
        "COALESCE(MAX(OCTET_LENGTH(canonical_plan)),0) AS max_plan_bytes "
        "FROM economic_accounting_operation WHERE lineage=%s AND epoch=%s AND reason<>38",
        (lineage, epoch))
    bounds = cursor.fetchone()
    if (bounds is None or bounds["root_count"] > MAX_ROWS or bounds["plan_bytes"] * 2 > MAX_INPUT_BYTES or
            bounds["max_plan_bytes"] > MAX_PLAN):
        raise ExportError("original plan source exceeds audit input limit")
    price_column = "realized_price_copper" if has_realized_price else "NULL AS realized_price_copper"
    rows = bounded(cursor,
        "SELECT operation_id,original_operation_id,reason,outcome,result_code,source_event,"
        "accounting_version,writer_id,policy_version,compiler_version,actor_kind,actor_id,"
        "intent_digest,domain_digest,plan_digest,canonical_plan,"
        "account_count,posting_count,child_count,before_witness_count,after_witness_count,item_event_count," + price_column + " "
        "FROM economic_accounting_operation WHERE lineage=%s AND epoch=%s "
        "AND reason<>38 ORDER BY operation_id", (lineage, epoch))
    operations = []
    ids = []
    for row in rows:
        ids.append(row["operation_id"])
        operations.append({"operation_id": hex_id(row["operation_id"]),
                           "original_operation_id": hex_id(row["original_operation_id"]),
                           "lineage": lineage.hex(), "epoch": epoch.hex(),
                           "reason": row["reason"],
                           "outcome": {1: "committed", 2: "rejected"}.get(row["outcome"], "unknown"),
                           "result_code": row["result_code"],
                           "source_event": hex_id(row["source_event"]),
                           **{field: row[field] for field in ("accounting_version", "writer_id", "policy_version",
                                                             "compiler_version", "actor_kind", "actor_id",
                                                             "before_witness_count", "after_witness_count")},
                           **{field: hex_id(row[field]) for field in ("intent_digest", "domain_digest", "plan_digest",
                                                                     "canonical_plan")},
                           "account_count": row["account_count"],
                           "posting_count": row["posting_count"],
                           "child_count": row["child_count"],
                           "item_event_count": row["item_event_count"],
                           "realized_price_copper": row["realized_price_copper"]})
    return operations, ids


def scoped_rows(cursor, table: str, columns: str, lineage: bytes, epoch: bytes,
                order: str) -> list[dict]:
    # Names are fixed call-site constants; only lineage and epoch are bound data.
    return bounded(cursor,
        f"SELECT {columns} FROM {table} e JOIN economic_accounting_operation o "
        "ON o.operation_id=e.operation_id WHERE o.lineage=%s AND o.epoch=%s "
        f"AND o.reason<>38 ORDER BY {order}", (lineage, epoch))


def read_orphan_evidence(cursor) -> tuple[list[dict], dict]:
    # Without a root there is no trusted lineage/epoch to filter on. Capture
    # database-wide anomalies in this same read view, never a commit watermark.
    result = []
    counts = {}
    for name, (table, index, _) in ORPHAN_EVIDENCE_SOURCES.items():
        extra, join, order = "", "", f"e.operation_id,e.{index}"
        if name == "baseline_reservations":
            extra = ",e.identity_id,e.lineage AS claimed_lineage,e.epoch AS claimed_epoch"
            join = ("LEFT JOIN economic_baseline_witness o ON o.operation_id=e.operation_id "
                    "AND o.lineage=e.lineage AND o.epoch=e.epoch ")
            order += ",e.lineage,e.epoch,e.identity_id"
        else:
            join = "LEFT JOIN economic_accounting_operation o ON o.operation_id=e.operation_id "
        rows = bounded(cursor,
            f"SELECT e.operation_id,e.{index} AS row_index{extra} FROM {table} e " + join +
            f"WHERE o.operation_id IS NULL ORDER BY {order}")
        if len(result) + len(rows) > MAX_ROWS:
            raise ExportError("SQL orphan evidence collection exceeds row limit")
        counts[name] = len(rows)
        for row in rows:
            entry = {"table": name, "operation_id": hex_id(row["operation_id"]), "row_index": row["row_index"]}
            if name == "baseline_reservations":
                entry.update(identity_id=row["identity_id"], claimed_lineage=hex_id(row["claimed_lineage"]),
                             claimed_epoch=hex_id(row["claimed_epoch"]))
            result.append(entry)
    return result, {"scope": "database", "table_counts": counts}


def item_ledger_action(row: dict) -> str:
    """Classify supply endpoints without replacing explicit legacy reasons."""
    reason = row["reason_type"]
    if reason == 2:
        return "create"
    if reason == 3 or row["to_owner_type"] == 8:
        return "destroy"
    if row.get("from_owner_type") == 7 and row["item_revision"] == 1:
        return "create"
    return "move"


def item_ledger_state(row: dict) -> str:
    """Retain the collector's explicit quarantine endpoint contract."""
    if row["to_owner_type"] == 8:
        return "tombstone"
    if (row["reason_type"] == 21 and row.get("from_owner_type") == 10 and
            row["to_owner_type"] == 7 and row["to_owner_id"] == 0 and
            row["to_owner_context_id"] == 0):
        return "quarantined"
    return "live"


def item_ledger_previous_owner(row: dict) -> dict:
    owner = [row.get(field) for field in
             ("from_owner_type", "from_owner_id", "from_owner_context_id")]
    if owner == [None, None, None]:
        return {}  # Historical omission remains unknown, not an unowned item.
    if (type(owner[0]) is not int or not 0 <= owner[0] <= 12 or
            any(type(value) is not int or not 0 <= value < 2**64 for value in owner[1:])):
        raise ExportError("invalid native item previous owner")
    return {"from_owner": owner}


def read_evidence(cursor, lineage: bytes, epoch: bytes, has_realized_price: bool) -> dict:
    result = {name: [] for name in TABLES}
    result["operations"], _ = operation_rows(cursor, lineage, epoch, has_realized_price)
    effects = scoped_rows(cursor, "economic_accounting_account_effect",
        "e.operation_id,e.account_index,e.account_key,e.before_copper,e.before_silver,"
        "e.before_gold,e.before_platinum,e.after_copper,e.after_silver,e.after_gold,"
        "e.after_platinum,e.before_revision,e.after_revision", lineage, epoch,
        "e.operation_id,e.account_index")
    for row in effects:
        result["effects"].append({
            "operation_id": hex_id(row["operation_id"]), "account_index": row["account_index"],
            "account_key": row["account_key"].hex(),
            "before": [row[f"before_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            "after": [row[f"after_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            "before_revision": row["before_revision"], "after_revision": row["after_revision"]})
    postings = scoped_rows(cursor, "economic_accounting_coin_posting",
        "e.operation_id,e.line_index,e.event_index,e.account_index,e.child_index,e.delta_copper,"
        "e.delta_silver,e.delta_gold,e.delta_platinum,e.copper_value", lineage, epoch,
        "e.operation_id,e.line_index")
    for row in postings:
        result["postings"].append({
            "operation_id": hex_id(row["operation_id"]), "line_index": row["line_index"],
            "event_index": row["event_index"],
            "account_index": row["account_index"], "child_index": row["child_index"],
            "delta": [row[f"delta_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            "copper_value": row["copper_value"]})
    children = scoped_rows(cursor, "economic_accounting_child",
        "e.operation_id,e.child_index,e.child_operation_id,e.parent_index,"
        "e.domain_id,e.discriminator,e.relationship,e.receipt_operation_id", lineage, epoch,
        "e.operation_id,e.child_index")
    for row in children:
        result["children"].append({"operation_id": hex_id(row["operation_id"]),
                                    "child_index": row["child_index"],
                                    "child_operation_id": hex_id(row["child_operation_id"]),
                                    "parent_index": row["parent_index"],
                                    "domain_id": row["domain_id"],
                                    "discriminator": row["discriminator"],
                                    "relationship": row["relationship"],
                                    "receipt_operation_id": hex_id(row["receipt_operation_id"])})
    references = scoped_rows(cursor, "economic_accounting_item_reference",
        "e.operation_id,e.line_index,e.event_index,e.child_index,e.item_uid,e.before_revision,"
        "e.after_revision,e.legacy_operation_id,e.legacy_event_index", lineage, epoch,
        "e.operation_id,e.event_index")
    for row in references:
        result["item_references"].append({
            "operation_id": hex_id(row["operation_id"]), "event_index": row["event_index"],
            "line_index": row["line_index"],
            "child_index": row["child_index"], "uid": row["item_uid"],
            "before_revision": row["before_revision"], "after_revision": row["after_revision"],
            "legacy_operation_id": hex_id(row["legacy_operation_id"]),
            "legacy_event_index": row["legacy_event_index"]})
    claims = bounded(cursor,
        "SELECT s.lineage,s.source_event,s.operation_id,"
        "o.lineage AS operation_lineage,o.epoch AS operation_epoch,"
        "o.source_event AS operation_source_event,o.outcome AS operation_outcome,"
        "o.reason AS operation_reason,o.result_code AS operation_result_code,"
        "i.status AS operation_inbox_status,i.result_code AS operation_inbox_result_code,"
        "i.failure_stage AS operation_inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS operation_inbox_committed_at_present "
        "FROM economic_accounting_source_claim s "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=s.operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE s.lineage=%s ORDER BY s.source_event", (lineage,))
    result["source_claims"] = [{"lineage": hex_id(row["lineage"]),
                                "source_event": hex_id(row["source_event"]),
                                "operation_id": hex_id(row["operation_id"]),
                                "operation_lineage": hex_id(row["operation_lineage"]),
                                "operation_epoch": hex_id(row["operation_epoch"]),
                                "operation_source_event": hex_id(row["operation_source_event"]),
                                "operation_outcome": {1: "committed", 2: "rejected"}.get(
                                    row["operation_outcome"], "unknown"),
                                "operation_reason": row["operation_reason"],
                                "operation_result_code": row["operation_result_code"],
                                "operation_inbox_receipt": {
                                    "status": row["operation_inbox_status"],
                                    "result_code": row["operation_inbox_result_code"],
                                    "failure_stage": row["operation_inbox_failure_stage"],
                                    "committed_at_present": bool(
                                        row["operation_inbox_committed_at_present"]),
                                } if row["operation_lineage"] is not None else None}
                               for row in claims]
    cursor.execute(
        "SELECT COUNT(*) AS source_operations,COALESCE(SUM(CASE WHEN EXISTS("
        "SELECT 1 FROM economic_accounting_source_claim s WHERE s.lineage=o.lineage "
        "AND s.source_event=o.source_event AND s.operation_id=o.operation_id) "
        "THEN 0 ELSE 1 END),0) AS missing_claim_operations "
        "FROM economic_accounting_operation o WHERE o.lineage=%s "
        "AND o.outcome=1 AND o.source_event IS NOT NULL", (lineage,))
    source_count = cursor.fetchone()
    cursor.execute(
        "SELECT COUNT(*) AS duplicate_source_values FROM ("
        "SELECT source_event FROM economic_accounting_operation "
        "WHERE lineage=%s AND outcome=1 AND source_event IS NOT NULL "
        "GROUP BY source_event HAVING COUNT(*)>1) duplicates", (lineage,))
    duplicates = cursor.fetchone()
    result["source_claim_coverage"] = {
        "source_operations": int(source_count["source_operations"]),
        "missing_claim_operations": int(source_count["missing_claim_operations"]),
        "duplicate_source_values": int(duplicates["duplicate_source_values"])}
    registry = json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))
    required_reasons = sorted(row["number"] for row in registry["reasons"]
                              if row["source_event_required"])
    placeholders = ",".join("%s" for _ in required_reasons)
    cursor.execute(
        "SELECT COUNT(*) AS required_operations,COALESCE(SUM(source_event IS NULL),0) "
        "AS missing_source_events FROM economic_accounting_operation "
        f"WHERE lineage=%s AND outcome=1 AND reason IN ({placeholders})",
        (lineage, *required_reasons))
    policy_count = cursor.fetchone()
    result["source_event_policy_coverage"] = {
        "required_committed_operations": int(policy_count["required_operations"]),
        "missing_required_source_events": int(policy_count["missing_source_events"])}
    receipts = bounded(cursor,
        "SELECT o.operation_id,i.status,i.result_code,i.failure_stage,"
        "(i.committed_at IS NOT NULL) AS committed_at_present "
        "FROM economic_accounting_operation o "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE o.lineage=%s AND o.epoch=%s AND o.reason<>38 "
        "ORDER BY o.operation_id", (lineage, epoch))
    result["receipts"] = [{"operation_id": hex_id(row["operation_id"]),
                           "status": row["status"], "result_code": row["result_code"],
                           "failure_stage": row["failure_stage"],
                           "committed_at_present": bool(row["committed_at_present"])}
                          for row in receipts]
    ledger = bounded(cursor,
        "SELECT l.operation_id,l.event_index,l.item_uid,l.root_item_uid,l.parent_item_uid,"
        "l.from_owner_type,l.from_owner_id,l.from_owner_context_id,"
        "l.from_equipment_slot,l.to_equipment_slot,"
        "l.to_owner_type,l.to_owner_id,l.to_owner_context_id,l.item_revision,l.reason_type,"
        "r.before_revision FROM item_ownership_ledger l "
        "JOIN economic_accounting_item_reference r ON r.legacy_operation_id=l.operation_id "
        "AND r.legacy_event_index=l.event_index "
        "JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
        "WHERE o.lineage=%s AND o.epoch=%s AND o.reason<>38 "
        "ORDER BY l.operation_id,l.event_index", (lineage, epoch))
    for row in ledger:
        result["ownership_events"].append({
            "operation_id": hex_id(row["operation_id"]), "event_index": row["event_index"],
            "uid": row["item_uid"], "before_revision": row["before_revision"],
            "revision": row["item_revision"], "root": row["root_item_uid"],
            "parent": row["parent_item_uid"],
            "owner": [row["to_owner_type"], row["to_owner_id"], row["to_owner_context_id"]],
            **item_ledger_previous_owner(row),
            "from_equipment_slot": row["from_equipment_slot"], "to_equipment_slot": row["to_equipment_slot"],
            "state": item_ledger_state(row),
            "action": item_ledger_action(row)})
    return result


def bind_baseline_claim_witnesses(cursor, lineage: bytes, epoch: bytes,
                                claims: list[dict], origins: dict) -> None:
    """Attach source identities from verified EAB1 books in this read view."""
    baselines = [row for row in claims if row.get("operation_reason") == 38]
    if not baselines:
        return
    # Bound the whole retained lineage before decoding another epoch's book.
    cursor.execute(
        "SELECT COUNT(*) AS row_count,COALESCE(SUM(OCTET_LENGTH(w.canonical_witness)+"
        "COALESCE(OCTET_LENGTH(o.canonical_intent),0)+COALESCE(OCTET_LENGTH(o.canonical_plan),0)),0) "
        "AS blob_bytes FROM economic_baseline_witness w LEFT JOIN economic_accounting_operation o "
        "ON o.operation_id=w.operation_id WHERE w.lineage=%s", (lineage,))
    bounds = cursor.fetchone()
    if (bounds is None or bounds["row_count"] > MAX_ROWS or
            bounds["blob_bytes"] > MAX_INPUT_BYTES):
        raise ExportError("baseline claim witness source exceeds audit input limit")
    try:
        baseline_projection_bound(cursor, lineage)
    except OriginError as error:
        raise ExportError("baseline claim projection source exceeds audit input limit") from error
    cache = {epoch.hex(): origins["baseline_source_events"]}
    for claim in baselines:
        claim["baseline_witness"] = None
        other = claim.get("operation_epoch")
        if (claim.get("operation_lineage") != lineage.hex() or not isinstance(other, str) or
                re.fullmatch(r"[0-9a-f]{32}", other) is None or other == "0" * 32):
            continue
        if other not in cache:
            try:
                retained = read_origins_in_transaction(cursor, lineage, bytes.fromhex(other))
            except OriginError:
                cache[other] = {}
            else:
                cache[other] = retained["baseline_source_events"]
        source = cache[other].get(claim["operation_id"])
        if source is not None:
            claim["baseline_witness"] = {"lineage": lineage.hex(), "epoch": other,
                                         "operation_id": claim["operation_id"], "source_event": source}


def read_lineage_realized_prices(cursor, lineage: bytes,
                                 has_realized_price: bool) -> tuple[list[dict], dict]:
    registry = json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))
    reason_numbers = [row["number"] for row in registry["reasons"]
                      if row.get("realized_price_required") is True]
    if not reason_numbers:
        raise ExportError("realized-price policy is empty")
    price_column = "o.realized_price_copper" if has_realized_price else "NULL"
    placeholders = ",".join("%s" for _ in reason_numbers)
    rows = bounded(cursor,
        "SELECT o.operation_id,o.lineage,o.epoch,o.reason,o.outcome,o.result_code,"
        "i.status AS inbox_status,i.result_code AS inbox_result_code,"
        "i.failure_stage AS inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS inbox_committed_at_present," + price_column +
        " AS realized_price_copper FROM economic_accounting_operation o "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        f"WHERE o.lineage=%s AND o.reason IN ({placeholders}) "
        "ORDER BY o.epoch,o.operation_id", (lineage, *reason_numbers))
    prices = [{"operation_id": hex_id(row["operation_id"]),
               "lineage": hex_id(row["lineage"]),
               "epoch": hex_id(row["epoch"]), "reason": row["reason"],
               "outcome": {1: "committed", 2: "rejected"}.get(row["outcome"], "unknown"),
               "result_code": row["result_code"],
               "inbox_receipt": {
                   "status": row["inbox_status"],
                   "result_code": row["inbox_result_code"],
                   "failure_stage": row["inbox_failure_stage"],
                   "committed_at_present": bool(row["inbox_committed_at_present"]),
               },
               "realized_price_copper": row["realized_price_copper"]}
              for row in rows]
    missing = sum(row["realized_price_copper"] is None for row in prices)
    return prices, {"column_available": has_realized_price,
                    "candidate_rows": len(prices), "missing_price_rows": missing}


def read_lineage_uid_references(cursor, lineage: bytes) -> tuple[list[dict], list[dict], dict]:
    references = bounded(cursor,
        "SELECT r.operation_id,r.event_index,r.child_index,r.item_uid,r.before_revision,"
        "r.after_revision,r.legacy_operation_id,r.legacy_event_index,o.epoch,o.outcome,"
        "o.item_event_count,l.item_uid AS ledger_uid,l.item_revision,"
        "l.root_item_uid,l.parent_item_uid,l.from_owner_type,l.to_owner_type,l.to_owner_id,"
        "l.to_owner_context_id,l.reason_type "
        "FROM economic_accounting_item_reference r "
        "JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
        "LEFT JOIN item_ownership_ledger l ON l.operation_id=r.legacy_operation_id "
        "AND l.event_index=r.legacy_event_index "
        "WHERE o.lineage=%s AND o.reason<>38 ORDER BY r.operation_id,r.event_index", (lineage,))
    result = []
    for row in references:
        owner = None
        state = None
        action = None
        if row["ledger_uid"] is not None:
            owner = [row["to_owner_type"], row["to_owner_id"], row["to_owner_context_id"]]
            state = item_ledger_state(row)
            action = item_ledger_action(row)
        result.append({
            "operation_id": hex_id(row["operation_id"]), "event_index": row["event_index"],
            "child_index": row["child_index"], "uid": row["item_uid"],
            "before_revision": row["before_revision"], "after_revision": row["after_revision"],
            "legacy_operation_id": hex_id(row["legacy_operation_id"]),
            "legacy_event_index": row["legacy_event_index"],
            "operation_epoch": hex_id(row["epoch"]),
            "operation_outcome": {1: "committed", 2: "rejected"}.get(row["outcome"], "unknown"),
            "operation_item_event_count": row["item_event_count"],
            "ledger_uid": row["ledger_uid"],
            "ledger_before_revision": (previous_item_revision(row["item_revision"])
                                       if row["ledger_uid"] is not None else None),
            "ledger_revision": row["item_revision"], "ledger_root": row["root_item_uid"],
            "ledger_parent": row["parent_item_uid"], "ledger_owner": owner,
            "ledger_state": state, "ledger_action": action})
    roots = bounded(cursor,
        "SELECT o.operation_id,o.epoch,o.outcome,o.result_code,o.reason,o.source_event,"
        "o.item_event_count,i.status AS inbox_status,i.result_code AS inbox_result_code,"
        "i.failure_stage AS inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS inbox_committed_at_present,"
        "COALESCE(r.reference_count,0) AS reference_count "
        "FROM economic_accounting_operation o LEFT JOIN ("
        "SELECT operation_id,COUNT(*) AS reference_count "
        "FROM economic_accounting_item_reference GROUP BY operation_id) r "
        "ON r.operation_id=o.operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE o.lineage=%s AND o.reason<>38 "
        "AND (o.item_event_count<>0 OR r.reference_count<>0) ORDER BY o.operation_id",
        (lineage,))
    lineage_roots = [{
        "operation_id": hex_id(row["operation_id"]), "epoch": hex_id(row["epoch"]),
        "outcome": {1: "committed", 2: "rejected"}.get(row["outcome"], "unknown"),
        "reason": row["reason"], "source_event": hex_id(row["source_event"]),
        "item_event_count": row["item_event_count"],
        "result_code": row["result_code"],
        "inbox_receipt": {
            "status": row["inbox_status"],
            "result_code": row["inbox_result_code"],
            "failure_stage": row["inbox_failure_stage"],
            "committed_at_present": bool(row["inbox_committed_at_present"]),
        },
        "reference_count": int(row["reference_count"])} for row in roots]
    return result, lineage_roots, {"rows": len(result), "root_rows": len(lineage_roots)}


def read_uid_event_census(cursor, lineage: bytes, item_origins: list[dict], evidence: dict,
                          native_items: list[dict],
                          lineage_uid_references: list[dict]) -> tuple[list[dict], list[dict], dict,
                                                                       list[int], list[int], dict,
                                                                       list[dict], dict]:
    baseline_revisions = {row["uid"]: row["revision"] for row in item_origins}
    tracked_uids = set(baseline_revisions)
    referenced = {(bytes.fromhex(row["legacy_operation_id"]), row["legacy_event_index"], row["uid"])
                  for row in evidence["item_references"]}
    referenced.update((bytes.fromhex(row["legacy_operation_id"]),
                       row["legacy_event_index"], row["uid"])
                      for row in lineage_uid_references)
    tracked_uids.update(row["uid"] for row in evidence["item_references"])
    tracked_uids.update(row["uid"] for row in native_items)
    tracked_uids.update(row["uid"] for row in lineage_uid_references)
    if len(tracked_uids) > MAX_ROWS:
        raise ExportError("UID history scope exceeds audit bounds")
    ownership_rows = bounded(cursor,
        "SELECT DISTINCT l.item_uid,o.lineage FROM item_ownership_ledger l "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=l.operation_id "
        "WHERE o.reason IS NULL OR o.reason<>38 ORDER BY l.item_uid,o.lineage")
    uid_lineages: dict[int, set[bytes | None]] = {}
    for row in ownership_rows:
        uid_lineages.setdefault(row["item_uid"], set()).add(row["lineage"])
    ambiguous_uids = {uid for uid, lineages in uid_lineages.items() if len(lineages) > 1}
    selected_lineage_uids = {
        uid for uid, lineages in uid_lineages.items() if lineages == {lineage}}
    other_lineage_uids = {
        uid for uid, lineages in uid_lineages.items()
        if None not in lineages and lineage not in lineages and len(lineages) == 1}
    unattributed_uids = {
        uid for uid, lineages in uid_lineages.items() if lineages == {None}}
    unattributed_event_uids = sorted({row["item_uid"] for row in ownership_rows
                                      if row["lineage"] is None})
    unanchored_uids = sorted(selected_lineage_uids - tracked_uids)
    ambiguous_uid_list = sorted(ambiguous_uids)
    history_uids = tracked_uids | selected_lineage_uids
    if len(history_uids) > MAX_ROWS:
        raise ExportError("UID history scope exceeds audit bounds")
    history_uids_sorted = sorted(history_uids)
    events = []
    unreferenced = []
    unattributed_events = []
    ledger_events = 0
    referenced_events = 0
    for offset in range(0, len(history_uids_sorted), 128):
        batch = history_uids_sorted[offset:offset + 128]
        placeholders = ",".join("%s" for _ in batch)
        rows = bounded(cursor,
            "SELECT l.operation_id,l.event_index,l.item_uid,l.root_item_uid,l.parent_item_uid,"
            "l.from_equipment_slot,l.to_equipment_slot,"
            "l.from_owner_type,l.from_owner_id,l.from_owner_context_id,"
            "l.to_owner_type,l.to_owner_id,l.to_owner_context_id,l.item_revision,"
            "l.reason_type,o.epoch AS operation_epoch,"
            "o.outcome AS operation_outcome "
            "FROM item_ownership_ledger l "
            "JOIN economic_accounting_operation o ON o.operation_id=l.operation_id "
            "WHERE o.lineage=%s AND o.reason<>38 "
            f"AND l.item_uid IN ({placeholders}) ORDER BY l.item_uid,l.item_revision",
            (lineage, *batch))
        for row in rows:
            before_revision = previous_item_revision(row["item_revision"])
            if before_revision < baseline_revisions.get(row["item_uid"], 0):
                continue
            ledger_events += 1
            if ledger_events > MAX_ROWS:
                raise ExportError("UID ownership history exceeds audit bounds")
            event_key = (row["operation_id"], row["event_index"], row["item_uid"])
            is_referenced = event_key in referenced
            event = {
                "operation_id": hex_id(row["operation_id"]),
                "event_index": row["event_index"], "uid": row["item_uid"],
                "before_revision": before_revision, "revision": row["item_revision"],
                "root": row["root_item_uid"], "parent": row["parent_item_uid"],
                "owner": [row["to_owner_type"], row["to_owner_id"],
                          row["to_owner_context_id"]],
                **item_ledger_previous_owner(row),
                "from_equipment_slot": row["from_equipment_slot"],
                "to_equipment_slot": row["to_equipment_slot"],
                "state": item_ledger_state(row),
                "action": item_ledger_action(row),
                "operation_epoch": hex_id(row["operation_epoch"]),
                "operation_outcome": {1: "committed", 2: "rejected"}.get(
                    row["operation_outcome"], "unknown"),
                "referenced": is_referenced}
            events.append(event)
            if is_referenced:
                referenced_events += 1
            else:
                unreferenced.append(event)
    for offset in range(0, len(unattributed_event_uids), 128):
        batch = unattributed_event_uids[offset:offset + 128]
        placeholders = ",".join("%s" for _ in batch)
        rows = bounded(cursor,
            "SELECT l.operation_id,l.event_index,l.item_uid,l.root_item_uid,l.parent_item_uid,"
            "l.from_equipment_slot,l.to_equipment_slot,"
            "l.from_owner_type,l.from_owner_id,l.from_owner_context_id,"
            "l.to_owner_type,l.to_owner_id,l.to_owner_context_id,l.item_revision,"
            "l.reason_type FROM item_ownership_ledger l "
            "LEFT JOIN economic_accounting_operation o ON o.operation_id=l.operation_id "
            "WHERE o.operation_id IS NULL "
            f"AND l.item_uid IN ({placeholders}) ORDER BY l.item_uid,l.item_revision",
            tuple(batch))
        for row in rows:
            before_revision = previous_item_revision(row["item_revision"])
            if before_revision < baseline_revisions.get(row["item_uid"], 0):
                continue
            if len(unattributed_events) >= MAX_ROWS:
                raise ExportError("unattributed UID ownership history exceeds audit bounds")
            unattributed_events.append({
                "operation_id": hex_id(row["operation_id"]),
                "event_index": row["event_index"], "uid": row["item_uid"],
                "before_revision": before_revision, "revision": row["item_revision"],
                "root": row["root_item_uid"], "parent": row["parent_item_uid"],
                "owner": [row["to_owner_type"], row["to_owner_id"],
                          row["to_owner_context_id"]],
                **item_ledger_previous_owner(row),
                "from_equipment_slot": row["from_equipment_slot"],
                "to_equipment_slot": row["to_equipment_slot"],
                "state": item_ledger_state(row),
                "action": item_ledger_action(row)})
    return (events, unreferenced, {
        "tracked_uids": len(history_uids), "ledger_events": ledger_events,
        "referenced_events": referenced_events,
        "unreferenced_events": len(unreferenced)}, unanchored_uids, ambiguous_uid_list, {
            "ownership_uid_count": len(selected_lineage_uids),
            "anchored_ownership_uid_count": len(selected_lineage_uids & tracked_uids),
            "unanchored_ownership_uid_count": len(unanchored_uids),
        "other_lineage_ownership_uid_count": len(other_lineage_uids),
        "unattributed_ownership_uid_count": len(unattributed_uids),
        "ambiguous_lineage_ownership_uid_count": len(ambiguous_uid_list)},
        unattributed_events, {"uids": len({row["uid"] for row in unattributed_events}),
                              "events": len(unattributed_events)})


def append_committed_item_creation_origin(item_origins: list[dict], known_uids: set[int],
                                          uid: int, action: str, before_revision: int,
                                          outcome: str) -> bool:
    if (action != "create" or before_revision != 0 or outcome != "committed" or
            uid in known_uids):
        return False
    item_origins.append({"uid": uid, "origin": "creation", "revision": 0,
                         "root": uid, "parent": None, "owner": [0, 0, 0],
                         "state": "absent", "equipment_slot": 0})
    known_uids.add(uid)
    return True


def infer_created_account_origins(cursor, lineage: bytes, effects: list[dict],
                                  account_origins: list[dict], operations: list[dict]) -> list[dict]:
    known = {row["account_key"] for row in account_origins}
    operations_by_id = {row["operation_id"]: row for row in operations}
    first_effect = {}
    for effect in effects:
        key = effect["account_key"]
        current = first_effect.get(key)
        sort_key = (effect["before_revision"], effect["operation_id"])
        if current is None or sort_key < (current["before_revision"], current["operation_id"]):
            first_effect[key] = effect
    candidates = {}
    for key, effect in first_effect.items():
        if key in known or effect["before_revision"] != 0 or effect["before"] != [0, 0, 0, 0]:
            continue
        raw = bytes.fromhex(key)
        key_lineage = raw[:16]
        version, kind = struct.unpack_from("<HH", raw, 16)
        mapping_id, context_id = struct.unpack_from("<QQ", raw, 20)
        operation = operations_by_id.get(effect["operation_id"])
        if (key_lineage != lineage or version != 1 or kind not in range(1, 7) or
                mapping_id == 0 or operation is None or operation["outcome"] != "committed" or
                operation["lineage"] != lineage.hex()):
            continue
        candidates[mapping_id] = (key, kind, context_id, effect["operation_id"])
    if not candidates:
        return []
    mapping_ids = list(candidates)
    verified = {}
    for offset in range(0, len(mapping_ids), 128):
        batch = mapping_ids[offset:offset + 128]
        placeholders = ",".join("%s" for _ in batch)
        rows = bounded(cursor,
            "SELECT mapping_id,account_kind,context_id,creating_operation_id "
            "FROM economic_account_mapping WHERE lineage=%s AND backend_kind=1 "
            f"AND mapping_id IN ({placeholders}) ORDER BY mapping_id",
            (lineage, *batch))
        for row in rows:
            verified[row["mapping_id"]] = row
    inferred = []
    for mapping_id, (key, kind, context_id, operation_id) in candidates.items():
        row = verified.get(mapping_id)
        if (row is None or row["account_kind"] != kind or row["context_id"] != context_id or
                row["creating_operation_id"] != bytes.fromhex(operation_id)):
            continue
        inferred.append({"account_key": key, "origin": "creation",
                         "balance": [0, 0, 0, 0], "revision": 0})
    return inferred


def read_mapping_retirements(cursor, lineage: bytes, epoch: bytes,
                             account_origins: list[dict]) -> tuple[list[dict], dict, list[dict]]:
    rows = bounded(cursor,
        "SELECT m.mapping_id,m.account_kind,m.context_id,m.native_id,m.active_native_id,"
        "m.retiring_operation_id,o.lineage AS operation_lineage,o.epoch AS operation_epoch,"
        "o.reason AS operation_reason,o.outcome AS operation_outcome,"
        "o.result_code AS operation_result_code,"
        "i.status AS retirement_inbox_status,i.result_code AS retirement_inbox_result_code,"
        "i.failure_stage AS retirement_inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS retirement_inbox_committed_at_present,"
        "o.account_count,o.posting_count,o.child_count,o.item_event_count "
        "FROM economic_account_mapping m LEFT JOIN economic_accounting_operation o "
        "ON o.operation_id=m.retiring_operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE m.lineage=%s AND m.backend_kind=1 AND m.retiring_operation_id IS NOT NULL "
        "ORDER BY m.mapping_id", (lineage,))
    origins_by_key = {row["account_key"]: row for row in account_origins}
    retired = []
    root_metadata = {}
    current_epoch_rows = 0
    matched_opening_origins = 0
    unmatched_current_epoch_rows = 0
    for row in rows:
        operation_id = row["retiring_operation_id"]
        if operation_id is not None:
            metadata = {
                "operation_id": hex_id(operation_id),
                "lineage": hex_id(row["operation_lineage"]),
                "epoch": hex_id(row["operation_epoch"]),
                "reason": row["operation_reason"],
                "outcome": {1: "committed", 2: "rejected"}.get(
                    row["operation_outcome"], "unknown"),
                "result_code": row["operation_result_code"],
                "retirement_inbox_status": row["retirement_inbox_status"],
                "retirement_inbox_result_code": row["retirement_inbox_result_code"],
                "retirement_inbox_failure_stage": row["retirement_inbox_failure_stage"],
                "retirement_inbox_committed_at_present": bool(
                    row["retirement_inbox_committed_at_present"]),
                "account_count": row["account_count"],
                "posting_count": row["posting_count"],
                "child_count": row["child_count"],
                "item_event_count": row["item_event_count"],
                "effects": [], "postings": []}
            previous = root_metadata.setdefault(operation_id, metadata)
            if previous != metadata:
                raise ExportError("inconsistent retirement operation metadata")
        key = account_key(lineage, row["account_kind"], row["mapping_id"], row["context_id"])
        operation_epoch = hex_id(row["operation_epoch"])
        if operation_epoch == epoch.hex():
            current_epoch_rows += 1
            origin = origins_by_key.get(key)
            if origin is None:
                unmatched_current_epoch_rows += 1
            else:
                matched_opening_origins += 1
                if origin.get("retired_by") is None:
                    origin["retired_by"] = hex_id(row["retiring_operation_id"])
        retired.append({
            "mapping_id": row["mapping_id"], "account_key": key,
            "native_id": row["native_id"], "active_native_id": row["active_native_id"],
            "retiring_operation_id": hex_id(row["retiring_operation_id"]),
            "operation_lineage": hex_id(row["operation_lineage"]),
            "operation_epoch": operation_epoch,
            "operation_outcome": {1: "committed", 2: "rejected"}.get(
                row["operation_outcome"], "unknown"),
            "operation_result_code": row["operation_result_code"]})
    root_ids = list(root_metadata)
    for offset in range(0, len(root_ids), 64):
        batch = root_ids[offset:offset + 64]
        placeholders = ",".join("%s" for _ in batch)
        effects = bounded(cursor,
            "SELECT operation_id,account_index,account_key,before_copper,before_silver,"
            "before_gold,before_platinum,after_copper,after_silver,after_gold,after_platinum,"
            "before_revision,after_revision FROM economic_accounting_account_effect "
            f"WHERE operation_id IN ({placeholders}) ORDER BY operation_id,account_index",
            tuple(batch))
        postings = bounded(cursor,
            "SELECT operation_id,line_index,account_index,child_index,delta_copper,"
            "delta_silver,delta_gold,delta_platinum,copper_value "
            "FROM economic_accounting_coin_posting "
            f"WHERE operation_id IN ({placeholders}) ORDER BY operation_id,line_index",
            tuple(batch))
        for row in effects:
            root_metadata[row["operation_id"]]["effects"].append({
                "account_index": row["account_index"], "account_key": row["account_key"].hex(),
                "before": [row[f"before_{unit}"] for unit in
                           ("copper", "silver", "gold", "platinum")],
                "after": [row[f"after_{unit}"] for unit in
                          ("copper", "silver", "gold", "platinum")],
                "before_revision": row["before_revision"],
                "after_revision": row["after_revision"]})
        for row in postings:
            root_metadata[row["operation_id"]]["postings"].append({
                "line_index": row["line_index"], "account_index": row["account_index"],
                "child_index": row["child_index"],
                "delta": [row[f"delta_{unit}"] for unit in
                          ("copper", "silver", "gold", "platinum")],
                "copper_value": row["copper_value"]})
    roots = [root_metadata[operation_id] for operation_id in root_ids]
    if (sum(len(root["effects"]) for root in roots) > MAX_ROWS or
            sum(len(root["postings"]) for root in roots) > MAX_ROWS):
        raise ExportError("retirement root evidence exceeds audit bounds")
    return retired, {
        "rows": len(rows), "current_epoch_rows": current_epoch_rows,
        "matched_opening_origins": matched_opening_origins,
        "unmatched_current_epoch_rows": unmatched_current_epoch_rows,
        "root_rows": len(roots)}, roots


def read_mapping_creations(cursor, lineage: bytes) -> tuple[list[dict], dict, list[dict]]:
    mappings = bounded(cursor,
        "SELECT mapping_id,account_kind,context_id,native_id,active_native_id,"
        "creating_operation_id "
        "FROM economic_account_mapping WHERE lineage=%s AND backend_kind=1 "
        "ORDER BY mapping_id", (lineage,))
    creators = {}
    records = []
    for row in mappings:
        key = account_key(lineage, row["account_kind"], row["mapping_id"], row["context_id"])
        creator = hex_id(row["creating_operation_id"])
        records.append({"mapping_id": row["mapping_id"], "account_key": key,
                        "account_kind": row["account_kind"], "context_id": row["context_id"],
                        "native_id": row["native_id"],
                        "active_native_id": row["active_native_id"],
                        "creating_operation_id": creator})
        if row["creating_operation_id"] is not None:
            creators[row["creating_operation_id"]] = None
    operation_ids = list(creators)
    for offset in range(0, len(operation_ids), 64):
        batch = operation_ids[offset:offset + 64]
        placeholders = ",".join("%s" for _ in batch)
        roots = bounded(cursor,
            "SELECT o.operation_id,o.lineage,o.epoch,o.reason,o.outcome,o.result_code,"
            "i.status AS creator_inbox_status,i.result_code AS creator_inbox_result_code,"
            "i.failure_stage AS creator_inbox_failure_stage,"
            "(i.committed_at IS NOT NULL) AS creator_inbox_committed_at_present,"
            "o.source_event,o.account_count,o.posting_count,o.child_count,o.item_event_count,"
            "e.account_index,e.account_key,e.before_copper,e.before_silver,e.before_gold,"
            "e.before_platinum,e.after_copper,e.after_silver,e.after_gold,e.after_platinum,"
            "e.before_revision,e.after_revision FROM economic_accounting_operation o "
            "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
            "LEFT JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
            f"WHERE o.operation_id IN ({placeholders}) "
            "ORDER BY o.operation_id,e.account_index", tuple(batch))
        for row in roots:
            root = creators[row["operation_id"]]
            if root is None:
                root = {"operation_id": hex_id(row["operation_id"]),
                        "lineage": hex_id(row["lineage"]), "epoch": hex_id(row["epoch"]),
                        "reason": row["reason"],
                        "outcome": {1: "committed", 2: "rejected"}.get(row["outcome"], "unknown"),
                        "result_code": row["result_code"],
                        "creator_inbox_status": row["creator_inbox_status"],
                        "creator_inbox_result_code": row["creator_inbox_result_code"],
                        "creator_inbox_failure_stage": row["creator_inbox_failure_stage"],
                        "creator_inbox_committed_at_present": bool(
                            row["creator_inbox_committed_at_present"]),
                        "source_event": hex_id(row["source_event"]),
                        "account_count": row["account_count"],
                        "posting_count": row["posting_count"],
                        "child_count": row["child_count"],
                        "item_event_count": row["item_event_count"], "effects": []}
                creators[row["operation_id"]] = root
            if row["account_index"] is not None:
                root["effects"].append({
                    "account_index": row["account_index"],
                    "account_key": row["account_key"].hex(),
                    "before": [row[f"before_{unit}"] for unit in
                               ("copper", "silver", "gold", "platinum")],
                    "after": [row[f"after_{unit}"] for unit in
                              ("copper", "silver", "gold", "platinum")],
                    "before_revision": row["before_revision"],
                    "after_revision": row["after_revision"]})
    missing_creator_ids = [operation_id for operation_id, root in creators.items()
                           if root is None]
    for offset in range(0, len(missing_creator_ids), 64):
        batch = missing_creator_ids[offset:offset + 64]
        placeholders = ",".join("%s" for _ in batch)
        installations = bounded(cursor,
            "SELECT l.operation_id,l.lineage,l.epoch,l.baseline_operation_id,l.phase,"
            "l.selected_epoch,l.revision,"
            "i.status AS install_status,i.result_code AS install_result_code,"
            "i.failure_stage AS install_failure_stage,"
            "(i.committed_at IS NOT NULL) AS install_committed_at_present,"
            "b.lineage AS baseline_lineage,b.epoch AS baseline_epoch,b.reason AS baseline_reason,"
            "b.outcome AS baseline_outcome,b.result_code AS baseline_result_code "
            "FROM economic_sql_lifecycle_installation l "
            "JOIN critical_operation_inbox i ON i.operation_id=l.operation_id "
            "LEFT JOIN economic_accounting_operation b "
            "ON b.operation_id=l.baseline_operation_id "
            f"WHERE l.lineage=%s AND l.operation_id IN ({placeholders})",
            (lineage, *batch))
        for row in installations:
            operation_id = row["operation_id"]
            if creators.get(operation_id) is not None:
                continue
            baseline_id = hex_id(row["baseline_operation_id"])
            install_committed = (row["install_status"] == 1 and
                                 row["install_result_code"] == 0 and
                                 row["install_failure_stage"] == 0 and
                                 row["install_committed_at_present"] == 1 and
                                 row["phase"] == 2 and
                                 row["selected_epoch"] == row["epoch"] and
                                 row["revision"] == 1)
            baseline_committed = (baseline_id is not None and
                                  row["baseline_lineage"] == row["lineage"] and
                                  row["baseline_epoch"] == row["epoch"] and
                                  row["baseline_reason"] == 38 and
                                  row["baseline_outcome"] == 1 and
                                  row["baseline_result_code"] == 0)
            creators[operation_id] = {
                "operation_id": hex_id(operation_id),
                "lineage": hex_id(row["lineage"]), "epoch": hex_id(row["epoch"]),
                "reason": 38,
                "outcome": "committed" if install_committed and baseline_committed else "unknown",
                "result_code": 0 if install_committed and baseline_committed else 1,
                "source_event": None, "account_count": 0, "posting_count": 0,
                "child_count": 0, "item_event_count": 0, "effects": [],
                "creator_kind": "baseline_installation",
                "installation_phase": row["phase"],
                "installation_selected_epoch": (hex_id(row["selected_epoch"])
                                                 if row["selected_epoch"] is not None else None),
                "installation_revision": row["revision"],
                "installation_failure_stage": row["install_failure_stage"],
                "installation_committed_at_present": bool(
                    row["install_committed_at_present"]),
                "baseline_operation_id": baseline_id,
                "baseline_lineage": (hex_id(row["baseline_lineage"])
                                     if row["baseline_lineage"] is not None else None),
                "baseline_epoch": (hex_id(row["baseline_epoch"])
                                   if row["baseline_epoch"] is not None else None),
                "baseline_reason": row["baseline_reason"],
                "baseline_outcome": ({1: "committed", 2: "rejected"}.get(
                    row["baseline_outcome"], "unknown")),
                "baseline_result_code": row["baseline_result_code"]}
    roots = [root for root in creators.values() if root is not None]
    if sum(len(root["effects"]) for root in roots) > MAX_ROWS:
        raise ExportError("mapping creator root effects exceed audit bounds")
    missing_roots = sum(root is None for root in creators.values())
    return records, {"rows": len(mappings),
                     "creator_rows": sum(row["creating_operation_id"] is not None
                                         for row in records),
                     "root_rows": len(roots), "missing_roots": missing_roots}, roots


def infer_created_mapping_origins(lineage: str, mappings: list[dict],
                                  roots: list[dict], origins: list[dict]) -> list[dict]:
    known = {row["account_key"] for row in origins}
    roots_by_id = {row["operation_id"]: row for row in roots}
    inferred = []
    for mapping in mappings:
        key = mapping["account_key"]
        operation_id = mapping["creating_operation_id"]
        root = roots_by_id.get(operation_id)
        if (key in known or operation_id is None or root is None or
                root["lineage"] != lineage or root["outcome"] != "committed"):
            continue
        effects = [row for row in root["effects"] if row["account_key"] == key]
        if (len(effects) == 1 and effects[0]["before_revision"] == 0 and
                effects[0]["before"] == [0, 0, 0, 0]):
            inferred.append({"account_key": key, "origin": "creation",
                             "balance": [0, 0, 0, 0], "revision": 0})
            known.add(key)
    return inferred


def read_pending_claim_consumers(cursor, lineage: bytes) -> tuple[list[dict], dict]:
    rows = bounded(cursor,
        "SELECT o.operation_id,o.epoch,o.outcome,o.result_code,"
        "i.status AS inbox_status,i.result_code AS inbox_result_code,"
        "i.failure_stage AS inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS inbox_committed_at_present,e.account_key,"
        "e.before_copper,e.before_silver,e.before_gold,e.before_platinum,"
        "e.after_copper,e.after_silver,e.after_gold,e.after_platinum,"
        "COALESCE(s.source_rows,0) AS source_rows,"
        "COALESCE(s.source_amount,0) AS source_amount "
        "FROM economic_accounting_operation o "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "JOIN economic_accounting_account_effect e ON e.operation_id=o.operation_id "
        "LEFT JOIN (SELECT allocation_operation_id,COUNT(*) AS source_rows,SUM(amount) AS source_amount "
        "FROM (SELECT claim_operation_id AS allocation_operation_id,amount "
        "FROM economic_pending_claim_source WHERE lineage=%s AND claim_operation_id IS NOT NULL "
        "UNION ALL SELECT c.spending_operation_id,c.amount FROM economic_pending_claim_consumption c "
        "LEFT JOIN economic_pending_claim_source p ON p.source_operation_id=c.source_operation_id "
        "AND p.source_slot=c.source_slot LEFT JOIN economic_accounting_operation spender "
        "ON spender.operation_id=c.spending_operation_id WHERE p.lineage=%s OR spender.lineage=%s) allocations "
        "GROUP BY allocation_operation_id) s ON s.allocation_operation_id=o.operation_id "
        "WHERE o.lineage=%s AND o.outcome=1 AND e.before_copper>e.after_copper "
        "AND SUBSTRING(e.account_key,17,4)=UNHEX('01000500') "
        "ORDER BY o.operation_id,e.account_index", (lineage, lineage, lineage, lineage))
    consumers = {}
    for row in rows:
        key = row["account_key"]
        if not isinstance(key, bytes) or len(key) != 40 or key[:16] != lineage:
            continue
        version, kind = struct.unpack_from("<HH", key, 16)
        if version != 1 or kind != 5:
            continue
        before = [row[f"before_{unit}"] for unit in ("copper", "silver", "gold", "platinum")]
        after = [row[f"after_{unit}"] for unit in ("copper", "silver", "gold", "platinum")]
        if any(value is None for value in before + after):
            continue
        debit = before[0] - after[0]
        if debit <= 0:
            continue
        operation_id = row["operation_id"]
        consumer = consumers.get(operation_id)
        if consumer is None:
            # SQL SUM(BIGINT) is an exact Decimal on both supported drivers.
            # Preserve its integer value for JSON without float conversion.
            source_amount = int(row["source_amount"])
            if source_amount != row["source_amount"]:
                raise ExportError("nonintegral pending-claim source amount")
            consumer = {"operation_id": hex_id(operation_id),
                        "epoch": hex_id(row["epoch"]), "outcome": "committed",
                        "result_code": row["result_code"], "source_rows": row["source_rows"],
                        "inbox_receipt": {
                            "status": row["inbox_status"],
                            "result_code": row["inbox_result_code"],
                            "failure_stage": row["inbox_failure_stage"],
                            "committed_at_present": bool(
                                row["inbox_committed_at_present"]),
                        },
                        "source_amount": source_amount, "pending_claim_debits": []}
            consumers[operation_id] = consumer
        consumer["pending_claim_debits"].append({"account_key": key.hex(), "amount": debit,
                                                 "before": before, "after": after})
    result = [consumers[operation_id] for operation_id in sorted(consumers)]
    missing = sum(row["source_rows"] == 0 for row in result)
    mismatched = sum(row["source_rows"] > 0 and row["source_amount"] !=
                     sum(effect["amount"] for effect in row["pending_claim_debits"])
                     for row in result)
    return result, {"rows": len(result), "missing_source_rows": missing,
                    "mismatched_source_amounts": mismatched}


def read_pending_claim_consumptions(cursor, lineage: bytes) -> tuple[list[dict], dict]:
    rows = bounded(cursor,
        "SELECT c.spending_operation_id,c.source_operation_id,c.source_slot,c.amount "
        "FROM economic_pending_claim_consumption c LEFT JOIN economic_pending_claim_source s "
        "ON s.source_operation_id=c.source_operation_id AND s.source_slot=c.source_slot "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=c.spending_operation_id "
        # A row with neither retained root nor source has unknown lineage. Keep
        # its IDs as a database-wide orphan instead of attributing or hiding it.
        "WHERE s.lineage=%s OR o.lineage=%s OR (s.source_operation_id IS NULL AND o.operation_id IS NULL) "
        "ORDER BY c.spending_operation_id,c.source_operation_id,c.source_slot", (lineage, lineage))
    result = []
    for row in rows:
        if (any(type(row[field]) is not bytes or len(row[field]) != 16 or not any(row[field])
                for field in ("spending_operation_id", "source_operation_id")) or
                type(row["source_slot"]) is not int or not 0 < row["source_slot"] < 2**16 or
                type(row["amount"]) is not int or not 0 < row["amount"] < 2**64):
            raise ExportError("invalid pending claim consumption row")
        result.append({"spending_operation_id": hex_id(row["spending_operation_id"]),
                       "source_operation_id": hex_id(row["source_operation_id"]),
                       "source_slot": row["source_slot"], "amount": row["amount"]})
    return result, {"rows": len(result)}


def native_source_count(cursor, lineage: bytes, table: str, identity_column: str,
                        kind: int, predicate: str = "1=1") -> tuple[int, int]:
    # The table, locator and account kind are fixed call-site constants.
    cursor.execute(
        f"SELECT COUNT(*) AS source_rows,COALESCE(SUM(CASE WHEN EXISTS("
        "SELECT 1 FROM economic_account_mapping m WHERE "
        f"m.lineage=%s AND m.backend_kind=1 AND m.account_kind={kind} "
        f"AND m.locator_kind={kind} "
        f"AND m.active_native_id=n.{identity_column}) THEN 0 ELSE 1 END),0) "
        f"AS unmapped_rows FROM {table} n WHERE {predicate}", (lineage,))
    row = cursor.fetchone()
    if row is None:
        raise ExportError("missing native source census")
    return int(row["source_rows"]), int(row["unmapped_rows"])


def native_mapping_identity_valid(kind: int, locator_kind: int, native_id: int,
                                  active_native_id: int | None) -> bool:
    return (type(kind) is int and 1 <= kind <= 6 and
            type(locator_kind) is int and locator_kind == kind and
            type(native_id) is int and 0 < native_id < 2**64 and
            (active_native_id is None or
             (type(active_native_id) is int and active_native_id == native_id)))


def auction_escrow_mapping_is_live(status: str | None,
                                   winning_bidder_pid: int | None) -> bool:
    return (status == "OPEN" or
            (status == "REMOVED" and type(winning_bidder_pid) is int and
             winning_bidder_pid > 0))


def auction_escrow_balance(status: str | None, price: int | None,
                           winning_bidder_pid: int | None) -> list[int] | None:
    if (status not in ("OPEN", "REMOVED") or type(price) is not int or price < 0 or
            (winning_bidder_pid is not None and
             (type(winning_bidder_pid) is not int or winning_bidder_pid < 0))):
        return None
    has_winner = type(winning_bidder_pid) is int and winning_bidder_pid > 0
    if not has_winner and price != 0:
        return None
    return [price if has_winner else 0, 0, 0, 0]


def read_ship_coffers(cursor) -> tuple[list[dict], dict]:
    # Ships have persisted copper value but no qualified accounting lifetime or
    # native revision. Never manufacture a mapping or export owner aliases.
    rows = bounded(cursor, "SELECT id,money FROM ships ORDER BY id")
    coffers = []
    coverage = dict(rows=len(rows), positive_rows=0, zero_rows=0,
                    unknown_rows=0, invalid_rows=0, missing_revision_rows=len(rows))
    seen = set()
    for row in rows:
        identity, amount = row["id"], row["money"]
        if (type(identity) is not int or not 1 <= identity < 2**31 or
                identity in seen or
                (amount is not None and
                 (type(amount) is not int or not -(2**31) <= amount < 2**31))):
            raise ExportError("invalid native ship coffer")
        seen.add(identity)
        coffers.append({"ship_id": identity, "copper": amount})
        bucket = ("unknown_rows" if amount is None else "invalid_rows" if amount < 0
                  else "zero_rows" if amount == 0 else "positive_rows")
        coverage[bucket] += 1
    return coffers, coverage


def read_guild_treasuries(cursor) -> tuple[list[dict], dict]:
    # Guild IDs are reusable native locators. outcome_revision belongs to
    # prestige/construction and does not witness ordinary money writes.
    rows = bounded(cursor, "SELECT id,copper,silver,gold,platinum FROM guilds ORDER BY id")
    treasuries = []
    coverage = dict(rows=len(rows), positive_rows=0, zero_rows=0,
                    missing_revision_rows=len(rows))
    seen = set()
    for row in rows:
        identity = row["id"]
        balance = [row[unit] for unit in ("copper", "silver", "gold", "platinum")]
        if (type(identity) is not int or not 1 <= identity < 2**32 or identity in seen or
                any(type(amount) is not int or not 0 <= amount < 2**32 for amount in balance)):
            raise ExportError("invalid native guild treasury")
        seen.add(identity)
        treasuries.append({"guild_id": identity, "balance": balance})
        coverage["positive_rows" if any(balance) else "zero_rows"] += 1
    return treasuries, coverage


def read_auction_custody(cursor) -> dict:
    """Capture independent persisted roots; never load or clone an item blob."""
    cursor.execute(
        "SELECT (SELECT COUNT(*) FROM auctions)+"
        "(SELECT COUNT(*) FROM auction_item_custody)+"
        "(SELECT COUNT(*) FROM auction_item_pickups) AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("auction custody source exceeds audit bounds")
    listings = bounded(cursor,
        "SELECT id AS auction_id,seller_pid,winning_bidder_pid AS winner_pid,status,"
        "auction_revision AS revision,custody_state,quantity,obj_vnum AS vnum,"
        "OCTET_LENGTH(obj_blob_str) AS blob_bytes,SHA2(obj_blob_str,256) AS blob_sha256 "
        "FROM auctions ORDER BY id")
    roots = bounded(cursor,
        "SELECT auction_id,slot,item_uid AS uid,item_revision AS revision,vnum,claim_pid,"
        "HEX(claim_operation_id) AS claim_operation_id,claimed_at IS NOT NULL AS claimed,"
        "OCTET_LENGTH(obj_blob) AS blob_bytes,SHA2(obj_blob,256) AS blob_sha256 "
        "FROM auction_item_custody ORDER BY auction_id,slot")
    for row in roots:
        row["claimed"] = bool(row["claimed"])
        if row["claim_operation_id"] is not None:
            row["claim_operation_id"] = row["claim_operation_id"].lower()
    legacy = bounded(cursor,
        "SELECT id,pid,quantity,retrieved,OCTET_LENGTH(obj_blob_str) AS blob_bytes,"
        "SHA2(obj_blob_str,256) AS blob_sha256 FROM auction_item_pickups ORDER BY id")
    return {"auction_listings": list(listings), "auction_roots": list(roots),
            "auction_legacy_pickups": list(legacy),
            "auction_custody_coverage": {"listings": len(listings), "roots": len(roots),
                                         "legacy_pickups": len(legacy)}}


def read_shop_custody(cursor) -> dict:
    """Capture whole physical forests without custody joins or row filtering."""
    cursor.execute("SELECT (SELECT COUNT(*) FROM shopkeepers)+"
                   "(SELECT COUNT(*) FROM shopkeeper_items) AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("shop custody source exceeds audit bounds")
    keepers = bounded(cursor, "SELECT id AS keeper_id,shop_id FROM shopkeepers ORDER BY id")
    rows = bounded(cursor,
        "SELECT id AS item_id,shopkeeper_id AS keeper_id,container_id AS parent_id,"
        "obj_uid AS uid,vnum,equip_slot AS equipment_slot,quantity "
        "FROM shopkeeper_items ORDER BY id")
    for row in rows:
        # Match the native legacy quantity default without SQL expression
        # promotion to Decimal on MariaDB's smallint COALESCE result.
        if row["quantity"] is None:
            row["quantity"] = 1
    return {"shop_keepers": list(keepers), "shop_items": list(rows),
            "shop_custody_coverage": {"keepers": len(keepers), "items": len(rows)}}


def read_player_custody(cursor) -> dict:
    """Retain physical player/pet projections independently of UID authority."""
    cursor.execute("SELECT (SELECT COUNT(*) FROM player_data)+"
                   "(SELECT COUNT(*) FROM player_items)+(SELECT COUNT(*) FROM player_pets)+"
                   "(SELECT COUNT(*) FROM player_pet_items) AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("player custody source exceeds audit bounds")
    players = bounded(cursor, "SELECT pid FROM player_data ORDER BY pid")
    pets = bounded(cursor, "SELECT id AS pet_id,owner_pid AS pid,pet_uid FROM player_pets ORDER BY id")
    collections = []
    for table, owner, quantity in (("player_items", "pid", "quantity"),
                                   ("player_pet_items", "pet_id", "1 AS quantity")):
        collections.append(list(bounded(cursor,
            f"SELECT id AS item_id,{owner},container_id AS parent_id,obj_uid AS uid,vnum,"
            f"equip_slot AS equipment_slot,{quantity},item_type,"
            f"value0,value1,value2,value3 FROM {table} ORDER BY id")))
    return {"player_ids": list(players), "player_pets": list(pets),
            "player_items": collections[0], "pet_items": collections[1],
            "player_custody_coverage": dict(players=len(players), pets=len(pets),
                items=len(collections[0]), pet_items=len(collections[1]))}


def read_corpse_custody(cursor) -> dict:
    """Keep numeric corpse identity and every physical row in the same cut."""
    cursor.execute("SELECT (SELECT COUNT(*) FROM corpses)+"
                   "(SELECT COUNT(*) FROM corpse_items) AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("corpse custody source exceeds audit bounds")
    corpses = list(bounded(cursor, "SELECT id AS corpse_id,value3 AS pid,save_id,"
        "corpse_revision AS revision,room_vnum FROM corpses ORDER BY id"))
    rows = list(bounded(cursor, "SELECT id AS item_id,corpse_id,container_id AS parent_id,"
        "obj_uid AS uid,vnum,quantity,weight,extra_flags,value0,value1,value2,value3 "
        "FROM corpse_items ORDER BY id"))
    return {"corpses": corpses, "corpse_items": rows,
            "corpse_custody_coverage": dict(corpses=len(corpses), items=len(rows))}


def read_locker_custody(cursor) -> dict:
    """Keep both SQL locker namespaces raw; never merge equal numeric IDs."""
    names = ("lockers", "private_chests", "locker_items", "account_lockers",
             "locker_chests", "account_locker_items")
    cursor.execute("SELECT " + "+".join("(SELECT COUNT(*) FROM " + name + ")" for name in names) +
                   " AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("locker custody source exceeds audit bounds")
    collections = {}
    for table in ("lockers", "account_lockers"):
        fields = "id AS locker_id,racewar" + (",owner_pid,owner_assoc_id" if table == "lockers" else "")
        collections[table] = list(bounded(cursor, "SELECT " + fields + " FROM " + table + " ORDER BY id"))
    for table in ("private_chests", "locker_chests"):
        collections[table] = list(bounded(cursor, "SELECT id AS chest_id,locker_id,is_public FROM " + table + " ORDER BY id"))
    for table in ("locker_items", "account_locker_items"):
        fields = "id AS item_id," + ("locker_id," if table == "locker_items" else "")
        fields += "chest_id,container_id AS parent_id,obj_uid AS uid,vnum,quantity,weight,extra_flags,"
        fields += "item_type," if table == "locker_items" else ""
        fields += "value0,value1,value2,value3"
        collections[table] = list(bounded(cursor, "SELECT " + fields + " FROM " + table + " ORDER BY id"))
    collections["locker_custody_coverage"] = {name: len(collections[name]) for name in names}
    return collections


def read_siege_custody(cursor) -> dict:
    """Retain every legacy siege row without claiming runtime admission."""
    cursor.execute("SELECT COUNT(*) AS rows_total FROM siege_items")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("siege custody source exceeds audit bounds")
    rows = list(bounded(cursor, "SELECT id AS item_id,room_vnum,container_id AS parent_id,"
                        "obj_uid AS uid,vnum,quantity,weight,extra_flags,item_type,"
                        "value0,value1,value2,value3 FROM siege_items ORDER BY id"))
    return {"siege_items": rows, "siege_custody_coverage": dict(items=len(rows))}


SAVED_PAYLOAD_COLUMNS = {
    "saved_items": "id,item_key,room_vnum,vnum,container_id,quantity,weight,cost,timer,extra_flags,wear_flags,"
        "item_type,value0,value1,value2,value3,value4,value5,value6,value7,name,short_descr,description,"
        "action_descr,obj_uid,created_at,updated_at,item_material,bitvector1,bitvector2,bitvector3,bitvector4,bitvector5",
    "saved_item_affects": "id,item_id,location,modifier",
    "saved_item_extra_descr": "id,item_id,keyword,description"}
MAX_SAVED_CELL_BYTES = 1024 * 1024


def read_saved_ground_payloads(cursor, items: list[dict]) -> dict:
    """Hash complete native-format rows independently, without exporting text.

    Native SELECT-* digests depend on schema order and UTC timestamp rendering.
    Read metadata without joins so orphans remain visible. Bound every raw cell
    and the aggregate framed bytes before buffering any complete text values.
    """
    cursor.execute("SELECT TABLE_NAME AS table_name,COLUMN_NAME AS column_name,"
        "CHARACTER_SET_NAME AS charset FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND table_name IN "
        "('saved_items','saved_item_affects','saved_item_extra_descr') ORDER BY table_name,ORDINAL_POSITION")
    schema = {name: [] for name in SAVED_PAYLOAD_COLUMNS}
    for row in cursor.fetchall():
        if row["charset"] not in (None, "utf8mb4"):
            raise ExportError("saved ground payload charset is unsupported")
        schema[row["table_name"]].append(row["column_name"])
    if any(schema[name] != columns.split(",") for name, columns in SAVED_PAYLOAD_COLUMNS.items()):
        raise ExportError("saved ground payload schema order is unsupported")
    # Native timestamps are rendered in UTC. A borrowed audit session must not
    # change settings; reject incompatible formatting rather than misdiagnose.
    cursor.execute("SELECT @@session.time_zone AS time_zone,@@system_time_zone AS system_time_zone")
    zone = cursor.fetchone()
    if zone is None or not (zone["time_zone"] == "+00:00" or
            zone["time_zone"] == "SYSTEM" and zone["system_time_zone"] in ("UTC", "GMT")):
        raise ExportError("saved ground payload capture requires a UTC session")
    owners = {row["item_id"]: row["key_group"] for row in items}
    counts = {group: [size, 0, 0] for group, size in Counter(owners.values()).items()}
    orphans, metadata = [], {}
    for ordinal, name in enumerate(SAVED_PAYLOAD_COLUMNS):
        if not ordinal:
            continue
        metadata[name] = bounded(cursor, "SELECT id,item_id FROM " + name + " ORDER BY id")
        for row in metadata[name]:
            group = owners.get(row["item_id"])
            if group is None:
                orphans.append(dict(table=name, row_id=row["id"], item_id=row["item_id"]))
            else:
                counts[group][ordinal] += 1
    source_bytes = sum(len(f"{ordinal}:{size}:") for sizes in counts.values()
                       for ordinal, size in enumerate(sizes))
    # Exact N / V<byte-count>:cell framing, including unjoined orphan metadata.
    for name, columns in SAVED_PAYLOAD_COLUMNS.items():
        lengths = ["OCTET_LENGTH(CAST(`" + col + "` AS BINARY))" for col in columns.split(",")]
        frames = [f"IF({size} IS NULL,1,{size}+LENGTH(CAST({size} AS CHAR))+2)" for size in lengths]
        maximum = ",".join(f"COALESCE({size},0)" for size in lengths)
        cursor.execute("SELECT COALESCE(SUM(" + "+".join(frames) + "),0) AS frame_bytes,"
            "COALESCE(MAX(GREATEST(" + maximum + ")),0) AS max_cell FROM " + name)
        bound = cursor.fetchone()
        if bound is None or bound["max_cell"] > MAX_SAVED_CELL_BYTES:
            raise ExportError("saved ground payload cell exceeds audit bounds")
        source_bytes += int(bound["frame_bytes"])
        if source_bytes > MAX_INPUT_BYTES:
            raise ExportError("saved ground payload source exceeds audit byte bounds")
    digests = {group: hashlib.sha256() for group in counts}
    for ordinal, (name, columns) in enumerate(SAVED_PAYLOAD_COLUMNS.items()):
        for group, sizes in counts.items():
            digests[group].update(f"{ordinal}:{sizes[ordinal]}:".encode("ascii"))
        select = ",".join("CAST(`"+col+"` AS BINARY) AS `"+col+"`" for col in columns.split(","))
        for row in bounded(cursor, "SELECT " + select + " FROM " + name + " ORDER BY id"):
            group = owners.get(int(row["id"] if not ordinal else row["item_id"]))
            if group is None:
                continue
            for value in row.values():
                digests[group].update(b"N" if value is None else b"V"+str(len(value)).encode("ascii")+b":"+value)
    ids = {group: hashlib.sha256() for group in counts}
    for row in sorted(items, key=lambda row: row["item_id"]):
        ids[row["key_group"]].update(f"{row['item_id']},".encode("ascii"))
    payloads = [dict(key_group=group, items=sizes[0], affects=sizes[1], extra_descr=sizes[2],
        id_digest=ids[group].hexdigest().upper(), payload_digest=digests[group].hexdigest().upper())
        for group, sizes in sorted(counts.items())]
    state = list(bounded(cursor, "SELECT state_id,season_epoch,reset_status FROM season_reset_state ORDER BY state_id"))
    return dict(saved_ground_payloads=payloads, saved_ground_metadata_orphans=orphans,
        saved_ground_recovery_state=state, saved_ground_payload_coverage=dict(version=1,
            groups=len(payloads), affects=len(metadata["saved_item_affects"]),
            extra_descr=len(metadata["saved_item_extra_descr"]), orphans=len(orphans), source_bytes=source_bytes))


def read_saved_ground_custody(cursor) -> dict:
    """Retain raw ground rows and possible handoffs, without exporting key text."""
    cursor.execute("SELECT (SELECT COUNT(*) FROM saved_items)+"
                   "(SELECT COUNT(*) FROM saved_item_recovery_handoff)+"
                   "(SELECT COUNT(*) FROM sql_room_item_payload)+(SELECT COUNT(*) FROM saved_item_affects)+"
                   "(SELECT COUNT(*) FROM saved_item_extra_descr)+(SELECT COUNT(*) FROM season_reset_state) AS rows_total")
    bound = cursor.fetchone()
    if bound is None or bound["rows_total"] > MAX_ROWS:
        raise ExportError("saved ground custody source exceeds audit bounds")
    # Key equality follows the loader's SQL collation, including case/accent and
    # trailing-space aliases. The minimum physical ID is an opaque per-cut group,
    # never a durable identity or a hash of potentially identifying key text.
    rows = list(bounded(cursor, "SELECT id AS item_id,"
        "MIN(id) OVER (PARTITION BY item_key) AS key_group,room_vnum,"
        "container_id AS parent_id,obj_uid AS uid,vnum,quantity,weight,extra_flags,item_type,"
        "value0,value1,value2,value3,"
        "(EXISTS(SELECT 1 FROM sql_room_item_payload p WHERE p.item_uid=s.obj_uid) OR "
        "EXISTS(SELECT 1 FROM item_ownership_ledger l "
        "JOIN critical_operation_inbox i ON i.operation_id=l.operation_id "
        "JOIN economic_accounting_operation o ON o.operation_id=l.operation_id "
        "WHERE l.item_uid=s.obj_uid AND l.from_owner_type=1 AND l.from_owner_id>0 "
        "AND l.from_owner_context_id=0 AND l.to_owner_type=3 AND l.to_owner_context_id=0 "
        "AND l.reason_type=6 AND l.reason_id=l.to_owner_id AND i.command_type=5 "
        "AND i.schema_version=2 AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 "
        "AND o.outcome=1 AND o.result_code=0) OR COALESCE(s.obj_uid=18446744073709551615,0) OR "
        "EXISTS(SELECT 1 FROM item_current_owner own WHERE own.item_uid=s.obj_uid AND own.coin_payload IS NOT NULL) OR "
        "EXISTS(SELECT 1 FROM economic_accounting_item_reference r "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=r.operation_id "
        "WHERE r.item_uid=s.obj_uid AND ((o.writer_id=5 AND o.reason=3) OR i.command_type=17)) OR "
        "EXISTS(SELECT 1 FROM economic_accounting_account_effect e "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=e.operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=e.operation_id "
        "WHERE OCTET_LENGTH(e.account_key)=40 AND SUBSTRING(e.account_key,17)="
        "CONCAT(X'01000300',REVERSE(UNHEX(LPAD(HEX(s.obj_uid),16,'0'))),REPEAT(CHAR(0),12)) "
        "AND ((o.writer_id=5 AND o.reason=3) OR i.command_type=17)) OR "
        "EXISTS(SELECT 1 FROM item_ownership_ledger l "
        "JOIN economic_accounting_child c ON c.child_operation_id=l.operation_id "
        "WHERE l.item_uid=s.obj_uid AND c.domain_id=1129269582)) AS modern_history "
        "FROM saved_items s ORDER BY id"))
    receipts = list(bounded(cursor, "SELECT season_epoch,source_root_id,source_uid,source_room_vnum,"
        "source_row_count,destination_root_id,(retired_at IS NOT NULL) AS retired,"
        "(BINARY destination_key=BINARY CONCAT('item.uid.',source_uid)) AS destination_key_canonical,"
        "HEX(source_id_digest) AS source_id_digest,HEX(source_payload_digest) AS source_payload_digest,"
        "HEX(destination_payload_digest) AS destination_payload_digest,"
        "(SELECT MIN(s.id) FROM saved_items s WHERE s.item_key=h.source_key) AS source_group,"
        "(SELECT MIN(s.id) FROM saved_items s WHERE s.item_key=h.destination_key) AS destination_group "
        "FROM saved_item_recovery_handoff h ORDER BY season_epoch,source_root_id"))
    return {"saved_ground_items": rows, "saved_ground_handoffs": receipts,
            "saved_ground_custody_coverage": dict(items=len(rows), handoffs=len(receipts)),
            **read_saved_ground_payloads(cursor, rows)}


ROOM_CANDIDATES_SQL = (
    "SELECT DISTINCT own.root_item_uid AS root FROM item_current_owner own "
    "JOIN season_reset_state season ON season.state_id=1 AND season.reset_status='active' AND season.season_epoch>0 "
    "WHERE own.owner_type=3 AND own.state=1 AND (EXISTS(SELECT 1 FROM sql_room_item_payload p "
    "WHERE p.item_uid=own.item_uid AND p.season_epoch=season.season_epoch) OR "
    "(NOT EXISTS(SELECT 1 FROM sql_room_item_payload history WHERE history.item_uid=own.item_uid) AND "
    "EXISTS(SELECT 1 FROM item_ownership_ledger l JOIN critical_operation_inbox i ON i.operation_id=l.operation_id "
    "JOIN economic_accounting_operation o ON o.operation_id=l.operation_id WHERE l.item_uid=own.item_uid "
    "AND l.root_item_uid=own.root_item_uid AND l.to_owner_type=3 AND l.to_owner_id=own.owner_id "
    "AND l.to_owner_context_id=0 AND l.from_owner_type=1 AND l.from_owner_id>0 AND l.from_owner_context_id=0 "
    "AND l.reason_type=6 AND l.reason_id=own.owner_id AND i.command_type=5 AND i.schema_version=2 "
    "AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 AND o.outcome=1 AND o.result_code=0)))")
ROOM_PROOF_FROM = (
    " FROM sql_room_item_payload p LEFT JOIN economic_accounting_item_reference r "
    "ON r.operation_id=p.operation_id AND r.item_uid=p.item_uid "
    "LEFT JOIN item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index "
    "LEFT JOIN critical_operation_inbox i ON i.operation_id=p.operation_id "
    "LEFT JOIN economic_accounting_operation o ON o.operation_id=p.operation_id")


def read_room_item_custody(cursor, *, select_rows=None) -> dict:
    """Borrow one consistent read-only cut; keep missing bindings and descendants."""
    def select(query, columns, binary=()):
        if select_rows is not None:
            return select_rows(query, columns, binary)
        return list(bounded(cursor, query))

    members_from = " FROM item_current_owner own WHERE own.root_item_uid IN (" + ROOM_CANDIDATES_SQL + ")"
    preflight = ("SELECT (SELECT COUNT(*) FROM sql_room_item_payload)+"
        "(SELECT COUNT(*)" + ROOM_PROOF_FROM + ")+"
        "(SELECT COUNT(*) FROM (" + ROOM_CANDIDATES_SQL + ") candidates)+"
        "(SELECT COUNT(*)" + members_from + ")+"
        "(SELECT COUNT(*) FROM season_reset_state) AS rows_total,"
        "(SELECT COALESCE(SUM(OCTET_LENGTH(payload)),0) FROM sql_room_item_payload) AS payload_bytes,"
        "(SELECT COALESCE(MAX(OCTET_LENGTH(payload)),0) FROM sql_room_item_payload) AS max_payload_bytes")
    if select_rows is None:
        cursor.execute(preflight)
        bounds = cursor.fetchone()
    else:
        values = select(preflight, ("rows_total", "payload_bytes", "max_payload_bytes"))
        bounds = values[0] if len(values) == 1 else None
    if (bounds is None or bounds["rows_total"] > MAX_ROWS or
            bounds["payload_bytes"] > MAX_INPUT_BYTES//2 or bounds["max_payload_bytes"] > 131072):
        raise ExportError("room item sources exceed audit bounds")
    payloads = select("SELECT item_uid AS uid,item_revision AS revision,payload_version,"
        "operation_id,season_epoch,OCTET_LENGTH(payload) AS payload_bytes,SUBSTRING(payload,1,131073) AS payload "
        "FROM sql_room_item_payload ORDER BY item_uid,item_revision",
        ("uid", "revision", "payload_version", "operation_id", "season_epoch", "payload_bytes", "payload"),
        ("operation_id", "payload"))
    for row in payloads:
        if not isinstance(row["payload"], bytes) or len(row["payload"]) != row["payload_bytes"]:
            raise ExportError("invalid room item payload source")
        row["payload"] = row["payload"].hex()
        row["operation_id"] = hex_id(row["operation_id"])
    operations = ("reference_operation", "legacy_operation", "ledger_operation", "inbox_operation", "root_operation")
    proofs = select("SELECT p.item_uid AS uid,p.item_revision AS revision,"
        "r.operation_id AS reference_operation,r.item_uid AS reference_uid,r.before_revision,r.after_revision,r.child_index,"
        "r.legacy_operation_id AS legacy_operation,l.operation_id AS ledger_operation,l.item_uid AS ledger_uid,"
        "l.root_item_uid AS ledger_root,l.parent_item_uid AS ledger_parent,l.item_revision AS ledger_revision,"
        "l.from_owner_type AS from_type,l.from_owner_id AS from_id,l.from_owner_context_id AS from_context,"
        "l.to_owner_type AS to_type,l.to_owner_id AS to_id,l.to_owner_context_id AS to_context,l.reason_type,l.reason_id,"
        "i.operation_id AS inbox_operation,i.command_type,i.schema_version,i.status,i.result_code,i.failure_stage,"
        "o.operation_id AS root_operation,o.outcome,o.result_code AS operation_result" + ROOM_PROOF_FROM +
        " ORDER BY p.item_uid,p.item_revision,r.event_index", ("uid", "revision", *operations, *PROOF_NUMBERS), operations)
    for row in proofs:
        for field in operations:
            row[field] = hex_id(row[field])
    roots = select(ROOM_CANDIDATES_SQL + " ORDER BY own.root_item_uid", ("root",))
    members = select("SELECT own.item_uid AS uid,own.root_item_uid AS root,own.parent_item_uid AS parent,"
        "own.owner_type,own.owner_id,own.owner_context_id AS owner_context,own.item_revision AS revision,"
        "own.vnum,own.state,own.equipment_slot,"
        "(SELECT revision FROM item_owner_revision rev WHERE rev.owner_type=own.owner_type AND "
        "rev.owner_id=own.owner_id AND rev.owner_context_id=own.owner_context_id) AS owner_revision,"
        "(SELECT COUNT(*) FROM saved_items saved WHERE saved.obj_uid=own.item_uid) AS saved_duplicates" +
        members_from + " ORDER BY own.root_item_uid,own.item_uid",
        ("uid", "root", "parent", "owner_type", "owner_id", "owner_context", "revision", "vnum", "state",
         "equipment_slot", "owner_revision", "saved_duplicates"))
    seasons = select("SELECT state_id,season_epoch,reset_status FROM season_reset_state ORDER BY state_id",
                     ("state_id", "season_epoch", "reset_status"))
    return dict(room_item_payloads=payloads, room_item_proofs=proofs, room_item_roots=roots,
        room_item_members=members, room_item_seasons=seasons, room_item_custody_coverage=dict(version=1,
            payloads=len(payloads), proofs=len(proofs), roots=len(roots), members=len(members), seasons=len(seasons),
            payload_bytes=int(bounds["payload_bytes"])))


def read_native(cursor, lineage: bytes) -> tuple[dict, list[str], dict]:
    # Both native-item and mapping projections can return payload bytes. Bound
    # them before either buffered SELECT, including repeated mapping joins.
    cursor.execute("SELECT COUNT(*) AS row_count,"
                   "COALESCE(SUM(OCTET_LENGTH(coin_payload)),0) AS payload_bytes,"
                   "COALESCE(MAX(OCTET_LENGTH(coin_payload)),0) AS max_payload_bytes "
                   "FROM item_current_owner WHERE " + coin_source_sql())
    coin_bounds = cursor.fetchone()
    if (coin_bounds is None or coin_bounds["row_count"] > MAX_ROWS or
            coin_bounds["payload_bytes"] > MAX_INPUT_BYTES or
            coin_bounds["max_payload_bytes"] > MAX_ITEM_PAYLOAD_BYTES):
        raise ExportError("coin-pile source exceeds audit bounds")
    cursor.execute("SELECT COALESCE(SUM(OCTET_LENGTH(i.coin_payload)),0) AS payload_bytes "
                   "FROM economic_account_mapping m JOIN item_current_owner i "
                   "ON m.account_kind=3 AND i.item_uid=m.active_native_id AND " + coin_source_sql("i") +
                   " WHERE m.lineage=%s AND m.backend_kind=1", (lineage,))
    mapping_bounds = cursor.fetchone()
    if mapping_bounds is None or mapping_bounds["payload_bytes"] > MAX_INPUT_BYTES:
        raise ExportError("mapped coin-pile source exceeds audit bounds")
    native = {"holdings": [], "items": [], "coin_piles": [],
              "coin_pile_mappings": [], "pending_claim_sources": []}
    native.update(read_auction_custody(cursor))
    native.update(read_shop_custody(cursor))
    native.update(read_player_custody(cursor))
    native.update(read_corpse_custody(cursor))
    native.update(read_locker_custody(cursor))
    native.update(read_siege_custody(cursor))
    native.update(read_saved_ground_custody(cursor))
    native.update(read_room_item_custody(cursor))
    native["ship_coffers"], native["ship_coffer_coverage"] = read_ship_coffers(cursor)
    native["guild_treasuries"], native["guild_treasury_coverage"] = read_guild_treasuries(cursor)
    gaps = ["ship_coffer_lifetime_origin_revision_and_writer_qualification",
            "guild_treasury_lifetime_origin_money_revision_and_writer_qualification",
            "coin_pile_creation_origin_and_lifecycle_source_completeness",
            "escrow_claim_treasury_lifecycle_and_origin_reconciliation",
            "pending_claim_consumer_completeness_and_legacy_coverage",
            "auction_template_prototype_coin_literals_history_and_legacy_identity",
            "shop_prototype_runtime_payload_coin_literals_and_enrollment_history",
            "player_pet_prototype_full_runtime_payload_hold_and_retained_death_history",
            "corpse_prototype_full_payload_catalog_revision_artifact_and_lifecycle_history",
            "locker_prototype_full_payload_access_history_and_account_runtime_authority",
            "siege_runtime_admission_full_payload_and_other_room_physical_authority",
            "saved_ground_prototype_full_payload_handoff_and_modern_runtime_authority",
            "unattributed_ownership_history",
            "unreferenced_uid_events_without_native_or_baseline_anchors",
            "unresolved_post_baseline_account_origins",
            "realized_domain_prices"]
    mappings = bounded(cursor,
        "SELECT m.mapping_id,m.account_kind,m.locator_kind,m.context_id,m.native_id,"
        "m.active_native_id,"
        "p.pid AS wallet_id,b.id AS bank_id,"
        "p.copper AS wallet_copper,p.silver AS wallet_silver,p.gold AS wallet_gold,"
        "p.platinum AS wallet_platinum,p.wallet_revision,"
        "b.bank_copper,b.bank_silver,b.bank_gold,b.bank_platinum,b.bank_revision,"
        "i.item_uid AS pile_uid,i.vnum AS pile_vnum,i.state AS pile_state,"
        "i.item_revision AS pile_revision,"
        f"CASE WHEN {coin_source_sql('i')} THEN i.coin_payload ELSE NULL END AS pile_payload,"
        "a.id AS escrow_id,a.status AS escrow_status,a.winning_bidder_pid AS escrow_winner_pid,"
        "a.cur_price AS escrow_copper,"
        "a.auction_revision AS escrow_revision,"
        "c.pid AS claim_row_id,c.money AS claim_copper,c.claim_revision,"
        "cp.pid AS claim_player_id,s.id AS treasury_id,s.cash AS treasury_copper,"
        "s.shop_revision AS treasury_revision "
        "FROM economic_account_mapping m "
        "LEFT JOIN player_data p ON m.account_kind=1 AND p.pid=m.active_native_id "
        "LEFT JOIN account_banks b ON m.account_kind=2 AND b.id=m.active_native_id "
        "LEFT JOIN item_current_owner i ON m.account_kind=3 AND i.item_uid=m.active_native_id "
        "LEFT JOIN auctions a ON m.account_kind=4 AND a.id=m.active_native_id "
        "LEFT JOIN auction_money_pickups c ON m.account_kind=5 AND c.pid=m.active_native_id "
        "LEFT JOIN player_data cp ON m.account_kind=5 AND cp.pid=m.active_native_id "
        "LEFT JOIN shopkeepers s ON m.account_kind=6 AND s.id=m.active_native_id "
        "WHERE m.lineage=%s AND m.backend_kind=1 ORDER BY m.mapping_id", (lineage,))
    kinds = ((1, "wallet"), (2, "bank"), (3, "pile"),
             (4, "auction_escrow"), (5, "pending_claim"), (6, "treasury"))
    coverage = {f"{prefix}_rows": 0 for _, prefix in kinds}
    for _, prefix in kinds:
        coverage.update({f"unmapped_{prefix}_rows": 0,
                         f"multiply_mapped_{prefix}_rows": 0,
                         f"dangling_{prefix}_mappings": 0,
                         f"invalid_{prefix}_mappings": 0,
                         f"invalid_{prefix}_rows": 0})
    coverage["invalid_unknown_kind_mappings"] = 0
    active_ids = Counter((row["account_kind"], row["active_native_id"])
                         for row in mappings if row["active_native_id"] is not None)
    existing_ids = set()
    for row in mappings:
        kind = row["account_kind"]
        if kind not in range(1, 7):
            coverage["invalid_unknown_kind_mappings"] += 1
            continue
        prefix = dict(kinds)[kind]
        if not native_mapping_identity_valid(
                kind, row["locator_kind"], row["native_id"], row["active_native_id"]):
            coverage[f"invalid_{prefix}_mappings"] += 1
            continue
        if row["active_native_id"] is None:
            continue
        mapping_entry = None
        if kind == 3:
            mapping_entry = {"account_key": account_key(
                lineage, kind, row["mapping_id"], row["context_id"]),
                "uid": row["active_native_id"],
                "item_exists": False, "holding_valid": False}
            native["coin_pile_mappings"].append(mapping_entry)
        exists = {
            1: row["wallet_id"] is not None,
            2: row["bank_id"] is not None,
            3: row["pile_uid"] is not None and row["pile_state"] == 1 and
               (row["pile_vnum"] == COIN_VNUM or row["pile_payload"] is not None),
            4: row["escrow_id"] is not None and auction_escrow_mapping_is_live(
                row["escrow_status"], row["escrow_winner_pid"]),
            5: row["claim_row_id"] is not None or row["claim_player_id"] is not None,
            6: row["treasury_id"] is not None,
        }[kind]
        if not exists:
            coverage[f"dangling_{prefix}_mappings"] += 1
            continue
        if mapping_entry is not None:
            mapping_entry["item_exists"] = True
        existing_ids.add((kind, row["active_native_id"]))
        balance = {
            1: [row[f"wallet_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            2: [row[f"bank_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            3: decode_coin_payload(row["pile_payload"], row["pile_uid"], row["pile_vnum"])
               if row["pile_payload"] is not None else None,
            4: auction_escrow_balance(row["escrow_status"], row["escrow_copper"],
                                      row["escrow_winner_pid"]),
            5: [row["claim_copper"] if row["claim_row_id"] is not None else 0, 0, 0, 0],
            6: [row["treasury_copper"], 0, 0, 0],
        }[kind]
        revision = {1: row["wallet_revision"], 2: row["bank_revision"],
                    3: row["pile_revision"], 4: row["escrow_revision"],
                    5: row["claim_revision"] if row["claim_row_id"] is not None else 0,
                    6: row["treasury_revision"]}[kind]
        if balance is None or any(amount is None for amount in balance) or revision is None:
            coverage[f"invalid_{prefix}_rows"] += 1
            continue
        if mapping_entry is not None:
            mapping_entry["holding_valid"] = True
            mapping_entry["balance"] = balance
            mapping_entry["revision"] = revision
        native["holdings"].append({"account_key": account_key(lineage, kind, row["mapping_id"],
                                                              row["context_id"]),
                                   "balance": balance, "revision": revision})
    for kind, prefix, table, locator, predicate in (
            (1, "wallet", "player_data", "pid", "1=1"),
            (2, "bank", "account_banks", "id", "1=1"),
            (3, "pile", "item_current_owner", "item_uid",
             coin_source_sql() + " AND state=1"),
            (4, "auction_escrow", "auctions", "id",
             "status='OPEN' OR (status='REMOVED' AND winning_bidder_pid<>0)"),
            (5, "pending_claim", "auction_money_pickups", "pid", "1=1"),
            (6, "treasury", "shopkeepers", "id", "cash IS NOT NULL")):
        if kind in (1, 2):
            total, unmapped = native_source_count(cursor, lineage, table, locator, kind)
        else:
            total, unmapped = native_source_count(cursor, lineage, table, locator, kind,
                                                 predicate)
        coverage[f"{prefix}_rows"] = total
        coverage[f"unmapped_{prefix}_rows"] = unmapped
        coverage[f"multiply_mapped_{prefix}_rows"] = sum(
            count > 1 for (account_kind, native_id), count in active_ids.items()
            if account_kind == kind and (account_kind, native_id) in existing_ids)
    claim_sources = bounded(cursor,
        "SELECT s.source_operation_id,s.source_slot,s.claim_mapping_id,s.beneficiary_pid,"
        "s.amount,s.claim_operation_id,m.mapping_id AS mapped_id,m.context_id,"
        "m.native_id AS mapped_native_id,m.active_native_id "
        "AS mapped_active_native_id FROM economic_pending_claim_source s "
        "LEFT JOIN economic_account_mapping m ON m.mapping_id=s.claim_mapping_id "
        "AND m.lineage=s.lineage AND m.backend_kind=1 AND m.account_kind=5 "
        "AND m.locator_kind=5 "
        "WHERE s.lineage=%s ORDER BY s.source_operation_id,s.source_slot", (lineage,))
    native["pending_claim_consumptions"], native["pending_claim_consumption_coverage"] = (
        read_pending_claim_consumptions(cursor, lineage))
    partial_amounts = Counter()
    for row in native["pending_claim_consumptions"]:
        partial_amounts[(bytes.fromhex(row["source_operation_id"]), row["source_slot"])] += row["amount"]
    claim_source_coverage = {"rows": len(claim_sources), "open_rows": 0,
                             "consumed_rows": 0, "invalid_account_mappings": 0,
                             "invalid_source_roots": 0, "invalid_consumer_roots": 0}
    source_pairs = set()
    source_keys = {}
    consumer_keys = {}
    consumer_amounts = Counter()
    source_amounts = Counter()
    for row in claim_sources:
        if row["mapped_id"] is not None:
            key = account_key(lineage, 5, row["claim_mapping_id"], row["context_id"])
            pair = (row["source_operation_id"], bytes.fromhex(key))
            source_pairs.add(pair)
            source_amounts[pair] += row["amount"]
            source_keys[(row["source_operation_id"], row["source_slot"])] = bytes.fromhex(key)
            if row["claim_operation_id"] is not None:
                consumer_pair = (row["claim_operation_id"], bytes.fromhex(key))
                source_pairs.add(consumer_pair)
                consumer_keys[(row["source_operation_id"], row["source_slot"])] = bytes.fromhex(key)
                consumer_amounts[consumer_pair] += row["amount"]
    source_root_metadata = {}
    source_root_effects = Counter()
    source_pair_values = {}
    source_root_postings = {}
    source_pairs = sorted(source_pairs)
    for offset in range(0, len(source_pairs), 64):
        batch = source_pairs[offset:offset + 64]
        batch_pairs = set(batch)
        operation_ids = tuple(dict.fromkeys(pair[0] for pair in batch))
        account_keys = tuple(dict.fromkeys(pair[1] for pair in batch))
        operation_placeholders = ",".join("%s" for _ in operation_ids)
        key_placeholders = ",".join("%s" for _ in account_keys)
        operations = bounded(cursor,
            "SELECT o.operation_id,o.lineage,o.outcome,o.result_code,o.posting_count,"
            "i.status AS inbox_status,i.result_code AS inbox_result_code,"
            "i.failure_stage AS inbox_failure_stage,"
            "(i.committed_at IS NOT NULL) AS inbox_committed_at_present "
            "FROM economic_accounting_operation o "
            "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
            f"WHERE o.operation_id IN ({operation_placeholders})", operation_ids)
        for operation in operations:
            source_root_metadata[operation["operation_id"]] = {
                **operation,
                "inbox_receipt": {
                    "status": operation["inbox_status"],
                    "result_code": operation["inbox_result_code"],
                    "failure_stage": operation["inbox_failure_stage"],
                    "committed_at_present": bool(operation["inbox_committed_at_present"]),
                },
            }
        posting_audits = bounded(cursor,
            "SELECT operation_id,COUNT(*) AS posting_rows,COALESCE(SUM(copper_value),0) AS net_copper "
            "FROM economic_accounting_coin_posting "
            f"WHERE operation_id IN ({operation_placeholders}) GROUP BY operation_id", operation_ids)
        source_root_postings.update({row["operation_id"]: row for row in posting_audits})
        effects = bounded(cursor,
            "SELECT operation_id,account_key,before_copper,before_silver,before_gold,"
            "before_platinum,after_copper,after_silver,after_gold,after_platinum "
            "FROM economic_accounting_account_effect "
            f"WHERE operation_id IN ({operation_placeholders}) "
            f"AND account_key IN ({key_placeholders})", operation_ids + account_keys)
        for effect in effects:
            effect_key = (effect["operation_id"], effect["account_key"])
            if effect_key not in batch_pairs:
                continue
            source_root_effects[effect_key] += 1
            source_pair_values[effect_key] = effect
    for row in claim_sources:
        if (row["source_operation_id"] is None or row["source_slot"] is None or
                row["beneficiary_pid"] is None or row["amount"] is None or
                row["amount"] <= 0):
            raise ExportError("invalid pending-claim source row")
        remaining = (0 if row["claim_operation_id"] is not None else
                     row["amount"] - partial_amounts[(row["source_operation_id"], row["source_slot"])])
        consumed = remaining <= 0
        claim_source_coverage["consumed_rows" if consumed else "open_rows"] += 1
        mapping_valid = (row["mapped_id"] is not None and
                         row["mapped_native_id"] == row["beneficiary_pid"] and
                         (remaining <= 0 or
                          row["mapped_active_native_id"] == row["beneficiary_pid"]))
        if not mapping_valid:
            claim_source_coverage["invalid_account_mappings"] += 1
        source_key = source_keys.get((row["source_operation_id"], row["source_slot"]))
        root = source_root_metadata.get(row["source_operation_id"])
        source_receipt = root["inbox_receipt"] if root is not None else None
        effect_key = ((row["source_operation_id"], source_key)
                      if source_key is not None else None)
        effect = source_pair_values.get(effect_key)
        source_root_valid = bool(
            mapping_valid and root is not None and root["lineage"] == lineage and
            root["outcome"] == 1 and root["result_code"] == 0 and
            source_receipt["status"] == 1 and source_receipt["result_code"] == 0 and
            source_receipt["failure_stage"] == 0 and
            source_receipt["committed_at_present"] and effect is not None and
            source_root_effects[effect_key] == 1 and
            row["source_operation_id"] in source_root_postings and
            source_root_postings[row["source_operation_id"]]["posting_rows"] ==
            root["posting_count"] and
            source_root_postings[row["source_operation_id"]]["net_copper"] == 0 and
            effect["before_copper"] is not None and effect["after_copper"] is not None and
            effect["after_copper"] - effect["before_copper"] == source_amounts[effect_key] and
            all(effect[field] is not None and effect[after] == effect[field]
                for field, after in (("before_silver", "after_silver"),
                                     ("before_gold", "after_gold"),
                                     ("before_platinum", "after_platinum"))))
        if not source_root_valid:
            claim_source_coverage["invalid_source_roots"] += 1
        consumer_root_valid = None
        consumer_receipt = None
        if row["claim_operation_id"] is not None:
            consumer_key = consumer_keys.get((row["source_operation_id"], row["source_slot"]))
            consumer_effect_key = ((row["claim_operation_id"], consumer_key)
                                   if consumer_key is not None else None)
            consumer = source_root_metadata.get(row["claim_operation_id"])
            consumer_receipt = consumer["inbox_receipt"] if consumer is not None else None
            consumer_effect = source_pair_values.get(consumer_effect_key)
            consumer_root_valid = bool(
                mapping_valid and consumer is not None and consumer["lineage"] == lineage and
                consumer["outcome"] == 1 and consumer["result_code"] == 0 and
                consumer_receipt["status"] == 1 and consumer_receipt["result_code"] == 0 and
                consumer_receipt["failure_stage"] == 0 and
                consumer_receipt["committed_at_present"] and consumer_effect is not None and
                source_root_effects[consumer_effect_key] == 1 and
                row["claim_operation_id"] in source_root_postings and
                source_root_postings[row["claim_operation_id"]]["posting_rows"] ==
                consumer["posting_count"] and
                source_root_postings[row["claim_operation_id"]]["net_copper"] == 0 and
                consumer_effect["before_copper"] is not None and
                consumer_effect["after_copper"] is not None and
                consumer_effect["after_copper"] - consumer_effect["before_copper"] ==
                -consumer_amounts[consumer_effect_key] and
                all(consumer_effect[field] is not None and consumer_effect[after] == consumer_effect[field]
                    for field, after in (("before_silver", "after_silver"),
                                         ("before_gold", "after_gold"),
                                         ("before_platinum", "after_platinum"))))
            if not consumer_root_valid:
                claim_source_coverage["invalid_consumer_roots"] += 1
        native["pending_claim_sources"].append({
            "source_operation_id": hex_id(row["source_operation_id"]),
            "source_slot": row["source_slot"],
            "account_key": (account_key(lineage, 5, row["claim_mapping_id"], row["context_id"])
                            if row["mapped_id"] is not None else None),
            "beneficiary_pid": row["beneficiary_pid"], "amount": row["amount"],
            "mapping_native_id": row["mapped_native_id"],
            "mapping_active_native_id": row["mapped_active_native_id"],
            "mapping_valid": mapping_valid, "source_root_valid": source_root_valid,
            "source_inbox_receipt": source_receipt,
            "claim_operation_id": hex_id(row["claim_operation_id"]),
            "consumer_root_valid": consumer_root_valid,
            "consumer_inbox_receipt": consumer_receipt})
    native["pending_claim_source_coverage"] = claim_source_coverage
    items = bounded(cursor,
        "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
        "owner_context_id,item_revision,state,equipment_slot,vnum,"
        f"CASE WHEN {coin_source_sql()} THEN coin_payload ELSE NULL END AS coin_payload "
        "FROM item_current_owner ORDER BY item_uid")
    coin_payload_rows = 0
    missing_coin_payload_rows = 0
    for row in items:
        if row["state"] not in ITEM_STATES:
            raise ExportError("invalid native item state")
        native["items"].append({"uid": row["item_uid"], "root": row["root_item_uid"],
                                "parent": row["parent_item_uid"],
                                "owner": [row["owner_type"], row["owner_id"],
                                          row["owner_context_id"]],
                                "revision": row["item_revision"],
                                "vnum": row["vnum"],
                                "state": ITEM_STATES[row["state"]],
                                "equipment_slot": row["equipment_slot"]})
        if row["vnum"] == COIN_VNUM or row["coin_payload"] is not None:
            blob = row["coin_payload"]
            if blob is None:
                amounts = None
                missing_coin_payload_rows += 1
            else:
                amounts = decode_coin_payload(blob, row["item_uid"], row["vnum"])
                coin_payload_rows += 1
            native["coin_piles"].append({
                "uid": row["item_uid"],
                "owner": [row["owner_type"], row["owner_id"], row["owner_context_id"]],
                "revision": row["item_revision"],
                "state": ITEM_STATES[row["state"]],
                "amounts": amounts})
    mapped_pile_counts = Counter(
        mapping["uid"] for mapping in native["coin_pile_mappings"]
        if mapping["item_exists"])
    live_coin_piles = [pile for pile in native["coin_piles"] if pile["state"] == "live"]
    native["coin_pile_coverage"] = {
        "rows": int(coin_bounds["row_count"]),
        "payload_rows": coin_payload_rows,
        "missing_payload_rows": missing_coin_payload_rows,
        "mapped_live_rows": sum(bool(mapped_pile_counts[pile["uid"]])
                                 for pile in live_coin_piles),
        "unmapped_live_rows": sum(not mapped_pile_counts[pile["uid"]]
                                   for pile in live_coin_piles),
        "multiply_mapped_live_rows": sum(
            max(0, mapped_pile_counts[pile["uid"]] - 1) for pile in live_coin_piles),
        "dangling_mappings": coverage["dangling_pile_mappings"],
        "invalid_mappings": coverage["invalid_pile_rows"]}
    return native, gaps, coverage


def capture(connection, lineage: bytes, epoch: bytes) -> dict:
    identity(lineage, "lineage")
    identity(epoch, "epoch")
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        origins = read_origins_in_transaction(cursor, lineage, epoch)
        cursor.execute(
            "SELECT TABLE_NAME AS table_name,ENGINE AS engine FROM information_schema.tables "
            "WHERE table_schema=DATABASE() AND table_name IN "
            "('economic_accounting_account_effect','economic_accounting_coin_posting',"
            "'economic_accounting_child','economic_accounting_item_reference',"
            "'economic_accounting_source_claim','economic_account_mapping',"
            "'economic_pending_claim_source','economic_pending_claim_consumption','economic_sql_lifecycle_installation',"
            "'critical_operation_inbox','player_data',"
            "'account_banks','item_current_owner','item_ownership_ledger','auctions',"
            "'auction_money_pickups','auction_item_custody','auction_item_pickups',"
            "'shopkeepers','shopkeeper_items','player_items','player_pets','player_pet_items',"
            "'corpses','corpse_items','lockers','private_chests','locker_items',"
            "'account_lockers','locker_chests','account_locker_items','siege_items','ships','guilds',"
            "'saved_items','saved_item_recovery_handoff','sql_room_item_payload',"
            "'saved_item_affects','saved_item_extra_descr','season_reset_state','item_owner_revision')")
        engines = {row["table_name"]: row["engine"] for row in cursor.fetchall()}
        if len(engines) != 41 or any(engine != "InnoDB" for engine in engines.values()):
            raise ExportError("SQL audit source is missing or not InnoDB")
        has_realized_price = realized_price_column_available(cursor)
        evidence = read_evidence(cursor, lineage, epoch, has_realized_price)
        bind_baseline_claim_witnesses(cursor, lineage, epoch, evidence["source_claims"], origins)
        evidence["orphan_evidence"], evidence["orphan_evidence_coverage"] = read_orphan_evidence(cursor)
        lineage_uid_references, lineage_uid_reference_roots, lineage_uid_reference_coverage = (
            read_lineage_uid_references(cursor, lineage))
        native, gaps, coverage = read_native(cursor, lineage)
        mapping_creations, mapping_creation_coverage, mapping_creation_roots = (
            read_mapping_creations(cursor, lineage))
        native["mapping_creations"] = mapping_creations
        native["mapping_creation_coverage"] = mapping_creation_coverage
        native["mapping_creation_roots"] = mapping_creation_roots
        native["baseline_operation_ids"] = origins["baseline_operation_ids"]
        pending_claim_consumers, pending_claim_consumer_coverage = (
            read_pending_claim_consumers(cursor, lineage))
        native["pending_claim_consumers"] = pending_claim_consumers
        native["pending_claim_consumer_coverage"] = pending_claim_consumer_coverage
        lineage_realized_prices, realized_price_coverage = read_lineage_realized_prices(
            cursor, lineage, has_realized_price)
        native["lineage_realized_prices"] = lineage_realized_prices
        native["realized_price_coverage"] = realized_price_coverage
        native["lineage_uid_references"] = lineage_uid_references
        native["lineage_uid_reference_roots"] = lineage_uid_reference_roots
        native["lineage_uid_reference_coverage"] = lineage_uid_reference_coverage
        account_origins = list(origins["account_origins"])
        account_origins.extend(infer_created_account_origins(
            cursor, lineage, evidence["effects"], account_origins, evidence["operations"]))
        account_origins.extend(infer_created_mapping_origins(
            lineage.hex(), mapping_creations, mapping_creation_roots, account_origins))
        retired_mappings, retirement_coverage, retirement_roots = read_mapping_retirements(
            cursor, lineage, epoch, account_origins)
        native["retired_mappings"] = retired_mappings
        native["retirement_coverage"] = retirement_coverage
        native["retirement_roots"] = retirement_roots
        item_origins = list(origins["item_origins"])
        known_item_origins = {row["uid"] for row in item_origins}
        current_operations = {row["operation_id"]: row for row in evidence["operations"]}
        for event in evidence["ownership_events"]:
            operation = current_operations.get(event["operation_id"])
            outcome = operation["outcome"] if operation is not None else "unknown"
            append_committed_item_creation_origin(
                item_origins, known_item_origins, event["uid"], event["action"],
                event["before_revision"], outcome)
        for ref in lineage_uid_references:
            append_committed_item_creation_origin(
                item_origins, known_item_origins, ref["uid"], ref["ledger_action"],
                ref["ledger_before_revision"], ref["operation_outcome"])
        (uid_history_events, unreferenced_uid_events, uid_event_coverage,
         unanchored_ownership_uids, ambiguous_lineage_ownership_uids,
         uid_scope_coverage, unattributed_uid_events,
         unattributed_uid_event_coverage) = read_uid_event_census(
            cursor, lineage, item_origins, evidence, native["items"], lineage_uid_references)
        for event in uid_history_events:
            append_committed_item_creation_origin(
                item_origins, known_item_origins, event["uid"], event["action"],
                event["before_revision"], event["operation_outcome"])
        unanchored_ownership_uids = [
            uid for uid in unanchored_ownership_uids if uid not in known_item_origins]
        uid_scope_coverage["unanchored_ownership_uid_count"] = len(
            unanchored_ownership_uids)
        uid_scope_coverage["anchored_ownership_uid_count"] = (
            uid_scope_coverage["ownership_uid_count"] -
            uid_scope_coverage["unanchored_ownership_uid_count"])
        native["uid_history_events"] = uid_history_events
        native["unreferenced_uid_events"] = unreferenced_uid_events
        native["uid_event_coverage"] = uid_event_coverage
        native["unanchored_ownership_uids"] = unanchored_ownership_uids
        native["ambiguous_lineage_ownership_uids"] = ambiguous_lineage_ownership_uids
        native["uid_scope_coverage"] = uid_scope_coverage
        native["unattributed_uid_events"] = unattributed_uid_events
        native["unattributed_uid_event_coverage"] = unattributed_uid_event_coverage
        result = {"schema_version": 1, "lineage": lineage.hex(), "epoch": epoch.hex(),
                  "backend": "sql_partial", "complete": False, "quiescent": True,
                  "capture_gaps": gaps, "native": native,
                  "native_mapping_coverage": coverage,
                  **evidence,
                  "account_origins": account_origins,
                  "item_origins": item_origins}
        encoded = json.dumps(result, sort_keys=True, separators=(",", ":")).encode()
        if len(encoded) > MAX_INPUT_BYTES:
            raise ExportError("SQL audit snapshot exceeds input limit")
        return result
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=3306)
    parser.add_argument("--socket", help="explicit local SQL Unix socket")
    parser.add_argument("--user", required=True)
    parser.add_argument("--database", required=True)
    parser.add_argument("--password-env", default="DB_PASSWORD")
    parser.add_argument("--lineage", required=True)
    parser.add_argument("--epoch", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        lineage, epoch = bytes.fromhex(args.lineage), bytes.fromhex(args.epoch)
        identity(lineage, "lineage")
        identity(epoch, "epoch")
        if not 1 <= args.port <= 65535:
            raise ExportError("invalid SQL port")
        password = os.environ[args.password_env]
        import pymysql
        try:
            connection = pymysql.connect(host=args.host, port=args.port, user=args.user,
                                         unix_socket=args.socket,
                                         password=password, database=args.database,
                                         charset="utf8mb4", autocommit=True,
                                         init_command="SET SESSION time_zone='+00:00'",
                                         cursorclass=pymysql.cursors.DictCursor,
                                         connect_timeout=5, read_timeout=30, write_timeout=5)
            try:
                snapshot = capture(connection, lineage, epoch)
            finally:
                connection.close()
        except pymysql.MySQLError as error:
            raise ExportError(f"SQL read failed with code {error.args[0]}") from error
        encoded = (json.dumps(snapshot, sort_keys=True, separators=(",", ":")) + "\n").encode()
        descriptor = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "wb") as output:
            output.write(encoded)
        return 0
    except (OriginError, ExportError, OSError, ValueError, KeyError, TypeError,
            ImportError, struct.error) as error:
        print(f"SQL partial audit export refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
