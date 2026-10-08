#!/usr/bin/env python3
"""Independent auction root audit controls and opt-in native SQL correspondence.

Run DURIS_RUN_SQL_AUCTION_AUDIT=1 for disposable canonical MySQL/MariaDB cuts.
Fixtures model metadata; the genuine C++ source reader authenticates capture
correspondence, not gameplay, prototype interpretation or release completion.
"""

import copy
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "scripts"), str(ROOT / "tests/async")]
import economic_sql_audit_snapshot as exporter
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError
from test_reconcile_economy_accounting import clean_snapshot

BLOB = b"opaque-native-template\x00"
DIGEST = hashlib.sha256(BLOB).hexdigest()


def packet(quantity=1):
    listing = dict(auction_id=99, seller_pid=7, winner_pid=0, status="OPEN", revision=1,
                   custody_state=1, quantity=quantity, vnum=0,
                   blob_bytes=len(BLOB), blob_sha256=DIGEST)
    roots = [dict(auction_id=99, slot=slot, uid=2**64-1-slot, revision=2**64-1,
                  vnum=0, claim_pid=None, claim_operation_id=None, claimed=False,
                  blob_bytes=len(BLOB), blob_sha256=DIGEST) for slot in range(quantity)]
    items = [dict(uid=row["uid"], root=row["uid"], parent=None, owner=[6, 99, 0],
                  revision=row["revision"], vnum=0, state="live", equipment_slot=0)
             for row in roots]
    return dict(auction_listings=[listing], auction_roots=roots, auction_legacy_pickups=[],
                auction_custody_coverage=dict(listings=1, roots=quantity, legacy_pickups=0),
                items=items)


def recount(native):
    native["auction_custody_coverage"] = dict(listings=len(native["auction_listings"]),
        roots=len(native["auction_roots"]), legacy_pickups=len(native["auction_legacy_pickups"]))


def audit(native, limit=50):
    before = copy.deepcopy(native)
    reader = Reconciler(limit)
    reader.audit_auction_custody("sql_partial", native,
                               {(row["uid"],): row for row in native["items"]})
    assert before == native
    assert len(reader.exceptions) <= limit
    assert sum(reader.counts.values()) >= len(reader.exceptions)
    return dict(reader.counts), reader.exceptions


class AuctionCustodyAuditTests(unittest.TestCase):
    def test_positive_claim_history_widths_and_nine_roots(self):
        for quantity in (1, 9):
            for status, winner, claimant in (("OPEN", 0, None), ("CLOSED", 8, 8),
                                             ("CLOSED", 0, 7), ("REMOVED", 8, 7)):
                for claimed in (False, True) if status != "OPEN" else (False,):
                    native = packet(quantity)
                    native["auction_listings"][0].update(status=status, winner_pid=winner,
                        revision=2**64-1, seller_pid=7)
                    for row, item in zip(native["auction_roots"], native["items"]):
                        row.update(claim_pid=claimant, claimed=claimed,
                                   claim_operation_id="12"*16 if claimed else None)
                        if claimed:
                            # Later moves, re-listing and tombstones never inherit
                            # the old auction's current owner/revision/vnum grant.
                            item.update(owner=[8, 0, 0], revision=1, state="tombstone", vnum=9)
                    with self.subTest(quantity=quantity, status=status, winner=winner, claimed=claimed):
                        self.assertEqual(audit(native)[0], {})

    def test_each_independent_disagreement_is_counted_at_every_limit(self):
        changes = (
            ("auction_owner_mismatch", "items", {"owner": [1, 7, 0]}),
            ("auction_owner_mismatch", "items", {"owner": [6, 99, 1]}),
            ("auction_uid_not_active", "items", {"state": "quarantined"}),
            ("auction_item_topology_mismatch", "items", {"root": 1, "parent": 1}),
            ("auction_item_revision_mismatch", "items", {"revision": 1}),
            ("auction_item_vnum_mismatch", "items", {"vnum": 1}),
            ("auction_item_equipment_mismatch", "items", {"equipment_slot": 1}),
            ("auction_claim_state_invalid", "auction_roots", {"claim_pid": 7}),
            ("auction_claim_receipt_state_invalid", "auction_roots", {"claim_operation_id": "12"*16}),
            ("auction_retained_template_mismatch", "auction_roots", {"blob_sha256": "ab"*32}),
            ("auction_retained_template_mismatch", "auction_roots", {"vnum": 1}),
            ("auction_root_blob_size_invalid", "auction_roots", {"blob_bytes": 32769}),
            ("auction_listing_blob_size_invalid", "auction_listings", {"blob_bytes": 0}),
        )
        for code, collection, change in changes:
            native = packet()
            native[collection][0].update(change)
            expected = audit(native, 100)[0]
            self.assertIn(code, expected)
            for limit in (0, 1, 100):
                with self.subTest(code=code, change=change, limit=limit):
                    self.assertEqual(audit(native, limit)[0], expected)

    def test_duplicates_orphans_missing_and_legacy_are_not_dropped(self):
        cases = []
        native = packet(9)
        native["auction_roots"].append(copy.deepcopy(native["auction_roots"][0]))
        cases.append((native, {"auction_duplicate_slot", "auction_unclaimed_uid_duplicate",
                               "auction_root_cardinality_mismatch"}))
        native = packet(); native["auction_listings"] *= 2
        cases.append((native, {"auction_duplicate_listing"}))
        native = packet(); native["auction_listings"].clear()
        cases.append((native, {"auction_root_missing_listing"}))
        native = packet(); native["auction_roots"].clear()
        cases.append((native, {"auction_root_cardinality_mismatch", "auction_uid_missing_unclaimed_root"}))
        native = packet(); native["items"].clear()
        cases.append((native, {"auction_uid_unadmitted"}))
        native = packet(); native["auction_listings"][0]["custody_state"] = 0
        cases.append((native, {"auction_legacy_listing_identity_unknown", "auction_root_non_authoritative_listing"}))
        for retrieved in (0, 1):
            native = packet()
            native["auction_legacy_pickups"] = [dict(id=1, pid=7, quantity=1, retrieved=retrieved,
                blob_bytes=len(BLOB), blob_sha256=DIGEST)] * 2
            cases.append((native, {"auction_legacy_pickup_identity_unknown", "auction_duplicate_legacy_pickup"}))
        for native, codes in cases:
            recount(native)
            expected = audit(native, 100)[0]
            self.assertTrue(codes <= expected.keys())
            for limit in (0, 1, 100):
                self.assertEqual(audit(native, limit)[0], expected)

    def test_no_float_bool_missing_or_out_of_range_identity_aliases(self):
        count = 0
        for collection, fields in (
                ("auction_listings", ("auction_id", "seller_pid", "winner_pid", "revision", "custody_state", "quantity", "vnum", "blob_bytes")),
                ("auction_roots", ("auction_id", "slot", "uid", "revision", "vnum", "blob_bytes"))):
            for field in fields:
                for value in (None, True, 1.0, "1", -1, 2**64):
                    native = packet(); native[collection][0][field] = value
                    with self.subTest(collection=collection, field=field, value=value):
                        with self.assertRaises(SnapshotError): audit(native, 0)
                    count += 1
        self.assertEqual(count, 84)
        for field, value in (("claimed", 1), ("claim_pid", 0), ("claim_pid", True),
                             ("claim_operation_id", "0"*32), ("blob_sha256", "AB"*32)):
            native = packet(); native["auction_roots"][0][field] = value
            with self.assertRaises(SnapshotError): audit(native, 0)
        for coverage in (None, {}, {"listings": True, "roots": 1, "legacy_pickups": 0},
                         {"listings": 1, "roots": 0, "legacy_pickups": 0}):
            native = packet(); native["auction_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError): audit(native, 0)
        native = dict(items=[])
        self.assertEqual(audit(native, 0)[0], {"missing_auction_custody_coverage": 1})

    def test_global_audit_and_cli_preserve_findings_and_private_input(self):
        snapshot = clean_snapshot()
        native = packet()
        native["auction_roots"][0].update(uid=81, revision=2)
        snapshot["native"].update({name: value for name, value in native.items() if name != "items"})
        snapshot["native"]["items"][0]["vnum"] = 0
        snapshot["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"
            path.write_text(json.dumps(snapshot))
            original = path.read_bytes()
            for limit in (0, 1, 100):
                result = subprocess.run([sys.executable, str(ROOT/"scripts/reconcile_economy_accounting.py"),
                    str(path), "--limit", str(limit)], capture_output=True, text=True, timeout=30)
                self.assertEqual(result.returncode, 1, result)
                self.assertEqual(result.stderr, "")
                data = json.loads(result.stdout)
                self.assertEqual(data["exception_counts"], {"auction_owner_mismatch": 1})
                self.assertNotIn("PRIVATE", result.stdout)
                self.assertEqual(path.read_bytes(), original)

    def test_exporter_bounds_before_any_detail_read(self):
        cursor = mock.Mock()
        cursor.fetchone.return_value = {"rows_total": MAX_ROWS+1}
        with self.assertRaisesRegex(exporter.ExportError, "exceeds audit bounds"):
            exporter.read_auction_custody(cursor)
        self.assertEqual(cursor.execute.call_count, 1)
        cursor.fetchall.assert_not_called()


NATIVE_SOURCE = r'''
#include "persistence/economic_sql_source_snapshot.h"
#include <mysql.h>
#include <cstdlib>
#include <iostream>
#include <string_view>
static void hex(std::string_view value) {
    constexpr char chars[] = "0123456789abcdef";
    std::cout << '"';
    for (unsigned char c : value) std::cout << chars[c >> 4] << chars[c & 15];
    std::cout << '"';
}
int main(int argc, char **argv) {
    if (argc != 2) return 2;
    MYSQL *db = mysql_init(nullptr);
    if (!db || !mysql_real_connect(db, "localhost", "auction_reader", "disposable-auction-reader",
                                  "duris_restore", 0, argv[1], 0)) return 3;
    economic_sql_source_snapshot result;
    const unsigned code = economic_sql_capture_sources(db, {}, &result);
    mysql_close(db);
    if (code || economic_sql_validate_sources(result)) return 4;
    std::cout << '{'; bool first = true;
    auto emit = [&](const economic_sql_source_table &table, const std::string &name) {
        if (!first) std::cout << ',';
        first = false;
        std::cout << '"' << name << "\":["; bool row_first = true;
        for (const auto &row : table.rows) {
            if (!row_first) std::cout << ',';
            row_first = false;
            std::cout << '['; bool cell_first = true;
            for (const auto &cell : row.cells) {
                if (!cell_first) std::cout << ',';
                cell_first = false;
                if (cell) hex(*cell); else std::cout << "null";
            }
            std::cout << ']';
        }
        std::cout << ']';
    };
    for (const auto &table : result.tables) {
        if (table.name == "auctions" || table.name == "auction_item_custody" ||
            table.name == "auction_item_pickups" || table.name == "item_current_owner")
            emit(table, table.name);
    }
    for (const auto &table : result.item_equipment_sources)
        if (table.name == "item_current_owner") emit(table, "item_current_owner_equipment");
    std::cout << "}\n";
}
'''


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_AUCTION_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeAuctionCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_native_capture_and_independent_read_only_cuts(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql

        work = ROOT/"bin/tests/sql-auction-custody"
        work.mkdir(parents=True, exist_ok=True)
        source, binary = work/"native.cpp", work/"native"
        source.write_text(NATIVE_SOURCE)
        command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O1", "-g",
                   "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
        command += [str(source), "src/persistence/economic_sql_source_snapshot.c"]
        command += shlex.split(subprocess.check_output(["mysql_config", "--libs"], text=True))
        command += ["-lcrypto", "-o", str(binary)]
        subprocess.run(command, cwd=ROOT, check=True, timeout=600)
        observations = []
        with tempfile.TemporaryDirectory(prefix="duris-auction-audit-") as folder:
            for engine in ("mariadb", "mysql"):
                candidate = Path(folder)/engine; candidate.mkdir(mode=0o700)
                with restore.private_database(candidate, engine) as env:
                    version = sql(env, "SELECT VERSION()")
                    sql(env, payload=(ROOT/"migrations/bootstrap_multithread_safe.sql").read_bytes())
                    with mock.patch.dict(os.environ, env, clear=True):
                        manifest = migrations.load_manifest(); executor = migrations.MysqlExecutor(manifest)
                        executor.adopt("fresh_bootstrap"); migrations.run_pending(manifest, executor)
                    terminal = sql(env, "SELECT sequence_number,migration_id FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1")
                    self.assertEqual(terminal, "64\t0064_auction_custody_history")
                    owner = pymysql.connect(unix_socket=env["DB_SOCKET"], user="root", database="duris_restore",
                        autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                    with owner:
                        with owner.cursor() as cursor:
                            cursor.execute("CREATE USER 'auction_reader'@'localhost' IDENTIFIED BY 'disposable-auction-reader'")
                            cursor.execute("GRANT SELECT ON duris_restore.* TO 'auction_reader'@'localhost'")
                            cursor.execute("INSERT INTO auctions(id,seller_pid,status,auction_revision,custody_state,quantity,obj_vnum,obj_blob_str) "
                                           "VALUES (99,7,'OPEN',1,1,9,0,%s)", (BLOB,))
                            for row in packet(9)["auction_roots"]:
                                cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                                    "owner_context_id,item_revision,vnum,state,equipment_slot) VALUES (%s,%s,NULL,6,99,0,%s,0,1,0)",
                                    (row["uid"], row["uid"], row["revision"]))
                                cursor.execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,obj_blob) "
                                    "VALUES (99,%s,%s,%s,0,%s)", (row["slot"], row["uid"], row["revision"], BLOB))
                        reader = pymysql.connect(unix_socket=env["DB_SOCKET"], user="auction_reader", password="disposable-auction-reader",
                            database="duris_restore", autocommit=True, cursorclass=pymysql.cursors.DictCursor)
                        with reader:
                            def inventory():
                                with owner.cursor() as cursor:
                                    cursor.execute("SHOW TABLES")
                                    names = sorted(next(iter(row.values())) for row in cursor.fetchall())
                                    values = []
                                    for name in names:
                                        cursor.execute("SELECT * FROM `"+name+"`")
                                        values.append((name, sorted(json.dumps(row, sort_keys=True,
                                            default=lambda value: value.hex() if isinstance(value, bytes) else str(value))
                                            for row in cursor.fetchall())))
                                    return hashlib.sha256(json.dumps(values).encode()).hexdigest()

                            def cut(label):
                                before = inventory()
                                with reader.cursor() as cursor:
                                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    try:
                                        native = exporter.read_auction_custody(cursor)
                                        cursor.execute("SELECT item_uid AS uid,root_item_uid AS root,parent_item_uid AS parent,"
                                            "owner_type,owner_id,owner_context_id,item_revision AS revision,vnum,state,equipment_slot "
                                            "FROM item_current_owner ORDER BY item_uid")
                                        native["items"] = cursor.fetchall()
                                        for row in native["items"]:
                                            row["owner"] = [row.pop(field) for field in ("owner_type", "owner_id", "owner_context_id")]
                                            row["state"] = {1: "live", 2: "tombstone", 3: "quarantined"}[row["state"]]
                                    finally: reader.rollback()
                                raw = subprocess.run([str(binary), env["DB_SOCKET"]], check=True,
                                    capture_output=True, text=True, timeout=60)
                                tables = json.loads(raw.stdout)
                                registry = {name: columns.split(",") for group, _, _, specifications in CAPTURE_REGISTRIES
                                            if group == "tables" for name, columns, _ in specifications}
                                registry["item_current_owner_equipment"] = ["item_uid", "equipment_slot"]
                                decoded = {name: [dict(zip(registry[name], [None if value is None else bytes.fromhex(value)
                                    for value in row])) for row in rows] for name, rows in tables.items()}
                                self.assertEqual(set(decoded), {"auctions", "auction_item_custody", "auction_item_pickups",
                                                               "item_current_owner", "item_current_owner_equipment"})
                                def template(row, field):
                                    return dict(blob_bytes=len(row[field]), blob_sha256=hashlib.sha256(row[field]).hexdigest())
                                expected_listings = [dict(auction_id=int(row["id"]), seller_pid=int(row["seller_pid"]),
                                    winner_pid=int(row["winning_bidder_pid"]), status=row["status"].decode(),
                                    revision=int(row["auction_revision"]), custody_state=int(row["custody_state"]),
                                    quantity=int(row["quantity"]), vnum=int(row["obj_vnum"]), **template(row, "obj_blob_str"))
                                    for row in decoded["auctions"]]
                                expected_roots = [dict(auction_id=int(row["auction_id"]), slot=int(row["slot"]),
                                    uid=int(row["item_uid"]), revision=int(row["item_revision"]), vnum=int(row["vnum"]),
                                    claim_pid=None if row["claim_pid"] is None else int(row["claim_pid"]),
                                    claim_operation_id=None if row["claim_operation_id"] is None else row["claim_operation_id"].hex(),
                                    claimed=bool(int(row["claimed_at IS NOT NULL"])), **template(row, "obj_blob"))
                                    for row in decoded["auction_item_custody"]]
                                expected_legacy = [dict(id=int(row["id"]), pid=int(row["pid"]),
                                    quantity=int(row["quantity"]), retrieved=int(row["retrieved"]), **template(row, "obj_blob_str"))
                                    for row in decoded["auction_item_pickups"]]
                                self.assertEqual(native["auction_listings"], expected_listings)
                                self.assertEqual(native["auction_roots"], expected_roots)
                                self.assertEqual(native["auction_legacy_pickups"], expected_legacy)
                                equipment = {int(row["item_uid"]): int(row["equipment_slot"])
                                             for row in decoded["item_current_owner_equipment"]}
                                expected_items = [dict(uid=int(row["item_uid"]), root=int(row["root_item_uid"]),
                                    parent=None if row["parent_item_uid"] is None else int(row["parent_item_uid"]),
                                    owner=[int(row[field]) for field in ("owner_type", "owner_id", "owner_context_id")],
                                    revision=int(row["item_revision"]), vnum=int(row["vnum"]),
                                    state={1: "live", 2: "tombstone", 3: "quarantined"}[int(row["state"])],
                                    equipment_slot=equipment[int(row["item_uid"])]) for row in decoded["item_current_owner"]]
                                self.assertEqual(native["items"], expected_items)
                                (work/(engine+"-"+label+"-native.json")).write_bytes(raw.stdout.encode())
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native, sort_keys=True)+"\n")
                                counts = audit(native, 100)[0]
                                for limit in (0, 1, 100): self.assertEqual(audit(native, limit)[0], counts)
                                self.assertEqual(inventory(), before)
                                observations.append(dict(engine=engine, version=version, label=label, counts=counts,
                                    native_sha256=hashlib.sha256(raw.stdout.encode()).hexdigest(),
                                    source_sha256=hashlib.sha256(json.dumps(native, sort_keys=True).encode()).hexdigest(),
                                    database_sha256=before, unchanged=True))
                                return counts

                            self.assertEqual(cut("healthy-nine-roots-vnum-zero-uint64"), {})
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE item_current_owner SET owner_id=100 WHERE item_uid=%s", (2**64-1,))
                            self.assertEqual(cut("wrong-owner"), {"auction_owner_mismatch": 1})
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE item_current_owner SET owner_id=99 WHERE item_uid=%s", (2**64-1,))
                                cursor.execute("UPDATE auctions SET status='CLOSED',winning_bidder_pid=8 WHERE id=99")
                                cursor.execute("UPDATE auction_item_custody SET claim_pid=8 WHERE auction_id=99")
                            self.assertEqual(cut("pending-winner"), {})
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE auction_item_custody SET claim_pid=7 WHERE slot=0")
                            self.assertEqual(cut("wrong-claimant"), {"auction_claim_state_invalid": 1})
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE auction_item_custody SET claim_pid=8,claim_operation_id=UNHEX(%s),claimed_at=CURRENT_TIMESTAMP(6) WHERE slot=0", ("12"*16,))
                                cursor.execute("UPDATE item_current_owner SET owner_type=8,owner_id=0,state=2,item_revision=1,vnum=9 WHERE item_uid=%s", (2**64-1,))
                            self.assertEqual(cut("claimed-tombstone-history"), {})
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO auctions(id,seller_pid,status,auction_revision,custody_state,quantity,obj_vnum,obj_blob_str) "
                                    "VALUES (100,8,'OPEN',1,1,1,9,%s)", (BLOB,))
                                cursor.execute("UPDATE item_current_owner SET owner_type=6,owner_id=100,state=1 WHERE item_uid=%s", (2**64-1,))
                                cursor.execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,obj_blob) "
                                    "VALUES (100,0,%s,1,9,%s)", (2**64-1, BLOB))
                            self.assertEqual(cut("claimed-history-and-new-unclaimed-listing"), {})
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO auction_item_pickups(pid,obj_blob_str,quantity,retrieved) VALUES (7,%s,1,0),(7,%s,1,1)", (BLOB, BLOB))
                            self.assertEqual(cut("legacy-pending-and-retrieved"), {"auction_legacy_pickup_identity_unknown": 2})
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor: cursor.execute("UPDATE auctions SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations, indent=2)+"\n")
        self.assertEqual(len(observations), 14)
        print("SQL_AUCTION_CUSTODY_NATIVE "+json.dumps(observations, sort_keys=True), flush=True)


if __name__ == "__main__":
    unittest.main()
