#!/usr/bin/env python3
"""Execute the bound Halfcut ambusher through setup, pulses and interrupted volleys."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import extract_function


PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>
constexpr int TRUE=1, FALSE=0, CMD_SET_PERIODIC=-10, CMD_PERIODIC=0;
constexpr int NOWHERE=-1, TO_ROOM=1, TO_VICT=2, TO_CHAR=3, TYPE_UNDEFINED=-1;
struct Character {
    uint64_t runtime_id=0;
    int in_room=0, hits=0;
    bool alive=true, pc=true;
    Character *next_in_room=nullptr;
};
using P_char=Character *;
struct Room { P_char people=nullptr; } world[5];
std::unordered_map<uint64_t,P_char> registry;
#define IS_PC(ch) ((ch)->pc)
#define IS_ALIVE(ch) ((ch) && (ch)->alive)
int missing_room=0, mode=0, shots=0, messages=0;
P_char shooter=nullptr, replacement=nullptr;
bool char_in_list(P_char ch) {
    for (const auto &entry:registry) if (entry.second==ch) return true;
    return false;
}
P_char find_character_by_runtime_id(uint64_t id) {
    const auto it=registry.find(id);
    return it==registry.end() ? nullptr : it->second;
}
int real_room0(int vnum) {
    if (vnum==missing_room) return 0;
    return vnum==27139 ? 1 : vnum==27137 ? 2 : vnum==27136 ? 3 : 0;
}
int dice(int count,int sides) { assert(count==2 && sides==4); return 5; }
void act(const char *,int,P_char ch,void *,P_char target,int audience) {
    assert(ch==target && char_in_list(target) && target->alive);
    // Production act suppresses the actor unless the audience is TO_CHAR.
    if (audience==TO_VICT && ch==target) return;
    ++messages;
}
bool damage(P_char ch,P_char target,int amount,int type) {
    assert(ch==shooter && amount==15 && type==TYPE_UNDEFINED);
    assert(char_in_list(ch) && ch->alive && char_in_list(target) && target->alive);
    ++shots; ++target->hits;
    if (shots!=1) return false;
    if (mode==1) target->alive=false;
    if (mode==2) target->in_room=4;
    if (mode==3) {
        registry.erase(target->runtime_id);
        // Same storage becomes a new incarnation; the old snapshot must not hit it.
        target->runtime_id=100; registry[100]=target;
    }
    if (mode==4) ch->alive=false;
    if (mode==5) ch->in_room=4;
    if (mode==6) registry.erase(ch->runtime_id);
    if (mode==7) {
        P_char next=target->next_in_room;
        assert(next); registry.erase(next->runtime_id);
        target->next_in_room=replacement;
        next->next_in_room=nullptr;
    }
    return false;
}
void reset(Character &ch,Character &a,Character &b,Character &npc) {
    registry.clear(); for (auto &room:world) room={};
    ch={}; ch.runtime_id=1; ch.pc=false;
    a={}; a.runtime_id=2; a.in_room=1;
    b={}; b.runtime_id=3; b.in_room=1;
    npc={}; npc.runtime_id=4; npc.in_room=1; npc.pc=false;
    registry={{1,&ch},{2,&a},{3,&b},{4,&npc}};
    a.next_in_room=&b; b.next_in_room=&npc; world[1].people=&a;
    shooter=&ch; replacement=nullptr; mode=shots=messages=missing_room=0;
}
'''

MAIN = r'''
int main() {
    Character ch,a,b,npc,extra;
    reset(ch,a,b,npc);
    assert(crossbow_ambusher(nullptr,nullptr,CMD_SET_PERIODIC,nullptr)==TRUE);
    assert(crossbow_ambusher(&ch,&a,CMD_SET_PERIODIC,nullptr)==TRUE);
    assert(shots==0 && messages==0);
    assert(crossbow_ambusher(&ch,&a,17,nullptr)==FALSE && shots==0);
    crossbow_ambusher(&ch,&npc,CMD_PERIODIC,nullptr);
    assert(shots==8 && messages==16 && a.hits==4 && b.hits==4 && npc.hits==0);
    reset(ch,a,b,npc);
    a.next_in_room=nullptr; b.in_room=2; b.next_in_room=nullptr;
    npc.pc=true; npc.in_room=3;
    world[2].people=&b; world[3].people=&npc;
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(a.hits==4 && b.hits==4 && npc.hits==4);
    reset(ch,a,b,npc); missing_room=27139; b.in_room=2;
    b.next_in_room=nullptr; world[2].people=&b;
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(a.hits==0 && b.hits==4);
    for (int interruption:{1,2,3}) {
        reset(ch,a,b,npc); mode=interruption;
        crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
        assert(a.hits==1 && b.hits==4 && shots==5);
    }
    for (int interruption:{4,5,6}) {
        reset(ch,a,b,npc); mode=interruption;
        crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
        assert(a.hits==1 && b.hits==0 && shots==1);
    }
    reset(ch,a,b,npc); mode=7; extra={}; extra.runtime_id=5;
    extra.in_room=1; registry[5]=&extra; replacement=&extra;
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(a.hits==4 && b.hits==0 && extra.hits==0);
    reset(ch,a,b,npc); a.alive=false;
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(a.hits==0 && b.hits==4);
    reset(ch,a,b,npc); ch.in_room=NOWHERE;
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(shots==0);
    ch.in_room=0; registry.erase(ch.runtime_id);
    crossbow_ambusher(&ch,nullptr,CMD_PERIODIC,nullptr);
    assert(shots==0);
    crossbow_ambusher(nullptr,nullptr,CMD_PERIODIC,nullptr);
    assert(shots==0);
}
'''

with tempfile.TemporaryDirectory(prefix="halfcut-crossbow-") as directory:
    program = Path(directory) / "crossbow.cpp"
    binary = Path(directory) / "crossbow"
    program.write_text(PRELUDE + extract_function("specs.halfcut.c", "int crossbow_ambusher(") + MAIN,
                       encoding="utf-8")
    subprocess.run([*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20",
                    "-Wall", "-Wextra", "-Werror", str(program), "-o", str(binary)],
                   check=True, timeout=60)
    subprocess.run([str(binary)], check=True, timeout=10)
print("Halfcut crossbow: setup, three lanes, four bolts, NPC exclusion, missing room, interrupted and removed targets/ambusher passed.")
