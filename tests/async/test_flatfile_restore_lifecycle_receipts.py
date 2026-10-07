#!/usr/bin/env python3
"""Independent origin/receipt audit against native codec/common-baseline fixtures.

The primitive fixture never calls lifecycle install, native source capture or
activation. Known v3 origins prove required-file discovery within this modeled
fixture; they do not authenticate native source capture or a complete cutover.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import sys
import tempfile

from test_flatfile_restore_economic_authority import ROOT, change, inventory, rehash
from test_flatfile_restore_baseline_markers import fingerprint
from native_build_artifacts import build_native
from test_flatfile_accounting_store import SOURCES
import build_restore_qualifier as qualifier


def retained(directory):
    result = {}
    for path in sorted(directory.rglob("*")):
        info = path.lstat()
        value = (hashlib.sha256(path.read_bytes()).hexdigest() if stat.S_ISREG(info.st_mode)
                 else os.readlink(path) if stat.S_ISLNK(info.st_mode) else None)
        result[str(path.relative_to(directory))] = {
            "mode": info.st_mode, "links": info.st_nlink, "inode": info.st_ino,
            "size": info.st_size, "mtime_ns": info.st_mtime_ns, "value": value}
    return result


def build_fixture(destination):
    return build_native(destination, [
        "tests/async/flatfile_restore_lifecycle_receipt_fixture.cpp",
        "src/flatfile/flatfile_accounting_authority.c",
        "src/flatfile/flatfile_accounting_baseline.c", "src/economy/economic_baseline_adapter.c",
        "src/economy/economic_baseline_codec.c", "src/economy/economic_baseline_command.c",
        *SOURCES[1:]],
        ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
         "-ffunction-sections", "-fdata-sections", "-DDURIS_FLATFILE_ACCOUNTING_TEST",
         "-Isrc", "-pthread"], ["-Wl,--gc-sections", "-lcrypto", "-pthread"],
        compiler="g++", name="restore-lifecycle-codec-fixture")


def layout(value):
    """Locate variable fields in native output, without making valid receipts."""
    mappings, sources, blobs = [], [], []
    count = struct.unpack_from("<I", value, 444)[0]
    at = 448
    for _ in range(count):
        length = struct.unpack_from("<H", value, at + 50)[0]
        mappings.append((at, length))
        at += 108 + length
    counts = at
    wallets, banks = struct.unpack_from("<II", value, at)
    at += 8
    for wallet in [True] * wallets + [False] * banks:
        start = at
        name = at + (4 if wallet else 0)
        length = struct.unpack_from("<H", value, name)[0]
        sources.append((start, name, length, wallet))
        at += (47 if wallet else 43) + length
    for _ in range(3):
        length = struct.unpack_from("<I", value, at)[0]
        blobs.append((at, at + 4, length))
        at += 4 + length
    assert at == len(value)
    return mappings, counts, sources, blobs


def check_lifecycle_pages(binary, fixture, audit, artifacts, environment):
    """Original native receipts/root links, durable fences and sticky fair refusals."""
    import fcntl
    import flatfile_economic_audit as pages
    import time
    from unittest import mock
    observations = []
    with tempfile.TemporaryDirectory(prefix="lifecycle-pages-", dir=artifacts) as temporary:
        parent = Path(temporary)
        root = parent / "state"
        root.mkdir(mode=0o700)
        evidence = root / "economic-evidence"

        def install(files):
            evidence.mkdir(mode=0o700, exist_ok=True)
            for path in evidence.iterdir():
                path.unlink()
            for name, value in files.items():
                (evidence / name).write_bytes(value)

        def produce(mode):
            install({})
            ran = subprocess.run([str(fixture), str(root), mode], env=environment,
                                 capture_output=True, text=True, timeout=600)
            assert ran.returncode == 0 and not ran.stderr, ran
            files = inventory(evidence)
            assert files["authority.eal"][112:128] == bytes(16)
            destination = artifacts / ("page-native-" + mode)
            destination.mkdir(mode=0o700)
            for name, value in files.items():
                (destination / name).write_bytes(value)
            return files

        def raw(label, bucket=20, after="-", ceiling="-", valid=True):
            before = retained(root)
            ran = subprocess.run([str(binary), "--economic-lifecycle-page", str(root),
                str(bucket), after, ceiling], env=environment, capture_output=True, text=True, timeout=45)
            assert retained(root) == before, label + ": page changed authority"
            assert ran.returncode == (0 if valid else 1), (label, ran)
            assert ran.stderr == ("" if valid else "native_restore_qualification_failed\n"), (label, ran)
            page = json.loads(ran.stdout) if valid else None
            if valid:
                assert page["rows"] <= 2 and page["bucket_rows"] <= 4096
                assert page["verified"] + len(page["invalid_receipts"]) == page["rows"]
            else:
                assert not ran.stdout
            observations.append(dict(label=label, exit=ran.returncode, page=page, native_state_unchanged=True))
            return page

        for mode in ("mixed", "empty", "renamed", "retired", "retained", "maximum", "generic"):
            produce(mode)
            page = raw("native " + mode)
            assert page["rows"] == page["verified"] == int(mode != "generic")
            assert page["range_exhausted"] and not page["invalid_receipts"]

        produce("maximum-paged")
        page = raw("two maximum native receipts")
        assert page["rows"] == page["verified"] == 2 and page["range_exhausted"]
        before = retained(root)
        measured = subprocess.run([str(audit), "lifecycle-budget", str(root)], env=environment,
            capture_output=True, text=True, timeout=45)
        assert measured.returncode == 0 and not measured.stderr, measured
        fields = list(map(int, measured.stdout.split()))
        assert len(fields) == 8 and fields[:3] == [16384, 128*1024*1024, 8192] and fields[-2:] == [2, 2], fields
        assert 0 < fields[3] <= fields[0] and 0 < fields[4] <= fields[1] and 0 <= fields[5] <= fields[2]
        assert retained(root) == before
        observations.append(dict(label="two maximum native receipt budget", limits=fields[:3],
            actual_physical_reads=fields[3], actual_bytes=fields[4], actual_directory_entries=fields[5],
            rows=2, verified=2, sanitizer_instrumented=True, native_state_unchanged=True))
        assert fields[5] == 0, "catalogue-derived page unexpectedly enumerated a directory"
        for mode in ("lifecycle-files", "lifecycle-bytes", "lifecycle-deadline"):
            ran = subprocess.run([str(audit), mode, str(root)], env=environment,
                capture_output=True, text=True, timeout=45)
            assert ran.returncode == 1 and not ran.stdout and ran.stderr == "native_restore_qualification_failed\n", ran
            assert retained(root) == before
            observations.append(dict(label=mode + " refuses", exit=1, sanitizer_instrumented=True, native_state_unchanged=True))

        short = produce("paged-short")
        extended = produce("paged")
        receipts = sorted(name for name in extended if name.endswith(".elr"))
        assert len(receipts) == 9 and len([name for name in short if name.endswith(".elr")]) == 7
        # Earlier original receipts remain byte-exact when the later epochs append.
        assert all(short[name] == extended[name] for name in receipts[:7])
        whole = subprocess.run([str(binary), "--economic-evidence-audit", str(root)], env=environment,
            capture_output=True, text=True, timeout=45)
        assert whole.returncode == 0 and not whole.stderr and json.loads(whole.stdout)["lifecycle_receipts"] == 9
        install(short)
        first = raw("first original page before append")
        assert first["rows"] == first["verified"] == 2 and first["bucket_rows"] == 7 and not first["range_exhausted"]
        install(extended)
        after = first["cursor"]
        observed = 2
        while True:
            page = raw("resumed original fence after append", after=after, ceiling=first["ceiling"])
            assert not page["invalid_receipts"] and page["verified"] == page["rows"]
            observed += page["rows"]
            after = page["cursor"]
            if page["range_exhausted"]:
                break
        assert observed == 7 and page["cursor"] == first["ceiling"]
        assert raw("later append visible on next range")["ceiling"] != first["ceiling"]
        raw("missing cursor", after="14" + "ff"*15, ceiling="14" + "ff"*15, valid=False)
        raw("missing ceiling", after=first["cursor"], ceiling="14" + "ff"*15, valid=False)
        raw("wrong bucket", bucket=21, after=first["cursor"], ceiling=first["ceiling"], valid=False)
        raw("noncanonical bucket", bucket="020", valid=False)
        raw("cursor without fence", after=first["cursor"], valid=False)

        missing = dict(extended)
        missing.pop(receipts[0])
        install(missing)
        invalid = raw("required filename wholly absent")
        assert invalid["verified"] == 1 and invalid["invalid_receipts"] == [receipts[0][10:-4]]
        damaged = dict(extended)
        changed = bytearray(damaged[receipts[0]])
        changed[-1] ^= 1
        damaged[receipts[0]] = bytes(changed)
        install(damaged)
        invalid = raw("damaged receipt with healthy sibling")
        assert invalid["verified"] == 1 and invalid["invalid_receipts"] == [receipts[0][10:-4]]
        install(extended)
        for label, unsafe in (
            ("world-readable receipt", lambda p: p.chmod(0o644)),
            ("hardlinked receipt", lambda p: os.link(p, evidence / "unrecognized-link")),
            ("symlink receipt", lambda p: (p.rename(evidence / "unrecognized-target"), p.symlink_to("unrecognized-target"))),
            ("FIFO receipt", lambda p: (p.unlink(), os.mkfifo(p, 0o600)))):
            unsafe(evidence / receipts[0])
            assert raw(label)["invalid_receipts"] == [receipts[0][10:-4]]
            install(extended)

        original = extended[receipts[0]][249:265].hex()
        root_bucket = original[:2]
        index_name = "bucket-" + root_bucket + ".eai"
        damaged = dict(extended)
        damaged.pop(index_name)
        install(damaged)
        invalid = raw("missing original common index")
        assert receipts[0][10:-4] in invalid["invalid_receipts"]
        missing_witness = dict(extended)
        witness_name = next(name for name in extended if name.endswith(original + ".eab"))
        missing_witness.pop(witness_name)
        install(missing_witness)
        assert receipts[0][10:-4] in raw("missing original common witness")["invalid_receipts"]
        damaged = dict(extended)
        index = damaged[index_name]
        count = struct.unpack_from("<I", index, 68)[0]
        row = next(index[80+i*64:144+i*64] for i in range(count) if index[80+i*64:96+i*64].hex() == original)
        segment_id, offset, size = struct.unpack_from("<III", row, 48)
        segment_name = f"bucket-{root_bucket}-{segment_id}.eas"
        segment = bytearray(damaged[segment_name])
        # Fully rehashed frame/index with a changed original durable revision.
        record = bytearray(segment[80+offset:80+offset+size])
        revision = struct.unpack_from("<Q", record, 64)[0]
        struct.pack_into("<Q", record, 64, revision+1)
        record = rehash(record)
        segment[80+offset:80+offset+size] = record
        index = bytearray(index)
        row_at = next(80+i*64 for i in range(count) if index[80+i*64:96+i*64].hex() == original)
        index[row_at+16:row_at+48] = hashlib.sha256(record).digest()
        damaged[index_name], damaged[segment_name] = rehash(index), rehash(segment)
        install(damaged)
        assert receipts[0][10:-4] in raw("changed original root revision")["invalid_receipts"]
        install(extended)

        progress = parent / "progress.json"
        source = pages.source_digest(root, binary)
        saved = pages.new_progress(source, time.time(), lifecycle_receipts=True)
        saved["rotation"] = 20
        pages.progress_io.save(progress, saved, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        command = [sys.executable, "-B", str(ROOT / "scripts/flatfile_economic_audit.py"), "--state-root", str(root),
            "--qualifier", str(binary), "--progress", str(progress), "--scope", "lifecycle-receipts"]

        def load():
            return pages.validate(pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES,
                pages.AuditError, "flatfile audit progress"), source, time.time(), lifecycle_receipts=True)

        def cli(label, findings=False):
            before = retained(root)
            ran = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=60)
            assert ran.returncode == int(findings) and not ran.stderr, (label, ran)
            assert retained(root) == before
            report = json.loads(ran.stdout)
            assert report["scope"] == "required_lifecycle_receipt_root_page"
            assert not any(report[key] for key in ("complete", "consistent_entire_sweep", "release_qualified",
                "native_holdings_compared", "baseline_books_closed", "lifecycle_receipts_closed", "orphan_namespace_closed"))
            observations.append(dict(label=label, exit=ran.returncode, report=report, native_state_unchanged=True))
            return report

        assert cli("durable first page")["examined_receipts"] == 2
        anchored = load()
        native_before = retained(root)
        assert anchored["buckets"][20]["cursor"] and anchored["buckets"][20]["ceiling"]
        for _ in range(255):
            report, updated = pages.scan(root, binary, load(), lifecycle_receipts=True)
            pages.progress_io.save(progress, updated, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
            assert report["examined_receipts"] == 0 and report["range_exhausted"] and not report["page_refused"]
        assert load()["rotation"] == 20 and load()["buckets"][20] == anchored["buckets"][20]
        assert retained(root) == native_before
        observations.append(dict(label="all 255 sibling buckets rotate before resume", native_state_unchanged=True))
        assert cli("durable resumed page")["examined_receipts"] == 2
        saved = load()
        saved["rotation"] = 20
        pages.progress_io.save(progress, saved, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        later_missing = dict(extended)
        later_missing.pop(receipts[4])
        install(later_missing)
        assert cli("second-page missing receipt sticky finding", True)["verified_receipt_roots"] == 1
        assert load()["findings"] == [dict(bucket=20, operation_id=receipts[4][10:-4], code="flatfile_lifecycle_receipt_invalid")]
        install(extended)
        assert cli("healthy sibling after sticky finding", True)["bucket"] == 21

        saved = load()
        saved["rotation"] = 20
        pages.progress_io.save(progress, saved, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        checkpoint = load()["buckets"][20].copy()
        control = bytearray(extended["authority.eal"])
        control[-1] ^= 1
        (evidence / "authority.eal").write_bytes(control)
        refused = cli("refused page rotates without advancing", True)
        assert refused["page_refused"] and not refused["range_exhausted"] and refused["next_bucket"] == 21
        assert load()["buckets"][20] == checkpoint
        install(extended)
        assert cli("healthy sibling after refusal", True)["bucket"] == 21

        with mock.patch.object(pages.subprocess, "run", side_effect=subprocess.TimeoutExpired("qualifier", 45)):
            refused, updated = pages.scan(root, binary, saved, lifecycle_receipts=True)
        assert refused["page_refused"] and updated["rotation"] == 21 and updated["buckets"][20] == checkpoint
        observations.append(dict(label="timeout rotates without advancing", native_state_unchanged=True))
        original_progress = progress.read_bytes()
        with mock.patch.object(pages.progress_io.os, "replace", side_effect=OSError("interrupted")):
            try:
                pages.progress_io.save(progress, updated, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
            except OSError:
                pass
            else:
                raise AssertionError("interrupted lifecycle progress replaced checkpoint")
        assert progress.read_bytes() == original_progress
        observations.append(dict(label="interrupted checkpoint preserves original", native_state_unchanged=True))
        for label, wrong in (("wrong scope", command[:-2]), ("checkpoint inside authority",
            [*command[:], "--progress", str(root / "forbidden.json")])):
            ran = subprocess.run(wrong, env=environment, capture_output=True, text=True, timeout=60)
            assert ran.returncode == 1 and not ran.stdout and ran.stderr == "flatfile_economic_audit_refused\n"
            assert progress.read_bytes() == original_progress
            observations.append(dict(label=label, exit=1, native_state_unchanged=True))
        foreign = load()
        foreign["lineage"] = "02" + "00"*15
        pages.progress_io.save(progress, foreign, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        foreign_bytes = progress.read_bytes()
        ran = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=60)
        assert ran.returncode == 1 and not ran.stdout and ran.stderr == "flatfile_economic_audit_refused\n"
        assert progress.read_bytes() == foreign_bytes
        progress.write_bytes(original_progress)
        observations.append(dict(label="foreign lineage preserves checkpoint", exit=1, native_state_unchanged=True))
        with pages.progress_io.lock(progress, pages.AuditError, "flatfile audit progress"):
            ran = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=60)
            assert ran.returncode == 1 and not ran.stdout and ran.stderr == "flatfile_economic_audit_refused\n"
        assert progress.read_bytes() == original_progress
        observations.append(dict(label="concurrent checkpoint owner refused", native_state_unchanged=True))
        with (root / "domains/.critical-authority.lock").open("rb") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            raw("native writer lock held", valid=False)
        journal = root / "domains/.critical-authority-transaction"
        journal.write_bytes(b"pending")
        raw("pending native journal", valid=False)
        journal.unlink()
        assert inventory(evidence) == extended
    return dict(cases=len(observations), observations=observations, skips=0, release_qualified=False,
                source_capture_executed=False, lifecycle_install_executed=False, activation_executed=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", required=True, type=Path)
    parser.add_argument("--previous-qualifier", type=Path)
    parser.add_argument("--legacy-artifacts", type=Path,
                        help="preserved actual v2 native receipt fixture artifacts")
    parser.add_argument("--state-parent", type=Path,
                        help="disposable native fixture filesystem (for example /dev/shm)")
    args = parser.parse_args()
    artifacts = args.artifacts.resolve()
    if artifacts.exists() or not artifacts.is_relative_to((ROOT / "bin").resolve()):
        parser.error("artifacts must be a fresh directory below bin/")
    os.umask(0o077)
    artifacts.mkdir(parents=True)
    native_inputs = fingerprint(ROOT / "src")
    legacy_inputs = fingerprint(args.legacy_artifacts) if args.legacy_artifacts else None
    names = ["scripts/qualify_flatfile_economic_authority.h",
             "scripts/qualify_flatfile_economic_baseline.h",
             "scripts/qualify_flatfile_economic_records.h",
             "scripts/qualify_flatfile_economic_lifecycle.h",
             "scripts/qualify_flatfile_restore.cpp", "scripts/build_restore_qualifier.py",
             "tests/async/flatfile_restore_lifecycle_receipt_fixture.cpp",
             "tests/async/test_flatfile_restore_lifecycle_receipts.py",
             "tests/async/test_flatfile_restore_economic_authority.py",
             "tests/async/test_flatfile_restore_baseline_markers.py",
             "tests/async/native_build_artifacts.py", "tests/async/test_flatfile_accounting_store.py"]
    owned_inputs = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in names}
    binary = qualifier.build(artifacts / "qualify")
    fixture = build_fixture(artifacts / "fixture")
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    audit_source = artifacts / "audit.cpp"
    audit_source.write_text('''#include "qualify_flatfile_economic_records.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) return 2;
    try {
        if (argc == 3) {
            const auto mode = std::string(argv[1]);
            if (mode != "maximum-budget" && !mode.starts_with("lifecycle-")) return 2;
            restore_economic_authority::audit_budget budget;
            const auto files = budget.remaining_files, bytes = budget.remaining_bytes,
                       entries = budget.remaining_entries;
            if (mode.starts_with("lifecycle-")) {
                if (mode == "lifecycle-files") budget.remaining_files = 1;
                else if (mode == "lifecycle-bytes") budget.remaining_bytes = 1;
                else if (mode == "lifecycle-deadline") budget.deadline = std::chrono::steady_clock::now();
                else if (mode != "lifecycle-budget") return 2;
                restore_economic_authority::scoped_audit_budget scope(budget);
                restore_economic_authority::authority_read_lock lock(argv[2]);
                auto value = restore_economic_records::checker(argv[2]).lifecycle_page(20, {}, {}, false);
                lock.finish(); budget.checkpoint();
                std::cout << files << " " << bytes << " " << entries << " "
                          << files-budget.remaining_files << " " << bytes-budget.remaining_bytes
                          << " " << entries-budget.remaining_entries << " " << value.rows
                          << " " << value.verified << "\\n";
                return 0;
            }
            auto value = restore_economic_records::audit(argv[2], budget);
            std::cout << files << " " << bytes << " " << entries << " "
                      << files-budget.remaining_files << " " << bytes-budget.remaining_bytes
                      << " " << entries-budget.remaining_entries << " "
                      << value.lifecycle_receipts << "\\n";
            return 0;
        }
        auto result = restore_economic_records::checker(argv[1]).run();
        std::cout << result.legacy_unknown_epochs << " " << result.never_initialized_epochs
                  << " " << result.initialized_epochs << " " << result.complete()
                  << " " << result.lifecycle_receipts << " " << result.unknown_initialized_origins
                  << " " << result.baseline_participant_epochs << " " << result.lifecycle_owner_epochs
                  << " " << result.lifecycle_complete() << "\\n";
        return 0;
    } catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
    audit = artifacts / "audit"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts"), str(audit_source),
                    "-lcrypto", "-o", str(audit)], check=True)
    results, red = [], []
    with tempfile.TemporaryDirectory(prefix="duris-lifecycle-reader-", dir=args.state_parent) as temporary:
        candidate = Path(temporary)
        (candidate / "ISOLATED_RESTORE").write_text(json.dumps({"generation": "synthetic"}))
        state = candidate / "state"
        for name in ("identities/accounts", "identities/names", "players", "domains"):
            (state / name).mkdir(parents=True, mode=0o700)
        evidence = state / "economic-evidence"

        def install(files):
            evidence.mkdir(mode=0o700, exist_ok=True)
            for path in evidence.iterdir():
                path.unlink()
            for name, value in files.items():
                (evidence / name).write_bytes(value)

        def run(tool, arguments, sanitized=False):
            return subprocess.run([str(tool), *map(str, arguments)], capture_output=True,
                                  text=True, timeout=30, env=environment if sanitized else None)

        def check(label, files, valid=False, receipts=1, unsafe=None, limitation=None, never=0,
                  origins=(0, 0, 1, True)):
            install(files)
            if unsafe:
                unsafe(evidence)
            before = retained(state)
            parsed = run(audit, [state], True)
            assert parsed.returncode == (0 if valid else 1), (label, parsed.stdout, parsed.stderr)
            assert parsed.stderr == ("" if valid else "native_restore_qualification_failed\n")
            unknown, generic, lifecycle, complete = origins
            assert parsed.stdout == (f"0 {never} 1 1 {receipts} {unknown} {generic} {lifecycle} {int(complete)}\n"
                                     if valid else ""), (label, parsed.stdout)
            assert retained(state) == before, label + ": independent audit wrote state"
            operator = run(binary, ["--economic-evidence-audit", state])
            assert operator.returncode == (0 if valid else 1), (label, operator.stderr)
            if valid:
                assert json.loads(operator.stdout) == {
                    "legacy_unknown_epochs": 0, "never_initialized_epochs": never,
                    "initialized_epochs": 1, "baseline_provenance_complete": True,
                    "lifecycle_receipts": receipts, "unknown_initialized_origins": unknown,
                    "baseline_participant_epochs": generic, "lifecycle_owner_epochs": lifecycle,
                    "lifecycle_provenance_complete": complete}
                assert not operator.stderr
            else:
                assert not operator.stdout and operator.stderr == "native_restore_qualification_failed\n"
            assert retained(state) == before, label + ": operator audit wrote state"
            economic_before = retained(evidence)
            qualified = valid and complete
            for arguments in (["--state-preflight", state], [state]):
                result = run(binary, arguments)
                assert result.returncode == (0 if qualified else 1), (label, result.stderr)
                if qualified:
                    assert json.loads(result.stdout) == {
                        "accounts": 0, "identities": 0, "players_loaded": 0, "snapshots": 0}
                    assert not result.stderr
                else:
                    assert not result.stdout and result.stderr == "native_restore_qualification_failed\n"
                assert retained(evidence) == economic_before, label + ": restore wrote retained evidence"
            measured_budget = None
            if label == "native maximum codec/common-baseline receipt":
                measured = run(audit, ["maximum-budget", state], True)
                assert measured.returncode == 0 and not measured.stderr, measured
                fields = list(map(int, measured.stdout.split()))
                assert len(fields) == 7, fields
                file_limit, size, entries, actual_files, actual_bytes, actual_entries, linked = fields
                assert (file_limit, size, entries, linked) == (16384, 128*1024*1024, 8192, 1), fields
                assert 2048 < actual_files <= file_limit and 0 < actual_bytes <= size, fields
                assert 0 < actual_entries <= entries and retained(state) == before
                measured_budget = dict(maximum_physical_reads=file_limit, maximum_bytes=size,
                    maximum_directory_entries=entries, actual_physical_reads=actual_files,
                    actual_bytes=actual_bytes, actual_directory_entries=actual_entries,
                    lifecycle_receipts=linked, sanitizer_instrumented=True)
            results.append({"label": label, "accepted": valid, "lifecycle_receipts": receipts if valid else None,
                            "lifecycle_provenance_complete": qualified,
                            "files": len(files), "bytes": sum(map(len, files.values())),
                            "limitation": limitation, "measured_operator_budget": measured_budget})
            print(("ACCEPTED " if valid else "REFUSED ") + label, flush=True)

        def produce(mode):
            install({})
            subprocess.run([str(fixture), str(state), mode], env=environment, check=True, timeout=600)
            files = inventory(evidence)
            assert struct.unpack_from("<I", files["epochs.eae"], 8)[0] == 3
            assert files["authority.eal"][112:128] == bytes(16), "fixture selected an active epoch"
            frozen = artifacts / ("native-" + mode)
            frozen.mkdir(mode=0o700)
            for name, value in files.items():
                (frozen / name).write_bytes(value)
            return files

        mixed = produce("mixed")
        receipt = next(name for name in mixed if name.endswith(".elr"))
        mappings, counts, sources, blobs = layout(mixed[receipt])
        # An older v1/v2 reader must refuse the new healthy format. This is a
        # compatibility refusal, not a semantic RED for lifecycle installation.
        if args.previous_qualifier:
            before = retained(evidence)
            for arguments in (["--economic-evidence-audit", state], ["--state-preflight", state], [state]):
                result = run(args.previous_qualifier, arguments)
                assert result.returncode == 1 and not result.stdout
                assert result.stderr == "native_restore_qualification_failed\n"
                assert retained(evidence) == before
                red.append({"label": "previous reader refuses healthy native v3", "arguments": list(map(str, arguments)),
                            "exit": result.returncode, "semantic_RED": False})
            print("ESTABLISHED: previous reader refuses healthy native v3 at all three gates", flush=True)
        check("native mixed codec/common-baseline receipt", mixed, True)
        for mode in ("empty", "renamed", "retired", "maximum"):
            check("native " + mode + " codec/common-baseline receipt", produce(mode), True)
        check("native old retained receipt, newer inactive epoch and explicit requested coverage",
              produce("retained"), True, never=1)
        check("native generic coincident initialization IDs", produce("generic"), True, 0,
              origins=(0, 1, 0, True))
        missing = dict(mixed)
        missing.pop(receipt)
        check("entire required lifecycle file loss", missing)
        for mode in ("retained", "renamed", "retired"):
            files = inventory(artifacts/("native-"+mode))
            files.pop(receipt)
            check("required old receipt loss after " + mode, files)
        if args.legacy_artifacts:
            for mode in ("mixed", "generic", "retained"):
                files = inventory(args.legacy_artifacts/("native-"+mode))
                assert struct.unpack_from("<I", files["epochs.eae"], 8)[0] == 2
                check("actual native v2 " + mode + " remains origin unknown", files, True,
                      0 if mode == "generic" else 1, never=int(mode == "retained"), origins=(1, 0, 0, False))
                if mode != "generic":
                    files = dict(files)
                    files.pop(receipt)
                    check("actual native v2 " + mode + " missing receipt stays unknown", files, True, 0,
                          never=int(mode == "retained"), origins=(1, 0, 0, False))

        def cut(label, offset=None, value=None, mutate=None):
            files = dict(mixed)
            if mutate:
                mutate(files)
            else:
                change(files, receipt, offset, value)
            check(label, files)

        def number_cut(label, offset, number, width=8):
            cut(label, offset, number.to_bytes(width, "little"))

        def catalog_change(files, offset, value):
            change(files, "epochs.eae", offset, value, 152)
        for origin in (3, 255):
            cut("invalid origin enum " + str(origin),
                mutate=lambda f, o=origin: catalog_change(f, 169, bytes([o])))
        cut("known baseline origin conflicts with present lifecycle receipt",
            mutate=lambda f: catalog_change(f, 169, b"\1"))
        for initialization_state in (0, 1):
            for origin in (1, 2):
                cut(f"known origin {origin} in noninitialized state {initialization_state}",
                    mutate=lambda f, s=initialization_state, o=origin: catalog_change(f, 168, bytes([s, o])+bytes(62)))
        for offset in range(170, 176):
            cut("origin reserved byte " + str(offset),
                mutate=lambda f, o=offset: catalog_change(f, o, b"\1"))
        cut("lifecycle origin creator differs from initializer",
            mutate=lambda f: catalog_change(f, 152, b"\x63"))
        cut("lifecycle origin transition is not initialization",
            mutate=lambda f: catalog_change(f, 112, b"\2"))
        unknown = dict(mixed)
        catalog_change(unknown, 169, b"\0")
        check("v3 unknown origin with valid receipt does not promote provenance", unknown, True,
              origins=(1, 0, 0, False))
        unknown.pop(receipt)
        check("v3 unknown origin without receipt does not infer generic origin", unknown, True, 0,
              origins=(1, 0, 0, False))
        duplicate = inventory(artifacts/"native-retained")
        catalog_change(duplicate, 312, duplicate["epochs.eae"][152:168])
        catalog_change(duplicate, 328, duplicate["epochs.eae"][168:232])
        check("duplicate lifecycle initializer across retained epochs", duplicate)
        cut("duplicate original receipt under foreign operation filename", mutate=lambda f: f.__setitem__(
            "lifecycle-"+"63"*16+".elr", f[receipt]))

        cut("invalid lifecycle filename", mutate=lambda f: f.__setitem__("lifecycle-bad.elr", b"invalid"))
        cut("upper-case operation filename", mutate=lambda f: f.__setitem__(
            "lifecycle-" + "A" * 32 + ".elr", f.pop(receipt)))
        for label, offset, value in (
            ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
            ("body size", 12, bytes(4)), ("original operation", 48, b"\x63"),
            ("lineage", 64, b"\x63"), ("epoch", 80, b"\x63"),
            ("requested coverage mismatch", 112, b"\x63"),
            ("resolved coverage", 144, bytes(32)), ("boundary", 176, bytes(32)),
            ("externally asserted flags", 208, b"\0"), ("unknown flags", 208, b"\7"),
            ("opening lineage", 209, b"\x63"), ("opening kind", 227, b"\1"),
            ("opening authority", 229, bytes(8)), ("opening context", 237, bytes(8)),
            ("derived baseline operation", 249, b"\x63"),
            ("lineage creating operation", 273, b"\x63"),
            ("immutable epoch", 297, b"\x63"), ("predecessor", 313, b"\x63"),
            ("epoch creating operation", 329, b"\x63"), ("transition kind", 353, b"\2"),
            ("transition digest", 355, b"\x63"), ("initialization state", 387, b"\1"),
            ("initializing operation", 388, b"\x63"), ("initialization opening", 404, b"\x63")):
            cut(label, offset, value)
        for label, offset, value, width in (
            ("actor zero", 96, 0, 8), ("accepted zero", 104, 0, 8),
            ("accepted disagrees with capsule", 104, 2, 8),
            ("baseline revision zero", 265, 0, 8), ("baseline revision mismatch", 265, 2, 8),
            ("control revision zero", 289, 0, 8), ("future control revision", 289, 2**64-1, 8),
            ("epoch ordinal", 345, 2, 8), ("mapping capacity", 444, 3072, 4),
            ("wallet count", counts, 3, 4), ("bank count", counts + 4, 2, 4)):
            number_cut(label, offset, value, width)
        cut("checksum", mutate=lambda f: f.__setitem__(receipt, f[receipt][:16] + bytes(32) + f[receipt][48:]))
        cut("truncated receipt", mutate=lambda f: f.__setitem__(receipt, f[receipt][:-1]))
        cut("encoded receipt capacity", mutate=lambda f: f.__setitem__(receipt, f[receipt] + bytes(6377937)))
        def tail(files):
            value = bytearray(files[receipt] + b"\0")
            struct.pack_into("<I", value, 12, len(value)-48)
            files[receipt] = rehash(value)
        cut("valid-envelope trailing byte", mutate=tail)
        for index, (at, length) in enumerate(mappings):
            for label, offset, value in (
                ("account lineage", at, b"\x63"), ("account reserved", at+36, b"\1"),
                ("account authority", at+20, bytes(8)), ("locator kind", at+40, bytes(2)),
                ("native identifier", at+42, bytes(8)), ("alias length", at+50, b"\xff\xff"),
                ("creating operation", at+52+length, bytes(16)),
                ("retiring operation", at+68+length, b"\x63"),
                ("last operation", at+84+length, bytes(16)),
                ("revision zero mismatched last", at+84+length, b"\x63")):
                cut(f"mapping {index} {label}", offset, value)
        cut("duplicate original mapping", mappings[1][0], mixed[receipt][mappings[0][0]:mappings[0][0]+40])
        for index, (start, name_at, length, wallet) in enumerate(sources):
            for label, offset, value in (
                ("name length", name_at, b"\xff\xff"), ("name grammar", name_at+2, b"A"),
                ("native context", name_at+2+length, b"\xff"),
                ("native revision", name_at+3+length, bytes(8)),
                ("balance", name_at+11+length, struct.pack("<Q", 2**63)),
                ("source fingerprint", name_at+11+length, struct.pack("<Q", 1))):
                cut(f"source {index} {label}", offset, value)
            if wallet:
                cut("wallet source native PID mismatch", start, struct.pack("<I", 2))
        for index, (size_at, start, length) in enumerate(blobs):
            cut(f"capsule {index} excessive size", size_at, struct.pack("<I", 2**32-1))
            cut(f"capsule {index} content mismatch", start, b"X")
        command_at = blobs[0][1]
        for label, offset, value in (("source", 28, b"\1"), ("deadline", 30, b"\1"),
                                     ("fence", 60, bytes(8)), ("intent", 120, b"X")):
            cut("canonical command " + label, command_at+offset, value)
        witness_at = blobs[1][1]
        for label, offset, value in (("actor", 64, bytes(8)), ("coverage", 152, bytes(32)),
                                     ("holding account", 192, b"\x63"),
                                     ("holding native revision", 264, bytes(8)),
                                     ("holding source digest", 272, bytes(32))):
            cut("baseline witness " + label, witness_at+offset, value)
        # Alter common roots while keeping the index/segment/record envelopes
        # valid. These cuts exercise the exact retained receipt link.
        operation = mixed[receipt][249:265]
        index_name = "bucket-" + operation[:1].hex() + ".eai"
        def rewrite_root(files, offset, value):
            index = files[index_name]
            rows = struct.unpack_from("<I", index, 68)[0]
            row = next(n for n in range(rows) if index[80+n*64:96+n*64] == operation)
            segment, position, size = struct.unpack_from("<III", index, 128+row*64)
            segment_name = index_name[:-4] + "-" + str(segment) + ".eas"
            encoded = bytearray(files[segment_name][80+position:80+position+size])
            encoded[offset:offset+len(value)] = value
            encoded = rehash(encoded)
            stored = bytearray(files[segment_name])
            stored[80+position:80+position+size] = encoded
            files[segment_name] = rehash(stored)
            change(files, index_name, 96+row*64, hashlib.sha256(encoded).digest())
        for label, offset, value in (("durable revision", 64, struct.pack("<Q", 2)),
                                     ("result status", 60, struct.pack("<I", 1)),
                                     ("failure stage", 72, b"\1\0"),
                                     ("command accepted time", 74+32, struct.pack("<Q", 2)),
                                     ("plan capsule", 74+376, b"X")):
            cut("retained common root " + label,
                mutate=lambda f, o=offset, v=value: rewrite_root(f, o, v))
        cut("entire referenced common bucket loss", mutate=lambda f: (
            f.pop(index_name), f.pop(index_name[:-4]+"-0.eas")))
        for label, unsafe in (
            ("world-readable receipt", lambda d: (d/receipt).chmod(0o644)),
            ("hard-linked receipt", lambda d: os.link(d/receipt, d/"unrecognized-hardlink")),
            ("symlink receipt", lambda d: ((d/receipt).rename(d/"unrecognized-target"),
                                          (d/receipt).symlink_to("unrecognized-target"))),
            ("FIFO receipt", lambda d: ((d/receipt).unlink(), os.mkfifo(d/receipt, 0o600)))):
            check(label, mixed, unsafe=unsafe)
    page_report = check_lifecycle_pages(binary, fixture, audit, artifacts, environment)
    assert fingerprint(ROOT / "src") == native_inputs, "native source changed during test"
    if args.legacy_artifacts:
        assert fingerprint(args.legacy_artifacts) == legacy_inputs, "legacy fixture artifacts changed"
    for name, checksum in owned_inputs.items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == checksum, name + " changed during test"
    report = {"format": 1, "native_raw_inputs": native_inputs, "owned_inputs": owned_inputs,
              "cases": results, "case_count": len(results), "refused": sum(not row["accepted"] for row in results),
              "accepted": sum(row["accepted"] for row in results), "skips": 0,
              "lifecycle_pages": page_report,
              "previous_reader_compatibility_refusals": red, "legacy_inputs": legacy_inputs,
              "source_capture_executed": False,
              "lifecycle_install_executed": False, "activation_executed": False,
              "required_file_discovery_qualified": True,
              "qualification_scope": "known lifecycle origins in modeled native participant/codec fixtures",
              "release_qualified": False,
              "read_only_checks": "bytes, mode, links, inode, size, mtime_ns",
              "state_filesystem": str(args.state_parent) if args.state_parent else "system temporary directory",
              "binary_sha256": {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                                for path in (binary, fixture, audit,
                                             *((args.previous_qualifier,) if args.previous_qualifier else ()))}}
    (artifacts/"evidence.json").write_text(json.dumps(report, indent=2, sort_keys=True)+"\n")
    print(json.dumps({key: value for key, value in report.items()
                      if key not in ("native_raw_inputs", "owned_inputs", "cases", "legacy_inputs",
                                     "previous_reader_compatibility_refusals")}))


if __name__ == "__main__":
    main()
