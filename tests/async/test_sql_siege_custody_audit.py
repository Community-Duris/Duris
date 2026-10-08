#!/usr/bin/env python3
"""Raw siege correspondence is evidence, never qualified runtime authority."""
import copy
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

UNQUALIFIED = {"siege_runtime_authority_unqualified":1}


def recount(native):
    native["siege_custody_coverage"] = dict(items=len(native["siege_items"]))


def packet(count=2):
    rows = [dict(item_id=400+index,room_vnum=10,parent_id=399+index if index else None,
        uid=81+index,vnum=100+index,quantity=1,weight=1,extra_flags=0,item_type=1,
        value0=0,value1=0,value2=0,value3=0) for index in range(count)]
    native = dict(siege_items=rows,items=[dict(uid=row["uid"],root=81,
        parent=row["uid"]-1 if row["parent_id"] else None,owner=[3,10,0],
        revision=2,vnum=row["vnum"],state="live",equipment_slot=0) for row in rows],coin_piles=[])
    recount(native); return native


def audit(native,limit=100):
    before = copy.deepcopy(native); reader = Reconciler(limit)
    reader.audit_siege_custody("sql_partial",native,{(row["uid"],):row for row in native["items"]})
    assert before == native and len(reader.exceptions) <= limit
    return dict(reader.counts),reader.exceptions


class SiegeCustodyAuditTests(unittest.TestCase):
    def test_retained_rows_never_grant_runtime_authority_or_invent_depth_policy(self):
        self.assertEqual(audit(packet(0))[0],{})
        for count in (1,2,33,65,66,3001):self.assertEqual(audit(packet(count))[0],UNQUALIFIED)
        native = packet(1); native["siege_items"][0].update(item_id=2**32-1,uid=2**64-1,
            weight=-2**31,extra_flags=2**64-1,room_vnum=2**31-1)
        native["items"][0].update(uid=2**64-1,root=2**64-1,owner=[3,2**31-1,0])
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        # Room custody is shared with saved ground items and modern payloads.
        native = packet(0); native["items"] = packet(1)["items"]
        self.assertEqual(audit(native)[0],{})

    def test_each_disagreement_preserves_counts_across_limits(self):
        cases = (
            ("siege_item_owner_mismatch","items",0,{"owner":[3,11,0]}),
            ("siege_item_owner_mismatch","items",0,{"owner":[3,10,1]}),
            ("siege_item_topology_mismatch","items",1,{"root":82}),
            ("siege_item_topology_mismatch","items",1,{"parent":400}),
            ("siege_item_vnum_mismatch","siege_items",1,{"vnum":9}),
            ("siege_item_equipment_mismatch","items",0,{"equipment_slot":1}),
            ("siege_projection_inactive_uid","items",0,{"state":"tombstone"}),
            ("siege_item_parent_missing","siege_items",1,{"parent_id":999}),
            ("siege_item_parent_cycle","siege_items",0,{"parent_id":401}),
            ("siege_item_parent_foreign_owner","siege_items",0,{"room_vnum":11}),
            ("siege_room_identity_unknown","siege_items",0,{"room_vnum":0}),
            ("siege_room_identity_unknown","siege_items",0,{"room_vnum":-1}),
            ("siege_legacy_uid_unknown","siege_items",0,{"uid":None}),
            ("siege_legacy_uid_unknown","siege_items",0,{"uid":0}),
            ("siege_unsupported_quantity","siege_items",0,{"quantity":None}),
            ("siege_unsupported_quantity","siege_items",0,{"quantity":0}),
            ("siege_unsupported_quantity","siege_items",0,{"quantity":2}),
            ("siege_item_vnum_invalid","siege_items",0,{"vnum":0}),
            ("siege_prototype_literal_unknown","siege_items",0,{"item_type":None}),
            ("siege_prototype_literal_unknown","siege_items",0,{"weight":None}),
            ("siege_prototype_literal_unknown","siege_items",0,{"extra_flags":None}),
        )
        for code,name,index,change in cases:
            native = packet(); native[name][index].update(change); counts = audit(native)[0]
            self.assertIn(code,counts)
            for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)

    def test_missing_admission_duplicates_and_competing_sources(self):
        native = packet(); native["items"].pop(); self.assertIn("siege_uid_unadmitted",audit(native)[0])
        native = packet(); native["siege_items"] *= 2; recount(native)
        self.assertIn("siege_duplicate_physical_row",audit(native)[0])
        native = packet(); native["siege_items"][1]["uid"] = 81
        self.assertIn("siege_duplicate_physical_uid",audit(native)[0])
        for table in ("player_items","pet_items","shop_items","corpse_items","locker_items","account_locker_items"):
            native = packet(); native[table] = [dict(uid=81)]
            self.assertIn("siege_duplicate_physical_uid",audit(native)[0])
        native = packet(); native["auction_roots"] = [dict(uid=81,claimed=False)]
        self.assertIn("siege_duplicate_physical_uid",audit(native)[0])
        native["auction_roots"][0]["claimed"] = True
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native = packet(); native["player_items"] = [dict(uid=81,item_type=20,parent_id=None)]
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native["player_items"][0]["parent_id"] = 0
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native["player_items"][0]["parent_id"] = 999
        self.assertIn("siege_duplicate_physical_uid",audit(native)[0])

    def test_coin_literals_do_not_gain_authority_from_unknown_or_aliased_payloads(self):
        native = packet(); native["siege_items"][1].update(vnum=3,item_type=20,value0=1,value1=2,value2=3,value3=4)
        native["items"][1]["vnum"] = 3
        self.assertIn("siege_coin_payload_unknown",audit(native)[0])
        native["coin_piles"] = [dict(uid=82,owner=[3,10,0],revision=2,state="live",amounts=[1,2,3,4])]
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        for value,code in ((None,"siege_coin_value_unknown"),(-1,"siege_negative_coin_value"),(9,"siege_coin_literal_mismatch")):
            bad = copy.deepcopy(native); bad["siege_items"][1]["value0"] = value; self.assertIn(code,audit(bad)[0])
        for change in ({"revision":1},{"owner":[3,11,0]},{"amounts":None}):
            bad = copy.deepcopy(native); bad["coin_piles"][0].update(change); self.assertIn("siege_coin_payload_unknown",audit(bad)[0])
        bad = copy.deepcopy(native); bad["coin_piles"] *= 2; self.assertIn("siege_coin_payload_unknown",audit(bad)[0])
        for change in ({"revision":True},{"revision":2.0},{"owner":[3.0,10,0]}):
            bad = copy.deepcopy(native); bad["coin_piles"][0].update(change)
            with self.assertRaises(SnapshotError):audit(bad,0)
        native = packet(); native["siege_items"][0].update(uid=None,item_type=20,value0=-1)
        self.assertIn("siege_negative_coin_value",audit(native)[0])
        native = packet(); native["siege_items"][0]["value0"] = None
        self.assertEqual(audit(native)[0],UNQUALIFIED)

    def test_exact_representations_and_unfiltered_bounded_export(self):
        example = packet()
        for field in example["siege_items"][0]:
            for value in (True,1.0,"1",[],{},2**64,-2**64):
                native = copy.deepcopy(example); native["siege_items"][0][field] = value
                with self.assertRaises(SnapshotError):audit(native,0)
            native = copy.deepcopy(example); native["siege_items"][0].pop(field)
            with self.assertRaises(SnapshotError):audit(native,0)
        for coverage in (None,{},dict(items=True),dict(items=2,extra=0)):
            native = packet(); native["siege_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError):audit(native,0)
        for value in (True,1.0,None,"1",2**31):
            native = packet(); native["items"][0]["vnum"] = value
            with self.assertRaises(SnapshotError):audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0],{"missing_siege_custody_coverage":1})
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError):exporter.read_siege_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        self.assertEqual(exporter.read_siege_custody(cursor),dict(siege_items=[],siege_custody_coverage=dict(items=0)))
        query = cursor.execute.call_args_list[-1]
        self.assertNotIn("JOIN",query.args[0]); self.assertNotIn("WHERE",query.args[0]); self.assertEqual(query.args[1],(MAX_ROWS+1,))
        for private in ("name","short_descr","description","action_descr"):self.assertNotIn(private,query.args[0])

    def test_complete_row_bound_and_linear_ancestry_budget(self):
        native = packet(MAX_ROWS); native["siege_items"].reverse(); start = time.monotonic()
        self.assertEqual(audit(native,0)[0],UNQUALIFIED); seconds = time.monotonic()-start
        self.assertLess(seconds,30)
        print("SIEGE_AUDIT_BUDGET "+json.dumps(dict(rows=MAX_ROWS,depth=MAX_ROWS,seconds=seconds,budget=30)),flush=True)
        native["siege_items"].append(native["siege_items"][0]); recount(native)
        with self.assertRaises(SnapshotError):audit(native,0)

    def test_global_cli_preserves_bytes_and_private_aliases(self):
        cut = clean_snapshot(); native = packet(1); native["siege_items"][0]["uid"] = 999
        cut["native"].update({key:value for key,value in native.items() if key not in ("items","coin_piles")})
        cut["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); before = path.read_bytes()
            for limit in (0,1,100):
                result = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),str(path),"--limit",str(limit)],
                    capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,1,result); self.assertEqual(result.stderr,"")
                self.assertEqual(json.loads(result.stdout)["exception_counts"],dict(UNQUALIFIED,siege_uid_unadmitted=1))
                self.assertNotIn("PRIVATE",result.stdout); self.assertEqual(path.read_bytes(),before)


from test_sql_player_custody_audit import NATIVE_SOURCE as PLAYER_SOURCE
NATIVE_SOURCE = PLAYER_SOURCE.replace('table.name == "player_data" || table.name == "item_current_owner"',
    'table.name == "item_current_owner"').replace('table.name == "player_pet_items"','table.name == "siege_items"')


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_SIEGE_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeSiegeCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_actual_siege_capture_and_coin_codec(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql
        work = ROOT/"bin/tests/sql-siege-custody"; work.mkdir(parents=True,exist_ok=True)
        source,binary = work/"native.cpp",work/"native"; source.write_text(NATIVE_SOURCE)
        command = ["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
            "-ffunction-sections","-fdata-sections","-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config","--cflags"],text=True))
        command += [str(source),"src/persistence/economic_sql_source_snapshot.c","src/player/player_snapshot_codec.c"]
        command += shlex.split(subprocess.check_output(["mysql_config","--libs"],text=True))
        command += ["-Wl,--gc-sections","-lcrypto","-o",str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=600)
        coin = bytes.fromhex(json.loads(subprocess.check_output([str(binary),"--coin","82","3"],text=True)))
        (work/"native-coin-82.bin").write_bytes(coin); observations = []
        with tempfile.TemporaryDirectory(prefix="duris-siege-audit-") as folder:
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
                                        native = exporter.read_siege_custody(cursor)
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
                                self.assertEqual(set(decoded),{"item_current_owner","item_current_owner_equipment","siege_items"})
                                physical = [dict(item_id=int(row["id"]),room_vnum=int(row["room_vnum"]),
                                    parent_id=int(row["container_id"]) if row["container_id"] else None,
                                    uid=int(row["obj_uid"]) if row["obj_uid"] else None,vnum=int(row["vnum"])) for row in decoded["siege_items"]]
                                self.assertEqual(physical,[{key:row[key] for key in ("item_id","room_vnum","parent_id","uid","vnum")} for row in native["siege_items"]])
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
                                    cursor.execute("UPDATE siege_items SET container_id=NULL"); cursor.execute("DELETE FROM siege_items")
                                    cursor.execute("UPDATE player_items SET container_id=NULL"); cursor.execute("DELETE FROM player_items")
                                    cursor.execute("DELETE FROM auction_item_custody"); cursor.execute("DELETE FROM auctions")
                                    cursor.execute("UPDATE item_current_owner SET parent_item_uid=NULL"); cursor.execute("DELETE FROM item_current_owner")
                                    cursor.execute("DELETE FROM saved_items")
                                    native = packet(count)
                                    for row in native["siege_items"]:
                                        fields = ["id","room_vnum","container_id","obj_uid","vnum","quantity","weight","extra_flags","item_type","value0","value1","value2","value3"]
                                        values = [row[key] for key in ("item_id","room_vnum","parent_id","uid","vnum","quantity","weight","extra_flags","item_type","value0","value1","value2","value3")]
                                        cursor.execute("INSERT INTO siege_items("+",".join(fields)+") VALUES ("+",".join(["%s"]*len(fields))+")",values)
                                    for row in native["items"]:
                                        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) "
                                            "VALUES (%s,%s,%s,3,10,0,2,%s,1,0)",(row["uid"],row["root"],row["parent"],row["vnum"]))
                            reset(); self.assertEqual(cut("retained-correspondence-unqualified"),UNQUALIFIED)
                            cases = (
                                ("wrong-owner","UPDATE item_current_owner SET owner_id=11 WHERE item_uid=81","siege_item_owner_mismatch"),
                                ("wrong-context","UPDATE item_current_owner SET owner_context_id=1 WHERE item_uid=81","siege_item_owner_mismatch"),
                                ("unadmitted-child","DELETE FROM item_current_owner WHERE item_uid=82","siege_uid_unadmitted"),
                                ("wrong-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","siege_item_topology_mismatch"),
                                ("wrong-parent","UPDATE item_current_owner SET parent_item_uid=NULL WHERE item_uid=82","siege_item_topology_mismatch"),
                                ("cross-room-parent","UPDATE siege_items SET room_vnum=11 WHERE id=400","siege_item_parent_foreign_owner"),
                                ("cycle","UPDATE siege_items SET container_id=401 WHERE id=400","siege_item_parent_cycle"),
                                ("duplicate-uid","UPDATE siege_items SET obj_uid=81 WHERE id=401","siege_duplicate_physical_uid"),
                                ("legacy-null-uid","UPDATE siege_items SET obj_uid=NULL WHERE id=400","siege_legacy_uid_unknown"),
                                ("legacy-zero-uid","UPDATE siege_items SET obj_uid=0 WHERE id=400","siege_legacy_uid_unknown"),
                                ("inactive-retained","UPDATE item_current_owner SET state=2 WHERE item_uid=81","siege_projection_inactive_uid"),
                                ("vnum","UPDATE siege_items SET vnum=9 WHERE id=401","siege_item_vnum_mismatch"),
                                ("equipment","UPDATE item_current_owner SET equipment_slot=1 WHERE item_uid=81","siege_item_equipment_mismatch"),
                                ("quantity-null","UPDATE siege_items SET quantity=NULL WHERE id=400","siege_unsupported_quantity"),
                                ("quantity-two","UPDATE siege_items SET quantity=2 WHERE id=400","siege_unsupported_quantity"),
                                ("weight-null","UPDATE siege_items SET weight=NULL WHERE id=400","siege_prototype_literal_unknown"),
                                ("type-null","UPDATE siege_items SET item_type=NULL WHERE id=400","siege_prototype_literal_unknown"),
                                ("room-zero","UPDATE siege_items SET room_vnum=0 WHERE id=400","siege_room_identity_unknown"),
                                ("room-negative","UPDATE siege_items SET room_vnum=-1 WHERE id=400","siege_room_identity_unknown"),
                                ("vnum-zero","UPDATE siege_items SET vnum=0 WHERE id=400","siege_item_vnum_invalid"),
                                ("orphan-parent","UPDATE siege_items SET container_id=999 WHERE id=401","siege_item_parent_missing"),
                            )
                            for label,statement,code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-"):cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try:cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-"):cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                self.assertIn(code,cut(label))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE siege_items SET vnum=3,item_type=20,value0=1,value1=2,value2=3,value3=4 WHERE id=401")
                                cursor.execute("UPDATE item_current_owner SET vnum=3,coin_payload=%s WHERE item_uid=82",(coin,))
                            self.assertEqual(cut("genuine-nested-coin"),UNQUALIFIED)
                            with owner.cursor() as cursor:cursor.execute("UPDATE siege_items SET value0=9 WHERE id=401")
                            self.assertIn("siege_coin_literal_mismatch",cut("genuine-nested-coin-mismatch"))
                            with owner.cursor() as cursor:cursor.execute("UPDATE siege_items SET obj_uid=NULL,value0=-1 WHERE id=401")
                            self.assertIn("siege_negative_coin_value",cut("uidless-negative-coin"))
                            reset(66); self.assertEqual(cut("no-invented-loader-depth"),UNQUALIFIED)
                            reset()
                            with owner.cursor() as cursor:cursor.execute("INSERT INTO player_items(pid,obj_uid,vnum) VALUES (7,81,100)")
                            self.assertIn("siege_duplicate_physical_uid",cut("competing-player-physical-uid"))
                            with owner.cursor() as cursor:cursor.execute("UPDATE player_items SET item_type=20")
                            self.assertEqual(cut("wallet-root-excluded"),UNQUALIFIED)
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO player_items(id,pid,obj_uid,vnum,item_type) VALUES (900,7,999,100,15)")
                                cursor.execute("UPDATE player_items SET container_id=900 WHERE obj_uid=81")
                            self.assertIn("siege_duplicate_physical_uid",cut("nested-player-money-competes"))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("INSERT INTO auctions(id,seller_pid,status,auction_revision,custody_state,quantity,obj_vnum,obj_blob_str) "
                                    "VALUES (99,7,'OPEN',1,1,1,100,'synthetic-retained-payload')")
                                cursor.execute("INSERT INTO auction_item_custody(auction_id,slot,item_uid,item_revision,vnum,obj_blob) "
                                    "VALUES (99,0,81,1,100,%s)",(coin,))
                            self.assertIn("siege_duplicate_physical_uid",cut("unclaimed-auction-competes"))
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE auction_item_custody SET claim_pid=7,claim_operation_id=UNHEX(%s),claimed_at=CURRENT_TIMESTAMP(6)",("12"*16,))
                            self.assertEqual(cut("claimed-auction-history-does-not-compete"),UNQUALIFIED)
                            reset(0); self.assertEqual(cut("empty-retained-projection"),{})
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor:cursor.execute("UPDATE siege_items SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),64)
        print("SQL_SIEGE_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__":unittest.main()
