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
         "src/flatfile/flatfile_accounting_authority.c",
         "src/flatfile/flatfile_accounting_baseline.c", "src/economy/economic_baseline_adapter.c",
         "src/economy/economic_baseline_codec.c", "src/economy/economic_baseline_command.c", *SOURCES[1:]],
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

            restore({})
            subprocess.run([str(fixture), str(state), "baseline"], env=environment, check=True)
            assert not list(evidence.glob("source-claim-*.bin"))
            check("native baseline own witness book needs no common claim", True)

            restore({})
            subprocess.run([str(fixture), str(state), "baseline-empty"], env=environment, check=True)
            check("native initialized empty inactive baseline book", True)
            empty_book = inventory(evidence)
            for name in sorted(n for n in empty_book if n.startswith("baseline-")):
                files = dict(empty_book)
                files.pop(name)
                restore(files)
                check("empty book partial loss " + name[-8:], False)

            restore({})
            subprocess.run([str(fixture), str(state), "baseline-maximum"], env=environment, check=True)
            check("native maximum baseline witness holdings and item forest", True)

            restore({})
            subprocess.run([str(fixture), str(state), "baseline-full-index"], env=environment, check=True)
            full_index, = evidence.glob("baseline-*-0.ebi")
            assert full_index.stat().st_size == 88 + 65536 * 32
            assert struct.unpack_from("<I", full_index.read_bytes(), 84)[0] == 65536
            assert len(list(evidence.glob("baseline-*.eab"))) == 22
            check("native full 65536-entry reservation shard exceeds generic 2 MiB bound", True)

            restore({})
            subprocess.run([str(fixture), str(state), "baseline-rich"], env=environment, check=True)
            check("native multibatch cross-epoch baseline numeric ordering and unchanged forest", True)
            baselines = inventory(evidence)
            witnesses = sorted(n for n in baselines if n.endswith(".eab"))
            assert len(witnesses) == 4
            witness = next(n for n in witnesses if baselines[n][32:48] == b"2" + b"\x00" * 15
                           and struct.unpack_from("<Q", baselines[n], 72)[0] == 0)
            operation = bytes.fromhex(witness[-36:-4])
            base = witness[:-36]
            head, reservation = base + "head.ebc", base + "f.ebi"
            holding_count = struct.unpack_from("<I", baselines[witness], 184)[0]
            items_start = 192 + holding_count * 112
            baseline_cases = []
            def baseline_case(label, mutate):
                baseline_cases.append((label, mutate))

            def baseline_record(files):
                name = "bucket-" + operation[:1].hex() + ".eai"
                count = struct.unpack_from("<I", files[name], 68)[0]
                row = next(n for n in range(count) if files[name][80 + n * 64:96 + n * 64] == operation)
                segment, offset, size = struct.unpack_from("<III", files[name], 128 + row * 64)
                segment_name = name[:-4] + "-" + str(segment) + ".eas"
                value = bytearray(files[segment_name][80 + offset:80 + offset + size])
                command_size = struct.unpack_from("<I", value, 48)[0]
                return name, row, segment_name, offset, value, 74 + 52 + 16 + 48 + 4, 74 + command_size

            def rewrite_baseline_record(files, mutate):
                name, row, segment_name, offset, value, intent, plan = baseline_record(files)
                mutate(value, intent, plan)
                # Preserve every generic immutable binding so baseline semantics,
                # rather than a broken outer checksum, decide these refusals.
                normalized = bytearray(value[74:intent - 4])
                normalized[4] = 1
                normalized[31] = 0
                normalized[32:40] = struct.pack("<Q", 1)
                value[intent + 160:intent + 192] = hashlib.sha256(
                    b"DURIS-ECONOMIC-COMMAND-V1\x00" + normalized).digest()
                domain = value[74 + 24:74 + 28] + value[74 + 48:74 + 52] + value[74 + 68:intent - 4]
                value[intent + 192:intent + 224] = hashlib.sha256(
                    b"DURIS-ECONOMIC-DOMAIN-V1\x00" + domain).digest()
                value[plan + 8:plan + 72] = value[intent + 32:intent + 96]
                value[plan + 72] = value[intent + 26]
                value[plan + 76:plan + 84] = value[intent + 96:intent + 104]
                value[plan + 84:plan + 98] = value[intent + 12:intent + 26]
                value[plan + 100] = value[intent + 27]
                value[plan + 104:plan + 152] = value[intent + 112:intent + 160]
                value[plan + 152:plan + 184] = hashlib.sha256(
                    b"DURIS-ECONOMIC-INTENT-V1\x00" + value[intent:intent + 256]).digest()
                value[plan + 184:plan + 216] = value[intent + 192:intent + 224]
                value = rehash(value)
                segment = bytearray(files[segment_name])
                segment[80 + offset:80 + offset + len(value)] = value
                files[segment_name] = rehash(segment)
                change(files, name, 96 + row * 64, hashlib.sha256(value).digest())

            def witness_change(files, offset, data):
                encoded = bytearray(files[witness])
                encoded[offset:offset + len(data)] = data
                files[witness] = bytes(encoded)
                def rebind(value, intent, plan):
                    value[74 + 68 + 8:74 + 68 + 12] = struct.pack("<I", len(encoded))
                    value[74 + 68 + 16:74 + 68 + 48] = hashlib.sha256(encoded).digest()
                rewrite_baseline_record(files, rebind)

            def reservation_change(files, offset, data):
                change(files, reservation, offset, data)
                change(files, head, 48 + 96 + 15 * 32, hashlib.sha256(files[reservation]).digest())

            for name in sorted(n for n in baselines if n.startswith("baseline-")):
                baseline_case("missing retained baseline " + name[-36:], lambda f, n=name: f.pop(n))
            baseline_case("complete baseline namespace loss with retained receipts",
                          lambda f: [f.pop(n) for n in list(f) if n.startswith("baseline-")])
            def remove_roots(files):
                for name in list(files):
                    if name.startswith("bucket-"):
                        files.pop(name)
                change(files, "authority.eal", 48 + 16520, b"\x00" * 32)
            baseline_case("book without retained receipts", remove_roots)
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("lineage", 48, b"\x63"), ("epoch", 64, b"\x63"),
                ("opening kind", 48 + 32 + 18, struct.pack("<H", 1)),
                ("opening lifetime", 48 + 32 + 20, struct.pack("<Q", 2)),
                ("opening reserved", 48 + 32 + 36, b"\x01"),
                ("zero revision", 120, b"\x00" * 8),
                ("missing revision", 120, struct.pack("<Q", 2)),
                ("future revision", 120, struct.pack("<Q", 4)),
                ("maximum revision without witnesses", 120, struct.pack("<Q", 2**64 - 1)),
                ("zero terminal", 128, b"\x00" * 16), ("foreign terminal", 128, b"\x63" * 16),
                ("zero index digest", 144, b"\x00" * 32), ("wrong index digest", 144, b"\x63" * 32)):
                baseline_case("baseline head " + label, lambda f, o=offset, d=data: change(f, head, o, d))
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("lineage", 48, b"\x63"), ("epoch", 64, b"\x63"),
                ("slot", 80, struct.pack("<I", 14)), ("count", 84, struct.pack("<I", 1)),
                ("capacity", 84, struct.pack("<I", 65537)),
                ("kind", 88, struct.pack("<Q", 2)), ("zero id", 96, b"\x00" * 8),
                ("wrong id", 96, struct.pack("<Q", 511)),
                ("wrong operation", 104, b"\x63" * 16), ("zero operation", 104, b"\x00" * 16)):
                baseline_case("baseline reservation " + label,
                              lambda f, o=offset, d=data: reservation_change(f, o, d))
            baseline_case("baseline duplicate reservation", lambda f: reservation_change(
                f, 120, f[reservation][88:120]))
            for name in (head, reservation, witness):
                baseline_case("baseline truncated " + name[-8:], lambda f, n=name: f.update({n: f[n][:-1]}))
                baseline_case("baseline trailing bytes " + name[-8:], lambda f, n=name: f.update({n: f[n] + b"\x00"}))
                baseline_case("baseline checksum " + name[-8:], lambda f, n=name: f.update({n: f[n][:-1] + b"\x63"}))
            # The witness digest, command binding, domain binding and plan metadata
            # are rebound for each malformed origin. Raw witness semantics decide.
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 4, struct.pack("<H", 2)),
                ("header size", 6, struct.pack("<H", 191)), ("total size", 8, b"\x00" * 4),
                ("reserved", 12, b"\x01"), ("lineage", 16, b"\x63"),
                ("epoch", 32, b"\x63"), ("preparation id", 48, b"\x63"),
                ("actor", 64, struct.pack("<Q", 8)), ("batch", 72, struct.pack("<Q", 9)),
                ("opening kind", 98, struct.pack("<H", 1)), ("opening lifetime", 100, b"\x00" * 8),
                ("boundary digest", 120, b"\x00" * 32), ("coverage digest", 152, b"\x00" * 32),
                ("holding capacity", 184, struct.pack("<I", 3072)),
                ("item capacity", 188, struct.pack("<I", 6001)),
                ("holding lineage", 192, b"\x63"), ("holding version", 208, b"\x02"),
                ("holding kind", 210, struct.pack("<H", 9)),
                ("negative balance", 232, struct.pack("<Q", 2**64 - 1)),
                ("copper overflow", 344, struct.pack("<Q", 2**63 - 1)),
                ("holding source digest", 272, b"\x00" * 32),
                ("holding order", 192, baselines[witness][304:416]),
                ("duplicate holding lifetime across kinds", 192 + 3 * 112 + 20, struct.pack("<Q", 255)),
                ("item UID", items_start, b"\x00" * 8),
                ("item order", items_start, baselines[witness][items_start + 88:items_start + 176]),
                ("item owner", items_start + 8, b"\x00"),
                ("absent item", items_start + 9, b"\x00"),
                ("item reserved", items_start + 10, b"\x01"),
                ("zero live owner", items_start + 16, b"\x00" * 8),
                ("zero item root", items_start + 32, b"\x00" * 8),
                ("live root mismatch", items_start + 32, struct.pack("<Q", 256)),
                ("self parent", items_start + 40, struct.pack("<Q", 255)),
                ("missing live parent", items_start + 88 + 40, struct.pack("<Q", 999)),
                ("cross owner parent", items_start + 88 + 16, struct.pack("<Q", 8)),
                ("cycle", items_start + 40, struct.pack("<Q", 256)),
                ("destroyed zero revision", items_start + 2 * 88 + 48, b"\x00" * 8),
                ("destroyed active owner", items_start + 2 * 88 + 8, b"\x01"),
                ("pet context too large", items_start + 3 * 88 + 24, struct.pack("<Q", 2**31)),
                ("collector context", items_start + 4 * 88 + 24, b"\x01"),
                ("system owner", items_start + 5 * 88 + 16, b"\x01"),
                ("item source digest", items_start + 56, b"\x00" * 32)):
                baseline_case("baseline witness " + label,
                              lambda f, o=offset, d=data: witness_change(f, o, d))
            for label, offset, data in (
                ("duplicate book revision", 64, struct.pack("<Q", 2)),
                ("zero book revision", 64, b"\x00" * 8),
                ("foreign book revision", 64, struct.pack("<Q", 2**64 - 1)),
                ("source site", 74 + 28, struct.pack("<H", 5)),
                ("deadline", 74 + 30, b"\x01"),
                ("fence type", 74 + 52, b"\x01"),
                ("fence id", 74 + 60, b"\x01"),
                ("payload version", 74 + 26, b"\x02")):
                baseline_case("baseline receipt " + label,
                              lambda f, o=offset, d=data: rewrite_baseline_record(
                                  f, lambda v, i, p: v.__setitem__(slice(o, o + len(d)), d)))
            def record_intent_change(files, offset, data):
                rewrite_baseline_record(files, lambda v, i, p: v.__setitem__(slice(i + offset, i + offset + len(data)), data))
            for label, offset, data in (("policy", 16, struct.pack("<I", 2)),
                                        ("compiler", 20, struct.pack("<I", 2)),
                                        ("source preparation", 116, b"\x63"),
                                        ("source sequence", 148, b"\x63")):
                baseline_case("baseline intent " + label, lambda f, o=offset, d=data: record_intent_change(f, o, d))
            def plan_change(files, offset, data):
                rewrite_baseline_record(files, lambda v, i, p: v.__setitem__(slice(p + offset, p + offset + len(data)), data))
            for label, offset, data in (
                ("holding after", 256 + 72, b"\x63"),
                ("holding before", 256 + 40, b"\x01"),
                ("holding revision", 256 + 112, b"\x02"),
                ("positive posting", 256 + 5 * 120 + 8, b"\x63"),
                ("opening equity", 256 + 5 * 120 + 3 * 48 + 8, b"\x63"),
                ("item snapshot", 256 + 5 * 120 + 6 * 48 + 16, b"\x63")):
                baseline_case("baseline regenerated plan " + label,
                              lambda f, o=offset, d=data: plan_change(f, o, d))
            for name in (base + "0" * 32 + ".eab", base + "63" * 16 + ".eab",
                         base + "f.ebi.tmp", base + "10.ebi", base + "F.ebi",
                         base + operation.hex().upper() + ".eab", "baseline-malformed"):
                baseline_case("untracked baseline filename " + name[-16:], lambda f, n=name: f.update({n: f[witness]}))
            for label, mutate in baseline_cases:
                files = dict(baselines)
                mutate(files)
                restore(files)
                check(label, False)
            for name in (head, reservation, witness):
                restore(baselines)
                (evidence / name).chmod(0o644)
                check("nonprivate baseline " + name[-8:], False)
                restore(baselines)
                (evidence / name).unlink()
                (evidence / name).symlink_to(evidence / witnesses[-1])
                check("symlink baseline " + name[-8:], False)
                restore(baselines)
                os.link(evidence / name, evidence / "baseline-hardlink")
                check("hardlinked baseline " + name[-8:], False)
                restore(baselines)
                (evidence / name).unlink()
                os.mkfifo(evidence / name, 0o600)
                check("FIFO baseline " + name[-8:] + " refuses without blocking", False)
            restore(baselines)
            check("native retained baseline books remain qualified", True)

            restore({})
            subprocess.run([str(fixture), str(state), "source-claims"], env=environment, check=True)
            check("native cross-epoch claims and claimless rejected retry", True)
            sources = inventory(evidence)
            claims = sorted(name for name in sources if name.startswith("source-claim-"))
            assert len(claims) == 2
            claim = next(name for name in claims if sources[name][112:128] == sources[index][80:96])
            claim_cases = []
            def claim_case(label, mutate):
                claim_cases.append((label, mutate))
            for name in claims:
                claim_case("missing native source claim " + name,
                           lambda f, n=name: f.pop(n))
            for label, offset, data in (
                ("magic", 0, b"X"), ("version", 8, struct.pack("<I", 2)),
                ("length", 12, struct.pack("<I", 87)), ("lineage", 48, b"\x63"),
                ("kind", 64, b"\x63"), ("event version", 66, b"\x02"),
                ("source identity", 68, b"\x63"), ("generation", 84, b"\x63"),
                ("sequence", 100, b"\x63"), ("slot", 108, b"\x63"),
                ("operation", 112, b"\x63"), ("outcome", 128, b"\x02"),
                ("reserved", 129, b"\x01")):
                claim_case("source claim " + label,
                           lambda f, o=offset, d=data: change(f, claim, o, d))
            claim_case("source claim checksum", lambda f: f.update({claim: f[claim][:-1] + b"\x01"}))
            claim_case("source claim truncated", lambda f: f.update({claim: f[claim][:-1]}))
            claim_case("source claim trailing bytes", lambda f: f.update({claim: f[claim] + b"\x00"}))
            claim_case("source claim too large", lambda f: f.update({claim: f[claim] + b"\x00" * 128}))
            for name in ("source-claim-" + "0" * 64 + ".bin", claim[:-4] + ".tmp",
                         claim[:13] + claim[13:-4].upper() + ".bin",
                         claim.replace("source-claim-", "source-claim"), claim[:-4] + "0.bin"):
                claim_case("untracked source claim/name " + name,
                           lambda f, n=name: f.update({n: f[claim]}))
            claim_case("swapped source claim bodies", lambda f: f.update({
                claims[0]: sources[claims[1]], claims[1]: sources[claims[0]]}))

            source_segment = "bucket-01-0.eas"
            def source_record(row):
                segment, offset, size = struct.unpack_from("<III", sources[index], 128 + row * 64)
                assert segment == 0
                value = sources[source_segment][80 + offset:80 + offset + size]
                keys, revs, payload = struct.unpack_from("<III", value, 74 + 40)
                intent = 74 + 52 + keys * 16 + revs * 24 + payload + 4
                return value, offset, intent, 74 + struct.unpack_from("<I", value, 48)[0]
            first_source, _, first_intent, _ = source_record(0)
            first_event = first_source[first_intent + 112:first_intent + 160]
            def rewrite_source_record(files, row, event, present=True):
                original, offset, intent, plan = source_record(row)
                value = bytearray(original)
                value[intent + 27] = present
                value[intent + 112:intent + 160] = event
                value[plan + 100] = present
                value[plan + 104:plan + 152] = event
                intent_size = struct.unpack_from("<I", value, intent + 8)[0]
                value[plan + 152:plan + 184] = hashlib.sha256(
                    b"DURIS-ECONOMIC-INTENT-V1\x00" + value[intent:intent + intent_size]).digest()
                value = rehash(value)
                segment = bytearray(files[source_segment])
                segment[80 + offset:80 + offset + len(value)] = value
                files[source_segment] = rehash(segment)
                change(files, index, 96 + row * 64, hashlib.sha256(value).digest())
            claim_case("duplicate source on successful root in another epoch",
                       lambda f: rewrite_source_record(f, 1, first_event))
            claim_case("orphan claim after root drops source metadata",
                       lambda f: rewrite_source_record(f, 0, b"\x00" * 48, False))
            claim_case("baseline source kind cannot bypass a common root claim",
                       lambda f: (f.pop(claim), rewrite_source_record(
                           f, 0, struct.pack("<H", 10) + first_event[2:])))

            rejected, _, rejected_intent, _ = source_record(2)
            rejected_body = (rejected[rejected_intent + 32:rejected_intent + 48] +
                             rejected[rejected_intent + 112:rejected_intent + 160] +
                             rejected[rejected_intent + 64:rejected_intent + 80] +
                             b"\x01" + b"\x00" * 7)
            rejected_name = "source-claim-" + hashlib.sha256(rejected_body[:64]).hexdigest() + ".bin"
            rejected_claim = (b"DURSCL1\x00" + struct.pack("<II", 1, 88) +
                              hashlib.sha256(rejected_body).digest() + rejected_body)
            claim_case("orphan source claim owned by rejected receipt",
                       lambda f: f.update({rejected_name: rejected_claim}))
            claim_case("claim assigned to rejected retry of successful source",
                       lambda f: change(f, claim, 112, source_record(3)[0][82:98]))
            for label, mutate in claim_cases:
                files = dict(sources)
                mutate(files)
                restore(files)
                check(label, False)
            restore(sources)
            target = evidence / claim
            target.chmod(0o644)
            check("nonprivate source claim", False)
            restore(sources)
            target.unlink()
            target.symlink_to(evidence / claims[1])
            check("symlink source claim", False)
            restore(sources)
            os.link(evidence / claim, evidence / "source-claim-hardlink")
            check("hardlinked source claim", False)
            restore(sources)
            (evidence / claim).unlink()
            os.mkfifo(evidence / claim, 0o600)
            check("FIFO source claim refuses without blocking", False)
            restore(sources)
            check("native cross-epoch source claims remain qualified", True)
        print(json.dumps({"positive_stores": successes, "refused_corruptions": refusals,
                          "native_invocations_per_case": 2, "economic_bytes_unchanged": True,
                          "qualifier_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                          "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
                          "sanitized_reader_sha256": hashlib.sha256(audit.read_bytes()).hexdigest()}))


if __name__ == "__main__":
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    main()
