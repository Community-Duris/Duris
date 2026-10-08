#!/usr/bin/env python3
"""Native and independent durable custody decoding, without authority repairs."""
import ast
import argparse
import copy
import functools
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as qualifier
import flatfile_auction_money_cases as auctions

UNITS = ("flatfile_item_repository", "flatfile_authority_transaction", "flatfile_store",
         "player_snapshot_codec", "item_transfer_command", "critical_command", "economic_source_event",
         "flatfile_world_item_repository", "flatfile_locker_repository", "flatfile_shopkeeper_repository",
         "flatfile_player_snapshot_file")


def build_fixture(destination, native_source=ROOT):
    units = []
    for name in UNITS:
        matches = list((native_source / "src").rglob(name + ".c"))
        assert len(matches) == 1, name
        units.append(str(matches[0]))
    subprocess.run(["g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                    "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g", "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-D__NO_MYSQL__",
                    "-I" + str(native_source / "src"), "-I" + str(native_source / "src/no_mysql"),
                    str(ROOT / "tests/async/flatfile_custody_audit_fixture.cpp"), *units,
                    "-lcrypto", "-pthread", "-o", str(destination)], cwd=ROOT, check=True)
    return destination


@functools.cache
def payload_encoder():
    path = ROOT / "tests/async/run_economic_sql_audit_snapshot_mysql.py"
    function, = [node for node in ast.parse(path.read_text()).body
                 if isinstance(node, ast.FunctionDef) and node.name == "coin_payload"]
    namespace = {"struct": struct}
    exec(compile(ast.Module(body=[function], type_ignores=[]), str(path), "exec"), namespace)
    return namespace["coin_payload"]


def item_payload(uid, vnum=402013, **changes):
    return payload_encoder()(uid, [1, 2, 3, 4], vnum, **changes)


def quest(version, *, pid=7, recipients=(7,), rewards=None, awards=None):
    rewards = rewards if rewards is not None else [(5, 100, 0, 75)]
    result = struct.pack("<6IQI", version, pid, 1, 0, 402010, 3001, 1, 0 if version == 6 else 1)
    if version != 6:
        result += struct.pack("<Q", 82)
    result += struct.pack("<I", len(rewards))
    for kind, number, flags, frozen in rewards:
        result += struct.pack("<2I", kind, number)
        if version >= 3:
            result += struct.pack("<I", flags)
        if version >= 4:
            result += struct.pack("<I", frozen)
    if version >= 2:
        result += struct.pack("<IiiIiI", 30, 50, -1, len(recipients), 51, len(recipients))
        result += struct.pack("<" + "I" * len(recipients), *recipients)
        result += struct.pack("<I", 4) + b"test" + struct.pack("<I", 7) + b"quest:1"
    if version >= 5:
        awards = awards if awards is not None else [(recipient, 0, 75) for recipient in recipients]
        result += struct.pack("<I", len(awards))
        result += b"".join(struct.pack("<3I", *award) for award in awards)
    if version == 6:
        action = bytes.fromhex("a1" * 16)
        source = struct.pack("<HH16s16sQI", 19, 1, action, bytes.fromhex("b2" * 16), 1, 0)
        acceptance = struct.pack("<QHB5x4Q", 82, 1, 0, 1, 1, 1, 0)
        result += b"QRF6" + action + source + struct.pack("<Q", 1) + bytes.fromhex("c3" * 16) + acceptance
    return result


def operation(index=1, **changes):
    value = dict(id=index.to_bytes(16, "little"), digest=bytes(32), code=0, root=82, count=1,
                 from_revision=0, to_revision=1, item_revision=1, corpse_revision=0,
                 collector=0, coin=0, coin_result=bytes(256), quest=b"", ack=0, mask=0,
                 xp_revisions=(), creation_source=0, creation_recipient=0, creation_vnum=0, legacy=0)
    value.update(changes)
    return value


def model(version=8):
    owners = [(kind, 0 if kind in (7, 8) else 7, 7 if kind == 11 else 0, 0)
              for kind in range(1, 13)]
    items = [dict(uid=82, root=82, parent=0, owner=(3, 7, 0), revision=1,
                  vnum=402013, state=1, payload=item_payload(82), equipment=0)]
    return dict(version=version, revision=1, owners=owners, items=items, operations=[operation()])


def frame(value):
    version = value["version"]
    result = struct.pack("<3I", len(value["owners"]), len(value["items"]), len(value["operations"]))
    for owner in value["owners"]:
        result += struct.pack("<B3Q", *owner)
    for item in value["items"]:
        result += struct.pack("<3QB3QiB", item["uid"], item["root"], item["parent"],
                              *item["owner"], item["revision"], item["vnum"], item["state"])
        if version >= 3:
            result += struct.pack("<I", len(item["payload"])) + item["payload"]
        if version >= 5:
            result += struct.pack("<H", item["equipment"])
    for op in value["operations"]:
        result += op["id"] + op["digest"] + struct.pack("<IQH3Q", op["code"], op["root"], op["count"],
                                                       op["from_revision"], op["to_revision"], op["item_revision"])
        if version >= 2:
            result += struct.pack("<Q", op["corpse_revision"])
        if version >= 4:
            result += struct.pack("<B", op["collector"])
        if version >= 3:
            result += struct.pack("<B", op["coin"])
            if op["coin"]:
                result += op["coin_result"]
        if version >= 6:
            result += struct.pack("<I", len(op["quest"])) + op["quest"] + struct.pack("<B", op["ack"])
        if version >= 7:
            result += struct.pack("<Q", op["mask"])
            result += b"".join(struct.pack("<Q", revision) for revision in op["xp_revisions"])
        if version >= 8:
            result += struct.pack("<QI iB", op["creation_source"], op["creation_recipient"],
                                  op["creation_vnum"], op["legacy"])
    return b"DUROWN\0\0" + struct.pack("<IIQ", version, len(result), value["revision"]) + hashlib.sha256(result).digest() + result


def cases():
    result = []

    def add(name, value, valid):
        result.append((name, frame(value), valid))

    for version in range(1, 9):
        add("version-" + str(version), model(version), True)
    for kind in range(1, 13):
        value = model()
        value["items"][0]["owner"] = value["owners"][kind - 1][:3]
        value["items"][0]["revision"] = 0  # Native legacy custody permits zero clocks.
        add("owner-" + str(kind), value, True)
    for state in (1, 2, 3):
        value = model()
        value["items"][0]["state"] = state
        add("state-" + str(state), value, True)
    value = model()
    value["revision"] = 2**64 - 1
    value["items"][0].update(uid=2**64 - 1, root=2**64 - 1, revision=2**64 - 1,
                              payload=item_payload(2**64 - 1))
    add("full-width-clocks-and-uid", value, True)
    for owner_kind in (1, 12):
        value = model()
        value["items"][0].update(owner=value["owners"][owner_kind - 1][:3], equipment=43)
        add("equipment-" + str(owner_kind), value, True)
    for version in range(6, 9):
        for quest_version in range(1, 7):
            value = model(version)
            value["operations"] = [operation(quest=quest(quest_version), ack=1, legacy=1)]
            add(f"quest-{version}-{quest_version}", value, True)
    value = model()
    value["operations"] = [operation(quest=quest(5, recipients=(7, 8)), mask=3,
                                      xp_revisions=(1, 2), legacy=1)]
    add("quest-party-xp", value, True)
    value = model()
    value["operations"] = [operation(quest=quest(5, recipients=tuple(range(7, 71))),
                                      mask=2**64 - 1, xp_revisions=(1,) * 64, legacy=1)]
    add("quest-full-width-party-mask", value, True)
    value = model()
    value["operations"] = [operation(quest=quest(4, rewards=[(5, 100, 0, 75)] * 64),
                                      mask=2**64 - 1, xp_revisions=(1,) * 64, legacy=1)]
    add("quest-full-width-solo-mask", value, True)
    value = model()
    value["operations"] = [operation(quest=quest(6, recipients=tuple(range(7, 71))),
                                      mask=2**64 - 1, xp_revisions=(1,) * 64, legacy=1)]
    add("quest-fee-full-width-party-mask", value, True)
    fee = quest(6)
    tail = len(fee) - 140
    for label, offset, replacement in (
        ("fee-tag", tail, b"X"), ("fee-action-zero", tail + 4, bytes(16)),
        ("fee-kind", tail + 20, struct.pack("<H", 18)),
        ("fee-source-version", tail + 22, struct.pack("<H", 2)),
        ("fee-source-action", tail + 24, bytes(16)),
        ("fee-generation", tail + 40, bytes(16)),
        ("fee-sequence", tail + 56, bytes(8)),
        ("fee-slot", tail + 64, struct.pack("<I", 1)),
        ("fee-lifetime-zero", tail + 68, bytes(8)),
        ("fee-lifetime-reserved", tail + 68, struct.pack("<Q", 2**64 - 1)),
        ("fee-acceptance-zero", tail + 76, bytes(16)),
        ("fee-same-action-acceptance", tail + 76, bytes.fromhex("a1" * 16)),
        ("fee-result-root", tail + 92, bytes(8)),
        ("fee-result-count", tail + 100, bytes(2)),
        ("fee-result-large-count", tail + 100, struct.pack("<H", 3001)),
        ("fee-result-collector", tail + 102, b"\1"),
        ("fee-result-padding", tail + 103, b"\1"),
        ("fee-result-corpse", tail + 132, struct.pack("<Q", 1)),
    ):
        value = model()
        body = fee[:offset] + replacement + fee[offset + len(replacement):]
        value["operations"] = [operation(quest=body, legacy=1)]
        add(label, value, False)
    value = model()
    value["operations"] = [operation(coin=1, root=0, count=0), operation(2, creation_source=1,
                                          creation_recipient=7, creation_vnum=402013)]
    add("coin-and-creation-receipts", value, True)
    value = model()
    value["items"][0]["payload"] = b""
    add("legacy-literal-unresolved", value, True)
    for label, payload in (("coin-uid", item_payload(83)), ("coin-vnum", item_payload(82, 3)),
                           ("coin-truncated", item_payload(82)[:-1]),
                           ("coin-trailing", item_payload(82) + b"\0"),
                           ("coin-nonmoney", item_payload(82)[:30] + b"\x13" + item_payload(82)[31:]),
                           ("coin-over-blob-limit", bytes(128 * 1024 + 1)),
                           ("coin-row-budget", item_payload(82)[:-8] + struct.pack("<I", 8192) +
                            bytes(12 * 8192) + bytes(4))):
        value = model()
        value["items"][0]["payload"] = payload
        add(label, value, False)
    for field, values in (("uid", (0,)), ("root", (0,)), ("vnum", (0, -1)),
                          ("state", (0, 4)), ("equipment", (44, 1))):
        for changed in values:
            value = model()
            value["items"][0][field] = changed
            add(f"invalid-item-{field}-{changed}", value, False)
    for kind, identity, context in ((0, 1, 0), (13, 1, 0), (1, 0, 0), (7, 1, 0), (8, 0, 1),
                                    (10, 1, 1), (11, 1, 0), (11, 1, 2**31), (12, 2**64 - 1, 0), (12, 1, 1)):
        value = model()
        value["owners"] = [(kind, identity, context, 1)]
        value["items"][0]["owner"] = (kind, identity, context)
        add(f"invalid-owner-{kind}-{identity}-{context}", value, False)
    for label, mutate in (
        ("owner-order", lambda value: value["owners"].reverse()),
        ("owner-duplicate", lambda value: value["owners"].insert(0, value["owners"][0])),
        ("item-duplicate", lambda value: value["items"].append(copy.deepcopy(value["items"][0]))),
        ("owner-absent", lambda value: value["items"][0].update(owner=(3, 999, 0))),
        ("operation-zero", lambda value: value["operations"][0].update(id=bytes(16))),
        ("operation-duplicate", lambda value: value["operations"].append(copy.deepcopy(value["operations"][0]))),
        ("result-zero-root", lambda value: value["operations"][0].update(root=0)),
        ("result-zero-count", lambda value: value["operations"][0].update(count=0)),
        ("result-large-count", lambda value: value["operations"][0].update(count=3001)),
        ("collector-boolean", lambda value: value["operations"][0].update(collector=2)),
        ("coin-boolean", lambda value: value["operations"][0].update(coin=2)),
        ("ack-boolean", lambda value: value["operations"][0].update(ack=2)),
        ("legacy-boolean", lambda value: value["operations"][0].update(legacy=2)),
        ("empty-ack", lambda value: value["operations"][0].update(ack=1)),
        ("empty-legacy", lambda value: value["operations"][0].update(legacy=1)),
        ("empty-xp", lambda value: value["operations"][0].update(mask=1, xp_revisions=(1,))),
        ("creation-missing-pid", lambda value: value["operations"][0].update(creation_source=1)),
        ("creation-missing-source", lambda value: value["operations"][0].update(creation_recipient=7)),
        ("creation-stray-vnum", lambda value: value["operations"][0].update(creation_vnum=3)),
    ):
        value = model()
        mutate(value)
        add(label, value, False)
    for label, body in (("quest-duplicate-recipient", quest(5, recipients=(7, 7))),
                        ("quest-missing-player", quest(5, recipients=(8,))),
                        ("quest-wrong-first-player", quest(5, recipients=(8, 7))),
                        ("quest-missing-award", quest(5, recipients=(7, 8), awards=[(7, 0, 75)])),
                        ("quest-duplicate-award", quest(5, awards=[(7, 0, 75), (7, 0, 75)])),
                        ("quest-wrong-frozen-award", quest(5, awards=[(7, 0, 76)])),
                        ("quest-invalid-reward", quest(5, rewards=[(2, 100, 0, 75)])),
                        ("quest-stray-flags", quest(5, rewards=[(5, 100, 1, 75)])),
                        ("quest-zero-frozen", quest(5, rewards=[(5, 100, 0, 0)])),
                        ("quest-trailing", quest(5) + b"\0")):
        value = model()
        value["operations"] = [operation(quest=body, legacy=1)]
        add(label, value, False)
    for label, changed in (("quest-xp-mask", dict(mask=2, xp_revisions=(1,))),
                            ("quest-xp-zero-revision", dict(mask=1, xp_revisions=(0,))),
                            ("quest-failed-root", dict(code=1)), ("quest-coin-root", dict(coin=1))):
        value = model()
        value["operations"] = [operation(quest=quest(5), legacy=1, **changed)]
        add(label, value, False)
    original = frame(model())
    for label, body in (("truncated", original[:-1]), ("trailing", original + b"\0"),
                        ("magic", b"X" + original[1:]), ("checksum", original[:24] + bytes(32) + original[56:]),
                        ("version-zero", original[:8] + bytes(4) + original[12:]),
                        ("future-version", original[:8] + struct.pack("<I", 9) + original[12:]),
                        ("zero-revision", original[:16] + bytes(8) + original[24:])):
        result.append((label, body, False))
    return result


def inventory(root):
    result = {}
    for path in root.rglob("*"):
        info = path.lstat()
        result[path.relative_to(root).as_posix()] = (stat.S_IFMT(info.st_mode), stat.S_IMODE(info.st_mode),
            info.st_nlink, os.readlink(path) if path.is_symlink() else
            hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None)
    return result


def check_catalogs(fixture, binary, build):
    observations = []
    for label, encoded, expected in cases():
        directory = build / label
        directory.mkdir(mode=0o700)
        incoming = directory / "input.bin"
        incoming.write_bytes(encoded)
        root = directory / "state"
        native = subprocess.run([str(fixture), str(root), str(incoming), "1" if expected else "0"],
                                capture_output=True, text=True, timeout=30)
        assert native.returncode == 0 and not native.stderr, (label, native.stdout, native.stderr)
        value = json.loads(native.stdout)
        assert value["native_accepted"] == value["independent_accepted"] == expected
        assert incoming.read_bytes() == encoded and (root / "domains/item_ownership").read_bytes() == encoded
        before = inventory(root)
        operator = subprocess.run([str(binary), "--economic-custody-catalog-audit", str(root)],
                                  capture_output=True, text=True, timeout=30)
        assert inventory(root) == before
        if expected:
            assert operator.returncode == 0 and not operator.stderr, (label, operator.stdout, operator.stderr)
            report = json.loads(operator.stdout)
            assert report["catalog_present"] and report["custody_catalog_decoded"]
            assert not any(report[name] for name in ("native_holdings_compared", "owner_literals_compared",
                "item_history_verified", "full_R7_qualified", "release_qualified"))
            assert report["items"] == value["items"]
        else:
            assert operator.returncode == 1 and not operator.stdout and operator.stderr == "native_restore_qualification_failed\n", (label, operator)
        row = dict(case=label, accepted=expected, native=value, operator_exit=operator.returncode,
                   input_sha256=hashlib.sha256(encoded).hexdigest(), authority_unchanged=True)
        observations.append(row)
        print("CUSTODY_CATALOG " + json.dumps(row, sort_keys=True), flush=True)
        (directory / "operator.json").write_text(json.dumps(dict(command=operator.args, exit=operator.returncode,
            stdout=operator.stdout, stderr=operator.stderr), indent=2) + "\n")
        for name in ("before", "after"):
            (directory / (name + ".json")).write_text(json.dumps(before, sort_keys=True) + "\n")
    (build / "observations.json").write_text(json.dumps(observations, indent=2) + "\n")
    return observations


def world_item(uid, *, parent=-1, kind=20, vnum=402013, strings=(b"",) * 4,
               dynamic_count=0, spell_counts=()):
    raw = bytearray(item_payload(uid, vnum, dynamic_count=dynamic_count, spell_counts=spell_counts)[4:])
    struct.pack_into("<i", raw, 0, parent)
    raw[26] = kind
    return bytes(raw[:28]) + b"".join(struct.pack("<I", len(value)) + value for value in strings) + bytes(raw[44:])


def world_model(version=3):
    return dict(version=version, revision=1,
        corpses=[dict(pid=7, name=b"erased-alias", save=5, room=0, short=b"", description=b"", keywords=b"",
                      weight=0, values=[0] * 8, money=[1, 2, 3, 4], revision=2,
                      items=[world_item(101, kind=1, vnum=57), world_item(102, parent=0)])],
        saved=[dict(key=b"saved-alpha", room=3002, revision=3,
                    items=[world_item(103, kind=1, vnum=58), world_item(104, parent=0)])],
        rooms=[dict(room=3003, revision=4, money=[0] * 4, items=[world_item(105)]),
               dict(room=3004, revision=5, money=[4, 3, 2, 1], items=[world_item(106, kind=1, vnum=59)])])


def world_records(value):
    return [*((row, (4, (row["pid"] << 32) | row["save"], 0)) for row in value["corpses"]),
            *((row, (3, row["room"], 0)) for row in value["saved"]),
            *((row, (3, row["room"], 0)) for row in value["rooms"] if value["version"] >= 3)]


def world_frame(value):
    def text(value):
        return struct.pack("<I", len(value)) + value

    def items(values):
        body = struct.pack("<I", len(values)) + b"".join(values)
        return struct.pack("<I", len(body)) + body

    version = value["version"]
    body = struct.pack("<2I", len(value["corpses"]), len(value["saved"]))
    if version >= 3:
        body += struct.pack("<I", len(value["rooms"]))
    for row in value["corpses"]:
        body += struct.pack("<I", row["pid"]) + text(row["name"]) + struct.pack("<Ii", row["save"], row["room"])
        body += text(row["short"]) + text(row["description"]) + text(row["keywords"])
        body += struct.pack("<i8i", row["weight"], *row["values"])
        if version >= 2:
            body += struct.pack("<4i", *row["money"])
        body += struct.pack("<Q", row["revision"]) + items(row["items"])
    for row in value["saved"]:
        body += text(row["key"]) + struct.pack("<iQ", row["room"], row["revision"]) + items(row["items"])
    if version >= 3:
        for row in value["rooms"]:
            body += struct.pack("<iQ4i", row["room"], row["revision"], *row["money"]) + items(row["items"])
    return b"DURWRLD\0" + struct.pack("<IIQ", version, len(body), value["revision"]) + hashlib.sha256(body).digest() + body


def world_custody(value):
    owners, items = set(), []
    for record, owner in world_records(value):
        owners.add((*owner, 0))  # Native custody clocks need not equal world aggregate clocks.
        rows = record["items"]
        for i, raw in enumerate(rows):
            uid, = struct.unpack_from("<Q", raw, 6)
            parent, = struct.unpack_from("<i", raw)
            root = i
            while struct.unpack_from("<i", rows[root])[0] >= 0:
                root = struct.unpack_from("<i", rows[root])[0]
            detached = struct.pack("<Ii", 1, -1) + raw[4:]
            items.append(dict(uid=uid, root=struct.unpack_from("<Q", rows[root], 6)[0],
                parent=0 if parent < 0 else struct.unpack_from("<Q", rows[parent], 6)[0], owner=owner,
                revision=1, vnum=struct.unpack_from("<i", raw, 22)[0], state=1, equipment=0,
                payload=detached if raw[26] == 20 and len(detached) <= 128 * 1024 else b""))
    return dict(version=8, revision=1, owners=sorted(owners), items=sorted(items, key=lambda row: row["uid"]), operations=[])


def world_cases():
    result = []

    def add(label, value, accepted):
        result.append((label, world_frame(value), accepted, value if accepted else None))

    for version in (1, 2, 3):
        add("version-" + str(version), world_model(version), True)
    for group in ("corpses", "saved", "rooms"):
        value = world_model()
        value[group][0]["items"] = []
        add("empty-" + group, value, group != "saved")
    value = world_model()
    value.update(corpses=[], saved=[], rooms=[])
    add("empty-catalog", value, True)
    for field, replacement in (("pid", 0), ("save", 0), ("room", -1), ("revision", 0),
                                ("name", b"Upper"), ("name", b""), ("name", b"x\x7f"),
                                ("short", b"\x1f"), ("description", b"x" * 65537), ("keywords", b"x" * 513),
                                ("money", [-1, 0, 0, 0])):
        value = world_model();value["corpses"][0][field] = replacement
        add("corpse-" + field + "-" + str(len(result)), value, False)
    for field, replacement in (("room", 0), ("revision", 0), ("key", b""), ("key", b"x" * 101)):
        value = world_model();value["saved"][0][field] = replacement
        add("saved-" + field + "-" + str(len(result)), value, False)
    for field, replacement in (("room", 0), ("revision", 0), ("money", [0, -1, 0, 0])):
        value = world_model();value["rooms"][0][field] = replacement
        add("room-" + field, value, False)
    for label in ("corpse-order", "owner-alias-reused", "owner-alias-differs", "saved-duplicate", "room-duplicate", "room-order"):
        value = world_model()
        if label.startswith("corpse") or label.startswith("owner"):
            row = copy.deepcopy(value["corpses"][0]);row.update(pid=8, save=6, name=b"another", items=[])
            value["corpses"].append(row)
            if label == "corpse-order":value["corpses"].reverse()
            elif label == "owner-alias-reused":row["name"] = b"erased-alias"
            else:row["pid"] = 7
        elif label == "saved-duplicate":
            row = copy.deepcopy(value["saved"][0]);row["items"] = [world_item(301)];value["saved"].append(row)
        elif label == "room-duplicate":value["rooms"][1]["room"] = 3003
        else:value["rooms"].reverse()
        add(label, value, False)
    for label, fmt, offset, replacement in (("zero-uid", "Q", 6, 0), ("duplicate-uid", "Q", 6, 101),
                ("zero-vnum", "i", 22, 0), ("equipment", "h", 4, 1), ("parent-negative", "i", 0, -2),
                ("parent-forward", "i", 0, 2), ("parent-self", "i", 0, 1)):
        value = world_model();raw = bytearray(value["corpses"][0]["items"][1]);struct.pack_into("<" + fmt, raw, offset, replacement)
        value["corpses"][0]["items"][1] = bytes(raw);add(label, value, False)
    for count in (4096, 4097):
        value = world_model();value["rooms"][0]["items"] = [world_item(10000 + i) for i in range(count)]
        add("objects-" + str(count), value, count == 4096)
    for depth in (32, 33):
        value = world_model();value["rooms"][0]["items"] = [world_item(10000 + i, parent=i - 1) for i in range(depth)]
        add("depth-" + str(depth), value, depth == 32)
    for size in (4096, 4097):
        value = world_model();value["rooms"][0]["items"] = [world_item(105, strings=(b"", b"", b"", b"x" * size))]
        add("string-" + str(size), value, size == 4096)
    for count in (4095, 4096):
        value = world_model();value["rooms"][0]["items"] = [world_item(105, dynamic_count=4095), world_item(107, dynamic_count=count)]
        add("shared-rows-" + str(count), value, count == 4095)
    for count in (8190, 8191):
        value = world_model();value["rooms"][0]["items"] = [world_item(105, spell_counts=(count,))]
        add("spell-rows-" + str(count), value, count == 8190)
    value = world_model();raw = bytearray(world_item(105, spell_counts=(0,)));raw[-5] = 2
    value["rooms"][0]["items"] = [bytes(raw)];add("description-boolean", value, False)
    description = struct.pack("<I", 4096) + b"k" * 4096 + struct.pack("<I", 4096) + b"d" * 4096 + bytes(5)
    initial = world_item(105)[:-4] + struct.pack("<I", 511) + description * 511
    padding = 4 * 1024 * 1024 - 4 - len(initial)
    assert 0 <= padding < 4096
    for extra in (0, 1):
        raw = world_item(105, strings=(b"", b"", b"", b"a" * (padding + extra)))[:-4] + struct.pack("<I", 511) + description * 511
        assert len(raw) + 4 == 4 * 1024 * 1024 + extra
        value = world_model();value["rooms"][0]["items"] = [raw]
        add("bytes-" + str(extra), value, extra == 0)
    value = world_model();value["revision"] = (1 << 64) - 1
    value["rooms"][0]["items"] = [world_item((1 << 64) - 1, strings=(b"binary\0name", b"\xff", b"", b""))]
    add("full-width-and-binary-item-strings", value, True)
    original = world_frame(world_model())
    for label, encoded in (("truncated", original[:-1]), ("trailing", original + b"\0"),
            ("magic", b"X" + original[1:]), ("checksum", original[:24] + bytes(32) + original[56:]),
            ("version-zero", original[:8] + bytes(4) + original[12:]),
            ("future-version", original[:8] + struct.pack("<I", 4) + original[12:]),
            ("zero-revision", original[:16] + bytes(8) + original[24:])):
        result.append((label, encoded, False, None))
    assert len(result) == len({row[0] for row in result})
    return result


def check_world_catalogs(fixture, binary, build):
    rows = []
    for label, encoded, expected, value in world_cases():
        directory = build / ("world-format-" + label);directory.mkdir(mode=0o700)
        incoming = directory / "world.bin";incoming.write_bytes(encoded);root = directory / "state"
        native = subprocess.run([str(fixture), str(root), str(incoming), "1" if expected else "0", "world"], capture_output=True, text=True, timeout=30)
        assert native.returncode == 0 and not native.stderr, (label, native.stdout, native.stderr)
        observed = json.loads(native.stdout)
        assert observed["native_accepted"] == observed["independent_accepted"] == expected
        if expected:
            custody = directory / "custody.bin";custody.write_bytes(frame(world_custody(value)))
            result = subprocess.run([str(fixture), str(root), str(custody), "1"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        before = inventory(root)
        result = subprocess.run([str(binary), "--economic-world-custody-audit", str(root)], capture_output=True, text=True, timeout=30)
        assert inventory(root) == before and incoming.read_bytes() == encoded
        if expected:
            report = json.loads(result.stdout)
            assert result.returncode == 0 and not result.stderr and report["world_owner_literals_verified"], (label, result)
            assert report["world_items"] == observed["items"]
        else:
            assert result.returncode == 1 and not result.stdout and result.stderr == "native_restore_qualification_failed\n", (label, result)
        (directory / "operator.json").write_text(json.dumps(dict(command=result.args, exit=result.returncode, stdout=result.stdout, stderr=result.stderr), indent=2) + "\n")
        (directory / "authority-before-after.json").write_text(json.dumps(before, sort_keys=True) + "\n")
        row = dict(case=label, accepted=expected, native=observed, input_sha256=hashlib.sha256(encoded).hexdigest(), authority_unchanged=True)
        rows.append(row);print("WORLD_CATALOG " + json.dumps(row, sort_keys=True), flush=True)
    (build / "world-observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    return rows


def check_world_findings(fixture, binary, build):
    cases = []

    def add(label, world, custody, counts):
        cases.append((label, world, custody, counts))

    base = world_model();owned = world_custody(base)
    add("healthy", base, owned, {})
    for state in (2, 3):
        custody = copy.deepcopy(owned);custody["items"][1]["state"] = state
        add("state-" + str(state), base, custody, {"world_uid_not_active": 1})
    for label, location in (("owner", (1, 7, 0)), ("context", (4, (7 << 32) | 5, 99))):
        custody = copy.deepcopy(owned);custody["items"][1]["owner"] = location
        custody["owners"] = sorted({*custody["owners"], (*location, 0)})
        add(label, base, custody, {"world_owner_mismatch": 1})
    for field, replacement in (("root", 999), ("parent", 0)):
        custody = copy.deepcopy(owned);custody["items"][1][field] = replacement
        add(field, base, custody, {"world_item_topology_mismatch": 1})
    custody = copy.deepcopy(owned);custody["items"][0]["vnum"] += 1
    add("vnum", base, custody, {"world_item_vnum_mismatch": 1})
    custody = copy.deepcopy(owned);custody["items"][4].update(owner=(1, 7, 0), equipment=1)
    custody["owners"] = sorted({*custody["owners"], (1, 7, 0, 0)})
    add("equipment", base, custody, {"world_owner_mismatch": 1, "world_item_equipment_mismatch": 1})
    for label, fmt, offset, replacement in (("generated", "q", 14, -55), ("mask", "B", 27, 255),
            ("coin-value", "i", 44, 9), ("other-value", "i", 72, -9), ("timer", "q", 100, -(1 << 63)),
            ("flag", "I", 140, (1 << 32) - 1), ("weight", "i", 144, -15), ("material", "b", 148, -128),
            ("cost", "i", 149, 33), ("condition", "h", 153, -20), ("craftsmanship", "h", 155, 25),
            ("bitvector", "Q", 189, (1 << 64) - 1), ("affect", "h", 211, -32768), ("type", "B", 26, 1)):
        world = copy.deepcopy(base);raw = bytearray(world["rooms"][0]["items"][0]);struct.pack_into("<" + fmt, raw, offset, replacement)
        world["rooms"][0]["items"][0] = bytes(raw)
        add(label, world, owned, {"world_coin_literal_mismatch": 1})
    for index in range(4):
        world = copy.deepcopy(base);strings = [b""] * 4;strings[index] = b"private-synthetic-literal"
        world["rooms"][0]["items"] = [world_item(105, strings=strings)]
        add("string-" + str(index), world, owned, {"world_coin_literal_mismatch": 1})
    for label, kwargs in (("dynamic", dict(dynamic_count=1)), ("description", dict(spell_counts=(1,)))):
        world = copy.deepcopy(base);world["rooms"][0]["items"] = [world_item(105, **kwargs)]
        add(label, world, owned, {"world_coin_literal_mismatch": 1})
    world = copy.deepcopy(base);world["corpses"][0]["items"].pop()
    add("missing-literal", world, owned, {"world_uid_missing_literal": 1})
    world = copy.deepcopy(base);world["rooms"][0]["items"].append(world_item(200))
    add("unadmitted", world, owned, {"world_uid_unadmitted": 1})
    world = copy.deepcopy(base);world["rooms"][0]["items"].extend(world_item(200 + i) for i in range(101))
    add("bounded-details", world, owned, {"world_uid_unadmitted": 101})
    world = copy.deepcopy(base);raw = bytearray(world["rooms"][0]["items"][0]);struct.pack_into("<i", raw, 44, -1)
    world["rooms"][0]["items"] = [bytes(raw)]
    add("negative-native-coin", world, world_custody(world), {"world_negative_coin_value": 1})
    custody = copy.deepcopy(owned)
    for item in custody["items"]:item["payload"] = b""
    add("legacy-inline-absent", base, custody, {})
    custody = copy.deepcopy(owned);custody["owners"].insert(0, (1, 7, 0, 0))
    custody["items"].append(dict(uid=300, root=300, parent=0, owner=(1, 7, 0), revision=1, vnum=57, state=1, payload=b"", equipment=0))
    add("other-owner-uncompared", base, custody, {})
    add("world-absent", None, owned, {"custody_world_catalog_missing": 1, "world_uid_missing_literal": 6})
    add("custody-absent", base, None, {"world_custody_catalog_missing": 1, "world_uid_unadmitted": 6})
    rows = []
    for label, world, custody, counts in cases:
        directory = build / ("world-finding-" + label);directory.mkdir(mode=0o700);root = directory / "state"
        if custody is not None:
            path = directory / "custody.bin";path.write_bytes(frame(custody))
            result = subprocess.run([str(fixture), str(root), str(path), "1"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        if world is not None:
            path = directory / "world.bin";path.write_bytes(world_frame(world))
            result = subprocess.run([str(fixture), str(root), str(path), "1", "world"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        before = inventory(root);reports = []
        for limit in (0, 1, 100):
            command = [str(binary), "--economic-world-custody-audit", str(root), "--limit", str(limit)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=30)
            assert result.returncode == bool(counts) and not result.stderr, (label, limit, result)
            assert inventory(root) == before
            report = json.loads(result.stdout)
            assert report["finding_counts"] == counts and report["finding_count"] == sum(counts.values()), (label, report)
            assert len(report["findings"]) == min(limit, report["finding_count"])
            assert report["findings_truncated"] == (report["finding_count"] > limit)
            assert report["world_owner_literals_verified"] == (not counts)
            assert not any(report[name] for name in ("other_owner_literals_compared", "native_holdings_compared", "item_history_verified", "full_R7_qualified", "release_qualified"))
            assert "erased-alias" not in result.stdout and "private-synthetic-literal" not in result.stdout
            reports.append(dict(command=command, exit=result.returncode, report=report))
        row = dict(case=label, counts=counts, reports=reports, authority_unchanged=True);rows.append(row)
        (directory / "evidence.json").write_text(json.dumps(row, indent=2) + "\n")
        (directory / "authority-before-after.json").write_text(json.dumps(before, sort_keys=True) + "\n")
        print("WORLD_FINDING " + json.dumps(dict(case=label, counts=counts, cuts=3, authority_unchanged=True)), flush=True)
    (build / "world-findings.json").write_text(json.dumps(rows, indent=2) + "\n")
    return rows


def locker_model(version=2):
    return dict(version=version, revision=1, lockers=[
        dict(id=1, name=b"erased-player-locker", pid=7, association=0, account=None,
             racewar=-1, race=-128, revision=90, chests=[
                 dict(id=10, name=b"public", password=b"", public=1, sort=b"", revision=80,
                      items=[world_item(101, kind=1, vnum=57), world_item(102, parent=0)]),
                 dict(id=11, name=b"private", password=b"private-test-only", public=0,
                      sort=b"\x00\xff", revision=81, items=[world_item(103)])])],
        access=[dict(owner=b"erased-player-locker", visitor=b"erased-visitor", revision=9)])


def locker_frame(value):
    def text(value):
        return struct.pack("<I", len(value)) + value
    body = struct.pack("<2I", len(value["lockers"]), len(value["access"]))
    for row in value["lockers"]:
        body += struct.pack("<I", row["id"]) + text(row["name"]) + struct.pack("<2i", row["pid"], row["association"])
        if value["version"] == 2:
            present = row.get("account_present", int(row["account"] is not None))
            body += struct.pack("<B", present)
            if present:
                account, side = row["account"] or (b"test", 0)
                body += text(account) + struct.pack("<b", side)
        body += struct.pack("<bbQI", row["racewar"], row["race"], row["revision"], len(row["chests"]))
        for chest in row["chests"]:
            body += struct.pack("<I", chest["id"]) + text(chest["name"]) + text(chest["password"])
            body += struct.pack("<B", chest["public"]) + text(chest["sort"]) + struct.pack("<Q", chest["revision"])
            encoded = chest.get("encoded", struct.pack("<I", len(chest["items"])) + b"".join(chest["items"]))
            body += struct.pack("<I", len(encoded)) + encoded
    for row in value["access"]:
        body += text(row["owner"]) + text(row["visitor"]) + struct.pack("<Q", row["revision"])
    return b"DURLOCK\0" + struct.pack("<IIQ", value["version"], len(body), value["revision"]) + hashlib.sha256(body).digest() + body


def locker_custody(value):
    owners, items = set(), []
    for locker in value["lockers"]:
        for chest in locker["chests"]:
            owner = (5, locker["id"], chest["id"])
            owners.add((*owner, 0))
            rows = chest["items"]
            for i, raw in enumerate(rows):
                uid, = struct.unpack_from("<Q", raw, 6)
                parent, = struct.unpack_from("<i", raw)
                root = i
                while struct.unpack_from("<i", rows[root])[0] >= 0:
                    root = struct.unpack_from("<i", rows[root])[0]
                detached = struct.pack("<Ii", 1, -1) + raw[4:]
                items.append(dict(uid=uid, root=struct.unpack_from("<Q", rows[root], 6)[0],
                    parent=0 if parent < 0 else struct.unpack_from("<Q", rows[parent], 6)[0], owner=owner,
                    revision=1, vnum=struct.unpack_from("<i", raw, 22)[0], state=1, equipment=0,
                    payload=detached if raw[26] == 20 and len(detached) <= 128 * 1024 else b""))
    return dict(version=8, revision=1, owners=sorted(owners), items=sorted(items, key=lambda row: row["uid"]), operations=[])


def locker_cases():
    result = []
    def add(label, value, accepted):
        result.append((label, locker_frame(value), accepted, value if accepted else None))
    for version in (1, 2):
        add("version-"+str(version), locker_model(version), True)
        value=locker_model(version);value["lockers"]=[];value["access"]=[]
        add("empty-"+str(version),value,True)
    value=locker_model();value["lockers"][0].update(pid=0,association=2**31-1)
    add("association-owner",value,True)
    for side in (0,4):
        value=locker_model();row=value["lockers"][0]
        row.update(pid=0,account=(b"erased-account",side),racewar=side,name=b"account.erased-account."+str(side).encode()+b".locker")
        value["access"][0]["owner"]=row["name"]
        add("account-owner-"+str(side),value,True)
    value=locker_model();row=value["lockers"][0];row.update(id=2**32-1,pid=2**31-1,revision=2**64-1)
    value["revision"]=2**64-1;row["chests"][0]["id"]=2**32-2;row["chests"][1]["id"]=2**32-1
    row["chests"][1].update(revision=2**64-1,items=[world_item(2**64-1)])
    add("full-width-identities-clocks",value,True)
    value=locker_model();value["lockers"][0]["chests"][1].update(password=bytes(range(64)),sort=b"\x00\xff"*2048)
    add("binary-private-policy-at-bound",value,True)
    value=locker_model();value["lockers"][0]["chests"][1]["password"]=b""
    add("empty-private-password",value,True)
    value=locker_model();row=value["lockers"][0];row["name"]=b"x"*100;row["chests"][0]["name"]=b"x"*32
    value["access"][0].update(owner=row["name"],visitor=b"x"*255)
    add("maximum-names",value,True)
    value=locker_model();value["lockers"][0]["chests"][1]["items"]=[world_item(103,strings=(b"\x00\xff",b"",b"",b"x"*4096))]
    add("complete-binary-item-fields",value,True)
    value=locker_model();value["lockers"][0]["chests"][1]["items"]=[]
    add("empty-chest-forest",value,True)
    for label, field, replacement in (
        ("zero-id","id",0),("zero-clock","revision",0),("no-owner","pid",0),
        ("negative-pid","pid",-1),("negative-association","association",-1),
        ("two-owners","association",1),("account-present-bool","account_present",2),
        ("reserved-account-prefix","name",b"account.test.0.locker"),
        ("no-chests","chests",[])):
        value=locker_model();value["lockers"][0][field]=replacement;value["access"]=[]
        add("locker-"+label,value,False)
    for scope, maximum in (("locker",100),("chest",32),("visitor",255)):
        for label,name in (("empty",b""),("uppercase",b"Upper"),("space",b"has space"),
                           ("control",b"x\x00"),("DEL",b"x\x7f"),("high-byte",b"x\x80"),("over-bound",b"x"*(maximum+1))):
            value=locker_model()
            if scope=="locker":value["lockers"][0]["name"]=name;value["access"]=[]
            elif scope=="chest":value["lockers"][0]["chests"][0]["name"]=name
            else:value["access"][0]["visitor"]=name
            add(scope+"-name-"+label,value,False)
    for label,account,side,racewar,name in (
        ("uppercase",b"Upper",0,0,b"account.Upper.0.locker"),
        ("empty",b"",0,0,b"account..0.locker"),
        ("over-name",b"x"*51,0,0,b"account."+b"x"*51+b".0.locker"),
        ("negative-side",b"test",-1,-1,b"account.test.-1.locker"),
        ("over-side",b"test",5,5,b"account.test.5.locker"),
        ("racewar-mismatch",b"test",0,1,b"account.test.0.locker"),
        ("name-mismatch",b"test",0,0,b"other")):
        value=locker_model();value["lockers"][0].update(pid=0,account=(account,side),racewar=racewar,name=name);value["access"]=[]
        add("account-"+label,value,False)
    for label,field,replacement in (("zero-id","id",0),("zero-clock","revision",0),
        ("public-bool","public",2),("public-password","password",b"secret"),
        ("sort-over-bound","sort",b"x"*4097),("no-encoded-items","encoded",b"")):
        value=locker_model();value["lockers"][0]["chests"][0][field]=replacement
        add("chest-"+label,value,False)
    value=locker_model();value["lockers"][0]["chests"][1]["password"]=b"x"*65
    add("password-over-bound",value,False)
    for label,second in (("no-public",0),("two-public",1)):
        value=locker_model();row=value["lockers"][0]["chests"]
        row[0]["public"]=0 if label=="no-public" else 1;row[1].update(public=second,password=b"")
        add(label,value,False)
    for label,scope,field in (("duplicate-locker-id","locker","id"),("duplicate-locker-name","locker","name"),
                             ("duplicate-pid-owner","locker","pid"),("duplicate-chest-id","chest","id"),
                             ("duplicate-chest-name","chest","name")):
        value=locker_model()
        if scope=="locker":
            other=copy.deepcopy(value["lockers"][0]);other.update(id=2,name=b"other",pid=8)
            other["chests"][0].update(id=12,items=[]);other["chests"][1].update(id=13,items=[])
            other[field]=value["lockers"][0][field];value["lockers"].append(other)
        else:value["lockers"][0]["chests"][1][field]=value["lockers"][0]["chests"][0][field]
        add(label,value,False)
    value=locker_model();value["lockers"][0]["chests"].reverse();add("unsorted-chests",value,False)
    value=locker_model();value["lockers"][0]["chests"][1]["items"]=[world_item(102)];add("duplicate-item-uid",value,False)
    for label,raw in (("zero-uid",world_item(0)),("zero-vnum",world_item(103,vnum=0)),
                      ("forward-parent",world_item(103,parent=0))):
        value=locker_model();value["lockers"][0]["chests"][1]["items"]=[raw];add(label,value,False)
    value=locker_model();raw=bytearray(world_item(103));struct.pack_into("<h",raw,4,0)
    value["lockers"][0]["chests"][1]["items"]=[bytes(raw)];add("equipped-chest-item",value,False)
    for label,depth,accepted in (("depth-exact",32,True),("depth-over",33,False)):
        value=locker_model();value["lockers"][0]["chests"][1]["items"]=[world_item(1000+i,parent=i-1) for i in range(depth)]
        add(label,value,accepted)
    for label,field,replacement in (("unknown-access-owner","owner",b"other"),("zero-access-clock","revision",0)):
        value=locker_model();value["access"][0][field]=replacement;add(label,value,False)
    value=locker_model();value["access"].append(copy.deepcopy(value["access"][0]));add("duplicate-access",value,False)
    value=locker_model();value["access"].append(dict(owner=value["access"][0]["owner"],visitor=b"aaa",revision=1));add("unsorted-access",value,False)
    original=locker_frame(locker_model())
    def raw(label,encoded):result.append((label,encoded,False,None))
    def changed(label,offset,value):
        body=bytearray(original);body[offset:offset+len(value)]=value
        if offset>=56:body[24:56]=hashlib.sha256(body[56:]).digest()
        raw(label,bytes(body))
    for label,offset,data in (("magic",0,b"X"),("version-zero",8,struct.pack("<I",0)),
        ("version-three",8,struct.pack("<I",3)),("length",12,struct.pack("<I",0)),
        ("catalog-clock-zero",16,bytes(8)),("checksum",24,b"X"),
        ("locker-count-over",56,struct.pack("<I",65537)),("access-count-over",60,struct.pack("<I",1048577))):
        changed(label,offset,data)
    raw("truncated-header",original[:55]);raw("truncated-body",original[:-1]);raw("trailing-byte",original+b"x")
    assert len({row[0] for row in result})==len(result)
    return result


def check_locker_catalogs(fixture, binary, build):
    rows = []
    for label, encoded, expected, value in locker_cases():
        directory = build / ("locker-format-" + label);directory.mkdir(mode=0o700)
        incoming = directory / "locker.bin";incoming.write_bytes(encoded);root = directory / "state"
        native = subprocess.run([str(fixture), str(root), str(incoming), "1" if expected else "0", "locker"], capture_output=True, text=True, timeout=30)
        assert native.returncode == 0 and not native.stderr, (label, native.stdout, native.stderr)
        observed = json.loads(native.stdout)
        assert observed["native_accepted"] == observed["independent_accepted"] == expected
        if expected:
            custody = directory / "custody.bin";custody.write_bytes(frame(locker_custody(value)))
            result = subprocess.run([str(fixture), str(root), str(custody), "1"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        before = inventory(root)
        result = subprocess.run([str(binary), "--economic-locker-custody-audit", str(root)], capture_output=True, text=True, timeout=30)
        assert inventory(root) == before and incoming.read_bytes() == encoded
        assert all(secret not in result.stdout for secret in ("erased-player-locker", "erased-visitor", "erased-account", "private-test-only", "private-synthetic-literal"))
        if expected:
            report = json.loads(result.stdout)
            assert result.returncode == 0 and not result.stderr and report["locker_owner_literals_verified"], (label, result)
            assert report["locker_items"] == observed["items"]
        else:
            assert result.returncode == 1 and not result.stdout and result.stderr == "native_restore_qualification_failed\n", (label, result)
        (directory / "operator.json").write_text(json.dumps(dict(command=result.args, exit=result.returncode, stdout=result.stdout, stderr=result.stderr), indent=2) + "\n")
        (directory / "authority-before-after.json").write_text(json.dumps(before, sort_keys=True) + "\n")
        row = dict(case=label, accepted=expected, native=observed, input_sha256=hashlib.sha256(encoded).hexdigest(), authority_unchanged=True)
        rows.append(row);print("LOCKER_CATALOG " + json.dumps(row, sort_keys=True), flush=True)
    (build / "locker-observations.json").write_text(json.dumps(rows, indent=2) + "\n")
    return rows


def check_locker_findings(fixture, binary, build):
    cases = []

    def add(label, locker, custody, counts):
        cases.append((label, locker, custody, counts))

    base = locker_model();owned = locker_custody(base)
    add("healthy", base, owned, {})
    for state in (2, 3):
        custody = copy.deepcopy(owned);custody["items"][1]["state"] = state
        add("state-" + str(state), base, custody, {"locker_uid_not_active": 1})
    for label, location in (("owner", (1, 7, 0)), ("context", (5, 1, 99))):
        custody = copy.deepcopy(owned);custody["items"][1]["owner"] = location
        custody["owners"] = sorted({*custody["owners"], (*location, 0)})
        add(label, base, custody, {"locker_owner_mismatch": 1})
    for field, replacement in (("root", 999), ("parent", 0)):
        custody = copy.deepcopy(owned);custody["items"][1][field] = replacement
        add(field, base, custody, {"locker_item_topology_mismatch": 1})
    custody = copy.deepcopy(owned);custody["items"][0]["vnum"] += 1
    add("vnum", base, custody, {"locker_item_vnum_mismatch": 1})
    custody = copy.deepcopy(owned);custody["items"][2].update(owner=(1, 7, 0), equipment=1)
    custody["owners"] = sorted({*custody["owners"], (1, 7, 0, 0)})
    add("equipment", base, custody, {"locker_owner_mismatch": 1, "locker_item_equipment_mismatch": 1})
    for label, fmt, offset, replacement in (("generated", "q", 14, -55), ("mask", "B", 27, 255),
            ("coin-value", "i", 44, 9), ("other-value", "i", 72, -9), ("timer", "q", 100, -(1 << 63)),
            ("flag", "I", 140, (1 << 32) - 1), ("weight", "i", 144, -15), ("material", "b", 148, -128),
            ("cost", "i", 149, 33), ("condition", "h", 153, -20), ("craftsmanship", "h", 155, 25),
            ("bitvector", "Q", 189, (1 << 64) - 1), ("affect", "h", 211, -32768), ("type", "B", 26, 1)):
        locker = copy.deepcopy(base);raw = bytearray(locker["lockers"][0]["chests"][1]["items"][0]);struct.pack_into("<" + fmt, raw, offset, replacement)
        locker["lockers"][0]["chests"][1]["items"][0] = bytes(raw)
        add(label, locker, owned, {"locker_coin_literal_mismatch": 1})
    for index in range(4):
        locker = copy.deepcopy(base);strings = [b""] * 4;strings[index] = b"private-synthetic-literal"
        locker["lockers"][0]["chests"][1]["items"] = [world_item(103, strings=strings)]
        add("string-" + str(index), locker, owned, {"locker_coin_literal_mismatch": 1})
    for label, kwargs in (("dynamic", dict(dynamic_count=1)), ("description", dict(spell_counts=(1,)))):
        locker = copy.deepcopy(base);locker["lockers"][0]["chests"][1]["items"] = [world_item(103, **kwargs)]
        add(label, locker, owned, {"locker_coin_literal_mismatch": 1})
    locker = copy.deepcopy(base);locker["lockers"][0]["chests"][0]["items"].pop()
    add("missing-literal", locker, owned, {"locker_uid_missing_literal": 1})
    locker = copy.deepcopy(base);locker["lockers"][0]["chests"][1]["items"].append(world_item(200))
    add("unadmitted", locker, owned, {"locker_uid_unadmitted": 1})
    locker = copy.deepcopy(base);locker["lockers"][0]["chests"][1]["items"].extend(world_item(200 + i) for i in range(101))
    add("bounded-details", locker, owned, {"locker_uid_unadmitted": 101})
    locker = copy.deepcopy(base);raw = bytearray(locker["lockers"][0]["chests"][1]["items"][0]);struct.pack_into("<i", raw, 44, -1)
    locker["lockers"][0]["chests"][1]["items"] = [bytes(raw)]
    add("negative-native-coin", locker, locker_custody(locker), {"locker_negative_coin_value": 1})
    custody = copy.deepcopy(owned)
    for item in custody["items"]:item["payload"] = b""
    add("legacy-inline-absent", base, custody, {})
    custody = copy.deepcopy(owned);custody["owners"].insert(0, (1, 7, 0, 0))
    custody["items"].append(dict(uid=300, root=300, parent=0, owner=(1, 7, 0), revision=1, vnum=57, state=1, payload=b"", equipment=0))
    add("other-owner-uncompared", base, custody, {})
    add("locker-absent", None, owned, {"custody_locker_catalog_missing": 1, "locker_uid_missing_literal": 3})
    add("custody-absent", base, None, {"locker_custody_catalog_missing": 1, "locker_uid_unadmitted": 3})
    rows = []
    for label, locker, custody, counts in cases:
        directory = build / ("locker-finding-" + label);directory.mkdir(mode=0o700);root = directory / "state"
        if custody is not None:
            path = directory / "custody.bin";path.write_bytes(frame(custody))
            result = subprocess.run([str(fixture), str(root), str(path), "1"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        if locker is not None:
            path = directory / "locker.bin";path.write_bytes(locker_frame(locker))
            result = subprocess.run([str(fixture), str(root), str(path), "1", "locker"], capture_output=True, text=True, timeout=30)
            assert result.returncode == 0 and not result.stderr, (label, result)
        before = inventory(root);reports = []
        for limit in (0, 1, 100):
            command = [str(binary), "--economic-locker-custody-audit", str(root), "--limit", str(limit)]
            result = subprocess.run(command, capture_output=True, text=True, timeout=30)
            assert result.returncode == bool(counts) and not result.stderr, (label, limit, result)
            assert inventory(root) == before
            report = json.loads(result.stdout)
            assert report["finding_counts"] == counts and report["finding_count"] == sum(counts.values()), (label, report)
            assert len(report["findings"]) == min(limit, report["finding_count"])
            assert report["findings_truncated"] == (report["finding_count"] > limit)
            assert report["locker_owner_literals_verified"] == (not counts)
            assert not any(report[name] for name in ("other_owner_literals_compared", "native_holdings_compared", "item_history_verified", "full_R7_qualified", "release_qualified"))
            assert all(secret not in result.stdout for secret in ("erased-player-locker", "erased-visitor", "erased-account", "private-test-only", "private-synthetic-literal"))
            reports.append(dict(command=command, exit=result.returncode, report=report))
        row = dict(case=label, counts=counts, reports=reports, authority_unchanged=True);rows.append(row)
        (directory / "evidence.json").write_text(json.dumps(row, indent=2) + "\n")
        (directory / "authority-before-after.json").write_text(json.dumps(before, sort_keys=True) + "\n")
        print("LOCKER_FINDING " + json.dumps(dict(case=label, counts=counts, cuts=3, authority_unchanged=True)), flush=True)
    (build / "locker-findings.json").write_text(json.dumps(rows, indent=2) + "\n")
    return rows

def shop_item(uid, *, equipment=0, **kwargs):
    raw = bytearray(world_item(uid, **kwargs))
    struct.pack_into("<h", raw, 4, equipment)
    return bytes(raw)


def shop_model(version=2):
    return dict(version=version, revision=1, keepers=[dict(id=0, mobile=5, room=10,
        saved_at=0, revision=90, cash=123, roaming=1,
        affects=[(1, -2, -3, -4, (0, 1, 2, 3, 2**64-1))],
        items=[shop_item(101, kind=1, vnum=57, equipment=255),
               shop_item(102, parent=0), shop_item(103, equipment=1)])])


def shop_frame(value):
    body = struct.pack("<I", len(value["keepers"]))
    for row in value["keepers"]:
        body += struct.pack("<IiiqQ", row["id"], row["mobile"], row["room"], row["saved_at"], row["revision"])
        if value["version"] == 2:
            body += struct.pack("<qB", row["cash"], row["roaming"])
        body += struct.pack("<I", len(row["affects"]))
        for type_, duration, modifier, location, bits in row["affects"]:
            body += struct.pack("<4i5Q", type_, duration, modifier, location, *bits)
        encoded = row.get("encoded", struct.pack("<I", len(row["items"])) + b"".join(row["items"]))
        body += struct.pack("<I", len(encoded)) + encoded
    return b"DURSHOP\0" + struct.pack("<IIQ", value["version"], len(body), value["revision"]) + hashlib.sha256(body).digest() + body


def shop_custody(value):
    # Reuse the established complete custody model, preserving native keeper slots
    # in detached coin literals and keeping the separate custody slot zero.
    proxy = dict(lockers=[dict(id=row["id"]+1, chests=[dict(id=1, items=row["items"])]) for row in value["keepers"]])
    result = locker_custody(proxy)
    result["owners"] = [(9, row[1], 0, row[3]) for row in result["owners"]]
    for row in result["items"]:
        row["owner"] = (9, row["owner"][1], 0)
    return result


def shop_cases():
    rows = []
    def add(label, value, accepted):
        rows.append((label, shop_frame(value), accepted, value if accepted else None))
    for version in (1, 2):
        add("version-"+str(version), shop_model(version), True)
        value=shop_model(version);value["keepers"]=[]
        add("empty-"+str(version), value, True)
    value=shop_model();row=value["keepers"][0]
    row.update(id=2**32-1,mobile=2**31-1,room=2**31-1,saved_at=2**63-1,revision=2**64-1,cash=2**31-1)
    value["revision"]=2**64-1;row["items"]=[shop_item(2**64-1)]
    add("full-width-identities-clocks",value,True)
    for cash in (-1,0,2**31-1):
        value=shop_model();value["keepers"][0]["cash"]=cash
        add("cash-"+str(cash),value,True)
    for label, affects in (("empty-affects",[]),("duplicate-affects",[(0,0,0,0,(0,)*5)]*2),
            ("maximum-affects",[(0,0,0,0,(0,)*5)]*4096),
            ("signed-affect-order",[(-2**31,2**31-1,-2**31,2**31-1,(2**64-1,)*5),(2**31-1,-2**31,2**31-1,-2**31,(0,)*5)])):
        value=shop_model();value["keepers"][0]["affects"]=affects;add(label,value,True)
    for label, items in (("empty-items",[]),("inventory",[shop_item(101)]),
            ("complete-binary-item",[shop_item(101,strings=(b"private-synthetic-literal",b"\x00\xff",b"",b"x"*4096))]),
            ("maximum-items",[shop_item(1000+i,kind=1,vnum=57) for i in range(4096)])):
        value=shop_model();value["keepers"][0]["items"]=items;add(label,value,True)
    value=shop_model();second=copy.deepcopy(value["keepers"][0]);second.update(id=1,items=[shop_item(200,equipment=255)])
    value["keepers"].append(second);add("slots-per-keeper",value,True)
    for label, field, replacement in (("zero-mobile","mobile",0),("negative-mobile","mobile",-1),
            ("zero-room","room",0),("negative-room","room",-1),("negative-time","saved_at",-1),
            ("zero-clock","revision",0),("cash-negative","cash",-2),("cash-overflow","cash",2**31),
            ("roaming-bool","roaming",2),("affect-overflow","affects",[(0,0,0,0,(0,)*5)]*4097),
            ("affect-order","affects",[(1,0,0,0,(0,)*5),(0,0,0,0,(0,)*5)])):
        value=shop_model();value["keepers"][0][field]=replacement;add(label,value,False)
    for label, items in (("zero-uid",[shop_item(0)]),("duplicate-uid",[shop_item(101),shop_item(101)]),
            ("zero-vnum",[shop_item(101,vnum=0)]),("negative-slot",[world_item(101)]),
            ("slot-overflow",[shop_item(101,equipment=256)]),
            ("duplicate-slot",[shop_item(101,equipment=1),shop_item(102,equipment=1)]),
            ("child-slot",[shop_item(101),shop_item(102,parent=0,equipment=1)]),
            ("bad-parent",[shop_item(101,parent=0)]),
            ("depth-overflow",[shop_item(101+i,parent=i-1) for i in range(33)]),
            ("string-overflow",[shop_item(101,strings=(b"x"*4097,b"",b"",b""))]),
            ("row-budget",[shop_item(101,dynamic_count=8192)])):
        value=shop_model();value["keepers"][0]["items"]=items;add(label,value,False)
    for label, ids, duplicate in (("duplicate-shop",[0,0],False),("shop-order",[1,0],False),("global-uid",[0,1],True)):
        value=shop_model();second=copy.deepcopy(value["keepers"][0]);value["keepers"][0]["id"]=ids[0]
        second.update(id=ids[1],items=[shop_item(101 if duplicate else 200)]);value["keepers"].append(second);add(label,value,False)
    for label, encoded in (("zero-list",b""),("item-count-overflow",struct.pack("<I",4097)),
            ("item-truncation",struct.pack("<I",1)+shop_item(101)[:-1]),
            ("item-trailing",struct.pack("<I",0)+b"x")):
        value=shop_model();value["keepers"][0]["encoded"]=encoded;add(label,value,False)
    good=shop_frame(shop_model())
    for label, offset, fmt, replacement in (("unknown-zero",8,"I",0),("unknown-version",8,"I",3),
            ("zero-catalog-clock",16,"Q",0),("body-size",12,"I",1),("checksum",24,"B",good[24]^1),("magic",0,"B",0)):
        raw=bytearray(good);struct.pack_into("<"+fmt,raw,offset,replacement);rows.append((label,bytes(raw),False,None))
    for label, encoded in (("truncated",good[:-1]),("trailing",good+b"x"),("header-only",good[:55])):
        rows.append((label,encoded,False,None))
    for label, body in (("catalog-count-overflow",struct.pack("<I",262145)),("empty-trailing",struct.pack("<I",0)+b"x")):
        rows.append((label,b"DURSHOP\0"+struct.pack("<IIQ",2,len(body),1)+hashlib.sha256(body).digest()+body,False,None))
    return rows


def check_shop_catalogs(fixture, binary, build):
    rows=[]
    for label, encoded, expected, value in shop_cases():
        directory=build/("shop-format-"+label);directory.mkdir(mode=0o700);root=directory/"state"
        incoming=directory/"shop.bin";incoming.write_bytes(encoded)
        result=subprocess.run([str(fixture),str(root),str(incoming),"1" if expected else "0","shopkeeper"],capture_output=True,text=True,timeout=30)
        assert result.returncode==0 and not result.stderr,(label,result)
        native=json.loads(result.stdout);assert native["native_accepted"]==native["independent_accepted"]==expected
        if expected:
            path=directory/"custody.bin";path.write_bytes(frame(shop_custody(value)))
            result=subprocess.run([str(fixture),str(root),str(path),"1"],capture_output=True,text=True,timeout=30)
            assert result.returncode==0 and not result.stderr,(label,result)
        before=inventory(root)
        result=subprocess.run([str(binary),"--economic-shopkeeper-custody-audit",str(root)],capture_output=True,text=True,timeout=30)
        assert inventory(root)==before and incoming.read_bytes()==encoded
        if expected:
            report=json.loads(result.stdout)
            assert result.returncode==0 and not result.stderr and report["shop_owner_literals_verified"],(label,result)
            known=[row["cash"] for row in value["keepers"] if row["cash"]>=0] if value["version"]==2 else []
            assert report["shop_items"]==native["items"] and report["cash_known"]==len(known)
            assert report["cash_legacy"]==len(value["keepers"])-len(known) and report["retained_cash_copper"]==str(sum(known))
            assert not report["native_holdings_compared"] and not report["release_qualified"]
        else:
            assert result.returncode==1 and not result.stdout and result.stderr=="native_restore_qualification_failed\n",(label,result)
        assert "private-synthetic-literal" not in result.stdout
        (directory/"operator.json").write_text(json.dumps(dict(command=result.args,exit=result.returncode,stdout=result.stdout,stderr=result.stderr),indent=2)+"\n")
        (directory/"authority-before-after.json").write_text(json.dumps(before,sort_keys=True)+"\n")
        row=dict(case=label,accepted=expected,native=native,input_sha256=hashlib.sha256(encoded).hexdigest(),authority_unchanged=True)
        rows.append(row);print("SHOP_CATALOG "+json.dumps(row,sort_keys=True),flush=True)
    (build/"shop-observations.json").write_text(json.dumps(rows,indent=2)+"\n")
    return rows


def check_shop_findings(fixture, binary, build):
    base=shop_model();owned=shop_custody(base);cases=[("healthy",base,owned,{})]
    def add(label,value,custody,counts):cases.append((label,value,custody,counts))
    for state in (2,3):
        custody=copy.deepcopy(owned);custody["items"][1]["state"]=state
        add("state-"+str(state),base,custody,{"shop_uid_not_active":1})
    for label, owner in (("owner",(1,7,0)),("context",(9,1,99))):
        custody=copy.deepcopy(owned);custody["items"][1]["owner"]=owner
        custody["owners"]=sorted({*custody["owners"],(*owner,0)})
        add(label,base,custody,{"shop_owner_mismatch":1})
    for field,replacement in (("root",999),("parent",0)):
        custody=copy.deepcopy(owned);custody["items"][1][field]=replacement
        add(field,base,custody,{"shop_item_topology_mismatch":1})
    custody=copy.deepcopy(owned);custody["items"][0]["vnum"]+=1
    add("vnum",base,custody,{"shop_item_vnum_mismatch":1})
    custody=copy.deepcopy(owned);custody["items"][0].update(owner=(1,7,0),equipment=1)
    custody["owners"]=sorted({*custody["owners"],(1,7,0,0)})
    add("equipment",base,custody,{"shop_owner_mismatch":1,"shop_item_equipment_mismatch":1})
    for label, fmt, offset, replacement in (("generated","q",14,-55),("mask","B",27,255),
            ("coin-value","i",44,9),("other-value","i",72,-9),("timer","q",100,-2**63),
            ("flag","I",140,2**32-1),("weight","i",144,-15),("material","b",148,-128),
            ("cost","i",149,33),("condition","h",153,-20),("craftsmanship","h",155,25),
            ("bitvector","Q",189,2**64-1),("affect","h",211,-32768),("type","B",26,1),
            ("native-equipment","h",4,2)):
        value=copy.deepcopy(base);raw=bytearray(value["keepers"][0]["items"][2]);struct.pack_into("<"+fmt,raw,offset,replacement)
        value["keepers"][0]["items"][2]=bytes(raw);add(label,value,owned,{"shop_coin_literal_mismatch":1})
    for index in range(4):
        value=copy.deepcopy(base);strings=[b""]*4;strings[index]=b"private-synthetic-literal"
        value["keepers"][0]["items"][2]=shop_item(103,equipment=1,strings=strings)
        add("string-"+str(index),value,owned,{"shop_coin_literal_mismatch":1})
    for label,kwargs in (("dynamic",dict(dynamic_count=1)),("description",dict(spell_counts=(1,)))):
        value=copy.deepcopy(base);value["keepers"][0]["items"][2]=shop_item(103,equipment=1,**kwargs)
        add(label,value,owned,{"shop_coin_literal_mismatch":1})
    value=copy.deepcopy(base);value["keepers"][0]["items"].pop()
    add("missing-literal",value,owned,{"shop_uid_missing_literal":1})
    value=copy.deepcopy(base);value["keepers"][0]["items"].append(shop_item(200))
    add("unadmitted",value,owned,{"shop_uid_unadmitted":1})
    value=copy.deepcopy(base);value["keepers"][0]["items"].extend(shop_item(200+i) for i in range(101))
    add("bounded-details",value,owned,{"shop_uid_unadmitted":101})
    value=copy.deepcopy(base);raw=bytearray(value["keepers"][0]["items"][2]);struct.pack_into("<i",raw,44,-1)
    value["keepers"][0]["items"][2]=bytes(raw)
    add("negative-native-coin",value,shop_custody(value),{"shop_negative_coin_value":1})
    custody=copy.deepcopy(owned)
    for row in custody["items"]:row["payload"]=b""
    add("legacy-inline-absent",base,custody,{})
    custody=copy.deepcopy(owned);custody["owners"].insert(0,(1,7,0,0))
    custody["items"].append(dict(uid=300,root=300,parent=0,owner=(1,7,0),revision=1,vnum=57,state=1,payload=b"",equipment=0))
    add("other-owner-uncompared",base,custody,{})
    add("shop-absent",None,owned,{"custody_shop_catalog_missing":1,"shop_uid_missing_literal":3})
    add("custody-absent",base,None,{"shop_custody_catalog_missing":1,"shop_uid_unadmitted":3})
    rows=[]
    for label,value,custody,counts in cases:
        directory=build/("shop-finding-"+label);directory.mkdir(mode=0o700);root=directory/"state"
        for data,family,encoder in ((custody,None,frame),(value,"shopkeeper",shop_frame)):
            if data is None:continue
            path=directory/("shop.bin" if family else "custody.bin");path.write_bytes(encoder(data))
            command=[str(fixture),str(root),str(path),"1"]+([family] if family else [])
            result=subprocess.run(command,capture_output=True,text=True,timeout=30)
            assert result.returncode==0 and not result.stderr,(label,result)
        before=inventory(root);reports=[]
        for limit in (0,1,100):
            command=[str(binary),"--economic-shopkeeper-custody-audit",str(root),"--limit",str(limit)]
            result=subprocess.run(command,capture_output=True,text=True,timeout=30)
            assert result.returncode==bool(counts) and not result.stderr and inventory(root)==before,(label,limit,result)
            report=json.loads(result.stdout)
            assert report["finding_counts"]==counts and report["finding_count"]==sum(counts.values()),(label,report)
            assert len(report["findings"])==min(limit,report["finding_count"])
            assert report["findings_truncated"]==(report["finding_count"]>limit) and report["shop_owner_literals_verified"]==(not counts)
            assert not any(report[name] for name in ("other_owner_literals_compared","native_holdings_compared","item_history_verified","full_R7_qualified","release_qualified"))
            assert "private-synthetic-literal" not in result.stdout
            reports.append(dict(command=command,exit=result.returncode,report=report))
        row=dict(case=label,counts=counts,reports=reports,authority_unchanged=True);rows.append(row)
        (directory/"evidence.json").write_text(json.dumps(row,indent=2)+"\n")
        (directory/"authority-before-after.json").write_text(json.dumps(before,sort_keys=True)+"\n")
        print("SHOP_FINDING "+json.dumps(dict(case=label,counts=counts,cuts=3,authority_unchanged=True)),flush=True)
    (build/"shop-findings.json").write_text(json.dumps(rows,indent=2)+"\n")
    return rows


def check_boundaries(fixture, binary, build, world=False, locker=False, shop=False, player=False, auction=False, auction_native=None):
    import fcntl

    prefix = "auction-boundary-" if auction else "player-boundary-" if player else "shop-boundary-" if shop else "locker-boundary-" if locker else "world-boundary-" if world else "boundary-"
    golden = build / (prefix + "golden")
    incoming = build / (prefix + "input.bin")
    if auction:
        golden.mkdir(mode=0o700)
        seeded = subprocess.run([str(auction_native), "seed", str(golden)], capture_output=True, text=True, timeout=30)
        assert seeded.returncode == 0 and not seeded.stderr, seeded
    incoming.write_bytes(frame(auction_custody(auction_model())) if auction else frame(player_custody(player_model())) if player else frame(shop_custody(shop_model())) if shop else frame(locker_custody(locker_model())) if locker else frame(world_custody(world_model())) if world else frame(model()))
    native = subprocess.run([str(fixture), str(golden), str(incoming), "1"],
                            capture_output=True, text=True, timeout=30)
    assert native.returncode == 0 and not native.stderr, native
    if auction:
        (golden / "domains/auction_catalog").write_bytes(auction_frame(auction_model()))
    if world or locker or shop or player:
        incoming.write_bytes(player_frame(player_model()) if player else shop_frame(shop_model()) if shop else locker_frame(locker_model()) if locker else world_frame(world_model()))
        native = subprocess.run([str(fixture), str(golden), str(incoming), "1", "player" if player else "shopkeeper" if shop else "locker" if locker else "world"],
                                capture_output=True, text=True, timeout=30)
        assert native.returncode == 0 and not native.stderr, native
    rows = []
    labels = ("healthy", "empty", "uninitialized", "missing-lock", "held-lock",
                  "critical-journal", "currency-journal", "player-journal", "public-file",
                  "public-root", "symlink", "dangling-symlink", "hardlink")
    for label in labels + (("zero-file",) if world or locker or shop or player or auction else ()) + (("public-players", "symlink-players", "noncanonical-pid", "pid-file-mismatch") if player else ()):
        root = build / (prefix + label)
        shutil.copytree(golden, root, copy_function=shutil.copy2)
        domains = root / "domains"
        catalog = root / "players/7.snapshot" if player else domains / ("auction_catalog" if auction else "shopkeeper_catalog" if shop else "locker_catalog" if locker else "world_item_catalog" if world else "item_ownership")
        held = None
        if label in ("empty", "uninitialized"):
            catalog.unlink()
            if world or locker or shop or player or auction:
                (domains / "item_ownership").unlink()
            if label == "uninitialized":
                shutil.rmtree(domains)
                if player:
                    shutil.rmtree(root / "players")
        elif label == "missing-lock":
            (domains / ".critical-authority.lock").unlink()
        elif label == "held-lock":
            held = (domains / ".critical-authority.lock").open("rb")
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
        elif label.endswith("journal"):
            marker = {"critical-journal": ".critical-authority-transaction",
                      "currency-journal": ".currency-transaction",
                      "player-journal": ".player-domain-transaction"}[label]
            (domains / marker).write_bytes(b"pending")
        elif label == "public-file":
            catalog.chmod(0o644)
        elif label == "public-root":
            root.chmod(0o755)
        elif label in ("symlink", "dangling-symlink"):
            catalog.unlink()
            catalog.symlink_to(incoming if label == "symlink" else build / "absent.bin")
        elif label == "hardlink":
            os.link(catalog, root / "duplicate")
        elif label == "zero-file":
            catalog.write_bytes(b"")
        elif label == "public-players":
            (root / "players").chmod(0o755)
        elif label == "symlink-players":
            (root / "players").rename(root / "original-players")
            (root / "players").symlink_to(root / "original-players", target_is_directory=True)
        elif label in ("noncanonical-pid", "pid-file-mismatch"):
            catalog.rename(root / "players" / ("07.snapshot" if label == "noncanonical-pid" else "8.snapshot"))
        before = inventory(root)
        option = "--economic-auction-custody-audit" if auction else "--economic-player-custody-audit" if player else "--economic-shopkeeper-custody-audit" if shop else "--economic-locker-custody-audit" if locker else "--economic-world-custody-audit" if world else "--economic-custody-catalog-audit"
        result = subprocess.run([str(binary), option, str(root)],
                                capture_output=True, text=True, timeout=30)
        assert inventory(root) == before
        if held is not None:
            held.close()
        expected = label in ("healthy", "empty", "uninitialized")
        if expected:
            assert result.returncode == 0 and not result.stderr, (label, result)
            value = json.loads(result.stdout)
            if auction:
                assert value["auction_root_metadata_verified"] == (label == "healthy")
            elif player:
                assert value["player_pet_owner_literals_verified"] == (label == "healthy")
            elif shop:
                assert value["shop_present"] == value["shop_owner_literals_verified"] == (label == "healthy")
            elif locker:
                assert value["locker_present"] == value["locker_owner_literals_verified"] == (label == "healthy")
            elif world:
                assert value["world_present"] == value["world_owner_literals_verified"] == (label == "healthy")
            else:
                assert value["catalog_present"] == value["custody_catalog_decoded"] == (label == "healthy")
        else:
            assert result.returncode == 1 and not result.stdout and result.stderr == "native_restore_qualification_failed\n", (label, result)
        rows.append(dict(case=label, exit=result.returncode, authority_unchanged=True))
    (build / ("auction-boundaries.json" if auction else "player-boundaries.json" if player else "shop-boundaries.json" if shop else "locker-boundaries.json" if locker else "world-boundaries.json" if world else "boundaries.json")).write_text(json.dumps(rows, indent=2) + "\n")
    return rows


def player_item(uid, *, equipment=0, **kwargs):
    raw = bytearray(world_item(uid, **kwargs))
    struct.pack_into("<h", raw, 4, equipment)
    return bytes(raw)


def player_model(version=7):
    wire = version - 13 if 20 <= version <= 32 else version
    schema = (7 if wire % 2 else 8) if wire <= 8 else wire
    death = schema in (8, 10, 13, 14, 15, 16, 18, 19)
    value = dict(version=version, pid=7, revision=1, components=16383, intent=4 if death else 0,
        bound=1, integers=[(32, -1, 5, 1), (62, -2**63, 2**64-1, 0)],
        strings=[(0, b"private-synthetic-literal")], conditions=[-1]*5, quest_values=[-2]*14,
        indexed=[[(1, -2**63, 2**64-1)]]*5, commands=[-1], skills=[(1, 255, 255)],
        affects=[dict(ward_type=1)],
        items=[] if death else [player_item(100, kind=1), player_item(101, parent=0),
            player_item(102, kind=1, equipment=1), player_item(103)],
        pets=[] if death else [dict(uid=51, hold=0, restore=b"private-synthetic-literal",
            items=[player_item(200, kind=1), player_item(201, parent=0)])],
        shapes=[(1, -1, -2**63, 2**63-1)], trophies=[(1, -1)], external=1,
        output=b"private-synthetic-literal", quest=[(bytes.fromhex("a1"*16), 0, 1)],
        spell=[(bytes.fromhex("b2"*16), 1)], craft=[(bytes.fromhex("c3"*16), 1, 1)])
    if death:
        corpse = bytearray(player_item(900, kind=24, vnum=2))
        for index, number in enumerate((0, 1, 0, 7, 0, 0, 1, 0)):
            struct.pack_into("<i", corpse, 44+4*index, number)
        value["death"] = dict(id=bytes.fromhex("d4"*16), room=3001, revision=1,
            money=[1, 2, 3, 4], pile=901, items=[bytes(corpse), player_item(901, parent=0, vnum=3)],
            custody=[(901, 900, 900, 1, 3, 1, 1, 7, 0, 1)], pending=[bytes.fromhex("e5"*16)])
        columns = (("id", "pid", "obj_uid", "vnum", "container_id"),
            ("id", "item_id", "location", "modifier"),
            ("id", "item_id", "keyword", "description"),
            ("item_uid", "root_item_uid", "parent_item_uid", "item_revision", "vnum", "state", "owner_type", "owner_id", "owner_context_id"),
            ("owner_type", "owner_id", "owner_context_id", "revision"))
        value["evidence"] = [dict(columns=[c.encode() for c in names],
            rows=[[None, *([b"private-synthetic-literal"]*(len(names)-1))]]) for names in columns]
    return value


def player_frame(value):
    wire = value["version"] - 13 if 20 <= value["version"] <= 32 else value["version"]
    schema = (7 if wire % 2 else 8) if wire <= 8 else wire
    text = lambda data: struct.pack("<I", len(data)) + data
    vector = lambda rows, encode: struct.pack("<I", len(rows)) + b"".join(encode(row) for row in rows)
    items = lambda rows: vector(rows, lambda row: row)
    result = struct.pack("<IiQQiiQ", value["version"], value["pid"], value["revision"],
        value["components"], value["intent"], -1, value["bound"])
    result += vector(value["integers"], lambda row: struct.pack("<HqQB", *row))
    result += vector(value["strings"], lambda row: struct.pack("<B", row[0])+text(row[1]))
    result += struct.pack("<19i", *value["conditions"], *value["quest_values"])
    result += b"".join(vector(rows, lambda row: struct.pack("<iqQ", *row)) for rows in value["indexed"])
    result += vector(value["commands"], lambda row: struct.pack("<i", row))
    result += vector(value["skills"], lambda row: struct.pack("<iBB", *row))
    def affect(row):
        data = struct.pack("<hiIiBH5Q", -1, -1, 2**32-1, -1, 255, 65535, *([2**64-1]*5))
        if 20 <= value["version"] <= 32:
            data += struct.pack("<QiqqiBBB", 2**64-1, -1, -2**63, 2**63-1, -1, row["ward_type"], 255, 255)
        return data+text(b"private-synthetic-literal")+text(b"")
    result += vector(value["affects"], affect)+items(value["items"])
    def pet(row):
        data = struct.pack("<Q", row["uid"]) if wire >= 7 else b""
        data += struct.pack("<10i", *([-1]*10))+items(row["items"])
        if wire >= 3:
            data += text(row["restore"])+struct.pack("<I", row["hold"])
        return data
    result += vector(value["pets"], pet)
    result += vector(value["shapes"], lambda row: struct.pack("<iiqq", *row))
    result += vector(value["trophies"], lambda row: struct.pack("<ii", *row))
    result += struct.pack("<B", value["external"])
    if wire >= 5:
        result += text(value["output"])
    if wire >= 17 or wire in (11, 12, 15, 16):
        result += vector(value["quest"], lambda row: struct.pack("<16sII", *row))
    if wire >= 17 or 12 <= wire <= 16:
        result += vector(value["spell"], lambda row: struct.pack("<16sI", *row))
    if wire >= 17:
        result += vector(value["craft"], lambda row: struct.pack("<16sII", *row))
    if schema in (8, 10, 13, 14, 15, 16, 18, 19):
        row = value["death"]
        result += struct.pack("<16siQ4iQ", row["id"], row["room"], row["revision"], *row["money"], row["pile"])
        result += items(row["items"])
        result += vector(row["custody"], lambda row: struct.pack("<4QiBB3Q", *row))
        result += vector(row["pending"], lambda row: row)
    if schema in (10, 14, 16, 19):
        for table in value["evidence"]:
            result += vector(table["columns"], text)
            result += vector(table["rows"], lambda row: b"".join(b"\0" if cell is None else b"\1"+text(cell) for cell in row))
    return b"DURPLYR\0"+struct.pack("<IIiQQ", 1, len(result), value["pid"], value["revision"], value["components"])+hashlib.sha256(result).digest()+result


def player_custody(value):
    wire = value["version"]-13 if 20 <= value["version"] <= 32 else value["version"]
    owners, items = set(), []
    forests = [(value["items"], (1, value["pid"], 0))]
    forests += [(pet["items"], (11, pet["uid"], value["pid"]) if wire >= 7 and pet["uid"] else (1, value["pid"], 0)) for pet in value["pets"]]
    for rows, owner in forests:
        owners.add((*owner, 0))
        for i, raw in enumerate(rows):
            uid, = struct.unpack_from("<Q", raw, 6)
            parent, = struct.unpack_from("<i", raw)
            if raw[26] == 20 and parent == -1:
                continue
            root = i
            while struct.unpack_from("<i", rows[root])[0] >= 0:
                root = struct.unpack_from("<i", rows[root])[0]
            items.append(dict(uid=uid, root=struct.unpack_from("<Q", rows[root], 6)[0],
                parent=0 if parent < 0 else struct.unpack_from("<Q", rows[parent], 6)[0], owner=owner,
                revision=1, vnum=struct.unpack_from("<i", raw, 22)[0], state=1,
                equipment=max(0, struct.unpack_from("<h", raw, 4)[0]),
                payload=struct.pack("<Ii", 1, -1)+raw[4:] if raw[26] == 20 else b""))
    return dict(version=8, revision=1, owners=sorted(owners), items=sorted(items, key=lambda row: row["uid"]), operations=[])


def player_cases():
    cases = [("version-"+str(version), player_frame(player_model(version)), True, player_model(version))
             for version in (*range(1, 9), *range(10, 22), *range(23, 33))]
    def add(label, version, change, expected=False):
        value = player_model(version); change(value)
        cases.append((label, player_frame(value), expected, value))
    for key, bad in (("revision", 0), ("components", 0), ("components", 16384), ("pid", 0), ("pid", 8), ("bound", 0), ("bound", 4194305), ("external", 2)):
        add(key+"-"+str(bad), 7, lambda v, key=key, bad=bad: v.update({key:bad}))
    for key, rows in (("integers", [(63, 0, 0, 0)]), ("integers", [(0, 0, 0, 2)]),
            ("strings", [(7, b"")]), ("strings", [(0, b"x"*4097)]),
            ("output", b"x"*513), ("commands", [0]*8192)):
        add("bad-"+key+"-"+str(len(cases)), 7, lambda v, key=key, rows=rows: v.update({key:rows}))
    add("pet-string-limit", 7, lambda v: v["pets"][0].update(restore=b"x"*32769))
    add("pet-hold-full-width", 7, lambda v: v["pets"][0].update(hold=2**32-1), True)
    add("global-object-limit", 7, lambda v: v.update(items=[player_item(i+1000, kind=1) for i in range(4096)]))
    add("global-object-exact-limit", 7, lambda v: v.update(items=[player_item(i+1000, kind=1) for i in range(4094)]), True)
    add("global-row-limit", 7, lambda v: v.update(commands=[0]*(8192-5)))
    add("global-row-exact-limit", 7, lambda v: v.update(commands=[0]*(8192-19)), True)
    for key, version, rows in (("quest", 11, []), ("quest", 11, [(bytes(16), 0, 1)]),
            ("quest", 11, [(b"a"*16, 64, 1)]), ("quest", 11, [(b"a"*16, 0, 0)]),
            ("quest", 11, [(b"a"*16, 0, 1)]*2), ("spell", 12, []),
            ("spell", 12, [(bytes(16), 1)]), ("spell", 12, [(b"b"*16, 7)]),
            ("spell", 12, [(b"b"*16, 1)]*2), ("craft", 17, []),
            ("craft", 17, [(bytes(16), 1, 1)]), ("craft", 17, [(b"c"*16, 7, 1)]),
            ("craft", 17, [(b"c"*16, 3, 1)]), ("craft", 17, [(b"c"*16, 1, 2**31)]),
            ("craft", 17, [(b"c"*16, 1, 1)]*2)):
        add("receipt-"+key+"-"+str(len(cases)), version, lambda v, key=key, rows=rows: v.update({key:rows}))
    add("optional-quest-spell", 17, lambda v: v.update(quest=[], spell=[]), True)
    for key, bad in (("id", bytes(16)), ("room", 0), ("revision", 0), ("money", [-1, 0, 0, 0]),
            ("pile", 0), ("pending", [bytes.fromhex("d4"*16)]), ("pending", [bytes(16)]),
            ("custody", []), ("pending", [b"e"*16]*2)):
        add("death-"+key+"-"+str(len(cases)), 10, lambda v, key=key, bad=bad: v["death"].update({key:bad}))
    add("death-intent", 10, lambda v: v.update(intent=0))
    add("death-active-inventory", 10, lambda v: v.update(items=[player_item(100, kind=1)]))
    add("death-unheld-pet", 10, lambda v: v.update(pets=[dict(uid=51, hold=0, restore=b"", items=[])]))
    for column in (b"", b"bad-name", b"x"*65, b"id"):
        add("evidence-column-"+str(len(cases)), 10, lambda v, column=column: v["evidence"][0]["columns"].__setitem__(1, column))
    add("evidence-zero-rows", 10, lambda v: [t.update(rows=[]) for t in v["evidence"]])
    add("evidence-global-row-limit", 10, lambda v: v["evidence"][0].update(rows=[[None]*5]*8192))
    golden = player_frame(player_model())
    for label, encoded in (("bad-magic", b"X"+golden[1:]), ("bad-checksum", golden[:36]+bytes(32)+golden[68:]),
            ("truncated-header", golden[:48]), ("trailing-file", golden+b"x"),
            ("empty-payload", golden[:68]), ("truncated-body", golden[:-1])):
        cases.append((label, encoded, False, None))
    for version in (0, 9, 22, 33, 2**32-1):
        body = bytearray(golden[68:]);struct.pack_into("<I", body, 0, version)
        cases.append(("unsupported-"+str(version), golden[:36]+hashlib.sha256(body).digest()+body, False, None))
    return cases


def check_player_catalogs(fixture, binary, build):
    rows = []
    for label, encoded, expected, value in player_cases():
        directory=build/("player-format-"+label);directory.mkdir(mode=0o700);root=directory/"state"
        incoming=directory/"player.bin";incoming.write_bytes(encoded)
        result=subprocess.run([str(fixture),str(root),str(incoming),"1" if expected else "0","player"],capture_output=True,text=True,timeout=30)
        assert result.returncode==0 and not result.stderr,(label,result)
        native=json.loads(result.stdout);assert native["native_accepted"]==native["independent_accepted"]==expected
        if expected:
            incoming=directory/"custody.bin";incoming.write_bytes(frame(player_custody(value)))
            result=subprocess.run([str(fixture),str(root),str(incoming),"1"],capture_output=True,text=True,timeout=30)
            assert result.returncode==0 and not result.stderr,(label,result)
        before=inventory(root)
        command=[str(binary),"--economic-player-custody-audit",str(root)]
        result=subprocess.run(command,capture_output=True,text=True,timeout=30)
        assert inventory(root)==before
        if expected:
            report=json.loads(result.stdout)
            assert result.returncode==0 and not result.stderr and report["player_pet_owner_literals_verified"],(label,result)
            assert report["player_items"]+report["pet_items"]==native["items"]
            assert report["compared_items"]==len(player_custody(value)["items"])
            assert not report["native_holdings_compared"] and not report["release_qualified"]
        else:
            assert result.returncode==1 and not result.stdout and result.stderr=="native_restore_qualification_failed\n",(label,result)
        assert "private-synthetic-literal" not in result.stdout
        generated=root/"native-generated-player.bin"
        row=dict(case=label,accepted=expected,native=native,input_sha256=hashlib.sha256(encoded).hexdigest(),
            native_generated_sha256=hashlib.sha256(generated.read_bytes()).hexdigest() if expected else None,
            command=command,exit=result.returncode,stdout=result.stdout,stderr=result.stderr,authority_unchanged=True)
        rows.append(row);print("PLAYER_CATALOG "+json.dumps(row,sort_keys=True),flush=True)
        (directory/"evidence.json").write_text(json.dumps(row,indent=2)+"\n")
        (directory/"authority-before-after.json").write_text(json.dumps(before,sort_keys=True)+"\n")
    (build/"player-observations.json").write_text(json.dumps(rows,indent=2)+"\n")
    return rows


def check_player_findings(fixture, binary, build):
    base=player_model();owned=player_custody(base);cases=[("healthy",base,owned,{})]
    def add(label,value,custody,counts):cases.append((label,value,custody,counts))
    for state in (2,3):
        custody=copy.deepcopy(owned);custody["items"][1]["state"]=state
        add("state-"+str(state),base,custody,{"player_uid_not_active":1})
    for label, owner in (("owner",(1,8,0)),("pet-context",(11,51,8))):
        custody=copy.deepcopy(owned);custody["items"][-1]["owner"]=owner
        custody["owners"]=sorted({*custody["owners"],(*owner,0)})
        add(label,base,custody,{"player_owner_mismatch":1})
    for field,replacement in (("root",999),("parent",0)):
        custody=copy.deepcopy(owned);custody["items"][1][field]=replacement
        add(field,base,custody,{"player_item_topology_mismatch":1})
    custody=copy.deepcopy(owned);custody["items"][0]["vnum"]+=1
    add("vnum",base,custody,{"player_item_vnum_mismatch":1})
    custody=copy.deepcopy(owned);custody["items"][2]["equipment"]=2
    add("equipment",base,custody,{"player_item_equipment_mismatch":1})
    for label, fmt, offset, replacement in (("generated","q",14,-55),("mask","B",27,255),
            ("coin-value","i",44,9),("other-value","i",72,-9),("timer","q",100,-2**63),
            ("flag","I",140,2**32-1),("weight","i",144,-15),("material","b",148,-128),
            ("cost","i",149,33),("condition","h",153,-20),("craftsmanship","h",155,25),
            ("bitvector","Q",189,2**64-1),("affect","h",211,-32768),("type","B",26,1)):
        value=copy.deepcopy(base);raw=bytearray(value["pets"][0]["items"][1]);struct.pack_into("<"+fmt,raw,offset,replacement)
        value["pets"][0]["items"][1]=bytes(raw);add(label,value,owned,{"player_coin_literal_mismatch":1})
    for index in range(4):
        value=copy.deepcopy(base);strings=[b""]*4;strings[index]=b"private-synthetic-literal"
        value["items"][1]=player_item(101,parent=0,strings=strings)
        add("string-"+str(index),value,owned,{"player_coin_literal_mismatch":1})
    for label,kwargs in (("dynamic",dict(dynamic_count=1)),("description",dict(spell_counts=(1,)))):
        value=copy.deepcopy(base);value["items"][1]=player_item(101,parent=0,**kwargs)
        add(label,value,owned,{"player_coin_literal_mismatch":1})
    value=copy.deepcopy(base);value["pets"][0]["items"].pop()
    add("missing-literal",value,owned,{"player_uid_missing_literal":1})
    value=copy.deepcopy(base);value["items"].extend(player_item(1000+i,kind=1) for i in range(101))
    add("bounded-details",value,owned,{"player_uid_unadmitted":101})
    value=copy.deepcopy(base);raw=bytearray(value["items"][1]);struct.pack_into("<i",raw,44,-1);value["items"][1]=bytes(raw)
    add("negative-nested-coin",value,player_custody(value),{"player_negative_coin_value":1})
    value=copy.deepcopy(base);value["items"][3]=player_item(0,vnum=-1)
    add("wallet-root-excluded",value,owned,{})
    value=copy.deepcopy(base);value["items"].append(player_item(0,kind=1))
    add("zero-item-uid",value,owned,{"player_item_identity_invalid":1})
    value=copy.deepcopy(base);value["items"].append(player_item(100,kind=1))
    add("duplicate-item-uid",value,owned,{"player_uid_duplicate_literal":1})
    value=copy.deepcopy(base);value["pets"].append(dict(uid=51,hold=0,restore=b"",items=[]))
    add("duplicate-pet-uid",value,owned,{"player_pet_uid_duplicate":1})
    value=copy.deepcopy(base);value["pets"][0]["uid"]=0
    add("legacy-pet-under-player",value,player_custody(value),{})
    custody=copy.deepcopy(owned)
    for row in custody["items"]:row["payload"]=b""
    add("legacy-inline-absent",base,custody,{})
    custody=copy.deepcopy(owned);custody["owners"].insert(0,(1,8,0,0));custody["owners"].sort()
    custody["items"].append(dict(uid=300,root=300,parent=0,owner=(1,8,0),revision=1,vnum=57,state=1,payload=b"",equipment=0))
    add("orphan-other-player",base,custody,{"player_uid_missing_literal":1})
    for version in range(1,5):
        custody=copy.deepcopy(owned);custody["version"]=version
        add("legacy-custody-"+str(version),base,custody,{})
    add("players-absent",None,owned,{"custody_player_directory_missing":1,"player_uid_missing_literal":5})
    add("custody-absent",base,None,{"player_custody_catalog_missing":1,"player_uid_unadmitted":5})
    rows=[]
    for label,value,custody,counts in cases:
        directory=build/("player-finding-"+label);directory.mkdir(mode=0o700);root=directory/"state"
        for data,family,encoder in ((custody,None,frame),(value,"player",player_frame)):
            if data is None:continue
            path=directory/("player.bin" if family else "custody.bin");path.write_bytes(encoder(data))
            command=[str(fixture),str(root),str(path),"1"]+([family] if family else [])
            result=subprocess.run(command,capture_output=True,text=True,timeout=30)
            assert result.returncode==0 and not result.stderr,(label,result)
        before=inventory(root);reports=[]
        for limit in (0,1,100):
            command=[str(binary),"--economic-player-custody-audit",str(root),"--limit",str(limit)]
            result=subprocess.run(command,capture_output=True,text=True,timeout=30)
            assert result.returncode==bool(counts) and not result.stderr and inventory(root)==before,(label,limit,result)
            report=json.loads(result.stdout)
            assert report["finding_counts"]==counts and report["finding_count"]==sum(counts.values()),(label,report)
            assert len(report["findings"])==min(limit,report["finding_count"])
            assert report["findings_truncated"]==(report["finding_count"]>limit) and report["player_pet_owner_literals_verified"]==(not counts)
            if label.startswith("legacy-custody-"):
                assert report["custody_equipment_fields_absent"]==5
                assert report["coin_payloads_absent"]==(2 if custody["version"]<3 else 0)
            if label=="legacy-inline-absent":
                assert report["coin_payloads_absent"]==2 and report["coin_payloads_compared"]==0
            assert not any(report[name] for name in ("other_owner_literals_compared","native_holdings_compared","death_history_compared","item_history_verified","full_R7_qualified","release_qualified"))
            assert "private-synthetic-literal" not in result.stdout
            reports.append(dict(command=command,exit=result.returncode,report=report))
        row=dict(case=label,counts=counts,reports=reports,authority_unchanged=True);rows.append(row)
        (directory/"evidence.json").write_text(json.dumps(row,indent=2)+"\n")
        (directory/"authority-before-after.json").write_text(json.dumps(before,sort_keys=True)+"\n")
        print("PLAYER_FINDING "+json.dumps(dict(case=label,counts=counts,cuts=3,authority_unchanged=True)),flush=True)
    (build/"player-findings.json").write_text(json.dumps(rows,indent=2)+"\n")
    return rows


def auction_model(version=2):
    def listing(id, seller, winner, status, roots):
        return dict(id=id, seller=seller, winner=winner, status=status, revision=1,
                    blob=b"opaque-native-template", items=roots)
    return dict(version=version, revision=1, listings=[
        listing(1, 11, 11, 2, [(100, 1, 10, 11, 1)]),
        listing(7, 11, 0, 1, [(100, 2, 10, 0, 0), (103, 4, 11, 0, 0)]),
        listing(8, 12, 13, 2, [(101, 2, 12, 13, 0)]),
        listing(9, 14, 15, 3, [(102, 2, 13, 14, 1)])])


def auction_frame(value):
    def text(data):return struct.pack("<I", len(data))+data
    body=struct.pack("<I",len(value["listings"]))
    for row in value["listings"]:
        body+=struct.pack("<4I2q2Q",row["id"],row["seller"],row["winner"],row["status"],0,0,row["revision"],0)
        body+=text(b"native-fixture")+b"".join(text(b"private-synthetic-literal") for _ in range(5))
        body+=struct.pack("<I",len(row["blob"]))+row["blob"]+struct.pack("<H",len(row["items"]))
        body+=b"".join(struct.pack("<QQIIB",*item) for item in row["items"])
    body+=struct.pack("<II",0,0)
    return b"DURAUCT\0"+struct.pack("<IIQ",value["version"],len(body),value["revision"])+hashlib.sha256(body).digest()+body


def auction_custody(value):
    result=model();result["items"]=[];result["operations"]=[]
    seen=set()
    for listing in value["listings"]:
        for uid,revision,vnum,claim_pid,claimed in listing["items"]:
            if claimed or uid in seen:continue
            seen.add(uid)
            result["items"].append(dict(uid=uid,root=uid,parent=0,owner=(6,listing["id"],0),
                revision=revision,vnum=vnum,state=1,payload=b"",equipment=0))
    # Claimed history may refer to a retired item or an item since moved elsewhere.
    if 102 not in seen:
        result["items"].append(dict(uid=102,root=102,parent=0,owner=(1,14,0),revision=5,vnum=13,state=1,payload=b"",equipment=0))
    result["items"].sort(key=lambda row:row["uid"])
    result["owners"]=sorted({(*row["owner"],0) for row in result["items"]})
    return result


def check_auction(fixture,binary,native,pure,build):
    cases=[];base=auction_model();owned=auction_custody(base)
    def add(label,value=base,custody=owned,counts=None,verified=True,refusal=False):
        cases.append((label,copy.deepcopy(value),copy.deepcopy(custody),counts or {},verified,refusal))
    add("healthy-current-and-relisted-history")
    value=auction_model();value["listings"][2]["winner"]=0
    value["listings"][2]["items"]=[(101,2,12,12,0)]
    add("unbid-closed-claim-belongs-to-seller",value)
    value=auction_model();value["listings"][3]["items"]=[(102,2,13,14,0)]
    add("removed-unclaimed-root-belongs-to-auction",value,auction_custody(value))
    value=auction_model();value["revision"]=2**64-1
    value["listings"]=[dict(id=2**32-1,seller=2**32-1,winner=0,status=1,revision=2**64-1,
        blob=b"opaque-native-template",items=[(2**64-9+i,2**64-1,2**31-1,0,0) for i in range(9)])]
    add("full-width-identities-and-nine-roots",value,auction_custody(value))
    value=auction_model();value["listings"]=[]
    for i in range(12):
        value["listings"].append(dict(id=i+1,seller=11,winner=0,status=1,revision=1,
            blob=b"opaque-native-template",items=[(1000+i*9+j,1,10,0,0) for j in range(9)]))
    custody=auction_custody(value);custody["items"]=[]
    add("bounded-details-exact-total",value,custody,counts={"auction_uid_unadmitted":108},verified=False)
    value=auction_model(1);add("legacy-auction-format",value)
    for version in range(1,5):
        custody=copy.deepcopy(owned);custody["version"]=version
        add("legacy-equipment-absent-"+str(version),custody=custody,verified=False)
    custody=copy.deepcopy(owned);custody["items"][-1]["payload"]=item_payload(103,11)
    add("retained-coin-payload-is-explicitly-uncompared",custody=custody)
    custody=copy.deepcopy(owned);custody["items"][2].update(owner=(8,0,0),state=2)
    add("claimed-tombstone-is-history",custody=custody)
    for label,field,value in (("wrong-owner","owner",(6,8,0)),("wrong-context","owner",(6,7,99)),
        ("wrong-revision","revision",3),("wrong-vnum","vnum",99),
        ("wrong-root","root",103),("wrong-parent","parent",103),
        ("tombstoned-unclaimed","state",2),("quarantined-unclaimed","state",3)):
        custody=copy.deepcopy(owned);custody["items"][0][field]=value
        code={"owner":"auction_owner_mismatch","revision":"auction_item_revision_mismatch",
              "vnum":"auction_item_vnum_mismatch","root":"auction_item_topology_mismatch",
              "parent":"auction_item_topology_mismatch","state":"auction_uid_not_active"}[field]
        add(label,custody=custody,counts={code:1},verified=False)
    custody=copy.deepcopy(owned);custody["items"][0].update(owner=(1,11,0),equipment=1)
    add("player-equipment-cannot-grant-auction-root",custody=custody,
        counts={"auction_owner_mismatch":1,"auction_item_equipment_mismatch":1},verified=False)
    custody=copy.deepcopy(owned);custody["items"].pop(0)
    add("missing-unclaimed-uid",custody=custody,counts={"auction_uid_unadmitted":1,"auction_claimed_uid_unadmitted":1},verified=False)
    custody=copy.deepcopy(owned);custody["items"].pop(2)
    add("missing-claimed-uid",custody=custody,counts={"auction_claimed_uid_unadmitted":1},verified=False)
    value=copy.deepcopy(base);value["listings"][1]["items"].pop()
    add("extra-custody-root",value,counts={"auction_uid_missing_unclaimed_root":1},verified=False)
    value=copy.deepcopy(base);value["listings"][2]["items"]=[(100,2,10,13,0)]
    add("duplicate-unclaimed-across-listings",value,counts={"auction_unclaimed_uid_duplicate":1,
        "auction_owner_mismatch":1,"auction_uid_missing_unclaimed_root":1},verified=False)
    for label,index,claim,claimed in (("open-claim-right",1,11,0),("open-claimed",1,0,1),
        ("closed-wrong-claimant",2,12,0),("closed-no-claimant",2,0,0),("removed-winner-claimant",3,15,1)):
        value=copy.deepcopy(base);item=list(value["listings"][index]["items"][0]);item[3:]=[claim,claimed]
        value["listings"][index]["items"][0]=tuple(item)
        counts={"auction_claim_state_invalid":1}
        if label=="open-claimed":counts["auction_uid_missing_unclaimed_root"]=1
        add(label,value,counts=counts,verified=False)
    value=copy.deepcopy(base);value["listings"]=[]
    add("empty-auction-with-current-custody",value,counts={"auction_uid_missing_unclaimed_root":3},verified=False)
    add("auction-absent",None,counts={"custody_auction_catalog_missing":1,"auction_uid_missing_unclaimed_root":3},verified=False)
    add("custody-absent",custody=None,counts={"auction_custody_catalog_missing":1,"auction_uid_unadmitted":3,"auction_claimed_uid_unadmitted":2},verified=False)
    for label,mutate in (("bad-checksum",lambda data:data[:24]+bytes(32)+data[56:]),
        ("future-version",lambda data:data[:8]+struct.pack("<I",3)+data[12:]),
        ("truncated",lambda data:data[:-1])):
        add(label,mutate(auction_frame(base)),verified=False,refusal=True)
    rows=[]
    for label,value,custody,counts,verified,refusal in cases:
        directory=build/("auction-finding-"+label);directory.mkdir(mode=0o700);root=directory/"state";root.mkdir(mode=0o700)
        seed=subprocess.run([str(native),"seed",str(root)],capture_output=True,text=True,timeout=30)
        assert seed.returncode==0 and not seed.stderr,(label,seed)
        catalog=root/"domains/auction_catalog"
        if value is None:catalog.unlink()
        else:catalog.write_bytes(value if isinstance(value,bytes) else auction_frame(value))
        incoming=directory/"custody.bin";incoming.write_bytes(frame(custody) if custody is not None else b"")
        if custody is not None:
            custody["owners"]=sorted({(*row["owner"],0) for row in custody["items"]})
            incoming.write_bytes(frame(custody))
            setup=subprocess.run([str(fixture),str(root),str(incoming),"1"],capture_output=True,text=True,timeout=30)
            assert setup.returncode==0 and not setup.stderr,(label,setup)
        before=inventory(root);decoded=None
        if value is not None:
            left=subprocess.run([str(native),"probe",str(root)],capture_output=True,text=True,timeout=30)
            right=subprocess.run([str(pure),"decode",str(root)],capture_output=True,text=True,timeout=30)
            assert inventory(root)==before and left.returncode==right.returncode==int(refusal),(label,left,right)
            if not refusal:
                decoded=json.loads(left.stdout);assert decoded==json.loads(right.stdout),(label,left,right)
                expected_roots=[[row["id"],row["seller"],row["winner"],row["status"],row["revision"],*item]
                    for row in value["listings"] for item in row["items"]]
                assert decoded["item_roots"]==expected_roots
                assert decoded["object_blob_sha256"]==[hashlib.sha256(row["blob"]).hexdigest() for row in value["listings"]]
        reports=[]
        for limit in (0,1,100):
            command=[str(binary),"--economic-auction-custody-audit",str(root),"--limit",str(limit)]
            result=subprocess.run(command,capture_output=True,text=True,timeout=30)
            assert inventory(root)==before and result.returncode==int(refusal or bool(counts)),(label,limit,result)
            if refusal:
                assert not result.stdout and result.stderr=="native_restore_qualification_failed\n"
                reports.append(dict(command=command,exit=result.returncode));continue
            assert not result.stderr,(label,result)
            report=json.loads(result.stdout)
            assert report["finding_counts"]==counts and report["finding_count"]==sum(counts.values()),(label,report)
            assert report["auction_root_metadata_verified"]==verified and len(report["findings"])==min(limit,sum(counts.values()))
            assert report["findings_truncated"]==(sum(counts.values())>limit)
            assert not any(report[key] for key in ("serialized_templates_compared","coin_literals_compared","native_holdings_compared","item_history_verified","full_R7_qualified","release_qualified"))
            assert "private-synthetic-literal" not in result.stdout
            reports.append(dict(command=command,exit=result.returncode,report=report))
        if not refusal:
            checked=subprocess.run([str(fixture),str(root),str(incoming),"0" if counts else "1","auction-audit"],capture_output=True,text=True,timeout=30)
            assert checked.returncode==0 and not checked.stderr and inventory(root)==before,(label,checked)
            sanitized=json.loads(checked.stdout)
            assert sanitized==dict(finding_count=sum(counts.values()),compared_roots=report["compared_roots"],verified=verified)
        row=dict(case=label,counts=counts,native_fields=decoded,reports=reports,authority_unchanged=True);rows.append(row)
        (directory/"evidence.json").write_text(json.dumps(row,indent=2)+"\n")
        (directory/"authority-before-after.json").write_text(json.dumps(before,sort_keys=True)+"\n")
        if label=="healthy-current-and-relisted-history":
            for mode in ("bytes","files","entries","deadline"):
                checked=subprocess.run([str(fixture),str(root),str(incoming),"1","auction-audit",mode],capture_output=True,text=True,timeout=30)
                assert inventory(root)==before
                if mode=="entries":
                    # This fixed-file audit performs no directory enumeration.
                    assert checked.returncode==0 and not checked.stderr and json.loads(checked.stdout)["verified"],checked
                else:
                    assert checked.returncode==1 and not checked.stdout and checked.stderr=="native_restore_qualification_failed\n",(mode,checked)
        print("AUCTION_CUSTODY "+json.dumps(dict(case=label,counts=counts,cuts=3,authority_unchanged=True)),flush=True)
    (build/"auction-findings.json").write_text(json.dumps(rows,indent=2)+"\n")
    return rows


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True,
                        help="exact integrated native checkout supporting quest continuation v6")
    arguments = parser.parse_args()
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-custody-audit-", dir=ROOT / "bin/tests") as temporary:
        build = Path(temporary)
        fixture = build_fixture(build / "fixture", arguments.native_source.absolute())
        binary = qualifier.build(build / "qualify")
        observations = check_catalogs(fixture, binary, build)
        boundaries = check_boundaries(fixture, binary, build)
        world_observations = check_world_catalogs(fixture, binary, build)
        world_findings = check_world_findings(fixture, binary, build)
        world_boundaries = check_boundaries(fixture, binary, build, world=True)
        locker_observations = check_locker_catalogs(fixture, binary, build)
        locker_findings = check_locker_findings(fixture, binary, build)
        locker_boundaries = check_boundaries(fixture, binary, build, locker=True)
        shop_observations = check_shop_catalogs(fixture, binary, build)
        shop_findings = check_shop_findings(fixture, binary, build)
        shop_boundaries = check_boundaries(fixture, binary, build, shop=True)
        player_observations = check_player_catalogs(fixture, binary, build)
        player_findings = check_player_findings(fixture, binary, build)
        player_boundaries = check_boundaries(fixture, binary, build, player=True)
        auction_native = auctions.build_native_oracle(arguments.native_source.absolute(), build)
        auction_pure = auctions.build_independent(build)
        auction_findings = check_auction(fixture, binary, auction_native, auction_pure, build)
        auction_boundaries = check_boundaries(fixture, binary, build, auction=True, auction_native=auction_native)
        print("auction custody: " + str(len(auction_findings)) + " findings and " + str(len(auction_boundaries)) + " boundaries passed")
        print("player/pet custody: " + str(len(player_observations)) + " format cases, " + str(len(player_findings)) + " findings, " + str(len(player_boundaries)) + " boundaries passed")
        print("shopkeeper custody: " + str(len(shop_observations)) + " format cases, " + str(len(shop_findings)) + " findings, " + str(len(shop_boundaries)) + " boundaries passed")
        print("locker custody: " + str(len(locker_observations)) + " format cases, " + str(len(locker_findings)) + " findings, " + str(len(locker_boundaries)) + " boundaries passed")
        print("independent custody catalog: " + str(len(observations)) + " native/operator cases passed")
        print("world custody: " + str(len(world_observations)) + " format cases, " +
              str(len(world_findings)) + " finding cases and " + str(len(world_boundaries)) + " boundaries passed")
