"""Independent read-only EAI1/EAP1 interpretation for retained SQL evidence.

No native mutation, codec, coordinator or storage implementation is imported.
Wire grammar and semantic decisions are compared with native decoder fixtures.
"""
import hashlib
import json
import struct

from reconcile_economy_accounting import (MAX_ROWS, ORDINARY_KINDS, account_key, copper,
                                          decode_source_event, same_projection, source_kind_allowed)

MAX_INTENT = 8192
MAX_PLAN = 4 * 1024 * 1024
LIMITS = (3072, 6144, 64, 6000, 6000, 3000)
WIDTHS = (120, 48, 32, 64, 64, 128)
SOURCE_REQUIRED = set(range(5, 23)) | set(range(26, 32)) | set(range(33, 37)) | set(range(38, 47))
ORIGINAL_REQUIRED = {19, 20, 40, 45, 46}
# Independent interpretation of the version-1 reason/account contract.
ACCOUNT_MASK = {
    **dict.fromkeys((1, 2, 3, 4, 37, 39), 126),
    **dict.fromkeys(range(5, 11), 134),
    **dict.fromkeys((*range(11, 18), 21, 24, 44), 326),
    18: 2050, 19: 2178, 20: 382, 22: 198, 23: 0, 25: 0,
    26: 274, **dict.fromkeys(range(27, 32), 306), 32: 0,
    33: 136, 34: 264, 35: 138, 36: 382, 38: 638,
    40: 1150, 41: 382, 42: 638, 43: 394, 45: 2304, 46: 2050,
}


class EvidenceError(ValueError):
    pass


def need(condition):
    if not condition:
        raise EvidenceError("invalid economic canonical evidence")


def number(value, offset, size):
    need(offset + size <= len(value))
    return int.from_bytes(value[offset:offset + size], "little")


def metadata(value, intent):
    if intent:
        fields = (value[32:48], value[48:64], value[64:80], value[80:96],
                  number(value, 4, 2), number(value, 12, 4), number(value, 16, 4),
                  number(value, 20, 4), value[26], number(value, 96, 8), number(value, 24, 2))
        present, event = value[27], value[112:160]
    else:
        fields = (value[8:24], value[24:40], value[40:56], value[56:72],
                  number(value, 4, 2), number(value, 84, 4), number(value, 88, 4),
                  number(value, 92, 4), value[72], number(value, 76, 8), number(value, 96, 2))
        present, event = value[100], value[104:152]
    lineage, epoch, operation, original, version, writer, policy, compiler, actor, actor_id, reason = fields
    need(all(any(identity) for identity in (lineage, epoch, operation)) and original != operation)
    need(version == policy == compiler == 1 and writer and actor_id and 1 <= reason <= 46)
    need(actor == (2 if 38 <= reason <= 42 else 1) and present in (0, 1))
    need(reason not in ORIGINAL_REQUIRED or any(original))
    need(reason not in SOURCE_REQUIRED or present)
    if present:
        try:
            kind = decode_source_event(event.hex())[0]
        except ValueError as error:
            raise EvidenceError("invalid economic canonical source") from error
        need(source_kind_allowed(reason, kind))
    else:
        need(not any(event))
    return (*fields, event if present else None)


def decode_intent(value):
    need(isinstance(value, bytes) and 256 <= len(value) <= MAX_INTENT)
    need(value[:4] == b"EAI1" and number(value, 6, 2) == 256 and
         number(value, 8, 4) == len(value) and number(value, 104, 4) == len(value) - 256)
    need(number(value, 28, 2) == 1 and not any(value[30:32] + value[108:112] + value[224:256]))
    need(any(value[160:192]) and any(value[192:224]))
    return {"metadata": metadata(value, True), "domain_digest": value[192:224],
            "intent_digest": hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + value).digest()}


def position(value):
    need(len(value) == 56 and not any(value[2:8] + value[50:56]))
    return (value[0], value[1], *struct.unpack_from("<QQQQQH", value, 8))


def valid_position(uid, value):
    owner_type, state, owner, context, root, parent, revision, slot = value
    if state == 0:
        need(not any(value))
        return
    need(1 <= owner_type <= 12)
    if owner_type in (7, 8):
        need(owner == context == 0)
    else:
        need(owner and (owner_type != 10 or not context) and
             (owner_type != 11 or 0 < context <= 2147483647) and
             (owner_type != 12 or (owner < 2**64 - 1 and not context)))
    # Original native-mobile lifetime IDs have no runtime/VNUM context. Their
    # equipment is bounded and belongs to an active, uncontained root UID.
    need(owner_type != 12 or slot <= 43)
    need(not slot or (owner_type in (1, 12) and not parent and state == 1 and
                     (owner_type != 12 or root == uid)))
    if state == 2:
        need(owner_type == 8 and root and parent != uid and revision)
    else:
        need(state in (1, 3) and owner_type != 8 and root and parent != uid and (parent or root == uid))


def forest(items):
    previous = 0
    for uid, value in items.items():
        need(uid > previous)
        previous = uid
        valid_position(uid, value)
    done = set()
    for uid, value in items.items():
        if uid in done or value[1] not in (1, 3):
            continue
        path = set()
        current = uid
        while current not in done:
            need(current not in path)
            path.add(current)
            child = items[current]
            parent = child[5]
            if not parent:
                break
            need(parent in items)
            ancestor = items[parent]
            need(ancestor[1] in (1, 3) and (ancestor[0], ancestor[2:5]) == (child[0], child[2:5]))
            current = parent
        done.update(path)


def coin_effects(meta, effects, postings, child_count):
    lineage, reason = meta[0], meta[10]
    previous = None
    totals = [[0] * 4 for _ in effects]
    referenced = set()
    for key, before, after, before_revision, after_revision in effects:
        try:
            order = account_key(key.hex())
        except ValueError as error:
            raise EvidenceError("invalid economic canonical account") from error
        need(bytes.fromhex(order[0]) == lineage and (previous is None or previous < order))
        previous = order
        kind = order[1]
        need(ACCOUNT_MASK[reason] & (1 << kind))
        if kind in ORDINARY_KINDS:
            need(all(part >= 0 for part in (*before, *after)))
            copper(before)
            copper(after)
            need(after_revision >= before_revision and (before == after or after_revision > before_revision))
        else:
            need(not any((*before, *after, before_revision, after_revision)))
    for index, (event, account, child, delta, amount) in enumerate(postings):
        need(event == index and account < len(effects) and child <= child_count and any(delta))
        need(copper(delta) == amount)
        kind = number(effects[account][0], 18, 2)
        need(kind not in (7, 10) or amount < 0)
        need(kind != 8 or (amount < 0 if reason == 20 else amount > 0))
        need(kind in ORDINARY_KINDS or amount)
        referenced.add(account)
        for part in range(4):
            totals[account][part] += delta[part]
    need(sum(row[4] for row in postings) == 0)
    for index, (key, before, after, before_revision, after_revision) in enumerate(effects):
        ordinary = number(key, 18, 2) in ORDINARY_KINDS
        need(index in referenced or (ordinary and before == after and after_revision > before_revision))
        need(not ordinary or tuple(before[i] + totals[index][i] for i in range(4)) == after)


def gambling(meta, effects, postings, counts):
    reason, event = meta[10], meta[11]
    if reason not in (18, 19, 45, 46):
        return
    opening = reason == 18
    need(event is not None and number(event, 44, 4) == int(not opening) and not any(counts[2:]))
    kinds = [number(row[0], 18, 2) for row in effects]
    wallets, sinks, issuances, stakes = (kinds.count(kind) for kind in (1, 8, 7, 11))
    if reason == 45:
        need(sinks == stakes == 1 and len(kinds) == 2)
    elif reason == 19:
        need(wallets == stakes == 1 and issuances <= 1 and len(kinds) == 2 + issuances)
    else:
        need(wallets == stakes == 1 and len(kinds) == 2)
    held = effects[kinds.index(11)]
    need(number(held[0], 28, 8) == number(event, 36, 8) != 0)
    stake, empty = (held[2], held[1]) if opening else (held[1], held[2])
    nonzero = [i for i, part in enumerate(stake) if part]
    need(not any(empty) and len(nonzero) == 1 and stake[nonzero[0]] > 0)
    denomination = nonzero[0]
    need(all(not part for row in postings for i, part in enumerate(row[3]) if i != denomination))
    if reason == 19 and issuances:
        issuance = [row for row in postings if kinds[row[1]] == 7]
        need(len(issuance) == 1 and issuance[0][3][denomination] == -stake[denomination])


def decode_plan(value):
    need(isinstance(value, bytes) and 256 <= len(value) <= MAX_PLAN)
    need(value[:4] == b"EAP1" and not any(value[6:8] + value[73:76] + value[98:100] +
                                           value[101:104] + value[240:256]))
    meta = metadata(value, False)
    need(any(value[152:184]) and any(value[184:216]))
    counts = struct.unpack_from("<6I", value, 216)
    need(all(count <= limit for count, limit in zip(counts, LIMITS)))
    need(len(value) == 256 + sum(count * width for count, width in zip(counts, WIDTHS)))
    offset = 256
    collections = []
    for count, width in zip(counts, WIDTHS):
        collections.append([value[offset + i * width:offset + (i + 1) * width] for i in range(count)])
        offset += count * width
    effects = [(row[:40], struct.unpack_from("<4q", row, 40), struct.unpack_from("<4q", row, 72),
                *struct.unpack_from("<QQ", row, 104)) for row in collections[0]]
    postings = [(*struct.unpack_from("<IHH", row), struct.unpack_from("<4q", row, 8),
                 struct.unpack_from("<q", row, 40)[0]) for row in collections[1]]
    children = [(row[:16], *struct.unpack_from("<IQHH", row, 16)) for row in collections[2]]
    emitted = set()
    for index, (child, domain, discriminator, parent, relationship) in enumerate(children):
        need(parent <= index and relationship == 1 and domain and any(child) and child != meta[2] and child not in emitted)
        parent_id = children[parent - 1][0] if parent else meta[2]
        need(hashlib.sha256(parent_id + struct.pack("<IQ", domain, discriminator)).digest()[:16] == child)
        available = [row[0] for row in children[index:] if not row[3] or row[3] <= index]
        need(child == min(available))
        emitted.add(child)
    coin_effects(meta, effects, postings, len(children))
    gambling(meta, effects, postings, counts)
    snapshots = []
    for rows in collections[3:5]:
        items = {}
        for row in rows:
            uid = number(row, 0, 8)
            need(uid not in items)
            items[uid] = position(row[8:])
        forest(items)
        snapshots.append(items)
    before, after = snapshots
    need(list(before) == list(after))
    current = dict(before)
    events = []
    for index, row in enumerate(collections[5]):
        event, child, uid = number(row, 0, 4), number(row, 4, 2), number(row, 8, 8)
        need(not any(row[6:8]) and event == index and child <= len(children) and uid in current)
        old, new = position(row[16:72]), position(row[72:128])
        valid_position(uid, new)
        need(current[uid] == old and old[1] != 2 and new[1] != 0 and
             (old[1] != 0 or new[1] in (1, 3)) and new[6] > old[6])
        current[uid] = new
        events.append((event, child, uid, old, new))
    need(current == after)
    return {"metadata": meta, "counts": counts, "intent_digest": value[152:184],
            "domain_digest": value[184:216], "plan_digest": hashlib.sha256(value).digest(),
            "effects": effects, "postings": postings, "children": children, "events": events,
            "before": before, "after": after}


def decode_native_mobile(value):
    """Independent QMNIMG v1/v2 value grammar; no lifetime/custody authority.

    Historical v1 has no cash observation. This returns only the four persisted
    row bindings and never synthesizes holdings, transitions or birth evidence.
    """
    need(type(value) is bytes and 220 <= len(value) <= MAX_PLAN)
    need(value[:8] == b"QMNIMG\0\0" and value[10] in (1, 2) and value[11] == 0 and
         number(value, 12, 4) == len(value) and hashlib.sha256(value[:-32]).digest() == value[-32:])
    version = number(value, 8, 2)
    need(version in (1, 2) and any(value[164:180]))
    reference = value[16:164]
    need(reference[:8] == b"QMNREF\0\0" and number(reference, 8, 2) == 1 and
         reference[10] in (1, 2) and reference[11] == 0 and number(reference, 12, 4) == 148 and
         hashlib.sha256(reference[:-32]).digest() == reference[-32:])
    identity = number(reference, 16, 8)
    need(0 < identity < 2**64-1 and any(reference[24:40]))
    decode_source_event(reference[40:88].hex())
    vnum, birthplace, zone, mobile_revision, stock_revision = struct.unpack_from('<iiiQQ', reference, 88)
    need(vnum >= 0 and mobile_revision > 0 and stock_revision > 0 and
         (zone >= 0 if reference[10] == 1 else zone == -1))
    length_offset = 180
    if version == 2:
        need(len(value) >= 260)
        cash_revision, *cash = struct.unpack_from('<Q4q', value, 180)
        need(cash_revision > 0 and all(0 <= coin <= 2147483647 for coin in cash) and
             (value[10] != 2 or not any(cash)))
        length_offset = 220
    offset, end, rows = length_offset + 4, len(value) - 32, 0
    need(number(value, length_offset, 4) == end - offset)

    def take(width):
        nonlocal offset
        need(offset + width <= end)
        result = value[offset:offset+width]
        offset += width
        return result

    def count():
        nonlocal rows
        size, = struct.unpack('<I', take(4))
        need(size <= 8192 and rows + size <= 8192)
        rows += size
        return size

    def string():
        size, = struct.unpack('<I', take(4))
        need(size <= 4096)
        take(size)  # Literal byte strings, including NUL/non-UTF8, are preserved.

    objects = count()
    need(objects <= 4096 and (value[10] != 2 or objects == 0))
    uids, path, last_slot, inventory_started = set(), [], 0, False
    for index in range(objects):
        parent, slot, uid, generated_key, item_vnum, item_type, mask = struct.unpack('<ihQqibB', take(28))
        need(0 < uid < 2**64-1 and uid not in uids and item_vnum >= 0 and mask == 15)
        uids.add(uid)
        if parent == -1:
            need(0 <= slot <= 43 and (not slot or (not inventory_started and slot > last_slot)))
            if slot:
                last_slot = slot
            else:
                inventory_started = True
            path = [index]
        else:
            need(0 <= parent < index and slot == 0)
            while path and path[-1] != parent:
                path.pop()
            need(path and len(path) < 32)
            path.append(index)
        for _ in range(4):
            string()
        take(169)  # Original scalar values/timers/flags/material/affects.
        take(12 * count())  # Dynamic affects.
        for _ in range(count()):
            string()
            string()
            need(take(1)[0] in (0, 1))
            take(4 * count())  # Spellbook IDs share the decoder's row budget.
    need(offset == end)
    return identity, mobile_revision, stock_revision, value[10]


def require_integrity(executor):
    """Bind every retained root to bounded canonical bytes and SQL projections.

    The restore candidate is quiescent. The caller owns its read transaction;
    this helper issues SELECT only and never repairs retained evidence. ID
    pagination enumerates the candidate, not a live commit watermark.
    """
    def mismatch(code):
        raise RuntimeError("restore_economic_" + code + "_mismatch")

    def arrays(table, columns, where, order):
        output = executor.sql("SELECT JSON_ARRAY(" + ",".join(columns) + ") FROM " + table +
                              " WHERE " + where + " ORDER BY " + order + ";")
        try:
            return [json.loads(row) for row in output.splitlines()]
        except ValueError:
            mismatch("canonical_projection")

    def hexadecimal(column):
        return "LOWER(HEX(" + column + "))"

    def binary(value, size, nullable=False):
        if value is None and nullable:
            return None
        need(isinstance(value, str) and len(value) == 2 * size)
        return bytes.fromhex(value)

    def capsule(operation, field, size, limit, code, table="economic_accounting_operation", minimum=256):
        if type(size) is not int or not minimum <= size <= limit:
            mismatch(code)
        result = bytearray()
        for offset in range(0, size, 65536):
            count = min(65536, size - offset)
            output = executor.sql("SELECT HEX(SUBSTRING(" + field + "," + str(offset + 1) + "," +
                                  str(count) + ")) FROM " + table + " WHERE " + operation + ";")
            try:
                part = bytes.fromhex(output)
            except ValueError:
                mismatch(code)
            if len(part) != count:
                mismatch(code)
            result.extend(part)
        return bytes(result)

    # Allocation metadata has no authority by itself. Bind its exact identities
    # and amounts below to the independently decoded original root effects.
    if executor.sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                    "AND ENGINE='InnoDB' AND table_name IN ('economic_pending_claim_source',"
                    "'economic_pending_claim_consumption','economic_account_mapping');") != "3":
        mismatch("pending_claim_source")

    def allocation_rows(table, columns, identities, code):
        count = executor.sql("SELECT COUNT(*) FROM " + table.split()[0] + ";")
        if not isinstance(count, str) or not count.isdecimal() or not 0 <= int(count) <= MAX_ROWS:
            mismatch(code)
        count = int(count)
        processed, previous = 0, None
        while processed < count:
            where = "1=1"
            if previous is not None:
                literals = ["UNHEX('" + value.hex() + "')" for value in previous[:-1]] + [str(previous[-1])]
                where = "(" + ",".join(identities) + ")>(" + ",".join(literals) + ")"
            page = arrays(table, columns, where, ",".join(identities) + " LIMIT 256")
            if not page or len(page) > min(256, count - processed):
                mismatch(code)
            for row in page:
                try:
                    need(type(row) is list and len(row) == len(columns) and
                         type(row[len(identities)-1]) is int and 0 < row[len(identities)-1] < 2**16)
                    key = tuple(binary(value, 16) for value in row[:len(identities)-1]) + (row[len(identities)-1],)
                    need(all(any(value) for value in key[:-1]) and (previous is None or key > previous))
                except (ValueError, TypeError):
                    mismatch(code)
                previous = key
                processed += 1
                yield key, row

    columns = [hexadecimal("s.source_operation_id"), "s.source_slot", hexadecimal("s.lineage"),
               "s.claim_mapping_id", "s.beneficiary_pid", "s.amount", hexadecimal("s.claim_operation_id"),
               "m.mapping_id", hexadecimal("m.lineage"), "m.backend_kind", "m.account_kind", "m.locator_kind",
               "m.native_id", "m.context_id"]
    table = ("economic_pending_claim_source s LEFT JOIN economic_account_mapping m "
             "ON m.mapping_id=s.claim_mapping_id AND m.lineage=s.lineage")
    sources, source_counts, credits, debits = {}, {}, {}, {}

    def add_amount(values, key, amount):
        values[key] = values.get(key, 0) + amount

    for identity, row in allocation_rows(table, columns, ("s.source_operation_id", "s.source_slot"),
                                          "pending_claim_source"):
        try:
            lineage, mapped_lineage = binary(row[2], 16), binary(row[8], 16)
            need(any(lineage) and lineage == mapped_lineage and
                 all(type(row[index]) is int for index in (3, 4, 5, 7, 9, 10, 11, 12, 13)) and
                 0 < row[3] < 2**64 and 0 < row[4] < 2**32 and 0 < row[5] < 2**32 and
                 row[3] == row[7] and row[4] == row[12] and row[9:12] == [1, 5, 5] and row[13] == 0)
            consumer = binary(row[6], 16, True)
            need(consumer is None or any(consumer))
            key = lineage + struct.pack('<HHQQ4x', 1, 5, row[3], 0)
            account_key(key.hex())
        except (ValueError, TypeError, struct.error):
            mismatch("pending_claim_source")
        sources[identity] = dict(key=key, amount=row[5], whole=consumer, consumed=0)
        add_amount(source_counts, identity[0], 1)
        add_amount(credits, (identity[0], key), row[5])
        if consumer is not None:
            add_amount(debits, (consumer, key), row[5])

    columns = [hexadecimal("spending_operation_id"), hexadecimal("source_operation_id"), "source_slot", "amount"]
    for identity, row in allocation_rows("economic_pending_claim_consumption c", columns,
                ("spending_operation_id", "source_operation_id", "source_slot"), "pending_claim_consumption"):
        source = sources.get(identity[1:])
        if (source is None or source["whole"] is not None or type(row[3]) is not int or
                not 0 < row[3] < 2**64 or source["consumed"] + row[3] > source["amount"]):
            mismatch("pending_claim_consumption")
        source["consumed"] += row[3]
        add_amount(debits, (identity[0], source["key"]), row[3])

    committed = ("o.operation_id IS NULL OR i.operation_id IS NULL OR o.lineage<>s.lineage "
                 "OR o.outcome<>1 OR o.result_code<>0 OR i.status<>1 OR i.result_code<>0 "
                 "OR i.failure_stage<>0 OR i.committed_at IS NULL")
    references = (
        ("economic_pending_claim_source s", "s.source_operation_id", "1=1", "pending_claim_source"),
        ("economic_pending_claim_source s", "s.claim_operation_id", "s.claim_operation_id IS NOT NULL",
         "pending_claim_consumption"),
        ("economic_pending_claim_consumption c LEFT JOIN economic_pending_claim_source s "
         "ON s.source_operation_id=c.source_operation_id AND s.source_slot=c.source_slot",
         "c.spending_operation_id", "1=1", "pending_claim_consumption"),
    )
    for table, operation, scope, code in references:
        if executor.sql("SELECT COUNT(*) FROM " + table + " LEFT JOIN economic_accounting_operation o "
                        "ON o.operation_id=" + operation + " LEFT JOIN critical_operation_inbox i "
                        "ON i.operation_id=o.operation_id WHERE (" + scope + ") AND (" + committed + ");") != "0":
            mismatch(code)

    claim_origin_column_count = executor.sql("SELECT COUNT(*) FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness' "
        "AND column_name='claim_origin_version';")
    if claim_origin_column_count not in ("0", "1"):
        mismatch("baseline_claim_origin")
    claim_origin_column = "w.claim_origin_version" if claim_origin_column_count == "1" else "NULL"

    if executor.sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                    "AND ENGINE='InnoDB' AND table_name='quest_mobile_native';") != "1":
        mismatch("native_mobile")
    mobile_count = executor.sql("SELECT COUNT(*) FROM quest_mobile_native;")
    if not mobile_count.isdecimal():
        mismatch("native_mobile")
    processed, cursor = 0, -1
    while processed < int(mobile_count):
        values = arrays("quest_mobile_native", ["mobile_instance_id", "mobile_revision", "stock_revision",
                        "lifetime_state", "OCTET_LENGTH(canonical_image)"],
                        "1=1" if cursor < 0 else "mobile_instance_id>" + str(cursor),
                        "mobile_instance_id LIMIT 256")
        if not values or len(values) > min(256, int(mobile_count) - processed):
            mismatch("native_mobile")
        for row in values:
            try:
                need(type(row) is list and len(row) == 5 and all(type(field) is int for field in row) and
                     cursor < row[0] < 2**64-1 and row[0] > 0 and
                     0 < row[1] < 2**64 and 0 < row[2] < 2**64 and row[3] in (1, 2))
                image = capsule("mobile_instance_id=" + str(row[0]), "canonical_image", row[4], MAX_PLAN,
                                "native_mobile", table="quest_mobile_native", minimum=220)
                need(tuple(row[:4]) == decode_native_mobile(image))
            except (ValueError, struct.error):
                mismatch("native_mobile")
            cursor = row[0]
            processed += 1

    # Restore must retain the opening witness namespace as well as the generic
    # canonical roots. These checks span all books, including inactive epochs
    # and unknown imported scopes; no selected-book filter may hide lost rows.
    if executor.sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                    "AND ENGINE='InnoDB' AND table_name IN ('economic_baseline_control',"
                    "'economic_baseline_witness','economic_baseline_reservation');") != "3":
        mismatch("baseline_source")
    checks = (
        ("SELECT COUNT(*) FROM economic_baseline_witness w "
         "LEFT JOIN economic_accounting_operation o ON o.operation_id=w.operation_id "
         "LEFT JOIN economic_baseline_control c ON c.lineage=w.lineage AND c.epoch=w.epoch "
         "WHERE o.operation_id IS NULL OR c.lineage IS NULL OR o.lineage<>w.lineage OR o.epoch<>w.epoch "
         "OR o.reason<>38 OR o.outcome<>1 OR o.result_code<>0;", "baseline_witness"),
        ("SELECT COUNT(*) FROM economic_accounting_operation o "
         "LEFT JOIN economic_baseline_witness w ON w.operation_id=o.operation_id "
         "WHERE o.reason=38 AND o.outcome=1 AND o.result_code=0 AND w.operation_id IS NULL;", "baseline_witness"),
        ("SELECT COUNT(*) FROM economic_baseline_reservation p LEFT JOIN economic_baseline_witness w "
         "ON w.operation_id=p.operation_id AND w.lineage=p.lineage AND w.epoch=p.epoch "
         "WHERE w.operation_id IS NULL;", "baseline_reservation"),
        ("SELECT COUNT(*) FROM (SELECT lineage,epoch,identity_kind,identity_id "
         "FROM economic_baseline_reservation GROUP BY lineage,epoch,identity_kind,identity_id "
         "HAVING COUNT(*)<>1) duplicate_identity;", "baseline_reservation"),
        ("SELECT COUNT(*) FROM economic_baseline_control c "
         "LEFT JOIN economic_epoch e ON e.lineage=c.lineage AND e.epoch=c.epoch "
         "LEFT JOIN economic_lineage_state l ON l.lineage=c.lineage "
         "LEFT JOIN critical_operation_inbox i ON i.operation_id=c.creating_operation_id "
         "LEFT JOIN (SELECT lineage,epoch,COUNT(*) n,COUNT(DISTINCT book_revision) revisions,"
         "MIN(book_revision) first_revision,MAX(book_revision) last_revision "
         "FROM economic_baseline_witness GROUP BY lineage,epoch) w ON w.lineage=c.lineage AND w.epoch=c.epoch "
         "LEFT JOIN economic_baseline_witness terminal ON terminal.lineage=c.lineage AND terminal.epoch=c.epoch "
         "AND terminal.book_revision=c.revision WHERE e.lineage IS NULL OR l.lineage IS NULL "
         "OR c.lineage=REPEAT(CHAR(0),16) OR c.epoch=REPEAT(CHAR(0),16) "
         "OR c.creating_operation_id=REPEAT(CHAR(0),16) OR i.operation_id IS NULL "
         "OR i.status NOT IN (0,1) OR OCTET_LENGTH(c.opening_account)<>40 "
         "OR SUBSTRING(c.opening_account,1,16)<>c.lineage OR SUBSTRING(c.opening_account,17,4)<>X'01000900' "
         "OR SUBSTRING(c.opening_account,21,8)=REPEAT(CHAR(0),8) "
         "OR SUBSTRING(c.opening_account,37,4)<>REPEAT(CHAR(0),4) "
         "OR c.revision<>COALESCE(w.n,0) OR COALESCE(w.revisions,0)<>COALESCE(w.n,0) "
         "OR (c.revision>0 AND (w.first_revision<>1 OR w.last_revision<>c.revision)) "
         "OR NOT (c.last_operation_id <=> terminal.operation_id);", "baseline_book"),
    )
    for query, code in checks:
        if executor.sql(query) != "0":
            mismatch(code)
    # A baseline owns no mutation children, native ledger effects or outbox.
    effects = [("economic_accounting_child", "operation_id"),
               ("economic_accounting_child", "child_operation_id"),
               ("economic_accounting_item_reference", "operation_id"),
               ("currency_ledger", "operation_id"), ("item_ownership_ledger", "operation_id"),
               ("critical_outbox", "operation_id")]
    zero_effects = " OR ".join("EXISTS(SELECT 1 FROM " + table + " e JOIN economic_baseline_witness w "
                              "ON w.operation_id=e." + column + ")" for table, column in effects)
    if executor.sql("SELECT " + zero_effects + ";") != "0":
        mismatch("baseline_zero_effect")

    admission_column_count = executor.sql("SELECT COUNT(*) FROM information_schema.columns "
        "WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness' "
        "AND column_name='command_accepted_at_usec';")
    if admission_column_count not in ("0", "1"):
        mismatch("baseline_witness")
    admission_column = "w.command_accepted_at_usec" if admission_column_count == "1" else "NULL"

    def baseline_witness(where, meta, original, row, frozen, encoded):
        # The second consumer reuses pure independent EAB1/EAB2 interpretation. The
        # import is local because the origin reader also consumes this decoder.
        from economic_sql_audit_origins import MAX_WITNESS_BYTES, decode_witness, verify_baseline_root
        columns = [hexadecimal("w.lineage"), hexadecimal("w.epoch"), "w.book_revision", "w.witness_version",
                   "w.holding_count", "w.item_count", hexadecimal("w.witness_digest"),
                   "OCTET_LENGTH(w.canonical_witness)", hexadecimal("c.opening_account"),
                   "i.durable_revision", "i.command_type", "i.schema_version", "i.payload_version",
                   hexadecimal("i.result_payload"), hexadecimal("i.keys_hash"),
                   admission_column, hexadecimal("i.command_hash"), claim_origin_column]
        table = ("economic_baseline_witness w JOIN economic_baseline_control c "
                 "ON c.lineage=w.lineage AND c.epoch=w.epoch "
                 "JOIN critical_operation_inbox i ON i.operation_id=w.operation_id")
        values = arrays(table, columns, "w." + where, "w.operation_id")
        if len(values) != 1 or len(values[0]) != len(columns):
            mismatch("baseline_witness")
        value = values[0]
        try:
            lineage, epoch = binary(value[0], 16), binary(value[1], 16)
            need((lineage, epoch) == meta[:2])
            names = ("root_lineage", "root_epoch", "operation_id", "original_operation_id", "accounting_version",
                     "writer_id", "policy_version", "compiler_version", "actor_kind", "actor_id", "reason", "source_event")
            witness = dict(zip(names, (*meta[:3], original, *meta[4:])))
            witness.update(book_revision=value[2], witness_version=value[3], holding_count=value[4], item_count=value[5],
                witness_digest=binary(value[6], 32), canonical_witness=capsule(where, "canonical_witness", value[7],
                    MAX_WITNESS_BYTES, "baseline_witness", table="economic_baseline_witness", minimum=192),
                inbox_revision=value[9], inbox_type=value[10], inbox_schema=value[11], inbox_payload=value[12],
                inbox_result_payload=binary(value[13], 0), canonical_intent=frozen, canonical_plan=encoded,
                inbox_keys_hash=binary(value[14], 32), command_accepted_at_usec=value[15],
                inbox_command_hash=binary(value[16], 32),
                intent_digest=binary(row[12], 32), domain_digest=binary(row[13], 32), plan_digest=binary(row[14], 32))
            witness.update(zip(("account_count", "posting_count", "child_count", "before_witness_count",
                                "after_witness_count", "item_event_count"), row[19:25]))
            holdings, items = decode_witness(witness, lineage, epoch, binary(value[8], 40))
            verify_baseline_root(witness, lineage, epoch)
        except (ValueError, struct.error, TypeError):
            mismatch("baseline_witness")
        expected = [[lineage.hex(), epoch.hex(), 1, account_key(holding["account_key"])[2], meta[2].hex()]
                    for holding in holdings]
        expected += [[lineage.hex(), epoch.hex(), 2, item["uid"], meta[2].hex()] for item in items]
        expected.sort(key=lambda value: (value[2], value[3]))
        columns = [hexadecimal("lineage"), hexadecimal("epoch"), "identity_kind", "identity_id",
                   hexadecimal("operation_id")]
        if not same_projection(arrays("economic_baseline_reservation", columns, where,
                  "identity_kind,identity_id,lineage,epoch LIMIT " + str(len(expected) + 1)), expected):
            mismatch("baseline_reservation")
        if value[17] is not None:
            if type(value[17]) is not int or value[17] != 1:
                mismatch("baseline_claim_origin")
            expected_origins = {}
            for index, holding in enumerate(holdings):
                if account_key(holding["account_key"])[1] == 5:
                    if any(holding["balance"][1:]):
                        mismatch("baseline_claim_origin")
                    if holding["balance"][0]:
                        expected_origins[(meta[2], index+1)] = (bytes.fromhex(holding["account_key"]),
                                                               holding["balance"][0])
            if source_counts.get(meta[2], 0) != len(expected_origins):
                mismatch("baseline_claim_origin")
            for identity, expected_origin in expected_origins.items():
                source = sources.get(identity)
                if source is None or (source["key"], source["amount"]) != expected_origin:
                    mismatch("baseline_claim_origin")
        return value[17]

    # Ordinary histories may have no baseline book. Their canonical capsules
    # still require the retained lifecycle namespace, including inactive epochs.
    if executor.sql("SELECT COUNT(*) FROM economic_accounting_operation o "
                    "LEFT JOIN economic_lineage_state l ON l.lineage=o.lineage "
                    "WHERE l.lineage IS NULL;") != "0":
        mismatch("lineage")
    if executor.sql("SELECT COUNT(*) FROM economic_accounting_operation o "
                    "LEFT JOIN economic_epoch e ON e.lineage=o.lineage AND e.epoch=o.epoch "
                    "WHERE e.epoch IS NULL;") != "0":
        mismatch("epoch")
    root_count = int(executor.sql("SELECT COUNT(*) FROM economic_accounting_operation;"))
    processed, cursor = 0, ""
    while processed < root_count:
        output = executor.sql("SELECT LOWER(HEX(operation_id)) FROM economic_accounting_operation "
                              "WHERE operation_id>UNHEX('" + cursor + "') ORDER BY operation_id LIMIT 256;")
        identities = output.splitlines()
        if not identities:
            mismatch("canonical_root_count")
        for operation in identities:
            if (len(operation) != 32 or any(c not in "0123456789abcdef" for c in operation) or
                    operation <= cursor):
                mismatch("metadata")
            cursor = operation
            where = "operation_id=UNHEX('" + operation + "')"
            fields = [hexadecimal(name) for name in ("lineage", "epoch", "operation_id", "original_operation_id")]
            fields += ["accounting_version", "writer_id", "policy_version", "compiler_version",
                       "actor_kind", "actor_id", "reason", hexadecimal("source_event")]
            fields += [hexadecimal(name) for name in ("intent_digest", "domain_digest", "plan_digest")]
            fields += ["OCTET_LENGTH(canonical_intent)", "OCTET_LENGTH(canonical_plan)", "outcome", "result_code",
                       "account_count", "posting_count", "child_count", "before_witness_count",
                       "after_witness_count", "item_event_count"]
            rows = arrays("economic_accounting_operation", fields, where, "operation_id")
            if len(rows) != 1 or len(rows[0]) != 25:
                mismatch("metadata")
            row = rows[0]
            try:
                original = binary(row[3], 16, True)
                need(original is None or any(original))
                meta = (*[binary(value, 16) for value in row[:3]], original or bytes(16),
                        *row[4:11], binary(row[11], 48, True))
                need(meta[2].hex() == operation)
                frozen = capsule(where, "canonical_intent", row[15], MAX_INTENT, "intent")
                intent = decode_intent(frozen)
                if (intent["intent_digest"] != binary(row[12], 32) or
                        intent["domain_digest"] != binary(row[13], 32)):
                    mismatch("intent")
            except (ValueError, struct.error):
                mismatch("intent")
            if not same_projection(meta, intent["metadata"]):
                mismatch("metadata")
            counts = tuple(row[19:25])
            if type(row[17]) is not int or type(row[18]) is not int or not 0 <= row[18] < 2**32:
                mismatch("plan")
            if row[17] == 2:
                if (row[18] == 0 or row[14] is not None or row[16] is not None or
                        not same_projection(counts, (0,) * 6)):
                    mismatch("plan")
            elif row[17] == 1 and row[18] == 0:
                try:
                    encoded = capsule(where, "canonical_plan", row[16], MAX_PLAN, "plan")
                    plan = decode_plan(encoded)
                    if (not same_projection(plan["metadata"], meta) or plan["intent_digest"] != intent["intent_digest"] or
                            plan["domain_digest"] != intent["domain_digest"] or
                            plan["plan_digest"] != binary(row[14], 32)):
                        mismatch("plan")
                except (ValueError, struct.error):
                    mismatch("plan")
                if not same_projection(counts, plan["counts"]):
                    mismatch("canonical_count")
                coins = ("copper", "silver", "gold", "platinum")
                fields = ["account_index", hexadecimal("account_key")]
                fields += [side + "_" + coin for side in ("before", "after") for coin in coins]
                fields += ["before_revision", "after_revision"]
                expected = [[i, key.hex(), *before, *after, before_revision, after_revision]
                            for i, (key, before, after, before_revision, after_revision) in enumerate(plan["effects"])]
                if not same_projection(arrays("economic_accounting_account_effect", fields, where, "account_index"), expected):
                    mismatch("canonical_account")
                fields = ["line_index", "event_index", "account_index", "child_index"]
                fields += ["delta_" + coin for coin in coins] + ["copper_value"]
                expected = [[i, event, account, child, *delta, amount]
                            for i, (event, account, child, delta, amount) in enumerate(plan["postings"])]
                if not same_projection(arrays("economic_accounting_coin_posting", fields, where, "line_index"), expected):
                    mismatch("canonical_posting")
                fields = ["child_index", hexadecimal("child_operation_id"), "domain_id", "discriminator",
                          "parent_index", "relationship"]
                expected = [[i + 1, child.hex(), domain, discriminator, parent, relationship]
                            for i, (child, domain, discriminator, parent, relationship) in enumerate(plan["children"])]
                if not same_projection(arrays("economic_accounting_child", fields, where, "child_index"), expected):
                    mismatch("canonical_child")
                fields = ["line_index", "event_index", "child_index", "item_uid", "before_revision", "after_revision"]
                expected = [[i, event, child, uid, old[6], new[6]]
                            for i, (event, child, uid, old, new) in enumerate(plan["events"])]
                if not same_projection(arrays("economic_accounting_item_reference", fields, where, "line_index"), expected):
                    mismatch("canonical_item")
                fields = ["r.line_index", "l.item_uid", "l.root_item_uid", "COALESCE(l.parent_item_uid,0)",
                          "l.from_owner_type", "l.from_owner_id", "l.from_owner_context_id", "l.to_owner_type",
                          "l.to_owner_id", "l.to_owner_context_id", "l.item_revision",
                          "l.from_equipment_slot", "l.to_equipment_slot"]
                table = ("economic_accounting_item_reference r LEFT JOIN item_ownership_ledger l "
                         "ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index")
                expected = [[i, uid, new[4], new[5], *( (7, 0, 0) if old[1] == 0 else (old[0], old[2], old[3]) ),
                             new[0], new[2], new[3], new[6], old[7], new[7]]
                            for i, (event, child, uid, old, new) in enumerate(plan["events"])]
                if not same_projection(arrays(table, fields, "r." + where, "r.line_index"), expected):
                    mismatch("canonical_custody")
                origin_version = None
                if meta[10] == 38:
                    origin_version = baseline_witness(where, meta, original, row, frozen, encoded)
                for key, before, after, before_revision, after_revision in plan["effects"]:
                    if account_key(key.hex())[1] != 5:
                        continue
                    pair = (meta[2], key)
                    amount = after[0] - before[0]
                    credit, debit = credits.pop(pair, 0), debits.pop(pair, 0)
                    # Historical NULL witnesses retain unknown allocation
                    # coverage; new version1 openings require exact origins.
                    if amount > 0 and not credit and not debit and meta[10] == 38 and origin_version is None:
                        continue
                    if credit != max(amount, 0):
                        mismatch("pending_claim_source")
                    if debit != max(-amount, 0):
                        mismatch("pending_claim_consumption")
                    if (credit or debit) and before[1:] != after[1:]:
                        mismatch("pending_claim_source" if credit else "pending_claim_consumption")
            else:
                mismatch("plan")
            processed += 1
    if processed != root_count:
        mismatch("canonical_root_count")
    if credits:
        mismatch("pending_claim_source")
    if debits:
        mismatch("pending_claim_consumption")
