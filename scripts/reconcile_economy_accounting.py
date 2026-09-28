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
ORDINARY_KINDS = {1, 2, 3, 4, 5, 6}
UNITS = (1, 10, 100, 1000)
HEX_ID = re.compile(r"[0-9a-f]{32}\Z")
HEX_KEY = re.compile(r"[0-9a-f]{80}\Z")
TABLES = (
    "operations", "effects", "postings", "children", "item_references",
    "ownership_events", "source_claims", "account_origins", "item_origins", "receipts",
)
REGISTRY_PATH = Path(__file__).resolve().parents[1] / "docs/persistence/economy_accounting/registry.json"


class SnapshotError(ValueError):
    pass


def vector(value: object) -> tuple[int, int, int, int]:
    if (not isinstance(value, list) or len(value) != 4 or
            any(type(part) is not int or not -(2**63) <= part < 2**63 for part in value)):
        raise SnapshotError("invalid denomination vector")
    return tuple(value)


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
    if raw[16:18] != b"\x01\x00" or raw[36:] != bytes(4) or not 1 <= kind <= 10 or not identity:
        raise SnapshotError("invalid account key")
    return raw[:16].hex(), kind, identity, context


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
                elif field in ("uid", "parent_uid", "child_index", "line_index", "net_copper") and type(value) is int:
                    safe[field] = value
                elif field == "table" and value in TABLES:
                    safe[field] = value
                elif field == "scope" and value == "snapshot":
                    safe[field] = value
            self.exceptions.append({"code": code, **safe})

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
        native = snapshot.get("native")
        if not isinstance(native, dict):
            raise SnapshotError("missing native authority")
        holdings = self.table(native, "holdings")
        items = self.table(native, "items")

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

        by_op = {name: defaultdict(list) for name in ("effects", "postings", "children", "item_references")}
        for name, indexed in (("effects", effects), ("postings", postings), ("children", children),
                              ("item_references", references)):
            for row in indexed.values():
                by_op[name][row.get("operation_id")].append(row)
                if (row.get("operation_id"),) not in operations:
                    self.emit("orphan_evidence", table=name, operation_id=row.get("operation_id"))

        by_account: dict[str, list[dict]] = defaultdict(list)
        for row in effects.values():
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
            policy = self.reasons.get(op.get("reason"))
            if policy is None:
                self.emit("unknown_policy_reason", operation_id=op_id)
            receipt = receipts.get((op_id,))
            if not receipt:
                self.emit("missing_receipt", operation_id=op_id)
            elif receipt.get("status") != op.get("outcome") or receipt.get("result_code") != op.get("result_code"):
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
            if source is not None and op.get("outcome") == "committed":
                if not isinstance(source, str) or not re.fullmatch(r"[0-9a-f]{96}", source):
                    self.emit("invalid_source_event", operation_id=op_id)
                claim = claims.get((lineage, source))
                if not claim or claim.get("operation_id") != op_id:
                    self.emit("missing_source_claim", operation_id=op_id)
            if policy and policy.get("original_operation_required") and (op.get("original_operation_id"),) not in operations:
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
            op = operations.get((claim.get("operation_id"),))
            if not op or op.get("source_event") != claim.get("source_event") or claim.get("lineage") != lineage:
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

        self.audit_accounts(lineage, operations, by_account, origins, native_holdings, posting_deltas)
        self.audit_items(ownership, references, item_origins, native_items)
        return {"exception_count": sum(self.counts.values()), "exception_counts": dict(sorted(self.counts.items())),
                "exceptions": self.exceptions, "truncated": sum(self.counts.values()) > len(self.exceptions),
                "checked": {name: len(tables[name]) for name in TABLES} |
                           {"native_holdings": len(holdings), "native_items": len(items)}}

    def audit_accounts(self, lineage: str, operations: dict, by_account: dict, origins: dict, native: dict,
                       deltas: dict) -> None:
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
            if type(revision) is not int or revision < 0:
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

    def audit_items(self, ownership: dict, references: dict, origins: dict, native: dict) -> None:
        referenced = set()
        for ref in references.values():
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
            uid = event.get("uid")
            by_uid[uid].append(event)
            if key not in referenced:
                self.emit("missing_item_reference", uid=uid, operation_id=key[0])
        for uid in set(by_uid) | {key[0] for key in origins} | {key[0] for key in native}:
            origin = origins.get((uid,))
            current = native.get((uid,))
            rows = sorted(by_uid.get(uid, []), key=lambda row: (row.get("revision", -1),
                                                                row.get("operation_id", "")))
            revisions = [row.get("revision") for row in rows]
            if len(set(revisions)) != len(revisions):
                self.emit("duplicate_uid_revision", uid=uid)
            if not origin or origin.get("origin") not in ("baseline", "creation"):
                self.emit("unknown_legacy_origin", uid=uid)
                continue
            state = {field: origin.get(field) for field in ("revision", "root", "parent", "owner", "state")}
            if origin["origin"] == "creation" and state != {
                    "revision": 0, "root": uid, "parent": None, "owner": [0, 0, 0], "state": "absent"}:
                self.emit("invalid_item_creation_origin", uid=uid)
            created = origin.get("origin") == "baseline"
            for event in rows:
                if event.get("before_revision") != state["revision"] or event.get("revision") != state["revision"] + 1:
                    self.emit("broken_item_history", uid=uid, operation_id=event.get("operation_id"))
                if event.get("action") == "create":
                    if created:
                        self.emit("duplicate_uid", uid=uid, operation_id=event.get("operation_id"))
                    created = True
                state = {field: event.get(field) for field in ("revision", "root", "parent", "owner", "state")}
            if not created:
                self.emit("missing_item_creation", uid=uid)
            if not current:
                self.emit("missing_native_item", uid=uid)
            elif any(current.get(field) != state[field] for field in state):
                self.emit("stale_native_item", uid=uid)
            if current and current.get("parent") is not None and (current.get("parent"),) not in native:
                self.emit("orphan_item_parent", uid=uid, parent_uid=current.get("parent"))
        for (uid,), item in native.items():
            visited = set()
            position = item
            while position.get("parent") is not None:
                if position.get("uid") in visited:
                    self.emit("cyclic_native_topology", uid=uid)
                    break
                visited.add(position.get("uid"))
                parent = native.get((position.get("parent"),))
                if parent is None:
                    break  # orphan_item_parent was reported above.
                if (item.get("state") == "live" and
                        (position.get("root") != parent.get("root") or
                         position.get("owner") != parent.get("owner"))):
                    self.emit("inconsistent_native_topology", uid=uid)
                    break
                position = parent
            else:
                if item.get("state") == "live" and position.get("uid") != item.get("root"):
                    self.emit("inconsistent_native_topology", uid=uid)


def bounded_rows(rows: list[dict], limit: int) -> dict:
    return {"count": len(rows), "rows": rows[:limit], "truncated": len(rows) > limit}


def view(snapshot: dict, report: dict, name: str, limit: int, uid: int | None = None) -> dict:
    if name == "exceptions":
        return report
    if name == "holdings":
        rows = [{"account_key": row["account_key"], "kind": account_key(row["account_key"])[1],
                 "balance": vector(row["balance"]), "revision": row["revision"]}
                for row in snapshot["native"]["holdings"]]
        if any(type(row["revision"]) is not int or row["revision"] < 0 for row in rows):
            raise SnapshotError("invalid native holding revision")
    elif name == "provenance":
        if uid is None:
            raise SnapshotError("provenance requires --uid")
        rows = [{"uid": uid, "operation_id": row["operation_id"],
                 "event_index": row["event_index"], "revision": row["revision"],
                 "root": row["root"], "parent": row["parent"], "owner": row["owner"],
                 "state": row["state"], "action": row["action"]}
                for row in snapshot["ownership_events"]
                if row.get("uid") == uid and isinstance(row.get("operation_id"), str)
                and HEX_ID.fullmatch(row["operation_id"]) and type(row.get("event_index")) is int
                and type(row.get("revision")) is int and type(row.get("root")) is int
                and (row.get("parent") is None or type(row.get("parent")) is int)
                and isinstance(row.get("owner"), list) and len(row["owner"]) == 3
                and all(type(part) is int for part in row["owner"])
                and row.get("state") in ("live", "tombstone", "quarantined")
                and row.get("action") in ("create", "move", "destroy", "quarantine")]
        rows.sort(key=lambda row: (row["revision"], row["operation_id"]))
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
        rows = [{"operation_id": row["operation_id"], "reason": row.get("reason"),
                 "price_copper": row["realized_price_copper"]}
                for row in snapshot["operations"]
                if isinstance(row.get("operation_id"), str) and HEX_ID.fullmatch(row["operation_id"])
                and type(row.get("reason")) is int and type(row.get("realized_price_copper")) is int
                and row["realized_price_copper"] >= 0]
    elif name == "routes":
        matrix = json.loads((Path(__file__).resolve().parents[1] /
                             "docs/persistence/economy_accounting/writer_coverage_matrix.json").read_text())
        rows = [{"id": row["id"], "disposition": row["disposition"],
                 "activation": row["blocking_policy_after_activation"]["decision"]}
                for row in matrix["routes"]]
    else:
        raise SnapshotError("unknown view")
    return bounded_rows(rows, limit)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("snapshot", type=Path, help="complete quiescent snapshot JSON")
    parser.add_argument("--view", choices=("exceptions", "holdings", "provenance", "supply", "prices", "routes"),
                        default="exceptions")
    parser.add_argument("--uid", type=int)
    parser.add_argument("--limit", type=int, default=50)
    args = parser.parse_args()
    try:
        if not 0 <= args.limit <= MAX_OUTPUT_ROWS or args.snapshot.stat().st_size > MAX_INPUT_BYTES:
            raise SnapshotError("snapshot or output limit exceeded")
        snapshot = json.loads(args.snapshot.read_text(encoding="utf-8"))
        report = Reconciler(args.limit).audit(snapshot)
        result = view(snapshot, report, args.view, args.limit, args.uid)
        print(json.dumps(result, sort_keys=True, separators=(",", ":")))
        return 0 if report["exception_count"] == 0 else 1
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"reconciliation failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
