#!/usr/bin/env python3
"""Guarded disposable-MySQL acceptance tests for reward projection #270.

This suite is intentionally excluded from default discovery because it mutates a
database.  It requires an explicit task-owned ``duris_270_*test`` database and
the opt-in ``TELEMETRY_REWARD_PROJECTION_DISPOSABLE=1`` flag.  It inserts only
synthetic critical-operation/epic-ledger rows identified by this process and
clears only the two projection tables plus those exact synthetic source rows.
It never reads the repository's game ``DB_*`` environment and never runs a
production migration or source-table mutation.
"""
from __future__ import annotations

from datetime import datetime, timedelta, timezone
from dataclasses import replace
import os
import re
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from scripts.telemetry.reward_projection import (  # noqa: E402
    RewardProjectionStore,
    SOURCE_ADAPTER_BY_NAME,
    build_reward_report_query,
    build_source_page_query,
)
from scripts.telemetry.reward_projection_definitions import (  # noqa: E402
    AuthorityKind,
    ProjectionState,
    ProjectionStatus,
    RewardKind,
    SourceCursor,
    SourceKind,
    SourceObservation,
    ZERO_OPERATION_ID,
    project_observations,
)
from scripts.telemetry.db_access import (  # noqa: E402
    ConnectionSettings,
    PyMySQLConnectionFactory,
)


class RewardProjectionMySQLTest(unittest.TestCase):
    """Run real source scans and durable replay semantics on one private DB."""

    @classmethod
    def setUpClass(cls) -> None:
        if os.environ.get("TELEMETRY_REWARD_PROJECTION_DISPOSABLE") != "1":
            raise RuntimeError(
                "explicit TELEMETRY_REWARD_PROJECTION_DISPOSABLE=1 required"
            )
        cls.database = os.environ.get("TELEMETRY_REWARD_TEST_DATABASE", "")
        if not re.fullmatch(r"duris_270_[a-z0-9_]*test", cls.database):
            raise RuntimeError(
                "TELEMETRY_REWARD_TEST_DATABASE must be a task-owned duris_270_*test database"
            )
        cls.host = os.environ.get("TELEMETRY_REWARD_TEST_HOST", "127.0.0.1")
        if cls.host.lower() not in {"localhost", "127.0.0.1", "::1"}:
            raise RuntimeError("reward projection disposable tests require a loopback host")
        cls.port = int(os.environ.get("TELEMETRY_REWARD_TEST_PORT", "3306"))
        cls.admin_user = os.environ.get("TELEMETRY_REWARD_TEST_ADMIN_USER", "root")
        cls.admin_password = os.environ.get("TELEMETRY_REWARD_TEST_ADMIN_PASSWORD")
        cls.worker_user = os.environ.get("TELEMETRY_REWARD_TEST_WORKER_USER", cls.admin_user)
        cls.worker_password = os.environ.get("TELEMETRY_REWARD_TEST_WORKER_PASSWORD")
        if cls.admin_password is None or cls.worker_password is None:
            raise RuntimeError(
                "explicit TELEMETRY_REWARD_TEST_ADMIN_PASSWORD and "
                "TELEMETRY_REWARD_TEST_WORKER_PASSWORD are required"
            )

        try:
            import pymysql
        except ImportError as error:  # pragma: no cover - environment guard
            raise RuntimeError("PyMySQL is required for the explicit DB suite") from error

        common = {
            "host": cls.host,
            "port": cls.port,
            "database": cls.database,
            "charset": "utf8mb4",
            "autocommit": True,
            "cursorclass": pymysql.cursors.DictCursor,
            "connect_timeout": 2,
            "read_timeout": 3,
            "write_timeout": 3,
        }
        cls.admin = pymysql.connect(
            user=cls.admin_user, password=cls.admin_password, **common
        )
        cls.factory = PyMySQLConnectionFactory(ConnectionSettings(
            host=cls.host,
            port=cls.port,
            database=cls.database,
            user=cls.worker_user,
            password=cls.worker_password,
            read_timeout_s=3.0,
            write_timeout_s=3.0,
        ))
        cls.synthetic_operations: list[bytes] = []
        cls._assert_schema()
        cls._reset_projection()

    @classmethod
    def tearDownClass(cls) -> None:
        try:
            if hasattr(cls, "admin"):
                cls._cleanup_synthetic_sources()
                cls._reset_projection()
                cls.admin.close()
        finally:
            if hasattr(cls, "factory"):
                cls.factory.close()

    @classmethod
    def _assert_schema(cls) -> None:
        required = {
            "telemetry_reward_projection",
            "telemetry_reward_projection_state",
            "critical_operation_inbox",
            "epic_ledger",
        }
        with cls.admin.cursor() as cursor:
            cursor.execute(
                "SELECT table_name FROM information_schema.tables "
                "WHERE table_schema=DATABASE() AND table_name IN (" +
                ",".join(["%s"] * len(required)) + ")",
                tuple(sorted(required)),
            )
            found = {row["table_name"] for row in cursor.fetchall()}
        if found != required:
            raise RuntimeError(
                "#270 disposable database is missing required source/projection tables"
            )

    @classmethod
    def _reset_projection(cls) -> None:
        with cls.admin.cursor() as cursor:
            cursor.execute("DELETE FROM telemetry_reward_projection")
            cursor.execute("DELETE FROM telemetry_reward_projection_state")

    @classmethod
    def _cleanup_synthetic_sources(cls) -> None:
        if not cls.synthetic_operations:
            return
        placeholders = ",".join(["%s"] * len(cls.synthetic_operations))
        values = tuple(cls.synthetic_operations)
        with cls.admin.cursor() as cursor:
            cursor.execute(
                "DELETE FROM epic_ledger WHERE operation_id IN (" + placeholders + ")",
                values,
            )
            cursor.execute(
                "DELETE FROM critical_operation_inbox WHERE operation_id IN (" +
                placeholders + ")",
                values,
            )
        cls.synthetic_operations.clear()

    def setUp(self) -> None:
        self._reset_projection()
        self._cleanup_synthetic_sources()

    @classmethod
    def _scalar(cls, statement: str, parameters: tuple[object, ...] = ()) -> object:
        with cls.admin.cursor() as cursor:
            cursor.execute(statement, parameters)
            row = cursor.fetchone()
        return next(iter(row.values())) if row else None

    @staticmethod
    def _operation(prefix: int, suffix: int) -> bytes:
        return bytes((prefix,)) + suffix.to_bytes(4, "big") + bytes([prefix]) * 11

    def _insert_epic_source(
        self,
        operation_id: bytes,
        created_at: datetime,
        *,
        pid: int,
        delta: int,
        revision: int,
    ) -> None:
        self.synthetic_operations.append(operation_id)
        with self.admin.cursor() as cursor:
            cursor.execute(
                "INSERT INTO critical_operation_inbox "
                "(operation_id,command_hash,keys_hash,command_type,schema_version,"
                "payload_version,status,result_code,durable_revision,result_payload,"
                "created_at,committed_at) VALUES (%s,%s,%s,1,1,1,1,0,0,%s,%s,%s)",
                (
                    operation_id,
                    bytes([0x27]) * 32,
                    bytes([0x28]) * 32,
                    b"",
                    created_at,
                    created_at,
                ),
            )
            cursor.execute(
                "INSERT INTO epic_ledger "
                "(operation_id,pid,delta,balance_after,epic_revision,reason_type,"
                "reason_id,source_site,created_at) VALUES (%s,%s,%s,%s,%s,1,270,1,%s)",
                (operation_id, pid, delta, delta, revision, created_at),
            )

    @staticmethod
    def _write_batch(
        store: RewardProjectionStore,
        observations: list[SourceObservation],
        *,
        cycle_id: int,
    ) -> None:
        batch = project_observations(observations)
        store._begin()
        try:
            store.upsert_projection(batch, cycle_id=cycle_id)
            store._commit()
        except Exception:
            store._rollback()
            raise

    def test_delayed_lower_id_is_found_by_real_reconciliation_scan(self) -> None:
        adapter = SOURCE_ADAPTER_BY_NAME["epic_ledger"]
        pid = 2_700_000_000 + (os.getpid() % 100_000)
        created_at = datetime.now(timezone.utc).replace(tzinfo=None) + timedelta(seconds=2)
        floor = created_at - timedelta(microseconds=1)
        floor_usec = int(floor.replace(tzinfo=timezone.utc).timestamp() * 1_000_000)
        high_water = created_at + timedelta(seconds=2)
        high_water_usec = int(
            high_water.replace(tzinfo=timezone.utc).timestamp() * 1_000_000
        )
        operation_high = self._operation(0xF0, os.getpid())
        operation_low = self._operation(0x10, os.getpid())
        self._insert_epic_source(
            operation_high, created_at, pid=pid, delta=7, revision=1
        )

        store = RewardProjectionStore(self.factory, page_size=16)
        try:
            initial = ProjectionState(
                SourceKind.EPIC_LEDGER,
                cycle_id=1,
                fast_cursor=SourceCursor(floor_usec, ZERO_OPERATION_ID),
                cycle_high_water_usec=high_water_usec,
            )
            batch, forward = store.process_page(
                adapter,
                initial,
                high_water_usec=high_water_usec,
                reconciliation=False,
            )
            self.assertEqual([row.operation_id for row in batch.records], [operation_high])

            # Commit a lower-sorting operation after the forward cursor has passed
            # the shared timestamp.  The fast keyset must miss it; reconciliation
            # must recover it from the retained floor.
            self._insert_epic_source(
                operation_low, created_at, pid=pid, delta=3, revision=2
            )
            missed, forward_after_delay = store.process_page(
                adapter,
                forward,
                high_water_usec=high_water_usec,
                reconciliation=False,
            )
            self.assertEqual(missed.records, ())
            self.assertEqual(forward_after_delay.fast_cursor.operation_id, operation_high)

            reconciliation = forward_after_delay.start_reconciliation(
                cycle_id=2,
                retention_floor_usec=floor_usec,
                high_water_usec=high_water_usec,
            )
            recovered, completed = store.process_page(
                adapter,
                reconciliation,
                high_water_usec=high_water_usec,
                reconciliation=True,
            )
            self.assertEqual(
                {row.operation_id for row in recovered.records},
                {operation_high, operation_low},
            )
            self.assertEqual(recovered.gross_total, 10)
            self.assertFalse(completed.provisional)
            self.assertEqual(completed.acknowledged_through_usec, high_water_usec)
            self.assertEqual(
                self._scalar("SELECT COUNT(*) FROM telemetry_reward_projection"), 2
            )
            self.assertEqual(store.connection_count, 1)
        finally:
            store.close()

        query, parameters = build_source_page_query(
            adapter,
            cursor=SourceCursor(floor_usec, ZERO_OPERATION_ID),
            high_water_usec=high_water_usec,
            page_size=16,
        )
        with self.admin.cursor() as cursor:
            cursor.execute("EXPLAIN " + query, parameters)
            plan = cursor.fetchall()
        self.assertTrue(
            any(row.get("key") == "idx_epic_created_operation" for row in plan),
            plan,
        )

    def test_duplicate_conflict_and_rollback_are_durable_mysql_semantics(self) -> None:
        operation_id = self._operation(0x42, os.getpid())
        first = SourceObservation(
            source_kind=SourceKind.EPIC_LEDGER,
            operation_id=operation_id,
            entry_index=0,
            participant_pid=42,
            source_table="epic_ledger",
            created_at_usec=1_900_000_000_000_000,
            authority_kind=AuthorityKind.COMMITTED_LEDGER,
            reward_kind=RewardKind.EPIC,
            gross_amount=50,
            net_amount=50,
            reason_type=1,
            reason_id=270,
            source_site=1,
            is_creation=True,
        )
        store = RewardProjectionStore(self.factory, page_size=16)
        try:
            self._write_batch(store, [first, first], cycle_id=3)
            self._write_batch(store, [first], cycle_id=3)
            self.assertEqual(
                self._scalar(
                    "SELECT COUNT(*) FROM telemetry_reward_projection "
                    "WHERE source_kind=%s AND operation_id=%s",
                    (int(SourceKind.EPIC_LEDGER), operation_id),
                ),
                1,
            )
            conflicting = replace(first, net_amount=51, gross_amount=51)
            self._write_batch(store, [first, conflicting], cycle_id=4)
            row = self._row(
                "SELECT status,gross_amount,net_amount,quality_flags "
                "FROM telemetry_reward_projection WHERE operation_id=%s",
                (operation_id,),
            )
            self.assertEqual(row["status"], int(ProjectionStatus.CONFLICT))
            self.assertIsNone(row["gross_amount"])
            self.assertIsNone(row["net_amount"])
            self.assertTrue(row["quality_flags"] & 2)

            rolled_back = replace(
                first, operation_id=self._operation(0x43, os.getpid())
            )
            store._begin()
            store.upsert_projection(project_observations([rolled_back]), cycle_id=5)
            store._rollback()
            self.assertEqual(
                self._scalar(
                    "SELECT COUNT(*) FROM telemetry_reward_projection "
                    "WHERE operation_id=%s",
                    (rolled_back.operation_id,),
                ),
                0,
            )
        finally:
            store.close()

    def test_state_and_report_expose_coverage_without_source_mutation(self) -> None:
        before = self._scalar("SELECT COUNT(*) FROM epic_ledger")
        operation_id = self._operation(0x52, os.getpid())
        observation = SourceObservation(
            source_kind=SourceKind.EPIC_LEDGER,
            operation_id=operation_id,
            entry_index=0,
            participant_pid=52,
            source_table="epic_ledger",
            created_at_usec=1_900_000_000_000_100,
            authority_kind=AuthorityKind.COMMITTED_LEDGER,
            reward_kind=RewardKind.EPIC,
            gross_amount=9,
            net_amount=9,
            is_creation=True,
        )
        store = RewardProjectionStore(self.factory, page_size=16)
        try:
            state = ProjectionState(SourceKind.EPIC_LEDGER).start_reconciliation(
                cycle_id=9,
                retention_floor_usec=1_899_999_000_000_000,
                high_water_usec=1_900_001_000_000_000,
            )
            store._begin()
            try:
                store.upsert_projection(project_observations([observation]), cycle_id=9)
                store.upsert_state(state)
                store._commit()
            except Exception:
                store._rollback()
                raise
            report = store.read_report(
                start_usec=1_899_999_000_000_000,
                end_usec=1_900_001_000_000_000,
                max_rows=16,
            )
            self.assertEqual(report["definition"]["name"], "committed_reward_projection")
            self.assertTrue(report["rows"])
            self.assertTrue(report["projection_state"])
            self.assertEqual(store.statement_count, 2)
        finally:
            store.close()
        self.assertEqual(self._scalar("SELECT COUNT(*) FROM epic_ledger"), before)

    @classmethod
    def _row(
        cls, statement: str, parameters: tuple[object, ...] = ()
    ) -> dict[str, object]:
        with cls.admin.cursor() as cursor:
            cursor.execute(statement, parameters)
            row = cursor.fetchone()
        if row is None:
            raise AssertionError("expected one row")
        return row


if __name__ == "__main__":
    unittest.main()
