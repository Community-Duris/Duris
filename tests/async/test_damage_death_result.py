#!/usr/bin/env python3
"""Actual damage wrapper and cleave stop their continuation after either death."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

production = '\n'.join((extract_function('fight.c', 'bool damage('),
                        extract_function('paladins.c', 'bool cleave(')))
fixture = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "net/comm.h"
#include <cassert>
#include <cstdio>
Skill skills[MAX_SKILLS] = {};
static int result, fighting_calls, counter_calls, timers;
static char_data attacker, victim;
static void reset(int outcome) {
    attacker={}; victim={}; result=outcome;
    attacker.player.level=50;
    victim.specials.act=ACT_ISNPC;
    attacker.in_room=victim.in_room=1;
    SET_POS(&attacker, POS_STANDING+STAT_NORMAL);
    SET_POS(&victim, POS_STANDING+STAT_NORMAL);
    fighting_calls=counter_calls=timers=0;
}
static int apply_deaths(P_char ch, P_char target) {
    if (result==DAM_CHARDEAD || result==DAM_BOTHDEAD) {
        SET_POS(ch, POS_PRONE+STAT_DEAD); ch->in_room=NOWHERE;
    }
    if (result==DAM_VICTDEAD || result==DAM_BOTHDEAD) {
        SET_POS(target, POS_PRONE+STAT_DEAD); target->in_room=NOWHERE;
    }
    return result;
}
int raw_damage(P_char ch,P_char target,double,uint,damage_messages *,int *) {
    return apply_deaths(ch,target);
}
int spell_damage(P_char ch,P_char target,double,int,uint,damage_messages *,int *) {
    return apply_deaths(ch,target);
}
P_char get_linked_char(P_char,ush_int) { return nullptr; }
void set_fighting(P_char,P_char) { ++fighting_calls; }
int attack_back(P_char,P_char,int) { ++counter_calls; return 0; }
int GetLowestSpellCircle_p(int) { return 1; }
bool is_wielding_paladin_sword(P_char) { return true; }
bool affected_by_spell(P_char,int) { return false; }
bool notch_skill(P_char,int,float) { return false; }
float get_property(const char *,double fallback) { return fallback; }
int get_property(const char *,int fallback) { return fallback; }
int GET_CHAR_SKILL_P(P_char,int) { return 100; }
int dice(int,int) { return 1; }
int number(int low,int) { return low; }
void act(const char *,int,P_char,P_obj,void *,int) {}
void set_short_affected_by(P_char target,int,int) {
    assert(target->in_room!=NOWHERE && GET_STAT(target)!=STAT_DEAD);
    ++timers;
}
''' + production + r'''
int main() {
    skills[SPELL_ARMOR].targets=TAR_SPELL;
    reset(DAM_BOTHDEAD);
    assert(cleave(&attacker,&victim));
    assert(!timers && !fighting_calls && !counter_calls);
    for (int skill : {SKILL_CLEAVE,SPELL_ARMOR}) {
        for (int outcome : {DAM_NONEDEAD,DAM_VICTDEAD,DAM_BOTHDEAD,DAM_CHARDEAD}) {
            reset(outcome);
            bool stopped=damage(&attacker,&victim,1,skill);
            assert(stopped==(outcome!=DAM_NONEDEAD));
            if (outcome!=DAM_NONEDEAD) assert(!fighting_calls && !counter_calls);
        }
    }
    for (int outcome : {DAM_NONEDEAD,DAM_VICTDEAD,DAM_BOTHDEAD,DAM_CHARDEAD}) {
        reset(outcome);
        assert(cleave(&attacker,&victim));
        assert(timers==(outcome==DAM_NONEDEAD ? 1 : 0));
        if (outcome!=DAM_NONEDEAD) assert(!fighting_calls && !counter_calls);
    }
    // Fresh use of the same character storage gets only its new live effect.
    reset(DAM_NONEDEAD);
    assert(cleave(&attacker,&victim) && timers==1);
    puts("PASS real damage/cleave: live, victim/caster/both death and fresh reuse");
}
'''
out = ROOT / 'bin/tests'; out.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='damage-death-', dir=out) as temporary:
    cpp, binary = Path(temporary) / 'test.cpp', Path(temporary) / 'test'
    cpp.write_text(fixture)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-g','-Isrc','-I/usr/include/mysql',
                    str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True,timeout=30)
