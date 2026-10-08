#!/usr/bin/env python3
"""Read-only corpse correspondence; native disposal code is only a test oracle."""
import copy
from collections import Counter
import errno
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
from reconcile_economy_accounting import MAX_ROWS, Reconciler, SnapshotError, physical_item_position
from test_reconcile_economy_accounting import clean_snapshot, bind_original_plans


def packet(count=2, chained=True):
    rows = [dict(item_id=400+i,corpse_id=5,parent_id=399+i if i and chained else None,
        uid=81+i,vnum=100+i,quantity=1,weight=1,extra_flags=0,
        value0=0,value1=0,value2=0,value3=0) for i in range(count)]
    items = [dict(uid=row["uid"],root=81 if chained else row["uid"],
        parent=row["uid"]-1 if i and chained else None,owner=[4,(7<<32)|9,0],
        revision=2,vnum=row["vnum"],state="live",equipment_slot=0) for i,row in enumerate(rows)]
    return dict(corpses=[dict(corpse_id=5,pid=7,save_id=9,revision=1,room_vnum=10)],
        corpse_items=rows,items=items,coin_piles=[],corpse_custody_coverage=dict(corpses=1,items=count))


def recount(native):
    native["corpse_custody_coverage"] = dict(corpses=len(native["corpses"]),items=len(native["corpse_items"]))


def audit(native,limit=100):
    before = copy.deepcopy(native); reader = Reconciler(limit)
    reader.audit_corpse_custody("sql_partial",native,{(row["uid"],):row for row in native["items"]})
    assert before == native and len(reader.exceptions) <= limit
    return dict(reader.counts),reader.exceptions


class CorpseCustodyAuditTests(unittest.TestCase):
    def test_numeric_identity_row_topology_and_native_widths(self):
        for count in (0,1,2,33,3000): self.assertEqual(audit(packet(count))[0],{})
        native = packet(1); native["corpses"][0].update(pid=2**31-1,save_id=2**31-1,revision=2**64-1)
        native["corpse_items"][0].update(item_id=2**32-1,uid=2**64-1,weight=-2**31,extra_flags=2**64-1)
        native["items"][0].update(uid=2**64-1,root=2**64-1,owner=[4,((2**31-1)<<32)|(2**31-1),0])
        self.assertEqual(audit(native)[0],{})
        native["items"][0]["state"] = "tombstone"
        self.assertEqual(audit(native)[0],{"corpse_projection_inactive_uid":1})

    def test_disagreements_and_output_limits(self):
        cases = (
            ("corpse_item_owner_mismatch","items",0,{"owner":[4,5,0]}),
            ("corpse_item_topology_mismatch","items",1,{"root":82}),
            ("corpse_item_topology_mismatch","items",1,{"parent":400}),
            ("corpse_item_vnum_mismatch","corpse_items",1,{"vnum":9}),
            ("corpse_item_equipment_mismatch","items",0,{"equipment_slot":1}),
            ("corpse_projection_inactive_uid","items",0,{"state":"quarantined"}),
            ("corpse_item_parent_missing","corpse_items",1,{"parent_id":999}),
            ("corpse_item_parent_cycle","corpse_items",0,{"parent_id":401}),
            ("corpse_item_missing_corpse","corpse_items",1,{"corpse_id":6}),
            ("corpse_legacy_uid_unknown","corpse_items",0,{"uid":None}),
            ("corpse_unsupported_quantity","corpse_items",0,{"quantity":None}),
            ("corpse_unsupported_quantity","corpse_items",0,{"quantity":2}),
            ("corpse_item_vnum_invalid","corpse_items",0,{"vnum":0}),
            ("corpse_physical_literal_unknown","corpse_items",0,{"extra_flags":None}),
            ("corpse_physical_literal_unknown","corpse_items",0,{"weight":None}),
            ("corpse_negative_value","corpse_items",1,{"value0":-1}),
            ("corpse_value_unknown","corpse_items",1,{"value0":None}),
            ("corpse_identity_unknown","corpses",0,{"pid":None}),
            ("corpse_identity_unknown","corpses",0,{"save_id":2**31}),
            ("corpse_revision_unknown","corpses",0,{"revision":0}),
            ("corpse_room_unknown","corpses",0,{"room_vnum":None}),
        )
        for code,name,index,change in cases:
            native = packet(); native[name][index].update(change); counts = audit(native)[0]
            self.assertIn(code,counts)
            for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)

    def test_duplicates_missing_directions_and_cached_corrupt_forests(self):
        cases = []
        native = packet(); native["corpse_items"].pop(); cases.append((native,"corpse_uid_missing_physical"))
        native = packet(); native["items"].pop(); cases.append((native,"corpse_uid_unadmitted"))
        native = packet(); native["corpse_items"] *= 2; cases.append((native,"corpse_duplicate_physical_row"))
        native = packet(); native["corpse_items"][1]["uid"] = 81; cases.append((native,"corpse_duplicate_physical_uid"))
        native = packet(); native["corpses"] *= 2; cases.append((native,"corpse_duplicate_id"))
        native = packet(); native["corpses"].append(dict(native["corpses"][0],corpse_id=6))
        cases.append((native,"corpse_duplicate_owner_identity"))
        native = packet(); native["corpse_items"][0]["corpse_id"] = 6
        cases.append((native,"corpse_item_parent_foreign_owner"))
        native = packet(3001); cases.append((native,"corpse_item_count_exceeds_native_limit"))
        native = packet(3000); native["corpse_items"][0]["parent_id"] = 3399
        cases.append((native,"corpse_item_parent_cycle"))
        for native,code in cases:
            recount(native); counts = audit(native)[0]; self.assertIn(code,counts)
            for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)

    def test_coin_literals_transient_ancestors_and_sum_overflow(self):
        native = packet(); native["corpse_items"][1].update(vnum=3,value0=1,value1=2,value2=3,value3=4)
        native["items"][1]["vnum"] = 3
        self.assertEqual(audit(native)[0],{"corpse_coin_payload_unknown":1})
        native["coin_piles"] = [dict(uid=82,owner=[4,(7<<32)|9,0],revision=2,state="live",amounts=[1,2,3,4])]
        self.assertEqual(audit(native)[0],{})
        for change in ({"revision":1},{"state":"tombstone"},{"owner":[4,5,0]},{"amounts":None}):
            value = copy.deepcopy(native); value["coin_piles"][0].update(change)
            self.assertIn("corpse_coin_payload_unknown",audit(value)[0])
        value = copy.deepcopy(native); value["coin_piles"] *= 2
        self.assertIn("corpse_coin_payload_unknown",audit(value)[0])
        value = copy.deepcopy(native); value["coin_piles"][0]["amounts"] = [4,3,2,1]
        self.assertIn("corpse_coin_literal_mismatch",audit(value)[0])
        for change in ({"owner":[4.0,(7<<32)|9,0]},{"revision":2.0},{"revision":True}):
            value = copy.deepcopy(native); value["coin_piles"][0].update(change)
            with self.assertRaises(SnapshotError): audit(value,0)
        for change in ({"vnum":3},{"extra_flags":524288}):
            value = packet(33); value["corpse_items"][0].update(change)
            self.assertIn("corpse_item_invalid_ancestor_kind",audit(value)[0])
        value = packet(33)
        for row in value["corpse_items"]: row["extra_flags"] = 524288
        self.assertEqual(audit(value)[0],{})
        value = packet(2,False)
        for row in value["corpse_items"]: row.update(vnum=3,value0=2**31-1)
        self.assertIn("corpse_coin_sum_overflow",audit(value)[0])

    def test_exact_fields_coverage_and_preflight_without_hidden_rows(self):
        for name,fields in (("corpses",("corpse_id","pid","save_id","revision","room_vnum")),
                ("corpse_items",tuple(packet()["corpse_items"][0]))):
            for field in fields:
                for invalid in (True,1.0,"1",[],{},2**64,-2**64):
                    native = packet(); native[name][0][field] = invalid
                    with self.assertRaises(SnapshotError): audit(native,0)
                native = packet(); native[name][0].pop(field)
                with self.assertRaises(SnapshotError): audit(native,0)
        for coverage in (None,{},dict(corpses=True,items=2)):
            native = packet(); native["corpse_custody_coverage"] = coverage
            with self.assertRaises(SnapshotError): audit(native,0)
        for invalid in (True,1.0,None,"1",2**31,-2**31-1):
            native = packet(); native["items"][0]["vnum"] = invalid
            with self.assertRaises(SnapshotError): audit(native,0)
        self.assertEqual(audit(dict(items=[]),0)[0],{"missing_corpse_custody_coverage":1})
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=MAX_ROWS+1)
        with self.assertRaises(exporter.ExportError): exporter.read_corpse_custody(cursor)
        self.assertEqual(cursor.execute.call_count,1)
        cursor = mock.Mock(); cursor.fetchone.return_value = dict(rows_total=0); cursor.fetchall.return_value = ()
        self.assertEqual(exporter.read_corpse_custody(cursor),dict(corpses=[],corpse_items=[],corpse_custody_coverage=dict(corpses=0,items=0)))
        for call in cursor.execute.call_args_list[1:]:
            self.assertNotIn("JOIN",call.args[0]); self.assertNotIn("WHERE",call.args[0])
            self.assertEqual(call.args[1],(MAX_ROWS+1,))
            self.assertNotIn("player_name",call.args[0])

    def test_maximum_projection_budget_is_linear_and_preserves_input(self):
        native = packet(0); native["corpses"].clear()
        for group in range(33):
            example = packet(3000); offset = group*3000
            native["corpses"].append(dict(example["corpses"][0],corpse_id=group+1,save_id=group+1))
            for row in example["corpse_items"]:
                row.update(item_id=row["item_id"]+offset,corpse_id=group+1,uid=row["uid"]+offset,
                    parent_id=row["parent_id"]+offset if row["parent_id"] else None)
            for row in example["items"]:
                row.update(uid=row["uid"]+offset,root=row["root"]+offset,
                    parent=row["parent"]+offset if row["parent"] else None,owner=[4,(7<<32)|(group+1),0])
            native["corpse_items"].extend(reversed(example["corpse_items"])); native["items"].extend(example["items"])
        recount(native); start = time.monotonic(); self.assertEqual(audit(native,0)[0],{})
        seconds = time.monotonic()-start; self.assertLess(seconds,30)
        print("CORPSE_AUDIT_BUDGET "+json.dumps(dict(physical_rows=99000,corpses=33,max_depth=3000,seconds=seconds,budget=30)),flush=True)

    def test_global_cli_keeps_numeric_ids_private_and_input_unchanged(self):
        cut = clean_snapshot(); native = packet(1); native["corpse_items"][0]["vnum"] = 1
        cut["native"].update({key:value for key,value in native.items() if key not in ("items","coin_piles")})
        cut["native"]["items"][0].update(vnum=1,owner=[4,(7<<32)|9,0]); cut["ownership_events"][0]["owner"] = [4,(7<<32)|9,0]
        bind_original_plans(cut); cut["native"]["corpse_items"][0]["uid"] = 999
        cut["private_alias"] = "PRIVATE-DO-NOT-PRINT"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)/"cut.json"; path.write_text(json.dumps(cut)); before = path.read_bytes()
            for limit in (0,1,100):
                result = subprocess.run([sys.executable,str(ROOT/"scripts/reconcile_economy_accounting.py"),str(path),"--limit",str(limit)],
                    capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,1,result); self.assertEqual(result.stderr,"")
                self.assertEqual(json.loads(result.stdout)["exception_counts"],{"corpse_uid_unadmitted":1,"corpse_uid_missing_physical":1})
                self.assertNotIn("PRIVATE",result.stdout); self.assertEqual(path.read_bytes(),before)
        cut["native"]["corpse_items"][0]["uid"] = 81
        cut["native"]["items"][0]["vnum"] = True; before = copy.deepcopy(cut)
        for limit in (0,1,100):
            with self.assertRaises(SnapshotError): Reconciler(limit).audit(cut)
        self.assertEqual(cut,before)


from test_sql_player_custody_audit import NATIVE_SOURCE as PLAYER_SOURCE
# Include the production body unchanged so this test can invoke its private
# physical parser. GC discards unrelated native mutation functions; no provider
# stubs, source rewriting or production interface changes are involved.
NATIVE_SOURCE = PLAYER_SOURCE.replace('#include <mysql.h>',
    '#include <mysql.h>\n#include "persistence/corpse_lifecycle_repository.c"').replace(
    'table.name == "player_data" || table.name == "item_current_owner"',
    'table.name == "item_current_owner"').replace(
    '    for (const auto &table : result.item_sources)\n'
    '        if (table.name == "player_pet_items") emit(table, table.name);\n', '').replace(
    '    if (argc != 2) return 2;', r'''
    if (argc == 4 && std::string_view(argv[1]) == "--identity") {
        std::cout << item_corpse_owner_id(std::stoul(argv[2]), std::stoul(argv[3])) << '\n'; return 0;
    }
    if (argc == 4 && std::string_view(argv[1]) == "--physical") {
        MYSQL *db = mysql_init(nullptr);
        if (!db || !mysql_real_connect(db,"localhost","root","","duris_restore",0,argv[2],0) ||
            mysql_query(db,"START TRANSACTION")) return 6;
        std::vector<physical_item> physical; std::array<int32_t,4> money{}; unsigned code=0;
        const bool loaded=load_physical_items(db,std::stoul(argv[3]),true,&physical,&money,&code);
        mysql_rollback(db); mysql_close(db); if (!loaded) return 7;
        std::cout << "{\"code\":" << code << ",\"limit\":" << ITEM_TRANSFER_MAX_ITEMS
                  << ",\"transient_mask\":" << ITEM_TRANSIENT << ",\"money\":[";
        for (size_t i=0;i<4;++i) std::cout << (i ? "," : "") << money[i];
        std::cout << "],\"items\":["; bool first=true;
        for (const auto &item : physical) {
            if (!first) std::cout << ',';
            first=false;
            std::cout << "{\"item_id\":" << item.id << ",\"parent_id\":" << item.parent_id
                      << ",\"uid\":" << item.item_uid << ",\"root\":" << item.root_item_uid
                      << ",\"parent\":" << item.parent_item_uid << ",\"vnum\":" << item.vnum
                      << ",\"weight\":" << item.weight << ",\"extra_flags\":" << item.extra_flags
                      << ",\"values\":[";
            for (size_t i=0;i<4;++i) std::cout << (i ? "," : "") << item.money[i];
            std::cout << "]}";
        }
        std::cout << "]}\n"; return 0;
    }
    if (argc != 2) return 2;''')


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_CORPSE_AUDIT") == "1",
                     "requires explicit disposable Linux native SQL invocation")
class NativeCorpseCustodyAuditTests(unittest.TestCase):
    def test_both_canonical_engines_unmodified_physical_parser(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from economic_sql_audit_origins import CAPTURE_REGISTRIES
        from test_persistence_backup_integration import sql
        work = ROOT/"bin/tests/sql-corpse-custody"; work.mkdir(parents=True,exist_ok=True)
        source,binary = work/"native.cpp",work/"native"; source.write_text(NATIVE_SOURCE)
        command = ["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-O1","-g",
            "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
            "-ffunction-sections","-fdata-sections","-Isrc"]
        command += shlex.split(subprocess.check_output(["mysql_config","--cflags"],text=True))
        command += [str(source),"src/persistence/economic_sql_source_snapshot.c","src/item/item_transfer_command.c","src/player/player_snapshot_codec.c"]
        command += shlex.split(subprocess.check_output(["mysql_config","--libs"],text=True))
        command += ["-Wl,--gc-sections","-lcrypto","-o",str(binary)]
        subprocess.run(command,cwd=ROOT,check=True,timeout=600)
        coin = bytes.fromhex(json.loads(subprocess.check_output([str(binary),"--coin","82","3"],text=True)))
        (work/"native-coin-82.bin").write_bytes(coin)
        self.assertEqual(subprocess.check_output([str(binary),"--identity","7","9"],text=True).strip(),str((7<<32)|9))
        observations = []
        with tempfile.TemporaryDirectory(prefix="duris-corpse-audit-") as folder:
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
                            cursor.execute("INSERT INTO corpses(id,player_name,save_id,value3,room_vnum) VALUES "
                                "(5,'PRIVATE-synthetic-player',9,7,10),(6,'PRIVATE-synthetic-other',9,8,10)")
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
                            def cut(label,expected_native=0):
                                before = inventory()
                                with reader.cursor() as cursor:
                                    cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                                    cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT, READ ONLY")
                                    try:
                                        native = exporter.read_corpse_custody(cursor)
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
                                physical = {}
                                for corpse_id in sorted({row["corpse_id"] for row in native["corpse_items"]}):
                                    body = subprocess.check_output([str(binary),"--physical",env["DB_SOCKET"],str(corpse_id)],text=True,timeout=60)
                                    result = json.loads(body); physical[str(corpse_id)] = result
                                    self.assertEqual((result["limit"],result["transient_mask"]),(3000,524288))
                                    if result["code"]==0:
                                        rows = [row for row in native["corpse_items"] if row["corpse_id"]==corpse_id]
                                        self.assertEqual([{key:row[key] for key in ("item_id","parent_id","uid","vnum","weight","extra_flags","values")} for row in result["items"]],
                                            [dict(item_id=row["item_id"],parent_id=row["parent_id"] or 0,uid=row["uid"] or 0,vnum=row["vnum"],
                                                weight=row["weight"],extra_flags=row["extra_flags"],values=[row["value"+str(index)] for index in range(4)]) for row in rows])
                                        positions = {(row["item_id"],):row for row in rows}; counts = Counter(row["item_id"] for row in rows); cache = {}
                                        for item in result["items"]:
                                            if not item["uid"]:
                                                self.assertEqual((item["root"],item["parent"]),(0,0)); continue
                                            root,parent,error = physical_item_position(positions[(item["item_id"],)],positions,counts,"corpse_id",max_depth=3000,position_cache=cache)
                                            self.assertIsNone(error); self.assertEqual((item["root"],item["parent"]),(root,parent or 0))
                                if "5" in physical:self.assertEqual(physical["5"]["code"],expected_native)
                                for row in decoded["item_current_owner"]:
                                    if row["coin_payload"] is not None:self.assertEqual(row["coin_payload"],coin)
                                parsed = dict(capture=tables,physical=physical)
                                native_bytes = (json.dumps(parsed,sort_keys=True)+"\n").encode()
                                (work/(engine+"-"+label+"-native.json")).write_bytes(native_bytes)
                                (work/(engine+"-"+label+"-audit.json")).write_text(json.dumps(native,sort_keys=True)+"\n")
                                counts = audit(native)[0]
                                for limit in (0,1,100): self.assertEqual(audit(native,limit)[0],counts)
                                self.assertEqual(inventory(),before)
                                self.assertNotIn("PRIVATE",json.dumps(native))
                                observations.append(dict(engine=engine,version=version,label=label,counts=counts,native_codes={key:value["code"] for key,value in physical.items()},
                                    unchanged=True,database_sha256=before,native_sha256=hashlib.sha256(native_bytes).hexdigest(),
                                    source_sha256=hashlib.sha256(json.dumps(native,sort_keys=True).encode()).hexdigest()))
                                return counts
                            def reset(count=2):
                                with owner.cursor() as cursor:
                                    # Clear only this private fixture between cuts; a bulk
                                    # delete of the 33-deep chain exceeds InnoDB's cascade limit.
                                    cursor.execute("UPDATE corpse_items SET container_id=NULL"); cursor.execute("DELETE FROM corpse_items")
                                    cursor.execute("UPDATE item_current_owner SET parent_item_uid=NULL"); cursor.execute("DELETE FROM item_current_owner")
                                    cursor.execute("UPDATE corpses SET value3=7,save_id=9 WHERE id=5")
                                    cursor.execute("UPDATE corpses SET value3=8,save_id=9 WHERE id=6")
                                    native = packet(count,count<=33)
                                    cursor.executemany("INSERT INTO corpse_items(id,corpse_id,container_id,obj_uid,vnum,quantity,weight,extra_flags,value0,value1,value2,value3) "
                                        "VALUES (%s,5,%s,%s,%s,1,1,0,0,0,0,0)",[(row["item_id"],row["parent_id"],row["uid"],row["vnum"]) for row in native["corpse_items"]])
                                    cursor.executemany("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot) "
                                        "VALUES (%s,%s,%s,4,%s,0,2,%s,1,0)",[(row["uid"],row["root"],row["parent"],(7<<32)|9,row["vnum"]) for row in native["items"]])
                            reset(); self.assertEqual(cut("healthy-row-id-versus-owner-id"),{})
                            cases = (
                                ("wrong-row-id-owner","UPDATE item_current_owner SET owner_id=5 WHERE item_uid=81","corpse_item_owner_mismatch",0),
                                ("wrong-numeric-pid","UPDATE corpses SET value3=8 WHERE id=5","corpse_item_owner_mismatch",0),
                                ("duplicate-numeric-identity","UPDATE corpses SET value3=7 WHERE id=6","corpse_duplicate_owner_identity",0),
                                ("missing-physical","DELETE FROM corpse_items WHERE id=401","corpse_uid_missing_physical",0),
                                ("unadmitted-child","DELETE FROM item_current_owner WHERE item_uid=82","corpse_uid_unadmitted",0),
                                ("wrong-root","UPDATE item_current_owner SET root_item_uid=82 WHERE item_uid=82","corpse_item_topology_mismatch",0),
                                ("foreign-parent","UPDATE corpse_items SET corpse_id=6 WHERE id=400","corpse_item_parent_foreign_owner",errno.EILSEQ),
                                ("cycle","UPDATE corpse_items SET container_id=401 WHERE id=400","corpse_item_parent_cycle",errno.EILSEQ),
                                ("duplicate-uid","UPDATE corpse_items SET obj_uid=81 WHERE id=401","corpse_duplicate_physical_uid",errno.EILSEQ),
                                ("legacy-null-uid","UPDATE corpse_items SET obj_uid=NULL WHERE id=400","corpse_legacy_uid_unknown",errno.EILSEQ),
                                ("inactive-retained","UPDATE item_current_owner SET state=2 WHERE item_uid=81","corpse_projection_inactive_uid",0),
                                ("vnum","UPDATE corpse_items SET vnum=9 WHERE id=401","corpse_item_vnum_mismatch",0),
                                ("quantity-null","UPDATE corpse_items SET quantity=NULL WHERE id=400","corpse_unsupported_quantity",0),
                                ("quantity-two","UPDATE corpse_items SET quantity=2 WHERE id=400","corpse_unsupported_quantity",0),
                                ("weight-null","UPDATE corpse_items SET weight=NULL WHERE id=400","corpse_physical_literal_unknown",errno.EILSEQ),
                                ("flags-null","UPDATE corpse_items SET extra_flags=NULL WHERE id=400","corpse_physical_literal_unknown",errno.EILSEQ),
                                ("value-null","UPDATE corpse_items SET value0=NULL WHERE id=401","corpse_value_unknown",errno.EILSEQ),
                                ("negative-value","UPDATE corpse_items SET value0=-1 WHERE id=401","corpse_negative_value",errno.EILSEQ),
                                ("transient-ancestor","UPDATE corpse_items SET extra_flags=524288 WHERE id=400","corpse_item_invalid_ancestor_kind",errno.EILSEQ),
                                ("coin-ancestor","UPDATE corpse_items SET vnum=3 WHERE id=400","corpse_item_invalid_ancestor_kind",errno.EILSEQ),
                                ("orphan-corpse","UPDATE corpse_items SET corpse_id=999 WHERE id=401","corpse_item_missing_corpse",0),
                                ("orphan-parent","UPDATE corpse_items SET container_id=999 WHERE id=401","corpse_item_parent_missing",errno.EILSEQ),
                                ("unknown-pid","UPDATE corpses SET value3=NULL WHERE id=5","corpse_identity_unknown",0),
                                ("invalid-save-id","UPDATE corpses SET save_id=2147483648 WHERE id=5","corpse_identity_unknown",0),
                                ("all-transient","UPDATE corpse_items SET extra_flags=524288",None,0),
                                ("uidless-coin","UPDATE corpse_items SET obj_uid=NULL,vnum=3 WHERE id=401","corpse_legacy_uid_unknown",0),
                                ("uidless-transient","UPDATE corpse_items SET obj_uid=NULL,extra_flags=524288 WHERE id=401","corpse_legacy_uid_unknown",0),
                            )
                            for label,statement,code,native_code in cases:
                                reset()
                                with owner.cursor() as cursor:
                                    if label.startswith("orphan-"): cursor.execute("SET FOREIGN_KEY_CHECKS=0")
                                    try:cursor.execute(statement)
                                    finally:
                                        if label.startswith("orphan-"):cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                                counts = cut(label,native_code)
                                if code is None:self.assertEqual(counts,{})
                                else:self.assertIn(code,counts)
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE corpse_items SET vnum=3,value0=1,value1=2,value2=3,value3=4 WHERE id=401")
                                cursor.execute("UPDATE item_current_owner SET vnum=3,coin_payload=%s WHERE item_uid=82",(coin,))
                            self.assertEqual(cut("genuine-nested-coin"),{})
                            with owner.cursor() as cursor:cursor.execute("UPDATE corpse_items SET value0=9 WHERE id=401")
                            self.assertIn("corpse_coin_literal_mismatch",cut("genuine-nested-coin-mismatch"))
                            reset()
                            with owner.cursor() as cursor:
                                cursor.execute("UPDATE corpse_items SET vnum=3,container_id=NULL,value0=IF(id=400,2147483647,1)")
                                cursor.execute("UPDATE item_current_owner SET vnum=3,root_item_uid=item_uid,parent_item_uid=NULL")
                            self.assertIn("corpse_coin_sum_overflow",cut("native-coin-sum-overflow",errno.EOVERFLOW))
                            reset(33); self.assertEqual(cut("native-33-deep-forest"),{})
                            reset(3000); self.assertEqual(cut("native-exact-3000-row-limit"),{})
                            reset(3001); self.assertIn("corpse_item_count_exceeds_native_limit",cut("native-over-3000-row-limit",errno.E2BIG))
                            with self.assertRaises(pymysql.MySQLError):
                                with reader.cursor() as cursor:cursor.execute("UPDATE corpse_items SET id=id")
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n")
        self.assertEqual(len(observations),68)
        print("SQL_CORPSE_CUSTODY_NATIVE "+json.dumps(observations,sort_keys=True),flush=True)


if __name__ == "__main__": unittest.main()
