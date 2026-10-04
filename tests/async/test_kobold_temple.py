#!/usr/bin/env python3
"""Execute Kobold's guardians in the rooms declared by their native resets."""

import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from _paths import ROOT, extract_function


PRELUDE = r'''
#include <cassert>
#include <string>
constexpr int TRUE=1, FALSE=0, CMD_SET_PERIODIC=-10, CMD_NORTH=1,
    CMD_EAST=2, CMD_SOUTH=3, CMD_WEST=4, CMD_UP=5, CMD_DOWN=6,
    CMD_CURSE=9, VIRTUAL=1, TO_CHAR=1, TO_NOTVICT=2, TO_ROOM=3, LOG_EXIT=1;
struct Npc { int spec[1]={0}; };
struct Character {
    int in_room=0, vnum=0;
    bool pc=false, trusted=false, fighting=false, visible=true;
    Character *next=nullptr, *next_in_room=nullptr;
    struct { Npc *npc=nullptr; } only;
};
using P_char=Character *;
struct Room { int number=0; P_char people=nullptr; bool exits[6]={}; } world[8];
P_char character_list=nullptr;
Character summoned;
int roll=50, loads=0, moves=0, attacks=0, curses=0;
bool load_fails=false;
P_char attacked=nullptr;
#define IMP_LIMIT 5
#define IS_PC(ch) ((ch)->pc)
#define IS_NPC(ch) (!(ch)->pc)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define IS_FIGHTING(ch) ((ch)->fighting)
#define GET_VNUM(ch) ((ch)->vnum)
#define CAN_SEE(ch,k) ((k)->visible)
#define EXIT(ch,d) (world[(ch)->in_room].exits[d])
int cmd_to_exitnumb(int cmd) { return cmd-1; }
int real_room(int vnum) {
    for (int i=0;i<8;++i) if (world[i].number==vnum) return i;
    assert(false); return -1;
}
int number(int low,int high) { assert(roll>=low && roll<=high); return roll; }
void act(const char *,int,P_char,void *,P_char,int) {}
void logit(int,const char *) {}
void char_from_room(P_char ch) { ch->in_room=-1; }
void char_to_room(P_char ch,int room,int) { assert(room>=0 && room<8); ch->in_room=room; ++moves; }
P_char read_mobile(int vnum,int mode) {
    assert(vnum==1440 && mode==VIRTUAL); ++loads;
    if (load_fails) return nullptr;
    summoned={}; summoned.vnum=vnum; return &summoned;
}
void do_action(P_char,void *,int cmd) { assert(cmd==CMD_CURSE); ++curses; }
void MobStartFight(P_char ch,P_char victim) {
    assert(ch->in_room==victim->in_room); ++attacks; attacked=victim;
}
'''

MAIN = r'''
int main() {
    for (int i=0;i<8;++i) world[i].number=1480+i;
    world[1].exits[0]=world[1].exits[1]=world[1].exits[2]=world[1].exits[3]=true;
    Npc priest_data;
    Character priest,player,golem,demon,head,outside,ledge,imps[5];
    priest.in_room=real_room(1481); priest.only.npc=&priest_data;
    player.pc=true; player.in_room=priest.in_room;
    assert(kobold_priest(&priest,nullptr,CMD_SET_PERIODIC,nullptr)==TRUE);
    // Altar barriers are deterministic; the old misplaced-room branch allowed low rolls.
    roll=1;
    for (int cmd:{CMD_NORTH,CMD_SOUTH,CMD_EAST})
        assert(kobold_priest(&priest,&player,cmd,nullptr)==TRUE);
    assert(kobold_priest(&priest,&player,CMD_WEST,nullptr)==TRUE);
    assert(player.in_room==real_room(1484) && moves==1);
    priest.fighting=true; roll=50;
    assert(kobold_priest(&priest,nullptr,0,nullptr)==TRUE);
    assert(loads==1 && summoned.in_room==priest.in_room && priest_data.spec[0]==4);
    for (int i=0;i<4;++i) assert(kobold_priest(&priest,nullptr,0,nullptr)==FALSE);
    assert(loads==1 && priest_data.spec[0]==0);
    for (int i=0;i<5;++i) { imps[i].vnum=1440; imps[i].next=i<4?&imps[i+1]:nullptr; }
    character_list=imps;
    assert(kobold_priest(&priest,nullptr,0,nullptr)==FALSE && loads==1);
    character_list=nullptr; priest_data.spec[0]=0; roll=90;
    assert(kobold_priest(&priest,nullptr,0,nullptr)==FALSE && curses==1 && loads==1);
    priest_data.spec[0]=0; roll=50; load_fails=true;
    assert(kobold_priest(&priest,nullptr,0,nullptr)==FALSE && loads==2);
    golem.in_room=real_room(1482); player.in_room=golem.in_room; roll=21;
    assert(stone_golem(&golem,&player,CMD_WEST,nullptr)==TRUE);
    roll=20; assert(stone_golem(&golem,&player,CMD_WEST,nullptr)==FALSE);
    roll=100; player.trusted=true;
    assert(stone_golem(&golem,&player,CMD_WEST,nullptr)==FALSE);
    player.trusted=false;
    assert(stone_golem(&golem,&player,CMD_EAST,nullptr)==FALSE);
    assert(stone_golem(&golem,nullptr,CMD_WEST,nullptr)==FALSE);
    demon.in_room=real_room(1484); player.in_room=demon.in_room;
    assert(tako_demon(&demon,nullptr,CMD_SET_PERIODIC,nullptr)==TRUE);
    roll=51; assert(tako_demon(&demon,&player,CMD_UP,nullptr)==TRUE);
    roll=50; assert(tako_demon(&demon,&player,CMD_UP,nullptr)==FALSE);
    player.trusted=true; roll=100;
    assert(tako_demon(&demon,&player,CMD_UP,nullptr)==FALSE);
    player.trusted=false;
    // A ledge NPC's global successor is elsewhere; its room successor is eligible.
    head.in_room=real_room(1483); head.next=&outside; head.next_in_room=&ledge;
    outside.pc=true; outside.in_room=real_room(1485);
    ledge.pc=true; ledge.in_room=real_room(1483);
    world[real_room(1483)].people=&head; roll=29;
    assert(tako_demon(&demon,nullptr,0,nullptr)==TRUE);
    assert(ledge.in_room==demon.in_room && outside.in_room==real_room(1485));
    assert(attacked==&ledge && attacks==1);
    ledge.in_room=real_room(1483); roll=30;
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE && attacks==1);
    roll=0; ledge.trusted=true;
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE);
    ledge.trusted=false; ledge.fighting=true;
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE);
    ledge.fighting=false; ledge.visible=false;
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE);
    ledge.visible=true; demon.fighting=true;
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE);
    demon.fighting=false; demon.in_room=real_room(1485);
    assert(tako_demon(&demon,nullptr,0,nullptr)==FALSE && attacks==1);
}
'''

resets = (ROOT / "areas/zon/kobold.zon").read_text(encoding="utf-8")
for vnum, room in ((1437, 1481), (1438, 1482), (1436, 1484)):
    assert re.search(rf"^M\s+0\s+{vnum}\s+\d+\s+{room}\s+100\b", resets, re.M)

with tempfile.TemporaryDirectory(prefix="kobold-temple-", dir=ROOT / "bin") as directory:
    program = Path(directory) / "temple.cpp"
    binary = Path(directory) / "temple"
    bodies = "\n".join(extract_function("specs.kobold.c", "int " + name + "(")
                       for name in ("kobold_priest", "stone_golem", "tako_demon"))
    program.write_text(PRELUDE + bodies + MAIN, encoding="utf-8")
    subprocess.run([*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20",
                    "-Wall", "-Wextra", "-Werror", str(program), "-o", str(binary)],
                   check=True, timeout=60)
    subprocess.run([str(binary)], check=True, timeout=10)
print("Kobold temple: actual reset rooms, altar barriers/pit, imp cadence/cap, tomb escape, pit escape and ledge room-list targeting passed.")
