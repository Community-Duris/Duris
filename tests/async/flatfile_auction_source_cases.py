#!/usr/bin/env python3
"""Independent native claim-source reconciliation; never release evidence alone."""
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


def build_independent(out):
    source = out / "independent.cpp"
    source.write_text(r'''#include "qualify_flatfile_auction_source_balance.h"
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
        const auto result = restore_auction_source_balance::audit(argv[2], budget);
        std::cout << result.valid() << " " << result.balance_verified() << " "
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


def green_checks(state, out, observations, fixture, operator, native_fixture, independent, environment):
    catalog, sources = state / "domains/auction_catalog", state / "domains/auction_claim_sources"
    original, original_sources = catalog.read_bytes(), sources.read_bytes()
    row = original_sources[60:130]
    # Native seed retains one listing, one pickup, then the receipt table.
    position = 108
    for _ in range(6):
        position += 4 + struct.unpack_from("<I", original, position)[0]
    item_count = position + 4 + struct.unpack_from("<I", original, position)[0]
    assert struct.unpack_from("<H", original, item_count)[0] == 1
    money_count = item_count + 2 + 25
    money = money_count + 4
    operation_count = money + 20

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

    def check(label, codes=(), *, refusal=False, verified=False, oracle=None):
        before = whole()
        retained_cut = cut(before)
        parsed = subprocess.run([str(operator), "--economic-auction-source-balance-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": operator wrote retained inputs"
        pure = subprocess.run([str(independent), "normal", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": independent reader wrote retained inputs"
        expected = int(refusal or bool(codes))
        assert parsed.returncode == pure.returncode == expected, (label, parsed, pure)
        report = None
        if refusal:
            assert not parsed.stdout and not pure.stdout
            assert parsed.stderr == pure.stderr == "native_restore_qualification_failed\n"
        else:
            assert not parsed.stderr and not pure.stderr, (label, parsed, pure)
            report = json.loads(parsed.stdout)
            assert {value["code"] for value in report["findings"]} == set(codes), (label, codes, report)
            assert report["remaining_claim_source_balances_verified"] == verified
            assert pure.stdout == f"{int(expected == 0)} {int(verified)} {report['finding_count']}\n"
            for flag in ("account_origins_verified", "claim_source_attribution_verified",
                "claim_source_consumption_order_verified", "cross_epoch_continuity_verified",
                "other_money_domains_verified", "native_holdings_compared", "full_R7_qualified", "release_qualified"):
                assert not report[flag]
            assert not any(secret in parsed.stdout for secret in ("Seller", "Buyer", "native-fixture", "fixture-object"))
        native_report = None
        if oracle is not None:
            native = subprocess.run([str(native_fixture), "source-balance", str(state)],
                capture_output=True, text=True, timeout=30)
            assert native.returncode == 0 and not native.stderr and whole() == before, native
            native_report = json.loads(native.stdout)
            assert native_report["source_balance_matches"] == oracle
        observations.append(dict(label=label, exit=parsed.returncode, report=report,
            independent_exit=pure.returncode, original_native_predicate=native_report,
            source_cut=retained_cut, complete_retained_inventory_unchanged=True))
        print("AUCTION_SOURCE_BALANCE " + label, flush=True)
        return report

    def frame(value, body=None):
        header = bytearray(value[:56])
        payload = value[56:] if body is None else body
        struct.pack_into("<I", header, 12, len(payload))
        header[24:56] = hashlib.sha256(payload).digest()
        return bytes(header) + payload

    def rows(values):
        ordered = sorted(values, key=lambda value: value[:18])
        sources.write_bytes(frame(original_sources, struct.pack("<I", len(ordered)) + b"".join(ordered)))

    def changed_row(offset, fmt, value):
        changed = bytearray(row)
        struct.pack_into(fmt, changed, offset, value)
        return bytes(changed)

    def pickup(amount):
        changed = bytearray(original)
        struct.pack_into("<q", changed, money + 4, amount)
        catalog.write_bytes(frame(changed))

    def replace(mode):
        assert state.resolve().is_relative_to(out)
        shutil.rmtree(state)
        state.mkdir(mode=0o700)
        for binary, args in ((native_fixture, ["seed", str(state)]), (fixture, [str(state), mode])):
            seeded = subprocess.run([str(binary), *args], env=environment,
                capture_output=True, text=True, timeout=90)
            assert seeded.returncode == 0 and not seeded.stderr, seeded

    rows([row])
    report = check("native remaining sources match current claim and lifetime", verified=True, oracle=True)
    assert report["current_auction_money_values_verified"] and report["compared_claims"] == 1
    rows([changed_row(46, "<Q", 61)])
    report = check("61 source copper cannot back 60 claim copper", ("claim_source_balance_mismatch",), oracle=False)
    assert report["current_auction_money_values_verified"] and report["source_finding_count"] == 1
    assert report["findings"][0]["observed"]["native"] == [61, 0, 0, 0]
    assert report["findings"][0]["observed"]["expected"] == [60, 0, 0, 0]
    rows([changed_row(46, "<Q", 59)])
    check("source deficit is never repaired", ("claim_source_balance_mismatch",), oracle=False)
    rows([])
    check("missing retained row cannot fund a positive claim", ("claim_source_balance_mismatch",), oracle=False)
    rows([changed_row(46, "<Q", 30), bytes([7]) + row[1:46] + struct.pack("<Q", 30) + row[54:]])
    check("two native source rows sum exactly", verified=True, oracle=True)
    rows([row, bytes([7]) + row[1:54] + bytes([8]) + bytes(15)])
    report = check("consumed historical row does not fund current claim", verified=True, oracle=True)
    assert report["consumed_source_rows"] == report["unconsumed_source_rows"] == 1
    rows([changed_row(54, "16s", bytes([8]) + bytes(15))])
    check("consumed row cannot fund unchanged claim", ("claim_source_balance_mismatch",), oracle=False)
    for label, bad, code in (
        ("unknown mapping", changed_row(38, "<Q", (1 << 64) - 1), "claim_source_lifetime_unknown"),
        ("foreign lineage", changed_row(18, "16s", bytes([2]) + bytes(15)), "claim_source_lifetime_unknown"),
        ("wrong beneficiary", changed_row(34, "<I", (1 << 32) - 1), "claim_source_beneficiary_mismatch")):
        rows([bad])
        check(label + " cannot fund current lifetime", (code, "claim_source_balance_mismatch"), oracle=False)
        rows([row, bytes([7]) + bad[1:54] + bytes([8]) + bytes(15)])
        check("consumed " + label + " still needs retained beneficiary mapping", (code,), oracle=True)
    rows([changed_row(16, "<H", (1 << 16) - 1)])
    check("opaque nonzero native slot does not imply attribution", verified=True, oracle=True)
    rows([changed_row(46, "<Q", (1 << 31) - 1)])
    check("native maximum source amount remains exact", ("claim_source_balance_mismatch",), oracle=False)
    rows([row])
    for name in (".critical-authority-transaction", ".player-domain-transaction", ".currency-transaction"):
        journal = state / "domains" / name
        journal.write_bytes(b"pending-native-intent")
        check("pending journal " + name, refusal=True)
        journal.unlink()
    for path in (catalog, sources):
        path.chmod(0o644)
        check("nonprivate " + path.name, refusal=True)
        path.chmod(0o600)
        alias = path.with_name("native-hardlink")
        os.link(path, alias)
        check("hardlinked " + path.name, refusal=True)
        alias.unlink()
        content = path.read_bytes()
        path.unlink()
        path.symlink_to(alias)
        check("symlink " + path.name, refusal=True)
        path.unlink()
        os.mkfifo(path, 0o600)
        check("FIFO " + path.name, refusal=True)
        path.unlink()
        path.write_bytes(content)
    lock = state / "domains/.critical-authority.lock"
    lock.unlink()
    check("missing native lock is never recreated", refusal=True)
    assert not lock.exists()
    recreated = subprocess.run([str(native_fixture), "public-query", str(state)],
        capture_output=True, text=True, timeout=30)
    assert recreated.returncode == 0 and lock.exists(), recreated
    holder = subprocess.Popen([str(fixture), str(state), "hold-authority-lock"], env=environment,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert select.select([holder.stdout], [], [], 10)[0]
        assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
        check("native writer excludes coherent source audit", refusal=True)
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
        observations.append(dict(label="cooperative source audit budget " + mode, exit=1,
            source_cut=retained_cut, complete_retained_inventory_unchanged=True))
    sources.unlink()
    check("missing source catalog cannot qualify positive claim", ("claim_source_catalog_missing", "claim_source_balance_mismatch"))
    rows([row])
    catalog.unlink()
    check("source rows need current native claim", ("native_domain_missing", "claim_sources_without_native_claim"))
    catalog.write_bytes(original)
    damaged = bytearray(original_sources)
    damaged[24] ^= 1
    sources.write_bytes(damaged)
    check("native source digest mismatch refuses structurally", refusal=True)
    rows([changed_row(16, "<H", 0)])
    check("zero native source slot refuses structurally", refusal=True)
    rows([changed_row(54, "16s", row[:16])])
    check("self-consumed native source refuses structurally", refusal=True)
    extras = [struct.pack("<QQ", index, 1) + row[16:38] + struct.pack("<Q", 1000 + index) + row[46:]
        for index in range(121)]
    rows([row, *extras])
    report = check("source finding cap preserves exact count", ("claim_source_lifetime_unknown",))
    assert report["finding_count"] == report["source_finding_count"] == 121
    assert len(report["findings"]) == 100 and report["findings_truncated"]
    many = [struct.pack("<QQ", index, 1) + row[16:46] + struct.pack("<Q", (1 << 31) - 1) + row[54:]
        for index in range(32768)]
    rows(many)
    report = check("bounded maximum source sum preserves 64 bit total", ("claim_source_balance_mismatch",), oracle=False)
    assert report["findings"][0]["observed"]["native"][0] == 32768 * ((1 << 31) - 1)
    rows([*many, struct.pack("<QQ", 32768, 1) + row[16:]])
    check("source row cap refuses without partial proof", refusal=True)
    replace("baseline-auction-money-claim-zero")
    pickup(0)
    rows([changed_row(54, "16s", bytes([8]) + bytes(15))])
    check("fully consumed source backs zero active claim", verified=True, oracle=True)
    rows([])
    check("present empty source catalog backs zero active claim", verified=True, oracle=True)
    sources.unlink()
    check("missing source catalog cannot qualify zero claim", ("claim_source_catalog_missing",))
    replace("baseline-auction-money-claim-retired")
    catalog.write_bytes(frame(original, original[56:money_count] + struct.pack("<I", 0) + original[operation_count:]))
    rows([row])
    check("retired lifetime cannot retain spendable source", ("retired_claim_source_unconsumed",))
    rows([changed_row(54, "16s", bytes([8]) + bytes(15))])
    check("consumed retired source remains retained historical row", verified=True)
    replace("baseline-auction-money-claim-recreated")
    rows([row])
    check("recreated native PID cannot borrow retired lifetime source", (
        "retired_claim_source_unconsumed", "claim_source_balance_mismatch", "economic_history_missing"))
    rows([changed_row(38, "<Q", 7)])
    report = check("new lifetime source balance cannot manufacture economic history", ("economic_history_missing",))
    assert not report["current_auction_money_values_verified"]
    replace("baseline-auction-money-epochs")
    changed = bytearray(original)
    struct.pack_into("<q", changed, 76, 90)
    struct.pack_into("<q", changed, money + 4, 80)
    catalog.write_bytes(frame(changed))
    rows([changed_row(46, "<Q", 80)])
    report = check("current native source balance remains separate from epoch origin proof", verified=True, oracle=True)
    assert report["epoch"] == "19000000000000000000000000000000"
    assert state.resolve().is_relative_to(out)
    shutil.rmtree(state)
    state.mkdir(mode=0o700)
    report = check("empty inactive authority reports observation without verification")
    assert not report["initialized"]


def qualify(native_source, artifacts, *, fixture=None, operator=None, native_fixture=None, red=False):
    native, out = native_source.resolve(), artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    native_before, inputs = fingerprint(native / "src"), audit_source_inputs()
    owned_names = ("tests/async/flatfile_auction_source_cases.py",
        "tests/async/flatfile_auction_money_fixture.cpp",
        "tests/async/flatfile_restore_authority_fixture.cpp", "tests/async/test_flatfile_restore_economic_authority.py")
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in owned_names}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "economic-fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    native_fixture = native_fixture or build_native_oracle(native, out)
    independent = None if red else build_independent(out)
    observations = []
    with tempfile.TemporaryDirectory(prefix="auction-sources-", dir=out) as temporary:
        state = Path(temporary)
        seeded = subprocess.run([str(native_fixture), "seed", str(state)], capture_output=True,
            text=True, timeout=30)
        assert seeded.returncode == 0 and not seeded.stderr, seeded
        economic = subprocess.run([str(fixture), str(state), "baseline-auction-money"],
            env=environment, capture_output=True, text=True, timeout=90)
        assert economic.returncode == 0 and not economic.stderr, economic
        source = state / "domains/auction_claim_sources"
        encoded = bytearray(source.read_bytes())
        struct.pack_into("<Q", encoded, 106, 61)
        encoded[24:56] = hashlib.sha256(encoded[56:]).digest()
        source.write_bytes(encoded)
        before = retained(state)
        oracle = subprocess.run([str(native_fixture), "source-balance", str(state)],
            capture_output=True, text=True, timeout=30)
        assert oracle.returncode == 0 and not oracle.stderr and retained(state) == before, oracle
        native_result = json.loads(oracle.stdout)
        assert native_result == dict(claim_amount=60, source_balance_matches=False)
        audit = subprocess.run([str(operator), "--economic-auction-money-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert audit.returncode == 0 and not audit.stderr and retained(state) == before, audit
        value_result = json.loads(audit.stdout)
        assert value_result["current_auction_money_values_verified"]
        assert not value_result["claim_source_attribution_verified"]
        for entry in sorted(state.rglob("*")):
            if entry.is_file():
                target = out / "red-native-cut" / entry.relative_to(state)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(entry.read_bytes())
        observation = dict(label="current values alone do not reconcile native source balance",
            native=native_result, original_value_audit=value_result, native_source_amount=61,
            complete_retained_inventory_unchanged=True)
        observations.append(observation)
        (out / "red-observation.json").write_text(json.dumps(observation, indent=2) + "\n")
        if not red:
            # Restore the native seed before running the independent balance cases.
            struct.pack_into("<Q", encoded, 106, 60)
            encoded[24:56] = hashlib.sha256(encoded[56:]).digest()
            source.write_bytes(encoded)
            green_checks(state, out, observations, fixture, operator, native_fixture, independent, environment)
    assert fingerprint(native / "src") == native_before and audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    result = dict(completed=True, red=red, observations=observations, skips=0,
        native_source_inputs=native_before, audit_source_inputs=inputs, owned_inputs=owned,
        executable_sha256={name: hashlib.sha256(Path(path).read_bytes()).hexdigest()
            for name, path in dict(native_fixture=native_fixture, economic_fixture=fixture, operator=operator,
                **({"independent": independent} if independent else {})).items()},
        original_native_providers=True, modeled_economic_witnesses=True,
        accounting_activated=False, full_R7_qualified=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(result, sort_keys=True, indent=2) + "\n")
    print(json.dumps(dict(completed=True, red=red, source_cases=len(observations), skips=0)), flush=True)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts, red=args.red)
