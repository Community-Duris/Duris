"""Independent canonical reward semantics for the external projection worker.

The v1 projection remains unchanged. Binary inspection is bounded and lossless;
it does not confer accounting authority. A supported bank receipt must also
agree with the committed inbox, immutable intent, source claim, compatibility
ledger and every indexed effect/posting. Other native routes stay unavailable
until their specific authority boundary is qualified.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
from enum import IntEnum
import hashlib
import json
import struct
from typing import Iterable, Mapping, Sequence

from .reward_projection_definitions import MAX_PAGE_SIZE, currency_value

DEFINITION_VERSION = 2
MAX_PLAN_BYTES = 4_194_304
MAX_INTENT_BYTES = 8_192
MAX_SOURCE_RECORD_BYTES = 8_192
MAX_REFERENCES = 16_384
ZERO_ID = bytes(16)
COINS = ("copper", "silver", "gold", "platinum")
COUNTS = ("account_count", "posting_count", "child_count", "before_witness_count",
          "after_witness_count", "item_event_count")
COUNT_LIMITS = (3_072, 6_144, 64, 6_000, 6_000, 3_000)
ROW_BYTES = (120, 48, 32, 64, 64, 128)


class EvidenceError(ValueError):
    """A payload-free refusal; the caller retains the exact conflicting inputs."""


def require(condition, reason):
    if not condition:
        raise EvidenceError(reason)


def integer(value, *, lower=0, upper=(1 << 64) - 1):
    require(type(value) is int and lower <= value <= upper, "canonical_integer_range")
    return value


def identifier(value, *, zero=False):
    require(type(value) is bytes and len(value) == 16 and (zero or value != ZERO_ID),
            "canonical_operation_identity")
    return value


def digest(value):
    require(type(value) is bytes and len(value) == 32 and any(value), "canonical_digest_shape")
    return value


def _zeros(raw, *ranges):
    require(all(not any(raw[start:end]) for start, end in ranges), "canonical_reserved_bytes")


def _source(raw):
    require(len(raw) == 48, "canonical_source_size")
    kind, version = struct.unpack_from("<HH", raw)
    require(version == 1 and 1 <= kind <= 23, "canonical_source_version_or_kind")
    identifier(raw[4:20]); identifier(raw[20:36])
    return kind, raw[4:20], raw[20:36], *struct.unpack_from("<QI", raw, 36)


@dataclass(frozen=True, slots=True)
class AccountEffect:
    key: bytes
    kind: int
    authority_id: int
    context_id: int
    before: tuple[int, ...]
    after: tuple[int, ...]
    before_revision: int
    after_revision: int

    @property
    def delta(self):
        return tuple(after - before for before, after in zip(self.before, self.after, strict=True))


@dataclass(frozen=True, slots=True)
class CoinPosting:
    event_index: int
    account_index: int
    child_index: int
    delta: tuple[int, ...]
    copper_value: int


@dataclass(frozen=True, slots=True)
class ChildLink:
    operation_id: bytes
    domain_id: int
    discriminator: int
    parent_index: int
    relationship: int


@dataclass(frozen=True, slots=True)
class Plan:
    raw: bytes
    metadata: Mapping[str, object]
    accounts: tuple[AccountEffect, ...]
    postings: tuple[CoinPosting, ...]
    children: tuple[ChildLink, ...]
    # Exact native rows, not equipment scores or independently qualified custody.
    items_before: tuple[bytes, ...]
    items_after: tuple[bytes, ...]
    item_events: tuple[bytes, ...]


def decode_plan(raw: bytes) -> Plan:
    """Inspect EAP1 without allocating from unverified counts or granting authority.

    This checks canonical framing, keys, coin conservation and derived child
    references. Item rows remain exact bytes; their domain authority is separate.
    It deliberately does not replace the native plan/intent capability verifier.
    """
    require(type(raw) is bytes and 256 <= len(raw) <= MAX_PLAN_BYTES, "canonical_plan_size")
    require(raw[:4] == b"EAP1" and struct.unpack_from("<H", raw, 4)[0] == 1,
            "canonical_plan_version")
    _zeros(raw, (6, 8), (73, 76), (98, 100), (101, 104), (240, 256))
    lineage, epoch, operation, original = (raw[start:start + 16] for start in (8, 24, 40, 56))
    identifier(lineage); identifier(epoch); identifier(operation); identifier(original, zero=True)
    require(original != operation, "canonical_original_identity")
    actor, writer, policy, compiler, reason = struct.unpack_from("<QIIIH", raw, 76)
    require(raw[72] in (1, 2) and actor and writer and policy == compiler == 1 and
            1 <= reason <= 46, "canonical_plan_metadata")
    require(raw[100] in (0, 1), "canonical_source_presence")
    source = raw[104:152] if raw[100] else None
    if source is None:
        _zeros(raw, (104, 152))
    else:
        _source(source)
    digest(raw[152:184]); digest(raw[184:216])
    counts = struct.unpack_from("<6I", raw, 216)
    require(all(n <= maximum for n, maximum in zip(counts, COUNT_LIMITS, strict=True)),
            "canonical_plan_capacity")
    require(len(raw) == 256 + sum(n * size for n, size in zip(counts, ROW_BYTES, strict=True)),
            "canonical_plan_exact_length")
    metadata = dict(operation_id=operation, lineage=lineage, epoch=epoch,
                    original_operation_id=None if original == ZERO_ID else original,
                    accounting_version=1, actor_kind=raw[72], actor_id=actor, writer_id=writer,
                    policy_version=policy, compiler_version=compiler, reason=reason,
                    source_event=source, intent_digest=raw[152:184], domain_digest=raw[184:216],
                    **dict(zip(COUNTS, counts, strict=True)))
    offset = 256
    accounts = []
    for _ in range(counts[0]):
        key = raw[offset:offset + 40]
        version, kind, authority, context = struct.unpack_from("<HHQQ", key, 16)
        require(key[:16] == lineage and version == 1 and 1 <= kind <= 11 and authority,
                "canonical_account_key")
        _zeros(key, (36, 40))
        values = struct.unpack_from("<8q2Q", raw, offset + 40)
        account = AccountEffect(key, kind, authority, context, values[:4], values[4:8], *values[8:])
        if kind in (1, 2, 3, 4, 5, 6, 11):
            require(all(value >= 0 for value in (*account.before, *account.after)),
                    "canonical_negative_holding")
            integer(currency_value(account.before), upper=(1 << 63) - 1)
            integer(currency_value(account.after), upper=(1 << 63) - 1)
            require(account.after_revision >= account.before_revision and
                    (account.before == account.after or account.after_revision > account.before_revision),
                    "canonical_stale_revision")
        else:
            require(not any((*values,)), "canonical_special_account_state")
        if accounts:
            prior = accounts[-1]
            require((prior.kind, prior.authority_id, prior.context_id) < (kind, authority, context),
                    "canonical_account_order")
        accounts.append(account)
        offset += 120
    postings = []
    totals = [[0] * 4 for _ in accounts]
    referenced = set()
    for index in range(counts[1]):
        event, account, child, *values = struct.unpack_from("<IHH5q", raw, offset)
        require(event == index and account < len(accounts) and child <= counts[2] and any(values[:4]),
                "canonical_posting_reference")
        require(currency_value(values[:4]) == values[4], "canonical_posting_value")
        postings.append(CoinPosting(event, account, child, tuple(values[:4]), values[4]))
        referenced.add(account)
        totals[account] = [a + b for a, b in zip(totals[account], values[:4], strict=True)]
        offset += 48
    require(sum(row.copper_value for row in postings) == 0, "canonical_unbalanced_plan")
    for index, account in enumerate(accounts):
        ordinary = account.kind in (1, 2, 3, 4, 5, 6, 11)
        require(index in referenced or (ordinary and account.before == account.after and
                                       account.after_revision > account.before_revision),
                "canonical_unreferenced_account")
        require(not ordinary or account.delta == tuple(totals[index]), "canonical_effect_delta")
    children = []
    for index in range(counts[2]):
        child_id = identifier(raw[offset:offset + 16])
        domain, discriminator, parent, relationship = struct.unpack_from("<IQHH", raw, offset + 16)
        require(domain and parent <= index and relationship == 1 and child_id != operation,
                "canonical_child_reference")
        parent_id = children[parent - 1].operation_id if parent else operation
        expected = hashlib.sha256(parent_id + struct.pack("<IQ", domain, discriminator)).digest()[:16]
        require(child_id == expected and all(row.operation_id != child_id for row in children),
                "canonical_child_identity")
        children.append(ChildLink(child_id, domain, discriminator, parent, relationship))
        offset += 32
    # Verify native canonical topological order, independently of host byte order.
    for index, child in enumerate(children):
        eligible = [row.operation_id for row in children[index:] if row.parent_index <= index]
        require(child.operation_id == min(eligible), "canonical_child_order")
    item_families = []
    for count, size in zip(counts[3:], ROW_BYTES[3:], strict=True):
        rows = tuple(raw[start:start + size] for start in range(offset, offset + count * size, size))
        item_families.append(rows)
        offset += count * size
    return Plan(raw, metadata, tuple(accounts), tuple(postings), tuple(children), *item_families)


class Unit(IntEnum):
    CURRENCY = 1
    EPIC = 2
    FRAG = 3
    EQUIPMENT = 4
    OBSERVED_XP = 5


class Disposition(IntEnum):
    EARNED = 1
    TRANSFER = 2
    OPENING = 3
    CORRECTION = 4
    ADMINISTRATIVE = 5
    SINK = 6
    REFUND = 7
    RESTORED = 8
    UNKNOWN = 9
    CONFLICT = 10


@dataclass(frozen=True, slots=True, order=True)
class EventIdentity:
    unit: Unit
    operation_id: bytes
    participant_pid: int
    event_index: int = 0
    item_uid: int = 0

    def validate(self):
        require(type(self.unit) is Unit, "canonical_unit")
        identifier(self.operation_id)
        integer(self.participant_pid, upper=(1 << 32) - 1)
        integer(self.event_index, upper=(1 << 32) - 1)
        integer(self.item_uid)
        require((self.unit == Unit.EQUIPMENT) == bool(self.item_uid), "canonical_item_identity")


@dataclass(frozen=True, slots=True, order=True)
class SourceReference:
    table: str
    key: bytes
    payload_digest: bytes

    def validate(self):
        require(type(self.table) is str and self.table.isascii() and self.table.isidentifier() and len(self.table) <= 64,
                "canonical_source_table")
        require(type(self.key) is bytes and 0 < len(self.key) <= 128, "canonical_source_key")
        digest(self.payload_digest)


def _json_value(value):
    if type(value) is bytes:
        require(len(value) <= MAX_SOURCE_RECORD_BYTES, "canonical_source_cell_capacity")
        return {"bytes": value.hex()}
    if value is None:
        return value
    if type(value) is int:
        return integer(value, lower=-(1 << 63))
    if type(value) is str:
        require(len(value) <= MAX_SOURCE_RECORD_BYTES, "canonical_source_cell_capacity")
        return value
    raise EvidenceError("canonical_source_value_type")


def encode_source_payload(payload: Mapping[str, object]) -> bytes:
    require(type(payload) is dict and 0 < len(payload) <= 128 and
            all(type(name) is str and name.isascii() and name.isidentifier() and len(name) <= 64 for name in payload),
            "canonical_source_columns")
    encoded = json.dumps({name: _json_value(value) for name, value in payload.items()},
                         sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")
    require(len(encoded) <= MAX_SOURCE_RECORD_BYTES, "canonical_source_record_capacity")
    return encoded


def decode_source_payload(encoded: bytes) -> dict[str, object]:
    """Decode only the exact bounded canonical representation we retain.

    Duplicate columns and alternate encodings are refused before a retained
    receipt can be used to reconstruct native authority.
    """
    require(type(encoded) is bytes and 0 < len(encoded) <= MAX_SOURCE_RECORD_BYTES,
            "canonical_source_record_capacity")

    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "canonical_source_duplicate_column")
            result[key] = value
        return result

    try:
        value = json.loads(encoded, object_pairs_hook=unique_object)
        require(type(value) is dict, "canonical_source_columns")
        decoded = {}
        for name, cell in value.items():
            if type(cell) is dict:
                require(set(cell) == {"bytes"} and type(cell["bytes"]) is str,
                        "canonical_source_binary_tag")
                decoded[name] = bytes.fromhex(cell["bytes"])
            else:
                decoded[name] = cell
        require(encode_source_payload(decoded) == encoded, "canonical_source_noncanonical_encoding")
        return decoded
    except (ValueError, TypeError, RecursionError) as error:
        if isinstance(error, EvidenceError):
            raise
        raise EvidenceError("canonical_source_payload_encoding") from error


def source_reference(table, key, payload: Mapping[str, object]):
    encoded = encode_source_payload(payload)
    result = SourceReference(table, key, hashlib.sha256(encoded).digest())
    result.validate()
    return result


@dataclass(frozen=True, slots=True)
class RewardEvent:
    identity: EventIdentity
    disposition: Disposition
    amount: int | None
    authority: str
    references: tuple[SourceReference, ...]
    lineage: bytes | None = None
    wallet_lifetime: int | None = None

    def validate(self):
        self.identity.validate()
        require(type(self.disposition) is Disposition, "canonical_disposition")
        require(type(self.authority) is str and 0 < len(self.authority) <= 64, "canonical_authority")
        require(0 < len(self.references) <= MAX_PAGE_SIZE, "canonical_reference_capacity")
        for reference in self.references:
            reference.validate()
        if self.disposition in (Disposition.UNKNOWN, Disposition.CONFLICT):
            require(self.amount is None, "canonical_unknown_amount")
        else:
            integer(self.amount, lower=-(1 << 63), upper=(1 << 63) - 1)
        if self.disposition == Disposition.EARNED:
            require(self.amount > 0 and self.authority != "unavailable" and
                    self.identity.unit != Unit.OBSERVED_XP, "canonical_earned_authority")
        if self.lineage is not None:
            identifier(self.lineage)
        if self.wallet_lifetime is not None:
            integer(self.wallet_lifetime, lower=1)


def reconcile_events(events: Iterable[RewardEvent]) -> tuple[RewardEvent, ...]:
    """One event per exact authority key; retain all distinct conflicting digests.

    Distinct source tables never establish equivalence by amount/time. Adapters
    must establish the canonical key using actual root/child/participant links.
    A source payload conflict invalidates every event that cites that source.
    """
    groups = {}
    sources = {}
    reference_count = 0
    count = 0
    for event in events:
        count += 1
        require(count <= MAX_PAGE_SIZE, "canonical_event_capacity")
        event.validate()
        reference_count += len(event.references)
        require(reference_count <= MAX_REFERENCES, "canonical_batch_reference_capacity")
        groups.setdefault(event.identity, []).append(event)
        for ref in event.references:
            sources.setdefault((ref.table, ref.key), set()).add(ref.payload_digest)
    result = []
    for key, rows in sorted(groups.items()):
        first = rows[0]
        refs = tuple(sorted({ref for row in rows for ref in row.references}))
        require(len(refs) <= MAX_PAGE_SIZE, "canonical_reference_capacity")
        claims = {(row.disposition, row.amount, row.authority, row.lineage, row.wallet_lifetime)
                  for row in rows if row.disposition != Disposition.UNKNOWN}
        conflict = len(claims) > 1 or any(row.disposition == Disposition.CONFLICT for row in rows) or any(
            len(sources[(ref.table, ref.key)]) > 1 for ref in refs)
        known = next((row for row in rows if row.disposition != Disposition.UNKNOWN), first)
        result.append(replace(known, references=refs,
                              disposition=Disposition.CONFLICT if conflict else known.disposition,
                              amount=None if conflict else known.amount))
    return tuple(result)


def earned_totals(events: Sequence[RewardEvent]) -> dict[Unit, int]:
    """Separate native units. Unknown/conflicted events never become zeros in cells."""
    totals = {}
    for event in reconcile_events(events):
        if event.disposition == Disposition.EARNED:
            totals[event.identity.unit] = totals.get(event.identity.unit, 0) + event.amount
    return totals


def _vector(row, prefix):
    return tuple(integer(row.get(prefix + name), lower=-(1 << 63), upper=(1 << 63) - 1)
                 for name in COINS)


def _exact(row, expected, reason):
    require(all(name in row and type(row[name]) is type(value) and row[name] == value
                for name, value in expected.items()), reason)


def qualified_native_plan(root):
    """Check exact retained root/plan/intent agreement without selecting a route."""
    plan = decode_plan(root.get("canonical_plan"))
    meta = plan.metadata
    op = meta["operation_id"]
    _exact(root, meta, "canonical_root_plan_metadata")
    _exact(root, dict(outcome=1, result_code=0), "canonical_root_not_committed")
    require(digest(root.get("plan_digest")) == hashlib.sha256(plan.raw).digest(),
            "canonical_plan_digest_mismatch")
    intent = root.get("canonical_intent")
    require(type(intent) is bytes and 256 <= len(intent) <= MAX_INTENT_BYTES,
            "canonical_intent_size")
    require(intent[:4] == b"EAI1" and struct.unpack_from("<HHI", intent, 4) == (1, 256, len(intent)),
            "canonical_intent_header")
    _zeros(intent, (30, 32), (108, 112), (224, 256))
    require(struct.unpack_from("<IIIHBBH", intent, 12) ==
            (meta["writer_id"], 1, 1, meta["reason"], meta["actor_kind"],
             int(meta["source_event"] is not None), 1) and
            intent[32:48] == meta["lineage"] and intent[48:64] == meta["epoch"] and
            intent[64:80] == op and intent[80:96] == (meta["original_operation_id"] or ZERO_ID) and
            struct.unpack_from("<Q", intent, 96)[0] == meta["actor_id"] and
            struct.unpack_from("<I", intent, 104)[0] == len(intent) - 256 and
            intent[112:160] == (meta["source_event"] or bytes(48)) and
            intent[192:224] == meta["domain_digest"], "canonical_intent_plan_mismatch")
    digest(intent[160:192])
    require(hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest() == meta["intent_digest"],
            "canonical_intent_digest_mismatch")
    return plan, intent


def indexed_native_references(plan, effects, postings):
    require(len(effects) == len(plan.accounts) and len(postings) == len(plan.postings),
            "canonical_bank_evidence_count")
    op, refs = plan.metadata["operation_id"], []
    for index, (row, account) in enumerate(zip(effects, plan.accounts, strict=True)):
        expected = dict(operation_id=op, account_index=index, account_key=account.key,
                        before_revision=account.before_revision, after_revision=account.after_revision)
        expected.update({"before_" + name: value for name, value in zip(COINS, account.before, strict=True)})
        expected.update({"after_" + name: value for name, value in zip(COINS, account.after, strict=True)})
        _exact(row, expected, "canonical_effect_receipt")
        refs.append(source_reference("economic_accounting_account_effect", op + index.to_bytes(2, "big"), row))
    for index, (row, posting) in enumerate(zip(postings, plan.postings, strict=True)):
        expected = dict(operation_id=op, line_index=index, event_index=posting.event_index,
                        account_index=posting.account_index, child_index=posting.child_index,
                        copper_value=posting.copper_value)
        expected.update({"delta_" + name: value for name, value in zip(COINS, posting.delta, strict=True)})
        _exact(row, expected, "canonical_posting_receipt")
        refs.append(source_reference("economic_accounting_coin_posting", op + index.to_bytes(2, "big"), row))
    return refs


def qualify_bank_root(root, ledger, inbox, effects, postings, claims) -> RewardEvent:
    """Qualify the native schema-2 bank owner's four current immutable routes.

    Inputs must originate in the same retained consistent SQL snapshot. This
    function establishes no capture completeness, human ownership, or current
    balance reconciliation. Missing/inconsistent evidence is a refusal, never
    a fallback to the legacy positive-net creation heuristic.
    """
    plan, intent = qualified_native_plan(root)
    meta, op = plan.metadata, plan.metadata["operation_id"]
    # Writer IDs are scoped to this exact owner shape, not a global capability.
    routes = {1: (1, 1, Disposition.TRANSFER), 2: (1, 2, Disposition.TRANSFER),
              3: (8, 15, Disposition.OPENING), 4: (5, 5, Disposition.EARNED)}
    require(meta["writer_id"] in routes, "canonical_bank_route_unavailable")
    reason, legacy_reason, disposition = routes[meta["writer_id"]]
    require(meta["actor_kind"] == 1 and meta["reason"] == reason and
            meta["original_operation_id"] is None and not plan.children and
            not plan.items_before and not plan.items_after and not plan.item_events and
            len(intent) == 280, "canonical_bank_owner_shape")
    issuance = disposition in (Disposition.OPENING, Disposition.EARNED)
    require(len(plan.accounts) == (3 if issuance else 2) and len(plan.postings) == 2 and
            tuple(account.kind for account in plan.accounts) == ((1, 2, 7) if issuance else (1, 2)),
            "canonical_bank_accounts")
    wallet, bank = plan.accounts[:2]
    require(wallet.context_id == 0 and struct.unpack_from("<3Q", intent, 256) ==
            (wallet.authority_id, bank.authority_id, bank.context_id), "canonical_bank_lifetimes")
    integer(bank.context_id, upper=255)
    require(all(value <= (1 << 31) - 1 for value in (*wallet.before, *wallet.after)),
            "canonical_wallet_native_range")
    _exact(ledger, dict(operation_id=op, reason_type=legacy_reason,
                        wallet_revision=wallet.after_revision, bank_revision=bank.after_revision),
           "canonical_compatibility_identity")
    integer(ledger.get("bank_id"), lower=1, upper=(1 << 32) - 1)
    integer(ledger.get("pid"), lower=1, upper=(1 << 32) - 1)
    require(meta["actor_id"] == (ledger["pid"] if disposition == Disposition.EARNED else wallet.authority_id),
            "canonical_bank_actor")
    integer(ledger.get("reason_id"), lower=-(1 << 63), upper=(1 << 63) - 1)
    integer(ledger.get("source_site"), upper=6)
    require(_vector(ledger, "wallet_delta_") == wallet.delta and
            _vector(ledger, "bank_delta_") == bank.delta and
            _vector(ledger, "wallet_after_") == wallet.after and
            _vector(ledger, "bank_after_") == bank.after, "canonical_compatibility_amount")
    _exact(inbox, dict(operation_id=op, command_type=3, schema_version=2, payload_version=1,
                       status=1, result_code=0, failure_stage=0,
                       durable_revision=max(wallet.after_revision, bank.after_revision)),
           "canonical_inbox_not_committed")
    digest(inbox.get("command_hash")); digest(inbox.get("keys_hash"))
    expected_result = struct.pack("<8q2Q", *wallet.after, *bank.after,
                                  wallet.after_revision, bank.after_revision)
    require(inbox.get("result_payload") == expected_result, "canonical_inbox_result")
    refs = [source_reference("economic_accounting_operation", op, root),
            source_reference("currency_ledger", op, ledger),
            source_reference("critical_operation_inbox", op, inbox)]
    refs.extend(indexed_native_references(plan, effects, postings))
    if issuance:
        issuer = plan.accounts[2]
        require(issuer.authority_id == 1 and issuer.context_id == 0 and len(claims) == 1,
                "canonical_issuance_authority")
        source = meta["source_event"]
        require(source is not None, "canonical_issuance_source")
        _exact(claims[0], dict(lineage=meta["lineage"], source_event=source, operation_id=op, outcome=1),
               "canonical_source_claim")
        refs.append(source_reference("economic_accounting_source_claim", meta["lineage"] + source, claims[0]))
        recipient_index = 0 if disposition == Disposition.EARNED else 1
        recipient = plan.accounts[recipient_index]
        positive = recipient.delta
        require(all(value >= 0 for value in positive) and any(positive) and
                plan.postings[0] == CoinPosting(0, recipient_index, 0, positive, currency_value(positive)) and
                plan.postings[1] == CoinPosting(1, 2, 0, tuple(-value for value in positive), -currency_value(positive)),
                "canonical_issuance_legs")
        if disposition == Disposition.EARNED:
            require(not any(bank.delta) and ledger.get("reason_id", 0) > 0 and
                    ledger["source_site"] == 5 and
                    _source(source) == (1, op, op, ledger["reason_id"], 1), "canonical_quest_source")
        else:
            seed = b"CHAOSEED" + struct.pack("<Q", ledger["pid"])
            expected_op = hashlib.sha256(seed + struct.pack("<IQ", 0x43484250, 1)).digest()[:16]
            require(not any(wallet.delta) and positive == (0, 0, 0, 1_000_000) and
                    ledger["reason_id"] == ledger["pid"] and ledger["source_site"] == 4 and
                    op == expected_op and _source(source) == (3, seed, seed, 1, 1),
                    "canonical_opening_source")
        amount = currency_value(positive)
    else:
        require(meta["source_event"] is None and not claims and
                wallet.delta == tuple(-value for value in bank.delta) and
                plan.postings[0] == CoinPosting(0, 0, 0, wallet.delta, currency_value(wallet.delta)) and
                plan.postings[1] == CoinPosting(1, 1, 0, bank.delta, currency_value(bank.delta)),
                "canonical_bank_transfer_legs")
        amount = abs(currency_value(wallet.delta))
        require((legacy_reason == 1 and currency_value(wallet.delta) < 0) or
                (legacy_reason == 2 and currency_value(wallet.delta) > 0), "canonical_transfer_direction")
        require(all(value <= 0 for value in wallet.delta) if legacy_reason == 1 else
                all(value >= 0 for value in wallet.delta), "canonical_transfer_denominations")
    # This native bank root has one beneficiary and one currency event. Keep its
    # economic identity independent of receipt authority: a quarantined or changed
    # beneficiary receipt is evidence about this root, not a second participant.
    # The exact PID remains in the retained currency receipt and native intent.
    result = RewardEvent(EventIdentity(Unit.CURRENCY, op, 0), disposition, amount,
                         "native_account_bank_v1", tuple(refs), meta["lineage"], wallet.authority_id)
    result.validate()
    return result
