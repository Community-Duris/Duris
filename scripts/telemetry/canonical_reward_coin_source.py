"""Bounded typed coin root/child receipts in one owning SQL snapshot."""
from __future__ import annotations

from . import canonical_reward_contract as c
from .canonical_reward_coin import CHILD_COLUMNS, ITEM_REFERENCE_COLUMNS, ITEM_LEDGER_COLUMNS, qualify_coin_root
from .canonical_reward_source import (
    CanonicalRewardSnapshot, COIN_SOURCE_TABLES, ROOT_COLUMNS, INBOX_COLUMNS,
    EFFECT_COLUMNS, POSTING_COLUMNS, CLAIM_COLUMNS, LEDGER_COLUMNS, unavailable_bank_event,
)


def _columns(alias, prefix, columns, timestamp=None):
    result = [alias + "." + name + " AS " + prefix + name for name in columns]
    if timestamp:
        result.append("TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00'," + alias + "." + timestamp +
                      ") AS " + prefix + timestamp + "_usec")
    return ",".join(result)


def _family(row, prefix):
    return {name[len(prefix):]: value for name, value in row.items() if name.startswith(prefix)}


class CanonicalCoinSnapshot(CanonicalRewardSnapshot):
    source_tables = COIN_SOURCE_TABLES
    # At most 17 physical rows per root. Both joined and single-family queries
    # include a sentinel and remain below 2,000 rows; total reservation stays 32 MiB.
    max_operations_per_page = 128

    def capture_coin_operations(self, operations, *, preserve_refusals=False):
        c.require(type(preserve_refusals) is bool, "canonical_quarantine_mode")
        return self._capture(operations, partition=None, preserve_refusals=preserve_refusals)

    def capture_bank_operations(self, operations, *, preserve_refusals=False):
        raise c.EvidenceError("canonical_coin_adapter_bank_api")

    def capture_bank_partition(self, *args, **kwargs):
        raise c.EvidenceError("canonical_coin_adapter_bank_discovery")

    def _capture_page(self, page, events, *, shapes=None, preserve_refusals=False):
        c.require(shapes is None, "canonical_coin_explicit_selection")
        markers = ",".join("%s" for _ in page)
        where = " WHERE operation_id IN (" + markers + ")"
        # Blob lengths and owner fan-out are checked before any blob fetch.
        shapes = self._execute("SELECT r.operation_id,r.writer_id,r.reason," +
            ",".join("r." + name for name in c.COUNTS) +
            ",OCTET_LENGTH(r.canonical_plan) AS plan_bytes,OCTET_LENGTH(r.canonical_intent) AS intent_bytes,"
            "(SELECT MAX(OCTET_LENGTH(i.result_payload)) FROM critical_operation_inbox i "
            "WHERE i.operation_id=r.operation_id) AS root_result_bytes,"
            "(SELECT MAX(OCTET_LENGTH(i.result_payload)) FROM economic_accounting_child ch "
            "JOIN critical_operation_inbox i ON i.operation_id=ch.child_operation_id "
            "WHERE ch.operation_id=r.operation_id) AS child_result_bytes "
            "FROM economic_accounting_operation r WHERE r.operation_id IN (" + markers +
            ") ORDER BY r.operation_id LIMIT %s", (*page, len(page)))
        c.require(len(shapes) == len(page) and {row["operation_id"] for row in shapes} == set(page),
                  "canonical_selected_root_missing")
        for shape in shapes:
            c.require(shape["writer_id"] == 5 and shape["reason"] == 3 and
                shape["account_count"] == shape["posting_count"] == shape["child_count"] == 2 and
                0 <= shape["item_event_count"] <= 2 and
                0 <= shape["before_witness_count"] == shape["after_witness_count"] <= 18 and
                shape["plan_bytes"] == 656 + 128 * shape["before_witness_count"] + 128 * shape["item_event_count"] and
                shape["plan_bytes"] <= 3072 and shape["intent_bytes"] == 272 and
                shape["root_result_bytes"] in (None, 256) and
                (shape["child_result_bytes"] is None or 0 <= shape["child_result_bytes"] <= 80),
                "canonical_selected_coin_shape_or_capacity")
        families = {op: {table: [] for table in self.source_tables} for op in page}
        retained = {op: [] for op in page}

        def retain(op, table, row):
            if row.get("operation_id") is None:
                return
            c.require(op in families, "canonical_coin_query_scope")
            physical_op = c.identifier(row["operation_id"])
            if table == "economic_accounting_account_effect":
                key = physical_op + c.integer(row["account_index"], upper=3071).to_bytes(2, "big")
            elif table in ("economic_accounting_coin_posting", "economic_accounting_item_reference"):
                key = physical_op + c.integer(row["line_index"], upper=6143).to_bytes(2, "big")
            elif table == "economic_accounting_child":
                key = physical_op + c.integer(row["child_index"], lower=1, upper=64).to_bytes(2, "big")
            elif table == "economic_accounting_source_claim":
                key = c.identifier(row["lineage"]) + row["source_event"]
            elif table == "item_ownership_ledger":
                key = physical_op + c.integer(row["event_index"], upper=2999).to_bytes(4, "big")
            else:
                key = physical_op
            families[op][table].append(row)
            retained[op].append(self._retain(table, key, row))

        rows = self._execute("SELECT " + _columns("r", "root_", ROOT_COLUMNS, "recorded_at") + "," +
            _columns("i", "inbox_", INBOX_COLUMNS, "committed_at") +
            " FROM economic_accounting_operation r LEFT JOIN critical_operation_inbox i "
            "ON i.operation_id=r.operation_id WHERE r.operation_id IN (" + markers +
            ") ORDER BY r.operation_id LIMIT %s", (*page, len(page) + 1))
        c.require(len(rows) == len(page), "canonical_coin_root_inbox_fanout")
        for row in rows:
            op = c.identifier(row["root_operation_id"])
            retain(op, "economic_accounting_operation", _family(row, "root_"))
            retain(op, "critical_operation_inbox", _family(row, "inbox_"))
        for table, columns, order, maximum in (
            ("economic_accounting_account_effect", EFFECT_COLUMNS, "operation_id,account_index", 2 * len(page)),
            ("economic_accounting_coin_posting", POSTING_COLUMNS, "operation_id,line_index", 2 * len(page)),
            ("economic_accounting_source_claim", CLAIM_COLUMNS, "operation_id", len(page)),
        ):
            rows = self._execute("SELECT " + ",".join(columns) + " FROM " + table + where +
                                 " ORDER BY " + order + " LIMIT %s", (*page, maximum + 1))
            c.require(len(rows) <= maximum, "canonical_coin_source_fanout")
            for row in rows:
                retain(row["operation_id"], table, row)
        rows = self._execute("SELECT " + _columns("ch", "child_", CHILD_COLUMNS) + "," +
            _columns("i", "inbox_", INBOX_COLUMNS, "committed_at") + "," +
            _columns("cu", "currency_", LEDGER_COLUMNS, "created_at") + "," +
            _columns("it", "item_", ITEM_LEDGER_COLUMNS, "created_at") +
            " FROM economic_accounting_child ch LEFT JOIN critical_operation_inbox i ON i.operation_id=ch.child_operation_id "
            "LEFT JOIN currency_ledger cu ON cu.operation_id=ch.child_operation_id "
            "LEFT JOIN item_ownership_ledger it ON it.operation_id=ch.child_operation_id "
            "WHERE ch.operation_id IN (" + markers + ") ORDER BY ch.operation_id,ch.child_index,it.event_index LIMIT %s",
            (*page, 2 * len(page) + 1))
        c.require(len(rows) <= 2 * len(page), "canonical_coin_child_join_fanout")
        for row in rows:
            op = c.identifier(row["child_operation_id"])
            for table, prefix in (("economic_accounting_child", "child_"), ("critical_operation_inbox", "inbox_"),
                                  ("currency_ledger", "currency_"), ("item_ownership_ledger", "item_")):
                retain(op, table, _family(row, prefix))
        rows = self._execute("SELECT " + ",".join(ITEM_REFERENCE_COLUMNS) +
            " FROM economic_accounting_item_reference" + where + " ORDER BY operation_id,line_index LIMIT %s",
            (*page, 2 * len(page) + 1))
        c.require(len(rows) <= 2 * len(page), "canonical_coin_item_reference_fanout")
        for row in rows:
            retain(row["operation_id"], "economic_accounting_item_reference", row)
        unexpected = self._execute("SELECT operation_id FROM currency_ledger" + where +
            " UNION ALL SELECT operation_id FROM item_ownership_ledger" + where + " LIMIT 1", (*page, *page))
        c.require(not unexpected, "canonical_coin_unexpected_root_legacy_receipt")
        for op in page:
            rows = families[op]
            try:
                c.require(len(rows["economic_accounting_operation"]) == 1, "canonical_coin_root_count")
                event = qualify_coin_root(rows["economic_accounting_operation"][0], rows["critical_operation_inbox"],
                    rows["economic_accounting_account_effect"], rows["economic_accounting_coin_posting"],
                    rows["economic_accounting_source_claim"], rows["economic_accounting_child"],
                    rows["currency_ledger"], rows["economic_accounting_item_reference"], rows["item_ownership_ledger"])
            except (c.EvidenceError, KeyError, TypeError, OverflowError):
                if not preserve_refusals:
                    raise
                event = unavailable_bank_event(op, retained[op])
            events.append(event)
