#!/usr/bin/env python3
"""Native escrow/claim money comparisons; component evidence, never release."""
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
import build_restore_qualifier as qualifier


def build_independent(out):
    source = out / "independent.cpp"
    source.write_text(r'''#include "qualify_flatfile_auction_money.h"
#include <iostream>
int main(int argc, char **argv) {
    try {
        using namespace restore_economic_authority;
        need(argc == 3);
        const std::string mode = argv[1];
        const std::filesystem::path root = argv[2];
        audit_budget budget;
        if (mode == "decode") {
            std::vector<bool> published;
            auto catalog = restore_native_auction::decode_catalog(
                file_bytes(root / "domains", "auction_catalog", restore_native_auction::catalog_limit),
                [&](const auto &receipt) { published.push_back(receipt.event_published); });
            auto sources = restore_native_auction::decode_sources(
                file_bytes(root / "domains", "auction_claim_sources", restore_native_auction::source_limit));
            std::cout << "{\"catalog_revision\":" << catalog.revision
                << ",\"listings\":" << catalog.listings << ",\"operations\":" << catalog.operations
                << ",\"source_revision\":" << sources.revision << ",\"source_rows\":" << sources.rows
                << ",\"holdings\":[";
            bool comma = false;
            for (const auto &holding : catalog.holdings) {
                std::cout << (comma ? "," : "") << '[' << holding.kind << ',' << holding.id
                    << ',' << holding.amount << ',' << holding.revision << ']';
                comma = true;
            }
            std::cout << "],\"event_published\":[";
            comma = false;
            for (bool flag : published) {
                std::cout << (comma ? "," : "") << (flag ? "true" : "false");
                comma = true;
            }
            std::cout << "]}\n";
            return 0;
        }
        if (mode == "bytes") budget.remaining_bytes = 0;
        if (mode == "files") budget.remaining_files = 0;
        if (mode == "entries") budget.remaining_entries = 0;
        if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
        auto result = restore_auction_money::audit(root, budget);
        std::cout << result.valid() << " " << result.verified() << " " << result.finding_count << "\n";
        return result.valid() ? 0 : 1;
    } catch (...) { std::cerr << "native_restore_qualification_failed\n"; return 1; }
}
''', encoding="utf-8")
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts")]
    independent = out / "independent"
    command = [*flags, str(source), "-lcrypto", "-o", str(independent)]
    subprocess.run(command, check=True, timeout=120)
    dependency = subprocess.check_output([*flags, "-MM", str(source)], text=True)
    assert "/src/" not in dependency
    (out / "independent.d").write_text(dependency)
    (out / "independent-build.json").write_text(json.dumps(command, indent=2) + "\n")
    return independent


def green_checks(state, out, observations, fixture, operator, native_fixture, independent, environment, original):
    catalog, sources = state / "domains/auction_catalog", state / "domains/auction_claim_sources"
    original_sources = sources.read_bytes()

    def whole():
        info = state.lstat()
        entries = retained(state)
        for name, row in entries.items():
            if stat.S_ISLNK(row["mode"]):
                row["symlink_target"] = os.readlink(state / name)
        return dict(root_mode=info.st_mode, root_inode=info.st_ino,
            root_links=info.st_nlink, root_mtime_ns=info.st_mtime_ns, entries=entries)

    def retain_input(inventory):
        encoded = json.dumps(inventory, sort_keys=True, indent=2) + "\n"
        target = out / "inputs" / f"case-{len(observations):03}.json"
        target.parent.mkdir(mode=0o700, exist_ok=True)
        target.write_text(encoded, encoding="utf-8")
        objects = out / "inputs/objects"
        objects.mkdir(mode=0o700, exist_ok=True)
        for name, row in inventory["entries"].items():
            if stat.S_ISREG(row["mode"]):
                payload = (state / name).read_bytes()
                assert hashlib.sha256(payload).hexdigest() == row["sha256"]
                blob = objects / row["sha256"]
                if blob.exists():
                    assert blob.read_bytes() == payload
                else:
                    blob.write_bytes(payload)
        return dict(path=str(target.relative_to(out)), sha256=hashlib.sha256(encoded.encode()).hexdigest())

    def check(label, codes=(), *, refusal=False, verified=False):
        before = whole()
        source_cut = retain_input(before)
        parsed = subprocess.run([str(operator), "--economic-auction-money-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": operator wrote authority"
        sanitized = subprocess.run([str(independent), "normal", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": independent reader wrote authority"
        expected = 1 if refusal or codes else 0
        assert parsed.returncode == sanitized.returncode == expected, (label, parsed, sanitized)
        report = None
        if refusal:
            assert not parsed.stdout and not sanitized.stdout
            assert parsed.stderr == sanitized.stderr == "native_restore_qualification_failed\n"
        else:
            assert not parsed.stderr and not sanitized.stderr, (label, parsed, sanitized)
            report = json.loads(parsed.stdout)
            assert {row["code"] for row in report["findings"]} == set(codes), (label, codes, report)
            assert report["current_auction_money_values_verified"] == verified
            for flag in ("account_origins_verified", "claim_source_attribution_verified", "other_money_domains_verified",
                         "native_holdings_compared", "full_R7_qualified", "release_qualified", "cross_epoch_continuity_verified"):
                assert not report[flag]
            assert sanitized.stdout == f"{int(expected == 0)} {int(verified)} {report['finding_count']}\n"
            assert not any(secret in parsed.stdout for secret in ("Seller", "Buyer", "native-fixture", "fixture-object"))
        observations.append(dict(label=label, exit=parsed.returncode, report=report, source_cut=source_cut,
            sanitized_exit=sanitized.returncode, complete_retained_inventory_unchanged=True))
        print("AUCTION_MONEY " + label, flush=True)
        return report

    def frame(value, body=None, revision=None, version=None):
        header = bytearray(value[:56])
        payload = value[56:] if body is None else body
        struct.pack_into("<I", header, 12, len(payload))
        header[24:56] = hashlib.sha256(payload).digest()
        if revision is not None:
            struct.pack_into("<Q", header, 16, revision)
        if version is not None:
            struct.pack_into("<I", header, 8, version)
        return bytes(header) + payload

    def changed(value, offset, fmt, replacement):
        data = bytearray(value)
        struct.pack_into(fmt, data, offset, replacement)
        return frame(data)

    def differential(label, cat=original, src=original_sources, valid=True, published=None):
        catalog.write_bytes(cat)
        sources.write_bytes(src)
        before = whole()
        source_cut = retain_input(before)
        native = subprocess.run([str(native_fixture), "probe", str(state)], capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": original native probe wrote authority"
        pure = subprocess.run([str(independent), "decode", str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        assert whole() == before, label + ": pure decoder wrote authority"
        assert native.returncode == pure.returncode == (0 if valid else 1), (label, native, pure)
        decoded = None
        if valid:
            assert not native.stderr and not pure.stderr
            decoded = json.loads(native.stdout)
            assert decoded == json.loads(pure.stdout), (label, native, pure)
            if published is not None:
                assert decoded["event_published"] == published, (label, decoded)
        else:
            assert not native.stdout and not pure.stdout
            assert native.stderr == "native_auction_decode_refused\n"
            assert pure.stderr == "native_restore_qualification_failed\n"
        observations.append(dict(label=label, differential=True, exit=pure.returncode,
            decoded=decoded, source_cut=source_cut, complete_retained_inventory_unchanged=True))
        print("AUCTION_DECODE " + label, flush=True)

    report = check("stale native escrow differs from economic tail", ("native_balance_mismatch",))
    assert report["findings"][0]["observed"] == dict(native=[900, 0, 0, 0], expected=[70, 0, 0, 0],
        native_revision=3, expected_revision=3)
    catalog.write_bytes(original)
    report = check("original native escrow and claim match baseline clocks", verified=True)
    assert report["compared_accounts"] == 2 and report["native_open_escrows"] == report["native_pending_claims"] == 1

    # Find variable fields from the original encoded listing, never guess offsets.
    strings, position = [], 108
    for _ in range(6):
        length = struct.unpack_from("<I", original, position)[0]
        strings.append((position, length))
        position += 4 + length
    blob_at = position
    item_count = blob_at + 4 + struct.unpack_from("<I", original, blob_at)[0]
    item = item_count + 2
    money_count = item + 25
    money = money_count + 4
    operation_count = money + 20
    operation = operation_count + 4
    result_at = operation + 52
    assert len(original) == result_at + 321

    differential("complete version 2 native catalog and sources", published=[False])
    differential("legacy version 1 omits event publication byte", cat=frame(original, original[56:-1], version=1), published=[True])
    differential("version 2 explicitly published receipt", cat=changed(original, result_at + 320, "<B", 1), published=[True])
    differential("native receipt allows event code and nonzero padding",
        cat=changed(changed(original, result_at + 1, "<B", 255), result_at + 319, "<B", 123))
    differential("native item vnum zero is valid", cat=changed(original, item + 16, "<I", 0))
    differential("native signed negative prices are structurally valid", cat=changed(original, 76, "<q", -(1 << 63)))
    differential("native maximum claim balance and clocks", cat=changed(changed(original, money + 4, "<q", (1 << 63) - 1), money + 12, "<Q", (1 << 64) - 1))
    differential("native empty pickup catalog", cat=frame(original, original[56:money_count] + struct.pack("<I", 0) + original[operation_count:]))
    differential("native empty source catalog", src=frame(original_sources, struct.pack("<I", 0)))
    differential("highest native receipt action is valid", cat=changed(original, result_at, "<B", 6))
    differential("source maximum native amount is valid", src=changed(original_sources, 106, "<Q", (1 << 31) - 1))
    differential("source consumed by another operation is structurally valid", src=changed(original_sources, 114, "<B", 7))
    mutations = [("listing ID zero", 60, "<I", 0), ("seller PID zero", 64, "<I", 0),
        ("listing status zero", 72, "<I", 0), ("listing status above removed", 72, "<I", 4),
        ("listing clock zero", 92, "<Q", 0), ("listing count overflow", 56, "<I", 262145),
        ("item UID zero", item, "<Q", 0), ("item clock zero", item + 8, "<Q", 0),
        ("negative item vnum", item + 16, "<I", (1 << 32) - 1), ("nonboolean item claimed", item + 24, "<B", 2),
        ("item count zero", item_count, "<H", 0), ("item count overflow", item_count, "<H", 10),
        ("blob length overflow", blob_at, "<I", 32769), ("pickup PID zero", money, "<I", 0),
        ("negative pickup amount", money + 4, "<q", -1), ("pickup clock zero", money + 12, "<Q", 0),
        ("pickup count overflow", money_count, "<I", 262145), ("receipt count overflow", operation_count, "<I", 1048577),
        ("receipt action zero", result_at, "<B", 0), ("receipt action overflow", result_at, "<B", 7),
        ("receipt items overflow", result_at + 142, "<H", 10), ("negative receipt claim credit", result_at + 160, "<q", -1),
        ("nonboolean receipt event publication", result_at + 320, "<B", 2)]
    for label, offset, fmt, value in mutations:
        differential(label, cat=changed(original, offset, fmt, value), valid=False)
    for index, maximum in enumerate((50, 32, 32, 255, 1024, 8192)):
        differential("native string length limit " + str(index), cat=changed(original, strings[index][0], "<I", maximum + 1), valid=False)
        differential("native string rejects NUL " + str(index), cat=changed(original, strings[index][0] + 4, "<B", 0), valid=False)
    differential("seller account must be canonical", cat=changed(original, strings[0][0] + 4, "<B", ord("N")), valid=False)
    differential("empty blob matches original native refusal", cat=frame(original, original[56:blob_at] + struct.pack("<I", 0) + original[item_count:]), valid=False)
    differential("duplicate item UID", cat=frame(original, original[56:item_count] + struct.pack("<H", 2) + original[item:item + 25] * 2 + original[money_count:]), valid=False)
    differential("duplicate listing ID", cat=frame(original, struct.pack("<I", 2) + original[60:money_count] * 2 + original[money_count:]), valid=False)
    lower = bytearray(original[60:money_count])
    struct.pack_into("<I", lower, 0, 1)
    differential("unsorted listing IDs", cat=frame(original, struct.pack("<I", 2) + original[60:money_count] + lower + original[money_count:]), valid=False)
    differential("duplicate pickup PID", cat=frame(original, original[56:money_count] + struct.pack("<I", 2) + original[money:money + 20] * 2 + original[operation_count:]), valid=False)
    differential("unsorted pickup PIDs", cat=frame(original, original[56:money_count] + struct.pack("<I", 2) + original[money:money + 20] + struct.pack("<IqQ", 1, 1, 1) + original[operation_count:]), valid=False)
    differential("duplicate receipt operation", cat=frame(original, original[56:operation_count] + struct.pack("<I", 2) + original[operation:] * 2), valid=False)
    differential("zero receipt operation", cat=frame(original, original[56:operation] + bytes(16) + original[operation + 16:]), valid=False)
    for version in (0, 3):
        differential("unsupported catalog version " + str(version), cat=frame(original, version=version), valid=False)
    differential("zero catalog revision", cat=frame(original, revision=0), valid=False)
    differential("catalog trailing byte", cat=frame(original, original[56:] + b"x"), valid=False)
    for offset in (0, 12, 24):
        data = bytearray(original)
        data[offset] ^= 1
        differential("catalog magic size or digest corrupt " + str(offset), cat=data, valid=False)
    for label, offset, fmt, value in (("source count overflow", 56, "<I", 262145),
            ("source slot zero", 76, "<H", 0), ("source PID zero", 94, "<I", 0),
            ("source mapping zero", 98, "<Q", 0), ("source amount zero", 106, "<Q", 0),
            ("source amount overflow", 106, "<Q", (1 << 31))):
        differential(label, src=changed(original_sources, offset, fmt, value), valid=False)
    for label, offset in (("source operation zero", 60), ("source lineage zero", 78)):
        data = bytearray(original_sources)
        data[offset:offset + 16] = bytes(16)
        differential(label, src=frame(data), valid=False)
    data = bytearray(original_sources)
    data[114:130] = data[60:76]
    differential("source cannot consume itself", src=frame(data), valid=False)
    differential("duplicate source key", src=frame(original_sources, struct.pack("<I", 2) + original_sources[60:] * 2), valid=False)
    lower = bytearray(original_sources[60:])
    lower[0] = 5
    differential("unsorted source keys", src=frame(original_sources, struct.pack("<I", 2) + original_sources[60:] + lower), valid=False)
    differential("unsupported source version", src=frame(original_sources, version=2), valid=False)
    differential("zero source revision", src=frame(original_sources, revision=0), valid=False)
    differential("source trailing byte", src=frame(original_sources, original_sources[56:] + b"x"), valid=False)
    catalog.write_bytes(original)
    sources.write_bytes(original_sources)
    catalog.write_bytes(frame(original, revision=(1 << 64) - 1))
    check("catalog clock does not substitute for escrow or claim clocks", verified=True)

    for label, value in (("negative signed escrow cannot wrap", -1), ("minimum signed escrow cannot wrap", -(1 << 63))):
        catalog.write_bytes(changed(original, 76, "<q", value))
        report = check(label, ("native_balance_outside_economic_range",))
        assert report["findings"][0]["observed"]["native"] == [value, 0, 0, 0]
    for offset, value, label in ((92, (1 << 64) - 1, "escrow clock mismatch"), (money + 12, 5, "claim clock mismatch")):
        catalog.write_bytes(changed(original, offset, "<Q", value))
        check(label, ("native_revision_mismatch",))
    catalog.write_bytes(changed(original, money + 4, "<q", 61))
    check("claim balance mismatch", ("native_balance_mismatch",))
    catalog.write_bytes(changed(original, 68, "<I", 0))
    check("unbid open listing holds zero escrow", ("native_balance_mismatch",))
    catalog.write_bytes(changed(original, 72, "<I", 2))
    check("closed listing is not a current escrow holding", ("native_domain_missing",))
    catalog.write_bytes(changed(original, money, "<I", (1 << 32) - 1))
    check("unmapped native claim cannot alias mapped signed PID", ("native_domain_unmapped", "native_domain_missing"))
    catalog.write_bytes(original)
    catalog.unlink()
    check("missing native catalog exposes both missing accounts", ("native_domain_missing",))
    catalog.write_bytes(original)
    sources.unlink()
    report = check("missing claim sources cannot establish provenance", verified=True)
    assert not report["claim_sources_present"] and not report["claim_source_attribution_verified"]
    sources.write_bytes(original_sources)
    for path in (catalog, sources):
        path.chmod(0o644)
        check("nonprivate native " + path.name, refusal=True)
        path.chmod(0o600)
        alias = path.with_name("native-hardlink")
        os.link(path, alias)
        check("hardlinked native " + path.name, refusal=True)
        alias.unlink()
        content = path.read_bytes()
        path.unlink()
        path.symlink_to(alias)
        check("symlink native " + path.name, refusal=True)
        path.unlink()
        os.mkfifo(path, 0o600)
        check("FIFO native " + path.name, refusal=True)
        path.unlink()
        path.write_bytes(content)
    for name in (".critical-authority-transaction", ".player-domain-transaction", ".currency-transaction"):
        path = state / "domains" / name
        path.write_bytes(b"pending-native-intent")
        check("pending native journal " + name, refusal=True)
        path.unlink()
    lock = state / "domains/.critical-authority.lock"
    lock.unlink()
    check("missing authority lock is never recreated", refusal=True)
    assert not lock.exists()
    restored = subprocess.run([str(native_fixture), "public-query", str(state)], capture_output=True, text=True, timeout=30)
    assert restored.returncode == 0 and lock.exists(), restored
    holder = subprocess.Popen([str(fixture), str(state), "hold-authority-lock"], env=environment,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        assert select.select([holder.stdout], [], [], 10)[0]
        assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
        check("exclusive native writer excludes current audit", refusal=True)
    finally:
        stdout, stderr = holder.communicate("release\n", timeout=10)
        assert holder.returncode == 0 and not stdout and not stderr
    for mode in ("bytes", "files", "entries", "deadline"):
        before = whole()
        cut = retain_input(before)
        refused = subprocess.run([str(independent), mode, str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        assert refused.returncode == 1 and not refused.stdout and refused.stderr == "native_restore_qualification_failed\n"
        assert whole() == before
        observations.append(dict(label="cooperative auction audit budget " + mode, exit=1,
            source_cut=cut, complete_retained_inventory_unchanged=True))
    extras = b"".join(struct.pack("<IqQ", pid, 1, 1) for pid in range(500, 621))
    catalog.write_bytes(frame(original, original[56:money_count] + struct.pack("<I", 122) + extras + original[money:]))
    report = check("finding cap retains exact unmapped total", ("native_domain_unmapped",))
    assert report["finding_count"] == 121 and len(report["findings"]) == 100 and report["findings_truncated"]
    extras = b"".join(struct.pack("<IqQ", pid, 1, 1) for pid in range(500, 33268))
    catalog.write_bytes(frame(original, original[56:money_count] + struct.pack("<I", 32768) + extras + original[operation_count:]))
    check("native account scan cap refuses without partial findings", refusal=True)

    def replace(mode):
        assert state.resolve().is_relative_to(out)
        shutil.rmtree(state)
        state.mkdir(mode=0o700)
        ran = subprocess.run([str(native_fixture), "seed", str(state)], capture_output=True, text=True, timeout=30)
        assert ran.returncode == 0 and not ran.stderr, ran
        if mode:
            ran = subprocess.run([str(fixture), str(state), mode], env=environment, capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, ran
    replace("baseline-auction-money-zero")
    catalog.write_bytes(changed(original, 68, "<I", 0))
    check("unbid current escrow matches zero baseline", verified=True)
    replace("baseline-auction-money-closed")
    catalog.write_bytes(changed(original, 72, "<I", 3))
    report = check("removed listing closing price does not own retired zero escrow", verified=True)
    assert report["retired_current_epoch_accounts"] == 1 and report["compared_accounts"] == 1
    replace("baseline-auction-money-retired")
    check("retired lifetime cannot own nonzero current escrow", ("retired_account_balance_nonzero", "native_domain_unmapped"))
    replace("baseline-auction-money-epochs")
    catalog.write_bytes(changed(changed(original, 76, "<q", 90), money + 4, "<q", 80))
    report = check("catalog current epoch wins over lexicographic ID order", verified=True)
    assert report["epoch"] == "19000000000000000000000000000000" and report["prior_epoch_auction_money_accounts"] == 2
    replace("baseline")
    check("active mappings with missing economic history", ("economic_history_missing",))
    replace(None)
    report = check("inactive native catalog retains unknown mapping findings", ("native_domain_unmapped",))
    assert not report["initialized"]
    assert state.resolve().is_relative_to(out)
    shutil.rmtree(state)
    state.mkdir(mode=0o700)
    report = check("empty inactive authority remains an empty observation")
    assert not report["initialized"] and not report["auction_money_holdings_compared"]


def build_native_oracle(native, out):
    sources = []
    for name in qualifier.SOURCES:
        if name == "flatfile_auction_repository":
            continue  # The fixture compiles this exact original file by inclusion.
        matches = list((native / "src").rglob(name + ".c"))
        assert len(matches) == 1
        sources.append(str(matches[0]))
    sources.append(str(native / "src/economy/auction_money_claim_accounting.c"))
    fixture = out / "native"
    command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-I" + str(native / "src"), "-I" + str(native / "src/no_mysql"),
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "tests/async/flatfile_auction_money_fixture.cpp", *sources,
        "-lcrypto", "-lz", "-pthread", "-o", str(fixture)]
    subprocess.run(command, cwd=ROOT, check=True, timeout=300)
    (out / "native-build.json").write_text(json.dumps(command, indent=2) + "\n")
    return fixture


def qualify(native_source, artifacts, *, fixture=None, operator=None, native_fixture=None, red=False):
    native, out = native_source.resolve(), artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    native_before, inputs = fingerprint(native / "src"), audit_source_inputs()
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_auction_money_cases.py",
        "tests/async/flatfile_auction_money_fixture.cpp",
        "tests/async/flatfile_restore_authority_fixture.cpp",
        "tests/async/test_flatfile_restore_economic_authority.py")}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "economic-fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    native_fixture = native_fixture or build_native_oracle(native, out)
    independent = None if red else build_independent(out)
    observations = []
    with tempfile.TemporaryDirectory(prefix="auction-money-", dir=out) as temporary:
        state = Path(temporary)
        seed = subprocess.run([str(native_fixture), "seed", str(state)],
            capture_output=True, text=True, timeout=30)
        assert seed.returncode == 0 and not seed.stderr, seed
        economic = subprocess.run([str(fixture), str(state), "baseline-auction-money"],
            env=environment, capture_output=True, text=True, timeout=90)
        assert economic.returncode == 0 and not economic.stderr, economic
        catalog = state / "domains/auction_catalog"
        original = catalog.read_bytes()
        changed = bytearray(original)
        struct.pack_into("<q", changed, 76, 900)
        changed[24:56] = hashlib.sha256(changed[56:]).digest()
        catalog.write_bytes(changed)
        inventory = retained(state)
        probe = subprocess.run([str(native_fixture), "public-query", str(state)],
            capture_output=True, text=True, timeout=30)
        assert probe.returncode == 0 and not probe.stderr and retained(state) == inventory, probe
        assert json.loads(probe.stdout)["holdings"][0] == [4, (1 << 32) - 1, 900, 3]
        old = subprocess.run([str(operator), "--economic-money-history-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert old.returncode == 0 and not old.stderr and retained(state) == inventory, old
        assert json.loads(old.stdout)["same_epoch_transition_continuity_verified"]
        for entry in sorted(state.rglob("*")):
            if entry.is_file():
                target = out / "red-native-cut" / entry.relative_to(state)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(entry.read_bytes())
        observation = dict(label="retained history accepts stale native escrow",
            original_money_history=json.loads(old.stdout), native_current=json.loads(probe.stdout),
            expected_economic_escrow=[70, 0, 0, 0], complete_retained_inventory_unchanged=True)
        observations.append(observation)
        (out / "red-observation.json").write_text(json.dumps(observation, indent=2) + "\n")
        if not red:
            green_checks(state, out, observations, fixture, operator, native_fixture, independent, environment, original)
    assert fingerprint(native / "src") == native_before and audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    report = dict(completed=True, red=red, observations=observations, skips=0,
        native_source_inputs=native_before, audit_source_inputs=inputs, owned_inputs=owned,
        native_mutation_providers_original=True, modeled_economic_witnesses=True,
        executable_sha256={name: hashlib.sha256(Path(path).read_bytes()).hexdigest()
            for name, path in dict(native_fixture=native_fixture, economic_fixture=fixture, operator=operator,
                **({"independent": independent} if independent else {})).items()},
        accounting_activated=False, full_R7_qualified=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(dict(completed=True, red=red, auction_money_cases=len(observations), skips=0)), flush=True)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts, red=args.red)
