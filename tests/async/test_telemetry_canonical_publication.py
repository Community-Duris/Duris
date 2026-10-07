#!/usr/bin/env python3
"""Generation evidence/unknown attribution contracts; native SQL proof is separate."""
from dataclasses import replace
import hashlib
import io
from contextlib import redirect_stderr
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from scripts.telemetry import canonical_reward_contract as contract
from scripts.telemetry.canonical_reward_source import CanonicalRewardSnapshot
from scripts.telemetry.canonical_reward_publication import make_generation, event_payload, decode_event, projection_digest
from scripts.telemetry.reward_projection import ProjectionBoundsExceeded
from scripts.telemetry.canonical_reward_retention import CanonicalRewardRetention
from scripts.telemetry.canonical_reward_conflicts import ConflictWitness
from test_telemetry_canonical_conflicts import captured, altered, Connection, Reader
from test_telemetry_reward_projection import operation, bank_evidence, _FakeFactory, _SnapshotConnection
from test_telemetry_canonical_health import health_payloads


class CanonicalPublicationTest(unittest.TestCase):
    def test_capture_cli_requires_exact_native_selection_and_separate_command_options(self):
        from scripts.telemetry import canonical_reward as cli
        first, second = operation(111).hex(), operation(112).hex()
        with patch.object(cli, "_run_killable_process", return_value=0) as worker:
            self.assertEqual(cli.main(["capture", "--route", "wallet-pile", "--operation", second,
                "--operation", first, "--quarantine"]), 0)
        self.assertEqual(worker.call_args.args[1], ("capture", None, (), None, 32 * 1024 * 1024,
            10.0, "wallet-pile", (operation(111), operation(112)), True))
        invalid = (["capture", "--route", "auction"],
            ["capture", "--operation", first],
            ["capture", "--route", "bank", "--operation", first, "--operation", first],
            ["capture", "--route", "bank", "--operation", first, "--generation", second],
            ["capture", "--route", "bank", "--operation", first, "--cut", "11" * 32],
            ["stage", "--generation", first, "--cut", "11" * 32, "--route", "bank"],
            ["report"], ["publish", "--generation", first, "--quarantine"])
        for arguments in invalid:
            with self.subTest(arguments=arguments), redirect_stderr(io.StringIO()), \
                    patch.object(cli, "_run_killable_process") as worker, self.assertRaises(SystemExit) as refused:
                cli.main(arguments)
            self.assertEqual(refused.exception.code, 2)
            worker.assert_not_called()

    def test_bank_quarantine_keeps_one_economic_root_despite_changed_participant_receipt(self):
        good = captured()
        for damaged in ("posting", "beneficiary"):
            with self.subTest(damaged=damaged):
                evidence = bank_evidence()
                if damaged == "posting":
                    evidence[4][0]["copper_value"] += 1
                else:
                    evidence[1]["pid"] += 1
                connection = _SnapshotConnection([evidence])
                refused = CanonicalRewardSnapshot(_FakeFactory(connection)).capture_bank_operations(
                    [operation(111)], preserve_refusals=True)
                self.assertEqual(refused.events[0].disposition, contract.Disposition.UNKNOWN)
                original = {(source.reference.table, source.reference.key): source for source in good.sources}
                witnesses = tuple(ConflictWitness(refused.source_digest, index, source)
                    for index, source in enumerate(refused.sources)
                    if source.reference.payload_digest !=
                        original[(source.reference.table, source.reference.key)].reference.payload_digest)
                cuts = tuple(sorted((good, refused), key=lambda cut: cut.source_digest))
                plan = make_generation(operation(55), 2_000_000, cuts, witnesses)
                self.assertEqual(len(plan.events), 1)
                self.assertEqual(plan.events[0].identity, good.events[0].identity)
                self.assertEqual(plan.events[0].identity.participant_pid, 0)
                self.assertEqual(plan.events[0].disposition, contract.Disposition.CONFLICT)
                self.assertIsNone(plan.events[0].amount)
                self.assertEqual(contract.earned_totals(plan.events), {})

    def test_combined_private_payloads_stay_within_each_write_statement_budget(self):
        store = object.__new__(CanonicalRewardRetention)
        statements = []
        store._execute = lambda statement, parameters: statements.append(parameters)
        rows = [(operation(55), index, b"a" * 8192, b"b" * 32, b"c" * 8192, b"d" * 32)
                for index in range(65)]
        store._bulk("private_health", ("generation_id", "health_index", "payload", "payload_digest",
                                      "projection_payload", "projection_digest"), rows)
        restored = []
        for parameters in statements:
            self.assertLessEqual(sum(len(cell) for cell in parameters if type(cell) is bytes), 512 * 1024)
            self.assertLessEqual(len(parameters) // 6, 64)
            restored.extend(tuple(parameters[start:start + 6]) for start in range(0, len(parameters), 6))
        self.assertEqual(restored, rows)
        with self.assertRaises(ProjectionBoundsExceeded):
            store._bulk("private_health", ("payload",), [(b"x" * (512 * 1024 + 1),)])

    def test_batch_and_generation_agree_when_quarantine_sorts_before_valid_authority(self):
        from scripts.telemetry.canonical_reward_source import source_cut_digest
        good = captured()
        evidence = bank_evidence()
        evidence[4][0]["copper_value"] += 1
        refused = CanonicalRewardSnapshot(_FakeFactory(_SnapshotConnection([evidence]))).capture_bank_operations(
            [operation(111)], preserve_refusals=True)
        original = {(source.reference.table, source.reference.key): source for source in good.sources}
        orders = set()
        for label in (1, 5):
            quarantine = replace(refused, captured_utc_usec=label,
                source_digest=source_cut_digest(label, refused.selected_operations, refused.sources))
            cuts = {cut.source_digest: cut for cut in (good, quarantine)}

            class BatchReader(Reader):
                def _header(self, cut_id, **kwargs):
                    return dict(sealed=1, cut_id=cut_id)

                def _load_rows(self, header):
                    return cuts[header["cut_id"]]

            variants = []
            for index, changed in enumerate(quarantine.sources):
                source = original[(changed.reference.table, changed.reference.key)]
                if source.reference.payload_digest != changed.reference.payload_digest:
                    variants.extend((ConflictWitness(quarantine.source_digest, index, changed),
                        ConflictWitness(good.source_digest, good.sources.index(source), source)))
            selection = tuple(sorted(cuts))
            orders.add(selection[0] == quarantine.source_digest)
            batch = BatchReader(_FakeFactory(Connection(variants)), good).resolve_many(selection)
            plan = make_generation(operation(55), batch.retained_snapshot_utc_usec,
                tuple(row.cut for row in batch.cuts), tuple(witness for row in batch.cuts for witness in row.witnesses))
            self.assertEqual(batch.events, plan.events)
            self.assertEqual(len(plan.events), 1)
            self.assertEqual(plan.events[0].disposition, contract.Disposition.CONFLICT)
            self.assertIsNone(plan.events[0].amount)
            self.assertEqual(set(plan.events[0].references),
                {source.reference for cut in cuts.values() for source in cut.sources})
        self.assertEqual(orders, {False, True})

    def test_generation_binds_frozen_health_and_derives_visible_gaps(self):
        cut = captured()
        plain = make_generation(operation(55), 2_000_000, (cut,), ())
        healthy = make_generation(operation(55), 2_000_000, (cut,), (), health_payloads=health_payloads())
        gap = make_generation(operation(55), 2_000_000, (cut,), (), health_payloads=health_payloads(gap=True))
        self.assertEqual(healthy.events, gap.events)
        self.assertEqual(healthy.header["projection_digest"], gap.header["projection_digest"])
        self.assertEqual(gap.header["health_count"], 257)
        self.assertNotEqual(healthy.header["evidence_digest"], gap.header["evidence_digest"])
        self.assertNotEqual(healthy.header["health_projection_digest"], gap.header["health_projection_digest"])
        self.assertNotEqual(plain.header["evidence_digest"], healthy.header["evidence_digest"])
        self.assertEqual(contract.decode_source_payload(gap.header["coverage_payload"])["sweep_retained_metadata_gaps"], 1)
        self.assertEqual(contract.decode_source_payload(plain.header["coverage_payload"])["sweep_revision"], None)
        with self.assertRaises(ProjectionBoundsExceeded):
            make_generation(operation(55), 2_000_000, (cut,), (), health_payloads=health_payloads(),
                byte_limit=healthy.header["reserved_bytes"] - 1)

    def test_native_authority_units_and_unknown_ownership_survive_projection(self):
        cut = captured()
        plan = make_generation(operation(55), 2_000_000, (cut,), ())
        row = decode_event(plan.rows[0]["payload"])
        self.assertEqual(row["amount"], 100)
        self.assertEqual(row["unit"], int(contract.Unit.CURRENCY))
        self.assertEqual(row["reference_count"], len(cut.events[0].references))
        for field in ("award_utc_usec", "account_token", "controller_token", "identity_registry_version"):
            self.assertIsNone(row[field])
        coverage = contract.decode_source_payload(plan.header["coverage_payload"])
        self.assertIsNone(coverage["source_backlog"])
        self.assertEqual(coverage["retention_acknowledged"], 0)
        self.assertEqual(coverage["future_commits_provisional"], 1)
        self.assertEqual(coverage["currency_earned_copper"], 100)

    def test_conflict_keeps_exact_binding_and_removes_the_original_amount(self):
        cut = captured()
        witness = altered(cut)
        plan = make_generation(operation(55), 2_000_000, (cut,), (witness,))
        self.assertEqual(len(plan.rows), 1)
        self.assertEqual(plan.events[0].disposition, contract.Disposition.CONFLICT)
        self.assertIsNone(decode_event(plan.rows[0]["payload"])["amount"])
        self.assertEqual(plan.bindings[1]["cut_id"], witness.cut_id)
        self.assertEqual(plan.bindings[1]["payload_digest"], witness.source.reference.payload_digest)
        self.assertIsNone(contract.decode_source_payload(plan.header["coverage_payload"])["currency_earned_copper"])

    def test_identical_witnesses_do_not_duplicate_bindings(self):
        cut, witness = captured(), altered(captured())
        one = make_generation(operation(55), 2_000_000, (cut,), (witness,))
        repeated = make_generation(operation(55), 2_000_000, (cut,), (witness, witness))
        self.assertEqual(one, repeated)

    def test_overlapping_exact_cuts_count_one_event_and_all_evidence(self):
        cut = captured()
        # A second retained source cut has a different capture label, while all
        # exact native receipt identities and bytes remain the same.
        from scripts.telemetry.canonical_reward_source import source_cut_digest
        later = replace(cut, captured_utc_usec=cut.captured_utc_usec + 1,
            source_digest=source_cut_digest(cut.captured_utc_usec + 1, cut.selected_operations, cut.sources))
        cuts = tuple(sorted((cut, later), key=lambda value: value.source_digest))
        plan = make_generation(operation(55), 2_000_000, cuts, ())
        self.assertEqual(len(plan.bindings), 2)
        self.assertEqual(len(plan.events), 1)
        self.assertEqual(contract.earned_totals(plan.events), {contract.Unit.CURRENCY: 100})

    def test_generation_identity_snapshot_and_witness_location_are_bound(self):
        cut, witness = captured(), altered(captured())
        original = make_generation(operation(55), 2_000_000, (cut,), (witness,))
        for generation, snapshot, changed in ((operation(56), 2_000_000, witness),
            (operation(55), 2_000_001, witness), (operation(55), 2_000_000, replace(witness, source_index=2))):
            plan = make_generation(generation, snapshot, (cut,), (changed,))
            self.assertNotEqual(original.header["evidence_digest"], plan.header["evidence_digest"])

    def test_projection_refuses_changed_bytes_foreign_identity_and_authority(self):
        cut = captured()
        row = decode_event(event_payload(cut.events[0]))
        for change in (dict(account_token=123), dict(award_utc_usec=2_000_000),
                       dict(unit=int(contract.Unit.OBSERVED_XP)), dict(reference_count=0)):
            changed = dict(row, **change)
            with self.subTest(change=change), self.assertRaises(contract.EvidenceError):
                decode_event(contract.encode_source_payload(changed))
        plan = make_generation(operation(55), 2_000_000, (cut,), ())
        changed = dict(plan.rows[0], payload=plan.rows[0]["payload"] + b" ")
        with self.assertRaises(contract.EvidenceError):
            projection_digest(plan.generation_id, (changed,))

    def test_source_and_projection_reservations_refuse_partial_generation(self):
        cut = captured()
        plan = make_generation(operation(55), 2_000_000, (cut,), ())
        with self.assertRaises(ProjectionBoundsExceeded):
            make_generation(operation(55), 2_000_000, (cut,), (), byte_limit=plan.header["reserved_bytes"] - 1)
        with self.assertRaises(contract.EvidenceError):
            make_generation(operation(55), 2_000_000, (replace(cut, events=()),), ())

    def test_batch_conflict_discovery_uses_one_snapshot_and_one_combined_budget(self):
        first = captured()
        second = CanonicalRewardSnapshot(_FakeFactory(_SnapshotConnection([bank_evidence(operation_number=112)]))).capture_bank_operations([operation(112)])
        cuts = {cut.source_digest: cut for cut in (first, second)}

        class BatchReader(Reader):
            def _header(self, cut_id, **kwargs):
                return dict(sealed=1, cut_id=cut_id)

            def _load_rows(self, header):
                return cuts[header["cut_id"]]

        connection = Connection()
        factory = _FakeFactory(connection)
        batch = BatchReader(factory, first).resolve_many(tuple(sorted(cuts)))
        self.assertEqual(factory.connects, 1)
        self.assertEqual(len(batch.cuts), 2)
        self.assertEqual(contract.earned_totals(batch.events), {contract.Unit.CURRENCY: 200})
        self.assertEqual({row.retained_snapshot_utc_usec for row in batch.cuts}, {batch.retained_snapshot_utc_usec})
        with self.assertRaises(ProjectionBoundsExceeded):
            BatchReader(_FakeFactory(Connection()), first, byte_limit=first.reserved_bytes + second.reserved_bytes - 1).resolve_many(tuple(sorted(cuts)))
        with self.assertRaises(ProjectionBoundsExceeded):
            BatchReader(_FakeFactory(Connection()), first, conflict_query_limit=len(first.sources)).resolve_many(tuple(sorted(cuts)))

    def test_overlapping_cuts_reuse_exact_variant_seeks_only_inside_the_owning_snapshot(self):
        from scripts.telemetry.canonical_reward_source import source_cut_digest
        first = captured()
        later = replace(first, captured_utc_usec=first.captured_utc_usec + 1,
            source_digest=source_cut_digest(first.captured_utc_usec + 1, first.selected_operations, first.sources))
        cuts = {cut.source_digest: cut for cut in (first, later)}

        class BatchReader(Reader):
            def _header(self, cut_id, **kwargs):
                return dict(sealed=1, cut_id=cut_id)

            def _load_rows(self, header):
                return cuts[header["cut_id"]]

        witness = altered(first)
        connection = Connection((witness,))
        limit = len(first.sources) + 1
        batch = BatchReader(_FakeFactory(connection), first, conflict_query_limit=limit).resolve_many(tuple(sorted(cuts)))
        self.assertEqual(batch.conflict_query_count, limit)
        self.assertEqual(connection.variant_queries, limit)
        self.assertEqual(len(batch.events), 1)
        self.assertEqual(batch.events[0].disposition, contract.Disposition.CONFLICT)
        self.assertEqual(tuple(resolved.witnesses for resolved in batch.cuts), ((witness,), (witness,)))
        # A separate read must seek again; a later-visible conflict cannot be
        # hidden by reusing an earlier transaction's empty variant result.
        fresh = Connection()
        clean = BatchReader(_FakeFactory(fresh), first, conflict_query_limit=limit).resolve_many(tuple(sorted(cuts)))
        self.assertEqual(fresh.variant_queries, len(first.sources))
        self.assertEqual(contract.earned_totals(clean.events), {contract.Unit.CURRENCY: 100})

        # A different payload under the same physical key has a different
        # exclusion digest and must perform its own seek in the shared cut.
        evidence = bank_evidence()
        evidence[4][0]["copper_value"] += 1
        quarantine = CanonicalRewardSnapshot(_FakeFactory(_SnapshotConnection([evidence]))).capture_bank_operations(
            [operation(111)], preserve_refusals=True)
        changed_source = next(source for source in quarantine.sources
                              if source.reference == witness.source.reference)
        original_source = next(source for source in first.sources
            if (source.reference.table, source.reference.key) == (changed_source.reference.table, changed_source.reference.key))
        cuts = {cut.source_digest: cut for cut in (first, quarantine)}
        variants = (ConflictWitness(quarantine.source_digest, quarantine.sources.index(changed_source), changed_source),
                    ConflictWitness(first.source_digest, first.sources.index(original_source), original_source))
        different = Connection(variants)
        batch = BatchReader(_FakeFactory(different), first, conflict_query_limit=len(first.sources) + 3).resolve_many(tuple(sorted(cuts)))
        self.assertEqual(different.variant_queries, len(first.sources) + 3)
        self.assertEqual(len(batch.events), 1)
        self.assertEqual(batch.events[0].disposition, contract.Disposition.CONFLICT)
        self.assertEqual({ref.payload_digest for ref in batch.events[0].references
            if (ref.table, ref.key) == (original_source.reference.table, original_source.reference.key)},
            {original_source.reference.payload_digest, changed_source.reference.payload_digest})


if __name__ == "__main__":
    unittest.main()
