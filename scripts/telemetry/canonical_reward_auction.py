"""Exact native auction sale-credit and money-claim receipt inspection.

The claim moves previously held value into a wallet. Inspection of its entire
ordered allocation inventory never grants earned authority to that value. The
currently supported upstream shape is a timed native settlement sale; other
origins require their own owner inspection and native qualification.
"""
from __future__ import annotations

import hashlib
import struct

from . import canonical_reward_contract as c
from .canonical_reward_coin import _position

AUCTION_LEDGER_COLUMNS = ("operation_id", "event_type", "auction_id", "auction_revision",
                          "actor_pid", "counterparty_pid", "value_delta", "final_price", "item_count")
ALLOCATION_COLUMNS = ("source_operation_id", "source_slot", "lineage", "claim_mapping_id",
                      "beneficiary_pid", "amount", "claim_operation_id")
# Each complete upstream bundle adds at most nine native physical references.
# This stays inside the unchanged 2,000-reference event bound. Larger native
# claims (whose owner supports 4,096 sources) require a different bounded proof.
MAX_CLAIM_SOURCES = 128


def _result(action, event, *, auction=0, status=0, seller=0, winner=0, price=0,
            delta=0, wallet=(0,) * 4, bank=(0,) * 4, wallet_revision=0,
            bank_revision=0, auction_revision=0):
    raw = struct.pack("<BB5I2q8q5QHq", action, event, auction, status, seller, winner, 0,
        price, delta, *wallet, *bank, wallet_revision, bank_revision, auction_revision, 0, 0, 0, 0)
    return raw + bytes(320 - len(raw))


def _inbox(row, op, payload, revision):
    c._exact(row, dict(operation_id=op, command_type=7, schema_version=2, payload_version=1,
                      status=1, result_code=0, failure_stage=0, durable_revision=revision,
                      result_payload=payload), "canonical_auction_inbox")
    c.digest(row.get("command_hash")); c.digest(row.get("keys_hash"))
    return c.source_reference("critical_operation_inbox", op, row)


def inspect_settlement_credit(root, inbox, effects, postings, ledger, allocation):
    """Inspect a timed sale's exact pending-claim credit, including its fee sink.

    Listing/bid identifiers and unchanged item witnesses are retained evidence;
    this does not establish listing/bid history, original issuance or a complete
    equipment transfer. A claim cannot use synthetic/unknown upstream roots.
    """
    plan, intent = c.qualified_native_plan(root)
    meta, op = plan.metadata, plan.metadata["operation_id"]
    c.require(meta["writer_id"] == 11 and meta["reason"] == 30 and meta["actor_kind"] == 1 and meta["source_event"] is not None and
              not plan.children and not plan.item_events and 1 <= len(plan.items_before) <= 9 and
              plan.items_before == plan.items_after and len(plan.accounts) in (2, 3) and
              len(plan.postings) == len(plan.accounts), "canonical_auction_sale_shape")
    facts = intent[256:]
    c.require(len(facts) == 122 + 27 * len(plan.items_before), "canonical_auction_sale_facts")
    escrow_id, pending_id, actor_wallet, actor_bank = struct.unpack_from("<4Q", facts)
    auction, seller, winner, status, custody, quantity = struct.unpack_from("<6I", facts, 32)
    price, buy_price, revision, end_time = struct.unpack_from("<4Q", facts, 56)
    listing, bid = c.identifier(facts[88:104]), c.identifier(facts[104:120])
    count = struct.unpack_from("<H", facts, 120)[0]
    c.require(escrow_id and pending_id and escrow_id != pending_id and not actor_wallet and not actor_bank and
              auction and 1 <= seller <= (1 << 31) - 1 and 1 <= winner <= (1 << 31) - 1 and
              status == custody == 1 and quantity == count == len(plan.items_before) and
              0 < price <= (1 << 32) - 1 and buy_price <= (1 << 63) - 1 and
              0 < revision < (1 << 64) - 1 and end_time and listing != bid and op not in (listing, bid) and
              meta["actor_id"] == auction and meta["original_operation_id"] == listing and
              c._source(meta["source_event"]) == (13, bid, listing, revision, 0),
              "canonical_auction_sale_identity")
    by_kind = {account.kind: (index, account) for index, account in enumerate(plan.accounts)}
    c.require(len(by_kind) == len(plan.accounts) and set(by_kind) == ({4, 5, 8} if len(plan.accounts) == 3 else {4, 5}) and
              all(account.context_id == 0 for account in plan.accounts), "canonical_auction_sale_accounts")
    escrow, pending = by_kind[4][1], by_kind[5][1]
    amount = c.integer(allocation.get("amount"), lower=1, upper=(1 << 31) - 1)
    c.require(escrow.authority_id == escrow_id and pending.authority_id == pending_id and
              escrow.before == (price, 0, 0, 0) and escrow.after == (0,) * 4 and
              escrow.before_revision == revision and escrow.after_revision == revision + 1 and
              pending.before[1:] == pending.after[1:] == (0,) * 3 and
              pending.delta == (amount, 0, 0, 0) and pending.after[0] <= (1 << 32) - 1 and
              pending.after_revision == pending.before_revision + 1 and amount <= price,
              "canonical_auction_sale_effects")
    fee = price - amount
    c.require(bool(fee) == (8 in by_kind), "canonical_auction_sale_fee_presence")
    if fee:
        sink = by_kind[8][1]
        c.require(sink.authority_id == 25 and sink.before == sink.after == (0,) * 4 and
                  sink.before_revision == sink.after_revision == 0,
                  "canonical_auction_sale_fee_sink")
    for event, kind, value in ((0, 4, -price), (1, 5, amount), *(((2, 8, fee),) if fee else ())):
        c.require(plan.postings[event] == c.CoinPosting(event, by_kind[kind][0], 0, (value, 0, 0, 0), value),
                  "canonical_auction_sale_postings")
    witnesses = {}
    for raw in plan.items_before:
        uid = struct.unpack_from("<Q", raw)[0]
        c.require(uid not in witnesses, "canonical_auction_sale_witness_duplicate")
        witnesses[uid] = _position(raw[8:])
    listed = set()
    for index in range(count):
        uid, item_revision, slot, vnum, claimant, claimed = struct.unpack_from("<QQHIIB", facts, 122 + 27 * index)
        c.require(uid not in listed and slot == index and vnum <= (1 << 31) - 1 and not claimant and not claimed and
                  witnesses.get(uid) == dict(owner=6, state=1, owner_id=auction, context=0, root=uid,
                                             parent=0, revision=item_revision, slot=0),
                  "canonical_auction_sale_item_witness")
        listed.add(uid)
    c.require(listed == set(witnesses), "canonical_auction_sale_witness_inventory")
    c._exact(allocation, dict(source_operation_id=op, source_slot=2, lineage=meta["lineage"],
                             claim_mapping_id=pending_id, beneficiary_pid=seller),
             "canonical_auction_sale_allocation")
    c._exact(ledger, dict(operation_id=op, event_type=3, auction_id=auction, auction_revision=revision + 1,
                         actor_pid=0, counterparty_pid=seller, value_delta=0, final_price=price, item_count=0),
             "canonical_auction_sale_legacy")
    refs = [c.source_reference("economic_accounting_operation", op, root),
            _inbox(inbox, op, _result(3, 3, auction=auction, status=2, seller=seller, winner=winner,
                   price=price, auction_revision=revision + 1), revision + 1),
            c.source_reference("auction_ledger", op, ledger)]
    refs.extend(c.indexed_native_references(plan, effects, postings))
    return tuple(refs)


def qualify_money_claim(root, ledger, inbox, effects, postings, claims, auction_ledger, allocations, sources):
    """Inspect one custody claim and every exact supported upstream allocation.

    ``sources`` is keyed by exact source operation ID, with root/inbox/effects/
    postings/ledger bundles from the same retained owning snapshot. There is no
    amount/time/name lookup and no inference from the first service source alone.
    """
    plan, intent = c.qualified_native_plan(root)
    meta, op = plan.metadata, plan.metadata["operation_id"]
    c.require(meta["writer_id"] == 13 and meta["reason"] == 31 and meta["actor_kind"] == 1 and meta["source_event"] is not None and
              len(intent) == 336 and len(plan.accounts) == len(plan.postings) == 2 and
              not plan.children and not plan.items_before and not plan.items_after and not plan.item_events,
              "canonical_auction_claim_shape")
    wallet_id, bank_id, pending_id, pid, money, revision, count = struct.unpack_from("<3QI2QI", intent, 256)
    c.require(len({wallet_id, bank_id, pending_id}) == 3 and min(wallet_id, bank_id, pending_id) > 0 and
              meta["actor_id"] == pid and 1 <= pid <= (1 << 31) - 1 and 0 < money <= (1 << 31) - 1 and
              revision < (1 << 64) - 1 and 1 <= count <= MAX_CLAIM_SOURCES and
              type(allocations) in (tuple, list) and len(allocations) == count and type(sources) is dict,
              "canonical_auction_claim_facts_or_capacity")
    c.require(tuple(account.kind for account in plan.accounts) == (1, 5) and
              all(account.context_id == 0 for account in plan.accounts), "canonical_auction_claim_accounts")
    wallet, pending = plan.accounts
    after_value = c.currency_value(wallet.before) + money
    denominations, remainder = [0] * 4, after_value
    for index, unit in reversed(tuple(enumerate((1, 10, 100, 1000)))):
        denominations[index], remainder = divmod(remainder, unit)
    c.require(wallet.authority_id == wallet_id and pending.authority_id == pending_id and
              wallet.after == tuple(denominations) and all(value <= (1 << 31) - 1 for value in (*wallet.before, *wallet.after)) and
              wallet.after_revision == wallet.before_revision + 1 and
              pending.before == (money, 0, 0, 0) and pending.after == (0,) * 4 and
              pending.before_revision == revision and pending.after_revision == revision + 1 and
              plan.postings == (c.CoinPosting(0, 0, 0, wallet.delta, money),
                                c.CoinPosting(1, 1, 0, (-money, 0, 0, 0), -money)),
              "canonical_auction_claim_effects")
    inventory = bytearray(b"DURIS-PENDING-CLAIM-SOURCES-V1" + struct.pack("<I", count))
    refs = [c.source_reference("economic_accounting_operation", op, root)]
    previous, total, operations = None, 0, set()
    for allocation in allocations:
        source = c.identifier(allocation.get("source_operation_id"))
        slot = c.integer(allocation.get("source_slot"), lower=1, upper=(1 << 16) - 1)
        amount = c.integer(allocation.get("amount"), lower=1, upper=(1 << 31) - 1)
        identity = source, slot
        c.require(source != op and (previous is None or previous < identity), "canonical_auction_claim_allocation_order")
        previous, total = identity, total + amount
        c._exact(allocation, dict(lineage=meta["lineage"], claim_mapping_id=pending_id,
                                 beneficiary_pid=pid, claim_operation_id=op), "canonical_auction_claim_allocation_binding")
        c.require(source not in operations and source in sources, "canonical_auction_claim_source_scope")
        operations.add(source)
        bundle = sources[source]
        c.require(type(bundle) is dict and set(bundle) == {"root", "inbox", "effects", "postings", "ledger"} and
                  bundle["root"].get("lineage") == meta["lineage"], "canonical_auction_claim_source_bundle")
        refs.extend(inspect_settlement_credit(bundle["root"], bundle["inbox"], bundle["effects"],
                                             bundle["postings"], bundle["ledger"], allocation))
        refs.append(c.source_reference("economic_pending_claim_source", source + slot.to_bytes(2, "big"), allocation))
        inventory.extend(source + struct.pack("<HIQQ", slot, pid, pending_id, amount))
    first = allocations[0]
    c.require(operations == set(sources) and total == money and hashlib.sha256(inventory).digest() == intent[304:336] and
              meta["original_operation_id"] == first["source_operation_id"] and
              c._source(meta["source_event"]) == (17, first["source_operation_id"], first["source_operation_id"], revision,
                                                first["source_slot"]), "canonical_auction_claim_source_inventory")
    c.require(len(claims) == 1, "canonical_auction_claim_service_count")
    c._exact(claims[0], dict(lineage=meta["lineage"], source_event=meta["source_event"], operation_id=op, outcome=1),
             "canonical_auction_claim_service_receipt")
    refs.append(c.source_reference("economic_accounting_source_claim", meta["lineage"] + meta["source_event"], claims[0]))
    bank_revision = c.integer(ledger.get("bank_revision"), lower=1)
    c.integer(ledger.get("bank_id"), lower=1, upper=(1 << 32) - 1)
    c.integer(ledger.get("source_site"), lower=1, upper=6)
    bank = c._vector(ledger, "bank_after_")
    c.require(all(value >= 0 for value in bank) and not any(c._vector(ledger, "bank_delta_")) and
              c._vector(ledger, "wallet_delta_") == wallet.delta and c._vector(ledger, "wallet_after_") == wallet.after,
              "canonical_auction_claim_currency_values")
    c._exact(ledger, dict(operation_id=op, pid=pid, reason_type=11, reason_id=0,
                         wallet_revision=wallet.after_revision), "canonical_auction_claim_currency_identity")
    c._exact(auction_ledger, dict(operation_id=op, event_type=5, auction_id=0,
        auction_revision=wallet.after_revision, actor_pid=pid, counterparty_pid=0, value_delta=money,
        final_price=0, item_count=0), "canonical_auction_claim_legacy")
    refs.extend(c.indexed_native_references(plan, effects, postings))
    refs.extend((c.source_reference("currency_ledger", op, ledger),
        c.source_reference("auction_ledger", op, auction_ledger),
        _inbox(inbox, op, _result(4, 5, delta=money, wallet=wallet.after, bank=bank,
               wallet_revision=wallet.after_revision, bank_revision=bank_revision,
               auction_revision=wallet.after_revision), wallet.after_revision)))
    # The native beneficiary PID stays exact source evidence. The custody root
    # has one event even when a damaged cut cannot qualify its participant.
    event = c.RewardEvent(c.EventIdentity(c.Unit.CURRENCY, op, 0), c.Disposition.TRANSFER, money,
                          "native_auction_money_claim_v1", tuple(refs), meta["lineage"], wallet_id)
    event.validate()
    return event


def qualify_retained_money_claim(root, rows):
    """Separate current/dependency bundles using only exact allocation links."""
    op = c.identifier(root.get("operation_id"))
    c.require(not any(rows[table] for table in ("economic_accounting_child", "economic_accounting_item_reference",
                                               "item_ownership_ledger")), "canonical_auction_unexpected_item_or_child")
    allocations = rows["economic_pending_claim_source"]
    c.require(0 < len(allocations) <= MAX_CLAIM_SOURCES, "canonical_auction_claim_allocation_capacity")
    upstream = {c.identifier(row.get("source_operation_id")) for row in allocations}
    bundles = {source: {} for source in upstream}
    current = {}
    for table, name, many in (("economic_accounting_operation", "root", False),
        ("critical_operation_inbox", "inbox", False), ("economic_accounting_account_effect", "effects", True),
        ("economic_accounting_coin_posting", "postings", True), ("auction_ledger", "ledger", False)):
        groups = {identity: [] for identity in (op, *upstream)}
        for row in rows[table]:
            identity = c.identifier(row.get("operation_id"))
            c.require(identity in groups, "canonical_auction_dependency_scope")
            groups[identity].append(row)
        for identity, family in groups.items():
            c.require(1 <= len(family) <= (3 if many else 1), "canonical_auction_dependency_receipt_count")
            if many:
                family.sort(key=lambda row: row["account_index"] if name == "effects" else row["line_index"])
            (current if identity == op else bundles[identity])[name] = family if many else family[0]
    c.require(current["root"] == root and len(rows["currency_ledger"]) == len(rows["economic_accounting_source_claim"]) == 1,
              "canonical_auction_current_receipt_count")
    return qualify_money_claim(root, rows["currency_ledger"][0], current["inbox"], current["effects"], current["postings"],
                               rows["economic_accounting_source_claim"], current["ledger"], allocations, bundles)
