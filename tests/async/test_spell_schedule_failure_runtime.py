#!/usr/bin/env python3
"""Inject scheduler rejection into the production spell/recovery submission helpers.

Compile extracted production bodies with real game types; the scheduler and UI
are controlled doubles. No server or database is started.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CAST = (ROOT / 'src/net/sparser.c').read_text()
MEM = (ROOT / 'src/classes/memorize.c').read_text()


def extract(text, signature):
    start = text.index(signature)
    while ';' in text[start:text.index('{', start)]:
        start = text.index(signature, start + len(signature))
    opening = text.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


# Guard every caller, including both do_memorize initiation branches. A future
# direct submission would bypass the runtime-tested rollback boundary.
assert len(re.findall(r'add_event\(event_spellcast,', CAST)) == 1
assert len(re.findall(r'add_event\(event_memorize,', MEM)) == 1
for signature in ('void do_will(', 'void do_cast('):
    body = extract(CAST, signature)
    assert re.search(r'if \(!schedule_spellcast\([^;]+\)\)\s*return;', body)
assert 'schedule_spellcast(' in extract(CAST, 'void event_spellcast(')
assert len(re.findall(r'if \(!schedule_memorize\([^;]+\)\)\s*return;', MEM)) == 3
for signature in ('void handle_undead_mem(', 'void handle_memorize(',
                  'void event_memorize(', 'void do_assimilate(',
                  'void do_npc_commune(', 'void do_memorize(', 'void use_spell('):
    assert 'schedule_memorize(' in extract(MEM, signature)

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "world/events.h"
#include "magic/spells.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
static nevent_schedule_status injected;
static int message_count, frees, meditation_stops;
static std::vector<event_func> disarmed;
static spellcast_datatype copied_cast;
static int copied_time;
static event_func submitted;
static int submitted_delay;
static bool room_link, world_link;
void event_spellcast(P_char, P_char, P_obj, void *) {}
void event_abort_spell(P_char, P_char, P_obj, void *) {}
void event_wait(P_char, P_char, P_obj, void *) {}
void event_memorize(P_char, P_char, P_obj, void *) {}
void __free(void *p, const char *, int) { ++frees; std::free(p); }
bool meming_class(P_char) { return true; }
void send_to_char(const char *, P_char) { ++message_count; }
void act(const char *, int, P_char, P_obj, void *, int) {}
void disarm_char_nevents(P_char, event_func_type f) { disarmed.push_back(f); }
void clear_links(P_char, ush_int type) {
    if (type == LNK_CAST_ROOM) room_link = false;
    if (type == LNK_CAST_WORLD) world_link = false;
}
void stop_meditation(P_char ch) {
    ++meditation_stops;
    REMOVE_BIT(ch->specials.affected_by, AFF_MEDITATE);
}
nevent_schedule_result add_event(event_func f, int delay, P_char, P_char, P_obj,
                                  int, const void *data, int size) {
    submitted = f;
    submitted_delay = delay;
    if (injected == nevent_schedule_status::scheduled) {
        if (f == event_spellcast) {
            assert(size == sizeof(copied_cast));
            copied_cast = *static_cast<const spellcast_datatype *>(data);
        } else if (data) {
            assert(size == sizeof(int));
            copied_time = *static_cast<const int *>(data);
        } else assert(size == 0);
    }
    return {injected, {}};
}
'''
DRIVER = r'''
int main() {
    // Exercise every typed rejection, even those ordinary game-thread callers
    // cannot supply, so newly broadened rejection handling cannot strand state.
    for (int status = int(nevent_schedule_status::null_callback);
         status <= int(nevent_schedule_status::wrong_thread); ++status) {
        for (bool continuation : {false, true}) {
            char_data ch{};
            descriptor_data descriptor{};
            char command[] = "look";
            txt_block queued{command, nullptr};
            descriptor.input = {&queued, &queued};
            ch.desc = &descriptor;
            spellcast_datatype payload{};
            payload.arg = strdup("target argument");
            payload.timeleft = 8;
            nevent_data current{};
            current.func = event_spellcast;
            current.data = &payload;
            // The executing event can still be visible in the owner's list.
            if (continuation) ch.nevents = &current;
            ch.specials.affected_by2 = AFF2_CASTING;
            ch.specials.act2 = PLR2_WAIT;
            room_link = world_link = true;
            message_count = frees = 0;
            disarmed.clear();
            injected = static_cast<nevent_schedule_status>(status);
            assert(!schedule_spellcast(&ch, nullptr, 4, &payload));
            assert(!IS_AFFECTED2(&ch, AFF2_CASTING));
            assert(CAN_ACT((&ch)));
            assert(descriptor.input.head == &queued && descriptor.input.tail == &queued);
            assert(std::strcmp(queued.text, "look") == 0);
            assert(!room_link && !world_link);
            assert(!payload.arg && frees == 1 && message_count == 1);
            assert(disarmed.size() == 3);
            assert(disarmed[0] == event_spellcast);
            assert(disarmed[1] == event_abort_spell);
            assert(disarmed[2] == event_wait);
            // A fresh cast can be scheduled after rejection.
            ch.nevents = nullptr;
            injected = nevent_schedule_status::scheduled;
            payload.arg = strdup("retry");
            ch.specials.affected_by2 = AFF2_CASTING;
            assert(schedule_spellcast(&ch, nullptr, 4, &payload));
            assert(copied_cast.arg == payload.arg && copied_cast.timeleft == 8);
            assert(submitted == event_spellcast && submitted_delay == 4);
            assert(IS_AFFECTED2(&ch, AFF2_CASTING) && frees == 1);
            std::free(copied_cast.arg);
        }
        for (bool continuation : {false, true}) {
            for (bool halftime : {false, true}) {
                char_data ch{};
                affected_type pending{};
                pending.type = TAG_MEMORIZE;
                pending.modifier = 42;
                ch.affected = &pending;
                ch.specials.affected_by2 = continuation ? AFF2_MEMORIZING : 0;
                ch.specials.affected_by = AFF_MEDITATE;
                // Memorization must preserve a separate, bounded wait gate.
                ch.specials.act2 = PLR2_WAIT;
                int time = 20;
                message_count = meditation_stops = 0;
                disarmed.clear();
                injected = static_cast<nevent_schedule_status>(status);
                assert(!schedule_memorize(&ch, 10, halftime ? &time : nullptr));
                assert(!IS_AFFECTED2(&ch, AFF2_MEMORIZING));
                assert(ch.affected == &pending && pending.type == TAG_MEMORIZE);
                assert(pending.modifier == 42 && pending.flags == 0);
                assert(!IS_AFFECTED(&ch, AFF_MEDITATE));
                assert(message_count == 1 && meditation_stops == 1 && time == 20);
                assert(disarmed.size() == 1 && disarmed[0] == event_memorize);
                assert(IS_SET(ch.specials.act2, PLR2_WAIT));
                injected = nevent_schedule_status::scheduled;
                assert(schedule_memorize(&ch, 10, &time));
                assert(copied_time == time && submitted_delay == 10);
                assert(message_count == 1 && meditation_stops == 1);
            }
        }
    }
}
'''
source = PRELUDE + '\n'.join([
    extract(CAST, 'void show_abort_casting('),
    extract(CAST, 'void StopCasting('),
    extract(CAST, 'static bool schedule_spellcast('),
    extract(MEM, 'static bool schedule_memorize('),
]) + DRIVER
with tempfile.TemporaryDirectory(prefix='spell-schedule-') as directory:
    cpp = Path(directory) / 'harness.cpp'
    binary = Path(directory) / 'harness'
    cpp.write_text(source)
    subprocess.run(['g++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I' + str(ROOT / 'src'), str(cpp), '-o', str(binary)], check=True)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1')
    subprocess.run([str(binary)], env=env, check=True)
print('Spell scheduling rejection and retry tests passed under ASan/UBSan')
