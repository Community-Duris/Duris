#!/usr/bin/env python3
"""Exact EAB1 origin decoding and SQL read-only snapshot boundary checks."""

import ast
import copy
from decimal import Decimal
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import economic_sql_audit_snapshot as snapshot_exporter
import economic_sql_audit_origins as origin_exporter
from economic_restore_evidence import decode_plan
from economic_sql_audit_origins import OriginError, capture, decode_witness  # noqa: E402
from economic_sql_audit_snapshot import (native_source_count,
                                         read_lineage_realized_prices)  # noqa: E402

LINEAGE = bytes.fromhex("11" * 16)
EPOCH = bytes.fromhex("22" * 16)
OP = hashlib.sha256(bytes.fromhex("44" * 16) + struct.pack("<IQ", 0x42415345, 1)).digest()[:16]


def key(kind, authority, context=0):
    return LINEAGE + struct.pack("<HHQQ4x", 1, kind, authority, context)


OPENING = key(9, 99)


def integer_aliases(value):
    aliases = [float(value), Decimal(value), str(value), None]
    if value in (0, 1):
        aliases.append(bool(value))
    return aliases


def baseline_root(blob, revision=1, accepted_at_usec=123456):
    """Synthetic canonical root for unit/sibling fixtures; native tests use C++."""
    lineage, epoch = blob[16:32], blob[32:48]
    operation = hashlib.sha256(blob[48:64] + struct.pack("<I", 0x42415345) + blob[72:80]).digest()[:16]
    source = struct.pack("<HH", 10, 1) + blob[48:64] + epoch + blob[72:80] + bytes(4)
    actor = struct.unpack_from("<Q", blob, 64)[0]
    holdings, items = struct.unpack_from("<II", blob, 184)
    version = struct.unpack_from("<H", blob, 4)[0]
    stride = 96 if version == 2 else 88
    payload = b"EBC1" + struct.pack("<HHII", 1, 48, len(blob), 0) + hashlib.sha256(blob).digest()
    domain = hashlib.sha256(b"DURIS-ECONOMIC-DOMAIN-V1\0" + struct.pack("<HHI", 20, 1, 48) + payload).digest()
    command = (b"CCM1" + struct.pack("<I", 1) + operation +
               struct.pack("<HHHBBQIII", 20, 1, 6, 4, 0, 1, 1, 0, 48) +
               struct.pack("<B7xQ", 9, 0x45434f4e42415345) + payload)
    binding = hashlib.sha256(b"DURIS-ECONOMIC-COMMAND-V1\0" + command).digest()
    intent = bytearray(256)
    intent[:4] = b"EAI1"
    struct.pack_into("<HHIIIIHBBH", intent, 4, 1, 256, 256, 4, 1, 1, 38, 2, 1, 1)
    intent[32:80] = lineage + epoch + operation
    struct.pack_into("<Q", intent, 96, actor)
    intent[112:160], intent[160:192], intent[192:224] = source, binding, domain
    intent = bytes(intent)
    intent_digest = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
    accounts, postings, equity = [], [], []
    for index in range(holdings):
        record = blob[192 + index * 112:192 + (index + 1) * 112]
        values = struct.unpack_from("<4q", record, 40)
        total = sum(value * unit for value, unit in zip(values, (1, 10, 100, 1000)))
        accounts.append(record[:40] + bytes(32) + record[40:72] + struct.pack("<QQ", 0, 1))
        if total:
            postings.append(struct.pack("<IHH4qq", len(postings), index, 0, *values, total))
            equity.append((values, total))
    if equity:
        accounts.append(blob[80:120] + bytes(80))
        for values, total in equity:
            postings.append(struct.pack("<IHH4qq", len(postings), holdings, 0,
                                        *(-value for value in values), -total))
    snapshots = [blob[192 + holdings * 112 + index * stride:
                      192 + holdings * 112 + index * stride + (64 if version == 2 else 56)] +
                 (b"" if version == 2 else bytes(8)) for index in range(items)]
    counts = (len(accounts), len(postings), 0, items, items, 0)
    plan = bytearray(256)
    plan[:4] = b"EAP1"
    struct.pack_into("<H", plan, 4, 1)
    plan[8:56], plan[72], plan[100] = lineage + epoch + operation, 2, 1
    struct.pack_into("<QIIIH", plan, 76, actor, 4, 1, 1, 38)
    plan[104:152], plan[152:184], plan[184:216] = source, intent_digest, domain
    struct.pack_into("<6I", plan, 216, *counts)
    plan = bytes(plan) + b"".join(accounts + postings + snapshots + snapshots)
    original_command = (b"CCM1" + struct.pack("<I", 2) + operation +
        struct.pack("<HHHBBQIII", 20, 1, 6, 4, 0, accepted_at_usec or 123456, 1, 0, 48) +
        struct.pack("<B7xQ", 9, 0x45434f4e42415345) + payload + struct.pack("<I", 256) + intent)
    return dict(operation_id=operation, book_revision=revision, holding_count=holdings, item_count=items,
                witness_digest=hashlib.sha256(blob).digest(), canonical_witness=blob, witness_version=version,
                reason=38, outcome=1, result_code=0, inbox_status=1, inbox_result=0,
                inbox_failure_stage=0, inbox_committed_at_present=1, inbox_revision=revision,
                inbox_type=20, inbox_schema=2, inbox_payload=1, inbox_result_payload=b"",
                inbox_keys_hash=hashlib.sha256(struct.pack("<BQ", 9, 0x45434f4e42415345)).digest(),
                command_accepted_at_usec=accepted_at_usec, claim_origin_version=None,
                inbox_command_hash=hashlib.sha256(original_command).digest(),
                root_lineage=lineage, root_epoch=epoch, original_operation_id=None,
                accounting_version=1, writer_id=4, policy_version=1, compiler_version=1,
                actor_kind=2, actor_id=actor, source_event=source, intent_digest=intent_digest,
                domain_digest=domain, plan_digest=hashlib.sha256(plan).digest(),
                canonical_intent=intent, canonical_plan=plan,
                account_count=counts[0], posting_count=counts[1], child_count=0,
                before_witness_count=items, after_witness_count=items, item_event_count=0)


def witness(holdings=None):
    holdings = [key(1, 7)] if holdings is None else holdings
    size = 192 + len(holdings) * 112 + 88
    blob = (b"EAB1" + struct.pack("<HHII", 1, 192, size, 0) + LINEAGE + EPOCH +
            bytes.fromhex("44" * 16) + struct.pack("<QQ", 7, 1) + OPENING +
            bytes.fromhex("55" * 32) + bytes.fromhex("66" * 32) +
            struct.pack("<II", len(holdings), 1))
    assert len(blob) == 192
    blob += b"".join(account + struct.pack("<4qQ", 5, 0, 0, 0, 4) + bytes.fromhex("77" * 32)
                     for account in holdings)
    blob += (struct.pack("<QBB6x5Q", 81, 1, 1, 7, 0, 81, 0, 2) +
             bytes.fromhex("88" * 32))
    assert len(blob) == size
    return baseline_root(blob)


def baseline_projections(row):
    """Project a synthetic fixture root; authentic integration rows use native bytes."""
    operation, blob = row["operation_id"], row["canonical_witness"]
    plan = decode_plan(row["canonical_plan"])
    effects = [dict(operation_id=operation, account_index=index, account_key=key,
                    **dict(zip((side + "_" + coin for side in ("before", "after")
                                for coin in ("copper", "silver", "gold", "platinum")), (*before, *after))),
                    before_revision=before_revision, after_revision=after_revision)
               for index, (key, before, after, before_revision, after_revision) in enumerate(plan["effects"])]
    postings = [dict(operation_id=operation, line_index=index, event_index=event,
                     account_index=account, child_index=child,
                     **dict(zip(("delta_" + coin for coin in ("copper", "silver", "gold", "platinum")), delta)),
                     copper_value=amount)
                for index, (event, account, child, delta, amount) in enumerate(plan["postings"])]
    holdings, items = struct.unpack_from("<II", blob, 184)
    stride = 96 if struct.unpack_from("<H", blob, 4)[0] == 2 else 88
    identities = [(1, struct.unpack_from("<Q", blob, 212 + index * 112)[0]) for index in range(holdings)]
    identities += [(2, struct.unpack_from("<Q", blob, 192 + holdings * 112 + index * stride)[0]) for index in range(items)]
    reservations = [dict(lineage=blob[16:32], epoch=blob[32:48], identity_kind=kind,
                         identity_id=identity, operation_id=operation) for kind, identity in identities]
    return effects, postings, reservations


def money_witness():
    """Independent modeled framing; original native vectors are separate proof."""
    blob = bytearray(witness([key(1, 7), key(5, 9), key(5, 10)])["canonical_witness"])

    def framed(value):
        value = value.encode("ascii")
        return struct.pack("<Q", len(value)) + value

    definition = (framed("ESD1") + framed("auction_money_pickups") + framed("pid") +
                  struct.pack("<Q", 3) + b"".join(framed(value) for value in ("pid", "money", "claim_revision")))
    mappings, sources = [], []
    for index, mapping, pid, amount in ((1, 9, 42, 5), (2, 10, 43, 0)):
        struct.pack_into("<4q", blob, 192 + index * 112 + 40, amount, 0, 0, 0)
        original = framed("ESR1") + hashlib.sha256(definition).digest()
        original += b"".join(struct.pack("<Q", 1) + framed(str(value)) for value in (pid, amount, 4))
        blob[192 + index * 112 + 80:192 + (index + 1) * 112] = hashlib.sha256(original).digest()
        mappings.append((mapping, LINEAGE, 1, 5, 5, pid, 0))
        if amount:
            sources.append((index + 1, LINEAGE, mapping, pid, amount))
    row = baseline_root(bytes(blob))
    request = b"DURIS-SQL-LIFECYCLE-V2"
    for value in (blob[48:64], LINEAGE, EPOCH):
        request += struct.pack("<Q", len(value)) + value
    request += blob[64:72] + struct.pack("<Q", row["command_accepted_at_usec"])
    request_hash = hashlib.sha256(request).digest()
    row.update(claim_origin_version=1,
        _claim_parents=[(bytes(blob[48:64]), LINEAGE, EPOCH, bytes(blob[120:152]), request_hash,
                        1, row["operation_id"], request_hash, hashlib.sha256(b"").digest(),
                        20, 2, 1, 1, 0, 0, 0, b"", 1)],
        _claim_mappings=mappings, _claim_sources=sources)
    return row


class Cursor:
    def __init__(self, rows):
        self.rows = rows
        self.statements = []
        self.index = -1
        self.closed = False
        self.admission_column_count = 1
        self.metadata_query = False
        self.claim_query = None
        self.claim_policy_column_count = 1
        self.claim_parents = []
        self.claim_mappings = []
        self.claim_sources = []

    def execute(self, statement, params=None):
        self.metadata_query = 'information_schema.columns' in statement
        self.claim_query = next((name for name in ("economic_sql_lifecycle_installation",
            "economic_account_mapping", "economic_pending_claim_source") if " FROM " + name in statement), None)
        if not self.metadata_query and not self.claim_query:
            self.index += 1
        self.statements.append((statement, params))

    def fetchone(self):
        if self.metadata_query:
            count = self.claim_policy_column_count if "claim_origin_version" in self.statements[-1][0] else self.admission_column_count
            return {"column_count": count}
        return self.rows[self.index]

    def fetchall(self):
        if self.claim_query:
            if self.claim_query == "economic_sql_lifecycle_installation":
                return [dict(zip(("policy_" + str(index) for index in range(18)), row)) for row in self.claim_parents]
            if self.claim_query == "economic_account_mapping":
                return [dict(zip(origin_exporter.CLAIM_MAPPING_COLUMNS, row)) for row in self.claim_mappings]
            return [dict(zip(("source_slot", "lineage", "claim_mapping_id", "beneficiary_pid", "amount"), row))
                    for row in self.claim_sources]
        return self.rows[self.index]

    def close(self):
        self.closed = True


class Connection:
    def __init__(self, control=None, rows=None):
        rows = [witness()] if rows is None else rows
        projections = [[], [], []]
        for row in rows:
            # Invalid capsule tests still exercise the root reader first.
            try:
                details = baseline_projections(row)
            except (ValueError, struct.error):
                details = ([], [], [])
            for family, values in zip(projections, details):
                family.extend(values)
        self.scan = Cursor([
            None, None,
            [{"table_name": name, "engine": "InnoDB"} for name in
             ("economic_baseline_control", "economic_baseline_witness",
              "economic_accounting_operation", "critical_operation_inbox",
              "economic_accounting_account_effect", "economic_accounting_coin_posting", "economic_baseline_reservation",
              "economic_accounting_child", "economic_accounting_item_reference", "currency_ledger",
              "item_ownership_ledger", "critical_outbox", "economic_sql_lifecycle_installation",
              "economic_account_mapping", "economic_pending_claim_source")],
            {"opening_account": OPENING, "revision": 1, "last_operation_id": OP}
            if control is None else control,
            {"row_count": len(rows),
             "blob_bytes": sum(len(row[name]) for row in rows
                               for name in ("canonical_witness", "canonical_intent", "canonical_plan"))},
            rows,
            {"projection_rows": sum(map(len, projections))},
            *projections,
            {"effect_" + str(index): 0 for index in range(6)},
        ])
        self.scan.claim_parents = rows[0].get("_claim_parents", []) if rows else []
        self.scan.claim_mappings = rows[0].get("_claim_mappings", []) if rows else []
        self.scan.claim_sources = rows[0].get("_claim_sources", []) if rows else []
        self.rollbacks = 0

    def cursor(self):
        return self.scan

    def rollback(self):
        self.rollbacks += 1


class CoinPayloadTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Reuse the existing SQL fixture's framing without starting a database.
        fixture = ROOT / "tests/async/run_economic_sql_audit_snapshot_mysql.py"
        function, = [node for node in ast.parse(fixture.read_text()).body
                     if isinstance(node, ast.FunctionDef) and node.name == "coin_payload"]
        namespace = {"struct": struct}
        exec(compile(ast.Module(body=[function], type_ignores=[]), str(fixture), "exec"), namespace)
        cls.payload = staticmethod(namespace["coin_payload"])

    def test_area_money_prototypes_preserve_exact_denominations(self):
        for vnum in (1, 3, 402013, 402014, 2**31 - 1):
            blob = self.payload(82, [1, 2, 3, 4], vnum)
            before = bytes(blob)
            self.assertEqual(snapshot_exporter.decode_coin_payload(blob, 82, vnum), [1, 2, 3, 4])
            self.assertEqual(blob, before)

    def test_area_money_binds_uid_and_native_prototype(self):
        blob = self.payload(82, [1, 2, 3, 4], 402013)
        for uid, vnum in ((83, 402013), (82, 402014), (82, 3)):
            with self.subTest(uid=uid, vnum=vnum), self.assertRaises(snapshot_exporter.ExportError):
                snapshot_exporter.decode_coin_payload(blob, uid, vnum)

    def test_area_money_refuses_noncanonical_prototype_representations(self):
        for vnum in (True, 1.0, "1", None, 0, -1, 2**31):
            with self.subTest(vnum=vnum), self.assertRaises(snapshot_exporter.ExportError):
                snapshot_exporter.decode_coin_payload(self.payload(82, [1, 2, 3, 4], 1), 82, vnum)

    def test_area_money_refuses_nonmoney_and_corrupt_literals(self):
        original = self.payload(82, [1, 2, 3, 4], 402013)
        nonmoney = bytearray(original)
        nonmoney[30] = 19
        for blob in (bytes(nonmoney), original[:-1], original + b"\0",
                     self.payload(82, [-1, 2, 3, 4], 402013)):
            with self.subTest(blob=blob), self.assertRaises(snapshot_exporter.ExportError):
                snapshot_exporter.decode_coin_payload(blob, 82, 402013)

    def test_coin_row_budget_includes_item_and_every_nested_vector(self):
        cases = ((8191, ()), (0, (0,) * 8191), (0, (8190,)))
        for vnum in (3, 402013):
            for dynamic, spells in cases:
                with self.subTest(vnum=vnum, dynamic=dynamic, spells=len(spells)):
                    blob = self.payload(82, [1, 2, 3, 4], vnum,
                                        dynamic_count=dynamic, spell_counts=spells)
                    self.assertEqual(snapshot_exporter.decode_coin_payload(blob, 82, vnum), [1, 2, 3, 4])
            for dynamic, spells in ((8192, ()), (0, (0,) * 8192), (0, (8191,))):
                with self.subTest(vnum=vnum, dynamic=dynamic, spells=len(spells)):
                    blob = self.payload(82, [1, 2, 3, 4], vnum,
                                        dynamic_count=dynamic, spell_counts=spells)
                    with self.assertRaisesRegex(snapshot_exporter.ExportError, "nested row count exceeds limit"):
                        snapshot_exporter.decode_coin_payload(blob, 82, vnum)

    def test_coin_row_budget_is_shared_across_collections_and_descriptions(self):
        for vnum in (3, 402013):
            for dynamic, spells in ((4095, (4095,)), (0, (4094, 4095))):
                with self.subTest(vnum=vnum, dynamic=dynamic, spells=spells):
                    blob = self.payload(82, [1, 2, 3, 4], vnum,
                                        dynamic_count=dynamic, spell_counts=spells)
                    self.assertEqual(snapshot_exporter.decode_coin_payload(blob, 82, vnum), [1, 2, 3, 4])
            for dynamic, spells in ((4095, (4096,)), (0, (4095, 4095)), (8190, (0, 0))):
                with self.subTest(vnum=vnum, dynamic=dynamic, spells=spells):
                    blob = self.payload(82, [1, 2, 3, 4], vnum,
                                        dynamic_count=dynamic, spell_counts=spells)
                    with self.assertRaisesRegex(snapshot_exporter.ExportError, "nested row count exceeds limit"):
                        snapshot_exporter.decode_coin_payload(blob, 82, vnum)


class ItemRevisionTests(unittest.TestCase):
    @staticmethod
    def event(uid, revision, owner_revision):
        return {"operation_id": OP, "event_index": 0, "item_uid": uid,
                "root_item_uid": uid, "parent_item_uid": None,
                "to_owner_type": 1, "to_owner_id": 7, "to_owner_context_id": 0,
                "from_equipment_slot": 0, "to_equipment_slot": 0,
                "item_revision": revision, "from_owner_revision": owner_revision,
                "reason_type": 1, "operation_epoch": EPOCH, "operation_outcome": 1}

    def census(self, origins, events, unattributed=False):
        ownership = [{"item_uid": row["item_uid"],
                      "lineage": None if unattributed else LINEAGE} for row in events]
        rows = [ownership, [], events] if unattributed else [ownership, events]
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=rows):
            return snapshot_exporter.read_uid_event_census(
                None, LINEAGE, origins, {"item_references": []}, [], [])

    def test_item_history_cut_uses_uid_revision_not_owner_revision(self):
        origins = [{"uid": 81, "revision": 3}]
        # Old event must be excluded despite a high aggregate counter; the
        # event after the witness must survive despite a lower owner counter.
        result = self.census(origins, [self.event(81, 3, 99), self.event(81, 4, 1)])
        self.assertEqual([(row["before_revision"], row["revision"])
                          for row in result[0]], [(3, 4)])
        self.assertEqual(result[2]["ledger_events"], 1)
        self.assertEqual(result[1], result[0])

    def test_unattributed_history_uses_uid_revision(self):
        result = self.census([{"uid": 81, "revision": 3}],
                             [self.event(81, 3, 99), self.event(81, 4, 1)], True)
        self.assertEqual([(row["before_revision"], row["revision"])
                          for row in result[6]], [(3, 4)])
        self.assertEqual(result[7], {"uids": 1, "events": 1})

    def test_lineage_reference_uses_individual_item_revision(self):
        row = {**self.event(81, 4, 99), "child_index": 0,
               "before_revision": 3, "after_revision": 4,
               "legacy_operation_id": OP, "legacy_event_index": 0,
               "epoch": EPOCH, "outcome": 1, "item_event_count": 1,
               "ledger_uid": 81}
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=[[row], []]):
            references, _, _ = snapshot_exporter.read_lineage_uid_references(None, LINEAGE)
        self.assertEqual(references[0]["ledger_before_revision"], 3)
        row["ledger_uid"] = None
        row["item_revision"] = None
        with mock.patch.object(snapshot_exporter, "bounded", side_effect=[[row], []]):
            references, _, _ = snapshot_exporter.read_lineage_uid_references(None, LINEAGE)
        self.assertIsNone(references[0]["ledger_before_revision"])

    def test_zero_item_revision_refuses_before_history_filter(self):
        for unattributed in (False, True):
            with self.subTest(unattributed=unattributed):
                with self.assertRaisesRegex(snapshot_exporter.ExportError,
                                            "invalid native item ledger revision"):
                    self.census([{"uid": 81, "revision": 3}],
                                [self.event(81, 0, 99)], unattributed)


class PartialClaimExportTests(unittest.TestCase):
    def test_bounded_exact_partial_rows_are_read_only_and_keep_identities(self):
        from economic_sql_audit_snapshot import read_pending_claim_consumptions, ExportError
        class Cursor:
            def __init__(self, rows):
                self.rows, self.calls = rows, []
            def execute(self, query, params):
                assert query.startswith("SELECT ")
                self.calls.append((query, params))
            def fetchall(self):
                return self.rows
        row = {"spending_operation_id": bytes.fromhex("33"*16),
            "source_operation_id": bytes.fromhex("44"*16), "source_slot": 1, "amount": 2}
        cursor = Cursor([row])
        rows, coverage = read_pending_claim_consumptions(cursor, LINEAGE)
        self.assertEqual(rows, [{"spending_operation_id": "33"*16,
            "source_operation_id": "44"*16, "source_slot": 1, "amount": 2}])
        self.assertEqual(coverage, {"rows": 1})
        self.assertEqual(cursor.calls[0][1], (LINEAGE, LINEAGE, 100001))
        for field, values in (("spending_operation_id", (None, bytes(16), "33"*16)),
                ("source_operation_id", (None, bytes(16), b"short")),
                ("source_slot", (True, 1.0, 0, 65536)), ("amount", (True, 2.0, 0, 2**64))):
            for value in values:
                with self.subTest(field=field, value=value), self.assertRaises(ExportError):
                    read_pending_claim_consumptions(Cursor([{**row, field: value}]), LINEAGE)
        with self.assertRaisesRegex(ExportError, "collection exceeds row limit"):
            read_pending_claim_consumptions(Cursor([row]*100001), LINEAGE)


class OriginTests(unittest.TestCase):
    def test_money_opening_policy_original_pid_and_zero_claim_read_only(self):
        original = money_witness()
        connection = Connection(rows=[original])
        result = capture(connection, LINEAGE, EPOCH)
        self.assertEqual([holding["balance"][0] for holding in result["account_origins"]], [5, 5, 0])
        self.assertEqual(connection.rollbacks, 1)
        for index in (0, 1):
            row = copy.deepcopy(original)
            mapping = list(row["_claim_mappings"][index])
            mapping[5] += 100
            row["_claim_mappings"][index] = tuple(mapping)
            if index == 0:
                source = list(row["_claim_sources"][0])
                source[3] += 100
                row["_claim_sources"][0] = tuple(source)
            before = copy.deepcopy(row)
            connection = Connection(rows=[row])
            with self.subTest(zero=index == 1), self.assertRaisesRegex(OriginError, "claim origin mismatch"):
                capture(connection, LINEAGE, EPOCH)
            self.assertEqual(row, before)
            self.assertEqual(connection.rollbacks, 1)
            self.assertTrue(all(statement.startswith(("SELECT ", "SET TRANSACTION ", "START TRANSACTION "))
                                for statement, _ in connection.scan.statements))

    def test_money_opening_policy_requires_original_lifecycle_receipt(self):
        original = money_witness()
        cases = [("claim_origin_version", None), ("command_accepted_at_usec", None),
                 ("_claim_parents", []), ("_claim_mappings", []), ("_claim_sources", [])]
        for field, value in cases:
            row = copy.deepcopy(original)
            row[field] = value
            with self.subTest(field=field), self.assertRaisesRegex(OriginError, "claim origin mismatch"):
                capture(Connection(rows=[row]), LINEAGE, EPOCH)
        for index, value in enumerate(original["_claim_parents"][0]):
            alternatives = integer_aliases(value) if type(value) is int else [None, b"bad"]
            if index == 6:
                alternatives = [bytes(16), "bad"]  # NULL baseline ID is an original supported phase.
            for damage in alternatives:
                row = copy.deepcopy(original)
                parent = list(row["_claim_parents"][0])
                parent[index] = damage
                row["_claim_parents"] = [tuple(parent)]
                with self.subTest(index=index, damage=damage), self.assertRaisesRegex(OriginError, "claim origin mismatch"):
                    capture(Connection(rows=[row]), LINEAGE, EPOCH)
        for phase, operation in ((1, None), (2, original["operation_id"])):
            row = copy.deepcopy(original)
            parent = list(row["_claim_parents"][0])
            parent[5:7] = [phase, operation]
            row["_claim_parents"] = [tuple(parent)]
            capture(Connection(rows=[row]), LINEAGE, EPOCH)
        for value in (0, 2, True, 1.0, "1", None):
            connection = Connection(rows=[original])
            connection.scan.claim_policy_column_count = value
            with self.subTest(column_count=value), self.assertRaisesRegex(OriginError, "claim (policy column metadata|origin mismatch)"):
                # A missing column behaves like SQL NULL rather than a new-policy waiver.
                if value == 0:
                    connection = Connection(rows=[{**original, "claim_origin_version": None}])
                    connection.scan.claim_policy_column_count = 0
                capture(connection, LINEAGE, EPOCH)

    def test_original_admission_time_and_full_command_hash_refuse_read_only(self):
        intact = witness()
        cuts = [("command_accepted_at_usec", value) for value in
                (0, -1, 2**64, True, 123456.0, "123456", 123457)]
        cuts += [("inbox_command_hash", value) for value in
                 (None, bytes(32), b"x" * 31, b"x" * 33, bytearray(intact["inbox_command_hash"]),
                  bytes([intact["inbox_command_hash"][0] ^ 1]) + intact["inbox_command_hash"][1:])]
        for field, value in cuts:
            with self.subTest(field=field, representation=type(value).__name__):
                row = witness()
                row[field] = value
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertIs(row[field], value)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
                self.assertTrue(all(sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                    for sql, _ in connection.scan.statements))
        for field in ("command_accepted_at_usec", "inbox_command_hash"):
            row = witness()
            del row[field]
            with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                origin_exporter.verify_baseline_root(row, LINEAGE, EPOCH)

    def test_historical_null_admission_time_is_preserved_without_inference(self):
        for column_count in (0, 1):
            with self.subTest(column_count=column_count):
                row = baseline_root(witness()["canonical_witness"], accepted_at_usec=None)
                connection = Connection(rows=[row])
                connection.scan.admission_column_count = column_count
                self.assertEqual(capture(connection, LINEAGE, EPOCH)["witness_count"], 1)
                self.assertIsNone(row["command_accepted_at_usec"])
                selected = next(sql for sql, _ in connection.scan.statements if 'AS inbox_command_hash' in sql)
                self.assertIn(('w.command_accepted_at_usec' if column_count else 'NULL') +
                              ' AS command_accepted_at_usec', selected)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)

    def test_admission_column_metadata_requires_exact_bounded_count(self):
        for value in (-1, 2, True, 1.0, "1", None):
            with self.subTest(value=value):
                connection = Connection()
                connection.scan.admission_column_count = value
                with self.assertRaisesRegex(OriginError, "invalid SQL baseline admission column metadata"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)

    def test_baseline_projection_integer_representations_refuse_read_only(self):
        for index in (7, 8, 9):
            for field, value in Connection().scan.rows[index][0].items():
                if type(value) is not int:
                    continue
                for alias in integer_aliases(value):
                    with self.subTest(family=index, field=field, representation=type(alias).__name__):
                        connection = Connection()
                        connection.scan.rows[index][0][field] = alias
                        with self.assertRaisesRegex(OriginError, "EAB1 SQL projection mismatch"):
                            capture(connection, LINEAGE, EPOCH)
                        self.assertIs(connection.scan.rows[index][0][field], alias)
                        self.assertEqual(connection.rollbacks, 1)
                        self.assertTrue(connection.scan.closed)
                        self.assertTrue(all(sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                            for sql, _ in connection.scan.statements))

    def test_baseline_root_integer_representations_refuse_read_only(self):
        fields = ("accounting_version", "writer_id", "policy_version", "compiler_version", "actor_kind",
                  "actor_id", "reason", "account_count", "posting_count", "child_count", "before_witness_count",
                  "after_witness_count", "item_event_count", "book_revision", "inbox_revision", "inbox_type",
                  "inbox_schema", "inbox_payload", "outcome", "result_code", "inbox_status", "inbox_result",
                  "inbox_failure_stage", "inbox_committed_at_present")
        for field in fields:
            for alias in integer_aliases(witness()[field]):
                with self.subTest(field=field, representation=type(alias).__name__):
                    row = witness()
                    row[field] = alias
                    connection = Connection(rows=[row])
                    with self.assertRaises(OriginError):
                        capture(connection, LINEAGE, EPOCH)
                    self.assertIs(row[field], alias)
                    self.assertEqual(connection.rollbacks, 1)
                    self.assertTrue(connection.scan.closed)
        # The restore reader calls the root verifier without the SQL read loop.
        for field in ("book_revision", "inbox_revision"):
            for value in (*integer_aliases(1), 0, -1, 2**64):
                with self.subTest(direct=field, value=value):
                    row = witness()
                    row[field] = value
                    with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                        origin_exporter.verify_baseline_root(row, LINEAGE, EPOCH)

    def test_baseline_binary_projection_representations_refuse(self):
        for field in ("root_lineage", "root_epoch", "operation_id", "source_event", "domain_digest",
                      "intent_digest", "plan_digest", "inbox_result_payload"):
            with self.subTest(root=field):
                row = witness()
                row[field] = bytearray(row[field])
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
        for index in (7, 8, 9):
            for field, value in Connection().scan.rows[index][0].items():
                if type(value) is not bytes:
                    continue
                with self.subTest(family=index, field=field):
                    connection = Connection()
                    connection.scan.rows[index][0][field] = bytearray(value)
                    with self.assertRaisesRegex(OriginError, "EAB1 SQL projection mismatch"):
                        capture(connection, LINEAGE, EPOCH)
                    self.assertEqual(connection.rollbacks, 1)
                    self.assertTrue(connection.scan.closed)

    def test_baseline_source_bounds_require_nonnegative_exact_integers(self):
        for index, field in ((4, "row_count"), (4, "blob_bytes"), (6, "projection_rows")):
            for alias in (*integer_aliases(Connection().scan.rows[index][field]), -1):
                with self.subTest(field=field, representation=type(alias).__name__, value=alias):
                    connection = Connection()
                    connection.scan.rows[index][field] = alias
                    with self.assertRaisesRegex(OriginError, "source exceeds audit input limit"):
                        capture(connection, LINEAGE, EPOCH)
                    self.assertEqual(connection.rollbacks, 1)
                    self.assertTrue(connection.scan.closed)

    def test_resealed_command_binding_substitution_refuses_read_only(self):
        for offset in range(160, 192):
            with self.subTest(offset=offset):
                row = witness()
                intent = bytearray(row["canonical_intent"])
                intent[offset] ^= 1
                row["canonical_intent"] = bytes(intent)
                row["intent_digest"] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
                plan = bytearray(row["canonical_plan"])
                plan[152:184] = row["intent_digest"]
                row["canonical_plan"] = bytes(plan)
                row["plan_digest"] = hashlib.sha256(plan).digest()
                self.assertEqual(decode_plan(row["canonical_plan"])["intent_digest"], row["intent_digest"])
                original = copy.deepcopy(row)
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(row, original)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
                self.assertTrue(all(sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                    for sql, _ in connection.scan.statements))

    def test_baseline_native_events_children_and_outbox_refuse_read_only(self):
        for index, (table, column) in enumerate(origin_exporter.BASELINE_ZERO_EFFECTS):
            for value in (1, -1, None, True):
                with self.subTest(table=table, column=column, value=value):
                    connection = Connection()
                    connection.scan.rows[10]["effect_" + str(index)] = value
                    with self.assertRaisesRegex(OriginError, "EAB1 SQL zero-effect mismatch: " + table + "." + column):
                        capture(connection, LINEAGE, EPOCH)
                    self.assertEqual(connection.rollbacks, 1)
                    self.assertTrue(connection.scan.closed)
                    self.assertTrue(all(sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                        for sql, _ in connection.scan.statements))
        connection = Connection()
        connection.scan.rows[10] = None
        with self.assertRaisesRegex(OriginError, "EAB1 SQL zero-effect mismatch"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)

    def test_baseline_zero_effect_sources_require_transactional_tables(self):
        for index in range(7, 12):
            for missing in (False, True):
                with self.subTest(index=index, missing=missing):
                    connection = Connection()
                    if missing:
                        connection.scan.rows[2].pop(index)
                    else:
                        connection.scan.rows[2][index]["engine"] = "MyISAM"
                    with self.assertRaisesRegex(OriginError, "missing or not InnoDB"):
                        capture(connection, LINEAGE, EPOCH)
                    self.assertEqual(connection.rollbacks, 1)
                    self.assertTrue(connection.scan.closed)

    def test_missing_extra_duplicate_and_altered_sql_baseline_projections_refuse(self):
        for index in (7, 8, 9):
            connection = Connection()
            for field in connection.scan.rows[index][0]:
                with self.subTest(family=index, field=field):
                    changed = Connection()
                    previous = changed.scan.rows[index][0][field]
                    changed.scan.rows[index][0][field] = (previous + 1 if isinstance(previous, int) else
                                                        bytes([previous[0] ^ 1]) + previous[1:])
                    with self.assertRaisesRegex(OriginError, "EAB1 SQL projection mismatch"):
                        capture(changed, LINEAGE, EPOCH)
                    self.assertEqual(changed.rollbacks, 1)
                    self.assertTrue(changed.scan.closed)
            for change in ("missing", "extra", "duplicate"):
                with self.subTest(family=index, change=change):
                    changed = Connection()
                    values = changed.scan.rows[index]
                    if change == "missing":
                        values.pop()
                    else:
                        values.append(copy.deepcopy(values[0]))
                        if change == "extra":
                            field = ("account_index", "line_index", "identity_id")[index - 7]
                            values[-1][field] += 100
                    changed.scan.rows[6]["projection_rows"] = sum(map(len, changed.scan.rows[7:10]))
                    with self.assertRaisesRegex(OriginError, "EAB1 SQL projection mismatch"):
                        capture(changed, LINEAGE, EPOCH)
                    self.assertEqual(changed.rollbacks, 1)
                    self.assertTrue(changed.scan.closed)

    def test_sql_baseline_projection_bounds_and_transactional_tables(self):
        connection = Connection()
        connection.scan.rows[6]["projection_rows"] = origin_exporter.MAX_ROWS + 1
        with self.assertRaisesRegex(OriginError, "baseline SQL projection source"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(len(connection.scan.statements), 9)
        for index in (4, 5, 6):
            with self.subTest(table=index):
                connection = Connection()
                connection.scan.rows[2][index]["engine"] = "MyISAM"
                with self.assertRaisesRegex(OriginError, "not InnoDB"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
        with mock.patch.object(origin_exporter, "MAX_ROWS", 5):
            with self.assertRaisesRegex(OriginError, "baseline SQL projection source"):
                capture(Connection(), LINEAGE, EPOCH)

    def test_rehashed_witness_and_canonical_root_disagreement_refuse(self):
        for offset in (64, 120, 152, 232, 264, 272, 320, 344, 360):
            with self.subTest(offset=offset):
                row = witness()
                blob = bytearray(row["canonical_witness"])
                blob[offset] ^= 1
                row["canonical_witness"] = bytes(blob)
                row["witness_digest"] = hashlib.sha256(blob).digest()
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
        for field, changed in (("root_lineage", bytes([1]) * 16), ("root_epoch", bytes([2]) * 16),
                               ("writer_id", 5), ("accounting_version", 2), ("policy_version", 2),
                               ("compiler_version", 2), ("actor_kind", 1), ("actor_id", 8),
                               ("original_operation_id", bytes(16)), ("source_event", None),
                               ("domain_digest", bytes([1]) * 32), ("intent_digest", bytes([1]) * 32),
                               ("plan_digest", bytes([1]) * 32), ("canonical_intent", b"EAI1"),
                               ("canonical_plan", b"EAP1"), ("account_count", 1),
                               ("inbox_revision", 2), ("inbox_type", 1), ("inbox_schema", 1),
                               ("inbox_payload", 2), ("inbox_result_payload", b"x"), ("witness_version", 2)):
            with self.subTest(field=field):
                row = witness()
                row[field] = changed
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)

    def test_zero_opening_vectors_bind_without_equity(self):
        blob = bytearray(witness()["canonical_witness"])
        blob[232:264] = bytes(32)
        row = baseline_root(bytes(blob))
        result = capture(Connection(rows=[row]), LINEAGE, EPOCH)
        self.assertEqual(result["account_origins"][0]["balance"], [0, 0, 0, 0])
        self.assertEqual(row["posting_count"], 0)
        self.assertEqual(row["account_count"], 1)

    def test_baseline_receipt_keys_hash_requires_exact_native_system_fence(self):
        keys = struct.pack("<BQ", 9, 0x45434f4e42415345)
        valid = hashlib.sha256(keys).digest()
        wrong = [None, b"", bytes(31), bytes(33), bytes(32),
                 hashlib.sha256(struct.pack("<BQ", 8, 0x45434f4e42415345)).digest(),
                 hashlib.sha256(struct.pack("<BQ", 9, 0x45434f4e42415344)).digest(),
                 hashlib.sha256(struct.pack(">BQ", 9, 0x45434f4e42415345)).digest(),
                 hashlib.sha256(struct.pack("<B7xQ", 9, 0x45434f4e42415345)).digest(),
                 hashlib.sha256(struct.pack("<I", 1) + keys).digest()]
        for index in range(32):
            changed = bytearray(valid)
            changed[index] ^= 1
            wrong.append(bytes(changed))
        for index, changed in enumerate(wrong):
            with self.subTest(case=index):
                row = witness()
                row["inbox_keys_hash"] = changed
                before = copy.deepcopy(row)
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(row, before)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
        row = witness()
        del row["inbox_keys_hash"]
        with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
            capture(Connection(rows=[row]), LINEAGE, EPOCH)

    def test_baseline_claim_books_are_cached_and_globally_bounded(self):
        current, retained = EPOCH.hex(), "88" * 16
        claims = [{"operation_reason": 38, "operation_id": OP.hex(),
                   "operation_lineage": LINEAGE.hex(), "operation_epoch": value}
                  for value in (current, retained, retained)]
        cursor = mock.Mock()
        cursor.fetchone.return_value = {"row_count": 2, "blob_bytes": 1000, "projection_rows": 6}
        origins = {"baseline_source_events": {OP.hex(): "aa" * 48}}
        with mock.patch.object(snapshot_exporter, "read_origins_in_transaction", return_value=origins) as read:
            snapshot_exporter.bind_baseline_claim_witnesses(cursor, LINEAGE, EPOCH, claims, origins)
            read.assert_called_once_with(cursor, LINEAGE, bytes.fromhex(retained))
        self.assertEqual([row["baseline_witness"]["epoch"] for row in claims], [current, retained, retained])
        for bounds in ({"row_count": 100_001, "blob_bytes": 1},
                       {"row_count": 1, "blob_bytes": snapshot_exporter.MAX_INPUT_BYTES + 1}):
            cursor.fetchone.return_value = bounds
            with mock.patch.object(snapshot_exporter, "read_origins_in_transaction") as read:
                with self.assertRaisesRegex(snapshot_exporter.ExportError, "baseline claim witness source"):
                    snapshot_exporter.bind_baseline_claim_witnesses(cursor, LINEAGE, EPOCH, claims, origins)
                read.assert_not_called()
        cursor.fetchone.return_value = {"row_count": 2, "blob_bytes": 1000, "projection_rows": 6}
        with mock.patch.object(snapshot_exporter, "read_origins_in_transaction", side_effect=OriginError("missing book")):
            snapshot_exporter.bind_baseline_claim_witnesses(cursor, LINEAGE, EPOCH, claims, origins)
        self.assertIsNone(claims[1]["baseline_witness"])
        self.assertIsNone(claims[2]["baseline_witness"])
        self.assertTrue(all(call.args[0].startswith("SELECT ") for call in cursor.execute.call_args_list))
        cursor.fetchone.return_value = {"row_count": 2, "blob_bytes": 1000, "projection_rows": 100_001}
        with mock.patch.object(snapshot_exporter, "read_origins_in_transaction") as read:
            with self.assertRaisesRegex(snapshot_exporter.ExportError, "baseline claim projection source"):
                snapshot_exporter.bind_baseline_claim_witnesses(cursor, LINEAGE, EPOCH, claims, origins)
            read.assert_not_called()

    def test_native_numeric_holding_order_at_unsigned_boundaries(self):
        for first, second in ((255, 256), (65535, 65536), (2**32 - 1, 2**32),
                              (2**63 - 1, 2**63), (2**64 - 2, 2**64 - 1)):
            with self.subTest(first=first, second=second):
                keys = [key(1, first), key(1, second)]
                row = witness(keys)
                holdings, _ = decode_witness(row, LINEAGE, EPOCH, OPENING)
                self.assertEqual([h["account_key"] for h in holdings], [k.hex() for k in keys])
                connection = Connection(rows=[row])
                self.assertEqual(capture(connection, LINEAGE, EPOCH)["account_origins"], holdings)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)

    def test_byte_ordered_numeric_regression_refuses_and_rolls_back(self):
        for lifetimes in ((256, 1), (256, 512, 255), (65536, 65535), (2**32, 2**32 - 1)):
            with self.subTest(lifetimes=lifetimes):
                keys = [key(1, lifetime) for lifetime in lifetimes]
                self.assertEqual(keys, sorted(keys))
                row = witness(keys)
                with self.assertRaisesRegex(OriginError, "invalid or duplicate EAB1 holding"):
                    decode_witness(row, LINEAGE, EPOCH, OPENING)
                connection = Connection(rows=[row])
                with self.assertRaisesRegex(OriginError, "invalid or duplicate EAB1 holding"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)

    def test_account_kind_order_precedes_unsigned_lifetime_and_context(self):
        keys = [key(1, 2**64 - 1, 2**64 - 1), key(2, 1, 2**64 - 1)]
        holdings, _ = decode_witness(witness(keys), LINEAGE, EPOCH, OPENING)
        self.assertEqual([h["account_key"] for h in holdings], [k.hex() for k in keys])
        with self.assertRaisesRegex(OriginError, "invalid or duplicate EAB1 holding"):
            decode_witness(witness(list(reversed(keys))), LINEAGE, EPOCH, OPENING)

    def test_duplicate_lifetimes_stay_invalid_across_kinds_and_contexts(self):
        for keys in ([key(1, 7), key(1, 7)], [key(1, 7), key(1, 7, 1)],
                     [key(1, 7), key(2, 7)]):
            with self.subTest(keys=keys):
                with self.assertRaisesRegex(OriginError, "invalid or duplicate EAB1 holding"):
                    decode_witness(witness(keys), LINEAGE, EPOCH, OPENING)

    def test_orphan_export_is_bounded_and_does_not_invent_a_lineage(self):
        cursor = mock.Mock()
        cursor.fetchall.side_effect = [[{"operation_id": OP, "row_index": 3}], [], [], [], []]
        rows, coverage = snapshot_exporter.read_orphan_evidence(cursor)
        self.assertEqual(rows, [{"table": "effects", "operation_id": OP.hex(), "row_index": 3}])
        self.assertEqual(coverage, {"scope": "database", "table_counts": {
            "effects": 1, "postings": 0, "children": 0, "item_references": 0, "baseline_reservations": 0}})
        for call in cursor.execute.call_args_list:
            sql, params = call.args
            self.assertTrue(sql.startswith("SELECT "))
            self.assertIn("WHERE o.operation_id IS NULL", sql)
            self.assertNotIn("lineage=%s", sql)
            self.assertIn("LIMIT %s", sql)
            self.assertEqual(params, (snapshot_exporter.MAX_ROWS + 1,))
        reservation_query = cursor.execute.call_args_list[-1].args[0]
        self.assertIn("LEFT JOIN economic_baseline_witness", reservation_query)
        self.assertIn("AND o.lineage=e.lineage AND o.epoch=e.epoch", reservation_query)
        self.assertIn("e.identity_id", reservation_query)
        cursor.fetchall.side_effect = [[{"operation_id": OP, "row_index": 0}],
                                      [{"operation_id": OP, "row_index": 1}]]
        with mock.patch.object(snapshot_exporter, "MAX_ROWS", 1):
            with self.assertRaisesRegex(snapshot_exporter.ExportError, "orphan evidence collection"):
                snapshot_exporter.read_orphan_evidence(cursor)

    def test_rootless_reservation_export_preserves_untrusted_scope_and_uint64(self):
        cursor = mock.Mock()
        cursor.fetchall.side_effect = [[], [], [], [], [{"operation_id": OP, "row_index": 2,
            "identity_id": 2**64 - 1, "claimed_lineage": bytes([77]) * 16, "claimed_epoch": bytes([88]) * 16}]]
        rows, coverage = snapshot_exporter.read_orphan_evidence(cursor)
        self.assertEqual(rows, [{"table": "baseline_reservations", "operation_id": OP.hex(), "row_index": 2,
            "identity_id": 2**64 - 1, "claimed_lineage": "4d" * 16, "claimed_epoch": "58" * 16}])
        self.assertEqual(coverage["table_counts"]["baseline_reservations"], 1)
        self.assertNotIn("lineage", rows[0])
        self.assertNotIn("epoch", rows[0])
        self.assertNotIn("lineage=%s", cursor.execute.call_args_list[-1].args[0])

    def test_native_mapping_coverage_is_scoped_to_selected_lineage(self):
        class Cursor:
            query = None
            parameters = None

            def execute(self, query, parameters):
                self.query = query
                self.parameters = parameters

            def fetchone(self):
                return {"source_rows": 2, "unmapped_rows": 1}

        cursor = Cursor()
        self.assertEqual(native_source_count(cursor, LINEAGE, "player_data", "pid", 1),
                         (2, 1))
        self.assertIn("m.lineage=%s", cursor.query)
        self.assertEqual(cursor.parameters, (LINEAGE,))

    def test_realized_price_query_uses_registry_candidate_reasons(self):
        class Cursor:
            query = None
            parameters = None

            def execute(self, query, parameters):
                self.query = query
                self.parameters = parameters

            def fetchall(self):
                return []

        registry = json.loads((ROOT / "docs/persistence/economy_accounting/registry.json")
                              .read_text(encoding="utf-8"))
        expected = [row["number"] for row in registry["reasons"]
                    if row.get("realized_price_required") is True]
        cursor = Cursor()
        rows, coverage = read_lineage_realized_prices(cursor, LINEAGE, True)
        self.assertEqual(rows, [])
        self.assertEqual(coverage["candidate_rows"], 0)
        self.assertIn("o.reason IN (" + ",".join("%s" for _ in expected) + ")",
                      cursor.query)
        self.assertEqual(cursor.parameters[:-1], (LINEAGE, *expected))
        self.assertEqual(cursor.parameters[-1], 100_001)

    def test_exact_witness_and_read_only_capture(self):
        account_origins, item_origins = decode_witness(witness(), LINEAGE, EPOCH, OPENING)
        self.assertEqual(account_origins[0]["balance"], [5, 0, 0, 0])
        self.assertEqual(account_origins[0]["revision"], 4)
        self.assertEqual(item_origins[0]["uid"], 81)
        self.assertEqual(item_origins[0]["owner"], [1, 7, 0])
        connection = Connection()
        result = capture(connection, LINEAGE, EPOCH)
        self.assertEqual(result["format"], "economic_sql_audit_origins_v1")
        self.assertEqual(result["account_origins"], account_origins)
        self.assertEqual(result["item_origins"], item_origins)
        self.assertEqual(result["baseline_operation_ids"], [OP.hex()])
        expected_source = (struct.pack("<HH", 10, 1) + bytes.fromhex("44" * 16) +
                           EPOCH + (1).to_bytes(8, "little") + bytes(4)).hex()
        self.assertEqual(result["baseline_source_events"], {OP.hex(): expected_source})
        self.assertEqual(connection.rollbacks, 1)
        self.assertTrue(connection.scan.closed)
        statements = [sql.upper() for sql, _ in connection.scan.statements]
        self.assertIn("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", statements)
        self.assertTrue(all(sql.startswith(("SET TRANSACTION", "START TRANSACTION", "SELECT"))
                            for sql in statements))
        self.assertEqual(connection.scan.statements[5][1], (LINEAGE, EPOCH))
        witness_query = connection.scan.statements[7][0]
        self.assertIn("i.failure_stage AS inbox_failure_stage", witness_query)
        self.assertIn("i.committed_at IS NOT NULL", witness_query)
        reservation_query, reservation_parameters = connection.scan.statements[11]
        self.assertIn("LEFT JOIN economic_baseline_witness", reservation_query)
        self.assertIn("((p.lineage=%s AND p.epoch=%s) OR (w.lineage=%s AND w.epoch=%s))", reservation_query)
        self.assertEqual(reservation_parameters, (LINEAGE, EPOCH, LINEAGE, EPOCH, origin_exporter.MAX_ROWS - 4 + 1))
        zero_query, zero_parameters = connection.scan.statements[12]
        self.assertEqual(zero_query.count("EXISTS(SELECT 1"), 6)
        self.assertEqual(zero_parameters, (LINEAGE, EPOCH) * 6)
        self.assertNotIn("payload", zero_query)
        for table, column in origin_exporter.BASELINE_ZERO_EFFECTS:
            self.assertIn("FROM " + table + " p JOIN economic_baseline_witness w ON w.operation_id=p." + column,
                          zero_query)

    def test_digest_header_count_and_origin_corruption_refuse(self):
        for change in ("digest", "header", "count", "lineage", "source", "owner"):
            with self.subTest(change=change):
                row = copy.deepcopy(witness())
                blob = bytearray(row["canonical_witness"])
                if change == "digest":
                    row["witness_digest"] = bytes(32)
                elif change == "header":
                    blob[4] = 2
                elif change == "count":
                    row["holding_count"] = 0
                elif change == "lineage":
                    blob[16] ^= 1
                elif change == "source":
                    blob[272:304] = bytes(32)
                else:
                    blob[312] = 0
                if change != "digest":
                    row["canonical_witness"] = bytes(blob)
                    row["witness_digest"] = hashlib.sha256(blob).digest()
                with self.assertRaises((OriginError, ValueError)):
                    decode_witness(row, LINEAGE, EPOCH, OPENING)

    def test_uncommitted_or_missing_control_rolls_back(self):
        for field, value in (("inbox_status", 0), ("inbox_failure_stage", 1),
                             ("inbox_committed_at_present", 0)):
            with self.subTest(field=field):
                bad = witness()
                bad[field] = value
                connection = Connection(rows=[bad])
                with self.assertRaisesRegex(OriginError, "uncommitted"):
                    capture(connection, LINEAGE, EPOCH)
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
        connection = Connection(control={"opening_account": OPENING,
                                         "revision": 2, "last_operation_id": OP})
        with self.assertRaisesRegex(OriginError, "revision gap"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)
        connection = Connection()
        connection.scan.rows[2][0]["engine"] = "MyISAM"
        with self.assertRaisesRegex(OriginError, "not InnoDB"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)

    def test_cross_witness_duplicate_origin_refuses(self):
        first = witness()
        blob = bytearray(first["canonical_witness"])
        struct.pack_into("<Q", blob, 72, 2)
        second = baseline_root(bytes(blob), revision=2)
        connection = Connection(
            control={"opening_account": OPENING, "revision": 2,
                     "last_operation_id": second["operation_id"]}, rows=[first, second])
        with self.assertRaisesRegex(OriginError, "duplicate baseline account"):
            capture(connection, LINEAGE, EPOCH)
        self.assertEqual(connection.rollbacks, 1)


class BaselineVersionTests(unittest.TestCase):
    """Explicit reference models; no v2 native producer or migration claim."""

    @staticmethod
    def row(version=2, positions=None, holdings=None):
        positions = [(81, (1, 1, 7, 0, 81, 0, 2, 43)),
                     (82, (12, 1, 42, 0, 82, 0, 3, 1))] if positions is None else positions
        original = witness([] if holdings is None else holdings)["canonical_witness"]
        count = struct.unpack_from("<I", original, 184)[0]
        blob = bytearray(original[:192 + count * 112])
        blob[:4] = b"EAB2" if version == 2 else b"EAB1"
        struct.pack_into("<H", blob, 4, version)
        struct.pack_into("<I", blob, 188, len(positions))
        for uid, (owner, state, identity, context, root, parent, revision, slot) in positions:
            blob += struct.pack("<QBB6x5Q", uid, owner, state, identity, context, root, parent, revision)
            if version == 2:
                blob += struct.pack("<H6x", slot)
            else:
                assert slot == 0
            blob += bytes.fromhex("88" * 32)
        struct.pack_into("<I", blob, 8, len(blob))
        return baseline_root(bytes(blob))

    def read(self, row, valid=True):
        before = copy.deepcopy(row)
        connection = Connection(rows=[row])
        if valid:
            result = capture(connection, LINEAGE, EPOCH)
        else:
            with self.assertRaises(OriginError):
                capture(connection, LINEAGE, EPOCH)
            result = None
        self.assertEqual(row, before)
        self.assertEqual(connection.rollbacks, 1)
        self.assertTrue(connection.scan.closed)
        self.assertTrue(all(sql.startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                            for sql, _ in connection.scan.statements))
        return result

    @staticmethod
    def changed(row, offset, value):
        result = copy.deepcopy(row)
        blob = bytearray(row["canonical_witness"])
        blob[offset:offset + len(value)] = value
        result["canonical_witness"] = bytes(blob)
        result["witness_digest"] = hashlib.sha256(blob).digest()
        return result

    def test_version_two_retains_player_and_native_mobile_equipment(self):
        for owner, identity, slots in ((1, 7, (0, 1, 43, 65535)),
                                       (12, 2**64 - 2, (0, 1, 43))):
            for slot in slots:
                with self.subTest(owner=owner, slot=slot):
                    row = self.row(positions=[(81, (owner, 1, identity, 0, 81, 0, 2, slot))])
                    result = self.read(row)
                    self.assertEqual(result["item_origins"][0]["equipment_slot"], slot)
                    self.assertEqual(decode_plan(row["canonical_plan"])["after"][81][-1], slot)

    def test_version_two_reservations_use_all_exact_item_strides(self):
        row = self.row()
        result = self.read(row)
        self.assertEqual([item["uid"] for item in result["item_origins"]], [81, 82])
        for uid in (81, 82):
            connection = Connection(rows=[row])
            reservation = next(item for item in connection.scan.rows[9] if item["identity_id"] == uid)
            reservation["identity_id"] += 100
            with self.assertRaisesRegex(ValueError, "projection mismatch"):
                capture(connection, LINEAGE, EPOCH)
            self.assertEqual(connection.rollbacks, 1)
            self.assertTrue(connection.scan.closed)

    def test_historical_version_one_keeps_bytes_zero_slot_and_equipped_root_refusal(self):
        row = witness([])
        result = self.read(row)
        self.assertNotIn("equipment_slot", result["item_origins"][0])
        self.assertEqual(decode_plan(row["canonical_plan"])["after"][81][-1], 0)
        self.assertEqual(baseline_root(row["canonical_witness"]), row)
        plan = bytearray(row["canonical_plan"])
        for offset in (256 + 56, 320 + 56):
            struct.pack_into("<H", plan, offset, 1)
        row["canonical_plan"] = bytes(plan)
        row["plan_digest"] = hashlib.sha256(plan).digest()
        self.read(row, False)

    def test_magic_version_exact_lengths_and_metadata_agreement(self):
        for version in (1, 2):
            row = self.row(version, [(81, (1, 1, 7, 0, 81, 0, 2, 0))])
            for offset, value in ((0, b"EAB0"), (0, b"EAB2" if version == 1 else b"EAB1"),
                                  (4, struct.pack("<H", 3)), (6, struct.pack("<H", 191)),
                                  (8, struct.pack("<I", len(row["canonical_witness"]) - 1)),
                                  (12, b"\x01"), (188, struct.pack("<I", 2))):
                with self.subTest(version=version, offset=offset, value=value):
                    self.read(self.changed(row, offset, value), False)
            for value in (0, 3 - version, None, True, float(version)):
                changed = copy.deepcopy(row)
                changed["witness_version"] = value
                with self.subTest(version=version, metadata=value):
                    self.read(changed, False)
            for blob in (row["canonical_witness"][:-1], row["canonical_witness"] + b"\0"):
                changed = copy.deepcopy(row)
                changed["canonical_witness"] = blob
                changed["witness_digest"] = hashlib.sha256(blob).digest()
                self.read(changed, False)

    def test_reserved_bytes_and_source_digest_fail_even_after_rehash(self):
        row = self.row()
        for offset in (*range(202, 208), *range(250, 256), *range(298, 304), *range(346, 352)):
            with self.subTest(offset=offset):
                self.read(self.changed(row, offset, b"\x01"), False)
        for offset in (256, 352):
            self.read(self.changed(row, offset, bytes(32)), False)

    def test_equipment_only_change_without_original_plan_refuses(self):
        row = self.row()
        for offset, value in ((248, struct.pack("<H", 1)), (344, struct.pack("<H", 43))):
            with self.subTest(offset=offset):
                self.read(self.changed(row, offset, value), False)

    def test_invalid_positions_preserve_retained_claim_origin_refusals(self):
        row = self.row()
        damaged = [self.changed(row, offset, b"\x01") for offset in range(250, 256)]
        damaged += [self.row(positions=[(81, (1, 1, 7, 0, 82, 0, 2, 0))]),
                    self.row(positions=[(81, (1, 1, 7, 0, 81, 82, 2, 0))])]
        for row in damaged:
            with self.subTest(witness_digest=row["witness_digest"].hex()):
                before = copy.deepcopy(row)
                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                    decode_witness(row, LINEAGE, EPOCH, OPENING)
                self.read(row, False)
                claims = [{"operation_reason": 38, "operation_id": OP.hex(),
                           "operation_lineage": LINEAGE.hex(), "operation_epoch": EPOCH.hex()}
                          for _ in range(2)]
                cursor = mock.Mock()
                cursor.fetchone.return_value = {"row_count": 1, "blob_bytes": 1000, "projection_rows": 6}
                with mock.patch.object(snapshot_exporter, "read_origins_in_transaction",
                        side_effect=lambda *args: decode_witness(row, LINEAGE, EPOCH, OPENING)) as read:
                    snapshot_exporter.bind_baseline_claim_witnesses(cursor, LINEAGE, bytes.fromhex("33" * 16),
                        claims, {"baseline_source_events": {}})
                    read.assert_called_once_with(cursor, LINEAGE, EPOCH)
                self.assertEqual([claim["baseline_witness"] for claim in claims], [None, None])
                self.assertEqual(row, before)
                self.assertTrue(all(call.args[0].startswith("SELECT ") for call in cursor.execute.call_args_list))

    def test_native_mobile_invalid_slot_state_owner_and_forest_refuse(self):
        invalid = [(12, 1, 42, 0, 81, 0, 2, 44), (12, 1, 42, 0, 81, 0, 2, 65535),
                   (12, 3, 42, 0, 81, 0, 2, 1), (12, 1, 42, 1, 81, 0, 2, 0),
                   (12, 1, 2**64 - 1, 0, 81, 0, 2, 0), (9, 1, 42, 0, 81, 0, 2, 1),
                   (1, 1, 7, 0, 82, 82, 2, 1), (1, 1, 7, 0, 82, 0, 2, 0),
                   (1, 1, 7, 0, 81, 81, 2, 0), (8, 2, 0, 0, 81, 0, 0, 0)]
        for value in invalid:
            with self.subTest(position=value):
                self.read(self.row(positions=[(81, value)]), False)
        positions = [(81, (12, 1, 42, 0, 81, 0, 2, 0)),
                     (82, (12, 3, 42, 0, 81, 81, 3, 0))]
        self.read(self.row(positions=positions))
        for changed in ([(81, (12, 1, 42, 0, 81, 82, 2, 0)), positions[1]],
                        [positions[0], (82, (12, 1, 43, 0, 81, 81, 3, 0))],
                        [positions[0], (82, (12, 1, 42, 0, 81, 83, 3, 0))],
                        list(reversed(positions)), [positions[0], positions[0]]):
            with self.subTest(positions=changed):
                self.read(self.row(positions=changed), False)

    def test_exact_versioned_maxima_and_count_bounds(self):
        holdings = [key(1, index + 1) for index in range(3071)]
        positions = [(index + 1, (1, 1, 7, 0, index + 1, 0, 2, 0)) for index in range(6000)]
        for version, maximum in ((1, 872144), (2, 920144)):
            with self.subTest(version=version):
                row = self.row(version, positions, holdings)
                self.assertEqual(len(row["canonical_witness"]), maximum)
                decoded = decode_witness(row, LINEAGE, EPOCH, OPENING)
                self.assertEqual(tuple(map(len, decoded)), (3071, 6000))
                for offset, value in ((184, 3072), (188, 6001)):
                    with self.assertRaises(ValueError):
                        decode_witness(self.changed(row, offset, struct.pack("<I", value)),
                                       LINEAGE, EPOCH, OPENING)


class CapturedItemBindingTests(unittest.TestCase):
    def packet(self, *, empty=False):
        # Modeled transport for the existing native DTO, never source authority.
        specifications = (
            ('player_data','pid,account_name,racewar,copper,silver,gold,platinum,wallet_revision,save_revision','pid'),
            ('account_banks','id,account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision','id'),
            ('shopkeepers','id,shop_id,mob_vnum,room_vnum,cash,shop_revision,keeper_roaming','id'),
            ('ships','id,owner_name,money','id'),
            ('auctions','id,seller_pid,status,winning_bidder_pid,cur_price,buy_price,quantity,auction_revision,custody_state,listing_operation_id,obj_vnum,obj_blob_str','id'),
            ('auction_money_pickups','pid,money,claim_revision','pid'),
            ('auction_item_pickups','id,pid,obj_blob_str,retrieved,quantity','id'),
            ('auction_item_custody','auction_id,slot,item_uid,item_revision,vnum,obj_blob,claim_pid,claim_operation_id,claimed_at IS NOT NULL','auction_id,slot'),
            ('collector_catalog_state','state_id,catalog_revision,next_listing','state_id'),
            ('collector_deaths','death_operation_id,beneficiary_pid,death_time,collection_delay,sale_delay,holding_duration,price_percent,minimum_value,hint_state,hint_revision','death_operation_id'),
            ('collector_listings','listing_id,death_operation_id,beneficiary_pid,item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,record_blob,item_blob','listing_id'),
            ('item_current_owner','item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload','item_uid'),
            ('item_owner_revision','owner_type,owner_id,owner_context_id,revision','owner_type,owner_id,owner_context_id'),
            ('item_uid_allocator','allocator_id,next_uid','allocator_id'),
            ('item_ownership_quarantine','quarantine_id,item_uid,source_table,source_row_id,conflict_code,evidence,repaired_at IS NOT NULL','quarantine_id'),
            ('auction_reconciliation_quarantine','quarantine_id,auction_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL','quarantine_id'),
            ('collector_reconciliation_quarantine','quarantine_id,listing_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL','quarantine_id'),
            ('critical_operation_inbox','operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at IS NOT NULL','operation_id'),
            ('critical_outbox','outbox_id,operation_id,event_index,destination,event_type,payload_version,payload,status,attempt_count,last_error_code,delivered_at IS NOT NULL,dead_lettered_at IS NOT NULL','outbox_id'),
            ('economic_account_mapping','mapping_id,lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id,retiring_operation_id,revision','mapping_id'))
        physical = (('player_pet_items','id,pet_id,container_id,obj_uid,vnum','id'),
                    ('shopkeeper_items','id,shopkeeper_id,container_id,obj_uid,vnum,item_condition','id'),
                    ('siege_items','id,room_vnum,container_id,obj_uid,vnum','id'))
        equipment = (('item_current_owner','item_uid,equipment_slot','item_uid'),)
        snapshot = dict(version=2, rows=0, cells=0, cell_bytes=0)
        for group, specs in (('tables',specifications),('item_sources',physical),('item_equipment_sources',equipment)):
            snapshot[group] = [dict(name=name, columns=columns.split(','), rows=[], _order=order)
                               for name,columns,order in specs]
        if not empty:
            table = next(table for table in snapshot['tables'] if table['name']=='item_current_owner')
            table['rows'] = [dict(cells=[value.encode().hex() if value is not None else None for value in
                ('81','81',None,'1','7','0','2','100','1',None)])]
            table = next(table for table in snapshot['tables'] if table['name']=='item_owner_revision')
            table['rows'] = [dict(cells=[value.encode().hex() for value in ('1','7','0','4')])]
            snapshot['item_equipment_sources'][0]['rows'] = [dict(cells=[b'81'.hex(),b'0'.hex()])]
        self.reframe(snapshot)
        blob = bytearray(witness()['canonical_witness'])
        if empty:
            blob = blob[:304]
            struct.pack_into('<I',blob,8,len(blob))
            struct.pack_into('<I',blob,188,0)
        else:
            # Original EAB2 item layout, explicitly modeled from the capture.
            blob[:4] = b'EAB2'
            struct.pack_into('<H',blob,4,2)
            blob = blob[:360]+bytes(8)+blob[360:]
            struct.pack_into('<I',blob,8,len(blob))
        legacy, coverage = bytes([0x55])*32, bytes([0x66])*32
        if not empty:
            native = next(table for table in snapshot['tables'] if table['name']=='item_current_owner')
            owner = next(table for table in snapshot['tables'] if table['name']=='item_owner_revision')
            equip = snapshot['item_equipment_sources'][0]
            source = hashlib.sha256(b'EBS2'+b''.join(self.frame(bytes.fromhex(table['rows'][0]['digest']))
                for table in (native,equip,owner))).digest()
            blob[-32:] = source
            boundary = hashlib.sha256(b'ESN5'+b''.join(self.frame(value) for value in
                (legacy,bytes.fromhex(native['content_digest']),bytes.fromhex(owner['content_digest']),
                 bytes.fromhex(equip['content_digest']),bytes.fromhex(snapshot['item_sources_digest'])))).digest()
            complete = hashlib.sha256(b'EIC2'+self.frame(coverage)+struct.pack('<QQ',1,81)+self.frame(source)).digest()
            blob[120:152],blob[152:184] = boundary,complete
        row = baseline_root(bytes(blob))
        packet = dict(format='economic_sql_captured_opening_v1',operation_id=row['operation_id'].hex(),
                      legacy_native_boundary_digest=legacy.hex(),holding_coverage_digest=coverage.hex(),
                      source_snapshot=snapshot)
        return row,packet

    @staticmethod
    def frame(value):
        return struct.pack('<Q',len(value))+value

    def reframe(self, snapshot):
        snapshot.update(rows=0,cells=0,cell_bytes=0)
        for group,tag,digest_name in (('tables',b'ESM1','digest'),('item_sources',b'EIM1','item_sources_digest'),
                                     ('item_equipment_sources',b'EIE2','item_equipment_sources_digest')):
            contents = []
            for table in snapshot[group]:
                definition = hashlib.sha256(self.frame(b'ESD1')+self.frame(table['name'].encode())+
                    self.frame(table['_order'].encode())+struct.pack('<Q',len(table['columns']))+
                    b''.join(self.frame(column.encode()) for column in table['columns'])).digest()
                table['definition_digest'] = definition.hex()
                digests = []
                for row in table['rows']:
                    cells = [None if value is None else bytes.fromhex(value) for value in row['cells']]
                    encoded = self.frame(b'ESR1')+definition+b''.join(struct.pack('<Q',int(cell is not None))+
                        (self.frame(cell) if cell is not None else b'') for cell in cells)
                    row['digest'] = hashlib.sha256(encoded).hexdigest()
                    digests.append(bytes.fromhex(row['digest']))
                    snapshot['rows'] += 1
                    snapshot['cells'] += len(cells)
                    snapshot['cell_bytes'] += sum(len(cell) for cell in cells if cell is not None)
                table['content_digest'] = hashlib.sha256(self.frame(b'EST1')+definition+
                    struct.pack('<Q',len(digests))+b''.join(digests)).hexdigest()
                contents.append(bytes.fromhex(table['content_digest']))
            snapshot[digest_name] = hashlib.sha256(self.frame(tag)+struct.pack('<Q',len(contents))+b''.join(contents)).hexdigest()
        snapshot['custody_digest'] = hashlib.sha256(self.frame(b'ESC2')+b''.join(bytes.fromhex(snapshot[name])
            for name in ('digest','item_sources_digest','item_equipment_sources_digest'))).hexdigest()

    def test_captured_item_bindings_exact_preimages_zero_slot_and_empty_compatibility(self):
        for empty in (False,True):
            row,packet = self.packet(empty=empty)
            before = copy.deepcopy((row,packet))
            report = origin_exporter.verify_captured_item_opening(row,packet)
            self.assertEqual(report['witness_item_count'],int(not empty))
            self.assertTrue(report['captured_source_framing_verified'])
            self.assertTrue(report['item_bindings_verified'])
            self.assertFalse(report['complete_item_selection_authenticated'])
            self.assertFalse(report['legacy_digest_authority_authenticated'])
            self.assertFalse(report['activation_qualified'])
            self.assertEqual((row,packet),before)

    def test_captured_item_bindings_refuse_changed_or_missing_original_inputs(self):
        for group,name,column in (('tables','item_current_owner',6),('tables','item_owner_revision',3),
                                  ('item_equipment_sources','item_current_owner',1)):
            row,packet = self.packet()
            table = next(table for table in packet['source_snapshot'][group] if table['name']==name)
            for change in ('changed','missing'):
                broken = copy.deepcopy(packet)
                target = next(table for table in broken['source_snapshot'][group] if table['name']==name)
                if change == 'changed': target['rows'][0]['cells'][column] = b'3'.hex()
                else: target['rows'].clear()
                self.reframe(broken['source_snapshot'])
                with self.subTest(name=name,group=group,change=change), self.assertRaises(OriginError):
                    origin_exporter.verify_captured_item_opening(row,broken)
        row,packet = self.packet()
        for field in ('legacy_native_boundary_digest','holding_coverage_digest'):
            for value in (None,'00'*32,'ab'*32):
                broken = copy.deepcopy(packet); broken[field] = value
                with self.subTest(field=field,value=value), self.assertRaises(OriginError):
                    origin_exporter.verify_captured_item_opening(row,broken)
        broken = copy.deepcopy(packet)
        broken['source_snapshot']['item_sources'][0]['rows'] = [dict(cells=[b'1'.hex()]*5)]
        self.reframe(broken['source_snapshot'])
        with self.assertRaises(OriginError): origin_exporter.verify_captured_item_opening(row,broken)

    def test_captured_source_transport_exact_types_registry_bounds_and_binary_cells(self):
        row,packet = self.packet()
        for field in ('version','rows','cells','cell_bytes'):
            for value in integer_aliases(packet['source_snapshot'][field]):
                broken = copy.deepcopy(packet); broken['source_snapshot'][field] = value
                with self.subTest(field=field,value=value), self.assertRaises(OriginError):
                    origin_exporter.verify_captured_item_opening(row,broken)
        for group in ('tables','item_sources','item_equipment_sources'):
            broken = copy.deepcopy(packet); broken['source_snapshot'][group].reverse()
            if group == 'item_equipment_sources': broken['source_snapshot'][group].clear()
            with self.subTest(group=group), self.assertRaises(OriginError):
                origin_exporter.verify_captured_item_opening(row,broken)
        for field,value in (('rows',262145),('cells',4194305),('cell_bytes',67108865)):
            broken = copy.deepcopy(packet); broken['source_snapshot'][field] = value
            with self.subTest(field=field), self.assertRaises(OriginError):
                origin_exporter.verify_captured_item_opening(row,broken)
        broken = copy.deepcopy(packet)
        broken['source_snapshot']['tables'][0]['columns'].reverse()
        with self.assertRaises(OriginError): origin_exporter.verify_captured_item_opening(row,broken)


    def test_captured_opening_operator_read_boundary_and_protected_input(self):
        row, packet = self.packet()
        for supplied in (None, packet, dict(packet, operation_id='ab'*16),
                         dict(packet, holding_coverage_digest='cd'*32)):
            connection = Connection(rows=[row])
            with self.subTest(supplied=supplied is not None):
                if supplied is not None and supplied != packet:
                    with self.assertRaises(OriginError):
                        capture(connection, LINEAGE, EPOCH, captured_opening=supplied)
                else:
                    result = capture(connection, LINEAGE, EPOCH, captured_opening=supplied)
                    self.assertEqual('captured_item_bindings' in result, supplied is not None)
                    if supplied:
                        self.assertFalse(result['captured_item_bindings']['activation_qualified'])
                self.assertEqual(connection.rollbacks, 1)
                self.assertTrue(connection.scan.closed)
                self.assertTrue(all(sql.startswith(('SELECT ', 'SET TRANSACTION ', 'START TRANSACTION '))
                                    for sql, _ in connection.scan.statements))
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'capture.json'
            path.write_text(json.dumps(packet), encoding='utf-8')
            path.chmod(0o600)
            self.assertEqual(origin_exporter.load_captured_opening(path), packet)
            for data in ('{"format":"economic_sql_captured_opening_v1","format":"other"}',
                         '[]', '{', '['*2000 + ']'*2000):
                path.write_text(data, encoding='utf-8')
                with self.assertRaises(OriginError): origin_exporter.load_captured_opening(path)
            with path.open('wb') as stream: stream.truncate(origin_exporter.MAX_INPUT_BYTES+1)
            with self.assertRaises(OriginError): origin_exporter.load_captured_opening(path)
            path.write_text(json.dumps(packet), encoding='utf-8')
            linked = path.with_name('linked.json')
            os.link(path, linked)
            with self.assertRaises(OriginError): origin_exporter.load_captured_opening(path)
            linked.unlink()
            if os.name == 'posix':
                path.chmod(0o644)
                with self.assertRaises(OriginError): origin_exporter.load_captured_opening(path)
                path.chmod(0o600)
                linked.symlink_to(path)
                with self.assertRaises(OSError): origin_exporter.load_captured_opening(linked)
                linked.unlink()
                os.mkfifo(linked, 0o600)
                with self.assertRaises(OriginError): origin_exporter.load_captured_opening(linked)
            connection = Connection(rows=[row])
            connection.close = mock.Mock()
            client = mock.Mock()
            client.MySQLError = RuntimeError
            client.connect.return_value = connection
            output = path.with_name('origins.json')
            arguments = ['economic_sql_audit_origins.py', '--host', 'localhost', '--user', 'reader',
                '--database', 'disposable', '--password-env', 'CAPTURE_TEST_PASSWORD',
                '--lineage', LINEAGE.hex(), '--epoch', EPOCH.hex(), '--output', str(output),
                '--captured-opening-evidence', str(path)]
            with mock.patch.object(sys, 'argv', arguments), mock.patch.dict(sys.modules, pymysql=client), \
                    mock.patch.dict(os.environ, CAPTURE_TEST_PASSWORD='disposable-test-only'):
                self.assertEqual(origin_exporter.main(), 0)
            exported = json.loads(output.read_bytes())
            self.assertTrue(exported['captured_item_bindings']['item_bindings_verified'])
            self.assertFalse(exported['captured_item_bindings']['release_qualified'])
            connection.close.assert_called_once_with()
            self.assertEqual(connection.rollbacks, 1)
            self.assertTrue(connection.scan.closed)
            options = client.connect.call_args.kwargs
            self.assertTrue(options['autocommit'])
            self.assertEqual((options['connect_timeout'],options['read_timeout'],options['write_timeout']), (5,30,5))
            if os.name == 'posix': self.assertEqual(output.stat().st_mode & 0o777, 0o600)

    def test_captured_raw_cells_historical_equipment_and_selection_limits(self):
        row, packet = self.packet()
        for raw in (None, b'', b'private\x00\xff'):
            changed = copy.deepcopy(packet)
            ships = next(t for t in changed['source_snapshot']['tables'] if t['name']=='ships')
            ships['rows'] = [dict(cells=[b'1'.hex(), None if raw is None else raw.hex(), b'0'.hex()])]
            self.reframe(changed['source_snapshot'])
            decoded = origin_exporter.validate_captured_sources(changed['source_snapshot'])
            self.assertEqual(next(t for t in decoded['tables'] if t['name']=='ships')['rows'][0]['cells'][1], raw)
            report = origin_exporter.verify_captured_item_opening(row, changed)
            self.assertFalse(report['complete_source_capture_authenticated'])
        for value in ('081', '+81', ' 81', '81\x00', str(2**64)):
            changed = copy.deepcopy(packet)
            native = next(t for t in changed['source_snapshot']['tables'] if t['name']=='item_current_owner')
            native['rows'][0]['cells'][0] = value.encode().hex()
            changed['source_snapshot']['item_equipment_sources'][0]['rows'][0]['cells'][0] = value.encode().hex()
            self.reframe(changed['source_snapshot'])
            with self.subTest(value=value), self.assertRaises(OriginError):
                origin_exporter.verify_captured_item_opening(row, changed)
        for label in ('duplicate_owner', 'foreign_equipment', 'null_equipment', 'oversize_cell'):
            changed = copy.deepcopy(packet)
            snapshot = changed['source_snapshot']
            owner = next(t for t in snapshot['tables'] if t['name']=='item_owner_revision')
            equip = snapshot['item_equipment_sources'][0]
            if label == 'duplicate_owner': owner['rows'] *= 2
            if label == 'foreign_equipment': equip['rows'][0]['cells'][0] = b'82'.hex()
            if label == 'null_equipment': equip['rows'][0]['cells'][1] = None
            if label == 'oversize_cell': owner['rows'][0]['cells'][3] = '00'*(1024*1024+1)
            self.reframe(snapshot)
            with self.subTest(label=label), self.assertRaises(OriginError):
                origin_exporter.verify_captured_item_opening(row, changed)
        empty_row, empty_packet = self.packet(empty=True)
        historical = copy.deepcopy(empty_packet)
        historical['source_snapshot'].update(version=1, item_equipment_sources=[],
            item_equipment_sources_digest='00'*32, custody_digest='00'*32)
        self.assertFalse(origin_exporter.verify_captured_item_opening(empty_row, historical)['witness_equipment_observed'])
        empty_packet['source_snapshot'] = packet['source_snapshot']
        report = origin_exporter.verify_captured_item_opening(empty_row, empty_packet)
        self.assertEqual((report['witness_item_count'], report['captured_native_item_count']), (0, 1))
        self.assertFalse(report['complete_item_selection_authenticated'])


@unittest.skipUnless(os.environ.get("DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION") == "1",
                     "requires explicit disposable Linux native/SQL integration invocation")
class NativeSQLOriginTests(unittest.TestCase):
    """Actual native EAB1/CCM1/EAI1/EAP1 bytes in fresh canonical SQL schemas.

    This tests the origin reader, not SQL mutation/lifecycle execution or an
    activation attestation. All SQL fixture writes use a separate private owner.
    """

    @classmethod
    def setUpClass(cls):
        for command in ("g++", "mysql", "mysqld", "mariadbd", "mariadb-install-db"):
            if not shutil.which(command):
                raise RuntimeError("native/SQL integration prerequisite unavailable: " + command)
        from test_flatfile_restore_economic_authority import build_fixture
        cls.temp = tempfile.TemporaryDirectory(prefix="duris-native-origin-sql-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.base = Path(cls.temp.name)
        cls.state = cls.base / "native"
        cls.fixture = build_fixture(ROOT / "bin/tests/flatfile_origin_sql_fixture")
        subprocess.run([str(cls.fixture), str(cls.state), "baseline-rich"], check=True,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
        cls.evidence = cls.state / "economic-evidence"
        cls.retained = {p.name: (p.read_bytes(), p.stat().st_mode, p.stat().st_nlink)
                        for p in cls.evidence.iterdir()}
        assert cls.retained["authority.eal"][0][112:128] == bytes(16)
        cls.batches = []
        for path in cls.evidence.glob("*.eab"):
            blob = path.read_bytes()
            op = bytes.fromhex(path.stem.rsplit("-", 1)[1])
            index = (cls.evidence / f"bucket-{op[0]:02x}.eai").read_bytes()
            entries = [index[n:n + 64] for n in range(80, len(index), 64)]
            entry, = [entry for entry in entries if entry[:16] == op]
            segment, offset, size = struct.unpack_from("<III", entry, 48)
            segment_bytes = (cls.evidence / f"bucket-{op[0]:02x}-{segment}.eas").read_bytes()
            # Native index offsets exclude the 48-byte envelope and 32-byte segment header.
            frame = segment_bytes[80 + offset:80 + offset + size]
            assert frame[:8] == b"DURECR2\0" and hashlib.sha256(frame[48:]).digest() == frame[16:48]
            assert hashlib.sha256(frame).digest() == entry[16:48]
            command_size, plan_size = struct.unpack_from("<II", frame, 48)
            command = frame[74:74 + command_size]
            plan = frame[74 + command_size:74 + command_size + plan_size]
            keys, revisions, payload_size = struct.unpack_from("<III", command, 40)
            intent_at = 52 + keys * 16 + revisions * 24 + payload_size + 4
            intent = command[intent_at:]
            assert command[:4] == b"CCM1" and command[8:24] == op
            assert plan[:4] == b"EAP1" and plan[40:56] == op and intent[:4] == b"EAI1"
            cls.batches.append((blob, op, struct.unpack_from("<Q", frame, 64)[0], command, plan, intent))
        cls.batches.sort(key=lambda row: (row[0][32:48], row[2]))
        assert len(cls.batches) == 4
        print("NATIVE_ORIGIN_FIXTURE " + json.dumps({
            "sha256": hashlib.sha256(cls.fixture.read_bytes()).hexdigest(),
            "witnesses": [{"sha256": hashlib.sha256(row[0]).hexdigest(), "revision": row[2],
                           "epoch": row[0][32:48].hex()} for row in cls.batches]}, sort_keys=True), flush=True)

    def seed(self, connection):
        cursor = connection.cursor()

        def insert(table, fields):
            cursor.execute("INSERT INTO " + table + " (" + ",".join(fields) + ") VALUES (" +
                           ",".join(["%s"] * len(fields)) + ")", tuple(fields.values()))

        creator = bytes([7]) + bytes(15)
        insert("critical_operation_inbox", dict(operation_id=creator, command_hash=bytes([1]) * 32,
               keys_hash=bytes([2]) * 32, command_type=1, schema_version=1, payload_version=1,
               status=1, result_code=0, result_payload=b""))
        lineage = self.batches[0][0][16:32]
        epochs = sorted({row[0][32:48] for row in self.batches})
        for ordinal, epoch in enumerate(epochs, 1):
            insert("economic_epoch", dict(lineage=lineage, epoch=epoch, ordinal=ordinal,
                   transition_kind=1, transition_digest=bytes([42]) + bytes(31),
                   creating_operation_id=creator))
        insert("economic_lineage_state", dict(lineage=lineage, active_epoch=None))
        for epoch in epochs:
            rows = [row for row in self.batches if row[0][32:48] == epoch]
            insert("economic_baseline_control", dict(lineage=lineage, epoch=epoch,
                   opening_account=rows[0][0][80:120], creating_operation_id=creator))
        for blob, op, revision, command, plan, intent in self.batches:
            epoch = blob[32:48]
            keys = b"".join(command[n:n + 1] + command[n + 8:n + 16]
                            for n in range(52, 52 + struct.unpack_from("<I", command, 40)[0] * 16, 16))
            insert("critical_operation_inbox", dict(operation_id=op,
                   command_hash=hashlib.sha256(command).digest(), keys_hash=hashlib.sha256(keys).digest(),
                   command_type=struct.unpack_from("<H", command, 24)[0], schema_version=2,
                   payload_version=1, status=1, result_code=0, failure_stage=0,
                   durable_revision=revision, result_payload=b""))
            cursor.execute("UPDATE critical_operation_inbox SET committed_at=CURRENT_TIMESTAMP(6) "
                           "WHERE operation_id=%s", (op,))
            accounts, postings, children, before, after, events = struct.unpack_from("<6I", plan, 216)
            insert("economic_accounting_operation", dict(operation_id=op, lineage=lineage, epoch=epoch,
                   accounting_version=1, writer_id=struct.unpack_from("<I", plan, 84)[0],
                   policy_version=struct.unpack_from("<I", plan, 88)[0],
                   compiler_version=struct.unpack_from("<I", plan, 92)[0], actor_kind=plan[72],
                   actor_id=struct.unpack_from("<Q", plan, 76)[0], reason=38, source_event=plan[104:152],
                   intent_digest=plan[152:184], domain_digest=plan[184:216],
                   plan_digest=hashlib.sha256(plan).digest(), canonical_intent=intent,
                   canonical_plan=plan, outcome=1, result_code=0, account_count=accounts,
                   posting_count=postings, child_count=children, item_event_count=events,
                   before_witness_count=before, after_witness_count=after))
            insert("economic_accounting_source_claim", dict(lineage=lineage, source_event=plan[104:152],
                   operation_id=op, outcome=1))
            for index in range(accounts):
                record = plan[256 + index * 120:256 + (index + 1) * 120]
                fields = dict(operation_id=op, account_index=index, account_key=record[:40])
                fields.update(zip((f"{side}_{coin}" for side in ("before", "after")
                                   for coin in ("copper", "silver", "gold", "platinum")),
                                  struct.unpack_from("<8q", record, 40)))
                fields.update(zip(("before_revision", "after_revision"), struct.unpack_from("<2Q", record, 104)))
                insert("economic_accounting_account_effect", fields)
            for index in range(postings):
                offset = 256 + accounts * 120 + index * 48
                event, account, child = struct.unpack_from("<IHH", plan, offset)
                fields = dict(operation_id=op, line_index=index, event_index=event,
                              account_index=account, child_index=child)
                fields.update(zip(("delta_copper", "delta_silver", "delta_gold", "delta_platinum", "copper_value"),
                                  struct.unpack_from("<5q", plan, offset + 8)))
                insert("economic_accounting_coin_posting", fields)
            witness_version, item_stride = origin_exporter.witness_layout(blob)
            holdings, items = struct.unpack_from("<II", blob, 184)
            insert("economic_baseline_witness", dict(operation_id=op, lineage=lineage, epoch=epoch,
                   book_revision=revision, witness_version=witness_version, holding_count=holdings, item_count=items,
                   witness_digest=hashlib.sha256(blob).digest(), canonical_witness=blob,
                   command_accepted_at_usec=struct.unpack_from("<Q", command, 32)[0]))
            identities = [(1, struct.unpack_from("<Q", blob, 192 + n * 112 + 20)[0]) for n in range(holdings)]
            identities += [(2, struct.unpack_from("<Q", blob, 192 + holdings * 112 + n * item_stride)[0]) for n in range(items)]
            for kind, identity in identities:
                insert("economic_baseline_reservation", dict(lineage=lineage, epoch=epoch,
                       identity_kind=kind, identity_id=identity, operation_id=op))
            cursor.execute("UPDATE economic_baseline_control SET revision=%s,last_operation_id=%s "
                           "WHERE lineage=%s AND epoch=%s", (revision, op, lineage, epoch))
        cursor.close()

    def check_engine(self, engine):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from test_persistence_backup_integration import sql
        candidate = self.base / engine
        candidate.mkdir(mode=0o700)
        with restore.private_database(candidate, engine) as env:
            version = sql(env, "SELECT VERSION();")
            self.assertTrue("MariaDB" in version if engine == "mariadb" else
                            version.startswith("8.0.") and "MariaDB" not in version, version)
            sql(env, payload=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_bytes())
            with mock.patch.dict(os.environ, env, clear=True):
                manifest = migrations.load_manifest()
                executor = migrations.MysqlExecutor(manifest)
                executor.adopt("fresh_bootstrap")
                migrations.run_pending(manifest, executor)
            # run_pending closes its owned SQL session; inspect history in a new private one.
            terminal = sql(env, "SELECT sequence_number,migration_id FROM mud_schema_history "
                                "ORDER BY sequence_number DESC LIMIT 1")
            self.assertEqual(terminal, "62\t0062_economic_pending_claim_consumption")
            print("ORIGIN_SQL_SCHEMA " + engine + " " + version + " through=" + terminal.replace("\t", " "), flush=True)
            owner = pymysql.connect(unix_socket=env["DB_SOCKET"], user="root", database="duris_restore",
                                    autocommit=True, cursorclass=pymysql.cursors.DictCursor)
            try:
                self.seed(owner)
                with owner.cursor() as cursor:
                    cursor.execute("CREATE USER 'origin_reader'@'localhost' IDENTIFIED BY 'disposable-origin-reader'")
                    cursor.execute("GRANT SELECT ON duris_restore.* TO 'origin_reader'@'localhost'")
                reader = pymysql.connect(unix_socket=env["DB_SOCKET"], user="origin_reader",
                                         password="disposable-origin-reader", database="duris_restore",
                                         autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                try:
                    with reader.cursor() as cursor:
                        with self.assertRaises(pymysql.MySQLError) as denied:
                            cursor.execute("UPDATE economic_baseline_control SET revision=revision")
                        self.assertEqual(denied.exception.args[0], 1142)

                    def database_rows():
                        with owner.cursor() as cursor:
                            rows = []
                            for table in ("economic_lineage_state", "economic_epoch", "critical_operation_inbox",
                                          "economic_accounting_operation", "economic_accounting_account_effect",
                                          "economic_accounting_coin_posting", "economic_accounting_source_claim",
                                          "economic_baseline_control", "economic_baseline_witness", "economic_baseline_reservation",
                                          "economic_accounting_child", "economic_accounting_item_reference", "currency_ledger",
                                          "item_ownership_ledger", "critical_outbox"):
                                cursor.execute("SELECT * FROM " + table + " ORDER BY 1,2")
                                rows.append(cursor.fetchall())
                            return rows

                    reads = {"captures": 0, "refusals": 0, "rollbacks": 0}
                    page_observations = []

                    def pages(refusal):
                        import economic_sql_canonical_audit as audit
                        book_before = database_rows() if refusal and refusal.startswith('baseline ') else None
                        progress = audit.new_progress('ab'*32, time.time())
                        reports = []
                        # All original fixture roots, including inactive books,
                        # are checked; a selected epoch cannot hide a witness.
                        for _ in range(len(self.batches)+1):
                            connection = mock.Mock(wraps=reader)
                            cursor = mock.Mock(wraps=reader.cursor())
                            connection.cursor.return_value = cursor
                            report, progress = audit.scan_page(connection, progress)
                            connection.rollback.assert_called_once_with()
                            cursor.close.assert_called_once_with()
                            self.assertFalse(report['coverage']['complete'])
                            self.assertFalse(report['coverage']['baseline_witnesses_authenticated'])
                            self.assertTrue(all(call.args[0].startswith(('SELECT ', 'SET TRANSACTION ', 'START TRANSACTION '))
                                                for call in cursor.execute.call_args_list))
                            reports.append(report)
                            if report['range_exhausted']:
                                break
                        self.assertEqual(progress['completed_sweeps'], 1)
                        self.assertEqual(progress['total_rows'], len(self.batches))
                        found = [finding['code'] for report in reports for finding in report['findings']]
                        expected = (['restore_economic_canonical_account_mismatch',
                                     'restore_economic_canonical_posting_mismatch',
                                     'restore_economic_canonical_storage_mismatch']
                                    if refusal == 'EAB1 SQL projection mismatch' else
                                    ['restore_economic_baseline_witness_mismatch'])
                        if book_before is not None:
                            expected = ['restore_economic_baseline_book_mismatch']
                            self.assertEqual(database_rows(), book_before)
                            print('BASELINE_BOOK_PAGE_OBSERVATION ' + json.dumps(dict(engine=engine,
                                origin_refusal=refusal, reports=reports, authority_unchanged=True,
                                release_qualified=False), sort_keys=True), flush=True)
                        if refusal:
                            self.assertTrue(found)
                            self.assertTrue(all(code in expected for code in found), found)
                        else:
                            self.assertEqual(found, [])
                            self.assertEqual(sum(report['baseline_roots_authenticated'] for report in reports), len(self.batches))
                        page_observations.append(dict(origin_refusal=refusal, reports=reports,
                            read_only=True, native_fixture=True, complete_reconciliation=False))
                        (candidate/'baseline-page-observations.json').write_text(json.dumps(page_observations,indent=2)+'\n')

                    def read(epoch, refusal=None):
                        before = database_rows()
                        cursor = mock.Mock(wraps=reader.cursor())
                        connection = mock.Mock(wraps=reader)
                        connection.cursor.return_value = cursor
                        try:
                            if refusal:
                                with self.assertRaisesRegex(OriginError, refusal):
                                    capture(connection, self.batches[0][0][16:32], epoch)
                                result = None
                            else:
                                result = capture(connection, self.batches[0][0][16:32], epoch)
                        finally:
                            connection.rollback.assert_called_once_with()
                            cursor.close.assert_called_once_with()
                        self.assertTrue(all(call.args[0].upper().startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                            for call in cursor.execute.call_args_list))
                        pages(refusal)
                        self.assertEqual(database_rows(), before)
                        reads["refusals" if refusal else "captures"] += 1
                        reads["rollbacks"] += 1
                        return result

                    for epoch in sorted({row[0][32:48] for row in self.batches}):
                        result = read(epoch)
                        rows = [row for row in self.batches if row[0][32:48] == epoch]
                        self.assertEqual(result["witness_count"], len(rows))
                        expected = [row[0][192 + n * 112:192 + n * 112 + 40].hex()
                                    for row in rows for n in range(struct.unpack_from("<I", row[0], 184)[0])]
                        self.assertEqual([h["account_key"] for h in result["account_origins"]], expected)
                    blob, op, _, _, _, _ = self.batches[0]
                    holdings, = struct.unpack_from("<I", blob, 184)
                    self.assertEqual(holdings, 4)
                    native_rows = [blob[192 + n * 112:192 + (n + 1) * 112] for n in range(holdings)]
                    self.assertEqual([struct.unpack_from("<Q", row, 20)[0] for row in native_rows],
                                     [255, 256, 512, 2**64 - 1])
                    for order in (sorted(native_rows, key=lambda row: row[:40]),
                                  list(reversed(native_rows)), [native_rows[0]] * 2 + native_rows[2:]):
                        bad = blob[:192] + b"".join(order) + blob[192 + holdings * 112:]
                        with owner.cursor() as cursor:
                            cursor.execute("UPDATE economic_baseline_witness SET canonical_witness=%s,witness_digest=%s "
                                           "WHERE operation_id=%s", (bad, hashlib.sha256(bad).digest(), op))
                        read(blob[32:48], "invalid or duplicate EAB1 holding")
                    with owner.cursor() as cursor:
                        cursor.execute("UPDATE economic_baseline_witness SET canonical_witness=%s,witness_digest=%s "
                                       "WHERE operation_id=%s", (blob, hashlib.sha256(blob).digest(), op))
                        cursor.execute("SELECT active_epoch FROM economic_lineage_state")
                        self.assertEqual(cursor.fetchall(), [{"active_epoch": None}])
                    read(blob[32:48])
                    # Negative disposable-schema cases: retain native bytes and
                    # exact numeric values while the driver returns float or
                    # Decimal projections. Restore the canonical column after
                    # each case; altered schemas never qualify for release.
                    for table, field in (("economic_accounting_account_effect", "after_silver"),
                                         ("economic_accounting_coin_posting", "delta_silver")):
                        for storage, representation in (("DOUBLE", float), ("DECIMAL(20,0)", Decimal)):
                            with self.subTest(engine=engine, table=table, storage=storage):
                                canonical = database_rows()
                                with owner.cursor() as cursor:
                                    cursor.execute("ALTER TABLE " + table + " MODIFY " + field + " " + storage + " NOT NULL")
                                    cursor.execute("SELECT " + field + " FROM " + table)
                                    self.assertTrue(all(type(row[field]) is representation for row in cursor.fetchall()))
                                try:
                                    read(blob[32:48], "EAB1 SQL projection mismatch")
                                finally:
                                    with owner.cursor() as cursor:
                                        cursor.execute("ALTER TABLE " + table + " MODIFY " + field + " BIGINT NOT NULL")
                                self.assertEqual(database_rows(), canonical)
                                read(blob[32:48])
                    self.assertEqual({p.name: (p.read_bytes(), p.stat().st_mode, p.stat().st_nlink)
                                      for p in self.evidence.iterdir()}, self.retained)
                    self.assertEqual(reads, {"captures": 7, "refusals": 7, "rollbacks": 14})
                    print("PASS native-origin " + engine + " " + json.dumps(reads, sort_keys=True) +
                          " SELECT-only bytes-unchanged inactive", flush=True)
                    original_reads = reads.copy()
                    with owner.cursor() as cursor:
                        cursor.execute("SELECT w.command_accepted_at_usec,i.command_hash FROM economic_baseline_witness w "
                                       "JOIN critical_operation_inbox i ON i.operation_id=w.operation_id "
                                       "WHERE w.operation_id=%s", (op,))
                        original_command = cursor.fetchone()
                    observations = []
                    for table, field, value in (
                        ("economic_baseline_witness", "command_accepted_at_usec",
                         original_command["command_accepted_at_usec"] +
                         (1 if original_command["command_accepted_at_usec"] < 2**64 - 1 else -1)),
                        ("critical_operation_inbox", "command_hash",
                         bytes([original_command["command_hash"][0] ^ 1]) + original_command["command_hash"][1:])):
                        unchanged = database_rows()
                        with owner.cursor() as cursor:
                            cursor.execute("UPDATE " + table + " SET " + field + "=%s WHERE operation_id=%s", (value, op))
                        try:
                            read(blob[32:48], "EAB1 committed root mismatch")
                            observations.append(dict(field=field, refused=True, read_only=True, authority_unchanged=True))
                        finally:
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE " + table + " SET " + field + "=%s WHERE operation_id=%s",
                                               (original_command[field], op))
                        self.assertEqual(database_rows(), unchanged)
                    self.assertEqual({key: reads[key] - original_reads[key] for key in reads},
                                     {"captures": 0, "refusals": 2, "rollbacks": 2})
                    (candidate/"command-preimage-refusals.json").write_text(json.dumps(observations,indent=2)+'\n')
                    print("PASS original-command-preimage " + engine + " 2 native SQL cuts SELECT-only bytes-unchanged", flush=True)
                    print('BASELINE_ROOT_PAGES ' + json.dumps(dict(engine=engine, version=version,
                        observations=page_observations, authority_unchanged=True, native_producer_or_gameplay=False,
                        release_qualified=False), sort_keys=True), flush=True)
                    with owner.cursor() as cursor:
                        cursor.execute('SELECT revision,last_operation_id FROM economic_baseline_control '
                                       'WHERE lineage=%s AND epoch=%s', (blob[16:32], blob[32:48]))
                        book_control = cursor.fetchone()
                    book_reads = reads.copy()
                    other_operation = next(row[1] for row in self.batches if row[1] != book_control['last_operation_id'])
                    for damaged, refusal in (
                        ({'revision': 0, 'last_operation_id': None}, 'baseline has no committed opening witness'),
                        ({'revision': book_control['revision']+1}, 'baseline witness revision gap or limit exceeded'),
                        ({'last_operation_id': other_operation}, 'baseline control terminal witness mismatch')):
                        unchanged = database_rows()
                        with owner.cursor() as cursor:
                            cursor.execute('UPDATE economic_baseline_control SET '+
                                           ','.join(field+'=%s' for field in damaged)+' WHERE lineage=%s AND epoch=%s',
                                           (*damaged.values(), blob[16:32], blob[32:48]))
                        try:
                            read(blob[32:48], refusal)
                        finally:
                            with owner.cursor() as cursor:
                                cursor.execute('UPDATE economic_baseline_control SET '+
                                               ','.join(field+'=%s' for field in damaged)+' WHERE lineage=%s AND epoch=%s',
                                               (*(book_control[field] for field in damaged), blob[16:32], blob[32:48]))
                        self.assertEqual(database_rows(), unchanged)
                        read(blob[32:48])
                    self.assertEqual({key: reads[key]-book_reads[key] for key in reads},
                                     {'captures': 3, 'refusals': 3, 'rollbacks': 6})
                    print('PASS baseline-book-control '+engine+' 3 native SQL refusals/3 restored controls '
                          'SELECT-only bytes-unchanged inactive', flush=True)
                    # Negative SELECT projections preserve the actual input
                    # version's layout before corrupting v2 positions. These
                    # projections do not establish complete native capture.
                    item_count, = struct.unpack_from("<I", blob, 188)
                    item_offset = 192 + holdings * 112
                    version, stride = origin_exporter.witness_layout(blob)
                    if version == 1:
                        projected = bytearray(blob[:item_offset])
                        for index in range(item_count):
                            start = item_offset + index * stride
                            projected += blob[start:start + 56] + bytes(8) + blob[start + 56:start + stride]
                        projected[:4] = b"EAB2"
                        struct.pack_into("<H", projected, 4, 2)
                        struct.pack_into("<I", projected, 8, len(projected))
                    else:
                        self.assertEqual(version, 2)
                        projected = bytearray(blob)
                    uid, = struct.unpack_from("<Q", projected, item_offset)
                    self.assertEqual(struct.unpack_from("<Q", projected, item_offset + 32)[0], uid)
                    self.assertEqual(struct.unpack_from("<Q", projected, item_offset + 40)[0], 0)
                    uids = {struct.unpack_from("<Q", projected, item_offset + index * 96)[0]
                            for index in range(item_count)}
                    missing = 1
                    while missing in uids:
                        missing += 1
                    cuts = [("padding-" + str(offset), item_offset + offset, b"\x01")
                            for offset in range(58, 64)]
                    cuts += [("root", item_offset + 32, struct.pack("<Q", missing)),
                             ("parent", item_offset + 40, struct.pack("<Q", missing))]
                    observations = []
                    for label, offset, value in cuts:
                        damaged = projected.copy()
                        damaged[offset:offset + len(value)] = value
                        for exporter in (origin_exporter, snapshot_exporter):
                            before = database_rows()
                            actual = reader.cursor()
                            cursor = mock.Mock(wraps=actual)
                            connection = mock.Mock(wraps=reader)
                            connection.cursor.return_value = cursor

                            def projected_rows():
                                rows = copy.deepcopy(actual.fetchall())
                                for row in rows:
                                    if row.get("operation_id") == op and "canonical_witness" in row:
                                        row.update(canonical_witness=bytes(damaged), witness_version=2,
                                                   witness_digest=hashlib.sha256(damaged).digest())
                                return rows

                            cursor.fetchall.side_effect = projected_rows
                            with self.subTest(engine=engine, label=label, exporter=exporter.__name__):
                                with self.assertRaisesRegex(OriginError, "EAB1 committed root mismatch"):
                                    exporter.capture(connection, blob[16:32], blob[32:48])
                            connection.rollback.assert_called_once_with()
                            cursor.close.assert_called_once_with()
                            self.assertTrue(all(call.args[0].startswith(("SELECT", "SET TRANSACTION", "START TRANSACTION"))
                                                for call in cursor.execute.call_args_list))
                            self.assertEqual(database_rows(), before)
                            observations.append(dict(label=label, exporter=exporter.__name__,
                                refused="EAB1 committed root mismatch", rollback_calls=1, cursor_closed=True,
                                read_only=True, database_unchanged=True, negative_projection=True))
                    self.assertEqual(len(observations), 16)
                    (candidate/"position-refusals.json").write_text(json.dumps(observations,indent=2)+'\n')
                    print("PASS projected-origin-position " + engine + " " + str(len(observations)) +
                          " controlled-refusals SELECT-only bytes-unchanged full-native-capture-unqualified", flush=True)
                finally:
                    reader.close()
            finally:
                owner.close()

    def test_native_origins_mysql_8(self):
        self.check_engine("mysql")

    def test_native_origins_mariadb(self):
        self.check_engine("mariadb")

    def test_native_captured_source_bindings_both_engines(self):
        """Native raw-capture oracle with a modeled opening, never activation."""
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from test_persistence_backup_integration import sql
        source = ROOT/'bin/tests/captured_origin_export.cpp'
        binary = source.with_suffix('')
        source.parent.mkdir(parents=True, exist_ok=True)
        source.write_text(r'''
#include "persistence/economic_sql_source_snapshot.h"
#include <iostream>
#include <string_view>
static void hex(std::string_view value) {
    constexpr char digits[] = "0123456789abcdef";
    std::cout << '"';
    for (unsigned char byte : value) std::cout << digits[byte >> 4] << digits[byte & 15];
    std::cout << '"';
}
static void digest(const economic_sql_source_digest &value) {
    hex({reinterpret_cast<const char *>(value.data()), value.size()});
}
static void tables(const std::vector<economic_sql_source_table> &values) {
    std::cout << '[';
    bool first = true;
    for (const auto &table : values) {
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"name\":\"" << table.name << "\",\"columns\":[";
        bool column_first = true;
        for (const auto &column : table.columns) {
            if (!column_first) std::cout << ',';
            column_first = false;
            std::cout << '"' << column << '"';
        }
        std::cout << "],\"definition_digest\":"; digest(table.definition_digest);
        std::cout << ",\"content_digest\":"; digest(table.content_digest);
        std::cout << ",\"rows\":[";
        bool row_first = true;
        for (const auto &row : table.rows) {
            if (!row_first) std::cout << ',';
            row_first = false;
            std::cout << "{\"digest\":"; digest(row.digest);
            std::cout << ",\"cells\":[";
            bool cell_first = true;
            for (const auto &cell : row.cells) {
                if (!cell_first) std::cout << ',';
                cell_first = false;
                if (cell) hex(*cell); else std::cout << "null";
            }
            std::cout << "]}";
        }
        std::cout << "]}";
    }
    std::cout << ']';
}
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    MYSQL *db = mysql_init(nullptr);
    if (!db) return 3;
    bool reconnect = false;
    unsigned int connect_timeout = 5, read_timeout = 30;
    if (mysql_options(db, MYSQL_OPT_RECONNECT, &reconnect) ||
        mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout) ||
        mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &read_timeout) ||
        !mysql_real_connect(db, "localhost", "captured_reader", "disposable-capture-reader",
                            "duris_restore", 0, argv[1], 0)) {
        mysql_close(db); return 4;
    }
    economic_sql_source_snapshot result;
    const unsigned int code = economic_sql_capture_sources(db, {}, &result);
    mysql_close(db);
    if (code || economic_sql_validate_sources(result)) return 5;
    std::cout << "{\"version\":" << result.version << ",\"rows\":" << result.rows
              << ",\"cells\":" << result.cells << ",\"cell_bytes\":" << result.cell_bytes;
    std::cout << ",\"digest\":"; digest(result.digest);
    std::cout << ",\"item_sources_digest\":"; digest(result.item_sources_digest);
    std::cout << ",\"item_equipment_sources_digest\":"; digest(result.item_equipment_sources_digest);
    std::cout << ",\"custody_digest\":"; digest(result.custody_digest);
    std::cout << ",\"tables\":"; tables(result.tables);
    std::cout << ",\"item_sources\":"; tables(result.item_sources);
    std::cout << ",\"item_equipment_sources\":"; tables(result.item_equipment_sources);
    std::cout << "}\n";
}
''', encoding='utf-8')
        command = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O1', '-g',
                   '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-Isrc']
        command += shlex.split(subprocess.check_output(['mysql_config', '--cflags'], text=True))
        command += [str(source), 'src/persistence/economic_sql_source_snapshot.c']
        command += shlex.split(subprocess.check_output(['mysql_config', '--libs'], text=True))
        command += ['-lcrypto', '-o', str(binary)]
        subprocess.run(command, cwd=ROOT, check=True, timeout=600)
        artifact = ROOT/'bin/tests/captured-item-bindings'
        artifact.mkdir(mode=0o700, exist_ok=True)
        (artifact/'build.json').write_text(json.dumps(dict(command=command,
            source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
            binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()), indent=2)+'\n')
        model = CapturedItemBindingTests()
        row, modeled_packet = model.packet()
        blob = row['canonical_witness']
        payload = b'EBC1'+struct.pack('<HHII',1,48,len(blob),0)+hashlib.sha256(blob).digest()
        command_bytes = (b'CCM1'+struct.pack('<I',2)+row['operation_id']+
            struct.pack('<HHHBBQIII',20,1,6,4,0,row['command_accepted_at_usec'],1,0,48)+
            struct.pack('<B7xQ',9,0x45434f4e42415345)+payload+struct.pack('<I',256)+row['canonical_intent'])
        for engine in ('mariadb', 'mysql'):
            with self.subTest(engine=engine):
                candidate = self.base/('captured-'+engine)
                candidate.mkdir(mode=0o700)
                with restore.private_database(candidate, engine) as env:
                    version = sql(env, 'SELECT VERSION()')
                    self.assertTrue('MariaDB' in version if engine=='mariadb' else version.startswith('8.0.'))
                    sql(env, payload=(ROOT/'migrations/bootstrap_multithread_safe.sql').read_bytes())
                    with mock.patch.dict(os.environ, env, clear=True):
                        manifest = migrations.load_manifest()
                        executor = migrations.MysqlExecutor(manifest)
                        executor.adopt('fresh_bootstrap')
                        migrations.run_pending(manifest, executor)
                    terminal = sql(env, 'SELECT sequence_number,migration_id FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1')
                    self.assertEqual(terminal, '62\t0062_economic_pending_claim_consumption')
                    owner = pymysql.connect(unix_socket=env['DB_SOCKET'], user='root', database='duris_restore',
                                            autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                    try:
                        self.batches = [(blob,row['operation_id'],1,command_bytes,row['canonical_plan'],row['canonical_intent'])]
                        try: self.seed(owner)
                        finally: del self.batches
                        with owner.cursor() as cursor:
                            cursor.execute('INSERT INTO item_current_owner (item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) VALUES (81,81,NULL,1,7,0,2,100,1,0)')
                            cursor.execute('INSERT INTO item_owner_revision (owner_type,owner_id,owner_context_id,revision) VALUES (1,7,0,4)')
                            cursor.execute("CREATE USER 'captured_reader'@'localhost' IDENTIFIED BY 'disposable-capture-reader'")
                            cursor.execute("GRANT SELECT ON duris_restore.* TO 'captured_reader'@'localhost'")
                        reader = pymysql.connect(unix_socket=env['DB_SOCKET'], user='captured_reader',
                            password='disposable-capture-reader', database='duris_restore', autocommit=True,
                            cursorclass=pymysql.cursors.DictCursor, connect_timeout=5, read_timeout=30, write_timeout=5)
                        observations = []
                        try:
                            def inventory():
                                with owner.cursor() as cursor:
                                    cursor.execute('SHOW TABLES')
                                    names = [next(iter(value.values())) for value in cursor.fetchall()]
                                    values = []
                                    for name in sorted(names):
                                        cursor.execute('SELECT * FROM `'+name+'`')
                                        rows = sorted(json.dumps(value, sort_keys=True,
                                            default=lambda value: value.hex() if isinstance(value,bytes) else str(value))
                                            for value in cursor.fetchall())
                                        values.append((name, rows))
                                return hashlib.sha256(json.dumps(values).encode()).hexdigest()

                            def native_packet(label):
                                before = inventory()
                                completed = subprocess.run([str(binary),env['DB_SOCKET']], cwd=ROOT,
                                    env=dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                                             UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'),
                                    capture_output=True, check=True, timeout=600)
                                self.assertEqual(completed.stderr, b'')
                                self.assertEqual(inventory(), before)
                                packet = dict(modeled_packet, source_snapshot=json.loads(completed.stdout))
                                path = artifact/(engine+'-'+label+'-capture.json')
                                path.write_text(json.dumps(packet), encoding='utf-8')
                                path.chmod(0o600)
                                return origin_exporter.load_captured_opening(path)

                            def read(packet, label, refusal=False):
                                before = inventory()
                                connection = mock.Mock(wraps=reader)
                                cursor = mock.Mock(wraps=reader.cursor())
                                connection.cursor.return_value = cursor
                                if refusal:
                                    with self.assertRaises(OriginError):
                                        capture(connection, LINEAGE, EPOCH, captured_opening=packet)
                                    report = None
                                else:
                                    report = capture(connection, LINEAGE, EPOCH, captured_opening=packet)
                                    self.assertFalse(report['captured_item_bindings']['activation_qualified'])
                                    self.assertFalse(report['captured_item_bindings']['complete_source_capture_authenticated'])
                                connection.rollback.assert_called_once_with()
                                cursor.close.assert_called_once_with()
                                self.assertTrue(all(call.args[0].startswith(('SELECT ', 'SET TRANSACTION ', 'START TRANSACTION '))
                                                    for call in cursor.execute.call_args_list))
                                self.assertEqual(inventory(), before)
                                observations.append(dict(label=label, refused=refusal, read_only=True,
                                    rollback_calls=1, cursor_closed=True, database_sha256=before, report=report))
                            with reader.cursor() as cursor:
                                with self.assertRaises(pymysql.MySQLError) as denied:
                                    cursor.execute('UPDATE item_current_owner SET item_revision=item_revision')
                                self.assertEqual(denied.exception.args[0],1142)
                            original = native_packet('original')
                            read(original, 'original')
                            broken = copy.deepcopy(original)
                            broken['source_snapshot']['digest'] = 'ab'*32
                            read(broken, 'altered-manifest', True)
                            for label, mutation, revert in (
                                ('owner-revision', 'UPDATE item_owner_revision SET revision=5', 'UPDATE item_owner_revision SET revision=4'),
                                ('equipment', 'UPDATE item_current_owner SET equipment_slot=1', 'UPDATE item_current_owner SET equipment_slot=0'),
                                ('prototype', 'UPDATE item_current_owner SET vnum=101', 'UPDATE item_current_owner SET vnum=100')):
                                with owner.cursor() as cursor: cursor.execute(mutation)
                                read(native_packet(label),label,True)
                                read(original,label+'-retained-original')
                                with owner.cursor() as cursor: cursor.execute(revert)
                            read(native_packet('restored'),'restored')
                            (artifact/(engine+'-observations.json')).write_text(json.dumps(dict(
                                engine=engine,version=version,canonical_terminal=terminal,observations=observations,
                                original_native_capture=True, modeled_opening=True,
                                private_new_producer_oracle=False, release_qualified=False),indent=2)+'\n')
                            print('CAPTURED_ITEM_NATIVE '+json.dumps(dict(engine=engine,version=version,
                                canonical_terminal=terminal,reads=len(observations),refusals=sum(value['refused'] for value in observations),
                                native_captures=5,modeled_opening=True,activation_qualified=False)),flush=True)
                        finally: reader.close()
                    finally: owner.close()


if __name__ == "__main__":
    unittest.main()
