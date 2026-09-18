#!/usr/bin/env python3
"""Execute both native sword callbacks and the owned state/energy redesign."""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

# Reuse the controlled world and real scheduler fixture, without executing its tests.
fixture_path = ROOT / 'tests/async/test_native_artifact_runtime.py'
fixture_text = fixture_path.read_text()
scope = {'__file__': str(fixture_path)}
exec(compile(fixture_text.split("HARNESS = r'''", 1)[0], str(fixture_path), 'exec'), scope)
literal, function = scope['literal'], scope['function']
native_boundary = literal(fixture_path, 'HARNESS').split('#define time native_test_time', 1)[0]
native_boundary = native_boundary.replace('int spell_damage(P_char actor,P_char target,double amount,int type,uint flags,damage_messages *,int *) {',
    'int ioun_unused_damage(P_char actor,P_char target,double amount,int type,uint flags,damage_messages *,int *) {')
scope['platform'] = scope['platform'].replace('P_char get_linked_char(P_char, ush_int)',
    'P_char original_get_linked_char(P_char, ush_int)')
affects = '\n'.join(function(ROOT/'src/magic/affects.c', signature) for signature in (
    'void clear_links(P_char ch, P_obj obj, int flag)',
    'void unlink_char_obj_affect(P_char ch, struct affected_type *af)'))

HARNESS = r'''
#include "item/weapon_actions.h"
static int attacks=0, hits=0, damage_amount=0, healed_amount=0, packed=0, dirty=0, call_mutation=0;
static bool skin=true;
static P_char sword_master=nullptr;
P_char get_linked_char(P_char,ush_int) { return sword_master; }
int BOUNDED(int low,int value,int high) { return std::clamp(value,low,high); }
int GET_CHAR_SKILL_P(P_char,int) { return 0; }
bool has_skin_spell(P_char) { return skin; }
void release_necroplasm_forms() {}
void do_itemmana(P_char,char *,int) {}
void stop_fighting(P_char actor) { actor->specials.fighting=nullptr; }
void attack(P_char actor,P_char target) {
    ++attacks; actor->specials.fighting=target;
    if(call_mutation==1) discard_character(target);
    if(call_mutation==2) depart(actor);
}
bool hit(P_char actor,P_char target,P_obj,int *) {
    ++hits;
    if(call_mutation==1) discard_character(target);
    if(call_mutation==2) depart(actor);
    if(call_mutation==3) actor->specials.fighting=nullptr;
    return false;
}
int spell_damage(P_char actor,P_char target,double amount,int type,uint flags,damage_messages *,int *dealt) {
    assert(type==SPLDAM_NEGATIVE && amount==50 && (flags&SPLDAM_NODEFLECT));
    damage_amount+=amount; if(dealt) *dealt+=amount;
    if(call_mutation==1) discard_character(target);
    if(call_mutation==2) discard_character(actor);
    return 0;
}
int vamp(P_char actor,double amount,double cap) {
    healed_amount+=amount; GET_HIT(actor)=std::min(static_cast<int>(cap),GET_HIT(actor)+static_cast<int>(amount)); return 0;
}
void heal(P_char target,P_char actor,int points,int cap) {
    healed_amount+=points; invoke(-10,points,actor,nullptr,cap,target,nullptr);
}
int is_char_in_room(const P_char actor,int room) { return actor && actor->in_room==room; }
item_action_start selected_packed_weapon_action(P_obj,P_char,P_char) { ++packed; return item_action_start::legacy; }
#define SWORD_SPELL(name,id) void name(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(id,p,a,s,t,v,o); }
SWORD_SPELL(spell_blur,SPELL_BLUR)
SWORD_SPELL(spell_deflect,SPELL_DEFLECT)
SWORD_SPELL(spell_dispel_magic,SPELL_DISPEL_MAGIC)
SWORD_SPELL(spell_fireshield,SPELL_FIRESHIELD)
SWORD_SPELL(spell_soulshield,SPELL_SOULSHIELD)
SWORD_SPELL(spell_stone_skin,SPELL_STONE_SKIN)
SWORD_SPELL(spell_stornogs_spheres,SPELL_STORNOGS_SPHERES)
SWORD_SPELL(spell_heal,SPELL_HEAL)
SWORD_SPELL(spell_vigorize_critic,SPELL_VIGORIZE_CRITIC)
SWORD_SPELL(spell_protection_from_cold,SPELL_PROTECT_FROM_COLD)
SWORD_SPELL(spell_protection_from_fire,SPELL_PROTECT_FROM_FIRE)
SWORD_SPELL(spell_protection_from_acid,SPELL_PROTECT_FROM_ACID)
SWORD_SPELL(spell_protection_from_gas,SPELL_PROTECT_FROM_GAS)
SWORD_SPELL(spell_protection_from_lightning,SPELL_PROTECT_FROM_LIGHTNING)
SWORD_SPELL(spell_armor,SPELL_ARMOR)
SWORD_SPELL(spell_bless,SPELL_BLESS)
SWORD_SPELL(spell_blindness,SPELL_BLINDNESS)
SWORD_SPELL(spell_curse,SPELL_CURSE)
SWORD_SPELL(spell_bigbys_crushing_hand,SPELL_BIGBYS_CRUSHING_HAND)
SWORD_SPELL(spell_bigbys_clenched_fist,SPELL_BIGBYS_CLENCHED_FIST)
SWORD_SPELL(spell_immolate,SPELL_IMMOLATE)
SWORD_SPELL(spell_earthquake,SPELL_EARTHQUAKE)
SWORD_SPELL(spell_poison,SPELL_POISON)
SWORD_SPELL(spell_holy_word,SPELL_HOLY_WORD)
SWORD_SPELL(spell_unholy_word,SPELL_UNHOLY_WORD)
SWORD_SPELL(spell_nova,SPELL_NOVA)
void resolve_nova(P_char actor) { invoke(SPELL_NOVA,60,actor,nullptr,0,nullptr,nullptr); }
SWORD_SPELL(spell_group_heal,SPELL_GROUP_HEAL)
SWORD_SPELL(spell_group_stone_skin,SPELL_GROUP_STONE_SKIN)
SWORD_SPELL(spell_group_stornog,-20) // Native helper has no registered spell ID.
#define time native_test_time
#include "item/native_artifact_actions.c"
namespace artifact_control {
bool control_enabled(int) { return true; }
bool control_allows_variant(int, holder_kind, const char *) { return true; }
bool control_power_enabled(int, const char *) { return true; }
int control_power_level(int, const char *, int fallback) { return fallback; }
}
#include "item/sword_actions.c"
// INSERT_CALLBACKS
#undef time

struct sword_scene : scene {
    P_obj enemy = new obj_data{};
    explicit sword_scene(int vnum=22,bool enabled=true) {
        native_configs.clear(); calls.clear(); choices.clear(); reserves.clear(); tokens.clear();
        debits=mutation=attacks=hits=damage_amount=healed_amount=packed=dirty=call_mutation=extracts=0;
        legal=visible=bound_owner=storage_ready=skin=true;
        static index_data index[2]{}; obj_index=index;
        obj_index[0].virtual_number=vnum; obj_index[1].virtual_number=vnum==21?22:21;
        obj_index[0].func.obj=good_evil_sword;
        enemy->R_num=1; enemy->obj_uid=200; enemy->next=object_list; object_list=enemy;
        source->type=ITEM_WEAPON; source->name=const_cast<char *>("sword");
        source->value[7]=-10000; actor->specials.fighting=target;
        actor->player.racewar=vnum==22?RACEWAR_GOOD:RACEWAR_EVIL;
        GET_ALIGNMENT(actor)=vnum==22?500:-500;
        GET_HIT(actor)=GET_HIT(target)=GET_MAX_HIT(actor)=GET_MAX_HIT(target)=1000;
        SET_BIT(actor->specials.affected_by2,AFF2_FIRESHIELD);
        world[0].people=actor; actor->next_in_room=target; world[0].room_flags=0; world[0].sector_type=SECT_INSIDE;
        auto prefix="itemActions.artifact."+std::to_string(vnum)+".";
        properties[prefix+"enabled"]=enabled; properties[prefix+"manaCost"]=10;
        properties[prefix+"manaCapacity"]=100; properties[prefix+"passiveFloor"]=0;
        update_native_artifact_properties();
        mana_profile={static_cast<uint64_t>(vnum),1,100,0,0};
        auto row=artifact_mana_empty(100,mana_profile,wall); row.reserve=100; reserves[100]=row;
    }
    ~sword_scene() { world[0].people=nullptr; }
    void nemesis() {
        target->equipment[PRIMARY_WEAPON]=enemy; enemy->loc_p=LOC_WORN; enemy->loc.wearing=target;
    }
    int event(int command=CMD_PERIODIC) { char argument[]="sword"; return good_evil_sword(source,actor,command,argument); }
    void combat(int choice) {
        source->value[6]=choice; choices={1}; if(OBJ_VNUM(source)==21) choices.push_back(choice);
        event();
    }
};

static void sword_combat_cases() {
    const int spells[15]={0,SPELL_BLINDNESS,SPELL_CURSE,SPELL_BIGBYS_CRUSHING_HAND,0,
        SPELL_HEAL,SPELL_BIGBYS_CLENCHED_FIST,SPELL_IMMOLATE,SPELL_EARTHQUAKE,0,
        SPELL_STORNOGS_SPHERES,SPELL_POISON,SPELL_HOLY_WORD,0,SPELL_NOVA};
    const int multipliers[15]={1,1,1,2,1,2,2,1,2,1,2,1,2,1,4};
    for(int vnum:{21,22}) for(int choice=0;choice<15;++choice) {
        sword_scene s(vnum); s.combat(choice);
        assert(calls.empty() && !damage_amount && !hits);
        assert(s.source->value[6]==(choice+1)%15 && reserves[100].reserve==100-10*multipliers[choice]);
        advance(choice==14?40:12); assert(choices.empty());
        if(choice==4 || choice==9 || choice==13) { assert(damage_amount==50 && healed_amount==50 && calls.empty()); }
        else if(choice==0) assert(calls.empty());
        else {
            assert(!calls.empty()); const auto &call=calls[0];
            assert(call.id==(choice==12 && vnum==21?SPELL_UNHOLY_WORD:spells[choice]));
            assert(call.target==((choice==8 || choice==14)?0:(choice==5 || choice==10)?s.actor->runtime_id:s.target->runtime_id));
            assert(call.power==(choice==11?30:choice==5?55:choice==10?56:60));
        }
        assert(!item_actions_pending() && reserves[100].reserve==100-10*multipliers[choice]);
    }
    for(int change=0;change<7;++change) {
        sword_scene s; s.combat(1);
        if(change==0) { depart(s.target); s.target->in_room=0; }
        if(change==1) item_actions_source_leaving(s.source);
        if(change==2) { properties["itemActions.artifact.22.enabled"]=0; update_native_artifact_properties(); }
        if(change==3) legal=false;
        if(change==4) world[0].room_flags=ROOM_NO_MAGIC;
        if(change==5) { SET_BIT(s.source->extra2_flags,ITEM2_ACCOUNT_BOUND); bound_owner=false; }
        if(change==6) SET_POS(s.actor,STAT_SLEEPING+POS_PRONE);
        advance(); assert(calls.empty() && reserves[100].reserve==90);
    }
    for(int change=0;change<3;++change) {
        sword_scene s; if(change==0) reserves[100].reserve=9;
        if(change==1) storage_ready=false;
        if(change==2) properties["itemActions.artifact.22.passiveFloor"]=95;
        s.combat(1); advance(); assert(calls.empty() && s.source->value[6]==1);
    }
    { sword_scene s; reserves[100].reserve=10; s.combat(1); advance(); assert(calls.size()==1 && reserves[100].reserve==0); }
    for(int change:{1,2}) {
        sword_scene s; call_mutation=change; s.combat(4); advance();
        assert(damage_amount==50 && healed_amount==(change==1?50:0));
    }
    for(int change:{1,2,5}) {
        sword_scene s; const uint64_t target_id=s.target->runtime_id; mutation=change; s.combat(1); advance();
        if(auto *target=find_character_by_runtime_id(target_id))
            assert(target->specials.apply_saving_throw[SAVING_SPELL]==0);
    }
}

static void sword_defense_cases() {
    for(int choice=0;choice<5;++choice) {
        sword_scene s; s.actor->specials.fighting=nullptr; s.source->value[6]=choice;
        GET_HIT(s.actor)=100; choices={1,0}; s.event();
        if(choice==1) { choices={5,2}; }
        advance(); assert(choices.empty());
        assert(reserves[100].reserve==(choice==0?70:90));
        assert(!calls.empty());
        if(choice==1) assert(calls[0].id==-10 && calls[0].power==75);
        if(choice==3) assert(calls.size()==5 && calls[0].id==SPELL_PROTECT_FROM_COLD && calls[4].id==SPELL_PROTECT_FROM_LIGHTNING);
        if(choice==4) assert(calls.size()==2 && calls[0].target==s.actor->runtime_id && calls[1].id==SPELL_BLESS);
    }
    { sword_scene s; s.actor->specials.fighting=nullptr; s.source->value[6]=1; choices={1,0}; s.event(); advance();
      assert(calls.size()==1 && calls[0].id==SPELL_VIGORIZE_CRITIC); }
    { sword_scene s; s.actor->specials.fighting=nullptr; s.source->value[6]=10;
      SET_BIT(s.actor->specials.affected_by4,AFF4_STORNOGS_SPHERES); choices={1,0,1}; s.event(); advance();
      assert(calls.size()==1 && calls[0].id==SPELL_STONE_SKIN && calls[0].power==45); }
    { sword_scene s; s.actor->specials.fighting=nullptr; s.source->value[6]=4;
      group_list group{}; s.actor->group=s.target->group=&group; choices={1,0}; s.event(); advance();
      assert(calls.size()==4 && calls[0].id==SPELL_ARMOR && calls[1].id==SPELL_BLESS && calls[2].target==s.target->runtime_id);
      s.actor->group=s.target->group=nullptr; }
    { sword_scene s; s.actor->specials.fighting=nullptr; s.source->value[6]=3; mutation=5;
      choices={1,0}; s.event(); advance(); assert(calls.size()==1); }
    { sword_scene s; REMOVE_BIT(s.actor->specials.affected_by2,AFF2_FIRESHIELD); skin=false;
      s.combat(1); advance(); assert(calls.size()==3 && calls[0].id==SPELL_FIRESHIELD && calls[1].id==SPELL_STONE_SKIN);
      assert(reserves[100].reserve==70 && s.source->timer[0]==wall); }
}

static void sword_nemesis_cases() {
    { sword_scene s; s.nemesis(); choices={1}; s.event(); advance(); assert(s.source->value[5] && reserves[100].reserve==90); }
    for(int change=0;change<3;++change) {
        sword_scene s; s.nemesis(); s.actor->specials.fighting=nullptr;
        s.event(); assert(!attacks && !calls.size() && !s.source->value[5]);
        if(change==1) depart(s.target);
        if(change==2) mutation=5;
        advance(); assert(attacks==(change==0?2:0)); assert(reserves[100].reserve==60);
    }
    for(int change=0;change<4;++change) {
        sword_scene s; s.nemesis(); s.source->value[5]=1; call_mutation=change;
        choices={1,1,0,4}; assert(!s.event(CMD_GOTNUKED)); assert(!hits); advance();
        assert(hits==(change?1:4) && reserves[100].reserve==60);
    }
    { sword_scene s; s.nemesis(); s.source->value[5]=1; choices={0};
      assert(!s.event(CMD_GOTNUKED)); advance(); assert(calls.size()==1 && calls[0].id==SPELL_DEFLECT); }
    { sword_scene s; s.source->value[5]=1; assert(!s.event(CMD_GOTNUKED)); assert(calls.size()==1 && calls[0].id==SPELL_DISPEL_MAGIC); }
    { sword_scene s; s.nemesis(); s.source->value[5]=1; assert(s.event(CMD_FLEE)); assert(!item_actions_pending() && !debits); }
    { sword_scene s; s.nemesis(); s.source->value[5]=1; s.source->value[7]=10;
      choices={0}; assert(!weapon_proc(s.source,s.actor,s.target)); advance(); assert(!packed && calls.size()==1); }
    for(bool enabled:{false,true}) {
        sword_scene s(22,enabled); s.actor->player.racewar=RACEWAR_EVIL;
        assert(!s.event(CMD_GOTNUKED)); assert(extracts==1 && !s.actor->equipment[PRIMARY_WEAPON]);
    }
    { sword_scene s; s.source->value[6]=13; obj_to_char(unequip_char(s.actor,WIELD),s.actor);
      s.event(CMD_WIELD); assert(s.source->value[6]==13 && s.source->value[1]==5 && !debits); }
}

static void sword_legacy_comparison() {
    // Compare actual old helper payloads separately from the new resource policy.
    for(int vnum:{21,22}) for(int choice:{1,2,3,6,7,8,11,12}) {
        sword_scene s(vnum); s.source->value[6]=choice;
        if(vnum==21) choices={choice};
        good_evil_fightingProc(s.actor,s.source,vnum==22,500);
        assert(calls.size()==1); const auto baseline=calls[0]; calls.clear();
        s.combat(choice); assert(calls.empty()); advance(); assert(calls.size()==1);
        const auto migrated=calls[0];
        assert(baseline.id==migrated.id && baseline.power==migrated.power &&
            baseline.type==migrated.type && baseline.target==migrated.target);
    }
    { sword_scene s; s.source->value[6]=4;
      const int old_energy=good_evil_fightingProc(s.actor,s.source,true,500);
      assert(old_energy==700 && GET_HIT(s.actor)==1050 && GET_HIT(s.target)==950);
      s.combat(4); advance(); assert(reserves[100].reserve==90 && GET_HIT(s.actor)==1000); }
    for(int choice:{8,14}) for(int changed=0;changed<2;++changed) {
        sword_scene s; s.combat(choice);
        if(changed==0) SET_BIT(world[0].room_flags,ROOM_SINGLE_FILE);
        else sword_master=s.target;
        advance(40); assert(calls.empty() && !item_actions_pending()); sword_master=nullptr;
    }
    { sword_scene s; s.combat(14); advance(12); assert(calls.empty() && item_actions_pending());
      item_actions_source_leaving(s.source); advance(40); assert(calls.empty()); }
    { sword_scene s; properties["itemActions.maxPulses"]=8; update_item_action_properties();
      s.combat(14); advance(40); assert(calls.empty() && !debits && !item_actions_pending()); }
}

static void sword_depletion_recovery() {
    sword_scene s;
    properties["itemActions.artifact.22.manaCost"]=1000;
    properties["itemActions.artifact.22.manaCapacity"]=10000;
    properties["itemActions.artifact.22.manaRegen"]=100;
    properties["itemActions.artifact.22.manaRevision"]=2;
    update_native_artifact_properties();
    mana_profile={22,2,10000,100,0};
    reserves[100]=artifact_mana_empty(100,mana_profile,wall); reserves[100].reserve=10000;
    // Twenty selected PvE attempts, every three seconds. Wall time is explicit;
    // advance() drives only the monotonic scheduler clock in this fixture.
    int pve=0;
    for(int attempt=0;attempt<20;++attempt) {
        calls.clear(); s.combat(1); advance(); pve+=calls.size(); wall+=3;
    }
    auto reserve=reserves[100]; assert(artifact_mana_project(reserve,mana_profile,wall));
    assert(pve==15 && reserve.reserve==1000);
    // Immediate hostile hand costs 2 MP and cannot consume a 1 MP reserve.
    calls.clear(); s.combat(3); advance(); assert(calls.empty());
    wall+=10; s.combat(3); advance(); assert(calls.size()==1 && reserves[100].reserve==0);
    std::puts("Sword scenario: 10 MP start; 15/20 blind selections over 60 s; 1 MP at PvP; 2 MP hand suppressed; 10 s recharge admits one hand, 0 MP left");
}

int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool; fake_clock_ns=1000000000ULL; ne_events();
    sword_combat_cases(); sword_defense_cases(); sword_nemesis_cases();
    sword_legacy_comparison(); sword_depletion_recovery();
    std::puts("Mayhem/Symmetry: native effects, bounded state, mana, groups, nemesis, flurries and effect liveness passed");
}
'''

specs = ROOT / 'src/specs/specs.object.c'
signatures = ('int good_evil_stoneOrSoulshield(', 'void good_evil_procDrain(',
    'int good_evil_attemptFightProc(', 'int good_evil_fightingProc(',
    'int good_evil_attemptDefenseProc(', 'int good_evil_defenseProc(', 'int good_evil_checkHunger(',
    'int isWieldingVnum(', 'void good_evil_spellUp(', 'void good_evil_startBigFight(',
    'void good_evil_coolDown(', 'int killOtherSword(', 'int attemptToDisengage(',
    'void good_evil_poofSword(', 'void good_evil_configSword(', 'int good_evil_sword(')
callbacks='\n'.join(function(specs,s) for s in signatures)
callbacks+='\n'+function(ROOT/'src/combat/fight.c','bool weapon_proc(')
with tempfile.TemporaryDirectory(prefix='duris-swords-') as directory:
    source=Path(directory)/'harness.cpp'; binary=Path(directory)/'harness'
    source.write_text(scope['platform']+scope['fixture']+scope['boundary']+
        native_boundary.replace('// INSERT_AFFECTS',affects)+HARNESS.replace('// INSERT_CALLBACKS',callbacks))
    subprocess.run(['g++','-std=c++20','-O1','-g','-ffunction-sections','-fdata-sections',
        '-fsanitize=address,undefined','-fno-omit-frame-pointer','-pthread','-I'+str(ROOT/'src'),
        str(source),str(ROOT/'src/persistence/latency_trace.c'),'-Wl,--gc-sections','-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,
        ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
        DURIS_NEVENT_ANALYTICS='0',DURIS_NEVENT_BUDGET_USEC='0',DURIS_NEVENT_MAX_CALLBACKS='0',
        DURIS_NEVENT_PLAYER_PRIORITY='1'))
