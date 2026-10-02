#!/usr/bin/env python3
"""Execute production rested branches with isolated engine collaborators.

No database/server required. The XP and login blocks are taken verbatim from
production, as are spell_rest, newb_spellup and wear_off_message. Purchase tests
execute the real merchant list/buy branches before its unrelated wandering code.
"""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

from _source_contract import function_body

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def source(name):
    return (SRC / name).read_text()


def body(name, signature):
    result = function_body(source(name), re.escape(signature))
    assert result, signature
    return result


def harness():
    limits = source("world/limits.c")
    xp = limits[limits.index("if (type == EXP_RESURRECT)", limits.index("int gain_exp(")):]
    xp = xp[:xp.index("if (progression_reason_for_type(type)")]
    login = source("account/nanny.c")
    login = login[login.index("\t// Add well-rested or rested bonus, if applicable."):login.index("\tGetMIA(ch->player.name, Gbuf1);")]
    score = source("cmd/actinf.c")
    score = score[score.index("for (aff = ch->affected; aff; aff = aff->next)"):]
    score_guard = score[score.index("{")+1:score.index("if ((aff->type > 0)")]
    merchant = body("specs/specs.mobile.c", "int witch_doctor(")
    merchant = merchant[:merchant.index("if (!cmd && !number(0, 150))")]
    # Wandering locals are not used in the extracted list/buy entry points.
    merchant = merchant.replace("int i, room, tries, code;", "int i, code;") + "return FALSE;\n}"
    cpp = r'''
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include "world/rested.h"
int setting = -1;
int get_property(const char *key, int fallback) {
    assert(std::string(key) == "exp.rested.enabled");
    return setting < 0 ? fallback : setting;
}
using byte = unsigned char;
using uint = unsigned;
constexpr int TAG_RESTED=1, TAG_WELLRESTED=2, TAG_INNATE_TIMER=3,
    TAG_LAYONHANDS=4, INNATE_LAY_HANDS=1, MAX_WEAR_OFF_MESSAGES=2,
    SPELL_BLINDNESS=5, EXP_RESURRECT=1, EXP_KILL=2, EXP_QUEST=3;
constexpr int AFFTYPE_PERM=1, AFFTYPE_NODISPEL=2, AFFTYPE_REMOVEBYDURATION=4,
    AFFTYPE_OFFLINE=8, AFFTYPE_NOMSG=16, AFFTYPE_NOSHOW=32, AFFTYPE_DOTCHAR=64,
    AFFTYPE_CUSTOM1=128;
constexpr int AFF_HASTE=1, AFF_PROT_FIRE=2, AFF2_PROT_COLD=4, AFF_FLY=8,
    AFF4_EPIC_INCREASE=16, TAG_SNEAK=6, TAG_WITCHSPELL=7;
constexpr int CMD_SET_PERIODIC=1, CMD_LIST=2, CMD_BUY=3;
constexpr int FALSE=0, TRUE=1, TO_ROOM=1, TO_CHAR=2, TO_VICT=3;
constexpr int TELEMETRY_PROGRESSION_MODIFIER_RESTED=1,
    TELEMETRY_PROGRESSION_MODIFIER_WELLRESTED=2;
struct affected_type {
    int type, duration, flags, location, modifier, wear_off_message_index;
    uint bitvector,bitvector2,bitvector3,bitvector4,bitvector5;
    affected_type *next;
};
struct char_data {
    struct { const char *name="test"; } player;
    std::vector<affected_type> effects;
    affected_type *affected=nullptr;
    int level=20, platinum=100000;
};
using character = char_data;
using P_char = char_data *;
using P_obj = void *;
constexpr int SPELL_TYPE_SPELL=0, LOG_WIZ=1;
#define IS_ALIVE(ch) true
#define GET_NAME(ch) "test"
#define J_NAME(ch) "test"
void debug(const char *, ...) {}
void wizlog(int, const char *, ...) {}
void logit(int, const char *, ...) {}
const char *file_to_string(const char *) { return ""; }
struct skill { const char *wear_off_char[2]; const char *wear_off_room[2]; };
skill skills[10]{};
std::string output;
int transactions=0, other_buffs=0, xp_gain=0;
#define GET_LEVEL(ch) ((ch)->level)
#define GET_PLATINUM(ch) ((ch)->platinum)
#define MIN(a,b) ((a)<(b)?(a):(b))
void send_to_char(const char *s, P_char) { output += s; }
void act(const char *s, int, P_char, P_obj, P_char, int) { output += s; }
bool affected_by_spell(P_char ch, int tag) {
    for (const auto &af : ch->effects) if (af.type == tag) return true;
    return false;
}
affected_type *get_spell_from_char(P_char ch,int tag,void * = nullptr,int mask=0) {
    for(auto &af : ch->effects)
        if(af.type==tag && (!mask || (af.flags & mask))) return &af;
    return nullptr;
}
void affect_from_char(P_char ch, int tag) {
    for(auto it=ch->effects.begin();it!=ch->effects.end();)
        if(it->type==tag) it=ch->effects.erase(it); else ++it;
}
void affect_to_char(P_char ch, affected_type *af) { ch->effects.push_back(*af); }
void affect_join(P_char ch, affected_type *af, int, int) {
    affect_from_char(ch, af->type); affect_to_char(ch, af);
}
bool transact(P_char ch, void *, P_char, int amount) {
    ++transactions; ch->platinum-=amount; return true;
}
bool isname(const char *arg, const char *words) {
    const std::string all=std::string(" ")+words+" ";
    return all.find(std::string(" ")+arg+" ")!=std::string::npos;
}
void spell_blindness(int, P_char, char *, int, P_char, P_obj) { ++other_buffs; }
#define STUB_BUFF(name) void name(int, P_char, char *, int, P_char, P_obj) { ++other_buffs; }
STUB_BUFF(spell_bless) STUB_BUFF(spell_spirit_armor) STUB_BUFF(spell_barkskin)
STUB_BUFF(spell_enhance_armor) STUB_BUFF(spell_stone_skin) STUB_BUFF(spell_fly)
STUB_BUFF(spell_haste) STUB_BUFF(spell_strength) STUB_BUFF(spell_agility)
STUB_BUFF(spell_dexterity) STUB_BUFF(spell_accel_healing)
const char *GET_SHORT(P_char) { return "test"; }
void wizardlogf(const char *, ...) {}
'''
    cpp += "bool rested_bonus_effect_active(const affected_type *affect) " + body("magic/affects.c", "bool rested_bonus_effect_active(")
    cpp += "\nbool has_active_rested_bonus(P_char ch,int tag) " + body("magic/affects.c", "bool has_active_rested_bonus(")
    cpp += "\nstatic void apply_rested_bonus(P_char ch,P_char victim,bool staff_override) " + body("magic/spells.c", "static void apply_rested_bonus(")
    cpp += "\nvoid grant_staff_rested_bonus(P_char ch,P_char victim) " + body("magic/spells.c", "void grant_staff_rested_bonus(")
    cpp += "\nvoid spell_rest(int, P_char ch, char *, int, P_char victim, P_obj) " + body("magic/spells.c", "void spell_rest(")
    cpp += "\nvoid newb_spellup(P_char ch, P_char victim) " + body("cmd/actwiz.c", "void newb_spellup(")
    cpp += "\nvoid wear_off_message(P_char ch, affected_type *af) " + body("magic/affects.c", "void wear_off_message(")
    cpp += "\nint witch_doctor(P_char witch,P_char customer,int cmd,char *arg) " + merchant
    cpp += "\nint visible_score_affects(P_char ch) { int count=0; for(auto *aff=ch->affected;aff;aff=aff->next) { " + score_guard + " ++count; } return count; }"
    cpp += "\nvoid login_bonus(P_char ch,long rest,bool nobonus) { affected_type af1; " + login + "\n}"
    cpp += "\nint earned(P_char ch,int type, unsigned &progression_modifier_flags) { double XP=100; " + xp + " return static_cast<int>(XP); }"
    cpp += r'''
int main() {
    character ch, caster, witch;
    for(int config : {-1,1,0}) {
        setting=config;
        const bool on=config!=0;
        assert(rested_bonus_enabled()==on);
        for(int tag : {0,TAG_RESTED,TAG_WELLRESTED}) {
            ch.effects.clear();
            if(tag) ch.effects.push_back({.type=tag,.duration=60});
            for(int type : {EXP_KILL,EXP_QUEST,EXP_RESURRECT}) {
                unsigned flags=0;
                const bool boosted=on && type!=EXP_RESURRECT && tag;
                assert(earned(&ch,type,flags)==(boosted?(tag==TAG_RESTED?150:200):100));
                assert(flags==(boosted?(tag==TAG_RESTED?1U:2U):0U));
            }
        }
        for(long offline : {0L, 8L*3600, 9L*3600, 20L*3600, 40L*3600, 365L*24*60*60}) {
            ch.effects.clear(); output.clear();
            login_bonus(&ch,offline,false);
            assert(ch.effects.size()==(on && offline>=9*3600 ? 1U:0U));
            if(on && offline>=9*3600)
                assert(ch.effects[0].type==(offline>=20*3600?TAG_WELLRESTED:TAG_RESTED));
            ch.effects.clear(); login_bonus(&ch,40*3600,true);
            assert(ch.effects.empty());
        }
        affected_type plain{.type=SPELL_BLINDNESS};
        affected_type well{.type=TAG_WELLRESTED,.next=&plain};
        affected_type rested{.type=TAG_RESTED,.next=&well};
        ch.affected=&rested;
        assert(visible_score_affects(&ch)==(on?3:1));
        ch.affected=nullptr;
        ch.effects.clear(); output.clear();
        spell_rest(61,&caster,nullptr,0,&ch,nullptr);
        assert(affected_by_spell(&ch,TAG_RESTED)==on);
        spell_rest(61,&caster,nullptr,0,&ch,nullptr);
        assert(affected_by_spell(&ch,TAG_WELLRESTED)==on);
        spell_rest(61,&caster,nullptr,0,&ch,nullptr);
        assert(ch.effects.size()==(on?1U:0U));
        if(on) assert(ch.effects[0].duration==150);
        ch.effects.clear(); output.clear(); other_buffs=0;
        newb_spellup(&caster,&ch);
        assert(other_buffs==11);
        // Staff spell-up remains effective even when automatic bonuses are off.
        assert(affected_by_spell(&ch,TAG_RESTED));
        unsigned staff_flags=0;
        assert(earned(&ch,EXP_KILL,staff_flags)==150 && staff_flags==1);
        assert(ch.effects[0].flags & AFFTYPE_CUSTOM1);
        // The ordinary affect flags are the persisted provenance; a restored
        // copy remains effective without relying on the caster's connection.
        auto saved_staff_affect=ch.effects[0];
        ch.effects.clear(); ch.effects.push_back(saved_staff_affect);
        staff_flags=0;
        assert(earned(&ch,EXP_QUEST,staff_flags)==150 && staff_flags==1);
        staff_flags=0;
        assert(earned(&ch,EXP_RESURRECT,staff_flags)==100 && staff_flags==0);
        ch.affected=&ch.effects[0]; assert(visible_score_affects(&ch)==1);
        ch.affected=nullptr;
        newb_spellup(&caster,&ch);
        assert(affected_by_spell(&ch,TAG_WELLRESTED));
        staff_flags=0;
        assert(earned(&ch,EXP_KILL,staff_flags)==200 && staff_flags==2);
        ch.effects[0].duration=1;
        newb_spellup(&caster,&ch);
        assert(ch.effects.size()==1 && ch.effects[0].duration==150);
        assert(ch.effects[0].flags & AFFTYPE_CUSTOM1);
        skills[TAG_WELLRESTED]={{"Expired",nullptr},{"Room expired",nullptr}};
        output.clear(); wear_off_message(&ch,&ch.effects[0]);
        assert(!output.empty());
        output.clear(); newb_spellup(&caster,&ch);
        assert(output.find("disabled")==std::string::npos);
        assert(output.find("Enjoy your blessings")!=std::string::npos);
        ch.effects.clear(); output.clear();
        char empty[]="";
        witch_doctor(&witch,&ch,CMD_LIST,empty);
        assert((output.find("Gnowsis")!=std::string::npos)==on);
        assert(output.find("Acce")!=std::string::npos);
        for(const char *choice : {"6","exp","experience","gnowsis"}) {
            ch.effects.clear(); output.clear(); transactions=0; ch.platinum=100000;
            char arg[32]; std::strcpy(arg,choice);
            witch_doctor(&witch,&ch,CMD_BUY,arg);
            assert(transactions==(on?1:0));
            assert(ch.effects.size()==(on?1U:0U));
            if(!on) assert(ch.platinum==100000);
        }
        ch.effects.clear(); transactions=0;
        char ordinary[]="1";
        witch_doctor(&witch,&ch,CMD_BUY,ordinary);
        assert(transactions==1 && ch.effects.size()==1);
        for(int tag : {TAG_RESTED,TAG_WELLRESTED,SPELL_BLINDNESS}) {
            skills[tag]={{"Expired",nullptr},{"Room expired",nullptr}};
            affected_type af{.type=tag}; output.clear();
            wear_off_message(&ch,&af);
            assert(output.empty()==(!on && tag!=SPELL_BLINDNESS));
        }
    }
    // Disabling must not delete or upgrade an existing saved affect; re-enable
    // restores the original tier until its ordinary duration expires.
    setting=1; ch.effects.clear(); spell_rest(61,&caster,nullptr,0,&ch,nullptr);
    setting=0; output.clear(); spell_rest(61,&caster,nullptr,0,&ch,nullptr);
    assert(ch.effects.size()==1 && ch.effects[0].type==TAG_RESTED);
    unsigned flags=0; assert(earned(&ch,EXP_KILL,flags)==100 && flags==0);
    setting=1; assert(earned(&ch,EXP_KILL,flags)==150 && flags==1);
    // A manual spell-up upgrades a dormant ordinary tag into a marked staff
    // grant rather than reviving every other character's disabled bonus.
    ch.effects.clear(); ch.effects.push_back({.type=TAG_RESTED,.duration=10});
    setting=0; newb_spellup(&caster,&ch);
    assert(ch.effects.size()==1 && ch.effects[0].type==TAG_WELLRESTED);
    assert(ch.effects[0].flags & AFFTYPE_CUSTOM1);
    flags=0; assert(earned(&ch,EXP_KILL,flags)==200 && flags==2);
    character ordinary;
    ordinary.effects.push_back({.type=TAG_WELLRESTED,.duration=150});
    flags=0; assert(earned(&ordinary,EXP_KILL,flags)==100 && flags==0);
    std::puts("rested runtime: defaults, automatic off, staff overrides, both XP tiers, login, purchases, score, expiry, live toggles passed");
}
'''
    return cpp


def main():
    assert re.search(r"^exp\.rested\.enabled=1\.000$",
                     (ROOT / "lib/duris.properties").read_text(), re.M)
    with tempfile.TemporaryDirectory(prefix="rested-runtime-") as tmp:
        cc=Path(tmp)/"harness.cc"
        cc.write_text(harness())
        binary=Path(tmp)/"harness"
        subprocess.run(shlex.split(os.environ.get("CXX","g++"))+[
            "-std=c++20","-Wall","-Wextra","-Werror","-Wno-missing-field-initializers",
            "-I",str(SRC),str(cc),"-o",str(binary)],check=True)
        subprocess.run([str(binary)],check=True,timeout=10)


if __name__ == "__main__":
    main()
