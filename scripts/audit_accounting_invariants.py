#!/usr/bin/env python3
"""Audit double-entry anti-duplication, conservation, and custody invariants."""

import argparse
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DOCS = ROOT / "docs/persistence/economy_accounting"


class AuditError(Exception):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AuditError(message)


def parse_copper(denomination_vector) -> int:
    require(isinstance(denomination_vector, (list, tuple)) and len(denomination_vector) == 4,
            "Denomination vector must contain exactly 4 values")
    require(all(type(amount) is int and -(2**63) <= amount < 2**63
                for amount in denomination_vector),
            "Denominations must be signed 64-bit integers")
    multipliers = [1, 10, 100, 1000]
    total = sum(amount * mult for amount, mult in zip(denomination_vector, multipliers))
    require(-(2**63) <= total < 2**63, "Total copper exceeds signed 64-bit range")
    return total


class AccountingInvariantAuditor:
    def __init__(self, registry_data: dict, verbose: bool = False):
        self.registry = registry_data
        self.verbose = verbose
        self.reasons = {r["id"]: r for r in self.registry.get("reasons", [])}
        self.account_kinds = {k["id"]: k for k in self.registry.get("account_kinds", [])}

    def audit_fixture(self, fixture: dict, fixture_name: str = "fixture") -> dict:
        stats = {
            "operations_checked": 0,
            "postings_checked": 0,
            "items_checked": 0,
            "zero_sum_verified": 0,
            "source_events_verified": 0,
            "custody_nodes_verified": 0,
        }

        # 1. Opening holdings validation
        holdings = fixture.get("holdings", {})
        for name, acc in holdings.items():
            require(acc.get("kind") in self.account_kinds, f"Unknown account kind in {name}: {acc.get('kind')}")
            bal = acc.get("balance", [0, 0, 0, 0])
            copper_val = parse_copper(bal)
            require(all(x >= 0 for x in bal), f"Negative opening balance in account {name}")
            require(copper_val >= 0, f"Negative total copper in opening account {name}")

        # 2. Custody acyclic and single-owner invariant
        custody = fixture.get("custody", {})
        self.audit_custody(custody)
        stats["custody_nodes_verified"] += len(custody)

        # 3. Operations audit
        receipts = {}
        consumed_source_events = set()

        for op in fixture.get("operations", []):
            op_id = op.get("operation_id")
            require(isinstance(op_id, str) and
                    bool(re.fullmatch(r"[0-9a-f]{32}", op_id)) and int(op_id, 16) != 0,
                    f"Invalid operation ID format: {op_id}")

            serialized = json.dumps(op, sort_keys=True)
            if op_id in receipts:
                # Idempotency / anti-duplication invariant: Exact replay must be bitwise identical
                require(receipts[op_id] == serialized, f"Duplicate operation ID {op_id} with conflicting payload")
                continue
            receipts[op_id] = serialized
            stats["operations_checked"] += 1

            reason_id = op.get("reason")
            require(reason_id in self.reasons, f"Unknown reason {reason_id} in op {op_id}")
            policy = self.reasons[reason_id]

            # Source event anti-replay invariant
            src_event = op.get("source_event")
            if policy.get("source_event_required") or src_event is not None:
                require(isinstance(src_event, str) and
                        bool(re.fullmatch(r"[0-9a-f]{32}", src_event)) and int(src_event, 16) != 0,
                        f"Op {op_id} missing valid 32-hex source event")
                require(src_event not in consumed_source_events,
                        f"Source event {src_event} replayed in op {op_id}")
                consumed_source_events.add(src_event)
                stats["source_events_verified"] += 1

            # Double-entry conservation: Postings must strictly balance to zero copper
            postings = op.get("postings", [])
            total_copper = 0
            for post in postings:
                stats["postings_checked"] += 1
                acc_name = post["account"]
                require(acc_name in holdings, f"Posting references unknown account {acc_name}")
                acc_kind = holdings[acc_name]["kind"]
                require(acc_kind in policy["account_kinds"],
                        f"Account {acc_name} kind {acc_kind} not authorized for reason {reason_id}")
                delta = post["delta"]
                total_copper += parse_copper(delta)

            require(total_copper == 0, f"Double-entry violation in op {op_id}: net delta is {total_copper} copper")
            stats["zero_sum_verified"] += 1

            # Child link invariants
            children = op.get("children", [])
            child_ids = set()
            for child in children:
                c_id = child.get("operation_id")
                require(isinstance(c_id, str) and
                        bool(re.fullmatch(r"[0-9a-f]{32}", c_id)) and int(c_id, 16) != 0,
                        f"Invalid child operation ID format: {c_id}")
                require(c_id != op_id, f"Self-referential child operation {c_id} in op {op_id}")
                require(c_id not in child_ids, f"Duplicate child operation {c_id} in op {op_id}")
                child_ids.add(c_id)
                require(child.get("parent_id") == op_id, f"Child {c_id} parent_id does not match op {op_id}")

            # Item events bind to this operation in their native event order.
            for index, item in enumerate(op.get("items", [])):
                require(isinstance(item, dict), f"Invalid item event {index} in op {op_id}")
                require(type(item.get("uid")) is int and 0 < item["uid"] < 2**64,
                        f"Invalid item UID at event {index} in op {op_id}")
                require(type(item.get("event_index")) is int and item["event_index"] == index,
                        f"Invalid item event index at event {index} in op {op_id}")
                require(item.get("operation_id") == op_id,
                        f"Item event {index} operation_id does not match op {op_id}")
                stats["items_checked"] += 1

        if self.verbose:
            print(f"[{fixture_name}] Verified: {stats}")
        return stats

    def audit_custody(self, custody: dict) -> None:
        for uid, state in custody.items():
            root_uid = str(state.get("root", ""))
            require(root_uid in custody, f"Custody entry {uid} references missing root {root_uid}")
            visited = set()
            curr = str(uid)
            while True:
                require(curr not in visited, f"Cyclic container hierarchy detected at UID {curr}")
                visited.add(curr)
                parent = custody[curr].get("parent", 0)
                if parent == 0:
                    require(curr == root_uid, f"Root mismatch: expected root {root_uid} but terminated at {curr}")
                    break
                curr = str(parent)
                require(curr in custody, f"Missing parent UID {curr} referenced by child")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--golden", type=Path, default=DEFAULT_DOCS / "golden.json",
                        help="Path to golden.json fixtures")
    parser.add_argument("--registry", type=Path, default=DEFAULT_DOCS / "registry.json",
                        help="Path to registry.json")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    if not args.registry.exists():
        print(f"ERROR: Registry file not found: {args.registry}", file=sys.stderr)
        return 1
    if not args.golden.exists():
        print(f"ERROR: Golden fixtures not found: {args.golden}", file=sys.stderr)
        return 1

    registry = json.loads(args.registry.read_text(encoding="utf-8"))
    golden = json.loads(args.golden.read_text(encoding="utf-8"))

    auditor = AccountingInvariantAuditor(registry, verbose=args.verbose)

    total_fixtures = 0
    total_ops = 0
    fixtures_list = golden.get("fixtures", []) if isinstance(golden, dict) and "fixtures" in golden else (
        golden if isinstance(golden, list) else list(golden.values())
    )
    for fixture in fixtures_list:
        if not isinstance(fixture, dict) or "operations" not in fixture:
            continue
        name = fixture.get("id", "unnamed")
        try:
            stats = auditor.audit_fixture(fixture, fixture_name=name)
            total_fixtures += 1
            total_ops += stats["operations_checked"]
        except AuditError as err:
            print(f"AUDIT FAILED on fixture '{name}': {err}", file=sys.stderr)
            return 1

    print(f"Audit PASSED: {total_fixtures} fixtures, {total_ops} operations verified. All invariants preserved.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
