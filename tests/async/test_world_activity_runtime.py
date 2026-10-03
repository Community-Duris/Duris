#!/usr/bin/env python3
"""Production policy and scheduler lifecycle checks under ASan/UBSan.

Reuse the scheduler regression's collaborators without executing its test
runner. World activity is compiled as its own production translation unit.
"""
import ast
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
tree = ast.parse((Path(__file__).with_name("test_nevent_scheduler_runtime.py")).read_text())
base = next(ast.literal_eval(node.value) for node in tree.body
            if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "HARNESS" for target in node.targets))
base = base[:base.index("int main(int argc, char **argv)")]
base = base.replace("test_world[1]", "test_world[260]").replace("top_of_world = 0", "top_of_world = 259")
base = base.replace("DEFINE_LABEL_CALLBACK(event_mob_mundane)",
                    "void event_mob_mundane(P_char, P_char, P_obj, void *);")
base = base.replace("P_char get_linked_char(P_char, ush_int)\n{\n\treturn nullptr;\n}",
                    extract_function("affects.c", "P_char get_linked_char("))

MAIN = r'''
#include "world/world_activity.h"
#include "world/ferry.h"
#include "world/specs.prototypes.h"
#include "ships/ships.h"
#include <cassert>
#include <climits>
int top_of_mobt = 0;
int top_of_objt = 0;
static P_ship test_ship = nullptr;
static Ferry *test_ferry = nullptr;
ShipObjHash::ShipObjHash() : table{}, sz(0) {}
P_ship ShipObjHash::find(P_obj object) { return test_ship && test_ship->shipobj == object ? test_ship : nullptr; }
ShipObjHash shipObjHash;
Ferry::Ferry() : obj(nullptr) {}
Ferry *get_ferry_from_obj(int prototype) { return test_ferry && test_ferry->obj_num == prototype ? test_ferry : nullptr; }
int real_room(int vnum) { return vnum >= 0 && vnum <= top_of_world ? vnum : NOWHERE; }
int portal_door(P_obj, P_char, int, char *) { return 0; }
int portal_wormhole(P_obj, P_char, int, char *) { return 0; }
int portal_etherportal(P_obj, P_char, int, char *) { return 0; }
static std::map<std::string, int> properties;
int get_property(const char *key, int fallback) {
    auto it = properties.find(key); return it == properties.end() ? fallback : it->second;
}
static bool movement_enabled = false;
static P_char remember_array[MAX_ZONES]{};
static int chosen_direction = 0;
int number(int low, int high) {
    if (movement_enabled && low == 0 && high == NUM_EXITS) return chosen_direction;
    return low < 0 ? 0 : (low + high) / 2;
}
bool should_teacher_move(P_char) { return true; }
int exitnumb_to_cmd(int direction) { return direction; }
bool check_castle_walls(int, int) { return false; }
static int moves = 0;
void do_move(P_char ch, char *, int direction) {
    // Movement collaborator publishes a successful legal move through the
    // production membership hooks. Selection/rules/cadence use production code.
    const int destination = world[ch->in_room].dir_option[direction]->to_room;
    world_activity_character_leave(ch); ch->in_room = destination;
    world_activity_character_enter(ch); ++moves;
}
static void production_wander(P_char ch);
static bool repeat_mundane = false;
static int mundane_calls = 0;
static void unrelated_event(P_char, P_char, P_obj, void *) {}
void event_mob_mundane(P_char ch, P_char, P_obj, void *) {
    ++mundane_calls;
    if (movement_enabled) production_wander(ch);
    else if (repeat_mundane) world_activity_schedule_mundane(ch, false, false);
}

static void corpse(obj_data &object, int room, bool npc = false) {
    object.type = ITEM_CORPSE; object.value[1] = npc ? NPC_CORPSE : PC_CORPSE;
    object.loc_p = LOC_ROOM; object.loc.room = room;
}
static void mob(char_data &ch, npc_only_data &data, int room) {
    ch.specials.act = ACT_ISNPC; ch.specials.position = STAT_NORMAL | POS_STANDING;
    ch.only.npc = &data; ch.in_room = room; ch.points.hit = ch.points.max_hit = 100;
}
static void move_corpse(obj_data &object, int room) {
    world_activity_object_leave(&object); object.loc.room = room; world_activity_object_enter(&object);
}
int main() {
    reset_scheduler();
    static index_data indexes[1]{}; mob_index = indexes;
    // One large connected zone and two small ordinary zones, one-way edge.
    static room_direction_data exits[259]{};
    for (int i = 0; i < 260; ++i) {
        world[i].zone = i < 256 ? 0 : (i < 258 ? 1 : 2);
        if (i < 259) { exits[i].to_room = i + 1; world[i].dir_option[0] = &exits[i]; }
    }
    char_data pc{}, near{}, far{}; npc_only_data near_data{}, far_data{};
    mob(near, near_data, 0); mob(far, far_data, 200);
    near.next = &far; character_list = &near;
    world[0].people = &near; world[200].people = &far;
    world_activity_reload();
    assert(!world_activity_is_enabled());
    assert(world_activity_mundane_delay(&far, false, false) == 90);
    properties["world.activity.enabled"] = 1;
    world_activity_reload(); world_activity_rebuild();
    assert(world_activity_get_health().indexed_npcs == 2);
    assert(world_activity_mundane_delay(&far, false, false) == 240);
    world_activity_schedule_mundane(&near, false, false);
    world_activity_schedule_mundane(&far, false, false);
    pc.specials.position = STAT_NORMAL | POS_STANDING; pc.in_room = 0;
    world_activity_player_enter(&pc);
    assert(world_activity_tier_for_room(0) == world_activity_tier::active);
    assert(world_activity_tier_for_room(64) == world_activity_tier::nearby);
    assert(world_activity_tier_for_room(200) == world_activity_tier::distant);
    assert(ne_event_time(world_activity_mundane_event(&near).event) == 1);
    assert(ne_event_time(world_activity_mundane_event(&far).event) == 240);
    // Nearby -> active promotion also wakes; encounter room next eligible pulse.
    char_data boundary{}; npc_only_data boundary_data{}; mob(boundary, boundary_data, 64);
    world_activity_character_enter(&boundary); world[64].people = &boundary;
    world_activity_schedule_mundane_after(&boundary, 200);
    world_activity_player_leave(&pc); pc.in_room = 64; world_activity_player_enter(&pc);
    assert(ne_event_time(world_activity_mundane_event(&boundary).event) == 1);
    world_activity_player_leave(&pc);
    ne_event_tick += 361;
    pulse = ne_event_tick % PULSES_IN_TICK;
    assert(world_activity_tier_for_room(0) == world_activity_tier::distant);
    // Two offline-player corpses, independent removal, normal halo cadence.
    obj_data first{}, second{}, ordinary{}; corpse(first, 0); corpse(second, 0); corpse(ordinary, 0, true);
    world_activity_object_enter(&first); world_activity_object_enter(&second); world_activity_object_enter(&ordinary);
    assert(world_activity_get_health().corpse_reasons == 2);
    assert(world_activity_tier_for_room(64) == world_activity_tier::active);
    assert(world_activity_mundane_delay(&boundary, false, false) == 30);
    world_activity_object_leave(&first); assert(world_activity_get_health().corpse_reasons == 1);
    move_corpse(second, 200); assert(world_activity_get_health().corpse_reasons == 1);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world_activity_object_leave(&second); assert(world_activity_get_health().corpse_reasons == 0);
    // Nest, carry, wear and carrier movement use the exact committed topology.
    obj_data container{}; container.loc_p = LOC_CARRIED; container.loc.carrying = &pc; pc.in_room = 0;
    container.contains = &first; first.loc_p = LOC_INSIDE; first.loc.inside = &container;
    pc.carrying = &container;
    world_activity_object_enter(&container); assert(world_activity_get_health().corpse_reasons == 1);
    world_activity_character_leave(&pc); pc.in_room = 200; world_activity_character_enter(&pc);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    assert(world_activity_get_health().corpse_reasons == 1);
    world_activity_object_leave(&container); pc.carrying = nullptr;
    container.loc_p = LOC_WORN; container.loc.wearing = &pc; pc.equipment[0] = &container;
    world_activity_object_enter(&container); assert(world_activity_get_health().corpse_reasons == 1);
    world_activity_object_leave(&container); pc.equipment[0] = nullptr;
    assert(world_activity_get_health().corpse_reasons == 0);
    // Malformed sibling/parent cycles terminate without multiplying a reason.
    container.loc_p = LOC_ROOM; container.loc.room = 0; first.next_content = &first;
    world_activity_object_enter(&container); assert(world_activity_get_health().corpse_reasons == 0);
    first.next_content = nullptr; first.contains = &container;
    world_activity_object_enter(&container); assert(world_activity_get_health().corpse_reasons == 1);
    world_activity_object_leave(&container); first.contains = nullptr;
    // Scheduler cancellation / owner and sequence mismatch never act on a stale cache.
    world_activity_schedule_mundane_after(&far, 240);
    auto old = world_activity_mundane_event(&far); assert(nevent_cancel(old) == nevent_cancel_result::canceled);
    assert(!world_activity_mundane_event(&far).event);
    world_activity_schedule_mundane_after(&far, 240);
    auto current = world_activity_mundane_event(&far);
    world_activity_record_mundane_event(&near, current);
    assert(world_activity_mundane_event(&near).event != current.event);
    world_activity_record_mundane_event(&far, { current.event, current.sequence + 1 });
    assert(world_activity_mundane_event(&far).event == current.event);
    world_activity_record_mundane_event(&far, { nullptr, 0 });
    world_activity_schedule_mundane_after(&far, 100);
    assert(world_activity_mundane_event(&far).sequence == current.sequence);
    assert(world_activity_get_health().repaired_mundane_handles >= 2);
    // Boot/recovery rebuild restores handles and counts once, not twice.
    first.loc_p = LOC_ROOM; first.loc.room = 200; first.next = &ordinary; object_list = &first;
    world_activity_rebuild(); world_activity_rebuild();
    assert(world_activity_get_health().corpse_reasons == 1);
    assert(world_activity_mundane_event(&far).event == current.event);
    assert(world_activity_get_health().indexed_npcs == 2);
    assert(ne_event_time(current.event) == 1); // Restored corpse wakes its room.
    // Reload bounds multiply only after clamping, even INT_MAX / INT_MIN.
    properties["world.activity.distant.seconds"] = INT_MAX;
    properties["world.activity.grace.seconds"] = INT_MIN;
    world_activity_reload(); far.in_room = 259;
    assert(world_activity_mundane_delay(&far, false, false) == 3600 * WAIT_SEC);
    properties["world.activity.enabled"] = 0; world_activity_reload();
    assert(!world_activity_get_health().ready);
    assert(world_activity_mundane_delay(&far, false, false) == 90);
    assert(world_activity_mundane_delay(&far, false, true) == 30);
    assert(world_activity_mundane_delay(&far, true, false) == PULSE_VIOLENCE);
    properties["world.activity.enabled"] = 1; world_activity_reload();
    assert(world_activity_get_health().ready && world_activity_get_health().corpse_reasons == 1);
    far_data.R_num = -1; assert(world_activity_mob_is_timing_sensitive(&far));
    far_data.R_num = 1; assert(world_activity_mob_is_timing_sensitive(&far));
    far_data.R_num = 0; far.specials.act |= ACT_SENTINEL;
    assert(world_activity_mundane_delay(&far, false, false) == 30);
    // Ordinary zone adjacency uses all exits, not only a player's current room.
    pc.in_room = 256; world_activity_player_enter(&pc);
    assert(world_activity_tier_for_room(258) == world_activity_tier::nearby);
    world_activity_player_leave(&pc);
    world_activity_character_leave(&far);
    assert(world_activity_get_health().indexed_npcs == 1);
    cancel_all_events();
    // Execute the production callback's ordinary scheduling tail repeatedly.
    // The dispatched event remains ACTIVE until teardown, so rescheduling it
    // would lose the successor when the scheduler destroys the current event.
    far.specials.act &= ~ACT_SENTINEL; far.in_room = 259;
    properties["world.activity.distant.seconds"] = 60; world_activity_reload();
    ne_event_tick += 14401; pulse = ne_event_tick % PULSES_IN_TICK;
    repeat_mundane = true; mundane_calls = 0;
    world_activity_schedule_mundane_after(&far, 1);
    auto initial = world_activity_mundane_event(&far);
    for (int i = 0; i < 725; ++i) run_one_heartbeat();
    assert(mundane_calls == 4);
    auto successor = world_activity_mundane_event(&far);
    assert(successor.event && successor.sequence != initial.sequence);
    assert(ne_event_counter == 1);
    world_activity_schedule_mundane_after(&far, 100);
    assert(world_activity_mundane_event(&far).sequence == successor.sequence);
    assert(ne_event_counter == 1);
    repeat_mundane = false; cancel_all_events();

    // Exceed the bounded owner-list fallback. Refuse another schedule while
    // an unsearched pending event might exist, then repair after cancellations.
    std::vector<nevent_handle> unrelated;
    for (int i = 0; i < 65; ++i)
        unrelated.push_back(add_event(unrelated_event, 1000, &far, 0, 0, 0, 0, 0).handle);
    auto bounded = add_event(event_mob_mundane, 240, &far, 0, 0, 0, 0, 0).handle;
    world_activity_record_mundane_event(&far, { nullptr, 0 });
    const long before_fallback = ne_event_counter;
    world_activity_schedule_mundane_after(&far, 300);
    assert(ne_event_counter == before_fallback && ne_event_time(bounded.event) == 240);
    for (auto handle : unrelated) nevent_cancel(handle);
    assert(world_activity_mundane_event(&far).event == bounded.event);
    cancel_all_events();

    // Live one-way exits: retain duplicate connections; retarget and remove
    // their independent corpse halo immediately, without a world rebuild.
    object_list = nullptr; character_list = &far; far.in_room = 259;
    properties["world.activity.grace.seconds"] = 0;
    world_activity_reload(); world_activity_rebuild();
    corpse(first, 0); world_activity_object_enter(&first);
    room_direction_data dynamic{}; dynamic.to_room = 259;
    world[0].dir_option[1] = &dynamic; world_activity_room_exits_changed(0);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    world[0].dir_option[2] = &dynamic; world_activity_room_exits_changed(0);
    world[0].dir_option[1] = nullptr; world_activity_room_exits_changed(0);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    dynamic.to_room = 200; world_activity_room_exits_changed(0);
    assert(world_activity_tier_for_room(259) == world_activity_tier::distant);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world[0].dir_option[2] = nullptr; world_activity_room_exits_changed(0);
    assert(world_activity_tier_for_room(200) == world_activity_tier::distant);

    // Portals and transport interiors compose with physical exits. Rebuild
    // reconstructs links from actual published objects; removal/pointer reuse
    // cannot leave a reason at the old destination.
    obj_data portal{}; portal.type = ITEM_TELEPORT; portal.loc_p = LOC_ROOM;
    portal.loc.room = 0; portal.value[0] = 259;
    world_activity_object_enter(&portal); world_activity_object_enter(&portal);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    world_activity_object_leave(&portal); portal.value[0] = 200;
    world_activity_object_enter(&portal);
    assert(world_activity_tier_for_room(259) == world_activity_tier::distant);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world_activity_object_leave(&portal);
    // Runtime portal procs use the same vnum destination, even for another type.
    static index_data object_indexes[1]{}; obj_index = object_indexes;
    object_indexes[0].func.obj = portal_door; portal.type = ITEM_OTHER;
    world_activity_object_enter(&portal);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world_activity_object_leave(&portal); object_indexes[0].func.obj = nullptr;
    portal.type = ITEM_TELEPORT; portal.value[0] = -1; portal.value[3] = 200;
    world_activity_object_enter(&portal);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world_activity_object_leave(&portal);
    obj_data hull{}; hull.type = ITEM_SHIP; hull.loc_p = LOC_ROOM; hull.loc.room = 200;
    ShipData ship{}; ship.shipobj = &hull; ship.room_count = 1;
    ship.room[0].roomnum = 0; test_ship = &ship;
    world_activity_object_enter(&hull);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    world_activity_object_leave(&hull); hull.loc.room = 259;
    world_activity_object_enter(&hull);
    assert(world_activity_tier_for_room(200) == world_activity_tier::distant);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    world_activity_object_leave(&hull); test_ship = nullptr;
    Ferry ferry; ferry.obj = &hull; ferry.obj_num = hull.R_num; ferry.rooms = {0}; test_ferry = &ferry;
    world_activity_object_enter(&hull);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    first.next = &hull; object_list = &first;
    world_activity_rebuild(); world_activity_rebuild();
    assert(world_activity_get_health().corpse_reasons == 1);
    assert(world_activity_tier_for_room(259) == world_activity_tier::active);
    world_activity_object_leave(&hull); test_ferry = nullptr;
    world_activity_object_leave(&first); object_list = nullptr;

    // The actual wandering selection block runs with a deterministic direction.
    // Last player has left; an offline corpse protects a predator at the region
    // edge and its normal opportunities after crossing. A sentinel stays put.
    far.in_room = 63; far_data.last_direction = -1;
    world_activity_rebuild(); corpse(first, 63); world_activity_object_enter(&first);
    movement_enabled = true; moves = 0; mundane_calls = 0;
    world_activity_schedule_mundane_after(&far, 1);
    for (int i = 0; i < 62; ++i) run_one_heartbeat();
    assert(mundane_calls == 3 && moves == 2 && far.in_room == 65);
    assert(world_activity_mundane_delay(&far, false, false) == 30);
    cancel_all_events(); far.specials.act |= ACT_SENTINEL; far.in_room = 63;
    world_activity_schedule_mundane_after(&far, 1);
    for (int i = 0; i < 62; ++i) run_one_heartbeat();
    assert(moves == 2 && far.in_room == 63);
    cancel_all_events(); far.specials.act &= ~ACT_SENTINEL;
    far.in_room = 257; far_data.last_direction = -1; far.specials.act |= ACT_STAY_ZONE;
    world_activity_schedule_mundane_after(&far, 1); run_one_heartbeat(); run_one_heartbeat();
    assert(far.in_room == 257); cancel_all_events(); far.specials.act &= ~ACT_STAY_ZONE;
    far.in_room = 63; far_data.last_direction = -1;
    world[64].room_flags |= ROOM_NO_MOB;
    production_wander(&far); assert(far.in_room == 63); cancel_all_events();
    world[64].room_flags &= ~ROOM_NO_MOB;
    world[63].dir_option[0]->exit_info |= EX_CLOSED;
    production_wander(&far); assert(far.in_room == 63); cancel_all_events();
    world[63].dir_option[0]->exit_info &= ~EX_CLOSED;
    world[64].sector_type = SECT_NO_GROUND;
    production_wander(&far); assert(far.in_room == 63); cancel_all_events();
    world[64].sector_type = SECT_CITY; movement_enabled = false;
    world_activity_object_leave(&first);

    properties["world.activity.wake.enabled"] = 0; world_activity_reload();
    assert(!world_activity_is_enabled());
    assert(world_activity_mundane_delay(&far, false, false) == 90);
    properties["world.activity.wake.enabled"] = 1; world_activity_reload();

    // A player-controlled NPC protects its room and halo independently of the
    // player's body. Actual forward/reverse link getters cover pets and mounts.
    far.in_room = 200; world_activity_rebuild();
    assert(world_activity_get_health().player_reasons == 0);
    char_link_data pet_link{}; pet_link.type = LNK_PET;
    pet_link.linking = &far; pet_link.linked = &pc;
    far.linking = &pet_link; pc.linked = &pet_link;
    world_activity_promote_character(&far);
    world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 1);
    assert(world_activity_tier_for_room(200) == world_activity_tier::active);
    assert(world_activity_tier_for_room(128) == world_activity_tier::nearby);
    world_activity_character_leave(&far); far.in_room = 256;
    world_activity_character_enter(&far);
    assert(world_activity_get_health().player_reasons == 1);
    assert(world_activity_tier_for_room(256) == world_activity_tier::active);
    far.linking = nullptr; pc.linked = nullptr;
    world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 0);
    char_link_data mount_link{}; mount_link.type = LNK_RIDING;
    mount_link.linking = &pc; mount_link.linked = &far;
    pc.linking = &mount_link; far.linked = &mount_link;
    world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 1);
    pc.linking = nullptr; far.linked = nullptr;
    world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 0);
    // A linkdead morph still has its original PC; no descriptor is required.
    far_data.orig_char = &pc;
    world_activity_rebuild(); world_activity_rebuild();
    assert(world_activity_get_health().player_reasons == 1);
    assert(world_activity_mundane_delay(&far, false, false) == 30);
    far_data.orig_char = nullptr; world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 0);
    descriptor_data switched{}; far.desc = &switched;
    world_activity_promote_character(&far);
    assert(world_activity_get_health().player_reasons == 1);
    world_activity_character_leave(&far);
    assert(world_activity_get_health().player_reasons == 0);
    far.desc = nullptr; world_activity_character_enter(&far);

    // Crossing rooms within one region does not advance its entire NPC index
    // on every movement. Encounter-room wakes remain separately next-pulse.
    far.in_room = 200; world[200].people = &far; world_activity_rebuild();
    pc.in_room = 200; world_activity_player_enter(&pc);
    world_activity_schedule_mundane_after(&far, 30);
    const auto promotions = world_activity_get_health().wake_promotions;
    world_activity_player_leave(&pc); pc.in_room = 201; world_activity_player_enter(&pc);
    assert(world_activity_get_health().wake_promotions == promotions);
    assert(ne_event_time(world_activity_mundane_event(&far).event) == 30);
    world_activity_player_leave(&pc); cancel_all_events();

    // Isolate subtree traversal from NPC wake cost and future subtree caching.
    // A wide ordinary container includes one PC corpse.
    static obj_data tree[129]{}; tree[0].loc_p = LOC_ROOM; tree[0].loc.room = 0;
    tree[0].contains = &tree[1];
    for (int i = 1; i < 129; ++i) {
        tree[i].loc_p = LOC_INSIDE; tree[i].loc.inside = &tree[0];
        if (i < 128) tree[i].next_content = &tree[i+1];
    }
    tree[128].type = ITEM_CORPSE; tree[128].value[1] = PC_CORPSE;
    character_list = nullptr; world_activity_rebuild();
    for (int i = 0; i <= top_of_world; ++i) world[i].people = nullptr;
    world_activity_object_enter(&tree[0]);
    assert(world_activity_get_health().corpse_reasons == 1);
    world_activity_object_leave(&tree[0]);
    assert(world_activity_get_health().corpse_reasons == 0);
    const auto started = std::chrono::steady_clock::now();
    for (int i = 0; i < 10000; ++i) {
        world_activity_object_enter(&tree[0]);
        world_activity_object_leave(&tree[0]);
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count();
    assert(world_activity_get_health().corpse_reasons == 0);
    std::printf("CORPSE TRAVERSAL: nodes=129 transfers=20000 ns_per_transfer=%lld sanitizer=%d\n", static_cast<long long>(elapsed / 20000), ACTIVITY_SANITIZED);
    object_list = nullptr;
    require_balanced(250);
    std::puts("world activity: bounded regions/halo, next-pulse wake, corpse lifecycle, malformed graphs, validated handles, rebuild and reload passed");
}
'''

# Compile the maintained callback's complete legal wandering selection and
# scheduling tail. Only profiling scopes and the actual movement collaborator
# are outside this fixture; injected direction choices remove RNG luck.
mundane = (SRC / "mob/mobact.c").read_text()
wander = mundane[mundane.index("/* random wanderings */"):mundane.index("\nbool MobDestroyWall(")]
wander = re.sub(r"PROFILE_(?:START|END)\([^)]*\);", "", wander)
MAIN += "\nstatic void production_wander(P_char ch) { int door = 0;\n" + wander
MAIN += "\n" + extract_function("affects.c", "P_char get_linking_char(")

sanitized = "--benchmark" not in sys.argv[1:]
flags = ["-O1", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if sanitized else ["-O2"]
with tempfile.TemporaryDirectory(prefix="activity-runtime-") as temporary:
    work = Path(temporary)
    cc, binary = work / "harness.cpp", work / "harness"
    cc.write_text(base + MAIN)
    subprocess.run(["g++", "-std=c++20", *flags, f"-DACTIVITY_SANITIZED={int(sanitized)}",
                    "-ffunction-sections", "-fdata-sections", "-pthread",
                    f"-I{SRC}", str(cc), str(SRC / "world/world_activity.c"),
                    str(SRC / "persistence/latency_trace.c"), "-Wl,--gc-sections", "-o", str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", DURIS_NEVENT_MAX_CALLBACKS="0",
               DURIS_NEVENT_BUDGET_USEC="0", DURIS_NEVENT_ANALYTICS="0")
    subprocess.run([str(binary)], env=env, check=True, timeout=30)
