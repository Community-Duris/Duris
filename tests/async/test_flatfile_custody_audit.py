#!/usr/bin/env python3
"""Native and independent durable custody decoding, without authority repairs."""
import ast
import argparse
import copy
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

UNITS = ("flatfile_item_repository", "flatfile_authority_transaction", "flatfile_store",
         "player_snapshot_codec", "item_transfer_command", "critical_command", "economic_source_event")


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


def item_payload(uid, vnum=402013):
    path = ROOT / "tests/async/run_economic_sql_audit_snapshot_mysql.py"
    function, = [node for node in ast.parse(path.read_text()).body
                 if isinstance(node, ast.FunctionDef) and node.name == "coin_payload"]
    namespace = {"struct": struct}
    exec(compile(ast.Module(body=[function], type_ignores=[]), str(path), "exec"), namespace)
    return namespace["coin_payload"](uid, [1, 2, 3, 4], vnum)


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


def check_boundaries(fixture, binary, build):
    import fcntl

    golden = build / "boundary-golden"
    incoming = build / "boundary-input.bin"
    incoming.write_bytes(frame(model()))
    native = subprocess.run([str(fixture), str(golden), str(incoming), "1"],
                            capture_output=True, text=True, timeout=30)
    assert native.returncode == 0 and not native.stderr, native
    rows = []
    for label in ("healthy", "empty", "uninitialized", "missing-lock", "held-lock",
                  "critical-journal", "currency-journal", "player-journal", "public-file",
                  "public-root", "symlink", "dangling-symlink", "hardlink"):
        root = build / ("boundary-" + label)
        shutil.copytree(golden, root, copy_function=shutil.copy2)
        domains = root / "domains"
        catalog = domains / "item_ownership"
        held = None
        if label in ("empty", "uninitialized"):
            catalog.unlink()
            if label == "uninitialized":
                shutil.rmtree(domains)
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
        before = inventory(root)
        result = subprocess.run([str(binary), "--economic-custody-catalog-audit", str(root)],
                                capture_output=True, text=True, timeout=30)
        assert inventory(root) == before
        if held is not None:
            held.close()
        expected = label in ("healthy", "empty", "uninitialized")
        if expected:
            assert result.returncode == 0 and not result.stderr, (label, result)
            value = json.loads(result.stdout)
            assert value["catalog_present"] == value["custody_catalog_decoded"] == (label == "healthy")
        else:
            assert result.returncode == 1 and not result.stdout and result.stderr == "native_restore_qualification_failed\n", (label, result)
        rows.append(dict(case=label, exit=result.returncode, authority_unchanged=True))
    (build / "boundaries.json").write_text(json.dumps(rows, indent=2) + "\n")
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
        print("independent custody catalog: " + str(len(observations)) + " native/operator cases passed")
