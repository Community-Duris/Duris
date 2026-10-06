#!/usr/bin/env python3
"""Authenticate retained EAI1/EAP1 roots and SQL details without changing data.

This database-wide check supplements the partial snapshot exporter. It does not
authenticate complete command receipts, prove writer coverage, or qualify a
release. The independent restore decoder performs the original-byte comparison.
"""

from __future__ import annotations

import argparse
import copy
from contextlib import contextmanager
import hashlib
import json
import math
import os
from pathlib import Path
import re
import stat
import sys
import tempfile
import time

from economic_restore_evidence import CanonicalReader, require_integrity, verify_canonical_root
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, same_projection


class AuditError(ValueError):
    pass


class PageBudgetError(RuntimeError):
    """A scheduling refusal must bypass canonical value-error classification."""


MAX_PAGE_ROOTS = 2
MAX_PAGE_QUERIES = 1024
MAX_PROGRESS_BYTES = 32768
MAX_FINDINGS = 32
PAGE_SECONDS = 30
ROOT_SOURCES = ("economic_accounting_operation", "economic_accounting_account_effect",
                "economic_accounting_coin_posting", "economic_accounting_child",
                "economic_accounting_item_reference", "economic_accounting_source_claim",
                "economic_lineage_state", "economic_epoch", "item_ownership_ledger", "critical_operation_inbox")


class CursorExecutor:
    """Adapt one existing read view to the independent SELECT-only verifier."""

    def __init__(self, cursor, *, row_limit=None, query_limit=None, total_bytes=None, deadline=None):
        self.cursor = cursor
        self.queries = 0
        self.bytes = 0
        self.row_limit = MAX_ROWS if row_limit is None else row_limit
        self.query_limit = query_limit
        self.total_bytes = total_bytes
        self.deadline = deadline

    def sql(self, query):
        if not query.startswith("SELECT "):
            raise AuditError("canonical audit requires SELECT only")
        if ((self.query_limit is not None and self.queries >= self.query_limit) or
                (self.deadline is not None and time.monotonic() >= self.deadline)):
            raise PageBudgetError("canonical audit page budget exhausted")
        # The verifier owns fixed single-column SELECTs. Bound the server result
        # as well as the streaming client; closing an unbuffered cursor otherwise
        # drains every remaining row after a limit refusal.
        self.cursor.execute("SELECT * FROM (" + query.rstrip("; ") +
                            ") AS canonical_audit_bounded LIMIT " + str(self.row_limit + 1))
        self.queries += 1
        rows, size = [], 0
        # fetchmany keeps a corrupted detail collection bounded even when its
        # SQL projection exceeds the original plan's declared native limit.
        while True:
            batch = self.cursor.fetchmany(256)
            if not batch:
                break
            for row in batch:
                if len(row) != 1:
                    raise AuditError("invalid canonical audit projection")
                value = next(iter(row.values()))
                if not isinstance(value, (str, int)):
                    raise AuditError("invalid canonical audit scalar")
                text = str(value)
                size += len(text.encode("utf-8")) + 1
                self.bytes += len(text.encode("utf-8")) + 1
                if self.total_bytes is not None and self.bytes > self.total_bytes:
                    raise PageBudgetError("canonical audit page byte budget exhausted")
                if len(rows) >= self.row_limit or size > MAX_INPUT_BYTES:
                    raise AuditError("canonical audit projection exceeds input limit")
                rows.append(text)
        return "\n".join(rows)


def valid_identity(value):
    # A cursor can enumerate a corrupt zero key; the original decoder refuses
    # it as economic identity. Scheduling must still advance past that finding.
    return isinstance(value, str) and bool(re.fullmatch("[0-9a-f]{32}", value))


def new_progress(source, now):
    return dict(format="economic_sql_canonical_progress_v1", source_digest=source,
                cursor="", ceiling=None, started_at=now, last_page_at=now, last_completed_at=None,
                completed_sweeps=0, sweep_rows=0, sweep_verified=0, sweep_findings=0,
                total_rows=0, total_findings=0, findings=[], findings_truncated=False)


def validate_progress(value, source, now):
    expected = new_progress(source, now)
    if (type(value) is not dict or set(value) != set(expected) or
            value["format"] != expected["format"] or value["source_digest"] != source or
            not isinstance(source, str) or not re.fullmatch("[0-9a-f]{64}", source)):
        raise AuditError("invalid canonical audit progress source or format")
    cursor, ceiling = value["cursor"], value["ceiling"]
    if (cursor != "" and not valid_identity(cursor) or
            ceiling not in (None, "") and not valid_identity(ceiling) or
            cursor and (ceiling is None or cursor > ceiling)):
        raise AuditError("invalid canonical audit progress range")
    for name in ("started_at", "last_page_at", "last_completed_at"):
        field = value[name]
        if field is None and name == "last_completed_at":
            continue
        if type(field) not in (int, float) or not math.isfinite(field) or field < 0 or field > now:
            raise AuditError("invalid canonical audit progress age")
    for name in ("completed_sweeps", "sweep_rows", "sweep_verified", "sweep_findings", "total_rows", "total_findings"):
        if type(value[name]) is not int or not 0 <= value[name] < 2**63:
            raise AuditError("invalid canonical audit progress counter")
    if (value["sweep_verified"] + value["sweep_findings"] != value["sweep_rows"] or
            value["sweep_rows"] > value["total_rows"] or value["sweep_findings"] > value["total_findings"] or
            type(value["findings_truncated"]) is not bool or type(value["findings"]) is not list or
            len(value["findings"]) > MAX_FINDINGS):
        raise AuditError("invalid canonical audit progress coverage")
    for row in value["findings"]:
        if (type(row) is not dict or set(row) != {"operation_id", "code"} or
                not valid_identity(row["operation_id"]) or not isinstance(row["code"], str) or
                not re.fullmatch("restore_economic_[a-z_]+_mismatch", row["code"])):
            raise AuditError("invalid canonical audit progress finding")
    return value


def load_progress(path, source, *, now=None):
    now = time.time() if now is None else now
    try:
        fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0) | getattr(os, "O_NONBLOCK", 0))
    except FileNotFoundError:
        return validate_progress(new_progress(source, now), source, now)
    with os.fdopen(fd, "rb") as stream:
        info = os.fstat(stream.fileno())
        if (not stat.S_ISREG(info.st_mode) or Path(path).is_symlink() or info.st_size > MAX_PROGRESS_BYTES or
                (os.name == "posix" and (info.st_uid != os.getuid() or info.st_mode & 0o077))):
            raise AuditError("canonical audit progress requires a protected regular file")
        data = stream.read(MAX_PROGRESS_BYTES + 1)
    def pairs(rows):
        result = {}
        for key, value in rows:
            if key in result:
                raise AuditError("duplicate canonical audit progress field")
            result[key] = value
        return result
    if len(data) > MAX_PROGRESS_BYTES:
        raise AuditError("canonical audit progress exceeds byte limit")
    try:
        value = json.loads(data, object_pairs_hook=pairs)
    except ValueError as error:
        raise AuditError("invalid canonical audit progress JSON") from error
    return validate_progress(value, source, now)


def save_progress(path, value):
    validate_progress(value, value["source_digest"], time.time())
    data = (json.dumps(value, allow_nan=False, sort_keys=True, separators=(",", ":")) + "\n").encode()
    if len(data) > MAX_PROGRESS_BYTES:
        raise AuditError("canonical audit progress exceeds byte limit")
    path = Path(path)
    fd, temporary = tempfile.mkstemp(prefix="." + path.name + "-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
        if os.name == "posix":
            directory = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


@contextmanager
def progress_lock(path):
    """CLI progress is single-owner; OS locks release on process interruption."""
    if os.name != "posix":
        raise AuditError("durable canonical audit CLI progress requires POSIX file locking")
    import fcntl
    fd = os.open(str(path) + ".lock", os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_uid != os.getuid() or info.st_mode & 0o077:
            raise AuditError("canonical audit progress lock requires a protected regular file")
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError as error:
            raise AuditError("canonical audit progress is already in use") from error
        yield
    finally:
        os.close(fd)


def scan_page(connection, progress, *, page_roots=MAX_PAGE_ROOTS, now=None):
    """One capped historical page; mixed read views never establish all-clear."""
    now = time.time() if now is None else now
    validate_progress(progress, progress["source_digest"], now)
    if type(page_roots) is not int or not 1 <= page_roots <= MAX_PAGE_ROOTS:
        raise AuditError("invalid canonical audit page root limit")
    state = copy.deepcopy(progress)
    started = time.monotonic()
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        executor = CursorExecutor(cursor, row_limit=8192, query_limit=MAX_PAGE_QUERIES,
                                  total_bytes=MAX_INPUT_BYTES, deadline=time.monotonic() + PAGE_SECONDS)
        tables = ",".join("'" + table + "'" for table in ROOT_SOURCES)
        if executor.sql("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
                        "AND ENGINE='InnoDB' AND table_name IN (" + tables + ");") != str(len(ROOT_SOURCES)):
            raise AuditError("canonical audit page source is missing or not InnoDB")
        if state["ceiling"] is None:
            state["ceiling"] = executor.sql("SELECT LOWER(HEX(operation_id)) FROM economic_accounting_operation "
                                           "FORCE INDEX (PRIMARY) ORDER BY operation_id DESC LIMIT 1;")
            if state["ceiling"] != "" and not valid_identity(state["ceiling"]):
                raise AuditError("invalid canonical audit page ceiling")
            state.update(cursor="", started_at=now, sweep_rows=0, sweep_verified=0, sweep_findings=0)
        output = executor.sql("SELECT LOWER(HEX(operation_id)) FROM economic_accounting_operation "
            "FORCE INDEX (PRIMARY) WHERE operation_id>UNHEX('" + state["cursor"] + "') AND operation_id<=UNHEX('" + state["ceiling"] +
            "') ORDER BY operation_id LIMIT " + str(page_roots + 1) + ";")
        identities = output.splitlines()
        previous = state["cursor"]
        if len(identities) > page_roots + 1:
            raise AuditError("canonical audit page exceeds root limit")
        for operation in identities:
            if not valid_identity(operation) or not previous < operation <= state["ceiling"]:
                raise AuditError("invalid canonical audit page identities")
            previous = operation
        findings = []
        reader = CanonicalReader(executor)
        for operation in identities[:page_roots]:
            try:
                meta, row, _, _, plan = verify_canonical_root(reader, operation)
                where = "o.operation_id=UNHEX('" + operation + "')"
                if executor.sql("SELECT EXISTS(SELECT 1 FROM economic_accounting_operation o "
                    "LEFT JOIN economic_lineage_state l ON l.lineage=o.lineage "
                    "LEFT JOIN economic_epoch e ON e.lineage=o.lineage AND e.epoch=o.epoch WHERE " + where +
                    " AND (l.lineage IS NULL OR e.epoch IS NULL));") != "0":
                    reader.mismatch("lifecycle_namespace")
                expected = [] if plan is None or meta[-1] is None else [[meta[0].hex(), meta[-1].hex(), operation, 1]]
                claims = reader.arrays("economic_accounting_source_claim", ["LOWER(HEX(lineage))",
                    "LOWER(HEX(source_event))", "LOWER(HEX(operation_id))", "outcome"],
                    "operation_id=UNHEX('" + operation + "')", "lineage,source_event LIMIT 2")
                if not same_projection(claims, expected):
                    reader.mismatch("source_claim")
                if expected and executor.sql("SELECT COUNT(*) FROM (SELECT 1 FROM economic_accounting_source_claim "
                    "WHERE lineage=UNHEX('" + meta[0].hex() + "') AND source_event=UNHEX('" + meta[-1].hex() +
                    "') LIMIT 2) canonical_source_identity;") != "1":
                    reader.mismatch("source_claim")
                if plan is None:
                    unwanted = " OR ".join("EXISTS(SELECT 1 FROM " + table +
                        " WHERE operation_id=UNHEX('" + operation + "') LIMIT 1)" for table in ROOT_SOURCES[1:5])
                    if executor.sql("SELECT " + unwanted + ";") != "0":
                        reader.mismatch("rejected_detail")
            except (RuntimeError, AuditError) as error:
                if isinstance(error, AuditError):
                    if str(error) != "canonical audit projection exceeds input limit":
                        raise
                    code = "restore_economic_canonical_projection_mismatch"
                else:
                    code = str(error)
                if not re.fullmatch("restore_economic_[a-z_]+_mismatch", code):
                    raise
                finding = dict(operation_id=operation, code=code)
                findings.append(dict(code=code))  # Routine stdout stays aggregate/ID-free.
                state["sweep_findings"] += 1
                state["total_findings"] += 1
                if len(state["findings"]) < MAX_FINDINGS:
                    state["findings"].append(finding)
                else:
                    state["findings_truncated"] = True
            else:
                state["sweep_verified"] += 1
            state["cursor"] = operation
            state["sweep_rows"] += 1
            state["total_rows"] += 1
        exhausted = len(identities) <= page_roots
        state["last_page_at"] = now
        if exhausted:
            state["completed_sweeps"] += 1
            state["last_completed_at"] = now
            state["cursor"], state["ceiling"] = "", None
        validate_progress(state, state["source_digest"], now)
        return dict(format="economic_sql_canonical_page_v1", scope="retained_root_page",
            examined_roots=min(len(identities), page_roots), findings=findings, queries=executor.queries,
            read_bytes=executor.bytes, seconds=time.monotonic()-started,
            range_exhausted=exhausted, completed_sweeps=state["completed_sweeps"],
            sweep_rows=state["sweep_rows"], sweep_findings=state["sweep_findings"],
            retained_finding_count=len(state["findings"]), findings_truncated=state["findings_truncated"],
            backlog_lower_bound=int(not exhausted), backlog_exact=False,
            sweep_age_seconds=now-state["started_at"],
            seconds_since_completed_sweep=None if state["last_completed_at"] is None else now-state["last_completed_at"],
            coverage=dict(complete=False, consistent_page=True, consistent_entire_sweep=False,
                          canonical_root_projections_only=True, native_holdings_authenticated=False,
                          baseline_witnesses_authenticated=False, pending_claim_allocations_authenticated=False,
                          orphan_evidence_authenticated=False, complete_command_receipts_authenticated=False),
            read_only=True, release_qualified=False), state
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()


def capture(connection):
    """Check all retained books in one read-only transaction; always roll back."""
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        executor = CursorExecutor(cursor)
        sources = ("economic_accounting_operation", "economic_accounting_account_effect",
                   "economic_accounting_coin_posting", "economic_accounting_child",
                   "economic_accounting_item_reference", "economic_accounting_source_claim", "economic_baseline_control",
                   "economic_baseline_witness", "economic_baseline_reservation",
                   "economic_lineage_state", "economic_epoch", "critical_operation_inbox",
                   "item_ownership_ledger", "currency_ledger", "critical_outbox", "quest_mobile_native",
                   "economic_pending_claim_source", "economic_pending_claim_consumption", "economic_account_mapping")
        tables = ",".join("'" + table + "'" for table in sources)
        engines = int(executor.sql("SELECT COUNT(*) FROM information_schema.tables "
            "WHERE table_schema=DATABASE() AND ENGINE='InnoDB' AND table_name IN (" + tables + ");"))
        if engines != len(sources):
            raise AuditError("canonical audit source is missing or not InnoDB")
        count = int(executor.sql("SELECT COUNT(*) FROM economic_accounting_operation;"))
        if not 0 <= count <= MAX_ROWS:
            raise AuditError("canonical audit root count exceeds input limit")
        mobiles = int(executor.sql("SELECT COUNT(*) FROM quest_mobile_native;"))
        if not 0 <= mobiles <= MAX_ROWS:
            raise AuditError("canonical audit native mobile count exceeds input limit")
        size = int(executor.sql("SELECT CAST((SELECT COALESCE(SUM(COALESCE(OCTET_LENGTH(canonical_intent),0)+"
            "COALESCE(OCTET_LENGTH(canonical_plan),0)),0) FROM economic_accounting_operation)+"
            "(SELECT COALESCE(SUM(OCTET_LENGTH(canonical_witness)),0) FROM economic_baseline_witness)+"
            "(SELECT COALESCE(SUM(OCTET_LENGTH(canonical_image)),0) FROM quest_mobile_native) "
            "AS UNSIGNED);"))
        if not 0 <= size <= MAX_INPUT_BYTES:
            raise AuditError("canonical audit capsules exceed input limit")
        # A rejected root has no compiled plan and therefore owns no details.
        # The restore consumer performs additional surrounding checks; this
        # standalone reader must also reject unattached ordinary projections.
        details = ("economic_accounting_account_effect", "economic_accounting_coin_posting",
                   "economic_accounting_child", "economic_accounting_item_reference")
        unwanted = " OR ".join("EXISTS(SELECT 1 FROM " + table + " d LEFT JOIN "
            "economic_accounting_operation o ON o.operation_id=d.operation_id "
            "WHERE o.operation_id IS NULL OR o.outcome<>1 OR o.result_code<>0)" for table in details)
        if executor.sql("SELECT " + unwanted + ";") != "0":
            raise AuditError("canonical audit orphan_or_rejected_detail")
        claims = int(executor.sql("SELECT COUNT(*) FROM economic_accounting_source_claim;"))
        if not 0 <= claims <= MAX_ROWS:
            raise AuditError("canonical audit source_claim count exceeds input limit")
        # The original capsules authenticate each root's source identity. Claims
        # must cover every committed source and name that exact successful root,
        # even for inactive/foreign books or evidence imported with lost checks.
        claim_checks = (
            "EXISTS(SELECT 1 FROM economic_accounting_operation o LEFT JOIN "
            "economic_accounting_source_claim c ON c.operation_id=o.operation_id "
            "AND c.lineage=o.lineage AND c.source_event=o.source_event AND c.outcome=o.outcome "
            "WHERE o.outcome=1 AND o.result_code=0 AND o.source_event IS NOT NULL AND c.operation_id IS NULL)",
            "EXISTS(SELECT 1 FROM economic_accounting_source_claim c LEFT JOIN "
            "economic_accounting_operation o ON o.operation_id=c.operation_id "
            "WHERE o.operation_id IS NULL OR o.source_event IS NULL OR c.outcome<>1 "
            "OR o.outcome<>1 OR o.result_code<>0 OR NOT (o.lineage <=> c.lineage) "
            "OR NOT (o.source_event <=> c.source_event))",
            "EXISTS(SELECT 1 FROM economic_accounting_source_claim GROUP BY operation_id HAVING COUNT(*)<>1)",
            "EXISTS(SELECT 1 FROM economic_accounting_source_claim GROUP BY lineage,source_event HAVING COUNT(*)<>1)",
        )
        if executor.sql("SELECT " + " OR ".join(claim_checks) + ";") != "0":
            raise AuditError("canonical audit source_claim mismatch")
        try:
            require_integrity(executor)
        except RuntimeError as error:
            # Verifier errors are fixed diagnostic codes, never capsules or IDs.
            raise AuditError(str(error)) from error
        return {"format": "economic_sql_canonical_audit_v1", "scope": "database",
                "retained_roots": count, "retained_native_mobiles": mobiles, "queries": executor.queries,
                "canonical_bytes": size,
                "canonical_roots_and_details": "verified", "read_only": True,
                "retained_pending_claim_allocations": "verified",
                "complete_command_receipts_authenticated": False,
                "source_capture_qualified": False, "release_qualified": False}
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=3306)
    parser.add_argument("--socket", help="explicit local SQL Unix socket")
    parser.add_argument("--user", required=True)
    parser.add_argument("--database", required=True)
    parser.add_argument("--password-env", default="DB_PASSWORD")
    parser.add_argument("--progress-path", type=Path, help="protected local progress file; audit one resumable SQL root page")
    parser.add_argument("--page-roots", type=int, default=MAX_PAGE_ROOTS, help="roots per progress page, 1..2")
    args = parser.parse_args()
    try:
        if not 1 <= args.port <= 65535:
            raise AuditError("invalid SQL port")
        password = os.environ[args.password_env]
        import pymysql
        try:
            connection = pymysql.connect(host=args.host, port=args.port, unix_socket=args.socket,
                user=args.user, password=password, database=args.database, charset="utf8mb4",
                autocommit=True, cursorclass=pymysql.cursors.SSDictCursor,
                connect_timeout=5, read_timeout=30, write_timeout=5)
            try:
                if args.progress_path is None:
                    report = capture(connection)
                else:
                    # This binds a scheduling cursor to the explicit connection
                    # target, not a trusted database incarnation or authority cut.
                    source = hashlib.sha256(json.dumps([args.host, args.port, args.socket, args.database],
                                                       separators=(",", ":")).encode()).hexdigest()
                    with progress_lock(args.progress_path):
                        progress = load_progress(args.progress_path, source)
                        report, progress = scan_page(connection, progress, page_roots=args.page_roots)
                        save_progress(args.progress_path, progress)
            finally:
                connection.close()
        except pymysql.MySQLError as error:
            raise AuditError(f"SQL read failed with code {error.args[0]}") from error
        print(json.dumps(report, sort_keys=True, separators=(",", ":")))
        return 1 if report.get("retained_finding_count", 0) or report.get("findings_truncated") else 0
    except (AuditError, PageBudgetError, OSError, ValueError, KeyError, TypeError, ImportError) as error:
        print(f"SQL canonical audit refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
