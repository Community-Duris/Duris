#!/usr/bin/env python3
"""Read-only reconciliation and bounded, ID-only views of an economic snapshot.

The input is a complete, quiescent export in the format documented in
docs/persistence/economy_accounting/AUDIT_OPERATIONS.md. This process never opens
the game database or writes native authority. An incomplete export is an
exception, never an empty baseline.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re
import sys

MAX_INPUT_BYTES = 32 * 1024 * 1024
MAX_ROWS = 100_000
MAX_OUTPUT_ROWS = 100
MAPPED_KINDS = {1, 2, 3, 4, 5, 6}
ORDINARY_KINDS = MAPPED_KINDS | {11}
UNITS = (1, 10, 100, 1000)
HEX_ID = re.compile(r"[0-9a-f]{32}\Z")
HEX_SOURCE_EVENT = re.compile(r"[0-9a-f]{96}\Z")
HEX_KEY = re.compile(r"[0-9a-f]{80}\Z")
# Independent policy interpretation; exhaustive native metadata comparisons
# protect this table without importing the mutation implementation at runtime.
SOURCE_KINDS_BY_REASON = {
    3: (16,), 5: (1,), 6: (2,), 7: (3, 7), 8: (3,), 9: (4,), 10: (5,),
    11: (17,), 12: (17,), 13: (17,), 14: (17,), 15: (17,), 16: (17,),
    17: (8,), 18: (6,), 19: (6,), 21: (12, 18), 22: (12, 18),
    24: (17, 18), 26: (13,), 27: (13,), 28: (13,), 29: (13,), 30: (13,),
    31: (13, 17), 36: (14,), 38: (10,), 41: (16,), 42: (16,),
    43: (18,), 44: (19,), 45: (6,), 46: (6,),
}
TABLES = (
    "operations", "effects", "postings", "children", "item_references",
    "ownership_events", "source_claims", "account_origins", "item_origins", "receipts",
)
ORPHAN_EVIDENCE_SOURCES = {
    "effects": ("economic_accounting_account_effect", "account_index", "orphan_account_effect"),
    "postings": ("economic_accounting_coin_posting", "line_index", "orphan_coin_posting"),
    "children": ("economic_accounting_child", "child_index", "orphan_accounting_child"),
    "item_references": ("economic_accounting_item_reference", "event_index", "orphan_item_reference"),
}
NATIVE_MAPPING_KINDS = (
    "wallet", "bank", "pile", "auction_escrow", "pending_claim", "treasury",
)
NATIVE_COVERAGE_EXCEPTIONS = {
    field: code
    for kind in NATIVE_MAPPING_KINDS
    for field, code in (
        (f"unmapped_{kind}_rows", f"unmapped_native_{kind}"),
        (f"multiply_mapped_{kind}_rows", f"multiply_mapped_native_{kind}"),
        (f"dangling_{kind}_mappings", f"dangling_{kind}_mapping"),
        (f"invalid_{kind}_mappings", f"invalid_native_{kind}_mapping"),
        (f"invalid_{kind}_rows", f"invalid_native_{kind}"),
    )
}
NATIVE_COVERAGE_EXCEPTIONS["invalid_unknown_kind_mappings"] = (
    "invalid_native_mapping_kind")
REGISTRY_PATH = Path(__file__).resolve().parents[1] / "docs/persistence/economy_accounting/registry.json"


class SnapshotError(ValueError):
    pass


def vector(value: object) -> tuple[int, int, int, int]:
    if (not isinstance(value, list) or len(value) != 4 or
            any(type(part) is not int or not -(2**63) <= part < 2**63 for part in value)):
        raise SnapshotError("invalid denomination vector")
    checked = tuple(value)
    # Matching native/opening/effect vectors can still exceed the native money
    # range. Validate their weighted value, not only each denomination field.
    copper(checked)
    return checked


def copper(value: tuple[int, int, int, int]) -> int:
    total = sum(part * unit for part, unit in zip(value, UNITS))
    if not -(2**63) <= total < 2**63:
        raise SnapshotError("copper overflow")
    return total


def account_key(value: object) -> tuple[str, int, int, int]:
    if not isinstance(value, str) or not HEX_KEY.fullmatch(value):
        raise SnapshotError("invalid account key")
    raw = bytes.fromhex(value)
    kind = int.from_bytes(raw[18:20], "little")
    identity = int.from_bytes(raw[20:28], "little")
    context = int.from_bytes(raw[28:36], "little")
    if raw[16:18] != b"\x01\x00" or raw[36:] != bytes(4) or not 1 <= kind <= 11 or not identity:
        raise SnapshotError("invalid account key")
    return raw[:16].hex(), kind, identity, context


def decode_source_event(value: object) -> tuple[int, str, str, int, int]:
    """Decode the native S48 identity without invoking a mutation codec."""
    if not isinstance(value, str) or not HEX_SOURCE_EVENT.fullmatch(value):
        raise SnapshotError("invalid source event")
    raw = bytes.fromhex(value)
    kind = int.from_bytes(raw[:2], "little")
    if (raw[2:4] != b"\x01\x00" or not 1 <= kind <= 23 or
            not any(raw[4:20]) or not any(raw[20:36])):
        raise SnapshotError("invalid source event")
    return (kind, raw[4:20].hex(), raw[20:36].hex(),
            int.from_bytes(raw[36:44], "little"), int.from_bytes(raw[44:48], "little"))


def source_kind_allowed(reason: object, kind: object) -> bool:
    # The current native contract permits every valid kind for other known
    # reasons. Unknown reasons never inherit that permissive default.
    return (type(reason) is int and 1 <= reason <= 46 and
            type(kind) is int and 1 <= kind <= 23 and
            kind in SOURCE_KINDS_BY_REASON.get(reason, range(1, 24)))


def unsigned_revision(value: object) -> bool:
    return type(value) is int and 0 <= value < 2**64


def item_revision_transition(before: object, after: object) -> bool:
    return unsigned_revision(before) and unsigned_revision(after) and after == before + 1


def require_id(value: object, label: str) -> str:
    if not isinstance(value, str) or not HEX_ID.fullmatch(value) or value == "0" * 32:
        raise SnapshotError(f"invalid {label}")
    return value


class Reconciler:
    def __init__(self, limit: int = 50):
        if not 0 <= limit <= MAX_OUTPUT_ROWS:
            raise SnapshotError("invalid output limit")
        self.limit = limit
        self.exceptions: list[dict] = []
        self.counts: Counter = Counter()
        registry = json.loads(REGISTRY_PATH.read_text(encoding="utf-8"))
        self.reasons = {row["number"]: row for row in registry["reasons"]}
        self.realized_price_reasons = {
            number for number, row in self.reasons.items()
            if row.get("realized_price_required") is True}
        self.kinds = {row["number"]: row for row in registry["account_kinds"]}

    def emit(self, code: str, **ids: object) -> None:
        self.counts[code] += 1
        if len(self.exceptions) < self.limit:
            safe = {}
            for field, value in ids.items():
                if field in ("operation_id",) and isinstance(value, str) and HEX_ID.fullmatch(value):
                    safe[field] = value
                elif field == "account_key" and isinstance(value, str) and HEX_KEY.fullmatch(value):
                    safe[field] = value
                elif field == "source_event" and isinstance(value, str) and re.fullmatch(r"[0-9a-f]{96}", value):
                    safe[field] = value
                elif field in ("uid", "parent_uid", "child_index", "line_index", "source_slot",
                               "net_copper", "ship_id", "guild_id") and type(value) is int:
                    safe[field] = value
                elif field == "table" and value in TABLES:
                    safe[field] = value
                elif field == "scope" and value == "snapshot":
                    safe[field] = value
            self.exceptions.append({"code": code, **safe})

    def emit_count(self, code: str, count: int) -> None:
        if count:
            self.emit(code, scope="snapshot")
            self.counts[code] += count - 1

    def table(self, snapshot: dict, name: str) -> list[dict]:
        value = snapshot.get(name)
        if not isinstance(value, list) or len(value) > MAX_ROWS or any(not isinstance(row, dict) for row in value):
            raise SnapshotError(f"invalid or oversized {name}")
        return value

    def index(self, rows: list[dict], keys: tuple[str, ...], code: str) -> dict[tuple, dict]:
        result = {}
        for row in rows:
            key = tuple(row.get(field) for field in keys)
            if key in result:
                self.emit(code, **{field: row.get(field) for field in keys})
            else:
                result[key] = row
        return result

    def audit(self, snapshot: dict) -> dict:
        if not isinstance(snapshot, dict) or type(snapshot.get("schema_version")) is not int or snapshot["schema_version"] != 1:
            raise SnapshotError("unsupported snapshot version")
        lineage = require_id(snapshot.get("lineage"), "lineage")
        epoch = require_id(snapshot.get("epoch"), "epoch")
        if snapshot.get("complete") is not True:
            self.emit("evidence_loss", scope="snapshot")
        if snapshot.get("quiescent") is not True:
            self.emit("unfenced_snapshot", scope="snapshot")
        tables = {name: self.table(snapshot, name) for name in TABLES}
        self.audit_orphan_evidence(snapshot)
        native = snapshot.get("native")
        if not isinstance(native, dict):
            raise SnapshotError("missing native authority")
        self.audit_ship_coffers(snapshot.get("backend"), native)
        self.audit_guild_treasuries(snapshot.get("backend"), native)
        holdings = self.table(native, "holdings")
        items = self.table(native, "items")
        unreferenced_uid_events = native.get("unreferenced_uid_events")
        uid_event_coverage = native.get("uid_event_coverage")
        if unreferenced_uid_events is None and snapshot.get("backend") == "sql_partial":
            self.emit("missing_uid_event_coverage", scope="snapshot")
        elif unreferenced_uid_events is not None or uid_event_coverage is not None:
            if (not isinstance(unreferenced_uid_events, list) or
                    len(unreferenced_uid_events) > MAX_ROWS or
                    any(not isinstance(row, dict) for row in unreferenced_uid_events)):
                raise SnapshotError("invalid or oversized unreferenced_uid_events")
            if (not isinstance(uid_event_coverage, dict) or
                    set(uid_event_coverage) != {"tracked_uids", "ledger_events",
                                                "referenced_events", "unreferenced_events"} or
                    any(type(value) is not int or not 0 <= value < 2**63
                        for value in uid_event_coverage.values()) or
                    uid_event_coverage["unreferenced_events"] != len(unreferenced_uid_events) or
                    uid_event_coverage["referenced_events"] + uid_event_coverage["unreferenced_events"] !=
                    uid_event_coverage["ledger_events"]):
                raise SnapshotError("invalid UID event coverage")
            seen_uid_events = set()
            for event in unreferenced_uid_events:
                operation_id = require_id(event.get("operation_id"), "UID event operation ID")
                event_index = event.get("event_index")
                uid = event.get("uid")
                before_revision = event.get("before_revision")
                revision = event.get("revision")
                root_uid = event.get("root")
                parent_uid = event.get("parent")
                owner = event.get("owner")
                if (type(event_index) is not int or not 0 <= event_index <= 65535 or
                        type(uid) is not int or not 0 < uid < 2**64 or
                        not item_revision_transition(before_revision, revision) or
                        type(root_uid) is not int or not 0 < root_uid < 2**64 or
                        (parent_uid is not None and
                         (type(parent_uid) is not int or not 0 < parent_uid < 2**64)) or
                        not isinstance(owner, list) or len(owner) != 3 or
                        any(type(value) is not int or value < 0 for value in owner) or
                        event.get("state") not in ("live", "tombstone") or
                        event.get("action") not in ("create", "destroy", "move")):
                    raise SnapshotError("invalid unreferenced UID event")
                key = (operation_id, event_index)
                if key in seen_uid_events:
                    raise SnapshotError("duplicate unreferenced UID event")
                seen_uid_events.add(key)
                self.emit("unreferenced_uid_event", operation_id=operation_id, uid=uid)
        pending_sources = native.get("pending_claim_sources")
        pending_source_coverage = native.get("pending_claim_source_coverage")
        if pending_sources is None and snapshot.get("backend") == "sql_partial":
            self.emit("missing_pending_claim_source_coverage", scope="snapshot")
        elif pending_sources is not None or pending_source_coverage is not None:
            if not isinstance(pending_sources, list) or len(pending_sources) > MAX_ROWS or any(
                    not isinstance(row, dict) for row in pending_sources):
                raise SnapshotError("invalid or oversized pending_claim_sources")
            if (not isinstance(pending_source_coverage, dict) or
                    set(pending_source_coverage) != {
                        "rows", "open_rows", "consumed_rows", "invalid_account_mappings",
                        "invalid_source_roots", "invalid_consumer_roots"} or
                    any(type(value) is not int or not 0 <= value < 2**63
                        for value in pending_source_coverage.values()) or
                    pending_source_coverage["rows"] != len(pending_sources) or
                    pending_source_coverage["open_rows"] + pending_source_coverage["consumed_rows"] !=
                    pending_source_coverage["rows"] or
                    pending_source_coverage["invalid_account_mappings"] >
                    pending_source_coverage["rows"] or
                    pending_source_coverage["invalid_source_roots"] >
                    pending_source_coverage["rows"] or
                    pending_source_coverage["invalid_consumer_roots"] >
                    pending_source_coverage["consumed_rows"]):
                raise SnapshotError("invalid pending claim source coverage")
            seen_sources = set()
            invalid_mappings = 0
            invalid_source_roots = 0
            invalid_consumer_roots = 0
            open_claim_amounts = Counter()
            for source in pending_sources:
                source_operation = require_id(source.get("source_operation_id"),
                                              "pending claim source operation ID")
                slot = source.get("source_slot")
                beneficiary = source.get("beneficiary_pid")
                amount = source.get("amount")
                claim_operation = source.get("claim_operation_id")
                if type(source.get("source_root_valid")) is not bool:
                    raise SnapshotError("invalid pending claim source root flag")
                source_receipt = source.get("source_inbox_receipt")
                if (not isinstance(source_receipt, dict) or
                        set(source_receipt) != {"status", "result_code", "failure_stage",
                                                "committed_at_present"} or
                        type(source_receipt.get("status")) is not int or
                        source_receipt.get("status") != 1 or
                        type(source_receipt.get("result_code")) is not int or
                        source_receipt.get("result_code") != 0 or
                        type(source_receipt.get("failure_stage")) is not int or
                        source_receipt.get("failure_stage") != 0 or
                        type(source_receipt.get("committed_at_present")) is not bool or
                        not source_receipt.get("committed_at_present")):
                    self.emit("invalid_pending_claim_source_root",
                              operation_id=source_operation, source_slot=source.get("source_slot"))
                consumer_root_valid = source.get("consumer_root_valid")
                consumer_receipt = source.get("consumer_inbox_receipt")
                if (claim_operation is None and consumer_root_valid is not None) or (
                        claim_operation is None and consumer_receipt is not None) or (
                        claim_operation is not None and type(consumer_root_valid) is not bool):
                    raise SnapshotError("invalid pending claim consumer root flag")
                if claim_operation is not None and (
                        not isinstance(consumer_receipt, dict) or
                        set(consumer_receipt) != {"status", "result_code", "failure_stage",
                                                  "committed_at_present"} or
                        type(consumer_receipt.get("status")) is not int or
                        consumer_receipt.get("status") != 1 or
                        type(consumer_receipt.get("result_code")) is not int or
                        consumer_receipt.get("result_code") != 0 or
                        type(consumer_receipt.get("failure_stage")) is not int or
                        consumer_receipt.get("failure_stage") != 0 or
                        type(consumer_receipt.get("committed_at_present")) is not bool or
                        not consumer_receipt.get("committed_at_present")):
                    self.emit("invalid_pending_claim_consumer_root",
                              operation_id=claim_operation, source_slot=source.get("source_slot"))
                if (type(slot) is not int or not 1 <= slot <= 65535 or
                        type(beneficiary) is not int or not 0 < beneficiary < 2**32 or
                        type(amount) is not int or not 0 < amount < 2**63):
                    raise SnapshotError("invalid pending claim source")
                if claim_operation is not None:
                    require_id(claim_operation, "pending claim consumer operation ID")
                source_key = (source_operation, slot)
                if source_key in seen_sources:
                    self.emit("duplicate_pending_claim_source", operation_id=source_operation,
                              source_slot=slot)
                seen_sources.add(source_key)
                account = source.get("account_key")
                mapping_valid = source.get("mapping_valid")
                mapping_native_id = source.get("mapping_native_id")
                mapping_active_native_id = source.get("mapping_active_native_id")
                native_id_valid = (type(mapping_native_id) is int and
                                   0 < mapping_native_id < 2**64)
                active_id_valid = (mapping_active_native_id is None or
                                   (type(mapping_active_native_id) is int and
                                    0 < mapping_active_native_id < 2**64))
                expected_mapping_valid = (
                    account is not None and native_id_valid and
                    mapping_native_id == beneficiary and
                    active_id_valid and
                    (claim_operation is not None or
                     mapping_active_native_id == beneficiary))
                if (type(mapping_valid) is not bool or
                        (account is None and
                         (mapping_native_id is not None or mapping_active_native_id is not None)) or
                        (account is not None and
                         (not native_id_valid or not active_id_valid)) or
                        mapping_valid != expected_mapping_valid):
                    raise SnapshotError("invalid pending claim source mapping flag")
                if not mapping_valid:
                    invalid_mappings += 1
                if not source["source_root_valid"]:
                    invalid_source_roots += 1
                if claim_operation is not None and not consumer_root_valid:
                    invalid_consumer_roots += 1
                if account is not None:
                    key = account_key(account)
                    if key[0] != lineage or key[1] != 5:
                        raise SnapshotError("pending claim source maps to another account kind")
                    if mapping_valid and claim_operation is None:
                        open_claim_amounts[account] += amount
            if invalid_mappings != pending_source_coverage["invalid_account_mappings"]:
                raise SnapshotError("pending claim source mapping coverage mismatch")
            if invalid_source_roots != pending_source_coverage["invalid_source_roots"]:
                raise SnapshotError("pending claim source root coverage mismatch")
            if invalid_consumer_roots != pending_source_coverage["invalid_consumer_roots"]:
                raise SnapshotError("pending claim consumer root coverage mismatch")
            self.emit_count("invalid_pending_claim_source_mapping", invalid_mappings)
            self.emit_count("invalid_pending_claim_source_root", invalid_source_roots)
            self.emit_count("invalid_pending_claim_consumer_root", invalid_consumer_roots)
            native_claim_amounts = {}
            for holding in holdings:
                account = holding.get("account_key")
                key = account_key(account)
                if key[0] == lineage and key[1] == 5:
                    native_claim_amounts[account] = vector(holding.get("balance"))[0]
            for account in set(open_claim_amounts) | set(native_claim_amounts):
                if open_claim_amounts[account] != native_claim_amounts.get(account, 0):
                    self.emit("pending_claim_source_balance_mismatch", account_key=account)
        coverage = snapshot.get("native_mapping_coverage")
        if coverage is None:
            if snapshot.get("backend") == "sql_partial":
                self.emit("missing_native_mapping_coverage", scope="snapshot")
        else:
            if not isinstance(coverage, dict) or set(coverage) != (
                    {f"{kind}_rows" for kind in NATIVE_MAPPING_KINDS} |
                    set(NATIVE_COVERAGE_EXCEPTIONS)):
                raise SnapshotError("invalid native mapping coverage")
            if any(type(value) is not int or not 0 <= value < 2**63 for value in coverage.values()):
                raise SnapshotError("invalid native mapping coverage count")
            for kind in NATIVE_MAPPING_KINDS:
                if (coverage[f"unmapped_{kind}_rows"] > coverage[f"{kind}_rows"] or
                        coverage[f"multiply_mapped_{kind}_rows"] > coverage[f"{kind}_rows"]):
                    raise SnapshotError("invalid native mapping coverage total")
            for field, code in NATIVE_COVERAGE_EXCEPTIONS.items():
                self.emit_count(code, coverage[field])
        claim_coverage = snapshot.get("source_claim_coverage")
        if claim_coverage is not None:
            if (not isinstance(claim_coverage, dict) or set(claim_coverage) !=
                    {"source_operations", "missing_claim_operations", "duplicate_source_values"} or
                    any(type(value) is not int or not 0 <= value < 2**63
                        for value in claim_coverage.values()) or
                    claim_coverage["missing_claim_operations"] > claim_coverage["source_operations"] or
                    claim_coverage["duplicate_source_values"] > claim_coverage["source_operations"]):
                raise SnapshotError("invalid source claim coverage")
            self.emit_count("lineage_missing_source_claim",
                            claim_coverage["missing_claim_operations"])
            self.emit_count("lineage_duplicate_source_event",
                            claim_coverage["duplicate_source_values"])
        source_policy_coverage = snapshot.get("source_event_policy_coverage")
        if source_policy_coverage is None and snapshot.get("backend") == "sql_partial":
            self.emit("missing_source_event_policy_coverage", scope="snapshot")
        elif source_policy_coverage is not None:
            if (not isinstance(source_policy_coverage, dict) or set(source_policy_coverage) != {
                    "required_committed_operations", "missing_required_source_events"} or
                    any(type(value) is not int or not 0 <= value < 2**63
                        for value in source_policy_coverage.values()) or
                    source_policy_coverage["missing_required_source_events"] >
                    source_policy_coverage["required_committed_operations"]):
                raise SnapshotError("invalid source event policy coverage")
            self.emit_count("lineage_missing_required_source_event",
                            source_policy_coverage["missing_required_source_events"])

        operations = self.index(tables["operations"], ("operation_id",), "duplicate_operation")
        effects = self.index(tables["effects"], ("operation_id", "account_index"), "duplicate_effect")
        postings = self.index(tables["postings"], ("operation_id", "line_index"), "duplicate_posting")
        children = self.index(tables["children"], ("operation_id", "child_index"), "duplicate_child")
        references = self.index(tables["item_references"], ("operation_id", "event_index"),
                                "duplicate_item_reference")
        ownership = self.index(tables["ownership_events"], ("operation_id", "event_index"),
                               "duplicate_ownership_event")
        claims = self.index(tables["source_claims"], ("lineage", "source_event"),
                            "duplicate_source_event")
        receipts = self.index(tables["receipts"], ("operation_id",), "duplicate_receipt")
        origins = self.index(tables["account_origins"], ("account_key",), "duplicate_account_origin")
        item_origins = self.index(tables["item_origins"], ("uid",), "duplicate_item_origin")
        native_holdings = self.index(holdings, ("account_key",), "duplicate_native_holding")
        native_items = self.index(items, ("uid",), "duplicate_native_uid")
        self.audit_mapping_creations(snapshot.get("backend"), lineage, native, origins,
                                     operations, effects, epoch)
        self.audit_pending_claim_consumers(snapshot.get("backend"), lineage, native)
        self.audit_coin_pile_mappings(snapshot.get("backend"), native,
                                      native_holdings, native_items, lineage)
        self.audit_uid_scope_coverage(snapshot.get("backend"), native,
                                      item_origins, native_items, references)
        self.audit_unattributed_uid_history(snapshot.get("backend"), native)
        self.audit_lineage_realized_prices(lineage, epoch, snapshot.get("backend"),
                                           native, operations)

        by_op = {name: defaultdict(list) for name in ("effects", "postings", "children", "item_references")}
        for name, indexed in (("effects", effects), ("postings", postings), ("children", children),
                              ("item_references", references)):
            for row in indexed.values():
                by_op[name][row.get("operation_id")].append(row)
                if (row.get("operation_id"),) not in operations:
                    self.emit("orphan_evidence", table=name, operation_id=row.get("operation_id"))

        by_account: dict[str, list[dict]] = defaultdict(list)
        for row in effects.values():
            if (not unsigned_revision(row.get("before_revision")) or
                    not unsigned_revision(row.get("after_revision"))):
                raise SnapshotError("invalid account effect revision")
            key = row.get("account_key")
            account_key(key)
            by_account[key].append(row)
        effect_accounts = Counter((row.get("operation_id"), row.get("account_key"))
                                  for row in effects.values())
        for (op_id, key), count in effect_accounts.items():
            if count > 1:
                self.emit("duplicate_account_effect", operation_id=op_id, account_key=key)
        posting_deltas: dict[tuple, list[int]] = defaultdict(lambda: [0, 0, 0, 0])
        for row in postings.values():
            op_id = row.get("operation_id")
            effect_key = (op_id, row.get("account_index"))
            if effect_key not in effects:
                self.emit("orphan_posting", operation_id=op_id, line_index=row.get("line_index"))
            delta = vector(row.get("delta"))
            if copper(delta) != row.get("copper_value"):
                self.emit("posting_value_mismatch", operation_id=op_id, line_index=row.get("line_index"))
            for index, amount in enumerate(delta):
                posting_deltas[effect_key][index] += amount

        linked_children: set[tuple] = set()
        for op_key, op in operations.items():
            op_id = require_id(op_key[0], "operation ID")
            if op.get("lineage") != lineage:
                self.emit("foreign_lineage", operation_id=op_id)
            if op.get("epoch") != epoch:
                self.emit("foreign_epoch", operation_id=op_id)
            if op.get("outcome") not in ("committed", "rejected"):
                self.emit("unknown_outcome", operation_id=op_id)
            elif (op["outcome"] == "committed" and op.get("result_code") != 0 or
                  op["outcome"] == "rejected" and op.get("result_code") == 0):
                self.emit("invalid_result_code", operation_id=op_id)
            price = op.get("realized_price_copper")
            if price is not None:
                if type(price) is not int or price < 0 or price >= 2**63:
                    self.emit("invalid_realized_price", operation_id=op_id)
                elif op.get("outcome") != "committed":
                    self.emit("rejected_realized_price", operation_id=op_id)
            policy = self.reasons.get(op.get("reason")) if type(op.get("reason")) is int else None
            if policy is None:
                self.emit("unknown_policy_reason", operation_id=op_id)
            elif price is not None and not policy.get("realized_price_required", False):
                self.emit("unexpected_realized_price", operation_id=op_id)
            receipt = receipts.get((op_id,))
            if not receipt:
                self.emit("missing_receipt", operation_id=op_id)
            elif (type(receipt.get("status")) is not int or receipt.get("status") != 1 or
                  type(receipt.get("result_code")) is not int or
                  receipt.get("result_code") != op.get("result_code") or
                  type(receipt.get("failure_stage")) is not int or
                  receipt.get("failure_stage") != 0 or
                  type(receipt.get("committed_at_present")) is not bool or
                  not receipt.get("committed_at_present")):
                self.emit("receipt_mismatch", operation_id=op_id)
            for name, field in (("effects", "account_count"), ("postings", "posting_count"),
                                ("children", "child_count"), ("item_references", "item_event_count")):
                if len(by_op[name][op_id]) != op.get(field):
                    self.emit("evidence_count_mismatch", operation_id=op_id, table=name)
            if op.get("outcome") == "rejected" and any(by_op[name][op_id] for name in by_op):
                self.emit("rejected_operation_has_effects", operation_id=op_id)
            total = sum(copper(vector(row.get("delta"))) for row in by_op["postings"][op_id])
            if total:
                self.emit("unbalanced_root", operation_id=op_id, net_copper=total)
            source = op.get("source_event")
            if policy and policy.get("source_event_required") and source is None and op.get("outcome") == "committed":
                self.emit("missing_source_event", operation_id=op_id)
            if source is not None:
                try:
                    kind = decode_source_event(source)[0]
                except SnapshotError:
                    self.emit("invalid_source_event", operation_id=op_id)
                else:
                    if policy and not source_kind_allowed(op.get("reason"), kind):
                        self.emit("unauthorized_source_kind", operation_id=op_id)
            if source is not None and op.get("outcome") == "committed":
                claim = claims.get((lineage, source))
                if not claim or claim.get("operation_id") != op_id:
                    self.emit("missing_source_claim", operation_id=op_id)
            original = op.get("original_operation_id")
            original_valid = True
            if original is not None:
                try:
                    require_id(original, "original operation ID")
                except SnapshotError:
                    original_valid = False
                else:
                    original_valid = original != op_id
                if not original_valid:
                    self.emit("invalid_original_operation", operation_id=op_id)
            if (original_valid and policy and policy.get("original_operation_required") and
                    (original,) not in operations):
                self.emit("missing_original_operation", operation_id=op_id)
            child_indexes = {row.get("child_index") for row in by_op["children"][op_id]}
            for row in by_op["children"][op_id]:
                index = row.get("child_index")
                parent = row.get("parent_index")
                if type(index) is not int or not 1 <= index <= 64 or type(parent) is not int or not 0 <= parent < index:
                    self.emit("invalid_child_link", operation_id=op_id, child_index=index)
                child_id = row.get("child_operation_id")
                if child_id == op_id or not isinstance(child_id, str):
                    self.emit("invalid_child_link", operation_id=op_id, child_index=index)
            for name in ("postings", "item_references"):
                for row in by_op[name][op_id]:
                    child_index = row.get("child_index", 0)
                    if child_index:
                        if child_index not in child_indexes:
                            self.emit("unlinked_child_evidence", operation_id=op_id, child_index=child_index)
                        else:
                            linked_children.add((op_id, child_index))
        for row in children.values():
            link = (row.get("operation_id"), row.get("child_index"))
            if link not in linked_children:
                self.emit("unlinked_child", operation_id=link[0], child_index=link[1])
        for claim in claims.values():
            kind = None
            try:
                kind = decode_source_event(claim.get("source_event"))[0]
            except SnapshotError:
                self.emit("invalid_source_claim", operation_id=claim.get("operation_id"))
            op = operations.get((claim.get("operation_id"),))
            if (op is not None and claim.get("operation_reason") is not None and
                    (type(claim.get("operation_reason")) is not int or
                     claim.get("operation_reason") != op.get("reason"))):
                self.emit("source_claim_reason_mismatch",
                          operation_id=claim.get("operation_id"))
            reason = op.get("reason") if op is not None else claim.get("operation_reason")
            if type(reason) is not int or reason not in self.reasons:
                self.emit("unknown_source_claim_policy", operation_id=claim.get("operation_id"))
            elif kind is not None and not source_kind_allowed(reason, kind):
                self.emit("unauthorized_source_claim", operation_id=claim.get("operation_id"))
            if reason == 38:
                self.emit("baseline_source_claim", operation_id=claim.get("operation_id"),
                          source_event=claim.get("source_event"))
                continue
            inbox_receipt = claim.get("operation_inbox_receipt")
            durable_receipt = (
                isinstance(inbox_receipt, dict) and
                set(inbox_receipt) == {"status", "result_code", "failure_stage",
                                       "committed_at_present"} and
                type(inbox_receipt.get("status")) is int and
                inbox_receipt.get("status") == 1 and
                type(inbox_receipt.get("result_code")) is int and
                inbox_receipt.get("result_code") == claim.get("operation_result_code") and
                type(inbox_receipt.get("failure_stage")) is int and
                inbox_receipt.get("failure_stage") == 0 and
                type(inbox_receipt.get("committed_at_present")) is bool and
                inbox_receipt.get("committed_at_present") is True and
                claim.get("operation_result_code") == 0)
            linked = (op and op.get("outcome") == "committed" and
                      op.get("source_event") == claim.get("source_event"))
            if op and "operation_epoch" in claim:
                linked = (linked and claim.get("operation_epoch") == epoch and
                          claim.get("operation_lineage") == lineage and
                          claim.get("operation_source_event") == op.get("source_event") and
                          claim.get("operation_outcome") == op.get("outcome") and
                          durable_receipt)
            if not op and "operation_epoch" in claim:
                other_epoch = claim.get("operation_epoch")
                linked = (isinstance(other_epoch, str) and
                          re.fullmatch(r"[0-9a-f]{32}", other_epoch) is not None and
                          other_epoch != epoch and claim.get("operation_lineage") == lineage and
                          claim.get("operation_source_event") == claim.get("source_event") and
                          durable_receipt and
                          claim.get("operation_outcome") == "committed")
            if not linked or claim.get("lineage") != lineage:
                self.emit("orphan_source_claim", operation_id=claim.get("operation_id"))
        for receipt in receipts.values():
            if (receipt.get("operation_id"),) not in operations:
                self.emit("orphan_receipt", operation_id=receipt.get("operation_id"))
        seen_sources = {}
        for op in operations.values():
            source = op.get("source_event")
            if source is not None and op.get("outcome") == "committed":
                previous = seen_sources.setdefault(source, op.get("operation_id"))
                if previous != op.get("operation_id"):
                    self.emit("duplicate_source_event", source_event=source)

        self.audit_lineage_uid_references(lineage, epoch, snapshot.get("backend"),
                                          native, references)
        self.audit_coin_pile_lifecycle_sources(snapshot.get("backend"), native)
        self.audit_lineage_uid_history(snapshot.get("backend"), native,
                                       item_origins, native_items)
        self.audit_accounts(lineage, operations, by_account, origins, native_holdings, posting_deltas)
        self.audit_mapping_retirements(lineage, epoch, snapshot.get("backend"), operations,
                                       by_account, origins, native, native_holdings)
        self.audit_items(ownership, references, item_origins, native_items,
                         {row.get("uid") for row in (native.get("uid_history_events") or [])
                          if isinstance(row, dict)})
        return {"exception_count": sum(self.counts.values()), "exception_counts": dict(sorted(self.counts.items())),
                "exceptions": self.exceptions, "truncated": sum(self.counts.values()) > len(self.exceptions),
                "checked": {name: len(tables[name]) for name in TABLES} |
                           {"native_holdings": len(holdings), "native_items": len(items)}}

    def audit_orphan_evidence(self, snapshot: dict) -> None:
        rows = snapshot.get("orphan_evidence")
        coverage = snapshot.get("orphan_evidence_coverage")
        if rows is None and coverage is None:
            if snapshot.get("backend") == "sql_partial":
                self.emit("missing_orphan_evidence_coverage", scope="snapshot")
            return
        rows = self.table(snapshot, "orphan_evidence")
        counts = dict.fromkeys(ORPHAN_EVIDENCE_SOURCES, 0)
        for row in rows:
            table = row.get("table")
            if not isinstance(table, str) or table not in ORPHAN_EVIDENCE_SOURCES:
                raise SnapshotError("invalid orphan evidence table")
            operation_id = require_id(row.get("operation_id"), "orphan operation ID")
            if type(row.get("row_index")) is not int or not 0 <= row["row_index"] <= 65535:
                raise SnapshotError("invalid orphan evidence index")
            counts[table] += 1
            self.emit(ORPHAN_EVIDENCE_SOURCES[table][2], operation_id=operation_id,
                      table=table, line_index=row["row_index"])
        if (not isinstance(coverage, dict) or set(coverage) != {"scope", "table_counts"} or
                coverage["scope"] != "database" or not isinstance(coverage["table_counts"], dict) or
                set(coverage["table_counts"]) != set(counts) or
                any(type(value) is not int or value < 0
                    for value in coverage["table_counts"].values()) or
                coverage["table_counts"] != counts):
            raise SnapshotError("invalid orphan evidence coverage")

    def audit_ship_coffers(self, backend: object, native: dict) -> None:
        rows, coverage = native.get("ship_coffers"), native.get("ship_coffer_coverage")
        if rows is None and coverage is None:
            if backend == "sql_partial":
                self.emit("missing_ship_coffer_coverage", scope="snapshot")
            return
        rows = self.table(native, "ship_coffers")
        expected = dict(rows=len(rows), positive_rows=0, zero_rows=0,
                        unknown_rows=0, invalid_rows=0, missing_revision_rows=len(rows))
        if (not isinstance(coverage, dict) or set(coverage) != set(expected) or
                any(type(value) is not int or not 0 <= value <= MAX_ROWS
                    for value in coverage.values())):
            raise SnapshotError("invalid ship coffer coverage")
        seen = set()
        for row in rows:
            identity, amount = row.get("ship_id"), row.get("copper")
            if (set(row) != {"ship_id", "copper"} or type(identity) is not int or
                    not 1 <= identity < 2**31 or identity in seen or
                    (amount is not None and
                     (type(amount) is not int or not -(2**31) <= amount < 2**31))):
                raise SnapshotError("invalid native ship coffer")
            seen.add(identity)
            bucket = ("unknown_rows" if amount is None else "invalid_rows" if amount < 0
                      else "zero_rows" if amount == 0 else "positive_rows")
            expected[bucket] += 1
            self.emit("unsupported_native_ship_coffer", ship_id=identity)
            self.emit("missing_ship_coffer_revision", ship_id=identity)
            if amount is None:
                self.emit("unknown_native_ship_coffer", ship_id=identity)
            elif amount < 0:
                self.emit("invalid_native_ship_coffer", ship_id=identity)
        if coverage != expected:
            raise SnapshotError("ship coffer coverage count mismatch")

    def audit_guild_treasuries(self, backend: object, native: dict) -> None:
        rows = native.get("guild_treasuries")
        coverage = native.get("guild_treasury_coverage")
        if rows is None and coverage is None:
            if backend == "sql_partial":
                self.emit("missing_guild_treasury_coverage", scope="snapshot")
            return
        rows = self.table(native, "guild_treasuries")
        expected = dict(rows=len(rows), positive_rows=0, zero_rows=0,
                        missing_revision_rows=len(rows))
        if (not isinstance(coverage, dict) or set(coverage) != set(expected) or
                any(type(value) is not int or not 0 <= value <= MAX_ROWS
                    for value in coverage.values())):
            raise SnapshotError("invalid guild treasury coverage")
        seen = set()
        for row in rows:
            identity, balance = row.get("guild_id"), row.get("balance")
            if (set(row) != {"guild_id", "balance"} or type(identity) is not int or
                    not 1 <= identity < 2**32 or identity in seen or
                    not isinstance(balance, list) or len(balance) != 4 or
                    any(type(amount) is not int or not 0 <= amount < 2**32 for amount in balance)):
                raise SnapshotError("invalid native guild treasury")
            seen.add(identity)
            expected["positive_rows" if any(balance) else "zero_rows"] += 1
            self.emit("unsupported_native_guild_treasury", guild_id=identity)
            self.emit("missing_guild_money_revision", guild_id=identity)
        if coverage != expected:
            raise SnapshotError("guild treasury coverage count mismatch")

    def audit_accounts(self, lineage: str, operations: dict, by_account: dict, origins: dict, native: dict,
                       deltas: dict) -> None:
        for holding in native.values():
            if not unsigned_revision(holding.get("revision")):
                raise SnapshotError("invalid native holding revision")
        for key in set(by_account) | {row[0] for row in origins} | {row[0] for row in native}:
            key_lineage, kind, _, _ = account_key(key)
            if key_lineage != lineage:
                self.emit("foreign_account_lineage", account_key=key)
            for row in by_account.get(key, []):
                op = operations.get((row.get("operation_id"),))
                policy = self.reasons.get(op.get("reason")) if op else None
                if policy and self.kinds[kind]["id"] not in policy["account_kinds"]:
                    self.emit("unauthorized_counterparty", account_key=key,
                              operation_id=row.get("operation_id"))
            if kind not in ORDINARY_KINDS:
                if (key,) in native:
                    self.emit("unexpected_native_system_account", account_key=key)
                if (key,) in origins:
                    self.emit("unexpected_system_origin", account_key=key)
                for row in by_account.get(key, []):
                    delta = deltas.get((row.get("operation_id"), row.get("account_index")), (0, 0, 0, 0))
                    value = copper(tuple(delta))
                    op = operations.get((row.get("operation_id"),))
                    policy = self.reasons.get(op.get("reason")) if op else None
                    sign = (policy or {}).get("system_signs", {}).get(
                        self.kinds[kind]["id"], self.kinds[kind]["sign"])
                    if (sign == "debit_only" and value >= 0) or (sign == "credit_only" and value <= 0):
                        self.emit("invalid_system_posting_sign", account_key=key,
                                  operation_id=row.get("operation_id"))
                continue
            origin = origins.get((key,))
            current = native.get((key,))
            if not origin:
                self.emit("unknown_opening", account_key=key)
                continue
            if origin.get("origin") not in ("baseline", "creation"):
                self.emit("unknown_opening", account_key=key)
                continue
            expected = vector(origin.get("balance"))
            revision = origin.get("revision")
            if not unsigned_revision(revision):
                raise SnapshotError("invalid account origin revision")
            if origin["origin"] == "creation" and (expected != (0, 0, 0, 0) or revision != 0):
                self.emit("invalid_account_creation_origin", account_key=key)
            rows = sorted(by_account.get(key, []), key=lambda row: (row.get("before_revision", -1),
                                                                    row.get("operation_id", "")))
            for row in rows:
                op_id = row.get("operation_id")
                before, after = vector(row.get("before")), vector(row.get("after"))
                if kind in ORDINARY_KINDS:
                    if before != expected or row.get("before_revision") != revision:
                        self.emit("broken_account_history", account_key=key, operation_id=op_id)
                    if (type(row.get("before_revision")) is not int or
                            type(row.get("after_revision")) is not int or
                            row["after_revision"] <= row["before_revision"]):
                        self.emit("broken_account_history", account_key=key, operation_id=op_id)
                    delta = tuple(a - b for a, b in zip(after, before))
                    if delta != tuple(deltas.get((op_id, row.get("account_index")), (0, 0, 0, 0))):
                        self.emit("account_effect_posting_mismatch", account_key=key, operation_id=op_id)
                    if any(amount < 0 for amount in after):
                        self.emit("negative_holding", account_key=key, operation_id=op_id)
                    expected = after
                    revision = row.get("after_revision")
            retired_by = origin.get("retired_by")
            if retired_by is not None:
                retired_by = require_id(retired_by, "retirement operation ID")
                retirement = operations.get((retired_by,))
                if not retirement or retirement.get("outcome") != "committed" or retirement.get("lineage") != lineage:
                    self.emit("unknown_account_retirement", account_key=key, operation_id=retired_by)
                if any(expected):
                    self.emit("retired_nonzero_holding", account_key=key)
                if current:
                    self.emit("retired_native_holding", account_key=key)
            elif not current:
                self.emit("missing_native_holding", account_key=key)
            elif vector(current.get("balance")) != expected or current.get("revision") != revision:
                self.emit("stale_native_balance", account_key=key)
        for key, row in native.items():
            if account_key(key[0])[1] in ORDINARY_KINDS and any(amount < 0 for amount in vector(row.get("balance"))):
                self.emit("negative_native_holding", account_key=key[0])

    def audit_mapping_creations(self, backend: object, lineage: str, native: dict,
                                origins: dict, operations: dict | None = None,
                                effects: dict | None = None, epoch: str | None = None) -> None:
        mappings = native.get("mapping_creations")
        roots = native.get("mapping_creation_roots")
        coverage = native.get("mapping_creation_coverage")
        if mappings is None or roots is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_mapping_creation_coverage", scope="snapshot")
            return
        if (not isinstance(mappings, list) or len(mappings) > MAX_ROWS or
                any(not isinstance(row, dict) for row in mappings) or
                not isinstance(roots, list) or len(roots) > MAX_ROWS or
                any(not isinstance(row, dict) for row in roots)):
            raise SnapshotError("invalid or oversized mapping creation evidence")
        required = {"rows", "creator_rows", "root_rows", "missing_roots"}
        if (not isinstance(coverage, dict) or set(coverage) != required or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or
                coverage["rows"] != len(mappings) or
                coverage["root_rows"] != len(roots)):
            raise SnapshotError("invalid mapping creation coverage")
        baseline_operation_ids = native.get("baseline_operation_ids", [])
        if (not isinstance(baseline_operation_ids, list) or
                len(baseline_operation_ids) > MAX_ROWS or
                any(not isinstance(value, str) or not HEX_ID.fullmatch(value)
                    or value == "0" * 32 for value in baseline_operation_ids) or
                len(set(baseline_operation_ids)) != len(baseline_operation_ids)):
            raise SnapshotError("invalid baseline operation identity coverage")
        baseline_operation_ids = set(baseline_operation_ids)
        # Compare selected creators with their own evidence without rescanning
        # every effect for every root. Keep references; normalize only effects
        # actually consumed by a selected creator below.
        effects_by_operation = defaultdict(dict)
        if effects is not None:
            for key, effect in effects.items():
                if isinstance(key, tuple) and len(key) == 2:
                    effects_by_operation[key[0]][key[1]] = effect
        roots_by_id = {}
        for root in roots:
            operation_id = require_id(root.get("operation_id"), "mapping creator operation ID")
            if operation_id in roots_by_id:
                self.emit("duplicate_mapping_creation_root", operation_id=operation_id)
            roots_by_id[operation_id] = root
            creator_kind = root.get("creator_kind", "accounting_operation")
            if (root.get("lineage") != lineage or
                    not isinstance(root.get("epoch"), str) or
                    not HEX_ID.fullmatch(root["epoch"]) or
                    creator_kind not in ("accounting_operation", "baseline_installation") or
                    type(root.get("reason")) is not int or root["reason"] < 0 or
                    root.get("outcome") not in ("committed", "rejected", "unknown") or
                    type(root.get("result_code")) is not int or
                    type(root.get("account_count")) is not int or root["account_count"] < 0 or
                    not isinstance(root.get("effects"), list) or
                    len(root["effects"]) > MAX_ROWS or
                    type(root.get("item_event_count")) is not int or
                    root["item_event_count"] < 0):
                raise SnapshotError("invalid mapping creation root")
            elif root.get("reason") != 38 and len(root["effects"]) != root["account_count"]:
                self.emit("mapping_creation_effect_count_mismatch", operation_id=operation_id)
            if creator_kind == "baseline_installation" and (
                    root.get("reason") != 38 or root.get("account_count") != 0 or
                    root.get("effects") != [] or type(root.get("installation_phase")) is not int or
                    root.get("installation_selected_epoch") != root.get("epoch") or
                    type(root.get("installation_revision")) is not int or
                    root.get("installation_revision") != 1 or
                    type(root.get("installation_failure_stage")) is not int or
                    root.get("installation_failure_stage") != 0 or
                    type(root.get("installation_committed_at_present")) is not bool or
                    not root.get("installation_committed_at_present") or
                    root.get("baseline_lineage") != lineage or
                    root.get("baseline_epoch") != root.get("epoch") or
                    root.get("baseline_reason") != 38 or
                    root.get("baseline_outcome") != "committed" or
                    root.get("baseline_result_code") != 0):
                self.emit("invalid_mapping_creation_root", operation_id=operation_id)
            elif creator_kind == "accounting_operation" and (
                    type(root.get("creator_inbox_status")) is not int or
                    root.get("creator_inbox_status") != 1 or
                    type(root.get("creator_inbox_result_code")) is not int or
                    root.get("creator_inbox_result_code") != root.get("result_code") or
                    type(root.get("creator_inbox_failure_stage")) is not int or
                    root.get("creator_inbox_failure_stage") != 0 or
                    type(root.get("creator_inbox_committed_at_present")) is not bool or
                    not root.get("creator_inbox_committed_at_present")):
                self.emit("invalid_mapping_creation_root", operation_id=operation_id)
            if (creator_kind == "accounting_operation" and epoch is not None and
                    root.get("epoch") == epoch):
                operation = operations.get((operation_id,)) if operations is not None else None
                operation_fields = ("lineage", "epoch", "reason", "outcome", "result_code",
                                    "source_event", "account_count", "posting_count",
                                    "child_count", "item_event_count")
                if operation is None or any(
                        root.get(field) != operation.get(field) for field in operation_fields):
                    self.emit("invalid_mapping_creation_root", operation_id=operation_id)
                if effects is not None:
                    root_effects = root.get("effects", [])
                    if not all(isinstance(effect, dict) for effect in root_effects):
                        self.emit("invalid_mapping_creation_root", operation_id=operation_id)
                        continue
                    root_effects_by_index = {
                        effect.get("account_index"): effect for effect in root_effects
                    }
                    evidence_effects = {
                        index: {field: value.get(field) for field in (
                            "account_index", "account_key", "before", "after",
                            "before_revision", "after_revision")}
                        for index, value in effects_by_operation.get(operation_id, {}).items()
                    }
                    if (len(root_effects_by_index) != len(root_effects) or
                            root_effects_by_index != evidence_effects):
                        self.emit("invalid_mapping_creation_root", operation_id=operation_id)
        observed_creator_rows = 0
        missing_root_ids = set()
        seen_mapping_ids = set()
        for mapping in mappings:
            mapping_id = mapping.get("mapping_id")
            kind = mapping.get("account_kind")
            context = mapping.get("context_id")
            native_id = mapping.get("native_id")
            active_native_id = mapping.get("active_native_id")
            creator_id = mapping.get("creating_operation_id")
            if (type(mapping_id) is not int or not 0 < mapping_id < 2**64 or
                    mapping_id in seen_mapping_ids or type(kind) is not int or
                    kind not in MAPPED_KINDS or type(context) is not int or
                    not 0 <= context < 2**64 or
                    type(native_id) is not int or not 0 < native_id < 2**64 or
                    (active_native_id is not None and
                     (type(active_native_id) is not int or not 0 < active_native_id < 2**64))):
                raise SnapshotError("invalid mapping creation row")
            seen_mapping_ids.add(mapping_id)
            key = mapping.get("account_key")
            decoded = account_key(key)
            if (decoded[0] != lineage or decoded[1] != kind or
                    decoded[2] != mapping_id or decoded[3] != context):
                raise SnapshotError("mapping creation key mismatch")
            origin = origins.get((key,))
            if creator_id is None:
                if origin is not None and origin.get("origin") == "creation":
                    self.emit("missing_mapping_creator_identity", account_key=key)
                continue
            creator_id = require_id(creator_id, "mapping creator operation ID")
            observed_creator_rows += 1
            root = roots_by_id.get(creator_id)
            if root is None:
                missing_root_ids.add(creator_id)
                self.emit("missing_mapping_creation_root", operation_id=creator_id,
                          account_key=key)
                continue
            if root.get("creator_kind") == "baseline_installation":
                baseline_operation_id = root.get("baseline_operation_id")
                if (origin is None or origin.get("origin") != "baseline" or
                        root.get("installation_phase") != 2 or
                        not isinstance(baseline_operation_id, str) or
                        baseline_operation_id not in baseline_operation_ids or
                        root.get("outcome") != "committed" or root.get("result_code") != 0):
                    self.emit("invalid_mapping_creation_root", operation_id=creator_id,
                              account_key=key)
                continue
            matching_effects = [effect for effect in root["effects"]
                                if isinstance(effect, dict) and effect.get("account_key") == key]
            if origin is not None and origin.get("origin") == "baseline" and root.get("reason") == 38:
                if root.get("outcome") != "committed" or root.get("result_code") != 0:
                    self.emit("invalid_mapping_creation_root", operation_id=creator_id,
                              account_key=key)
                continue
            if (root.get("outcome") != "committed" or root.get("result_code") != 0 or
                    len(matching_effects) != 1):
                self.emit("invalid_mapping_creation_effect", operation_id=creator_id,
                          account_key=key)
                continue
            policy = self.reasons.get(root.get("reason"))
            kind_id = self.kinds.get(kind, {}).get("id")
            if (policy is None or kind_id is None or
                    kind_id not in policy.get("creates_account_kinds", [])):
                self.emit("unauthorized_mapping_creation", operation_id=creator_id,
                          account_key=key)
            effect = matching_effects[0]
            if (not unsigned_revision(effect.get("before_revision")) or
                    not unsigned_revision(effect.get("after_revision")) or
                    effect["after_revision"] <= effect["before_revision"] or
                    ((origin is None or origin.get("origin") == "creation") and
                     (effect.get("before") != [0, 0, 0, 0] or
                      effect.get("before_revision") != 0 or
                      effect["after_revision"] < 1))):
                self.emit("invalid_mapping_creation_origin", operation_id=creator_id,
                          account_key=key)
            if origin is None:
                self.emit("unresolved_postbaseline_account_origin", operation_id=creator_id,
                          account_key=key)
            elif (origin.get("origin") == "creation" and
                  (origin.get("revision") != 0 or origin.get("balance") != [0, 0, 0, 0])):
                self.emit("mapping_creation_origin_mismatch", operation_id=creator_id,
                          account_key=key)
        if (observed_creator_rows != coverage["creator_rows"] or
                len(missing_root_ids) != coverage["missing_roots"] or
                coverage["creator_rows"] > coverage["rows"] or
                coverage["root_rows"] + coverage["missing_roots"] !=
                len({row.get("creating_operation_id") for row in mappings
                     if row.get("creating_operation_id") is not None})):
            raise SnapshotError("mapping creation coverage mismatch")
    def audit_pending_claim_consumers(self, backend: object, lineage: str,
                                      native: dict) -> None:
        rows = native.get("pending_claim_consumers")
        coverage = native.get("pending_claim_consumer_coverage")
        source_records = native.get("pending_claim_sources")
        if rows is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_pending_claim_consumer_coverage", scope="snapshot")
            return
        if source_records is None:
            if backend == "sql_partial":
                self.emit("missing_pending_claim_source_coverage", scope="snapshot")
            source_records = []
        if (not isinstance(source_records, list) or len(source_records) > MAX_ROWS or
                any(not isinstance(row, dict) for row in source_records)):
            raise SnapshotError("invalid or oversized pending claim source rows")
        if (not isinstance(rows, list) or len(rows) > MAX_ROWS or
                any(not isinstance(row, dict) for row in rows)):
            raise SnapshotError("invalid or oversized pending claim consumer evidence")
        if (not isinstance(coverage, dict) or set(coverage) != {
                "rows", "missing_source_rows", "mismatched_source_amounts"} or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or coverage["rows"] != len(rows)):
            raise SnapshotError("invalid pending claim consumer coverage")
        seen = set()
        missing = mismatched = 0
        source_totals = defaultdict(lambda: [0, 0])
        for source in source_records:
            consumer_id = source.get("claim_operation_id")
            if consumer_id is None:
                continue
            require_id(consumer_id, "pending claim source consumer operation ID")
            amount = source.get("amount")
            if type(amount) is not int or not 0 < amount < 2**63:
                raise SnapshotError("invalid pending claim source amount")
            source_totals[consumer_id][0] += 1
            source_totals[consumer_id][1] += amount
            if source_totals[consumer_id][1] >= 2**63:
                raise SnapshotError("pending claim source amount overflow")
        for row in rows:
            operation_id = require_id(row.get("operation_id"),
                                      "pending claim consumer operation ID")
            epoch = row.get("epoch")
            effects = row.get("pending_claim_debits")
            source_row_count = row.get("source_rows")
            source_amount = row.get("source_amount")
            inbox_receipt = row.get("inbox_receipt")
            if (operation_id in seen or not isinstance(epoch, str) or not HEX_ID.fullmatch(epoch) or
                    row.get("outcome") != "committed" or type(row.get("result_code")) is not int or
                    row["result_code"] != 0 or type(source_row_count) is not int or
                    source_row_count < 0 or
                    type(source_amount) is not int or source_amount < 0 or
                    (source_row_count == 0 and source_amount != 0) or
                    not isinstance(inbox_receipt, dict) or
                    set(inbox_receipt) != {"status", "result_code", "failure_stage",
                                           "committed_at_present"} or
                    type(inbox_receipt.get("status")) is not int or
                    inbox_receipt.get("status") != 1 or
                    type(inbox_receipt.get("result_code")) is not int or
                    inbox_receipt.get("result_code") != row.get("result_code") or
                    type(inbox_receipt.get("failure_stage")) is not int or
                    inbox_receipt.get("failure_stage") != 0 or
                    type(inbox_receipt.get("committed_at_present")) is not bool or
                    not inbox_receipt.get("committed_at_present") or
                    not isinstance(effects, list) or not effects or len(effects) > MAX_ROWS):
                raise SnapshotError("invalid pending claim consumer root")
            seen.add(operation_id)
            exported_source_count, exported_source_amount = source_totals.pop(
                operation_id, (0, 0))
            if (exported_source_count != source_row_count or
                    exported_source_amount != source_amount):
                self.emit("pending_claim_consumer_source_coverage_mismatch",
                          operation_id=operation_id,
                          root_source_rows=source_row_count,
                          exported_source_rows=exported_source_count,
                          root_source_amount=source_amount,
                          exported_source_amount=exported_source_amount)
            total_debit = 0
            seen_accounts = set()
            for effect in effects:
                if not isinstance(effect, dict):
                    raise SnapshotError("invalid pending claim consumer effect")
                key = effect.get("account_key")
                decoded = account_key(key)
                amount = effect.get("amount")
                before = vector(effect.get("before"))
                after = vector(effect.get("after"))
                if (decoded[0] != lineage or decoded[1] != 5 or key in seen_accounts or
                        type(amount) is not int or not 0 < amount < 2**63 or
                        before[0] - after[0] != amount or any(before[1:]) or any(after[1:])):
                    raise SnapshotError("invalid pending claim consumer debit")
                seen_accounts.add(key)
                total_debit += amount
                if total_debit >= 2**63:
                    raise SnapshotError("pending claim consumer debit overflow")
            if source_row_count == 0:
                missing += 1
                self.emit("pending_claim_consumer_without_sources", operation_id=operation_id,
                          debit_copper=total_debit)
            elif source_amount != total_debit:
                mismatched += 1
                self.emit("pending_claim_consumer_amount_mismatch", operation_id=operation_id,
                          source_amount=source_amount, debit_copper=total_debit)
        for operation_id, (source_count, source_amount) in source_totals.items():
            self.emit("missing_pending_claim_consumer_root", operation_id=operation_id,
                      source_rows=source_count, source_amount=source_amount)
        if (coverage["missing_source_rows"] != missing or
                coverage["mismatched_source_amounts"] != mismatched):
            raise SnapshotError("pending claim consumer coverage mismatch")

    def audit_mapping_retirements(self, lineage: str, epoch: str, backend: object,
                                  operations: dict,
                                  by_account: dict, origins: dict, native: dict,
                                  native_holdings: dict) -> None:
        retirements = native.get("retired_mappings")
        coverage = native.get("retirement_coverage")
        retirement_roots = native.get("retirement_roots")
        if retirements is None and backend != "sql_partial":
            return
        if retirements is None:
            self.emit("missing_retirement_coverage", scope="snapshot")
            return
        if (not isinstance(retirements, list) or len(retirements) > MAX_ROWS or
                any(not isinstance(row, dict) for row in retirements)):
            raise SnapshotError("invalid or oversized retired_mappings")
        if (not isinstance(coverage, dict) or set(coverage) != {
                "rows", "current_epoch_rows", "matched_opening_origins",
                "unmatched_current_epoch_rows", "root_rows"} or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or
                coverage["rows"] != len(retirements) or
                coverage["matched_opening_origins"] + coverage["unmatched_current_epoch_rows"] !=
                coverage["current_epoch_rows"]):
            raise SnapshotError("invalid mapping retirement coverage")
        if (not isinstance(retirement_roots, list) or len(retirement_roots) > MAX_ROWS or
                len(retirement_roots) != coverage["root_rows"] or
                any(not isinstance(row, dict) for row in retirement_roots)):
            raise SnapshotError("invalid or oversized retirement_roots")
        mapping_creations = native.get("mapping_creations")
        has_mapping_creation_evidence = isinstance(mapping_creations, list)
        creation_native_ids = {}
        if has_mapping_creation_evidence:
            creation_native_ids = {
                row.get("account_key"): row.get("native_id")
                for row in mapping_creations if isinstance(row, dict)
            }
        roots_by_id = {}
        for root in retirement_roots:
            operation_id = require_id(root.get("operation_id"),
                                      "retirement operation ID")
            root_lineage = root.get("lineage")
            root_epoch = root.get("epoch")
            root_reason = root.get("reason")
            account_count = root.get("account_count")
            posting_count = root.get("posting_count")
            child_count = root.get("child_count")
            item_event_count = root.get("item_event_count")
            root_effects = root.get("effects")
            root_postings = root.get("postings")
            if (operation_id in roots_by_id or
                    (root_lineage is not None and
                     (not isinstance(root_lineage, str) or not HEX_ID.fullmatch(root_lineage))) or
                    (root_epoch is not None and
                     (not isinstance(root_epoch, str) or not HEX_ID.fullmatch(root_epoch))) or
                    (root_reason is not None and
                     (type(root_reason) is not int or not 1 <= root_reason <= 65535)) or
                    root.get("outcome") not in ("committed", "rejected", "unknown") or
                    (root.get("result_code") is not None and
                     type(root.get("result_code")) is not int) or
                    any(value is not None and (type(value) is not int or not 0 <= value < 2**63)
                        for value in (account_count, posting_count, child_count, item_event_count)) or
                    not isinstance(root_effects, list) or len(root_effects) > MAX_ROWS or
                    not isinstance(root_postings, list) or len(root_postings) > MAX_ROWS):
                raise SnapshotError("invalid retirement root")
            if (any(value is None for value in
                    (root_lineage, root_epoch, root_reason, root.get("result_code"), account_count,
                     posting_count, child_count, item_event_count)) or
                    len(root_effects) != account_count or len(root_postings) != posting_count):
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id)
            effects_by_index = {}
            effects_by_key = {}
            for effect in root_effects:
                if not isinstance(effect, dict):
                    raise SnapshotError("invalid retirement root effect")
                account_index = effect.get("account_index")
                key = effect.get("account_key")
                key_lineage, _, _, _ = account_key(key)
                before = vector(effect.get("before"))
                after = vector(effect.get("after"))
                before_revision = effect.get("before_revision")
                after_revision = effect.get("after_revision")
                if (type(account_index) is not int or not 0 <= account_index < 3072 or
                        account_index in effects_by_index or key in effects_by_key or
                        (root_lineage is not None and key_lineage != root_lineage) or
                        not unsigned_revision(before_revision) or
                        not unsigned_revision(after_revision) or
                        after_revision <= before_revision):
                    raise SnapshotError("invalid retirement root effect")
                effects_by_index[account_index] = effect
                effects_by_key[key] = effect
            posting_indexes = set()
            posting_deltas = defaultdict(lambda: [0, 0, 0, 0])
            net_copper = 0
            for posting in root_postings:
                if not isinstance(posting, dict):
                    raise SnapshotError("invalid retirement root posting")
                line_index = posting.get("line_index")
                account_index = posting.get("account_index")
                child_index = posting.get("child_index")
                delta = vector(posting.get("delta"))
                copper_value = posting.get("copper_value")
                if (type(line_index) is not int or not 0 <= line_index < 6144 or
                        line_index in posting_indexes or account_index not in effects_by_index or
                        type(child_index) is not int or not 0 <= child_index <= 64 or
                        type(copper_value) is not int or copper(delta) != copper_value):
                    raise SnapshotError("invalid retirement root posting")
                posting_indexes.add(line_index)
                net_copper += copper_value
                for index, amount in enumerate(delta):
                    posting_deltas[account_index][index] += amount
            if net_copper != 0 or child_count != 0 or item_event_count != 0:
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id)
            for account_index, effect in effects_by_index.items():
                before = vector(effect["before"])
                after = vector(effect["after"])
                if tuple(a - b for a, b in zip(after, before)) != tuple(
                        posting_deltas[account_index]):
                    self.emit("invalid_mapping_retirement_root", operation_id=operation_id,
                              account_key=effect["account_key"])
            if (root.get("outcome") != "committed" or root.get("result_code") != 0 or
                    root_lineage != lineage):
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id)
            if (type(root.get("retirement_inbox_status")) is not int or
                    root.get("retirement_inbox_status") != 1 or
                    type(root.get("retirement_inbox_result_code")) is not int or
                    root.get("retirement_inbox_result_code") != root.get("result_code") or
                    type(root.get("retirement_inbox_failure_stage")) is not int or
                    root.get("retirement_inbox_failure_stage") != 0 or
                    type(root.get("retirement_inbox_committed_at_present")) is not bool or
                    not root.get("retirement_inbox_committed_at_present")):
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id)
            if root_epoch == epoch:
                operation = operations.get((operation_id,))
                if (operation is None or operation.get("lineage") != root_lineage or
                        operation.get("epoch") != root_epoch or
                        operation.get("reason") != root_reason or
                        operation.get("outcome") != root.get("outcome") or
                        operation.get("result_code") != root.get("result_code") or
                        operation.get("account_count") != account_count or
                        operation.get("posting_count") != posting_count):
                    self.emit("invalid_mapping_retirement_root", operation_id=operation_id)
            roots_by_id[operation_id] = root
        seen_mappings = set()
        current_epoch_rows = 0
        matched_origins = 0
        unmatched_origins = 0
        current_epoch_retirements = set()
        for row in retirements:
            mapping_id = row.get("mapping_id")
            native_id = row.get("native_id")
            active_native_id = row.get("active_native_id")
            key = row.get("account_key")
            operation_id = require_id(row.get("retiring_operation_id"),
                                      "mapping retirement operation ID")
            operation_lineage = row.get("operation_lineage")
            operation_epoch = row.get("operation_epoch")
            operation = operations.get((operation_id,))
            key_lineage, kind, identity, _ = account_key(key)
            if (type(mapping_id) is not int or not 0 < mapping_id < 2**64 or
                    type(native_id) is not int or not 0 < native_id < 2**64 or
                    active_native_id is not None or identity != mapping_id or
                    kind not in MAPPED_KINDS or key_lineage != lineage or
                    (operation_lineage is not None and
                     (not isinstance(operation_lineage, str) or
                      not HEX_ID.fullmatch(operation_lineage))) or
                    (operation_epoch is not None and
                     (not isinstance(operation_epoch, str) or
                      not HEX_ID.fullmatch(operation_epoch))) or
                    row.get("operation_outcome") not in ("committed", "rejected", "unknown") or
                    (row.get("operation_result_code") is not None and
                     type(row.get("operation_result_code")) is not int)):
                raise SnapshotError("invalid retired mapping")
            if mapping_id in seen_mappings:
                self.emit("duplicate_retired_mapping", account_key=key)
            seen_mappings.add(mapping_id)
            if (has_mapping_creation_evidence and
                    creation_native_ids.get(key) != native_id):
                self.emit("mapping_retirement_identity_mismatch", account_key=key,
                          operation_id=operation_id)
            if (operation_lineage != lineage or operation_epoch is None or
                    row.get("operation_outcome") != "committed" or
                    row.get("operation_result_code") != 0):
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id,
                          account_key=key)
            root = roots_by_id.get(operation_id)
            if (root is None or root.get("lineage") != operation_lineage or
                    root.get("epoch") != operation_epoch or
                    root.get("outcome") != row.get("operation_outcome") or
                    root.get("result_code") != row.get("operation_result_code")):
                self.emit("invalid_mapping_retirement_root", operation_id=operation_id,
                          account_key=key)
            policy = self.reasons.get(root.get("reason")) if root is not None else None
            if (policy is None or self.kinds[kind]["id"] not in
                    policy.get("retires_account_kinds", [])):
                self.emit("unauthorized_mapping_retirement", operation_id=operation_id,
                          account_key=key)
            if root is not None:
                retired_effects = [effect for effect in root["effects"]
                                   if effect.get("account_key") == key]
                if (len(retired_effects) != 1 or
                        vector(retired_effects[0].get("after")) != (0, 0, 0, 0)):
                    self.emit("invalid_mapping_retirement_effect", operation_id=operation_id,
                              account_key=key)
            if operation_epoch == epoch:
                if (operation is None or operation.get("lineage") != lineage or
                        operation.get("epoch") != epoch or
                        operation.get("outcome") != row.get("operation_outcome") or
                        operation.get("result_code") != row.get("operation_result_code")):
                    self.emit("invalid_mapping_retirement_root", operation_id=operation_id,
                              account_key=key)
                current_epoch_rows += 1
                current_epoch_retirements.add((operation_id, key))
                origin = origins.get((key,))
                if origin is None:
                    unmatched_origins += 1
                    self.emit("unmatched_mapping_retirement", operation_id=operation_id,
                              account_key=key)
                else:
                    matched_origins += 1
                    if origin.get("retired_by") != operation_id:
                        self.emit("mapping_retirement_origin_mismatch", operation_id=operation_id,
                                  account_key=key)
                effect_rows = [effect for effect in by_account.get(key, [])
                               if effect.get("operation_id") == operation_id]
                if (len(effect_rows) != 1 or
                        vector(effect_rows[0].get("after")) != (0, 0, 0, 0)):
                    self.emit("invalid_mapping_retirement_effect", operation_id=operation_id,
                              account_key=key)
                if (key,) in native_holdings:
                    self.emit("retired_native_holding", account_key=key,
                              operation_id=operation_id)
        if (coverage["current_epoch_rows"] != current_epoch_rows or
                coverage["matched_opening_origins"] != matched_origins or
                coverage["unmatched_current_epoch_rows"] != unmatched_origins):
            raise SnapshotError("mapping retirement coverage mismatch")
        for (key,), origin in origins.items():
            operation_id = origin.get("retired_by")
            if (operation_id and account_key(key)[1] in MAPPED_KINDS and
                    (operation_id, key) not in current_epoch_retirements):
                if (operation_id,) in operations:
                    self.emit("missing_retired_mapping_evidence", operation_id=operation_id,
                              account_key=key)

    def audit_lineage_uid_references(self, lineage: str, epoch: str, backend: object,
                                     native: dict, current_references: dict) -> None:
        references = native.get("lineage_uid_references")
        roots = native.get("lineage_uid_reference_roots")
        coverage = native.get("lineage_uid_reference_coverage")
        if references is None and backend != "sql_partial":
            return
        if references is None:
            self.emit("missing_lineage_uid_reference_coverage", scope="snapshot")
            return
        if (not isinstance(references, list) or len(references) > MAX_ROWS or
                any(not isinstance(row, dict) for row in references) or
                not isinstance(roots, list) or len(roots) > MAX_ROWS or
                any(not isinstance(row, dict) for row in roots)):
            raise SnapshotError("invalid or oversized lineage UID reference evidence")
        if (not isinstance(coverage, dict) or set(coverage) != {"rows", "root_rows"} or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or
                coverage["rows"] != len(references) or coverage["root_rows"] != len(roots)):
            raise SnapshotError("invalid lineage UID reference coverage")
        roots_by_id = {}
        for root in roots:
            operation_id = require_id(root.get("operation_id"),
                                      "lineage item operation ID")
            operation_epoch = root.get("epoch")
            item_event_count = root.get("item_event_count")
            reference_count = root.get("reference_count")
            reason = root.get("reason")
            source_event = root.get("source_event")
            inbox_receipt = root.get("inbox_receipt")
            if (operation_id in roots_by_id or not isinstance(operation_epoch, str) or
                    not HEX_ID.fullmatch(operation_epoch) or
                    root.get("outcome") not in ("committed", "rejected", "unknown") or
                    type(item_event_count) is not int or not 0 <= item_event_count < 2**63 or
                    type(reference_count) is not int or not 0 <= reference_count < 2**63 or
                    (backend == "sql_partial" and type(reason) is not int) or
                    (reason is not None and (type(reason) is not int or reason < 0)) or
                    (source_event is not None and
                     (not isinstance(source_event, str) or
                      not HEX_SOURCE_EVENT.fullmatch(source_event))) or
                    (backend == "sql_partial" and "source_event" not in root)):
                raise SnapshotError("invalid lineage UID reference root")
            if source_event is not None:
                try:
                    kind = decode_source_event(source_event)[0]
                except SnapshotError:
                    self.emit("invalid_lineage_uid_reference_root", operation_id=operation_id)
                else:
                    if not source_kind_allowed(reason, kind):
                        self.emit("unauthorized_lineage_uid_source", operation_id=operation_id)
            if backend == "sql_partial" and (
                    not isinstance(inbox_receipt, dict) or
                    set(inbox_receipt) != {"status", "result_code", "failure_stage",
                                           "committed_at_present"} or
                    type(inbox_receipt.get("status")) is not int or
                    inbox_receipt.get("status") != 1 or
                    type(inbox_receipt.get("result_code")) is not int or
                    inbox_receipt.get("result_code") != root.get("result_code") or
                    type(inbox_receipt.get("failure_stage")) is not int or
                    inbox_receipt.get("failure_stage") != 0 or
                    type(inbox_receipt.get("committed_at_present")) is not bool or
                    not inbox_receipt.get("committed_at_present") or
                    root.get("outcome") != "committed" or root.get("result_code") != 0):
                self.emit("invalid_lineage_uid_reference_root", operation_id=operation_id)
            if (root.get("outcome") != "committed" and
                    (item_event_count != 0 or reference_count != 0)):
                self.emit("invalid_lineage_uid_reference_root", operation_id=operation_id)
            if item_event_count != reference_count:
                self.emit("lineage_uid_reference_count_mismatch", operation_id=operation_id)
            roots_by_id[operation_id] = root
        seen_references = set()
        seen_legacy_events = set()
        observed_by_root = Counter()
        current_scope = set()
        for ref in references:
            operation_id = require_id(ref.get("operation_id"),
                                      "lineage item reference operation ID")
            event_index = ref.get("event_index")
            uid = ref.get("uid")
            child_index = ref.get("child_index")
            before_revision = ref.get("before_revision")
            after_revision = ref.get("after_revision")
            legacy_operation_id = require_id(ref.get("legacy_operation_id"),
                                             "legacy UID event operation ID")
            legacy_event_index = ref.get("legacy_event_index")
            operation_epoch = ref.get("operation_epoch")
            root = roots_by_id.get(operation_id)
            if (type(event_index) is not int or not 0 <= event_index <= 65535 or
                    type(uid) is not int or not 0 < uid < 2**64 or
                    type(child_index) is not int or not 0 <= child_index <= 64 or
                    not item_revision_transition(before_revision, after_revision) or
                    type(legacy_event_index) is not int or not 0 <= legacy_event_index <= 65535 or
                    not isinstance(operation_epoch, str) or not HEX_ID.fullmatch(operation_epoch) or
                    root is None or root.get("epoch") != operation_epoch or
                    root.get("outcome") != ref.get("operation_outcome") or
                    root.get("item_event_count") != ref.get("operation_item_event_count")):
                raise SnapshotError("invalid lineage UID reference")
            reference_key = (operation_id, event_index)
            legacy_key = (legacy_operation_id, legacy_event_index)
            if reference_key in seen_references:
                self.emit("duplicate_lineage_uid_reference", operation_id=operation_id,
                          uid=uid)
            if legacy_key in seen_legacy_events:
                self.emit("duplicate_lineage_uid_event_reference", operation_id=operation_id,
                          uid=uid)
            seen_references.add(reference_key)
            seen_legacy_events.add(legacy_key)
            observed_by_root[operation_id] += 1
            ledger_uid = ref.get("ledger_uid")
            ledger_before = ref.get("ledger_before_revision")
            ledger_revision = ref.get("ledger_revision")
            ledger_root = ref.get("ledger_root")
            ledger_parent = ref.get("ledger_parent")
            owner = ref.get("ledger_owner")
            if (type(ledger_uid) is not int or ledger_uid != uid or
                    type(ledger_before) is not int or ledger_before != before_revision or
                    type(ledger_revision) is not int or ledger_revision != after_revision or
                    type(ledger_root) is not int or not 0 < ledger_root < 2**64 or
                    (ledger_parent is not None and
                     (type(ledger_parent) is not int or not 0 < ledger_parent < 2**64)) or
                    not isinstance(owner, list) or len(owner) != 3 or
                    any(type(value) is not int or value < 0 for value in owner) or
                    ref.get("ledger_state") not in ("live", "tombstone") or
                    ref.get("ledger_action") not in ("create", "destroy", "move")):
                self.emit("lineage_orphan_uid_reference", operation_id=operation_id, uid=uid)
            if operation_epoch == epoch:
                current_scope.add((operation_id, event_index, uid, before_revision,
                                   after_revision, legacy_operation_id, legacy_event_index,
                                   child_index))
        for operation_id, root in roots_by_id.items():
            if observed_by_root[operation_id] != root["reference_count"]:
                self.emit("lineage_uid_reference_count_mismatch", operation_id=operation_id)
        current_expected = {
            (row.get("operation_id"), row.get("event_index"), row.get("uid"),
             row.get("before_revision"), row.get("after_revision"),
             row.get("legacy_operation_id"), row.get("legacy_event_index"),
             row.get("child_index", 0))
            for row in current_references.values()}
        if current_scope != current_expected:
            self.emit("lineage_uid_reference_scope_mismatch", scope="snapshot")

    def audit_lineage_uid_history(self, backend: object, native: dict,
                                  origins: dict, current: dict) -> None:
        events = native.get("uid_history_events")
        if events is None and backend != "sql_partial":
            return
        if (not isinstance(events, list) or len(events) > MAX_ROWS or
                any(not isinstance(row, dict) for row in events)):
            self.emit("missing_lineage_uid_history", scope="snapshot")
            return
        coverage = native.get("uid_event_coverage")
        unreferenced_events = native.get("unreferenced_uid_events")
        if coverage is None or unreferenced_events is None:
            if backend == "sql_partial":
                self.emit("missing_uid_event_coverage", scope="snapshot")
        elif (not isinstance(coverage, dict) or set(coverage) != {
                    "tracked_uids", "ledger_events", "referenced_events", "unreferenced_events"} or
              any(type(value) is not int or not 0 <= value < 2**63
                  for value in coverage.values()) or
              not isinstance(unreferenced_events, list) or
              len(unreferenced_events) > MAX_ROWS or
              any(not isinstance(row, dict) for row in unreferenced_events)):
            raise SnapshotError("invalid UID history coverage")
        else:
            referenced_count = sum(row.get("referenced") is True for row in events)
            unreferenced_count = sum(row.get("referenced") is False for row in events)
            if (coverage["ledger_events"] != len(events) or
                    coverage["referenced_events"] != referenced_count or
                    coverage["unreferenced_events"] != unreferenced_count):
                self.emit("lineage_uid_history_coverage_mismatch", scope="snapshot")
            expected_unreferenced = [row for row in events
                                     if row.get("referenced") is False]
            if unreferenced_events != expected_unreferenced:
                self.emit("unreferenced_uid_history_scope_mismatch", scope="snapshot")
        refs = native.get("lineage_uid_references", [])
        roots = native.get("lineage_uid_reference_roots")
        roots_by_id = {}
        if isinstance(roots, list):
            roots_by_id = {root.get("operation_id"): root for root in roots
                           if isinstance(root, dict)}
        expected = {}
        for ref in refs:
            key = (ref.get("legacy_operation_id"), ref.get("legacy_event_index"), ref.get("uid"))
            uid_origin = origins.get((ref.get("uid"),))
            origin_revision = uid_origin.get("revision", 0) if uid_origin else 0
            if ref.get("after_revision", -1) > origin_revision:
                expected[key] = (ref.get("before_revision"), ref.get("after_revision"))
        by_uid = defaultdict(list)
        seen_events = set()
        seen_revisions = set()
        for row in events:
            operation_id = require_id(row.get("operation_id"), "UID history operation ID")
            event_index = row.get("event_index")
            uid = row.get("uid")
            before = row.get("before_revision")
            revision = row.get("revision")
            owner = row.get("owner")
            operation_epoch = row.get("operation_epoch")
            root = roots_by_id.get(operation_id)
            if backend == "sql_partial" and (
                    root is None or not isinstance(operation_epoch, str) or
                    not HEX_ID.fullmatch(operation_epoch) or
                    root.get("epoch") != operation_epoch or
                    root.get("outcome") != row.get("operation_outcome")):
                self.emit("invalid_uid_history_root", operation_id=operation_id, uid=uid)
            if (type(event_index) is not int or not 0 <= event_index <= 65535 or
                    type(uid) is not int or not 0 < uid < 2**64 or
                    not item_revision_transition(before, revision) or
                    type(row.get("referenced")) is not bool or
                    type(row.get("root")) is not int or not 0 < row["root"] < 2**64 or
                    (row.get("parent") is not None and
                     (type(row["parent"]) is not int or row["parent"] <= 0)) or
                    not isinstance(owner, list) or len(owner) != 3 or
                    any(type(value) is not int or value < 0 for value in owner) or
                    row.get("state") not in ("live", "tombstone") or
                    row.get("action") not in ("create", "destroy", "move") or
                    row.get("operation_outcome") not in ("committed", "rejected", "unknown")):
                raise SnapshotError("invalid lineage UID history event")
            if row["operation_outcome"] != "committed":
                self.emit("uid_history_operation_not_committed", operation_id=operation_id,
                          uid=uid)
            key = (operation_id, event_index, uid)
            if key in seen_events:
                self.emit("duplicate_lineage_uid_event", operation_id=operation_id, uid=uid)
            seen_events.add(key)
            ref_revisions = expected.get(key)
            if row["referenced"] != (ref_revisions is not None):
                self.emit("lineage_uid_event_reference_mismatch", operation_id=operation_id, uid=uid)
            elif ref_revisions is not None and ref_revisions != (before, revision):
                self.emit("lineage_uid_event_revision_mismatch", operation_id=operation_id, uid=uid)
            revision_key = (uid, revision)
            if revision_key in seen_revisions:
                self.emit("duplicate_uid_revision", uid=uid)
            seen_revisions.add(revision_key)
            by_uid[uid].append(row)
        for ref_key in expected:
            if ref_key not in seen_events:
                self.emit("missing_lineage_uid_event", uid=ref_key[2])
        for uid, rows in by_uid.items():
            rows.sort(key=lambda row: (row["revision"], row["operation_id"], row["event_index"]))
            origin = origins.get((uid,))
            if not origin or origin.get("origin") not in ("baseline", "creation"):
                self.emit("unknown_legacy_origin", uid=uid)
                continue
            self.audit_item_lifetime(uid, origin, rows)
            state = {field: origin.get(field)
                     for field in ("revision", "root", "parent", "owner", "state")}
            for row in rows:
                if row["before_revision"] != state["revision"]:
                    self.emit("broken_uid_history", uid=uid, operation_id=row["operation_id"])
                state = {"revision": row["revision"], "root": row["root"],
                         "parent": row["parent"], "owner": row["owner"], "state": row["state"]}
            current_item = current.get((uid,))
            if current_item and any(current_item.get(field) != state[field] for field in state):
                self.emit("stale_native_item", uid=uid)

    def audit_uid_scope_coverage(self, backend: object, native: dict,
                                 origins: dict, current: dict, references: dict) -> None:
        uids = native.get("unanchored_ownership_uids")
        ambiguous_uids = native.get("ambiguous_lineage_ownership_uids")
        coverage = native.get("uid_scope_coverage")
        if uids is None or ambiguous_uids is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_uid_scope_coverage", scope="snapshot")
            return
        if (not isinstance(uids, list) or len(uids) > MAX_ROWS or
                any(type(uid) is not int or not 0 < uid < 2**64 for uid in uids) or
                len(set(uids)) != len(uids) or uids != sorted(uids)):
            raise SnapshotError("invalid unanchored ownership UID list")
        if (not isinstance(ambiguous_uids, list) or len(ambiguous_uids) > MAX_ROWS or
                any(type(uid) is not int or not 0 < uid < 2**64 for uid in ambiguous_uids) or
                len(set(ambiguous_uids)) != len(ambiguous_uids) or
                ambiguous_uids != sorted(ambiguous_uids)):
            raise SnapshotError("invalid ambiguous ownership UID list")
        fields = {"ownership_uid_count", "anchored_ownership_uid_count",
                  "unanchored_ownership_uid_count", "other_lineage_ownership_uid_count",
                  "unattributed_ownership_uid_count",
                  "ambiguous_lineage_ownership_uid_count"}
        if (not isinstance(coverage, dict) or set(coverage) != fields or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or
                coverage["unanchored_ownership_uid_count"] != len(uids) or
                coverage["ambiguous_lineage_ownership_uid_count"] != len(ambiguous_uids) or
                coverage["anchored_ownership_uid_count"] + len(uids) !=
                coverage["ownership_uid_count"]):
            raise SnapshotError("invalid UID scope coverage")
        referenced_uids = {row.get("uid") for row in references.values()}
        anchored = ({key[0] for key in origins} | {key[0] for key in current} |
                    referenced_uids)
        if anchored.intersection(uids):
            raise SnapshotError("UID scope marks an anchored UID as unanchored")
        self.emit_count("unanchored_ownership_uid", len(uids))
        self.emit_count("unattributed_ownership_uid",
                        coverage["unattributed_ownership_uid_count"])
        for uid in ambiguous_uids:
            self.emit("ambiguous_lineage_ownership_uid", uid=uid)

    def audit_unattributed_uid_history(self, backend: object, native: dict) -> None:
        events = native.get("unattributed_uid_events")
        coverage = native.get("unattributed_uid_event_coverage")
        if events is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_unattributed_uid_history", scope="snapshot")
            return
        if (not isinstance(events, list) or len(events) > MAX_ROWS or
                any(not isinstance(row, dict) for row in events)):
            raise SnapshotError("invalid or oversized unattributed UID history")
        if (not isinstance(coverage, dict) or set(coverage) != {"uids", "events"} or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or coverage["events"] != len(events) or
                coverage["uids"] != len({row.get("uid") for row in events})):
            raise SnapshotError("invalid unattributed UID history coverage")
        seen = set()
        for row in events:
            operation_id = require_id(row.get("operation_id"),
                                      "unattributed UID operation ID")
            event_index = row.get("event_index")
            uid = row.get("uid")
            before = row.get("before_revision")
            revision = row.get("revision")
            owner = row.get("owner")
            root = row.get("root")
            parent = row.get("parent")
            if (type(event_index) is not int or not 0 <= event_index <= 65535 or
                    type(uid) is not int or not 0 < uid < 2**64 or
                    not item_revision_transition(before, revision) or
                    type(root) is not int or not 0 < root < 2**64 or
                    (parent is not None and (type(parent) is not int or not 0 < parent < 2**64)) or
                    not isinstance(owner, list) or len(owner) != 3 or
                    any(type(value) is not int or value < 0 for value in owner) or
                    row.get("state") not in ("live", "tombstone") or
                    row.get("action") not in ("create", "destroy", "move")):
                raise SnapshotError("invalid unattributed UID history event")
            key = (operation_id, event_index, uid)
            if key in seen:
                raise SnapshotError("duplicate unattributed UID history event")
            seen.add(key)
            self.emit("unattributed_ownership_event", operation_id=operation_id, uid=uid)

    def audit_coin_pile_mappings(self, backend: object, native: dict,
                                 holdings: dict, items: dict, lineage: str) -> None:
        piles = native.get("coin_piles")
        mappings = native.get("coin_pile_mappings")
        coverage = native.get("coin_pile_coverage")
        if piles is None or mappings is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_coin_pile_mapping_coverage", scope="snapshot")
            return
        if (not isinstance(piles, list) or len(piles) > MAX_ROWS or
                any(not isinstance(row, dict) for row in piles) or
                not isinstance(mappings, list) or len(mappings) > MAX_ROWS or
                any(not isinstance(row, dict) for row in mappings)):
            raise SnapshotError("invalid or oversized coin-pile mapping snapshot")
        pile_by_uid = {}
        live_piles = set()
        for pile in piles:
            uid = pile.get("uid")
            state = pile.get("state")
            if type(uid) is not int or not 0 < uid < 2**64 or state not in ("live", "tombstone"):
                raise SnapshotError("invalid coin-pile row")
            if uid in pile_by_uid:
                self.emit("duplicate_coin_pile_uid", uid=uid)
                continue
            item = items.get((uid,))
            if (item is None or item.get("owner") != pile.get("owner") or
                    item.get("revision") != pile.get("revision") or
                    item.get("state") != state):
                self.emit("coin_pile_item_mismatch", uid=uid)
            amounts = pile.get("amounts")
            if amounts is not None:
                amounts = vector(amounts)
                if any(amount < 0 for amount in amounts):
                    raise SnapshotError("negative coin-pile denomination")
            pile_by_uid[uid] = pile
            if state == "live":
                live_piles.add(uid)
        mapping_counts = Counter()
        dangling = invalid = 0
        seen_mapping_keys = set()
        for mapping in mappings:
            key = mapping.get("account_key")
            decoded = account_key(key)
            uid = mapping.get("uid")
            item_exists = mapping.get("item_exists")
            holding_valid = mapping.get("holding_valid")
            if (decoded[0] != lineage or decoded[1] != 3 or
                    type(uid) is not int or not 0 < uid < 2**64 or
                    type(item_exists) is not bool or type(holding_valid) is not bool):
                raise SnapshotError("invalid coin-pile mapping row")
            if key in seen_mapping_keys:
                self.emit("duplicate_coin_pile_mapping", uid=uid)
                continue
            seen_mapping_keys.add(key)
            if not item_exists:
                dangling += 1
                if holding_valid:
                    raise SnapshotError("dangling coin-pile mapping has a valid holding")
                self.emit("dangling_coin_pile_mapping", uid=uid)
                continue
            mapping_counts[uid] += 1
            pile = pile_by_uid.get(uid)
            if pile is None or pile.get("state") != "live":
                self.emit("coin_pile_mapping_without_live_item", uid=uid)
            if not holding_valid:
                invalid += 1
                self.emit("invalid_coin_pile_mapping", uid=uid)
                continue
            balance = vector(mapping.get("balance"))
            revision = mapping.get("revision")
            holding = holdings.get((key,))
            if (not unsigned_revision(revision) or
                    holding is None or holding.get("balance") != list(balance) or
                    holding.get("revision") != revision or pile is None or
                    pile.get("amounts") != list(balance) or pile.get("revision") != revision):
                self.emit("coin_pile_holding_mismatch", uid=uid)
        mapped_live = sum(bool(mapping_counts[uid]) for uid in live_piles)
        unmapped_live = sum(not mapping_counts[uid] for uid in live_piles)
        multiply_mapped_live = sum(max(0, mapping_counts[uid] - 1) for uid in live_piles)
        required = {"rows", "payload_rows", "missing_payload_rows", "mapped_live_rows",
                    "unmapped_live_rows", "multiply_mapped_live_rows",
                    "dangling_mappings", "invalid_mappings"}
        if (not isinstance(coverage, dict) or set(coverage) != required or
                any(type(value) is not int or not 0 <= value < 2**63
                    for value in coverage.values()) or
                coverage["rows"] != len(piles) or
                coverage["payload_rows"] + coverage["missing_payload_rows"] != len(piles) or
                coverage["mapped_live_rows"] != mapped_live or
                coverage["unmapped_live_rows"] != unmapped_live or
                coverage["multiply_mapped_live_rows"] != multiply_mapped_live or
                coverage["dangling_mappings"] != dangling or
                coverage["invalid_mappings"] != invalid):
            raise SnapshotError("invalid coin-pile mapping coverage")
        self.emit_count("unmapped_coin_pile", unmapped_live)
        self.emit_count("multiply_mapped_coin_pile", multiply_mapped_live)

    def audit_coin_pile_lifecycle_sources(self, backend: object, native: dict) -> None:
        roots = native.get("lineage_uid_reference_roots")
        references = native.get("lineage_uid_references")
        mappings = native.get("mapping_creations")
        if roots is None or references is None or mappings is None:
            if backend == "sql_partial":
                self.emit("missing_coin_pile_lifecycle_source_coverage", scope="snapshot")
            return
        pile_uid_by_mapping = {row.get("mapping_id"): row.get("native_id")
                               for row in mappings
                               if isinstance(row, dict) and row.get("account_kind") == 3}
        created_pile_uids_by_operation = defaultdict(set)
        for mapping in mappings:
            if (isinstance(mapping, dict) and mapping.get("account_kind") == 3 and
                    mapping.get("creating_operation_id") is not None):
                created_pile_uids_by_operation[mapping["creating_operation_id"]].add(
                    mapping.get("native_id"))
        by_operation = defaultdict(list)
        for row in references:
            by_operation[row.get("operation_id")].append(row)
        for root in roots:
            if root.get("reason") != 3 or root.get("outcome") != "committed":
                continue
            operation_id = root.get("operation_id")
            source_event = root.get("source_event")
            events = by_operation.get(operation_id, [])
            created_events = [row for row in events if row.get("ledger_action") == "create"]
            retired_events = [row for row in events if row.get("ledger_action") == "destroy"]
            created = bool(created_events)
            retired = bool(retired_events)
            if source_event is None:
                if created or retired:
                    self.emit("missing_coin_pile_lifecycle_source", operation_id=operation_id)
                continue
            try:
                decode_source_event(source_event)
            except SnapshotError:
                self.emit("invalid_coin_pile_lifecycle_source", operation_id=operation_id)
                continue
            raw = bytes.fromhex(source_event)
            if (int.from_bytes(raw[0:2], "little") != 16 or
                    int.from_bytes(raw[2:4], "little") != 1 or
                    raw[4] not in (1, 3) or not any(raw[5:13]) or
                    raw[13:20] != b"COINACC" or raw[28:36] != b"COINLIFE" or
                    any(raw[36:44])):
                self.emit("invalid_coin_pile_lifecycle_source", operation_id=operation_id)
                continue
            slot = int.from_bytes(raw[44:48], "little")
            if slot not in (1, 2, 3):
                self.emit("invalid_coin_pile_lifecycle_source", operation_id=operation_id)
                continue
            source_retired = bool(slot & 1)
            destination_created = bool(slot & 2)
            if len(created_events) > 1 or len(retired_events) > 1:
                self.emit("coin_pile_lifecycle_event_count_mismatch",
                          operation_id=operation_id, created_events=len(created_events),
                          retired_events=len(retired_events))
            if source_retired != retired or destination_created != created:
                self.emit("coin_pile_lifecycle_source_mismatch", operation_id=operation_id,
                          source_created=destination_created, event_created=created,
                          source_retired=source_retired, event_retired=retired)
            if source_retired and raw[4] == 3:
                mapping_id = int.from_bytes(raw[5:13], "little")
                source_uid = pile_uid_by_mapping.get(mapping_id)
                destroyed_uids = {row.get("uid") for row in retired_events}
                if (type(source_uid) is not int or not 0 < source_uid < 2**64 or
                        destroyed_uids != {source_uid}):
                    self.emit("coin_pile_lifecycle_uid_mismatch",
                              operation_id=operation_id, source_mapping_id=mapping_id,
                              source_uid=source_uid,
                              destroyed_uids=sorted(destroyed_uids, key=repr))
            if destination_created:
                created_uids = {row.get("uid") for row in created_events}
                mapped_created_uids = created_pile_uids_by_operation.get(operation_id, set())
                if created_uids != mapped_created_uids:
                    self.emit("coin_pile_creation_origin_mismatch",
                              operation_id=operation_id,
                              created_uids=sorted(created_uids, key=repr),
                              mapped_created_uids=sorted(mapped_created_uids, key=repr))

    def audit_lineage_realized_prices(self, lineage: str, epoch: str, backend: object,
                                      native: dict, operations: dict) -> None:
        rows = native.get("lineage_realized_prices")
        coverage = native.get("realized_price_coverage")
        if rows is None or coverage is None:
            if backend == "sql_partial":
                self.emit("missing_realized_price_coverage", scope="snapshot")
            return
        if (not isinstance(rows, list) or len(rows) > MAX_ROWS or
                any(not isinstance(row, dict) for row in rows)):
            raise SnapshotError("invalid or oversized lineage realized prices")
        required = {"column_available", "candidate_rows", "missing_price_rows"}
        if (not isinstance(coverage, dict) or set(coverage) != required or
                type(coverage["column_available"]) is not bool or
                type(coverage["candidate_rows"]) is not int or
                type(coverage["missing_price_rows"]) is not int or
                coverage["candidate_rows"] != len(rows) or
                not 0 <= coverage["missing_price_rows"] <= len(rows)):
            raise SnapshotError("invalid realized price coverage")
        if not coverage["column_available"]:
            self.emit("realized_price_column_missing", scope="database")
        by_operation = {}
        missing = 0
        current_ids = {key[0]: row for key, row in operations.items()
                       if row.get("reason") in self.realized_price_reasons}
        for row in rows:
            operation_id = require_id(row.get("operation_id"), "priced operation ID")
            row_lineage = row.get("lineage", lineage)
            row_epoch = row.get("epoch")
            reason = row.get("reason")
            outcome = row.get("outcome")
            result_code = row.get("result_code")
            inbox_receipt = row.get("inbox_receipt")
            price = row.get("realized_price_copper")
            if (operation_id in by_operation or row_lineage != lineage or
                    not isinstance(row_epoch, str) or not HEX_ID.fullmatch(row_epoch) or
                    type(reason) is not int or reason not in self.realized_price_reasons or
                    outcome not in ("committed", "rejected", "unknown") or
                    type(result_code) is not int or
                    not isinstance(inbox_receipt, dict) or
                    set(inbox_receipt) != {"status", "result_code", "failure_stage",
                                           "committed_at_present"} or
                    type(inbox_receipt.get("status")) is not int or
                    inbox_receipt.get("status") != 1 or
                    type(inbox_receipt.get("result_code")) is not int or
                    inbox_receipt.get("result_code") != result_code or
                    type(inbox_receipt.get("failure_stage")) is not int or
                    inbox_receipt.get("failure_stage") != 0 or
                    type(inbox_receipt.get("committed_at_present")) is not bool or
                    not inbox_receipt.get("committed_at_present") or
                    (outcome == "committed" and result_code != 0)):
                raise SnapshotError("invalid lineage realized price record")
            if price is not None and (type(price) is not int or not 0 <= price < 2**63):
                self.emit("invalid_realized_price", operation_id=operation_id)
            if price is None:
                missing += 1
                if outcome == "committed":
                    self.emit("missing_realized_trade_price", operation_id=operation_id)
            elif outcome != "committed":
                self.emit("rejected_realized_price", operation_id=operation_id)
            if outcome == "unknown":
                self.emit("unknown_outcome", operation_id=operation_id)
            if row_epoch == epoch:
                current = current_ids.get(operation_id)
                if current is None or current.get("realized_price_copper") != price:
                    self.emit("realized_price_scope_mismatch", operation_id=operation_id)
                current_ids.pop(operation_id, None)
            by_operation[operation_id] = row
        for operation_id in current_ids:
            self.emit("realized_price_scope_mismatch", operation_id=operation_id)
        if missing != coverage["missing_price_rows"]:
            raise SnapshotError("realized price coverage count mismatch")

    def audit_item_lifetime(self, uid: int, origin: dict, events: list[dict]) -> None:
        if origin["origin"] == "creation" and {
                field: origin.get(field)
                for field in ("revision", "root", "parent", "owner", "state")} != {
                "revision": 0, "root": uid, "parent": None,
                "owner": [0, 0, 0], "state": "absent"}:
            self.emit("invalid_item_creation_origin", uid=uid)
        created = origin["origin"] == "baseline"
        retired = origin.get("state") == "tombstone"
        for event in events:
            action = event.get("action")
            if ((action == "create" and event.get("state") != "live") or
                    (action == "destroy" and event.get("state") != "tombstone")):
                self.emit("invalid_item_supply_state", uid=uid,
                          operation_id=event.get("operation_id"))
            if action == "create":
                if created:
                    self.emit("duplicate_uid", uid=uid, operation_id=event.get("operation_id"))
                created = True
            if retired and action == "destroy":
                self.emit("duplicate_item_retirement", uid=uid,
                          operation_id=event.get("operation_id"))
            if retired and event.get("state") == "live":
                self.emit("resurrected_item_uid", uid=uid, operation_id=event.get("operation_id"))
            retired = retired or action == "destroy" or event.get("state") == "tombstone"
        if not created:
            self.emit("missing_item_creation", uid=uid)

    def audit_items(self, ownership: dict, references: dict, origins: dict, native: dict,
                    lineage_history_uids: set[int] | None = None) -> None:
        for row in list(origins.values()) + list(native.values()):
            if not unsigned_revision(row.get("revision")):
                raise SnapshotError("invalid item origin or native revision")
        referenced = set()
        for ref in references.values():
            if (not unsigned_revision(ref.get("before_revision")) or
                    not unsigned_revision(ref.get("after_revision"))):
                raise SnapshotError("invalid item reference revision")
            legacy_key = (ref.get("legacy_operation_id"), ref.get("legacy_event_index"))
            event = ownership.get(legacy_key)
            uid = ref.get("uid")
            if (not event or event.get("uid") != uid or
                    event.get("before_revision") != ref.get("before_revision") or
                    event.get("revision") != ref.get("after_revision")):
                self.emit("orphan_item_reference", operation_id=ref.get("operation_id"), uid=uid)
            else:
                if legacy_key in referenced:
                    self.emit("duplicate_item_reference", uid=uid, operation_id=ref.get("operation_id"))
                referenced.add(legacy_key)
        by_uid: dict[int, list[dict]] = defaultdict(list)
        for key, event in ownership.items():
            if (not unsigned_revision(event.get("before_revision")) or
                    not unsigned_revision(event.get("revision"))):
                raise SnapshotError("invalid ownership event revision")
            uid = event.get("uid")
            by_uid[uid].append(event)
            if key not in referenced:
                self.emit("missing_item_reference", uid=uid, operation_id=key[0])
        for uid in set(by_uid) | {key[0] for key in origins} | {key[0] for key in native}:
            origin = origins.get((uid,))
            current = native.get((uid,))
            if lineage_history_uids and uid in lineage_history_uids:
                if not current:
                    self.emit("missing_native_item", uid=uid)
                continue
            rows = sorted(by_uid.get(uid, []), key=lambda row: (row.get("revision", -1),
                                                                row.get("operation_id", "")))
            revisions = [row.get("revision") for row in rows]
            if len(set(revisions)) != len(revisions):
                self.emit("duplicate_uid_revision", uid=uid)
            if not origin or origin.get("origin") not in ("baseline", "creation"):
                self.emit("unknown_legacy_origin", uid=uid)
                continue
            self.audit_item_lifetime(uid, origin, rows)
            state = {field: origin.get(field) for field in ("revision", "root", "parent", "owner", "state")}
            for event in rows:
                if event.get("before_revision") != state["revision"] or event.get("revision") != state["revision"] + 1:
                    self.emit("broken_item_history", uid=uid, operation_id=event.get("operation_id"))
                state = {field: event.get(field) for field in ("revision", "root", "parent", "owner", "state")}
            if not current:
                self.emit("missing_native_item", uid=uid)
            elif any(current.get(field) != state[field] for field in state):
                self.emit("stale_native_item", uid=uid)
        topology = {}
        edge_mismatches = {}
        for (uid,), item in native.items():
            # Check every direct edge independently of origin/history proof.
            parent_uid = item.get("parent")
            if parent_uid is None:
                topology[uid] = ("root", item.get("uid"), False)
                continue
            parent = native.get((parent_uid,))
            if parent is None:
                self.emit("orphan_item_parent", uid=uid, parent_uid=parent_uid)
                topology[uid] = ("orphan", None, False)
                continue
            edge_mismatches[uid] = (item.get("root") != parent.get("root") or
                                    item.get("owner") != parent.get("owner"))
        # Resolve each node once without recursion. Memoizing terminal roots,
        # missing ancestors and cycles bounds work even on corrupt deep cuts.
        for (uid,) in native:
            path = []
            positions = {}
            position = uid
            while position not in topology:
                if position in positions:
                    cycle_start = positions[position]
                    cycle = path[cycle_start:]
                    mismatch = any(edge_mismatches[node] for node in cycle)
                    for node in cycle:
                        topology[node] = ("cycle", None, mismatch)
                    path = path[:cycle_start]
                    break
                positions[position] = len(path)
                path.append(position)
                position = native[(position,)]["parent"]
            for node in reversed(path):
                kind, terminal, mismatch = topology[native[(node,)]["parent"]]
                topology[node] = (kind, terminal, mismatch or edge_mismatches[node])
        for (uid,), item in native.items():
            kind, terminal, mismatch = topology[uid]
            if item.get("state") == "live" and (mismatch or (
                    kind == "root" and terminal != item.get("root"))):
                self.emit("inconsistent_native_topology", uid=uid)
            elif kind == "cycle":
                self.emit("cyclic_native_topology", uid=uid)


def bounded_rows(rows: list[dict], limit: int) -> dict:
    return {"count": len(rows), "rows": rows[:limit], "truncated": len(rows) > limit}


def view(snapshot: dict, report: dict, name: str, limit: int, uid: int | None = None,
         operation_id: str | None = None, holding_key: str | None = None) -> dict:
    if type(limit) is not int or not 0 <= limit <= MAX_OUTPUT_ROWS:
        raise SnapshotError("invalid output limit")
    if operation_id is not None:
        require_id(operation_id, "operation lookup ID")
        if name != "operation":
            raise SnapshotError("operation filter requires operation view")
    if holding_key is not None:
        account_key(holding_key)
        if name != "holdings":
            raise SnapshotError("account filter requires holdings view")
    if name == "exceptions":
        return report
    coverage = {
        "lineage": snapshot["lineage"], "selected_epoch": snapshot["epoch"],
        "complete": snapshot.get("complete") is True,
        "quiescent": snapshot.get("quiescent") is True,
        "exception_count": report.get("exception_count"),
    }
    if name == "holdings":
        rows = [{"account_key": row["account_key"], "kind": account_key(row["account_key"])[1],
                 "balance": vector(row["balance"]), "revision": row["revision"]}
                for row in snapshot["native"]["holdings"]
                if holding_key is None or row["account_key"] == holding_key]
        if any(not unsigned_revision(row["revision"]) for row in rows):
            raise SnapshotError("invalid native holding revision")
        if holding_key is not None:
            return {**bounded_rows(rows, limit), "coverage": {
                **coverage,
                "scope": "captured_native_holdings", "account_key": holding_key}}
    elif name == "provenance":
        if type(uid) is not int or not 0 < uid < 2**64:
            raise SnapshotError("provenance requires --uid")
        rows = [{"uid": uid, "operation_id": row["operation_id"],
                 "event_index": row["event_index"], "revision": row["revision"],
                 "root": row["root"], "parent": row["parent"], "owner": row["owner"],
                 "state": row["state"], "action": row["action"]}
                for row in (snapshot["ownership_events"] +
                            (snapshot["native"].get("uid_history_events") or []) +
                            (snapshot["native"].get("unattributed_uid_events") or []))
                if row.get("uid") == uid and isinstance(row.get("operation_id"), str)
                and HEX_ID.fullmatch(row["operation_id"]) and type(row.get("event_index")) is int
                and type(row.get("revision")) is int and type(row.get("root")) is int
                and (row.get("parent") is None or type(row.get("parent")) is int)
                and isinstance(row.get("owner"), list) and len(row["owner"]) == 3
                and all(type(part) is int for part in row["owner"])
                and row.get("state") in ("live", "tombstone", "quarantined")
                and row.get("action") in ("create", "move", "destroy", "quarantine")]
        # The epoch and lineage collections can project the same native event.
        # Deduplicate exact projections; conflicting positions stay visible.
        unique = {json.dumps(row, sort_keys=True): row for row in rows}
        rows = sorted(unique.values(),
                      key=lambda row: (row["revision"], row["operation_id"], row["event_index"]))
        return {**bounded_rows(rows, limit), "coverage": {
            **coverage,
            "lineage_history_available": isinstance(snapshot["native"].get("uid_history_events"), list),
            "unattributed_history_available": isinstance(
                snapshot["native"].get("unattributed_uid_events"), list)}}
    elif name == "operation":
        if operation_id is None:
            raise SnapshotError("operation view requires --operation-id")
        rows = []
        record_counts = {}
        for collection, fields in (
                ("operations", ("reason", "result_code", "account_count", "posting_count",
                                "child_count", "item_event_count", "realized_price_copper",
                                "accounting_version", "writer_id", "policy_version", "compiler_version")),
                ("effects", ("account_index", "before_revision", "after_revision")),
                ("postings", ("line_index", "event_index", "account_index", "child_index", "copper_value")),
                ("children", ("child_index", "parent_index", "domain_id", "discriminator", "relationship")),
                ("item_references", ("line_index", "event_index", "uid", "child_index", "before_revision",
                                     "after_revision", "legacy_event_index")),
                ("receipts", ("status", "result_code", "failure_stage")),
                ("source_claims", ()),
                ("orphan_evidence", ("row_index",))):
            selected = [row for row in snapshot.get(collection, [])
                        if row.get("operation_id") == operation_id]
            record_counts[collection] = len(selected)
            for row in selected:
                safe = {"record": collection, "operation_id": operation_id}
                safe.update({field: row[field] for field in fields if type(row.get(field)) is int})
                for field in ("lineage", "epoch", "original_operation_id", "child_operation_id",
                              "receipt_operation_id", "legacy_operation_id"):
                    if row.get(field) is not None:
                        safe[field] = require_id(row[field], field)
                if row.get("source_event") is not None:
                    source = row["source_event"]
                    if not isinstance(source, str) or not HEX_SOURCE_EVENT.fullmatch(source):
                        raise SnapshotError("invalid operation source event")
                    safe["source_event"] = source
                if collection == "operations":
                    safe["outcome"] = (row["outcome"] if row.get("outcome") in
                                       ("committed", "rejected") else "unknown")
                elif collection == "effects":
                    account_key(row["account_key"])
                    safe.update(account_key=row["account_key"], before=vector(row["before"]),
                                after=vector(row["after"]))
                elif collection == "postings":
                    safe["delta"] = vector(row["delta"])
                elif collection == "receipts":
                    safe["committed_at_present"] = row.get("committed_at_present") is True
                elif collection == "orphan_evidence":
                    if row.get("table") not in ORPHAN_EVIDENCE_SOURCES:
                        raise SnapshotError("invalid orphan evidence table")
                    safe["table"] = row["table"]
                rows.append(safe)
        order = {collection: index for index, collection in enumerate(record_counts)}
        rows.sort(key=lambda row: (order[row["record"]], next(
            (row[field] for field in ("line_index", "event_index", "account_index", "child_index", "row_index")
             if field in row), 0), json.dumps(row, sort_keys=True)))
        return {**bounded_rows(rows, limit), "record_counts": record_counts, "coverage": {
            **coverage, "root_scope": "selected_epoch",
            "operation_id": operation_id}}
    elif name == "supply":
        totals: dict[tuple, int] = defaultdict(int)
        effect_kind = {(row["operation_id"], row["account_index"]): account_key(row["account_key"])[1]
                       for row in snapshot["effects"]}
        reasons = {row["operation_id"]: row.get("reason") for row in snapshot["operations"]}
        for post in snapshot["postings"]:
            kind = effect_kind.get((post.get("operation_id"), post.get("account_index")))
            reason = reasons.get(post["operation_id"])
            if kind in (7, 8, 9, 10) and type(reason) is int:
                totals[(kind, reason)] += copper(vector(post["delta"]))
        rows = [{"account_kind": kind, "reason": reason, "net_copper": total}
                for (kind, reason), total in sorted(totals.items())]
    elif name == "prices":
        history = snapshot["native"].get("lineage_realized_prices")
        price_coverage = snapshot["native"].get("realized_price_coverage")
        history_available = isinstance(history, list) and isinstance(price_coverage, dict)
        rows = [{"operation_id": row["operation_id"], "epoch": row["epoch"],
                 "reason": row["reason"],
                 "price_copper": row["realized_price_copper"]}
                for row in snapshot["operations"] + (history if history_available else [])
                if isinstance(row.get("operation_id"), str) and HEX_ID.fullmatch(row["operation_id"])
                and isinstance(row.get("epoch"), str) and HEX_ID.fullmatch(row["epoch"])
                and row.get("outcome") == "committed"
                and type(row.get("reason")) is int and type(row.get("realized_price_copper")) is int
                and 0 <= row["realized_price_copper"] < 2**63]
        # Selected-epoch roots overlap captured lineage prices. Preserve
        # conflicting projections instead of choosing either price as truth.
        unique = {(row["epoch"], row["operation_id"], row["reason"], row["price_copper"]): row
                  for row in rows}
        rows = [unique[key] for key in sorted(unique)]
        coverage.update(lineage_history_available=history_available,
                        realized_price_coverage=dict(price_coverage) if history_available else None)
    elif name == "routes":
        matrix = json.loads((Path(__file__).resolve().parents[1] /
                             "docs/persistence/economy_accounting/writer_coverage_matrix.json").read_text())
        rows = [{"id": row["id"], "disposition": row["disposition"],
                 "activation": row["blocking_policy_after_activation"]["decision"]}
                for row in matrix["routes"]]
    else:
        raise SnapshotError("unknown view")
    return {**bounded_rows(rows, limit), "coverage": coverage}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path, help="complete quiescent snapshot JSON")
    parser.add_argument("--view", choices=("exceptions", "holdings", "provenance", "operation", "supply", "prices", "routes"),
                        default="exceptions")
    parser.add_argument("--uid", type=int)
    parser.add_argument("--operation-id")
    parser.add_argument("--account-key")
    parser.add_argument("--limit", type=int, default=50)
    args = parser.parse_args()
    try:
        if not 0 <= args.limit <= MAX_OUTPUT_ROWS or args.snapshot.stat().st_size > MAX_INPUT_BYTES:
            raise SnapshotError("snapshot or output limit exceeded")
        snapshot = json.loads(args.snapshot.read_text(encoding="utf-8"))
        report = Reconciler(args.limit).audit(snapshot)
        result = view(snapshot, report, args.view, args.limit, args.uid,
                      operation_id=args.operation_id, holding_key=args.account_key)
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        return 0 if report["exception_count"] == 0 else 1
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"reconciliation failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
