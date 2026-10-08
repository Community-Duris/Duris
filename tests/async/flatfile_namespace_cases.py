"""Real native filename inventory, reverse links, cut refusal and protected resume."""
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from unittest import mock

import flatfile_namespace_audit as namespace


def check_namespace_pages(binary, fixture, audit, environment, build, *, lifecycle=False):
    from test_flatfile_restore_baseline_markers import retained
    root_source = Path(__file__).resolve().parents[2]
    destination = root_source / "bin/tests" / ("namespace-pages-lifecycle" if lifecycle else "namespace-pages-authority")
    destination.mkdir(mode=0o700)
    for tool in (binary, fixture, audit, Path(build) / "audit.cpp"):
        shutil.copy2(tool, destination / tool.name)
    probe_source = destination / "budget.cpp"
    probe_source.write_text('''#include "qualify_flatfile_economic_namespace.h"
#include <iostream>
int main(int argc, char **argv) {
    if (argc != 4) return 2;
    try {
        using namespace restore_economic_authority;
        authority_read_lock lock(argv[2]);
        restore_economic_records::checker reader(argv[2]);
        const auto context = reader.history_context();
        audit_budget budget;
        const auto files = budget.remaining_files, bytes = budget.remaining_bytes, entries = budget.remaining_entries;
        const std::string mode = argv[1];
        if (mode == "files") budget.remaining_files = 1;
        else if (mode == "bytes") budget.remaining_bytes = 1;
        else if (mode == "deadline") budget.deadline = std::chrono::steady_clock::now();
        else need(mode == "bounded");
        scoped_audit_budget scope(budget);
        reader.namespace_file(argv[3], context);
        lock.finish(); budget.checkpoint();
        std::cout << files << " " << bytes << " " << entries << " " << files-budget.remaining_files
                  << " " << bytes-budget.remaining_bytes << " " << entries-budget.remaining_entries << "\\n";
        return 0;
    } catch (const restore_economic_authority::audit_budget_refused &) { return 1; }
      catch (...) { return 2; }
}
''')
    probe = destination / "budget"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                    "-I" + str(root_source / "scripts"), str(probe_source), "-lcrypto", "-o", str(probe)], check=True)
    observations = []
    with tempfile.TemporaryDirectory(prefix="physical-namespace-", dir=build) as temporary:
        parent = Path(temporary)
        root = parent / "state"
        root.mkdir(mode=0o700)
        evidence = root / "economic-evidence"
        sequence = 0

        def produce(mode):
            if evidence.exists():
                shutil.rmtree(evidence)
            ran = subprocess.run([str(fixture), str(root), mode], env=environment,
                                 capture_output=True, text=True, timeout=600)
            assert ran.returncode == 0 and not ran.stderr, (mode, ran)
            assert (evidence / "authority.eal").read_bytes()[112:128] == bytes(16)
            saved = destination / ("native-" + mode)
            shutil.copytree(evidence, saved, dirs_exist_ok=True)
            return {path.name: path.read_bytes() for path in evidence.iterdir()}

        def fresh(*, tool=None):
            nonlocal sequence
            sequence += 1
            path = parent / ("progress-" + str(sequence) + ".json")
            before = retained(root)
            page, state = namespace.run(root, binary if tool is None else tool, path)
            assert page["phase"] == "inventory" and state["phase"] == "files" and not state["total_findings"], page
            assert retained(root) == before
            return path, state

        def sweep(path, state, *, tool=None):
            before = retained(root)
            while state["phase"] != "closed":
                if tool is None:
                    page, state = namespace.scan(root, binary, path, state)
                else:
                    # Fresh checkpoint uses the sanitizer reader as its bound qualifier.
                    page, state = namespace.scan(root, tool, path, state)
                assert not page["page_refused"], page
                assert not any(page[key] for key in ("complete", "consistent_entire_sweep", "orphan_namespace_closed",
                    "native_holdings_compared", "baseline_books_closed", "release_qualified"))
            assert retained(root) == before
            namespace.progress_io.save(path, state, namespace.MAX_PROGRESS_BYTES, namespace.pages.AuditError, namespace.LABEL)
            return page, state

        def record(label, page):
            observations.append(dict(label=label, report=page, native_state_unchanged=True))

        if not lifecycle:
            heads = produce("pile-heads")
            active = "pile-head-00000000000000ff.eph"
            retired = "pile-head-0000000000000100.eph"
            maximum = "pile-head-ffffffffffffffff.eph"
            assert {name for name in heads if name.endswith(".eph")} == {active, retired, maximum}
            assert heads[active][:4] == b"EPH1" and len(heads[active]) == 133
            shutil.copytree(root, destination / "pile-head-fixture")

            def rehash_head(value):
                result = bytearray(value)
                result[-32:] = hashlib.sha256(result[:-32]).digest()
                return bytes(result)

            damaged = []
            for offset, value, label in (
                    (0, b"EPH2", "version"), (4, bytes(16), "zero lineage"),
                    (4, b"\x02" + bytes(15), "foreign lineage"),
                    (20, bytes(16), "zero epoch"),
                    (20, b"\xfe" + bytes(15), "unknown epoch"), (36, bytes(8), "zero UID"),
                    (36, struct.pack("<Q", 256), "filename UID mismatch"),
                    (44, bytes(8), "zero revision"),
                    (52, struct.pack("<Q", 2**63), "negative denomination"),
                    (60, struct.pack("<Q", 2**31), "denomination overflow"),
                    (84, b"\x02", "retirement flag"),
                    (84, b"\x01", "retired nonzero balance"),
                    (85, bytes(16), "zero operation")):
                value_head = bytearray(heads[active])
                value_head[offset:offset + len(value)] = value
                damaged.append(("pile head " + label, active, rehash_head(value_head)))
            bad_checksum = bytearray(heads[active])
            bad_checksum[-1] ^= 1
            damaged.extend((
                ("pile head checksum", active, bytes(bad_checksum)),
                ("pile head truncated", active, heads[active][:-1]),
                ("pile head trailing bytes", active, heads[active] + b"\0"),
                ("pile head short payload", active, b"EPH1"),
                ("pile head noncanonical name", "pile-head-ff.eph", heads[active]),
                ("pile head uppercase name", "pile-head-00000000000000FF.eph", heads[active]),
                ("pile head zero name", "pile-head-0000000000000000.eph", heads[active]),
                ("pile head malformed prefix", "pile-head-broken", heads[active]),
                ("pile head malformed suffix", "unrelated.eph", heads[active])))
            for label, name, encoded in damaged:
                produce("pile-heads")
                (evidence / active).unlink()
                (evidence / name).write_bytes(encoded)
                assert (evidence / name).stat().st_mode & 0o077 == 0
                before = retained(root)
                native = subprocess.run([str(fixture), str(root), "read-pile-head"], env=environment,
                                        capture_output=True, text=True, timeout=60)
                assert native.returncode == (0 if label in ("pile head foreign lineage", "pile head unknown epoch") else 1) and not native.stderr and retained(root) == before, (label, native)
                path, state = fresh()
                page, state = sweep(path, state)
                assert not page["known_physical_economic_namespace_closed"] and state["invalid"] == 1, (label, page)
                assert any(item["name_sha256"] == hashlib.sha256(os.fsencode(name)).hexdigest() for item in state["findings"])
                record(label, page)
            for tool, label in ((binary, "native operator"), (audit, "sanitized dispatcher")):
                produce("pile-heads")
                path, state = fresh(tool=tool)
                page, state = sweep(path, state, tool=tool)
                assert page["known_physical_economic_namespace_closed"] and state["verified"] >= 5 and not state["ignored"], page
                record(label + " authentic active/retired/full-width pile heads", page)

        modes = ("mixed", "empty", "retained", "generic") if lifecycle else (
            "baseline-empty-history", "baseline-history", "baseline-rich", "source-claims")
        for mode in modes:
            produce(mode)
            path, state = fresh()
            page, state = sweep(path, state)
            assert page["known_physical_economic_namespace_closed"] and state["cursor"] == state["entries"], (mode, page)
            record(mode + " independent reverse traversal", page)
            cli = [sys.executable, str(root_source / "scripts/flatfile_economic_audit.py"), "--state-root", str(root),
                   "--qualifier", str(binary), "--scope", "physical-namespace", "--progress", str(path)]
            before = retained(root)
            ran = subprocess.run(cli, env=environment, capture_output=True, text=True, timeout=60)
            assert ran.returncode == 0 and not ran.stderr and retained(root) == before, ran
            reopened = json.loads(ran.stdout)
            assert reopened["phase"] == "closed" and reopened["next_phase"] == "files" and not reopened["known_physical_economic_namespace_closed"]
            record(mode + " closed CLI starts another traversal", reopened)
            # The same native dispatcher and independent readers run under ASan/UBSan.
            protected = parent / ("sanitized-" + mode + ".json")
            with mock.patch.dict(os.environ, environment):
                page, sanitized = namespace.run(root, audit, protected)
                page, sanitized = sweep(protected, sanitized, tool=audit)
            assert page["known_physical_economic_namespace_closed"] and not sanitized["total_findings"], (mode, page)
            record(mode + " sanitized reverse traversal", page)

        files = produce("mixed" if lifecycle else "baseline-history")
        witness = next(name for name in files if name.endswith(".eab"))
        for mode in ("bounded", "files", "bytes", "deadline"):
            before = retained(root)
            ran = subprocess.run([str(probe), mode, str(root), witness], env=environment,
                                 capture_output=True, text=True, timeout=60)
            assert ran.returncode == (0 if mode == "bounded" else 1) and not ran.stderr and retained(root) == before, (mode, ran)
            if mode == "bounded":
                capacity_files, capacity_bytes, capacity_entries, reads, size, visits = map(int, ran.stdout.split())
                assert capacity_files == 16384 and capacity_bytes == 128 * 1024 * 1024 and capacity_entries == 8192
                assert 0 < reads <= capacity_files and 0 < size <= capacity_bytes and visits == 0
            observations.append(dict(label="native reverse check " + mode + " budget", exit=ran.returncode,
                                     counters=ran.stdout.strip(), native_state_unchanged=True))
        samples = []
        for suffix, label in ((".eab", "unreferenced baseline witness"), (".ebi", "malformed baseline shard"),
                              (".eas", "unreferenced common segment"), (".eai", "malformed common index"),
                              (".eam", "malformed mapping index"), (".ean", "malformed native index"),
                              (".elr", "unreferenced lifecycle receipt")):
            name = next((name for name in files if name.endswith(suffix)), None)
            if name is None:
                assert suffix == ".elr" and not lifecycle
                continue
            extra = (name[:-36] + "11" * 16 + suffix if suffix == ".eab" else
                     name[:-5] + "g.ebi" if suffix == ".ebi" else
                     name[:-5] + "999.eas" if suffix == ".eas" else
                     "lifecycle-" + "11" * 16 + ".elr" if suffix == ".elr" else name[:-4] + ".extra" + suffix)
            samples.append((label, extra, files[name]))
        if not lifecycle:
            source = produce("source-claims")
            claim = next(name for name in source if name.startswith("source-claim-"))
            samples.append(("unreferenced native source claim", claim, source[claim]))
        for label, name, data in samples:
            produce("mixed" if lifecycle else "baseline-history")
            assert not (evidence / name).exists()
            (evidence / name).write_bytes(data)
            path, state = fresh()
            page, state = sweep(path, state)
            assert not page["known_physical_economic_namespace_closed"] and state["invalid"] >= 1 and state["total_findings"] >= 1, (label, page)
            assert any(item["name_sha256"] == hashlib.sha256(os.fsencode(name)).hexdigest() for item in state["findings"])
            record(label, page)

        produce("mixed" if lifecycle else "baseline-history")
        path, state = fresh()
        # Reopened processes authenticate exactly one captured chunk, not directory cookies.
        before_progress = path.read_bytes()
        for index, label in enumerate(("inventory chunk changed", "inventory manifest changed")):
            target = namespace.auxiliary(path)[index]
            original = target.read_bytes()
            target.write_bytes(original + b"x")
            try:
                namespace.run(root, binary, path)
                raise AssertionError(label)
            except (namespace.pages.AuditError, ValueError):
                pass
            assert path.read_bytes() == before_progress
            target.write_bytes(original)
            # Restoring bytes changes the inode fingerprint: use a fresh path.
            path, state = fresh()
            before_progress = path.read_bytes()
            observations.append(dict(label=label, refused=True, progress_unchanged=True))
        for target_name in ("binary", "manifest"):
            for unsafe in ("public", "symlink", "hardlink"):
                path, state = fresh()
                target = namespace.auxiliary(path)[0 if target_name == "binary" else 1]
                saved = target.with_name(target.name + ".saved")
                if unsafe == "public":
                    target.chmod(0o644)
                elif unsafe == "symlink":
                    target.rename(saved)
                    target.symlink_to(saved)
                else:
                    os.link(target, saved)
                original_progress = path.read_bytes()
                try:
                    namespace.run(root, binary, path)
                    raise AssertionError(unsafe)
                except (namespace.pages.AuditError, OSError):
                    pass
                assert path.read_bytes() == original_progress
                observations.append(dict(label=target_name + " " + unsafe + " refused", progress_unchanged=True))

        path, state = fresh()
        before = retained(root)
        for label, mutate in (("foreign executable/source", lambda value: value.update(source_digest="0" * 64)),
                              ("boolean cursor alias", lambda value: value.update(cursor=False)),
                              ("inconsistent count", lambda value: value.update(verified=1)),
                              ("wrong scope format", lambda value: value.update(format="flatfile_economic_roots_progress_v1")),
                              ("premature closure", lambda value: value.update(phase="closed"))):
            changed = copy.deepcopy(state)
            mutate(changed)
            try:
                namespace.validate(changed, namespace.source_digest(root, binary), time.time())
                raise AssertionError(label)
            except namespace.pages.AuditError:
                pass
            observations.append(dict(label=label + " refused", refused=True))
        inside = [sys.executable, str(root_source / "scripts/flatfile_economic_audit.py"), "--state-root", str(root),
                  "--qualifier", str(binary), "--scope", "physical-namespace", "--progress", str(root / "unsafe-progress.json")]
        ran = subprocess.run(inside, env=environment, capture_output=True, text=True, timeout=60)
        assert ran.returncode == 1 and not ran.stdout and ran.stderr == "flatfile_economic_audit_refused\n" and retained(root) == before
        observations.append(dict(label="CLI checkpoint inside authority refused before writes", native_state_unchanged=True))
        with namespace.progress_io.lock(path, namespace.pages.AuditError, namespace.LABEL):
            try:
                namespace.run(root, binary, path)
                raise AssertionError("concurrent owner")
            except namespace.pages.AuditError:
                pass
        assert retained(root) == before
        observations.append(dict(label="concurrent checkpoint owner refused", native_state_unchanged=True))
        original_progress = path.read_bytes()
        with mock.patch.object(namespace.progress_io.os, "replace", side_effect=OSError("interrupted")):
            try:
                namespace.run(root, binary, path)
                raise AssertionError("interrupted checkpoint")
            except OSError:
                pass
        assert path.read_bytes() == original_progress and retained(root) == before
        page, state = namespace.run(root, binary, path)
        assert state["cursor"] == 1
        record("interrupted file checkpoint retries same captured entry", page)
        anchored = copy.deepcopy(state)
        with mock.patch.object(namespace.subprocess, "run", side_effect=subprocess.TimeoutExpired("qualifier", 45)):
            page, refused = namespace.scan(root, binary, path, anchored)
        assert refused["cursor"] == anchored["cursor"] and refused["total_findings"] == 1 and page["page_refused"]
        record("native timeout preserves cursor", page)
        (evidence / "ignored-new-name").write_bytes(b"")
        before = retained(root)
        page, refused = namespace.scan(root, binary, path, anchored)
        assert page["page_refused"] and refused["cursor"] == anchored["cursor"] and retained(root) == before
        record("directory cut change preserves cursor", page)

        produce("mixed" if lifecycle else "baseline-history")
        path, state = fresh()
        page, state = sweep(path, state)
        witness = next(evidence.glob("baseline-*.eab"))
        damaged = bytearray(witness.read_bytes())
        damaged[-1] ^= 1
        witness.write_bytes(damaged)
        page, restarted = namespace.run(root, binary, path)
        assert not page["known_physical_economic_namespace_closed"] and restarted["cursor"] == 0
        page, restarted = sweep(path, restarted)
        assert restarted["invalid"] and not page["known_physical_economic_namespace_closed"]
        record("closed traversal revisits same-cut in-place witness damage", page)

        produce("mixed" if lifecycle else "baseline-history")
        failed_path = parent / "interrupted-inventory.json"
        with mock.patch.object(namespace.progress_io.os, "replace", side_effect=OSError("interrupted publication")):
            try:
                namespace.run(root, binary, failed_path)
                raise AssertionError("interrupted inventory")
            except OSError:
                pass
        assert not failed_path.exists() and namespace.auxiliary(failed_path)[0].is_file()
        preserved = namespace.auxiliary(failed_path)[0].read_bytes()
        try:
            namespace.run(root, binary, failed_path)
            raise AssertionError("unclaimed inventory overwritten")
        except namespace.pages.AuditError:
            pass
        assert namespace.auxiliary(failed_path)[0].read_bytes() == preserved
        observations.append(dict(label="interrupted inventory preserved; fresh path required", refused=True))

        for unsafe in ("inside-authority", "public", "hardlink", "nonempty", "append"):
            target = (root / "invalid-output" if unsafe == "inside-authority" else parent / ("invalid-" + unsafe))
            flags = os.O_CREAT | os.O_EXCL | os.O_RDWR | (os.O_APPEND if unsafe == "append" else 0)
            fd = os.open(target, flags, 0o600)
            try:
                if unsafe == "public":
                    target.chmod(0o644)
                elif unsafe == "hardlink":
                    os.link(target, parent / "invalid-output-linked")
                elif unsafe == "nonempty":
                    os.write(fd, b"retained")
                before = retained(root)
                original = target.read_bytes()
                ran = subprocess.run([str(audit), "--economic-namespace-inventory", str(root), str(fd)],
                                     pass_fds=(fd,), env=environment, capture_output=True, text=True, timeout=60)
                assert ran.returncode == 1 and not ran.stdout and retained(root) == before and target.read_bytes() == original, (unsafe, ran)
                observations.append(dict(label="native capture output " + unsafe + " refused", native_state_unchanged=True))
            finally:
                os.close(fd)

        if not lifecycle:
            produce("baseline-history")
            for index in range(8193):
                (evidence / ("ignored-" + str(index))).write_bytes(b"")
            path, state = fresh()
            assert state["entries"] > 8192
            page, state = sweep(path, state)
            assert page["known_physical_economic_namespace_closed"] and state["ignored"] == 8193 and not state["total_findings"], page
            record("healthy inventory above former8192 entry limit completes real bounded traversal", page)

    report = dict(cases=len(observations), observations=observations, skips=0, original_native_fixtures=True,
                  modeled_native_physical_families_exercised=True, complete_economic_release_qualified=False)
    (destination / "observations.json").write_text(json.dumps(report, indent=2) + "\n")
    return {key: value for key, value in report.items() if key != "observations"}
