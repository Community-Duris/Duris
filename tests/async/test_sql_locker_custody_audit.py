#!/usr/bin/env python3
"""SQL locker observations are independent evidence, never mutation authority."""
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
from test_reconcile_economy_accounting import clean_snapshot, bind_original_plans

NAMES = ("lockers","private_chests","locker_items","account_lockers","locker_chests","account_locker_items")


def recount(native):
    native["locker_custody_coverage"] = {name:len(native[name]) for name in NAMES}


def packet(count=2,account=False):
    native = dict(lockers=[dict(locker_id=5,racewar=0,owner_pid=7,owner_assoc_id=None)],
        private_chests=[dict(chest_id=9,locker_id=5,is_public=1)],
        account_lockers=[dict(locker_id=5,racewar=0)],locker_chests=[dict(chest_id=9,locker_id=5,is_public=1)],
        locker_items=[],account_locker_items=[],items=[],coin_piles=[])
    table = "account_locker_items" if account else "locker_items"
    for index in range(count):
        row = dict(item_id=400+index,chest_id=9,parent_id=399+index if index else None,uid=81+index,
            vnum=100+index,quantity=1,weight=1,extra_flags=0,value0=0,value1=0,value2=0,value3=0)
        if not account:row.update(locker_id=5,item_type=1)
        native[table].append(row)
        native["items"].append(dict(uid=row["uid"],root=81,parent=row["uid"]-1 if index else None,
            owner=[5,9,0] if account else [5,5,9],revision=2,vnum=row["vnum"],state="live",equipment_slot=0))
    recount(native); return native


def audit(native,limit=100):
    before = copy.deepcopy(native); reader = Reconciler(limit)
    reader.audit_locker_custody("sql_partial",native,{(row["uid"],):row for row in native["items"]})
    assert before == native and len(reader.exceptions) <= limit
    return dict(reader.counts),reader.exceptions


class LockerCustodyAuditTests(unittest.TestCase):
    def test_numeric_namespaces_historical_account_identity_and_depth(self):
        for count in (0,1,2,33,65): self.assertEqual(audit(packet(count))[0],{})
        self.assertIn("locker_item_depth_exceeds_native_limit",audit(packet(66))[0])
        native = packet(account=True)
        self.assertEqual(audit(native)[0],{"locker_account_runtime_authority_unqualified":1})
        native["items"][0]["owner"] = [5,5,9]
        self.assertIn("locker_item_owner_mismatch",audit(native)[0])
        native = packet(1); native["lockers"][0].update(locker_id=2**32-1,racewar=-128,owner_pid=None,owner_assoc_id=2**31-1)
        native["private_chests"][0].update(chest_id=2**32-1,locker_id=2**32-1)
        native["locker_items"][0].update(item_id=2**32-1,locker_id=2**32-1,chest_id=2**32-1,uid=2**64-1,weight=-2**31,extra_flags=2**64-1)
        native["items"][0].update(uid=2**64-1,root=2**64-1,owner=[5,2**32-1,2**32-1])
        self.assertEqual(audit(native)[0],{})

    def test_each_disagreement_keeps_global_counts_at_all_limits(self):
        cases = (
            ("locker_item_owner_mismatch","items",0,{"owner":[5,9,5]}),
            ("locker_item_topology_mismatch","items",1,{"root":82}),
            ("locker_item_topology_mismatch","items",1,{"parent":400}),
            ("locker_item_vnum_mismatch","locker_items",1,{"vnum":9}),
            ("locker_item_equipment_mismatch","items",0,{"equipment_slot":1}),
            ("locker_projection_inactive_uid","items",0,{"state":"tombstone"}),
            ("locker_item_parent_missing","locker_items",1,{"parent_id":999}),
            ("locker_item_parent_cycle","locker_items",0,{"parent_id":401}),
            ("locker_item_parent_foreign_owner","locker_items",0,{"chest_id":10}),
            ("locker_item_parent_foreign_owner","locker_items",0,{"locker_id":6}),
            ("locker_item_ambiguous_chest","locker_items",0,{"locker_id":6}),
            ("locker_item_missing_chest","locker_items",0,{"chest_id":999}),
            ("locker_legacy_chest_unknown","locker_items",0,{"chest_id":None}),
            ("locker_legacy_uid_unknown","locker_items",0,{"uid":None}),
            ("locker_unsupported_quantity","locker_items",0,{"quantity":None}),
            ("locker_unsupported_quantity","locker_items",0,{"quantity":2}),
            ("locker_item_vnum_invalid","locker_items",0,{"vnum":0}),
            ("locker_item_type_unknown","locker_items",0,{"item_type":None}),
            ("locker_prototype_literal_unknown","locker_items",0,{"weight":None}),
            ("locker_prototype_literal_unknown","locker_items",0,{"extra_flags":None}),
            ("locker_chest_policy_unknown","private_chests",0,{"is_public":None}),
            ("locker_chest_missing_locker","private_chests",0,{"locker_id":6}),
            ("locker_owner_identity_unknown","lockers",0,{"owner_pid":None}),
            ("locker_owner_identity_unknown","lockers",0,{"owner_assoc_id":8}),
            ("locker_side_unknown","lockers",0,{"racewar":None}),
            ("locker_side_unknown","account_lockers",0,{"racewar":None}),
        )
        for code,name,index,change in cases:
            native = packet(); native[name][index].update(change); counts = audit(native)[0]; self.assertIn(code,counts)
            for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)

    def test_duplicates_orphans_and_reverse_presence_keep_namespaces_separate(self):
        cases = []
        native = packet(); native["locker_items"].pop(); cases.append((native,"locker_uid_missing_physical"))
        native = packet(); native["items"].pop(); cases.append((native,"locker_uid_unadmitted"))
        for table,code in (("lockers","locker_duplicate_id"),("private_chests","locker_duplicate_chest_id"),("locker_items","locker_duplicate_physical_row")):
            native = packet(); native[table] *= 2; cases.append((native,code))
        native = packet(); native["locker_items"][1]["uid"] = 81; cases.append((native,"locker_duplicate_physical_uid"))
        native = packet(); native["private_chests"].append(dict(chest_id=10,locker_id=5,is_public=1)); cases.append((native,"locker_public_chest_ambiguous"))
        native = packet(); native["account_locker_items"] = packet(account=True)["account_locker_items"]
        cases.append((native,"locker_duplicate_physical_uid"))
        for native,code in cases:
            recount(native); counts = audit(native)[0]; self.assertIn(code,counts)
            for limit in (0,1,100):self.assertEqual(audit(native,limit)[0],counts)
        native = packet(); native["private_chests"].clear(); recount(native)
        self.assertIn("locker_item_missing_chest",audit(native)[0])

    def test_coin_literals_unknown_prototypes_and_exact_inline_identity(self):
        for account in (False,True):
            native = packet(account=account); table = "account_locker_items" if account else "locker_items"
            native[table][1].update(vnum=3,value0=1,value1=2,value2=3,value3=4)
            native["items"][1]["vnum"] = 3
            base = {"locker_account_runtime_authority_unqualified":1} if account else {}
            self.assertIn("locker_coin_payload_unknown",audit(native)[0])
            native["coin_piles"] = [dict(uid=82,owner=native["items"][1]["owner"],revision=2,state="live",amounts=[1,2,3,4])]
            self.assertEqual(audit(native)[0],base)
            for value,code in ((None,"locker_coin_value_unknown"),(-1,"locker_negative_coin_value"),(9,"locker_coin_literal_mismatch")):
                bad = copy.deepcopy(native); bad[table][1]["value0"] = value; self.assertIn(code,audit(bad)[0])
            for change in ({"revision":1},{"owner":[5,999,0]},{"amounts":None}):
                bad = copy.deepcopy(native); bad["coin_piles"][0].update(change); self.assertIn("locker_coin_payload_unknown",audit(bad)[0])
            bad = copy.deepcopy(native); bad["coin_piles"] *= 2; self.assertIn("locker_coin_payload_unknown",audit(bad)[0])
            for change in ({"revision":True},{"revision":2.0},{"owner":[5.0,9,0]}):
                bad = copy.deepcopy(native); bad["coin_piles"][0].update(change)
                with self.assertRaises(SnapshotError):audit(bad,0)
        native = packet(); native["locker_items"][1].update(item_type=20,value0=-1)
        self.assertIn("locker_negative_coin_value",audit(native)[0])
        native["locker_items"][1]["uid"] = None
        self.assertIn("locker_negative_coin_value",audit(native)[0])
        native = packet(); native["locker_items"][0]["value0"] = None
        self.assertEqual(audit(native)[0],{})  # ordinary prototype fallback is not coin authority

    def test_exact_representations_coverage_and_unfiltered_export_preflight(self):
        for account in (False,True):
            example = packet(account=account)
            for name in NAMES:
                for field in example[name][0] if example[name] else ():
                    for value in (True,1.0,"1",[],{},2**64,-2**64):
                        native = copy.deepcopy(example); native[name][0][field] = value
                        with self.assertRaises(SnapshotError):audit(native,0)
                    native = copy.deepcopy(example); native[name][0].pop(field)
                    with self.assertRaises(SnapshotError):audit(native,0)
        for coverage in (None,{},dict(packet()["locker_custody_coverage"],lockers=True)):
            native = packet(); native["locker_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError):audit(native,0)
        for value in (True,1.0,None,"1",2**31):
            native = packet(); native["items"][0]["vnum"] = value
            with self.assertRaises(SnapshotError):audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0],{"missing_locker_custody_coverage":1})
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError):exporter.read_locker_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        expected = {name:[] for name in NAMES}; expected["locker_custody_coverage"] = {name:0 for name in NAMES}
        self.assertEqual(exporter.read_locker_custody(cursor),expected)
        for call in cursor.execute.call_args_list[1:]:
            self.assertNotIn("JOIN",call.args[0]); self.assertNotIn("WHERE",call.args[0]); self.assertEqual(call.args[1],(MAX_ROWS+1,))
            for private in ("account_name","locker_name","chest_name","password_hash","keyword","sort_config"):
                self.assertNotIn(private,call.args[0])

    def test_maximum_projection_with_65_deep_forests_keeps_budget_and_input(self):
        native = packet(0)
        for index in range(99900):
            offset = index%65; root = 81+index-offset
            row = dict(packet(1)["locker_items"][0],item_id=400+index,uid=81+index,parent_id=399+index if offset else None)
            native["locker_items"].append(row)
            native["items"].append(dict(uid=row["uid"],root=root,parent=row["uid"]-1 if offset else None,
                owner=[5,5,9],revision=2,vnum=row["vnum"],state="live",equipment_slot=0))
        native["locker_items"].reverse(); recount(native); start = time.monotonic()
        self.assertEqual(audit(native,0)[0],{}); seconds = time.monotonic()-start; self.assertLess(seconds,30)
        print("LOCKER_AUDIT_BUDGET "+json.dumps(dict(rows=99900,depth=65,seconds=seconds,budget=30)),flush=True)

    def test_global_cli_is_private_bounded_and_preserves_original_bytes(self):
        cut = clean_snapshot(); native = packet(1); native["locker_items"][0]["vnum"] = 1
        cut["native"].update({key:value for key,value in native.items() if key not in ("items","coin_piles")})
        cut["native"]["items"][0].update(owner=[5,5,9],vnum=1); cut["ownership_events"][0]["owner"] = [5,5,9]
        bind_original_plans(cut); cut["native"]["locker_items"][0]["uid"] = 999; cut["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); before = path.read_bytes()
            for limit in (0,1,100):
                result = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),str(path),"--limit",str(limit)],
                    capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,1,result); self.assertEqual(result.stderr,"")
                self.assertEqual(json.loads(result.stdout)["exception_counts"],{"locker_uid_unadmitted":1,"locker_uid_missing_physical":1})
                self.assertNotIn("PRIVATE",result.stdout); self.assertEqual(path.read_bytes(),before)
        cut["native"]["locker_items"][0]["uid"] = 81; cut["native"]["lockers"][0]["racewar"] = None
        before = copy.deepcopy(cut)
        for limit in (0,1,100):self.assertEqual(Reconciler(limit).audit(cut)["exception_counts"],{"locker_side_unknown":1})
        self.assertEqual(cut,before)


from test_sql_player_custody_audit import NATIVE_SOURCE as PLAYER_SOURCE
NATIVE_SOURCE = PLAYER_SOURCE.replace('table.name == "player_data" || table.name == "item_current_owner"',
    'table.name == "item_current_owner"').replace(
    '    for (const auto &table : result.item_sources)\n'
    '        if (table.name == "player_pet_items") emit(table, table.name);\n', '')


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_LOCKER_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeLockerCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_actual_current_capture_and_raw_locker_cuts(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql
        work = ROOT/"bin/tests/sql-locker-custody"; work.mkdir(parents=True,exist_ok=True)
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
        with tempfile.TemporaryDirectory(prefix="duris-locker-audit-") as folder:
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
                            cursor.execute("INSERT INTO lockers(id,locker_name,owner_pid) VALUES (5,'PRIVATE-locker',7),(6,'PRIVATE-other',8)")
                            cursor.execute("INSERT INTO private_chests(id,locker_id,chest_name,is_public,password_hash) VALUES "
                                "(9,5,'PRIVATE-public',1,'PRIVATE-hash'),(10,5,'PRIVATE-private',0,NULL),(11,6,'PRIVATE-other',1,NULL)")
                            cursor.execute("INSERT INTO accounts(account_name) VALUES ('PRIVATE-account')")
                            cursor.execute("INSERT INTO account_lockers(id,account_name) VALUES (5,'PRIVATE-account')")
                            cursor.execute("INSERT INTO locker_chests(id,locker_id,keyword,is_public,keyword_hash) VALUES (9,5,'PRIVATE-chest',1,'PRIVATE-hash')")
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
                                        native = exporter.read_locker_custody(cursor)
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
                                registry = {name:columns.split(",") for group,_,_,specs in CAPTURE_REGISTRIES if group=="tables" for name,columns,_ in specs}
                                registry["item_current_owner_equipment"] = ["item_uid","equipment_slot"]
                                decoded = {name:[dict(zip(registry[name],[None if value is None else bytes.fromhex(value) for value in row])) for row in rows]
                                    for name,rows in tables.items()}
                                self.assertEqual(set(decoded),{"item_current_owner","item_current_owner_equipment"})
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
                            def reset(count=2,account=False):
                                with owner.cursor() as cursor:
                                    for table in ("locker_items","account_locker_items"):
                                        cursor.execute("UPDATE "+table+" SET container_id=NULL"); cursor.execute("DELETE FROM "+table)
                                    cursor.execute("UPDATE item_current_owner SET parent_item_uid=NULL"); cursor.execute("DELETE FROM item_current_owner")
                                    cursor.execute("UPDATE lockers SET owner_pid=7,owner_assoc_id=NULL WHERE id=5")
                                    cursor.execute("UPDATE lockers SET racewar=0 WHERE id=5")
                                    cursor.execute("UPDATE private_chests SET is_public=IF(id=10,0,1)")
                                    native = packet(count,account); table = "account_locker_items" if account else "locker_items"
                                    for row in native[table]:
                                        fields = ["id","chest_id","container_id","obj_uid","vnum","quantity","weight","extra_flags","value0","value1","value2","value3"]
                                        values = [row[key] for key in ("item_id","chest_id","parent_id","uid","vnum","quantity","weight","extra_flags","value0","value1","value2","value3")]
                                        if not account:fields += ["locker_id","item_type"]; values += [5,1]
                                        cursor.execute("INSERT INTO "+table+"("+",".join(fields)+") VALUES ("+",".join(["%s"]*len(fields))+")",values)
                                    for row in native["items"]:
                                        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) "
                                            "VALUES (%s,%s,%s,5,%s,%s,2,%s,1,0)",(row["uid"],row["root"],row["parent"],*row["owner"][1:],row["vnum"]))
                            reset(); self.assertEqual(cut("healthy-distinct-locker-and-chest-ids"),{})
                            cases = (
                                ("wrong-owner","UPDATE item_current_owner SET owner_id=9,owner_context_id=5 WHERE item_uid=81","locker_item_owner_mismatch"),
                                ("wrong-context","UPDATE item_current_owner SET owner_context_id=10 WHERE item_uid=81","locker_item_owner_mismatch"),
                                ("missing-physical","DELETE FROM locker_items WHERE id=401","locker_uid_missing_physical"),
                                ("unadmitted-child","DELETE FROM item_current_owner WHERE item_uid=82","locker_uid_unadmitted"),
                                ("wrong-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","locker_item_topology_mismatch"),
                                ("cross-chest-parent","UPDATE locker_items SET chest_id=10 WHERE id=400","locker_item_parent_foreign_owner"),
                                ("cross-locker-parent","UPDATE locker_items SET locker_id=6 WHERE id=400","locker_item_parent_foreign_owner"),
                                ("cycle","UPDATE locker_items SET container_id=401 WHERE id=400","locker_item_parent_cycle"),
                                ("duplicate-uid","UPDATE locker_items SET obj_uid=81 WHERE id=401","locker_duplicate_physical_uid"),
                                ("legacy-null-uid","UPDATE locker_items SET obj_uid=NULL WHERE id=400","locker_legacy_uid_unknown"),
                                ("legacy-null-chest","UPDATE locker_items SET chest_id=NULL WHERE id=400","locker_legacy_chest_unknown"),
                                ("inactive-retained","UPDATE item_current_owner SET state=2 WHERE item_uid=81","locker_projection_inactive_uid"),
                                ("vnum","UPDATE locker_items SET vnum=9 WHERE id=401","locker_item_vnum_mismatch"),
                                ("equipment","UPDATE item_current_owner SET equipment_slot=1 WHERE item_uid=81","locker_item_equipment_mismatch"),
                                ("quantity-null","UPDATE locker_items SET quantity=NULL WHERE id=400","locker_unsupported_quantity"),
                                ("quantity-two","UPDATE locker_items SET quantity=2 WHERE id=400","locker_unsupported_quantity"),
                                ("weight-null","UPDATE locker_items SET weight=NULL WHERE id=400","locker_prototype_literal_unknown"),
                                ("type-null","UPDATE locker_items SET item_type=NULL WHERE id=400","locker_item_type_unknown"),
                                ("unknown-owner","UPDATE lockers SET owner_pid=NULL WHERE id=5","locker_owner_identity_unknown"),
                                ("ambiguous-owner","UPDATE lockers SET owner_assoc_id=8 WHERE id=5","locker_owner_identity_unknown"),
                                ("unknown-side","UPDATE lockers SET racewar=NULL WHERE id=5","locker_side_unknown"),
                                ("duplicate-public","UPDATE private_chests SET is_public=1 WHERE id=10","locker_public_chest_ambiguous"),
                                ("unknown-public","UPDATE private_chests SET is_public=NULL WHERE id=9","locker_chest_policy_unknown"),
                                ("orphan-chest","UPDATE locker_items SET chest_id=999 WHERE id=401","locker_item_missing_chest"),
                                ("orphan-parent","UPDATE locker_items SET container_id=999 WHERE id=401","locker_item_parent_missing"),
                            )
                            for label,statement,code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-"):cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try:cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-"):cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                self.assertIn(code,cut(label))
                            reset(account=True); self.assertEqual(cut("historical-account-context-zero"),{"locker_account_runtime_authority_unqualified":1})
                            with owner.cursor() as cursor:cursor.execute("UPDATE item_current_owner SET owner_id=5,owner_context_id=9 WHERE item_uid=81")
                            self.assertIn("locker_item_owner_mismatch",cut("account-id-namespace-confusion"))
                            reset()
                            with owner.cursor() as cursor:cursor.execute("INSERT INTO account_locker_items(id,chest_id,obj_uid,vnum) VALUES (400,9,81,100)")
                            self.assertIn("locker_duplicate_physical_uid",cut("cross-namespace-uid-duplicate"))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE locker_items SET vnum=3,item_type=20,value0=1,value1=2,value2=3,value3=4 WHERE id=401")
                                cursor.execute("UPDATE item_current_owner SET vnum=3,coin_payload=%s WHERE item_uid=82",(coin,))
                            self.assertEqual(cut("genuine-nested-coin"),{})
                            with owner.cursor() as cursor:cursor.execute("UPDATE locker_items SET value0=9 WHERE id=401")
                            self.assertIn("locker_coin_literal_mismatch",cut("genuine-nested-coin-mismatch"))
                            reset(65); self.assertEqual(cut("loader-source-65-deep-bound"),{})
                            reset(66); self.assertIn("locker_item_depth_exceeds_native_limit",cut("loader-source-over-depth-bound"))
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor:cursor.execute("UPDATE locker_items SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),66)
        print("SQL_LOCKER_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__":unittest.main()
