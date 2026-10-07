#!/usr/bin/env python3
"""Durable independent flatfile audit pages; partial pages never establish release coverage."""
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


def authority_key(value, bucket, direction):
    if type(value) is not str or re.fullmatch(r"[0-9a-f]+", value) is None:
        return False
    if direction == "mapping":
        return len(value) == 16 and 0 < int(value, 16) <= 256*4096 and int(value, 16) % 256 == bucket
    return (26 <= len(value) <= 124 and len(value) % 2 == 0 and
            hashlib.sha256(bytes.fromhex(value)).digest()[0] == bucket)


def source_digest(root, qualifier):
    result = hashlib.sha256(str(root).encode() + b"\0")
    for path in (qualifier, Path(__file__), Path(progress_io.__file__)):
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                result.update(block)
        result.update(b"\0")
    return result.hexdigest()


def new_progress(source, now, authority_links=False):
    return dict(format="flatfile_economic_authority_progress_v1" if authority_links else
                "flatfile_economic_roots_progress_v1", source_digest=source,
                lineage=None, rotation=0, started_at=now, last_page_at=now,
                total_rows=0, total_verified=0, total_findings=0,
                findings=[], findings_truncated=False,
                buckets=[dict(cursor="", ceiling=None, completed_ranges=0)
                         for _ in range(512 if authority_links else 256)])


def validate(value, source, now, authority_links=False):
    empty = new_progress(source, now, authority_links)
    require(type(value) is dict and set(value) == set(empty))
    require(value["format"] == empty["format"] and
            value["source_digest"] == source and digest(source))
    require(value["lineage"] is None or identity(value["lineage"]))
    require(integer(value["rotation"], len(empty["buckets"])-1))
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
        if authority_links:
            require(type(finding) is dict and set(finding) == {"bucket", "direction", "key", "code"} and
                    integer(finding["bucket"], 255) and finding["direction"] in ("mapping", "native") and
                    (finding["key"] is None or authority_key(finding["key"], finding["bucket"], finding["direction"])) and
                    finding["code"] in ("flatfile_authority_link_invalid", "flatfile_authority_page_refused"))
        else:
            require(type(finding) is dict and set(finding) == {"bucket", "operation_id", "code"} and
                    integer(finding["bucket"], 255) and
                    (finding["operation_id"] is None or identity(finding["operation_id"], finding["bucket"])) and
                    finding["code"] in ("flatfile_retained_record_invalid", "flatfile_root_page_refused"))
    require(value["total_findings"] >= len(value["findings"]))
    require(type(value["buckets"]) is list and len(value["buckets"]) == len(empty["buckets"]))
    for slot, state in enumerate(value["buckets"]):
        bucket, direction = slot % 256, "mapping" if slot < 256 else "native"
        valid = (lambda key: authority_key(key, bucket, direction)) if authority_links else (
            lambda key: identity(key, bucket))
        require(type(state) is dict and set(state) == {"cursor", "ceiling", "completed_ranges"})
        require(integer(state["completed_ranges"]))
        require(state["cursor"] == "" or valid(state["cursor"]))
        require(state["ceiling"] is None or valid(state["ceiling"]))
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


def scan(root, qualifier, previous, *, now=None, authority_links=False):
    now = time.time() if now is None else now
    source = source_digest(root, qualifier)
    validate(previous, source, now, authority_links)
    state = copy.deepcopy(previous)
    slot = state["rotation"]
    bucket, direction = slot % 256, "mapping" if slot < 256 else "native"
    selected = state["buckets"][slot]
    command = ([str(qualifier), "--economic-authority-page", str(root), direction, str(bucket)] if authority_links else
               [str(qualifier), "--economic-evidence-page", str(root), str(bucket)]) + [
               selected["cursor"] or "-", selected["ceiling"] or "-"]
    empty_key = "" if authority_links else ZERO
    valid = (lambda key: authority_key(key, bucket, direction)) if authority_links else (
        lambda key: identity(key, bucket))
    format_name = "flatfile_economic_authority_page_v1" if authority_links else "flatfile_economic_roots_page_v1"
    scope = "authority_crosslink_page" if authority_links else "retained_record_page"
    invalid_field = "invalid_links" if authority_links else "invalid_records"
    def finding(key, refused=False):
        if authority_links:
            return dict(bucket=bucket, direction=direction, key=key,
                        code="flatfile_authority_page_refused" if refused else "flatfile_authority_link_invalid")
        return dict(bucket=bucket, operation_id=key,
                    code="flatfile_root_page_refused" if refused else "flatfile_retained_record_invalid")
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
        findings = [finding(None, True)]
    else:
        require(len(ran.stdout.encode()) <= 4096 and not ran.stderr)
        page = json.loads(ran.stdout)
        require(type(page) is dict and type(page.get("initialized")) is bool)
        initialized = page["initialized"]
        if not initialized:
            require(page == {"initialized": False} and state["lineage"] is None)
            return dict(format=format_name, scope=scope, initialized=False,
                        complete=False, consistent_entire_sweep=False, release_qualified=False), previous
        fields = {"initialized", "lineage", "authority_body_sha256", "bucket", "cursor",
                  "ceiling", "rows", "verified", "bucket_rows", "range_exhausted", invalid_field}
        if authority_links:
            fields.add("direction")
            require(page.get("direction") == direction)
        require(set(page) == fields)
        require(identity(page["lineage"]) and digest(page["authority_body_sha256"]) and
                integer(page["bucket"], 255) and page["bucket"] == bucket and
                (state["lineage"] is None or page["lineage"] == state["lineage"]))
        require(integer(page["rows"], 2) and integer(page["verified"], page["rows"]) and
                integer(page["bucket_rows"], 4096) and type(page["range_exhausted"]) is bool)
        require(page["cursor"] == empty_key or valid(page["cursor"]))
        require(page["ceiling"] == empty_key or valid(page["ceiling"]))
        require(page["cursor"] <= page["ceiling"] and
                (selected["ceiling"] is None or page["ceiling"] == selected["ceiling"]))
        require((page["rows"] == 0 and page["cursor"] == (selected["cursor"] or empty_key)) or
                (page["rows"] > 0 and page["cursor"] > (selected["cursor"] or empty_key)))
        require(type(page[invalid_field]) is list and
                len(page[invalid_field]) == page["rows"] - page["verified"] and
                len(set(page[invalid_field])) == len(page[invalid_field]))
        for key in page[invalid_field]:
            require(valid(key) and (selected["cursor"] or empty_key) < key <= page["cursor"])
            findings.append(finding(key))
        state["lineage"] = page["lineage"]
        examined, verified, exhausted = page["rows"], page["verified"], page["range_exhausted"]
        if exhausted:
            selected.update(cursor="", ceiling=None, completed_ranges=selected["completed_ranges"] + 1)
        else:
            require(page["rows"] > 0 and page["cursor"] != empty_key and page["ceiling"] != empty_key)
            selected.update(cursor=page["cursor"], ceiling=page["ceiling"])
    state["rotation"] = (slot + 1) % len(state["buckets"])
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
    validate(state, source, now, authority_links)
    report = dict(format=format_name, scope=scope, initialized=initialized,
                  bucket=bucket, next_bucket=state["rotation"], examined_roots=examined,
                  semantically_checked_records=verified, page_refused=refused, range_exhausted=exhausted,
                  completed_historical_ranges=min(row["completed_ranges"] for row in state["buckets"]),
                  total_roots_observed=state["total_rows"], total_finding_count=state["total_findings"],
                  retained_finding_count=len(state["findings"]), findings_truncated=state["findings_truncated"],
                  elapsed_seconds=now-state["started_at"], complete=False,
                  consistent_page=not refused, consistent_entire_sweep=False, release_qualified=False,
                  native_holdings_compared=False, baseline_books_closed=False,
                  lifecycle_receipts_closed=False, orphan_namespace_closed=False)
    if authority_links:
        report.update(direction=direction, next_direction="mapping" if state["rotation"] < 256 else "native",
                      next_bucket=state["rotation"] % 256, examined_links=report.pop("examined_roots"),
                      verified_crosslinks=report.pop("semantically_checked_records"),
                      total_links_observed=report.pop("total_roots_observed"), authority_crosslinks_closed=False)
    return report, state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state-root", type=Path, required=True)
    parser.add_argument("--progress", type=Path, required=True)
    parser.add_argument("--qualifier", type=Path, default=ROOT / "bin/tools/qualify_flatfile_restore")
    parser.add_argument("--scope", choices=("retained-roots", "authority-links"), default="retained-roots")
    args = parser.parse_args()
    try:
        require(args.state_root.is_absolute() and args.state_root.is_dir() and args.qualifier.is_file())
        root, qualifier = args.state_root.resolve(), args.qualifier.resolve()
        path = checkpoint_path(args.progress, root)
        source, now = source_digest(root, qualifier), time.time()
        authority_links = args.scope == "authority-links"
        with progress_io.lock(path, AuditError, "flatfile audit progress"):
            try:
                state = validate(progress_io.load(path, MAX_PROGRESS_BYTES, AuditError, "flatfile audit progress"),
                                 source, now, authority_links)
            except FileNotFoundError:
                state = new_progress(source, now, authority_links)
            report, updated = scan(root, qualifier, state, authority_links=authority_links)
            if updated is not state:
                progress_io.save(path, updated, MAX_PROGRESS_BYTES, AuditError, "flatfile audit progress")
        print(json.dumps(report, sort_keys=True, separators=(",", ":")))
        return 1 if updated["total_findings"] else 0
    except (AuditError, OSError, ValueError):
        print("flatfile_economic_audit_refused", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
