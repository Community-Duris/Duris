#!/usr/bin/env python3
"""Execute character maintenance with the production timing wheel and light work.

--benchmark optionally compares a captured legacy sweep against the maintained
owner agenda. The legacy source is supplied under bin/, never kept as a second
implementation in the test suite.
"""

import argparse
import ast
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import SRC, extract_function

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--benchmark", action="store_true")
parser.add_argument("--baseline", type=Path)
parser.add_argument("--samples", type=int, default=5)
parser.add_argument("--no-analytics", action="store_true",
                    help="Measure aggregate dispatch overhead without per-callback profiling")
args = parser.parse_args()

# Reuse the existing scheduler fixture without running its test driver.
tree = ast.parse(Path(__file__).with_name("test_nevent_scheduler_runtime.py").read_text())
base = next(ast.literal_eval(node.value) for node in tree.body
            if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and
            target.id == "HARNESS" for target in node.targets))
base = base[:base.index("int main(int argc, char **argv)")]

SUPPORT = r'''
#include "world/character_maintenance.h"
#include "magic/spells.h"
#include <cassert>
#include <ctime>
#include <memory>
#include <string>

time_info_data time_info{};
static long long phase_matches = 0;
static long long list_visits = 0, body_calls = 0, sun_calls = 0, poison_calls = 0;
static long long pleasantry_calls = 0, regen_hit_calls = 0, regen_ward_calls = 0;
static int light_changes = 0, extractions = 0;
static P_char extract_on_pleasantry = nullptr;
static P_char extract_on_poison = nullptr;
static bool record_ticks = true;
static std::map<uint64_t, std::vector<unsigned long long>> body_ticks;
static int script_calls = 0;
int number(int low, int high) { return (low + high) / 2; }
int empty_room_script(P_char, P_char, int command, char *) {
    assert(command == CMD_PERIODIC); ++script_calls; return 1;
}

int GET_CLASS(P_char ch, unsigned int cls) { return ch->player.m_class & cls; }
int IS_MORPH(P_char ch) { return IS_NPC(ch) && ch->only.npc && ch->only.npc->orig_char; }
bool has_innate(P_char ch, int innate) {
    return ch->player.race == RACE_VAMPIRE && innate == INNATE_VULN_SUN;
}
bool affected_by_spell(P_char ch, int spell) {
    for (auto af = ch->affected; af; af = af->next) if (af->type == spell) return true;
    return false;
}
void send_to_char(const char *, P_char ch) { assert(IS_ALIVE(ch)); }
void wizlog(int, const char *, ...) {}
int char_light(P_char) { ++light_changes; return 0; }
int room_light(int, int) { return 0; }
void act(const char *, int, P_char, P_obj, void *, int) {}
void sun_damage_check(P_char) { ++sun_calls; }
int poison_common_remove(P_char ch) {
    ++poison_calls; REMOVE_BIT(ch->specials.affected_by2, AFF2_POISONED);
    if (extract_on_poison == ch) extract_char(ch);
    return 1;
}
void extract_char(P_char ch) {
#ifndef LEGACY_MAINTENANCE
    character_maintenance_leave(ch);
#endif
    ++extractions;
    disarm_char_nevents(ch, nullptr);
    P_char *cursor = &character_list;
    while (*cursor && *cursor != ch) cursor = &(*cursor)->next;
    if (*cursor) *cursor = ch->next;
    SET_POS(ch, STAT_DEAD | POS_PRONE);
}
void pleasantry(P_char ch) {
    ++pleasantry_calls;
    if (extract_on_pleasantry == ch) extract_char(ch);
}
void StartRegen(P_char, regen_resource resource) {
    if (resource == regen_resource::hit) ++regen_hit_calls;
    if (resource == regen_resource::ward) ++regen_ward_calls;
}

struct maintenance_building_fixture { int get_id() { return 1; } };
static maintenance_building_fixture *maintenance_get_building(P_char) {
    static maintenance_building_fixture building; return &building;
}
static void maintenance_set_building_hitpoints(maintenance_building_fixture *) {}
void stop_fighting(P_char ch) { ch->specials.fighting = nullptr; }
void StopAllAttackers(P_char) {}
void clear_all_links(P_char) {}
void event_short_affect(P_char, P_char, P_obj, void *) { assert(false); }
static int reset_commands = 0;
int maintenance_reset_proc(P_char ch, P_char, int command, char *) {
    assert(command == CMD_DEATH && ch->character_maintenance_event);
    ++reset_commands; return 1;
}

static void clear_metrics() {
    phase_matches = 0;
    list_visits = body_calls = sun_calls = poison_calls = pleasantry_calls = 0;
    regen_hit_calls = regen_ward_calls = 0;
    light_changes = extractions = 0;
    body_ticks.clear();
}
static void pc(char_data &ch, pc_only_data &data, uint64_t id) {
    ch.runtime_id = id; ch.only.pc = &data; ch.in_room = 0;
    SET_POS(&ch, STAT_NORMAL | POS_STANDING);
    ch.points.hit = ch.points.max_hit = 100;
    ch.points.ward = ch.points.max_ward = 100;
}
static void npc(char_data &ch, npc_only_data &data, uint64_t id) {
    ch.runtime_id = id; ch.only.npc = &data; ch.in_room = 0;
    SET_POS(&ch, STAT_NORMAL | POS_STANDING); ch.specials.act = ACT_ISNPC;
    ch.points.hit = ch.points.max_hit = 100;
    ch.points.ward = ch.points.max_ward = 100;
}
static void light(char_data &ch, obj_data &obj, int fuel) {
    obj.type = ITEM_LIGHT; obj.value[2] = fuel; ch.equipment[PRIMARY_WEAPON] = &obj;
}
static void advance_to(unsigned long long end) {
    while (ne_event_tick < end) run_one_heartbeat();
}
'''

MAIN = r'''
static void empty_zone_autonomy() {
    reset_scheduler(); clear_metrics(); script_calls = 0;
    char_data mob{}; npc_only_data data{}; npc(mob, data, 80);
    static index_data index{}; index.func.mob = empty_room_script; mob_index = &index;
    character_list = &mob; character_maintenance_init();
    add_event(event_mob_proc, 1, &mob, nullptr, nullptr, 0, nullptr, 0);
    add_record(700, 35, 35); // Independent room/world process on its own clock.
    advance_to(61);
    assert(script_calls == 2 && fired.size() == 1 && body_calls == 0);
    // Updating/removing maintenance membership must not cancel the NPC's
    // script or another world process in a zone containing no players.
    character_maintenance_changed(&mob);
    character_maintenance_leave(&mob);
    assert(get_scheduled(&mob, event_mob_proc));
    advance_to(181); assert(script_calls == 6 && ne_event_counter == 1);
    extract_char(&mob); assert(ne_event_counter == 0);
    mob_index = nullptr; character_list = nullptr; require_balanced(500);
    std::puts("maintenance: production NPC script recurrence and independent world work continue in an empty zone");
}

static void cadence_and_states() {
    reset_scheduler(); clear_metrics();
    world[0].room_flags = ROOM_INDOORS;
    char_data player{}, burning{}, idle{}, druid{}, speaker{}, damaged{}, posture{};
    pc_only_data player_data{};
    npc_only_data burning_data{}, idle_data{}, druid_data{}, speaker_data{}, damaged_data{}, posture_data{};
    pc(player, player_data, 80); player.player.race = RACE_VAMPIRE;
    npc(burning, burning_data, 81); npc(idle, idle_data, 82);
    npc(druid, druid_data, 83); npc(speaker, speaker_data, 84);
    npc(damaged, damaged_data, 85); npc(posture, posture_data, 86);
    player.next = &burning; burning.next = &idle; idle.next = &druid;
    druid.next = &speaker; speaker.next = &damaged; damaged.next = &posture;
    character_list = &player;
    player_data.skills[FIRST_SKILL].taught = 30;
    player_data.skills[FIRST_SKILL].learned = 80;
    obj_data lamp{}; light(burning, lamp, 8);
    druid.player.level = 31; druid.player.m_class = CLASS_DRUID;
    SET_BIT(druid.specials.affected_by2, AFF2_POISONED);
    affected_type pleas{}; pleas.type = SPELL_PLEASANTRY; speaker.affected = &pleas;
    damaged.points.hit = 50; damaged.points.ward = 50;
    posture.specials.z_cord = 3; SET_BIT(posture.specials.affected_by3, AFF3_SWIMMING);
    character_maintenance_init();
    auto original = burning.character_maintenance_event_sequence;
    for (int i = 0; i < 20; ++i) character_maintenance_enter(&burning);
    assert(original == burning.character_maintenance_event_sequence && ne_event_counter == 7);
    advance_to(80);
    assert(body_calls == 0 && lamp.value[2] == 8);
    character_maintenance_changed(&player);
    assert(player.character_maintenance_body_due == 80);
    advance_to(81); character_maintenance_changed(&burning);
    assert(burning.character_maintenance_body_due == 81);
    advance_to(167);
    assert(player_data.skills[FIRST_SKILL].learned == 30);
    assert(lamp.value[2] == 6 && poison_calls == 1 && pleasantry_calls == 2);
    assert(posture.specials.z_cord == 0 && !IS_AFFECTED3(&posture, AFF3_SWIMMING));
    assert(body_ticks[idle.runtime_id].empty());
    assert(body_ticks[druid.runtime_id].size() == 1 && body_ticks[posture.runtime_id].size() == 1);
    assert(regen_hit_calls == 2 && regen_ward_calls == 2 && sun_calls == 2);
    for (auto &entry : body_ticks)
        for (size_t i = 1; i < entry.second.size(); ++i) assert(entry.second[i] - entry.second[i-1] == 80);
    // Direct legacy writes, affect/gear activation, healing, and extinguishing
    // are observed on the original body deadline, without a discovery pass.
    idle.points.hit = 90; idle.points.ward = 80;
    obj_data second_lamp{}; light(idle, second_lamp, 1);
    idle.affected = &pleas;
    druid.specials.affected_by2 |= AFF2_POISONED;
    character_maintenance_changed(&idle);
    auto preserved_due = idle.character_maintenance_body_due;
    advance_to(preserved_due);
    assert(second_lamp.value[2] == 1);
    advance_to(preserved_due + 1);
    assert(second_lamp.value[2] == 0 && body_ticks[idle.runtime_id].size() == 1);
    idle.points.hit = idle.points.max_hit; idle.points.ward = idle.points.max_ward;
    idle.affected = nullptr; character_maintenance_changed(&idle);
    advance_to(preserved_due + 81);
    assert(body_ticks[idle.runtime_id].size() == 1);
    // Bodies continue with no connected players or occupied-zone dependency.
    character_maintenance_leave(&player); player.next = nullptr; character_list = &burning;
    auto before_pleas = pleasantry_calls;
    advance_to(567);
    assert(pleasantry_calls > before_pleas && lamp.value[2] == 1);
    advance_to(647); assert(lamp.value[2] == 0 && light_changes == 2);
    cancel_all_events();
    assert(!burning.character_maintenance_event && !idle.character_maintenance_event);
    character_list = nullptr; require_balanced(510);
    std::puts("maintenance: cadence, skill caps, poison, sunlight, posture, light fuel, regen repair, direct state changes and empty-zone body work passed");
}

static void cancellation_and_identity() {
    reset_scheduler(); clear_metrics();
    char_data speaker{}; npc_only_data data{}; npc(speaker, data, 80);
    affected_type pleas{}; pleas.type = SPELL_PLEASANTRY; speaker.affected = &pleas;
    character_list = &speaker; character_maintenance_init();
    auto first = nevent_handle_from_event(speaker.character_maintenance_event);
    nevent_cancel(first); assert(!speaker.character_maintenance_event);
    character_maintenance_changed(&speaker);
    assert(speaker.character_maintenance_event_sequence != first.sequence);
    extract_on_pleasantry = &speaker; advance_to(81);
    assert(extractions == 1 && ne_event_counter == 0 && !speaker.character_maintenance_in_world);
    extract_on_pleasantry = nullptr;
    // Storage reused for a new runtime identity; no old successor or payload.
    speaker = {}; npc(speaker, data, 160); speaker.affected = &pleas;
    character_list = &speaker; character_maintenance_enter(&speaker);
    advance_to(161); assert(extractions == 1 && pleasantry_calls == 2);
    // Exercise extraction while still pending and direct free's cancellation path.
    extract_char(&speaker); assert(ne_event_counter == 0);
    character_maintenance_changed(&speaker); assert(ne_event_counter == 0);
    char_data revived{}; npc_only_data revived_data{}; npc(revived, revived_data, 200);
    character_list = &revived; character_maintenance_enter(&revived);
    auto due_before_death = revived.character_maintenance_body_due;
    SET_POS(&revived, STAT_DEAD | POS_PRONE); character_maintenance_changed(&revived);
    assert(!revived.character_maintenance_event && revived.character_maintenance_in_world);
    SET_POS(&revived, STAT_NORMAL | POS_STANDING); character_maintenance_changed(&revived);
    assert(revived.character_maintenance_event && revived.character_maintenance_body_due == due_before_death);
    character_maintenance_leave(&revived);
    char_data stale{}; npc_only_data stale_data{}; npc(stale, stale_data, 240);
    character_list = &stale; character_maintenance_enter(&stale);
    stale.runtime_id = 241; advance_to(181);
    assert(!stale.character_maintenance_event && ne_event_counter == 0);
    // Malformed NPC checks use safe logging, retain the five-second obligation.
    char_data malformed{}; malformed.runtime_id = 300; malformed.in_room = 0;
    malformed.specials.act = ACT_ISNPC; SET_POS(&malformed, STAT_NORMAL | POS_STANDING);
    character_list = &malformed; character_maintenance_enter(&malformed);
    advance_to(201); assert(extractions == 3 && ne_event_counter == 0);
    character_list = nullptr; require_balanced(530);
    std::puts("maintenance: pending and in-callback extraction, cancellation, stale identities, storage reuse and malformed NPC checks passed");
}

static void mass_activation(bool budgeted) {
    reset_scheduler(); clear_metrics(); script_calls = 0;
    constexpr int count = 4000;
    auto characters = std::make_unique<char_data[]>(count);
    auto data = std::make_unique<npc_only_data[]>(count);
    auto lights = std::make_unique<obj_data[]>(count);
    character_maintenance_init();
    char_data autonomous{}; npc_only_data autonomous_data{}; npc(autonomous, autonomous_data, 10000);
    static index_data index{}; index.func.mob = empty_room_script; mob_index = &index;
    add_event(event_mob_proc, 1, &autonomous, nullptr, nullptr, 0, nullptr, 0);
    std::map<unsigned long long, int> deadlines, body_deadlines;
    for (int i = 0; i < count; ++i) {
        npc(characters[i], data[i], i+1); light(characters[i], lights[i], 10);
        character_maintenance_enter(&characters[i]);
        ++deadlines[characters[i].character_maintenance_event->due_tick];
        ++body_deadlines[characters[i].character_maintenance_body_due];
    }
    assert(deadlines.size() == 20);
    for (auto &entry : deadlines) assert(entry.second == 200);
    assert(body_deadlines.size() == 80);
    for (auto &entry : body_deadlines) assert(entry.second == 50);
    // Removal of all owner work must release every handle without list lookup.
    for (int i = 0; i < count; i += 7) character_maintenance_leave(&characters[i]);
    long max_executed = 0;
    bool saw_deferral = false;
    while (ne_event_tick < 250) {
        // State notifications while due/late must preserve pending work.
        for (int i = 1; i < 20; ++i) {
            if (!characters[i].character_maintenance_in_world) continue;
            auto due = characters[i].character_maintenance_body_due;
            character_maintenance_changed(&characters[i]);
            assert(characters[i].character_maintenance_body_due == due);
        }
        auto sequence_before = ne_event_sequence;
        ne_events();
        // Every maintenance invocation schedules exactly one successor.
        long executed = static_cast<long>(ne_event_sequence - sequence_before);
        max_executed = std::max(max_executed, executed);
        if (budgeted) assert(executed <= nevent_max_callbacks());
        saw_deferral = saw_deferral || !nevent_deferred_due_counts.empty();
        require_balanced(550); nevent_advance_tick();
    }
    assert(body_calls > 0 && script_calls > 0 && max_executed <= 200);
    if (budgeted) assert(saw_deferral);
    for (int i = 0; i < count; ++i) {
        if (characters[i].character_maintenance_in_world) {
            assert(!body_ticks[characters[i].runtime_id].empty());
            assert(lights[i].value[2] >= 7 && lights[i].value[2] < 10);
        } else assert(lights[i].value[2] == 10);
    }
    for (auto &entry : body_ticks)
        for (size_t i = 1; i < entry.second.size(); ++i) assert(entry.second[i]-entry.second[i-1] >= 80);
    cancel_all_events(); assert(ne_event_counter == 0); mob_index = nullptr; require_balanced(560);
    std::printf("maintenance: mass activation spread across %zu deadlines; peak callbacks=%ld; scheduler budget and cadence passed\n", deadlines.size(), max_executed);
}

static void role_changes() {
    reset_scheduler(); clear_metrics();
    char_data actor{}; pc_only_data player{}; npc_only_data mobile{};
    pc(actor, player, 81); character_list = &actor; character_maintenance_init();
    auto body_due = actor.character_maintenance_body_due;
    assert(actor.character_maintenance_event->due_tick == body_due);
    actor.specials.act |= ACT_ISNPC; actor.only.npc = &mobile;
    character_maintenance_changed(&actor);
    assert(actor.character_maintenance_event->due_tick == 1);
    assert(actor.character_maintenance_body_due == body_due && ne_event_counter == 1);
    advance_to(2);
    actor.specials.act &= ~ACT_ISNPC; actor.only.pc = &player;
    character_maintenance_changed(&actor);
    advance_to(22);
    assert(actor.character_maintenance_event->due_tick == body_due);
    player.skills[FIRST_SKILL].taught = 20; player.skills[FIRST_SKILL].learned = 80;
    advance_to(body_due + 1);
    assert(player.skills[FIRST_SKILL].learned == 20 && body_calls == 1);
    character_maintenance_leave(&actor); character_list = nullptr; require_balanced(570);
    std::puts("maintenance: PC/NPC role changes preserve the body deadline and adjust the safety cadence");
}

static void live_reset() {
    reset_scheduler(); clear_metrics(); reset_commands = 0;
    char_data actor{}, killer{}; npc_only_data data{}; pc_only_data killer_data{};
    npc(actor, data, 80); pc(killer, killer_data, 81);
    actor.specials.act |= ACT_SPEC | ACT_SPEC_DIE; actor.points.hit = 90;
    affected_type building{}; building.type = TAG_BUILDING; actor.affected = &building;
    static index_data index{}; index.func.mob = maintenance_reset_proc; mob_index = &index;
    character_list = &actor; character_maintenance_init();
    auto old_sequence = actor.character_maintenance_event_sequence;
    auto due = actor.character_maintenance_body_due;
    add_event(event_mob_proc, 3, &actor, nullptr, nullptr, 0, nullptr, 0);
    SET_POS(&actor, STAT_DEAD | POS_PRONE);
    assert(check_outpost_death(&actor, &killer));
    assert(reset_commands == 1 && IS_ALIVE(&actor) && ne_event_counter == 1);
    assert(actor.character_maintenance_event && actor.character_maintenance_event_sequence != old_sequence);
    assert(actor.character_maintenance_body_due == due);
    advance_to(due + 1); assert(body_calls == 1 && regen_hit_calls == 1);
    SET_POS(&actor, STAT_DEAD | POS_PRONE); character_maintenance_changed(&actor);
    actor.only.npc->training_dummy = true; actor.points.hit = -10;
    maintenance_dummy_death(&actor);
    assert(actor.points.hit == actor.points.max_hit && IS_ALIVE(&actor));
    assert(actor.character_maintenance_event && actor.character_maintenance_body_due == due + 80);
    character_maintenance_leave(&actor); character_list = nullptr; mob_index = nullptr;
    require_balanced(575);
    std::puts("maintenance: production outpost reset and dummy death recovery rearm after cancellation; synchronous death processing retained");
}

static void poison_extraction() {
    reset_scheduler(); clear_metrics();
    char_data actor{}; npc_only_data data{}; npc(actor, data, 80);
    actor.player.level = 31; actor.player.m_class = CLASS_DRUID;
    actor.specials.affected_by2 = AFF2_POISONED; extract_on_poison = &actor;
    character_list = &actor; character_maintenance_init(); advance_to(81);
    assert(poison_calls == 1 && extractions == 1 && body_calls == 0 && ne_event_counter == 0);
    assert(!actor.character_maintenance_event && !actor.character_maintenance_in_world);
    extract_on_poison = nullptr; character_list = nullptr; require_balanced(576);
    std::puts("maintenance: poison-removal extraction cannot send to or rearm the retired owner");
}

static void benchmark() {
    reset_scheduler(); clear_metrics(); record_ticks = false;
    constexpr int npc_count = 30000, pc_count = 120, rounds = 10;
    constexpr int count = npc_count + pc_count;
    auto characters = std::make_unique<char_data[]>(count);
    auto npc_data = std::make_unique<npc_only_data[]>(npc_count);
    auto pc_data = std::make_unique<pc_only_data[]>(pc_count);
    auto lamps = std::make_unique<obj_data[]>(120);
    for (int i = 0; i < count; ++i) {
        if (i < npc_count) npc(characters[i], npc_data[i], i+1);
        else pc(characters[i], pc_data[i-npc_count], i+1);
        characters[i].next = i+1 < count ? &characters[i+1] : nullptr;
        if (i < 120) light(characters[i], lamps[i], 1000);
        if (i >= npc_count) {
            pc_data[i-npc_count].skills[FIRST_SKILL].learned = 80;
            pc_data[i-npc_count].skills[FIRST_SKILL].taught = 20;
        }
    }
    character_list = &characters[0];
    long long callback_us = 0, peak_callback_us = 0, peak_pulse_us = 0;
    long long callbacks = 0;
#ifdef LEGACY_MAINTENANCE
    nevent_periodic_reset();
    assert(nevent_periodic_register("generic-character-sweep", generic_char_event,
        20 * WAIT_SEC, 5 * WAIT_SEC, nevent_periodic_policy::fixed_delay, true) ==
        nevent_periodic_result::registered);
#else
    character_maintenance_init();
#endif
    // Both implementations warm through one complete body period, then start
    // the same 200-second window with identical fuel and pending skill repairs.
    while (ne_event_tick < 160) { ne_events(); nevent_advance_tick(); }
    clear_metrics(); nevent_analytics_reset(ne_event_tick);
    for (int i = 0; i < 120; ++i) lamps[i].value[2] = 1000;
    for (int i = 0; i < pc_count; ++i) pc_data[i].skills[FIRST_SKILL].learned = 80;
    auto cpu_started = std::clock();
    for (int tick = 0; tick < 80*rounds; ++tick) {
        auto before = ne_event_sequence;
        auto begin = std::chrono::steady_clock::now();
        ne_events();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-begin).count();
        callback_us += us; peak_pulse_us = std::max(peak_pulse_us, static_cast<long long>(us));
        callbacks += ne_event_sequence-before;
        nevent_analytics.pulses = 0; // Keep the measured window open across all rounds.
        nevent_advance_tick();
    }
    auto process_cpu_us = static_cast<long long>((std::clock()-cpu_started) * (1000000.0 / CLOCKS_PER_SEC));
    if (!nevent_analytics.callbacks.empty())
        peak_callback_us = nevent_analytics.callbacks.begin()->second.max_us;
    if (!nevent_analytics_enabled()) peak_callback_us = -1;
#ifndef DISCOVERY_ONLY
    for (int i = 0; i < 120; ++i) assert(lamps[i].value[2] == 1000-rounds);
    for (int i = 0; i < pc_count; ++i) assert(pc_data[i].skills[FIRST_SKILL].learned == 20);
#endif
    std::printf("BENCHMARK mode=%s analytics=%s population=%d simulated_seconds=%d list_visits=%lld bodies=%lld slice_matches=%lld callbacks=%lld total_dispatch_us=%lld process_cpu_us=%lld peak_callback_us=%lld peak_pulse_us=%lld\n",
#ifdef LEGACY_MAINTENANCE
#ifdef DISCOVERY_ONLY
        "discovery-only",
#else
        "before",
#endif
#else
        "after",
#endif
        nevent_analytics_enabled() ? "on" : "off", count, 20*rounds, list_visits, body_calls,
        phase_matches, callbacks, callback_us, process_cpu_us, peak_callback_us, peak_pulse_us);
#ifdef LEGACY_MAINTENANCE
    assert(callbacks == 4*rounds && list_visits == 4LL*rounds*count);
    nevent_periodic_reset();
#else
    assert(callbacks == 4LL*rounds*npc_count + rounds*pc_count && list_visits == 0);
#endif
    cancel_all_events(); character_list = nullptr; require_balanced(580);
}
int main(int argc, char **argv) {
    if (argc > 1 && std::string(argv[1]) == "benchmark") { benchmark(); return 0; }
#ifndef LEGACY_MAINTENANCE
    if (argc > 1 && (std::string(argv[1]) == "budget" || std::string(argv[1]) == "callback-budget")) { mass_activation(true); return 0; }
    empty_zone_autonomy(); cadence_and_states(); cancellation_and_identity(); mass_activation(false); role_changes(); live_reset(); poison_extraction();
#endif
    return 0;
}
'''

light_body = extract_function("handler.c", "void update_char_objects(")
light_body = light_body.replace("int i, change;", "int i, change;\n\t++body_calls;\n"
                               "\tif (record_ticks) body_ticks[ch->runtime_id].push_back(ne_event_tick);")
maintenance = (SRC / "character_maintenance.c").read_text()
# Benchmarks use the production monotonic clock and existing callback analytics.
# Budget tests retain the scheduler fixture's deterministic clock.
if args.benchmark:
    base = base.replace("#define clock_gettime nevent_test_clock_gettime", "")
    base = base.replace("#undef clock_gettime", "")
    # Include actual periodic admission, completion and rearming for the old
    # sweep, rather than driving it manually on a different clock/dispatch path.
    periodic_start = base.index("bool nevent_periodic_begin(")
    periodic_end = base.index("struct panic_signal", periodic_start)
    base = (base[:periodic_start] + '#include "world/nevent_periodic.c"\n\n' +
            base[periodic_end:])

output_root = ROOT / "bin/tests"
output_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="character-maintenance-", dir=output_root) as directory:
    temp = Path(directory)

    def build_and_run(name: str, code: str, sanitized: bool, modes: tuple[str, ...]) -> None:
        harness, binary = temp / f"{name}.cpp", temp / name
        harness.write_text(code)
        command = ["g++", "-std=c++20", "-O2", "-ffunction-sections", "-fdata-sections",
                   "-pthread", f"-I{SRC}", str(harness),
                   str(SRC / "persistence/latency_trace.c"), "-Wl,--gc-sections", "-o", str(binary)]
        if sanitized:
            command[1:1] = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, check=True)
        environment = os.environ.copy()
        environment.update({"ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1", "UBSAN_OPTIONS": "halt_on_error=1",
                            "DURIS_NEVENT_BUDGET_USEC": "0", "DURIS_NEVENT_MAX_CALLBACKS": "0",
                            "DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC": "0",
                            "DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS": "0", "DURIS_NEVENT_ANALYTICS": "1" if args.benchmark and not args.no_analytics else "0",
                            "DURIS_NEVENT_PLAYER_PRIORITY": "0"})
        for mode in modes:
            env = environment.copy()
            if mode == "budget":
                env.update(DURIS_NEVENT_MAX_CALLBACKS="200", DURIS_NEVENT_BUDGET_USEC="450")
            if mode == "callback-budget":
                env.update(DURIS_NEVENT_MAX_CALLBACKS="40", DURIS_NEVENT_BUDGET_USEC="0")
            for _ in range(args.samples if mode == "benchmark" else 1):
                subprocess.run([str(binary), mode], check=True, env=env)

    script = extract_function("mobact.c", "void event_mob_proc(")
    reset_code = extract_function("buildings.c", "int check_outpost_death(")
    reset_code = reset_code.replace("Building", "maintenance_building_fixture")
    reset_code = reset_code.replace("get_building_from_char", "maintenance_get_building")
    reset_code = reset_code.replace("set_current_outpost_hitpoints", "maintenance_set_building_hitpoints")
    death = extract_function("fight.c", "void die(")
    dummy_death = death[death.index("if (training_dummy_is(ch))"):death.index("if (!killer)")]
    dummy_death = dummy_death.replace("training_dummy_is(ch)", "ch->only.npc->training_dummy")
    dummy_death = "static void maintenance_dummy_death(P_char ch) {\n" + dummy_death + "}\n"
    after = base + SUPPORT + light_body + "\n" + maintenance + script + reset_code + dummy_death + "\n" + MAIN
    build_and_run("after", after, not args.benchmark, ("benchmark",) if args.benchmark else ("common", "budget", "callback-budget"))
    if args.baseline:
        legacy = args.baseline.read_text()
        legacy = legacy[legacy.index("#define GENERIC_CHAR_EVENT_SLICES"):]
        if "void event_sundamage" in legacy:
            legacy = legacy[:legacy.index("void event_sundamage")]
        legacy = legacy.replace("i_next = i->next;", "i_next = i->next; ++list_visits;")
        legacy_main = MAIN[MAIN.index("static void benchmark()") :]
        build_and_run("before", "#define LEGACY_MAINTENANCE\n" + base + SUPPORT + light_body + "\n" + legacy + legacy_main,
                      False, ("benchmark",))
        discovery = legacy.replace("\t\tif (!IS_BLOODLUST", "\t\t++phase_matches; continue; // Instrument only legacy discovery.\n\t\tif (!IS_BLOODLUST", 1)
        build_and_run("discovery", "#define LEGACY_MAINTENANCE\n#define DISCOVERY_ONLY\n" +
                      base + SUPPORT + light_body + "\n" + discovery + legacy_main, False, ("benchmark",))

extract = extract_function("handler.c", "void extract_char(")
assert extract.index("if (IS_MORPH(ch))") < extract.index("character_maintenance_leave(ch)")
assert extract.index("character_maintenance_leave(ch)") < extract.index("disarm_char_nevents(ch, NULL)")
free = extract_function("db.c", "void free_char(")
assert free.index("character_maintenance_leave(ch)") < free.index("affect_remove(ch, af)")
movement = extract_function("handler.c", "bool char_to_room(")
assert movement.index("ch->in_room = room") < movement.index("character_maintenance_enter(ch)")
assert "character_maintenance_changed(ch)" in extract_function("fight_state.c", "void update_pos(")
create = extract_function("db.c", "P_char read_mobile(int nr, int type, bool apply_mob_gold)")
assert create.index("convertMob(mob, apply_mob_gold)") < create.index("character_maintenance_enter(mob)")
restore = extract_function("staff_character_recovery.c", "void do_restore(")
assert restore.count("character_maintenance_changed(victim)") == 2
reset_outpost = extract_function("outposts.c", "bool reset_one_outpost(")
assert reset_outpost.index("SET_POS(building->get_mob()") < reset_outpost.index("character_maintenance_changed(building->get_mob())")

print("character maintenance executable checks passed")
