#!/usr/bin/env python3
"""Read and verify SQL EAB1 opening origins without changing native authority.

This is one input to a future complete audit snapshot, not an audit snapshot or
an activation attestation. It contains only non-personal account keys and UIDs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, account_key, copper
from economic_restore_evidence import decode_intent, decode_plan

HEADER_BYTES = 192
HOLDING_BYTES = 112
ITEM_BYTES = 88
MAX_HOLDINGS_PER_WITNESS = 3071
MAX_ITEMS_PER_WITNESS = 6000
ITEM_STATES = {1: "live", 2: "tombstone", 3: "quarantined"}


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


def decode_witness(row: dict, lineage: bytes, epoch: bytes, opening: bytes) -> tuple[list[dict], list[dict]]:
    blob = row.get("canonical_witness")
    if not isinstance(blob, bytes) or len(blob) < HEADER_BYTES or len(blob) > 872144:
        raise OriginError("invalid EAB1 size")
    if digest(row.get("witness_digest"), "witness digest") != hashlib.sha256(blob).digest():
        raise OriginError("EAB1 digest mismatch")
    if (blob[:4] != b"EAB1" or struct.unpack_from("<HHII", blob, 4) !=
            (1, HEADER_BYTES, len(blob), 0)):
        raise OriginError("invalid EAB1 header")
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
    if (holding_count != row.get("holding_count") or item_count != row.get("item_count") or
            holding_count > MAX_HOLDINGS_PER_WITNESS or item_count > MAX_ITEMS_PER_WITNESS or
            len(blob) != HEADER_BYTES + holding_count * HOLDING_BYTES + item_count * ITEM_BYTES):
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
    previous_uid = 0
    for _ in range(item_count):
        uid = unsigned(blob[offset:offset + 8])
        owner_type = blob[offset + 8]
        state = blob[offset + 9]
        if (uid <= previous_uid or not 1 <= owner_type <= 11 or state not in ITEM_STATES or
                blob[offset + 10:offset + 16] != bytes(6)):
            raise OriginError("invalid or duplicate EAB1 item")
        previous_uid = uid
        owner_id, context, root, parent, revision = struct.unpack_from("<5Q", blob, offset + 16)
        system_owner = owner_type in (7, 8)
        owner_valid = (owner_id == 0 and context == 0 if system_owner else
                       owner_id > 0 and (owner_type != 10 or context == 0) and
                       (owner_type != 11 or 0 < context <= 2**31 - 1))
        if not owner_valid or not root or parent == uid:
            raise OriginError("invalid EAB1 item topology")
        digest(blob[offset + 56:offset + 88], "item source digest")
        items.append({"uid": uid, "origin": "baseline", "revision": revision,
                      "root": root, "parent": parent or None,
                      "owner": [owner_type, owner_id, context], "state": ITEM_STATES[state]})
        offset += ITEM_BYTES
    return holdings, items


def verify_baseline_root(row: dict, lineage: bytes, epoch: bytes) -> None:
    """Bind all EAB1 bytes and their opening projections to a committed root."""
    try:
        blob = row["canonical_witness"]
        operation = hashlib.sha256(blob[48:64] + struct.pack("<I", 0x42415345) + blob[72:80]).digest()[:16]
        source = struct.pack("<HH", 10, 1) + blob[48:64] + epoch + blob[72:80] + bytes(4)
        expected = (lineage, epoch, operation, bytes(16), 1, 4, 1, 1, 2,
                    unsigned(blob[64:72]), 38, source)
        root = (row["root_lineage"], row["root_epoch"], row["operation_id"],
                row["original_operation_id"] or bytes(16),
                *(row[name] for name in ("accounting_version", "writer_id", "policy_version",
                                         "compiler_version", "actor_kind", "actor_id", "reason")),
                row["source_event"])
        if (root != expected or row["original_operation_id"] is not None or
                row["witness_version"] != 1 or row["inbox_revision"] != row["book_revision"] or
                (row["inbox_type"], row["inbox_schema"], row["inbox_payload"], row["inbox_result_payload"]) !=
                (20, 2, 1, b"")):
            raise OriginError("EAB1 committed root mismatch")
        intent = decode_intent(row["canonical_intent"])
        plan = decode_plan(row["canonical_plan"])
        payload = b"EBC1" + struct.pack("<HHII", 1, 48, len(blob), 0) + hashlib.sha256(blob).digest()
        domain = hashlib.sha256(b"DURIS-ECONOMIC-DOMAIN-V1\0" + struct.pack("<HHI", 20, 1, 48) + payload).digest()
        if (len(row["canonical_intent"]) != 256 or intent["metadata"] != expected or
                plan["metadata"] != expected or intent["domain_digest"] != domain or
                plan["domain_digest"] != domain or row["domain_digest"] != domain or
                intent["intent_digest"] != plan["intent_digest"] or
                row["intent_digest"] != intent["intent_digest"] or row["plan_digest"] != plan["plan_digest"]):
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
        snapshots = [blob[192 + holdings * 112 + index * 88:192 + holdings * 112 + index * 88 + 56] + bytes(8)
                     for index in range(items)]
        counts = (len(effects), len(postings), 0, items, items, 0)
        if (plan["counts"] != counts or
                tuple(row[name] for name in ("account_count", "posting_count", "child_count",
                                              "before_witness_count", "after_witness_count", "item_event_count")) != counts or
                row["canonical_plan"][256:] != b"".join(effects + postings + snapshots + snapshots)):
            raise OriginError("EAB1 committed root mismatch")
    except (ValueError, KeyError, TypeError, struct.error) as error:
        raise OriginError("EAB1 committed root mismatch") from error


def read_origins_in_transaction(cursor, lineage: bytes, epoch: bytes) -> dict:
    """Read verified origins within a caller-owned consistent read-only cut."""
    identity(lineage, "lineage")
    identity(epoch, "epoch")
    cursor.execute(
        "SELECT TABLE_NAME AS table_name,ENGINE AS engine FROM information_schema.tables "
        "WHERE table_schema=DATABASE() AND table_name IN "
        "('economic_baseline_control','economic_baseline_witness',"
        "'economic_accounting_operation','critical_operation_inbox')")
    engines = {row["table_name"]: row["engine"] for row in cursor.fetchall()}
    if (len(engines) != 4 or any(engine != "InnoDB" for engine in engines.values())):
        raise OriginError("SQL baseline source is missing or not InnoDB")
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
        "SELECT COUNT(*) AS row_count,COALESCE(SUM(OCTET_LENGTH(w.canonical_witness)+"
        "COALESCE(OCTET_LENGTH(o.canonical_intent),0)+COALESCE(OCTET_LENGTH(o.canonical_plan),0)),0) "
        "AS blob_bytes FROM economic_baseline_witness w LEFT JOIN economic_accounting_operation o "
        "ON o.operation_id=w.operation_id WHERE w.lineage=%s AND w.epoch=%s",
        (lineage, epoch))
    bounds = cursor.fetchone()
    if (bounds is None or bounds["row_count"] > MAX_ROWS or
            bounds["blob_bytes"] > MAX_INPUT_BYTES):
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
    for expected_revision, row in enumerate(witnesses, 1):
        if (row["book_revision"] != expected_revision or row["reason"] != 38 or
                row["outcome"] != 1 or row["result_code"] != 0 or
                row["inbox_status"] != 1 or row["inbox_result"] != 0 or
                row["inbox_failure_stage"] != 0 or
                row["inbox_committed_at_present"] != 1):
            raise OriginError("uncommitted or noncanonical baseline witness")
        batch_holdings, batch_items = decode_witness(row, lineage, epoch, opening)
        verify_baseline_root(row, lineage, epoch)
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
    encoded = json.dumps(result, sort_keys=True, separators=(",", ":")).encode()
    if len(encoded) > MAX_INPUT_BYTES:
        raise OriginError("origin export exceeds audit input limit")
    return result


def capture(connection, lineage: bytes, epoch: bytes) -> dict:
    """Read one bounded, consistent baseline cut; always end it with rollback."""
    identity(lineage, "lineage")
    identity(epoch, "epoch")
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        return read_origins_in_transaction(cursor, lineage, epoch)
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()

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
    args = parser.parse_args()
    try:
        lineage, epoch = bytes.fromhex(args.lineage), bytes.fromhex(args.epoch)
        identity(lineage, "lineage")
        identity(epoch, "epoch")
        if not 1 <= args.port <= 65535:
            raise OriginError("invalid SQL port")
        password = os.environ[args.password_env]
        import pymysql
        try:
            connection = pymysql.connect(host=args.host, port=args.port, user=args.user,
                                         password=password, database=args.database,
                                         charset="utf8mb4", autocommit=True,
                                         cursorclass=pymysql.cursors.DictCursor,
                                         connect_timeout=5, read_timeout=30, write_timeout=5)
            try:
                result = capture(connection, lineage, epoch)
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
