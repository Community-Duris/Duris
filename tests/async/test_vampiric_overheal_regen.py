#!/usr/bin/env python3
"""Verify that allowed vampiric over-cap HP decays instead of snapping."""

from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function
from _source_contract import function_body


events_source = (SRC / "events.c").read_text(encoding="utf-8", errors="replace")
event_body = function_body(events_source, r"\bvoid\s+event_hit_regen\s*\(")
assert event_body is not None, "event_hit_regen definition is missing"
assert "if (regen_value_int > 0 && GET_HIT(ch) > GET_MAX_HIT(ch))" in event_body
assert "if (GET_HIT(ch) > GET_MAX_HIT(ch))\n\t\t\tGET_HIT(ch) = GET_MAX_HIT(ch);" not in event_body

limits_source = (SRC / "limits.c").read_text(encoding="utf-8", errors="replace")
limits_body = function_body(limits_source, r"\bint\s+hit_regen\s*\(")
assert limits_body is not None, "hit_regen definition is missing"
assert "return MIN(-1, gain);" in limits_body

fight_source = (SRC / "fight.c").read_text(encoding="utf-8", errors="replace")
vamp_body = function_body(fight_source, r"\bint\s+vamp\s*\(")
assert vamp_body is not None, "vamp definition is missing"
assert "hits = MAX(0, MIN(hits, cap - GET_HIT(ch)));" in vamp_body


PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "magic/spells.h"
#include "world/epic_bonus.h"
#include "world/events.h"
#include <cassert>
#include <cmath>
#include <cstdio>

struct regen_event_state {
    float accumulated;
    unsigned long long last_tick;
};

int pulse = 0;
unsigned long long ne_event_tick = 0;
P_nevent current_nevent = nullptr;
static room_data rooms[1]{};
P_room world = rooms;

static regen_event_state next_state{};
static int scheduled = 0;

static int innate_gain = 100;
bool affected_by_spell(P_char ch, int type) {
    for (auto *af = ch->affected; af; af = af->next)
        if (af->type == type) return true;
    return false;
}
void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
float get_property(const char *, double) { return 9.0f; }
bool has_innate(P_char, int) { return false; }
int get_innate_regeneration(P_char) { return innate_gain; }
float get_epic_bonus(P_char, int) { return 0; }
int difficulty_scale_player_regen(P_char, int gain) { return gain; }
room_affect *get_spell_from_room(P_room, int) { return nullptr; }
bool IS_TWILIGHT_ROOM(int) { return false; }
bool IS_OUTDOORS(int) { return false; }
#undef IS_SUNLIT
#define IS_SUNLIT(r) false
int char_in_list(const P_char) { return 1; }
void die(P_char, P_char) { assert(false && "the over-cap test must not enter death"); }
void update_pos(P_char) {}
void gmcp_char_vitals(P_char) {}
void logit(const char *, const char *, ...) {}
void statuslog(int, const char *, ...) {}

nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int,
                                 const void *data, int data_size) {
    assert(data != nullptr);
    assert(data_size == static_cast<int>(sizeof(regen_event_state)));
    next_state = *static_cast<const regen_event_state *>(data);
    ++scheduled;
    return {};
}
'''

HELPERS = "\n".join(
    [
        extract_function("events.c", "static struct regen_event_state regen_state_from_data("),
        extract_function("events.c", "static unsigned long long regen_elapsed_ticks("),
        extract_function("limits.c", "int hit_regen("),
        extract_function("events.c", "void event_hit_regen(P_char ch,"),
    ]
)

DRIVER = r'''
static void run_pulses(P_char ch, int expected_sign, int count, regen_event_state &state) {
    for (int index = 0; index < count; ++index) {
        ++ne_event_tick;
        const int before = GET_HIT(ch);
        const int per_tick = hit_regen(ch, false);
        if (per_tick == 0)
            break;
        assert((expected_sign < 0 && per_tick < 0) ||
               (expected_sign > 0 && per_tick > 0));
        event_hit_regen(ch, nullptr, nullptr, &state);
        assert(scheduled > 0);
        state = next_state;
        assert(GET_HIT(ch) >= -10);
        if (per_tick < 0 && GET_HIT(ch) != before)
            assert(GET_HIT(ch) < before);
    }
}

int main() {
    char_data ch{};
    ch.in_room = 0;
    ch.player.level = 50;
    ch.points.hit = 115;
    ch.points.max_hit = 100;
    ch.specials.position = STAT_NORMAL;
    regen_event_state state{0.0f, 0};

    // A permitted 115/100 result must move downward in small regen steps,
    // rather than being normalized to 100 on the first negative tick.
    assert(hit_regen(&ch, false) < 0);
    run_pulses(&ch, -1, 600, state);
    assert(GET_HIT(&ch) < 115);
    assert(GET_HIT(&ch) > GET_MAX_HIT(&ch));
    const int after_first_decay = GET_HIT(&ch);
    run_pulses(&ch, -1, 600, state);
    assert(GET_HIT(&ch) < after_first_decay);
    assert(GET_HIT(&ch) > GET_MAX_HIT(&ch));

    // Positive regeneration still respects the ordinary max-HP boundary.
    ch.points.hit = 99;
    state = {0.0f, ne_event_tick};
    assert(hit_regen(&ch, false) > 0);
    run_pulses(&ch, 1, 600, state);
    assert(GET_HIT(&ch) == GET_MAX_HIT(&ch));

    std::puts("vampiric over-cap decay and ordinary healing cap regressions passed");
}
'''


with tempfile.TemporaryDirectory(prefix="duris-vampiric-overheal-") as directory:
    directory_path = Path(directory)
    harness = directory_path / "harness.cpp"
    binary = directory_path / "harness"
    harness.write_text(PRELUDE + HELPERS + DRIVER, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-I" + str(ROOT / "src"),
            str(harness),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)
