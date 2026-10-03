#!/usr/bin/env python3
"""Execute Sin's actual procedure with null and unrelated callback actors."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from _paths import extract_function


PRELUDE = r'''
#include <cassert>
#include <cstring>
#include <string>
#include <strings.h>
#define TRUE 1
#define FALSE 0
constexpr int CMD_SET_PERIODIC=1, CMD_PERIODIC=2, CLASS_PALADIN=3;
constexpr int TO_VICT=1, TO_NOTVICT=2, SPELL_MAJOR_PARALYSIS=3;
constexpr int AFFTYPE_SHORT=1, AFF2_MAJOR_PARALYSIS=2, WAIT_SEC=4;
struct Character {
    Character *opponent=nullptr;
    bool fighting=false, paladin=false, trusted=false, freedom=false;
    int stopped_casting=0, stopped_fighting=0, affected=0, wait=0;
};
using P_char=Character *;
struct affected_type { int type,flags,duration,bitvector2; };
#define IS_FIGHTING(ch) ((ch)->fighting)
#define GET_OPPONENT(ch) ((ch)->opponent)
#define GET_CLASS(ch,cls) ((ch)->paladin)
#define IS_TRUSTED(ch) ((ch)->trusted)
int roll=0, checks=0;
P_char checked=nullptr;
std::string room_message;
int number(int,int) { return roll; }
bool check_freedom_of_movement(P_char ch,bool clear) {
    // The production helper dereferences ch; a null callback must never reach it.
    assert(ch);
    ++checks; checked=ch;
    if (!ch->freedom) return false;
    if (clear) ch->freedom=false;
    return true;
}
void act(const char *text,int,P_char,void *,P_char,int audience) {
    if (audience==TO_NOTVICT) room_message=text;
}
void StopCasting(P_char ch) { ++ch->stopped_casting; }
void stop_fighting(P_char ch) { ++ch->stopped_fighting; ch->fighting=false; }
void affect_to_char(P_char ch,affected_type *af) {
    assert(af->type==SPELL_MAJOR_PARALYSIS && af->flags==AFFTYPE_SHORT);
    assert(af->bitvector2==AFF2_MAJOR_PARALYSIS && af->duration==WAIT_SEC*10);
    ++ch->affected;
}
void CharWait(P_char ch,int duration) { ch->wait=duration; }
'''

MAIN = r'''
int main() {
    Character sin, opponent, bystander;
    sin.fighting=true; sin.opponent=&opponent;
    opponent.fighting=true; opponent.freedom=true;
    assert(hoa_sin(&sin,nullptr,CMD_SET_PERIODIC,nullptr)==TRUE && checks==0);
    assert(hoa_sin(&sin,nullptr,CMD_PERIODIC,nullptr)==FALSE);
    assert(checked==&opponent && !opponent.freedom && opponent.affected==0);
    bystander.freedom=true;
    assert(hoa_sin(&sin,&bystander,CMD_PERIODIC,nullptr)==TRUE);
    assert(checked==&opponent && bystander.freedom);
    assert(opponent.affected==1 && opponent.stopped_casting==1);
    assert(opponent.stopped_fighting==1 && opponent.wait==WAIT_SEC*10);
    assert(room_message.find("$N's sight")!=std::string::npos);
    assert(room_message.find("&N's sight")==std::string::npos);
    const int prior=checks;
    opponent.paladin=true;
    assert(hoa_sin(&sin,nullptr,CMD_PERIODIC,nullptr)==FALSE && checks==prior);
    opponent.paladin=false; opponent.trusted=true;
    assert(hoa_sin(&sin,nullptr,CMD_PERIODIC,nullptr)==FALSE && checks==prior);
    opponent.trusted=false; sin.opponent=nullptr;
    assert(hoa_sin(&sin,nullptr,CMD_PERIODIC,nullptr)==FALSE && checks==prior);
    sin.opponent=&opponent; roll=1;
    assert(hoa_sin(&sin,nullptr,CMD_PERIODIC,nullptr)==FALSE && checks==prior);
    assert(hoa_sin(nullptr,nullptr,CMD_PERIODIC,nullptr)==FALSE);
}
'''

with tempfile.TemporaryDirectory(prefix="hall-sin-target-") as directory:
    program = Path(directory) / "sin.cpp"
    binary = Path(directory) / "sin"
    program.write_text(PRELUDE + extract_function("specs.hoa.c", "int hoa_sin(") + MAIN,
                       encoding="utf-8")
    subprocess.run([*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20",
                    "-Wall", "-Wextra", "-Werror", str(program), "-o", str(binary)],
                   check=True, timeout=60)
    subprocess.run([str(binary)], check=True, timeout=10)
print("Hall Sin: periodic/null actors, actual opponent protection, unrelated actors, immunity and effect behavior passed.")
