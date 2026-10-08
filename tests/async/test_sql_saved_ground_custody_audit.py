#!/usr/bin/env python3
"""Raw saved_ground correspondence is evidence, never qualified runtime authority."""
import copy
import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/"scripts"),str(ROOT/"tests/async")]
import economic_sql_audit_snapshot as exporter
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError
from test_reconcile_economy_accounting import clean_snapshot

UNQUALIFIED = {"saved_ground_full_runtime_authority_unqualified":1}


def recount(native):
    native["saved_ground_custody_coverage"] = dict(items=len(native["saved_ground_items"]),handoffs=len(native["saved_ground_handoffs"]))


def packet(count=2):
    rows = [dict(item_id=400+index,key_group=400,modern_history=0,room_vnum=10,parent_id=399+index if index else None,
        uid=81+index,vnum=100+index,quantity=1,weight=1,extra_flags=0,item_type=1,
        value0=0,value1=0,value2=0,value3=0) for index in range(count)]
    native = dict(saved_ground_items=rows,saved_ground_handoffs=[],items=[dict(uid=row["uid"],root=81,
        parent=row["uid"]-1 if row["parent_id"] else None,owner=[3,10,0],
        revision=2,vnum=row["vnum"],state="live",equipment_slot=0) for row in rows],coin_piles=[])
    recount(native); return native


def audit(native,limit=100):
    before = copy.deepcopy(native); reader = Reconciler(limit)
    reader.audit_saved_ground_custody("sql_partial",native,{(row["uid"],):row for row in native["items"]})
    assert before == native and len(reader.exceptions) <= limit
    return dict(reader.counts),reader.exceptions


class SavedGroundCustodyAuditTests(unittest.TestCase):
    def test_key_graphs_native_depth_bound_and_retained_history(self):
        self.assertEqual(audit(packet(0))[0],{})
        for count in (1,2,33,65):self.assertEqual(audit(packet(count))[0],UNQUALIFIED)
        self.assertIn("saved_ground_item_depth_exceeds_native_limit",audit(packet(66))[0])
        native=packet(3001)
        for row in native["saved_ground_items"][1:]:row["parent_id"]=400
        self.assertIn("saved_ground_group_exceeds_native_limit",audit(native,0)[0])
        native=packet();native["saved_ground_items"][1]["key_group"]=401
        self.assertIn("saved_ground_item_parent_foreign_owner",audit(native)[0])
        for value in (None,0):
            native=packet();native["saved_ground_items"][1]["parent_id"]=value
            self.assertIn("saved_ground_key_graph_invalid",audit(native)[0])
        native=packet(1);native["saved_ground_items"][0].update(modern_history=1,uid=999)
        self.assertEqual(audit(native)[0],dict(UNQUALIFIED,saved_ground_history_authority_unqualified=1))
        native=packet(1);source=native["saved_ground_items"][0]
        native["saved_ground_items"].append(dict(source,item_id=500,key_group=500))
        receipt=dict(season_epoch=1,source_root_id=400,source_uid=81,source_room_vnum=10,source_row_count=1,
            destination_root_id=500,retired=0,source_id_digest="12"*32,source_payload_digest=None,
            destination_payload_digest=None,source_group=400,destination_group=500)
        native["saved_ground_handoffs"]=[receipt];recount(native)
        self.assertEqual(audit(native)[0],dict(UNQUALIFIED,saved_ground_history_authority_unqualified=1))
        for field in receipt:
            for value in (True,1.0,[],{},2**65):
                bad=copy.deepcopy(native);bad["saved_ground_handoffs"][0][field]=value
                with self.assertRaises(SnapshotError):audit(bad,0)
            bad=copy.deepcopy(native);bad["saved_ground_handoffs"][0].pop(field)
            with self.assertRaises(SnapshotError):audit(bad,0)
        native=packet(0);native["items"]=packet(1)["items"]
        self.assertEqual(audit(native)[0],{})

    def test_each_disagreement_preserves_counts_across_limits(self):
        cases = (
            ("saved_ground_item_owner_mismatch","items",0,{"owner":[3,11,0]}),
            ("saved_ground_item_owner_mismatch","items",0,{"owner":[3,10,1]}),
            ("saved_ground_item_topology_mismatch","items",1,{"root":82}),
            ("saved_ground_item_topology_mismatch","items",1,{"parent":400}),
            ("saved_ground_item_vnum_mismatch","saved_ground_items",1,{"vnum":9}),
            ("saved_ground_item_equipment_mismatch","items",0,{"equipment_slot":1}),
            ("saved_ground_projection_inactive_uid","items",0,{"state":"tombstone"}),
            ("saved_ground_item_parent_missing","saved_ground_items",1,{"parent_id":999}),
            ("saved_ground_item_parent_cycle","saved_ground_items",0,{"parent_id":401}),
            ("saved_ground_item_parent_foreign_owner","saved_ground_items",0,{"room_vnum":11}),
            ("saved_ground_room_identity_unknown","saved_ground_items",0,{"room_vnum":0}),
            ("saved_ground_room_identity_unknown","saved_ground_items",0,{"room_vnum":-1}),
            ("saved_ground_legacy_uid_unknown","saved_ground_items",0,{"uid":None}),
            ("saved_ground_legacy_uid_unknown","saved_ground_items",0,{"uid":0}),
            ("saved_ground_unsupported_quantity","saved_ground_items",0,{"quantity":None}),
            ("saved_ground_unsupported_quantity","saved_ground_items",0,{"quantity":0}),
            ("saved_ground_unsupported_quantity","saved_ground_items",0,{"quantity":2}),
            ("saved_ground_item_vnum_invalid","saved_ground_items",0,{"vnum":0}),
            ("saved_ground_prototype_literal_unknown","saved_ground_items",0,{"item_type":None}),
            ("saved_ground_prototype_literal_unknown","saved_ground_items",0,{"weight":None}),
            ("saved_ground_prototype_literal_unknown","saved_ground_items",0,{"extra_flags":None}),
        )
        for code,name,index,change in cases:
            native = packet(); native[name][index].update(change); counts = audit(native)[0]
            self.assertIn(code,counts)
            for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)

    def test_missing_admission_duplicates_and_competing_sources(self):
        native = packet(); native["items"].pop(); self.assertIn("saved_ground_uid_unadmitted",audit(native)[0])
        native = packet(); native["saved_ground_items"] *= 2; recount(native)
        self.assertIn("saved_ground_duplicate_physical_row",audit(native)[0])
        native = packet(); native["saved_ground_items"][1]["uid"] = 81
        self.assertIn("saved_ground_duplicate_physical_uid",audit(native)[0])
        for table in ("player_items","pet_items","shop_items","corpse_items","locker_items","account_locker_items","siege_items"):
            native = packet(); native[table] = [dict(uid=81)]
            self.assertIn("saved_ground_duplicate_physical_uid",audit(native)[0])
        native = packet(); native["auction_roots"] = [dict(uid=81,claimed=False)]
        self.assertIn("saved_ground_duplicate_physical_uid",audit(native)[0])
        native["auction_roots"][0]["claimed"] = True
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native = packet(); native["player_items"] = [dict(uid=81,item_type=20,parent_id=None)]
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native["player_items"][0]["parent_id"] = 0
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native["player_items"][0]["parent_id"] = 999
        self.assertIn("saved_ground_duplicate_physical_uid",audit(native)[0])

    def test_coin_literals_do_not_gain_authority_from_unknown_or_aliased_payloads(self):
        native = packet(); native["saved_ground_items"][1].update(vnum=3,item_type=20,value0=1,value1=2,value2=3,value3=4)
        native["items"][1]["vnum"] = 3
        self.assertIn("saved_ground_coin_payload_unknown",audit(native)[0])
        native["coin_piles"] = [dict(uid=82,owner=[3,10,0],revision=2,state="live",amounts=[1,2,3,4])]
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        for value,code in ((None,"saved_ground_coin_value_unknown"),(-1,"saved_ground_negative_coin_value"),(9,"saved_ground_coin_literal_mismatch")):
            bad = copy.deepcopy(native); bad["saved_ground_items"][1]["value0"] = value; self.assertIn(code,audit(bad)[0])
        for change in ({"revision":1},{"owner":[3,11,0]},{"amounts":None}):
            bad = copy.deepcopy(native); bad["coin_piles"][0].update(change); self.assertIn("saved_ground_coin_payload_unknown",audit(bad)[0])
        bad = copy.deepcopy(native); bad["coin_piles"] *= 2; self.assertIn("saved_ground_coin_payload_unknown",audit(bad)[0])
        for change in ({"revision":True},{"revision":2.0},{"owner":[3.0,10,0]}):
            bad = copy.deepcopy(native); bad["coin_piles"][0].update(change)
            with self.assertRaises(SnapshotError):audit(bad,0)
        native = packet(); native["saved_ground_items"][0].update(uid=None,item_type=20,value0=-1)
        self.assertIn("saved_ground_negative_coin_value",audit(native)[0])
        native = packet(); native["saved_ground_items"][0]["value0"] = None
        self.assertEqual(audit(native)[0],UNQUALIFIED)

    def test_exact_representations_and_unfiltered_bounded_export(self):
        example = packet()
        for field in example["saved_ground_items"][0]:
            for value in (True,1.0,"1",[],{},2**64,-2**64):
                native = copy.deepcopy(example); native["saved_ground_items"][0][field] = value
                with self.assertRaises(SnapshotError):audit(native,0)
            native = copy.deepcopy(example); native["saved_ground_items"][0].pop(field)
            with self.assertRaises(SnapshotError):audit(native,0)
        for coverage in (None,{},dict(items=True,handoffs=0),dict(items=2,handoffs=0,extra=0)):
            native = packet(); native["saved_ground_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError):audit(native,0)
        for value in (True,1.0,None,"1",2**31):
            native = packet(); native["items"][0]["vnum"] = value
            with self.assertRaises(SnapshotError):audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0],{"missing_saved_ground_custody_coverage":1})
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError):exporter.read_saved_ground_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        self.assertEqual(exporter.read_saved_ground_custody(cursor),dict(saved_ground_items=[],saved_ground_handoffs=[],saved_ground_custody_coverage=dict(items=0,handoffs=0)))
        query = cursor.execute.call_args_list[-1]
        self.assertEqual(query.args[1],(MAX_ROWS+1,))
        for private in ("name","short_descr","description","action_descr"):self.assertNotIn(private,query.args[0])

    def test_complete_row_bound_and_linear_ancestry_budget(self):
        native=packet(MAX_ROWS);rows=native["saved_ground_items"]
        for index,row in enumerate(rows):
            anchor=index//65*65
            row.update(key_group=400+anchor,parent_id=399+index if index%65 else None)
            native["items"][index]["root"]=81+anchor
            if not index%65:native["items"][index]["parent"]=None
        rows.reverse();start=time.monotonic()
        self.assertEqual(audit(native,0)[0],UNQUALIFIED);seconds=time.monotonic()-start
        self.assertLess(seconds,30)
        print("SAVED_GROUND_AUDIT_BUDGET "+json.dumps(dict(rows=MAX_ROWS,depth=65,seconds=seconds,budget=30)),flush=True)
        rows.append(rows[0]);recount(native)
        with self.assertRaises(SnapshotError):audit(native,0)

    def test_global_cli_preserves_bytes_and_private_aliases(self):
        cut = clean_snapshot(); native = packet(1); native["saved_ground_items"][0]["uid"] = 999
        cut["native"].update({key:value for key,value in native.items() if key not in ("items","coin_piles")})
        cut["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); before = path.read_bytes()
            for limit in (0,1,100):
                result = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),str(path),"--limit",str(limit)],
                    capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,1,result); self.assertEqual(result.stderr,"")
                self.assertEqual(json.loads(result.stdout)["exception_counts"],dict(UNQUALIFIED,saved_ground_uid_unadmitted=1))
                self.assertNotIn("PRIVATE",result.stdout); self.assertEqual(path.read_bytes(),before)


from test_sql_player_custody_audit import NATIVE_SOURCE as PLAYER_SOURCE
NATIVE_SOURCE = PLAYER_SOURCE.replace('#include <mysql.h>',
    '#include <mysql.h>\n#include "persistence/sql_room_item_payload.h"\n#include "persistence/sql_room_coin_payload.h"').replace(
    'table.name == "player_data" || table.name == "item_current_owner"','table.name == "item_current_owner"').replace(
    '    for (const auto &table : result.item_sources)\n'
    '        if (table.name == "player_pet_items") emit(table, table.name);\n','').replace(
    '    if (argc != 2) return 2;',r'''
    if ((argc == 4 || argc == 5) && std::string_view(argv[1]) == "--history") {
        MYSQL *db=mysql_init(nullptr); bool present=false;
        if (!db || !mysql_real_connect(db,"localhost","auction_reader","disposable-auction-reader",
            argc == 5 ? argv[4] : "duris_restore",0,argv[2],0) || mysql_query(db,"START TRANSACTION READ ONLY")) return 6;
        bool coin_history=false;
        bool ok=sql_room_item_payload_present(db,std::stoull(argv[3]),&present) &&
            sql_room_coin_payload_present(db,std::stoull(argv[3]),&coin_history);
        present=present || coin_history;
        if (!ok) std::cerr << "native history refused errno=" << errno << " mysql=" << mysql_errno(db) << '\n';
        mysql_rollback(db); mysql_close(db); if (!ok) return 7;
        std::cout << (present ? "1" : "0") << '\n'; return 0;
    }
    if (argc != 2) return 2;''')


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_SAVED_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeSavedGroundCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_current_capture_modern_presence_and_coin_codec(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql
        work = ROOT/"bin/tests/sql-saved-ground-custody"; work.mkdir(parents=True,exist_ok=True)
        source,binary = work/"native.cpp",work/"native"; source.write_text(NATIVE_SOURCE)
        command = ["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
            "-ffunction-sections","-fdata-sections","-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config","--cflags"],text=True))
        command += [str(source),"src/persistence/economic_sql_source_snapshot.c","src/player/player_snapshot_codec.c","src/persistence/sql_room_item_payload.c","src/persistence/sql_room_coin_payload.c"]
        command += shlex.split(subprocess.check_output(["mysql_config","--libs"],text=True))
        command += ["-Wl,--gc-sections","-lcrypto","-o",str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=600)
        coin = bytes.fromhex(json.loads(subprocess.check_output([str(binary),"--coin","82","3"],text=True)))
        (work/"native-coin-82.bin").write_bytes(coin); observations = []
        with tempfile.TemporaryDirectory(prefix="duris-saved-ground-audit-") as folder:
            for engine in ("mariadb","mysql"):
                candidate = Path(folder)/engine; candidate.mkdir(mode=0o700)
                with restore.private_database(candidate,engine) as env:
                    version = sql(env,"SELECT VERSION()")
                    sql(env,payload=(ROOT/"migrations/bootstrap_multithread_safe.sql").read_bytes())
                    with mock.patch.dict(os.environ,env,clear=True):
                        manifest = migrations.load_manifest(); executor = migrations.MysqlExecutor(manifest)
                        executor.adopt("fresh_bootstrap"); migrations.run_pending(manifest,executor)
                    self.assertEqual(sql(env,"SELECT sequence_number,migration_id FROM mud_schema_history ORDER BY sequence_number DESC LIMIT 1"),
                                     "64\t0064_auction_custody_history")
                    owner = pymysql.connect(unix_socket=env["DB_SOCKET"],user="root",database="duris_restore",
                        autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                    with owner:
                        with owner.cursor() as cursor:
                            cursor.execute("CREATE USER 'auction_reader'@'localhost' IDENTIFIED BY 'disposable-auction-reader'")
                            cursor.execute("GRANT SELECT ON duris_restore.* TO 'auction_reader'@'localhost'")
                            cursor.execute("INSERT INTO player_data(pid,name) VALUES (7,'PRIVATE-player')")
                        reader = pymysql.connect(unix_socket=env["DB_SOCKET"],user="auction_reader",password="disposable-auction-reader",
                            database="duris_restore",autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                        with reader:
                            def inventory():
                                with owner.cursor() as cursor:
                                    cursor.execute("SHOW TABLES"); names = sorted(next(iter(row.values())) for row in cursor.fetchall()); values = []
                                    for name in names:
                                        cursor.execute("SELECT * FROM `"+name+"`")
                                        values.append((name,sorted(json.dumps(row,sort_keys=True,
                                            default=lambda value:value.hex() if isinstance(value,bytes) else str(value)) for row in cursor.fetchall())))
                                    return hashlib.sha256(json.dumps(values).encode()).hexdigest()
                            def cut(label):
                                before = inventory()
                                with reader.cursor() as cursor:
                                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    try:
                                        native = exporter.read_saved_ground_custody(cursor)
                                        native.update(exporter.read_player_custody(cursor))
                                        native.update(exporter.read_shop_custody(cursor))
                                        native.update(exporter.read_corpse_custody(cursor))
                                        native.update(exporter.read_locker_custody(cursor))
                                        native.update(exporter.read_auction_custody(cursor))
                                        cursor.execute("SELECT item_uid AS uid,root_item_uid AS root,parent_item_uid AS parent,owner_type,owner_id,"
                                            "owner_context_id,item_revision AS revision,vnum,state,equipment_slot,coin_payload FROM item_current_owner ORDER BY item_uid")
                                        native["items"] = list(cursor.fetchall()); native["coin_piles"] = []
                                        for row in native["items"]:
                                            row["owner"] = [row.pop(field) for field in ("owner_type","owner_id","owner_context_id")]
                                            row["state"] = {1:"live",2:"tombstone",3:"quarantined"}[row["state"]]
                                            blob = row.pop("coin_payload")
                                            if blob is not None:
                                                native["coin_piles"].append(dict(uid=row["uid"],owner=row["owner"],revision=row["revision"],state=row["state"],
                                                    amounts=exporter.decode_coin_payload(blob,row["uid"],row["vnum"])))
                                    finally:reader.rollback()
                                raw = subprocess.run([str(binary),env["DB_SOCKET"]],check=True,capture_output=True,text=True,timeout=60)
                                tables = json.loads(raw.stdout)
                                registry = {name:columns.split(",") for group,_,_,specs in CAPTURE_REGISTRIES if group in ("tables","item_sources") for name,columns,_ in specs}
                                registry["item_current_owner_equipment"] = ["item_uid","equipment_slot"]
                                decoded = {name:[dict(zip(registry[name],[None if value is None else bytes.fromhex(value) for value in row])) for row in rows]
                                    for name,rows in tables.items()}
                                self.assertEqual(set(decoded),{"item_current_owner","item_current_owner_equipment"})
                                for row in native["saved_ground_items"]:
                                    if row["uid"] and row["uid"] != 2**64-1:
                                        present=subprocess.check_output([str(binary),"--history",env["DB_SOCKET"],str(row["uid"])],text=True,timeout=30).strip()
                                        self.assertEqual(row["modern_history"],int(present))
                                    elif row["uid"] == 2**64-1:
                                        refused=subprocess.run([str(binary),"--history",env["DB_SOCKET"],str(row["uid"])],capture_output=True,text=True,timeout=30)
                                        self.assertEqual(refused.returncode,7);self.assertEqual(refused.stdout,"")
                                        self.assertIn("errno=22",refused.stderr);self.assertEqual(row["modern_history"],1)
                                equipment = {int(row["item_uid"]):int(row["equipment_slot"]) for row in decoded["item_current_owner_equipment"]}
                                expected = [dict(uid=int(row["item_uid"]),root=int(row["root_item_uid"]),parent=int(row["parent_item_uid"]) if row["parent_item_uid"] else None,
                                    owner=[int(row[field]) for field in ("owner_type","owner_id","owner_context_id")],revision=int(row["item_revision"]),
                                    vnum=int(row["vnum"]),state={1:"live",2:"tombstone",3:"quarantined"}[int(row["state"])],equipment_slot=equipment[int(row["item_uid"])])
                                    for row in decoded["item_current_owner"]]
                                self.assertEqual(native["items"],expected)
                                for row in decoded["item_current_owner"]:
                                    if row["coin_payload"] is not None:self.assertEqual(row["coin_payload"],coin)
                                (work/(engine+"-"+label+"-native.json")).write_bytes(raw.stdout.encode())
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native,sort_keys=True)+"\n")
                                counts = audit(native)[0]
                                for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)
                                self.assertEqual(inventory(),before); self.assertNotIn("PRIVATE",json.dumps(native))
                                observations.append(dict(engine=engine,version=version,label=label,counts=counts,unchanged=True,database_sha256=before,
                                    native_sha256=hashlib.sha256(raw.stdout.encode()).hexdigest(),source_sha256=hashlib.sha256(json.dumps(native,sort_keys=True).encode()).hexdigest()))
                                return counts
                            def reset(count=2):
                                with owner.cursor() as cursor:
                                    cursor.execute("DELETE FROM saved_item_recovery_handoff"); cursor.execute("DELETE FROM sql_room_item_payload")
                                    cursor.execute("UPDATE saved_items SET container_id=NULL"); cursor.execute("DELETE FROM saved_items")
                                    cursor.execute("UPDATE player_items SET container_id=NULL"); cursor.execute("DELETE FROM player_items")
                                    cursor.execute("DELETE FROM auction_item_custody"); cursor.execute("DELETE FROM auctions")
                                    cursor.execute("UPDATE item_current_owner SET parent_item_uid=NULL"); cursor.execute("DELETE FROM item_current_owner")
                                    native = packet(count)
                                    for row in native["saved_ground_items"]:
                                        fields = ["id","room_vnum","container_id","obj_uid","vnum","quantity","weight","extra_flags","item_type","value0","value1","value2","value3"]
                                        values = [row[key] for key in ("item_id","room_vnum","parent_id","uid","vnum","quantity","weight","extra_flags","item_type","value0","value1","value2","value3")]
                                        cursor.execute("INSERT INTO saved_items("+",".join(fields)+",item_key) VALUES ("+",".join(["%s"]*len(fields))+",%s)",values+["PRIVATE-ground"])
                                    for row in native["items"]:
                                        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) "
                                            "VALUES (%s,%s,%s,3,10,0,2,%s,1,0)",(row["uid"],row["root"],row["parent"],row["vnum"]))
                            reset(); self.assertEqual(cut("retained-correspondence-unqualified"),UNQUALIFIED)
                            cases = (
                                ("wrong-owner","UPDATE item_current_owner SET owner_id=11 WHERE item_uid=81","saved_ground_item_owner_mismatch"),
                                ("wrong-context","UPDATE item_current_owner SET owner_context_id=1 WHERE item_uid=81","saved_ground_item_owner_mismatch"),
                                ("unadmitted-child","DELETE FROM item_current_owner WHERE item_uid=82","saved_ground_uid_unadmitted"),
                                ("wrong-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","saved_ground_item_topology_mismatch"),
                                ("wrong-parent","UPDATE item_current_owner SET parent_item_uid=NULL WHERE item_uid=82","saved_ground_item_topology_mismatch"),
                                ("cross-room-parent","UPDATE saved_items SET room_vnum=11 WHERE id=400","saved_ground_item_parent_foreign_owner"),
                                ("cycle","UPDATE saved_items SET container_id=401 WHERE id=400","saved_ground_item_parent_cycle"),
                                ("duplicate-uid","UPDATE saved_items SET obj_uid=81 WHERE id=401","saved_ground_duplicate_physical_uid"),
                                ("legacy-null-uid","UPDATE saved_items SET obj_uid=NULL WHERE id=400","saved_ground_legacy_uid_unknown"),
                                ("legacy-zero-uid","UPDATE saved_items SET obj_uid=0 WHERE id=400","saved_ground_legacy_uid_unknown"),
                                ("inactive-retained","UPDATE item_current_owner SET state=2 WHERE item_uid=81","saved_ground_projection_inactive_uid"),
                                ("vnum","UPDATE saved_items SET vnum=9 WHERE id=401","saved_ground_item_vnum_mismatch"),
                                ("equipment","UPDATE item_current_owner SET equipment_slot=1 WHERE item_uid=81","saved_ground_item_equipment_mismatch"),
                                ("quantity-null","UPDATE saved_items SET quantity=NULL WHERE id=400","saved_ground_unsupported_quantity"),
                                ("quantity-two","UPDATE saved_items SET quantity=2 WHERE id=400","saved_ground_unsupported_quantity"),
                                ("weight-null","UPDATE saved_items SET weight=NULL WHERE id=400","saved_ground_prototype_literal_unknown"),
                                ("type-null","UPDATE saved_items SET item_type=NULL WHERE id=400","saved_ground_prototype_literal_unknown"),
                                ("room-null","UPDATE saved_items SET room_vnum=NULL WHERE id=400","saved_ground_room_identity_unknown"),
                                ("parent-zero","UPDATE saved_items SET container_id=0 WHERE id=401","saved_ground_key_graph_invalid"),
                                ("room-zero","UPDATE saved_items SET room_vnum=0 WHERE id=400","saved_ground_room_identity_unknown"),
                                ("room-negative","UPDATE saved_items SET room_vnum=-1 WHERE id=400","saved_ground_room_identity_unknown"),
                                ("vnum-zero","UPDATE saved_items SET vnum=0 WHERE id=400","saved_ground_item_vnum_invalid"),
                                ("orphan-parent","UPDATE saved_items SET container_id=999 WHERE id=401","saved_ground_item_parent_missing"),
                            )
                            for label,statement,code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-") or label=="parent-zero":cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try:cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-") or label=="parent-zero":cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                self.assertIn(code,cut(label))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE saved_items SET vnum=3,item_type=20,value0=1,value1=2,value2=3,value3=4 WHERE id=401")
                                cursor.execute("UPDATE item_current_owner SET vnum=3,coin_payload=%s WHERE item_uid=82",(coin,))
                            self.assertEqual(cut("genuine-nested-coin"),dict(UNQUALIFIED,saved_ground_history_authority_unqualified=1))
                            with owner.cursor() as cursor:cursor.execute("UPDATE saved_items SET value0=9 WHERE id=401")
                            self.assertEqual(cut("genuine-nested-coin-mismatch"),dict(UNQUALIFIED,saved_ground_history_authority_unqualified=1))
                            with owner.cursor() as cursor:cursor.execute("UPDATE saved_items SET obj_uid=NULL,value0=-1 WHERE id=401")
                            self.assertIn("saved_ground_negative_coin_value",cut("uidless-negative-coin"))
                            reset(65); self.assertEqual(cut("native-source-depth-65"),UNQUALIFIED)
                            reset(66); self.assertIn("saved_ground_item_depth_exceeds_native_limit",cut("native-source-depth-66"))
                            reset()
                            with owner.cursor() as cursor:cursor.execute("UPDATE saved_items SET item_key='privaté-GROUND ' WHERE id=401")
                            self.assertEqual(cut("collation-case-accent-trailing-space"),UNQUALIFIED)
                            with owner.cursor() as cursor:cursor.execute("UPDATE saved_items SET item_key='PRIVATE-other' WHERE id=401")
                            self.assertIn("saved_ground_item_parent_foreign_owner",cut("cross-key-parent"))
                            reset(1)
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO saved_items(id,item_key,room_vnum,obj_uid,vnum,quantity,weight,extra_flags,item_type) "
                                    "VALUES (500,'item.uid.81',10,81,100,1,1,0,1)")
                                cursor.execute("INSERT INTO saved_item_recovery_handoff(season_epoch,source_root_id,source_key,source_uid,source_room_vnum,"
                                    "source_row_count,source_id_digest,destination_root_id,destination_key) "
                                    "VALUES (1,400,'PRIVATE-ground',81,10,1,UNHEX(%s),500,'item.uid.81')",("12"*32,))
                            history=dict(UNQUALIFIED,saved_ground_history_authority_unqualified=1)
                            self.assertEqual(cut("unverifiable-handoff-not-two-current-grants"),history)
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE saved_item_recovery_handoff SET retired_at=CURRENT_TIMESTAMP(6)")
                                cursor.execute("DELETE FROM saved_items WHERE id=400")
                            self.assertEqual(cut("retired-source-absent-still-unqualified"),history)
                            reset(1)
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO critical_operation_inbox(operation_id,command_type,schema_version,payload_version,result_payload,keys_hash,command_hash,status) "
                                    "VALUES (UNHEX(%s),5,2,1,X'',UNHEX(%s),UNHEX(%s),1)",("34"*16,"56"*32,"78"*32))
                                cursor.execute("INSERT INTO sql_room_item_payload(item_uid,item_revision,payload_version,operation_id,season_epoch,payload) "
                                    "VALUES (81,1,1,UNHEX(%s),1,%s)",("34"*16,coin))
                                cursor.execute("UPDATE item_current_owner SET owner_id=11 WHERE item_uid=81")
                            self.assertEqual(cut("modern-payload-history-not-current-legacy-grant"),history)
                            reset()
                            with owner.cursor() as cursor:cursor.execute("UPDATE saved_items SET obj_uid=18446744073709551615 WHERE id=400")
                            self.assertEqual(cut("native-coin-history-sentinel-refused"),history)
                            reset()
                            with owner.cursor() as cursor:cursor.execute("INSERT INTO player_items(pid,obj_uid,vnum) VALUES (7,81,100)")
                            self.assertIn("saved_ground_duplicate_physical_uid",cut("competing-player-physical-uid"))
                            with owner.cursor() as cursor:cursor.execute("UPDATE player_items SET item_type=20")
                            self.assertEqual(cut("wallet-root-excluded"),UNQUALIFIED)
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO player_items(id,pid,obj_uid,vnum,item_type) VALUES (900,7,999,100,15)")
                                cursor.execute("UPDATE player_items SET container_id=900 WHERE obj_uid=81")
                            self.assertIn("saved_ground_duplicate_physical_uid",cut("nested-player-money-competes"))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO auctions(id,seller_pid,status,auction_revision,custody_state,quantity,obj_vnum,obj_blob_str) "
                                    "VALUES (99,7,'OPEN',1,1,1,100,'synthetic-retained-payload')")
                                cursor.execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,obj_blob) "
                                    "VALUES (99,0,81,1,100,%s)",(coin,))
                            self.assertIn("saved_ground_duplicate_physical_uid",cut("unclaimed-auction-competes"))
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE auction_item_custody SET claim_pid=7,claim_operation_id=UNHEX(%s),claimed_at=CURRENT_TIMESTAMP(6)",("12"*16,))
                            self.assertEqual(cut("claimed-auction-history-does-not-compete"),UNQUALIFIED)
                            reset(0); self.assertEqual(cut("empty-retained-projection"),{})
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor:cursor.execute("UPDATE saved_items SET id=id")
                    # Reuse the original diagnostic model schema for predicate
                    # parity only. These synthetic markers are never full native
                    # capsules, canonical recovery, or publication authority.
                    tree=ast.parse((ROOT/"tests/async/run_economic_sql_audit_snapshot_mysql.py").read_text(encoding="utf-8"))
                    statements=next(ast.literal_eval(node.value) for node in tree.body if isinstance(node,ast.Assign)
                        and any(isinstance(target,ast.Name) and target.id=="TABLES" for target in node.targets))
                    marker=pymysql.connect(unix_socket=env["DB_SOCKET"],user="root",autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                    with marker:
                        with marker.cursor() as cursor:
                            cursor.execute("CREATE DATABASE duris_history_markers CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
                            cursor.execute("USE duris_history_markers")
                            for statement in statements:cursor.execute(statement)
                            for name in ("economic_epoch","economic_lineage_state"):
                                cursor.execute("CREATE TABLE "+name+" LIKE duris_restore."+name)
                            cursor.execute("CREATE TABLE season_reset_state(state_id INT,season_epoch BIGINT UNSIGNED) ENGINE=InnoDB")
                            cursor.execute("CREATE TABLE item_owner_revision(owner_type INT,owner_id BIGINT,owner_context_id BIGINT,revision BIGINT UNSIGNED) ENGINE=InnoDB")
                            cursor.execute("GRANT SELECT ON duris_history_markers.* TO 'auction_reader'@'localhost'")
                            cursor.execute("INSERT INTO saved_items VALUES (400,'PRIVATE-marker',10,NULL,81,100,1,1,0,1,0,0,0,0)")
                        marker_reader=pymysql.connect(unix_socket=env["DB_SOCKET"],user="auction_reader",password="disposable-auction-reader",
                            database="duris_history_markers",autocommit=True,cursorclass=pymysql.cursors.DictCursor)
                        with marker_reader:
                            def marker_inventory():
                                with marker.cursor() as cursor:
                                    cursor.execute("SHOW TABLES");names=sorted(next(iter(row.values())) for row in cursor.fetchall());values=[]
                                    for name in names:
                                        cursor.execute("SELECT * FROM `"+name+"`")
                                        values.append((name,sorted(json.dumps(row,sort_keys=True,
                                            default=lambda value:value.hex() if isinstance(value,bytes) else str(value)) for row in cursor.fetchall())))
                                    return hashlib.sha256(json.dumps(values).encode()).hexdigest()
                            def marker_cut(label,expected):
                                before=marker_inventory()
                                with marker_reader.cursor() as cursor:
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    try:native=exporter.read_saved_ground_custody(cursor)
                                    finally:marker_reader.rollback()
                                native.update(items=[],coin_piles=[])
                                raw=subprocess.check_output([str(binary),"--history",env["DB_SOCKET"],"81","duris_history_markers"],text=True,timeout=30)
                                self.assertEqual(int(raw.strip()),expected)
                                self.assertEqual(native["saved_ground_items"][0]["modern_history"],expected)
                                counts=audit(native)[0]
                                for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)
                                self.assertEqual(counts,dict(UNQUALIFIED,**({"saved_ground_history_authority_unqualified":1} if expected else {"saved_ground_uid_unadmitted":1})))
                                self.assertEqual(marker_inventory(),before);self.assertNotIn("PRIVATE",json.dumps(native))
                                label="synthetic-marker-"+label
                                (work/(engine+"-"+label+"-native.json")).write_text(raw)
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native,sort_keys=True)+"\n")
                                observations.append(dict(engine=engine,version=version,label=label,counts=counts,unchanged=True,database_sha256=before,
                                    native_sha256=hashlib.sha256(raw.encode()).hexdigest(),source_sha256=hashlib.sha256(json.dumps(native,sort_keys=True).encode()).hexdigest(),
                                    synthetic_marker_only=True,canonical_recovery_qualified=False))
                            def clear_markers():
                                with marker.cursor() as cursor:
                                    for name in ("item_current_owner","sql_room_item_payload","item_ownership_ledger","economic_accounting_child",
                                        "economic_accounting_item_reference","economic_accounting_account_effect","economic_accounting_operation","critical_operation_inbox"):
                                        cursor.execute("DELETE FROM "+name)
                            marker_cut("absent",0)
                            branches=(
                                ("immutable-payload",["INSERT INTO sql_room_item_payload VALUES (81)"],1),
                                ("current-coin-bytes",["INSERT INTO item_current_owner(item_uid,coin_payload) VALUES (81,X'78')"],1),
                                ("coin-reference-inbox",["INSERT INTO critical_operation_inbox(operation_id,command_type) VALUES (X'12',17)",
                                    "INSERT INTO economic_accounting_item_reference(operation_id,item_uid) VALUES (X'12',81)"],1),
                                ("coin-reference-writer",["INSERT INTO economic_accounting_operation(operation_id,writer_id,reason) VALUES (X'12',5,3)",
                                    "INSERT INTO economic_accounting_item_reference(operation_id,item_uid) VALUES (X'12',81)"],1),
                                ("wrong-writer",["INSERT INTO economic_accounting_operation(operation_id,writer_id,reason) VALUES (X'12',6,3)",
                                    "INSERT INTO economic_accounting_item_reference(operation_id,item_uid) VALUES (X'12',81)"],0),
                                ("wrong-reference-uid",["INSERT INTO critical_operation_inbox(operation_id,command_type) VALUES (X'12',17)",
                                    "INSERT INTO economic_accounting_item_reference(operation_id,item_uid) VALUES (X'12',82)"],0),
                                ("pile-effect-inbox",["INSERT INTO critical_operation_inbox(operation_id,command_type) VALUES (X'12',17)",
                                    "INSERT INTO economic_accounting_account_effect(operation_id,account_key) VALUES (X'12',UNHEX('"+
                                    exporter.account_key(bytes(16),3,81,0)+"'))"],1),
                                ("wallet-effect-not-pile",["INSERT INTO critical_operation_inbox(operation_id,command_type) VALUES (X'12',17)",
                                    "INSERT INTO economic_accounting_account_effect(operation_id,account_key) VALUES (X'12',UNHEX('"+
                                    exporter.account_key(bytes(16),1,81,0)+"'))"],0),
                                ("coin-child-ledger",["INSERT INTO economic_accounting_child(child_operation_id,domain_id) VALUES (X'12',1129269582)",
                                    "INSERT INTO item_ownership_ledger(operation_id,item_uid) VALUES (X'12',81)"],1),
                                ("wrong-child-domain",["INSERT INTO economic_accounting_child(child_operation_id,domain_id) VALUES (X'12',1129269583)",
                                    "INSERT INTO item_ownership_ledger(operation_id,item_uid) VALUES (X'12',81)"],0),
                                ("admitted-drop",["INSERT INTO critical_operation_inbox(operation_id,command_type,schema_version,status,result_code,failure_stage) VALUES (X'12',5,2,1,0,0)",
                                    "INSERT INTO economic_accounting_operation(operation_id,outcome,result_code) VALUES (X'12',1,0)",
                                    "INSERT INTO item_ownership_ledger(operation_id,item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,reason_type,reason_id) VALUES (X'12',81,1,7,0,3,10,0,6,10)"],1),
                                ("failed-drop",["INSERT INTO critical_operation_inbox(operation_id,command_type,schema_version,status,result_code,failure_stage) VALUES (X'12',5,2,1,0,1)",
                                    "INSERT INTO economic_accounting_operation(operation_id,outcome,result_code) VALUES (X'12',1,0)",
                                    "INSERT INTO item_ownership_ledger(operation_id,item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,reason_type,reason_id) VALUES (X'12',81,1,7,0,3,10,0,6,10)"],0),
                                ("wrong-drop-room-reason",["INSERT INTO critical_operation_inbox(operation_id,command_type,schema_version,status,result_code,failure_stage) VALUES (X'12',5,2,1,0,0)",
                                    "INSERT INTO economic_accounting_operation(operation_id,outcome,result_code) VALUES (X'12',1,0)",
                                    "INSERT INTO item_ownership_ledger(operation_id,item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,reason_type,reason_id) VALUES (X'12',81,1,7,0,3,10,0,6,11)"],0),
                                ("empty-coin-bytes-still-fences",["INSERT INTO item_current_owner(item_uid,coin_payload) VALUES (81,X'')"],1))
                            for label,statements,expected in branches:
                                clear_markers()
                                with marker.cursor() as cursor:
                                    for statement in statements:cursor.execute(statement)
                                marker_cut(label,expected)
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),112)
        print("SQL_SAVED_GROUND_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__":unittest.main()
