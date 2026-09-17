#!/usr/bin/env python3
"""Execute combat observation guard, accepted-start block, and stop_fighting.

The full combat engine is not stubbed into a fake battle. Source checks locate
entry after its existing gates; the actual mutation block and complete stop
function run with isolated observation/service seams.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    match = re.search(r'\s+'.join(re.escape(w) for w in signature.split()), source)
    assert match, signature
    opening = source.index('{', match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def main():
    source = (ROOT / 'src/combat/fight.c').read_text()
    helper = function(source, 'static void telemetry_combat_context_changed(')
    start = function(source, 'void set_fighting(P_char ch, P_char vict)')
    stop = function(source, 'void stop_fighting(P_char ch)')
    assert start.count('telemetry_combat_context_changed(ch)') == 1
    assert start.index('can_hit_target(ch, victim)') < start.index('GET_OPPONENT(ch) = victim;')
    assert start.index('IS_IMMOBILE(ch)') < start.index('GET_OPPONENT(ch) = victim;')
    assert start.index('GET_OPPONENT(ch) = victim;') < start.index('telemetry_combat_context_changed(ch)')
    assert stop.count('telemetry_combat_context_changed(ch)') == 1
    assert stop.index('GET_OPPONENT(ch) = NULL;') < stop.index('telemetry_combat_context_changed(ch)')
    assert 'game_evidence' not in helper  # automatic combat is not human activity
    block = start[start.index('GET_OPPONENT(ch) = victim;'):start.index('if (ch->in_room >= 0)')]
    harness = r'''
#include "telemetry/telemetry_runtime.h"
#include <cassert>
#include <cstdio>
#include <vector>
struct char_data;
struct descriptor_data { int connected = 0; };
struct char_data {
    struct { unsigned affected_by3 = 0; char_data *was_fighting = nullptr;
             char_data *next_fighting = nullptr; char_data *fighting = nullptr; } specials;
    descriptor_data *desc = nullptr;
    bool npc = false;
    int in_room = -1;
};
using P_char = char_data *;
P_char combat_list = nullptr, combat_next_ch = nullptr;
constexpr int CON_PLAYING = 0, LOG_EXIT = 1, AFF3_TRACKING = 1;
constexpr int SPELL_CEGILUNE_BLADE = 1, SKILL_LANCE_CHARGE = 2, PULSE_VIOLENCE = 10;
#define IS_PC(ch) (!(ch)->npc)
#define IS_FIGHTING(ch) ((ch)->specials.fighting != nullptr)
#define GET_OPPONENT(ch) ((ch)->specials.fighting)
#define GET_NAME(ch) "synthetic"
#define GET_CHAR_SKILL(ch, skill) 0
#define IS_SET(v, bit) ((v) & (bit))
#define REMOVE_BIT(v, bit) ((v) &= ~(bit))
struct affected_type { int modifier = 0; };
bool SanityCheck(P_char ch, const char *) { return ch != nullptr; }
bool affected_by_spell(P_char, int) { return false; }
affected_type *get_spell_from_char(P_char, int) { return nullptr; }
void logit(int, const char *, ...) {}
void set_short_affected_by(P_char, int, int) {}
void gmcp_mark_room_dirty(int) {}
void gmcp_combat_end(P_char) {}
void update_pos(P_char) {}
std::vector<bool> observed;
telemetry_capture_result telemetry_runtime_game_context(char_data *ch, descriptor_data *) {
    observed.push_back(ch->specials.fighting != nullptr);
    return {};
}
telemetry_capture_result telemetry_runtime_game_encounter_begin(char_data *, telemetry_encounter_mode) {
    return {};
}
''' + helper + '\nvoid accepted_start(P_char ch, P_char victim) {\n' + block + '\n}\n' + stop + r'''
int main() {
    char_data player{}, target{};
    descriptor_data descriptor{};
    player.desc = &descriptor;
    accepted_start(&player, &target);
    assert(combat_list == &player && IS_FIGHTING(&player));
    assert(observed.size() == 1 && observed.back());
    stop_fighting(&player);
    assert(!combat_list && !IS_FIGHTING(&player));
    assert(observed.size() == 2 && !observed.back());
    stop_fighting(&player); // no-op doesn't emit another boundary
    assert(observed.size() == 2);
    telemetry_combat_context_changed(nullptr);
    player.npc = true;
    telemetry_combat_context_changed(&player);
    player.npc = false; player.desc = nullptr;
    telemetry_combat_context_changed(&player);
    player.desc = &descriptor; descriptor.connected = 1;
    telemetry_combat_context_changed(&player);
    assert(observed.size() == 2);
    std::puts("combat start/stop context boundaries and observer guards passed");
}
'''
    with tempfile.TemporaryDirectory(prefix='telemetry-combat-hooks-') as directory:
        cpp, binary = Path(directory) / 'combat.cc', Path(directory) / 'combat'
        cpp.write_text(harness)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-I', str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=10)


if __name__ == '__main__':
    main()
