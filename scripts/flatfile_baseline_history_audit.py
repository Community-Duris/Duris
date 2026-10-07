"""Bounded independent history proof for catalogue-required initialized books."""
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import time

import economic_audit_progress as progress_io
import flatfile_economic_audit as pages

MAX_ROOTS = 256 * 4096
MAX_BOOKS = 4096
MAX_MEMBERS = 16 * 65536
MAX_ROOT_MEMBERS = 3071 + 6000
MAX_PROGRESS_BYTES = 2 * 1024 * 1024
MAX_FINDINGS = pages.MAX_FINDINGS
FORMAT = "flatfile_economic_baseline_history_progress_v1"
FINDING_CODES = {
    "flatfile_baseline_history_context_refused", "flatfile_baseline_history_control_refused",
    "flatfile_baseline_history_page_refused", "flatfile_baseline_history_record_invalid",
    "flatfile_baseline_history_duplicate_revision", "flatfile_baseline_history_missing_revision",
    "flatfile_baseline_history_reservation_coverage", "flatfile_baseline_history_terminal_missing",
    "flatfile_baseline_history_control_missing",
}
require, identity, digest, integer = pages.require, pages.identity, pages.digest, pages.integer


def source_digest(root, qualifier):
    value = hashlib.sha256(pages.source_digest(root, qualifier).encode() + b"\0")
    value.update(Path(__file__).read_bytes())
    return value.hexdigest()


def new_progress(source, now):
    return dict(format=FORMAT, source_digest=source, lineage=None, cut=None,
                phase="context", control_index=0, rotation=0, legacy_unknown_epochs=0,
                started_at=now, last_page_at=now, books=[], total_rows=0, total_verified=0,
                total_findings=0, findings=[], findings_truncated=False,
                buckets=[dict(cursor="", exhausted=False) for _ in range(256)])


def bitmap(book):
    return int("0" + book["bitmap"], 16)


def validate(value, source, now):
    empty = new_progress(source, now)
    require(type(value) is dict and set(value) == set(empty) and value["format"] == FORMAT)
    require(value["source_digest"] == source and digest(source))
    require(value["phase"] in ("context", "controls", "roots", "closed"))
    require(integer(value["rotation"], 255) and integer(value["legacy_unknown_epochs"], MAX_BOOKS))
    for name in ("started_at", "last_page_at"):
        require(type(value[name]) in (int, float) and math.isfinite(value[name]) and 0 <= value[name] <= now + 300)
    require(value["started_at"] <= value["last_page_at"])
    for name in ("total_rows", "total_verified", "total_findings"):
        require(integer(value[name]))
    require(value["total_verified"] <= value["total_rows"] <= MAX_ROOTS and type(value["findings_truncated"]) is bool)
    require(type(value["findings"]) is list and len(value["findings"]) <= MAX_FINDINGS)
    require(value["total_findings"] >= len(value["findings"]))
    for finding in value["findings"]:
        require(type(finding) is dict and set(finding) == {"bucket", "epoch_id", "operation_id", "code"})
        require(finding["bucket"] is None or integer(finding["bucket"], 255))
        require(finding["epoch_id"] is None or identity(finding["epoch_id"]))
        require(finding["operation_id"] is None or identity(finding["operation_id"], finding["bucket"]))
        require(finding["code"] in FINDING_CODES)
    require(type(value["books"]) is list and len(value["books"]) <= MAX_BOOKS)
    require(integer(value["control_index"], len(value["books"])))
    previous = ""
    expected = observed = 0
    for book in value["books"]:
        require(type(book) is dict and set(book) == {"epoch", "revision", "terminal", "control_checked",
            "reservation_total", "roots", "bitmap", "observed_reservations", "terminal_seen"})
        require(identity(book["epoch"]) and book["epoch"] > previous and identity(book["terminal"]))
        previous = book["epoch"]
        require(integer(book["revision"], MAX_ROOTS) and integer(book["roots"], MAX_ROOTS))
        expected += book["revision"]
        observed += book["roots"]
        require(type(book["control_checked"]) is bool and type(book["terminal_seen"]) is bool)
        require(book["reservation_total"] is None or integer(book["reservation_total"], MAX_MEMBERS))
        require(book["control_checked"] == (book["reservation_total"] is not None))
        require(integer(book["observed_reservations"], MAX_ROOTS * MAX_ROOT_MEMBERS))
        require(type(book["bitmap"]) is str and len(book["bitmap"]) == 2 * ((book["revision"] + 7) // 8) and
                re.fullmatch(r"[0-9a-f]*", book["bitmap"]) is not None)
        bits = bitmap(book)
        require(bits.bit_length() <= book["revision"] and bits.bit_count() <= book["roots"])
    require(expected <= MAX_ROOTS and observed <= value["total_verified"])
    require(type(value["buckets"]) is list and len(value["buckets"]) == 256)
    for bucket, row in enumerate(value["buckets"]):
        require(type(row) is dict and set(row) == {"cursor", "exhausted"} and type(row["exhausted"]) is bool)
        require(row["cursor"] == "" or identity(row["cursor"], bucket))
    if value["phase"] == "context":
        require(value["lineage"] is None and value["cut"] is None and not value["books"] and
                not value["control_index"] and not value["total_rows"] and not value["total_verified"] and
                all(not row["cursor"] and not row["exhausted"] for row in value["buckets"]))
    else:
        require(identity(value["lineage"]) and digest(value["cut"]))
    if value["phase"] in ("roots", "closed"):
        require(value["control_index"] == len(value["books"]))
    if value["phase"] == "controls":
        require(value["control_index"] < len(value["books"]) and not value["total_rows"] and
                not value["total_verified"] and not observed and
                all(not row["cursor"] and not row["exhausted"] for row in value["buckets"]))
    if value["phase"] == "closed":
        require(all(row["exhausted"] for row in value["buckets"]))
    return value


def finding(code, bucket=None, epoch=None, operation=None):
    return dict(code=code, bucket=bucket, epoch_id=epoch, operation_id=operation)


def retain(state, findings):
    state["total_findings"] += len(findings)
    for item in findings:
        if item in state["findings"]:
            continue
        if len(state["findings"]) == MAX_FINDINGS:
            state["findings_truncated"] = True
        else:
            state["findings"].append(item)


def closed_proof(state):
    return (state["phase"] == "closed" and not state["total_findings"] and
            all(book["control_checked"] and book["roots"] == book["revision"] and
                bitmap(book).bit_count() == book["revision"] and
                book["observed_reservations"] == book["reservation_total"] and
                (book["terminal_seen"] or book["revision"] == 0) for book in state["books"]))


def close_books(state):
    findings = []
    for book in state["books"]:
        epoch = book["epoch"]
        if not book["control_checked"]:
            findings.append(finding("flatfile_baseline_history_control_missing", epoch=epoch))
        if book["roots"] != book["revision"] or bitmap(book).bit_count() != book["revision"]:
            findings.append(finding("flatfile_baseline_history_missing_revision", epoch=epoch))
        if book["observed_reservations"] != book["reservation_total"]:
            findings.append(finding("flatfile_baseline_history_reservation_coverage", epoch=epoch))
        if book["revision"] and not book["terminal_seen"]:
            findings.append(finding("flatfile_baseline_history_terminal_missing", epoch=epoch))
    state["phase"] = "closed"
    return findings


def context_page(page, state, *, initialize):
    require(set(page) == {"initialized", "lineage", "source_cut_sha256", "legacy_unknown_epochs", "books"})
    require(identity(page["lineage"]) and digest(page["source_cut_sha256"]) and
            integer(page["legacy_unknown_epochs"], MAX_BOOKS) and type(page["books"]) is list and
            len(page["books"]) <= MAX_BOOKS)
    books = []
    previous = ""
    expected = 0
    for entry in page["books"]:
        require(type(entry) is dict and set(entry) == {"epoch", "revision", "terminal"})
        require(identity(entry["epoch"]) and entry["epoch"] > previous and
                integer(entry["revision"], MAX_ROOTS) and identity(entry["terminal"]))
        previous = entry["epoch"]
        expected += entry["revision"]
        require(expected <= MAX_ROOTS)
        books.append(dict(entry, control_checked=False, reservation_total=None, roots=0,
            bitmap="0" * (2 * ((entry["revision"] + 7) // 8)), observed_reservations=0, terminal_seen=False))
    if initialize:
        state.update(lineage=page["lineage"], cut=page["source_cut_sha256"], books=books,
            legacy_unknown_epochs=page["legacy_unknown_epochs"], phase="controls" if books else "roots")
    else:
        require(page["lineage"] == state["lineage"] and page["source_cut_sha256"] == state["cut"] and
                page["legacy_unknown_epochs"] == state["legacy_unknown_epochs"] and
                [{key: book[key] for key in ("epoch", "revision", "terminal")} for book in state["books"]] == page["books"])


def scan(root, qualifier, previous, *, now=None):
    now = time.time() if now is None else now
    source = source_digest(root, qualifier)
    validate(previous, source, now)
    state = copy.deepcopy(previous)
    phase = state["phase"]
    bucket = state["rotation"] if phase == "roots" else None
    book = state["books"][state["control_index"]] if phase == "controls" else None
    if phase in ("context", "closed"):
        command = [str(qualifier), "--economic-baseline-history-context", str(root), state["cut"] or "-"]
    elif phase == "controls":
        command = [str(qualifier), "--economic-baseline-history-control", str(root), book["epoch"], state["cut"]]
    else:
        require(not state["buckets"][bucket]["exhausted"])
        command = [str(qualifier), "--economic-baseline-history-page", str(root), str(bucket),
            state["buckets"][bucket]["cursor"] or "-", state["cut"]]
    try:
        ran = subprocess.run(command, capture_output=True, text=True, timeout=45)
    except subprocess.TimeoutExpired:
        refused = True
    else:
        refused = ran.returncode != 0
    findings = []
    examined = verified = 0
    initialized = True
    if refused:
        code = ("flatfile_baseline_history_context_refused" if phase in ("context", "closed") else
                "flatfile_baseline_history_control_refused" if phase == "controls" else
                "flatfile_baseline_history_page_refused")
        findings.append(finding(code, bucket=bucket, epoch=book["epoch"] if book else None))
    else:
        require(len(ran.stdout.encode()) <= MAX_PROGRESS_BYTES and not ran.stderr)
        page = json.loads(ran.stdout)
        require(type(page) is dict and type(page.get("initialized")) is bool)
        initialized = page["initialized"]
        if not initialized:
            require(page == {"initialized": False} and phase == "context")
            return report(previous, phase, None, 0, 0, False, False, initialized=False), previous
        if phase in ("context", "closed"):
            context_page(page, state, initialize=phase == "context")
        else:
            require(page.get("lineage") == state["lineage"] and page.get("source_cut_sha256") == state["cut"])
            if phase == "controls":
                require(set(page) == {"initialized", "lineage", "source_cut_sha256", "epoch", "revision", "terminal", "reservations"})
                require(page["epoch"] == book["epoch"] and integer(page["revision"], MAX_ROOTS) and
                        page["revision"] == book["revision"] and page["terminal"] == book["terminal"] and
                        integer(page["reservations"], MAX_MEMBERS) and (book["revision"] or page["reservations"] == 0))
                book.update(control_checked=True, reservation_total=page["reservations"])
            else:
                require(set(page) == {"initialized", "lineage", "source_cut_sha256", "bucket", "cursor", "rows",
                    "verified", "bucket_rows", "range_exhausted", "invalid_records", "baseline"})
                selected = state["buckets"][bucket]
                require(integer(page["bucket"], 255) and page["bucket"] == bucket and integer(page["rows"], 1) and
                        integer(page["verified"], page["rows"]) and integer(page["bucket_rows"], 4096) and
                        type(page["range_exhausted"]) is bool)
                cursor = "" if page["cursor"] == pages.ZERO else page["cursor"]
                require(cursor == "" or identity(cursor, bucket))
                require((page["rows"] == 0 and cursor == selected["cursor"] and page["range_exhausted"]) or
                        (page["rows"] == 1 and cursor > selected["cursor"]))
                require(type(page["invalid_records"]) is list and len(page["invalid_records"]) == page["rows"] - page["verified"])
                for operation in page["invalid_records"]:
                    require(operation == cursor and identity(operation, bucket))
                    findings.append(finding("flatfile_baseline_history_record_invalid", bucket=bucket, operation=operation))
                original = page["baseline"]
                if original is not None:
                    require(type(original) is dict and set(original) == {"epoch", "operation", "revision", "reservations", "book_reservations", "terminal"})
                    require(page["rows"] == page["verified"] == 1 and original["operation"] == cursor and identity(original["epoch"]))
                    matching = next((entry for entry in state["books"] if entry["epoch"] == original["epoch"]), None)
                    require(matching is not None and integer(original["revision"], matching["revision"]) and original["revision"] > 0 and
                            integer(original["reservations"], MAX_ROOT_MEMBERS) and integer(original["book_reservations"], MAX_MEMBERS) and
                            type(original["terminal"]) is bool and original["terminal"] == (cursor == matching["terminal"]))
                    require(matching["reservation_total"] is None or original["book_reservations"] == matching["reservation_total"])
                    bits = bitmap(matching)
                    flag = 1 << (original["revision"] - 1)
                    if bits & flag:
                        findings.append(finding("flatfile_baseline_history_duplicate_revision", bucket, matching["epoch"], cursor))
                    matching["bitmap"] = format(bits | flag, "0" + str(len(matching["bitmap"])) + "x")
                    matching["roots"] += 1
                    matching["observed_reservations"] += original["reservations"]
                    matching["terminal_seen"] |= original["terminal"]
                selected.update(cursor=cursor, exhausted=page["range_exhausted"])
                examined, verified = page["rows"], page["verified"]
    if phase == "controls":
        state["control_index"] += 1
        if state["control_index"] == len(state["books"]):
            state["phase"] = "roots"
    if phase == "roots":
        state["total_rows"] += examined
        state["total_verified"] += verified
        if all(row["exhausted"] for row in state["buckets"]):
            findings.extend(close_books(state))
        else:
            state["rotation"] = next((bucket + step) % 256 for step in range(1, 257)
                if not state["buckets"][(bucket + step) % 256]["exhausted"])
    retain(state, findings)
    state["last_page_at"] = now
    validate(state, source, now)
    return report(state, phase, bucket, examined, verified, refused, bool(findings), initialized=initialized), state


def report(state, phase, bucket, examined, verified, refused, current_findings, *, initialized=True):
    return dict(format="flatfile_economic_baseline_history_page_v1", scope="required_initialized_baseline_history",
        initialized=initialized, phase=phase, next_phase=state["phase"], bucket=bucket, next_bucket=state["rotation"],
        source_cut_sha256=state["cut"], required_books=len(state["books"]), controls_checked=sum(book["control_checked"] for book in state["books"]),
        examined_roots=examined, semantically_checked_records=verified, total_roots_observed=state["total_rows"],
        total_baseline_roots=sum(book["roots"] for book in state["books"]), completed_buckets=sum(row["exhausted"] for row in state["buckets"]),
        page_refused=refused, consistent_page=not refused and not current_findings,
        historical_range_complete=state["phase"] == "closed", known_initialized_baseline_books_closed=closed_proof(state) and not refused,
        legacy_unknown_epochs=state["legacy_unknown_epochs"], total_finding_count=state["total_findings"],
        retained_finding_count=len(state["findings"]), findings_truncated=state["findings_truncated"],
        complete=False, consistent_entire_sweep=False, baseline_books_closed=False, orphan_namespace_closed=False,
        native_holdings_compared=False, release_qualified=False)


def run(root, qualifier, path):
    source, now = source_digest(root, qualifier), time.time()
    with progress_io.lock(path, pages.AuditError, "flatfile baseline history progress"):
        try:
            state = validate(progress_io.load(path, MAX_PROGRESS_BYTES, pages.AuditError, "flatfile baseline history progress"), source, now)
        except FileNotFoundError:
            state = new_progress(source, now)
        result, updated = scan(root, qualifier, state)
        if updated is not state:
            progress_io.save(path, updated, MAX_PROGRESS_BYTES, pages.AuditError, "flatfile baseline history progress")
    return result, updated
