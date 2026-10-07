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
            if (std::string(argv[1]) != "maximum-budget") return 2;
            restore_economic_authority::audit_budget budget;
            const auto files = budget.remaining_files, bytes = budget.remaining_bytes,
                       entries = budget.remaining_entries;
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
    assert fingerprint(ROOT / "src") == native_inputs, "native source changed during test"
    if args.legacy_artifacts:
        assert fingerprint(args.legacy_artifacts) == legacy_inputs, "legacy fixture artifacts changed"
    for name, checksum in owned_inputs.items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == checksum, name + " changed during test"
    report = {"format": 1, "native_raw_inputs": native_inputs, "owned_inputs": owned_inputs,
              "cases": results, "case_count": len(results), "refused": sum(not row["accepted"] for row in results),
              "accepted": sum(row["accepted"] for row in results), "skips": 0,
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
