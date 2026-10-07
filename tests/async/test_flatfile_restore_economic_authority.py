#!/usr/bin/env python3
"""Actual native restore refusal of damaged retained accounting authority."""
import hashlib
import fcntl
import json
import os
import select
import stat
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


def build_fixture(destination, native_source=ROOT):
    sources = ["src/flatfile/flatfile_accounting_authority.c",
               "src/flatfile/flatfile_accounting_baseline.c", "src/economy/economic_baseline_adapter.c",
               "src/economy/economic_baseline_codec.c", "src/economy/economic_baseline_command.c",
               *SOURCES[1:]]
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
             "-DDURIS_FLATFILE_ACCOUNTING_TEST", "-I" + str(native_source / "src"), "-pthread"]
    # The native fixture also observes independent operator headers. The shared
    # native cache fingerprints src/ and tests/async/, so declare these owned
    # script inputs in its existing flag key instead of accepting a stale hit.
    headers = hashlib.sha256()
    for header in sorted((ROOT / "scripts").glob("qualify_flatfile_*.h")):
        headers.update(header.name.encode() + b"\0" + header.read_bytes() + b"\0")
    flags.append('-DDURIS_RESTORE_OPERATOR_HEADERS_SHA256="' + headers.hexdigest() + '"')
    fixture = "tests/async/flatfile_restore_authority_fixture.cpp"
    if native_source.resolve() != ROOT.resolve():
        # Isolated native prerequisites are explicit inputs, never mixed with
        # checkout headers or cached objects fingerprinted for the checkout.
        subprocess.run(["g++", *flags, fixture,
                        *(str(native_source / name) for name in sources),
                        "-lcrypto", "-pthread", "-o", str(destination)], cwd=ROOT, check=True)
        return destination
    return build_native(destination, [fixture, *sources], flags, ["-lcrypto", "-pthread"],
                        compiler="g++", name="restore-authority-fixture")


def check_audit_boundary(binary, fixture, environment, build):
    """The operator itself must exclude native writers without changing files."""
    with tempfile.TemporaryDirectory(prefix="duris-audit-boundary-", dir=build) as root:
        root = Path(root)
        produced = subprocess.run([str(fixture), str(root), "envelope-records"],
            env=environment, capture_output=True, text=True, timeout=60)
        assert produced.returncode == 0 and not produced.stderr, produced

        def whole_inventory():
            result = {}
            for path in sorted(root.rglob("*")):
                info = path.lstat()
                result[str(path.relative_to(root))] = (
                    stat.S_IFMT(info.st_mode), stat.S_IMODE(info.st_mode), info.st_nlink,
                    os.readlink(path) if path.is_symlink() else
                    path.read_bytes() if path.is_file() else None)
            return result

        observations = []

        def check(label, valid):
            before = whole_inventory()
            observed = subprocess.run([str(binary), "--economic-evidence-audit", str(root)],
                env=environment, capture_output=True, text=True, timeout=30)
            assert whole_inventory() == before, label
            row = {"case": label, "exit": observed.returncode, "expected": 0 if valid else 1,
                   "stdout": observed.stdout, "stderr": observed.stderr,
                   "inventory_unchanged": True}
            observations.append(row)
            print("AUDIT_BOUNDARY_OBSERVATION " + json.dumps(row), flush=True)

        check("native inactive store", True)
        holder = subprocess.Popen([str(fixture), str(root), "hold-authority-lock"],
            env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True)
        try:
            assert select.select([holder.stdout], [], [], 10)[0], "native lock readiness timeout"
            assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
            check("native exclusive authority lock", False)
        finally:
            stdout, stderr = holder.communicate("release\n", timeout=10)
            assert holder.returncode == 0 and not stdout and not stderr, (stdout, stderr)
        check("native writer released", True)

        native = subprocess.run([str(fixture), str(root), "pending-authority-journal"],
            env=environment, capture_output=True, text=True, timeout=30)
        assert native.returncode == 0 and not native.stderr, native
        assert native.stdout == "NATIVE_PENDING_AUTHORITY_JOURNAL\n"
        journal = root / "domains/.critical-authority-transaction"
        print("NATIVE_PENDING_JOURNAL_SHA256 " + hashlib.sha256(journal.read_bytes()).hexdigest(),
              flush=True)
        check("native unresolved authority journal", False)
        # Explicit fixture cleanup, never operator recovery or correction.
        journal.unlink()
        (root / "domains/audit-boundary-target").rmdir()
        check("private journal fixture removed", True)

        lock = root / "domains/.critical-authority.lock"
        original = lock.read_bytes()
        original_mode = stat.S_IMODE(lock.stat().st_mode)
        for mode in ("missing", "symlink", "hardlink", "FIFO", "public", "directory"):
            lock.unlink()
            alternate = root / "domains/audit-boundary-alternate"
            if mode in ("symlink", "hardlink"):
                alternate.write_bytes(original)
                if mode == "symlink":
                    lock.symlink_to(alternate)
                else:
                    os.link(alternate, lock)
            elif mode == "FIFO":
                os.mkfifo(lock, 0o600)
            elif mode == "directory":
                lock.mkdir()
            elif mode == "public":
                lock.write_bytes(original)
                lock.chmod(0o644)
            check("authority lock " + mode, False)
            if lock.is_symlink() or (lock.exists() and not lock.is_dir()):
                lock.unlink()
            elif lock.is_dir():
                lock.rmdir()
            if alternate.exists():
                alternate.unlink()
            lock.write_bytes(original)
            lock.chmod(original_mode)
        check("native lock restored", True)
        assert all(row["exit"] == row["expected"] and
                   (not row["stdout"] and row["stderr"] == "native_restore_qualification_failed\n"
                    if row["expected"] else not row["stderr"])
                   for row in observations), observations
        return len(observations)


def check_audit_limits(audit, binary, fixture, environment, build):
    with tempfile.TemporaryDirectory(prefix="duris-audit-limits-", dir=build) as root:
        root = Path(root)
        produced = subprocess.run([str(fixture), str(root), "envelope-records"],
            env=environment, capture_output=True, text=True, timeout=60)
        assert produced.returncode == 0 and not produced.stderr, produced
        evidence = root / "economic-evidence"
        before, domains_before = inventory(evidence), inventory(root / "domains")
        cases = 0
        for mode in ("bounded", "bytes", "files", "entries", "deadline"):
            observed = subprocess.run([str(audit), mode, str(root)], env=environment,
                                      capture_output=True, text=True, timeout=30)
            assert observed.returncode == (0 if mode == "bounded" else 1), (mode, observed)
            assert not observed.stdout and observed.stderr == (
                "" if mode == "bounded" else "native_restore_qualification_failed\n"), observed
            assert inventory(evidence) == before
            assert inventory(root / "domains") == domains_before
            with (root / "domains/.critical-authority.lock").open("rb") as lock:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            cases += 1
        # The default directory budget counts unknown entries even though the
        # offline format reader deliberately ignores this unrelated namespace.
        for index in range(1700):
            (evidence / ("unrelated-" + str(index))).write_bytes(b"")
        oversized = inventory(evidence)
        for command, valid in (([str(audit), str(root)], True),
                               ([str(binary), "--economic-evidence-audit", str(root)], False)):
            observed = subprocess.run(command, env=environment, capture_output=True,
                                      text=True, timeout=30)
            assert observed.returncode == (0 if valid else 1), observed
            assert not observed.stderr if valid else (
                not observed.stdout and observed.stderr == "native_restore_qualification_failed\n")
            assert inventory(evidence) == oversized
            assert inventory(root / "domains") == domains_before
            cases += 1
        for index in range(1700):
            (evidence / ("unrelated-" + str(index))).unlink()
        # Shared readers coexist. A native writer cannot enter until every
        # reader releases; lock-name replacement invalidates a held audit cut.
        lock_path = root / "domains/.critical-authority.lock"
        for replace in (False, True):
            holder = subprocess.Popen([str(audit), "hold-read-lock", str(root)],
                env=environment, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.PIPE, text=True)
            try:
                assert select.select([holder.stdout], [], [], 10)[0]
                assert holder.stdout.readline() == "INDEPENDENT_AUTHORITY_READ_LOCK_HELD\n"
                with lock_path.open("rb") as lock:
                    try:
                        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                    except BlockingIOError:
                        pass
                    else:
                        raise AssertionError("writer entered during independent audit")
                shared = subprocess.run([str(binary), "--economic-evidence-audit", str(root)],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert shared.returncode == 0 and not shared.stderr, shared
                if replace:
                    lock_path.rename(root / "domains/held-authority-lock")
                    lock_path.write_bytes(b"")
            finally:
                stdout, stderr = holder.communicate("release\n", timeout=10)
            assert holder.returncode == (1 if replace else 0), (stdout, stderr)
            assert not stdout and stderr == (
                "native_restore_qualification_failed\n" if replace else ""), (stdout, stderr)
            if replace:
                lock_path.unlink()
                (root / "domains/held-authority-lock").rename(lock_path)
            assert inventory(evidence) == before
            assert inventory(root / "domains") == domains_before
            cases += 1
        return cases


def check_root_pages(binary, fixture, environment, build):
    with tempfile.TemporaryDirectory(prefix="duris-root-pages-", dir=build) as root:
        root = Path(root)
        produced = subprocess.run([str(fixture), str(root), "paged-records"],
            env=environment, capture_output=True, text=True, timeout=60)
        assert produced.returncode == 0 and not produced.stderr, produced
        assert produced.stdout == "NATIVE_PAGED_RECORDS 4\n", produced.stdout
        print(produced.stdout, end="", flush=True)
        evidence = root / "economic-evidence"
        before, domains = inventory(evidence), inventory(root / "domains")
        assert before["authority.eal"][112:128] == bytes(16)
        whole = subprocess.run([str(binary), "--economic-evidence-audit", str(root)],
            env=environment, capture_output=True, text=True, timeout=30)
        assert whole.returncode == 0 and not whole.stderr, whole
        observations = []
        for bucket, rows in ((0, 0), (1, 2), (2, 1)):
            observed = subprocess.run([str(binary), "--economic-evidence-page", str(root),
                str(bucket), "-", "-"], env=environment, capture_output=True, text=True, timeout=45)
            print("ROOT_PAGE_OBSERVATION " + json.dumps(dict(bucket=bucket, exit=observed.returncode,
                stdout=observed.stdout, stderr=observed.stderr)), flush=True)
            observations.append((observed, rows))
            assert inventory(evidence) == before and inventory(root / "domains") == domains
        assert all(observed.returncode == 0 and not observed.stderr and
                   json.loads(observed.stdout)["rows"] == rows for observed, rows in observations), observations
        import flatfile_economic_audit as pages
        from unittest import mock
        import time
        progress = Path(build) / "root-page-progress.json"
        source = pages.source_digest(root, binary)
        cases = 3

        def cli(valid=True, path=progress):
            snapshot, native = inventory(evidence), inventory(root / "domains")
            observed = subprocess.run([sys.executable, "-B", str(ROOT / "scripts/flatfile_economic_audit.py"),
                "--state-root", str(root), "--qualifier", str(binary), "--progress", str(path)],
                env=environment, capture_output=True, text=True, timeout=60)
            assert observed.returncode == (0 if valid else 1), observed
            assert inventory(evidence) == snapshot and inventory(root / "domains") == native
            return observed

        first = json.loads(cli().stdout)
        assert first["bucket"] == 0 and first["next_bucket"] == 1
        second = json.loads(cli().stdout)
        state = pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        assert second["bucket"] == 1 and second["examined_roots"] == 2
        assert state["buckets"][1]["cursor"].endswith("00000004")
        assert state["buckets"][1]["ceiling"].endswith("00000006")
        assert json.loads(cli().stdout)["bucket"] == 2
        cases += 3
        appended = subprocess.run([str(fixture), str(root), "paged-append"],
            env=environment, capture_output=True, text=True, timeout=30)
        assert appended.returncode == 0 and appended.stdout == "NATIVE_DELAYED_LOWER_RECORD\n" and not appended.stderr
        appended_before = inventory(evidence)

        def step():
            state = pages.validate(pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES,
                pages.AuditError, "flatfile audit progress"), source, time.time())
            report, updated = pages.scan(root, binary, state)
            pages.progress_io.save(progress, updated, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
            assert not report["complete"] and not report["consistent_entire_sweep"] and not report["release_qualified"]
            assert inventory(evidence) == appended_before and inventory(root / "domains") == domains
            return report, updated

        for _ in range(255):
            report, state = step()
        assert report["bucket"] == 1 and report["examined_roots"] == 1 and report["range_exhausted"]
        assert state["buckets"][1]["completed_ranges"] == 1
        for _ in range(256):
            report, state = step()
        assert report["bucket"] == 1 and report["examined_roots"] == 2
        assert state["buckets"][1]["cursor"].endswith("00000002")
        assert state["buckets"][1]["ceiling"].endswith("00000006")
        assert report["completed_historical_ranges"] == 1
        cases += 2

        # A damaged bucket cannot starve the next one. Its cursor remains exact
        # and its refusal survives later clean pages and actual CLI restarts.
        damaged = bytearray(appended_before["bucket-02.eai"])
        damaged[-1] ^= 1
        (evidence / "bucket-02.eai").write_bytes(damaged)
        refused = json.loads(cli(False).stdout)
        stored = pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        assert refused["bucket"] == 2 and refused["page_refused"] and refused["next_bucket"] == 3
        assert stored["buckets"][2] == state["buckets"][2]
        (evidence / "bucket-02.eai").write_bytes(appended_before["bucket-02.eai"])
        subsequent = json.loads(cli(False).stdout)
        assert subsequent["bucket"] == 3 and subsequent["examined_roots"] == 0
        assert subsequent["total_finding_count"] == 1 and not subsequent["page_refused"]
        assert "operation_id" not in subsequent
        cases += 2
        checkpoint = progress.read_bytes()
        with mock.patch.object(pages.subprocess, "run", side_effect=subprocess.TimeoutExpired("private-page", 45)):
            state = pages.validate(pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES,
                pages.AuditError, "flatfile audit progress"), source, time.time())
            report, refused_state = pages.scan(root, binary, state)
        assert report["page_refused"] and refused_state["buckets"] == state["buckets"]
        assert progress.read_bytes() == checkpoint
        with mock.patch.object(pages.progress_io.os, "replace", side_effect=OSError("private interrupted publication")):
            try:
                pages.progress_io.save(progress, refused_state, pages.MAX_PROGRESS_BYTES,
                    pages.AuditError, "flatfile audit progress")
            except OSError:
                pass
            else:
                raise AssertionError("checkpoint failure was hidden")
        assert progress.read_bytes() == checkpoint
        assert not list(progress.parent.glob("." + progress.name + "-*"))
        cases += 2

        for label, data in (("duplicate fields", b'{"format":1,"format":2}'),
                            ("wrong source", checkpoint.replace(source.encode(), b"a" * 64)),
                            ("oversized", b" " * (pages.MAX_PROGRESS_BYTES + 1))):
            progress.write_bytes(data)
            observed = cli(False)
            assert not observed.stdout and observed.stderr == "flatfile_economic_audit_refused\n", label
            assert progress.read_bytes() == data
            cases += 1
        progress.write_bytes(checkpoint)
        for label in ("symlink", "hardlink", "public", "FIFO"):
            progress.unlink()
            alternate = progress.with_name("alternate-progress")
            alternate.write_bytes(checkpoint)
            if label == "symlink":
                progress.symlink_to(alternate)
            elif label == "hardlink":
                os.link(alternate, progress)
            elif label == "public":
                progress.write_bytes(checkpoint)
                progress.chmod(0o644)
            else:
                os.mkfifo(progress, 0o600)
            observed = cli(False)
            assert not observed.stdout and observed.stderr == "flatfile_economic_audit_refused\n", label
            assert alternate.read_bytes() == checkpoint
            progress.unlink()
            alternate.unlink()
            progress.write_bytes(checkpoint)
            progress.chmod(0o600)
            cases += 1
        inside = root / "operator-progress.json"
        observed = cli(False, inside)
        assert not observed.stdout and not inside.exists() and not Path(str(inside) + ".lock").exists()
        cases += 1

        # A semantic defect remains visible after rebinding every physical
        # checksum. The native record decoder is an independent refusal oracle.
        index = bytearray(appended_before["bucket-01.eai"])
        segment_name = "bucket-01-" + str(struct.unpack_from("<I", index, 128)[0]) + ".eas"
        segment = bytearray(appended_before[segment_name])
        offset, size = struct.unpack_from("<II", index, 132)
        value = bytearray(segment[80 + offset:80 + offset + size])
        value[74 + 24:74 + 26] = struct.pack("<H", 22)
        value = rehash(value)
        raw = Path(build) / "page-invalid-record.bin"
        raw.write_bytes(value)
        native = subprocess.run([str(fixture), str(raw), "decode-record"],
            env=environment, capture_output=True, text=True, timeout=30)
        assert native.returncode == 1 and not native.stderr, native
        index[96:128] = hashlib.sha256(value).digest()
        segment[80 + offset:80 + offset + size] = value
        (evidence / "bucket-01.eai").write_bytes(rehash(index))
        (evidence / segment_name).write_bytes(rehash(segment))
        observed = subprocess.run([str(binary), "--economic-evidence-page", str(root), "1", "-", "-"],
            env=environment, capture_output=True, text=True, timeout=45)
        decoded = json.loads(observed.stdout)
        assert observed.returncode == 0 and not observed.stderr and decoded["rows"] == 2
        assert decoded["verified"] == 1 and len(decoded["invalid_records"]) == 1
        semantic_progress = Path(build) / "semantic-page-progress.json"
        assert json.loads(cli(path=semantic_progress).stdout)["bucket"] == 0
        semantic = json.loads(cli(False, semantic_progress).stdout)
        assert semantic["scope"] == "retained_record_page" and semantic["semantically_checked_records"] == 1
        assert not any(semantic[key] for key in ("complete", "release_qualified", "baseline_books_closed",
            "lifecycle_receipts_closed", "orphan_namespace_closed", "native_holdings_compared"))
        semantic_state = pages.progress_io.load(semantic_progress, pages.MAX_PROGRESS_BYTES,
            pages.AuditError, "flatfile audit progress")
        assert semantic_state["findings"][0]["code"] == "flatfile_retained_record_invalid"
        (evidence / "bucket-01.eai").write_bytes(appended_before["bucket-01.eai"])
        (evidence / segment_name).write_bytes(appended_before[segment_name])
        cases += 3
        stored = pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        stored["lineage"] = "02" + "00" * 15
        pages.progress_io.save(progress, stored, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        foreign = progress.read_bytes()
        observed = cli(False)
        assert not observed.stdout and observed.stderr == "flatfile_economic_audit_refused\n"
        assert progress.read_bytes() == foreign
        progress.write_bytes(checkpoint)
        with pages.progress_io.lock(progress, pages.AuditError, "flatfile audit progress"):
            observed = cli(False)
            assert not observed.stdout and observed.stderr == "flatfile_economic_audit_refused\n"
        assert progress.read_bytes() == checkpoint
        cases += 2
        # A durable range cannot claim exhaustion when a persisted anchor was
        # never indexed or disappeared, even if the index checksum is valid.
        missing_cursor = subprocess.run([str(binary), "--economic-evidence-page", str(root), "1",
            "01" + "00" * 14 + "05", "01" + "00" * 14 + "06"],
            env=environment, capture_output=True, text=True, timeout=45)
        assert missing_cursor.returncode == 1 and not missing_cursor.stdout
        assert missing_cursor.stderr == "native_restore_qualification_failed\n", missing_cursor
        anchor_progress = Path(build) / "anchor-page-progress.json"
        anchored = pages.new_progress(source, time.time())
        anchored["lineage"] = state["lineage"]
        anchored["rotation"] = 1
        anchored["buckets"][1].update(cursor="01" + "00" * 14 + "04",
            ceiling="01" + "00" * 14 + "06")
        pages.progress_io.save(anchor_progress, anchored, pages.MAX_PROGRESS_BYTES,
            pages.AuditError, "flatfile audit progress")
        shortened = bytearray(before["bucket-01.eai"])
        count = struct.unpack_from("<I", shortened, 68)[0]
        total = struct.unpack_from("<Q", shortened, 72)[0]
        removed_size = struct.unpack_from("<I", shortened, len(shortened) - 8)[0]
        assert count == 3 and shortened[-64:-48] == bytes.fromhex(anchored["buckets"][1]["ceiling"])
        struct.pack_into("<I", shortened, 68, count - 1)
        struct.pack_into("<Q", shortened, 72, total - removed_size)
        shortened = shortened[:-64]
        struct.pack_into("<I", shortened, 12, len(shortened) - 48)
        (evidence / "bucket-01.eai").write_bytes(rehash(shortened))
        refused = json.loads(cli(False, anchor_progress).stdout)
        saved = pages.progress_io.load(anchor_progress, pages.MAX_PROGRESS_BYTES,
            pages.AuditError, "flatfile audit progress")
        assert refused["page_refused"] and refused["examined_roots"] == 0 and not refused["range_exhausted"]
        assert saved["buckets"][1] == anchored["buckets"][1] and saved["rotation"] == 2
        assert saved["buckets"][1]["completed_ranges"] == 0
        (evidence / "bucket-01.eai").write_bytes(appended_before["bucket-01.eai"])
        cases += 2
        assert inventory(evidence) == appended_before and inventory(root / "domains") == domains
        return cases


def check_authority_pages(binary, fixture, environment, build):
    """Durable metadata cross-links use independent frames and never mutate authority."""
    import flatfile_economic_audit as pages
    import time
    from unittest import mock
    cases = 0
    with tempfile.TemporaryDirectory(prefix="duris-authority-pages-", dir=build) as root:
        root = Path(root)
        produced = subprocess.run([str(fixture), str(root), "paged-authority"],
            env=environment, capture_output=True, text=True, timeout=120)
        assert produced.returncode == 0 and not produced.stderr, produced
        assert produced.stdout == "NATIVE_PAGED_AUTHORITY 608\n", produced
        evidence, domains = root / "economic-evidence", root / "domains"
        def domain_inventory():
            result = {}
            for path in domains.rglob("*"):
                info = path.lstat()
                result[str(path.relative_to(domains))] = (info.st_mode, info.st_nlink,
                    path.read_bytes() if path.is_file() else None)
            return result

        before, native_before = inventory(evidence), domain_inventory()
        assert before["authority.eal"][112:128] == bytes(16)

        def raw(direction, bucket, after="-", ceiling="-", valid=True):
            snapshot = inventory(evidence)
            observed = subprocess.run([str(binary), "--economic-authority-page", str(root),
                direction, str(bucket), after, ceiling], env=environment, capture_output=True,
                text=True, timeout=45)
            assert inventory(evidence) == snapshot and domain_inventory() == native_before
            assert observed.returncode == (0 if valid else 1), observed
            assert observed.stderr == ("" if valid else "native_restore_qualification_failed\n"), observed
            if valid:
                report = json.loads(observed.stdout)
                assert report["direction"] == direction and report["bucket"] == bucket
                assert report["rows"] <= 2 and report["bucket_rows"] <= 4096
                return report
            assert not observed.stdout, observed

        first = raw("mapping", 1)
        assert first["rows"] == first["verified"] == 2 and first["bucket_rows"] == 3
        assert first["cursor"] == format(257, "016x") and first["ceiling"] == format(513, "016x")
        assert not first["range_exhausted"]
        second = raw("mapping", 1, first["cursor"], first["ceiling"])
        assert second["rows"] == second["verified"] == 1 and second["range_exhausted"]
        cases += 2

        def locator_rows(name):
            data = before[name]
            count = struct.unpack_from("<I", data, 68)[0]
            offset, result = 72, []
            for _ in range(count):
                length = struct.unpack_from("<H", data, offset)[0]
                active, last = struct.unpack_from("<QQ", data, offset+4)
                key = data[offset+20:offset+20+length]
                result.append((key, active, last, offset))
                offset += 20+length
            assert offset == len(data)
            return result

        indexes = {name: locator_rows(name) for name in before if name.startswith("native-")}
        dense = next(name for name, values in indexes.items() if len(values) > 2)
        native_bucket = int(dense[7:9], 16)
        first_native = raw("native", native_bucket)
        assert first_native["rows"] == first_native["verified"] == 2 and not first_native["range_exhausted"]
        continued = raw("native", native_bucket, first_native["cursor"], first_native["ceiling"])
        assert continued["rows"] > 0 and not continued["invalid_links"]
        cases += 2
        # Native bank keys permit a one-byte alias and the maximum 50-byte alias.
        for name, values in indexes.items():
            for position, (key, _, last, _) in enumerate(values):
                if last not in (607, 608):
                    continue
                result = raw("native", int(name[7:9], 16),
                    values[position-1][0].hex() if position else "-", values[-1][0].hex())
                assert not result["invalid_links"] and result["rows"] > 0
                cases += 1

        progress = Path(build) / "authority-page-progress.json"
        source = pages.source_digest(root, binary)
        command = [sys.executable, "-B", str(ROOT / "scripts/flatfile_economic_audit.py"),
            "--state-root", str(root), "--qualifier", str(binary), "--progress", str(progress),
            "--scope", "authority-links"]

        def cli(valid=True):
            snapshot = inventory(evidence)
            observed = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=60)
            assert observed.returncode == (0 if valid else 1), observed
            assert inventory(evidence) == snapshot and domain_inventory() == native_before
            assert not observed.stderr, observed
            return json.loads(observed.stdout)

        assert cli()["bucket"] == 0
        persisted = cli()
        assert persisted["direction"] == "mapping" and persisted["examined_links"] == 2
        assert persisted["bucket"] == 1 and not persisted["range_exhausted"]
        cases += 2

        def load():
            return pages.validate(pages.progress_io.load(progress, pages.MAX_PROGRESS_BYTES,
                pages.AuditError, "flatfile audit progress"), source, time.time(), True)

        def step():
            report, updated = pages.scan(root, binary, load(), authority_links=True)
            pages.progress_io.save(progress, updated, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
            assert report["examined_links"] <= 2
            for flag in ("complete", "consistent_entire_sweep", "release_qualified", "authority_crosslinks_closed",
                         "native_holdings_compared", "baseline_books_closed", "lifecycle_receipts_closed", "orphan_namespace_closed"):
                assert report[flag] is False
            assert inventory(evidence) == before and domain_inventory() == native_before
            return report, updated

        for _ in range(510):
            report, saved = step()
        assert report["direction"] == "native" and report["bucket"] == 255 and saved["rotation"] == 0
        assert saved["total_findings"] == 0 and len(saved["buckets"]) == 512
        assert saved["buckets"][1]["cursor"] == format(257, "016x")
        step()
        report, saved = step()
        assert report["bucket"] == 1 and report["range_exhausted"] and report["examined_links"] == 1
        assert saved["buckets"][1] == dict(cursor="", ceiling=None, completed_ranges=1)
        cases += 2

        # Both saved anchors must still be members; arbitrary fences cannot earn completion.
        raw("mapping", 1, format(258, "016x"), format(513, "016x"), False)
        raw("mapping", 1, format(257, "016x"), format(769, "016x"), False)
        for label, missing in (("cursor", first_native["cursor"]), ("ceiling", first_native["ceiling"])):
            values = indexes[dense]
            key, _, _, offset = next(row for row in values if row[0].hex() == missing)
            content = bytearray(before[dense])
            del content[offset:offset+20+len(key)]
            struct.pack_into("<I",content,68,len(values)-1)
            struct.pack_into("<I",content,12,len(content)-48)
            files = dict(before);files[dense] = rehash(content)
            change(files,"authority.eal",184+native_bucket*32,hashlib.sha256(files[dense]).digest())
            for name,data in files.items():
                (evidence / name).write_bytes(data)
            raw("native",native_bucket,first_native["cursor"],first_native["ceiling"],False)
            for name,data in before.items():
                (evidence / name).write_bytes(data)
        cases += 4

        # A well-framed native index can still disagree with its live mapping.
        target_name, values, position = next((name, values, i) for name, values in indexes.items()
            for i, (_, _, last, _) in enumerate(values) if last == 3)
        target_bucket = int(target_name[7:9], 16)
        target_key, _, _, offset = values[position]
        files = dict(before)
        change(files, target_name, offset+4, struct.pack("<Q", 0), 184+target_bucket*32)
        for name, data in files.items():
            (evidence / name).write_bytes(data)
        mapping = raw("mapping", 3)
        assert mapping["invalid_links"] == [format(3, "016x")] and mapping["verified"] == 1
        native = raw("native", target_bucket, values[position-1][0].hex() if position else "-", values[-1][0].hex())
        assert native["invalid_links"] == [target_key.hex()] and native["verified"] == native["rows"]-1
        saved = load();saved["rotation"] = 3
        saved["buckets"][3].update(cursor="",ceiling=None)
        pages.progress_io.save(progress, saved, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        retained = cli(False)
        assert retained["total_finding_count"] == 1 and retained["retained_finding_count"] == 1
        assert load()["findings"][0] == dict(bucket=3, direction="mapping", key=format(3,"016x"), code="flatfile_authority_link_invalid")
        for name, data in before.items():
            (evidence / name).write_bytes(data)
        assert cli(False)["bucket"] == 4
        cases += 4

        saved = load();saved["rotation"] = 1
        pages.progress_io.save(progress, saved, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
        damaged = bytearray(before["mapping-01.eam"]);damaged[-1] ^= 1
        (evidence / "mapping-01.eam").write_bytes(damaged)
        checkpoint = load()["buckets"][1].copy()
        refused = cli(False)
        assert refused["page_refused"] and refused["next_bucket"] == 2
        assert load()["buckets"][1] == checkpoint
        (evidence / "mapping-01.eam").write_bytes(before["mapping-01.eam"])
        assert cli(False)["bucket"] == 2
        cases += 2

        saved = load();saved["rotation"] = 1
        with mock.patch.object(pages.subprocess, "run", side_effect=subprocess.TimeoutExpired("qualifier",45)):
            report, timed_out = pages.scan(root, binary, saved, authority_links=True)
        assert report["page_refused"] and timed_out["rotation"] == 2 and timed_out["buckets"][1] == saved["buckets"][1]
        original = progress.read_bytes()
        with mock.patch.object(pages.progress_io.os, "replace", side_effect=OSError("interrupted")):
            try:
                pages.progress_io.save(progress, timed_out, pages.MAX_PROGRESS_BYTES, pages.AuditError, "flatfile audit progress")
            except OSError:
                pass
            else:
                raise AssertionError("interrupted authority progress replaced checkpoint")
        assert progress.read_bytes() == original
        cases += 2
        wrong_scope = subprocess.run(command[:-2], env=environment, capture_output=True, text=True, timeout=60)
        assert wrong_scope.returncode == 1 and not wrong_scope.stdout and wrong_scope.stderr == "flatfile_economic_audit_refused\n"
        assert progress.read_bytes() == original and inventory(evidence) == before
        cases += 1

        for mode in ("hold-authority-lock", "pending-authority-journal"):
            if mode == "hold-authority-lock":
                holder = subprocess.Popen([str(fixture), str(root), mode], env=environment,
                    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                try:
                    assert select.select([holder.stdout],[],[],10)[0]
                    assert holder.stdout.readline() == "NATIVE_AUTHORITY_LOCK_HELD\n"
                    for direction in ("mapping", "native"):
                        raw(direction, 1, valid=False);cases += 1
                finally:
                    stdout, stderr = holder.communicate("release\n",timeout=10)
                    assert holder.returncode == 0 and not stdout and not stderr
            else:
                produced = subprocess.run([str(fixture),str(root),mode],env=environment,capture_output=True,text=True,timeout=30)
                assert produced.returncode == 0 and not produced.stderr
                native_before = domain_inventory()
                for direction in ("mapping", "native"):
                    raw(direction,1,valid=False);cases += 1
    print("AUTHORITY_PAGE_CONTROLS " + str(cases),flush=True)
    return cases


def main():
    os.umask(0o077)
    with tempfile.TemporaryDirectory(prefix="duris-restore-authority-build-",
                                     dir=ROOT / "bin/tests") as build:
        binary = qualifier.build(Path(build) / "qualify")
        fixture = build_fixture(Path(build) / "fixture")
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        root_page_cases = check_root_pages(binary, fixture, environment, build)
        authority_page_cases = check_authority_pages(binary, fixture, environment, build)
        metadata = subprocess.run([str(fixture), str(build), "compare-metadata"],
            env=environment, capture_output=True, text=True, timeout=30)
        assert metadata.returncode == 0 and not metadata.stderr, metadata
        assert metadata.stdout == "NATIVE_INDEPENDENT_METADATA_COMPARISONS 1058\n", metadata.stdout
        print(metadata.stdout, end="", flush=True)
        envelopes = subprocess.run([str(fixture), str(build), "compare-command-envelope"],
            env=environment, capture_output=True, text=True, timeout=30)
        assert envelopes.returncode == 0 and not envelopes.stderr, envelopes
        assert envelopes.stdout == "NATIVE_INDEPENDENT_COMMAND_ENVELOPE_COMPARISONS 574\n"
        print(envelopes.stdout, end="", flush=True)
        boundary_cases = check_audit_boundary(binary, fixture, environment, build)
        # Exercise the independent reader under sanitizers without invoking
        # candidate recovery or any native mutation/storage interface.
        audit_source = Path(build) / "audit.cpp"
        audit_source.write_text('''#include "qualify_flatfile_economic_records.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) return 2;
    try {
        if (argc == 2) restore_economic_records::checker(argv[1]).run();
        else if (std::string(argv[1]) == "root-page-budget") {
            restore_economic_authority::audit_budget budget;
            budget.remaining_files = 6; // Context/index/segment, then source claim refusal.
            restore_economic_authority::scoped_audit_budget scope(budget);
            restore_economic_authority::authority_read_lock lock(argv[2]);
            restore_economic_records::checker(argv[2]).page(1, {}, {}, false);
        } else if (std::string(argv[1]) == "authority-mapping-budget" ||
                   std::string(argv[1]) == "authority-native-budget") {
            using namespace restore_economic_authority;
            audit_budget budget;
            budget.remaining_files = 3; // Control/catalog/local index, then cross-link refusal.
            scoped_audit_budget scope(budget);
            authority_read_lock lock(argv[2]);
            bytes key;
            put(key, 1, 2); put(key, 0, 8); put(key, 1, 2); put(key, 11, 8);
            const bool mapping = std::string(argv[1]) == "authority-mapping-budget";
            checker(argv[2]).page(mapping, mapping ? 3 : hash(key)[0], {}, {}, false);
        } else if (std::string(argv[1]) == "hold-read-lock") {
            restore_economic_authority::authority_read_lock lock(argv[2]);
            std::cout << "INDEPENDENT_AUTHORITY_READ_LOCK_HELD\\n" << std::flush;
            std::string release;
            restore_economic_authority::need(std::getline(std::cin, release) && release == "release");
            lock.finish();
        } else {
            restore_economic_authority::audit_budget budget;
            const std::string mode = argv[1];
            if (mode == "bytes") budget.remaining_bytes = 48;
            else if (mode == "files") budget.remaining_files = 1;
            else if (mode == "entries") budget.remaining_entries = 1;
            else if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
            else restore_economic_authority::need(mode == "bounded");
            restore_economic_records::audit(argv[2], budget);
        }
        return 0;
    }
    catch (...) { std::cerr << "native_restore_qualification_failed\\n"; return 1; }
}
''')
        audit = Path(build) / "audit"
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-fno-pie", "-no-pie", "-I" + str(ROOT / "scripts"), str(audit_source),
                        "-lcrypto", "-o", str(audit)], check=True)
        limit_cases = check_audit_limits(audit, binary, fixture, environment, build)
        with tempfile.TemporaryDirectory(prefix="duris-root-page-budget-", dir=build) as budget_root:
            budget_root = Path(budget_root)
            produced = subprocess.run([str(fixture), str(budget_root), "source-claims"],
                env=environment, capture_output=True, text=True, timeout=60)
            assert produced.returncode == 0 and not produced.stderr, produced
            before = inventory(budget_root / "economic-evidence")
            native_before = inventory(budget_root / "domains")
            refused = subprocess.run([str(audit), "root-page-budget", str(budget_root)],
                env=environment, capture_output=True, text=True, timeout=30)
            assert refused.returncode == 1 and not refused.stdout
            assert refused.stderr == "native_restore_qualification_failed\n", refused
            assert inventory(budget_root / "economic-evidence") == before
            assert inventory(budget_root / "domains") == native_before
            root_page_cases += 1
            for mode in ("authority-mapping-budget", "authority-native-budget"):
                refused = subprocess.run([str(audit), mode, str(budget_root)],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert refused.returncode == 1 and not refused.stdout
                assert refused.stderr == "native_restore_qualification_failed\n", refused
                assert inventory(budget_root / "economic-evidence") == before
                assert inventory(budget_root / "domains") == native_before
                authority_page_cases += 1
        with tempfile.TemporaryDirectory(prefix="duris-envelope-records-",
                                         dir=build) as envelope_root:
            envelope_root = Path(envelope_root)
            produced = subprocess.run([str(fixture), str(envelope_root), "envelope-records"],
                env=environment, capture_output=True, text=True, timeout=60)
            assert produced.returncode == 0 and not produced.stderr, produced
            assert produced.stdout == "NATIVE_ENVELOPE_RECORDS 11\n", produced.stdout
            print(produced.stdout, end="", flush=True)
            before = inventory(envelope_root / "economic-evidence")
            native_before = inventory(envelope_root / "domains")
            for command in ([str(audit), str(envelope_root)],
                            [str(binary), "--economic-evidence-audit", str(envelope_root)]):
                observed = subprocess.run(command, env=environment, capture_output=True,
                                          text=True, timeout=30)
                assert observed.returncode == 0 and not observed.stderr, observed
                assert inventory(envelope_root / "economic-evidence") == before
                assert inventory(envelope_root / "domains") == native_before
            # Rebind every checksum and normalized intent hash so the changed
            # envelope grammar, rather than stale transport hashes, is decisive.
            index, segment = "bucket-01.eai", "bucket-01-0.eas"
            cases = (
                ("native-mobile command key sentinel", 1, 60, struct.pack("<Q", 2**64 - 1)),
                ("native-mobile revision key sentinel", 1, 76, struct.pack("<Q", 2**64 - 1)),
                ("native-mobile zero key", 1, 60, b"\0" * 8),
                ("unknown entity kind", 1, 52, b"\x10"),
                ("reserved entity bytes", 1, 53, b"\x01"),
                ("unknown command type", 1, 24, struct.pack("<H", 22)),
                ("zero payload version", 1, 26, b"\0\0"),
                ("nonboolean publication", 0, 31, b"\x02"),
                ("unaccounted shop publication", 3, 26, struct.pack("<H", 5)),
                ("unknown shop publication version", 3, 26, struct.pack("<H", 9)),
            )
            for label, row, offset, data in cases:
                slot = 80 + row * 64
                position, size = struct.unpack_from("<II", before[index], slot + 52)
                position += 80
                value = bytearray(before[segment][position:position + size])
                command_start = 74
                value[command_start + offset:command_start + offset + len(data)] = data
                payload_size = struct.unpack_from("<I", value, command_start + 48)[0]
                intent_start = command_start + 96 + payload_size
                normalized = bytearray(value[command_start:intent_start - 4])
                normalized[4:8] = struct.pack("<I", 1)
                normalized[31] = 0
                normalized[32:40] = struct.pack("<Q", 1)
                value[intent_start + 160:intent_start + 192] = hashlib.sha256(
                    b"DURIS-ECONOMIC-COMMAND-V1\0" + normalized).digest()
                domain = (value[command_start + 24:command_start + 28] +
                          value[command_start + 48:command_start + 52] +
                          value[command_start + 92:command_start + 92 + payload_size])
                value[intent_start + 192:intent_start + 224] = hashlib.sha256(
                    b"DURIS-ECONOMIC-DOMAIN-V1\0" + domain).digest()
                value = rehash(value)
                raw = Path(build) / "envelope-record.bin"
                raw.write_bytes(value)
                native = subprocess.run([str(fixture), str(raw), "decode-record"],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert native.returncode == 1 and not native.stderr, (label, native)
                files = dict(before)
                changed_segment = bytearray(files[segment])
                changed_segment[position:position + size] = value
                files[segment] = rehash(changed_segment)
                change(files, index, slot + 16, hashlib.sha256(value).digest())
                for name, content in files.items():
                    (envelope_root / "economic-evidence" / name).write_bytes(content)
                for command in ([str(audit), str(envelope_root)],
                                [str(binary), "--economic-evidence-audit", str(envelope_root)]):
                    observed = subprocess.run(command, env=environment, capture_output=True,
                                              text=True, timeout=30)
                    assert observed.returncode == 1 and not observed.stdout, (label, observed)
                    assert observed.stderr == "native_restore_qualification_failed\n"
                    assert inventory(envelope_root / "economic-evidence") == files
                    assert inventory(envelope_root / "domains") == native_before
                print("ENVELOPE_REFUSED " + label, flush=True)
            for name, content in before.items():
                (envelope_root / "economic-evidence" / name).write_bytes(content)
            restored = subprocess.run([str(audit), str(envelope_root)], env=environment,
                                      capture_output=True, text=True, timeout=30)
            assert restored.returncode == 0 and not restored.stdout and not restored.stderr
            assert inventory(envelope_root / "economic-evidence") == before
            assert inventory(envelope_root / "domains") == native_before
        successes, refusals, native_semantic_decodes = 0, 0, 0
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
                operator = subprocess.run([str(binary), "--economic-evidence-audit", str(state)],
                                          capture_output=True, text=True, timeout=30)
                assert (operator.returncode == 0) == valid, (label, operator.stderr)
                provenance_complete = False
                if valid:
                    provenance_complete = json.loads(operator.stdout)["baseline_provenance_complete"]
                    assert not operator.stderr
                else:
                    assert not operator.stdout
                    assert operator.stderr == "native_restore_qualification_failed\n"
                assert retained() == before, label + ": operator audit changed evidence"
                # Both manager preflight and post-replay qualification use this same gate.
                qualified = valid and provenance_complete
                for arguments in (["--state-preflight", str(state)], [str(state)]):
                    result = subprocess.run([str(binary), *arguments], capture_output=True,
                                            text=True, timeout=30, check=False)
                    assert (result.returncode == 0) == qualified, (label, result.returncode,
                                                              result.stdout, result.stderr)
                    if qualified:
                        assert json.loads(result.stdout) == {
                            "accounts": 0, "identities": 0, "players_loaded": 0, "snapshots": 0}
                    else:
                        assert not result.stdout and result.stderr.strip() == \
                            "native_restore_qualification_failed", (label, result.stderr)
                    assert retained() == before, label + ": retained authority changed"
                successes += valid
                refusals += not valid
                print(("PASS " if qualified else "READABLE_UNQUALIFIED " if valid else "REFUSED ")
                      + label, flush=True)

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

            mobile_checks = {"positive_images": 0, "refused_images": 0, "invocations": 0}
            domains = state / "domains"
            mobile = domains / "quest-mobile-native-42.qmn"
            independent_values = True

            def mobile_check(label, valid):
                def retained_images():
                    result = {}
                    for path in domains.iterdir():
                        if not path.name.startswith("quest-mobile-native"):
                            continue
                        info = path.lstat()
                        payload = (path.read_bytes() if path.is_file() and not path.is_symlink()
                                   else os.readlink(path) if path.is_symlink() else None)
                        result[path.name] = (info.st_mode, info.st_nlink, payload)
                    return result
                before = retained_images()
                if independent_values:
                    from economic_restore_evidence import decode_native_mobile
                    try:
                        decoded = decode_native_mobile(mobile.read_bytes())
                        accepted = decoded[:3] == (42, 2, 3)
                    except ValueError:
                        accepted = False
                    assert accepted == valid, (label, "independent native-mobile value disagreement")
                for command in ([str(binary), "--state-preflight", str(state)], [str(binary), str(state)]):
                    result = subprocess.run(command, env=environment, capture_output=True, text=True, timeout=30)
                    assert (result.returncode == 0) == valid, (label, result.returncode,
                                                             result.stdout, result.stderr)
                    if valid:
                        assert json.loads(result.stdout) == {
                            "accounts": 0, "identities": 0, "players_loaded": 0, "snapshots": 0}
                    else:
                        assert not result.stdout and result.stderr.strip() == "native_restore_qualification_failed"
                    assert retained_images() == before, label + ": native images changed"
                    mobile_checks["invocations"] += 1
                mobile_checks["positive_images" if valid else "refused_images"] += 1
                print(("MOBILE_PASS " if valid else "MOBILE_REFUSED ") + label, flush=True)

            def seal_image(data):
                return bytes(data[:-32]) + hashlib.sha256(data[:-32]).digest()

            images = {}
            for version in (1, 2):
                for lifetime in ("live", "retired"):
                    label = f"mobile-v{version}-{lifetime}"
                    subprocess.run([str(fixture), str(state), label], env=environment, check=True)
                    images[label] = mobile.read_bytes()
                    mobile_check(label, True)
            live = images["mobile-v2-live"]
            mobile.write_bytes(b"corrupt-native-mobile-image")
            mobile_check("damaged retained native mobile image", False)

            for label, offset, data in (
                    ("image magic", 0, b"X"), ("image version", 8, struct.pack("<H", 3)),
                    ("image state", 10, b"\x03"), ("image reserved", 11, b"\x01"),
                    ("image length", 12, b"\x00" * 4),
                    ("zero last transition", 164, b"\x00" * 16),
                    ("zero cash revision", 180, b"\x00" * 8),
                    ("negative cash", 188, struct.pack("<q", -1)),
                    ("cash native overflow", 188, struct.pack("<q", 2**31)),
                    ("stock length", 220, b"\x00" * 4),
                    ("stock count", 224, struct.pack("<I", 3001))):
                changed = bytearray(live)
                changed[offset:offset + len(data)] = data
                mobile.write_bytes(seal_image(changed))
                mobile_check(label, False)
            for label, offset, data in (
                    ("reference magic", 0, b"X"), ("reference version", 8, struct.pack("<H", 2)),
                    ("reference provenance", 10, b"\x03"), ("reference reserved", 11, b"\x01"),
                    ("reference length", 12, b"\x00" * 4),
                    ("zero lifetime ID", 16, b"\x00" * 8),
                    ("reserved lifetime ID", 16, struct.pack("<Q", 2**64-1)),
                    ("zero birth operation", 24, b"\x00" * 16),
                    ("invalid birth source", 40, b"\x00" * 48),
                    ("negative mobile vnum", 88, struct.pack("<i", -1)),
                    ("invalid reset zone", 96, struct.pack("<i", -1)),
                    ("zero mobile revision", 100, b"\x00" * 8),
                    ("zero stock revision", 108, b"\x00" * 8),
                    ("filename lifetime differs", 16, struct.pack("<Q", 43))):
                changed = bytearray(live)
                reference = bytearray(changed[16:164])
                reference[offset:offset + len(data)] = data
                changed[16:164] = seal_image(reference)
                mobile.write_bytes(seal_image(changed))
                mobile_check(label, False)
            mobile.write_bytes(live[:-1] + bytes([live[-1] ^ 1]))
            mobile_check("image checksum", False)
            mobile.write_bytes(live[:-1])
            mobile_check("truncated image", False)
            mobile.write_bytes(live + b"\x00")
            mobile_check("trailing image bytes", False)
            changed = bytearray(images["mobile-v2-retired"])
            changed[188:196] = struct.pack("<q", 1)
            mobile.write_bytes(seal_image(changed))
            mobile_check("retired image retains cash", False)
            changed = bytearray(live)
            changed[10] = 2
            changed[188:220] = b"\x00" * 32
            mobile.write_bytes(seal_image(changed))
            mobile_check("retired image retains stock", False)
            from test_economic_sql_canonical_audit import (NATIVE_MOBILE_STOCK_DAMAGE,
                native_mobile_forests, native_mobile_image, native_mobile_stock)
            stock = native_mobile_stock()
            mobile.write_bytes(native_mobile_image(items=stock))
            mobile_check("modeled literal stock with dynamic affects and spellbook", True)
            for label, offset, data in NATIVE_MOBILE_STOCK_DAMAGE:
                changed = bytearray(stock)
                changed[offset:offset+len(data)] = data
                mobile.write_bytes(native_mobile_image(items=bytes(changed)))
                mobile_check("modeled stock " + label, False)
            for label, valid, stock in native_mobile_forests():
                mobile.write_bytes(native_mobile_image(items=stock))
                mobile_check("modeled forest " + label, valid)
            mobile.write_bytes(live)
            independent_values = False  # File/name safety is the native reader's responsibility.
            for name in ("quest-mobile-native-042.qmn", "quest-mobile-native-0.qmn",
                         "quest-mobile-native-18446744073709551615.qmn",
                         "quest-mobile-native-18446744073709551616.qmn",
                         "quest-mobile-native-+42.qmn", "quest-mobile-native-42.qmn.tmp",
                         "quest-mobile-native-42", "quest-mobile-native-42.QMN",
                         "quest-mobile-native42.qmn", "quest-mobile-native.qmn",
                         "quest-mobile-native-.qmn"):
                mobile.unlink()
                alternate = domains / name
                alternate.write_bytes(live)
                mobile_check("noncanonical filename " + name, False)
                alternate.unlink()
                mobile.write_bytes(live)
            mobile.chmod(0o644)
            mobile_check("nonprivate native image", False)
            mobile.chmod(0o600)
            os.link(mobile, domains / "quest-mobile-native-hardlink")
            mobile_check("hardlinked native image", False)
            (domains / "quest-mobile-native-hardlink").unlink()
            mobile.unlink()
            mobile.symlink_to(domains / ".critical-authority.lock")
            mobile_check("symlink native image", False)
            mobile.unlink()
            os.mkfifo(mobile, 0o600)
            mobile_check("FIFO native image refuses without blocking", False)
            mobile.unlink()
            mobile.mkdir(mode=0o700)
            mobile_check("directory native image", False)
            mobile.rmdir()
            mobile.write_bytes(live + b"\x00" * (4 * 1024 * 1024 + 1 - len(live)))
            mobile_check("oversized native image", False)
            mobile.unlink()
            print("NATIVE_MOBILE_RESTORE_CHECKS " + json.dumps(mobile_checks, sort_keys=True), flush=True)
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
            stride = 96 if struct.unpack_from("<I", clean["epochs.eae"], 8)[0] == 1 else 160
            case("catalog predecessor", lambda f: change(f, "epochs.eae", 72 + stride + 24, b"\x63", 152))
            case("catalog duplicate epoch", lambda f: change(f, "epochs.eae", 72 + stride, f["epochs.eae"][72:88], 152))
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
            # Keep every physical checksum valid. Generic successful EAP1
            # bodies must receive the same semantic refusal as native decode.
            account_start = plan_start + 256
            posting_start = account_start + struct.unpack_from("<I", first, plan_start + 216)[0] * 120
            semantic_cases = []
            for label, offset, data in (
                ("account lineage", account_start, b"\x63"),
                ("account version", account_start + 16, b"\x02"),
                ("account reserved", account_start + 36, b"\x01"),
                ("account zero lifetime", account_start + 20, b"\x00" * 8),
                ("duplicate account key", account_start + 120, first[account_start:account_start + 40]),
                ("forbidden bank-transfer stake", account_start + 120 + 18, struct.pack("<H", 11)),
                ("negative opening", account_start + 40, struct.pack("<q", -1)),
                ("negative result", account_start + 72, struct.pack("<q", -1)),
                ("holding copper overflow", account_start + 64, struct.pack("<q", 2**63 - 1)),
                ("changed value unchanged revision", account_start + 112, struct.pack("<Q", 0)),
                ("after vector disagreement", account_start + 72, struct.pack("<q", 91)),
                ("posting event order", posting_start, struct.pack("<I", 1)),
                ("posting missing account", posting_start + 4, struct.pack("<H", 2)),
                ("posting unsupported child", posting_start + 6, struct.pack("<H", 1)),
                ("posting zero vector", posting_start + 8, b"\x00" * 32),
                ("posting copper disagreement", posting_start + 40, struct.pack("<q", -9)),
                ("posting copper overflow", posting_start + 32, struct.pack("<q", 2**63 - 1)),
                ("unreferenced changed holding", posting_start + 4, struct.pack("<H", 1))):
                semantic_cases.append(("generic plan " + label,
                    lambda f, o=offset, d=data: alter_record(f, o, d)))
            for label, mutate in semantic_cases:
                record_case(label, mutate)

            def rebind_intent(files, offset, data):
                value = bytearray(first)
                value[intent_start + offset:intent_start + offset + len(data)] = data
                value[plan_start + 8:plan_start + 72] = value[intent_start + 32:intent_start + 96]
                value[plan_start + 72] = value[intent_start + 26]
                value[plan_start + 84:plan_start + 98] = value[intent_start + 12:intent_start + 26]
                intent_length = struct.unpack_from("<I", value, intent_start - 4)[0]
                value[plan_start + 152:plan_start + 184] = hashlib.sha256(
                    b"DURIS-ECONOMIC-INTENT-V1\x00" + value[intent_start:intent_start + intent_length]).digest()
                value = rehash(value)
                segment = bytearray(files[sealed])
                segment[80:80 + first_size] = value
                files[sealed] = rehash(segment)
                change(files, index, 96, hashlib.sha256(value).digest())

            intent_cases = []
            for label, offset, data in (
                ("policy version", 16, struct.pack("<I", 2)),
                ("compiler version", 20, struct.pack("<I", 2)),
                ("actor policy", 26, b"\x02"),
                ("self original", 80, first[intent_start + 64:intent_start + 80]),
                ("required source", 24, struct.pack("<H", 5)),
                ("required original", 24, struct.pack("<H", 19))):
                intent_cases.append(("generic intent " + label,
                    lambda f, o=offset, d=data: rebind_intent(f, o, d)))
            for label, mutate in intent_cases:
                record_case(label, mutate)

            def native_decode(files, label, valid, intent=False):
                nonlocal native_semantic_decodes
                size = struct.unpack_from("<I", files[index], 136)[0]
                record = files[sealed][80:80 + size]
                command_size = struct.unpack_from("<I", record, 48)[0]
                keys, revisions, payload_size = struct.unpack_from("<III", record, 74 + 40)
                start = 74 + 52 + keys * 16 + revisions * 24 + payload_size + 4 if intent else 74 + command_size
                length = struct.unpack_from("<I", record, start - 4 if intent else 52)[0]
                raw = Path(build) / ("semantic-native.eai" if intent else "semantic-native.eap")
                raw.write_bytes(record[start:start + length])
                before = raw.read_bytes()
                result = subprocess.run([str(fixture), str(raw), "decode-intent" if intent else "decode-plan"],
                    env=environment, capture_output=True, text=True, timeout=30)
                assert result.returncode == int(not valid) and not result.stderr, (label, result)
                assert (result.stdout.strip() == "0") == valid, (label, result.stdout)
                assert raw.read_bytes() == before
                native_semantic_decodes += 1

            native_decode(records, "clean generic plan", True)
            native_decode(records, "clean generic intent", True, intent=True)
            for label, mutate in semantic_cases + intent_cases:
                files = dict(records)
                mutate(files)
                native_decode(files, label, False, intent=label.startswith("generic intent"))
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

            # Native structural item plans keep valid equipment, destruction's
            # former edges and an all-zero absent creation witness. Domain
            # gameplay is not applied and accounting remains inactive.
            restore({})
            subprocess.run([str(fixture), str(state), "item-records"], env=environment, check=True)
            check("native equipment destruction and creation record", True)
            item_records = inventory(evidence)
            item_size = struct.unpack_from("<I", item_records[index], 136)[0]
            item_first = item_records[sealed][80:80 + item_size]
            item_plan = 74 + struct.unpack_from("<I", item_first, 48)[0]
            assert struct.unpack_from("<6I", item_first, item_plan + 216) == (0, 0, 0, 3, 3, 3)
            item_before = item_plan + 256
            item_after = item_before + 3 * 64
            item_events = item_after + 3 * 64
            native_decode(item_records, "clean item plan", True)
            native_decode(item_records, "clean item intent", True, intent=True)

            def alter_item_record(files, offset, data):
                value = bytearray(item_first)
                value[offset:offset + len(data)] = data
                value = rehash(value)
                segment = bytearray(files[sealed])
                segment[80:80 + item_size] = value
                files[sealed] = rehash(segment)
                change(files, index, 96, hashlib.sha256(value).digest())

            item_cases = (
                ("zero UID", item_before, b"\x00" * 8),
                ("UID order", item_before, struct.pack("<Q", 82)),
                ("reserved position", item_before + 10, b"\x01"),
                ("unknown state", item_before + 9, b"\x04"),
                ("live destruction", item_before + 8, b"\x08"),
                ("zero live owner", item_before + 16, b"\x00" * 8),
                ("mobile context", item_before + 64 + 8, struct.pack("<BB6xQQ", 12, 1, 7, 1)),
                ("zero root", item_before + 32, b"\x00" * 8),
                ("root identity", item_before + 32, struct.pack("<Q", 99)),
                ("self parent", item_before + 64 + 40, struct.pack("<Q", 82)),
                ("missing parent", item_before + 64 + 40, struct.pack("<Q", 999)),
                ("cross owner parent", item_before + 64 + 16, struct.pack("<Q", 8)),
                ("contained equipment", item_before + 64 + 56, struct.pack("<H", 1)),
                ("absent nonzero root", item_before + 2 * 64 + 32, struct.pack("<Q", 83)),
                ("UID set disagreement", item_after + 2 * 64, struct.pack("<Q", 84)),
                ("player tombstone", item_after + 64 + 8, b"\x01"),
                ("destroyed zero revision", item_after + 64 + 48, b"\x00" * 8),
                ("after equipment disagreement", item_after + 56, struct.pack("<H", 7)),
                ("event index", item_events, struct.pack("<I", 1)),
                ("event child", item_events + 4, struct.pack("<H", 1)),
                ("event reserved", item_events + 6, b"\x01"),
                ("event missing UID", item_events + 8, struct.pack("<Q", 99)),
                ("event before equipment", item_events + 16 + 48, struct.pack("<H", 4)),
                ("event after revision", item_events + 72 + 40, struct.pack("<Q", 3)),
                ("event after absent", item_events + 72, b"\x00" * 56),
                ("creation destroyed", item_events + 2 * 128 + 72,
                 struct.pack("<BB6x5QH6x", 8, 2, 0, 0, 83, 0, 1, 0)),
            )
            for label, offset, data in item_cases:
                files = dict(item_records)
                alter_item_record(files, offset, data)
                native_decode(files, label, False)
                restore(files)
                check("generic item " + label, False)
            restore(item_records)
            check("clean native item record remains qualified", True)

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
            witness_version = struct.unpack_from("<H", baselines[witness], 4)[0]
            assert witness_version in (1, 2)
            assert baselines[witness][:4] == {1: b"EAB1", 2: b"EAB2"}[witness_version]
            item_stride = {1: 88, 2: 96}[witness_version]
            item_source_offset = item_stride - 32
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
                ("magic", 0, b"X"), ("version", 4, struct.pack("<H", 3)),
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
                ("item order", items_start, baselines[witness][items_start + item_stride:items_start + 2 * item_stride]),
                ("item owner", items_start + 8, b"\x00"),
                ("absent item", items_start + 9, b"\x00"),
                ("item reserved", items_start + 10, b"\x01"),
                ("zero live owner", items_start + 16, b"\x00" * 8),
                ("zero item root", items_start + 32, b"\x00" * 8),
                ("live root mismatch", items_start + 32, struct.pack("<Q", 256)),
                ("self parent", items_start + 40, struct.pack("<Q", 255)),
                ("missing live parent", items_start + item_stride + 40, struct.pack("<Q", 999)),
                ("cross owner parent", items_start + item_stride + 16, struct.pack("<Q", 8)),
                ("cycle", items_start + 40, struct.pack("<Q", 256)),
                ("destroyed zero revision", items_start + 2 * item_stride + 48, b"\x00" * 8),
                ("destroyed active owner", items_start + 2 * item_stride + 8, b"\x01"),
                ("pet context too large", items_start + 3 * item_stride + 24, struct.pack("<Q", 2**31)),
                ("collector context", items_start + 4 * item_stride + 24, b"\x01"),
                ("system owner", items_start + 5 * item_stride + 16, b"\x01"),
                ("item source digest", items_start + item_source_offset, b"\x00" * 32)):
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
                          "native_invocations_per_case": 3, "economic_bytes_unchanged": True,
                          "generic_semantic_corruptions": 50, "native_semantic_decodes": native_semantic_decodes,
                          "native_metadata_comparisons": 1058,
                          "native_command_envelope_comparisons": 574,
                          "native_command_envelope_records": 11,
                          "native_command_envelope_refusals": 10,
                          "audit_boundary_cases": boundary_cases,
                          "audit_limit_cases": limit_cases,
                          "root_page_cases": root_page_cases,
                          "authority_page_cases": authority_page_cases,
                          "qualifier_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                          "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
                          "sanitized_reader_sha256": hashlib.sha256(audit.read_bytes()).hexdigest()}))


if __name__ == "__main__":
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    main()
