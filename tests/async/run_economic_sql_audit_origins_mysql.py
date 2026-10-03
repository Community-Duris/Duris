#!/usr/bin/env python3
"""Exercise read-only EAB1 extraction on an explicitly disposable loopback DB."""

import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import uuid
import json

import pymysql

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from economic_sql_audit_origins import OriginError, capture  # noqa: E402
from test_economic_sql_audit_origins import EPOCH, LINEAGE, OPENING, OP, witness  # noqa: E402

if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET")):
    raise SystemExit("explicit disposable loopback database required")

settings = {"host": "127.0.0.1", "port": int(os.environ.get("DB_PORT", "3306")),
            "user": os.environ["DB_USER"], "password": os.environ["DB_PASSWORD"],
            "autocommit": True, "cursorclass": pymysql.cursors.DictCursor,
            "connect_timeout": 5, "read_timeout": 20, "write_timeout": 5}
schema = "economic_schema_test_auditor_" + uuid.uuid4().hex
reader = "audit_" + uuid.uuid4().hex[:12]
assert re.fullmatch(r"economic_schema_test_auditor_[0-9a-f]{32}", schema)
assert re.fullmatch(r"audit_[0-9a-f]{12}", reader)
admin = pymysql.connect(**settings)
try:
    with admin.cursor() as cursor:
        cursor.execute(f"CREATE DATABASE `{schema}`")
        cursor.execute(f"CREATE USER '{reader}'@'127.0.0.1' IDENTIFIED BY 'disposable-audit-only'")
        cursor.execute(f"GRANT SELECT ON `{schema}`.* TO '{reader}'@'127.0.0.1'")
    setup = pymysql.connect(**(settings | {"database": schema}))
    try:
        with setup.cursor() as cursor:
            cursor.execute("CREATE TABLE economic_baseline_control (lineage BINARY(16),"
                           "epoch BINARY(16),opening_account VARBINARY(40),revision BIGINT UNSIGNED,"
                           "last_operation_id BINARY(16),PRIMARY KEY(lineage,epoch)) ENGINE=InnoDB")
            cursor.execute("CREATE TABLE economic_accounting_operation (operation_id BINARY(16) "
                           "PRIMARY KEY,reason SMALLINT UNSIGNED,outcome TINYINT UNSIGNED,"
                           "result_code INT UNSIGNED) ENGINE=InnoDB")
            cursor.execute("CREATE TABLE critical_operation_inbox (operation_id BINARY(16) "
                           "PRIMARY KEY,status TINYINT UNSIGNED,result_code INT UNSIGNED,"
                           "failure_stage INT NOT NULL DEFAULT 0,"
                           "committed_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB")
            cursor.execute("CREATE TABLE economic_baseline_witness (operation_id BINARY(16) "
                           "PRIMARY KEY,lineage BINARY(16),epoch BINARY(16),book_revision BIGINT UNSIGNED,"
                           "holding_count SMALLINT UNSIGNED,item_count SMALLINT UNSIGNED,"
                           "witness_digest BINARY(32),canonical_witness MEDIUMBLOB) ENGINE=InnoDB")
            row = witness()
            cursor.execute("INSERT INTO economic_baseline_control VALUES (%s,%s,%s,1,%s)",
                           (LINEAGE, EPOCH, OPENING, OP))
            cursor.execute("INSERT INTO economic_accounting_operation VALUES (%s,38,1,0)", (OP,))
            cursor.execute("INSERT INTO critical_operation_inbox (operation_id,status,result_code) VALUES (%s,1,0)", (OP,))
            cursor.execute("INSERT INTO economic_baseline_witness VALUES "
                           "(%s,%s,%s,1,1,1,%s,%s)",
                           (OP, LINEAGE, EPOCH, row["witness_digest"], row["canonical_witness"]))
        audit = pymysql.connect(**(settings | {"database": schema,
                                             "user": reader,
                                             "password": "disposable-audit-only"}))
        try:
            result = capture(audit, LINEAGE, EPOCH)
            assert len(result["account_origins"]) == len(result["item_origins"]) == 1
            with tempfile.TemporaryDirectory(prefix="duris-audit-origins-") as temporary:
                output = Path(temporary) / "origins.json"
                command = [sys.executable, str(ROOT / "scripts/economic_sql_audit_origins.py"),
                           "--host", "127.0.0.1", "--port", str(settings["port"]),
                           "--user", reader, "--database", schema,
                           "--lineage", LINEAGE.hex(), "--epoch", EPOCH.hex(),
                           "--output", str(output)]
                environment = dict(os.environ, DB_PASSWORD="disposable-audit-only")
                subprocess.run(command, env=environment, check=True, timeout=30)
                exported = json.loads(output.read_text(encoding="utf-8"))
                assert exported == result
                repeat = subprocess.run(command, env=environment, capture_output=True,
                                        text=True, timeout=30)
                assert repeat.returncode == 2 and "File exists" in repeat.stderr
            with setup.cursor() as cursor:
                cursor.execute("UPDATE economic_baseline_witness SET witness_digest=%s",
                               (bytes.fromhex("aa" * 32),))
            try:
                capture(audit, LINEAGE, EPOCH)
                raise AssertionError("corrupt SQL witness was accepted")
            except OriginError as error:
                assert "digest mismatch" in str(error), error
        finally:
            audit.close()
    finally:
        setup.close()
    print("SQL audit origins: consistent read-only export and digest refusal passed")
finally:
    with admin.cursor() as cursor:
        cursor.execute(f"DROP DATABASE IF EXISTS `{schema}`")
        cursor.execute(f"DROP USER IF EXISTS '{reader}'@'127.0.0.1'")
    admin.close()
