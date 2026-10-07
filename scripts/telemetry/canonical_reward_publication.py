"""Independent reward-definition-2 generations over exact retained evidence.

Selection and conflict discovery share one owning snapshot. Sealed generation
bindings preserve that selected evidence; publication independently replays it.
Reports read only complete public coverage and exact event projections. Native
bank evidence supplies no qualified award date or historical human ownership.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib

from . import canonical_reward_contract as contract
from .canonical_reward_source import RetainedSource, replay_retained_bank_cut, INPUT_ROW_BYTE_BOUND, HEADER_BYTE_BOUND, DEFAULT_BYTE_LIMIT
from .canonical_reward_retention import CanonicalRewardRetention, CUT_TABLE, SOURCE_TABLE, SOURCE_COLUMNS
from .canonical_reward_conflicts import CanonicalRewardConflictReader, ConflictWitness, apply_conflicts
from .canonical_reward_health import replay_health, public_health_rows, decode_public_health, health_summary
from .reward_projection import ProjectionBoundsExceeded

GENERATION_TABLE = "telemetry_reward_generation_v2"
BINDING_TABLE = "telemetry_reward_binding_v2"
PRIVATE_EVENT_TABLE = "telemetry_reward_event_private_v2"
COVERAGE_TABLE = "telemetry_reward_coverage_v2"
EVENT_TABLE = "telemetry_reward_event_v2"
PRIVATE_HEALTH_TABLE = "telemetry_reward_health_private_v2"
HEALTH_TABLE = "telemetry_reward_health_v2"
PRIVATE_TABLES = (GENERATION_TABLE, BINDING_TABLE, PRIVATE_EVENT_TABLE, PRIVATE_HEALTH_TABLE)
PUBLIC_TABLES = (COVERAGE_TABLE, EVENT_TABLE, HEALTH_TABLE)
GENERATION_COLUMNS = ("generation_id", "definition_version", "snapshot_utc_usec", "cut_count", "binding_count",
    "event_count", "health_count", "reserved_bytes", "evidence_digest", "projection_digest",
    "health_evidence_digest", "health_projection_digest", "coverage_payload", "coverage_digest", "phase")
BINDING_COLUMNS = ("binding_index", "binding_kind", "cut_id", "source_index", "payload_digest")
EVENT_COLUMNS = ("event_index", "payload", "payload_digest")
HEALTH_COLUMNS = ("health_index", "payload", "payload_digest")
PRIVATE_HEALTH_COLUMNS = (*HEALTH_COLUMNS, "projection_payload", "projection_digest")
PUBLIC_COLUMNS = tuple(name for name in GENERATION_COLUMNS if name not in ("cut_count", "reserved_bytes", "phase")) + ("complete",)
EVENT_FIELDS = ("unit", "operation_id", "participant_pid", "event_index", "item_uid", "disposition", "amount",
    "authority", "reference_count", "reference_digest", "lineage", "wallet_lifetime", "award_utc_usec",
    "account_token", "controller_token", "identity_registry_version", "attribution_status")


@dataclass(frozen=True, slots=True)
class RewardGeneration:
    generation_id: bytes
    snapshot_utc_usec: int
    cuts: tuple
    witnesses: tuple[ConflictWitness, ...]
    bindings: tuple[dict, ...]
    events: tuple[contract.RewardEvent, ...]
    rows: tuple[dict, ...]
    health_payloads: tuple[bytes, ...]
    health_rows: tuple[dict, ...]
    header: dict


def reference_digest(references):
    checksum = hashlib.sha256(b"duris-canonical-reward-references-v2\0")
    for reference in references:
        reference.validate()
        checksum.update(reference.table.encode("ascii") + b"\0" +
            len(reference.key).to_bytes(2, "big") + reference.key + reference.payload_digest)
    return checksum.digest()


def event_payload(event):
    event.validate()
    key = event.identity
    return contract.encode_source_payload(dict(unit=int(key.unit), operation_id=key.operation_id,
        participant_pid=key.participant_pid, event_index=key.event_index, item_uid=key.item_uid,
        disposition=int(event.disposition), amount=event.amount, authority=event.authority,
        reference_count=len(event.references), reference_digest=reference_digest(event.references),
        lineage=event.lineage, wallet_lifetime=event.wallet_lifetime,
        award_utc_usec=None, account_token=None, controller_token=None, identity_registry_version=None,
        attribution_status="unknown"))


def decode_event(payload):
    row = contract.decode_source_payload(payload)
    contract.require(set(row) == set(EVENT_FIELDS), "canonical_public_event_columns")
    key = contract.EventIdentity(contract.Unit(contract.integer(row["unit"], lower=1, upper=5)),
        row["operation_id"], row["participant_pid"], row["event_index"], row["item_uid"])
    key.validate()
    kind = contract.Disposition(contract.integer(row["disposition"], lower=1, upper=10))
    if kind in (contract.Disposition.UNKNOWN, contract.Disposition.CONFLICT):
        contract.require(row["amount"] is None, "canonical_public_unknown_amount")
    else:
        contract.integer(row["amount"], lower=-(1 << 63), upper=(1 << 63) - 1)
    contract.require(type(row["authority"]) is str and 0 < len(row["authority"]) <= 64,
                     "canonical_public_authority")
    if kind == contract.Disposition.EARNED:
        contract.require(row["amount"] > 0 and row["authority"] != "unavailable" and
            key.unit != contract.Unit.OBSERVED_XP, "canonical_public_earned_authority")
    contract.integer(row["reference_count"], lower=1, upper=contract.MAX_PAGE_SIZE)
    contract.digest(row["reference_digest"])
    if row["lineage"] is not None:
        contract.identifier(row["lineage"])
    if row["wallet_lifetime"] is not None:
        contract.integer(row["wallet_lifetime"], lower=1)
    contract.require(all(row[name] is None for name in ("award_utc_usec", "account_token", "controller_token", "identity_registry_version"))
        and row["attribution_status"] == "unknown", "canonical_public_unqualified_attribution")
    return row


def projection_digest(generation_id, rows):
    checksum = hashlib.sha256(b"duris-canonical-reward-projection-v2\0" + contract.identifier(generation_id))
    for index, row in enumerate(rows):
        contract.require(row["event_index"] == index and hashlib.sha256(row["payload"]).digest() == row["payload_digest"],
                         "canonical_generation_event_digest")
        decode_event(row["payload"])
        checksum.update(index.to_bytes(4, "big") + row["payload_digest"])
    return checksum.digest()


def health_digest(generation_id, payloads, *, private):
    domain = b"duris-canonical-reward-health-evidence-v2\0" if private else b"duris-canonical-reward-health-projection-v2\0"
    checksum = hashlib.sha256(domain + contract.identifier(generation_id))
    contract.require(type(payloads) is tuple and len(payloads) in (0, 257), "canonical_generation_health_count")
    for index, payload in enumerate(payloads):
        contract.decode_source_payload(payload)
        checksum.update(index.to_bytes(4, "big") + hashlib.sha256(payload).digest())
    return checksum.digest()


def evidence_digest(generation_id, snapshot, bindings, health_payloads=()):
    checksum = hashlib.sha256(b"duris-canonical-reward-generation-v2\0" + contract.identifier(generation_id) +
        contract.integer(snapshot, lower=1).to_bytes(8, "big"))
    for index, row in enumerate(bindings):
        contract.require(row["binding_index"] == index, "canonical_generation_binding_index")
        checksum.update(contract.encode_source_payload(row))
    if health_payloads:
        checksum.update(health_digest(generation_id, health_payloads, private=True))
    return checksum.digest()


def make_generation(generation_id, snapshot, cuts, witnesses, *, health_payloads=(), byte_limit=DEFAULT_BYTE_LIMIT):
    contract.identifier(generation_id)
    contract.integer(snapshot, lower=1)
    contract.integer(byte_limit, lower=1, upper=DEFAULT_BYTE_LIMIT)
    contract.require(type(cuts) is tuple and 0 < len(cuts) <= 256 and
        tuple(cut.source_digest for cut in cuts) == tuple(sorted({cut.source_digest for cut in cuts})),
        "canonical_generation_cut_selection")
    witnesses = tuple(sorted(set(witnesses), key=lambda witness: (witness.cut_id, witness.source_index, witness.source.reference)))
    contract.require(len(witnesses) + len(cuts) <= contract.MAX_REFERENCES, "canonical_generation_binding_capacity")
    events, bindings = [], []
    reserved = HEADER_BYTE_BOUND
    for cut in cuts:
        reserved += cut.reserved_bytes
        if reserved > byte_limit:
            raise ProjectionBoundsExceeded("canonical generation source budget")
        restored = replay_retained_bank_cut(cut.captured_utc_usec, cut.selected_operations, cut.sources,
            cut.source_digest, strict=False, discovery=cut.discovery, byte_limit=byte_limit)
        contract.require(restored.events == cut.events and restored.reserved_bytes == cut.reserved_bytes and
                         restored.coverage == cut.coverage,
                         "canonical_generation_cut_disagreement")
        events.extend(restored.events)
        bindings.append(dict(binding_index=len(bindings), binding_kind=1, cut_id=cut.source_digest,
                             source_index=None, payload_digest=cut.source_digest))
    for witness in witnesses:
        witness.validate()
        reserved += INPUT_ROW_BYTE_BOUND
        if reserved > byte_limit:
            raise ProjectionBoundsExceeded("canonical generation witness budget")
        bindings.append(dict(binding_index=len(bindings), binding_kind=2, cut_id=witness.cut_id,
                             source_index=witness.source_index, payload_digest=witness.source.reference.payload_digest))
    events = apply_conflicts(events, witnesses)
    reserved += len(bindings) * 256 + len(events) * 2 * INPUT_ROW_BYTE_BOUND + HEADER_BYTE_BOUND
    if reserved > byte_limit:
        raise ProjectionBoundsExceeded("canonical generation projection budget")
    has_nonbank = any(cut.coverage == "selected_native_economic_operations" for cut in cuts)
    has_coin = has_auction = False
    for cut in cuts:
        selected = set(cut.selected_operations)
        for source in cut.sources:
            if source.reference.table == "economic_accounting_operation" and source.reference.key in selected:
                root = contract.decode_source_payload(source.payload)
                has_coin |= root.get("writer_id") == 5 and root.get("reason") == 3
                has_auction |= root.get("writer_id") == 13 and root.get("reason") == 31
    contract.require(not health_payloads or not has_nonbank, "canonical_coin_history_health_unavailable")
    health_rows, public_health = (), None
    if health_payloads:
        retained = replay_health(health_payloads, snapshot=snapshot, byte_limit=byte_limit)
        public_payloads = public_health_rows(retained)
        public_health = decode_public_health(public_payloads, snapshot=snapshot)
        health_rows = tuple(dict(health_index=index, payload=payload, payload_digest=hashlib.sha256(payload).digest())
                            for index, payload in enumerate(public_payloads))
        reserved += retained.health.reserved_bytes + len(health_rows) * INPUT_ROW_BYTE_BOUND
        if reserved > byte_limit:
            raise ProjectionBoundsExceeded("canonical generation health budget")
    contract.require(type(health_payloads) is tuple and len(health_payloads) in (0, 257), "canonical_generation_health_count")
    rows = []
    for index, event in enumerate(events):
        payload = event_payload(event)
        rows.append(dict(event_index=index, payload=payload, payload_digest=hashlib.sha256(payload).digest()))
    rows = tuple(rows)
    totals = contract.earned_totals(events)
    for total in totals.values():
        contract.integer(total, lower=1, upper=(1 << 63) - 1)
    coverage = contract.encode_source_payload(dict(generation_id=generation_id, definition_version=2,
        snapshot_utc_usec=snapshot, coverage="selected_native_economic_events_against_visible_sealed_retention" if has_nonbank else
        "selected_native_bank_events_against_visible_sealed_retention",
        cut_count=len(cuts), binding_count=len(bindings), event_count=len(events),
        earned_count=sum(event.disposition == contract.Disposition.EARNED for event in events),
        unknown_count=sum(event.disposition in (contract.Disposition.UNKNOWN, contract.Disposition.CONFLICT) for event in events),
        opening_count=sum(event.disposition == contract.Disposition.OPENING for event in events),
        transfer_count=sum(event.disposition == contract.Disposition.TRANSFER for event in events),
        currency_earned_copper=totals.get(contract.Unit.CURRENCY),
        source_min_capture_utc_usec=min(cut.captured_utc_usec for cut in cuts),
        source_max_capture_utc_usec=max(cut.captured_utc_usec for cut in cuts),
        source_backlog=None, source_retention_floor=None, retention_acknowledged=0, future_commits_provisional=1,
        award_date_unknown_count=len(events), account_unknown_count=len(events), controller_unknown_count=len(events),
        supported_routes="native_sql_bank_deposit,withdraw,starter,quest" + (",native_coin_wallet_pile" if has_coin else "") +
            (",native_auction_money_claim_settlement" if has_auction else ""),
        unsupported_routes=("" if has_coin else "wallet_pile,") +
            ("auction_other_origins,auction_item_claim," if has_auction else "auction,") + "other_accounting_owners",
        unsupported_units="epic,frag,equipment,committed_xp", flatfile_authority="unavailable",
        health_count=len(health_payloads), **health_summary(public_health)))
    header = dict(generation_id=generation_id, definition_version=2, snapshot_utc_usec=snapshot,
        cut_count=len(cuts), binding_count=len(bindings), event_count=len(rows), health_count=len(health_payloads), reserved_bytes=reserved,
        evidence_digest=evidence_digest(generation_id, snapshot, bindings, health_payloads), projection_digest=projection_digest(generation_id, rows),
        health_evidence_digest=health_digest(generation_id, health_payloads, private=True),
        health_projection_digest=health_digest(generation_id, tuple(row["payload"] for row in health_rows), private=False),
        coverage_payload=coverage, coverage_digest=hashlib.sha256(coverage).digest(), phase=1)
    return RewardGeneration(generation_id, snapshot, cuts, witnesses, tuple(bindings), events, rows, health_payloads, health_rows, header)


class CanonicalRewardPublisher(CanonicalRewardRetention):
    def _query(self, statement, parameters=()):
        self._statement_count = 0
        return self._execute(statement, parameters)

    def _start(self, *, read_only):
        super()._start(read_only=read_only)
        tables = (*PRIVATE_TABLES, *PUBLIC_TABLES)
        for start in range(0, len(tables), 3):
            selected = tables[start:start + 3]
            rows = self._query("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables WHERE table_schema=DATABASE() "
                "AND table_name IN (" + ",".join("%s" for _ in selected) + ")", selected)
            contract.require(len(rows) == len(selected) and {row["table_name"] for row in rows} == set(selected) and
                all(row["engine"] == "InnoDB" for row in rows), "canonical_publication_snapshot_engines")

    def _generation_header(self, generation_id, *, lock=False):
        rows = self._query("SELECT " + ",".join(GENERATION_COLUMNS) + " FROM " + GENERATION_TABLE +
            " WHERE generation_id=%s" + (" FOR UPDATE" if lock else ""), (generation_id,))
        contract.require(len(rows) <= 1, "canonical_generation_header_count")
        if not rows:
            return None
        header = rows[0]
        contract.require(header["generation_id"] == generation_id and header["definition_version"] == 2 and header["phase"] in (1, 2),
                         "canonical_generation_header_identity")
        contract.integer(header["cut_count"], lower=1, upper=256)
        contract.integer(header["binding_count"], lower=header["cut_count"], upper=contract.MAX_REFERENCES)
        contract.integer(header["event_count"], upper=contract.MAX_PAGE_SIZE)
        contract.require(header["health_count"] in (0, 257), "canonical_generation_health_count")
        contract.integer(header["reserved_bytes"], lower=HEADER_BYTE_BOUND, upper=DEFAULT_BYTE_LIMIT)
        if header["reserved_bytes"] > self.byte_limit:
            raise ProjectionBoundsExceeded("canonical generation retained budget")
        return header

    def _rows(self, table, generation_id, columns, count):
        result = []
        index = columns[0]
        for start in range(0, count + 1, self.page_size):
            rows = self._query("SELECT " + ",".join(columns) + " FROM " + table + " WHERE generation_id=%s AND " +
                index + ">=%s ORDER BY " + index + " LIMIT %s", (generation_id, start, min(self.page_size, count + 1 - start)))
            contract.require(len(rows) == max(0, min(self.page_size, count - start)), "canonical_generation_row_count")
            contract.require(all(row[index] == offset for offset, row in enumerate(rows, start)), "canonical_generation_row_order")
            result.extend(rows)
        return tuple(result)

    def _restore(self, header):
        bindings = self._rows(BINDING_TABLE, header["generation_id"], BINDING_COLUMNS, header["binding_count"])
        cuts, witnesses, reserved = [], [], HEADER_BYTE_BOUND
        for binding in bindings:
            cut_id = contract.digest(binding["cut_id"])
            if binding["binding_kind"] == 1:
                contract.require(binding["source_index"] is None and binding["payload_digest"] == cut_id,
                                 "canonical_generation_cut_binding")
                cut_header = self._header(cut_id)
                contract.require(cut_header is not None and cut_header["sealed"] == 1, "canonical_generation_cut_missing")
                reserved += cut_header["reserved_bytes"]
                if reserved > self.byte_limit:
                    raise ProjectionBoundsExceeded("canonical generation source budget")
                cuts.append(self._load_rows(cut_header))
            else:
                contract.require(binding["binding_kind"] == 2, "canonical_generation_binding_kind")
                source_index = contract.integer(binding["source_index"], upper=contract.MAX_REFERENCES - 1)
                reserved += INPUT_ROW_BYTE_BOUND
                if reserved > self.byte_limit:
                    raise ProjectionBoundsExceeded("canonical generation witness budget")
                witness_header = self._header(cut_id)
                contract.require(witness_header is not None and witness_header["sealed"] == 1,
                                 "canonical_generation_witness_cut_missing")
                rows = self._query("SELECT " + ",".join(SOURCE_COLUMNS) + " FROM " + SOURCE_TABLE +
                    " WHERE cut_id=%s AND source_index=%s", (cut_id, source_index))
                contract.require(len(rows) == 1 and rows[0]["payload_digest"] == binding["payload_digest"],
                                 "canonical_generation_witness_missing")
                row = rows[0]
                name = row["source_table"]
                contract.require(type(name) is bytes and name.isascii(), "canonical_generation_witness_table")
                reference = contract.SourceReference(name.decode("ascii"), row["source_key"], row["payload_digest"])
                witnesses.append(ConflictWitness(cut_id, source_index, RetainedSource(reference, row["payload"])))
        private_health = self._rows(PRIVATE_HEALTH_TABLE, header["generation_id"], PRIVATE_HEALTH_COLUMNS, header["health_count"])
        for row in private_health:
            contract.require(hashlib.sha256(row["payload"]).digest() == row["payload_digest"], "canonical_generation_health_payload_digest")
        plan = make_generation(header["generation_id"], header["snapshot_utc_usec"], tuple(cuts), tuple(witnesses),
            health_payloads=tuple(row["payload"] for row in private_health), byte_limit=self.byte_limit)
        contract.require(plan.bindings == bindings and all(plan.header[name] == header[name] for name in GENERATION_COLUMNS if name != "phase"),
                         "canonical_generation_retained_derivation")
        contract.require(all(row["projection_payload"] == projected["payload"] and row["projection_digest"] == projected["payload_digest"]
            for row, projected in zip(private_health, plan.health_rows)), "canonical_generation_health_derivation")
        rows = self._rows(PRIVATE_EVENT_TABLE, plan.generation_id, EVENT_COLUMNS, header["event_count"])
        contract.require(plan.rows == rows, "canonical_generation_projection_disagreement")
        self._check_deadline()
        return plan

    def load(self, generation_id):
        contract.identifier(generation_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            header = self._generation_header(generation_id)
            result = None if header is None else self._restore(header)
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def stage(self, generation_id, cut_ids, *, scan_id=None):
        contract.identifier(generation_id)
        contract.require(type(cut_ids) is tuple and cut_ids == tuple(sorted(set(cut_ids))), "canonical_generation_cut_selection")
        deadline = self.clock() + self.time_limit_s
        existing = self.load(generation_id)
        if existing is not None:
            contract.require(tuple(cut.source_digest for cut in existing.cuts) == cut_ids, "canonical_generation_replay_selection")
            self._check_scan_selection(existing, scan_id)
            return existing
        remaining = deadline - self.clock()
        if remaining <= 0:
            raise ProjectionBoundsExceeded("canonical generation staging deadline")
        batch = CanonicalRewardConflictReader(self.connection_factory, byte_limit=self.byte_limit,
            time_limit_s=remaining, clock=self.clock).resolve_many(cut_ids, scan_id=scan_id)
        plan = make_generation(generation_id, batch.retained_snapshot_utc_usec,
            tuple(resolved.cut for resolved in batch.cuts), tuple(witness for resolved in batch.cuts for witness in resolved.witnesses),
            health_payloads=() if batch.health is None else batch.health.payloads, byte_limit=self.byte_limit)
        contract.require(plan.events == batch.events, "canonical_generation_snapshot_derivation")
        self._deadline = deadline
        try:
            self._start(read_only=False)
            header = self._generation_header(generation_id, lock=True)
            if header is not None:
                original = self._restore(header)
                contract.require(tuple(cut.source_digest for cut in original.cuts) == cut_ids, "canonical_generation_replay_selection")
                self._check_scan_selection(original, scan_id)
                self._rollback()
                return original
            self._query("INSERT INTO " + GENERATION_TABLE + " (" + ",".join(GENERATION_COLUMNS) + ") VALUES (" +
                ",".join("%s" for _ in GENERATION_COLUMNS) + ")", tuple(0 if name == "phase" else plan.header[name] for name in GENERATION_COLUMNS))
            self._bulk(BINDING_TABLE, ("generation_id", *BINDING_COLUMNS),
                [tuple([generation_id] + [row[name] for name in BINDING_COLUMNS]) for row in plan.bindings])
            self._bulk(PRIVATE_EVENT_TABLE, ("generation_id", *EVENT_COLUMNS),
                [tuple([generation_id] + [row[name] for name in EVENT_COLUMNS]) for row in plan.rows])
            self._bulk(PRIVATE_HEALTH_TABLE, ("generation_id", *PRIVATE_HEALTH_COLUMNS),
                [(generation_id, index, payload, hashlib.sha256(payload).digest(),
                  plan.health_rows[index]["payload"], plan.health_rows[index]["payload_digest"])
                 for index, payload in enumerate(plan.health_payloads)])
            self._query("UPDATE " + GENERATION_TABLE + " SET phase=1 WHERE generation_id=%s AND phase=0", (generation_id,))
            contract.require(self._restore(self._generation_header(generation_id)) == plan, "canonical_generation_staging_disagreement")
            self._commit()
            return plan
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    @staticmethod
    def _check_scan_selection(plan, scan_id):
        retained_scan = None if not plan.health_payloads else contract.decode_source_payload(plan.health_payloads[0])["scan_id"]
        contract.require(retained_scan == scan_id, "canonical_generation_replay_scan_selection")

    def publish(self, generation_id):
        contract.identifier(generation_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=False)
            header = self._generation_header(generation_id, lock=True)
            contract.require(header is not None, "canonical_generation_missing")
            plan = self._restore(header)
            if header["phase"] == 2:
                result = self._read_public(generation_id)
                self._rollback()
                return result
            self._query("INSERT INTO " + COVERAGE_TABLE + " (" + ",".join(PUBLIC_COLUMNS) + ") VALUES (" +
                ",".join("%s" for _ in PUBLIC_COLUMNS) + ")", tuple(0 if name == "complete" else plan.header[name] for name in PUBLIC_COLUMNS))
            self._bulk(EVENT_TABLE, ("generation_id", *EVENT_COLUMNS),
                [tuple([generation_id] + [row[name] for name in EVENT_COLUMNS]) for row in plan.rows])
            self._bulk(HEALTH_TABLE, ("generation_id", *HEALTH_COLUMNS),
                [tuple([generation_id] + [row[name] for name in HEALTH_COLUMNS]) for row in plan.health_rows])
            self._query("UPDATE " + GENERATION_TABLE + " SET phase=2 WHERE generation_id=%s AND phase=1", (generation_id,))
            self._query("UPDATE " + COVERAGE_TABLE + " SET complete=1 WHERE generation_id=%s AND complete=0", (generation_id,))
            result = self._read_public(generation_id)
            self._commit()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()

    def _read_public(self, generation_id):
        rows = self._query("SELECT " + ",".join(PUBLIC_COLUMNS) + " FROM " + COVERAGE_TABLE +
            " WHERE generation_id=%s AND complete=1", (generation_id,))
        contract.require(len(rows) == 1, "canonical_public_generation_unavailable")
        header = rows[0]
        contract.require(header["generation_id"] == generation_id and header["definition_version"] == 2 and
            hashlib.sha256(header["coverage_payload"]).digest() == header["coverage_digest"], "canonical_public_coverage_digest")
        count = contract.integer(header["event_count"], upper=contract.MAX_PAGE_SIZE)
        health_count = header["health_count"]
        contract.require(health_count in (0, 257), "canonical_public_health_count")
        if HEADER_BYTE_BOUND + (count + health_count) * INPUT_ROW_BYTE_BOUND > self.byte_limit:
            raise ProjectionBoundsExceeded("canonical public report budget")
        events = self._rows(EVENT_TABLE, generation_id, EVENT_COLUMNS, count)
        contract.require(projection_digest(generation_id, events) == header["projection_digest"], "canonical_public_projection_digest")
        coverage = contract.decode_source_payload(header["coverage_payload"])
        health_rows = self._rows(HEALTH_TABLE, generation_id, HEALTH_COLUMNS, health_count)
        for row in health_rows:
            contract.require(hashlib.sha256(row["payload"]).digest() == row["payload_digest"], "canonical_public_health_payload_digest")
        payloads = tuple(row["payload"] for row in health_rows)
        contract.require(health_digest(generation_id, payloads, private=False) == header["health_projection_digest"],
                         "canonical_public_health_projection_digest")
        public_health = None if not payloads else decode_public_health(payloads, snapshot=header["snapshot_utc_usec"])
        contract.require(all(coverage[name] == value for name, value in health_summary(public_health).items()),
                         "canonical_public_health_summary")
        contract.require(all(coverage[name] == header[name] for name in ("generation_id", "definition_version", "snapshot_utc_usec", "binding_count", "event_count", "health_count")),
                         "canonical_public_coverage_fields")
        decoded = tuple(decode_event(row["payload"]) for row in events)
        contract.require(coverage["earned_count"] == sum(row["disposition"] == int(contract.Disposition.EARNED) for row in decoded) and
            coverage["unknown_count"] == sum(row["disposition"] in (int(contract.Disposition.UNKNOWN), int(contract.Disposition.CONFLICT)) for row in decoded),
            "canonical_public_coverage_counts")
        contract.require(coverage["opening_count"] == sum(row["disposition"] == int(contract.Disposition.OPENING) for row in decoded) and
            coverage["transfer_count"] == sum(row["disposition"] == int(contract.Disposition.TRANSFER) for row in decoded),
            "canonical_public_custody_counts")
        totals = {}
        for row in decoded:
            if row["disposition"] == int(contract.Disposition.EARNED):
                totals[row["unit"]] = totals.get(row["unit"], 0) + row["amount"]
        for total in totals.values():
            contract.integer(total, lower=1, upper=(1 << 63) - 1)
        contract.require(coverage["currency_earned_copper"] == totals.get(int(contract.Unit.CURRENCY)) and
            coverage["award_date_unknown_count"] == coverage["account_unknown_count"] == coverage["controller_unknown_count"] == count,
            "canonical_public_amount_attribution_counts")
        contract.require(coverage["future_commits_provisional"] == 1 and coverage["retention_acknowledged"] == 0 and
            coverage["source_backlog"] is None and coverage["source_retention_floor"] is None and
            coverage["coverage"] in ("selected_native_bank_events_against_visible_sealed_retention",
                                      "selected_native_economic_events_against_visible_sealed_retention"),
            "canonical_public_provisional_coverage")
        self._check_deadline()
        return dict(coverage=coverage, events=decoded, health=public_health,
            evidence_digest=header["evidence_digest"], projection_digest=header["projection_digest"],
            health_evidence_digest=header["health_evidence_digest"], health_projection_digest=header["health_projection_digest"])


class CanonicalRewardReporter(CanonicalRewardPublisher):
    """The read path owns only complete public reward and coverage projections."""
    def _start(self, *, read_only):
        contract.require(read_only, "canonical_report_read_only")
        connection = self._ensure_connection()
        connection.rollback()
        self._query("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        self._query("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        rows = self._query("SELECT table_name AS table_name,engine AS engine FROM information_schema.tables WHERE table_schema=DATABASE() "
            "AND table_name IN (%s,%s,%s)", PUBLIC_TABLES)
        contract.require(len(rows) == len(PUBLIC_TABLES) and {row["table_name"] for row in rows} == set(PUBLIC_TABLES) and
            all(row["engine"] == "InnoDB" for row in rows), "canonical_report_snapshot_engines")

    def read(self, generation_id):
        contract.identifier(generation_id)
        self._deadline = self.clock() + self.time_limit_s
        try:
            self._start(read_only=True)
            result = self._read_public(generation_id)
            self._rollback()
            return result
        except Exception:
            self._rollback()
            raise
        finally:
            self._deadline = None
            self.close()
