#!/usr/bin/env python3
"""Source contracts for the live ownership ACK boundary."""

from _paths import SRC, extract_function
from pathlib import Path
import unittest
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
GIVE_PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cstring>
struct object;
struct character {int pid=0,in_room=1,carry_w=0,carry_n=0,runtime_flags=0; object *carrying=nullptr;};
struct object {int R_num=1,type=1,value[8]{},runtime_flags=0,extra_flags=0,loc_p=0,z_cord=0,g_key=0; uint64_t obj_uid=1; object *next_content=nullptr; const char *short_description="sword"; struct {character *carrying=nullptr;} loc;};
using P_obj=object*; using P_char=character*;
enum class obj_to_char_result {placed,deferred,destroyed,rejected};
enum class item_owner_type {player};
enum class item_custody_state {active};
struct item_owner_identity {item_owner_type type; uint64_t id,context;};
struct item_ownership_runtime_entry {item_owner_identity owner; item_custody_state state;};
constexpr int MAX_STRING_LENGTH=1024,LOG_MOB=0,LOG_OBJ=0,LOG_DEBUG=0,LOG_FILE=0,NOWHERE=-1,
    ITEM2_CRUMBLELOOT=1,ITEM_MONEY=2,ITEM_CORPSE=3,CORPSE_FLAGS=0,PC_CORPSE=1,
    OBJ_RFLAG_CREATION_CANDIDATE=2,ITEM_LIT=4,ITEM_LIGHT=5,REAL=0,LOC_CARRIED=1,
    CHAR_RFLAG_DIRTY_INVENTORY=4,TO_NOTVICT=0,TO_VICT=1;
constexpr bool TRUE=true,FALSE=false;
constexpr int PLAYER_COMPONENT_STATUS=1,PLAYER_COMPONENT_EQUIPMENT=2,PLAYER_COMPONENT_INVENTORY=4;
#define IS_PC(c) true
#define IS_NPC(c) false
#define IS_TRUSTED(c) false
#define GET_PID(c) ((c)->pid)
#define GET_VNUM(c) 0
#define GET_NAME(c) "player"
#define J_NAME(c) "player"
#define GET_LEVEL(c) 30
#define OBJ_VNUM(o) 100
#define OBJ_NOWHERE(o) ((o)->loc_p==0)
#define IS_OBJ_STAT2(o,f) false
#define IS_SET(n,f) ((n)&(f))
#define SET_BIT(n,f) ((n)|=(f))
#define GET_CARRYING_W(c) ((c)->carry_w)
#define IS_CARRYING_N(c) ((c)->carry_n)
#define GET_OBJ_WEIGHT(o) 1
#define IS_ARTIFACT(o) false
void logit(int,const char*,...) {}
bool training_dummy_is(P_char) {return false;}
void obj_to_room(P_obj,int) {}
void send_to_room(const char*,int) {}
void send_to_char(const char*,P_char) {}
void extract_obj(P_obj,bool) {}
static uint64_t owner_pid = 1;
bool item_ownership_runtime_lookup(uint64_t,item_ownership_runtime_entry *out) {*out={{item_owner_type::player,owner_pid,0},item_custody_state::active};return true;}
bool item_owner_identity_equal(item_owner_identity a,item_owner_identity b) {return a.id==b.id;}
bool item_creation_grant_submit_to_player(P_char,P_obj,P_char) {return false;}
void char_light(P_char) {}
void room_light(int,int) {}
void world_activity_object_enter(P_obj) {}
int encumbrance_weight(int v) {return v;}
void artifact_update_location_sql(P_obj) {}
void mark_char_or_owner_dirty(P_char) {}
void mark_player_dirty_components(int,int) {}
void act(const char*,int,P_char,P_obj,P_char,int) {}
void obj_from_char(P_obj o) {o->loc.carrying->carrying=nullptr;o->loc.carrying=nullptr;o->loc_p=0;}

#define OBJ_CARRIED_BY(o,c) ((o)->loc_p==LOC_CARRIED && (o)->loc.carrying==(c))
constexpr int AVATAR=60;
enum class item_transfer_reason { player_give };
enum class item_movement_reject { none };
struct item_transfer_result {};
static P_char recipient_view=nullptr;
static P_obj live_object=nullptr;
static bool admitted=true;
using callback=void(*)(P_char,bool,const item_transfer_result &,unsigned,const uint8_t *,size_t);
static callback pending_callback=nullptr;
static std::vector<uint8_t> pending_context;
static int scripts_run=0;
P_char find_live_player_pid(uint32_t pid) { return recipient_view && recipient_view->pid==static_cast<int>(pid) ? recipient_view : nullptr; }
P_obj find_live_item_uid(uint64_t uid) { return live_object && live_object->obj_uid==uid ? live_object : nullptr; }
bool item_command_uses_durable_ownership(P_obj) { return true; }
void report_movement_reject(P_char,item_movement_reject,const char *,P_obj) {}
void persistence_alert(int,const char *,const char *,const char *,const char *,const char *,...) {}
void nq_action_check(P_char,P_char,void *) {}
void studioproc_give(P_char,P_obj,P_char) { ++scripts_run; }
bool item_movement_transaction_submit(P_char,P_obj,P_obj,item_owner_identity,item_owner_identity,
    item_transfer_reason,int64_t,callback complete,const void *context,size_t size,P_obj,item_movement_reject *) {
    if(!admitted) return false;
    pending_callback=complete;
    pending_context.assign(static_cast<const uint8_t *>(context),static_cast<const uint8_t *>(context)+size);
    return true;
}
'''

GIVE_DRIVER = r'''
int main() {
    character donor,recipient; donor.pid=1; recipient.pid=2;
    object sword; donor.carrying=&sword; sword.loc.carrying=&donor; sword.loc_p=LOC_CARRIED;
    live_object=&sword; recipient_view=&recipient;
    admitted=false;
    give_fragment(&donor,&recipient,&sword);
    assert(donor.carrying==&sword && !recipient.carrying && !OBJ_NOWHERE(&sword));
    admitted=true;
    give_fragment(&donor,&recipient,&sword);
    assert(pending_callback && donor.carrying==&sword && !recipient.carrying);
    pending_callback(&donor,false,{},0,pending_context.data(),pending_context.size());
    assert(donor.carrying==&sword && !recipient.carrying && owner_pid==1);
    give_fragment(&donor,&recipient,&sword);
    owner_pid=2; // The committed receipt updates authoritative ownership first.
    pending_callback(&donor,true,{},0,pending_context.data(),pending_context.size());
    assert(!donor.carrying && recipient.carrying==&sword && OBJ_CARRIED_BY(&sword,&recipient));
    assert(scripts_run==1);
}
'''

class LiveItemMovementContractTests(unittest.TestCase):
    def test_runtime_hydrates_authoritative_rows_not_relative_events(self):
        sql = (SRC / "sql.c").read_text()
        owner_check = sql[sql.rindex("bool sql_persistence_item_owner_matches_identity"):]
        owner_check = owner_check[:owner_check.index("bool sql_hydrate_item_owner_revisions")]
        self.assertIn("FROM item_current_owner", owner_check)
        self.assertIn("item_owner_revision", owner_check)
        self.assertNotIn("persistence_item_events", owner_check)
        self.assertIn("item_ownership_runtime_hydrate", owner_check)

    def test_pending_adapter_retains_scalars_and_publishes_after_commit(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        pending = movement[movement.index("struct pending_movement"):]
        pending = pending[:pending.index("};")]
        self.assertNotIn("P_obj", pending)
        self.assertNotIn("P_char", pending)
        self.assertIn("critical_command_coordinator_submit", movement)
        self.assertIn("critical_command_coordinator_submit_for_publication", movement)
        self.assertIn("critical_command_coordinator_acknowledge_publication", movement)
        self.assertIn("critical_command_coordinator_is_fenced", movement)
        self.assertIn("item_ownership_runtime_apply", movement)
        self.assertIn("publication_status", movement)
        self.assertIn("ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS", movement)
        batch = extract_function(
            "item_movement_transaction.c", "bool item_movement_transaction_submit_batch("
        )
        self.assertIn("movement_conflicts(from_owner, to_owner)", batch)
        self.assertLess(
            batch.index("movement_conflicts(from_owner, to_owner)"),
            batch.index("critical_command_coordinator_submit"),
        )
        self.assertLess(movement.index("const bool committed"),
                        movement.index("item_ownership_runtime_apply"))

    def test_transfer_captures_exact_snapshot_before_submission(self):
        movement = extract_function("item_movement_transaction.c", "bool item_movement_transaction_submit(")
        capture = movement.index("player_item_snapshot_tree_capture")
        encode = movement.index("player_item_snapshot_list_encode")
        build = movement.index("item_transfer_command_build")
        submit = movement.index("critical_command_coordinator_submit")
        self.assertLess(capture, encode)
        self.assertLess(encode, build)
        self.assertLess(build, submit)
        self.assertIn("payload.item_blob_size", movement)

    def test_audited_commands_defer_pointer_mutation(self):
        actobj = (SRC / "actobj.c").read_text()
        for reason in ("player_get", "player_drop", "player_put", "player_give",
                       "corpse_loot"):
            self.assertIn(f"item_transfer_reason::{reason}", actobj)
        self.assertIn("item_movement_transaction_submit", actobj)
        self.assertIn("item_get_ack_publication", actobj)
        self.assertIn("item_put_ack_publication", actobj)
        self.assertIn("start_container_bulk_get", actobj)
        self.assertIn("bulk_get_completion", actobj)
        self.assertIn("start_bulk_drop", actobj)
        self.assertIn("bulk_drop_completion", actobj)
        self.assertIn("start_bulk_put", actobj)
        self.assertIn("bulk_put_completion", actobj)
        self.assertIn("item_movement_transaction_submit_batch", actobj)
        # Each core bulk movement publishes one durable forest only after its
        # shared commit.
        self.assertNotIn("Durable container items must be collected one at a time", actobj)
        self.assertNotIn("Durable items must be dropped one at a time", actobj)
        self.assertNotIn("Durable items must be put away one at a time", actobj)

    def test_pc_corpse_roots_bypass_generic_ownership_transfers(self):
        actobj = (SRC / "actobj.c").read_text()
        policy = (SRC / "item/item_command_policy.c").read_text()
        self.assertIn("ITEM_CORPSE", policy)
        self.assertIn("PC_CORPSE", policy)
        self.assertGreaterEqual(
            actobj.count("item_command_uses_durable_ownership("), 10
        )
        get_body = actobj[actobj.index("void get(P_char ch") :]
        get_body = get_body[: get_body.index("int fight_in_room")]
        self.assertIn(
            "IS_PC(ch) && item_command_uses_durable_ownership(o_obj)", get_body
        )

    def test_death_items_are_chained_after_ack_and_failure_is_preserved(self):
        fight = (SRC / "fight.c").read_text()
        make_corpse = fight[fight.index("P_obj make_corpse"):]
        self.assertIn("submit_next_corpse_item", fight)
        self.assertIn("item_transfer_reason::corpse_create", fight)
        self.assertIn("corpse->weight = GET_WEIGHT(ch);", fight)
        self.assertIn("int contents_weight = total_carried_weight(ch);", fight)
        self.assertIn("sizeof(context), corpse", fight)
        self.assertIn("failed_preserved", fight)
        self.assertNotIn("sql_delete_player_items", make_corpse)
        completion = fight[fight.index("void corpse_item_completion"):]
        self.assertLess(completion.index("if (!committed)"), completion.index("obj_from_char"))

    def test_corpse_identity_and_floor_hints_are_non_authoritative(self):
        migration = (ROOT / "migrations/live_item_movement_cutover.sql").read_text()
        baseline = (ROOT / "migrations/baseline_item_ownership.sh").read_text()
        self.assertIn("c.save_id", migration)
        self.assertIn("CAST(pd.pid AS UNSIGNED) << 32", baseline)
        self.assertIn("CAST(c.save_id AS UNSIGNED)", baseline)
        actobj = (SRC / "actobj.c").read_text()
        self.assertNotIn("redis_check_floor_pickup", actobj)
        self.assertNotIn("redis_check_floor_drop", actobj)
        recovery = (SRC / "world_recovery_pipeline.c").read_text()
        self.assertIn("sql_persistence_reconcile_world_recovery_items", recovery)
        self.assertIn("item_ownership_runtime_hydrate_many_atomic", recovery)
        self.assertNotIn("redis_check_floor_pickup", recovery)
        self.assertNotIn("redis_check_floor_drop", recovery)

    def test_reconnect_replays_retained_completion(self):
        nanny = (SRC / "nanny.c").read_text()
        self.assertGreaterEqual(nanny.count("item_movement_transaction_player_ready"), 2)

    def test_linkdead_actor_completion_publishes_while_character_is_live(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        helper = movement[movement.index("P_char find_live_player(uint32_t pid)") :]
        helper = helper[: helper.index("bool creation_grant_request_valid")]
        self.assertIn("character_list", helper)
        completions = movement[
            movement.index("void item_movement_transaction_handle_completions") :
        ]
        completions = completions[
            : completions.index("void item_movement_transaction_player_ready")
        ]
        self.assertIn("find_live_player(found->second.actor_pid)", completions)
        self.assertNotIn("find_player_by_pid(found->second.actor_pid)", completions)

    def test_give_completion_can_publish_to_a_linkdead_recipient(self):
        actobj = (SRC / "actobj.c").read_text()
        helper = actobj[actobj.index("P_char find_live_player_pid"):]
        helper = helper[:helper.index("void item_get_completion")]
        self.assertIn("character_list", helper)
        give = actobj[actobj.index("void item_give_completion"):]
        give = give[:give.index("void item_put_completion")]
        self.assertIn("find_live_player_pid(context.recipient_pid)", give)
        self.assertNotIn("find_player_by_pid(context.recipient_pid)", give)

    def test_same_owner_reparenting_is_authoritative(self):
        command = (SRC / "item_transfer_command.c").read_text()
        repository = (SRC / "item_transfer_repository.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        self.assertIn("target_root_item_uid", command)
        self.assertIn("target_parent_item_uid", command)
        self.assertIn("const bool same_owner", repository)
        self.assertIn("SET root_item_uid=?", repository)
        self.assertIn("target_container", movement)

    def test_flat_corpse_creation_and_loot_are_composite(self):
        repository = (SRC / "flatfile_item_repository.c").read_text()
        world = (SRC / "flatfile_world_item_repository.c").read_text()
        artifact = (SRC / "flatfile_artifact_repository.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        self.assertIn("flatfile_world_item_prepare_corpse_transfer", world)
        self.assertIn("flatfile_artifact_prepare_corpse_transfer", artifact)
        self.assertIn("capture_corpse_metadata", movement)
        self.assertIn("corpse_loot_transfer(payload)", repository)
        self.assertIn("corpse_create_transfer(payload)", repository)
        apply = repository[repository.index(
            "critical_apply_result flatfile_item_repository_apply") :]
        prepare = apply.index("flatfile_world_item_prepare_corpse_transfer")
        artifact_prepare = apply.index("flatfile_artifact_prepare_corpse_transfer", prepare)
        image = apply.index("corpse.after_image", prepare)
        artifact_image = apply.index("corpse_artifacts.after_image", artifact_prepare)
        commit = apply.index("flatfile_authority_transaction_commit", image)
        self.assertLess(prepare, image)
        self.assertLess(artifact_prepare, artifact_image)
        self.assertLess(image, commit)
        self.assertLess(artifact_image, commit)
        supported = repository[repository.index("bool generic_transfer_supported") :]
        supported = supported[:supported.index("bool locker_custody_matches")]
        self.assertIn("corpse_loot_transfer(payload)", supported)
        self.assertIn("corpse_create_transfer(payload)", supported)
        self.assertIn("ITEM_TRANSFER_CORPSE_PAYLOAD_VERSION", supported)

    def test_flat_room_item_moves_are_composite(self):
        repository = (SRC / "flatfile_item_repository.c").read_text()
        world = (SRC / "flatfile_world_item_repository.c").read_text()
        artifact = (SRC / "flatfile_artifact_repository.c").read_text()
        self.assertIn("flatfile_world_item_prepare_room_transfer", world)
        self.assertIn("flatfile_artifact_prepare_room_transfer", artifact)
        supported = repository[repository.index("bool generic_transfer_supported") :]
        supported = supported[:supported.index("bool locker_custody_matches")]
        self.assertIn("room_transfer(payload)", supported)
        apply = repository[repository.index(
            "critical_apply_result flatfile_item_repository_apply") :]
        prepare = apply.index("flatfile_world_item_prepare_room_transfer")
        artifact_prepare = apply.index("flatfile_artifact_prepare_room_transfer", prepare)
        image = apply.index("room.after_image", prepare)
        artifact_image = apply.index("room_artifacts.after_image", artifact_prepare)
        commit = apply.index("flatfile_authority_transaction_commit", image)
        self.assertLess(prepare, image)
        self.assertLess(artifact_prepare, artifact_image)
        self.assertLess(image, commit)
        self.assertLess(artifact_image, commit)
    def test_523_retain_shared_fences_until_safe_callback(self):
        actobj = (SRC / "actobj.c").read_text()
        movement = (SRC / "item_movement_transaction.c").read_text()
        movement_header = (SRC / "item/item_movement_transaction.h").read_text()
        self.assertIn("using item_movement_publication_fn", movement_header)
        self.assertIn("publication = nullptr", movement_header)
        self.assertIn("const empty_movement_context context = { actor_pid, actor->runtime_id }", actobj)
        self.assertIn("sizeof(context), NULL", actobj)
        self.assertIn("&reject, empty_completion", actobj)
        empty = extract_function("actobj.c", "bool empty_completion(")
        self.assertIn("return false", empty)
        self.assertIn("return true", empty)
        self.assertIn("actor->runtime_id != context.actor_runtime_id", empty)
        self.assertIn("critical_command_coordinator_acknowledge_publication", movement)

    def test_failed_publication_is_bounded_and_not_erased(self):
        movement = (SRC / "item_movement_transaction.c").read_text()
        failure = movement[movement.index("if (!published)"):]
        failure = failure[:failure.index("if (!critical_command_coordinator_acknowledge_publication")]
        self.assertIn("retain_publication_failure", failure)
        self.assertNotIn("pending.erase", failure)
        self.assertIn("publication_status", movement)
        self.assertIn("ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS", movement)
        coordinator = (SRC / "persistence/critical_command_coordinator.c").read_text()
        self.assertIn("publication_pending", coordinator)
        self.assertIn("state.retain_until_publication", coordinator)
        self.assertIn("if (!state.publication_pending)", coordinator)


    def test_give_keeps_custody_until_receipt(self):
        handler = extract_function("handler.c", "obj_to_char_result obj_to_char_checked(")
        context = extract_function("actobj.c", "struct give_movement_context") + ";"
        completion = extract_function("actobj.c", "void item_give_completion(")
        give = extract_function("actobj.c", "void do_give(")
        start = give.index("\tif (IS_PC(ch) && IS_PC(vict) && ch != vict")
        end = give.index("\n\tif (IS_TRUSTED(ch))", start)
        fragment = "void give_fragment(P_char ch,P_char vict,P_obj obj) {\n" + give[start:end] + "\n}"
        harness = GIVE_PRELUDE + handler + context + completion + fragment + GIVE_DRIVER
        with tempfile.TemporaryDirectory(prefix="give-custody-") as directory:
            source = Path(directory) / "give.cpp"
            binary = Path(directory) / "give"
            source.write_text(harness)
            subprocess.run(["g++", "-std=c++20", "-fsanitize=address,undefined", "-g",
                            str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    unittest.main()
