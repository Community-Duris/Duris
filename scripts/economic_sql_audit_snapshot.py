#!/usr/bin/env python3
"""Export a bounded, explicitly incomplete SQL accounting audit cut.

The same read-only transaction captures baseline origins, retained accounting
rows, mapped wallet/bank balances, and current UID positions. The result is
diagnostic input for the reconciler. It always says complete=false because the
remaining native money classes and unreferenced UID history are not proven.
"""

from __future__ import annotations

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import struct
import sys

from economic_sql_audit_origins import ITEM_STATES, OriginError, identity, read_origins_in_transaction
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, TABLES


class ExportError(ValueError):
    pass


def hex_id(value: bytes | None) -> str | None:
    return value.hex() if value is not None else None


def account_key(lineage: bytes, kind: int, lifetime: int, context: int) -> str:
    return (lineage + struct.pack("<HHQQ4x", 1, kind, lifetime, context)).hex()


def bounded(cursor, sql: str, params: tuple = ()) -> list[dict]:
    cursor.execute(sql + " LIMIT %s", params + (MAX_ROWS + 1,))
    rows = cursor.fetchall()
    if len(rows) > MAX_ROWS:
        raise ExportError("SQL audit collection exceeds row limit")
    return rows


def operation_rows(cursor, lineage: bytes, epoch: bytes) -> tuple[list[dict], list[bytes]]:
    rows = bounded(cursor,
        "SELECT operation_id,original_operation_id,reason,outcome,result_code,source_event,"
        "account_count,posting_count,child_count,item_event_count "
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
                           "account_count": row["account_count"],
                           "posting_count": row["posting_count"],
                           "child_count": row["child_count"],
                           "item_event_count": row["item_event_count"]})
    return operations, ids


def scoped_rows(cursor, table: str, columns: str, lineage: bytes, epoch: bytes,
                order: str) -> list[dict]:
    # Names are fixed call-site constants; only lineage and epoch are bound data.
    return bounded(cursor,
        f"SELECT {columns} FROM {table} e JOIN economic_accounting_operation o "
        "ON o.operation_id=e.operation_id WHERE o.lineage=%s AND o.epoch=%s "
        f"AND o.reason<>38 ORDER BY {order}", (lineage, epoch))


def read_evidence(cursor, lineage: bytes, epoch: bytes) -> dict:
    result = {name: [] for name in TABLES}
    result["operations"], _ = operation_rows(cursor, lineage, epoch)
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
        "e.operation_id,e.line_index,e.account_index,e.child_index,e.delta_copper,"
        "e.delta_silver,e.delta_gold,e.delta_platinum,e.copper_value", lineage, epoch,
        "e.operation_id,e.line_index")
    for row in postings:
        result["postings"].append({
            "operation_id": hex_id(row["operation_id"]), "line_index": row["line_index"],
            "account_index": row["account_index"], "child_index": row["child_index"],
            "delta": [row[f"delta_{unit}"] for unit in ("copper", "silver", "gold", "platinum")],
            "copper_value": row["copper_value"]})
    children = scoped_rows(cursor, "economic_accounting_child",
        "e.operation_id,e.child_index,e.child_operation_id,e.parent_index", lineage, epoch,
        "e.operation_id,e.child_index")
    for row in children:
        result["children"].append({"operation_id": hex_id(row["operation_id"]),
                                    "child_index": row["child_index"],
                                    "child_operation_id": hex_id(row["child_operation_id"]),
                                    "parent_index": row["parent_index"]})
    references = scoped_rows(cursor, "economic_accounting_item_reference",
        "e.operation_id,e.event_index,e.child_index,e.item_uid,e.before_revision,"
        "e.after_revision,e.legacy_operation_id,e.legacy_event_index", lineage, epoch,
        "e.operation_id,e.event_index")
    for row in references:
        result["item_references"].append({
            "operation_id": hex_id(row["operation_id"]), "event_index": row["event_index"],
            "child_index": row["child_index"], "uid": row["item_uid"],
            "before_revision": row["before_revision"], "after_revision": row["after_revision"],
            "legacy_operation_id": hex_id(row["legacy_operation_id"]),
            "legacy_event_index": row["legacy_event_index"]})
    claims = bounded(cursor,
        "SELECT s.lineage,s.source_event,s.operation_id,"
        "o.lineage AS operation_lineage,o.epoch AS operation_epoch,"
        "o.source_event AS operation_source_event,o.outcome AS operation_outcome "
        "FROM economic_accounting_source_claim s "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=s.operation_id "
        "WHERE s.lineage=%s AND (o.reason<>38 OR o.operation_id IS NULL) "
        "ORDER BY s.source_event", (lineage,))
    result["source_claims"] = [{"lineage": hex_id(row["lineage"]),
                                "source_event": hex_id(row["source_event"]),
                                "operation_id": hex_id(row["operation_id"]),
                                "operation_lineage": hex_id(row["operation_lineage"]),
                                "operation_epoch": hex_id(row["operation_epoch"]),
                                "operation_source_event": hex_id(row["operation_source_event"]),
                                "operation_outcome": {1: "committed", 2: "rejected"}.get(
                                    row["operation_outcome"], "unknown")}
                               for row in claims]
    cursor.execute(
        "SELECT COUNT(*) AS source_operations,COALESCE(SUM(CASE WHEN EXISTS("
        "SELECT 1 FROM economic_accounting_source_claim s WHERE s.lineage=o.lineage "
        "AND s.source_event=o.source_event AND s.operation_id=o.operation_id) "
        "THEN 0 ELSE 1 END),0) AS missing_claim_operations "
        "FROM economic_accounting_operation o WHERE o.lineage=%s AND o.reason<>38 "
        "AND o.outcome=1 AND o.source_event IS NOT NULL", (lineage,))
    source_count = cursor.fetchone()
    cursor.execute(
        "SELECT COUNT(*) AS duplicate_source_values FROM ("
        "SELECT source_event FROM economic_accounting_operation "
        "WHERE lineage=%s AND reason<>38 AND outcome=1 AND source_event IS NOT NULL "
        "GROUP BY source_event HAVING COUNT(*)>1) duplicates", (lineage,))
    duplicates = cursor.fetchone()
    result["source_claim_coverage"] = {
        "source_operations": int(source_count["source_operations"]),
        "missing_claim_operations": int(source_count["missing_claim_operations"]),
        "duplicate_source_values": int(duplicates["duplicate_source_values"])}
    receipts = bounded(cursor,
        "SELECT o.operation_id,i.status,i.result_code FROM economic_accounting_operation o "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=o.operation_id "
        "WHERE o.lineage=%s AND o.epoch=%s AND o.reason<>38 "
        "ORDER BY o.operation_id", (lineage, epoch))
    result["receipts"] = [{"operation_id": hex_id(row["operation_id"]),
                           "status": "committed" if row["result_code"] == 0 else "rejected",
                           "result_code": row["result_code"]}
                          for row in receipts if row["status"] == 1]
    ledger = bounded(cursor,
        "SELECT l.operation_id,l.event_index,l.item_uid,l.root_item_uid,l.parent_item_uid,"
        "l.to_owner_type,l.to_owner_id,l.to_owner_context_id,l.item_revision,l.reason_type,"
        "r.before_revision FROM item_ownership_ledger l "
        "JOIN economic_accounting_item_reference r ON r.legacy_operation_id=l.operation_id "
        "AND r.legacy_event_index=l.event_index "
        "JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
        "WHERE o.lineage=%s AND o.epoch=%s AND o.reason<>38 "
        "ORDER BY l.operation_id,l.event_index", (lineage, epoch))
    for row in ledger:
        reason = row["reason_type"]
        result["ownership_events"].append({
            "operation_id": hex_id(row["operation_id"]), "event_index": row["event_index"],
            "uid": row["item_uid"], "before_revision": row["before_revision"],
            "revision": row["item_revision"], "root": row["root_item_uid"],
            "parent": row["parent_item_uid"],
            "owner": [row["to_owner_type"], row["to_owner_id"], row["to_owner_context_id"]],
            "state": "tombstone" if row["to_owner_type"] == 8 else "live",
            "action": "create" if reason == 2 else "destroy" if reason == 3 else "move"})
    return result


def native_source_count(cursor, table: str, identity_column: str,
                        kind: int) -> tuple[int, int]:
    # The table, locator and account kind are fixed call-site constants.
    cursor.execute(
        f"SELECT COUNT(*) AS source_rows,COALESCE(SUM(CASE WHEN EXISTS("
        "SELECT 1 FROM economic_account_mapping m WHERE "
        f"m.backend_kind=1 AND m.account_kind={kind} "
        f"AND m.active_native_id=n.{identity_column}) THEN 0 ELSE 1 END),0) "
        f"AS unmapped_rows FROM {table} n")
    row = cursor.fetchone()
    if row is None:
        raise ExportError("missing native source census")
    return int(row["source_rows"]), int(row["unmapped_rows"])


def read_native(cursor, lineage: bytes) -> tuple[dict, list[str], dict]:
    native = {"holdings": [], "items": []}
    gaps = ["coin_pile_payloads", "escrow_claim_treasury_native_classes",
            "global_uid_scope_not_lineage_bound", "unreferenced_uid_event_scope",
            "retired_mapping_semantics", "post_baseline_creation_origins",
            "cross_epoch_required_source_event_scope", "baseline_source_claim_scope",
            "realized_domain_prices"]
    mappings = bounded(cursor,
        "SELECT m.mapping_id,m.account_kind,m.context_id,m.active_native_id,"
        "p.pid AS wallet_id,b.id AS bank_id,"
        "p.copper AS wallet_copper,p.silver AS wallet_silver,p.gold AS wallet_gold,"
        "p.platinum AS wallet_platinum,p.wallet_revision,"
        "b.bank_copper,b.bank_silver,b.bank_gold,b.bank_platinum,b.bank_revision "
        "FROM economic_account_mapping m "
        "LEFT JOIN player_data p ON m.account_kind=1 AND p.pid=m.active_native_id "
        "LEFT JOIN account_banks b ON m.account_kind=2 AND b.id=m.active_native_id "
        "WHERE m.lineage=%s AND m.backend_kind=1 ORDER BY m.mapping_id", (lineage,))
    coverage = {"wallet_rows": 0, "bank_rows": 0,
                "unmapped_wallet_rows": 0, "unmapped_bank_rows": 0,
                "multiply_mapped_wallet_rows": 0, "multiply_mapped_bank_rows": 0,
                "dangling_wallet_mappings": 0, "dangling_bank_mappings": 0,
                "invalid_wallet_rows": 0, "invalid_bank_rows": 0}
    active_ids = Counter((row["account_kind"], row["active_native_id"])
                         for row in mappings if row["active_native_id"] is not None)
    existing_ids = set()
    for row in mappings:
        kind = row["account_kind"]
        if kind not in (1, 2):
            continue
        if row["active_native_id"] is None:
            continue
        prefix = "wallet" if kind == 1 else "bank"
        if row[f"{prefix}_id"] is None:
            coverage[f"dangling_{prefix}_mappings"] += 1
            continue
        existing_ids.add((kind, row["active_native_id"]))
        balance = ([row[f"wallet_{unit}"] for unit in ("copper", "silver", "gold", "platinum")]
                   if kind == 1 else
                   [row[f"bank_{unit}"] for unit in ("copper", "silver", "gold", "platinum")])
        revision = row[f"{prefix}_revision"]
        if any(amount is None for amount in balance) or revision is None:
            coverage[f"invalid_{prefix}_rows"] += 1
            continue
        native["holdings"].append({"account_key": account_key(lineage, kind, row["mapping_id"],
                                                              row["context_id"]),
                                   "balance": balance, "revision": revision})
    for kind, prefix, table, locator in ((1, "wallet", "player_data", "pid"),
                                          (2, "bank", "account_banks", "id")):
        total, unmapped = native_source_count(cursor, table, locator, kind)
        coverage[f"{prefix}_rows"] = total
        coverage[f"unmapped_{prefix}_rows"] = unmapped
        coverage[f"multiply_mapped_{prefix}_rows"] = sum(
            count > 1 for (account_kind, native_id), count in active_ids.items()
            if account_kind == kind and (account_kind, native_id) in existing_ids)
    items = bounded(cursor,
        "SELECT item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
        "owner_context_id,item_revision,state FROM item_current_owner ORDER BY item_uid")
    for row in items:
        if row["state"] not in ITEM_STATES:
            raise ExportError("invalid native item state")
        native["items"].append({"uid": row["item_uid"], "root": row["root_item_uid"],
                                "parent": row["parent_item_uid"],
                                "owner": [row["owner_type"], row["owner_id"],
                                          row["owner_context_id"]],
                                "revision": row["item_revision"],
                                "state": ITEM_STATES[row["state"]]})
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
            "'economic_accounting_source_claim','economic_account_mapping','player_data',"
            "'account_banks','item_current_owner','item_ownership_ledger')")
        engines = {row["table_name"]: row["engine"] for row in cursor.fetchall()}
        if len(engines) != 10 or any(engine != "InnoDB" for engine in engines.values()):
            raise ExportError("SQL audit source is missing or not InnoDB")
        evidence = read_evidence(cursor, lineage, epoch)
        native, gaps, coverage = read_native(cursor, lineage)
        result = {"schema_version": 1, "lineage": lineage.hex(), "epoch": epoch.hex(),
                  "backend": "sql_partial", "complete": False, "quiescent": True,
                  "capture_gaps": gaps, "native": native,
                  "native_mapping_coverage": coverage,
                  **evidence,
                  "account_origins": origins["account_origins"],
                  "item_origins": origins["item_origins"]}
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
                                         password=password, database=args.database,
                                         charset="utf8mb4", autocommit=True,
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
