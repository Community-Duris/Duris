#!/usr/bin/env python3
"""Authenticate retained EAI1/EAP1 roots and SQL details without changing data.

This database-wide check supplements the partial snapshot exporter. It does not
authenticate complete command receipts, prove writer coverage, or qualify a
release. The independent restore decoder performs the original-byte comparison.
"""

from __future__ import annotations

import argparse
import json
import os
import sys

from economic_restore_evidence import require_integrity
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS


class AuditError(ValueError):
    pass


class CursorExecutor:
    """Adapt one existing read view to the independent SELECT-only verifier."""

    def __init__(self, cursor):
        self.cursor = cursor
        self.queries = 0

    def sql(self, query):
        if not query.startswith("SELECT "):
            raise AuditError("canonical audit requires SELECT only")
        # The verifier owns fixed single-column SELECTs. Bound the server result
        # as well as the streaming client; closing an unbuffered cursor otherwise
        # drains every remaining row after a limit refusal.
        self.cursor.execute("SELECT * FROM (" + query.rstrip("; ") +
                            ") AS canonical_audit_bounded LIMIT " + str(MAX_ROWS + 1))
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
                if len(rows) >= MAX_ROWS or size > MAX_INPUT_BYTES:
                    raise AuditError("canonical audit projection exceeds input limit")
                rows.append(text)
        return "\n".join(rows)


def capture(connection):
    """Check all retained books in one read-only transaction; always roll back."""
    cursor = connection.cursor()
    try:
        cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
        cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
        executor = CursorExecutor(cursor)
        sources = ("economic_accounting_operation", "economic_accounting_account_effect",
                   "economic_accounting_coin_posting", "economic_accounting_child",
                   "economic_accounting_item_reference", "economic_baseline_control",
                   "economic_baseline_witness", "economic_baseline_reservation",
                   "economic_lineage_state", "economic_epoch", "critical_operation_inbox",
                   "item_ownership_ledger", "currency_ledger", "critical_outbox")
        tables = ",".join("'" + table + "'" for table in sources)
        engines = int(executor.sql("SELECT COUNT(*) FROM information_schema.tables "
            "WHERE table_schema=DATABASE() AND ENGINE='InnoDB' AND table_name IN (" + tables + ");"))
        if engines != len(sources):
            raise AuditError("canonical audit source is missing or not InnoDB")
        count = int(executor.sql("SELECT COUNT(*) FROM economic_accounting_operation;"))
        if not 0 <= count <= MAX_ROWS:
            raise AuditError("canonical audit root count exceeds input limit")
        size = int(executor.sql("SELECT CAST((SELECT COALESCE(SUM(COALESCE(OCTET_LENGTH(canonical_intent),0)+"
            "COALESCE(OCTET_LENGTH(canonical_plan),0)),0) FROM economic_accounting_operation)+"
            "(SELECT COALESCE(SUM(OCTET_LENGTH(canonical_witness)),0) FROM economic_baseline_witness) "
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
        try:
            require_integrity(executor)
        except RuntimeError as error:
            # Verifier errors are fixed diagnostic codes, never capsules or IDs.
            raise AuditError(str(error)) from error
        return {"format": "economic_sql_canonical_audit_v1", "scope": "database",
                "retained_roots": count, "queries": executor.queries,
                "canonical_bytes": size,
                "canonical_roots_and_details": "verified", "read_only": True,
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
                report = capture(connection)
            finally:
                connection.close()
        except pymysql.MySQLError as error:
            raise AuditError(f"SQL read failed with code {error.args[0]}") from error
        print(json.dumps(report, sort_keys=True, separators=(",", ":")))
        return 0
    except (AuditError, OSError, ValueError, KeyError, TypeError, ImportError) as error:
        print(f"SQL canonical audit refused: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
