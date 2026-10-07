#!/usr/bin/env python3
"""Read and verify SQL EAB1/EAB2 opening origins without changing native authority.

This is one input to a future complete audit snapshot, not an audit snapshot or
an activation attestation. It contains only non-personal account keys and UIDs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import stat
from pathlib import Path
import struct
import sys

from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, account_key, copper, same_projection
from economic_restore_evidence import EvidenceError, decode_intent, decode_plan, forest, position

HEADER_BYTES = 192
HOLDING_BYTES = 112
ITEM_BYTES = 88
ITEM_BYTES_V2 = 96
MAX_HOLDINGS_PER_WITNESS = 3071
MAX_ITEMS_PER_WITNESS = 6000
ITEM_STATES = {1: "live", 2: "tombstone", 3: "quarantined"}
MAX_WITNESS_BYTES = HEADER_BYTES + MAX_HOLDINGS_PER_WITNESS * HOLDING_BYTES + MAX_ITEMS_PER_WITNESS * ITEM_BYTES_V2


class OriginError(ValueError):
    pass


def identity(raw: object, label: str) -> bytes:
    if not isinstance(raw, bytes) or len(raw) != 16 or raw == bytes(16):
        raise OriginError(f"invalid {label}")
    return raw


def digest(raw: object, label: str) -> bytes:
    if not isinstance(raw, bytes) or len(raw) != 32 or raw == bytes(32):
        raise OriginError(f"invalid {label}")
    return raw


def unsigned(data: bytes) -> int:
    return int.from_bytes(data, "little")


def witness_layout(blob: bytes) -> tuple[int, int]:
    """Select only an exact historical or version-two original wire layout."""
    if not isinstance(blob, bytes) or len(blob) < HEADER_BYTES or len(blob) > MAX_WITNESS_BYTES:
        raise OriginError("invalid EAB1 size")
    version = {b"EAB1": 1, b"EAB2": 2}.get(blob[:4])
    if version is None or struct.unpack_from("<HHII", blob, 4) != (version, HEADER_BYTES, len(blob), 0):
        raise OriginError("invalid EAB1/EAB2 header")
    stride = ITEM_BYTES if version == 1 else ITEM_BYTES_V2
    holdings, items = struct.unpack_from("<II", blob, 184)
    if (holdings > MAX_HOLDINGS_PER_WITNESS or items > MAX_ITEMS_PER_WITNESS or
            len(blob) != HEADER_BYTES + holdings * HOLDING_BYTES + items * stride):
        raise OriginError(f"EAB{version} count or length mismatch")
    return version, stride


def decode_witness(row: dict, lineage: bytes, epoch: bytes, opening: bytes) -> tuple[list[dict], list[dict]]:
    blob = row.get("canonical_witness")
    version, stride = witness_layout(blob)
    if digest(row.get("witness_digest"), "witness digest") != hashlib.sha256(blob).digest():
        raise OriginError("EAB1 digest mismatch")
    if blob[16:32] != lineage or blob[32:48] != epoch:
        raise OriginError("foreign EAB1 lineage or epoch")
    identity(blob[48:64], "preparation ID")
    if unsigned(blob[64:72]) == 0:
        raise OriginError("invalid EAB1 actor")
    opening_lineage, opening_kind, _, _ = account_key(blob[80:120].hex())
    if blob[80:120] != opening or opening_lineage != lineage.hex() or opening_kind != 9:
        raise OriginError("EAB1 opening account mismatch")
    digest(blob[120:152], "boundary digest")
    digest(blob[152:184], "coverage digest")
    holding_count, item_count = struct.unpack_from("<II", blob, 184)
    if (type(row.get("holding_count")) is not int or type(row.get("item_count")) is not int or
            holding_count != row["holding_count"] or item_count != row["item_count"]):
        raise OriginError("EAB1 count or length mismatch")

    holdings: list[dict] = []
    previous_key = None
    lifetimes: set[int] = set()
    offset = HEADER_BYTES
    for _ in range(holding_count):
        key = blob[offset:offset + 40].hex()
        decoded_key = account_key(key)
        key_lineage, kind, lifetime, _ = decoded_key
        if (key_lineage != lineage.hex() or kind not in range(1, 7) or
                (previous_key is not None and decoded_key <= previous_key) or lifetime in lifetimes):
            raise OriginError("invalid or duplicate EAB1 holding")
        # Native canonical order compares numeric fields, not their LE bytes.
        previous_key = decoded_key
        lifetimes.add(lifetime)
        balance = list(struct.unpack_from("<4q", blob, offset + 40))
        if any(amount < 0 for amount in balance):
            raise OriginError("negative EAB1 opening")
        copper(tuple(balance))
        revision = unsigned(blob[offset + 72:offset + 80])
        digest(blob[offset + 80:offset + 112], "holding source digest")
        holdings.append({"account_key": key, "origin": "baseline", "balance": balance,
                         "revision": revision})
        offset += HOLDING_BYTES

    items: list[dict] = []
    positions = {}
    previous_uid = 0
    for _ in range(item_count):
        uid = unsigned(blob[offset:offset + 8])
        owner_type = blob[offset + 8]
        state = blob[offset + 9]
        if (uid <= previous_uid or not 1 <= owner_type <= 12 or state not in ITEM_STATES or
                blob[offset + 10:offset + 16] != bytes(6)):
            raise OriginError("invalid or duplicate EAB1 item")
        previous_uid = uid
        owner_id, context, root, parent, revision = struct.unpack_from("<5Q", blob, offset + 16)
        system_owner = owner_type in (7, 8)
        owner_valid = (owner_id == 0 and context == 0 if system_owner else
                       owner_id > 0 and (owner_type != 10 or context == 0) and
                       (owner_type != 11 or 0 < context <= 2**31 - 1) and
                       (owner_type != 12 or (owner_id < 2**64 - 1 and context == 0 and state in (1, 3))))
        if not owner_valid or not root or parent == uid:
            raise OriginError("invalid EAB1 item topology")
        digest(blob[offset + stride - 32:offset + stride], "item source digest")
        items.append({"uid": uid, "origin": "baseline", "revision": revision,
                      "root": root, "parent": parent or None,
                      "owner": [owner_type, owner_id, context], "state": ITEM_STATES[state]})
        if version == 2:
            try:
                positions[uid] = position(blob[offset + 8:offset + 64])
            except EvidenceError as error:
                raise OriginError("EAB1 committed root mismatch") from error
            items[-1]["equipment_slot"] = positions[uid][-1]
        offset += stride
    if version == 2:
        try:
            forest(positions)
        except EvidenceError as error:
            raise OriginError("EAB1 committed root mismatch") from error
    return holdings, items


def verify_baseline_root(row: dict, lineage: bytes, epoch: bytes) -> dict:
    """Bind every original versioned witness byte to its committed root."""
    try:
        blob = row["canonical_witness"]
        version, stride = witness_layout(blob)
        operation = hashlib.sha256(blob[48:64] + struct.pack("<I", 0x42415345) + blob[72:80]).digest()[:16]
        source = struct.pack("<HH", 10, 1) + blob[48:64] + epoch + blob[72:80] + bytes(4)
        expected = (lineage, epoch, operation, bytes(16), 1, 4, 1, 1, 2,
                    unsigned(blob[64:72]), 38, source)
        root = (row["root_lineage"], row["root_epoch"], row["operation_id"],
                row["original_operation_id"] or bytes(16),
                *(row[name] for name in ("accounting_version", "writer_id", "policy_version",
                                         "compiler_version", "actor_kind", "actor_id", "reason")),
                row["source_event"])
        keys_hash = hashlib.sha256(struct.pack("<BQ", 9, 0x45434f4e42415345)).digest()
        if (not same_projection(root, expected) or row["original_operation_id"] is not None or
                type(row["witness_version"]) is not int or row["witness_version"] != version or
                type(row["book_revision"]) is not int or not 0 < row["book_revision"] < 2**64 or
                not same_projection(row["inbox_revision"], row["book_revision"]) or
                not same_projection((row["inbox_type"], row["inbox_schema"], row["inbox_payload"],
                                     row["inbox_result_payload"]), (20, 2, 1, b"")) or
                type(row.get("inbox_keys_hash")) is not bytes or
                row["inbox_keys_hash"] != keys_hash):
            raise OriginError("EAB1 committed root mismatch")
        intent = decode_intent(row["canonical_intent"])
        plan = decode_plan(row["canonical_plan"])
        payload = b"EBC1" + struct.pack("<HHII", 1, 48, len(blob), 0) + hashlib.sha256(blob).digest()
        domain = hashlib.sha256(b"DURIS-ECONOMIC-DOMAIN-V1\0" + struct.pack("<HHI", 20, 1, 48) + payload).digest()
        # Baseline CCM1 binding uses schema 1, no intent/publication/revisions,
        # operator-repair source, recovery deadline, one system fence and time 1.
        command = (b"CCM1" + struct.pack("<I", 1) + operation +
                   struct.pack("<HHHBBQIII", 20, 1, 6, 4, 0, 1, 1, 0, 48) +
                   struct.pack("<B7xQ", 9, 0x45434f4e42415345) + payload)
        binding = hashlib.sha256(b"DURIS-ECONOMIC-COMMAND-V1\0" + command).digest()
        if (len(row["canonical_intent"]) != 256 or intent["metadata"] != expected or
                plan["metadata"] != expected or row["canonical_intent"][160:192] != binding or
                intent["domain_digest"] != domain or
                plan["domain_digest"] != domain or not same_projection(row["domain_digest"], domain) or
                intent["intent_digest"] != plan["intent_digest"] or
                not same_projection(row["intent_digest"], intent["intent_digest"]) or
                not same_projection(row["plan_digest"], plan["plan_digest"])):
            raise OriginError("EAB1 committed root mismatch")
        command_hash = digest(row["inbox_command_hash"], "baseline command hash")
        accepted_at_usec = row["command_accepted_at_usec"]
        # EAI1 deliberately normalizes admission time. When SQL retains the
        # original time, separately authenticate the full schema-2 CCM1 bytes.
        # Historical NULL is unknown; never replace it with time 1 or a clock.
        if accepted_at_usec is not None:
            if type(accepted_at_usec) is not int or not 0 < accepted_at_usec < 2**64:
                raise OriginError("EAB1 committed root mismatch")
            original_command = (b"CCM1" + struct.pack("<I", 2) + operation +
                struct.pack("<HHHBBQIII", 20, 1, 6, 4, 0, accepted_at_usec, 1, 0, 48) +
                struct.pack("<B7xQ", 9, 0x45434f4e42415345) + payload +
                struct.pack("<I", len(row["canonical_intent"])) + row["canonical_intent"])
            if hashlib.sha256(original_command).digest() != command_hash:
                raise OriginError("EAB1 committed root mismatch")
        holdings, items = struct.unpack_from("<II", blob, 184)
        effects, postings, equity = [], [], []
        for index in range(holdings):
            record = blob[192 + index * 112:192 + (index + 1) * 112]
            amounts = struct.unpack_from("<4q", record, 40)
            effects.append(record[:40] + bytes(32) + record[40:72] + struct.pack("<QQ", 0, 1))
            value = copper(amounts)
            if value:
                postings.append(struct.pack("<IHH4qq", len(postings), index, 0, *amounts, value))
                equity.append((amounts, value))
        if equity:
            effects.append(blob[80:120] + bytes(80))
            for amounts, value in equity:
                postings.append(struct.pack("<IHH4qq", len(postings), holdings, 0,
                                            *(-amount for amount in amounts), -value))
        snapshots = [blob[192 + holdings * 112 + index * stride:
                          192 + holdings * 112 + index * stride + (64 if version == 2 else 56)] +
                     (b"" if version == 2 else bytes(8)) for index in range(items)]
        counts = (len(effects), len(postings), 0, items, items, 0)
        if (plan["counts"] != counts or
                not same_projection(tuple(row[name] for name in (
                    "account_count", "posting_count", "child_count", "before_witness_count",
                    "after_witness_count", "item_event_count")), counts) or
                row["canonical_plan"][256:] != b"".join(effects + postings + snapshots + snapshots)):
            raise OriginError("EAB1 committed root mismatch")
        return plan
    except (ValueError, KeyError, TypeError, struct.error) as error:
        raise OriginError("EAB1 committed root mismatch") from error


COINS = ("copper", "silver", "gold", "platinum")
CLAIM_POLICY_COLUMNS = (
    "p.operation_id", "p.lineage", "p.epoch", "p.native_boundary_digest", "p.request_digest",
    "p.phase", "p.baseline_operation_id", "i.command_hash", "i.keys_hash", "i.command_type",
    "i.schema_version", "i.payload_version", "i.status", "i.result_code", "i.failure_stage",
    "i.durable_revision", "i.result_payload", "CAST((i.committed_at IS NOT NULL) AS UNSIGNED)",
)
CLAIM_MAPPING_COLUMNS = (
    "mapping_id", "lineage", "backend_kind", "account_kind", "locator_kind", "native_id", "context_id",
)


def verify_baseline_claim_policy(row: dict, holdings: list[dict], parents: list[tuple]) -> bool:
    """Authenticate the new money-opening policy; historical NULL stays unknown."""
    marker = row["claim_origin_version"]
    if marker is not None and (type(marker) is not int or marker != 1):
        raise OriginError("EAB1 claim origin mismatch")
    if not any(account_key(holding["account_key"])[1] in (4, 5) for holding in holdings):
        return marker is not None
    if marker is None and not parents:
        return False
    accepted = row["command_accepted_at_usec"]
    if marker != 1 or len(parents) != 1 or type(accepted) is not int or not 0 < accepted < 2**64:
        raise OriginError("EAB1 claim origin mismatch")
    blob = row["canonical_witness"]
    request = b"DURIS-SQL-LIFECYCLE-V2"
    for value in (blob[48:64], blob[16:32], blob[32:48]):
        request += struct.pack("<Q", len(value)) + value
    request += blob[64:72] + struct.pack("<Q", accepted)
    request_hash = hashlib.sha256(request).digest()
    parent = parents[0]
    if (len(parent) != len(CLAIM_POLICY_COLUMNS) or type(parent[5]) is not int or parent[5] not in (1, 2) or
            (parent[6] is not None and (type(parent[6]) is not bytes or parent[6] != row["operation_id"]))):
        raise OriginError("EAB1 claim origin mismatch")
    expected = (blob[48:64], blob[16:32], blob[32:48], blob[120:152], request_hash,
                parent[5], parent[6], request_hash, hashlib.sha256(b"").digest(), 20, 2, 1, 1, 0, 0, 0, b"", 1)
    if not same_projection(tuple(parent), expected):
        raise OriginError("EAB1 claim origin mismatch")
    return True


def verify_baseline_claim_identity(row: dict, holdings: list[dict], mappings: list[tuple]) -> list[tuple]:
    """Recompute original ESD1/ESR1, including zero and retired claim mappings."""
    blob = row["canonical_witness"]
    claims = [(index, holding, account_key(holding["account_key"]))
              for index, holding in enumerate(holdings) if account_key(holding["account_key"])[1] == 5]
    by_id = {}
    for mapping in mappings:
        if len(mapping) != len(CLAIM_MAPPING_COLUMNS) or type(mapping[0]) is not int or mapping[0] in by_id:
            raise OriginError("EAB1 claim origin mismatch")
        by_id[mapping[0]] = tuple(mapping)
    if set(by_id) != {key[2] for _, _, key in claims}:
        raise OriginError("EAB1 claim origin mismatch")

    def text(value: str) -> bytes:
        encoded = value.encode("ascii")
        return struct.pack("<Q", len(encoded)) + encoded

    definition = text("ESD1") + text("auction_money_pickups") + text("pid") + struct.pack("<Q", 3)
    definition += b"".join(text(column) for column in ("pid", "money", "claim_revision"))
    schema = hashlib.sha256(definition).digest()
    expected_sources = []
    for index, holding, key in claims:
        mapping, pid, amount = by_id[key[2]], by_id[key[2]][5], holding["balance"][0]
        if (type(pid) is not int or not 0 < pid < 2**32 or key[3] != 0 or
                not 0 <= amount < 2**32 or any(holding["balance"][1:]) or
                not same_projection(mapping, (key[2], blob[16:32], 1, 5, 5, pid, 0))):
            raise OriginError("EAB1 claim origin mismatch")
        original = text("ESR1") + schema
        original += b"".join(struct.pack("<Q", 1) + text(str(value))
                             for value in (pid, amount, holding["revision"]))
        if hashlib.sha256(original).digest() != blob[192 + index * 112 + 80:192 + (index + 1) * 112]:
            raise OriginError("EAB1 claim origin mismatch")
        if amount:
            expected_sources.append((index + 1, blob[16:32], key[2], pid, amount))
    return expected_sources


CAPTURE_REGISTRIES = (
    ('tables', 'ESM1', 'digest', (
        ('player_data', 'pid,account_name,racewar,copper,silver,gold,platinum,wallet_revision,save_revision', 'pid'),
        ('account_banks', 'id,account_name,racewar,bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision', 'id'),
        ('shopkeepers', 'id,shop_id,mob_vnum,room_vnum,cash,shop_revision,keeper_roaming', 'id'),
        ('ships', 'id,owner_name,money', 'id'),
        ('auctions', 'id,seller_pid,status,winning_bidder_pid,cur_price,buy_price,quantity,auction_revision,custody_state,listing_operation_id,obj_vnum,obj_blob_str', 'id'),
        ('auction_money_pickups', 'pid,money,claim_revision', 'pid'),
        ('auction_item_pickups', 'id,pid,obj_blob_str,retrieved,quantity', 'id'),
        ('auction_item_custody', 'auction_id,slot,item_uid,item_revision,vnum,obj_blob,claim_pid,claim_operation_id,claimed_at IS NOT NULL', 'auction_id,slot'),
        ('collector_catalog_state', 'state_id,catalog_revision,next_listing', 'state_id'),
        ('collector_deaths', 'death_operation_id,beneficiary_pid,death_time,collection_delay,sale_delay,holding_duration,price_percent,minimum_value,hint_state,hint_revision', 'death_operation_id'),
        ('collector_listings', 'listing_id,death_operation_id,beneficiary_pid,item_uid,status,holding_paused,due_at,listing_revision,item_revision,price_value,record_blob,item_blob', 'listing_id'),
        ('item_current_owner', 'item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,coin_payload', 'item_uid'),
        ('item_owner_revision', 'owner_type,owner_id,owner_context_id,revision', 'owner_type,owner_id,owner_context_id'),
        ('item_uid_allocator', 'allocator_id,next_uid', 'allocator_id'),
        ('item_ownership_quarantine', 'quarantine_id,item_uid,source_table,source_row_id,conflict_code,evidence,repaired_at IS NOT NULL', 'quarantine_id'),
        ('auction_reconciliation_quarantine', 'quarantine_id,auction_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL', 'quarantine_id'),
        ('collector_reconciliation_quarantine', 'quarantine_id,listing_id,item_uid,conflict_code,evidence,repaired_at IS NOT NULL', 'quarantine_id'),
        ('critical_operation_inbox', 'operation_id,command_hash,keys_hash,command_type,schema_version,payload_version,status,result_code,failure_stage,durable_revision,result_payload,committed_at IS NOT NULL', 'operation_id'),
        ('critical_outbox', 'outbox_id,operation_id,event_index,destination,event_type,payload_version,payload,status,attempt_count,last_error_code,delivered_at IS NOT NULL,dead_lettered_at IS NOT NULL', 'outbox_id'),
        ('economic_account_mapping', 'mapping_id,lineage,account_kind,context_id,backend_kind,locator_kind,native_id,active_native_id,creating_operation_id,retiring_operation_id,revision', 'mapping_id'),
    )),
    ('item_sources', 'EIM1', 'item_sources_digest', (
        ('player_pet_items', 'id,pet_id,container_id,obj_uid,vnum', 'id'),
        ('shopkeeper_items', 'id,shopkeeper_id,container_id,obj_uid,vnum,item_condition', 'id'),
        ('siege_items', 'id,room_vnum,container_id,obj_uid,vnum', 'id'),
    )),
    ('item_equipment_sources', 'EIE2', 'item_equipment_sources_digest', (
        ('item_current_owner', 'item_uid,equipment_slot', 'item_uid'),
    )),
)


def capture_frame(value: bytes) -> bytes:
    return struct.pack("<Q", len(value)) + value


def captured_hex(value, size=None):
    if (type(value) is not str or len(value) % 2 or
            (size is not None and len(value) != size * 2) or
            len(value) > 2 * 1024 * 1024 or not re.fullmatch("[0-9a-f]*", value)):
        raise OriginError("invalid captured source bytes")
    return bytes.fromhex(value)


def validate_captured_sources(snapshot: dict) -> dict:
    """Independently validate the native DTO's raw framing, never its authority."""
    try:
        if type(snapshot) is not dict or type(snapshot["version"]) is not int or snapshot["version"] not in (1, 2):
            raise OriginError("invalid captured source version")
        limits = dict(rows=262144, cells=4 * 1024 * 1024, cell_bytes=64 * 1024 * 1024)
        totals = dict.fromkeys(limits, 0)
        for name, maximum in limits.items():
            if type(snapshot[name]) is not int or not 0 <= snapshot[name] <= maximum:
                raise OriginError("captured source bounds exceeded")
        decoded = {}
        for group, tag, field, specifications in CAPTURE_REGISTRIES:
            tables = snapshot[group]
            if group == "item_equipment_sources" and snapshot["version"] == 1:
                if tables != [] or snapshot[field] != "00" * 32 or snapshot["custody_digest"] != "00" * 32:
                    raise OriginError("invalid historical captured equipment")
                decoded[group] = []
                continue
            if type(tables) is not list or len(tables) != len(specifications):
                raise OriginError("invalid captured source registry")
            registry = hashlib.sha256(capture_frame(tag.encode()) + struct.pack("<Q", len(tables)))
            result = []
            for table, (name, columns, order) in zip(tables, specifications):
                expected = columns.split(",")
                if type(table) is not dict or table["name"] != name or table["columns"] != expected:
                    raise OriginError("invalid captured source definition")
                definition = hashlib.sha256(capture_frame(b"ESD1") + capture_frame(name.encode()) +
                    capture_frame(order.encode()) + struct.pack("<Q", len(expected)) +
                    b"".join(capture_frame(column.encode()) for column in expected)).digest()
                if captured_hex(table["definition_digest"], 32) != definition:
                    raise OriginError("captured source definition mismatch")
                rows = table["rows"]
                if type(rows) is not list or len(rows) > limits["rows"] - totals["rows"]:
                    raise OriginError("captured source bounds exceeded")
                totals["rows"] += len(rows)
                totals["cells"] += len(rows) * len(expected)
                if totals["cells"] > limits["cells"]:
                    raise OriginError("captured source bounds exceeded")
                content = hashlib.sha256(capture_frame(b"EST1") + definition + struct.pack("<Q", len(rows)))
                captured = []
                for row in rows:
                    if type(row) is not dict or type(row["cells"]) is not list or len(row["cells"]) != len(expected):
                        raise OriginError("invalid captured source cells")
                    hashed = hashlib.sha256(capture_frame(b"ESR1") + definition)
                    cells = []
                    for value in row["cells"]:
                        cell = None if value is None else captured_hex(value)
                        hashed.update(struct.pack("<Q", int(cell is not None)))
                        if cell is not None:
                            totals["cell_bytes"] += len(cell)
                            if totals["cell_bytes"] > limits["cell_bytes"]:
                                raise OriginError("captured source bounds exceeded")
                            hashed.update(capture_frame(cell))
                        cells.append(cell)
                    row_digest = hashed.digest()
                    if captured_hex(row["digest"], 32) != row_digest:
                        raise OriginError("captured source row mismatch")
                    content.update(row_digest)
                    captured.append(dict(cells=cells, digest=row_digest))
                content_digest = content.digest()
                if captured_hex(table["content_digest"], 32) != content_digest:
                    raise OriginError("captured source content mismatch")
                registry.update(content_digest)
                result.append(dict(name=name, rows=captured, content_digest=content_digest))
            if registry.digest() != captured_hex(snapshot[field], 32):
                raise OriginError("captured source manifest mismatch")
            decoded[group] = result
        if totals != {name: snapshot[name] for name in totals}:
            raise OriginError("captured source count mismatch")
        if snapshot["version"] == 2:
            main = next(table for table in decoded["tables"] if table["name"] == "item_current_owner")
            equipment = decoded["item_equipment_sources"][0]
            if (len(main["rows"]) != len(equipment["rows"]) or any(left["cells"][0] != right["cells"][0]
                    for left, right in zip(main["rows"], equipment["rows"]))):
                raise OriginError("captured equipment correspondence mismatch")
            custody = hashlib.sha256(capture_frame(b"ESC2") + b"".join(captured_hex(snapshot[field], 32)
                for field in ("digest", "item_sources_digest", "item_equipment_sources_digest"))).digest()
            if custody != captured_hex(snapshot["custody_digest"], 32):
                raise OriginError("captured custody digest mismatch")
        return decoded
    except (KeyError, TypeError, ValueError, StopIteration) as error:
        if isinstance(error, OriginError):
            raise
        raise OriginError("invalid captured source evidence") from error


def captured_unsigned(value, maximum=2**64-1):
    if type(value) is not bytes or not re.fullmatch(rb"0|[1-9][0-9]*", value) or len(value) > 20:
        raise OriginError("invalid captured native integer")
    number = int(value)
    if number > maximum:
        raise OriginError("invalid captured native integer")
    return number


def verify_captured_item_opening(row: dict, packet: dict) -> dict:
    """Check original item preimages against one retained root.

    Legacy boundary/holding hashes are explicit inputs, not independently
    authenticated authority. Source provenance and complete item selection
    remain separate prerequisites; this function cannot attest activation.
    """
    try:
        if type(packet) is not dict or packet["format"] != "economic_sql_captured_opening_v1":
            raise OriginError("invalid captured opening format")
        if identity(captured_hex(packet["operation_id"], 16), "captured operation") != row["operation_id"]:
            raise OriginError("foreign captured opening operation")
        legacy = digest(captured_hex(packet["legacy_native_boundary_digest"], 32), "legacy native boundary")
        coverage = digest(captured_hex(packet["holding_coverage_digest"], 32), "holding coverage")
        snapshot = packet["source_snapshot"]
        decoded = validate_captured_sources(snapshot)
        blob = row["canonical_witness"]
        verify_baseline_root(row, blob[16:32], blob[32:48])
        holdings, items = decode_witness(row, blob[16:32], blob[32:48], blob[80:120])
        version, stride = witness_layout(blob)
        native = next(table for table in decoded["tables"] if table["name"] == "item_current_owner")
        owners = next(table for table in decoded["tables"] if table["name"] == "item_owner_revision")
        sources = []
        if items:
            if snapshot["version"] != 2:
                raise OriginError("captured equipment is unobserved")
            equipment = decoded["item_equipment_sources"][0]
            by_uid, previous = {}, 0
            for source, slot in zip(native["rows"], equipment["rows"]):
                uid = captured_unsigned(source["cells"][0])
                if uid <= previous:
                    raise OriginError("invalid captured native item order")
                previous = uid
                by_uid[uid] = source, slot
            by_owner = {}
            for owner in owners["rows"]:
                cells = owner["cells"]
                key = tuple(captured_unsigned(value, 255 if index == 0 else 2**64-1)
                            for index, value in enumerate(cells[:3]))
                captured_unsigned(cells[3])
                if key in by_owner:
                    raise OriginError("duplicate captured owner revision")
                by_owner[key] = owner
            for index, item in enumerate(items):
                source, slot = by_uid[item["uid"]]
                cells = source["cells"]
                owner_key = tuple(captured_unsigned(value, 255 if index == 0 else 2**64-1)
                                  for index, value in enumerate(cells[3:6]))
                projected = (captured_unsigned(cells[1]), None if cells[2] is None else captured_unsigned(cells[2]),
                             owner_key, captured_unsigned(cells[6]), captured_unsigned(cells[8], 255))
                expected = (item["root"], item["parent"], tuple(item["owner"]), item["revision"],
                            next(state for state, name in ITEM_STATES.items() if name == item["state"]))
                observed_slot = captured_unsigned(slot["cells"][1], 65535)
                if projected != expected or (version == 2 and observed_slot != item["equipment_slot"]):
                    raise OriginError("captured native item projection mismatch")
                if not 0 < captured_unsigned(cells[7], 2**31-1):
                    raise OriginError("invalid captured native item prototype")
                owner = by_owner[owner_key]
                source_digest = hashlib.sha256(b"EBS2" + b"".join(capture_frame(value["digest"])
                    for value in (source, slot, owner))).digest()
                offset = HEADER_BYTES + len(holdings) * HOLDING_BYTES + index * stride
                if blob[offset + stride - 32:offset + stride] != source_digest:
                    raise OriginError("captured item source digest mismatch")
                sources.append((item["uid"], source_digest))
            boundary = hashlib.sha256(b"ESN5" + b"".join(capture_frame(value) for value in
                (legacy, native["content_digest"], owners["content_digest"], equipment["content_digest"],
                 captured_hex(snapshot["item_sources_digest"], 32)))).digest()
            complete = hashlib.sha256(b"EIC2" + capture_frame(coverage) + struct.pack("<Q", len(sources)) +
                b"".join(struct.pack("<Q", uid) + capture_frame(source) for uid, source in sources)).digest()
        else:
            boundary, complete = legacy, coverage
        if blob[120:152] != boundary or blob[152:184] != complete:
            raise OriginError("captured opening digest mismatch")
        return dict(format="economic_sql_captured_item_bindings_v1", captured_source_framing_verified=True,
            item_bindings_verified=True, witness_item_count=len(items), captured_native_item_count=len(native["rows"]),
            witness_equipment_observed=version == 2, complete_item_selection_authenticated=False,
            legacy_digest_authority_authenticated=False, original_capture_provenance_authenticated=False,
            complete_source_capture_authenticated=False, activation_qualified=False, release_qualified=False)
    except (KeyError, TypeError, ValueError, StopIteration, IndexError, struct.error) as error:
        if isinstance(error, OriginError):
            raise
        raise OriginError("invalid captured opening evidence") from error


def verify_baseline_claim_rows(cursor, row: dict, holdings: list[dict]) -> None:
    parents = []
    if any(account_key(holding["account_key"])[1] in (4, 5) for holding in holdings):
        cursor.execute("SELECT " + ",".join(column + " AS policy_" + str(index)
            for index, column in enumerate(CLAIM_POLICY_COLUMNS)) +
            " FROM economic_sql_lifecycle_installation p LEFT JOIN critical_operation_inbox i "
            "ON i.operation_id=p.operation_id WHERE p.operation_id=%s LIMIT 2",
            (row["canonical_witness"][48:64],))
        parents = [tuple(value["policy_" + str(index)] for index in range(len(CLAIM_POLICY_COLUMNS)))
                   for value in cursor.fetchall()]
    if not verify_baseline_claim_policy(row, holdings, parents):
        return
    identities = [account_key(holding["account_key"])[2] for holding in holdings
                  if account_key(holding["account_key"])[1] == 5]
    mappings = []
    for start in range(0, len(identities), 256):
        page = identities[start:start + 256]
        cursor.execute("SELECT " + ",".join(CLAIM_MAPPING_COLUMNS) +
            " FROM economic_account_mapping WHERE lineage=%s AND mapping_id IN (" +
            ",".join("%s" for _ in page) + ") LIMIT %s", (row["root_lineage"], *page, len(page) + 1))
        mappings.extend(tuple(value[column] for column in CLAIM_MAPPING_COLUMNS) for value in cursor.fetchall())
    expected = verify_baseline_claim_identity(row, holdings, mappings)
    cursor.execute("SELECT source_slot,lineage,claim_mapping_id,beneficiary_pid,amount "
        "FROM economic_pending_claim_source WHERE source_operation_id=%s ORDER BY source_slot LIMIT %s",
        (row["operation_id"], len(expected) + 1))
    actual = [tuple(value[column] for column in ("source_slot", "lineage", "claim_mapping_id", "beneficiary_pid", "amount"))
              for value in cursor.fetchall()]
    if not same_projection(actual, expected):
        raise OriginError("EAB1 claim origin mismatch")


BASELINE_PROJECTIONS = {
    "economic_accounting_account_effect": ("operation_id", "account_index", "account_key",
        *(side + "_" + coin for side in ("before", "after") for coin in COINS),
        "before_revision", "after_revision"),
    "economic_accounting_coin_posting": ("operation_id", "line_index", "event_index", "account_index",
        "child_index", *("delta_" + coin for coin in COINS), "copper_value"),
    "economic_baseline_reservation": ("lineage", "epoch", "identity_kind", "identity_id", "operation_id"),
}
BASELINE_ZERO_EFFECTS = (
    ("economic_accounting_child", "operation_id"),
    ("economic_accounting_item_reference", "operation_id"),
    ("currency_ledger", "operation_id"),
    ("item_ownership_ledger", "operation_id"),
    ("critical_outbox", "operation_id"),
    ("economic_accounting_child", "child_operation_id"),
)


def verify_baseline_zero_effects(cursor, lineage: bytes, epoch: bytes) -> None:
    """A baseline retains opening positions, never native events or children."""
    probes = ["EXISTS(SELECT 1 FROM " + table + " p JOIN economic_baseline_witness w "
              "ON w.operation_id=p." + column + " WHERE w.lineage=%s AND w.epoch=%s) AS effect_" + str(index)
              for index, (table, column) in enumerate(BASELINE_ZERO_EFFECTS)]
    cursor.execute("SELECT " + ",".join(probes), (lineage, epoch) * len(probes))
    result = cursor.fetchone()
    for index, (table, column) in enumerate(BASELINE_ZERO_EFFECTS):
        value = result.get("effect_" + str(index)) if result is not None else None
        if type(value) is not int or value != 0:
            raise OriginError("EAB1 SQL zero-effect mismatch: " + table + "." + column)


def baseline_projection_source(table: str, lineage: bytes, epoch: bytes | None = None) -> tuple[str, str, tuple]:
    """Include reservations by their claimed scope and by their witness root."""
    scope = "w.lineage=%s" + (" AND w.epoch=%s" if epoch is not None else "")
    parameters = (lineage,) if epoch is None else (lineage, epoch)
    if table == "economic_baseline_reservation":
        return (" LEFT JOIN economic_baseline_witness w ON w.operation_id=p.operation_id",
                "((" + scope.replace("w.", "p.") + ") OR (" + scope + "))", parameters * 2)
    return " JOIN economic_baseline_witness w ON w.operation_id=p.operation_id", scope, parameters


def baseline_projection_bound(cursor, lineage: bytes, epoch: bytes | None = None) -> int:
    """Bound all three fixed-width projection families before fetching rows."""
    counts, parameters = [], ()
    for table in BASELINE_PROJECTIONS:
        join, scope, arguments = baseline_projection_source(table, lineage, epoch)
        counts.append("SELECT COUNT(*) AS n FROM " + table + " p" + join + " WHERE " + scope)
        parameters += arguments
    cursor.execute("SELECT CAST(COALESCE(SUM(n),0) AS UNSIGNED) AS projection_rows FROM (" +
                   " UNION ALL ".join(counts) + ") baseline_projections", parameters)
    bounds = cursor.fetchone()
    if (bounds is None or type(bounds["projection_rows"]) is not int or
            not 0 <= bounds["projection_rows"] <= MAX_ROWS):
        raise OriginError("baseline SQL projection source exceeds audit input limit")
    return bounds["projection_rows"]


def verify_baseline_projections(cursor, verified: list[tuple[dict, dict]], lineage: bytes, epoch: bytes) -> None:
    """Compare persisted baseline details with independently verified EAP1/EAB1."""
    expected = {table: [] for table in BASELINE_PROJECTIONS}
    effects, postings, reservations = expected.values()
    for row, plan in verified:
        operation = row["operation_id"]
        effects.extend((operation, index, key, *before, *after, before_revision, after_revision)
                       for index, (key, before, after, before_revision, after_revision) in enumerate(plan["effects"]))
        postings.extend((operation, index, event, account, child, *delta, amount)
                        for index, (event, account, child, delta, amount) in enumerate(plan["postings"]))
        blob = row["canonical_witness"]
        _, stride = witness_layout(blob)
        holdings, items = struct.unpack_from("<II", blob, 184)
        reservations.extend((lineage, epoch, 1, unsigned(blob[212 + index * 112:220 + index * 112]), operation)
                            for index in range(holdings))
        reservations.extend((lineage, epoch, 2,
                             unsigned(blob[192 + holdings * 112 + index * stride:200 + holdings * 112 + index * stride]),
                             operation) for index in range(items))
    if sum(map(len, expected.values())) > MAX_ROWS:
        raise OriginError("baseline SQL projection source exceeds audit input limit")
    total = baseline_projection_bound(cursor, lineage, epoch)
    if total != sum(map(len, expected.values())):
        raise OriginError("EAB1 SQL projection mismatch")
    fetched = 0
    for table, fields in BASELINE_PROJECTIONS.items():
        join, scope, parameters = baseline_projection_source(table, lineage, epoch)
        cursor.execute("SELECT " + ",".join("p." + field for field in fields) + " FROM " + table + " p" + join +
                       " WHERE " + scope + " LIMIT %s", (*parameters, MAX_ROWS - fetched + 1))
        rows = cursor.fetchall()
        fetched += len(rows)
        if fetched > MAX_ROWS:
            raise OriginError("baseline SQL projection source exceeds audit input limit")
        try:
            actual = [tuple(row[field] for field in fields) for row in rows]
            if not same_projection(sorted(actual), sorted(expected[table])):
                raise OriginError("EAB1 SQL projection mismatch")
        except (KeyError, TypeError) as error:
            raise OriginError("EAB1 SQL projection mismatch") from error


def read_origins_in_transaction(cursor, lineage: bytes, epoch: bytes, *, captured_opening=None) -> dict:
    """Read verified origins within a caller-owned consistent read-only cut."""
    identity(lineage, "lineage")
    identity(epoch, "epoch")
    cursor.execute(
        "SELECT TABLE_NAME AS table_name,ENGINE AS engine FROM information_schema.tables "
        "WHERE table_schema=DATABASE() AND table_name IN "
        "('economic_baseline_control','economic_baseline_witness',"
        "'economic_accounting_operation','critical_operation_inbox',"
        "'economic_accounting_account_effect','economic_accounting_coin_posting','economic_baseline_reservation',"
        "'economic_accounting_child','economic_accounting_item_reference',"
        "'currency_ledger','item_ownership_ledger','critical_outbox',"
        "'economic_sql_lifecycle_installation','economic_account_mapping','economic_pending_claim_source')")
    engines = {row["table_name"]: row["engine"] for row in cursor.fetchall()}
    baseline_tables = {"economic_baseline_control", "economic_baseline_witness", "economic_accounting_operation",
        "critical_operation_inbox", *BASELINE_PROJECTIONS, *(table for table, _ in BASELINE_ZERO_EFFECTS)}
    if any(engines.get(table) != "InnoDB" for table in baseline_tables):
        raise OriginError("SQL baseline source is missing or not InnoDB")
    cursor.execute("SELECT COUNT(*) AS column_count FROM information_schema.columns "
                   "WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness' "
                   "AND column_name='command_accepted_at_usec'")
    admission = cursor.fetchone()
    if (admission is None or type(admission["column_count"]) is not int or
            admission["column_count"] not in (0, 1)):
        raise OriginError("invalid SQL baseline admission column metadata")
    admission_column = "w.command_accepted_at_usec" if admission["column_count"] else "NULL"
    cursor.execute("SELECT COUNT(*) AS column_count FROM information_schema.columns "
                   "WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness' "
                   "AND column_name='claim_origin_version'")
    policy = cursor.fetchone()
    if (policy is None or type(policy["column_count"]) is not int or policy["column_count"] not in (0, 1)):
        raise OriginError("invalid SQL baseline claim policy column metadata")
    policy_column = "w.claim_origin_version" if policy["column_count"] else "NULL"
    cursor.execute(
        "SELECT opening_account,revision,last_operation_id FROM economic_baseline_control "
        "WHERE lineage=%s AND epoch=%s", (lineage, epoch))
    control = cursor.fetchone()
    if control is None:
        raise OriginError("missing SQL baseline control")
    opening = control["opening_account"]
    if not isinstance(opening, bytes) or len(opening) != 40:
        raise OriginError("invalid SQL opening account")
    opening_lineage, opening_kind, _, _ = account_key(opening.hex())
    if opening_lineage != lineage.hex() or opening_kind != 9:
        raise OriginError("invalid SQL opening account")
    if type(control["revision"]) is not int or control["revision"] < 1:
        raise OriginError("baseline has no committed opening witness")
    cursor.execute(
        "SELECT COUNT(*) AS row_count,CAST(COALESCE(SUM(OCTET_LENGTH(w.canonical_witness)+"
        "COALESCE(OCTET_LENGTH(o.canonical_intent),0)+COALESCE(OCTET_LENGTH(o.canonical_plan),0)),0) AS UNSIGNED) "
        "AS blob_bytes FROM economic_baseline_witness w LEFT JOIN economic_accounting_operation o "
        "ON o.operation_id=w.operation_id WHERE w.lineage=%s AND w.epoch=%s",
        (lineage, epoch))
    bounds = cursor.fetchone()
    if (bounds is None or type(bounds["row_count"]) is not int or
            not 0 <= bounds["row_count"] <= MAX_ROWS or type(bounds["blob_bytes"]) is not int or
            not 0 <= bounds["blob_bytes"] <= MAX_INPUT_BYTES):
        raise OriginError("baseline witness source exceeds audit input limit")
    cursor.execute(
        "SELECT w.operation_id,w.book_revision,w.holding_count,w.item_count,"
        "w.witness_digest,w.canonical_witness,w.witness_version,o.reason,o.outcome,o.result_code,"
        "o.lineage AS root_lineage,o.epoch AS root_epoch,o.original_operation_id,"
        "o.accounting_version,o.writer_id,o.policy_version,o.compiler_version,o.actor_kind,o.actor_id,"
        "o.source_event,o.intent_digest,o.domain_digest,o.plan_digest,o.canonical_intent,o.canonical_plan,"
        "o.account_count,o.posting_count,o.child_count,o.item_event_count,o.before_witness_count,o.after_witness_count,"
        "i.durable_revision AS inbox_revision,i.command_type AS inbox_type,i.schema_version AS inbox_schema,"
        "i.payload_version AS inbox_payload,i.result_payload AS inbox_result_payload,"
        "i.keys_hash AS inbox_keys_hash,i.command_hash AS inbox_command_hash," +
        admission_column + " AS command_accepted_at_usec," + policy_column + " AS claim_origin_version,"
        "i.status AS inbox_status,i.result_code AS inbox_result,"
        "i.failure_stage AS inbox_failure_stage,"
        "(i.committed_at IS NOT NULL) AS inbox_committed_at_present "
        "FROM economic_baseline_witness w "
        "LEFT JOIN economic_accounting_operation o ON o.operation_id=w.operation_id "
        "LEFT JOIN critical_operation_inbox i ON i.operation_id=w.operation_id "
        "WHERE w.lineage=%s AND w.epoch=%s ORDER BY w.book_revision LIMIT %s",
        (lineage, epoch, MAX_ROWS + 1))
    witnesses = cursor.fetchall()
    if len(witnesses) != bounds["row_count"] or control["revision"] != len(witnesses):
        raise OriginError("baseline witness revision gap or limit exceeded")
    holdings: list[dict] = []
    items: list[dict] = []
    seen_keys: set[str] = set()
    seen_lifetimes: set[int] = set()
    seen_uids: set[int] = set()
    verified = []
    for expected_revision, row in enumerate(witnesses, 1):
        if not same_projection(tuple(row[name] for name in (
                "book_revision", "reason", "outcome", "result_code", "inbox_status", "inbox_result",
                "inbox_failure_stage", "inbox_committed_at_present")), (expected_revision, 38, 1, 0, 1, 0, 0, 1)):
            raise OriginError("uncommitted or noncanonical baseline witness")
        batch_holdings, batch_items = decode_witness(row, lineage, epoch, opening)
        verified.append((row, verify_baseline_root(row, lineage, epoch)))
        if (any(account_key(holding["account_key"])[1] in (4, 5) for holding in batch_holdings) and
                any(engines.get(table) != "InnoDB" for table in ("economic_sql_lifecycle_installation",
                    "economic_account_mapping", "economic_pending_claim_source"))):
            raise OriginError("SQL claim origin source is missing or not InnoDB")
        verify_baseline_claim_rows(cursor, row, batch_holdings)
        for holding in batch_holdings:
            key = holding["account_key"]
            lifetime = account_key(key)[2]
            if key in seen_keys or lifetime in seen_lifetimes:
                raise OriginError("duplicate baseline account across witnesses")
            seen_keys.add(key)
            seen_lifetimes.add(lifetime)
            holdings.append(holding)
        for item in batch_items:
            uid = item["uid"]
            if uid in seen_uids:
                raise OriginError("duplicate baseline UID across witnesses")
            seen_uids.add(uid)
            items.append(item)
        if len(holdings) > MAX_ROWS or len(items) > MAX_ROWS:
            raise OriginError("baseline origin collection limit exceeded")
    if (witnesses[-1]["operation_id"] if witnesses else None) != control["last_operation_id"]:
        raise OriginError("baseline control terminal witness mismatch")
    verify_baseline_projections(cursor, verified, lineage, epoch)
    verify_baseline_zero_effects(cursor, lineage, epoch)
    baseline_sources = {}
    for row in witnesses:
        blob = row["canonical_witness"]
        # Native baseline source identity is preparation/epoch/batch, slot zero.
        source = struct.pack("<HH", 10, 1) + blob[48:64] + epoch + blob[72:80] + bytes(4)
        baseline_sources[row["operation_id"].hex()] = source.hex()
    result = {"format": "economic_sql_audit_origins_v1", "lineage": lineage.hex(),
              "epoch": epoch.hex(), "control_revision": control["revision"],
              "witness_count": len(witnesses), "account_origins": holdings,
              "item_origins": items,
              "baseline_operation_ids": [row["operation_id"].hex() for row in witnesses],
              "baseline_source_events": baseline_sources}
    if captured_opening is not None:
        try:
            operation = identity(captured_hex(captured_opening["operation_id"], 16), "captured operation")
        except (KeyError, TypeError) as error:
            raise OriginError("invalid captured opening operation") from error
        selected = [row for row in witnesses if row["operation_id"] == operation]
        if len(selected) != 1:
            raise OriginError("captured opening is outside the selected book")
        result["captured_item_bindings"] = verify_captured_item_opening(selected[0], captured_opening)
    encoded = json.dumps(result, sort_keys=True, separators=(",", ":")).encode()
    if len(encoded) > MAX_INPUT_BYTES:
        raise OriginError("origin export exceeds audit input limit")
    return result


def capture(connection, lineage: bytes, epoch: bytes, *, captured_opening=None) -> dict:
    """Read one bounded, consistent baseline cut; always end it with rollback."""
    identity(lineage, "lineage")
    identity(epoch, "epoch")
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        return read_origins_in_transaction(cursor, lineage, epoch, captured_opening=captured_opening)
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()


def load_captured_opening(path: Path) -> dict:
    """Read private original evidence; refuse truncation and ambiguous JSON."""
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise OriginError("duplicate captured opening field")
            result[key] = value
        return result
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0) | getattr(os, "O_NONBLOCK", 0))
    with os.fdopen(descriptor, "rb") as stream:
        info = os.fstat(stream.fileno())
        if (not stat.S_ISREG(info.st_mode) or info.st_nlink != 1 or info.st_size > MAX_INPUT_BYTES or
                (os.name == "posix" and info.st_mode & 0o077)):
            raise OriginError("captured opening input is not protected or exceeds input limit")
        data = stream.read(MAX_INPUT_BYTES + 1)
    if len(data) > MAX_INPUT_BYTES:
        raise OriginError("captured opening input exceeds input limit")
    try:
        result = json.loads(data, object_pairs_hook=unique)
        if type(result) is not dict or result.get("format") != "economic_sql_captured_opening_v1":
            raise OriginError("invalid captured opening format")
        return result
    except (ValueError, UnicodeError, RecursionError) as error:
        if isinstance(error, OriginError):
            raise
        raise OriginError("invalid captured opening JSON") from error


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=3306)
    parser.add_argument("--user", required=True)
    parser.add_argument("--database", required=True)
    parser.add_argument("--password-env", default="DB_PASSWORD")
    parser.add_argument("--lineage", required=True)
    parser.add_argument("--epoch", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--captured-opening-evidence", type=Path,
                        help="private original capture and legacy digest inputs for one retained opening")
    args = parser.parse_args()
    try:
        lineage, epoch = bytes.fromhex(args.lineage), bytes.fromhex(args.epoch)
        identity(lineage, "lineage")
        identity(epoch, "epoch")
        if not 1 <= args.port <= 65535:
            raise OriginError("invalid SQL port")
        captured_opening = load_captured_opening(args.captured_opening_evidence) if args.captured_opening_evidence else None
        password = os.environ[args.password_env]
        import pymysql
        try:
            connection = pymysql.connect(host=args.host, port=args.port, user=args.user,
                                         password=password, database=args.database,
                                         charset="utf8mb4", autocommit=True,
                                         cursorclass=pymysql.cursors.DictCursor,
                                         connect_timeout=5, read_timeout=30, write_timeout=5)
            try:
                result = capture(connection, lineage, epoch, captured_opening=captured_opening)
            finally:
                connection.close()
        except pymysql.MySQLError as error:
            raise OriginError(f"SQL read failed with code {error.args[0]}") from error
        encoded = (json.dumps(result, sort_keys=True, separators=(",", ":")) + "\n").encode()
        descriptor = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "wb") as output:
            output.write(encoded)
        return 0
    except (OriginError, OSError, ValueError, KeyError, ImportError) as error:
        print(f"SQL audit origin export refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
