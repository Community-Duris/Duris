#!/usr/bin/env python3
"""Execute actual group mutation bodies with isolated telemetry observations."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    source = (ROOT / 'src/guild/group.c').read_text()
    hook_signature = 'static void telemetry_group_context_changed('
    helper = function(source, hook_signature) if hook_signature in source else ''
    bodies = '\n'.join(function(source, s) for s in (
        'bool group_remove_member(', 'bool group_add_member(', 'void do_disband('))
    prelude = r'''
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <utility>
#include <algorithm>
struct character;
using P_char = character*;
struct descriptor { int connected = 0; };
struct group_list { P_char ch; group_list *next; };
struct character {
    group_list *group = nullptr;
    descriptor *desc = nullptr;
    bool alive = true, npc = false;
    const char *name = "synthetic";
    struct { unsigned act2 = 0; } specials;
    struct { const char *short_descr = "synthetic"; } player;
};
struct mm_ds {};
mm_ds pool;
mm_ds *dead_group_pool = &pool;
constexpr int CON_PLAYING = 0, MAX_STRING_LENGTH = 1024, LOG_DEBUG = 0;
constexpr unsigned PLR2_BACK_RANK = 1;
constexpr int LNK_CONSENT = 0;
#define TRUE true
#define FALSE false
#define IS_ALIVE(c) ((c)->alive)
#define IS_PC(c) (!(c)->npc)
#define IS_NPC(c) ((c)->npc)
#define IS_PC_PET(c) false
#define IS_DISGUISE(c) false
#define IS_RACEWAR_EVIL(c) false
#define IS_RACEWAR_GOOD(c) true
#define GET_NAME(c) ((c)->name)
#define REMOVE_BIT(v,b) ((v) &= ~(b))
enum class telemetry_encounter_outcome { withdrawal = 5 };
void purge_linked_auras(P_char) {}
bool in_command_aura(P_char) { return false; }
void remove_aura_message(P_char, P_char) {}
void add_aura_message(P_char, P_char) {}
void send_to_char(const char *, P_char) {}
void wizlog(int, const char *, const char *) {}
void logit(int, const char *) {}
void fix_group_ranks(P_char) {}
int free_back_slots(P_char) { return 0; }
void update_groupies(P_char) {}
bool racewar(P_char, P_char) { return false; }
bool is_linked_to(P_char, P_char, int) { return true; }
int get_property(const char *, int n) { return n; }
mm_ds *mm_create(const char *, std::size_t, std::size_t, int) { return &pool; }
void *mm_get(mm_ds *) { return new group_list{}; }
void mm_release(mm_ds *, void *p) { delete static_cast<group_list*>(p); }
std::vector<std::pair<P_char, int>> observed;
int telemetry_runtime_game_context(P_char c, descriptor *) {
    int n = c->group ? 0 : 1;
    for (auto *g = c->group; g && n < 300; g = g->next) ++n;
    observed.emplace_back(c, n);
    return 0;
}
int telemetry_runtime_game_encounter_group_sync(P_char) { return 0; }
int telemetry_runtime_game_encounter_leave(P_char, telemetry_encounter_outcome) { return 0; }
'''
    cases = r'''
int last_size(P_char c) {
    for (auto it = observed.rbegin(); it != observed.rend(); ++it)
        if (it->first == c) return it->second;
    return -1;
}
int main() {
    descriptor da, db, dc;
    character a, b, c;
    a.desc = &da; b.desc = &db; c.desc = &dc;
    assert(group_add_member(&a, &b));
    assert(last_size(&a) == 2 && last_size(&b) == 2);
    observed.clear();
    assert(group_add_member(&a, &c));
    assert(observed.size() == 3 && last_size(&a) == 3 && last_size(&b) == 3 && last_size(&c) == 3);
    observed.clear();
    assert(!group_add_member(&a, &b));
    assert(observed.empty());
    assert(group_remove_member(&b));
    assert(last_size(&b) == 1 && last_size(&a) == 2 && last_size(&c) == 2);
    observed.clear();
    assert(group_remove_member(&a));
    assert(last_size(&a) == 1 && last_size(&c) == 1);
    assert(!a.group && !b.group && !c.group);
    observed.clear();
    assert(group_remove_member(&a));
    assert(observed.empty());
    assert(group_add_member(&a, &b));
    assert(group_add_member(&a, &c));
    observed.clear();
    do_disband(&a, nullptr, 0);
    assert(last_size(&a) == 1 && last_size(&b) == 1 && last_size(&c) == 1);
    assert(!a.group && !b.group && !c.group);
    // A leader departure with a retained multi-player group must notify every survivor.
    character d; descriptor dd; d.desc = &dd;
    assert(group_add_member(&a, &b));
    assert(group_add_member(&a, &c));
    assert(group_add_member(&a, &d));
    observed.clear();
    assert(group_remove_member(&a));
    assert(last_size(&a) == 1 && last_size(&b) == 3 && last_size(&c) == 3 && last_size(&d) == 3);
    do_disband(&b, nullptr, 0);
    observed.clear();
#if HAVE_TELEMETRY_GROUP_HOOK
    group_list cycle{&a, nullptr}; cycle.next = &cycle; a.group = &cycle;
    telemetry_group_context_changed(&cycle);
    assert(observed.size() == 256);
    a.group = nullptr;
#endif
    std::puts("telemetry real group mutation journeys passed");
}
'''
    with tempfile.TemporaryDirectory(prefix='telemetry-group-') as temp:
        cpp = Path(temp) / 'group-hooks.cc'
        exe = Path(temp) / 'group-hooks'
        cpp.write_text(prelude + '\n' + helper + '\n' + bodies + '\n' + cases)
        subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                        '-DHAVE_TELEMETRY_GROUP_HOOK=' + str(int(bool(helper))),
                        str(cpp), '-o', str(exe)], check=True, timeout=30)
        subprocess.run([str(exe)], check=True, timeout=10)


if __name__ == '__main__':
    main()
