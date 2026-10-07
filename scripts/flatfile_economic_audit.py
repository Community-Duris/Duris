#!/usr/bin/env python3
"""Durable retained-root pages; partial pages never establish release coverage."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import time

import economic_audit_progress as progress_io

ROOT = Path(__file__).resolve().parents[1]
MAX_PROGRESS_BYTES = 131072
MAX_FINDINGS = 32
ZERO = "0" * 32


class AuditError(ValueError):
    pass


def require(value):
    if not value:
        raise AuditError("invalid flatfile audit progress or page")


def identity(value, bucket=None):
    return (type(value) is str and re.fullmatch(r"[0-9a-f]{32}", value) is not None and
            value != ZERO and (bucket is None or int(value[:2], 16) == bucket))


def digest(value):
    return type(value) is str and re.fullmatch(r"[0-9a-f]{64}", value) is not None


def integer(value, maximum=2**63-1):
    return type(value) is int and 0 <= value <= maximum


def source_digest(root, qualifier):
    result = hashlib.sha256(str(root).encode() + b"\0")
    for path in (qualifier, Path(__file__), Path(progress_io.__file__)):
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                result.update(block)
        result.update(b"\0")
    return result.hexdigest()


def new_progress(source, now):
    return dict(format="flatfile_economic_roots_progress_v1", source_digest=source,
                lineage=None, rotation=0, started_at=now, last_page_at=now,
                total_rows=0, total_verified=0, total_findings=0,
                findings=[], findings_truncated=False,
                buckets=[dict(cursor="", ceiling=None, completed_ranges=0) for _ in range(256)])


def validate(value, source, now):
    require(type(value) is dict and set(value) == set(new_progress(source, now)))
    require(value["format"] == "flatfile_economic_roots_progress_v1" and
            value["source_digest"] == source and digest(source))
    require(value["lineage"] is None or identity(value["lineage"]))
    require(integer(value["rotation"], 255))
    for name in ("started_at", "last_page_at"):
        field = value[name]
        require(type(field) in (int, float) and math.isfinite(field) and 0 <= field <= now)
    require(value["started_at"] <= value["last_page_at"])
    for name in ("total_rows", "total_verified", "total_findings"):
        require(integer(value[name]))
    require(value["total_verified"] <= value["total_rows"] and
            type(value["findings_truncated"]) is bool and
            type(value["findings"]) is list and len(value["findings"]) <= MAX_FINDINGS)
    for finding in value["findings"]:
        require(type(finding) is dict and set(finding) == {"bucket", "operation_id", "code"} and
                integer(finding["bucket"], 255) and
                (finding["operation_id"] is None or identity(finding["operation_id"], finding["bucket"])) and
                finding["code"] in ("flatfile_retained_record_invalid", "flatfile_root_page_refused"))
    require(value["total_findings"] >= len(value["findings"]))
    require(type(value["buckets"]) is list and len(value["buckets"]) == 256)
    for bucket, state in enumerate(value["buckets"]):
        require(type(state) is dict and set(state) == {"cursor", "ceiling", "completed_ranges"})
        require(integer(state["completed_ranges"]))
        require(state["cursor"] == "" or identity(state["cursor"], bucket))
        require(state["ceiling"] is None or identity(state["ceiling"], bucket))
        require(not state["cursor"] or (state["ceiling"] is not None and
                state["cursor"] <= state["ceiling"]))
    require(value["lineage"] is not None or (value["total_rows"] == 0 and
            all(not row["cursor"] and row["ceiling"] is None and row["completed_ranges"] == 0
                for row in value["buckets"])))
    return value


def checkpoint_path(path, root):
    # Checkpoint and lock writes belong outside native authority, including
    # resolved parent aliases. An explicit fresh path is required for new scope.
    path = Path(path)
    require(path.is_absolute() and path.parent.is_dir() and not path.is_symlink())
    resolved = path.parent.resolve() / path.name
    require(resolved != root and root not in resolved.parents)
    return resolved


def scan(root, qualifier, previous, *, now=None):
    now = time.time() if now is None else now
    source = source_digest(root, qualifier)
    validate(previous, source, now)
    state = copy.deepcopy(previous)
    bucket = state["rotation"]
    selected = state["buckets"][bucket]
    command = [str(qualifier), "--economic-evidence-page", str(root), str(bucket),
               selected["cursor"] or "-", selected["ceiling"] or "-"]
    refused = False
    try:
        ran = subprocess.run(command, capture_output=True, text=True, timeout=45)
    except subprocess.TimeoutExpired:
        refused = True
    else:
        refused = ran.returncode != 0
    findings = []
    examined = verified = 0
    exhausted = False
    initialized = True
    if refused:
        # A refused bucket never advances its cursor or earns a completed range.
        # Persist its sticky exception and rotate so it cannot starve siblings.
        findings = [dict(bucket=bucket, operation_id=None, code="flatfile_root_page_refused")]
    else:
        require(len(ran.stdout.encode()) <= 4096 and not ran.stderr)
        page = json.loads(ran.stdout)
        require(type(page) is dict and type(page.get("initialized")) is bool)
        initialized = page["initialized"]
        if not initialized:
            require(page == {"initialized": False} and state["lineage"] is None)
            return dict(format="flatfile_economic_roots_page_v1", scope="retained_record_page", initialized=False,
                        complete=False, consistent_entire_sweep=False, release_qualified=False), previous
        require(set(page) == {"initialized", "lineage", "authority_body_sha256", "bucket", "cursor",
                             "ceiling", "rows", "verified", "bucket_rows", "range_exhausted", "invalid_records"})
        require(identity(page["lineage"]) and digest(page["authority_body_sha256"]) and
                integer(page["bucket"], 255) and page["bucket"] == bucket and
                (state["lineage"] is None or page["lineage"] == state["lineage"]))
        require(integer(page["rows"], 2) and integer(page["verified"], page["rows"]) and
                integer(page["bucket_rows"], 4096) and type(page["range_exhausted"]) is bool)
        require(page["cursor"] == ZERO or identity(page["cursor"], bucket))
        require(page["ceiling"] == ZERO or identity(page["ceiling"], bucket))
        require(page["cursor"] <= page["ceiling"] and
                (selected["ceiling"] is None or page["ceiling"] == selected["ceiling"]))
        require((page["rows"] == 0 and page["cursor"] == (selected["cursor"] or ZERO)) or
                (page["rows"] > 0 and page["cursor"] > (selected["cursor"] or ZERO)))
        require(type(page["invalid_records"]) is list and
                len(page["invalid_records"]) == page["rows"] - page["verified"] and
                len(set(page["invalid_records"])) == len(page["invalid_records"]))
        for operation in page["invalid_records"]:
            require(identity(operation, bucket) and (selected["cursor"] or ZERO) < operation <= page["cursor"])
            findings.append(dict(bucket=bucket, operation_id=operation, code="flatfile_retained_record_invalid"))
        state["lineage"] = page["lineage"]
        examined, verified, exhausted = page["rows"], page["verified"], page["range_exhausted"]
        if exhausted:
            selected.update(cursor="", ceiling=None, completed_ranges=selected["completed_ranges"] + 1)
        else:
            require(page["rows"] > 0 and page["cursor"] != ZERO and page["ceiling"] != ZERO)
            selected.update(cursor=page["cursor"], ceiling=page["ceiling"])
    state["rotation"] = (bucket + 1) % 256
    state["last_page_at"] = now
    state["total_rows"] += examined
    state["total_verified"] += verified
    state["total_findings"] += len(findings)
    for finding in findings:
        if finding in state["findings"]:
            continue
        if len(state["findings"]) == MAX_FINDINGS:
            state["findings_truncated"] = True
        else:
            state["findings"].append(finding)
    validate(state, source, now)
    report = dict(format="flatfile_economic_roots_page_v1", scope="retained_record_page", initialized=initialized,
                  bucket=bucket, next_bucket=state["rotation"], examined_roots=examined,
                  semantically_checked_records=verified, page_refused=refused, range_exhausted=exhausted,
                  completed_historical_ranges=min(row["completed_ranges"] for row in state["buckets"]),
                  total_roots_observed=state["total_rows"], total_finding_count=state["total_findings"],
                  retained_finding_count=len(state["findings"]), findings_truncated=state["findings_truncated"],
                  elapsed_seconds=now-state["started_at"], complete=False,
                  consistent_page=not refused, consistent_entire_sweep=False, release_qualified=False,
                  native_holdings_compared=False, baseline_books_closed=False,
                  lifecycle_receipts_closed=False, orphan_namespace_closed=False)
    return report, state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state-root", type=Path, required=True)
    parser.add_argument("--progress", type=Path, required=True)
    parser.add_argument("--qualifier", type=Path, default=ROOT / "bin/tools/qualify_flatfile_restore")
    args = parser.parse_args()
    try:
        require(args.state_root.is_absolute() and args.state_root.is_dir() and args.qualifier.is_file())
        root, qualifier = args.state_root.resolve(), args.qualifier.resolve()
        path = checkpoint_path(args.progress, root)
        source, now = source_digest(root, qualifier), time.time()
        with progress_io.lock(path, AuditError, "flatfile audit progress"):
            try:
                state = validate(progress_io.load(path, MAX_PROGRESS_BYTES, AuditError, "flatfile audit progress"), source, now)
            except FileNotFoundError:
                state = new_progress(source, now)
            report, updated = scan(root, qualifier, state)
            if updated is not state:
                progress_io.save(path, updated, MAX_PROGRESS_BYTES, AuditError, "flatfile audit progress")
        print(json.dumps(report, sort_keys=True, separators=(",", ":")))
        return 1 if updated["total_findings"] else 0
    except (AuditError, OSError, ValueError):
        print("flatfile_economic_audit_refused", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
