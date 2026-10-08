"""Private inventory and bounded independent physical economic reverse checks."""
import copy
import hashlib
import json
import math
import os
from pathlib import Path
import stat
import struct
import subprocess
import tempfile
import time

import economic_audit_progress as progress_io
import flatfile_economic_audit as pages

FORMAT = "flatfile_economic_namespace_progress_v1"
INVENTORY_FORMAT = "flatfile_economic_namespace_inventory_v1"
SEGMENT_COUNT_LIMIT = (256 << 20) // (8 * 1024 * 1024 - (48 + 26 + 512 * 1024 + 4 * 1024 * 1024 + 4096) - 80) + 1
MAX_ENTRIES = 2 * 256 * 4096 + 4096 * 18 + 256 * (3 + SEGMENT_COUNT_LIMIT) + 8192 + 2
MAX_INVENTORY_BYTES = 128 * 1024 * 1024
MAX_METADATA_BYTES = 4 * 1024 * 1024
MAX_PROGRESS_BYTES = 32768
LABEL = "flatfile namespace progress"
FAMILIES = {"authority_metadata", "common_index", "common_segment", "baseline_head",
            "baseline_reservations", "baseline_witness", "lifecycle_receipt", "source_claim", "pile_head", "ignored", "invalid"}
CODES = {"flatfile_namespace_capture_refused", "flatfile_namespace_context_refused",
         "flatfile_namespace_page_refused", "flatfile_namespace_file_invalid", "flatfile_namespace_file_refused"}
require, identity, digest, integer = pages.require, pages.identity, pages.digest, pages.integer


def source_digest(root, qualifier):
    value = hashlib.sha256(pages.source_digest(root, qualifier).encode() + b"\0")
    value.update(Path(__file__).read_bytes())
    return value.hexdigest()


def new_progress(source, now):
    return dict(format=FORMAT, source_digest=source, inventory_binding=None, lineage=None, cut=None,
                phase="inventory", entries=0, cursor=0, verified=0, ignored=0, invalid=0,
                legacy_unknown_epochs=0, started_at=now, last_page_at=now,
                total_findings=0, findings=[], findings_truncated=False)


def validate(value, source, now):
    require(type(value) is dict and set(value) == set(new_progress(source, now)))
    require(value["format"] == FORMAT and value["source_digest"] == source and digest(source))
    require(value["phase"] in ("inventory", "files", "closed"))
    for key in ("entries", "cursor", "verified", "ignored", "invalid"):
        require(integer(value[key], MAX_ENTRIES))
    require(value["verified"] + value["ignored"] + value["invalid"] == value["cursor"] <= value["entries"])
    require(integer(value["legacy_unknown_epochs"], 4096) and integer(value["total_findings"]) and
            value["invalid"] <= value["total_findings"])
    require(type(value["findings_truncated"]) is bool and type(value["findings"]) is list and
            len(value["findings"]) <= pages.MAX_FINDINGS and value["total_findings"] >= len(value["findings"]))
    for item in value["findings"]:
        require(type(item) is dict and set(item) == {"code", "name_sha256"} and item["code"] in CODES and
                (item["name_sha256"] is None or digest(item["name_sha256"])))
        require((item["name_sha256"] is not None) == (item["code"] in
                ("flatfile_namespace_file_invalid", "flatfile_namespace_file_refused")))
    for key in ("started_at", "last_page_at"):
        require(type(value[key]) in (int, float) and math.isfinite(value[key]) and 0 <= value[key] <= now + 300)
    require(value["started_at"] <= value["last_page_at"])
    if value["phase"] == "inventory":
        require(all(value[key] is None for key in ("inventory_binding", "lineage", "cut")) and
                not value["entries"] and not value["cursor"] and not value["legacy_unknown_epochs"])
    else:
        require(digest(value["inventory_binding"]) and identity(value["lineage"]) and digest(value["cut"]))
        require((value["phase"] == "closed") == (value["cursor"] == value["entries"]))
    return value


def metadata(value):
    require(type(value) is dict and set(value) == {"initialized", "format", "lineage", "source_cut_sha256",
            "legacy_unknown_epochs", "entries", "chunks"} and value["initialized"] is True and
            value["format"] == INVENTORY_FORMAT and identity(value["lineage"]) and digest(value["source_cut_sha256"]))
    require(integer(value["entries"], MAX_ENTRIES) and value["entries"] >= 2 and integer(value["legacy_unknown_epochs"], 4096) and
            type(value["chunks"]) is list and len(value["chunks"]) == (value["entries"] + 127) // 128)
    offset = count = 0
    for index, chunk in enumerate(value["chunks"]):
        require(type(chunk) is dict and set(chunk) == {"offset", "bytes", "entries", "sha256"} and
                integer(chunk["offset"], MAX_INVENTORY_BYTES) and chunk["offset"] == offset and
                integer(chunk["entries"], 128) and 0 < chunk["entries"] and
                (index == len(value["chunks"]) - 1 or chunk["entries"] == 128) and
                integer(chunk["bytes"], 2 + 128 * 257) and
                2 + chunk["entries"] * 3 <= chunk["bytes"] and digest(chunk["sha256"]))
        offset += chunk["bytes"]
        count += chunk["entries"]
    require(count == value["entries"] and offset <= MAX_INVENTORY_BYTES)
    return value, offset


def fingerprint(info):
    return [info.st_dev, info.st_ino, info.st_size, info.st_nlink, info.st_mode, info.st_uid,
            info.st_mtime_ns, info.st_ctime_ns]


def protected_open(path, maximum):
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        require(stat.S_ISREG(info.st_mode) and info.st_uid == os.getuid() and
                info.st_nlink == 1 and not info.st_mode & 0o077 and info.st_size <= maximum)
    except BaseException:
        os.close(fd)
        raise
    return fd, info


def auxiliary(path):
    return Path(str(path) + ".inventory.bin"), Path(str(path) + ".inventory.json")


def inventory_open(path, state):
    binary, manifest = auxiliary(path)
    meta_fd, meta_info = protected_open(manifest, MAX_METADATA_BYTES)
    try:
        raw = os.read(meta_fd, MAX_METADATA_BYTES + 1)
        require(len(raw) == meta_info.st_size and fingerprint(os.fstat(meta_fd)) == fingerprint(meta_info))
    finally:
        os.close(meta_fd)
    value, size = metadata(progress_io.load(manifest, MAX_METADATA_BYTES, pages.AuditError, LABEL))
    # Both reads must agree; duplicate fields are rejected by the protected JSON loader.
    require(json.loads(raw) == value)
    fd, info = protected_open(binary, MAX_INVENTORY_BYTES)
    try:
        require(info.st_size == size)
        binding = hashlib.sha256(raw + b"\0" + json.dumps(fingerprint(info), separators=(",", ":")).encode()).hexdigest()
        if state["inventory_binding"] is not None:
            require(binding == state["inventory_binding"] and value["entries"] == state["entries"] and
                    value["lineage"] == state["lineage"] and value["source_cut_sha256"] == state["cut"] and
                    value["legacy_unknown_epochs"] == state["legacy_unknown_epochs"])
        return fd, info, value, binding
    except BaseException:
        os.close(fd)
        raise


def selected_name(fd, info, value, cursor):
    require(cursor < value["entries"])
    chunk = value["chunks"][cursor // 128]
    block = os.pread(fd, chunk["bytes"], chunk["offset"])
    require(len(block) == chunk["bytes"] and hashlib.sha256(block).hexdigest() == chunk["sha256"] and
            fingerprint(os.fstat(fd)) == fingerprint(info))
    count, = struct.unpack_from("<H", block)
    require(count == chunk["entries"])
    names = []
    offset = 2
    for _ in range(count):
        require(offset + 2 <= len(block))
        length, = struct.unpack_from("<H", block, offset)
        offset += 2
        require(0 < length <= 255 and offset + length <= len(block))
        name = block[offset:offset + length]
        require(name not in (b".", b"..") and b"/" not in name and b"\0" not in name)
        names.append(name)
        offset += length
    require(offset == len(block))
    return names[cursor % 128]


def native(qualifier, operation, root, *arguments, **kwargs):
    try:
        ran = subprocess.run([str(qualifier), "--economic-namespace-" + operation, str(root), *arguments],
                             capture_output=True, timeout=45, **kwargs)
    except subprocess.TimeoutExpired:
        return None
    if ran.returncode:
        return None
    require(len(ran.stdout) <= MAX_METADATA_BYTES and not ran.stderr)
    # Duplicate native JSON fields are rejected as well.
    def pairs(rows):
        value = {}
        for key, item in rows:
            require(key not in value)
            value[key] = item
        return value
    value = json.loads(ran.stdout, object_pairs_hook=pairs)
    require(type(value) is dict and type(value.get("initialized")) is bool)
    return value


def capture(root, qualifier, path):
    binary, manifest = auxiliary(path)
    # A failed publication is preserved. A fresh checkpoint path is required;
    # neither an old manifest nor an unclaimed inventory is silently replaced.
    require(not os.path.lexists(binary) and not os.path.lexists(manifest))
    fd, temporary = tempfile.mkstemp(prefix="." + path.name + "-inventory-", dir=path.parent)
    try:
        page = native(qualifier, "inventory", root, str(fd), pass_fds=(fd,))
        if page is None or page == {"initialized": False}:
            return page
        value, size = metadata(page)
        require(os.fstat(fd).st_size == size)
        os.fsync(fd)
        # Hard-link publication refuses concurrent path substitution rather
        # than overwriting it. The temporary name is then removed.
        os.link(temporary, binary, follow_symlinks=False)
        os.unlink(temporary)
        meta_fd, meta_temporary = tempfile.mkstemp(prefix="." + path.name + "-manifest-", dir=path.parent)
        try:
            data = (json.dumps(value, allow_nan=False, sort_keys=True, separators=(",", ":")) + "\n").encode()
            require(len(data) <= MAX_METADATA_BYTES)
            with os.fdopen(meta_fd, "wb") as stream:
                stream.write(data)
                stream.flush()
                os.fsync(stream.fileno())
            os.link(meta_temporary, manifest, follow_symlinks=False)
            os.unlink(meta_temporary)
            directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
        finally:
            if os.path.exists(meta_temporary):
                os.unlink(meta_temporary)
        return value
    finally:
        os.close(fd)
        if os.path.exists(temporary):
            os.unlink(temporary)


def retain(state, code, name=None):
    state["total_findings"] += 1
    item = dict(code=code, name_sha256=name)
    if item not in state["findings"]:
        if len(state["findings"]) == pages.MAX_FINDINGS:
            state["findings_truncated"] = True
        else:
            state["findings"].append(item)


def scan(root, qualifier, path, previous, *, now=None):
    now = time.time() if now is None else now
    source = source_digest(root, qualifier)
    validate(previous, source, now)
    state = copy.deepcopy(previous)
    phase, refused, examined, current_findings = state["phase"], False, 0, False
    if phase == "inventory":
        page = capture(root, qualifier, path)
        if page == {"initialized": False}:
            return report(state, phase, False, 0, False, initialized=False), previous
        if page is None:
            retain(state, "flatfile_namespace_capture_refused")
            refused = current_findings = True
        else:
            fd, _, value, binding = inventory_open(path, state)
            os.close(fd)
            state.update(inventory_binding=binding, lineage=value["lineage"], cut=value["source_cut_sha256"],
                         entries=value["entries"], legacy_unknown_epochs=value["legacy_unknown_epochs"],
                         phase="files" if value["entries"] else "closed")
    else:
        fd, info, value, _ = inventory_open(path, state)
        try:
            name = selected_name(fd, info, value, state["cursor"]) if phase == "files" else None
            page = native(qualifier, "file" if name else "context", root, state["cut"],
                          *([name.hex()] if name else []))
            if page is None:
                retain(state, "flatfile_namespace_page_refused" if name else "flatfile_namespace_context_refused")
                refused = current_findings = True
            else:
                require(page.get("initialized") is True and page.get("lineage") == state["lineage"] and
                        page.get("source_cut_sha256") == state["cut"])
                if phase == "closed":
                    require(set(page) == {"initialized", "lineage", "source_cut_sha256"})
                    state.update(cursor=0, verified=0, ignored=0, invalid=0,
                                 phase="files" if state["entries"] else "closed")
                else:
                    require(set(page) == {"initialized", "lineage", "source_cut_sha256", "name_sha256", "family", "valid", "file_refused"})
                    key = hashlib.sha256(name).hexdigest()
                    require(page["name_sha256"] == key and page["family"] in FAMILIES and
                            type(page["valid"]) is bool and type(page["file_refused"]) is bool and
                            not (page["valid"] and page["file_refused"]) and
                            (not page["valid"] or page["family"] != "invalid"))
                    if not page["valid"]:
                        retain(state, "flatfile_namespace_file_refused" if page["file_refused"] else
                               "flatfile_namespace_file_invalid", key)
                        state["invalid"] += 1
                        current_findings = True
                    else:
                        state["ignored" if page["family"] == "ignored" else "verified"] += 1
                    state["cursor"] += 1
                    examined = 1
                    if state["cursor"] == state["entries"]:
                        state["phase"] = "closed"
            require(fingerprint(os.fstat(fd)) == fingerprint(info))
            # Detect auxiliary substitution during the native call as well.
            checked, _, _, _ = inventory_open(path, state)
            os.close(checked)
        finally:
            os.close(fd)
    state["last_page_at"] = now
    validate(state, source, now)
    return report(state, phase, refused, examined, current_findings), state


def report(state, phase, refused, examined, current_findings, *, initialized=True):
    return dict(format="flatfile_economic_namespace_page_v1", scope="captured_physical_economic_namespace",
        initialized=initialized, phase=phase, next_phase=state["phase"], source_cut_sha256=state["cut"],
        captured_entries=state["entries"], examined_files=examined, next_file=state["cursor"],
        verified_economic_files=state["verified"], ignored_files=state["ignored"], invalid_files=state["invalid"],
        page_refused=refused, consistent_page=not refused and not current_findings,
        historical_range_complete=state["phase"] == "closed",
        known_physical_economic_namespace_closed=state["phase"] == "closed" and not state["total_findings"] and not refused,
        legacy_unknown_epochs=state["legacy_unknown_epochs"], total_finding_count=state["total_findings"],
        retained_finding_count=len(state["findings"]), findings_truncated=state["findings_truncated"],
        complete=False, consistent_entire_sweep=False, orphan_namespace_closed=False,
        native_holdings_compared=False, baseline_books_closed=False, release_qualified=False)


def run(root, qualifier, path):
    source, now = source_digest(root, qualifier), time.time()
    with progress_io.lock(path, pages.AuditError, LABEL):
        try:
            previous = validate(progress_io.load(path, MAX_PROGRESS_BYTES, pages.AuditError, LABEL), source, now)
        except FileNotFoundError:
            previous = new_progress(source, now)
        result, state = scan(root, qualifier, path, previous)
        if state is not previous:
            progress_io.save(path, state, MAX_PROGRESS_BYTES, pages.AuditError, LABEL)
    return result, state
