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

from economic_restore_evidence import (CanonicalReader, require_integrity, verify_canonical_root,
                                       verify_canonical_baseline)
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
                "economic_lineage_state", "economic_epoch", "item_ownership_ledger", "critical_operation_inbox",
                "economic_baseline_control", "economic_baseline_witness", "economic_baseline_reservation",
                "currency_ledger", "critical_outbox", "economic_sql_lifecycle_installation",
                "economic_account_mapping", "economic_pending_claim_source")
CANDIDATE_SOURCES = (("economic_accounting_operation", "PRIMARY"),
                     ("economic_accounting_account_effect", "PRIMARY"),
                     ("economic_accounting_coin_posting", "PRIMARY"),
                     ("economic_accounting_child", "PRIMARY"),
                     ("economic_accounting_item_reference", "PRIMARY"),
                     ("economic_accounting_source_claim", "uq_economic_source_operation"),
                     ("economic_baseline_witness", "PRIMARY"))
NAMESPACES = ("roots", "controls", "reservations")
COMPOSITE_SOURCES = {"controls": ("economic_baseline_control", ("lineage", "epoch")),
                     "reservations": ("economic_baseline_reservation",
                                      ("lineage", "epoch", "identity_kind", "identity_id"))}


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


def candidate_identities(executor, *, after=None, ceiling=None, limit=1):
    """Merge bounded index seeks; root loss must not hide its retained details.

    Each source contributes at most `limit` distinct IDs. Seeking strictly past
    each ID avoids scanning all of its details or aggregating the entire table.
    An omitted lower bound selects the largest ID across the same sources.
    """
    identities = set()
    for table, index in CANDIDATE_SOURCES:
        previous = after
        for _ in range(1 if after is None else limit):
            where = "" if after is None else (" WHERE operation_id>UNHEX('" + previous +
                "') AND operation_id<=UNHEX('" + ceiling + "')")
            output = executor.sql("SELECT LOWER(HEX(operation_id)) FROM " + table + " FORCE INDEX (" + index + ")" +
                where + " ORDER BY operation_id" + (" DESC" if after is None else "") + " LIMIT 1;")
            values = output.splitlines()
            if not values:
                break
            if (len(values) != 1 or not valid_identity(values[0]) or
                    (after is not None and not previous < values[0] <= ceiling)):
                raise AuditError("invalid canonical audit candidate identities")
            identities.add(values[0])
            previous = values[0]
    return max(identities, default="") if after is None else sorted(identities)[:limit]


def new_progress(source, now):
    return dict(format="economic_sql_canonical_progress_v1", source_digest=source,
                cursor="", ceiling=None, started_at=now, last_page_at=now, last_completed_at=None,
                completed_sweeps=0, sweep_rows=0, sweep_verified=0, sweep_findings=0,
                total_rows=0, total_findings=0, findings=[], findings_truncated=False)


def valid_composite(namespace, value):
    return (namespace in COMPOSITE_SOURCES and isinstance(value, str) and
            bool(re.fullmatch("[0-9a-f]{" + str(64 if namespace == "controls" else 82) + "}", value)))


def new_all_progress(source, now):
    namespaces = {name: new_progress(source, now) for name in NAMESPACES}
    for name in COMPOSITE_SOURCES:
        namespaces[name]["format"] = "economic_sql_canonical_" + name + "_progress_v1"
    return dict(format="economic_sql_canonical_progress_v2", source_digest=source,
                next_namespace="roots", namespaces=namespaces)


def validate_progress(value, source, now, *, namespace="roots"):
    expected = new_progress(source, now)
    if namespace != "roots":
        if namespace not in COMPOSITE_SOURCES:
            raise AuditError("invalid canonical audit progress namespace")
        expected["format"] = "economic_sql_canonical_" + namespace + "_progress_v1"
    valid = valid_identity if namespace == "roots" else lambda value: valid_composite(namespace, value)
    if (type(value) is not dict or set(value) != set(expected) or
            value["format"] != expected["format"] or value["source_digest"] != source or
            not isinstance(source, str) or not re.fullmatch("[0-9a-f]{64}", source)):
        raise AuditError("invalid canonical audit progress source or format")
    cursor, ceiling = value["cursor"], value["ceiling"]
    if (cursor != "" and not valid(cursor) or
            ceiling not in (None, "") and not valid(ceiling) or
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
        key = "operation_id" if namespace == "roots" else "key"
        if (type(row) is not dict or set(row) != {key, "code"} or
                not valid(row[key]) or not isinstance(row["code"], str) or
                not re.fullmatch("restore_economic_[a-z_]+_mismatch", row["code"])):
            raise AuditError("invalid canonical audit progress finding")
    return value


def validate_all_progress(value, source, now):
    if (type(value) is not dict or set(value) != {"format", "source_digest", "next_namespace", "namespaces"} or
            value["format"] != "economic_sql_canonical_progress_v2" or value["source_digest"] != source or
            value["next_namespace"] not in NAMESPACES or type(value["namespaces"]) is not dict or
            set(value["namespaces"]) != set(NAMESPACES)):
        raise AuditError("invalid canonical audit all-namespace progress")
    for name in NAMESPACES:
        validate_progress(value["namespaces"][name], source, now, namespace=name)
    return value


def load_progress(path, source, *, now=None, all_namespaces=False):
    now = time.time() if now is None else now
    try:
        fd = os.open(path, os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0) | getattr(os, "O_NONBLOCK", 0))
    except FileNotFoundError:
        if all_namespaces:
            return validate_all_progress(new_all_progress(source, now), source, now)
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
    if all_namespaces:
        if type(value) is dict and value.get("format") == "economic_sql_canonical_progress_v1":
            root = validate_progress(value, source, now)
            value = new_all_progress(source, now)
            value["namespaces"]["roots"] = root
        return validate_all_progress(value, source, now)
    return validate_progress(value, source, now)


def save_progress(path, value):
    validate = validate_all_progress if value.get("format") == "economic_sql_canonical_progress_v2" else validate_progress
    validate(value, value["source_digest"], time.time())
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
            state["ceiling"] = candidate_identities(executor)
            if state["ceiling"] != "" and not valid_identity(state["ceiling"]):
                raise AuditError("invalid canonical audit page ceiling")
            state.update(cursor="", started_at=now, sweep_rows=0, sweep_verified=0, sweep_findings=0)
        identities = candidate_identities(executor, after=state["cursor"], ceiling=state["ceiling"], limit=page_roots+1)
        previous = state["cursor"]
        if len(identities) > page_roots + 1:
            raise AuditError("canonical audit page exceeds root limit")
        for operation in identities:
            if not valid_identity(operation) or not previous < operation <= state["ceiling"]:
                raise AuditError("invalid canonical audit page identities")
            previous = operation
        findings = []
        baselines, historical_claim_origins, unattached = 0, 0, 0
        reader = CanonicalReader(executor)
        for operation in identities[:page_roots]:
            try:
                root = verify_canonical_root(reader, operation)
                meta, row, _, _, plan = root
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
                if plan is not None and meta[10] == 38:
                    columns = []
                    for name, code in (("command_accepted_at_usec", "baseline_witness"),
                                       ("claim_origin_version", "baseline_claim_origin")):
                        count = executor.sql("SELECT COUNT(*) FROM information_schema.columns "
                            "WHERE table_schema=DATABASE() AND table_name='economic_baseline_witness' "
                            "AND column_name='" + name + "';")
                        if count not in ("0", "1"):
                            reader.mismatch(code)
                        columns.append("w." + name if count == "1" else "NULL")
                    if executor.sql("SELECT EXISTS(SELECT 1 FROM critical_operation_inbox WHERE " +
                        "operation_id=UNHEX('" + operation + "') AND status=1 AND result_code=0 "
                        "AND failure_stage=0 AND committed_at IS NOT NULL);") != "1":
                        reader.mismatch("baseline_witness")
                    version, origins = verify_canonical_baseline(reader, operation, root,
                        admission_column=columns[0], claim_origin_column=columns[1])
                    if origins is not None:
                        expected_sources = [[slot, lineage.hex(), mapping, pid, amount]
                                            for slot, lineage, mapping, pid, amount in origins]
                        actual = reader.arrays("economic_pending_claim_source",
                            ["source_slot", "LOWER(HEX(lineage))", "claim_mapping_id", "beneficiary_pid", "amount"],
                            "source_operation_id=UNHEX('" + operation + "')", "source_slot LIMIT " + str(len(origins)+1))
                        if not same_projection(actual, expected_sources):
                            reader.mismatch("baseline_claim_origin")
                    effects = (("economic_accounting_child", "operation_id"),
                               ("economic_accounting_child", "child_operation_id"),
                               ("economic_accounting_item_reference", "operation_id"),
                               ("currency_ledger", "operation_id"), ("item_ownership_ledger", "operation_id"),
                               ("critical_outbox", "operation_id"))
                    if executor.sql("SELECT " + " OR ".join("EXISTS(SELECT 1 FROM " + table +
                        " WHERE " + column + "=UNHEX('" + operation + "') LIMIT 1)" for table, column in effects) + ";") != "0":
                        reader.mismatch("baseline_zero_effect")
                    baselines += 1
                    historical_claim_origins += int(version is None)
                elif executor.sql("SELECT EXISTS(SELECT 1 FROM economic_baseline_witness WHERE " +
                    "operation_id=UNHEX('" + operation + "') LIMIT 1) OR EXISTS(SELECT 1 FROM " +
                    "economic_baseline_reservation WHERE operation_id=UNHEX('" + operation + "') LIMIT 1);") != "0":
                    reader.mismatch("baseline_witness")
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
                if code == "restore_economic_metadata_mismatch" and int(operation, 16):
                    present = executor.sql("SELECT EXISTS(SELECT 1 FROM economic_accounting_operation "
                        "WHERE operation_id=UNHEX('" + operation + "') LIMIT 1);")
                    if present not in ("0", "1"):
                        raise AuditError("invalid canonical audit root presence")
                    if present == "0":
                        code = "restore_economic_orphan_root_mismatch"
                        unattached += 1
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
            baseline_roots_authenticated=baselines, historical_claim_origin_roots=historical_claim_origins,
            unattached_root_ids=unattached, candidate_source_count=len(CANDIDATE_SOURCES),
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


def composite_values(namespace, key):
    if not valid_composite(namespace, key):
        raise AuditError("invalid canonical audit composite key")
    values = ["UNHEX('" + key[:32] + "')", "UNHEX('" + key[32:64] + "')"]
    if namespace == "reservations":
        values += [str(int(key[64:66], 16)), str(int(key[66:], 16))]
    return values


def composite_range(namespace, key, *, upper=False):
    # Expanded lexicographic ranges permit native PRIMARY range seeks on both
    # supported engines; row constructors do not reliably use every key part.
    columns = COMPOSITE_SOURCES[namespace][1]
    values = composite_values(namespace, key)
    terms = []
    for index, (column, value) in enumerate(zip(columns, values)):
        comparison = "<=" if upper and index == len(columns)-1 else "<" if upper else ">"
        terms.append("(" + " AND ".join([columns[j] + "=" + values[j] for j in range(index)] +
                                         [column + comparison + value]) + ")")
    return "(" + " OR ".join(terms) + ")"


def composite_candidates(executor, namespace, *, after=None, ceiling=None, limit=1):
    if namespace not in COMPOSITE_SOURCES or type(limit) is not int or not 1 <= limit <= MAX_PAGE_ROOTS+1:
        raise AuditError("invalid canonical audit composite page")
    table, columns = COMPOSITE_SOURCES[namespace]
    expressions = ["LOWER(HEX(lineage))", "LOWER(HEX(epoch))"]
    if namespace == "reservations":
        expressions += ["LPAD(LOWER(HEX(identity_kind)),2,'0')", "LPAD(LOWER(HEX(identity_id)),16,'0')"]
    where = ""
    if after is not None:
        if ceiling == "":
            return []
        where = (" WHERE " + (composite_range(namespace, after) + " AND " if after else "") +
                 composite_range(namespace, ceiling, upper=True))
    output = executor.sql("SELECT CONCAT(" + ",".join(expressions) + ") FROM " + table +
        " FORCE INDEX (PRIMARY)" + where + " ORDER BY " +
        ",".join(column + (" DESC" if after is None else "") for column in columns) +
        " LIMIT " + str(1 if after is None else limit) + ";")
    keys = output.splitlines()
    previous = after or ""
    if len(keys) > (1 if after is None else limit):
        raise AuditError("canonical audit composite page exceeds key limit")
    for key in keys:
        if not valid_composite(namespace, key) or key <= previous or (after is not None and key > ceiling):
            raise AuditError("invalid canonical audit composite candidates")
        previous = key
    return (keys[0] if keys else "") if after is None else keys


def verify_namespace_record(reader, namespace, key):
    """Bounded namespace checks supplement per-root original-byte verification.

    These reads neither decode every capsule again per reservation nor assert
    complete book continuity across separate views. Surviving witnesses remain
    candidates in the original root interpreter.
    """
    reader.require_integer_storage()
    table, columns = COMPOSITE_SOURCES[namespace]
    alias = "c" if namespace == "controls" else "p"
    where = " AND ".join(alias + "." + column + "=" + value for column, value in
                         zip(columns, composite_values(namespace, key)))
    if namespace == "controls":
        scope = "w.lineage=c.lineage AND w.epoch=c.epoch"
        successful = (" FROM economic_baseline_witness w FORCE INDEX (uq_economic_baseline_witness_revision) "
            "JOIN economic_accounting_operation o ON o.operation_id=w.operation_id WHERE " + scope +
            " AND o.lineage=w.lineage AND o.epoch=w.epoch AND o.reason=38 AND o.outcome=1 AND o.result_code=0")
        query = ("SELECT EXISTS(SELECT 1 FROM " + table + " c "
            "LEFT JOIN economic_epoch e ON e.lineage=c.lineage AND e.epoch=c.epoch "
            "LEFT JOIN economic_lineage_state l ON l.lineage=c.lineage "
            "LEFT JOIN critical_operation_inbox i ON i.operation_id=c.creating_operation_id WHERE " + where +
            " AND (e.epoch IS NULL OR l.lineage IS NULL OR i.operation_id IS NULL OR i.status NOT IN (0,1) "
            "OR c.lineage=REPEAT(CHAR(0),16) OR c.epoch=REPEAT(CHAR(0),16) "
            "OR c.creating_operation_id=REPEAT(CHAR(0),16) OR c.revision<0 OR OCTET_LENGTH(c.opening_account)<>40 "
            "OR SUBSTRING(c.opening_account,1,16)<>c.lineage OR SUBSTRING(c.opening_account,17,4)<>X'01000900' "
            "OR SUBSTRING(c.opening_account,21,8)=REPEAT(CHAR(0),8) "
            "OR SUBSTRING(c.opening_account,37,4)<>REPEAT(CHAR(0),4) "
            "OR (c.revision=0 AND (c.last_operation_id IS NOT NULL OR EXISTS(SELECT 1 "
            "FROM economic_baseline_witness w FORCE INDEX (uq_economic_baseline_witness_revision) WHERE " + scope +
            " LIMIT 1))) OR (c.revision>0 AND (c.last_operation_id IS NULL OR "
            "c.last_operation_id=REPEAT(CHAR(0),16) OR NOT EXISTS(SELECT 1" + successful +
            " AND w.book_revision=1 LIMIT 1) OR NOT EXISTS(SELECT 1" + successful +
            " AND w.book_revision=c.revision AND w.operation_id=c.last_operation_id LIMIT 1) "
            "OR EXISTS(SELECT 1 FROM economic_baseline_witness w "
            "FORCE INDEX (uq_economic_baseline_witness_revision) WHERE " + scope +
            " AND (w.book_revision=0 OR w.book_revision>c.revision) LIMIT 1)))));")
        code = "baseline_book"
    else:
        query = ("SELECT EXISTS(SELECT 1 FROM " + table + " p "
            "LEFT JOIN economic_baseline_witness w ON w.lineage=p.lineage AND w.epoch=p.epoch "
            "AND w.operation_id=p.operation_id LEFT JOIN economic_baseline_control c "
            "ON c.lineage=p.lineage AND c.epoch=p.epoch LEFT JOIN economic_epoch e "
            "ON e.lineage=p.lineage AND e.epoch=p.epoch LEFT JOIN economic_lineage_state l ON l.lineage=p.lineage "
            "LEFT JOIN economic_accounting_operation o ON o.operation_id=p.operation_id WHERE " + where +
            " AND (w.operation_id IS NULL OR c.lineage IS NULL OR e.epoch IS NULL OR l.lineage IS NULL "
            "OR o.operation_id IS NULL OR o.lineage<>p.lineage OR o.epoch<>p.epoch "
            "OR o.reason<>38 OR o.outcome<>1 OR o.result_code<>0 OR w.book_revision=0 OR w.book_revision>c.revision "
            "OR p.lineage=REPEAT(CHAR(0),16) OR p.epoch=REPEAT(CHAR(0),16) "
            "OR p.operation_id=REPEAT(CHAR(0),16) OR p.identity_kind NOT IN (1,2) OR p.identity_id=0 "
            "OR (p.identity_kind=2 AND p.identity_id=18446744073709551615)));")
        code = "baseline_reservation"
    result = reader.executor.sql(query)
    if result not in ("0", "1"):
        raise AuditError("invalid canonical audit namespace projection")
    if result == "1":
        reader.mismatch(code)


def scan_composite_page(connection, progress, namespace, *, page_roots=MAX_PAGE_ROOTS, now=None):
    now = time.time() if now is None else now
    validate_progress(progress, progress["source_digest"], now, namespace=namespace)
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
        if namespace == "reservations" and executor.sql("SELECT COUNT(*) FROM information_schema.columns "
            "WHERE table_schema=DATABASE() AND table_name='economic_baseline_reservation' AND "
            "((column_name='identity_kind' AND data_type='tinyint' AND column_type LIKE '%unsigned%') OR "
            "(column_name='identity_id' AND data_type='bigint' AND column_type LIKE '%unsigned%'));") != "2":
            # HEX(-1) would alias UINT64_MAX while signed SQL ordering differs.
            # Refuse incompatible key storage before capturing any range.
            raise AuditError("canonical audit composite source requires unsigned integer keys")
        if state["ceiling"] is None:
            state["ceiling"] = composite_candidates(executor, namespace)
            state.update(cursor="", started_at=now, sweep_rows=0, sweep_verified=0, sweep_findings=0)
        keys = composite_candidates(executor, namespace, after=state["cursor"], ceiling=state["ceiling"], limit=page_roots+1)
        reader, findings = CanonicalReader(executor), []
        for key in keys[:page_roots]:
            try:
                verify_namespace_record(reader, namespace, key)
            except RuntimeError as error:
                code = str(error)
                if not re.fullmatch("restore_economic_[a-z_]+_mismatch", code):
                    raise
                findings.append(dict(code=code))
                state["sweep_findings"] += 1
                state["total_findings"] += 1
                if len(state["findings"]) < MAX_FINDINGS:
                    state["findings"].append(dict(key=key, code=code))
                else:
                    state["findings_truncated"] = True
            else:
                state["sweep_verified"] += 1
            state["cursor"] = key
            state["sweep_rows"] += 1
            state["total_rows"] += 1
        exhausted = len(keys) <= page_roots
        state["last_page_at"] = now
        if exhausted:
            state["completed_sweeps"] += 1
            state["last_completed_at"] = now
            state["cursor"], state["ceiling"] = "", None
        validate_progress(state, state["source_digest"], now, namespace=namespace)
        return dict(findings=findings, queries=executor.queries, read_bytes=executor.bytes,
                    seconds=time.monotonic()-started, examined_records=min(len(keys),page_roots),
                    range_exhausted=exhausted, read_only=True, release_qualified=False), state
    finally:
        try:
            connection.rollback()
        finally:
            cursor.close()


def scan_all_page(connection, progress, *, page_roots=MAX_PAGE_ROOTS, now=None):
    """Rotate bounded pages fairly; no operation/key is a commit watermark."""
    now = time.time() if now is None else now
    validate_all_progress(progress, progress["source_digest"], now)
    state = copy.deepcopy(progress)
    namespace = state["next_namespace"]
    if namespace == "roots":
        report, advanced = scan_page(connection, state["namespaces"][namespace], page_roots=page_roots, now=now)
    else:
        report, advanced = scan_composite_page(connection, state["namespaces"][namespace], namespace,
                                              page_roots=page_roots, now=now)
    state["namespaces"][namespace] = advanced
    state["next_namespace"] = NAMESPACES[(NAMESPACES.index(namespace)+1) % len(NAMESPACES)]
    summaries = {}
    for name, part in state["namespaces"].items():
        summaries[name] = {field: part[field] for field in ("completed_sweeps", "sweep_rows", "sweep_findings", "total_rows", "total_findings")}
        summaries[name].update(sweep_age_seconds=now-part["started_at"],
            seconds_since_last_page=now-part["last_page_at"], seconds_since_completed_sweep=None
            if part["last_completed_at"] is None else now-part["last_completed_at"],
            retained_finding_count=len(part["findings"]), findings_truncated=part["findings_truncated"])
    report.update(format="economic_sql_canonical_page_v2", scope="retained_namespaces_page", namespace=namespace,
        next_namespace=state["next_namespace"], namespaces=summaries,
        completed_sweeps=min(part["completed_sweeps"] for part in state["namespaces"].values()),
        retained_finding_count=sum(len(part["findings"]) for part in state["namespaces"].values()),
        findings_truncated=any(part["findings_truncated"] for part in state["namespaces"].values()),
        backlog_lower_bound=int(not report["range_exhausted"]), backlog_exact=False,
        coverage=dict(complete=False, consistent_page=True, consistent_entire_sweep=False,
            canonical_root_projections_only=False, native_holdings_authenticated=False,
            baseline_witnesses_authenticated=False, pending_claim_allocations_authenticated=False,
            orphan_evidence_authenticated=False, complete_command_receipts_authenticated=False),
        read_only=True, release_qualified=False)
    validate_all_progress(state, state["source_digest"], now)
    return report, state


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
    parser.add_argument("--all-namespaces", action="store_true",
                        help="rotate root/control/reservation pages; requires --progress-path; upgrades v1 progress")
    args = parser.parse_args()
    try:
        if not 1 <= args.port <= 65535:
            raise AuditError("invalid SQL port")
        if args.all_namespaces and args.progress_path is None:
            raise AuditError("all-namespace audit requires --progress-path")
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
                        progress = load_progress(args.progress_path, source, all_namespaces=args.all_namespaces)
                        scan = scan_all_page if args.all_namespaces else scan_page
                        report, progress = scan(connection, progress, page_roots=args.page_roots)
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
