#!/usr/bin/env python3
"""Exact retained-conflict boundary checks; native SQL proof is a separate gate."""
from dataclasses import replace
import hashlib
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_source import CanonicalRewardSnapshot, RetainedSource
from scripts.telemetry.canonical_reward_retention import RETENTION_TABLES
from scripts.telemetry.canonical_reward_conflicts import (
    CanonicalRewardConflictReader, ConflictWitness, apply_conflicts, evidence_digest,
)
from scripts.telemetry.reward_projection import ProjectionBoundsExceeded
from test_telemetry_reward_projection import bank_evidence, operation, _SnapshotConnection, _FakeFactory


def captured():
    return CanonicalRewardSnapshot(_FakeFactory(_SnapshotConnection([bank_evidence()]))).capture_bank_operations([operation(111)])


def altered(cut, *, amount_delta=1, number=9):
    source = next(row for row in cut.sources if row.reference.table == "economic_accounting_coin_posting")
    row = contract.decode_source_payload(source.payload)
    row["copper_value"] += amount_delta
    payload = contract.encode_source_payload(row)
    retained = RetainedSource(replace(source.reference, payload_digest=hashlib.sha256(payload).digest()), payload)
    return ConflictWitness(bytes([number]) * 32, 1, retained)


class Cursor:
    def __init__(self, connection):
        self.connection = connection
        self.description = None
        self.rows = []

    def execute(self, statement, parameters):
        connection = self.connection
        connection.statements.append((statement, parameters))
        self.description = ("column",) if statement.startswith("SELECT") else None
        if statement.startswith("START TRANSACTION"):
            connection.snapshot = list(connection.witnesses)
        elif "information_schema.tables" in statement:
            self.rows = [dict(table_name=name, engine="InnoDB") for name in RETENTION_TABLES]
        elif "UTC_TIMESTAMP" in statement:
            self.rows = [dict(captured_utc_usec=1_000_000)]
        elif "idx_reward2_source_payloads" in statement:
            connection.variant_queries += 1
            if connection.fail_at == connection.variant_queries:
                raise OSError("injected lost reader")
            if connection.after_first_query is not None:
                callback, connection.after_first_query = connection.after_first_query, None
                callback()
            table, key, after, excluded = parameters
            candidates = sorted((w for w in connection.snapshot if w.source.reference.table.encode() == table and
                w.source.reference.key == key and w.source.reference.payload_digest > after and
                w.source.reference.payload_digest != excluded), key=lambda w: (w.source.reference.payload_digest, w.cut_id))
            self.rows = [dict(cut_id=w.cut_id, source_index=w.source_index, payload=w.source.payload,
                payload_digest=w.source.reference.payload_digest) for w in candidates[:1]]
        elif statement.startswith("SELECT"):
            raise AssertionError(statement)

    def fetchall(self):
        return self.rows

    def close(self):
        pass


class Connection:
    def __init__(self, witnesses=()):
        self.witnesses = list(witnesses)
        self.statements, self.snapshot = [], []
        self.variant_queries, self.rollbacks = 0, 0
        self.fail_at = None
        self.after_first_query = None
        self.closed = False

    def cursor(self):
        return Cursor(self)

    def rollback(self):
        self.rollbacks += 1

    def close(self):
        self.closed = True


class Reader(CanonicalRewardConflictReader):
    # The focused SQL gate exercises the actual retained header/row loader.
    def __init__(self, factory, cut, **kwargs):
        super().__init__(factory, **kwargs)
        self.cut = cut

    def _header(self, cut_id, **kwargs):
        assert cut_id == self.cut.source_digest
        return dict(sealed=1)

    def _load_rows(self, header):
        return self.cut


class CanonicalConflictTest(unittest.TestCase):
    def test_selected_valid_event_loses_amount_to_external_exact_witness(self):
        cut = captured()
        witness = altered(cut)
        resolved = apply_conflicts(cut.events, (witness,))
        self.assertEqual(resolved[0].identity, cut.events[0].identity)
        self.assertEqual(resolved[0].disposition, contract.Disposition.CONFLICT)
        self.assertIsNone(resolved[0].amount)
        self.assertIn(witness.source.reference, resolved[0].references)
        self.assertEqual(len(resolved[0].references), len(cut.events[0].references) + 1)
        self.assertEqual(contract.earned_totals(resolved), {})

    def test_all_events_citing_a_physical_source_lose_authority(self):
        cut = captured()
        other = replace(cut.events[0], identity=replace(cut.events[0].identity, event_index=1))
        events = apply_conflicts((*cut.events, other), (altered(cut),))
        self.assertEqual(len(events), 2)
        self.assertTrue(all(event.amount is None for event in events))

    def test_unchanged_source_replay_keeps_earned_authority(self):
        cut = captured()
        self.assertEqual(apply_conflicts(cut.events, ()), contract.reconcile_events(cut.events))

    def test_unrelated_key_corrupt_payload_and_wrong_physical_key_are_refused(self):
        cut, witness = captured(), altered(captured())
        wrong = replace(witness.source, reference=replace(witness.source.reference, key=operation(222) + bytes(2)))
        corrupt = replace(witness.source, payload=witness.source.payload + b" ")
        for source in (wrong, corrupt):
            with self.subTest(source=source.reference.key), self.assertRaises(contract.EvidenceError):
                apply_conflicts(cut.events, (replace(witness, source=source),))
        unrelated = altered(CanonicalRewardSnapshot(_FakeFactory(_SnapshotConnection([bank_evidence(operation_number=222)]))).capture_bank_operations([operation(222)]))
        with self.assertRaisesRegex(contract.EvidenceError, "witness_scope"):
            apply_conflicts(cut.events, (unrelated,))

    def test_reader_uses_one_owning_cut_and_retains_each_distinct_payload_once(self):
        cut = captured()
        first, second = altered(cut), altered(cut, amount_delta=2, number=8)
        connection = Connection((first, replace(first, cut_id=b"\x0a" * 32), second))
        factory = _FakeFactory(connection)
        result = Reader(factory, cut).resolve(cut.source_digest)
        self.assertEqual(factory.connects, 1)
        self.assertEqual(len(result.witnesses), 2)
        self.assertEqual(result.conflict_query_count, len(cut.sources) + 2)
        self.assertTrue(result.future_evidence_provisional)
        self.assertEqual(contract.earned_totals(result.events), {})
        self.assertTrue(connection.closed)
        starts = [sql for sql, _ in connection.statements if sql.startswith("START TRANSACTION")]
        self.assertEqual(starts, ["START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY"])
        self.assertTrue(all("LIMIT 1" in sql for sql, _ in connection.statements if "idx_reward2_source_payloads" in sql))

    def test_later_conflict_is_excluded_then_visible_in_a_fresh_reader(self):
        cut = captured()
        connection = Connection()
        connection.after_first_query = lambda: connection.witnesses.append(altered(cut))
        first = Reader(_FakeFactory(connection), cut).resolve(cut.source_digest)
        self.assertEqual(contract.earned_totals(first.events), {contract.Unit.CURRENCY: 100})
        second = Reader(_FakeFactory(Connection(connection.witnesses)), cut).resolve(cut.source_digest)
        self.assertEqual(contract.earned_totals(second.events), {})
        self.assertNotEqual(first.evidence_digest, second.evidence_digest)

    def test_query_byte_and_connection_failures_return_no_partial_result(self):
        cut = captured()
        for kwargs, fail_at, expected in ((dict(conflict_query_limit=1), None, ProjectionBoundsExceeded),
            (dict(byte_limit=cut.reserved_bytes), None, ProjectionBoundsExceeded), ({}, 1, OSError)):
            connection = Connection((altered(cut),))
            connection.fail_at = fail_at
            with self.subTest(kwargs=kwargs), self.assertRaises(expected):
                Reader(_FakeFactory(connection), cut, **kwargs).resolve(cut.source_digest)
            self.assertTrue(connection.closed)
            self.assertGreaterEqual(connection.rollbacks, 2)

    def test_evidence_digest_binds_the_exact_witness_location_and_snapshot_label(self):
        cut, witness = captured(), altered(captured())
        initial = evidence_digest(cut.source_digest, 1, (witness,))
        self.assertNotEqual(initial, evidence_digest(cut.source_digest, 2, (witness,)))
        self.assertNotEqual(initial, evidence_digest(cut.source_digest, 1, (replace(witness, source_index=2),)))
        self.assertNotEqual(initial, evidence_digest(cut.source_digest, 1, (replace(witness, cut_id=b"\x08" * 32),)))


if __name__ == "__main__":
    unittest.main()
