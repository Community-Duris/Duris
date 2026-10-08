#!/usr/bin/env python3
"""Independent room restore refusals; modeled roots never qualify publication."""
import copy
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/"scripts"),str(ROOT/"tests/async")]
import economic_room_restore_evidence as restore_room
import economic_sql_audit_snapshot as exporter
from test_sql_room_item_custody_audit import UNQUALIFIED, packet


class Executor:
    def __init__(self, outputs):
        self.outputs=iter(outputs);self.queries=[]

    def sql(self, query):
        assert query.startswith("SELECT "),query
        self.queries.append(query)
        return next(self.outputs)


class RoomRestoreTests(unittest.TestCase):
    def test_cli_pages_preserve_binary_and_null_values(self):
        lines=lambda rows:"\n".join(json.dumps(row) for row in rows)
        executor=Executor([lines([[i,"11"*16] for i in range(256)]),lines([[256,None],[257,"22"*16]])])
        rows=restore_room.RoomRows(executor)("SELECT item_uid AS uid,operation_id FROM payloads ORDER BY uid",
                                           ("uid","operation_id"),("operation_id",))
        self.assertEqual(len(rows),258);self.assertEqual(rows[0],dict(uid=0,operation_id=bytes.fromhex("11"*16)))
        self.assertIsNone(rows[256]["operation_id"]);self.assertEqual(rows[257]["operation_id"],bytes.fromhex("22"*16))
        self.assertIn("OFFSET 256",executor.queries[1])
        executor=Executor([lines([[i,"00"] for i in range(4)]),lines([[4,"ff"]])])
        rows=restore_room.RoomRows(executor)("SELECT uid,payload FROM payloads ORDER BY uid",("uid","payload"),("payload",))
        self.assertEqual(rows[-1],dict(uid=4,payload=b"\xff"));self.assertIn("LIMIT 4 OFFSET 4",executor.queries[1])

    def test_cli_malformed_rows_and_aggregate_bounds_refuse(self):
        for output in ("{}","[1]","[1,[]]","[1,3]",'[1,"GG"]','[1,"AA"]',"invalid"):
            with self.subTest(output=output),self.assertRaisesRegex(RuntimeError,"restore_room_item_packet_invalid"):
                restore_room.RoomRows(Executor([output]))("SELECT uid,operation_id FROM payloads",("uid","operation_id"),("operation_id",))
        for target,value in (("MAX_INPUT_BYTES",4),("MAX_ROWS",0)):
            with mock.patch.object(restore_room,target,value),self.assertRaisesRegex(RuntimeError,"capture_bounds"):
                restore_room.RoomRows(Executor(['[1,"11"]\n[2,"22"]']))("SELECT uid,payload FROM payloads",("uid","payload"),("payload",))

    def test_integer_storage_fence_precedes_json_normalization(self):
        count=sum(map(len,restore_room.INTEGER_SOURCES.values()))
        for shape in ([count,1],[count-1,0],[float(count),0],[True,0],{},None):
            executor=Executor(["9",json.dumps(shape)])
            with self.assertRaisesRegex(RuntimeError,"source_invalid"):restore_room.capture_room_items(executor)
            self.assertEqual(len(executor.queries),2)
        executor=Executor(["8"])
        with self.assertRaisesRegex(RuntimeError,"source_invalid"):restore_room.capture_room_items(executor)
        self.assertEqual(len(executor.queries),1)

    def test_room_lob_preflight_refuses_before_payload_query(self):
        count=sum(map(len,restore_room.INTEGER_SOURCES.values()))
        for row in ([100001,0,0],[1,16777217,1],[1,1,131073]):
            executor=Executor(["9",json.dumps([count,0]),json.dumps(row)])
            with self.assertRaisesRegex(RuntimeError,"packet_invalid"):restore_room.capture_room_items(executor)
            self.assertEqual(len(executor.queries),3)
            self.assertNotIn("SUBSTRING(payload",executor.queries[-1])

    def test_structural_diagnosis_preserves_unqualified_authority(self):
        native=packet();before=copy.deepcopy(native)
        with mock.patch.object(restore_room,"capture_room_items",return_value=native):
            self.assertEqual(restore_room.require_room_item_integrity(None),UNQUALIFIED)
        self.assertEqual(native,before)
        native["room_item_payloads"][0]["payload"]="00"*native["room_item_payloads"][0]["payload_bytes"]
        with mock.patch.object(restore_room,"capture_room_items",return_value=native):
            with self.assertRaisesRegex(RuntimeError,"restore_room_item_payload_invalid"):
                restore_room.require_room_item_integrity(None)
        with mock.patch.object(restore_room,"capture_room_items",return_value=packet(0)):
            self.assertEqual(restore_room.require_room_item_integrity(None),{})


def native_fixture(work):
    original=(ROOT/"tests/async/sql_room_item_payload_test.cpp").read_text(encoding="utf-8")
    source='#include "economy/economic_accounting_intent.h"\n'+original[:original.index("static void unchanged_refusal")]+r'''
int main() {
    const std::vector<player_item_snapshot> items={item(100,-1),item(101,0),item(102,1)};
    const auto payload=command(items); sql_room_item_payload_batch batch;
    assert(sql_room_item_payload_capture(*payload,&batch));
    auto emit=[](const std::vector<uint8_t> &bytes) {
        for (uint8_t byte:bytes) std::printf("%02x",byte);
        std::printf("\n");
    };
    for (const auto &body:batch.payloads) emit(body);
    auto identity=[](uint8_t byte) {critical_operation_id id;id.bytes.fill(byte);return id;};
    economic_accounting_plan plan;
    auto &meta=plan.metadata;
    meta.lineage=identity(0x22);meta.epoch=identity(0x33);meta.operation_id=identity(0x11);
    meta.writer_id=4;meta.actor_kind=economic_actor_kind::domain;meta.actor_id=7;
    meta.reason=economic_reason::item_move;
    for (uint32_t index=0;index<3;++index) {
        economic_item_position before,after;
        before.owner={item_owner_type::player,7,0};before.state=item_custody_state::active;
        before.root_uid=100;before.parent_uid=index?99+index:0;before.revision=10+index;
        after=before;after.owner={item_owner_type::room,120,0};++after.revision;
        plan.items_before.push_back({100+index,before});plan.items_after.push_back({100+index,after});
        plan.item_events.push_back({index,0,100+index,before,after});
    }
    economic_frozen_intent frozen;frozen.admission.metadata=meta;
    frozen.command_binding[0]=21;frozen.domain_digest[0]=22;
    std::vector<uint8_t> intent,encoded;
    assert(economic_intent_encode(frozen,&intent)==economic_accounting_error::ok);
    assert(economic_intent_digest(frozen,&meta.intent_digest)==economic_accounting_error::ok);
    meta.domain_digest=frozen.domain_digest;
    assert(economic_plan_encode(plan,&encoded)==economic_accounting_error::ok);
    economic_accounting_plan verified;
    assert(economic_plan_decode(encoded,&verified)==economic_accounting_error::ok);
    emit(intent);emit(encoded);
}
'''
    cpp,binary=work/"native.cpp",work/"native";cpp.write_text(source,encoding="utf-8",newline="\n")
    sources=[str(cpp),"src/persistence/sql_room_item_payload.c","src/player/player_snapshot_codec.c",
        "src/economy/economic_accounting_plan.c","src/economy/economic_source_event.c",
        "src/economy/economic_accounting_types.c","src/economy/economic_accounting_intent.c",
        "src/persistence/critical_command.c","src/item/item_transfer_command.c",
        "src/world/quest_mobile_native_reference.c","src/item/craft_pouch_mutation.c",
        "src/economy/shop_trade_recovery_manifest.c","src/combat/chaos_pouch_ledger.c",
        "src/item/lockpick_retirement_continuation.c","src/economy/native_quest_cost.c",
        "src/economy/native_quest_coin_give.c"]
    command=["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror","-D__NO_MYSQL__",
        "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
        "-ffunction-sections","-fdata-sections","-Wl,--gc-sections","-Isrc",*sources,"-lcrypto","-o",str(binary)]
    subprocess.run(command,cwd=ROOT,check=True,timeout=300)
    output=subprocess.check_output([str(binary)],timeout=30)
    (work/"native-output.txt").write_bytes(output)
    blocks=[bytes.fromhex(line.decode()) for line in output.splitlines()];assert len(blocks)==5
    (work/"native-inputs.json").write_text(json.dumps(dict(command=command,
        binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
        output_sha256=hashlib.sha256(output).hexdigest(),modeled_root_not_producer_authority=True),indent=2)+"\n",encoding="utf-8")
    return blocks


@unittest.skipUnless(os.environ.get("DURIS_RUN_ROOM_RESTORE_SQL")=="1","requires isolated canonical room restore SQL services")
class RoomRestoreSQLTests(unittest.TestCase):
    def test_canonical_both_engines_cli_capture_and_restore_refusals(self):
        import migration_runner as migrations
        import persistence_restore as restore
        from test_persistence_backup_integration import sql
        work=ROOT/"bin/tests/room-item-restore";work.mkdir(parents=True,exist_ok=True)
        blocks=native_fixture(work);observations=[]
        with tempfile.TemporaryDirectory(prefix="plan5-restore-room-items-",dir="/") as directory:
            for engine in ("mariadb","mysql"):
                candidate=Path(directory)/engine;candidate.mkdir(mode=0o700)
                with restore.private_database(candidate,engine) as env:
                    sql(env,payload=(ROOT/"migrations/bootstrap_multithread_safe.sql").read_bytes())
                    with mock.patch.dict(os.environ,env,clear=True):
                        manifest=migrations.load_manifest();executor=migrations.MysqlExecutor(manifest)
                        try:executor.adopt("fresh_bootstrap");migrations.run_pending(manifest,executor)
                        finally:executor.release_lock()
                    self.exercise(env,blocks,engine,observations,work)
        (work/"observations.json").write_text(json.dumps(observations,indent=2)+"\n",encoding="utf-8")

    def exercise(self,env,blocks,engine,observations,work):
        import pymysql
        import qualify_database_restore as qualifier
        from economic_item_payload_audit import ROOM_COLLECTIONS, audit_room_items
        bodies,intent,plan=blocks[:3],blocks[3],blocks[4]
        operation,lineage,epoch=(bytes.fromhex(value*16) for value in ("11","22","33"))
        owner=pymysql.connect(unix_socket=env["DB_SOCKET"],user="root",database="duris_restore",autocommit=True)
        reader=None;predecessor=None
        if os.environ.get("DURIS_ROOM_RESTORE_PREDECESSOR"):
            path=Path(os.environ["DURIS_ROOM_RESTORE_PREDECESSOR"])
            spec=importlib.util.spec_from_file_location("room_restore_predecessor",path)
            predecessor=importlib.util.module_from_spec(spec);spec.loader.exec_module(predecessor)
        try:
            with owner.cursor() as c:
                c.execute("SELECT COUNT(*) FROM mud_schema_history");self.assertEqual(c.fetchone()[0],64)
                c.execute("CREATE USER 'room_restore_reader'@'localhost' IDENTIFIED BY 'disposable-room-restore'")
                c.execute("GRANT SELECT ON duris_restore.* TO 'room_restore_reader'@'localhost'")
                c.execute("INSERT INTO critical_operation_inbox(operation_id,command_hash,keys_hash,command_type,"
                    "schema_version,payload_version,status,result_payload,committed_at) VALUES (%s,%s,%s,5,2,10,1,X'',CURRENT_TIMESTAMP(6))",
                    (operation,intent[160:192],bytes(32)))
                c.execute("INSERT INTO economic_epoch(lineage,epoch,ordinal,transition_kind,transition_digest,creating_operation_id) "
                    "VALUES (%s,%s,1,1,%s,%s)",(lineage,epoch,bytes(32),operation))
                c.execute("INSERT INTO economic_lineage_state(lineage) VALUES (%s)",(lineage,))
                c.execute("INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES (3,120,0,4)")
                for index,body in enumerate(bodies):
                    uid,revision,parent=100+index,11+index,99+index if index else None
                    c.execute("INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
                        "owner_context_id,item_revision,vnum,state) VALUES (%s,100,%s,3,120,0,%s,1200,1)",(uid,parent,revision))
                    c.execute("INSERT INTO sql_room_item_payload VALUES (%s,%s,1,%s,1,%s)",(uid,revision,operation,body))
                    c.execute("INSERT INTO item_ownership_ledger(operation_id,event_index,item_uid,root_item_uid,parent_item_uid,"
                        "from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,"
                        "item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site) "
                        "VALUES (%s,%s,%s,100,%s,1,7,0,3,120,0,%s,3,4,6,120,1)",(operation,index,uid,parent,revision))
            reader=pymysql.connect(unix_socket=env["DB_SOCKET"],user="room_restore_reader",password="disposable-room-restore",
                database="duris_restore",autocommit=True)

            trace=[]
            class SelectExecutor:
                def __init__(self):self.queries=[]
                def sql(self,query):
                    assert query.startswith("SELECT "),query
                    self.queries.append(query)
                    with reader.cursor() as c:
                        c.execute(query);rows=c.fetchall();assert all(len(row)==1 for row in rows)
                        trace.append(dict(query=query,raw=[repr(row[0]) for row in rows]))
                        (work/(engine+"-sql-trace.json")).write_text(json.dumps(trace,indent=2)+"\n",encoding="utf-8")
                        return "\n".join(row[0].decode("utf-8") if isinstance(row[0],bytes) else str(row[0]) for row in rows)

            def inventory():
                with owner.cursor() as c:
                    c.execute("SHOW TABLES");tables=c.fetchall();result={}
                    for (name,) in tables:
                        c.execute("SELECT * FROM `"+name+"`");result[name]=sorted(map(repr,c.fetchall()))
                return result

            def invoke_main(module,expected=None):
                output=io.StringIO()
                local=dict(env,DB_USER="room_restore_reader",DB_PASSWD="disposable-room-restore")
                with mock.patch.dict(os.environ,local,clear=True),contextlib.redirect_stdout(output):
                    if expected:
                        with self.assertRaisesRegex(RuntimeError,"^"+expected+"$"):module.main()
                        return None
                    module.main()
                return json.loads(output.getvalue())

            def cut(label,expected=None,old_accepts=False,full=False):
                before=inventory();counts={};executor=SelectExecutor()
                try:
                    with reader.cursor() as c:
                        c.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                        c.execute("START TRANSACTION READ ONLY, WITH CONSISTENT SNAPSHOT")
                    native=restore_room.capture_room_items(executor)
                    with reader.cursor(pymysql.cursors.DictCursor) as c:
                        self.assertEqual(native,exporter.read_room_item_custody(c))
                        complete,_,_=exporter.read_native(c,lineage)
                    self.assertEqual({name:complete[name] for name in (*ROOM_COLLECTIONS,"room_item_custody_coverage")},native)
                    audit_room_items(complete,lambda code,**details:counts.__setitem__(code,counts.get(code,0)+1),
                                     restore_room.MAX_ROWS,restore_room.MAX_INPUT_BYTES)
                    if expected:
                        with self.assertRaisesRegex(RuntimeError,"^"+expected+"$"):
                            restore_room.require_room_item_integrity(executor)
                    else:
                        self.assertEqual(restore_room.require_room_item_integrity(executor),UNQUALIFIED)
                        self.assertEqual(counts,UNQUALIFIED)
                finally:
                    with reader.cursor() as c:c.execute("ROLLBACK")
                result=invoke_main(qualifier,expected) if full else None
                if full and expected is None:
                    self.assertEqual(result,dict(history="ok",reconciliation="ok",room_item_diagnostics=UNQUALIFIED))
                if old_accepts and predecessor:
                    self.assertEqual(invoke_main(predecessor),dict(history="ok",reconciliation="ok"))
                self.assertEqual(before,inventory())
                observations.append(dict(engine=engine,label=label,counts=counts,result=result,expected_refusal=expected,
                    cli_dbapi_full_native_equal=True,whole_database_unchanged=True,modeled_root_not_producer_authority=True,
                    predecessor_accepted=bool(old_accepts and predecessor),full_qualifier_executed=full,queries=len(executor.queries)))
                (work/"observations.json").write_text(json.dumps(observations,indent=2)+"\n",encoding="utf-8")
                return native

            cut("missing-original-root","restore_room_item_retained_binding_invalid",old_accepts=True,full=True)

            def install_root():
                with owner.cursor() as c:
                    c.execute("INSERT INTO economic_accounting_operation(operation_id,lineage,epoch,accounting_version,writer_id,"
                        "policy_version,compiler_version,actor_kind,actor_id,reason,intent_digest,domain_digest,plan_digest,"
                        "canonical_intent,canonical_plan,outcome,result_code,account_count,posting_count,child_count,"
                        "item_event_count,before_witness_count,after_witness_count) "
                        "VALUES (%s,%s,%s,1,4,1,1,1,7,32,%s,%s,%s,%s,%s,1,0,0,0,0,3,3,3)",
                        (operation,lineage,epoch,hashlib.sha256(b"DURIS-ECONOMIC-INTENT-V1\0"+intent).digest(),
                         intent[192:224],hashlib.sha256(plan).digest(),intent,plan))
                    for index in range(3):
                        c.execute("INSERT INTO economic_accounting_item_reference(operation_id,event_index,line_index,child_index,"
                            "item_uid,before_revision,after_revision,legacy_operation_id,legacy_event_index) "
                            "VALUES (%s,%s,%s,0,%s,%s,%s,%s,%s)",(operation,index,index,100+index,10+index,11+index,operation,index))
            install_root();baseline=cut("intact-native-codecs-modeled-history",old_accepts=True,full=True)
            for label,body in (("truncated",bodies[1][:-1]),("trailing",bodies[1]+b"\0")):
                try:
                    with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(body,))
                    cut(label,"restore_room_item_payload_invalid",old_accepts=True,full=True)
                finally:
                    with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1],))
            controls=(
                ("stale-revision","UPDATE item_current_owner SET item_revision=99 WHERE item_uid=101",
                 "UPDATE item_current_owner SET item_revision=12 WHERE item_uid=101","current_payload_missing"),
                ("foreign-custody","UPDATE item_current_owner SET owner_id=121 WHERE item_uid=101",
                 "UPDATE item_current_owner SET owner_id=120 WHERE item_uid=101","current_binding_mismatch"),
                ("quarantined-child","UPDATE item_current_owner SET state=3 WHERE item_uid=101",
                 "UPDATE item_current_owner SET state=1 WHERE item_uid=101","current_custody_invalid"),
                ("parent-cycle","UPDATE item_current_owner SET parent_item_uid=102 WHERE item_uid=101",
                 "UPDATE item_current_owner SET parent_item_uid=100 WHERE item_uid=101","current_binding_mismatch"),
                ("zero-owner-clock","UPDATE item_owner_revision SET revision=0 WHERE owner_type=3",
                 "UPDATE item_owner_revision SET revision=4 WHERE owner_type=3","current_custody_invalid"),
                ("saved-duplicate","INSERT INTO saved_items(item_key,room_vnum,vnum,obj_uid) VALUES ('RESTORE-duplicate',120,1200,101)",
                 "DELETE FROM saved_items WHERE obj_uid=101","duplicate_saved_payload"))
            for label,change,repair,error in controls:
                try:
                    with owner.cursor() as c:c.execute(change)
                    cut(label,"restore_room_item_"+error)
                finally:
                    with owner.cursor() as c:c.execute(repair)
            try:
                with owner.cursor() as c:c.execute("DELETE FROM sql_room_item_payload")
                native=cut("all-sidecars-missing-typed-drop","restore_room_item_current_payload_missing",old_accepts=True,full=True)
                self.assertEqual(native["room_item_roots"],[dict(root=100)])
            finally:
                with owner.cursor() as c:
                    for i,body in enumerate(bodies):c.execute("INSERT INTO sql_room_item_payload VALUES (%s,%s,1,%s,1,%s)",
                                                            (100+i,11+i,operation,body))
            for label,change,repair in (
                ("later-player-custody","UPDATE item_current_owner SET owner_type=1,owner_id=7",
                 "UPDATE item_current_owner SET owner_type=3,owner_id=120"),
                ("prior-season-history","UPDATE season_reset_state SET season_epoch=2 WHERE state_id=1",
                 "UPDATE season_reset_state SET season_epoch=1 WHERE state_id=1")):
                try:
                    with owner.cursor() as c:c.execute(change)
                    self.assertEqual(cut(label)["room_item_roots"],[])
                finally:
                    with owner.cursor() as c:c.execute(repair)
            try:
                with owner.cursor() as c:
                    c.executemany("INSERT INTO sql_room_item_payload VALUES (%s,1,1,%s,1,%s)",
                                  [(1000+i,operation,bodies[0]) for i in range(5)])
                native=cut("invalid-history-on-second-payload-page","restore_room_item_payload_invalid",old_accepts=True)
                self.assertEqual(len(native["room_item_payloads"]),8)
            finally:
                with owner.cursor() as c:c.execute("DELETE FROM sql_room_item_payload WHERE item_uid>=1000")
            try:
                with reader.cursor() as c:
                    c.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
                    c.execute("START TRANSACTION READ ONLY, WITH CONSISTENT SNAPSHOT")
                self.assertEqual(restore_room.capture_room_items(SelectExecutor()),baseline)
                with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1][:-1],))
                self.assertEqual(restore_room.capture_room_items(SelectExecutor()),baseline)
                self.assertEqual(restore_room.require_room_item_integrity(SelectExecutor()),UNQUALIFIED)
                with reader.cursor() as c:c.execute("ROLLBACK")
                cut("fresh-cut-after-late-writer","restore_room_item_payload_invalid",old_accepts=True)
            finally:
                with reader.cursor() as c:c.execute("ROLLBACK")
                with owner.cursor() as c:c.execute("UPDATE sql_room_item_payload SET payload=%s WHERE item_uid=101",(bodies[1],))
            try:
                with owner.cursor() as c:
                    c.executemany("INSERT INTO sql_room_item_payload VALUES (%s,1,1,%s,1,%s)",
                                  [(1000+i,operation,bytes(131072)) for i in range(129)])
                before=inventory();executor=SelectExecutor()
                with self.assertRaisesRegex(RuntimeError,"restore_room_item_packet_invalid"):
                    restore_room.capture_room_items(executor)
                self.assertEqual(len(executor.queries),3)
                self.assertNotIn("SUBSTRING(payload",executor.queries[-1]);self.assertEqual(before,inventory())
                observations.append(dict(engine=engine,label="aggregate-lob-refused-before-read",whole_database_unchanged=True,
                    queries=len(executor.queries),modeled_root_not_producer_authority=True))
            finally:
                with owner.cursor() as c:c.execute("DELETE FROM sql_room_item_payload WHERE item_uid>=1000")
            try:
                with owner.cursor() as c:c.execute("UPDATE economic_accounting_operation SET canonical_plan=%s WHERE operation_id=%s",
                                                (plan+b"\0",operation))
                before=inventory();invoke_main(qualifier,"restore_economic_plan_mismatch");self.assertEqual(before,inventory())
                observations.append(dict(engine=engine,label="original-canonical-root-refusal-preserved",whole_database_unchanged=True,
                    modeled_root_not_producer_authority=True))
            finally:
                with owner.cursor() as c:c.execute("UPDATE economic_accounting_operation SET canonical_plan=%s WHERE operation_id=%s",(plan,operation))
            self.assertEqual(cut("restored",full=True),baseline)
            with owner.cursor() as c:
                c.execute("SELECT active_epoch IS NULL FROM economic_lineage_state");self.assertEqual(c.fetchone()[0],1)
            with reader.cursor() as c:
                with self.assertRaises(pymysql.MySQLError):c.execute("UPDATE item_owner_revision SET revision=99")
        finally:
            if reader is not None:reader.close()
            owner.close()


if __name__=="__main__":unittest.main()
