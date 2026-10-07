"""Bounded native money-claim allocations and sale dependencies in one SQL cut."""
from __future__ import annotations

from . import canonical_reward_contract as c
from .canonical_reward_auction import (
    ALLOCATION_COLUMNS, AUCTION_LEDGER_COLUMNS, MAX_CLAIM_SOURCES, qualify_retained_money_claim,
)
from .canonical_reward_coin_source import _columns, _family
from .canonical_reward_source import (
    CanonicalRewardSnapshot, RETAINED_SOURCE_TABLES, ROOT_COLUMNS, INBOX_COLUMNS,
    EFFECT_COLUMNS, POSTING_COLUMNS, CLAIM_COLUMNS, LEDGER_COLUMNS, unavailable_bank_event,
)


class CanonicalAuctionSnapshot(CanonicalRewardSnapshot):
    source_tables = RETAINED_SOURCE_TABLES
    # One claim may depend on 128 complete native sales. Its worst fan-out
    # query remains below 2,000 rows and its receipt reservation below 32 MiB.
    max_operations_per_page = 1

    def capture_money_claims(self, operations, *, preserve_refusals=False):
        c.require(type(preserve_refusals) is bool, "canonical_quarantine_mode")
        return self._capture(operations, partition=None, preserve_refusals=preserve_refusals)

    def capture_bank_operations(self, *args, **kwargs):
        raise c.EvidenceError("canonical_auction_adapter_bank_api")

    def capture_bank_partition(self, *args, **kwargs):
        raise c.EvidenceError("canonical_auction_history_discovery_unavailable")

    def _shapes(self, operations):
        markers = ",".join("%s" for _ in operations)
        rows = self._execute("SELECT r.operation_id,r.writer_id,r.reason," +
            ",".join("r." + name for name in c.COUNTS) +
            ",OCTET_LENGTH(r.canonical_plan) AS plan_bytes,OCTET_LENGTH(r.canonical_intent) AS intent_bytes,"
            "(SELECT MAX(OCTET_LENGTH(i.result_payload)) FROM critical_operation_inbox i "
            "WHERE i.operation_id=r.operation_id) AS result_bytes FROM economic_accounting_operation r "
            "WHERE r.operation_id IN (" + markers + ") ORDER BY r.operation_id LIMIT %s",
            (*operations, len(operations) + 1))
        c.require(len(rows) == len(operations) and {row["operation_id"] for row in rows} == set(operations),
                  "canonical_auction_selected_root_missing")
        return rows

    def _capture_page(self, page, events, *, shapes=None, preserve_refusals=False):
        c.require(shapes is None and len(page) == 1, "canonical_auction_explicit_claim_page")
        op = page[0]
        shape = self._shapes(page)[0]
        c.require(shape["writer_id"] == 13 and shape["reason"] == 31 and
                  shape["account_count"] == shape["posting_count"] == 2 and
                  shape["child_count"] == shape["before_witness_count"] == shape["after_witness_count"] == shape["item_event_count"] == 0 and
                  shape["plan_bytes"] == 592 and shape["intent_bytes"] == 336 and
                  shape["result_bytes"] in (None, 320), "canonical_auction_selected_claim_shape")
        allocations = self._execute("SELECT " + ",".join(ALLOCATION_COLUMNS) +
            " FROM economic_pending_claim_source FORCE INDEX (idx_economic_pending_claim_consumed) WHERE claim_operation_id=%s "
            "ORDER BY source_operation_id,source_slot LIMIT %s", (op, MAX_CLAIM_SOURCES + 1))
        c.require(len(allocations) <= MAX_CLAIM_SOURCES, "canonical_auction_allocation_capacity")
        dependencies = tuple(sorted({c.identifier(row["source_operation_id"]) for row in allocations}))
        c.require(op not in dependencies, "canonical_auction_self_source")
        if dependencies:
            for shape in self._shapes(dependencies):
                c.require(shape["writer_id"] == 11 and shape["reason"] == 30 and
                    shape["account_count"] == shape["posting_count"] in (2, 3) and
                    shape["child_count"] == shape["item_event_count"] == 0 and
                    1 <= shape["before_witness_count"] == shape["after_witness_count"] <= 9 and
                    shape["plan_bytes"] == 256 + 168 * shape["account_count"] + 128 * shape["before_witness_count"] and
                    shape["plan_bytes"] <= 2048 and shape["intent_bytes"] == 378 + 27 * shape["before_witness_count"] and
                    shape["result_bytes"] in (None, 320), "canonical_auction_dependency_shape_or_capacity")
        identities = tuple(sorted((op, *dependencies)))
        markers = ",".join("%s" for _ in identities)
        where = " WHERE operation_id IN (" + markers + ")"
        families = {table: [] for table in self.source_tables}
        retained = []

        def retain(table, row):
            if table == "economic_pending_claim_source":
                c.require(row["claim_operation_id"] == op, "canonical_auction_allocation_query_scope")
                key = c.identifier(row["source_operation_id"]) + c.integer(row["source_slot"], lower=1,
                    upper=(1 << 16) - 1).to_bytes(2, "big")
            else:
                identity = c.identifier(row["operation_id"])
                c.require(identity in identities, "canonical_auction_receipt_query_scope")
                if table == "economic_accounting_account_effect":
                    key = identity + c.integer(row["account_index"], upper=2).to_bytes(2, "big")
                elif table == "economic_accounting_coin_posting":
                    key = identity + c.integer(row["line_index"], upper=2).to_bytes(2, "big")
                elif table == "economic_accounting_source_claim":
                    key = c.identifier(row["lineage"]) + row["source_event"]
                else:
                    key = identity
            families[table].append(row)
            retained.append(self._retain(table, key, row))

        for row in allocations:
            retain("economic_pending_claim_source", row)
        rows = self._execute("SELECT " + _columns("r", "root_", ROOT_COLUMNS, "recorded_at") + "," +
            _columns("i", "inbox_", INBOX_COLUMNS, "committed_at") + "," +
            _columns("cu", "currency_", LEDGER_COLUMNS, "created_at") + "," +
            _columns("au", "auction_", AUCTION_LEDGER_COLUMNS, "created_at") +
            " FROM economic_accounting_operation r LEFT JOIN critical_operation_inbox i "
            "ON i.operation_id=r.operation_id LEFT JOIN currency_ledger cu ON cu.operation_id=r.operation_id "
            "LEFT JOIN auction_ledger au ON au.operation_id=r.operation_id WHERE r.operation_id IN (" + markers +
            ") ORDER BY r.operation_id LIMIT %s", (*identities, len(identities) + 1))
        c.require(len(rows) == len(identities), "canonical_auction_root_inbox_fanout")
        current_root = None
        for row in rows:
            root = _family(row, "root_")
            retain("economic_accounting_operation", root)
            if root["operation_id"] == op:
                current_root = root
            for table, prefix in (("critical_operation_inbox", "inbox_"), ("currency_ledger", "currency_"),
                                  ("auction_ledger", "auction_")):
                family = _family(row, prefix)
                if family.get("operation_id") is not None:
                    retain(table, family)
        for table, columns, order, maximum, timestamp in (
            ("economic_accounting_account_effect", EFFECT_COLUMNS, "operation_id,account_index", 3 * len(identities), None),
            ("economic_accounting_coin_posting", POSTING_COLUMNS, "operation_id,line_index", 3 * len(identities), None),
            ("economic_accounting_source_claim", CLAIM_COLUMNS, "operation_id", 1, None),
        ):
            rows = self._execute("SELECT " + ",".join(columns) +
                (",TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00'," + timestamp + ") AS " + timestamp + "_usec" if timestamp else "") +
                " FROM " + table + where + " ORDER BY " + order + " LIMIT %s", (*identities, maximum + 1))
            c.require(len(rows) <= maximum, "canonical_auction_receipt_fanout")
            for row in rows:
                retain(table, row)
        unexpected = self._execute("SELECT operation_id FROM economic_accounting_child" + where +
            " UNION ALL SELECT operation_id FROM economic_accounting_item_reference" + where +
            " UNION ALL SELECT operation_id FROM item_ownership_ledger" + where + " LIMIT 1",
            (*identities, *identities, *identities))
        c.require(not unexpected, "canonical_auction_unexpected_native_item_or_child")
        try:
            event = qualify_retained_money_claim(current_root, families)
        except (c.EvidenceError, KeyError, TypeError, OverflowError):
            if not preserve_refusals:
                raise
            event = unavailable_bank_event(op, retained)
        events.append(event)
