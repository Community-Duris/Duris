#!/usr/bin/env python3
"""Source-paired retained money continuity checks; never a release certificate."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time

from test_flatfile_restore_economic_authority import ROOT, build_fixture, inventory, rehash
from test_flatfile_restore_baseline_markers import audit_source_inputs, fingerprint, retained
import build_restore_qualifier as qualifier


def records(files):
    result = []
    for name in sorted(files):
        if not name.endswith(".eai"):
            continue
        index = files[name]
        for at in range(80, len(index), 64):
            segment_id, offset, size = struct.unpack_from("<III", index, at + 48)
            segment = name[:-4] + "-" + str(segment_id) + ".eas"
            encoded = files[segment][80 + offset:80 + offset + size]
            command_size, plan_size, _, code = struct.unpack_from("<IIII", encoded, 48)
            if code:
                continue
            plan_at = 74 + command_size
            assert plan_size >= 256
            result.append((name, at, segment, offset, encoded, plan_at))
    return result


def rewrite(files, operation, mutate):
    for index, at, segment, offset, encoded, plan_at in records(files):
        if encoded[82:98] != operation:
            continue
        value = bytearray(encoded)
        mutate(value, plan_at)
        value = rehash(value)
        body = bytearray(files[segment])
        body[80 + offset:80 + offset + len(value)] = value
        files[segment] = rehash(body)
        body = bytearray(files[index])
        body[at + 16:at + 48] = hashlib.sha256(value).digest()
        files[index] = rehash(body)
        return
    raise AssertionError("operation absent from retained native index")


def linear(files, wallet_opening_revision=0):
    # Native append order is 2,4,6,3; ID order is 2,4,6 then the other bucket.
    # Actual revision order is 4,3,2,6, so neither traversal order can be used.
    position = {4: 0, 3: 1, 2: 2, 6: 3}
    for _, _, _, _, encoded, _ in records(files):
        operation = encoded[82:98]
        step = position[int.from_bytes(operation[12:16], "big")]

        def mutate(value, start, n=step):
            for part in range(2):
                row = start + 256 + part * 120
                balance = 100 - n * 10 if part == 0 else 50 + n * 10
                after = balance + (-10 if part == 0 else 10)
                revision = n + (wallet_opening_revision if part == 0 else 0)
                struct.pack_into("<q", value, row + 40, balance)
                struct.pack_into("<q", value, row + 72, after)
                struct.pack_into("<QQ", value, row + 104, revision, revision + 1)
        rewrite(files, operation, mutate)
    return files


def qualify(native_source, artifacts, *, red=False, fixture=None, operator=None):
    args = argparse.Namespace(native_source=native_source, artifacts=artifacts, red=red)
    native, out = args.native_source.resolve(), args.artifacts.resolve()
    assert out.is_relative_to((ROOT / "bin").resolve()) and not out.exists()
    os.umask(0o077)
    out.mkdir(parents=True)
    native_before = fingerprint(native / "src")
    inputs = audit_source_inputs()
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "tests/async/flatfile_money_history_cases.py",
        "tests/async/test_flatfile_restore_economic_authority.py",
        "tests/async/flatfile_restore_authority_fixture.cpp")}
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    fixture = fixture or build_fixture(out / "fixture", native)
    operator = operator or qualifier.build(out / "qualify")
    observations = []
    with tempfile.TemporaryDirectory(prefix="duris-money-history-", dir=out) as temporary:
        state = Path(temporary)
        evidence = state / "economic-evidence"

        def native_store(mode):
            if evidence.exists():
                for path in evidence.iterdir():
                    path.unlink()
            ran = subprocess.run([str(fixture), str(state), mode], env=environment,
                                 capture_output=True, text=True, timeout=90)
            assert ran.returncode == 0 and not ran.stderr, ran
            files = inventory(evidence)
            target = out / ("native-" + mode)
            target.mkdir(mode=0o700)
            for name, value in files.items():
                (target / name).write_bytes(value)
            return files

        def install(files):
            for path in evidence.iterdir():
                path.unlink()
            for name, value in files.items():
                (evidence / name).write_bytes(value)

        fork = native_store("paged-records")
        effects = []
        for _, _, _, _, encoded, start in records(fork):
            for i in range(struct.unpack_from("<I", encoded, start + 216)[0]):
                row = start + 256 + i * 120
                effects.append(dict(operation=encoded[82:98].hex(),
                    account=encoded[row:row + 40].hex(),
                    before=list(struct.unpack_from("<4q", encoded, row + 40)),
                    after=list(struct.unpack_from("<4q", encoded, row + 72)),
                    revisions=list(struct.unpack_from("<QQ", encoded, row + 104))))
        assert len(effects) == 8 and len({row["account"] for row in effects}) == 2
        assert all(row["revisions"] == [0, 1] for row in effects)
        before = retained(state)
        old = subprocess.run([str(operator), "--economic-evidence-audit", str(state)],
                             env=environment, capture_output=True, text=True, timeout=30)
        assert old.returncode == 0 and not old.stderr and retained(state) == before, old
        (out / "red-observation.json").write_text(json.dumps(dict(
            original_reader_exit=old.returncode, original_reader=json.loads(old.stdout),
            mutually_overlapping_native_account_transitions=effects,
            native_state_unchanged=True, release_qualified=False), indent=2) + "\n")
        print("ORIGINAL_READER_ACCEPTS_FOUR_OVERLAPPING_NATIVE_TRANSITIONS", flush=True)
        if not args.red:
            audit_cpp = out / "independent.cpp"
            audit_cpp.write_text('''#include "qualify_flatfile_economic_money_history.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    try {
        if (std::string(argv[1]) == "projection-limits") {
            // Accumulator units only: these are not authenticated source roots.
            using namespace restore_economic_money_history;
            bytes plan(376);
            plan[216] = plan[272] = 1;
            plan[274] = 11; // Retained gambling stake is also ordinary money.
            plan[360 + 8] = 1;
            identity epoch = {}, operation = {};
            epoch[0] = operation[0] = 1;
            restore_economic_money_history::checker bounded;
            for (size_t i = 1; i <= maximum_edges; ++i) {
                for (size_t byte = 0; byte < 8; ++byte)
                    plan[276 + byte] = uint8_t(i >> (byte * 8));
                bounded.observe(epoch, operation, plan, {});
            }
            auto exact = bounded.finish();
            need(exact.valid() && exact.edges == maximum_edges && exact.accounts == maximum_edges);
            bool refused = false;
            try { bounded.observe(epoch, operation, plan, {}); }
            catch (const edge_budget_refused &) { refused = true; }
            need(refused);
            restore_economic_money_history::checker findings;
            for (size_t i = 1; i <= maximum_findings + 1; ++i) {
                for (size_t byte = 0; byte < 8; ++byte)
                    plan[276 + byte] = uint8_t(i >> (byte * 8));
                findings.observe(epoch, operation, plan, {});
                operation[1] = 1;
                findings.observe(epoch, operation, plan, {});
                operation[1] = 0;
            }
            auto truncated = findings.finish();
            need(!truncated.valid() && truncated.invalid_accounts == maximum_findings + 1 &&
                 truncated.findings.size() == maximum_findings);
            std::cout << exact.edges << " " << truncated.invalid_accounts << " "
                      << truncated.findings.size() << " " << sizeof(edge) << "\\n";
            return 0;
        }
        restore_economic_authority::audit_budget budget;
        std::string mode = argv[1];
        if (mode == "bytes") budget.remaining_bytes = 1;
        if (mode == "files") budget.remaining_files = 0;
        if (mode == "entries") budget.remaining_entries = 0;
        if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
        auto result = restore_economic_money_history::audit(argv[2], budget);
        std::cout << result.accepted_roots << " " << result.edges << " " << result.accounts
                  << " " << result.baseline_anchored_accounts << " " << result.unanchored_accounts
                  << " " << result.invalid_accounts << "\\n";
        return result.valid() ? 0 : 1;
    } catch (const restore_economic_money_history::edge_budget_refused &) {
        std::cerr << "money_history_edge_budget_refused\\n"; return 1;
    } catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
            audit = out / "independent"
            flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                     "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                     "-I" + str(ROOT / "scripts")]
            dependencies = subprocess.check_output(["g++", *flags, "-MM", str(audit_cpp)], text=True)
            assert "/src/" not in dependencies and "flatfile_accounting" not in dependencies
            (out / "independent-dependencies.txt").write_text(dependencies)
            subprocess.run(["g++", *flags, str(audit_cpp), "-lcrypto", "-o", str(audit)], check=True)
            limits = subprocess.run([str(audit), "projection-limits", str(state)], env=environment,
                                    capture_output=True, text=True, timeout=30)
            assert limits.returncode == 0 and not limits.stderr, limits
            maximum, invalid, returned, edge_size = map(int, limits.stdout.split())
            assert (maximum, invalid, returned) == (32768, 101, 100) and edge_size <= 192
            observations.append(dict(label="projection units: exact edge cap, finding cap and stake kind",
                ordinary_edge_cap=maximum, invalid_accounts=invalid, returned_findings=returned,
                edge_bytes=edge_size, authenticated_source_qualified=False))

            def check(label, files, expected=None, codes=(), edge_budget=False):
                install(files)
                frozen = out / ("case-" + str(len(observations)).zfill(2))
                frozen.mkdir(mode=0o700)
                for name, value in files.items():
                    (frozen / name).write_bytes(value)
                # Every success root must remain accepted by the original native codec.
                for index, _, _, _, encoded, _ in (records(files) if expected is not None else []):
                    raw = out / "native-decode-record.bin"
                    raw.write_bytes(encoded)
                    native_decode = subprocess.run([str(fixture), str(raw), "decode-record"],
                        env=environment, capture_output=True, text=True, timeout=10)
                    assert native_decode.returncode == 0 and not native_decode.stderr, (label, index, native_decode)
                before = retained(state)
                started = time.monotonic()
                ran = subprocess.run([str(operator), "--economic-money-history-audit", str(state)],
                    env=environment, capture_output=True, text=True, timeout=30)
                independent = subprocess.run([str(audit), "bounded", str(state)], env=environment,
                    capture_output=True, text=True, timeout=30)
                assert retained(state) == before, label + ": audit modified native files"
                observed = dict(label=label, operator_exit=ran.returncode,
                    operator_stdout=ran.stdout, operator_stderr=ran.stderr,
                    independent_exit=independent.returncode,
                    independent_stdout=independent.stdout, independent_stderr=independent.stderr,
                    expected=expected, native_state_unchanged=True, source_directory=str(frozen),
                    source_files={name: hashlib.sha256(value).hexdigest() for name, value in files.items()})
                (frozen / "observation.json").write_text(json.dumps(observed, sort_keys=True, indent=2) + "\n")
                if expected is None:
                    assert ran.returncode == independent.returncode == 1, (label, ran, independent)
                    assert not ran.stdout and not independent.stdout
                    assert ran.stderr == "native_restore_qualification_failed\n"
                    assert independent.stderr == ("money_history_edge_budget_refused\n"
                        if edge_budget else "native_restore_qualification_failed\n")
                    report = None
                else:
                    report = json.loads(ran.stdout)
                    assert not ran.stderr and not independent.stderr, (label, ran, independent)
                    assert independent.stdout == " ".join(map(str, expected)) + "\n", (label, independent)
                    assert ran.returncode == independent.returncode == (1 if expected[-1] else 0), (label, ran)
                    fields = ("accepted_roots", "ordinary_account_edges", "epoch_accounts",
                              "baseline_anchored_accounts", "unanchored_accounts", "invalid_accounts")
                    assert tuple(report[name] for name in fields) == expected, (label, report)
                    assert report["same_epoch_transition_continuity_verified"] == (expected[-1] == 0)
                    assert tuple(row["code"] for row in report["findings"]) == codes
                    assert not any(report[name] for name in ("account_origins_verified",
                        "cross_epoch_continuity_verified", "native_holdings_compared",
                        "full_R7_qualified", "release_qualified", "findings_truncated"))
                    for row in report["findings"]:
                        assert set(row) == {"epoch", "operation", "account", "code"}
                        assert len(bytes.fromhex(row["account"])) == 40
                observations.append(dict(label=label, exit=ran.returncode, report=report,
                    seconds=time.monotonic() - started, native_state_unchanged=True))
                print("MONEY_HISTORY " + label, flush=True)
                return report

            check("original native forks across physical buckets", fork, (4, 8, 2, 0, 2, 2),
                  ("overlapping_revision", "overlapping_revision"))
            chain = linear(dict(fork))
            check("continuous history uses revisions rather than append or ID order", chain, (4, 8, 2, 0, 2, 0))
            ordered = sorted(records(chain), key=lambda row: struct.unpack_from("<Q", row[4], row[5] + 360)[0])
            operation = ordered[2][4][82:98]
            damaged = dict(chain)
            def wrong_balance(value, start):
                struct.pack_into("<2q", value, start + 296, 70, 1)
                struct.pack_into("<2q", value, start + 328, 60, 1)
            rewrite(damaged, operation, wrong_balance)
            check("balanced native plan with wrong prior denomination vector", damaged,
                  (4, 8, 2, 0, 2, 1), ("balance_discontinuity",))
            damaged = dict(chain)
            rewrite(damaged, operation, lambda v, p: struct.pack_into("<QQ", v, p + 360, 5, 6))
            check("balanced native plan with a missing revision edge", damaged,
                  (4, 8, 2, 0, 2, 1), ("revision_gap",))
            jumped = dict(chain)
            revision_pairs = ((0, 2), (2, 5), (5, 6), (6, (1 << 64) - 1))
            for (_, _, _, _, encoded, _), pair in zip(ordered, revision_pairs):
                rewrite(jumped, encoded[82:98], lambda v, p, r=pair:
                        [struct.pack_into("<QQ", v, p + 360 + i * 120, *r) for i in range(2)])
            check("strictly increasing revisions need not increment by one", jumped, (4, 8, 2, 0, 2, 0))
            contexts = dict(chain)
            for _, _, _, _, encoded, _ in records(contexts):
                def separate_context(value, start):
                    row = start + 256 + 120
                    struct.pack_into("<H", value, row + 18, 1)
                    struct.pack_into("<Q", value, row + 20, 3)
                rewrite(contexts, encoded[82:98], separate_context)
            check("complete account key keeps shared kind and lifetime with distinct context separate",
                  contexts, (4, 8, 2, 0, 2, 0))
            stationary = dict(chain)
            operation = ordered[1][4][82:98]
            def net_zero(value, start):
                for i, balance in enumerate((90, 60)):
                    row = start + 256 + i * 120
                    struct.pack_into("<qq", value, row + 40, balance, 0)
                    struct.pack_into("<q", value, row + 72, balance)
                    struct.pack_into("<QQ", value, row + 104, 1, 2 if i == 0 else 1)
                for i in range(2):
                    struct.pack_into("<H", value, start + 496 + i * 48 + 4, 1)
            rewrite(stationary, operation, net_zero)
            for _, _, _, _, encoded, _ in ordered[2:]:
                def shift_balances(value, start):
                    for i, delta in enumerate((10, -10)):
                        for at in (40, 72):
                            offset = start + 256 + i * 120 + at
                            struct.pack_into("<q", value, offset, struct.unpack_from("<q", value, offset)[0] + delta)
                    row = start + 256 + 120 + 104
                    before, after = struct.unpack_from("<QQ", value, row)
                    struct.pack_into("<QQ", value, row, before - 1, after - 1)
                rewrite(stationary, encoded[82:98], shift_balances)
            check("unchanged balance with revision advance and same-revision observation", stationary,
                  (4, 8, 2, 0, 2, 0))
            broken_observation = dict(stationary)
            def wrong_observation(value, start):
                struct.pack_into("<q", value, start + 416, 61)
                struct.pack_into("<q", value, start + 448, 61)
            rewrite(broken_observation, operation, wrong_observation)
            check("same-revision observation must match the retained balance", broken_observation,
                  (4, 8, 2, 0, 2, 1), ("balance_discontinuity",))
            for pair in ((3, 4), (5, 6)):
                kinds = dict(chain)
                for _, _, _, _, encoded, _ in records(kinds):
                    def rekey(value, start, selected=pair):
                        command = 74
                        keys, revisions, payload = struct.unpack_from("<III", value, command + 40)
                        intent_at = command + 52 + keys * 16 + revisions * 24 + payload + 4
                        intent_size = struct.unpack_from("<I", value, intent_at - 4)[0]
                        struct.pack_into("<H", value, intent_at + 24, 1)
                        struct.pack_into("<H", value, start + 96, 1)
                        value[start + 152:start + 184] = hashlib.sha256(
                            b"DURIS-ECONOMIC-INTENT-V1\0" + value[intent_at:intent_at + intent_size]).digest()
                        for i, kind in enumerate(selected):
                            struct.pack_into("<H", value, start + 256 + i * 120 + 18, kind)
                    rewrite(kinds, encoded[82:98], rekey)
                check("ordinary holding kind pair " + str(pair), kinds, (4, 8, 2, 0, 2, 0))
                forked_kinds = dict(kinds)
                rewrite(forked_kinds, ordered[-1][4][82:98],
                        lambda v, p: struct.pack_into("<QQ", v, p + 360, 2, 4))
                check("overlap detected for ordinary holding kind " + str(pair[0]), forked_kinds,
                      (4, 8, 2, 0, 2, 1), ("overlapping_revision",))
            check("same lifetime in distinct retained epochs is kept separate",
                  native_store("source-claims"), (2, 4, 4, 0, 4, 0))
            check("independently authenticated baselines across epochs",
                  native_store("baseline-rich"), (4, 9, 9, 9, 0, 0))
            actual = native_store("baseline-money-history")
            check("actual native baseline witness clock 42 joins native prepared transfer 42 to 43",
                  actual, (2, 3, 2, 1, 1, 0))
            wallet_operation = next(encoded[82:98] for _, _, _, _, encoded, start in records(actual)
                                    if struct.unpack_from("<H", encoded, start + 96)[0] != 38)
            wrong_clock = dict(actual)
            rewrite(wrong_clock, wallet_operation,
                    lambda v, p: struct.pack_into("<QQ", v, p + 360, 1, 2))
            check("synthetic baseline posting revision is not the native holding clock", wrong_clock,
                  (2, 3, 2, 1, 1, 1), ("overlapping_revision",))
            missing_origin_edge = dict(actual)
            rewrite(missing_origin_edge, wallet_operation,
                    lambda v, p: struct.pack_into("<QQ", v, p + 360, 43, 44))
            check("witness anchored native clock detects the missing first ordinary edge", missing_origin_edge,
                  (2, 3, 2, 1, 1, 1), ("revision_gap",))
            baseline = native_store("baseline")
            joined = dict(baseline)
            for name, value in linear(dict(fork)).items():
                if name.startswith("bucket-"):
                    assert name not in joined
                    joined[name] = value
            control = bytearray(joined["authority.eal"])
            for i in range(32):
                control[48 + 16520 + i] |= fork["authority.eal"][48 + 16520 + i]
            joined["authority.eal"] = rehash(control)
            check("baseline opening continues into authenticated ordinary roots", joined, (5, 9, 2, 1, 1, 0))
            check("rejected native envelopes produce no money edges", native_store("envelope-records"),
                  (0, 0, 0, 0, 0, 0))
            check("inactive empty authority produces no money edges", native_store("bootstrap"),
                  (0, 0, 0, 0, 0, 0))
            corrupted = dict(chain)
            name = next(name for name in corrupted if name.endswith(".eas"))
            corrupted[name] = corrupted[name][:-1] + bytes([corrupted[name][-1] ^ 1])
            check("malformed retained root refuses without a partial result", corrupted)
            check("native full reservation shard exceeds bounded history accumulation",
                  native_store("baseline-full-index"), edge_budget=True)
            install(chain)
            for mode in ("bytes", "files", "entries", "deadline"):
                before = retained(state)
                ran = subprocess.run([str(audit), mode, str(state)], env=environment,
                                     capture_output=True, text=True, timeout=30)
                assert ran.returncode == 1 and not ran.stdout
                assert ran.stderr == "native_restore_qualification_failed\n" and retained(state) == before
                observations.append(dict(label="cooperative budget " + mode, exit=1, native_state_unchanged=True))
            holder = subprocess.Popen([str(fixture), str(state), "hold-authority-lock"], env=environment,
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
                before = retained(state)
                ran = subprocess.run([str(operator), "--economic-money-history-audit", str(state)],
                                     env=environment, capture_output=True, text=True, timeout=30)
                assert ran.returncode == 1 and not ran.stdout and ran.stderr == "native_restore_qualification_failed\n"
                assert retained(state) == before
                observations.append(dict(label="native exclusive writer excludes audit", exit=1, native_state_unchanged=True))
            finally:
                stdout, stderr = holder.communicate("release\n", timeout=10)
                assert holder.returncode == 0 and not stdout and not stderr
    assert fingerprint(native / "src") == native_before
    assert audit_source_inputs() == inputs
    assert all(hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == value for name, value in owned.items())
    report = dict(completed=True, red=args.red, native_inputs=native_before, audit_source_inputs=inputs,
                  owned_inputs=owned, observations=observations, skips=0, source_unchanged=True,
                  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
                  operator_sha256=hashlib.sha256(operator.read_bytes()).hexdigest(),
                  independent_sha256=hashlib.sha256(audit.read_bytes()).hexdigest() if not args.red else None,
                  full_R7_qualified=False, native_holdings_compared=False, release_qualified=False)
    (out / "evidence.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if not k.endswith("inputs")}), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", type=Path, required=True)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--red", action="store_true")
    args = parser.parse_args()
    qualify(args.native_source, args.artifacts, red=args.red)


if __name__ == "__main__":
    main()
