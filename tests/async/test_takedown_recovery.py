#!/usr/bin/env python3
"""Run takedown failure branches through production CharWait/event_wait.

Combat fixtures and the scheduler transport are stubbed. Branches are extracted
from production without rewriting them; the separate scheduler runtime test
covers the real queue implementation.
"""
from _paths import ROOT, extract_function
from pathlib import Path
import os
import re
import subprocess
import tempfile

source = (ROOT / 'src/cmd/actoff.c').read_text()

def block(text, marker):
    start = text.index('{', text.index(marker))
    depth = 1
    end = start + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

functions = {name: extract_function('actoff.c', signature) for name, signature in {
    'bodyslam': 'void bodyslam(', 'springleap': 'void do_springleap(',
    'trip': 'void do_trip(', 'bash': 'void bash(', 'maul': 'void maul('
}.items()}
branches = {}
for name, text in functions.items():
    branches[name + '_penalty'] = block(text, 'else if (percent_chance == TAKEDOWN_PENALTY)')
    marker = 'if (percent_chance == TAKEDOWN_CANCELLED)'
    after = text[text.index(marker) + len(marker):].lstrip()
    branches[name + '_cancelled'] = block(text, marker) if after.startswith('{') else '{ return; }'
    assert after.startswith('{') or after.startswith('return;')

for name in ('springleap', 'trip'):
    branches[name + '_immovable'] = block(functions[name], '/* you must be size of centaur')
branches['trip_small'] = block(functions['trip'], 'else if (get_takedown_size(vict) <')
branches['trip_large'] = block(functions['trip'], 'else if (get_takedown_size(vict) >')
# Missing-target exits remain free of recovery/resource/position side effects.
for name in ('springleap', 'trip'):
    branches[name + '_missing'] = block(functions[name], 'if (!vict)')
branches['bodyslam_invalid'] = '{' + functions['bodyslam'].split('if (!CanDoFightMove(ch, victim))', 1)[1].split('if (should_not_kill', 1)[0] + '}'
assert 'GET_VITALITY(ch) -= 30;' in functions['bodyslam']
assert functions['bodyslam'].index('GET_VITALITY(ch) -= 30;') < functions['bodyslam'].index('TAKEDOWN_CANCELLED')
# The shared helper owns cancellation-specific waits (sleeping target / freedom /
# evade). Callers must not replace those with the generic failure delay.
helper = extract_function('actoff.c', 'float takedown_check(')
for marker in ('if (GET_STAT(victim) <= STAT_SLEEPING)', 'if (check_freedom_of_movement(', '// Evade -'):
    portion = block(helper, marker)
    assert 'CharWait(ch,' in portion and 'return TAKEDOWN_CANCELLED;' in portion

harness = r'''
#include <cassert>
#include <climits>
#include <cstdio>
#include <algorithm>
struct character {
    struct { unsigned int act2 = 0; unsigned long long wait_until_pulse = 0; } specials;
    bool alive = true; int in_room = 1, pos = 10, stat = 0, vitality = 100;
    bool singing = true;
};
using P_char = character *;
using P_obj = void *;
/* TIMING_CONSTANTS */
constexpr int PLR2_WAIT = 1, LOG_EXIT = 0, LOG_DEBUG = 1, NOWHERE = -1;
constexpr int POS_SITTING = 2, POS_KNEELING = 3, TO_NOTVICT = 0, TO_CHAR = 1, TO_VICT = 2;
constexpr bool FALSE = false;
#define IS_ALIVE(ch) ((ch)->alive)
#define CAN_ACT(ch) (!((ch)->specials.act2 & PLR2_WAIT))
#define IS_TRUSTED(ch) false
#define J_NAME(ch) "fixture"
#define REMOVE_BIT(value, bit) ((value) &= ~(bit))
#define SET_BIT(value, bit) ((value) |= (bit))
#define SET_POS(ch, value) ((ch)->pos = (value))
#define GET_STAT(ch) ((ch)->stat)
#define GET_VITALITY(ch) ((ch)->vitality)
void logit(int, const char *, ...) {}
void debug(const char *, ...) {}
void update_pos(P_char) {}
void act(const char *, bool, P_char, int, P_char, int) {}
void send_to_char(const char *, P_char) {}
void stop_singing(P_char ch) { ch->singing = false; }
int number(int low, int) { return low; }
void event_wait(P_char, P_char, P_obj, void *);
using callback = void (*)(P_char, P_char, P_obj, void *);
struct event { unsigned long long due_tick; callback function; P_char owner; };
using P_nevent = event *;
struct nevent_handle { P_nevent event; unsigned int generation; };
enum class nevent_schedule_status { invalid_replace_target, scheduled };
struct nevent_schedule_result {
    nevent_schedule_status status;
    nevent_handle handle;
    explicit operator bool() const { return status == nevent_schedule_status::scheduled; }
};
unsigned long long tick = 100;
event scheduled_event{};
bool active = false;
P_nevent get_scheduled(P_char ch, callback fn) {
    return active && scheduled_event.owner == ch && scheduled_event.function == fn ? &scheduled_event : nullptr;
}
int ne_event_time(P_nevent e) { return e->due_tick - tick; }
nevent_handle nevent_handle_from_event(P_nevent e) { return {e, 1}; }
nevent_schedule_result add_event(callback fn, int delay, P_char ch, P_char, P_obj, int, void *, int) {
    scheduled_event = {tick + delay, fn, ch}; active = true;
    return {nevent_schedule_status::scheduled, {&scheduled_event, 1}};
}
nevent_schedule_result nevent_replace(nevent_handle, callback fn, int delay, P_char ch,
                                      P_char victim, P_obj obj, int value, void *data, int size) {
    return add_event(fn, delay, ch, victim, obj, value, data, size);
}
''' + extract_function('events.c', 'void event_wait(') + '\n' + extract_function('events.c', 'void CharWait(')
config = (ROOT / 'src/core/config.h').read_text()
timing = [re.search(r'^#define ' + name + r'\s+[^\n]+', config, re.M).group(0)
          for name in ('PULSE_VIOLENCE', 'WAIT_SEC', 'PULSES_IN_TICK')]
harness = harness.replace('/* TIMING_CONSTANTS */', '\n'.join(timing))
for name, branch in branches.items():
    # Both names occur in the original branch text. Unused locals are intentional.
    harness += '\nvoid ' + name + '(P_char ch) { [[maybe_unused]] P_char vict = ch, victim = ch;\n'
    if name == 'bodyslam_penalty':
        harness += 'GET_VITALITY(ch) -= 30;\n'
    harness += branch
    if name == 'bodyslam_penalty':
        harness += block(functions['bodyslam'], '\n\tif (fall)')
    harness += '\n}\n'

harness += r'''
void reset(character &ch) { ch = {}; active = false; tick = 100; }
void check_delay(void (*branch)(P_char), int vitality, bool singing, int posture) {
    character ch; reset(ch); branch(&ch);
    assert(active && scheduled_event.function == event_wait);
    assert(scheduled_event.owner == &ch && scheduled_event.due_tick == tick + 2 * PULSE_VIOLENCE);
    assert(!CAN_ACT(&ch)); // a repeat playing command remains gated
    assert(ch.vitality == vitality && ch.singing == singing);
    assert(ch.pos == posture);
    tick += 2 * PULSE_VIOLENCE - 1;
    assert(!CAN_ACT(&ch) && ne_event_time(&scheduled_event) == 1);
    ++tick;
    scheduled_event.function(&ch, nullptr, nullptr, nullptr); active = false;
    assert(CAN_ACT(&ch));
}
int main() {
    check_delay(bodyslam_penalty, 70, true, POS_KNEELING);
    check_delay(springleap_immovable, 100, true, POS_SITTING);
    check_delay(trip_immovable, 100, true, POS_SITTING);
    check_delay(trip_small, 100, false, POS_SITTING);
    check_delay(trip_large, 100, false, POS_SITTING);
    check_delay(bash_penalty, 100, true, POS_SITTING);
    check_delay(maul_penalty, 100, true, POS_SITTING);
    check_delay(springleap_penalty, 100, true, POS_SITTING);
    check_delay(trip_penalty, 100, false, POS_SITTING);
    for (auto branch : {bodyslam_cancelled, springleap_cancelled, trip_cancelled,
                        bash_cancelled, maul_cancelled, springleap_missing,
                        trip_missing, bodyslam_invalid}) {
        character ch; reset(ch); branch(&ch);
        assert(!active && CAN_ACT(&ch) && ch.pos == 10 && ch.vitality == 100);
        // Preserve a shorter defense-specific wait already supplied by the helper.
        CharWait(&ch, PULSE_VIOLENCE / 2);
        const auto deadline = scheduled_event.due_tick;
        branch(&ch);
        assert(scheduled_event.due_tick == deadline);
    }
    puts("PASS: takedown failures schedule recovery; invalid/cancelled exits preserve wait policy");
}
'''
build_root = ROOT / 'bin/tests'
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='takedown-', dir=build_root) as tmp:
    cpp = Path(tmp) / 'takedown.cpp'; exe = Path(tmp) / 'takedown'
    cpp.write_text(harness)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
