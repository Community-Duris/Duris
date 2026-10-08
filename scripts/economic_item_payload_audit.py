"""Independent bounded item-payload decoding and modern room diagnostics.

This module has no database, producer or mutation dependency. Passing these
checks does not authenticate an original accounting root or runtime publication.
"""
from collections import Counter, defaultdict, deque
import struct

MAX_PAYLOAD_BYTES = 4 * 1024 * 1024
MAX_CODEC_ROWS = 8192
MAX_ROOM_ITEM_BYTES = 131072
MAX_ROOM_GRAPH_ITEMS = 3000
MAX_ROOM_GRAPH_BYTES = MAX_ROOM_ITEM_BYTES + 4 * MAX_ROOM_GRAPH_ITEMS
MAX_ROOM_ROOTS = 4096
MAX_ROOM_DEPTH = 32


class PayloadError(ValueError):
    pass


def decode_single_item(blob: bytes, label: str = "item") -> dict:
    if not isinstance(blob, bytes) or not blob or len(blob) > MAX_PAYLOAD_BYTES:
        raise PayloadError("invalid " + label + " payload size")
    offset, remaining = 0, MAX_CODEC_ROWS - 1

    def number(code):
        nonlocal offset
        size = struct.calcsize("<" + code)
        if size > len(blob) - offset:
            raise PayloadError("truncated " + label + " payload")
        result = struct.unpack_from("<" + code, blob, offset)[0]
        offset += size
        return result

    def string():
        nonlocal offset
        size = number("I")
        if size > 4096 or size > len(blob) - offset:
            raise PayloadError("invalid " + label + " item string")
        offset += size

    def rows():
        nonlocal remaining
        count = number("I")
        if count > remaining:
            raise PayloadError(label + " nested row count exceeds limit")
        remaining -= count
        return count

    if number("I") != 1:
        raise PayloadError(label + " payload must contain exactly one item")
    result = dict(parent=number("i"), equipment_slot=number("h"), uid=number("Q"),
                  generated_key=number("q"), vnum=number("i"), item_type=number("b"),
                  string_mask=number("B"))
    for _ in range(4):
        string()
    result["values"] = [number("i") for _ in range(8)]
    for _ in range(6):
        number("q")
    flags = [number("I") for _ in range(5)]
    result["extra_flags"] = flags[1]
    for code in ("i", "b", "i", "h", "h"):
        number(code)
    for _ in range(5):
        number("Q")
    for _ in range(8):
        number("h")
    for _ in range(rows()):
        number("h"); number("h"); number("Q")
    for _ in range(rows()):
        string(); string()
        if number("B") > 1:
            raise PayloadError("invalid " + label + " spellbook flag")
        for _ in range(rows()):
            number("i")
    if offset != len(blob):
        raise PayloadError(label + " payload has trailing bytes")
    result["codec_rows"] = MAX_CODEC_ROWS - remaining
    return result


ROOM_COLLECTIONS = ("room_item_payloads", "room_item_proofs", "room_item_roots",
                    "room_item_members", "room_item_seasons")
PROOF_NUMBERS = ("reference_uid", "before_revision", "after_revision", "child_index",
                 "ledger_uid", "ledger_root", "ledger_parent", "ledger_revision",
                 "from_type", "from_id", "from_context", "to_type", "to_id", "to_context",
                 "reason_type", "reason_id", "command_type", "schema_version", "status",
                 "result_code", "failure_stage", "outcome", "operation_result")


def audit_room_items(native, emit, max_rows, max_bytes):
    def integer(row, name, minimum=0, maximum=2**64-1, nullable=False):
        value = row.get(name)
        if name not in row or (value is None and not nullable) or (value is not None and
                (type(value) is not int or not minimum <= value <= maximum)):
            raise PayloadError("invalid room item " + name)
        return value

    def hex_value(row, name, size, nullable=False):
        value = row.get(name)
        if name not in row or (value is None and not nullable) or (value is not None and
                (not isinstance(value, str) or len(value) != size*2 or
                 any(c not in "0123456789abcdef" for c in value))):
            raise PayloadError("invalid room item " + name)
        return value

    tables = []
    for name in ROOM_COLLECTIONS:
        rows = native.get(name)
        if not isinstance(rows, list) or len(rows) > max_rows or any(type(r) is not dict for r in rows):
            raise PayloadError("invalid room item collection " + name)
        tables.append(rows)
    payloads, proofs, roots, members, seasons = tables
    coverage = native.get("room_item_custody_coverage")
    fields = ("version", "payloads", "proofs", "roots", "members", "seasons", "payload_bytes")
    if (type(coverage) is not dict or set(coverage) != set(fields) or
            any(type(coverage[k]) is not int or coverage[k] < 0 for k in fields) or
            coverage["version"] != 1 or sum(map(len, tables)) > max_rows or
            any(coverage[k] != len(rows) for k, rows in zip(fields[1:6], tables)) or
            coverage["payload_bytes"] > max_bytes//2):
        raise PayloadError("invalid room item coverage")
    if payloads or roots or members:
        emit("room_item_full_runtime_authority_unqualified", scope="snapshot")
        emit("room_item_retained_root_authority_unqualified", scope="snapshot")
    state = []
    for row in seasons:
        integer(row, "state_id", 0, 255); integer(row, "season_epoch")
        if not isinstance(row.get("reset_status"), str) or len(row["reset_status"]) > 16:
            raise PayloadError("invalid room reset status")
        if row["state_id"] == 1 and row["season_epoch"] and row["reset_status"] == "active":
            state.append(row["season_epoch"])
    epoch = state[0] if len(seasons) == 1 and len(state) == 1 else None
    if (payloads or roots or members) and epoch is None:
        emit("room_item_season_unqualified", scope="snapshot")
    payload_index, decoded, total_bytes = {}, {}, 0
    for row in payloads:
        for field in ("uid", "revision", "season_epoch"):
            integer(row, field, 1)
        integer(row, "payload_version", 0, 65535); hex_value(row, "operation_id", 16)
        size = integer(row, "payload_bytes", 0, MAX_ROOM_ITEM_BYTES)
        body = row.get("payload")
        if (not isinstance(body, str) or len(body) != size*2 or
                any(c not in "0123456789abcdef" for c in body)):
            raise PayloadError("invalid room item payload representation")
        total_bytes += size
        key = row["uid"], row["revision"]
        if key in payload_index:
            emit("room_item_duplicate_payload", uid=key[0], revision=key[1])
        payload_index[key] = row
        if row["payload_version"] != 1:
            emit("room_item_payload_version_invalid", uid=key[0], revision=key[1])
        try:
            item = decode_single_item(bytes.fromhex(body))
            if (item["uid"] != row["uid"] or not 0 < item["vnum"] < 2**31 or
                    item["parent"] != -1 or item["equipment_slot"] != 0 or item["string_mask"] != 15 or
                    not 1 <= item["item_type"] <= 41 or item["item_type"] in (20, 24) or
                    item["extra_flags"] & 268435456):
                raise PayloadError("ordinary room literal identity is invalid")
            decoded[key] = item
        except PayloadError:
            emit("room_item_payload_invalid", uid=key[0], revision=key[1])
    if total_bytes != coverage["payload_bytes"]:
        raise PayloadError("room item payload byte coverage mismatch")
    by_payload = defaultdict(list)
    for row in proofs:
        key = integer(row, "uid", 1), integer(row, "revision", 1)
        if key not in payload_index:
            raise PayloadError("orphan room proof packet row")
        for field in PROOF_NUMBERS:
            integer(row, field, nullable=True)
        for field in ("reference_operation", "legacy_operation", "ledger_operation", "inbox_operation", "root_operation"):
            hex_value(row, field, 16, nullable=True)
        by_payload[key].append(row)

    def bound_proof(row, payload):
        op = payload["operation_id"]
        return (all(row[k] == op for k in ("reference_operation", "legacy_operation", "ledger_operation", "inbox_operation", "root_operation")) and
            row["reference_uid"] == row["ledger_uid"] == payload["uid"] and
            row["after_revision"] == row["ledger_revision"] == payload["revision"] and
            row["ledger_root"] is not None and row["ledger_root"] > 0 and
            ((row["ledger_uid"] == row["ledger_root"] and row["ledger_parent"] in (None, 0)) or
             (row["ledger_uid"] != row["ledger_root"] and row["ledger_parent"] is not None and
              row["ledger_parent"] > 0 and row["ledger_parent"] != row["ledger_uid"])) and
            row["before_revision"] is not None and row["before_revision"] + 1 == row["after_revision"] and
            row["child_index"] == 0 and row["from_type"] == 1 and row["from_id"] is not None and
            row["from_id"] > 0 and row["from_context"] == 0 and row["to_type"] == 3 and
            row["to_id"] is not None and 0 < row["to_id"] < 2**31 and row["to_context"] == 0 and
            row["reason_type"] == 6 and row["reason_id"] == row["to_id"] and
            row["command_type"] == 5 and row["schema_version"] == 2 and row["status"] == 1 and
            row["result_code"] == row["failure_stage"] == row["operation_result"] == 0 and row["outcome"] == 1)

    good_proofs = {}
    for key, payload in payload_index.items():
        selected = [r for r in by_payload[key] if bound_proof(r, payload)]
        if len(by_payload[key]) != 1 or len(selected) != 1:
            emit("room_item_retained_binding_invalid", uid=key[0], revision=key[1])
        else:
            good_proofs[key] = selected[0]
    root_ids = [integer(row, "root", 1) for row in roots]
    root_set = set(root_ids)
    if len(root_ids) > MAX_ROOM_ROOTS:
        emit("room_item_root_limit_exceeded", scope="snapshot")
    for root, count in Counter(root_ids).items():
        if count != 1:
            emit("room_item_duplicate_root", uid=root)
    grouped = defaultdict(list)
    for row in members:
        for field in ("uid", "root", "revision"):
            integer(row, field, 1)
        integer(row, "parent", nullable=True)
        integer(row, "vnum", -2**31, 2**31-1)
        integer(row, "state", 0, 255); integer(row, "owner_type", 0, 255)
        for field in ("owner_id", "owner_context", "equipment_slot", "saved_duplicates"):
            integer(row, field)
        integer(row, "owner_revision", nullable=True)
        if row["root"] not in root_set:
            raise PayloadError("orphan room member packet row")
        grouped[row["root"]].append(row)
    if "items" in native:
        items = native["items"]
        if not isinstance(items, list) or len(items) > max_rows or any(type(r) is not dict for r in items):
            raise PayloadError("invalid room current UID census")
        selected = []
        for row in items:
            integer(row, "uid", 1); integer(row, "root", 1)
            if row["root"] in root_set:
                selected.append(row)
        current_payload_uids = {r["uid"] for r in payloads if r["season_epoch"] == epoch}
        expected_roots = {r["root"] for r in items if r.get("state") == "live" and
            isinstance(r.get("owner"), list) and len(r["owner"]) == 3 and r["owner"][0] == 3 and
            r["uid"] in current_payload_uids}
        if not expected_roots <= root_set:
            emit("room_item_root_census_mismatch", scope="snapshot")
        if Counter(r["uid"] for r in selected) != Counter(r["uid"] for r in members):
            emit("room_item_current_census_mismatch", scope="snapshot")
        positions = {row["uid"]: row for row in selected}
        for row in members:
            current = positions.get(row["uid"])
            if current is None:
                continue
            expected = (row["root"], row["parent"], [row["owner_type"], row["owner_id"], row["owner_context"]],
                        row["revision"], row["vnum"], {1:"live",2:"tombstone",3:"quarantined"}.get(row["state"]), row["equipment_slot"])
            if tuple(current.get(k) for k in ("root","parent","owner","revision","vnum","state","equipment_slot")) != expected:
                emit("room_item_current_projection_mismatch", uid=row["uid"])
    for root in dict.fromkeys(root_ids):
        graph = grouped[root]; index = {r["uid"]: r for r in graph}; start = index.get(root)
        if not start or start["parent"] not in (None, 0) or len(index) != len(graph):
            emit("room_item_graph_identity_invalid", uid=root)
        if not graph or len(graph) > MAX_ROOM_GRAPH_ITEMS:
            emit("room_item_graph_item_limit_invalid", uid=root)
        children = defaultdict(list); graph_bytes = graph_rows = 0
        for row in graph:
            uid = row["uid"]; key = uid, row["revision"]
            if (row["state"] != 1 or row["owner_type"] != 3 or not 0 < row["owner_id"] < 2**31 or
                    row["owner_context"] != 0 or row["equipment_slot"] != 0 or not row["owner_revision"] or
                    (start and (row["owner_id"], row["owner_revision"]) != (start["owner_id"], start["owner_revision"]))):
                emit("room_item_current_custody_invalid", uid=uid)
            if row["saved_duplicates"]:
                emit("room_item_duplicate_saved_payload", uid=uid)
            payload = payload_index.get(key)
            if epoch is not None and (payload is None or payload["season_epoch"] != epoch):
                emit("room_item_current_payload_missing", uid=uid)
            elif payload:
                graph_bytes += payload["payload_bytes"]
                item = decoded.get(key)
                if item:
                    graph_rows += item["codec_rows"]
                    if item["vnum"] != row["vnum"]:
                        emit("room_item_current_vnum_mismatch", uid=uid)
                proof = good_proofs.get(key)
                if proof and (proof["ledger_root"] != root or (proof["ledger_parent"] or 0) != (row["parent"] or 0) or
                        proof["to_id"] != row["owner_id"] or proof["to_context"] != row["owner_context"]):
                    emit("room_item_current_binding_mismatch", uid=uid)
            if uid != root:
                if row["parent"] not in index or row["parent"] == uid:
                    emit("room_item_graph_parent_invalid", uid=uid)
                children[row["parent"]].append(uid)
        if graph_bytes > MAX_ROOM_GRAPH_BYTES or graph_rows > MAX_CODEC_ROWS:
            emit("room_item_graph_payload_budget_invalid", uid=root)
        reached = set(); queue = deque([(root, 1)]) if start else deque()
        while queue:
            uid, depth = queue.popleft()
            if uid in reached:
                emit("room_item_graph_cycle", uid=uid); continue
            reached.add(uid)
            if depth > MAX_ROOM_DEPTH:
                emit("room_item_graph_depth_invalid", uid=uid)
            queue.extend((child, depth+1) for child in children[uid])
        if len(reached) != len(graph):
            emit("room_item_graph_disconnected", uid=root)
