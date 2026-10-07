"""Private durable canonical source cuts; no publication or native source writes.

A whole bounded cut commits atomically. SQL guards freeze its exact selected
operations and physical payloads at sealing. Recovery independently reconstructs
authority from those payloads, including after an ambiguous COMMIT reply.
The three tables are introduced by the additive reward-definition-2 migration.
"""
from __future__ import annotations

import time

from . import canonical_reward_contract as contract
from .canonical_reward_source import (
    BankSourceCut, RetainedSource, DiscoveryScope, replay_retained_bank_cut, DEFAULT_BYTE_LIMIT,
    HEADER_BYTE_BOUND, INPUT_ROW_BYTE_BOUND, MAX_OPERATIONS,
)
from .reward_projection import RewardProjectionStore, ProjectionBoundsExceeded

CUT_TABLE = "telemetry_reward_cut_v2"
SELECTION_TABLE = "telemetry_reward_selection_v2"
SOURCE_TABLE = "telemetry_reward_source_v2"
RETENTION_TABLES = (CUT_TABLE, SELECTION_TABLE, SOURCE_TABLE)
HEADER_COLUMNS = ("cut_id", "definition_version", "captured_utc_usec", "source_digest",
                  "selected_count", "source_count", "reserved_bytes", "sealed",
                  "discovery_bucket", "discovery_cursor", "discovery_limit", "discovery_through", "discovery_upper")
SOURCE_COLUMNS = ("source_index", "source_table", "source_key", "payload", "payload_digest")
WRITE_PAGE_SIZE = 64  # At most 512 KiB of exact payload bytes per statement.


class CanonicalRewardRetention(RewardProjectionStore):
    """Own only projection tables and one private connection at a time."""

    def __init__(self, connection_factory, *, page_size=256, byte_limit=DEFAULT_BYTE_LIMIT,
                 time_limit_s=10.0, clock=time.monotonic):
        super().__init__(connection_factory, page_size=page_size)
        contract.integer(byte_limit, lower=1, upper=DEFAULT_BYTE_LIMIT)
        contract.require(type(time_limit_s) in (int, float) and 0 < time_limit_s <= 10,
                         "canonical_retention_time_budget")
        self.byte_limit, self.time_limit_s, self.clock = byte_limit, time_limit_s, clock
        self._deadline = None

    def _execute(self, statement, parameters=()):
        self._check_deadline()
        result = super()._execute(statement, parameters)
        self._check_deadline()
        if len(result) > max(self.page_size, len(RETENTION_TABLES)):
            raise ProjectionBoundsExceeded("canonical retained query row budget")
        return result

    def _check_deadline(self):
        if self._deadline is not None and self.clock() >= self._deadline:
            raise ProjectionBoundsExceeded("canonical retention deadline")

    def _start(self, *, read_only):
        connection = self._ensure_connection()
        connection.rollback()
        self._statement_count = 0
        self._execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        self._execute("START TRANSACTION WITH CONSISTENT SNAPSHOT" + (", READ ONLY" if read_only else ""))
        rows = self._execute("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables "
                             "WHERE table_schema=DATABASE() AND table_name IN (%s,%s,%s)", RETENTION_TABLES)
        contract.require(len(rows) == len(RETENTION_TABLES) and
                         {row["table_name"] for row in rows} == set(RETENTION_TABLES) and
                         all(row["engine"] == "InnoDB" for row in rows), "canonical_retention_snapshot_engines")

    def _header(self, cut_id, *, lock=False):
        self._statement_count = 0
        rows = self._execute("SELECT " + ",".join(HEADER_COLUMNS) + " FROM " + CUT_TABLE +
                             " WHERE cut_id=%s" + (" FOR UPDATE" if lock else ""), (cut_id,))
        contract.require(len(rows) <= 1, "canonical_retained_header_count")
        if not rows:
            return None
        row = rows[0]
        contract.require(row["cut_id"] == cut_id == row["source_digest"] and
                         row["definition_version"] == contract.DEFINITION_VERSION and row["sealed"] in (0, 1),
                         "canonical_retained_header_identity")
        contract.integer(row["captured_utc_usec"], lower=1)
        contract.integer(row["selected_count"], upper=MAX_OPERATIONS)
        contract.integer(row["source_count"], upper=contract.MAX_REFERENCES)
        contract.integer(row["reserved_bytes"], lower=HEADER_BYTE_BOUND, upper=DEFAULT_BYTE_LIMIT)
        if row["reserved_bytes"] > self.byte_limit:
            raise ProjectionBoundsExceeded("canonical retained source budget")
        if row["discovery_bucket"] is None:
            contract.require(all(row[name] is None for name in HEADER_COLUMNS[9:]), "canonical_retained_absent_discovery")
        else:
            contract.integer(row["discovery_bucket"], upper=255)
            contract.identifier(row["discovery_cursor"], zero=True)
            contract.integer(row["discovery_limit"], lower=1, upper=666)
            contract.identifier(row["discovery_upper"], zero=True)
            if row["discovery_through"] is not None:
                contract.identifier(row["discovery_through"], zero=True)
        return row

    def _bulk(self, table, columns, rows):
        start = 0
        while start < len(rows):
            self._statement_count = 0
            page, payload_bytes = [], 0
            for row in rows[start:start + WRITE_PAGE_SIZE]:
                size = sum(len(cell) for cell in row if type(cell) is bytes)
                if size > 512 * 1024:
                    raise ProjectionBoundsExceeded("canonical retained write payload budget")
                if page and payload_bytes + size > 512 * 1024:
                    break
                page.append(row)
                payload_bytes += size
            markers = "(" + ",".join("%s" for _ in columns) + ")"
            self._execute("INSERT INTO " + table + " (" + ",".join(columns) + ") VALUES " +
                          ",".join(markers for _ in page), tuple(cell for row in page for cell in row))
            start += len(page)

    def retain(self, cut: BankSourceCut) -> bytes:
        contract.require(type(cut) is BankSourceCut and cut.future_commits_provisional and
                         cut.coverage in ("selected_native_bank_operations", "selected_native_economic_operations"),
                         "canonical_retained_cut_scope")
        replay = replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources,
                                          cut.source_digest, byte_limit=self.byte_limit, strict=False, discovery=cut.discovery)
        contract.require(replay.events == cut.events and replay.reserved_bytes == cut.reserved_bytes and replay.coverage == cut.coverage,
                         "canonical_retained_projection_disagreement")
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=False)
            existing = self._header(cut.source_digest, lock=True)
            if existing is not None:
                contract.require(existing["sealed"] == 1, "canonical_retained_unsealed_cut")
                restored = self._load_rows(existing)
                contract.require(restored == replay, "canonical_retained_replay_disagreement")
                self._rollback()
                return cut.source_digest
            discovery = cut.discovery
            self._execute("INSERT INTO " + CUT_TABLE + " (" + ",".join(HEADER_COLUMNS) +
                          ") VALUES (" + ",".join("%s" for _ in HEADER_COLUMNS) + ")",
                          (cut.source_digest, contract.DEFINITION_VERSION, cut.captured_utc_usec,
                           cut.source_digest, len(cut.selected_operations), len(cut.sources), cut.reserved_bytes, 0,
                           discovery.bucket if discovery else None, discovery.cursor if discovery else None,
                           discovery.page_limit if discovery else None, discovery.through if discovery else None,
                           discovery.pass_upper if discovery else None))
            self._bulk(SELECTION_TABLE, ("cut_id", "selection_index", "operation_id"),
                       [(cut.source_digest, index, op) for index, op in enumerate(cut.selected_operations)])
            self._bulk(SOURCE_TABLE, ("cut_id", *SOURCE_COLUMNS),
                       [(cut.source_digest, index, row.reference.table.encode("ascii"), row.reference.key,
                         row.payload, row.reference.payload_digest) for index, row in enumerate(cut.sources)])
            self._statement_count = 0
            self._execute("UPDATE " + CUT_TABLE + " SET sealed=1 WHERE cut_id=%s AND sealed=0", (cut.source_digest,))
            # Re-read the exact physical rows in this transaction before sealing
            # becomes durable. A malformed or incomplete write rolls back whole.
            header = self._header(cut.source_digest)
            contract.require(header is not None and header["sealed"] == 1, "canonical_retained_seal_missing")
            contract.require(self._load_rows(header) == replay, "canonical_retained_write_disagreement")
            self._commit()
            return cut.source_digest
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def _load_rows(self, header):
        selected, sources = [], []
        reserved = HEADER_BYTE_BOUND + header["selected_count"] * 32
        if reserved > self.byte_limit:
            raise ProjectionBoundsExceeded("canonical retained source budget")
        for table, columns, count, destination in (
            (SELECTION_TABLE, ("selection_index", "operation_id"), header["selected_count"], selected),
            (SOURCE_TABLE, SOURCE_COLUMNS, header["source_count"], sources),
        ):
            index_column = columns[0]
            for start in range(0, count + 1, self.page_size):
                self._statement_count = 0
                rows = self._execute("SELECT " + ",".join(columns) + " FROM " + table +
                    " WHERE cut_id=%s AND " + index_column + ">=%s ORDER BY " + index_column + " LIMIT %s",
                    (header["cut_id"], start, min(self.page_size, count + 1 - start)))
                expected = min(self.page_size, count - start)
                contract.require(len(rows) == max(0, expected), "canonical_retained_row_count")
                for offset, row in enumerate(rows, start):
                    contract.require(row[index_column] == offset, "canonical_retained_row_index")
                    if table == SELECTION_TABLE:
                        destination.append(contract.identifier(row["operation_id"]))
                    else:
                        name = row["source_table"]
                        contract.require(type(name) is bytes and name.isascii(), "canonical_retained_source_table")
                        reference = contract.SourceReference(name.decode("ascii"), row["source_key"], row["payload_digest"])
                        reference.validate()
                        payload = row["payload"]
                        contract.require(type(payload) is bytes and 0 < len(payload) <= contract.MAX_SOURCE_RECORD_BYTES,
                                         "canonical_source_record_capacity")
                        reserved += max(INPUT_ROW_BYTE_BOUND, len(payload) + len(name) + len(reference.key) + 32 + 128)
                        if reserved > self.byte_limit:
                            raise ProjectionBoundsExceeded("canonical retained source budget")
                        destination.append(RetainedSource(reference, payload))
        discovery = None if header["discovery_bucket"] is None else DiscoveryScope(header["discovery_bucket"],
            header["discovery_cursor"], header["discovery_limit"], header["discovery_through"], header["discovery_upper"])
        result = replay_retained_bank_cut(header["captured_utc_usec"], tuple(selected), tuple(sources),
                                          header["source_digest"], byte_limit=self.byte_limit, strict=False, discovery=discovery)
        contract.require(result.reserved_bytes == header["reserved_bytes"], "canonical_retained_reservation_disagreement")
        self._check_deadline()
        return result

    def load(self, cut_id):
        contract.digest(cut_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            header = self._header(cut_id)
            contract.require(header is not None and header["sealed"] == 1, "canonical_retained_sealed_cut_missing")
            result = self._load_rows(header)
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()
