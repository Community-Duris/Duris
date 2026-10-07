#!/usr/bin/env python3
"""Independent native source-credit/consumer proof; never whole-release evidence."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import stat
import struct
import subprocess
import tempfile

from test_flatfile_restore_economic_authority import ROOT, build_fixture
from test_flatfile_restore_baseline_markers import audit_source_inputs, fingerprint, retained
from flatfile_auction_money_cases import build_native_oracle
import build_restore_qualifier as qualifier
from flatfile_money_history_cases import records, rewrite
from test_flatfile_restore_economic_authority import inventory, rehash


def build_independent(out):
    source = out / "independent.cpp"
    source.write_text(r'''#include "qualify_flatfile_auction_source_credit.h"
#include <iostream>
int main(int argc, char **argv) {
    try {
        using namespace restore_economic_authority;
        need(argc == 3);
        audit_budget budget;
        const std::string mode = argv[1];
        if (mode == "bytes") budget.remaining_bytes = 0;
        if (mode == "files") budget.remaining_files = 0;
        if (mode == "entries") budget.remaining_entries = 0;
        if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
        auto result = restore_auction_source_credit::audit(argv[2], budget);
        std::cout << result.valid() << " " << result.credits_verified() << " "
            << result.finding_count << "\n";
        return result.valid() ? 0 : 1;
    } catch (...) { std::cerr << "native_restore_qualification_failed\n"; return 1; }
}
''', encoding="utf-8")
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts")]
    binary = out / "independent"
    command = [*flags, str(source), "-lcrypto", "-o", str(binary)]
    subprocess.run(command, check=True, timeout=120)
    dependencies = subprocess.check_output([*flags, "-MM", str(source)], text=True)
    assert "/src/" not in dependencies
    (out / "independent.d").write_text(dependencies, encoding="utf-8")
    (out / "independent-build.json").write_text(json.dumps(command, indent=2) + "\n", encoding="utf-8")
    return binary


def frame(value, body=None):
    header = bytearray(value[:56])
    payload = value[56:] if body is None else body
    struct.pack_into("<I", header, 12, len(payload))
    header[24:56] = hashlib.sha256(payload).digest()
    return bytes(header) + payload


def green_checks(state, out, observations, fixture, operator, native_fixture, independent, environment):
    operation = bytes([6]) + bytes(15)
    evidence = state / "economic-evidence"
    catalog, source = state / "domains/auction_catalog", state / "domains/auction_claim_sources"

    def whole():
        info = state.lstat()
        entries = retained(state)
        for name, value in entries.items():
            if stat.S_ISLNK(value["mode"]):
                value["symlink_target"] = os.readlink(state / name)
        return dict(root_mode=info.st_mode, root_inode=info.st_ino,
            root_links=info.st_nlink, root_mtime_ns=info.st_mtime_ns, entries=entries)

    def cut(before):
        encoded = json.dumps(before, sort_keys=True, indent=2) + "\n"
        target = out / "inputs" / f"case-{len(observations):03}.json"
        target.parent.mkdir(mode=0o700, exist_ok=True)
        target.write_text(encoded, encoding="utf-8")
        objects = out / "inputs/objects"
        objects.mkdir(mode=0o700, exist_ok=True)
        for name, value in before["entries"].items():
            if stat.S_ISREG(value["mode"]):
                payload = (state / name).read_bytes()
                assert hashlib.sha256(payload).hexdigest() == value["sha256"]
                blob = objects / value["sha256"]
                if blob.exists():
                    assert blob.read_bytes() == payload
                else:
                    blob.write_bytes(payload)
        return dict(path=str(target.relative_to(out)), sha256=hashlib.sha256(encoded.encode()).hexdigest())

    def check(label, *, credits=False, codes=(), refusal=False, balance=None):
        before = whole()
        retained_cut = cut(before)
        audited = subprocess.run([str(operator), "--economic-auction-source-credit-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": operator changed input"
        pure = subprocess.run([str(independent), "normal", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": independent reader changed input"
        expected = int(refusal or bool(codes))
        assert audited.returncode == pure.returncode == expected, (label, audited, pure)
        report = None
        if refusal:
            assert not audited.stdout and not pure.stdout
            assert audited.stderr == pure.stderr == "native_restore_qualification_failed\n"
        else:
            assert not audited.stderr and not pure.stderr, (label, audited, pure)
            report = json.loads(audited.stdout)
            assert {row["code"] for row in report["findings"]} == set(codes), (label, report, codes)
            assert report["claim_source_credit_roots_verified"] == credits, (label, report)
            assert pure.stdout == f"{int(expected == 0)} {int(credits)} {report['finding_count']}\n"
            for flag in ("account_origins_verified", "claim_source_attribution_verified",
                "claim_source_digest_verified", "claim_source_consumption_order_verified",
                "cross_epoch_continuity_verified", "native_holdings_compared", "full_R7_qualified", "release_qualified"):
                assert not report[flag], (label, flag)
            assert not any(secret in audited.stdout for secret in ("native-fixture", "renamed", "Seller", "Buyer"))
        if balance is not None:
            prior = subprocess.run([str(operator), "--economic-auction-source-balance-audit", str(state)],
                env=environment, capture_output=True, text=True, timeout=30)
            assert whole() == before and not prior.stderr
            assert json.loads(prior.stdout)["remaining_claim_source_balances_verified"] == balance, (label, prior)
        observations.append(dict(label=label, exit=audited.returncode, report=report,
            independent_exit=pure.returncode, source_cut=retained_cut,
            complete_retained_inventory_unchanged=True))
        print("AUCTION_SOURCE_CREDIT " + label, flush=True)
        return report

    def produce(mode):
        assert state.resolve().is_relative_to(out)
        shutil.rmtree(state)
        state.mkdir(mode=0o700)
        for binary, args in ((fixture, [str(state), "baseline-auction-attribution-" + mode]),
            (native_fixture, ["seed-attribution-" + mode, str(state)])):
            ran = subprocess.run([str(binary), *args], env=environment,
                capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, (mode, ran)
        return inventory(evidence), catalog.read_bytes(), source.read_bytes()

    def install(files, image, sources):
        for path in evidence.iterdir():
            path.unlink()
        for name, value in files.items():
            (evidence / name).write_bytes(value)
        catalog.write_bytes(image)
        source.write_bytes(sources)

    def receipts(image):
        # Original catalog: variable listing, pickup table, then fixed receipts.
        at = 108
        for _ in range(6):
            at += 4 + struct.unpack_from("<I", image, at)[0]
        at += 4 + struct.unpack_from("<I", image, at)[0]
        count = struct.unpack_from("<H", image, at)[0]
        at += 2 + count * 25
        money = struct.unpack_from("<I", image, at)[0]
        at += 4 + money * 20
        count = struct.unpack_from("<I", image, at)[0]
        assert len(image) == at + 4 + count * 373
        return at, at + 4

    # The original reproduction remains a valid balance but an unproven credit.
    check("balanced row references absent creator", codes=("claim_source_creator_missing_or_unproven",), balance=True)
    modes = ("settlement", "settlement-fee", "settlement-zero", "settlement-born",
        "settlement-expired", "settlement-removed", "bid", "bid-sold", "bid-sold-fee", "bid-sold-zero")
    for mode in modes:
        files, image, sources = produce(mode)
        codes = ("unknown_legacy_origin",) if mode.endswith("born") else (
            ("native_domain_missing",) if mode.endswith("removed") else ())
        report = check("original typed " + mode, credits=True, codes=codes, balance=not bool(codes))
        assert report["creator_roots"] == 1
        assert report["expected_credits"] == report["compared_credits"]
        assert report["expected_credits"] == struct.unpack_from("<I", sources, 56)[0]

    produce("settlement-born-wrong")
    check("AEC1 cannot borrow a lifetime created by another operation", codes=(
        "unknown_legacy_origin", "claim_source_creator_semantics_invalid", "claim_source_creator_missing_or_unproven"))

    files, image, sources = produce("settlement")
    row = sources[60:130]
    assert len(row) == 70 and row[:16] == operation and struct.unpack_from("<H", row, 16)[0] == 2
    def rows(values):
        source.write_bytes(frame(sources, struct.pack("<I", len(values)) + b"".join(sorted(values, key=lambda value: value[:18]))))

    def changed(offset, fmt, value):
        changed = bytearray(row)
        struct.pack_into(fmt, changed, offset, value)
        return bytes(changed)

    for label, value, codes, good_balance in (
        ("unknown successful root", changed(0, "16s", bytes([7]) + bytes(15)), ("claim_source_creator_missing_or_unproven", "claim_source_credit_row_missing"), True),
        ("wrong native credit slot", changed(16, "<H", 1), ("claim_source_creator_missing_or_unproven", "claim_source_credit_row_missing"), True),
        ("credit deficit", changed(46, "<Q", 59), ("claim_source_credit_mismatch", "claim_source_balance_mismatch"), False),
        ("credit surplus", changed(46, "<Q", 61), ("claim_source_credit_mismatch", "claim_source_balance_mismatch"), False),
        ("wrong beneficiary", changed(34, "<I", 22), ("claim_source_credit_mismatch", "claim_source_beneficiary_mismatch", "claim_source_balance_mismatch"), False),
        ("wrong lifetime", changed(38, "<Q", 4), ("claim_source_credit_mismatch", "claim_source_lifetime_unknown", "claim_source_balance_mismatch"), False),
        ("wrong lineage", changed(18, "16s", bytes([2]) + bytes(15)), ("claim_source_credit_mismatch", "claim_source_lifetime_unknown", "claim_source_balance_mismatch"), False)):
        rows([value])
        check(label, codes=codes, balance=good_balance)
    rows([])
    check("positive compiler credit needs retained row", codes=("claim_source_credit_row_missing", "claim_source_balance_mismatch"))
    baseline_root = next(encoded[82:98] for *_, encoded, _ in records(files) if encoded[98:100] == struct.pack("<H", 20))
    rows([changed(0, "16s", baseline_root)])
    check("baseline observation is not a credit creator", codes=("claim_source_creator_missing_or_unproven", "claim_source_credit_row_missing"), balance=True)
    rows([row, changed(0, "16s", bytes([7]) + bytes(15))])
    check("extra source is never invented by creator", codes=("claim_source_creator_missing_or_unproven", "claim_source_balance_mismatch"))
    historical = changed(0, "16s", bytes([7]) + bytes(15))
    historical = historical[:54] + bytes([8]) + bytes(15)
    rows([row, historical])
    check("consumed historical rows also need creators", codes=("claim_source_creator_missing_or_unproven",), balance=True)
    rows([changed(54, "16s", bytes([8]) + bytes(15))])
    check("valid historical credit does not prove its consumer", credits=True, codes=("claim_source_balance_mismatch",), balance=False)
    rows([row])
    count_at, receipt_at = receipts(image)
    for label, offset, fmt, value, code in (
        ("native receipt unknown operation", receipt_at, "16s", bytes([7]) + bytes(15), "claim_source_creator_native_receipt_missing"),
        ("native receipt command digest", receipt_at + 16, "32s", bytes(32), "claim_source_creator_native_receipt_mismatch"),
        ("native receipt result code", receipt_at + 48, "<I", 5, "claim_source_creator_native_receipt_invalid"),
        ("native receipt result fields", receipt_at + 52 + 22, "<q", 61, "claim_source_creator_native_receipt_mismatch")):
        damaged = bytearray(image)
        struct.pack_into(fmt, damaged, offset, value)
        catalog.write_bytes(frame(damaged))
        check(label, codes=(code,), balance=True)
    catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", 0)))
    check("common root alone cannot establish applied native credit", codes=("claim_source_creator_native_receipt_missing",), balance=True)
    published = bytearray(image)
    published[receipt_at + 372] = 1
    catalog.write_bytes(frame(published))
    check("native event publication does not change credit", credits=True, balance=True)
    padding = bytearray(image)
    padding[receipt_at + 52 + 319] = 99
    catalog.write_bytes(frame(padding))
    check("native result codec trailing padding carries no credit", credits=True, balance=True)
    catalog.write_bytes(image)

    def frozen_change(label, offset, fmt, value, *, command=False, facts=False, writer=False):
        changed_files = dict(files)
        def mutate(encoded, plan_at):
            keys, revisions, payload_size = struct.unpack_from("<III", encoded, 114)
            payload_at = 126 + keys * 16 + revisions * 24
            intent_at = payload_at + payload_size + 4
            assert intent_at + struct.unpack_from("<I", encoded, intent_at - 4)[0] == plan_at
            if writer:
                struct.pack_into(fmt, encoded, intent_at + 12, value)
                struct.pack_into(fmt, encoded, plan_at + 84, value)
            else:
                struct.pack_into(fmt, encoded, (74 if command else intent_at + 256 if facts else payload_at) + offset, value)
            normalized = bytearray(encoded[74:payload_at + payload_size])
            normalized[4] = 1
            normalized[31] = 0
            normalized[32:40] = struct.pack("<Q", 1)
            encoded[intent_at + 160:intent_at + 192] = hashlib.sha256(b"DURIS-ECONOMIC-COMMAND-V1\0" + normalized).digest()
            domain = struct.pack("<HHI", 7, 1, payload_size) + encoded[payload_at:payload_at + payload_size]
            encoded[intent_at + 192:intent_at + 224] = hashlib.sha256(b"DURIS-ECONOMIC-DOMAIN-V1\0" + domain).digest()
            intent = encoded[intent_at:plan_at]
            encoded[plan_at + 152:plan_at + 184] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
            encoded[plan_at + 184:plan_at + 216] = encoded[intent_at + 192:intent_at + 224]
        rewrite(changed_files, operation, mutate)
        updated = next(encoded for *_, encoded, _ in records(changed_files) if encoded[82:98] == operation)
        changed_image = bytearray(image)
        command_size = struct.unpack_from("<I", updated, 48)[0]
        changed_image[receipt_at + 16:receipt_at + 48] = hashlib.sha256(updated[74:74 + command_size]).digest()
        install(changed_files, frame(changed_image), sources)
        # The generic immutable grammar must still pass: this proves the new
        # independent interpretation, not just its envelope checksum guard.
        generic = subprocess.run([str(operator), "--economic-evidence-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert generic.returncode == 0 and not generic.stderr, (label, generic)
        check(label, codes=("claim_source_credit_writer_unproven" if writer else "claim_source_creator_semantics_invalid",
            "claim_source_creator_missing_or_unproven"), balance=True)

    frozen_change("fee changed with all original hashes rebound", 58, "<I", 1500)
    frozen_change("unsupported payload action cannot mint source", 0, "<B", 4)
    frozen_change("wrong auction fence with original bindings rebound", 60, "<Q", (1 << 32) - 2, command=True)
    frozen_change("frozen listing clock needs exact source metadata and plan", 72, "<Q", 3, facts=True)
    frozen_change("unsupported positive claim writer is unproven", 0, "<I", 999, writer=True)
    install(files, image, sources)
    extras = [struct.pack("<QQ", index, 1) + row[16:54] + bytes([8]) + bytes(15) for index in range(121)]
    rows([row, *extras])
    report = check("credit finding cap keeps exact aggregate", codes=("claim_source_creator_missing_or_unproven",), balance=True)
    assert report["credit_finding_count"] == report["finding_count"] == 121
    assert len(report["findings"]) == 100 and report["findings_truncated"]
    rows([row])
    many = [struct.pack("<QQ", index, 1) + row[16:54] + bytes([8]) + bytes(15) for index in range(32767)]
    rows([row, *many])
    report = check("maximum retained source count keeps bounded creator findings", codes=("claim_source_creator_missing_or_unproven",), balance=True)
    assert report["credit_finding_count"] == 32767 and len(report["findings"]) == 100
    rows([row, *many, struct.pack("<QQ", 32767, 1) + row[16:]])
    check("source cap refuses before any partial credit proof", refusal=True)
    rows([row])
    native_receipt = image[receipt_at:receipt_at + 373]
    extra_receipts = [struct.pack("<QQ", index, 1) + native_receipt[16:] for index in range(32767)]
    catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", 32768) + native_receipt + b"".join(extra_receipts)))
    report = check("maximum bounded native receipt table", credits=True, balance=True)
    assert report["creator_roots"] == 1
    catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", 32769) + native_receipt +
        b"".join(extra_receipts) + struct.pack("<QQ", 32767, 1) + native_receipt[16:]))
    check("native receipt cap refuses without partial proof", refusal=True)
    catalog.write_bytes(image)
    for journal_name in (".critical-authority-transaction", ".player-domain-transaction", ".currency-transaction"):
        journal = state / "domains" / journal_name
        journal.write_bytes(b"pending-private-intent")
        check("pending journal " + journal_name, refusal=True)
        journal.unlink()
    for path in (catalog, source):
        content = path.read_bytes()
        path.chmod(0o644)
        check("nonprivate " + path.name, refusal=True)
        path.chmod(0o600)
        alias = path.with_name("native-hardlink")
        os.link(path, alias)
        check("hardlinked " + path.name, refusal=True)
        alias.unlink()
        path.unlink()
        path.symlink_to(alias)
        check("symlink " + path.name, refusal=True)
        path.unlink()
        os.mkfifo(path, 0o600)
        check("FIFO " + path.name, refusal=True)
        path.unlink()
        path.write_bytes(content)
    holder = subprocess.Popen([str(fixture), str(state), "hold-authority-lock"], env=environment,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert select.select([holder.stdout], [], [], 10)[0]
        assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
        check("native writer excludes creator scan", refusal=True)
    finally:
        stdout, stderr = holder.communicate("release\n", timeout=10)
        assert holder.returncode == 0 and not stdout and not stderr
    for mode in ("bytes", "files", "entries", "deadline"):
        before = whole()
        retained_cut = cut(before)
        refused = subprocess.run([str(independent), mode, str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        assert refused.returncode == 1 and not refused.stdout
        assert refused.stderr == "native_restore_qualification_failed\n" and whole() == before
        observations.append(dict(label="cooperative creator budget " + mode, exit=1,
            source_cut=retained_cut, complete_retained_inventory_unchanged=True))
    source.unlink()
    check("creator cannot substitute for missing source file", codes=("claim_source_credit_row_missing", "claim_source_catalog_missing", "claim_source_balance_mismatch"))
    source.write_bytes(sources)
    catalog.unlink()
    check("retained roots cannot substitute native catalog", codes=("claim_source_creator_native_receipt_missing", "native_domain_missing", "claim_sources_without_native_claim"))
    catalog.write_bytes(image)
    check("original creator and source remain unchanged", credits=True, balance=True)


def qualify(native_source, artifacts, *, fixture=None, operator=None, native_fixture=None, red=False):
    native, out = native_source.resolve(), artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    inputs, native_inputs = audit_source_inputs(), fingerprint(native / "src")
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_auction_attribution_cases.py",
        "tests/async/flatfile_auction_money_fixture.cpp", "tests/async/flatfile_restore_authority_fixture.cpp")}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "economic-fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    native_fixture = native_fixture or build_native_oracle(native, out)
    observations = []
    with tempfile.TemporaryDirectory(prefix="auction-attribution-", dir=out) as temporary:
        root = Path(temporary)
        for binary, args in ((native_fixture, ["seed", str(root)]),
            (fixture, [str(root), "baseline-auction-money"])):
            ran = subprocess.run([str(binary), *args], env=environment,
                capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, ran
        source = root / "domains/auction_claim_sources"
        encoded = bytearray(source.read_bytes())
        operation = bytes([7]) + bytes(15)
        encoded[60:76] = operation
        encoded[24:56] = hashlib.sha256(encoded[56:]).digest()
        source.write_bytes(encoded)
        retained_roots = []
        for index in sorted((root / "economic-evidence").glob("bucket-*.eai")):
            frame = index.read_bytes()
            body = frame[48:]
            assert hashlib.sha256(body).digest() == frame[16:48]
            count = struct.unpack_from("<I", body, 20)[0]
            assert len(body) == 32 + count * 64
            retained_roots.extend(body[32 + n * 64:48 + n * 64].hex() for n in range(count))
        assert retained_roots and operation.hex() not in retained_roots
        before = retained(root)
        oracle = subprocess.run([str(native_fixture), "source-balance", str(root)],
            capture_output=True, text=True, timeout=30)
        assert oracle.returncode == 0 and not oracle.stderr and retained(root) == before, oracle
        assert json.loads(oracle.stdout) == dict(claim_amount=60, source_balance_matches=True)
        audit = subprocess.run([str(operator), "--economic-auction-source-balance-audit", str(root)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert audit.returncode == 0 and not audit.stderr and retained(root) == before, audit
        report = json.loads(audit.stdout)
        assert report["remaining_claim_source_balances_verified"] and not report["claim_source_attribution_verified"]
        observation = dict(label="balanced source references no retained accounting creator",
            source_operation=operation.hex(), retained_roots=retained_roots,
            original_native_predicate=json.loads(oracle.stdout), original_balance_audit=report,
            complete_retained_inventory_unchanged=True)
        for entry in sorted(root.rglob("*")):
            if entry.is_file():
                target = out / "red-native-cut" / entry.relative_to(root)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(entry.read_bytes())
        observations.append(observation)
        if not red:
            independent = build_independent(out)
            green_checks(root, out, observations, fixture, operator, native_fixture, independent, environment)
    assert fingerprint(native / "src") == native_inputs and audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    result = dict(completed=True, red=red, observations=observations, skips=0,
        native_source_inputs=native_inputs, audit_source_inputs=inputs, owned_inputs=owned,
        executable_sha256={name: hashlib.sha256(Path(path).read_bytes()).hexdigest()
            for name, path in dict(economic_fixture=fixture, operator=operator, native_fixture=native_fixture,
                **({} if red else dict(independent=independent))).items()},
        modeled_economic_witnesses=True, original_native_providers=True,
        accounting_activated=False, full_R7_qualified=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(result, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(completed=True, red=red, source_attribution_cases=len(observations), skips=0)), flush=True)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts, red=args.red)
