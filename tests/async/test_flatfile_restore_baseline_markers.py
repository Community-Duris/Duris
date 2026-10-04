#!/usr/bin/env python3
"""Source-paired native marker-v2 fixtures and independent read-only restore cuts.

Pass the complete native checkout explicitly while its shared implementation is
being integrated. This does not qualify an installer, activation or release.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from test_flatfile_restore_economic_authority import ROOT, build_fixture, change, inventory, rehash

sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as qualifier


def fingerprint(directory):
    return {str(path.relative_to(directory)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(directory.rglob("*")) if path.is_file()}


def retained(directory):
    result = {}
    for path in sorted(directory.rglob("*")):
        info = path.lstat()
        result[str(path.relative_to(directory))] = {
            "mode": info.st_mode, "links": info.st_nlink, "inode": info.st_ino,
            "size": info.st_size, "mtime_ns": info.st_mtime_ns,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-source", required=True, type=Path)
    parser.add_argument("--legacy-native-source", type=Path)
    parser.add_argument("--artifacts", required=True, type=Path)
    args = parser.parse_args()
    native = args.native_source.resolve()
    artifacts = args.artifacts.resolve()
    if not artifacts.is_relative_to((ROOT / "bin").resolve()) or artifacts.exists():
        parser.error("artifacts must be a fresh directory below bin/")
    if not (native / "src/flatfile/flatfile_accounting_authority.c").is_file():
        parser.error("native-source must contain the complete source tree")
    os.umask(0o077)
    artifacts.mkdir(parents=True)
    source_before = fingerprint(native / "src")
    legacy_native = args.legacy_native_source.resolve() if args.legacy_native_source else None
    legacy_before = fingerprint(legacy_native / "src") if legacy_native else None
    owned = {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in (
        "scripts/qualify_flatfile_economic_authority.h",
        "scripts/qualify_flatfile_economic_baseline.h",
        "scripts/qualify_flatfile_economic_records.h", "scripts/qualify_flatfile_restore.cpp",
        "scripts/build_restore_qualifier.py", "tests/async/flatfile_restore_authority_fixture.cpp",
        "tests/async/test_flatfile_restore_economic_authority.py",
        "tests/async/test_flatfile_restore_baseline_markers.py")}
    binary = qualifier.build(artifacts / "qualify")
    fixture = build_fixture(artifacts / "fixture", native)
    legacy_fixture = build_fixture(artifacts / "fixture-legacy", legacy_native) if legacy_native else None
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    # Independently decode without including any native header or storage API.
    audit_source = artifacts / "audit.cpp"
    audit_source.write_text('''#include "qualify_flatfile_economic_records.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    try {
        auto result = restore_economic_records::checker(argv[1]).run();
        std::cout << result.legacy_unknown_epochs << " " << result.never_initialized_epochs
                  << " " << result.initialized_epochs << " " << result.complete() << "\\n";
        return 0;
    } catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
    audit = artifacts / "audit"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts"), str(audit_source),
                    "-lcrypto", "-o", str(audit)], check=True)
    results = []
    with tempfile.TemporaryDirectory(prefix="duris-marker-restore-") as temporary:
        candidate = Path(temporary)
        state = candidate / "state"
        for name in ("identities/accounts", "identities/names", "players", "domains"):
            (state / name).mkdir(parents=True, mode=0o700)
        (candidate / "ISOLATED_RESTORE").write_text(json.dumps({"generation": "synthetic"}))
        evidence = state / "economic-evidence"

        def install(files):
            evidence.mkdir(mode=0o700, exist_ok=True)
            for path in evidence.iterdir():
                path.unlink()
            for name, value in files.items():
                (evidence / name).write_bytes(value)

        def check(label, files, expected=None):
            install(files)
            before = retained(state)
            parsed = subprocess.run([str(audit), str(state)], env=environment,
                                    capture_output=True, text=True, timeout=30)
            valid = expected is not None
            assert parsed.returncode == (0 if valid else 1), (label, parsed.stdout, parsed.stderr)
            assert parsed.stderr == ("" if valid else "native_restore_qualification_failed\n")
            assert parsed.stdout == (" ".join(str(value) for value in expected) + "\n"
                                     if valid else ""), (label, parsed.stdout)
            assert retained(state) == before, label + ": independent audit wrote state"
            operator = subprocess.run([str(binary), "--economic-evidence-audit", str(state)],
                                      capture_output=True, text=True, timeout=30)
            assert operator.returncode == (0 if valid else 1), (label, operator.stderr)
            if valid:
                assert json.loads(operator.stdout) == {
                    "legacy_unknown_epochs": expected[0], "never_initialized_epochs": expected[1],
                    "initialized_epochs": expected[2], "baseline_provenance_complete": bool(expected[3])}
                assert not operator.stderr
            else:
                assert not operator.stdout and operator.stderr == "native_restore_qualification_failed\n"
            assert retained(state) == before, label + ": operator audit wrote state"
            qualified = valid and bool(expected[3])
            economic_before = retained(evidence)
            for arguments in (["--state-preflight", str(state)], [str(state)]):
                result = subprocess.run([str(binary), *arguments], capture_output=True, text=True,
                                        timeout=30)
                assert result.returncode == (0 if qualified else 1), (label, result.stdout, result.stderr)
                if qualified:
                    assert json.loads(result.stdout) == {
                        "accounts": 0, "identities": 0, "players_loaded": 0, "snapshots": 0}
                    assert not result.stderr
                else:
                    assert not result.stdout and result.stderr == "native_restore_qualification_failed\n"
                assert retained(evidence) == economic_before, label + ": qualification wrote evidence"
            results.append({"label": label, "structurally_readable": valid,
                            "baseline_provenance_complete": qualified,
                            "provenance": expected, "files": len(files),
                            "encoded_bytes": sum(map(len, files.values()))})
            print(("QUALIFIED " if qualified else "READABLE_UNQUALIFIED " if valid else "REFUSED ")
                  + label, flush=True)

        def native_store(mode, producer=fixture, version=2):
            if evidence.exists():
                for path in evidence.iterdir():
                    path.unlink()
            subprocess.run([str(producer), str(state), mode], env=environment, check=True)
            files = inventory(evidence)
            assert struct.unpack_from("<I", files["epochs.eae"], 8)[0] == version, \
                "explicit native source does not implement the requested catalog version"
            frozen = artifacts / ("native-v" + str(version) + "-" + mode)
            frozen.mkdir(mode=0o700)
            for name, value in files.items():
                (frozen / name).write_bytes(value)
            return files

        check("native empty catalog", native_store("bootstrap"), (0, 0, 0, 1))
        never = native_store("lifetimes")
        check("native never initialized, inactive retained epochs", never, (0, 2, 0, 1))
        empty = native_store("baseline-empty")
        check("native initialized empty old epoch with newer inactive epoch", empty, (0, 1, 1, 1))
        populated = native_store("baseline-rich")
        check("native populated books in both inactive retained epochs", populated, (0, 0, 2, 1))
        if legacy_fixture:
            for mode in ("baseline-empty", "baseline-rich"):
                check("actual native v1 " + mode + " exposes unknown provenance",
                      native_store(mode, legacy_fixture, 1), (2, 0, 0, 0))

        def cut(label, mutate, original=empty, expected=None):
            files = dict(original)
            mutate(files)
            check(label, files, expected)

        def catalog_change(files, offset, value, bind=True):
            change(files, "epochs.eae", offset, value, 152 if bind else None)

        def book_names(files, epoch):
            return sorted(name for name in files if name.startswith("baseline-")
                          and name[42:74] == epoch)

        head = next(name for name in empty if name.endswith("head.ebc"))
        epoch = head[42:74]
        namespace = book_names(empty, epoch)
        assert len(namespace) == 17
        cut("complete initialized empty book loss", lambda f: [f.pop(name) for name in namespace])
        for name in namespace:
            cut("partial initialized empty book loss " + name[75:], lambda f, n=name: f.pop(n))
        for retained_epoch in sorted({name[42:74] for name in populated if name.endswith("head.ebc")}):
            cut("complete populated retained book loss " + retained_epoch,
                lambda f, e=retained_epoch: [f.pop(name) for name in book_names(f, e)], populated)
        cut("revision-zero head initialization ID substitution",
            lambda f: change(f, head, 128, b"\x7f" + b"\0" * 15))
        cut("revision-zero head opening authority substitution",
            lambda f: change(f, head, 100, struct.pack("<Q", 2)))
        cut("revision-zero head opening context substitution",
            lambda f: change(f, head, 108, struct.pack("<Q", 1)))
        cut("coherent synthetic maximum opening authority and context", lambda f: (
            change(f, head, 100, b"\xff" * 16),
            catalog_change(f, 212, b"\xff" * 16)), expected=(0, 1, 1, 1))
        cut("marker original operation substitution", lambda f: catalog_change(f, 176, b"\x7f"))
        cut("marker exact opening authority substitution",
            lambda f: catalog_change(f, 212, struct.pack("<Q", 2)))
        cut("marker exact opening context substitution",
            lambda f: catalog_change(f, 220, struct.pack("<Q", 1)))
        cut("stale authority digest with valid catalog checksum",
            lambda f: catalog_change(f, 176, b"\x7f", False))
        cut("marker zero original initialization ID", lambda f: catalog_change(f, 176, b"\0" * 16))
        cut("marker opening wrong lineage", lambda f: catalog_change(f, 192, b"\x63"))
        cut("marker opening wrong version", lambda f: catalog_change(f, 208, b"\x02"))
        cut("marker opening wrong kind", lambda f: catalog_change(f, 210, b"\x01"))
        cut("marker opening zero authority", lambda f: catalog_change(f, 212, b"\0" * 8))
        cut("marker opening reserved", lambda f: catalog_change(f, 228, b"\x01"))
        for state_value in (3, 255):
            cut("invalid marker state " + str(state_value),
                lambda f, s=state_value: catalog_change(f, 168, bytes([s])))
        for byte in range(7):
            cut("marker reserved byte " + str(byte),
                lambda f, b=byte: catalog_change(f, 169 + b, b"\x01"))
        for state_value in (0, 1):
            cut("nonzero proof in state " + str(state_value),
                lambda f, s=state_value: catalog_change(f, 168, bytes([s])))
            cut("nonzero operation in zero-key state " + str(state_value),
                lambda f, s=state_value: (
                    catalog_change(f, 168, bytes([s])), catalog_change(f, 192, b"\0" * 40)))
            cut("nonzero opening in zero-ID state " + str(state_value),
                lambda f, s=state_value: (
                    catalog_change(f, 168, bytes([s])), catalog_change(f, 176, b"\0" * 16)))

        def unproven(files, state_value):
            catalog_change(files, 168, bytes([state_value]) + b"\0" * 63)
        cut("never-initialized marker with surviving empty book", lambda f: unproven(f, 1))
        cut("never-initialized marker with successful baseline receipt", lambda f: unproven(f, 1), populated)
        cut("explicit unknown marker retains structural readability", lambda f: unproven(f, 0),
            expected=(1, 1, 0, 0))
        cut("explicit unknown marker with missing book stays unqualified", lambda f: (
            unproven(f, 0), [f.pop(name) for name in namespace]), expected=(1, 1, 0, 0))

        def legacy(files):
            catalog = files["epochs.eae"]
            count = struct.unpack_from("<I", catalog, 64)[0]
            body = catalog[48:72] + b"".join(catalog[72 + 160*i:168 + 160*i] for i in range(count))
            files["epochs.eae"] = rehash(catalog[:8] + struct.pack("<II", 1, len(body))
                                         + b"\0" * 32 + body)
            control = bytearray(files["authority.eal"])
            control[152:184] = hashlib.sha256(files["epochs.eae"]).digest()
            files["authority.eal"] = rehash(control)
        for mode, files in (("never", never), ("empty", empty), ("populated", populated)):
            cut("v1 " + mode + " history readable with unknown provenance", legacy, files, (2, 0, 0, 0))
        cut("v1 completely missing initialized book remains unqualified", lambda f: (
            legacy(f), [f.pop(name) for name in namespace]), expected=(2, 0, 0, 0))
        cut("coherently downgraded populated history remains unqualified", legacy, populated, (2, 0, 0, 0))

        # Bounded format checks supplement native fixtures; synthetic capacity
        # and coherent proof replacements do not establish authentic provenance.
        def maximum(files, count):
            row = files["epochs.eae"][72:232]
            body = bytearray(files["epochs.eae"][48:72])
            body[16:20] = struct.pack("<I", count)
            previous = b"\0" * 16
            for ordinal in range(1, count + 1):
                current = ordinal.to_bytes(16, "little")
                item = bytearray(row)
                item[:16] = current
                item[16:24] = struct.pack("<Q", ordinal)
                item[24:40] = previous
                item[96:] = b"\x01" + b"\0" * 63
                body.extend(item)
                previous = current
            files["epochs.eae"] = rehash(files["epochs.eae"][:8] + struct.pack("<II", 2, len(body))
                                         + b"\0" * 32 + body)
            control = bytearray(files["authority.eal"])
            control[96:112] = previous
            control[144:148] = struct.pack("<I", count)
            control[128:136] = b"\xff" * 8
            control[152:184] = hashlib.sha256(files["epochs.eae"]).digest()
            files["authority.eal"] = rehash(control)
        cut("synthetic maximum 4096-row marker catalog", lambda f: maximum(f, 4096), never,
            (0, 4096, 0, 1))
        assert results[-1]["encoded_bytes"] >= 655432
        cut("4097-row marker catalog refuses", lambda f: maximum(f, 4097), never)
        cut("version 3 catalog refuses", lambda f: catalog_change(f, 8, struct.pack("<I", 3)))
        cut("v2 extension truncation", lambda f: catalog_change(f, 12, struct.pack("<I", len(f["epochs.eae"])-49)))
        cut("non-catalog version2 remains refused", lambda f: change(f, "authority.eal", 8, struct.pack("<I", 2)))
        cut("catalog duplicate epoch", lambda f: catalog_change(f, 232, f["epochs.eae"][72:88]))
        cut("catalog wrong predecessor", lambda f: catalog_change(f, 256, b"\x63"))
        cut("catalog wrong ordinal", lambda f: catalog_change(f, 248, b"\x63"))

    assert fingerprint(native / "src") == source_before, "native inputs changed during qualification"
    if legacy_native:
        assert fingerprint(legacy_native / "src") == legacy_before, "legacy native inputs changed during qualification"
    for name, checksum in owned.items():
        assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == checksum, name + " changed during qualification"
    report = {"format": 1, "native_source": str(native), "native_raw_inputs": source_before,
              "legacy_native_source": str(legacy_native) if legacy_native else None,
              "legacy_native_raw_inputs": legacy_before,
              "owned_inputs": owned, "cases": results,
              "case_count": len(results), "structural_refusals": sum(not row["structurally_readable"] for row in results),
              "readable_unqualified": sum(row["structurally_readable"] and not row["baseline_provenance_complete"] for row in results),
              "provenance_qualified": sum(row["baseline_provenance_complete"] for row in results),
              "skips": 0, "economic_evidence_unchanged": True,
              "independent_and_operator_state_unchanged": True,
              "binary_sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                                for path in (binary, fixture, audit, *((legacy_fixture,) if legacy_fixture else ()))}}
    (artifacts / "evidence.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps({key: value for key, value in report.items()
                      if key not in ("cases", "native_raw_inputs", "legacy_native_raw_inputs", "owned_inputs")}))


if __name__ == "__main__":
    main()
