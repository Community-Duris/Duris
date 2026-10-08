#!/usr/bin/env python3
"""Independent player/pet SQL projection controls, never mutation authority."""
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
sys.path[:0] = [str(ROOT/"scripts"),str(ROOT/"tests/async")]
import economic_sql_audit_snapshot as exporter
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError
from test_reconcile_economy_accounting import clean_snapshot


def packet(depth=2):
    rows = [dict(item_id=400+i,pid=7,parent_id=399+i if i else None,uid=81+i,
        vnum=100+i,equipment_slot=0,quantity=1,item_type=1,
        value0=0,value1=0,value2=0,value3=0) for i in range(depth)]
    pet_rows = [dict(item_id=400+i,pet_id=51,parent_id=399+i if i else None,uid=9001+i,
        vnum=200+i,equipment_slot=0,quantity=1,item_type=1,
        value0=0,value1=0,value2=0,value3=0) for i in range(2)]
    items = [dict(uid=row["uid"],root=rows[0]["uid"],parent=row["uid"]-1 if i else None,
        owner=[1,7,0],revision=2,vnum=row["vnum"],state="live",equipment_slot=0) for i,row in enumerate(rows)]
    items += [dict(uid=row["uid"],root=9001,parent=row["uid"]-1 if i else None,
        owner=[11,900,7],revision=2,vnum=row["vnum"],state="live",equipment_slot=0) for i,row in enumerate(pet_rows)]
    return dict(player_ids=[dict(pid=7)],player_pets=[dict(pet_id=51,pid=7,pet_uid=900)],
        player_items=rows,pet_items=pet_rows,items=items,coin_piles=[],
        player_custody_coverage=dict(players=1,pets=1,items=depth,pet_items=2))


def recount(native):
    native["player_custody_coverage"] = dict(zip(("players","pets","items","pet_items"),
        (len(native[name]) for name in ("player_ids","player_pets","player_items","pet_items"))))


def audit(native,limit=100):
    before = copy.deepcopy(native); reader = Reconciler(limit)
    reader.audit_player_custody("sql_partial",native,{(row["uid"],):row for row in native["items"]})
    assert before == native and len(reader.exceptions) <= limit
    return dict(reader.counts),reader.exceptions


class PlayerCustodyAuditTests(unittest.TestCase):
    def test_player_pet_and_legacy_owners_row_ids_limits_and_widths(self):
        for depth in (1,2,32):
            native = packet(depth); self.assertEqual(audit(native)[0],{})
        for pet_uid in (None,0,900,2**64-1):
            for legacy in (False,True):
                native = packet(); native["player_pets"][0]["pet_uid"] = pet_uid
                for row in native["items"][2:]: row["owner"] = [1,7,0] if legacy or not pet_uid else [11,pet_uid,7]
                self.assertEqual(audit(native)[0],{})
        native = packet(1); native["player_items"][0].update(uid=2**64-1,item_id=2**32-1,equipment_slot=43)
        native["items"][0].update(uid=2**64-1,root=2**64-1,equipment_slot=43,revision=2**64-1)
        self.assertEqual(audit(native)[0],{})

    def test_each_disagreement_and_global_output_limits(self):
        changes = (
            ("player_projection_stale_owner","items",0,{"owner":[1,8,0]}),
            ("player_projection_stale_owner","items",2,{"owner":[11,900,8]}),
            ("player_projection_inactive_uid","items",0,{"state":"tombstone"}),
            ("player_projection_inactive_uid","items",2,{"state":"quarantined"}),
            ("player_item_topology_mismatch","items",1,{"root":82}),
            ("player_item_topology_mismatch","items",3,{"parent":400}),
            ("player_item_vnum_mismatch","player_items",0,{"vnum":9}),
            ("player_item_equipment_mismatch","pet_items",0,{"equipment_slot":1}),
            ("player_item_equipment_invalid","player_items",1,{"equipment_slot":1}),
            ("player_item_equipment_invalid","player_items",0,{"equipment_slot":None}),
            ("player_item_quantity_invalid","player_items",0,{"quantity":None}),
            ("player_item_quantity_invalid","player_items",0,{"quantity":2}),
            ("player_item_vnum_invalid","pet_items",0,{"vnum":0}),
            ("player_item_parent_missing","player_items",1,{"parent_id":999}),
            ("player_item_parent_cycle","pet_items",0,{"parent_id":401}),
            ("player_legacy_uid_unknown","pet_items",0,{"uid":None}),
            ("player_legacy_uid_unknown","player_items",0,{"uid":0}),
            ("player_item_missing_pet","pet_items",0,{"pet_id":52}),
            ("player_item_missing_player","player_items",0,{"pid":8}),
        )
        for code,collection,index,change in changes:
            native = packet(); native[collection][index].update(change); counts = audit(native)[0]
            self.assertIn(code,counts)
            for limit in (0,1,100):
                with self.subTest(code=code,limit=limit): self.assertEqual(audit(native,limit)[0],counts)

    def test_cross_table_duplicates_reverse_coverage_foreign_parents_and_bounds(self):
        cases = []
        native = packet(); native["player_items"].pop(); cases.append((native,"player_uid_missing_physical"))
        native = packet(); native["items"].pop(); cases.append((native,"player_uid_unadmitted"))
        native = packet(); native["pet_items"][0]["uid"] = 81; cases.append((native,"player_duplicate_physical_uid"))
        native = packet(); native["player_items"] *= 2; cases.append((native,"player_duplicate_physical_row"))
        native = packet(); native["player_pets"] *= 2; cases.append((native,"player_duplicate_pet_row"))
        native = packet(); native["player_pets"].append(dict(pet_id=52,pid=7,pet_uid=900)); cases.append((native,"player_duplicate_pet_uid"))
        native = packet(); native["player_ids"] *= 2; cases.append((native,"player_duplicate_pid"))
        native = packet(); native["player_items"][0]["pid"] = 8; cases.append((native,"player_item_parent_foreign_owner"))
        native = packet(); native["player_pets"][0]["pid"] = 8; cases.append((native,"player_pet_missing_player"))
        cases.append((packet(33),"player_item_depth_exceeds_native_limit"))
        native = packet(1); native["player_items"] = [dict(native["player_items"][0],item_id=i+1,uid=None) for i in range(4097)]
        cases.append((native,"player_item_count_exceeds_native_limit"))
        native = packet(); native["player_pets"] = [dict(pet_id=i+1,pid=7,pet_uid=0) for i in range(65)]
        cases.append((native,"player_pet_count_exceeds_native_limit"))
        native = packet(); native["player_items"][1].update(parent_id=None,equipment_slot=1)
        native["player_items"][0]["equipment_slot"] = 1
        cases.append((native,"player_duplicate_equipment_slot"))
        for native,code in cases:
            recount(native); counts = audit(native)[0]; self.assertIn(code,counts)
            for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)

    def test_wallet_root_exclusion_precedes_uid_validation_and_keeps_nested_money(self):
        native = packet()
        for name,group in (("player_items","pid"),("pet_items","pet_id")):
            # Malformed item identity/literals are opaque under the wallet rule.
            native[name].append(dict(item_id=777,**{group:7 if group=="pid" else 51},
                parent_id=None,item_type=20,uid=True,vnum="opaque",quantity=None))
        recount(native); self.assertEqual(audit(native)[0],{})
        for item_type in (None,1):
            value = copy.deepcopy(native); value["player_items"][-1]["item_type"] = item_type
            with self.assertRaises(SnapshotError): audit(value)
        for name in ("player_items","pet_items"):
            value = packet(); row = value[name][1]; row.update(item_type=20,value0=-1)
            self.assertIn("player_negative_coin_value",audit(value)[0])
            row["value0"] = None; self.assertIn("player_coin_literal_unknown",audit(value)[0])

    def test_inline_coin_reconstruction_never_invents_missing_payload_authority(self):
        native = packet(); item = native["items"][1]
        native["coin_piles"] = [dict(uid=item["uid"],owner=item["owner"],revision=2,state="live",amounts=[1,2,3,4])]
        native["player_items"].pop(); recount(native); self.assertEqual(audit(native)[0],{})
        for change in ({"amounts":None},{"revision":1},{"owner":[1,8,0]},{"state":"tombstone"}):
            value = copy.deepcopy(native); value["coin_piles"][0].update(change)
            self.assertIn("player_uid_missing_physical",audit(value)[0])
        value = copy.deepcopy(native); value["coin_piles"] *= 2
        self.assertIn("player_uid_missing_physical",audit(value)[0])
        for change in ({"revision":True},{"revision":2.0},{"revision":2**64},
                {"owner":[True,7,0]},{"owner":[1,7.0,0]},{"owner":[1,7,False]}):
            value = copy.deepcopy(native); value["coin_piles"][0].update(change)
            with self.assertRaises(SnapshotError): audit(value,0)
        cut = clean_snapshot(); evidence = packet(1)
        evidence["player_items"].clear(); evidence["pet_items"].clear(); evidence["player_pets"].clear(); recount(evidence)
        cut["native"].update({key:value for key,value in evidence.items() if key not in ("items","coin_piles")})
        item = cut["native"]["items"][0]; item["vnum"] = 1
        cut["native"]["coin_piles"] = [dict(uid=item["uid"],owner=[True,7,0],
            revision=float(item["revision"]),state="live",amounts=[1,2,3,4])]
        before = copy.deepcopy(cut)
        for limit in (0,1,100):
            with self.assertRaises(SnapshotError): Reconciler(limit).audit(cut)
        self.assertEqual(cut,before)
        value = packet(); value["pet_items"][1].update(item_type=20,value0=4,value1=3,value2=2,value3=1)
        item = value["items"][-1]; value["coin_piles"] = [dict(uid=item["uid"],owner=item["owner"],revision=2,state="live",amounts=[1,2,3,4])]
        self.assertIn("player_coin_literal_mismatch",audit(value)[0])
        value["pet_items"].pop(); recount(value)
        self.assertIn("player_uid_missing_physical",audit(value)[0])

    def test_exact_representation_coverage_bounds_and_exporter_preflight(self):
        for collection,fields in (("player_ids",("pid",)),("player_pets",("pet_id","pid","pet_uid")),
                ("player_items",("item_id","pid","parent_id","uid","vnum","equipment_slot","quantity","item_type","value0"))):
            for field in fields:
                for value in (True,1.0,"1",[],{},-2**32,2**64):
                    native = packet(); native[collection][0][field] = value
                    with self.assertRaises(SnapshotError): audit(native,0)
                native = packet(); native[collection][0].pop(field)
                with self.assertRaises(SnapshotError): audit(native,0)
        for coverage in (None,{},dict(players=True,pets=1,items=2,pet_items=2)):
            native = packet(); native["player_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError): audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0],{"missing_player_custody_coverage":1})
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError): exporter.read_player_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        self.assertEqual(exporter.read_player_custody(cursor),dict(player_ids=[],player_pets=[],player_items=[],pet_items=[],
            player_custody_coverage=dict(players=0,pets=0,items=0,pet_items=0)))
        for call in cursor.execute.call_args_list[1:]:
            self.assertNotIn("JOIN",call.args[0]); self.assertNotIn("WHERE",call.args[0])
            self.assertEqual(call.args[1],(MAX_ROWS+1,))

    def test_global_cli_is_bounded_private_and_read_only(self):
        cut = clean_snapshot(); native = packet(1); native["pet_items"].clear(); native["player_pets"].clear()
        native["player_items"][0]["vnum"] = 1; native["player_items"][0]["equipment_slot"] = 2
        recount(native); cut["native"].update({name:value for name,value in native.items() if name not in ("items","coin_piles")})
        cut["native"]["items"][0]["vnum"] = 1; cut["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); before = path.read_bytes()
            for limit in (0,1,100):
                value = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),str(path),
                    "--limit",str(limit)],capture_output=True,text=True,timeout=30)
                self.assertEqual(value.returncode,1,value); self.assertEqual(value.stderr,"")
                self.assertEqual(json.loads(value.stdout)["exception_counts"],{"player_item_equipment_mismatch":1})
                self.assertNotIn("PRIVATE",value.stdout); self.assertEqual(path.read_bytes(),before)


from test_sql_auction_custody_audit import NATIVE_SOURCE as AUCTION_SOURCE
NATIVE_SOURCE = AUCTION_SOURCE.replace('#include <mysql.h>',
    '#include <mysql.h>\n#include "player/player_snapshot_codec.h"').replace(
    'table.name == "auctions" || table.name == "auction_item_custody" ||\n            table.name == "auction_item_pickups"',
    'table.name == "player_data"').replace(
    '    for (const auto &table : result.item_equipment_sources)',
    '    for (const auto &table : result.item_sources)\n'
    '        if (table.name == "player_pet_items") emit(table, table.name);\n'
    '    for (const auto &table : result.item_equipment_sources)').replace(
    '    if (argc != 2) return 2;', r'''
    if (argc == 4 && std::string_view(argv[1]) == "--coin") {
        player_item_snapshot item{};
        item.object_uid = std::stoull(argv[2]); item.vnum = std::stoi(argv[3]);
        item.parent_index = PLAYER_SNAPSHOT_NO_PARENT; item.type = 20;
        item.values = {1,2,3,4,0,0,0,0};
        std::vector<uint8_t> bytes;
        if (player_item_snapshot_list_encode({item}, &bytes) != player_snapshot_codec_result::ok) return 5;
        hex(std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
        std::cout << '\n'; return 0;
    }
    if (argc != 2) return 2;''')


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_PLAYER_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativePlayerCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_native_cuts_and_coin_codec(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql
        work = ROOT/"bin/tests/sql-player-custody"; work.mkdir(parents=True,exist_ok=True)
        source,binary = work/"native.cpp",work/"native"; source.write_text(NATIVE_SOURCE)
        command = ["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
            "-ffunction-sections","-fdata-sections","-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config","--cflags"],text=True))
        command += [str(source),"src/persistence/economic_sql_source_snapshot.c","src/player/player_snapshot_codec.c"]
        command += shlex.split(subprocess.check_output(["mysql_config","--libs"],text=True))
        command += ["-Wl,--gc-sections","-lcrypto","-o",str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=600)
        coins = {uid: bytes.fromhex(json.loads(subprocess.check_output([str(binary),"--coin",str(uid),str(vnum)],text=True)))
                 for uid,vnum in ((82,101),(9002,201))}
        for uid,blob in coins.items(): (work/("native-coin-"+str(uid)+".bin")).write_bytes(blob)
        observations = []
        with tempfile.TemporaryDirectory(prefix="duris-player-audit-") as folder:
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
                            cursor.execute("INSERT INTO player_data(pid,name) VALUES (7,'synthetic-player'),(8,'synthetic-other')")
                            cursor.execute("INSERT INTO player_pets(id,owner_pid,pet_uid,mob_vnum) VALUES (51,7,900,1),(52,8,901,1)")
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
                                        native = exporter.read_player_custody(cursor)
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
                                    finally: reader.rollback()
                                raw = subprocess.run([str(binary),env["DB_SOCKET"]],check=True,capture_output=True,text=True,timeout=60)
                                tables = json.loads(raw.stdout)
                                registry = {name:columns.split(",") for group,_,_,specs in CAPTURE_REGISTRIES
                                    if group in ("tables","item_sources") for name,columns,_ in specs}
                                registry["item_current_owner_equipment"] = ["item_uid","equipment_slot"]
                                decoded = {name:[dict(zip(registry[name],[None if value is None else bytes.fromhex(value)
                                    for value in row])) for row in rows] for name,rows in tables.items()}
                                self.assertEqual(set(decoded),{"player_data","player_pet_items","item_current_owner","item_current_owner_equipment"})
                                self.assertEqual(native["player_ids"],[dict(pid=int(row["pid"])) for row in decoded["player_data"]])
                                number = lambda value: None if value is None else int(value)
                                self.assertEqual([{key:row[key] for key in ("item_id","pet_id","parent_id","uid","vnum")} for row in native["pet_items"]],
                                    [dict(zip(("item_id","pet_id","parent_id","uid","vnum"),[number(row[key])
                                        for key in ("id","pet_id","container_id","obj_uid","vnum")])) for row in decoded["player_pet_items"]])
                                equipment = {int(row["item_uid"]):int(row["equipment_slot"]) for row in decoded["item_current_owner_equipment"]}
                                expected_items = [dict(uid=int(row["item_uid"]),root=int(row["root_item_uid"]),parent=number(row["parent_item_uid"]),
                                    owner=[int(row[field]) for field in ("owner_type","owner_id","owner_context_id")],revision=int(row["item_revision"]),
                                    vnum=int(row["vnum"]),state={1:"live",2:"tombstone",3:"quarantined"}[int(row["state"])],
                                    equipment_slot=equipment[int(row["item_uid"])]) for row in decoded["item_current_owner"]]
                                self.assertEqual(native["items"],expected_items)
                                for row in decoded["item_current_owner"]:
                                    if row["coin_payload"] is not None: self.assertEqual(row["coin_payload"],coins[int(row["item_uid"])])
                                (work/(engine+"-"+label+"-native.json")).write_bytes(raw.stdout.encode())
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native,sort_keys=True)+"\n")
                                counts = audit(native)[0]
                                for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)
                                self.assertEqual(inventory(),before)
                                observations.append(dict(engine=engine,version=version,label=label,counts=counts,unchanged=True,database_sha256=before,
                                    native_sha256=hashlib.sha256(raw.stdout.encode()).hexdigest(),source_sha256=hashlib.sha256(json.dumps(native,sort_keys=True).encode()).hexdigest()))
                                return counts
                            def reset():
                                with owner.cursor() as cursor:
                                    for table in ("player_items","player_pet_items"):
                                        cursor.execute("DELETE FROM "+table+" WHERE container_id IS NOT NULL"); cursor.execute("DELETE FROM "+table)
                                    cursor.execute("DELETE FROM item_current_owner WHERE parent_item_uid IS NOT NULL"); cursor.execute("DELETE FROM item_current_owner")
                                    cursor.execute("UPDATE player_pets SET pet_uid=900 WHERE id=51")
                                    native = packet()
                                    for table,rows,group in (("player_items",native["player_items"],"pid"),("player_pet_items",native["pet_items"],"pet_id")):
                                        for row in rows:
                                            fields = ["id",group,"container_id","obj_uid","vnum","equip_slot","item_type","value0","value1","value2","value3"]
                                            cursor.execute("INSERT INTO "+table+"("+",".join(fields)+") VALUES ("+",".join(["%s"]*len(fields))+")",
                                                tuple(row[key] for key in ("item_id",group,"parent_id","uid","vnum","equipment_slot","item_type","value0","value1","value2","value3")))
                                    for row in native["items"]:
                                        cursor.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,"
                                            "item_revision,vnum,state,equipment_slot) VALUES (%s,%s,%s,%s,%s,%s,2,%s,1,0)",
                                            (row["uid"],row["root"],row["parent"],*row["owner"],row["vnum"]))
                            reset(); self.assertEqual(cut("healthy-player-pet-row-id-overlap"),{})
                            cases = (
                                ("legacy-pet-under-player","UPDATE item_current_owner SET owner_type=1,owner_id=7,owner_context_id=0 WHERE item_uid>=9001",None),
                                ("missing-player-physical","DELETE FROM player_items WHERE id=401","player_uid_missing_physical"),
                                ("missing-pet-physical","DELETE FROM player_pet_items WHERE id=401","player_uid_missing_physical"),
                                ("stale-player-owner","UPDATE item_current_owner SET owner_id=8 WHERE item_uid=81","player_projection_stale_owner"),
                                ("stale-pet-context","UPDATE item_current_owner SET owner_context_id=8 WHERE item_uid=9001","player_projection_stale_owner"),
                                ("unadmitted-pet-child","DELETE FROM item_current_owner WHERE item_uid=9002","player_uid_unadmitted"),
                                ("stale-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","player_item_topology_mismatch"),
                                ("foreign-player-parent","UPDATE player_items SET pid=8 WHERE id=400","player_item_parent_foreign_owner"),
                                ("foreign-pet-parent","UPDATE player_pet_items SET pet_id=52 WHERE id=400","player_item_parent_foreign_owner"),
                                ("pet-cycle","UPDATE player_pet_items SET container_id=401 WHERE id=400","player_item_parent_cycle"),
                                ("cross-table-uid","UPDATE player_pet_items SET obj_uid=81 WHERE id=400","player_duplicate_physical_uid"),
                                ("legacy-null-pet-uid","UPDATE player_pet_items SET obj_uid=NULL WHERE id=400","player_legacy_uid_unknown"),
                                ("inactive-retained-projection","UPDATE item_current_owner SET state=2 WHERE item_uid=81","player_projection_inactive_uid"),
                                ("pet-vnum","UPDATE player_pet_items SET vnum=9 WHERE id=401","player_item_vnum_mismatch"),
                                ("player-equipment","UPDATE player_items SET equip_slot=2 WHERE id=400","player_item_equipment_mismatch"),
                                ("player-quantity-null","UPDATE player_items SET quantity=NULL WHERE id=400","player_item_quantity_invalid"),
                                ("wallet-root-excluded","INSERT INTO player_items(id,pid,vnum,item_type,obj_uid) VALUES (777,7,-1,20,0)",None),
                                ("negative-nested-pet-coin","UPDATE player_pet_items SET item_type=20,value0=-1 WHERE id=401","player_negative_coin_value"),
                                ("orphan-pet","UPDATE player_pet_items SET pet_id=999 WHERE id=401","player_item_missing_pet"),
                                ("orphan-parent","UPDATE player_items SET container_id=999 WHERE id=401","player_item_parent_missing"),
                            )
                            for label,statement,code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-"): cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try: cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-"): cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                counts = cut(label)
                                if code is None: self.assertEqual(counts,{})
                                else: self.assertIn(code,counts)
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=82",(coins[82],))
                                cursor.execute("DELETE FROM player_items WHERE id=401")
                            self.assertEqual(cut("genuine-inline-coin-no-physical-row"),{})
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE item_current_owner SET coin_payload=%s WHERE item_uid=9002",(coins[9002],))
                                cursor.execute("UPDATE player_pet_items SET item_type=20,value0=4,value1=3,value2=2,value3=1 WHERE id=401")
                            self.assertIn("player_coin_literal_mismatch",cut("genuine-nested-pet-coin-mismatch"))
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor: cursor.execute("UPDATE player_items SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),46)
        print("SQL_PLAYER_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__": unittest.main()
