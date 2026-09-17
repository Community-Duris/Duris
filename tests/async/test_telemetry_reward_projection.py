#!/usr/bin/env python3
"""Pure and SQL-boundary tests for telemetry reward projection (#270)."""

from __future__ import annotations

from datetime import datetime, timezone
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


if __name__ == "__main__":
    unittest.main()
