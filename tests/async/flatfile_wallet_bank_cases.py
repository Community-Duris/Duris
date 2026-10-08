#!/usr/bin/env python3
"""Native current wallet/bank comparisons; component evidence, never release."""
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
from flatfile_native_domain_cases import qualify as qualify_domains
import build_restore_qualifier as qualifier


def qualify(native_source, artifacts, *, fixture=None, operator=None, native_fixture=None, red=False):
    native, out = native_source.resolve(), artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    native_before, inputs = fingerprint(native / "src"), audit_source_inputs()
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_wallet_bank_cases.py",
        "tests/async/flatfile_restore_authority_fixture.cpp",
        "tests/async/flatfile_native_domains_fixture.cpp",
        "tests/async/test_flatfile_restore_economic_authority.py")}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "economic-fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    if native_fixture is None:
        qualify_domains(native, out / "native-domains")
        native_fixture = out / "native-domains/native"
    independent = None
    if not red:
        source = out / "independent.cpp"
        source.write_text(r'''#include "qualify_flatfile_wallet_bank.h"
#include <iostream>
int main(int argc, char **argv) {
    try {
        restore_economic_authority::need(argc == 3);
        restore_economic_authority::audit_budget budget;
        const std::string mode = argv[1];
        if (mode == "bytes") budget.remaining_bytes = 0;
        if (mode == "files") budget.remaining_files = 0;
        if (mode == "entries") budget.remaining_entries = 0;
        if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
        auto result = restore_wallet_bank::audit(argv[2], budget);
        std::cout << result.valid() << " " << result.verified() << " " << result.finding_count << "\n";
        return result.valid() ? 0 : 1;
    } catch (...) { std::cerr << "native_restore_qualification_failed\n"; return 1; }
}
''', encoding="utf-8")
        flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts")]
        independent = out / "independent"
        subprocess.run([*flags, str(source), "-lcrypto", "-o", str(independent)], check=True, timeout=120)
        dependency = subprocess.check_output([*flags, "-MM", str(source)], text=True)
        assert "/src/" not in dependency
        (out / "independent.d").write_text(dependency)
        (out / "independent-build.json").write_text(json.dumps(flags, indent=2) + "\n")
    observations = []
    with tempfile.TemporaryDirectory(prefix="current-money-", dir=out) as temporary:
        state = Path(temporary)
        seed = subprocess.run([str(native_fixture), "seed-economic", str(state)],
            capture_output=True, text=True, timeout=30)
        assert seed.returncode == 0 and not seed.stderr, seed
        economic = subprocess.run([str(fixture), str(state), "baseline-wallet-bank"],
            env=environment, capture_output=True, text=True, timeout=90)
        assert economic.returncode == 0 and not economic.stderr, economic
        wallet = state / "domains/player-11.domain"
        original_wallet = wallet.read_bytes()
        original_bank = (state / "domains/bank-renamed-1.domain").read_bytes()
        actual = bytearray(original_wallet)
        account_size = struct.unpack_from("<I", actual, 60)[0]
        coins_at = 56 + 4 + 4 + account_size + 1 + 24
        struct.pack_into("<Q", actual, coins_at, 900)
        actual[24:56] = hashlib.sha256(actual[56:]).digest()
        wallet.write_bytes(actual)
        inventory = retained(state)
        probe = subprocess.run([str(native_fixture), "probe-economic", str(state)],
            capture_output=True, text=True, timeout=30)
        assert probe.returncode == 0 and not probe.stderr and retained(state) == inventory
        assert json.loads(probe.stdout)["wallet"] == [900, 0, 0, 0]
        old = subprocess.run([str(operator), "--economic-money-history-audit", str(state)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert old.returncode == 0 and not old.stderr and retained(state) == inventory, old
        assert json.loads(old.stdout)["same_epoch_transition_continuity_verified"]
        for entry in sorted(state.rglob("*")):
            if entry.is_file():
                target = out / "red-native-cut" / entry.relative_to(state)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(entry.read_bytes())
        observation = dict(label="retained history accepts a native stale wallet value",
            original_money_history=json.loads(old.stdout), native_current=json.loads(probe.stdout),
            expected_economic_wallet=[100, 0, 0, 0], complete_retained_inventory_unchanged=True)
        (out / "red-observation.json").write_text(json.dumps(observation, indent=2) + "\n")
        observations.append(observation)
        if not red:
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
                return dict(path=str(target.relative_to(out)),
                    sha256=hashlib.sha256(encoded.encode("utf-8")).hexdigest())

            def check(label, codes=(), *, refusal=False, verified=False):
                before = whole()
                source_cut = retain_input(before)
                parsed = subprocess.run([str(operator), "--economic-wallet-bank-audit", str(state)],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert whole() == before, label + ": operator wrote authority"
                sanitized = subprocess.run([str(independent), "normal", str(state)],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert whole() == before, label + ": independent reader wrote authority"
                expected = 1 if refusal or codes else 0
                assert parsed.returncode == sanitized.returncode == expected, (label, parsed, sanitized)
                if refusal:
                    assert not parsed.stdout and not sanitized.stdout
                    assert parsed.stderr == sanitized.stderr == "native_restore_qualification_failed\n"
                    report = None
                else:
                    assert not parsed.stderr and not sanitized.stderr, (label, parsed, sanitized)
                    report = json.loads(parsed.stdout)
                    found = {row["code"] for row in report["findings"]}
                    assert set(codes) == found, (label, codes, found, report)
                    assert report["current_wallet_bank_values_verified"] == verified
                    assert not report["account_origins_verified"] and not report["other_money_domains_verified"]
                    assert not report["native_holdings_compared"] and not report["full_R7_qualified"]
                    assert not report["release_qualified"] and not report["cross_epoch_continuity_verified"]
                    assert sanitized.stdout == f"{int(expected == 0)} {int(verified)} {report['finding_count']}\n"
                    assert "renamed" not in parsed.stdout and "native-fixture" not in parsed.stdout
                observations.append(dict(label=label, exit=parsed.returncode, report=report,
                    sanitized_exit=sanitized.returncode, source_cut=source_cut,
                    complete_retained_inventory_unchanged=True))
                print("WALLET_BANK " + label, flush=True)
                return report

            def frame(original, body=None, revision=None):
                header = bytearray(original[:56])
                payload = original[56:] if body is None else body
                struct.pack_into("<I", header, 12, len(payload))
                header[24:56] = hashlib.sha256(payload).digest()
                if revision is not None:
                    struct.pack_into("<Q", header, 16, revision)
                return bytes(header) + payload

            report = check("actual native stale wallet differs from economic tail", ("native_balance_mismatch",))
            assert report["findings"][0]["observed"] == dict(native=[900, 0, 0, 0],
                expected=[100, 0, 0, 0], native_revision=0, expected_revision=0)
            wallet.write_bytes(original_wallet)
            check("actual native wallet and bank agree with baseline clocks", verified=True)
            changed = bytearray(original_wallet)
            struct.pack_into("<Q", changed, coins_at - 16, 100)
            wallet.write_bytes(frame(changed, changed[56:], revision=100))
            check("nonmoney native clock does not change wallet money clock", verified=True)
            wallet.write_bytes(original_wallet)
            for vector, label, code in (([0, 10, 0, 0], "same value different denomination vector", "native_balance_mismatch"),
                    ([(1 << 64) - 1, 0, 0, 0], "native unsigned amount cannot wrap economic range", "native_balance_outside_economic_range"),
                    ([0, 0, 0, ((1 << 63) - 1) // 1000 + 1], "native weighted value exceeds economic range", "native_balance_outside_economic_range")):
                changed = bytearray(original_wallet)
                struct.pack_into("<4Q", changed, coins_at, *vector)
                wallet.write_bytes(frame(changed, changed[56:]))
                check(label, (code,))
            for revision in (1, (1 << 64) - 1):
                changed = bytearray(original_wallet)
                struct.pack_into("<Q", changed, coins_at - 24, revision)
                wallet.write_bytes(frame(changed, changed[56:], revision=max(1, revision)))
                check("native wallet revision mismatch " + str(revision), ("native_revision_mismatch",))
            wallet.write_bytes(original_wallet)
            bank = state / "domains/bank-renamed-1.domain"
            changed = bytearray(original_bank)
            struct.pack_into("<Q", changed, 56 + 4 + 7 + 1, 51)
            bank.write_bytes(frame(changed, changed[56:]))
            check("native bank balance mismatch", ("native_balance_mismatch",))
            bank.write_bytes(frame(original_bank, revision=2))
            check("native bank revision mismatch", ("native_revision_mismatch",))
            bank.write_bytes(original_bank)
            wallet.unlink()
            check("missing native wallet", ("native_domain_missing",))
            wallet.write_bytes(original_wallet)
            bank.unlink()
            check("missing native bank and wallet bank relationship", ("native_domain_missing", "wallet_bank_domain_missing"))
            bank.write_bytes(original_bank)
            for filename in (".critical-authority-transaction", ".player-domain-transaction", ".currency-transaction"):
                path = state / "domains" / filename
                path.write_bytes(b"pending-native-intent")
                check("pending native journal " + filename, refusal=True)
                path.unlink()
            wallet.chmod(0o644)
            check("nonprivate native wallet", refusal=True)
            wallet.chmod(0o600)
            alias = state / "domains/native-hardlink"
            os.link(wallet, alias)
            check("hardlinked native wallet", refusal=True)
            alias.unlink()
            wallet.unlink()
            wallet.symlink_to(bank)
            check("symlink native wallet", refusal=True)
            wallet.unlink()
            os.mkfifo(wallet, 0o600)
            check("FIFO native wallet refuses without blocking", refusal=True)
            wallet.unlink()
            wallet.write_bytes(original_wallet)
            changed = bytearray(original_wallet)
            changed[24] ^= 1
            wallet.write_bytes(changed)
            check("native checksum corruption", refusal=True)
            wallet.write_bytes(original_wallet)
            renamed_wallet = wallet.with_name("player-011.domain")
            wallet.rename(renamed_wallet)
            check("noncanonical native filename cannot alias the mapped wallet", refusal=True)
            renamed_wallet.rename(wallet)
            lock = state / "domains/.critical-authority.lock"
            lock.unlink()
            check("missing authority lock is not recreated", refusal=True)
            assert not lock.exists()
            # Only the original native fixture reestablishes its own test lock.
            probe = subprocess.run([str(native_fixture), "probe-economic", str(state)],
                capture_output=True, text=True, timeout=30)
            assert probe.returncode == 0 and lock.exists(), probe
            holder = subprocess.Popen([str(fixture), str(state), "hold-authority-lock"],
                env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                assert select.select([holder.stdout], [], [], 10)[0]
                assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
                check("native exclusive writer excludes current audit", refusal=True)
            finally:
                stdout, stderr = holder.communicate("release\n", timeout=10)
                assert holder.returncode == 0 and not stdout and not stderr
            for mode in ("bytes", "files", "entries", "deadline"):
                before = whole()
                source_cut = retain_input(before)
                refused = subprocess.run([str(independent), mode, str(state)], env=environment,
                    capture_output=True, text=True, timeout=30)
                assert refused.returncode == 1 and not refused.stdout
                assert refused.stderr == "native_restore_qualification_failed\n" and whole() == before
                observations.append(dict(label="cooperative current audit budget " + mode,
                    exit=1, source_cut=source_cut, complete_retained_inventory_unchanged=True))
            check("healthy current authority restored", verified=True)
            # Overflow has exact aggregate counts while published findings stay bounded.
            extras = []
            for pid in range(500, 621):
                changed = bytearray(original_wallet)
                struct.pack_into("<I", changed, 56, pid)
                path = wallet.with_name(f"player-{pid}.domain")
                path.write_bytes(frame(changed, changed[56:]))
                extras.append(path)
            report = check("native unmapped finding list caps at 100 without dropping total", ("native_domain_unmapped",))
            assert report["finding_count"] == 121 and report["findings_truncated"]
            assert len(report["findings"]) == 100
            for path in extras:
                path.unlink()

            def replace(seed_mode, economic_mode):
                # The entire disposable state is owned by this test; no user root.
                assert state.resolve().is_relative_to(out)
                shutil.rmtree(state)
                state.mkdir(mode=0o700)
                ran = subprocess.run([str(native_fixture), seed_mode, str(state)],
                    capture_output=True, text=True, timeout=30)
                assert ran.returncode == 0 and not ran.stderr, ran
                if economic_mode:
                    ran = subprocess.run([str(fixture), str(state), economic_mode], env=environment,
                        capture_output=True, text=True, timeout=90)
                    assert ran.returncode == 0 and not ran.stderr, ran

            replace("seed-economic-transfer", "baseline-wallet-bank-transfer")
            report = check("actual native ATM transfer matches retained prepared effects", verified=True)
            assert report["compared_accounts"] == 2 and report["accepted_roots"] == 2
            replace("seed-economic-latest", "baseline-wallet-bank-epochs")
            report = check("catalog current epoch wins over lexicographic ID order", verified=True)
            assert report["epoch"] == "19000000000000000000000000000000"
            assert report["prior_epoch_wallet_bank_accounts"] == 2 and report["compared_accounts"] == 2
            replace("seed-economic", "baseline-wallet-bank-retired")
            report = check("retired lifetime cannot own a nonzero current balance",
                ("retired_account_balance_nonzero", "native_domain_unmapped"))
            assert report["retired_current_epoch_accounts"] == 1
            replace("seed-economic-transfer", "baseline-wallet-bank-unanchored")
            report = check("matching current balances retain unknown legacy origin finding", ("unknown_legacy_origin",))
            assert report["unanchored_current_accounts"] == 2 and report["compared_accounts"] == 2
            replace("seed-economic-extra", "baseline-wallet-bank")
            check("unmapped native wallet cannot substitute for mapped PID", ("native_domain_unmapped", "native_domain_missing"))
            replace("seed-economic", "baseline")
            check("active bank mapping with no economic history", ("economic_history_missing",))
            replace("seed-economic", None)
            report = check("inactive legacy native domains have unknown mapping origins", ("native_domain_unmapped",))
            assert not report["initialized"]
            assert state.resolve().is_relative_to(out)
            shutil.rmtree(state)
            state.mkdir(mode=0o700)
            report = check("empty inactive authority remains an empty observation")
            assert not report["initialized"] and not report["wallet_bank_holdings_compared"]
    assert fingerprint(native / "src") == native_before and audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    report = dict(completed=True, red=red, observations=observations, skips=0,
        native_source_inputs=native_before, audit_source_inputs=inputs, owned_inputs=owned,
        executable_sha256={name: hashlib.sha256(Path(path).read_bytes()).hexdigest()
            for name, path in dict(native_fixture=native_fixture, economic_fixture=fixture,
                operator=operator, **({"independent": independent} if independent else {})).items()},
        native_mutation_providers_original=True, modeled_economic_witnesses=True,
        scope="current_wallet_bank_values_and_revisions",
        accounting_activated=False, full_R7_qualified=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(dict(completed=True, red=red, wallet_bank_cases=len(observations), skips=0)), flush=True)
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts, red=args.red)
