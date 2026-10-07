"""Bounded native bank receipts from one owning consistent SQL source snapshot.

The selected operation list is explicit. This reader makes no whole-history,
retention, publication or human-identity claim. A caller must retain this exact
cut before advancing durable historical progress; a broken connection requires
a fresh capture rather than continuing its cursor under a different snapshot.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import time

from . import canonical_reward_contract as contract
from .reward_projection import RewardProjectionStore, ProjectionBoundsExceeded
from .reward_projection_definitions import DEFAULT_PAGE_SIZE, MAX_PAGE_SIZE

DEFAULT_BYTE_LIMIT = 32 * 1024 * 1024
INPUT_ROW_BYTE_BOUND = 12_288
HEADER_BYTE_BOUND = 4_096
MAX_OPERATIONS = 16_384
# One root can supply three account effects. Every individual source query
# remains inside the existing 2,000-row page limit.
MAX_BANK_OPERATIONS_PER_PAGE = MAX_PAGE_SIZE // 3
ROOT_COLUMNS = ("operation_id", "lineage", "epoch", "original_operation_id", "accounting_version",
                "writer_id", "policy_version", "compiler_version", "actor_kind", "actor_id", "reason",
                "source_event", "intent_digest", "domain_digest", "plan_digest", "canonical_intent",
                "canonical_plan", "outcome", "result_code", *contract.COUNTS)
LEDGER_COLUMNS = ("operation_id", "pid", "bank_id", *(prefix + coin for prefix in
                 ("wallet_delta_", "bank_delta_", "wallet_after_", "bank_after_") for coin in contract.COINS),
                 "wallet_revision", "bank_revision", "reason_type", "reason_id", "source_site")
INBOX_COLUMNS = ("operation_id", "command_hash", "keys_hash", "command_type", "schema_version",
                 "payload_version", "status", "result_code", "failure_stage", "durable_revision", "result_payload")
EFFECT_COLUMNS = ("operation_id", "account_index", "account_key", *(prefix + coin for prefix in
                 ("before_", "after_") for coin in contract.COINS), "before_revision", "after_revision")
POSTING_COLUMNS = ("operation_id", "line_index", "event_index", "account_index", "child_index",
                   *("delta_" + coin for coin in contract.COINS), "copper_value")
CLAIM_COLUMNS = ("lineage", "source_event", "operation_id", "outcome")
SOURCE_TABLES = ("economic_accounting_operation", "currency_ledger", "critical_operation_inbox",
                 "economic_accounting_account_effect", "economic_accounting_coin_posting",
                 "economic_accounting_source_claim", "economic_accounting_child", "economic_accounting_item_reference")
COIN_SOURCE_TABLES = (*SOURCE_TABLES, "item_ownership_ledger")
RETAINED_SOURCE_TABLES = (*COIN_SOURCE_TABLES, "auction_ledger", "economic_pending_claim_source")


@dataclass(frozen=True, slots=True)
class RetainedSource:
    reference: contract.SourceReference
    payload: bytes


@dataclass(frozen=True, slots=True)
class DiscoveryScope:
    bucket: int
    cursor: bytes
    page_limit: int
    through: bytes | None
    pass_upper: bytes

    def validate(self, selected):
        contract.integer(self.bucket, upper=255)
        contract.identifier(self.cursor, zero=True)
        contract.integer(self.page_limit, lower=1, upper=MAX_BANK_OPERATIONS_PER_PAGE)
        contract.identifier(self.pass_upper, zero=True)
        contract.require(self.cursor == contract.ZERO_ID or self.cursor[0] == self.bucket,
                         "canonical_discovery_cursor_scope")
        if self.through is not None:
            contract.identifier(self.through, zero=True)
            contract.require(self.pass_upper == self.through and self.cursor <= self.through,
                             "canonical_discovery_fixed_upper")
        contract.require(self.pass_upper == contract.ZERO_ID or self.pass_upper[0] == self.bucket,
                         "canonical_discovery_upper_scope")
        contract.require(len(selected) <= self.page_limit and
                         all(op[0] == self.bucket and self.cursor < op <= self.pass_upper for op in selected),
                         "canonical_discovery_selected_range")

    def payload(self):
        return contract.encode_source_payload(dict(bucket=self.bucket, cursor=self.cursor, page_limit=self.page_limit,
                                                    through=self.through, pass_upper=self.pass_upper))


@dataclass(frozen=True, slots=True)
class BankSourceCut:
    captured_utc_usec: int
    selected_operations: tuple[bytes, ...]
    sources: tuple[RetainedSource, ...]
    source_digest: bytes
    reserved_bytes: int
    events: tuple[contract.RewardEvent, ...]
    query_counts: tuple[int, ...]
    elapsed_seconds: float
    # Completeness is restricted to the exact selected operations and SQL cut.
    coverage: str = "selected_native_bank_operations"
    future_commits_provisional: bool = True
    discovery: DiscoveryScope | None = None


@dataclass(frozen=True, slots=True)
class BankPartitionPage:
    cut: BankSourceCut
    bucket: int
    cursor_before: bytes
    cursor_after: bytes
    wrapped: bool
    page_limit: int
    pass_upper: bytes
    # A short page wraps only this discovery cursor. Later commits are unknown.
    future_commits_provisional: bool = True


def source_cut_digest(captured, selected, sources, discovery=None):
    checksum = hashlib.sha256(b"duris-canonical-bank-source-v1\0" + captured.to_bytes(8, "big") +
                              len(selected).to_bytes(4, "big"))
    for operation in selected:
        checksum.update(operation)
    for row in sources:
        checksum.update(row.reference.table.encode("ascii") + b"\0" +
                        len(row.reference.key).to_bytes(2, "big") + row.reference.key +
                        row.reference.payload_digest)
    result = checksum.digest()
    if discovery is not None:
        discovery.validate(selected)
        result = hashlib.sha256(b"duris-canonical-bank-discovery-v1\0" + result + discovery.payload()).digest()
    return result


def unavailable_bank_event(operation, sources):
    references = tuple(sorted(source.reference for source in sources))
    result = contract.RewardEvent(contract.EventIdentity(contract.Unit.CURRENCY, operation, 0),
                                  contract.Disposition.UNKNOWN, None, "unavailable", references)
    result.validate()
    return result


def decode_retained_source(retained):
    """Verify canonical bytes and the native physical key without granting authority."""
    contract.require(type(retained) is RetainedSource, "canonical_retained_source_type")
    reference = retained.reference
    reference.validate()
    contract.require(reference.table in RETAINED_SOURCE_TABLES, "canonical_retained_source_family")
    contract.require(type(retained.payload) is bytes and
                     0 < len(retained.payload) <= contract.MAX_SOURCE_RECORD_BYTES,
                     "canonical_source_record_capacity")
    contract.require(hashlib.sha256(retained.payload).digest() == reference.payload_digest,
                     "canonical_retained_payload_digest")
    row = contract.decode_source_payload(retained.payload)
    op = contract.identifier(row.get("claim_operation_id") if reference.table == "economic_pending_claim_source"
                             else row.get("operation_id"))
    if reference.table == "economic_pending_claim_source":
        key = contract.identifier(row.get("source_operation_id")) + contract.integer(
            row.get("source_slot"), lower=1, upper=(1 << 16) - 1).to_bytes(2, "big")
    elif reference.table == "economic_accounting_source_claim":
        source_event = row.get("source_event")
        contract.require(type(source_event) is bytes and len(source_event) == 48,
                         "canonical_retained_source_claim_shape")
        key = contract.identifier(row.get("lineage")) + source_event
    elif reference.table == "economic_accounting_account_effect":
        key = op + contract.integer(row.get("account_index"), upper=3071).to_bytes(2, "big")
    elif reference.table == "economic_accounting_coin_posting":
        key = op + contract.integer(row.get("line_index"), upper=6143).to_bytes(2, "big")
    elif reference.table == "economic_accounting_child":
        key = op + contract.integer(row.get("child_index"), lower=1, upper=64).to_bytes(2, "big")
    elif reference.table == "economic_accounting_item_reference":
        key = op + contract.integer(row.get("line_index"), upper=2999).to_bytes(2, "big")
    elif reference.table == "item_ownership_ledger":
        key = op + contract.integer(row.get("event_index"), upper=2999).to_bytes(4, "big")
    else:
        key = op
    contract.require(key == reference.key, "canonical_retained_physical_key")
    return op, row


def replay_retained_bank_cut(captured, selected, sources, expected_digest, *, byte_limit=DEFAULT_BYTE_LIMIT,
                             strict=True, discovery=None):
    """Reconstruct authority solely from a sealed, exact retained source cut.

    Never consult present native rows or trust stored projected amounts. Decode
    one receipt at a time; the index retains references to existing payloads.
    A digest, a missing physical row, or changed canonical representation is a
    refusal of the whole cut, rather than a partial earned-reward result.
    """
    contract.integer(captured, lower=1)
    contract.require(type(strict) is bool, "canonical_retained_replay_mode")
    contract.integer(byte_limit, lower=1, upper=DEFAULT_BYTE_LIMIT)
    contract.digest(expected_digest)
    contract.require(type(selected) is tuple and len(selected) <= MAX_OPERATIONS and
                     type(sources) is tuple and len(sources) <= contract.MAX_REFERENCES,
                     "canonical_retained_cut_capacity")
    for operation in selected:
        contract.identifier(operation)
    contract.require(selected == tuple(sorted(set(selected))), "canonical_retained_selection_order")
    groups = {op: {table: [] for table in RETAINED_SOURCE_TABLES} for op in selected}
    reserved = HEADER_BYTE_BOUND + len(selected) * 32
    if reserved > byte_limit:
        raise ProjectionBoundsExceeded("canonical retained source budget")
    previous, decoded, child_roots, credit_roots = None, [], {}, {}
    for retained in sources:
        contract.require(type(retained) is RetainedSource, "canonical_retained_source_type")
        reference = retained.reference
        reference.validate()
        identity = reference.table, reference.key
        contract.require(previous is None or previous < identity, "canonical_retained_source_order")
        previous = identity
        contract.require(reference.table in RETAINED_SOURCE_TABLES, "canonical_retained_source_family")
        contract.require(type(retained.payload) is bytes and
                         0 < len(retained.payload) <= contract.MAX_SOURCE_RECORD_BYTES,
                         "canonical_source_record_capacity")
        reserved += max(INPUT_ROW_BYTE_BOUND, len(retained.payload) + len(reference.table.encode("ascii")) +
                        len(reference.key) + 32 + 128)
        if reserved > byte_limit:
            raise ProjectionBoundsExceeded("canonical retained source budget")
        op, row = decode_retained_source(retained)
        decoded.append((op, retained))
        if reference.table == "economic_accounting_child":
            child = contract.identifier(row.get("child_operation_id"))
            contract.require(op in groups and child not in groups and child not in child_roots,
                             "canonical_retained_child_root_scope")
            child_roots[child] = op
        elif reference.table == "economic_pending_claim_source":
            source = contract.identifier(row.get("source_operation_id"))
            contract.require(op in groups and source not in groups and source not in child_roots and
                             (source not in credit_roots or credit_roots[source] == op),
                             "canonical_retained_auction_source_scope")
            credit_roots[source] = op
    for op, retained in decoded:
        # Only explicit native child rows establish compatibility/inbox parentage.
        root = child_roots.get(op, op) if retained.reference.table in (
            "currency_ledger", "critical_operation_inbox", "item_ownership_ledger") else op
        root = credit_roots.get(root, root)
        contract.require(root in groups, "canonical_retained_operation_scope")
        groups[root][retained.reference.table].append(retained)
    if discovery is not None:
        contract.require(type(discovery) is DiscoveryScope, "canonical_retained_discovery_type")
        discovery.validate(selected)
    contract.require(source_cut_digest(captured, selected, sources, discovery) == expected_digest,
                     "canonical_retained_cut_digest")
    events, has_nonbank = [], False
    for op in selected:
        families = groups[op]
        receipt_sources = tuple(source for table in RETAINED_SOURCE_TABLES for source in families[table])
        try:
            roots = [contract.decode_source_payload(source.payload) for source in families["economic_accounting_operation"]]
            current = [root for root in roots if root.get("operation_id") == op]
            contract.require(len(current) == 1, "canonical_retained_root_count")
            root = current[0]
            coin = root.get("writer_id") == 5 and root.get("reason") == 3
            auction = root.get("writer_id") == 13 and root.get("reason") == 31
            has_nonbank |= coin or auction
            if auction:
                from .canonical_reward_auction import qualify_retained_money_claim
                rows = {table: [contract.decode_source_payload(source.payload) for source in families[table]]
                        for table in RETAINED_SOURCE_TABLES}
                event = qualify_retained_money_claim(root, rows)
            elif coin:
                contract.require(len(roots) == 1 and not families["auction_ledger"] and not families["economic_pending_claim_source"],
                                 "canonical_coin_unexpected_auction_receipt")
                from .canonical_reward_coin import qualify_coin_root
                rows = {table: [contract.decode_source_payload(source.payload) for source in families[table]]
                        for table in RETAINED_SOURCE_TABLES}
                event = qualify_coin_root(root, rows["critical_operation_inbox"],
                    rows["economic_accounting_account_effect"], rows["economic_accounting_coin_posting"],
                    rows["economic_accounting_source_claim"], rows["economic_accounting_child"],
                    rows["currency_ledger"], rows["economic_accounting_item_reference"], rows["item_ownership_ledger"])
            else:
                contract.require(len(roots) == 1, "canonical_retained_root_count")
                contract.require(all(not families[table] for table in RETAINED_SOURCE_TABLES[6:]),
                                 "canonical_bank_unexpected_child_or_item")
                # Refuse adversarial fan-out before decoding a native receipt again.
                contract.require(all(len(families[table]) == 1 for table in SOURCE_TABLES[:3]) and
                                 len(families[SOURCE_TABLES[3]]) in (2, 3) and
                                 len(families[SOURCE_TABLES[4]]) == 2 and len(families[SOURCE_TABLES[5]]) <= 1,
                                 "canonical_retained_receipt_shape")
                rows = [[contract.decode_source_payload(source.payload) for source in families[table]]
                        for table in SOURCE_TABLES[:6]]
                event = contract.qualify_bank_root(*(family[0] for family in rows[:3]), *rows[3:])
        except (contract.EvidenceError, KeyError, TypeError, OverflowError) as error:
            if strict:
                if isinstance(error, contract.EvidenceError):
                    raise
                raise contract.EvidenceError("canonical_retained_receipt_fields") from error
            event = unavailable_bank_event(op, receipt_sources)
        contract.require(set(event.references) == {source.reference for table in RETAINED_SOURCE_TABLES
                         for source in families[table]}, "canonical_retained_reference_set")
        events.append(event)
    coverage = "selected_native_economic_operations" if has_nonbank else "selected_native_bank_operations"
    contract.require(not has_nonbank or discovery is None, "canonical_bank_discovery_coin_scope")
    return BankSourceCut(captured, selected, sources, expected_digest, reserved, tuple(events), (), 0.0,
                         coverage=coverage, discovery=discovery)


class CanonicalRewardSnapshot(RewardProjectionStore):
    """Reuse the existing private one-connection worker boundary; never write sources."""

    source_tables = SOURCE_TABLES
    max_operations_per_page = MAX_BANK_OPERATIONS_PER_PAGE

    def __init__(self, connection_factory, *, page_size=DEFAULT_PAGE_SIZE,
                 byte_limit=DEFAULT_BYTE_LIMIT, time_limit_s=10.0, clock=time.monotonic):
        super().__init__(connection_factory, page_size=page_size)
        contract.integer(byte_limit, lower=1, upper=DEFAULT_BYTE_LIMIT)
        contract.require(type(time_limit_s) in (int, float) and 0 < time_limit_s <= 10,
                         "canonical_capture_time_budget")
        self.byte_limit = byte_limit
        self.time_limit_s = time_limit_s
        self.clock = clock
        self._deadline = None
        self._bytes = 0
        self._sources = {}

    def _execute(self, statement, parameters=()):
        if self._deadline is not None and self.clock() >= self._deadline:
            raise ProjectionBoundsExceeded("canonical source capture deadline")
        rows = super()._execute(statement, parameters)
        if self._deadline is not None and self.clock() >= self._deadline:
            raise ProjectionBoundsExceeded("canonical source capture deadline")
        if len(rows) > MAX_PAGE_SIZE:
            raise ProjectionBoundsExceeded("canonical source query row bound")
        return rows

    def _retain(self, table, key, row):
        payload = contract.encode_source_payload(row)
        reference = contract.source_reference(table, key, row)
        identity = table, key
        prior = self._sources.get(identity)
        contract.require(prior is None or prior.payload == payload, "canonical_source_cut_conflict")
        if prior is None:
            # Include identity/digest overhead, not just native blob bytes.
            amount = max(INPUT_ROW_BYTE_BOUND, len(payload) + len(table.encode("ascii")) + len(key) + 32 + 128)
            if self._bytes + amount > self.byte_limit or len(self._sources) >= contract.MAX_REFERENCES:
                raise ProjectionBoundsExceeded("canonical retained source budget")
            self._sources[identity] = RetainedSource(reference, payload)
            self._bytes += amount
        return self._sources[identity]

    def capture_bank_operations(self, operations, *, preserve_refusals=False) -> BankSourceCut:
        contract.require(type(preserve_refusals) is bool, "canonical_quarantine_mode")
        return self._capture(operations, partition=None, preserve_refusals=preserve_refusals)

    def _partition_limit(self):
        # Reserve the worst supported bank receipt, including selection, before
        # discovery. A full historical page must fit the unchanged byte budget.
        limit = min(self.page_size, MAX_BANK_OPERATIONS_PER_PAGE,
                    (self.byte_limit - HEADER_BYTE_BOUND) // (9 * INPUT_ROW_BYTE_BOUND + 32))
        if limit <= 0:
            raise ProjectionBoundsExceeded("canonical historical page budget")
        return limit

    def capture_bank_partition(self, bucket, after=contract.ZERO_ID, *, through=None,
                               preserve_refusals=False) -> BankPartitionPage:
        contract.integer(bucket, upper=255)
        contract.identifier(after, zero=True)
        contract.require(after == contract.ZERO_ID or after[0] == bucket, "canonical_partition_cursor_scope")
        if through is not None:
            contract.identifier(through, zero=True)
            contract.require((through == contract.ZERO_ID or through[0] == bucket) and after <= through,
                             "canonical_partition_upper_scope")
        limit = self._partition_limit()
        contract.require(type(preserve_refusals) is bool, "canonical_quarantine_mode")
        discovery = dict(bucket=bucket, after=after, limit=limit, through=through, pass_upper=through or contract.ZERO_ID)
        cut = self._capture((), partition=discovery, preserve_refusals=preserve_refusals)
        wrapped = len(cut.selected_operations) < limit or cut.selected_operations[-1] == discovery["pass_upper"]
        cursor_after = contract.ZERO_ID if wrapped else cut.selected_operations[-1]
        return BankPartitionPage(cut, bucket, after, cursor_after, wrapped, limit, discovery["pass_upper"])

    def _capture(self, operations, *, partition, preserve_refusals) -> BankSourceCut:
        # Materialize only inside the finite input contract, including generators.
        selected = []
        for operation in operations:
            contract.require(len(selected) < MAX_OPERATIONS, "canonical_capture_operation_capacity")
            selected.append(contract.identifier(operation))
        contract.require(len(selected) == len(set(selected)), "canonical_capture_duplicate_operation")
        selected = tuple(sorted(selected))
        self._bytes, self._sources, self._statement_count = 0, {}, 0
        started = self.clock()
        self._deadline = started + self.time_limit_s
        events, query_counts = [], []
        connection = self._ensure_connection()
        try:
            # The factory may have performed session/readiness probes. Release
            # its implicit read transaction before setting the owning isolation.
            connection.rollback()
            if HEADER_BYTE_BOUND + len(selected) * 32 > self.byte_limit:
                raise ProjectionBoundsExceeded("canonical source header budget")
            self._bytes = HEADER_BYTE_BOUND + len(selected) * 32
            self._execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
            self._execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
            engines = self._execute("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables "
                "WHERE table_schema=DATABASE() AND table_name IN (" +
                ",".join("%s" for _ in self.source_tables) + ")", self.source_tables)
            contract.require(len(engines) == len(self.source_tables) and
                {row["table_name"] for row in engines} == set(self.source_tables) and
                all(row["engine"] == "InnoDB" for row in engines), "canonical_source_snapshot_engines")
            clock_rows = self._execute("SELECT TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00',"
                                      "UTC_TIMESTAMP(6)) AS captured_utc_usec")
            contract.require(len(clock_rows) == 1, "canonical_capture_clock_row")
            captured = contract.integer(clock_rows[0]["captured_utc_usec"], lower=1)
            if partition is not None:
                self._statement_count = 0
                bucket, after, limit, through = (partition[name] for name in ("bucket", "after", "limit", "through"))
                lower = bytes((bucket,)) + bytes(15)
                scope = "operation_id>=%s AND operation_id>%s"
                values = [lower, after]
                upper_scope, upper_values = "operation_id>=%s", [lower]
                if bucket < 255:
                    scope += " AND operation_id<%s"
                    values.append(bytes((bucket + 1,)) + bytes(15))
                    upper_scope += " AND operation_id<%s"
                    upper_values.append(bytes((bucket + 1,)) + bytes(15))
                supported = " AND outcome=1 AND result_code=0 AND writer_id IN (1,2,3,4) AND reason IN (1,5,8) "
                if through is None:
                    upper_select = "(SELECT MAX(operation_id) FROM economic_accounting_operation WHERE " + upper_scope + supported + ") AS pass_upper"
                else:
                    upper_select, upper_values = "CAST(%s AS BINARY(16)) AS pass_upper", [through]
                    scope += " AND operation_id<=%s"
                    values.append(through)
                # Discovery and receipt evidence belong to this same owning SQL
                # snapshot. Reuse the length/count preflight so the entire page
                # still consumes at most eight SELECT statements.
                shapes = self._execute("SELECT operation_id,writer_id,reason," + ",".join(contract.COUNTS) +
                    ",OCTET_LENGTH(canonical_plan) AS plan_bytes,OCTET_LENGTH(canonical_intent) AS intent_bytes "
                    "," + upper_select + " "
                    "FROM economic_accounting_operation WHERE " + scope +
                    supported + "ORDER BY operation_id LIMIT %s", (*upper_values, *values, limit))
                selected = tuple(contract.identifier(row["operation_id"]) for row in shapes)
                contract.require(selected == tuple(sorted(set(selected))) and len(selected) <= limit and
                                 all(op[0] == bucket and op > after for op in selected),
                                 "canonical_partition_discovery_scope")
                if shapes:
                    pass_upper = contract.identifier(shapes[0]["pass_upper"])
                    contract.require(pass_upper[0] == bucket and pass_upper >= selected[-1] and
                        all(row["pass_upper"] == pass_upper for row in shapes) and
                        (through is None or pass_upper == through), "canonical_partition_frozen_upper")
                    partition["pass_upper"] = pass_upper
                self._bytes += len(selected) * 32
                if selected:
                    self._capture_page(selected, events, shapes=shapes, preserve_refusals=preserve_refusals)
                query_counts.append(self.statement_count)
            else:
                size = min(self.page_size, self.max_operations_per_page)
                for start in range(0, len(selected), size):
                    self._statement_count = 0
                    page = selected[start:start + size]
                    self._capture_page(page, events, preserve_refusals=preserve_refusals)
                    query_counts.append(self.statement_count)
            # No write or commit acknowledgement can grant source completeness.
            # Return only after every selected receipt has been captured intact.
            sources = tuple(self._sources[key] for key in sorted(self._sources))
            discovery = None if partition is None else DiscoveryScope(partition["bucket"], partition["after"],
                partition["limit"], partition["through"], partition["pass_upper"])
            result = BankSourceCut(captured, selected, sources, source_cut_digest(captured, selected, sources, discovery), self._bytes,
                                   tuple(events), tuple(query_counts), self.clock() - started, discovery=discovery)
            if self.source_tables != SOURCE_TABLES:
                replay = replay_retained_bank_cut(captured, selected, sources, result.source_digest,
                                                 byte_limit=self.byte_limit, strict=not preserve_refusals)
                contract.require(replay.events == result.events, "canonical_capture_replay_disagreement")
                result = BankSourceCut(captured, selected, sources, result.source_digest, self._bytes,
                    tuple(events), tuple(query_counts), result.elapsed_seconds, coverage=replay.coverage)
            connection.rollback()
            return result
        except Exception:
            try:
                connection.rollback()
            finally:
                self._sources.clear()
                self._bytes = 0
            raise
        finally:
            self._deadline = None
            self._sources.clear()
            self.close()

    def _capture_page(self, page, events, *, shapes=None, preserve_refusals=False):
        markers = ",".join("%s" for _ in page)
        where = " WHERE operation_id IN (" + markers + ")"
        evidence_by_operation = {op: [] for op in page}
        # Inspect lengths/counts before fetching any blob or fan-out rows.
        if shapes is None:
            shapes = self._execute("SELECT operation_id,writer_id,reason," + ",".join(contract.COUNTS) +
                ",OCTET_LENGTH(canonical_plan) AS plan_bytes,OCTET_LENGTH(canonical_intent) AS intent_bytes "
                "FROM economic_accounting_operation" + where + " ORDER BY operation_id LIMIT %s", (*page, len(page)))
        contract.require(len(shapes) == len(page) and {row["operation_id"] for row in shapes} == set(page),
                         "canonical_selected_root_missing")
        for shape in shapes:
            contract.require(shape["writer_id"] in (1, 2, 3, 4) and shape["reason"] in (1, 5, 8) and
                shape["account_count"] in (2, 3) and shape["posting_count"] == 2 and
                all(shape[name] == 0 for name in contract.COUNTS[2:]) and
                shape["plan_bytes"] in (592, 712) and shape["intent_bytes"] == 280,
                "canonical_selected_bank_route_unavailable")
        specifications = (
            ("economic_accounting_operation", ROOT_COLUMNS, "operation_id", len(page), "recorded_at"),
            ("currency_ledger", LEDGER_COLUMNS, "operation_id", len(page), "created_at"),
            ("critical_operation_inbox", INBOX_COLUMNS, "operation_id", len(page), "committed_at"),
            ("economic_accounting_account_effect", EFFECT_COLUMNS, "operation_id,account_index", len(page) * 3, None),
            ("economic_accounting_coin_posting", POSTING_COLUMNS, "operation_id,line_index", len(page) * 2, None),
            ("economic_accounting_source_claim", CLAIM_COLUMNS, "operation_id", len(page), None),
        )
        families = []
        for table, columns, order, maximum, timestamp in specifications:
            extra = (",TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00'," + timestamp +
                     ") AS " + timestamp + "_usec") if timestamp else ""
            # +1 detects unexpected duplicate/fan-out rows. For the maximum
            # effects page, an exact per-root declared count is checked below.
            limit = min(MAX_PAGE_SIZE, maximum + 1)
            rows = self._execute("SELECT " + ",".join(columns) + extra + " FROM " + table + where +
                                 " ORDER BY " + order + " LIMIT %s", (*page, limit))
            contract.require(len(rows) <= maximum, "canonical_source_family_capacity")
            groups = {operation: [] for operation in page}
            for row in rows:
                op = contract.identifier(row["operation_id"])
                contract.require(op in groups, "canonical_source_query_scope")
                groups[op].append(row)
                if table == "economic_accounting_source_claim":
                    key = row["lineage"] + row["source_event"]
                elif table == "economic_accounting_account_effect":
                    key = op + contract.integer(row["account_index"], upper=3071).to_bytes(2, "big")
                elif table == "economic_accounting_coin_posting":
                    key = op + contract.integer(row["line_index"], upper=6143).to_bytes(2, "big")
                else:
                    key = op
                evidence_by_operation[op].append(self._retain(table, key, row))
            families.append(groups)
        unexpected = self._execute("SELECT operation_id FROM economic_accounting_child" + where +
            " UNION ALL SELECT operation_id FROM economic_accounting_item_reference" + where + " LIMIT 1", (*page, *page))
        contract.require(not unexpected, "canonical_bank_unexpected_child_or_item")
        for op in page:
            groups = [family[op] for family in families]
            try:
                contract.require(all(len(rows) == 1 for rows in groups[:3]), "canonical_primary_receipt_count")
                event = contract.qualify_bank_root(*(rows[0] for rows in groups[:3]), *groups[3:])
            except (contract.EvidenceError, KeyError, TypeError, OverflowError):
                if not preserve_refusals:
                    raise
                event = unavailable_bank_event(op, evidence_by_operation[op])
            events.append(event)
