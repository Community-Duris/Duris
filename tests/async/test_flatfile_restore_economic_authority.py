#!/usr/bin/env python3
"""Actual native restore refusal of damaged retained accounting authority."""
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
import build_restore_qualifier as qualifier
from native_build_artifacts import build_native
from test_flatfile_accounting_store import SOURCES


def inventory(directory):
    return {path.name: path.read_bytes() for path in directory.iterdir()}


def rehash(frame):
    result = bytearray(frame)
    result[16:48] = hashlib.sha256(result[48:]).digest()
    return bytes(result)


def change(files, name, offset, data, bind=None):
    frame = bytearray(files[name])
    frame[offset:offset + len(data)] = data
    files[name] = rehash(frame)
    if bind is not None:
        control = bytearray(files["authority.eal"])
        control[bind:bind + 32] = hashlib.sha256(files[name]).digest()
        files["authority.eal"] = rehash(control)


def build_fixture(destination):
    return build_native(
        destination,
        ["tests/async/flatfile_restore_authority_fixture.cpp",
         "src/flatfile/flatfile_accounting_authority.c", *SOURCES[1:]],
        ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
         "-DDURIS_FLATFILE_ACCOUNTING_TEST", "-Isrc", "-pthread"],
        ["-lcrypto", "-pthread"], compiler="g++", name="restore-authority-fixture")


def main():
    os.umask(0o077)
    with tempfile.TemporaryDirectory(prefix="duris-restore-authority-build-",
                                     dir=ROOT / "bin/tests") as build:
        binary = qualifier.build(Path(build) / "qualify")
        fixture = build_fixture(Path(build) / "fixture")
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        # Exercise the independent reader under sanitizers without invoking
        # candidate recovery or any native mutation/storage interface.
        audit_source = Path(build) / "audit.cpp"
        audit_source.write_text('''#include "qualify_flatfile_economic_records.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    try { restore_economic_records::checker(argv[1]).run(); return 0; }
    catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
        audit = Path(build) / "audit"
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts"), str(audit_source),
                        "-lcrypto", "-o", str(audit)], check=True)
        successes, refusals = 0, 0
        with tempfile.TemporaryDirectory(prefix="duris-restore-authority-state-") as temporary:
            candidate = Path(temporary)
            (candidate / "ISOLATED_RESTORE").write_text(json.dumps({"generation": "synthetic"}))
            state = candidate / "state"
            evidence = state / "economic-evidence"
            for name in ("identities/accounts", "identities/names", "players", "domains"):
                (state / name).mkdir(parents=True, mode=0o700)

            def check(label, valid):
                nonlocal successes, refusals
                # lstat-aware inventory also covers unsafe metadata without following it.
                def retained():
                    if not evidence.exists():
                        # Existing native recovery may provision an empty private
                        # directory. No retained evidence exists in either case.
                        return {}
                    result = {}
                    for path in evidence.iterdir():
                        info = path.lstat()
                        payload = (path.read_bytes() if path.is_file() and not path.is_symlink()
                                   else os.readlink(path) if path.is_symlink() else None)
                        result[path.name] = (info.st_mode, info.st_nlink, payload)
                    return result
                before = retained()
                independent = subprocess.run([str(audit), str(state)], env=environment,
                                             capture_output=True, text=True, timeout=30)
                assert (independent.returncode == 0) == valid, (label, independent.stderr)
                assert not independent.stdout
                assert independent.stderr == ("" if valid else "native_restore_qualification_failed\n")
                assert retained() == before, label + ": independent reader changed evidence"
                # Both manager preflight and post-replay qualification use this same gate.
                for arguments in (["--state-preflight", str(state)], [str(state)]):
                    result = subprocess.run([str(binary), *arguments], capture_output=True,
                                            text=True, timeout=30, check=False)
                    assert (result.returncode == 0) == valid, (label, result.returncode,
                                                              result.stdout, result.stderr)
                    if valid:
                        assert json.loads(result.stdout) == {
                            "accounts": 0, "identities": 0, "players_loaded": 0, "snapshots": 0}
                    else:
                        assert not result.stdout and result.stderr.strip() == \
                            "native_restore_qualification_failed", (label, result.stderr)
                    assert retained() == before, label + ": retained authority changed"
                successes += valid
                refusals += not valid
                print(("PASS " if valid else "REFUSED ") + label, flush=True)

            check("legacy authority absent", True)
            evidence.mkdir(mode=0o700, exist_ok=True)
            check("legacy authority empty", True)
            (evidence / "authority.eal").write_bytes(b"corrupt-economic-control")
            check("original false qualification", False)
            (evidence / "authority.eal").unlink()
            subprocess.run([str(fixture), str(state), "bootstrap"], env=environment, check=True)
            check("native partial inactive bootstrap", True)
            for path in evidence.iterdir():
                path.unlink()
            subprocess.run([str(fixture), str(state), "evidence-bootstrap"], env=environment, check=True)
            check("native empty initialized evidence before any epoch", True)
            for path in evidence.iterdir():
                path.unlink()
            subprocess.run([str(fixture), str(state), "lifetimes"], env=environment, check=True)
            check("native inactive epochs lifetimes rename retirement", True)
            clean = inventory(evidence)

            def restore(files):
                for path in evidence.iterdir():
                    path.unlink()
                for name, data in files.items():
                    (evidence / name).write_bytes(data)
                evidence.chmod(0o700)

            cases = []
            def case(label, mutate):
                files = dict(clean)
                mutate(files)
                cases.append((label, files))

            case("control body checksum", lambda f: f.update({
                "authority.eal": f["authority.eal"][:-1] + b"\x01"}))
            case("control magic", lambda f: change(f, "authority.eal", 0, b"X"))
            case("control version", lambda f: change(f, "authority.eal", 8, struct.pack("<I", 2)))
            case("control body length", lambda f: change(f, "authority.eal", 12, b"\x00" * 4))
            case("control zero lineage", lambda f: change(f, "authority.eal", 48, b"\x00" * 16))
            case("control zero creating operation", lambda f: change(f, "authority.eal", 64, b"\x00" * 16))
            case("control reserved", lambda f: change(f, "authority.eal", 148, b"\x01"))
            case("control next mapping zero", lambda f: change(f, "authority.eal", 136, b"\x00" * 8))
            case("control mapping capacity", lambda f: change(f, "authority.eal", 136,
                                                               struct.pack("<Q", 256 * 4096 + 2)))
            case("control epoch capacity", lambda f: change(f, "authority.eal", 144, struct.pack("<I", 4097)))
            case("control active differs from latest", lambda f: change(f, "authority.eal", 112, b"\x63"))
            case("missing epochs", lambda f: f.pop("epochs.eae"))
            case("catalog hash mismatch", lambda f: change(f, "epochs.eae", 88, b"\x02"))
            case("catalog lineage", lambda f: change(f, "epochs.eae", 48, b"\x63", 152))
            case("catalog count", lambda f: change(f, "epochs.eae", 64, b"\x01", 152))
            case("catalog ordinal", lambda f: change(f, "epochs.eae", 88, b"\x02", 152))
            case("catalog predecessor", lambda f: change(f, "epochs.eae", 192, b"\x63", 152))
            case("catalog duplicate epoch", lambda f: change(f, "epochs.eae", 168, f["epochs.eae"][72:88], 152))
            case("catalog reserved", lambda f: change(f, "epochs.eae", 114, b"\x01", 152))
            case("catalog zero transition digest", lambda f: change(f, "epochs.eae", 120, b"\x00" * 32, 152))
            mapping = "mapping-02.eam" # Native bank allocated lifetime 2.
            binding = 8376 + 2 * 32
            case("missing mapping", lambda f: f.pop(mapping))
            case("mapping hash mismatch", lambda f: change(f, mapping, 104, b"\x02"))
            case("mapping sequence", lambda f: change(f, mapping, 96, b"\x63", binding))
            case("mapping reserved key", lambda f: change(f, mapping, 112, b"\x01", binding))
            case("mapping invalid locator", lambda f: change(f, mapping, 116, b"\x01", binding))
            case("mapping bank id mismatch", lambda f: change(f, mapping, 120, b"\x63", binding))
            case("mapping invalid bank name", lambda f: change(f, mapping, 128, b"!", binding))
            case("mapping zero last operation", lambda f: change(f, mapping, 128 + len("renamed") + 32,
                                                                b"\x00" * 16, binding))
            case("mapping zero revision after rename", lambda f: change(f, mapping, 128 + len("renamed") + 48,
                                                                       b"\x00" * 8, binding))
            case("untracked mapping", lambda f: f.update({"mapping-ff.eam": f[mapping]}))
            case("invalid metadata filename", lambda f: f.update({"mapping-GG.eam": f[mapping]}))
            key = struct.pack("<HQH", 2, 1, 2) + b"renamed"
            bucket = hashlib.sha256(key).digest()[0]
            native = f"native-{bucket:02x}.ean"
            native_binding = 184 + bucket * 32
            # The fixture has a single renamed-bank entry in this bucket.
            assert struct.unpack_from("<I", clean[native], 68)[0] == 1
            case("missing native index", lambda f: f.pop(native))
            case("native index hash mismatch", lambda f: change(f, native, 76, b"\x03"))
            case("native index reserved", lambda f: change(f, native, 74, b"\x01", native_binding))
            case("native index wrong bucket", lambda f: change(f, native, 64, b"\x00", native_binding))
            case("native index zero lifetime", lambda f: change(f, native, 84, b"\x00" * 8, native_binding))
            case("native index active differs from last", lambda f: change(f, native, 76, b"\x03", native_binding))
            case("native index dangling active crosslink", lambda f: (
                change(f, native, 76, struct.pack("<Q", 3), native_binding),
                change(f, native, 84, struct.pack("<Q", 3), native_binding)))
            case("native index forged tombstone for active bank", lambda f: change(
                f, native, 76, b"\x00" * 8, native_binding))
            case("partial control", lambda f: f.update({"authority.eal": f["authority.eal"][:48]}))
            for label, files in cases:
                restore(files)
                check(label, False)

            restore(clean)
            maximum = dict(clean)
            change(maximum, "authority.eal", 128, struct.pack("<Q", 2**64 - 1))
            change(maximum, mapping, 128 + len("renamed") + 48,
                   struct.pack("<Q", 2**64 - 1), binding)
            restore(maximum)
            check("unsigned maximum durable revisions", True)
            restore(clean)
            target = evidence / "authority.eal"
            target.chmod(0o644)
            check("nonprivate metadata file", False)
            target.chmod(0o600)
            evidence.chmod(0o755)
            check("nonprivate metadata directory", False)
            evidence.chmod(0o700)
            target.unlink()
            target.symlink_to(evidence / "epochs.eae")
            check("symlink control", False)
            restore(clean)
            os.link(target, evidence / "control-link")
            check("hardlinked control", False)
            restore(clean)
            target.unlink()
            os.mkfifo(target, 0o600)
            check("FIFO control refuses without blocking", False)
            restore(clean)
            check("clean native evidence remains qualified", True)

            # A native-encoded retained operation store has one successful plan,
            # 22 rejections, a sealed segment, an active segment and an empty
            # initialized bucket. No epoch is selected or native gameplay applied.
            restore({})
            subprocess.run([str(fixture), str(state), "records"], env=environment, check=True)
            check("native retained operation store sealed active empty bucket", True)
            records = inventory(evidence)
            index = "bucket-01.eai"
            sealed, active = "bucket-01-0.eas", "bucket-01-1.eas"
            assert sealed in records and active in records
            record_cases = []
            def record_case(label, mutate):
                record_cases.append((label, mutate))

            for name in (index, "bucket-02.eai", sealed, active):
                record_case("missing " + name, lambda f, n=name: f.pop(n))
            record_case("untracked initialized index", lambda f: f.update({
                "bucket-03.eai": f["bucket-02.eai"]}))
            for name in ("bucket-01-2.eas", "bucket-01-70.eas", "bucket-01-00.eas",
                         "bucket-GG.eai", "bucket-01-stray.bin"):
                record_case("untracked segment/name " + name, lambda f, n=name: f.update({n: f[active]}))
            record_case("missing initialization flag", lambda f: change(
                f, "authority.eal", 48 + 16520, b"\x04"))
            record_case("initialization flag without index", lambda f: change(
                f, "authority.eal", 48 + 16520, b"\x0e"))
            record_case("index checksum", lambda f: f.update({index: f[index][:-1] + b"\x01"}))
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("lineage", 48, b"\x63"), ("bucket", 64, b"\x02"),
                ("count capacity", 68, struct.pack("<I", 4097)),
                ("total bytes", 72, b"\x00" * 8), ("zero operation", 80, b"\x00" * 16),
                ("operation bucket", 80, b"\x02"), ("zero digest", 96, b"\x00" * 32),
                ("missing segment number", 128, struct.pack("<I", 2)),
                ("segment capacity", 128, struct.pack("<I", 255)),
                ("nonzero starting offset", 132, b"\x01"),
                ("record below frame minimum", 136, struct.pack("<I", 47)),
                ("reserved", 140, b"\x01")):
                record_case("index " + label, lambda f, o=offset, d=data: change(f, index, o, d))
            record_case("index duplicate operation", lambda f: change(f, index, 144, f[index][80:96]))
            record_case("sealed segment checksum", lambda f: f.update({sealed: f[sealed][:-1] + b"\x01"}))
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("lineage", 48, b"\x63"), ("bucket", 64, b"\x02"),
                ("segment", 68, b"\x01"), ("count", 72, b"\x00" * 4),
                ("reserved", 76, b"\x01")):
                record_case("sealed segment " + label,
                            lambda f, o=offset, d=data: change(f, sealed, o, d))

            # Rehash the segment AND record index when altering record content.
            # This makes the independent wire identity/binding checks decisive.
            first_size = struct.unpack_from("<I", records[index], 136)[0]
            first = records[sealed][80:80 + first_size]
            command_start = 48 + 26
            key_count, rev_count, payload_size = struct.unpack_from("<III", first, command_start + 40)
            intent_start = command_start + 52 + key_count * 16 + rev_count * 24 + payload_size + 4
            command_size = struct.unpack_from("<I", first, 48)[0]
            plan_start = command_start + command_size
            def alter_record(files, offset, data):
                value = bytearray(first)
                value[offset:offset + len(data)] = data
                value = rehash(value)
                segment = bytearray(files[sealed])
                segment[80:80 + first_size] = value
                files[sealed] = rehash(segment)
                change(files, index, 96, hashlib.sha256(value).digest())

            record_case("record hash missing from index", lambda f: change(f, sealed, 80 + 16, b"\x63"))
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("command length", 48, struct.pack("<I", 512 * 1024 + 1)),
                ("plan length", 52, struct.pack("<I", 4 * 1024 * 1024 + 1)),
                ("result length", 56, struct.pack("<I", 4097)),
                ("rejection retains success plan", 60, b"\x01"),
                ("success failure stage", 72, b"\x01"),
                ("unsupported failure bits", 72, b"\x00\x80"),
                ("command magic", command_start, b"X"),
                ("command schema", command_start + 4, b"\x01"),
                ("command operation", command_start + 8, b"\x02"),
                ("command key reserved", command_start + 53, b"\x01"),
                ("command timestamp binding", command_start + 32, b"\x00" * 8),
                ("intent lineage", intent_start + 32, b"\x63"),
                ("intent unknown epoch", intent_start + 48, b"\x63"),
                ("intent operation", intent_start + 64, b"\x63"),
                ("intent reserved", intent_start + 224, b"\x01"),
                ("intent command binding", intent_start + 160, b"\x63"),
                ("intent domain binding", intent_start + 192, b"\x63"),
                ("plan operation", plan_start + 40, b"\x63"),
                ("plan intent digest", plan_start + 152, b"\x63"),
                ("plan count shape", plan_start + 216, b"\x00" * 4),
                ("plan reserved", plan_start + 240, b"\x01")):
                record_case("record " + label,
                            lambda f, o=offset, d=data: alter_record(f, o, d))
            for label, mutate in record_cases:
                files = dict(records)
                mutate(files)
                restore(files)
                check(label, False)
            files = dict(records)
            alter_record(files, 64, struct.pack("<Q", 2**64 - 1))
            restore(files)
            check("retained receipt unsigned maximum revision", True)
            restore(records)
            target = evidence / sealed
            target.chmod(0o644)
            check("nonprivate sealed operation segment", False)
            restore(records)
            target.unlink()
            target.symlink_to(evidence / active)
            check("symlink sealed operation segment", False)
            restore(records)
            os.link(evidence / active, evidence / "hardlink-active")
            check("hardlinked active operation segment", False)
            restore(records)
            (evidence / active).unlink()
            os.mkfifo(evidence / active, 0o600)
            check("FIFO active operation segment refuses without blocking", False)
            restore(records)
            check("clean native retained records remain qualified", True)
        print(json.dumps({"positive_stores": successes, "refused_corruptions": refusals,
                          "native_invocations_per_case": 2, "economic_bytes_unchanged": True,
                          "qualifier_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                          "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
                          "sanitized_reader_sha256": hashlib.sha256(audit.read_bytes()).hexdigest()}))


if __name__ == "__main__":
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    main()
