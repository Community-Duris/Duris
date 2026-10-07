"""Resolve retained bank cuts against physical payload conflicts in one SQL cut.

The lookup covers all sealed definition-2 cuts visible in the owning snapshot,
including quarantines outside the selected sweep. It never reads native tables,
chooses a winning payload, or claims that future evidence is complete.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
import hashlib

from . import canonical_reward_contract as contract
from .canonical_reward_source import BankSourceCut, RetainedSource, decode_retained_source, INPUT_ROW_BYTE_BOUND
from .canonical_reward_retention import CanonicalRewardRetention, CUT_TABLE, SOURCE_TABLE
from .reward_projection import ProjectionBoundsExceeded

MAX_CONFLICT_LOOKUPS = 2000
VARIANT_CACHE_ENTRY_BYTES = 512


@dataclass(frozen=True, slots=True)
class ConflictWitness:
    cut_id: bytes
    source_index: int
    source: RetainedSource

    def validate(self):
        contract.digest(self.cut_id)
        contract.integer(self.source_index, upper=contract.MAX_REFERENCES - 1)
        decode_retained_source(self.source)


@dataclass(frozen=True, slots=True)
class ResolvedBankCut:
    cut: BankSourceCut
    witnesses: tuple[ConflictWitness, ...]
    events: tuple[contract.RewardEvent, ...]
    retained_snapshot_utc_usec: int
    evidence_digest: bytes
    reserved_bytes: int
    conflict_query_count: int
    elapsed_seconds: float
    coverage: str = "selected_bank_events_against_visible_sealed_retention"
    future_evidence_provisional: bool = True


@dataclass(frozen=True, slots=True)
class ResolvedBankBatch:
    cuts: tuple[ResolvedBankCut, ...]
    events: tuple[contract.RewardEvent, ...]
    retained_snapshot_utc_usec: int
    reserved_bytes: int
    conflict_query_count: int
    health: object | None = None


def apply_conflicts(events, witnesses):
    """Keep the original event key and every conflicting payload reference."""
    events = contract.reconcile_events(events)
    selected_sources = {(ref.table, ref.key) for event in events for ref in event.references}
    by_source = {}
    for witness in witnesses:
        contract.require(type(witness) is ConflictWitness, "canonical_conflict_witness_type")
        witness.validate()
        reference = witness.source.reference
        contract.require((reference.table, reference.key) in selected_sources,
                         "canonical_conflict_witness_scope")
        by_source.setdefault((reference.table, reference.key), set()).add(reference)
    result = []
    for event in events:
        references = set(event.references)
        for reference in event.references:
            references.update(by_source.get((reference.table, reference.key), ()))
        physical = {}
        for reference in references:
            physical.setdefault((reference.table, reference.key), set()).add(reference.payload_digest)
        conflict = any(len(digests) > 1 for digests in physical.values())
        result.append(replace(event, references=tuple(sorted(references)),
            disposition=contract.Disposition.CONFLICT if conflict else event.disposition,
            amount=None if conflict else event.amount))
    return contract.reconcile_events(result)


def evidence_digest(cut_id, snapshot_utc_usec, witnesses):
    contract.digest(cut_id)
    contract.integer(snapshot_utc_usec, lower=1)
    checksum = hashlib.sha256(b"duris-canonical-retained-conflicts-v2\0" + cut_id +
                              snapshot_utc_usec.to_bytes(8, "big"))
    for witness in witnesses:
        witness.validate()
        reference = witness.source.reference
        checksum.update(witness.cut_id + witness.source_index.to_bytes(4, "big") +
                        reference.table.encode("ascii") + b"\0" +
                        len(reference.key).to_bytes(2, "big") + reference.key + reference.payload_digest)
    return checksum.digest()


class CanonicalRewardConflictReader(CanonicalRewardRetention):
    """Bounded exact variant seeks; identical copies do not enlarge the result."""

    def __init__(self, connection_factory, *, conflict_query_limit=MAX_CONFLICT_LOOKUPS, **kwargs):
        super().__init__(connection_factory, **kwargs)
        contract.integer(conflict_query_limit, lower=1, upper=MAX_CONFLICT_LOOKUPS)
        self.conflict_query_limit = conflict_query_limit

    def resolve(self, cut_id):
        return self.resolve_many((cut_id,)).cuts[0]

    def resolve_many(self, cut_ids, *, scan_id=None):
        contract.require(type(cut_ids) is tuple and 0 < len(cut_ids) <= 256 and
                         cut_ids == tuple(sorted(set(cut_ids))), "canonical_conflict_cut_selection")
        for cut_id in cut_ids:
            contract.digest(cut_id)
        started = self.clock()
        self._deadline = started + self.time_limit_s
        try:
            self._start(read_only=True)
            if scan_id is not None:
                from .canonical_reward_reconciliation import JOURNAL_TABLES
                self._statement_count = 0
                engines = self._execute("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables "
                    "WHERE table_schema=DATABASE() AND table_name IN (%s,%s,%s)", JOURNAL_TABLES)
                contract.require(len(engines) == 3 and {row["table_name"] for row in engines} == set(JOURNAL_TABLES) and
                    all(row["engine"] == "InnoDB" for row in engines), "canonical_sweep_snapshot_engines")
            self._statement_count = 0
            label = self._execute("SELECT TIMESTAMPDIFF(MICROSECOND,'1970-01-01 00:00:00',UTC_TIMESTAMP(6)) "
                                  "AS captured_utc_usec")
            contract.require(len(label) == 1, "canonical_conflict_snapshot_label")
            captured = contract.integer(label[0]["captured_utc_usec"], lower=1)
            cuts, events, reserved, queries = [], [], 0, 0
            # Exact physical identity AND payload digest qualify reuse only in
            # this owning snapshot. Never keep negative lookups between reads.
            variants = {}
            for cut_id in cut_ids:
                resolved = self._resolve_in_transaction(cut_id, captured, started, reserved, queries, variants)
                reserved += resolved.reserved_bytes
                queries += resolved.conflict_query_count
                cuts.append(resolved)
                # Merge original authority before applying global witnesses.
                # Per-cut quarantine becomes CONFLICT after resolution; using
                # that derived event here can make its unavailable metadata
                # win over a valid cut solely because its digest sorts first.
                events.extend(resolved.cut.events)
                contract.require(len(events) <= contract.MAX_PAGE_SIZE, "canonical_conflict_batch_events")
            health = None
            if scan_id is not None:
                from .canonical_reward_health import capture_health_in_transaction
                health = capture_health_in_transaction(self, scan_id, snapshot=captured, prior_bytes=reserved)
                reserved += health.health.reserved_bytes
            witnesses = tuple(witness for resolved in cuts for witness in resolved.witnesses)
            result = ResolvedBankBatch(tuple(cuts), apply_conflicts(events, witnesses), captured, reserved, queries, health)
            self._check_deadline()
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def _resolve_in_transaction(self, cut_id, captured, started, prior_bytes, prior_queries, variants):
        witnesses, query_count = [], 0
        header = self._header(cut_id)
        contract.require(header is not None and header["sealed"] == 1,
                         "canonical_retained_sealed_cut_missing")
        cut = self._load_rows(header)
        reserved = cut.reserved_bytes
        if prior_bytes + reserved > self.byte_limit:
            raise ProjectionBoundsExceeded("canonical conflict byte budget")
        for source in cut.sources:
            reference = source.reference
            cache_key = reference.table, reference.key, reference.payload_digest
            source_witnesses = variants.get(cache_key)
            if source_witnesses is None:
                reserved += VARIANT_CACHE_ENTRY_BYTES
                if prior_bytes + reserved > self.byte_limit:
                    raise ProjectionBoundsExceeded("canonical conflict byte budget")
                source_witnesses, after = [], bytes(32)
                while True:
                    if prior_queries + query_count >= self.conflict_query_limit:
                        raise ProjectionBoundsExceeded("canonical conflict query budget")
                    self._statement_count = 0
                    # Seek past distinct digests; identical retained copies do
                    # not cause an unbounded duplicate scan or repeated seeks.
                    rows = self._execute("SELECT s.cut_id,s.source_index,s.payload,s.payload_digest "
                        "FROM " + SOURCE_TABLE + " s FORCE INDEX (idx_reward2_source_payloads) "
                        "JOIN " + CUT_TABLE + " c ON c.cut_id=s.cut_id "
                        "WHERE s.source_table=%s AND s.source_key=%s AND s.payload_digest>%s "
                        "AND s.payload_digest<>%s AND c.sealed=1 AND c.definition_version=2 "
                        "ORDER BY s.payload_digest,s.cut_id LIMIT 1",
                        (reference.table.encode("ascii"), reference.key, after, reference.payload_digest))
                    query_count += 1
                    contract.require(len(rows) <= 1, "canonical_conflict_variant_count")
                    if not rows:
                        break
                    row = rows[0]
                    digest = contract.digest(row["payload_digest"])
                    contract.require(digest > after and digest != reference.payload_digest,
                                     "canonical_conflict_variant_order")
                    retained = RetainedSource(contract.SourceReference(reference.table, reference.key, digest), row["payload"])
                    witness = ConflictWitness(row["cut_id"], row["source_index"], retained)
                    witness.validate()
                    source_witnesses.append(witness)
                    after = digest
                    # Reserve each variant before another payload can arrive.
                    reserved += max(INPUT_ROW_BYTE_BOUND, len(retained.payload) + len(reference.table) +
                                    len(reference.key) + 32 + 128)
                    if prior_bytes + reserved > self.byte_limit or len(source_witnesses) > contract.MAX_PAGE_SIZE:
                        raise ProjectionBoundsExceeded("canonical conflict byte or witness budget")
                source_witnesses = tuple(source_witnesses)
                variants[cache_key] = source_witnesses
            else:
                # Retained witness bindings still count against each selected
                # cut's reservation even when their SQL seek is shared.
                reserved += sum(max(INPUT_ROW_BYTE_BOUND, len(witness.source.payload) + len(reference.table) +
                    len(reference.key) + 32 + 128) for witness in source_witnesses)
                if prior_bytes + reserved > self.byte_limit:
                    raise ProjectionBoundsExceeded("canonical conflict byte budget")
            for witness in source_witnesses:
                contract.require(witness.cut_id != cut_id, "canonical_conflict_same_cut")
                if len(witnesses) >= contract.MAX_PAGE_SIZE:
                    raise ProjectionBoundsExceeded("canonical conflict witness budget")
                witnesses.append(witness)
        witnesses = tuple(witnesses)
        events = apply_conflicts(cut.events, witnesses)
        result = ResolvedBankCut(cut, witnesses, events, captured,
            evidence_digest(cut_id, captured, witnesses), reserved, query_count, self.clock() - started)
        self._check_deadline()
        return result
