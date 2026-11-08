#!/usr/bin/env python3
"""Production policy and scheduler lifecycle checks under ASan/UBSan.

Reuse the scheduler regression's collaborators without executing its test
runner. World activity is compiled as its own production translation unit.
"""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
tree = ast.parse((Path(__file__).with_name("test_nevent_scheduler_runtime.py")).read_text())
base = next(ast.literal_eval(node.value) for node in tree.body
            if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and target.id == "HARNESS" for target in node.targets))
base = base[:base.index("int main(int argc, char **argv)")]
base = base.replace("test_world[1]", "test_world[260]").replace("top_of_world = 0", "top_of_world = 259")
base = base.replace("DEFINE_LABEL_CALLBACK(event_mob_mundane)",
                    "void event_mob_mundane(P_char, P_char, P_obj, void *);")

MAIN = r'''
#include "world/world_activity.h"
#include <cassert>
#include <climits>
int top_of_mobt = 0;
static std::map<std::string, int> properties;
int get_property(const char *key, int fallback) {
    auto it = properties.find(key); return it == properties.end() ? fallback : it->second;
}
int number(int low, int high) { return low < 0 ? 0 : (low + high) / 2; }
P_char get_linking_char(P_char, ush_int) { return nullptr; }
static bool repeat_mundane = false;
static int mundane_calls = 0;
void event_mob_mundane(P_char ch, P_char, P_obj, void *) {
    ++mundane_calls;
    if (repeat_mundane) world_activity_schedule_mundane(ch, false, false);
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
    world_activity_record_mundane_event(&near, current); assert(!world_activity_mundane_event(&near).event);
    world_activity_record_mundane_event(&far, { current.event, current.sequence + 1 });
    assert(!world_activity_mundane_event(&far).event);
    // Boot/recovery rebuild restores handles and counts once, not twice.
    first.loc_p = LOC_ROOM; first.loc.room = 200; first.next = &ordinary; object_list = &first;
    world_activity_rebuild(); world_activity_rebuild();
    assert(world_activity_get_health().corpse_reasons == 1);
    assert(world_activity_mundane_event(&far).event == current.event);
    assert(world_activity_get_health().indexed_npcs == 2);
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
    repeat_mundane = false; cancel_all_events(); character_list = nullptr; object_list = nullptr;
    require_balanced(250);
    std::puts("world activity: bounded regions/halo, next-pulse wake, corpse lifecycle, malformed graphs, validated handles, rebuild and reload passed");
}
'''

with tempfile.TemporaryDirectory(prefix="activity-runtime-") as temporary:
    work = Path(temporary)
    cc, binary = work / "harness.cpp", work / "harness"
    cc.write_text(base + MAIN)
    subprocess.run(["g++", "-std=c++20", "-O1", "-ffunction-sections", "-fdata-sections",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-pthread",
                    f"-I{SRC}", str(cc), str(SRC / "world/world_activity.c"),
                    str(SRC / "persistence/latency_trace.c"), "-Wl,--gc-sections", "-o", str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", DURIS_NEVENT_MAX_CALLBACKS="0",
               DURIS_NEVENT_BUDGET_USEC="0", DURIS_NEVENT_ANALYTICS="0")
    subprocess.run([str(binary)], env=env, check=True, timeout=30)
