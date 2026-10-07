"""Exact receipt inspection for the native typed coin-transfer owner.

Wallet lifetimes and pile UIDs are economic authority keys, not human identity.
Each balanced root is one custody transfer, including change-making and pile
creation/retirement. Its compatibility children never become earned issuance.
"""
from __future__ import annotations

import struct

from . import canonical_reward_contract as c

CHILD_COLUMNS = ("operation_id", "child_index", "child_operation_id", "domain_id",
                 "discriminator", "parent_index", "relationship", "receipt_operation_id")
ITEM_REFERENCE_COLUMNS = ("operation_id", "line_index", "event_index", "child_index",
                          "item_uid", "before_revision", "after_revision",
                          "legacy_operation_id", "legacy_event_index")
ITEM_LEDGER_COLUMNS = ("operation_id", "event_index", "item_uid", "root_item_uid", "parent_item_uid",
                       "from_owner_type", "from_owner_id", "from_owner_context_id",
                       "to_owner_type", "to_owner_id", "to_owner_context_id", "item_revision",
                       "from_owner_revision", "to_owner_revision", "reason_type", "reason_id", "source_site",
                       "from_equipment_slot", "to_equipment_slot")
COIN_CHILD_DOMAIN = 0x434F494E


def _position(raw):
    c.require(type(raw) is bytes and len(raw) == 56, "canonical_coin_item_position_size")
    c._zeros(raw, (2, 8), (50, 56))
    owner, state = raw[:2]
    owner_id, context, root, parent, revision = struct.unpack_from("<5Q", raw, 8)
    slot = struct.unpack_from("<H", raw, 48)[0]
    if state == 0:
        c.require(not any(raw), "canonical_coin_absent_position")
    else:
        c.require(state in (1, 2) and 1 <= owner <= 11 and owner != 7 and root and revision and
                  0 <= slot <= 43 and (slot == 0 or (owner == 1 and state == 1 and parent == 0)) and
                  (owner == 8) == (state == 2) and
                  ((owner_id == context == 0) if owner in (7, 8) else owner_id > 0),
                  "canonical_coin_item_position")
    return dict(owner=owner, state=state, owner_id=owner_id, context=context,
                root=root, parent=parent, revision=revision, slot=slot)


def _item_events(plan):
    snapshots = []
    for family in (plan.items_before, plan.items_after):
        rows, previous = {}, 0
        for raw in family:
            uid = struct.unpack_from("<Q", raw)[0]
            c.require(uid > previous, "canonical_coin_witness_order")
            previous = uid
            position = _position(raw[8:])
            c.require(position["parent"] != uid and
                      (position["state"] != 1 or position["parent"] != 0 or position["root"] == uid),
                      "canonical_coin_witness_topology")
            rows[uid] = raw[8:]
        snapshots.append(rows)
    c.require(set(snapshots[0]) == set(snapshots[1]), "canonical_coin_witness_inventory")
    result, uids = {}, set()
    for index, raw in enumerate(plan.item_events):
        event, child, uid = struct.unpack_from("<IH2xQ", raw)
        c._zeros(raw, (6, 8))
        c.require(event == index and 1 <= child <= len(plan.children) and uid not in uids and
                  snapshots[0].get(uid) == raw[16:72] and snapshots[1].get(uid) == raw[72:128],
                  "canonical_coin_item_event_witness")
        uids.add(uid)
        c.require(child not in result, "canonical_coin_item_event_child")
        result[child] = (event, uid, _position(raw[16:72]), _position(raw[72:128]))
    # Unchanged ancestors are context witnesses, not additional custody events.
    c.require(all(uid in uids or snapshots[0][uid] == snapshots[1][uid] for uid in snapshots[0]),
              "canonical_coin_changed_unreferenced_witness")
    return result


def _committed_inbox(row, op, command_type, payload_version, result, revision, *, schema=1):
    c._exact(row, dict(operation_id=op, command_type=command_type, schema_version=schema,
                      payload_version=payload_version, status=1, result_code=0, failure_stage=0,
                      durable_revision=revision, result_payload=result), "canonical_coin_inbox_receipt")
    c.digest(row.get("command_hash")); c.digest(row.get("keys_hash"))
    return c.source_reference("critical_operation_inbox", op, row)


def qualify_coin_root(root, inboxes, effects, postings, claims, children, currency, item_refs, items):
    """Inspect all exact native root/child receipts from one consistent source cut.

    This supplements the authoritative native transaction owner's checks. It
    never reconstructs a command from hashes or consults current holdings.
    """
    plan, intent = c.qualified_native_plan(root)
    meta, op = plan.metadata, plan.metadata["operation_id"]
    c.require(meta["writer_id"] == 5 and meta["reason"] == 3 and meta["actor_kind"] == 1 and
              meta["original_operation_id"] is None and len(intent) == 272 and
              len(plan.accounts) == len(plan.postings) == len(plan.children) == 2 and
              len(plan.item_events) <= 2, "canonical_coin_owner_shape")
    c.require(all(account.kind in (1, 3) and account.context_id == 0 for account in plan.accounts),
              "canonical_coin_accounts")
    c.require(all(child.domain_id == COIN_CHILD_DOMAIN and child.parent_index == 0 and
                  child.relationship == 1 for child in plan.children) and
              {child.discriminator for child in plan.children} == {0, 1}, "canonical_coin_children")
    c.require(len(children) == 2 and len(inboxes) == 3 and len(claims) <= 1 and
              len(currency) + len(items) == 2 and len(items) == len(item_refs) == len(plan.item_events),
              "canonical_coin_receipt_counts")
    refs = [c.source_reference("economic_accounting_operation", op, root)]
    refs.extend(c.indexed_native_references(plan, effects, postings))
    for index, (child, row) in enumerate(zip(plan.children, children, strict=True), 1):
        c._exact(row, dict(operation_id=op, child_index=index, child_operation_id=child.operation_id,
                          domain_id=child.domain_id, discriminator=child.discriminator, parent_index=0,
                          relationship=1, receipt_operation_id=child.operation_id), "canonical_coin_child_receipt")
        refs.append(c.source_reference("economic_accounting_child", op + index.to_bytes(2, "big"), row))
    inbox_by_id = {row["operation_id"]: row for row in inboxes}
    currency_by_id = {row["operation_id"]: row for row in currency}
    item_by_id = {row["operation_id"]: row for row in items}
    c.require(len(inbox_by_id) == 3 and len(currency_by_id) == len(currency) and
              len(item_by_id) == len(items) and set(inbox_by_id) == {op, *(child.operation_id for child in plan.children)},
              "canonical_coin_receipt_identities")
    item_events = _item_events(plan)
    item_ref_by_index = {row["line_index"]: row for row in item_refs}
    c.require(set(item_ref_by_index) == set(range(len(item_refs))), "canonical_coin_item_reference_indices")
    endpoint_accounts, results, revisions = {}, {}, []
    created = retired = False
    source_revision = None
    for posting in plan.postings:
        c.require(posting.child_index in (1, 2), "canonical_coin_posting_child")
        child = plan.children[posting.child_index - 1]
        endpoint = child.discriminator
        account = plan.accounts[posting.account_index]
        c.require(endpoint not in endpoint_accounts and posting.delta == account.delta and
                  (posting.copper_value < 0 if endpoint == 0 else posting.copper_value > 0),
                  "canonical_coin_endpoint_posting")
        endpoint_accounts[endpoint] = account
        child_inbox = inbox_by_id[child.operation_id]
        if account.kind == 1:
            c.require(child.operation_id in currency_by_id and child.operation_id not in item_by_id and
                      posting.child_index not in item_events, "canonical_coin_wallet_receipt_family")
            ledger = currency_by_id[child.operation_id]
            c.integer(ledger.get("pid"), lower=1, upper=(1 << 31) - 1)
            c.integer(ledger.get("bank_id"), lower=1, upper=(1 << 32) - 1)
            bank_revision = c.integer(ledger.get("bank_revision"), lower=1)
            c.integer(ledger.get("reason_id"), lower=-(1 << 63), upper=(1 << 63) - 1)
            c.integer(ledger.get("source_site"), lower=1, upper=6)
            c.require(all(value <= (1 << 31) - 1 for value in (*account.before, *account.after)),
                      "canonical_coin_wallet_range")
            c._exact(ledger, dict(reason_type=16, wallet_revision=account.after_revision),
                     "canonical_coin_wallet_legacy_identity")
            c.require(c._vector(ledger, "wallet_delta_") == posting.delta and
                      not any(c._vector(ledger, "bank_delta_")) and
                      c._vector(ledger, "wallet_after_") == account.after and
                      all(value >= 0 for value in c._vector(ledger, "bank_after_")),
                      "canonical_coin_wallet_legacy_values")
            payload = struct.pack("<8q2Q", *account.after, *c._vector(ledger, "bank_after_"),
                                  account.after_revision, bank_revision)
            revision = max(account.after_revision, bank_revision)
            refs.append(_committed_inbox(child_inbox, child.operation_id, 3, 1, payload, revision))
            refs.append(c.source_reference("currency_ledger", child.operation_id, ledger))
            results[endpoint] = payload + bytes(48)
            if endpoint == 0:
                source_revision = account.before_revision
        else:
            c.require(child.operation_id in item_by_id and child.operation_id not in currency_by_id and
                      posting.child_index in item_events, "canonical_coin_pile_receipt_family")
            index, uid, before, after = item_events[posting.child_index]
            creation, retirement = before["state"] == 0, after["state"] == 2
            c.require(uid == account.authority_id and after["revision"] == account.after_revision and
                      before["slot"] == after["slot"] == 0 and
                      after["revision"] == before["revision"] + 1 and
                      account.before_revision <= before["revision"] and
                      (not creation or (endpoint == 1 and account.before_revision == 0 and not any(account.before))) and
                      (not retirement or (endpoint == 0 and not any(account.after))) and
                      (creation or before["state"] == 1), "canonical_coin_pile_lifetime")
            if endpoint == 0:
                source_revision, retired = before["revision"], retirement
            else:
                created = creation
            ledger = item_by_id[child.operation_id]
            from_revision = c.integer(ledger.get("from_owner_revision"))
            to_revision = c.integer(ledger.get("to_owner_revision"))
            c.integer(ledger.get("reason_type"), lower=1, upper=34)
            c.integer(ledger.get("reason_id"), lower=-(1 << 63), upper=(1 << 63) - 1)
            c.integer(ledger.get("source_site"), lower=1, upper=6)
            c._exact(ledger, dict(event_index=0, item_uid=uid, root_item_uid=after["root"],
                parent_item_uid=after["parent"] or None, from_owner_type=7 if creation else before["owner"],
                from_owner_id=before["owner_id"], from_owner_context_id=before["context"],
                to_owner_type=after["owner"], to_owner_id=after["owner_id"], to_owner_context_id=after["context"],
                item_revision=after["revision"], from_equipment_slot=0, to_equipment_slot=0), "canonical_coin_item_legacy_values")
            ref = item_ref_by_index[index]
            c._exact(ref, dict(operation_id=op, line_index=index, event_index=index,
                child_index=posting.child_index, item_uid=uid, before_revision=before["revision"],
                after_revision=after["revision"], legacy_operation_id=child.operation_id, legacy_event_index=0),
                "canonical_coin_item_reference")
            payload = struct.pack("<QHB5x4Q", uid, 1, 0, from_revision, to_revision, after["revision"], 0)
            revision = max(from_revision, to_revision, after["revision"])
            # Current native coin piles use the version-10 exact item payload.
            # Older item command versions are unavailable until independently qualified.
            refs.append(_committed_inbox(child_inbox, child.operation_id, 5, 10, payload, revision))
            refs.append(c.source_reference("item_ownership_ledger", child.operation_id + bytes(4), ledger))
            refs.append(c.source_reference("economic_accounting_item_reference", op + index.to_bytes(2, "big"), ref))
            results[endpoint] = bytes(80) + payload
        revisions.append(revision)
    source, destination = endpoint_accounts[0], endpoint_accounts[1]
    c.require(meta["actor_id"] == source.authority_id and struct.unpack_from("<2Q", intent, 256) ==
              (source.authority_id, destination.authority_id), "canonical_coin_endpoint_authorities")
    source_event = None
    if created or retired:
        source_event = struct.pack("<HH", 16, 1) + bytes((source.kind,)) + struct.pack("<Q", source.authority_id) + \
            b"COINACC" + struct.pack("<Q", source_revision) + b"COINLIFE" + struct.pack("<QI", 0, int(retired) | (int(created) << 1))
    c.require(meta["source_event"] == source_event and len(claims) == int(source_event is not None),
              "canonical_coin_lifecycle_source")
    if claims:
        c._exact(claims[0], dict(lineage=meta["lineage"], source_event=source_event, operation_id=op, outcome=1),
                 "canonical_coin_lifecycle_claim")
        refs.append(c.source_reference("economic_accounting_source_claim", meta["lineage"] + source_event, claims[0]))
    refs.append(_committed_inbox(inbox_by_id[op], op, 17, 1, results[0] + results[1], max(revisions), schema=2))
    amount = -next(posting.copper_value for posting in plan.postings
                   if plan.children[posting.child_index - 1].discriminator == 0)
    event = c.RewardEvent(c.EventIdentity(c.Unit.CURRENCY, op, 0), c.Disposition.TRANSFER,
                          amount, "native_coin_transfer_v1", tuple(refs), meta["lineage"],
                          source.authority_id if source.kind == 1 else None)
    event.validate()
    return event
