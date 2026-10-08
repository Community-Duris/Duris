#!/usr/bin/env python3
"""Independent modern room diagnostics; components never qualify full recovery."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/"scripts"), str(ROOT/"tests/async")]
import economic_sql_audit_snapshot as exporter
from economic_item_payload_audit import (MAX_ROOM_ITEM_BYTES, PROOF_NUMBERS, ROOM_COLLECTIONS,
                                        PayloadError, decode_single_item)
from reconcile_economy_accounting import MAX_INPUT_BYTES, MAX_ROWS, Reconciler, SnapshotError

UNQUALIFIED = dict(room_item_full_runtime_authority_unqualified=1, room_item_retained_root_authority_unqualified=1)
OP = "11"*16


def payload(uid):
    return (struct.pack("<IihQqibB",1,-1,0,uid,0,1200,15,15)+bytes(4*4+8*4+6*8+5*4)+
            struct.pack("<ibihh",0,0,0,0,0)+bytes(5*8+8*2+2*4))


def proof(uid,revision,parent):
    return dict(uid=uid,revision=revision,reference_operation=OP,legacy_operation=OP,ledger_operation=OP,
        inbox_operation=OP,root_operation=OP,reference_uid=uid,before_revision=revision-1,
        after_revision=revision,child_index=0,ledger_uid=uid,ledger_root=100,ledger_parent=parent,
        ledger_revision=revision,from_type=1,from_id=7,from_context=0,to_type=3,to_id=120,to_context=0,
        reason_type=6,reason_id=120,command_type=5,schema_version=2,status=1,result_code=0,
        failure_stage=0,outcome=1,operation_result=0)


def recount(native):
    native["room_item_custody_coverage"] = dict(version=1,
        **{name:len(native["room_item_"+name]) for name in ("payloads","proofs","roots","members","seasons")},
        payload_bytes=sum(row["payload_bytes"] for row in native["room_item_payloads"]))


def packet(count=3):
    native = {name:[] for name in ROOM_COLLECTIONS}
    native["room_item_seasons"]=[dict(state_id=1,season_epoch=1,reset_status="active")]
    native["room_item_roots"]=[dict(root=100)] if count else []
    for index in range(count):
        uid=100+index;revision=11+index;body=payload(uid);parent=uid-1 if index else None
        native["room_item_payloads"].append(dict(uid=uid,revision=revision,payload_version=1,
            operation_id=OP,season_epoch=1,payload_bytes=len(body),payload=body.hex()))
        native["room_item_proofs"].append(proof(uid,revision,parent))
        native["room_item_members"].append(dict(uid=uid,root=100,parent=parent,revision=revision,
            vnum=1200,state=1,owner_type=3,owner_id=120,owner_context=0,equipment_slot=0,
            owner_revision=4,saved_duplicates=0))
    recount(native);return native


def audit(native,limit=100):
    before=copy.deepcopy(native);reader=Reconciler(limit);reader.audit_room_item_custody("sql_partial",native)
    assert native==before and len(reader.exceptions)<=limit
    return dict(reader.counts),reader.exceptions


class RoomItemAuditTests(unittest.TestCase):
    def test_current_graph_and_history_do_not_grant_authority(self):
        self.assertEqual(audit(packet())[0],UNQUALIFIED)
        self.assertEqual(audit(packet(0))[0],{})
        for phase in ("moved","old-season"):
            native=packet();native["room_item_roots"]=[];native["room_item_members"]=[]
            if phase=="old-season":native["room_item_seasons"][0]["season_epoch"]=2
            recount(native);self.assertEqual(audit(native)[0],UNQUALIFIED)
        for field,value in (("state_id",2),("season_epoch",0),("reset_status","resetting")):
            native=packet();native["room_item_seasons"][0][field]=value
            self.assertEqual(audit(native)[0],dict(UNQUALIFIED,room_item_season_unqualified=1))
        native=packet();native["items"]=[]
        self.assertIn("room_item_current_census_mismatch",audit(native)[0])
        native["items"]=[dict(uid=r["uid"],root=r["root"],parent=r["parent"],
            owner=[r["owner_type"],r["owner_id"],r["owner_context"]],revision=r["revision"],vnum=r["vnum"],
            state="live",equipment_slot=r["equipment_slot"]) for r in native["room_item_members"]]
        self.assertEqual(audit(native)[0],UNQUALIFIED)
        native["items"][1]["revision"]+=1
        self.assertIn("room_item_current_projection_mismatch",audit(native)[0])
        native["room_item_roots"]=[];native["room_item_members"]=[];recount(native)
        self.assertIn("room_item_root_census_mismatch",audit(native)[0])

    def test_each_current_disagreement_and_output_limit(self):
        cases=(("room_item_current_custody_invalid","owner_revision",None),
            ("room_item_current_custody_invalid","owner_id",121),
            ("room_item_current_custody_invalid","owner_context",1),
            ("room_item_current_custody_invalid","owner_type",1),
            ("room_item_current_custody_invalid","equipment_slot",1),
            ("room_item_current_custody_invalid","state",3),
            ("room_item_current_vnum_mismatch","vnum",5),
            ("room_item_duplicate_saved_payload","saved_duplicates",1),
            ("room_item_graph_parent_invalid","parent",999))
        for code,field,value in cases:
            native=packet();native["room_item_members"][1][field]=value;counts=None
            for limit in (0,1,100):
                observed=audit(native,limit)[0];self.assertIn(code,observed)
                if counts is not None:self.assertEqual(counts,observed)
                counts=observed
        native=packet();native["room_item_payloads"].pop();native["room_item_proofs"].pop();recount(native)
        self.assertIn("room_item_current_payload_missing",audit(native)[0])
        native=packet();native["room_item_payloads"][0]["season_epoch"]=2
        self.assertIn("room_item_current_payload_missing",audit(native)[0])
        native=packet(33)
        self.assertIn("room_item_graph_depth_invalid",audit(native)[0])
        native=packet();native["room_item_members"][1]["parent"]=102
        self.assertIn("room_item_graph_disconnected",audit(native)[0])

    def test_literal_codec_and_each_original_binding(self):
        for field in PROOF_NUMBERS:
            native=packet();native["room_item_proofs"][1][field]=None
            self.assertIn("room_item_retained_binding_invalid",audit(native)[0],field)
        for field in ("reference_operation","legacy_operation","ledger_operation","inbox_operation","root_operation"):
            native=packet();native["room_item_proofs"][0][field]="22"*16
            self.assertIn("room_item_retained_binding_invalid",audit(native)[0])
        native=packet();native["room_item_proofs"].append(native["room_item_proofs"][0].copy());recount(native)
        self.assertIn("room_item_retained_binding_invalid",audit(native)[0])
        native["room_item_proofs"][-1]["reference_operation"]="22"*16
        self.assertIn("room_item_retained_binding_invalid",audit(native)[0])
        for changed in (b"",payload(100)[:-1],payload(100)+b"\0",payload(999)):
            native=packet();row=native["room_item_payloads"][0];row.update(payload=changed.hex(),payload_bytes=len(changed));recount(native)
            self.assertIn("room_item_payload_invalid",audit(native)[0])
        for offset,value in ((4,0),(8,1),(30,20),(30,24),(30,42),(31,0)):
            native=packet();body=bytearray(payload(100));body[offset]=value
            native["room_item_payloads"][0]["payload"]=body.hex()
            self.assertIn("room_item_payload_invalid",audit(native)[0])

    def test_whole_graph_codec_rows_and_bytes_are_shared_budgets(self):
        for kind,count in (("rows",3),("bytes",6)):
            native=packet(count)
            for row in native["room_item_payloads"]:
                body=payload(row["uid"])
                if kind=="rows":
                    body=body[:-8]+struct.pack("<I",3000)+bytes(12*3000)+struct.pack("<I",0)
                else:
                    text=struct.pack("<I",4096)+b"x"*4096
                    body=body[:32]+text*4+body[48:-4]+struct.pack("<I",1)+text*2+b"\0"+struct.pack("<I",0)
                self.assertLess(len(body),MAX_ROOM_ITEM_BYTES)
                decode_single_item(body)
                row.update(payload=body.hex(),payload_bytes=len(body))
            recount(native)
            for limit in (0,1,100):
                self.assertEqual(audit(native,limit)[0],dict(UNQUALIFIED,room_item_graph_payload_budget_invalid=1))

    def test_packet_representations_and_preflight_before_lob(self):
        for name in (*ROOM_COLLECTIONS,"room_item_custody_coverage"):
            native=packet();native.pop(name)
            with self.assertRaises(SnapshotError):audit(native,0)
        native=packet()
        for collection in ROOM_COLLECTIONS:
            if not native[collection]:continue
            for field,value in native[collection][0].items():
                if value is None:continue
                for bad in (True,1.0,[],{}):
                    changed=copy.deepcopy(native);changed[collection][0][field]=bad
                    with self.assertRaises(SnapshotError,msg=field):audit(changed,0)
        for row in (dict(rows_total=MAX_ROWS+1,payload_bytes=1,max_payload_bytes=1),
                    dict(rows_total=1,payload_bytes=MAX_INPUT_BYTES//2+1,max_payload_bytes=1),
                    dict(rows_total=1,payload_bytes=1,max_payload_bytes=MAX_ROOM_ITEM_BYTES+1)):
            cursor=mock.Mock();cursor.fetchone.return_value=row
            with self.assertRaises(exporter.ExportError):exporter.read_room_item_custody(cursor)
            self.assertEqual(cursor.execute.call_count,1);cursor.fetchall.assert_not_called()
        reader=Reconciler();reader.audit_room_item_custody("sql_partial",{})
        self.assertEqual(dict(reader.counts),dict(missing_room_item_custody_coverage=1))

    def test_near_row_limit_is_iterative_and_diagnostic_limit_independent(self):
        native=packet(1);row=native["room_item_members"][0]
        native["room_item_members"] += [dict(row,uid=uid,parent=100,revision=1) for uid in range(101,100096)]
        recount(native);started=time.monotonic();counts=audit(native,0)[0]
        self.assertIn("room_item_graph_item_limit_invalid",counts)
        self.assertEqual(counts["room_item_current_payload_missing"],99995)
        self.assertLess(time.monotonic()-started,5)


def native_payloads(work):
    original=(ROOT/"tests/async/sql_room_item_payload_test.cpp").read_text(encoding="utf-8")
    source=original[:original.index("static void unchanged_refusal")]+r'''
static_assert(PLAYER_SNAPSHOT_MAX_ROWS==8192 && PLAYER_SNAPSHOT_MAX_DEPTH==32 &&
              PLAYER_SNAPSHOT_MAX_OBJECTS==4096 && PLAYER_SNAPSHOT_MAX_STRING_BYTES==4096);
static_assert(ITEM_TRANSFER_MAX_ITEMS==3000 && ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES==131072);
static_assert(ITEM_LOWEST==1 && ITEM_LAST==41 && ITEM_ARTIFACT==268435456);
int main() {
    const std::vector<player_item_snapshot> items={item(100,-1),item(101,0),item(102,1)};
    const auto payload=command(items); sql_room_item_payload_batch batch;
    assert(sql_room_item_payload_capture(*payload,&batch));
    for (const auto &body:batch.payloads) {
        for (uint8_t byte:body) std::printf("%02x",byte);
        std::printf("\n");
    }
}
'''
    cpp,binary=work/"native.cpp",work/"native";cpp.write_text(source,encoding="utf-8",newline="\n")
    command=["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-D__NO_MYSQL__",
        "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
        "-ffunction-sections","-fdata-sections","-Wl,--gc-sections","-Isrc",str(cpp),
        "src/persistence/sql_room_item_payload.c","src/player/player_snapshot_codec.c","-o",str(binary)]
    subprocess.run(command,cwd=ROOT,check=True,timeout=120)
    result=subprocess.check_output([str(binary)],text=True,timeout=30).splitlines()
    rows=[bytes.fromhex(x) for x in result];assert len(rows)==3
    for index,body in enumerate(rows):
        (work/(str(index)+".bin")).write_bytes(body)
        item=decode_single_item(body);assert item["uid"]==100+index and item["codec_rows"]==3
    (work/"native-inputs.json").write_text(json.dumps(dict(command=command,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
        payload_sha256=[hashlib.sha256(body).hexdigest() for body in rows]),indent=2)+"\n",encoding="utf-8")
    return rows


@unittest.skipUnless(os.environ.get("DURIS_RUN_SQL_ROOM_AUDIT")=="1","requires isolated canonical SQL services")
class RoomItemSQLTests(unittest.TestCase):
    def test_both_canonical_engines_complete_raw_graph_and_refusals(self):
        import pymysql
        import migration_runner as migrations
        import persistence_restore as restore
        from test_persistence_backup_integration import sql
        work=ROOT/"bin/tests/sql-room-item-audit";work.mkdir(parents=True,exist_ok=True)
        bodies=native_payloads(work);observations=[]
        with tempfile.TemporaryDirectory(prefix="duris-room-audit-") as folder:
            for engine in ("mariadb","mysql"):
                candidate=Path(folder)/engine;candidate.mkdir(mode=0o700)
                with restore.private_database(candidate,engine) as env:
                    sql(env,payload=(ROOT/"migrations/bootstrap_multithread_safe.sql").read_bytes())
                    with mock.patch.dict(os.environ,env,clear=True):
                        manifest=migrations.load_manifest();executor=migrations.MysqlExecutor(manifest)
                        executor.adopt("fresh_bootstrap");migrations.run_pending(manifest,executor)
                    with pymysql.connect(unix_socket=env["DB_SOCKET"],user="root",database="duris_restore",
                            autocommit=True,cursorclass=pymysql.cursors.DictCursor) as owner:
                        with owner.cursor() as c:
                            c.execute("CREATE USER 'room_reader'@'localhost' IDENTIFIED BY 'disposable-room-reader'")
                            c.execute("GRANT SELECT ON duris_restore.* TO 'room_reader'@'localhost'")
                        with pymysql.connect(unix_socket=env["DB_SOCKET"],user="room_reader",password="disposable-room-reader",
                                database="duris_restore",autocommit=True,cursorclass=pymysql.cursors.DictCursor) as reader:
                            self.exercise_database(owner,reader,bodies,observations,engine)
        (work/"evidence.json").write_text(json.dumps(observations,indent=2)+"\n",encoding="utf-8")
        print("SQL_ROOM_ITEM_AUDIT "+json.dumps(observations,sort_keys=True),flush=True)

    def exercise_database(self,owner,reader,bodies,observations,engine):
        import pymysql
        operation=bytes.fromhex(OP);lineage=bytes.fromhex("22"*16);epoch=bytes.fromhex("33"*16)
        with owner.cursor() as c:
            c.execute("SELECT COUNT(*) AS n FROM mud_schema_history");self.assertEqual(c.fetchone()["n"],64)
            c.execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
                "schema_version,payload_version,status,result_payload) VALUES (%s,%s,%s,5,2,10,1,X'')",
                (operation,bytes(32),bytes(32)))
            c.execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,predecessor,transition_kind,transition_digest,"
                "creating_operation_id) VALUES (%s,%s,1,NULL,1,%s,%s)",(lineage,epoch,bytes(32),operation))
            # Deliberately synthetic capsules: satisfy storage, never full root authentication.
            c.execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,accounting_version,writer_id,"
                "policy_version,compiler_version,actor_kind,actor_id,reason,intent_digest,domain_digest,plan_digest,"
                "canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,child_count,"
                "item_event_count,before_witness_count,after_witness_count) "
                "VALUES (%s,%s,%s,1,4,1,1,1,7,4,%s,%s,%s,%s,%s,1,0,0,0,0,3,0,0)",
                (operation,lineage,epoch,bytes(32),bytes(32),bytes(32),bytes(256),bytes(256)))
            c.execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES (3,120,0,4)")
            for index,body in enumerate(bodies):
                uid=100+index;revision=11+index;parent=uid-1 if index else None
                c.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                    "owner_context_id,item_revision,vnum,state) VALUES (%s,100,%s,3,120,0,%s,1200,1)",(uid,parent,revision))
                c.execute("INSERT INTO sql_room_item_payload(item_uid,item_revision,payload_version,operation_id,season_epoch,payload) "
                    "VALUES (%s,%s,1,%s,1,%s)",(uid,revision,operation,body))
                c.execute("INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,parent_item_uid,"
                    "from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
                    "item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) "
                    "VALUES (%s,%s,%s,100,%s,1,7,0,3,120,0,%s,4,4,6,120,1)",(operation,index,uid,parent,revision))
                c.execute("INSERT INTO economic_accounting_item_reference(operation_id,event_index,line_index,child_index,item_uid,"
                    "before_revision,after_revision,legacy_operation_id,legacy_event_index) VALUES (%s,%s,%s,0,%s,%s,%s,%s,%s)",
                    (operation,index,index,uid,revision-1,revision,operation,index))

        class SelectCursor(pymysql.cursors.DictCursor):
            def execute(self,query,args=None):
                assert query.lstrip().upper().startswith(("SELECT ","SHOW ")),query
                return super().execute(query,args)

        def inventory():
            with owner.cursor() as c:
                c.execute("SHOW TABLES");names=sorted(next(iter(r.values())) for r in c.fetchall());rows=[]
                for name in names:
                    c.execute("SELECT * FROM `"+name+"`");rows.append((name,sorted(json.dumps(r,sort_keys=True,
                        default=lambda x:x.hex() if isinstance(x,bytes) else str(x)) for r in c.fetchall())))
            return rows

        def capture(label):
            before=inventory()
            try:
                with reader.cursor() as c:
                    c.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                    c.execute("START TRANSACTION READ ONLY, WITH CONSISTENT SNAPSHOT")
                with reader.cursor(SelectCursor) as c:result=exporter.read_room_item_custody(c)
                counts=[audit(result,limit)[0] for limit in (0,1,100)];self.assertEqual(counts[0],counts[1]);self.assertEqual(counts[0],counts[2])
            finally:
                with reader.cursor() as c:c.execute("ROLLBACK")
            self.assertEqual(before,inventory())
            observations.append(dict(engine=engine,label=label,counts=counts[0],native=result,
                whole_database_unchanged=True,synthetic_root=True,full_recovery_qualified=False))
            (ROOT/"bin/tests/sql-room-item-audit/evidence.json").write_text(json.dumps(observations,indent=2)+"\n",encoding="utf-8")
            return result,counts[0]

        baseline,counts=capture("intact-native-capture-bytes");self.assertEqual(counts,UNQUALIFIED)
        predecessor=None
        if os.environ.get("DURIS_ROOM_AUDIT_PREDECESSOR"):
            path=Path(os.environ["DURIS_ROOM_AUDIT_PREDECESSOR"])
            spec=importlib.util.spec_from_file_location("room_audit_predecessor",path);predecessor=importlib.util.module_from_spec(spec);spec.loader.exec_module(predecessor)

        def old_native():
            with reader.cursor() as c:
                c.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                c.execute("START TRANSACTION READ ONLY, WITH CONSISTENT SNAPSHOT")
            try:
                with reader.cursor(SelectCursor) as c:return predecessor.read_native(c,lineage)
            finally:
                with reader.cursor() as c:c.execute("ROLLBACK")

        before_old=old_native() if predecessor else None
        for label,changed in (("truncated",bodies[1][:-1]),("trailing",bodies[1]+b"\0")):
            try:
                with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(changed,))
                _,counts=capture(label);self.assertIn("room_item_payload_invalid",counts)
                if predecessor:self.assertEqual(old_native(),before_old);observations[-1]["predecessor_unchanged"]=True
            finally:
                with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1],))
        controls=(
            ("stale-revision","UPDATE item_current_owner SET item_revision=99 WHERE item_uid=101",
             "UPDATE item_current_owner SET item_revision=12 WHERE item_uid=101","room_item_current_payload_missing"),
            ("failed-receipt","UPDATE critical_operation_inbox SET failure_stage=1 WHERE operation_id=UNHEX('"+OP+"')",
             "UPDATE critical_operation_inbox SET failure_stage=0 WHERE operation_id=UNHEX('"+OP+"')","room_item_retained_binding_invalid"),
            ("wrong-reference","UPDATE economic_accounting_item_reference SET after_revision=13 WHERE item_uid=101",
             "UPDATE economic_accounting_item_reference SET after_revision=12 WHERE item_uid=101","room_item_retained_binding_invalid"),
            ("wrong-legacy-event","UPDATE economic_accounting_item_reference SET legacy_event_index=0 WHERE item_uid=101",
             "UPDATE economic_accounting_item_reference SET legacy_event_index=1 WHERE item_uid=101","room_item_retained_binding_invalid"),
            ("foreign-custody","UPDATE item_current_owner SET owner_id=121 WHERE item_uid=101",
             "UPDATE item_current_owner SET owner_id=120 WHERE item_uid=101","room_item_current_custody_invalid"),
            ("quarantined-child","UPDATE item_current_owner SET state=3 WHERE item_uid=101",
             "UPDATE item_current_owner SET state=1 WHERE item_uid=101","room_item_current_custody_invalid"),
            ("cycle","UPDATE item_current_owner SET parent_item_uid=102 WHERE item_uid=101",
             "UPDATE item_current_owner SET parent_item_uid=100 WHERE item_uid=101","room_item_graph_disconnected"),
            ("zero-owner-revision","UPDATE item_owner_revision SET revision=0 WHERE owner_type=3",
             "UPDATE item_owner_revision SET revision=4 WHERE owner_type=3","room_item_current_custody_invalid"),
            ("saved-duplicate","INSERT INTO saved_items(item_key,room_vnum,vnum,obj_uid) VALUES ('PRIVATE-duplicate',120,1200,101)",
             "DELETE FROM saved_items WHERE obj_uid=101","room_item_duplicate_saved_payload"))
        for label,change,restore,expected in controls:
            if label in ("wrong-reference","wrong-legacy-event"):
                before=inventory()
                with owner.cursor() as c:
                    with self.assertRaises(pymysql.IntegrityError):c.execute(change)
                self.assertEqual(before,inventory())
                observations.append(dict(engine=engine,label=label,canonical_fk_refused=True,whole_database_unchanged=True,
                    synthetic_root=True,full_recovery_qualified=False))
                continue
            try:
                with owner.cursor() as c:c.execute(change)
                _,counts=capture(label);self.assertIn(expected,counts)
            finally:
                with owner.cursor() as c:c.execute(restore)
        try:
            with owner.cursor() as c:c.execute("DELETE FROM sql_room_item_payload")
            result,counts=capture("all-payloads-missing-original-typed-drop")
            self.assertEqual(result["room_item_roots"],[dict(root=100)])
            self.assertEqual(counts["room_item_current_payload_missing"],3)
            if predecessor:self.assertEqual(old_native(),before_old);observations[-1]["predecessor_unchanged"]=True
        finally:
            with owner.cursor() as c:
                for index,body in enumerate(bodies):c.execute("INSERT INTO sql_room_item_payload VALUES (%s,%s,1,%s,1,%s)",(100+index,11+index,operation,body))
        for label,change,restore in (
            ("moved-to-player","UPDATE item_current_owner SET owner_type=1,owner_id=7","UPDATE item_current_owner SET owner_type=3,owner_id=120"),
            ("prior-season","UPDATE season_reset_state SET season_epoch=2 WHERE state_id=1","UPDATE season_reset_state SET season_epoch=1 WHERE state_id=1")):
            try:
                with owner.cursor() as c:c.execute(change)
                result,counts=capture(label);self.assertFalse(result["room_item_roots"]);self.assertEqual(counts,UNQUALIFIED)
            finally:
                with owner.cursor() as c:c.execute(restore)
        with reader.cursor() as c:
            c.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ");c.execute("START TRANSACTION READ ONLY, WITH CONSISTENT SNAPSHOT")
        try:
            with reader.cursor(SelectCursor) as c:self.assertEqual(exporter.read_room_item_custody(c),baseline)
            with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1][:-1],))
            with reader.cursor(SelectCursor) as c:self.assertEqual(exporter.read_room_item_custody(c),baseline)
            with reader.cursor() as c:c.execute("ROLLBACK")
            self.assertIn("room_item_payload_invalid",capture("fresh-cut-after-late-writer")[1])
        finally:
            with reader.cursor() as c:c.execute("ROLLBACK")
            with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1],))
        try:
            with owner.cursor() as c:
                c.executemany("INSERT INTO sql_room_item_payload VALUES (%s,1,1,%s,1,%s)",
                    [(1000+i,operation,bytes(MAX_ROOM_ITEM_BYTES)) for i in range(129)])
            with reader.cursor(SelectCursor) as c:
                with self.assertRaisesRegex(exporter.ExportError,"exceed audit bounds"):exporter.read_room_item_custody(c)
        finally:
            with owner.cursor() as c:c.execute("DELETE FROM sql_room_item_payload WHERE item_uid>=1000")
        self.assertEqual(capture("restored")[0],baseline)
        with reader.cursor() as c:
            with self.assertRaises(pymysql.MySQLError):c.execute("UPDATE item_owner_revision SET revision=99")


if __name__=="__main__":unittest.main()
