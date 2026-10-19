#!/usr/bin/env python3
"""Execute real how_close and its limited-zone teleport callers at their bounds."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

source = (ROOT / 'src/world/graph.c').read_text()
queue = source[source.index('int bfs_cur_marker;'):source.index('// some very specialized versions')]
fixture = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/graph.h"
#include "world/map.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "core/mm.h"
#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
const int top_of_world = BFS_MAX_ROOMS + 4;
static std::vector<room_data> rooms(top_of_world + 1);
static std::vector<room_direction_data> exits(top_of_world);
P_room world = rooms.data();
static mm_ds pool{};
static int live, allocated, diagnostics;
static std::string last_log;
mm_ds *mm_create(const char *, size_t size, size_t, unsigned) { pool.size=size; return &pool; }
void *_mm_get(mm_ds *p, const char *, int) { ++live; ++allocated; return std::calloc(1,p->size); }
void mm_release(mm_ds *, void *p) { --live; std::free(p); }
void logit(const char *,const char *format,...) { ++diagnostics; last_log=format; }
''' + queue + extract_function('graph.c','int how_close(') + r'''
// Spell adapters control RNG/eligibility and observe movement/combat side effects.
// Spell bodies, room restrictions, graph traversal, and their ordering are real.
static zone_data zones[1]{};
zone_data *zone_table = zones;
static int destination, moved, stopped, waited;
static std::string feedback;
int number(int low, int high) { assert(destination >= low && destination <= high); return destination; }
int get_property(const char *, int fallback) { return fallback; }
int is_Raidable(P_char, char *, int) { return true; }
void send_to_char(const char *text, P_char) { feedback += text; }
void act(const char *, int, P_char, P_obj, void *, int) {}
void stop_fighting(P_char ch) { ++stopped; ch->specials.fighting = nullptr; }
void char_from_room(P_char ch) { ch->in_room = NOWHERE; }
bool char_to_room(P_char ch, int room, int) { ++moved; ch->in_room = room; return true; }
void CharWait(P_char, int ticks) { waited += ticks; }
''' + '\n'.join(extract_function(path, signature) for path, signature in [
    ('magic.c', 'void spell_teleport('),
    ('psionics.c', 'void spell_depart('),
    ('ethermancer.c', 'void spell_etheric_gust('),
]) + r'''
static void check_teleports() {
    zones[0].real_bottom = 0;
    zones[0].real_top = top_of_world;
    for (auto spell : {spell_teleport, spell_depart, spell_etheric_gust}) {
        for (bool separate_target : {false, true}) {
            for (int target : {1, 5, 0, 6, top_of_world}) {
                char_data caster{}, victim{}, opponent{};
                P_char traveler = separate_target ? &victim : &caster;
                caster.specials.position = traveler->specials.position = POS_STANDING | STAT_NORMAL;
                traveler->in_room = 0;
                traveler->specials.fighting = &opponent;
                opponent.specials.fighting = traveler;
                rooms[0].people = &opponent;
                rooms[0].number = 5700;
                destination = target;
                moved = stopped = waited = 0;
                feedback.clear();
                spell(50, &caster, nullptr, SPELL_TYPE_SPELL,
                      separate_target ? traveler : nullptr, nullptr);
                const bool allowed = target <= 5;
                if (allowed && traveler->in_room != target)
                    std::fprintf(stderr, "limited teleport target=%d stayed in room %d\n", target, traveler->in_room);
                assert(traveler->in_room == (allowed ? target : 0));
                assert(moved == (allowed && target != 0 ? 1 : 0));
                assert(stopped == (allowed ? 2 : 0));
                assert(traveler->specials.fighting == (allowed ? nullptr : &opponent));
                assert(opponent.specials.fighting == (allowed ? nullptr : traveler));
                assert((feedback.find("fades away") != std::string::npos) == !allowed);
                assert(waited == (allowed && spell != spell_teleport ? 3 * WAIT_SEC : 0));
                assert(!live && !queue_head && !queue_tail);
            }
        }
        // Outside the limited zone, the same otherwise-valid six-room move succeeds.
        char_data caster{};
        caster.specials.position = POS_STANDING | STAT_NORMAL;
        rooms[0].number = 6000;
        rooms[0].people = nullptr;
        destination = 6;
        spell(50, &caster, nullptr, SPELL_TYPE_SPELL, nullptr, nullptr);
        assert(caster.in_room == 6);
        // A destination excluded by the real graph must not move or apply wait.
        rooms[0].number = 5999;
        exits[0].to_room = NOWHERE;
        caster.in_room = 0;
        stopped = moved = waited = 0;
        feedback.clear();
        destination = 1;
        spell(50, &caster, nullptr, SPELL_TYPE_SPELL, nullptr, nullptr);
        assert(caster.in_room == 0 && !moved && !stopped && !waited);
        assert(feedback.find("fades away") != std::string::npos);
        assert(!live && !queue_head && !queue_tail);
        exits[0].to_room = 1;
    }
}
static void check(int target,int limit,int expected) {
    const int result=how_close(0,target,limit);
    if (result!=expected) std::fprintf(stderr,"distance target=%d limit=%d got=%d expected=%d\n",target,limit,result,expected);
    assert(result==expected);
    assert(!live && !queue_head && !queue_tail);
}
int main() {
    for (int i=0;i<top_of_world;++i) {
        exits[i].to_room=i+1;
        rooms[i].dir_option[0]=&exits[i];
    }
    bfs_reset_marks();
    check(4,3,BFS_NO_PATH); // Target must not bypass the maximum distance.
    check(2,3,2); check(3,3,3); check(1,0,BFS_NO_PATH); check(1,-1,BFS_NO_PATH);
    check(0,0,0); check(-1,3,BFS_ERROR); check(top_of_world+1,3,BFS_ERROR);
    // Door flags remain eligible; ocean destinations and NOWHERE do not.
    exits[0].exit_info=EX_CLOSED|EX_SECRET|EX_BLOCKED|EX_LOCKED;
    check(1,1,1);
    rooms[1].sector_type=SECT_OCEAN;
    check(1,1,BFS_NO_PATH);
    rooms[1].sector_type=SECT_CITY;
    exits[0].to_room=NOWHERE;
    check(1,1,BFS_NO_PATH);
    exits[0].to_room=1;
    // Large traversal retains its room cap, emits one diagnostic and empties its queue.
    diagnostics=allocated=0;
    check(top_of_world,top_of_world,BFS_ERROR);
    assert(allocated<=static_cast<int>(BFS_MAX_ROOMS)+3);
    assert(diagnostics==1 && last_log.find("BFS_MAX_ROOMS")!=std::string::npos);
    check(1,1,1); // A failed search cannot poison the next search.
    diagnostics=0;
    for(int i=0;i<37;++i) check(4,2,BFS_NO_PATH);
    assert(diagnostics==37); // One line per bounded exit, no diagnostic loop.
    check_teleports();
    std::puts("PASS real graph and three spell callers: exact distance, limited-zone movement, rejected moves preserve combat, room cap, eligibility, cleanup and reuse");
}
'''
out=ROOT/'bin/tests'; out.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix='graph-distance-',dir=out) as temporary:
    cpp,binary=Path(temporary)/'test.cpp',Path(temporary)/'test'
    cpp.write_text(fixture)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                    '-g','-Isrc',str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
