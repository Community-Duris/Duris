#!/usr/bin/env python3
"""Native-file differential checks for the independent holding decoder."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from test_flatfile_restore_economic_authority import ROOT
from test_flatfile_restore_baseline_markers import audit_source_inputs, fingerprint, retained
import build_restore_qualifier as qualifier


def qualify(native_source, artifacts):
    native, out = native_source.resolve(), artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    before = fingerprint(native / "src")
    inputs = audit_source_inputs()
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_native_domain_cases.py",
        "tests/async/flatfile_native_domains_fixture.cpp")}
    sources = []
    for name in qualifier.SOURCES:
        matches = list((native / "src").rglob(name + ".c"))
        assert len(matches) == 1
        sources.append(str(matches[0]))
    fixture = out / "native"
    command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-I" + str(native / "src"), "-I" + str(native / "src/no_mysql"),
        "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
        "tests/async/flatfile_native_domains_fixture.cpp", *sources,
        "-lcrypto", "-lz", "-pthread", "-o", str(fixture)]
    subprocess.run(command, cwd=ROOT, check=True, timeout=300)
    (out / "native-build.json").write_text(json.dumps(command, indent=2) + "\n")
    source = out / "independent.cpp"
    source.write_text(r'''#include "qualify_flatfile_native_domains.h"
#include <iostream>
int main(int argc, char **argv) {
    try {
        restore_economic_authority::need(argc == 2);
        auto directory = std::filesystem::path(argv[1]) / "domains";
        auto wallet = restore_native_domains::wallet(
            restore_economic_authority::file_bytes(directory, "player-11.domain", 65536), 11);
        auto bank = restore_native_domains::bank(
            restore_economic_authority::file_bytes(directory, "bank-native-fixture-1.domain", 65536),
            "native-fixture", 1);
        restore_economic_authority::need(wallet.account == bank.account &&
                                        wallet.racewar == bank.racewar);
        std::cout << "{\"wallet_revision\":" << wallet.revision
                  << ",\"bank_revision\":" << bank.revision << ",\"wallet\":[";
        for (size_t i = 0; i < 4; ++i) std::cout << (i ? "," : "") << wallet.balance[i];
        std::cout << "],\"bank\":[";
        for (size_t i = 0; i < 4; ++i) std::cout << (i ? "," : "") << bank.balance[i];
        std::cout << "]}\n";
        return 0;
    } catch (...) { std::cerr << "native_domain_decode_refused\n"; return 1; }
}
''', encoding="utf-8")
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I" + str(ROOT / "scripts")]
    audit = out / "independent"
    subprocess.run([*flags, str(source), "-lcrypto", "-o", str(audit)], check=True, timeout=120)
    dependency = subprocess.check_output([*flags, "-MM", str(source)], text=True)
    assert "/src/" not in dependency
    (out / "independent.d").write_text(dependency)
    (out / "independent-build.json").write_text(json.dumps(flags, indent=2) + "\n")
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
        UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    observations = []
    with tempfile.TemporaryDirectory(prefix="native-domains-", dir=out) as temporary:
        state = Path(temporary)
        seed = subprocess.run([str(fixture), "seed", str(state)], capture_output=True, text=True, timeout=30)
        assert seed.returncode == 0 and not seed.stderr, seed
        wallet_path = state / "domains/player-11.domain"
        bank_path = state / "domains/bank-native-fixture-1.domain"
        original_wallet, original_bank = wallet_path.read_bytes(), bank_path.read_bytes()

        def check(label, wallet=original_wallet, bank=original_bank, valid=True):
            wallet_path.write_bytes(wallet)
            bank_path.write_bytes(bank)
            inventory = retained(state)
            native_ran = subprocess.run([str(fixture), "probe", str(state)],
                capture_output=True, text=True, timeout=30)
            assert retained(state) == inventory, (label, "native oracle changed authority")
            independent = subprocess.run([str(audit), str(state)], env=environment,
                capture_output=True, text=True, timeout=30)
            assert retained(state) == inventory, (label, "independent reader changed authority")
            expected = 0 if valid else 1
            assert native_ran.returncode == independent.returncode == expected, (label, native_ran, independent)
            if valid:
                assert not native_ran.stderr and not independent.stderr
                assert json.loads(native_ran.stdout) == json.loads(independent.stdout), label
            else:
                assert not native_ran.stdout and not independent.stdout
                assert native_ran.stderr == independent.stderr == "native_domain_decode_refused\n"
            cut = out / f"case-{len(observations):02}"
            cut.mkdir()
            (cut / "player.domain").write_bytes(wallet)
            (cut / "bank.domain").write_bytes(bank)
            observations.append(dict(label=label, native_exit=native_ran.returncode,
                independent_exit=independent.returncode, native_stdout=native_ran.stdout,
                independent_stdout=independent.stdout, complete_retained_inventory_unchanged=True,
                wallet_sha256=hashlib.sha256(wallet).hexdigest(), bank_sha256=hashlib.sha256(bank).hexdigest()))

        def frame(original, body=None, version=None, revision=None):
            header = bytearray(original[:56])
            body = original[56:] if body is None else body
            struct.pack_into("<I", header, 12, len(body))
            header[24:56] = hashlib.sha256(body).digest()
            if version is not None:
                struct.pack_into("<I", header, 8, version)
            if revision is not None:
                struct.pack_into("<Q", header, 16, revision)
            return bytes(header) + body

        account_size = struct.unpack_from("<I", original_wallet, 60)[0]
        wallet_revision_at = 56 + 4 + 4 + account_size + 1
        coins_at = wallet_revision_at + 24
        recent_count_at = coins_at + 32 + 24
        recent_count = struct.unpack_from("<I", original_wallet, recent_count_at)[0]
        zones_count_at = recent_count_at + 4 + recent_count * 8
        zones_count = struct.unpack_from("<I", original_wallet, zones_count_at)[0]
        stats_at = zones_count_at + 4 + zones_count * 4
        operations_at = stats_at + 28
        check("actual native v4 wallet and bank")
        body = original_wallet[56:]
        for version in (1, 2, 3, 4):
            vbody = body if version >= 3 else body[:stats_at - 56] + body[operations_at - 56:]
            if version == 1:
                vbody = body[:stats_at - 56]
            check("native domain version " + str(version), frame(original_wallet, vbody, version),
                  frame(original_bank, version=version))
        for location, value, label, valid in (
                (wallet_revision_at, 42, "wallet revision distinct from file clock", True),
                (wallet_revision_at + 8, 100, "nonmoney file clock is not wallet revision", True),
                (coins_at, (1 << 64) - 1, "native unsigned money remains exact", True),
                (recent_count_at + 4, 0, "nonpositive recent death refuses", False),
                (recent_count_at + 4, (1 << 64) - 1, "negative signed recent death refuses", False),
                (recent_count_at + 4, 50, "unsorted recent deaths refuse", False)):
            changed = bytearray(original_wallet)
            struct.pack_into("<Q", changed, location, value)
            revision = max(struct.unpack_from("<3Q", changed, wallet_revision_at)[0:3] + (1,))
            check(label, frame(changed, changed[56:], revision=revision), valid=valid)
        changed = bytearray(original_wallet)
        struct.pack_into("<Q", changed, wallet_revision_at, 42)
        check("wallet file envelope clock must match maximum native domain clock",
              frame(changed, changed[56:]), valid=False)
        changed = bytearray(original_wallet)
        struct.pack_into("<i", changed, zones_count_at + 8, 1)
        check("duplicate completed zone refuses", frame(changed, changed[56:]), valid=False)
        for value, label in ((101, "out of range base stat"), (1, "stat without native revision")):
            changed = bytearray(original_wallet)
            struct.pack_into("<h", changed, stats_at + 8, value)
            check(label, frame(changed, changed[56:]), valid=False)
        for field, value, label in ((0, 12, "wrong native PID"),):
            changed = bytearray(original_wallet)
            struct.pack_into("<I", changed, 56 + field, value)
            check(label, frame(changed, changed[56:]), valid=False)
        altered = bytearray(original_wallet)
        altered[64] = ord("N")
        check("wallet account must already be canonical", frame(altered, altered[56:]), valid=False)
        altered[64] = ord("x")
        check("wallet must match requested native account", frame(altered, altered[56:]), valid=False)
        altered = bytearray(original_wallet)
        altered[wallet_revision_at - 1] = 2
        check("wallet must match requested racewar", frame(altered, altered[56:]), valid=False)
        altered = bytearray(original_bank)
        altered[60] = ord("N")
        check("native bank body account canonicalizes", bank=frame(altered, altered[56:]))
        altered = bytearray(original_bank)
        altered[60] = ord("x")
        check("bank locator name must match body", bank=frame(altered, altered[56:]), valid=False)
        for index in (56 + 4 + len("native-fixture"),):
            altered = bytearray(original_bank)
            altered[index] = 2
            check("bank racewar mismatch " + str(index), bank=frame(altered, altered[56:]), valid=False)
        for payload, label in ((original_wallet + b"x", "trailing native wallet payload"),
                               (original_wallet[:-1], "truncated native wallet payload")):
            check(label, frame(original_wallet, payload[56:]), valid=False)
        for version in (0, 5):
            check("unknown native wallet version " + str(version),
                  frame(original_wallet, version=version), valid=False)
        for original, bank_case in ((original_wallet, False), (original_bank, True)):
            kind = "bank" if bank_case else "wallet"
            for index, label in ((0, "magic"), (12, "size"), (24, "checksum")):
                altered = bytearray(original)
                altered[index] ^= 1
                check("invalid native " + kind + " " + label,
                      **{kind: bytes(altered)}, valid=False)
            check("zero native " + kind + " envelope revision",
                  **{kind: frame(original, revision=0)}, valid=False)
        check("native bank unsigned revision remains exact",
              bank=frame(original_bank, revision=(1 << 64) - 1))
        altered = bytearray(original_wallet)
        struct.pack_into("<Q", altered, stats_at, 101)
        struct.pack_into("<h", altered, stats_at + 8, 100)
        check("native base stat clock drives envelope but not money",
              frame(altered, altered[56:], revision=101))
        for index, maximum, label in ((recent_count_at, 20, "recent deaths"),
                                      (zones_count_at, 1024, "completed zones"),
                                      (operations_at, 512, "retained receipts")):
            altered = bytearray(original_wallet)
            struct.pack_into("<I", altered, index, maximum + 1)
            check("bounded native " + label, frame(altered, altered[56:]), valid=False)
        operation = bytes([9]) + bytes(15) + bytes(32) + struct.pack("<IH", 0, 80) + bytes(80)
        for version in (2, 3, 4):
            base = body if version >= 3 else body[:stats_at - 56] + body[operations_at - 56:]
            tail = operation + (struct.pack("<II", 2, 5) if version == 4 else b"")
            check("native retained domain receipt v" + str(version),
                  frame(original_wallet, base[:-4] + struct.pack("<I", 1) + tail, version))
        for tail, label in ((operation[:16] + operation[16:] + struct.pack("<II", 65, 5), "quest slot overflow"),
                            (operation + struct.pack("<II", 2, 0), "quest pair mismatch"),
                            (bytes(16) + operation[16:] + bytes(8), "zero retained operation ID")):
            check(label, frame(original_wallet, body[:-4] + struct.pack("<I", 1) + tail), valid=False)
        duplicate = operation + bytes(8)
        check("duplicate retained domain operation IDs",
              frame(original_wallet, body[:-4] + struct.pack("<I", 2) + duplicate * 2), valid=False)
        for changed_operation, label in (
                (operation[:48] + struct.pack("<I", 1) + operation[52:] + struct.pack("<II", 2, 5),
                 "quest receipt result must be success"),
                (operation[:52] + struct.pack("<H", 2049) + bytes(2049) + bytes(8),
                 "native receipt payload size remains bounded")):
            check(label, frame(original_wallet, body[:-4] + struct.pack("<I", 1) + changed_operation),
                  valid=False)
        check("healthy native domain source restored")
        # The public loader is an oracle only. Even this inactive read creates
        # its missing authority lock; an operator audit must not call it.
        lock_path = state / "domains/.critical-authority.lock"
        assert lock_path.is_file()
        lock_path.unlink()
        inventory = retained(state)
        independent = subprocess.run([str(audit), str(state)], env=environment,
            capture_output=True, text=True, timeout=30)
        assert independent.returncode == 0 and not independent.stderr
        assert retained(state) == inventory and not lock_path.exists()
        native_ran = subprocess.run([str(fixture), "probe", str(state)],
            capture_output=True, text=True, timeout=30)
        assert native_ran.returncode == 0 and not native_ran.stderr
        assert json.loads(native_ran.stdout) == json.loads(independent.stdout)
        changed = retained(state)
        assert set(changed) - set(inventory) == {"domains/.critical-authority.lock"}
        assert all(changed[name] == row for name, row in inventory.items() if name != "domains")
        observations.append(dict(label="native loader creates lock; independent decoder stays read only",
            native_exit=0, independent_exit=0, native_stdout=native_ran.stdout,
            independent_stdout=independent.stdout, native_loader_created_missing_lock=True,
            independent_complete_retained_inventory_unchanged=True, accounting_activated=False))
    assert fingerprint(native / "src") == before and audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    report = dict(completed=True, native_source_inputs=before, audit_source_inputs=inputs,
        owned_inputs=owned, observations=observations, skips=0, accounting_activated=False,
        native_domains_decoded=True, native_holdings_compared=False, full_R7_qualified=False,
        release_qualified=False, fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        independent_sha256=hashlib.sha256(audit.read_bytes()).hexdigest())
    (out / "evidence.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(dict(completed=True, native_domain_cases=len(observations), skips=0)), flush=True)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts)
