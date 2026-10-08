#!/usr/bin/env python3
"""Independent native claim-source consumption; never whole-release evidence."""
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
from flatfile_auction_attribution_cases import frame
from flatfile_money_history_cases import records, rewrite
from test_flatfile_restore_economic_authority import inventory, rehash
import build_restore_qualifier as qualifier


def build_independent(out):
    source = out / "independent.cpp"
    source.write_text(r'''#include "qualify_flatfile_auction_source_consumption.h"
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
        auto result = restore_auction_source_consumption::audit(argv[2], budget);
        std::cout << result.valid() << " " << result.attribution_verified() << " "
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


def run_checks(native, out, red=False, *, fixture=None, operator=None, native_fixture=None):
    os.umask(0o077)
    out.mkdir(parents=True, exist_ok=True)
    native_inputs = fingerprint(native / "src")
    audit_inputs = audit_source_inputs()
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_auction_consumption_cases.py",
        "tests/async/flatfile_auction_money_fixture.cpp",
        "tests/async/flatfile_restore_authority_fixture.cpp")}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "economic-fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    original = native_fixture or build_native_oracle(native, out)
    independent = None if red else build_independent(out)
    observations = []
    with tempfile.TemporaryDirectory(prefix="native-claim-consumption-", dir=out) as temporary:
        state = Path(temporary)
        for binary, args in ((fixture, [str(state), "baseline-auction-attribution-settlement-consumption-money"]),
            (original, ["seed-attribution-settlement-consumption-money", str(state)])):
            ran = subprocess.run([str(binary), *args], env=environment,
                capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, ran
        source = state / "domains/auction_claim_sources"
        healthy = source.read_bytes()
        assert len(healthy) == 130 and healthy[114:130] == bytes([8]) + bytes(15)

        def check(label, native_proof):
            before = retained(state)
            cut = out / "inputs" / f"case-{len(observations):03}.json"
            cut.parent.mkdir(exist_ok=True)
            cut.write_text(json.dumps(before, sort_keys=True, indent=2) + "\n", encoding="utf-8")
            objects = out / "inputs/objects"
            objects.mkdir(exist_ok=True)
            for name, entry in before.items():
                if stat.S_ISREG(entry["mode"]):
                    payload = (state / name).read_bytes()
                    blob = objects / entry["sha256"]
                    if not blob.exists():
                        blob.write_bytes(payload)
                    assert hashlib.sha256(payload).hexdigest() == entry["sha256"]
            ran = subprocess.run([str(operator), "--economic-auction-source-credit-audit", str(state)],
                env=environment, capture_output=True, text=True, timeout=30)
            assert ran.returncode == 0 and not ran.stderr, ran
            report = json.loads(ran.stdout)
            assert report["claim_source_credit_roots_verified"]
            assert report["remaining_claim_source_balances_verified"]
            assert not report["claim_source_attribution_verified"]
            assert not report["claim_source_consumption_order_verified"]
            assert not report["claim_source_digest_verified"]
            assert not report["release_qualified"]
            predicates = {}
            for mode in ("source-balance", "consumer-proof"):
                probed = subprocess.run([str(original), mode, str(state)], env=environment,
                    capture_output=True, text=True, timeout=30)
                assert probed.returncode == 0 and not probed.stderr, probed
                predicates.update(json.loads(probed.stdout))
            assert predicates["claim_amount"] == 0 and predicates["source_balance_matches"]
            assert predicates["native_frozen_consumer_sources_match"] == native_proof
            successor = None
            if not red:
                audited = subprocess.run([str(operator), "--economic-auction-source-attribution-audit", str(state)],
                    env=environment, capture_output=True, text=True, timeout=30)
                pure = subprocess.run([str(independent), "normal", str(state)], env=environment,
                    capture_output=True, text=True, timeout=30)
                expected = int(not native_proof)
                assert audited.returncode == pure.returncode == expected, (label, audited, pure)
                assert not audited.stderr and not pure.stderr, (label, audited, pure)
                successor = json.loads(audited.stdout)
                assert successor["claim_source_attribution_verified"] == native_proof, successor
                assert successor["claim_source_consumption_order_verified"] == native_proof
                assert successor["claim_source_digest_verified"] == native_proof
                assert pure.stdout == f"{int(native_proof)} {int(native_proof)} {successor['finding_count']}\n"
                codes = set() if native_proof else {"claim_source_consumer_marker_mismatch",
                    "claim_source_frozen_digest_mismatch", "claim_source_consumer_missing_or_unproven"}
                assert {row["code"] for row in successor["findings"]} == codes, successor
                for flag in ("account_origins_verified", "cross_epoch_continuity_verified",
                    "native_holdings_compared", "full_R7_qualified", "release_qualified"):
                    assert not successor[flag], (label, flag)
            assert retained(state) == before, label
            observations.append(dict(label=label, original_credit_audit=report,
                successor=successor,
                original_native_predicates=predicates,
                source_cut=dict(path=str(cut.relative_to(out)), sha256=hashlib.sha256(cut.read_bytes()).hexdigest()),
                complete_retained_inventory_unchanged=True))
            print("AUCTION_SOURCE_CONSUMPTION " + label, flush=True)

        check("original typed cashout source set", True)
        damaged = bytearray(healthy)
        damaged[114:130] = bytes([9]) + bytes(15)
        source.write_bytes(frame(damaged))
        check("unknown consumer preserves balanced totals and verified creators", False)
        if not red:
            chain_checks(state, out, observations, fixture, operator, original, independent, environment)
    assert native_inputs == fingerprint(native / "src")
    assert audit_inputs == audit_source_inputs()
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name,value in owned.items())
    report = dict(completed=True, red=red, observations=observations, skips=0,
        native_source_inputs=native_inputs, audit_source_inputs=audit_inputs, owned_inputs=owned,
        qualifier_sha256=hashlib.sha256(operator.read_bytes()).hexdigest(),
        original_fixture_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),
        accounting_activated=False, canonical_consumption_qualified=False,
        claim_source_digest_qualified=False, full_R7_qualified=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(dict(completed=True, red=red, source_consumption_cases=len(observations), skips=0)))
    return report


def chain_checks(state, out, observations, fixture, operator, original, independent, environment):
    catalog, source = state / "domains/auction_catalog", state / "domains/auction_claim_sources"
    evidence = state / "economic-evidence"

    def whole():
        info = state.lstat()
        entries = retained(state)
        for name, value in entries.items():
            if stat.S_ISLNK(value["mode"]):
                value["symlink_target"] = os.readlink(state / name)
        return dict(root_mode=info.st_mode, root_inode=info.st_ino,
            root_links=info.st_nlink, root_mtime_ns=info.st_mtime_ns, entries=entries)

    def cut(before):
        target = out / "inputs" / f"case-{len(observations):03}.json"
        target.parent.mkdir(exist_ok=True)
        target.write_text(json.dumps(before, sort_keys=True, indent=2) + "\n", encoding="utf-8")
        objects = out / "inputs/objects"
        objects.mkdir(exist_ok=True)
        for name, value in before["entries"].items():
            if stat.S_ISREG(value["mode"]):
                payload = (state / name).read_bytes()
                assert hashlib.sha256(payload).hexdigest() == value["sha256"]
                blob = objects / value["sha256"]
                if not blob.exists():
                    blob.write_bytes(payload)
        return dict(path=str(target.relative_to(out)), sha256=hashlib.sha256(target.read_bytes()).hexdigest())

    def check(label, *, codes=(), refusal=False, native=None, balanced=False, healthy=False):
        before = whole()
        retained_cut = cut(before)
        audited = subprocess.run([str(operator), "--economic-auction-source-attribution-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        pure = subprocess.run([str(independent), "normal", str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        expected = int(refusal or not healthy)
        assert audited.returncode == pure.returncode == expected, (label, audited, pure)
        report = None
        if refusal:
            assert not audited.stdout and not pure.stdout
            assert audited.stderr == pure.stderr == "native_restore_qualification_failed\n", (label, audited, pure)
        else:
            assert not audited.stderr and not pure.stderr, (label, audited, pure)
            report = json.loads(audited.stdout)
            assert set(codes).issubset({row["code"] for row in report["findings"]}), (label, report, codes)
            assert bool(report["finding_count"]) == (not healthy), (label, report)
            assert report["claim_source_attribution_verified"] == healthy, (label, report)
            assert pure.stdout == f"{int(healthy)} {int(healthy)} {report['finding_count']}\n", (label, pure)
            for flag in ("account_origins_verified", "cross_epoch_continuity_verified",
                "native_holdings_compared", "full_R7_qualified", "release_qualified"):
                assert not report[flag], (label, flag)
            assert not any(secret in audited.stdout for secret in ("native-fixture", "renamed", "Seller", "Buyer"))
        predicate = None
        if native is not None:
            probe = subprocess.run([str(original), "consumer-proof", str(state)], env=environment,
                capture_output=True, text=True, timeout=30)
            assert probe.returncode == 0 and not probe.stderr, (label, probe)
            predicate = json.loads(probe.stdout)
            assert predicate["native_frozen_consumer_sources_match"] == native[0]
            assert predicate["native_whole_consumption_match"] == native[1]
        if balanced:
            prior = subprocess.run([str(operator), "--economic-auction-source-credit-audit", str(state)],
                env=environment, capture_output=True, text=True, timeout=30)
            assert prior.returncode == 0 and not prior.stderr, (label, prior)
            assert json.loads(prior.stdout)["claim_source_credit_roots_verified"]
            assert json.loads(prior.stdout)["remaining_claim_source_balances_verified"]
        assert whole() == before, label
        observations.append(dict(label=label, report=report, original_native_predicates=predicate,
            source_cut=retained_cut, complete_retained_inventory_unchanged=True))
        print("AUCTION_SOURCE_CONSUMPTION " + label, flush=True)
        return report

    def produce(mode):
        assert state.is_relative_to(out)
        shutil.rmtree(state)
        state.mkdir(mode=0o700)
        for binary, args in ((fixture, [str(state), "baseline-auction-attribution-consumption-chain-" + mode]),
            (original, ["seed-attribution-consumption-chain-" + mode, str(state)])):
            ran = subprocess.run([str(binary), *args], env=environment, capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, (mode, ran)
        return inventory(evidence), catalog.read_bytes(), source.read_bytes()

    def install(files, image, sources):
        for path in evidence.iterdir():
            path.unlink()
        for name, value in files.items():
            (evidence / name).write_bytes(value)
        catalog.write_bytes(image)
        source.write_bytes(sources)

    def receipt_table(image):
        at = 60
        for _ in range(struct.unpack_from("<I", image, 56)[0]):
            at += 48
            for _ in range(7):
                at += 4 + struct.unpack_from("<I", image, at)[0]
            at += 2 + 25 * struct.unpack_from("<H", image, at)[0]
        at += 4 + 20 * struct.unpack_from("<I", image, at)[0]
        count = struct.unpack_from("<I", image, at)[0]
        assert len(image) == at + 4 + count * 373
        return at, {image[pos:pos + 16]: pos for pos in range(at + 4, len(image), 373)}

    for mode in ("two", "reverse", "partial", "partial-reverse", "future"):
        files, image, sources = produce(mode)
        report = check("original native consumption chain " + mode, healthy=True, native=(True, True), balanced=True)
        assert report["creator_roots"] == (3 if mode in ("partial", "partial-reverse", "future") else 2)
        assert report["selected_source_rows"] == 2
        assert report["consumer_roots"] == report["compared_consumers"] == (2 if mode in ("partial", "partial-reverse", "future") else 1)
        assert report["cashout_roots"] == report["verified_digests"] == 1
        if mode in ("partial", "partial-reverse", "future"):
            damaged = bytearray(sources)
            damaged[114:130], damaged[184:200] = damaged[184:200], damaged[114:130]
            source.write_bytes(frame(damaged))
            check("balanced consumer swap " + mode, native=(False, False), balanced=True,
                codes=("claim_source_consumer_marker_mismatch", "claim_source_frozen_digest_mismatch"))
            source.write_bytes(sources)

    operation = bytes([10]) + bytes(15)
    baseline = next(encoded[82:98] for *_, encoded, _ in records(files) if encoded[98:100] == struct.pack("<H", 20))
    for label, marker in (("unknown consumer", bytes([12]) + bytes(15)),
        ("creator cannot be its source consumer", bytes([6]) + bytes(15)),
        ("baseline cannot consume a historical source", baseline),
        ("cashout cannot borrow earlier rebid source", operation)):
        damaged = bytearray(sources)
        damaged[184:200] = marker
        source.write_bytes(frame(damaged))
        check(label, codes=("claim_source_consumer_marker_mismatch",), balanced=True)
    source.write_bytes(sources)
    damaged = bytearray(sources)
    damaged[114:130] = bytes(16)
    source.write_bytes(frame(damaged))
    check("unconsumed historical row cannot erase successful cashout", codes=("claim_source_consumer_marker_mismatch", "claim_source_balance_mismatch"))
    source.write_bytes(sources)

    count_at, positions = receipt_table(image)
    for root in (8, 10):
        key = bytes([root]) + bytes(15)
        at = positions[key]
        for label, offset, fmt, value in (("digest", 16, "32s", bytes(32)),
            ("result code", 48, "<I", 5), ("result balance", 52 + 38, "<q", 1)):
            damaged = bytearray(image)
            struct.pack_into(fmt, damaged, at + offset, value)
            catalog.write_bytes(frame(damaged))
            check(f"consumer {root} native receipt {label}", balanced=root == 10,
                codes=("claim_source_consumer_native_receipt_mismatch",))
        catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", len(positions) - 1) +
            b"".join(image[pos:pos + 373] for identity, pos in positions.items() if identity != key)))
        check(f"consumer {root} native receipt missing", balanced=root == 10,
            codes=("claim_source_consumer_native_receipt_missing",))
    for label, offset, value in (("published event", 372, 1), ("ignored result padding", 52 + 319, 99)):
        damaged = bytearray(image)
        damaged[positions[operation] + offset] = value
        catalog.write_bytes(frame(damaged))
        check(label + " does not change consumed source proof", healthy=True)
    catalog.write_bytes(image)

    def frozen_change(label, offset, fmt, value, *, writer=False, header=False):
        changed_files = dict(files)
        def mutate(encoded, plan_at):
            keys, revisions, payload_size = struct.unpack_from("<III", encoded, 114)
            payload_at = 126 + keys * 16 + revisions * 24
            intent_at = payload_at + payload_size + 4
            if writer:
                struct.pack_into(fmt, encoded, intent_at + 12, value)
                struct.pack_into(fmt, encoded, plan_at + 84, value)
            elif header:
                struct.pack_into(fmt, encoded, intent_at + offset, value)
                struct.pack_into(fmt, encoded, plan_at + offset - (24 if offset == 80 else 8), value)
            else:
                struct.pack_into(fmt, encoded, intent_at + 256 + offset, value)
            intent = encoded[intent_at:plan_at]
            encoded[plan_at + 152:plan_at + 184] = hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0" + intent).digest()
        rewrite(changed_files, operation, mutate)
        updated = next(encoded for *_, encoded, _ in records(changed_files) if encoded[82:98] == operation)
        keys, revisions, payload_size = struct.unpack_from("<III", updated, 114)
        intent_at = 126 + keys * 16 + revisions * 24 + payload_size + 4
        intent = updated[intent_at:74 + struct.unpack_from("<I", updated, 48)[0]]
        # Rebind the actual native immutable claim too. A changed source slot
        # must reach this independent interpretation after the common grammar.
        claim_name, claim_wire = next((name, value) for name, value in changed_files.items()
            if name.startswith("source-claim-") and value[112:128] == operation)
        body = intent[32:48] + intent[112:160] + operation + bytes([1]) + bytes(7)
        del changed_files[claim_name]
        changed_files["source-claim-" + hashlib.sha256(body[:64]).hexdigest() + ".bin"] = rehash(claim_wire[:48] + body)
        changed_image = bytearray(image)
        command_size = struct.unpack_from("<I", updated, 48)[0]
        changed_image[positions[operation] + 16:positions[operation] + 48] = hashlib.sha256(updated[74:74 + command_size]).digest()
        install(changed_files, frame(changed_image), sources)
        generic = subprocess.run([str(operator), "--economic-evidence-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert generic.returncode == 0 and not generic.stderr, (label, generic)
        return check(label, codes=("claim_source_consumer_writer_unproven",) if writer else (),
            native=None if writer else (False, True))

    frozen_change("frozen digest rebound independently of original native sources", 48, "32s", bytes(32))
    frozen_change("frozen source count rebound independently of original native sources", 44, "<I", 2)
    frozen_change("zero source count cannot qualify", 44, "<I", 0)
    frozen_change("source count above native 4096 cap cannot qualify", 44, "<I", 4097)
    frozen_change("frozen claim clock differs from applied native claim edge", 36, "<Q", 5)
    frozen_change("frozen original creator cannot borrow another root", 80, "16s", bytes([9]) + bytes(15), header=True)
    frozen_change("frozen source slot cannot borrow another credit leg", 156, "<I", 1, header=True)
    frozen_change("unsupported negative claim writer cannot prove consumption", 0, "<I", 999, writer=True)
    install(files, image, sources)

    rows = [sources[pos:pos + 70] for pos in range(60, len(sources), 70)]
    extras = [struct.pack("<QQ", index, 1) + rows[0][16:54] + bytes([12]) + bytes(15) for index in range(121)]
    source.write_bytes(frame(sources, struct.pack("<I", 123) + b"".join(sorted([*rows, *extras], key=lambda row: row[:18]))))
    report = check("consumer findings retain exact aggregate beyond rendering cap")
    assert report["consumption_finding_count"] == 121 and report["finding_count"] == 242
    assert len(report["findings"]) == 100 and report["findings_truncated"]
    source.write_bytes(sources)

    extras = [struct.pack("<QQ", index, 1) + rows[0][16:54] + bytes([12]) + bytes(15) for index in range(32766)]
    source.write_bytes(frame(sources, struct.pack("<I", 32768) + b"".join(sorted([*rows, *extras], key=lambda row: row[:18]))))
    report = check("maximum bounded native source table retains exact consumer aggregate")
    assert report["consumption_finding_count"] == 32766 and report["finding_count"] == 65532
    source.write_bytes(frame(sources, struct.pack("<I", 32769) + b"".join(sorted(
        [*rows, *extras, struct.pack("<QQ", 32766, 1) + rows[0][16:]], key=lambda row: row[:18]))))
    check("native source table cap refuses without partial attribution", refusal=True)
    source.write_bytes(sources)
    receipt = image[next(iter(positions.values())):next(iter(positions.values())) + 373]
    extra_receipts = [struct.pack("<QQ", index, 1) + receipt[16:] for index in range(32768 - len(positions))]
    catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", 32768) + image[count_at + 4:] + b"".join(extra_receipts)))
    check("maximum bounded native receipt table supports full attribution", healthy=True)
    catalog.write_bytes(frame(image, image[56:count_at] + struct.pack("<I", 32769) + image[count_at + 4:] +
        b"".join(extra_receipts) + struct.pack("<QQ", 32768 - len(positions), 1) + receipt[16:]))
    check("native receipt table cap refuses without partial attribution", refusal=True)
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
        check("native writer excludes consumption scan", refusal=True)
    finally:
        stdout, stderr = holder.communicate("release\n", timeout=10)
        assert holder.returncode == 0 and not stdout and not stderr
    for mode in ("bytes", "files", "entries", "deadline"):
        before = whole()
        retained_cut = cut(before)
        refused = subprocess.run([str(independent), mode, str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        assert refused.returncode == 1 and not refused.stdout and whole() == before
        assert refused.stderr == "native_restore_qualification_failed\n"
        observations.append(dict(label="cooperative consumption budget " + mode, exit=1,
            source_cut=retained_cut, complete_retained_inventory_unchanged=True))
    source.unlink()
    check("frozen source digest cannot substitute missing source catalog")
    source.write_bytes(sources)
    catalog.unlink()
    check("retained consumer roots cannot substitute missing native receipts")
    catalog.write_bytes(image)
    check("complete original sources and consumers remain unchanged", healthy=True)
    produce("oversize")
    check("native whole-prefix refusal prevents skipping an oversized earlier source", balanced=True,
        native=(True, False), codes=("claim_source_whole_row_selection_unproven", "claim_source_consumed_row_not_selected"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--native-source", type=Path, default=ROOT)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    run_checks(args.native_source, args.artifacts, args.red)
