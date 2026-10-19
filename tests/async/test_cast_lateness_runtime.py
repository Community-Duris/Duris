#!/usr/bin/env python3
"""A cast whose segments run late finishes at its cast time plus the lateness of its
last segment, never sooner, and a cast with no lateness takes exactly its cast time (#14).

The production schedule_spellcast(), event_spellcast() and MobCastSpell() are compiled
with real game types. The scheduler is a double: add_event() records each segment's
delay and the driver runs the callback at its due tick plus an injected lateness. No
server or database is started.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/events.h"
#include "magic/spells.h"
#include "combat/damage.h"
#include "combat/grapple.h"
#include "combat/guard.h"
#include "cmd/interp.h"
#include "core/mm.h"
#include "sql/sql.h"
#include "world/specs.prototypes.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

/* The scheduler's tick, which the cast helpers read; the driver moves it. */
unsigned long long ne_event_tick = 1000;

static room_data rooms[1]{};
P_room world = rooms;
static index_data indexes[1]{};
P_index mob_index = indexes, obj_index = indexes;
Skill skills[MAX_AFFECT_TYPES + 1];
spell_target_data common_target_data{};
static int cast_time;                 // what SpellCastTime() answers the mob
static bool pending;                  // a segment is scheduled
static unsigned long long pending_due;
static spellcast_datatype pending_payload;
static P_char pending_caster, pending_victim;
static std::vector<int> delays;       // the delay of each segment the cast submitted
static int commune;                   // the pulses the continuations gave DelayCommune()
static bool completed;

void event_spellcast(P_char, P_char, P_obj, void *);
void telemetry_runtime_game_combat_cast_attempt(P_char, int) {}
void telemetry_runtime_game_combat_cast_complete(P_char) { completed = true; }
void telemetry_runtime_game_combat_cast_abort(P_char) {}
nevent_schedule_result add_event(event_func fn, int delay, P_char ch, P_char victim, P_obj, int,
                                 const void *data, int size)
{
    assert(fn == event_spellcast && size == sizeof pending_payload);
    assert(delay >= 1 && !pending);
    pending_payload = *(const spellcast_datatype *)data;
    pending_due = ne_event_tick + delay;
    // The first segment is due when it is meant to be; a later one keeps the tick it was
    // meant for even when the one-pulse minimum has put it past that.
    assert(delays.empty() ? pending_payload.due_tick == pending_due
                          : pending_payload.due_tick <= pending_due);
    pending_caster = ch; pending_victim = victim; pending = true;
    delays.push_back(delay);
    return {nevent_schedule_status::scheduled, {}};
}
void DelayCommune(P_char, int delay) { commune += delay; }
void event_abort_spell(P_char, P_char, P_obj, void *) {}
void event_wait(P_char, P_char, P_obj, void *) {}
void disarm_char_nevents(P_char, event_func_type) { assert(false && "no scheduling is refused"); }
void StopCasting(P_char) { assert(false && "the cast was stopped"); }
bool is_obj_in_list_vis(P_char, P_obj, P_obj) { return true; }
P_char misfire_check(P_char, P_char target, int) { return target; }
void perform_chaos_check(P_char, P_char, spellcast_datatype *) {}
void say_silent_spell(P_char, int) {}
bool devotion_spell_check(int) { return false; }
bool check_disruptive_blow(P_char) { return false; }
bool divine_blessing_check(P_char, P_char, int) { return false; }
int devotion_skill_check(P_char) { return 0; }
void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}
void __free(void *p, const char *, int) { free(p); }
void *__malloc(size_t n, const char *, const char *, int) { return calloc(1, n); }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
int number(int, int high) { return high; } // MobCastSpell() keeps the whole cast time
float get_property(const char *, double fallback) { return fallback; }
int get_property(const char *, int fallback) { return fallback; }
void send_to_char(const char *, P_char) {}
void send_to_char(const char *, P_char, int) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
bool ac_can_see(P_char, P_char, bool) { return true; }
bool AdjacentInRoom(P_char, P_char) { return true; }
int is_char_in_room(P_char ch, int room)
{
    for (auto *t = world[room].people; t; t = t->next_in_room) if (ch == t) return true;
    return false;
}
int char_in_list(P_char ch) { return ch && is_char_in_room(ch, ch->in_room); }
bool has_innate(P_char, int) { return false; }
affected_type *get_spell_from_char(P_char, int, void *, int) { return nullptr; }
affected_type *affect_to_char(P_char, affected_type *) { return nullptr; }
void affect_from_char(P_char, int) {}
bool affected_by_spell(P_char, int) { return false; }
bool affected_by_spell_flagged(P_char, int, uint) { return false; }
void clear_links(P_char, ush_int) {}
char_link_data *link_char(P_char, P_char, ush_int) { return nullptr; }
P_char get_linked_char(P_char, ush_int) { return nullptr; }
int GET_CHAR_SKILL_P(P_char, int) { return 0; }
int GET_CLASS(P_char ch, uint cls) { return ch->player.m_class & cls; }
int GET_PRIME_CLASS(P_char ch, uint cls) { return GET_CLASS(ch, cls); }
int GET_SECONDARY_CLASS(P_char, uint) { return 0; }
bool notch_skill(P_char, int, float) { return false; }
void CharWait(P_char, int) {}
bool cast_common_generic(P_char, int) { return true; }
const char *elemental_aura_failure_message(P_char) { return nullptr; }
void appear(P_char, bool) {}
int BOUNDED(int low, int val, int high) { return std::clamp(val, low, high); }
void use_spell(P_char, int) {}
void wizlog(int, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
P_char get_random_char_in_room(int, P_char, int) { return nullptr; }
P_char grapple_attack_check(P_char) { return nullptr; }
int grapple_misfire_chance(P_char, P_char, int) { return 0; }
P_char guard_check(P_char, P_char target) { return target; }
bool is_silent(P_char, bool) { return false; }
void say_spell(P_char, int) {}
void MobRetaliateRange(P_char, P_char) {}
bool lightbringer_proc(P_char, P_char, bool) { return false; }
int GetLowestSpellCircle(int) { return 1; }
int SpellCastTime(P_char, int) { return cast_time; }
void SpellCastShow(P_char, int) {}
int STAT_INDEX(int) { return 0; }
void MobStartFight(P_char, P_char) {}
bool hit(P_char, P_char, P_obj, int *) { return false; }
bool should_area_hit(P_char, P_char) { return false; }
bool safe_room_spell_target_allowed(P_char, int, P_char) { return true; }
static void self_spell(int, P_char, char *, int, P_char, P_obj) {}
'''

DRIVER = r'''
// Runs each scheduled callback at its due tick plus the lateness injected for it, and
// answers the tick the cast completed at.
static unsigned long long run_segments(const std::vector<int> &late)
{
    size_t index = 0;
    completed = false;
    while (pending)
    {
        pending = false;
        spellcast_datatype payload = pending_payload;
        ne_event_tick = pending_due + (index < late.size() ? late[index] : 0);
        ++index;
        event_spellcast(pending_caster, pending_victim, nullptr, &payload);
    }
    assert(completed);
    return ne_event_tick;
}

// do_cast's tail: the first segment comes off the cast time and is scheduled.
static unsigned long long player_cast(P_char ch, int pulses, const std::vector<int> &late)
{
    spellcast_datatype payload{};
    payload.timeleft = pulses;
    payload.spell = SPELL_ADRENALINE_CONTROL;
    const int segment = BOUNDED(1, pulses, 4);
    payload.timeleft -= segment;
    delays.clear();
    commune = 0;
    SET_BIT(ch->specials.affected_by2, AFF2_CASTING);
    const unsigned long long start = ne_event_tick;
    assert(schedule_spellcast(ch, ch, segment, &payload));
    const unsigned long long finish = run_segments(late);
    assert(!IS_CASTING(ch));
    return finish - start;
}

static unsigned long long mob_cast(P_char ch, int pulses, const std::vector<int> &late)
{
    cast_time = pulses;
    delays.clear();
    commune = 0;
    const unsigned long long start = ne_event_tick;
    assert(MobCastSpell(ch, ch, nullptr, SPELL_ADRENALINE_CONTROL, 50));
    const unsigned long long finish = run_segments(late);
    assert(!IS_CASTING(ch));
    return finish - start;
}

int main()
{
    skills[SPELL_ADRENALINE_CONTROL].spell_pointer = self_spell;
    skills[SPELL_ADRENALINE_CONTROL].targets = TAR_SELF_ONLY;
    char_data player{}, mob{};
    pc_only_data pc{};
    npc_only_data npc{};
    player.only.pc = &pc;
    mob.only.npc = &npc;
    mob.specials.act = ACT_ISNPC;
    for (P_char ch : {&player, &mob})
    {
        ch->player.level = 50;
        ch->player.m_class = CLASS_MINDFLAYER;
        ch->specials.position = POS_STANDING | STAT_NORMAL;
        ch->in_room = 0;
    }
    player.next_in_room = &mob;
    world[0].people = &player;

    // No lateness: exactly the cast time, in segments of four pulses, each of which
    // extends the memorize event by its own length (DelayCommune is unchanged).
    for (int pulses = 1; pulses <= 20; ++pulses)
    {
        std::vector<int> segments;
        for (int left = pulses; left > 0; left -= 4)
            segments.push_back(std::min(left, 4));
        assert(player_cast(&player, pulses, {}) == (unsigned long long)pulses);
        assert(delays == segments);
        assert(commune == pulses - segments[0]);
    }

    // The work item's example: a 12-pulse cast whose three callbacks each run two
    // pulses late finishes in 14 pulses, not 18.
    assert(player_cast(&player, 12, {2, 2, 2}) == 14);
    assert((delays == std::vector<int>{4, 2, 2}));

    // Only the last callback is late: the cast time plus that lateness.
    assert(player_cast(&player, 12, {0, 0, 3}) == 15);
    assert((delays == std::vector<int>{4, 4, 4}));

    // Lateness before the last segment is made up in full.
    assert(player_cast(&player, 12, {1, 0, 0}) == 12);
    assert((delays == std::vector<int>{4, 3, 4}));

    // The one-pulse minimum: a segment is never shorter than a pulse, so a callback
    // later than the segment that follows it leaves one pulse per remaining segment.
    assert(player_cast(&player, 12, {0, 5, 0}) == 14);
    assert((delays == std::vector<int>{4, 4, 1}));
    assert(player_cast(&player, 12, {10, 0, 0}) == 16);
    assert((delays == std::vector<int>{4, 1, 1}));

    // Five segments, every callback two late: 18 + 2, and one more pulse because the
    // last segment is two pulses and the callback before it was two late. The
    // memorize event still moves by the nominal segments.
    assert(player_cast(&player, 18, {2, 2, 2, 2, 2}) == 21);
    assert((delays == std::vector<int>{4, 2, 2, 2, 1}));
    assert(commune == 14);

    // A mob's first segment is scheduled by MobCastSpell() itself and carries its due
    // tick too, so its continuations make up its lateness as a player's do.
    const unsigned long long mob_on_time = mob_cast(&mob, 18, {});
    assert((delays == std::vector<int>{4, 4, 4, 4, 4, 2}));
    assert(mob_cast(&mob, 18, {2, 2, 2, 2, 2, 2}) == mob_on_time + 3);
    assert((delays == std::vector<int>{4, 2, 2, 2, 2, 1}));

    puts("Late cast segments shorten the next one; on-time casts take their cast time.");
}
'''


def main():
    build = ROOT / 'bin/tests'
    build.mkdir(parents=True, exist_ok=True)
    functions = [
        ('sparser.c', 'static bool schedule_spellcast('),
        ('sparser.c', 'void event_spellcast(P_char ch,'),
        ('mobact.c', 'bool MobCastSpell(P_char ch,'),
    ]
    with tempfile.TemporaryDirectory(prefix='cast-lateness-', dir=build) as directory:
        source, binary = Path(directory) / 'harness.cpp', Path(directory) / 'harness'
        source.write_text('\n'.join([PRELUDE, *[extract_function(*f) for f in functions], DRIVER]))
        subprocess.run(['g++', '-std=c++20', '-g', '-O1', '-fsanitize=address,undefined',
                        '-Isrc', '-D__NO_MYSQL__', '-Isrc/no_mysql', str(source), '-o', str(binary)],
                       cwd=ROOT, check=True, timeout=120)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == '__main__':
    main()
