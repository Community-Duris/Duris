#!/usr/bin/env python3
"""Independent persisted shop forest controls and opt-in genuine SQL capture.

The native source reader corroborates the captured source facts. These fixtures
do not qualify shop gameplay, prototype payloads, enrollment or release.
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


def packet(depth=2):
    rows = [dict(item_id=400+i, keeper_id=17, parent_id=399+i if i else None,
        uid=81+i, vnum=0, equipment_slot=0, quantity=1) for i in range(depth)]
    items = [dict(uid=row["uid"], root=81, parent=row["uid"]-1 if i else None,
        owner=[9, 1, 0], revision=2, vnum=0, state="live", equipment_slot=0)
        for i, row in enumerate(rows)]
    return dict(shop_keepers=[dict(keeper_id=17, shop_id=0)], shop_items=rows,
        shop_custody_coverage=dict(keepers=1, items=depth), items=items)


def recount(native):
    native["shop_custody_coverage"] = dict(keepers=len(native["shop_keepers"]),
                                         items=len(native["shop_items"]))


def audit(native, limit=100):
    before = copy.deepcopy(native)
    reader = Reconciler(limit)
    reader.audit_shop_custody("sql_partial", native, {(row["uid"],): row for row in native["items"]})
    assert before == native
    assert len(reader.exceptions) <= limit
    return dict(reader.counts), reader.exceptions


class ShopCustodyAuditTests(unittest.TestCase):
    def test_native_row_ids_logical_owner_zero_parent_depth_and_widths(self):
        for depth in (1, 2, 32):
            native = packet(depth)
            native["shop_items"][0]["parent_id"] = 0
            native["shop_items"][0]["equipment_slot"] = 43
            native["items"][0].update(equipment_slot=43, revision=2**64-1)
            self.assertEqual(audit(native)[0], {})
        native = packet(1)
        native["shop_keepers"][0]["shop_id"] = 2**31-1
        native["shop_items"][0].update(item_id=2**32-1, uid=2**64-2, vnum=2**31-1)
        native["items"][0].update(uid=2**64-2, root=2**64-2, owner=[9,2**31,0], vnum=2**31-1)
        self.assertEqual(audit(native)[0], {})

    def test_disagreements_counts_are_independent_of_output_limit(self):
        changes = (
            ("shop_item_owner_mismatch", "items", 0, {"owner":[1,7,0]}),
            ("shop_item_owner_mismatch", "items", 0, {"owner":[9,17,0]}),
            ("shop_item_owner_mismatch", "items", 0, {"owner":[9,1,1]}),
            ("shop_uid_not_active", "items", 0, {"state":"quarantined"}),
            ("shop_uid_not_active", "items", 0, {"state":"tombstone"}),
            ("shop_item_topology_mismatch", "items", 1, {"parent":400}),
            ("shop_item_topology_mismatch", "items", 1, {"root":82}),
            ("shop_item_vnum_mismatch", "shop_items", 1, {"vnum":7}),
            ("shop_item_vnum_invalid", "shop_items", 1, {"vnum":-1}),
            ("shop_item_equipment_mismatch", "shop_items", 0, {"equipment_slot":2}),
            ("shop_item_equipment_invalid", "shop_items", 1, {"equipment_slot":1}),
            ("shop_item_equipment_invalid", "shop_items", 0, {"equipment_slot":44}),
            ("shop_item_equipment_invalid", "shop_items", 0, {"equipment_slot":None}),
            ("shop_item_quantity_invalid", "shop_items", 0, {"quantity":2}),
            ("shop_item_parent_missing", "shop_items", 1, {"parent_id":999}),
            ("shop_item_parent_cycle", "shop_items", 0, {"parent_id":401}),
            ("shop_legacy_uid_unknown", "shop_items", 0, {"uid":None}),
            ("shop_legacy_uid_unknown", "shop_items", 0, {"uid":0}),
            ("shop_uid_sentinel_invalid", "shop_items", 0, {"uid":2**64-1}),
            ("shop_item_missing_keeper", "shop_items", 0, {"keeper_id":18}),
            ("shop_logical_identity_invalid", "shop_keepers", 0, {"shop_id":-1}),
        )
        for code, collection, index, change in changes:
            native = packet(); native[collection][index].update(change)
            expected = audit(native)[0]
            self.assertIn(code, expected)
            for limit in (0, 1, 100):
                with self.subTest(code=code, limit=limit):
                    self.assertEqual(audit(native, limit)[0], expected)

    def test_reverse_coverage_duplicates_foreign_parent_and_native_bounds(self):
        cases = []
        native = packet(); native["shop_items"].pop()
        cases.append((native,"shop_uid_missing_physical"))
        native = packet(); native["items"].pop()
        cases.append((native,"shop_uid_unadmitted"))
        native = packet(); native["shop_keepers"].clear()
        cases.append((native,"shop_uid_unknown_keeper"))
        native = packet(); native["shop_keepers"] *= 2
        cases.append((native,"shop_duplicate_keeper"))
        native = packet(); native["shop_keepers"].append(dict(keeper_id=18,shop_id=0))
        cases.append((native,"shop_duplicate_logical_id"))
        native = packet(); native["shop_items"] *= 2
        cases.append((native,"shop_duplicate_row"))
        native = packet(); native["shop_items"][1]["uid"] = 81
        cases.append((native,"shop_duplicate_uid"))
        native = packet(); native["shop_items"][0]["keeper_id"] = 18
        native["shop_keepers"].append(dict(keeper_id=18,shop_id=77))
        cases.append((native,"shop_item_parent_foreign_keeper"))
        cases.append((packet(33),"shop_item_depth_exceeds_native_limit"))
        native = packet(1)
        native["shop_items"] = [dict(native["shop_items"][0],item_id=i+1,uid=None) for i in range(4097)]
        native["items"].clear()
        cases.append((native,"shop_item_count_exceeds_native_limit"))
        for native, code in cases:
            recount(native); counts = audit(native)[0]
            self.assertIn(code, counts)
            for limit in (0,1,100): self.assertEqual(audit(native,limit)[0], counts)
        # Missing physical rows never grant history ownership to a tombstone.
        native = packet(); native["shop_items"].clear()
        for row in native["items"]: row["state"] = "tombstone"
        recount(native); self.assertEqual(audit(native)[0], {})

    def test_input_representation_bounds_before_indexing(self):
        for collection, fields in (("shop_items", ("item_id","keeper_id","parent_id","uid","vnum","equipment_slot","quantity")),
                                   ("shop_keepers", ("keeper_id","shop_id"))):
            for field in fields:
                for value in (True,1.0,"1",[],{},2**64,-2**32):
                    native = packet(); native[collection][0][field] = value
                    with self.subTest(collection=collection,field=field,value=value):
                        with self.assertRaises(SnapshotError): audit(native,0)
                native = packet(); native[collection][0].pop(field)
                with self.assertRaises(SnapshotError): audit(native,0)
        for coverage in (None,{},dict(keepers=True,items=2),dict(keepers=1,items=1)):
            native = packet(); native["shop_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError): audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0], {"missing_shop_custody_coverage":1})
        for field in ("shop_items","shop_keepers"):
            native = packet(); native[field] *= MAX_ROWS+1
            with self.assertRaises(SnapshotError): audit(native,0)

    def test_exporter_aggregate_bounds_empty_lists_and_no_identity_joins(self):
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError): exporter.read_shop_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        self.assertEqual(exporter.read_shop_custody(cursor), dict(shop_keepers=[],shop_items=[],
            shop_custody_coverage=dict(keepers=0,items=0)))
        for call in cursor.execute.call_args_list[1:]:
            self.assertNotIn("JOIN",call.args[0]); self.assertNotIn("WHERE",call.args[0])
            self.assertEqual(call.args[1],(MAX_ROWS+1,))

    def test_global_cli_reports_private_unchanged_input(self):
        cut = clean_snapshot(); native = packet(1)
        cut["native"].update({key:value for key,value in native.items() if key != "items"})
        cut["native"]["items"][0]["vnum"] = 0
        cut["native"]["shop_items"][0]["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); original = path.read_bytes()
            for limit in (0,1,100):
                result = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),
                    str(path),"--limit",str(limit)],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,1,result)
                self.assertEqual(result.stderr,"")
                self.assertEqual(json.loads(result.stdout)["exception_counts"],{"shop_item_owner_mismatch":1})
                self.assertNotIn("PRIVATE",result.stdout); self.assertEqual(path.read_bytes(),original)


# Reuse the existing test-only JSON emitter, selecting the native shop source
# tables instead. The compiled production source/header are unmodified.
from test_sql_auction_custody_audit import NATIVE_SOURCE as AUCTION_SOURCE
NATIVE_SOURCE = AUCTION_SOURCE.replace(
    'table.name == "auctions" || table.name == "auction_item_custody" ||\n            table.name == "auction_item_pickups"',
    'table.name == "shopkeepers"').replace(
    '    for (const auto &table : result.item_equipment_sources)',
    '    for (const auto &table : result.item_sources)\n'
    '        if (table.name == "shopkeeper_items") emit(table, table.name);\n'
    '    for (const auto &table : result.item_equipment_sources)')
assert NATIVE_SOURCE != AUCTION_SOURCE and 'result.item_sources' in NATIVE_SOURCE


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_SHOP_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeShopCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_native_sources_and_read_only_forests(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql

        work = ROOT/"bin/tests/sql-shop-custody"; work.mkdir(parents=True,exist_ok=True)
        source,binary = work/"native.cpp",work/"native"; source.write_text(NATIVE_SOURCE)
        command = ["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie","-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config","--cflags"],text=True))
        command += [str(source),"src/persistence/economic_sql_source_snapshot.c"]
        command += shlex.split(subprocess.check_output(["mysql_config","--libs"],text=True))
        command += ["-lcrypto","-o",str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=600)
        observations = []
        with tempfile.TemporaryDirectory(prefix="duris-shop-audit-") as folder:
            for engine in ("mariadb","mysql"):
                candidate = Path(folder)/engine; candidate.mkdir(mode=0o700)
                with restore.private_database(candidate,engine) as env:
                    version = sql(env,"SELECT VERSION()")
                    sql(env,payload=(ROOT/"migrations/bootstrap_multithread_safe.sql").read_bytes())
                    with mock.patch.dict(os.environ,env,clear=True):
                        manifest = migrations.load_manifest(); executor = migrations.MysqlExecutor(manifest)
                        executor.adopt("fresh_bootstrap"); migrations.run_pending(manifest,executor)
                    terminal = sql(env,"SELECT sequence_number,migration_id FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1")
                    self.assertEqual(terminal,"64\t0064_auction_custody_history")
                    owner = pymysql.connect(unix_socket=env["DB_SOCKET"],user="root",database="duris_restore",
                        autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                    with owner:
                        with owner.cursor() as cursor:
                            cursor.execute("CREATE USER 'auction_reader'@'localhost' IDENTIFIED BY 'disposable-auction-reader'")
                            cursor.execute("GRANT SELECT ON duris_restore.* TO 'auction_reader'@'localhost'")
                            cursor.execute("INSERT INTO shopkeepers(id,shop_id) VALUES (17,0),(18,77)")
                        reader = pymysql.connect(unix_socket=env["DB_SOCKET"],user="auction_reader",password="disposable-auction-reader",
                            database="duris_restore",autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                        with reader:
                            def inventory():
                                with owner.cursor() as cursor:
                                    cursor.execute("SHOW TABLES"); names = sorted(next(iter(row.values())) for row in cursor.fetchall())
                                    values = []
                                    for name in names:
                                        cursor.execute("SELECT * FROM `"+name+"`")
                                        values.append((name,sorted(json.dumps(row,sort_keys=True,
                                            default=lambda value:value.hex() if isinstance(value,bytes) else str(value))
                                            for row in cursor.fetchall())))
                                    return hashlib.sha256(json.dumps(values).encode()).hexdigest()

                            def cut(label):
                                before = inventory()
                                with reader.cursor() as cursor:
                                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    try:
                                        native = exporter.read_shop_custody(cursor)
                                        cursor.execute("SELECT item_uid AS uid,root_item_uid AS root,parent_item_uid AS parent,"
                                            "owner_type,owner_id,owner_context_id,item_revision AS revision,vnum,state,equipment_slot "
                                            "FROM item_current_owner ORDER BY item_uid")
                                        native["items"] = list(cursor.fetchall())
                                        for row in native["items"]:
                                            row["owner"] = [row.pop(field) for field in ("owner_type","owner_id","owner_context_id")]
                                            row["state"] = {1:"live",2:"tombstone",3:"quarantined"}[row["state"]]
                                    finally: reader.rollback()
                                raw = subprocess.run([str(binary),env["DB_SOCKET"]],check=True,
                                    capture_output=True,text=True,timeout=60)
                                tables = json.loads(raw.stdout)
                                registry = {name:columns.split(",") for group,_,_,specs in CAPTURE_REGISTRIES
                                    if group in ("tables","item_sources") for name,columns,_ in specs}
                                registry["item_current_owner_equipment"] = ["item_uid","equipment_slot"]
                                decoded = {name:[dict(zip(registry[name],[None if value is None else bytes.fromhex(value)
                                    for value in row])) for row in rows] for name,rows in tables.items()}
                                self.assertEqual(set(decoded),{"shopkeepers","shopkeeper_items","item_current_owner","item_current_owner_equipment"})
                                self.assertEqual(native["shop_keepers"],[dict(keeper_id=int(row["id"]),shop_id=int(row["shop_id"]))
                                    for row in decoded["shopkeepers"]])
                                def number(value): return None if value is None else int(value)
                                self.assertEqual([{key:row[key] for key in ("item_id","keeper_id","parent_id","uid","vnum")}
                                    for row in native["shop_items"]], [dict(zip(("item_id","keeper_id","parent_id","uid","vnum"),
                                        [number(row[key]) for key in ("id","shopkeeper_id","container_id","obj_uid","vnum")]))
                                        for row in decoded["shopkeeper_items"]])
                                equipment = {int(row["item_uid"]):int(row["equipment_slot"]) for row in decoded["item_current_owner_equipment"]}
                                expected_items = [dict(uid=int(row["item_uid"]),root=int(row["root_item_uid"]),
                                    parent=number(row["parent_item_uid"]),owner=[int(row[field]) for field in ("owner_type","owner_id","owner_context_id")],
                                    revision=int(row["item_revision"]),vnum=int(row["vnum"]),state={1:"live",2:"tombstone",3:"quarantined"}[int(row["state"])],
                                    equipment_slot=equipment[int(row["item_uid"])]) for row in decoded["item_current_owner"]]
                                self.assertEqual(native["items"],expected_items)
                                (work/(engine+"-"+label+"-native.json")).write_bytes(raw.stdout.encode())
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native,sort_keys=True)+"\n")
                                counts = audit(native)[0]
                                for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)
                                self.assertEqual(inventory(),before)
                                observations.append(dict(engine=engine,version=version,label=label,counts=counts,
                                    native_sha256=hashlib.sha256(raw.stdout.encode()).hexdigest(),
                                    source_sha256=hashlib.sha256(json.dumps(native,sort_keys=True).encode()).hexdigest(),
                                    database_sha256=before,unchanged=True))
                                return counts

                            def reset():
                                with owner.cursor() as cursor:
                                    cursor.execute("DELETE FROM shopkeeper_items WHERE container_id IS NOT NULL")
                                    cursor.execute("DELETE FROM shopkeeper_items")
                                    cursor.execute("DELETE FROM item_current_owner WHERE parent_item_uid IS NOT NULL")
                                    cursor.execute("DELETE FROM item_current_owner")
                                    cursor.execute("UPDATE shopkeepers SET shop_id=0 WHERE id=17")
                                    for row,item in zip(packet()["shop_items"],packet()["items"]):
                                        cursor.execute("INSERT INTO shopkeeper_items(id,shopkeeper_id,container_id,obj_uid,vnum,equip_slot,quantity) "
                                            "VALUES (%s,%s,%s,%s,%s,%s,%s)",tuple(row[key] for key in ("item_id","keeper_id","parent_id","uid","vnum","equipment_slot","quantity")))
                                        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                                            "owner_context_id,item_revision,vnum,state,equipment_slot) VALUES (%s,%s,%s,9,1,0,2,0,1,0)",
                                            (item["uid"],item["root"],item["parent"]))
                            reset(); self.assertEqual(cut("healthy-row-id-nesting-shop-zero"),{})
                            cases = (
                                ("wrong-database-key-owner","UPDATE item_current_owner SET owner_id=17 WHERE item_uid=81","shop_item_owner_mismatch"),
                                ("wrong-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","shop_item_topology_mismatch"),
                                ("missing-physical-child","DELETE FROM shopkeeper_items WHERE id=401","shop_uid_missing_physical"),
                                ("unadmitted-child","DELETE FROM item_current_owner WHERE item_uid=82","shop_uid_unadmitted"),
                                ("foreign-keeper-parent","UPDATE shopkeeper_items SET shopkeeper_id=18 WHERE id=400","shop_item_parent_foreign_keeper"),
                                ("physical-cycle","UPDATE shopkeeper_items SET container_id=401 WHERE id=400","shop_item_parent_cycle"),
                                ("duplicate-physical-uid","UPDATE shopkeeper_items SET obj_uid=81 WHERE id=401","shop_duplicate_uid"),
                                ("legacy-null-root","UPDATE shopkeeper_items SET obj_uid=NULL WHERE id=400","shop_legacy_uid_unknown"),
                                ("tombstone-physical-root","UPDATE item_current_owner SET state=2 WHERE item_uid=81","shop_uid_not_active"),
                                ("physical-vnum-disagreement","UPDATE shopkeeper_items SET vnum=7 WHERE id=401","shop_item_vnum_mismatch"),
                                ("physical-equipment-disagreement","UPDATE shopkeeper_items SET equip_slot=2 WHERE id=400","shop_item_equipment_mismatch"),
                                ("physical-child-equipment","UPDATE shopkeeper_items SET equip_slot=1 WHERE id=401","shop_item_equipment_invalid"),
                                ("physical-quantity","UPDATE shopkeeper_items SET quantity=2 WHERE id=400","shop_item_quantity_invalid"),
                                ("orphan-keeper","UPDATE shopkeeper_items SET shopkeeper_id=999 WHERE id=401","shop_item_missing_keeper"),
                                ("orphan-parent","UPDATE shopkeeper_items SET container_id=999 WHERE id=401","shop_item_parent_missing"),
                                ("uid-sentinel","UPDATE shopkeeper_items SET obj_uid=18446744073709551615 WHERE id=400","shop_uid_sentinel_invalid"),
                                ("physical-null-equipment","UPDATE shopkeeper_items SET equip_slot=NULL WHERE id=400","shop_item_equipment_invalid"),
                            )
                            for label,statement,code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-"): cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try: cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-"): cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                self.assertIn(code,cut(label))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE shopkeeper_items SET quantity=NULL WHERE id=400")
                            self.assertEqual(cut("legacy-null-quantity-default-one"),{})
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor: cursor.execute("UPDATE shopkeeper_items SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),38)
        print("SQL_SHOP_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__":
    unittest.main()
