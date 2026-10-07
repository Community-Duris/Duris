#!/usr/bin/env python3
"""Independent published lifecycle V2 wire contract, with native EAB round trips.

This does not execute the private native V2 lifecycle encoder or installer.
The original lifecycle reader suite remains the historical V1 acceptance check.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LINEAGE = bytes([1]) + bytes(15)
BASE = bytes([0x31]) * 32


def key(kind, uid, context=0):
    return LINEAGE + struct.pack("<HHQQ4x", 1, kind, uid, context)


def witness(piles, *, mapped=True, eab=2):
    holdings = [(key(1, 11), (4, 3, 2, 1), 100, bytes([11])*32),
                (key(2, 22, 1), (1, 2, 3, 4), 102, bytes([12])*32)] if mapped else []
    holdings += [(key(3, uid), balance, revision, source) for uid, room, revision, balance, source in piles]
    size = 192 + 112*len(holdings) + (88 if eab == 1 else 96)*len(piles)
    out = b"EAB" + str(eab).encode() + struct.pack("<HHII", eab, 192, size, 0)
    out += LINEAGE + bytes([2])+bytes(15) + bytes([3])+bytes(15)
    out += struct.pack("<QQ", 7, 0) + key(9, 9001) + bytes([11])*32 + bytes([12])*32
    out += struct.pack("<II", len(holdings), len(piles))
    assert len(out) == 192
    for account, balance, revision, source in holdings:
        out += account + struct.pack("<4QQ", *balance, revision) + source
    for uid, room, revision, balance, source in piles:
        out += struct.pack("<QBB6x5Q", uid, 3, 1, room, 0, uid, 0, revision)
        if eab == 2:
            out += bytes(8)
        out += source
    assert len(out) == size
    return out, b"".join(account for account, *_ in holdings[:2]) if mapped else b""


def reference(piles, version):
    if version == 1:
        return BASE.hex()
    data = b"DURIS-FLATFILE-COVERAGE-V2" + BASE + struct.pack("<Q", len(piles))
    for uid, room, revision, balance, source in piles:
        data += struct.pack("<7Q", uid, room, revision, *balance) + source
    return hashlib.sha256(data).hexdigest()


def cases():
    pile = (333, 44, 7, (1, 2, 3, 4), bytes([0x51])*32)
    rows = []
    for label, piles, version, mapped, eab in (
        ("historical-v1", [], 1, True, 2), ("historical-eab1", [], 1, True, 1),
        ("v2-empty", [], 2, False, 2), ("v2-wallet-bank", [], 2, True, 2),
        ("v2-room-pile", [pile], 2, True, 2), ("v2-room-pile-eab1", [pile], 2, True, 1),
        ("v2-pile-only", [pile], 2, False, 2),
        ("v2-two-sorted", [pile, (444, 45, 9, (9, 8, 7, 6), bytes([0x52])*32)], 2, True, 2),
        ("native-revision-independent", [(333, 44, 100, pile[3], pile[4])], 2, True, 2),
        ("full-width-uid-revision", [(2**64-1, 2**31-1, 2**64-1, (2**31-1,)*4, pile[4])], 2, False, 2),
        ("zero-denominations-parser-only", [(333, 44, 1, (0,)*4, pile[4])], 2, False, 2),
        ("maximum-paired-holdings", [(10000+i, 44, 7, (0,)*4, pile[4]) for i in range(3071)], 2, False, 2)):
        value, keys = witness(piles, mapped=mapped, eab=eab)
        rows.append(dict(case=label, value=value, keys=keys, version=version, accepted=True, coverage=reference(piles, version)))
    value, keys = witness([pile])
    holding = 192+2*112
    item = 192+3*112

    def cut(label, offset, data):
        changed = bytearray(value)
        changed[offset:offset+len(data)] = data
        rows.append(dict(case=label, value=bytes(changed), keys=keys, version=2, accepted=False))

    for label, offset, number, width in (
        ("holding-kind", holding+18, 4, 2), ("holding-uid", holding+20, 334, 8),
        ("holding-context", holding+28, 1, 8), ("holding-revision", holding+72, 8, 8),
        ("holding-revision-zero", holding+72, 0, 8),
        ("uid-zero", item, 0, 8), ("owner-player", item+8, 1, 1),
        ("state-destroyed", item+9, 2, 1), ("state-held", item+9, 3, 1),
        ("room-zero", item+16, 0, 8), ("room-over-int32", item+16, 2**31, 8),
        ("item-context", item+24, 1, 8), ("wrong-root", item+32, 334, 8),
        ("parent", item+40, 334, 8), ("item-revision-zero", item+48, 0, 8),
        ("item-revision-mismatch", item+48, 8, 8), ("equipment", item+56, 1, 2),
        ("item-reserved", item+10, 1, 1), ("equipment-reserved", item+58, 1, 1),
        ("account-reserved", holding+36, 1, 1), ("account-version", holding+16, 2, 2),
        ("holding-count-extra", 184, 4, 4), ("missing-item", 188, 0, 4),
        ("item-count-extra", 188, 2, 4), ("items-over-limit", 188, 6001, 4),
        ("holdings-over-limit", 184, 3072, 4), ("witness-version", 4, 3, 2),
        ("witness-header", 6, 191, 2), ("witness-reserved", 12, 1, 4)):
        cut(label, offset, number.to_bytes(width, "little"))
    for part in range(4):
        cut("denomination-over-"+str(part), holding+40+8*part, struct.pack("<Q", 2**31))
        cut("denomination-negative-"+str(part), holding+40+8*part, bytes([255])*8)
    for label, offset in (("holding-lineage", holding), ("holding-fingerprint", holding+80),
                          ("item-fingerprint", item+64)):
        cut(label, offset, bytes([0x99]))
    cut("zero-source-both-holding", holding+80, bytes(32))
    changed = bytearray(value);changed[holding+80:holding+112] = bytes(32);changed[item+64:item+96] = bytes(32)
    rows.append(dict(case="zero-source-both", value=bytes(changed), keys=keys, version=2, accepted=False))
    for label, piles in (("duplicate-uid", [pile, pile]), ("unsorted-uid", [pile, (332,44,7,pile[3],pile[4])])):
        value2, keys2 = witness(piles)
        rows.append(dict(case=label,value=value2,keys=keys2,version=2,accepted=False))
    for label, version in (("native-v0",0),("native-v3-is-not-catalogue",3),("native-v4",4),("v1-with-pile",1)):
        rows.append(dict(case=label,value=value,keys=keys,version=version,accepted=False))
    for length in (0, 16, 191, len(value)-1):
        rows.append(dict(case="truncated-"+str(length),value=value[:length],keys=keys,version=2,accepted=False))
    rows.append(dict(case="trailing-witness",value=value+b"x",keys=keys,version=2,accepted=False))
    assert len({row['case'] for row in rows}) == len(rows)
    return rows


def build_fixture(destination, native_source):
    from test_flatfile_accounting_store import SOURCES
    command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
               "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
               "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-D__NO_MYSQL__",
               "-I"+str(native_source/"src"), "-I"+str(native_source/"src/no_mysql"),
               str(ROOT/"tests/async/flatfile_lifecycle_v2_fixture.cpp")]
    command += [str(native_source/name) for name in ["src/economy/economic_baseline_codec.c",
                "src/economy/economic_baseline_adapter.c", *SOURCES[1:],
                "src/item/lockpick_retirement_continuation.c", "src/economy/native_quest_cost.c",
                "src/economy/native_quest_coin_give.c"]]
    command += ["-lcrypto", "-pthread", "-o", str(destination)]
    subprocess.run(command,check=True,cwd=ROOT,timeout=600)
    return destination


def check(binary, artifacts):
    from test_flatfile_restore_lifecycle_receipts import retained
    results = []
    for row in cases():
        folder = artifacts / row['case'];folder.mkdir(mode=0o700)
        value, keys = folder/"witness.eab", folder/"mapping-keys"
        value.write_bytes(row['value']);keys.write_bytes(row['keys'])
        before = retained(folder)
        ran = subprocess.run([str(binary),"coverage",str(value),str(row['version']),str(keys)],
                             capture_output=True,text=True,timeout=45,check=True)
        assert not ran.stderr and retained(folder) == before
        report = json.loads(ran.stdout)
        assert report['independent_accepted'] == row['accepted'], (row['case'],report)
        if row['accepted']:
            assert report['native_baseline_accepted'] and report['native_baseline_roundtrip'], (row['case'],report)
            assert report['coverage'] == row['coverage'], (row['case'],report)
        results.append(dict(case=row['case'],accepted=row['accepted'],report=report,
                            witness_sha256=hashlib.sha256(row['value']).hexdigest(),authority_unchanged=True))
    frames = []
    for version in (0,1,2,3,4):
        folder=artifacts/('envelope-'+str(version));folder.mkdir(mode=0o700)
        body=b"published-wire-contract-only"
        value=b"DURELR\0\0"+struct.pack("<II",version,len(body))+hashlib.sha256(body).digest()+body
        path=folder/"receipt.elr";path.write_bytes(value)
        before=retained(folder)
        ran=subprocess.run([str(binary),"frame",str(folder),str(version),path.name],capture_output=True,text=True,timeout=45)
        accepted=version in (1,2)
        assert ran.returncode == int(not accepted) and retained(folder) == before
        assert (ran.stdout,ran.stderr) == ((str(len(body))+"\n","") if accepted else ("","lifecycle_v2_contract_refused\n"))
        frames.append(dict(version=version,accepted=accepted,exit=ran.returncode,authority_unchanged=True))
    return dict(cases=results,envelopes=frames,skips=0,native_EAB_roundtrip=True,
                native_lifecycle_V2_encoder_executed=False,lifecycle_install_executed=False,
                current_world_recaptured=False,combined_candidate_qualified=False,release_complete=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source",required=True,type=Path)
    parser.add_argument("--artifacts",required=True,type=Path)
    args=parser.parse_args();os.umask(0o077)
    args.artifacts.mkdir(parents=True)
    binary=build_fixture(args.artifacts/"fixture",args.native_source)
    result=check(binary,args.artifacts)
    (args.artifacts/"evidence.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(dict(cases=len(result['cases']),accepted=sum(r['accepted'] for r in result['cases']),envelopes=len(result['envelopes']),skips=0)))


if __name__ == "__main__":
    main()
