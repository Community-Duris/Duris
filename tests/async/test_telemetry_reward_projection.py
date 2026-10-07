#!/usr/bin/env python3
"""Pure and SQL-boundary tests for telemetry reward projection (#270)."""

from __future__ import annotations

from datetime import datetime, timezone
from dataclasses import replace
import copy
import hashlib
import struct
import sys
import unittest

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from scripts.telemetry.reward_projection import (  # noqa: E402
    AmbiguousCommit,
    SOURCE_ADAPTERS,
    RewardProjectionStore,
    build_source_page_query,
)
from scripts.telemetry.reward_projection_definitions import (  # noqa: E402
    AuthorityKind,
    ProjectionQuality,
    ProjectionState,
    ProjectionStatus,
    RewardKind,
    SourceCursor,
    SourceKind,
    SourceObservation,
    ZERO_OPERATION_ID,
    currency_value,
    project_observations,
    retention_warnings,
)
from scripts.telemetry import canonical_reward_contract as canonical  # noqa: E402
from scripts.telemetry.canonical_reward_source import (  # noqa: E402
    CanonicalRewardSnapshot, SOURCE_TABLES, ProjectionBoundsExceeded,
    replay_retained_bank_cut, source_cut_digest,
)


def bank_evidence(*, disposition=canonical.Disposition.EARNED, operation_number=111):
    """Independent boundary fixture, not a native gameplay qualification.

    Native adapter bytes are cross-checked separately by the existing adapter
    runner; actual SQL/gameplay receipt coverage is a separate acceptance gate.
    """
    op, lineage, epoch = operation(operation_number), operation(1), operation(2)
    pid = 7
    source = None
    if disposition == canonical.Disposition.EARNED:
        writer, reason, legacy = 4, 5, 5
        wallet_before, wallet_after = (2, 0, 0, 0), (102, 0, 0, 0)
        bank_before = bank_after = (0, 1, 0, 0)
        source = struct.pack("<HH", 1, 1) + op + op + struct.pack("<QI", 17, 1)
        reason_id, site = 17, 5
    elif disposition == canonical.Disposition.OPENING:
        writer, reason, legacy = 3, 8, 15
        seed = b"CHAOSEED" + struct.pack("<Q", pid)
        op = hashlib.sha256(seed + struct.pack("<IQ", 0x43484250, 1)).digest()[:16]
        wallet_before = wallet_after = (2, 0, 0, 0)
        bank_before, bank_after = (0, 1, 0, 0), (0, 1, 0, 1_000_000)
        source = struct.pack("<HH", 3, 1) + seed + seed + struct.pack("<QI", 1, 1)
        reason_id, site = pid, 4
    else:
        writer, reason, legacy = 1, 1, 1
        wallet_before, wallet_after = (102, 0, 0, 0), (2, 0, 0, 0)
        bank_before, bank_after = (0, 1, 0, 0), (100, 1, 0, 0)
        reason_id, site = 0, 1
    intent = bytearray(280)
    actor = pid if disposition == canonical.Disposition.EARNED else 71
    intent[:4] = b"EAI1"
    struct.pack_into("<HHIIIIHBBH", intent, 4, 1, 256, 280, writer, 1, 1, reason, 1, int(source is not None), 1)
    intent[32:48], intent[48:64], intent[64:80] = lineage, epoch, op
    struct.pack_into("<QI", intent, 96, actor, 24)
    intent[112:160] = source or bytes(48)
    intent[160:192], intent[192:224] = bytes([11]) * 32, bytes([12]) * 32
    struct.pack_into("<3Q", intent, 256, 71, 81, 1)
    intent = bytes(intent)
    header = bytearray(256)
    header[:4] = b"EAP1"
    struct.pack_into("<H", header, 4, 1)
    header[8:24], header[24:40], header[40:56] = lineage, epoch, op
    struct.pack_into("<B3xQIIIH", header, 72, 1, actor, writer, 1, 1, reason)
    header[100] = int(source is not None)
    header[104:152] = source or bytes(48)
    header[152:184] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
    header[184:216] = intent[192:224]
    issuance = disposition != canonical.Disposition.TRANSFER
    struct.pack_into("<6I", header, 216, 3 if issuance else 2, 2, 0, 0, 0, 0)
    key = lambda kind, identity, context=0: lineage + struct.pack("<HHQQ4x", 1, kind, identity, context)
    account = lambda raw_key, before, after, br, ar: raw_key + struct.pack("<8q2Q", *before, *after, br, ar)
    raw = bytes(header) + account(key(1, 71), wallet_before, wallet_after, 4, 5)
    raw += account(key(2, 81, 1), bank_before, bank_after, 9, 10)
    if issuance:
        raw += account(key(7, 1), (0,) * 4, (0,) * 4, 0, 0)
    wallet_delta = tuple(a - b for a, b in zip(wallet_after, wallet_before))
    bank_delta = tuple(a - b for a, b in zip(bank_after, bank_before))
    recipient = 1 if disposition == canonical.Disposition.OPENING else 0
    first = bank_delta if recipient == 1 else wallet_delta
    second = tuple(-n for n in first) if issuance else bank_delta
    raw += struct.pack("<IHH5q", 0, recipient, 0, *first, currency_value(first))
    raw += struct.pack("<IHH5q", 1, 2 if issuance else 1, 0, *second, currency_value(second))
    plan = canonical.decode_plan(raw)
    root = dict(plan.metadata, canonical_plan=raw, canonical_intent=intent,
                plan_digest=hashlib.sha256(raw).digest(), outcome=1, result_code=0)
    ledger_row = dict(operation_id=op, pid=pid, bank_id=3, reason_type=legacy,
                      reason_id=reason_id, source_site=site, wallet_revision=5, bank_revision=10)
    for prefix, values in (("wallet_delta_", wallet_delta), ("bank_delta_", bank_delta),
                           ("wallet_after_", wallet_after), ("bank_after_", bank_after)):
        ledger_row.update({prefix + name: value for name, value in zip(canonical.COINS, values)})
    inbox = dict(operation_id=op, command_type=3, schema_version=2, payload_version=1, status=1,
                 result_code=0, failure_stage=0, durable_revision=10, command_hash=bytes([13]) * 32,
                 keys_hash=bytes([14]) * 32,
                 result_payload=struct.pack("<8q2Q", *wallet_after, *bank_after, 5, 10))
    effects, postings = [], []
    for index, value in enumerate(plan.accounts):
        row = dict(operation_id=op, account_index=index, account_key=value.key,
                   before_revision=value.before_revision, after_revision=value.after_revision)
        for prefix, vector in (("before_", value.before), ("after_", value.after)):
            row.update({prefix + name: part for name, part in zip(canonical.COINS, vector)})
        effects.append(row)
    for index, value in enumerate(plan.postings):
        row = dict(operation_id=op, line_index=index, event_index=value.event_index,
                   account_index=value.account_index, child_index=value.child_index,
                   copper_value=value.copper_value)
        row.update({"delta_" + name: part for name, part in zip(canonical.COINS, value.delta)})
        postings.append(row)
    claims = [dict(lineage=lineage, source_event=source, operation_id=op, outcome=1)] if source else []
    return root, ledger_row, inbox, effects, postings, claims


def native_bank_component_evidence(parts):
    """Build boundary-test SQL cells from separately emitted native component bytes.

    The completed inbox/SQL rows here are fixtures, not actual committed rows.
    The maintained native gameplay gate must independently prove their origin.
    """
    raw, intent, payload, result, command_hash, keys_hash = parts
    plan = canonical.decode_plan(raw)
    meta = plan.metadata
    op = meta["operation_id"]
    root = dict(meta, canonical_plan=raw, canonical_intent=intent,
                plan_digest=hashlib.sha256(raw).digest(), outcome=1, result_code=0)
    pid = struct.unpack_from("<I", payload, 0)[0]
    reason = struct.unpack_from("<H", payload, 6)[0]
    wallet_after = struct.unpack_from("<4q", result, 0)
    bank_after = struct.unpack_from("<4q", result, 32)
    wallet_revision, bank_revision = struct.unpack_from("<2Q", result, 64)
    ledger_row = dict(operation_id=op, pid=pid, bank_id=3, reason_type=reason,
                      reason_id=struct.unpack_from("<q", payload, 8)[0],
                      source_site=5 if reason == 5 else 4 if reason == 15 else 1,
                      wallet_revision=wallet_revision, bank_revision=bank_revision)
    for prefix, values in (("wallet_delta_", struct.unpack_from("<4q", payload, 68)),
                           ("bank_delta_", struct.unpack_from("<4q", payload, 100)),
                           ("wallet_after_", wallet_after), ("bank_after_", bank_after)):
        ledger_row.update({prefix + name: value for name, value in zip(canonical.COINS, values)})
    inbox = dict(operation_id=op, command_type=3, schema_version=2, payload_version=1, status=1,
                 result_code=0, failure_stage=0, durable_revision=max(wallet_revision, bank_revision),
                 command_hash=command_hash, keys_hash=keys_hash,
                 result_payload=result)
    effects, postings = [], []
    # Exact canonical rows independently unpacked from native EAP1 bytes.
    for index in range(meta["account_count"]):
        start = 256 + index * 120
        values = struct.unpack_from("<8q2Q", raw, start + 40)
        row = dict(operation_id=op, account_index=index, account_key=raw[start:start + 40],
                   before_revision=values[8], after_revision=values[9])
        for prefix, vector in (("before_", values[:4]), ("after_", values[4:8])):
            row.update({prefix + name: part for name, part in zip(canonical.COINS, vector)})
        effects.append(row)
    for index in range(meta["posting_count"]):
        start = 256 + meta["account_count"] * 120 + index * 48
        event, account, child, *values = struct.unpack_from("<IHH5q", raw, start)
        row = dict(operation_id=op, line_index=index, event_index=event, account_index=account,
                   child_index=child, copper_value=values[4])
        row.update({"delta_" + name: part for name, part in zip(canonical.COINS, values[:4])})
        postings.append(row)
    claims = [dict(lineage=meta["lineage"], source_event=meta["source_event"], operation_id=op, outcome=1)] if meta["source_event"] else []
    return root, ledger_row, inbox, effects, postings, claims


class CanonicalRewardContractTest(unittest.TestCase):
    def test_exact_typed_quest_receipts_count_one_currency_earned_event(self):
        evidence = bank_evidence()
        event = canonical.qualify_bank_root(*evidence)
        self.assertEqual(event.disposition, canonical.Disposition.EARNED)
        self.assertEqual(event.amount, 100)
        self.assertEqual(event.wallet_lifetime, 71)
        self.assertEqual(len(event.references), 9)
        self.assertEqual(canonical.earned_totals([event, event]), {canonical.Unit.CURRENCY: 100})

    def test_transfer_and_initial_supply_do_not_become_earned(self):
        for disposition, amount in ((canonical.Disposition.TRANSFER, 100),
                                    (canonical.Disposition.OPENING, 1_000_000_000)):
            event = canonical.qualify_bank_root(*bank_evidence(disposition=disposition))
            self.assertEqual(event.disposition, disposition)
            self.assertEqual(event.amount, amount)
            self.assertEqual(canonical.earned_totals([event]), {})

    def test_every_native_receipt_boundary_refuses_disagreement(self):
        cases = ((0, "actor_id", 8), (0, "plan_digest", bytes([99]) * 32),
                 (0, "outcome", True), (1, "pid", 8), (1, "wallet_delta_copper", 101),
                 (1, "source_site", 1), (1, "reason_id", -1), (2, "status", 0),
                 (2, "schema_version", 1), (2, "failure_stage", 1),
                 (2, "durable_revision", 11),
                 (2, "result_payload", bytes(80)))
        for index, column, value in cases:
            with self.subTest(column=column):
                evidence = list(bank_evidence())
                evidence[index][column] = value
                with self.assertRaises(canonical.EvidenceError):
                    canonical.qualify_bank_root(*evidence)
        for index, column in ((3, "account_key"), (4, "copper_value"), (5, "operation_id")):
            with self.subTest(column=column):
                evidence = list(bank_evidence())
                evidence[index][0][column] = bytes(40) if index == 3 else 101 if index == 4 else operation(999)
                with self.assertRaises(canonical.EvidenceError):
                    canonical.qualify_bank_root(*evidence)

    def test_missing_or_extra_indexed_receipts_refuse_amount(self):
        for index in (3, 4, 5):
            for extra in (False, True):
                evidence = list(bank_evidence())
                if extra:
                    evidence[index].append(copy.deepcopy(evidence[index][0]))
                else:
                    evidence[index].pop()
                with self.assertRaises(canonical.EvidenceError):
                    canonical.qualify_bank_root(*evidence)

    def test_plan_truncation_reserved_bytes_and_capacity_fail_before_rows(self):
        raw = bank_evidence()[0]["canonical_plan"]
        for size in range(len(raw)):
            with self.assertRaises(canonical.EvidenceError):
                canonical.decode_plan(raw[:size])
        for offset in (6, 7, 73, 74, 75, 98, 99, 101, 102, 103, 240, 255, 292, 293, 294, 295):
            broken = bytearray(raw)
            broken[offset] = 1
            with self.assertRaises(canonical.EvidenceError):
                canonical.decode_plan(bytes(broken))
        for offset in range(216, 240, 4):
            broken = bytearray(raw)
            struct.pack_into("<I", broken, offset, 0xffffffff)
            with self.assertRaises(canonical.EvidenceError):
                canonical.decode_plan(bytes(broken))

    def test_plan_coin_corruption_does_not_pass_digest_only_check(self):
        raw = bytearray(bank_evidence()[0]["canonical_plan"])
        for offset in (256 + 40, 256 + 72, 256 + 104, 256 + 360 + 8, 256 + 360 + 40):
            broken = bytearray(raw)
            broken[offset] ^= 1
            with self.assertRaises(canonical.EvidenceError):
                canonical.decode_plan(bytes(broken))

    def test_intent_mutation_and_digest_disagreement_refuse_amount(self):
        for offset in (6, 8, 12, 24, 27, 30, 32, 48, 64, 80, 96, 104, 108, 112, 192, 224, 256):
            evidence = list(bank_evidence())
            raw = bytearray(evidence[0]["canonical_intent"])
            raw[offset] ^= 1
            evidence[0]["canonical_intent"] = bytes(raw)
            with self.subTest(offset=offset), self.assertRaises(canonical.EvidenceError):
                canonical.qualify_bank_root(*evidence)

    def test_conflicts_preserve_both_payload_digests_and_invalidate_shared_source(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        original = event.references[0]
        different = replace(original, payload_digest=bytes([99]) * 32)
        conflict = replace(event, references=(different,))
        affected = replace(event, identity=replace(event.identity, unit=canonical.Unit.EPIC),
                           references=(original,))
        rows = canonical.reconcile_events([event, conflict, affected])
        self.assertEqual(len(rows), 2)
        self.assertTrue(all(row.disposition == canonical.Disposition.CONFLICT and row.amount is None for row in rows))
        retained = next(row for row in rows if row.identity.unit == canonical.Unit.CURRENCY)
        self.assertIn(original, retained.references)
        self.assertIn(different, retained.references)
        self.assertEqual(canonical.earned_totals([event, conflict]), {})

    def test_equal_amounts_or_different_units_do_not_coalesce_events(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        other = replace(event, identity=replace(event.identity, operation_id=operation(112)))
        epic = replace(event, identity=replace(event.identity, unit=canonical.Unit.EPIC), amount=3)
        self.assertEqual(canonical.earned_totals([event, other, epic]),
                         {canonical.Unit.CURRENCY: 200, canonical.Unit.EPIC: 3})

    def test_unknown_context_retains_reference_without_adding_a_second_amount(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        ref = canonical.source_reference("combat_outcome_participant", operation(120),
                                         dict(ledger_operation_id=event.identity.operation_id, pid=7))
        context_row = replace(event, disposition=canonical.Disposition.UNKNOWN, amount=None,
                              authority="unavailable", references=(ref,))
        rows = canonical.reconcile_events([context_row, event])
        self.assertEqual(len(rows), 1)
        self.assertIn(ref, rows[0].references)
        self.assertEqual(canonical.earned_totals(rows), {canonical.Unit.CURRENCY: 100})

    def test_conflicting_authoritative_classification_never_selects_a_winner(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        for change in (dict(amount=101), dict(disposition=canonical.Disposition.OPENING),
                       dict(lineage=operation(9)), dict(wallet_lifetime=72)):
            rows = canonical.reconcile_events([event, replace(event, **change)])
            self.assertEqual(rows[0].disposition, canonical.Disposition.CONFLICT)
            self.assertIsNone(rows[0].amount)

    def test_observed_xp_cannot_gain_committed_earned_authority(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        xp = replace(event, identity=replace(event.identity, unit=canonical.Unit.OBSERVED_XP))
        with self.assertRaises(canonical.EvidenceError):
            canonical.reconcile_events([xp])

    def test_event_and_reference_bounds_are_enforced(self):
        event = canonical.qualify_bank_root(*bank_evidence())
        with self.assertRaises(canonical.EvidenceError):
            canonical.reconcile_events([event] * (canonical.MAX_PAGE_SIZE + 1))
        with self.assertRaises(canonical.EvidenceError):
            replace(event, references=event.references * canonical.MAX_PAGE_SIZE).validate()

    def test_source_payload_bounds_and_types_precede_digest_acceptance(self):
        for payload in ({"raw": bytes(8193)}, {"raw": bytes(4096)}, {"raw": "x" * 8192},
                        {"raw": True}, {"raw": 1.0}, {"raw": 1 << 64}, {"raw": -(1 << 63) - 1}):
            with self.subTest(payload_type=type(payload["raw"]).__name__), self.assertRaises(canonical.EvidenceError):
                canonical.source_reference("currency_ledger", operation(1), payload)
        event = canonical.qualify_bank_root(*bank_evidence())
        expanded = replace(event, references=event.references * 100)
        with self.assertRaises(canonical.EvidenceError):
            canonical.reconcile_events([expanded] * 19)


def operation(number: int) -> bytes:
    return number.to_bytes(16, "big")


def ledger(*, source_kind: SourceKind, op: int, pid: int, net: int,
           gross: int | None = None, transfer: int | None = None,
           is_transfer: bool = False) -> SourceObservation:
    return SourceObservation(
        source_kind=source_kind,
        operation_id=operation(op),
        entry_index=0,
        participant_pid=pid,
        source_table=("currency_ledger" if source_kind == SourceKind.CURRENCY_LEDGER
                      else "epic_ledger"),
        created_at_usec=1_000_000 + op,
        authority_kind=AuthorityKind.COMMITTED_LEDGER,
        reward_kind=(RewardKind.CURRENCY if source_kind == SourceKind.CURRENCY_LEDGER
                     else RewardKind.EPIC),
        gross_amount=max(net, 0) if gross is None else gross,
        net_amount=net,
        transfer_amount=transfer,
        is_transfer=is_transfer,
        is_creation=net > 0 and not is_transfer,
        reason_type=4,
        reason_id=9,
        source_site=3,
    )


def context(*, source_kind: SourceKind, op: int, pid: int, entry: int = 0,
            linked_ledger: bytes | None = None, complete: bool = True) -> SourceObservation:
    return SourceObservation(
        source_kind=source_kind,
        operation_id=operation(op),
        entry_index=entry,
        participant_pid=pid,
        source_table="combat_outcome_participant",
        created_at_usec=1_000_000 + op,
        authority_kind=AuthorityKind.OUTCOME_CONTEXT,
        ledger_operation_id=linked_ledger,
        context_complete=complete,
    )


class RewardProjectionPureTest(unittest.TestCase):
    def test_currency_denominations_use_canonical_copper_weights(self) -> None:
        self.assertEqual(currency_value((3, 2, 1, 1)), 1123)

    def test_currency_creation_transfer_and_sink_have_distinct_semantics(self) -> None:
        batch = project_observations([
            ledger(source_kind=SourceKind.CURRENCY_LEDGER, op=1, pid=10, net=123),
            ledger(source_kind=SourceKind.CURRENCY_LEDGER, op=2, pid=10, net=0,
                   gross=0, transfer=50, is_transfer=True),
            ledger(source_kind=SourceKind.CURRENCY_LEDGER, op=3, pid=10, net=-7, gross=0),
        ])
        self.assertEqual(batch.gross_total, 123)
        self.assertEqual(batch.net_total, 116)
        self.assertEqual(batch.transfer_total, 50)
        transfer = next(record for record in batch.records if record.operation_id == operation(2))
        self.assertEqual(transfer.status, ProjectionStatus.PROJECTED)
        self.assertTrue(transfer.quality_flags & int(ProjectionQuality.TRANSFER_NOT_CREATION))
        self.assertFalse(transfer.is_creation)

    def test_frag_ledger_is_an_independent_committed_authority(self) -> None:
        observation = SourceObservation(
            source_kind=SourceKind.COMBAT_FRAG_LEDGER,
            operation_id=operation(4_001),
            entry_index=2,
            participant_pid=77,
            source_table="combat_frag_ledger",
            created_at_usec=4_001_000_000,
            authority_kind=AuthorityKind.COMMITTED_LEDGER,
            reward_kind=RewardKind.FRAGS,
            gross_amount=3,
            net_amount=3,
            is_creation=True,
        )
        batch = project_observations([observation])
        self.assertEqual(batch.authoritative_count, 1)
        self.assertEqual(batch.gross_total, 3)
        self.assertEqual(batch.net_total, 3)
        self.assertEqual(batch.records[0].reward_kind, RewardKind.FRAGS)

    def test_exact_replay_is_one_identity_and_conflict_fails_closed(self) -> None:
        first = ledger(source_kind=SourceKind.EPIC_LEDGER, op=11, pid=7, net=50)
        replay = ledger(source_kind=SourceKind.EPIC_LEDGER, op=11, pid=7, net=50)
        self.assertEqual(project_observations([first, replay]).gross_total, 50)
        replay_batch = project_observations([first, replay])
        self.assertEqual(len(replay_batch.records), 1)
        self.assertEqual(replay_batch.duplicate_count, 1)

        conflict = ledger(source_kind=SourceKind.EPIC_LEDGER, op=11, pid=7, net=51)
        conflict_batch = project_observations([first, conflict])
        self.assertEqual(conflict_batch.conflict_count, 1)
        self.assertEqual(conflict_batch.records[0].status, ProjectionStatus.CONFLICT)
        self.assertEqual(conflict_batch.gross_total, 0)
        self.assertFalse(conflict_batch.records[0].contributes_amount)

    def test_parent_and_participant_context_rows_do_not_collide_or_add_amount(self) -> None:
        op = operation(21)
        rows = [
            context(source_kind=SourceKind.PVP_OUTCOME, op=21, pid=99),
            context(source_kind=SourceKind.PVP_OUTCOME_PARTICIPANT, op=21, pid=99),
            ledger(source_kind=SourceKind.EPIC_LEDGER, op=22, pid=99, net=25),
            context(source_kind=SourceKind.PVP_OUTCOME_PARTICIPANT, op=21, pid=99,
                    entry=1, linked_ledger=operation(22)),
        ]
        batch = project_observations(rows)
        self.assertEqual(len(batch.records), 4)
        self.assertEqual(batch.context_count, 3)
        self.assertEqual(batch.gross_total, 25)
        linked = next(record for record in batch.records if record.entry_index == 1)
        self.assertEqual(linked.status, ProjectionStatus.CONTEXT_ONLY)
        self.assertEqual(linked.economic_operation_id, operation(22))
        self.assertNotEqual((int(SourceKind.PVP_OUTCOME), op, 0, 99),
                            (int(SourceKind.PVP_OUTCOME_PARTICIPANT), op, 0, 99))

    def test_context_linkage_is_retained_without_contributing_amount(self) -> None:
        row = context(source_kind=SourceKind.PVP_OUTCOME, op=23, pid=99,
                      linked_ledger=operation(24))
        record = project_observations([row]).records[0]
        self.assertEqual(record.status, ProjectionStatus.CONTEXT_ONLY)
        self.assertEqual(record.economic_operation_id, operation(24))
        self.assertEqual(record.gross_amount, None)
        self.assertEqual(record.net_amount, None)

    def test_incomplete_context_is_unknown_even_when_ledger_link_is_present(self) -> None:
        row = context(source_kind=SourceKind.ZONE_OUTCOME, op=31, pid=44,
                      linked_ledger=operation(32), complete=False)
        record = project_observations([row]).records[0]
        self.assertEqual(record.status, ProjectionStatus.UNKNOWN_CONTEXT)
        self.assertTrue(record.quality_flags & int(ProjectionQuality.UNKNOWN_CONTEXT))

    def test_reconciliation_is_restartable_and_does_not_claim_overlap_perfection(self) -> None:
        state = ProjectionState(SourceKind.EPIC_LEDGER).start_reconciliation(
            cycle_id=1, retention_floor_usec=100, high_water_usec=200
        )
        self.assertEqual(state.reconcile_cursor.operation_id, ZERO_OPERATION_ID)
        state = state.acknowledge_page(
            cursor=SourceCursor(150, operation(90)), rows_seen=1,
            reconciliation=True, page_complete=False
        )
        self.assertTrue(state.provisional)
        self.assertEqual(state.backlog_rows, 1)
        state = state.acknowledge_page(
            cursor=SourceCursor(200, operation(91)), rows_seen=1,
            reconciliation=True, page_complete=True
        )
        self.assertFalse(state.provisional)
        self.assertEqual(state.acknowledged_through_usec, 200)
        self.assertEqual(state.backlog_rows, 0)

        later = state.start_reconciliation(
            cycle_id=2, retention_floor_usec=100, high_water_usec=300
        )
        self.assertEqual(later.reconcile_cursor.created_at_usec, 100)
        self.assertEqual(later.reconcile_cursor.operation_id, ZERO_OPERATION_ID)
        self.assertTrue(later.provisional)

    def test_retention_warning_prevents_silent_pruning(self) -> None:
        state = ProjectionState(
            SourceKind.CURRENCY_LEDGER,
            acknowledged_through_usec=100,
            provisional=True,
            backlog_rows=2,
            quality_flags=int(ProjectionQuality.INCOMPLETE_SOURCE_RANGE),
        )
        warnings = retention_warnings(state, now_usec=1_000, retention_horizon_usec=200)
        self.assertEqual(
            warnings,
            (
                "reconciliation_acknowledgement_behind_retention_floor",
                "projection_cycle_provisional",
                "reconciliation_backlog_nonzero",
                "source_range_incomplete",
            ),
        )


class RewardProjectionQueryTest(unittest.TestCase):
    def test_source_keyset_uses_physical_columns_not_select_aliases(self) -> None:
        expected_keys = {
            "currency_ledger": ("L.CREATED_AT", "L.OPERATION_ID", "0+0", "L.PID"),
            "epic_ledger": ("L.CREATED_AT", "L.OPERATION_ID", "0+0", "L.PID"),
            "combat_frag_ledger": (
                "L.CREATED_AT", "L.OPERATION_ID", "L.PARTICIPANT_INDEX", "L.PID"
            ),
            "zone_touch_outcome": ("O.CREATED_AT", "O.OPERATION_ID", "0+0", "O.TOUCHER_PID"),
            "zone_touch_outcome_participant": (
                "O.CREATED_AT", "P.OPERATION_ID", "P.PARTICIPANT_INDEX", "P.PID"
            ),
            "combat_outcome": ("O.CREATED_AT", "O.OPERATION_ID", "0+0", "O.VICTIM_PID"),
            "combat_outcome_participant": (
                "O.CREATED_AT", "P.OPERATION_ID", "P.PARTICIPANT_INDEX", "P.PID"
            ),
            "boon_reward_outcome": ("O.CREATED_AT", "O.OPERATION_ID", "0+0", "O.PID"),
            "boon_reward_outcome_entry": (
                "O.CREATED_AT", "E.OPERATION_ID", "E.ENTRY_INDEX", "O.PID"
            ),
        }
        for adapter in SOURCE_ADAPTERS:
            if not adapter.supported:
                continue
            query, _ = build_source_page_query(
                adapter,
                cursor=SourceCursor(1_000_000, operation(99)),
                high_water_usec=2_000_000,
                page_size=37,
            )
            normalized = " ".join(query.upper().split())
            self.assertEqual(
                (
                    adapter.key_created_sql.upper(),
                    adapter.key_operation_sql.upper(),
                    adapter.key_entry_sql.upper(),
                    adapter.key_participant_sql.upper(),
                ),
                expected_keys[adapter.name],
            )
            self.assertIn(adapter.key_created_sql.upper(), normalized)
            self.assertIn(adapter.key_operation_sql.upper(), normalized)
            self.assertIn(adapter.key_entry_sql.upper(), normalized)
            self.assertIn(adapter.key_participant_sql.upper(), normalized)

    def test_all_supported_adapters_are_explicit_bounded_read_only_queries(self) -> None:
        for adapter in SOURCE_ADAPTERS:
            if not adapter.supported:
                continue
            query, params = build_source_page_query(
                adapter,
                cursor=SourceCursor(1_000_000, ZERO_OPERATION_ID),
                high_water_usec=2_000_000,
                page_size=37,
            )
            normalized = " ".join(query.upper().split())
            self.assertNotIn("SELECT *", normalized)
            self.assertNotIn("FOR UPDATE", normalized)
            self.assertNotIn(" OFFSET ", normalized)
            self.assertIn("LIMIT %S", normalized)
            self.assertIn("ORDER BY", normalized)
            self.assertEqual(params[-1], 37)

    def test_legacy_quest_sources_never_fabricate_operation_ids(self) -> None:
        legacy = [adapter for adapter in SOURCE_ADAPTERS if not adapter.supported]
        self.assertEqual({adapter.source_kind for adapter in legacy}, {SourceKind.QUEST_OUTCOME})
        for adapter in legacy:
            self.assertIn("no committed operation_id", adapter.description)


class _FakeCursor:
    def __init__(self, rows: list[dict[str, object]]) -> None:
        self.rows = rows
        self.description = ("column",)
        self.statements: list[tuple[str, tuple[object, ...]]] = []

    def execute(self, statement: str, parameters: tuple[object, ...]) -> None:
        self.statements.append((statement, parameters))

    def fetchall(self):
        rows, self.rows = self.rows, []
        return rows

    def close(self) -> None:
        pass


class _FakeConnection:
    def __init__(self, rows: list[dict[str, object]]) -> None:
        self.cursor_instance = _FakeCursor(rows)
        self.commits = 0
        self.rollbacks = 0

    def cursor(self):
        return self.cursor_instance

    def begin(self):
        pass

    def commit(self):
        self.commits += 1

    def rollback(self):
        self.rollbacks += 1

    def close(self):
        pass


class _AmbiguousCommitConnection(_FakeConnection):
    def commit(self):
        self.commits += 1
        raise OSError("commit acknowledgement lost")


class _FakeFactory:
    def __init__(self, connection: _FakeConnection) -> None:
        self.connection = connection
        self.connects = 0

    def connect(self):
        self.connects += 1
        return self.connection

    def close(self, connection=None):
        connection.close()


class RewardProjectionStoreTest(unittest.TestCase):
    def test_ambiguous_commit_discards_connection_for_state_reread(self) -> None:
        connection = _AmbiguousCommitConnection([
            {
                "operation_id": operation(40), "participant_pid": 5,
                "created_at": datetime.fromtimestamp(2, timezone.utc),
                "net_amount": 17, "gross_amount": 17, "transfer_amount": 0,
                "is_transfer": 0, "is_creation": 1,
                "reason_type": 1, "reason_id": 2, "source_site": 3,
            },
        ])
        factory = _FakeFactory(connection)
        store = RewardProjectionStore(factory, page_size=10)
        with self.assertRaises(AmbiguousCommit):
            store.process_page(
                SOURCE_ADAPTERS[0], ProjectionState(SourceKind.CURRENCY_LEDGER),
                high_water_usec=3_000_000, reconciliation=False,
            )
        self.assertEqual(connection.commits, 1)
        self.assertEqual(store.connection_count, 0)

    def test_store_uses_one_connection_and_one_bulk_projection_write(self) -> None:
        connection = _FakeConnection([
            {
                "operation_id": operation(41), "participant_pid": 5,
                "created_at": datetime.fromtimestamp(2, timezone.utc),
                "net_amount": 17, "gross_amount": 17, "transfer_amount": 0,
                "is_transfer": 0, "is_creation": 1,
                "reason_type": 1, "reason_id": 2, "source_site": 3,
            },
        ])
        factory = _FakeFactory(connection)
        store = RewardProjectionStore(factory, page_size=10)
        state = ProjectionState(SourceKind.CURRENCY_LEDGER, cycle_id=4)
        batch, next_state = store.process_page(
            SOURCE_ADAPTERS[0], state, high_water_usec=3_000_000,
            reconciliation=False, page_complete=True,
        )
        self.assertEqual(len(batch.records), 1)
        self.assertEqual(factory.connects, 1)
        self.assertEqual(store.connection_count, 1)
        self.assertEqual(connection.commits, 1)
        self.assertTrue(next_state.provisional)  # fast-window pages remain provisional
        self.assertGreater(next_state.fast_started_at_usec, 0)
        self.assertGreater(next_state.fast_completed_at_usec, 0)
        statements = connection.cursor_instance.statements
        self.assertEqual(len(statements), 3)
        self.assertEqual(sum("INSERT INTO telemetry_reward_projection (" in sql for sql, _ in statements), 1)
        self.assertTrue(all("FOR UPDATE" not in sql.upper() for sql, _ in statements))
        store.close()


class _SnapshotCursor:
    def __init__(self, connection):
        self.connection = connection
        self.description = None
        self.rows = []

    def execute(self, statement, parameters):
        owner = self.connection
        owner.statements.append((statement, parameters))
        self.description = ("column",) if statement.startswith("SELECT") else None
        if "information_schema.tables" in statement:
            self.rows = [{"table_name": table, "engine": "MyISAM" if table == owner.bad_engine else "InnoDB"}
                         for table in SOURCE_TABLES]
        elif "AS captured_utc_usec" in statement:
            self.rows = [{"captured_utc_usec": 1_000_000}]
        elif "AS plan_bytes" in statement:
            candidates = owner.tables["economic_accounting_operation"]
            pass_upper = None
            if "operation_id>=" in statement:
                lower = parameters[0] if "SELECT MAX(operation_id)" in statement else parameters[1]
                prefix = lower[0]
                offset = (2 if prefix < 255 else 1) if "SELECT MAX(operation_id)" in statement else 1
                outer = parameters[offset:]
                lower, after = outer[:2]
                upper = outer[2] if prefix < 255 else None
                through = outer[-2] if "operation_id<=%s" in statement else None
                eligible = [row for row in candidates if row["operation_id"] >= lower and
                    (upper is None or row["operation_id"] < upper) and
                    row["outcome"] == 1 and row["result_code"] == 0 and row["writer_id"] in (1, 2, 3, 4) and
                    row["reason"] in (1, 5, 8)]
                pass_upper = through if through is not None else max((row["operation_id"] for row in eligible), default=None)
                candidates = sorted((row for row in eligible if row["operation_id"] > after and
                    (through is None or row["operation_id"] <= through)), key=lambda row: row["operation_id"])[:parameters[-1]]
            else:
                candidates = [row for row in candidates if row["operation_id"] in parameters]
            self.rows = [dict(operation_id=row["operation_id"], writer_id=row["writer_id"], reason=row["reason"],
                         **{name: row[name] for name in canonical.COUNTS},
                         plan_bytes=len(row["canonical_plan"]), intent_bytes=len(row["canonical_intent"]))
                         for row in candidates]
            if "AS pass_upper" in statement:
                for row in self.rows:
                    row["pass_upper"] = pass_upper
        elif "UNION ALL" in statement:
            self.rows = owner.unexpected
        elif statement.startswith("SELECT"):
            table = statement.split(" FROM ", 1)[1].split()[0]
            self.rows = [row for row in owner.tables[table] if row["operation_id"] in parameters]
        else:
            self.rows = []

    def fetchall(self):
        return self.rows

    def close(self):
        pass


class _SnapshotConnection:
    def __init__(self, evidence):
        self.statements, self.rollbacks, self.closed = [], 0, False
        self.bad_engine = None
        self.unexpected = []
        self.tables = {name: [] for name in SOURCE_TABLES}
        for root, ledger_row, inbox, effects, postings, claims in evidence:
            self.tables["economic_accounting_operation"].append(root)
            self.tables["currency_ledger"].append(ledger_row)
            self.tables["critical_operation_inbox"].append(inbox)
            self.tables["economic_accounting_account_effect"].extend(effects)
            self.tables["economic_accounting_coin_posting"].extend(postings)
            self.tables["economic_accounting_source_claim"].extend(claims)

    def cursor(self):
        return _SnapshotCursor(self)

    def rollback(self):
        self.rollbacks += 1

    def close(self):
        self.closed = True


class CanonicalRewardSnapshotTest(unittest.TestCase):
    def snapshot(self, *, evidence=None, **kwargs):
        evidence = evidence or [bank_evidence()]
        connection = _SnapshotConnection(evidence)
        factory = _FakeFactory(connection)
        return CanonicalRewardSnapshot(factory, **kwargs), connection, factory

    def test_owning_read_only_snapshot_retains_exact_sources_in_eight_query_pages(self):
        store, connection, factory = self.snapshot()
        op = connection.tables["currency_ledger"][0]["operation_id"]
        result = store.capture_bank_operations([op])
        self.assertEqual(result.selected_operations, (op,))
        self.assertEqual(result.query_counts, (8,))
        self.assertEqual(len(result.sources), 9)
        self.assertEqual(result.coverage, "selected_native_bank_operations")
        self.assertTrue(result.future_commits_provisional)
        self.assertEqual(result.events[0].amount, 100)
        self.assertEqual(factory.connects, 1)
        self.assertEqual(connection.rollbacks, 2)
        self.assertTrue(connection.closed)
        self.assertEqual(store.connection_count, 0)
        self.assertIn("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ", connection.statements[0][0])
        self.assertIn("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY", connection.statements[1][0])
        for sql, parameters in connection.statements:
            self.assertNotIn("SELECT *", sql.upper())
            self.assertNotIn("FOR UPDATE", sql.upper())
            self.assertNotIn("OFFSET", sql.upper())
            self.assertFalse(sql.startswith(("INSERT", "UPDATE", "DELETE")))
        for row in result.sources:
            self.assertEqual(hashlib.sha256(row.payload).digest(), row.reference.payload_digest)

    def test_distinct_selected_pages_keep_one_snapshot_and_no_cross_cut_cursor(self):
        evidence = [bank_evidence(), bank_evidence(disposition=canonical.Disposition.OPENING)]
        store, connection, factory = self.snapshot(evidence=evidence, page_size=1)
        operations = [row[0]["operation_id"] for row in evidence]
        result = store.capture_bank_operations(operations)
        self.assertEqual(result.query_counts, (8, 8))
        self.assertEqual(len(result.events), 2)
        self.assertEqual(sum("START TRANSACTION" in sql for sql, _ in connection.statements), 1)
        self.assertEqual(factory.connects, 1)
        self.assertEqual(canonical.earned_totals(result.events), {canonical.Unit.CURRENCY: 100})

    def test_nontransactional_source_refuses_before_native_receipt_reads(self):
        store, connection, _ = self.snapshot()
        connection.bad_engine = "currency_ledger"
        with self.assertRaises(canonical.EvidenceError):
            store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        self.assertEqual(len(connection.statements), 3)
        self.assertTrue(connection.closed)

    def test_unexpected_child_missing_source_or_blob_shape_discards_the_cut(self):
        for failure in ("child", "missing", "oversized"):
            store, connection, _ = self.snapshot()
            op = connection.tables["currency_ledger"][0]["operation_id"]
            if failure == "child":
                connection.unexpected = [{"operation_id": op}]
            elif failure == "missing":
                connection.tables["currency_ledger"] = []
            else:
                connection.tables["economic_accounting_operation"][0]["canonical_plan"] += bytes(1)
            with self.subTest(failure=failure), self.assertRaises(canonical.EvidenceError):
                store.capture_bank_operations([op])
            self.assertTrue(connection.closed)
            self.assertEqual(store._sources, {})

    def test_capture_byte_or_deadline_refusal_cannot_return_a_partial_cut(self):
        store, connection, _ = self.snapshot(byte_limit=100)
        with self.assertRaises(ProjectionBoundsExceeded):
            store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        self.assertTrue(connection.closed)
        self.assertEqual(store._bytes, 0)
        counter = iter((0, 0, 11))
        store, connection, _ = self.snapshot(clock=lambda: next(counter))
        with self.assertRaises(ProjectionBoundsExceeded):
            store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        self.assertTrue(connection.closed)

    def test_duplicate_operation_is_not_a_native_source_completeness_claim(self):
        store, connection, factory = self.snapshot()
        op = connection.tables["currency_ledger"][0]["operation_id"]
        with self.assertRaises(canonical.EvidenceError):
            store.capture_bank_operations([op, op])
        self.assertEqual(factory.connects, 0)

    def test_empty_selected_scope_preserves_provisional_future_coverage(self):
        store, connection, _ = self.snapshot()
        result = store.capture_bank_operations([])
        self.assertEqual(result.events, ())
        self.assertEqual(result.selected_operations, ())
        self.assertEqual(result.sources, ())
        self.assertTrue(result.future_commits_provisional)
        self.assertEqual(result.coverage, "selected_native_bank_operations")

    def test_retained_source_replay_reconstructs_authority_without_live_rows(self):
        store, connection, _ = self.snapshot()
        cut = store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        connection.tables.clear()
        restored = replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations,
                                            cut.sources, cut.source_digest)
        self.assertEqual(restored.events, cut.events)
        self.assertEqual(restored.reserved_bytes, cut.reserved_bytes)
        self.assertTrue(restored.future_commits_provisional)

    def test_retained_source_refuses_missing_reordered_changed_or_foreign_evidence(self):
        store, connection, _ = self.snapshot()
        cut = store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        changed = replace(cut.sources[0], payload=cut.sources[0].payload + b" ")
        foreign = replace(cut.sources[0], reference=replace(cut.sources[0].reference, key=operation(999)))
        for sources in (cut.sources[1:], tuple(reversed(cut.sources)),
                        (changed, *cut.sources[1:]), (foreign, *cut.sources[1:])):
            with self.subTest(first=sources[0].reference.table), self.assertRaises(canonical.EvidenceError):
                replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, sources,
                                         source_cut_digest(cut.captured_utc_usec, cut.selected_operations, sources))

    def test_retained_source_digest_alone_cannot_grant_native_earned_authority(self):
        store, connection, _ = self.snapshot()
        cut = store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        sources = list(cut.sources)
        position = next(index for index, source in enumerate(sources) if source.reference.table == "currency_ledger")
        source = sources[position]
        row = canonical.decode_source_payload(source.payload)
        row["wallet_delta_copper"] += 1
        sources[position] = replace(source, payload=canonical.encode_source_payload(row),
            reference=canonical.source_reference(source.reference.table, source.reference.key, row))
        sources = tuple(sources)
        with self.assertRaises(canonical.EvidenceError):
            replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, sources,
                                     source_cut_digest(cut.captured_utc_usec, cut.selected_operations, sources))

    def test_retained_source_rejects_different_cut_clock_and_bytes_budget(self):
        store, connection, _ = self.snapshot()
        cut = store.capture_bank_operations([connection.tables["currency_ledger"][0]["operation_id"]])
        with self.assertRaises(canonical.EvidenceError):
            replay_retained_bank_cut(cut.captured_utc_usec + 1, cut.selected_operations, cut.sources, cut.source_digest)
        with self.assertRaises(ProjectionBoundsExceeded):
            replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources, cut.source_digest,
                                     byte_limit=cut.reserved_bytes - 1)

    def test_retained_payload_decoder_accepts_only_exact_bounded_canonical_bytes(self):
        payload = {"operation_id": operation(1), "count": 1, "absent": None}
        self.assertEqual(canonical.decode_source_payload(canonical.encode_source_payload(payload)), payload)
        for value in (b'{"count":1,"count":1}', b'{"count": 1}', b'{"count":true}',
                      b'{"blob":{"bytes":"AA"}}', b'{"blob":{"bytes":"00","extra":1}}',
                      b'[]', b'{"count":NaN}', b'x' * 8193):
            with self.subTest(value=value[:80]), self.assertRaises(canonical.EvidenceError):
                canonical.decode_source_payload(value)

    def test_partition_discovery_and_native_receipts_share_one_eight_query_cut(self):
        store, connection, factory = self.snapshot()
        page = store.capture_bank_partition(0)
        self.assertEqual(page.cut.selected_operations, (operation(111),))
        self.assertEqual(page.cut.query_counts, (8,))
        self.assertTrue(page.wrapped)
        self.assertTrue(page.future_commits_provisional)
        self.assertEqual(page.cursor_after, canonical.ZERO_ID)
        self.assertEqual(factory.connects, 1)
        self.assertEqual(sum("START TRANSACTION" in sql for sql, _ in connection.statements), 1)
        discovery = next(sql for sql, _ in connection.statements if "operation_id>=" in sql)
        self.assertIn("outcome=1 AND result_code=0", discovery)
        self.assertNotIn("OFFSET", discovery)

    def test_partition_wrap_revisits_lower_sorting_later_observations(self):
        store, _, _ = self.snapshot(evidence=[bank_evidence(), bank_evidence(operation_number=112)], page_size=1)
        first = store.capture_bank_partition(0)
        self.assertFalse(first.wrapped)
        self.assertEqual(first.cursor_after, operation(111))
        self.assertEqual(first.pass_upper, operation(112))
        # This is a delayed boundary fixture; actual native commit timing still
        # requires the maintained disposable producer journey.
        evidence = [bank_evidence(), bank_evidence(operation_number=110)]
        store, _, _ = self.snapshot(evidence=evidence, page_size=1)
        tail = store.capture_bank_partition(0, first.cursor_after)
        self.assertTrue(tail.wrapped)
        self.assertEqual(tail.cut.query_counts, (1,))
        self.assertEqual(tail.cut.events, ())
        store, _, _ = self.snapshot(evidence=evidence, page_size=1)
        revisited = store.capture_bank_partition(0, tail.cursor_after)
        self.assertEqual(revisited.cut.selected_operations, (operation(110),))
        self.assertTrue(revisited.future_commits_provisional)

    def test_partition_cursor_and_budget_refusals_precede_connection(self):
        for kwargs, bucket, after in (({}, 1, operation(111)), (dict(byte_limit=4096), 0, canonical.ZERO_ID)):
            store, _, factory = self.snapshot(**kwargs)
            with self.subTest(kwargs=kwargs), self.assertRaises((canonical.EvidenceError, ProjectionBoundsExceeded)):
                store.capture_bank_partition(bucket, after)
            self.assertEqual(factory.connects, 0)

    def test_last_partition_has_bounded_lower_range_without_overflow(self):
        op = (255 << 120) + 111
        store, connection, _ = self.snapshot(evidence=[bank_evidence(operation_number=op)])
        page = store.capture_bank_partition(255)
        self.assertEqual(page.cut.selected_operations, (operation(op),))
        self.assertEqual(page.cut.query_counts, (8,))
        discovery = next(sql for sql, _ in connection.statements if "operation_id>=" in sql)
        self.assertNotIn("operation_id<%s", discovery)

    def test_bounded_refused_receipt_can_retain_exact_quarantine_without_authority(self):
        store, connection, _ = self.snapshot()
        op = connection.tables["currency_ledger"][0]["operation_id"]
        connection.tables["economic_accounting_coin_posting"][0]["copper_value"] += 1
        cut = store.capture_bank_operations([op], preserve_refusals=True)
        self.assertEqual(len(cut.sources), 9)
        self.assertEqual(cut.events[0].disposition, canonical.Disposition.UNKNOWN)
        self.assertIsNone(cut.events[0].amount)
        self.assertEqual(cut.events[0].identity.participant_pid, 0)
        with self.assertRaises(canonical.EvidenceError):
            replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources, cut.source_digest)
        restored = replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources,
                                            cut.source_digest, strict=False)
        self.assertEqual(restored.events, cut.events)
        self.assertEqual(restored.sources, cut.sources)

    def test_quarantine_physical_disagreement_invalidates_prior_earned_event(self):
        store, connection, _ = self.snapshot()
        op = connection.tables["currency_ledger"][0]["operation_id"]
        original = store.capture_bank_operations([op])
        store, connection, _ = self.snapshot()
        connection.tables["economic_accounting_coin_posting"][0]["copper_value"] += 1
        refused = store.capture_bank_operations([op], preserve_refusals=True)
        reconciled = canonical.reconcile_events((*original.events, *refused.events))
        self.assertEqual(canonical.earned_totals(reconciled), {})
        self.assertTrue(all(event.amount is None for event in reconciled))
        posting_digests = {ref.payload_digest for event in reconciled for ref in event.references
                           if ref.table == "economic_accounting_coin_posting" and ref.key == op + bytes(2)}
        self.assertEqual(len(posting_digests), 2)

    def test_missing_compatibility_receipt_is_unknown_in_quarantine_not_zero_earned(self):
        store, connection, _ = self.snapshot()
        op = connection.tables["currency_ledger"][0]["operation_id"]
        connection.tables["currency_ledger"] = []
        cut = store.capture_bank_operations([op], preserve_refusals=True)
        self.assertEqual(len(cut.sources), 8)
        self.assertEqual(cut.events[0].disposition, canonical.Disposition.UNKNOWN)
        restored = replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources,
                                            cut.source_digest, strict=False)
        self.assertEqual(restored.events, cut.events)
        self.assertTrue(restored.future_commits_provisional)


if __name__ == "__main__":
    unittest.main()
