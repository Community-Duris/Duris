#!/usr/bin/env python3
"""Run native artifact adapters and actual legacy entry points with the owned scheduler."""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def literal(path, name):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
                if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == name for t in n.targets))


def function(path, signature):
    text = path.read_text(); start = text.index(signature)
    end = text.index("{", start) + 1; depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}"); end += 1
    return text[start:end]


platform = literal(ROOT / "tests/async/test_nevent_scheduler_runtime.py", "HARNESS")
platform = platform.split("struct record_payload\n", 1)[0]
platform = platform.replace("DEFINE_LABEL_CALLBACK(event_item_action_active)", "")
platform = platform.replace("test_world[1]", "test_world[2]").replace("top_of_world = 0", "top_of_world = 1")
fixture = literal(ROOT / "tests/async/test_item_actions_runtime.py", "HARNESS")
fixture = fixture.replace("int main() {", "void foundation_regression_main() {")
fixture = fixture.replace("void send_to_char(const char *, P_char) {}", "")
fixture = fixture.replace("void act(const char *, int, P_char, P_obj, void *, int) {}", "")
fixture = fixture.replace("void affect_from_char(P_char, int) {}", "")
fixture = fixture.replace("// INSERT_PRODUCTION_ABORT", function(ROOT / "src/net/sparser.c", "void do_abort(P_char ch,"))
boundary = literal(ROOT / "tests/async/test_device_actions_runtime.py", "HARNESS").split("// INSERT_COMMANDS", 1)[0]
boundary = boundary.replace('#include "item/device_actions.c"', '')
boundary = boundary.replace('#include "sql/sql.h"', '')
boundary = boundary.replace('P_obj unequip_char(P_char actor, int slot, bool)', 'P_obj unequip_char(P_char actor, int slot, bool saving)')
boundary = boundary.replace('actor->equipment[slot] = nullptr;', 'if(!saving) clear_links(actor,source,LNKFLG_BREAK_REMOVE); actor->equipment[slot] = nullptr;')
boundary = boundary.replace('bool affected_by_spell(P_char, int) { return false; }', '''
bool affected_by_spell(P_char actor, int spell) {
    for(auto *af=actor->affected;af;af=af->next) if(af->type==spell) return true;
    return false;
}''')

HARNESS = r'''
#include "item/artifact_mana.h"
#include "persistence/persistence_checkpoint.h"
void mark_player_dirty_components(int, player_component_mask_t) {}
#include "item/artifact_mana_model.c"
#include "combat/damage.h"
#include <deque>
static std::deque<int> choices;
static artifact_mana_profile mana_profile;
static std::map<uint64_t, artifact_mana_record> reserves;
static std::map<uint64_t, uint64_t> tokens;
static bool storage_ready = true;
static int debits = 0, hums = 0, reflected = 0, reflect_mutation = 0;
static uint64_t reflected_target = 0;
static time_t wall = 1000;
static bool prime_allowed=true, theurgist=false, save_succeeds=true;
static int worn_slot=-1, form_spells=0, off_messages=0;
static mm_ds native_link_pool{};
static mm_ds *dead_obj_link_pool=&native_link_pool;
static link_description link_types[LNK_MAX+1]{};
void unlink_char_obj_affect(P_char, affected_type *);
void wear_off_message(P_char, affected_type *af) { assert(af->type>0); ++off_messages; }
char_obj_link_data *link_char_obj_with_affect(P_char actor,P_obj source,ush_int type,affected_type *af) {
    auto *link=static_cast<char_obj_link_data *>(calloc(1,sizeof(char_obj_link_data))); ++native_link_pool.objs_used;
    link->ch=actor; link->obj=source; link->type=type; link->affect=af;
    link->next=actor->obj_linked; actor->obj_linked=link; return link;
}
affected_type *affect_to_char(P_char actor,affected_type *af) {
    auto *copy=new affected_type(*af); copy->next=actor->affected; actor->affected=copy;
    actor->specials.affected_by2|=af->bitvector2;
    actor->specials.affected_by4|=af->bitvector4;
    if(af->type==SPELL_VAMPIRE || af->type==SPELL_ANGELIC_COUNTENANCE) {
        ++form_spells;
        if(mutation==6) item_actions_source_leaving(object_list);
    }
    return copy;
}
void affect_remove(P_char actor,affected_type *af) {
    affected_type **entry=&actor->affected;
    while(*entry && *entry!=af) entry=&(*entry)->next;
    assert(*entry); *entry=af->next;
    if(af->flags&AFFTYPE_LINKED_OBJ) unlink_char_obj_affect(actor,af);
    delete af;
    actor->specials.affected_by2=actor->specials.affected_by4=0;
    for(auto *other=actor->affected;other;other=other->next) {
        actor->specials.affected_by2|=other->bitvector2;
        actor->specials.affected_by4|=other->bitvector4;
    }
}
void affect_from_char(P_char actor,int spell) {
    for(auto *af=actor->affected,*next=af;af;af=next) {
        next=af->next; if(af->type==spell) affect_remove(actor,af);
    }
}
int can_prime_class_use_item(P_char,P_obj) { return prime_allowed; }
bool NewSaves(P_char,int,int) { return save_succeeds; }
int GET_CLASS(P_char,uint cls) { return theurgist && (cls&CLASS_THEURGIST); }
int GET_PRIME_CLASS(P_char,uint) { return CLASS_WARRIOR; }
void stop_follower(P_char) {}
int setup_pet(P_char,P_char,int,int) { return 0; }
void event_pet_death(P_char,P_char,P_obj,void *) {}
int get_spell_circle(P_char,int) { return 1; }
void obj_to_char(P_obj object,P_char actor) {
    object->loc_p=LOC_CARRIED; object->loc.carrying=actor;
    object->next_content=actor->carrying; actor->carrying=object;
}
void obj_from_char(P_obj object) {
    item_actions_source_leaving(object); P_obj *entry=&object->loc.carrying->carrying;
    while(*entry && *entry!=object) entry=&(*entry)->next_content;
    assert(*entry); *entry=object->next_content; object->next_content=nullptr; object->loc_p=LOC_NOWHERE;
}
void obj_from_room(P_obj object) { item_actions_source_leaving(object); object->loc_p=LOC_NOWHERE; }
void equip_char(P_char actor,P_obj object,int slot,int) {
    actor->equipment[slot]=object; object->loc_p=LOC_WORN; object->loc.wearing=actor; worn_slot=slot;
    if(mutation==7) discard_character(actor);
}
// INSERT_AFFECTS
time_t native_test_time(time_t *) { return wall; }
float get_property(const char *key, double fallback) { return get_property(key, fallback, false); }
int get_property(const char *key, int fallback) { return get_property(key, static_cast<double>(fallback), false); }
int number(int low, int high) {
    assert(!choices.empty()); int result=choices.front(); choices.pop_front();
    assert(result>=low && result<=high); return result;
}
bool artifact_mana_publish(int, const artifact_mana_profile &profile) {
    mana_profile=profile; return artifact_mana_valid(profile);
}
bool artifact_mana_debit(P_obj source, uint64_t cost, bool passive, uint64_t token) {
    ++debits; assert(item_actions_pending());
    if(!storage_ready || !reserves.contains(source->obj_uid) || tokens[source->obj_uid]>=token) return false;
    if(!artifact_mana_spend(reserves[source->obj_uid],mana_profile,wall,cost,passive)) return false;
    tokens[source->obj_uid]=token; return true;
}
void hummer(P_obj) { ++hums; }
void spell_vitality(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(SPELL_VITALITY,p,a,s,t,v,o); }
int spell_damage(P_char actor,P_char target,double amount,int type,uint flags,damage_messages *,int *) {
    assert(flags&SPLDAM_NODEFLECT); assert(amount==73 && type==SPELL_FIREBALL);
    ++reflected; reflected_target=target->runtime_id;
    if(reflect_mutation==1) discard_character(target);
    if(reflect_mutation==2) discard_character(actor);
    if(reflect_mutation==3) { properties["itemActions.enabled"]=0; update_item_action_properties(); }
    return 0;
}
#define time native_test_time
#include "item/native_artifact_actions.c"
namespace artifact_control {
bool control_enabled(int) { return true; }
bool control_allows_variant(int, holder_kind, const char *) { return true; }
bool control_power_enabled(int, const char *) { return true; }
int control_power_level(int, const char *, int fallback) { return fallback; }
}
#include "item/tsunami_actions.c"
#include "item/ioun_actions.c"
#include "item/necroplasm_actions.c"
// INSERT_CALLBACKS
#undef time

struct native_scene : scene {
    native_scene(int vnum=31514, bool enabled=true) {
        native_configs.clear(); calls.clear(); output.clear(); reserves.clear(); tokens.clear(); choices.clear();
        debits=hums=reflected=reflect_mutation=mutation=form_spells=off_messages=0;
        storage_ready=legal=visible=bound_owner=prime_allowed=save_succeeds=true; theurgist=false; worn_slot=-1;
        link_types[LNK_CHAR_OBJ_AFF].flags=LNKFLG_OBJECT|LNKFLG_REMOVE_AFF|LNKFLG_BREAK_REMOVE|LNKFLG_SHOW_REMOVE_MSG;
        wall=1000; obj_index[0].virtual_number=vnum;
        source->name=const_cast<char *>("tsunami"); source->short_description=const_cast<char *>("Tsunami");
        world[0].room_flags=0; world[0].sector_type=SECT_INSIDE; world[0].people=actor; actor->next_in_room=target;
        actor->player.level=50;
        auto prefix="itemActions.artifact."+std::to_string(vnum)+".";
        properties[prefix+"enabled"]=enabled;
        properties[prefix+"manaCost"]=30; properties[prefix+"manaCapacity"]=100;
        update_native_artifact_properties();
        mana_profile={static_cast<uint64_t>(vnum),1,100,0,0};
        auto row=artifact_mana_empty(source->obj_uid,mana_profile,wall); row.reserve=100;
        reserves[source->obj_uid]=row;
    }
    ~native_scene() {
        item_actions_reload();
        for(P_char actor=character_list;actor;actor=actor->next) {
            while(actor->affected) affect_remove(actor,actor->affected);
            assert(!actor->obj_linked);
        }
        assert(!native_link_pool.objs_used);
        world[0].people=nullptr;
    }
    int tsunami(int cmd) { char argument[]="tsunami"; return SeaKingdom_Tsunami(source,actor,cmd,argument); }
    int mirror() {
        proc_data data{target,73,SPELL_FIREBALL,0,nullptr};
        return deflect_ioun(source,actor,CMD_GOTNUKED,reinterpret_cast<char *>(&data));
    }
};

static void tsunami_cases() {
    for(bool enabled:{false,true}) {
        native_scene s(31514,enabled); assert(s.tsunami(CMD_TAP));
        if(enabled) { assert(calls.empty() && item_action_active(s.actor)); assert(reserves[100].reserve==70); advance(); }
        assert(calls.size()==1 && calls[0].id==SPELL_VITALITY && calls[0].target==s.actor->runtime_id);
        assert(s.source->timer[0]==wall); const int paid=debits; s.tsunami(CMD_TAP); advance(); assert(debits==paid);
    }
    for(int interruption=0;interruption<7;++interruption) {
        native_scene s; assert(s.tsunami(CMD_TAP));
        if(interruption==0) { depart(s.actor); s.actor->in_room=0; }
        if(interruption==1) item_actions_source_leaving(s.source);
        if(interruption==2) abort_item_action(s.actor);
        if(interruption==3) { properties["itemActions.artifact.31514.enabled"]=0; update_native_artifact_properties(); }
        if(interruption==4) { SET_POS(s.actor,STAT_SLEEPING+POS_PRONE); }
        if(interruption==5) world[0].room_flags=ROOM_NO_MAGIC;
        if(interruption==6) { SET_BIT(s.source->extra2_flags,ITEM2_ACCOUNT_BOUND); bound_owner=false; }
        advance(); assert(calls.empty() && reserves[100].reserve==70 && s.source->timer[0]==wall);
    }
    for(int fail=0;fail<5;++fail) {
        native_scene s;
        if(fail==0) reserves[100].reserve=29;
        if(fail==1) storage_ready=false;
        if(fail==2) properties["itemActions.artifact.31514.manaCost"]=0;
        if(fail==3) SET_BIT(s.actor->specials.affected_by2,AFF2_CASTING);
        if(fail==4) s.source->timer[0]=wall+500;
        const auto before=reserves[100].reserve; s.tsunami(CMD_TAP); advance();
        assert(calls.empty() && reserves[100].reserve==before);
    }
    { // Exact-last-charge admission and no typed combat payload parsed as a string.
        native_scene s; reserves[100].reserve=30;
        assert(!SeaKingdom_Tsunami(s.source,s.actor,CMD_MELEE_HIT,reinterpret_cast<char *>(s.target)));
        s.tsunami(CMD_TAP); advance(); assert(reserves[100].reserve==0 && calls.size()==1);
    }
    for(int terrain:{SECT_INSIDE,SECT_WATER_SWIM,SECT_NO_GROUND}) {
        native_scene s; world[0].sector_type=terrain;
        const int cmd=terrain==SECT_INSIDE?CMD_THRUST:CMD_RAISE;
        assert(s.tsunami(cmd)); assert(GET_POS(s.target)!=POS_PRONE);
        choices.push_back(terrain==SECT_NO_GROUND?0:1); advance();
        assert(GET_POS(s.target)==POS_PRONE && calls.empty() && choices.empty());
    }
    { native_scene s; s.tsunami(CMD_RAISE); assert(!item_actions_pending() && !debits); }
    { native_scene s; s.tsunami(CMD_THRUST); depart(s.target); advance(); assert(choices.empty()); }
    { // Existing vitality on a non-group bystander is no longer refreshed.
        native_scene s; affected_type af{}; af.type=SPELL_VITALITY; af.duration=2;
        s.target->affected=&af; s.tsunami(CMD_TAP); advance(); assert(af.duration==2); s.target->affected=nullptr;
    }
    { // Removing a participant in the first native spell stops/reacquires the group loop.
        native_scene s; s.actor->group=s.target->group=reinterpret_cast<group_list *>(1);
        mutation=5; s.tsunami(CMD_TAP); advance(); assert(calls.size()==1);
        s.actor->group=s.target->group=nullptr;
    }
}

static void mirror_cases() {
    for(int change=0;change<8;++change) {
        native_scene s(922); choices={0}; // Original one-in-four selection, before migration hook.
        if(change==1) reserves[100].reserve=29;
        if(change==2) storage_ready=false;
        if(change==3) legal=false;
        if(change==4) properties["itemActions.artifact.922.passiveFloor"]=80;
        if(change==5) reflect_mutation=1;
        if(change==6) reflect_mutation=2;
        if(change==7) reflect_mutation=3;
        if(change!=3) choices.push_back(0);
        const bool success=s.mirror();
        assert(success==(change==0 || change>=5));
        assert(reflected==success && item_actions_pending()==0 && ne_event_counter==0);
        assert(reserves[100].reserve==(success?70:change==1?29:100));
        if(success) assert(reflected_target!=0);
    }
    { native_scene s(922); choices={1}; assert(!s.mirror() && !debits && !reflected); }
    { native_scene s(922); choices={0,0}; reserves[100].reserve=30; assert(s.mirror() && reserves[100].reserve==0); }
    { native_scene s(922); choices={0,0}; assert(s.start()==item_action_start::scheduled);
      assert(!s.mirror() && !debits && !reflected); }
    { native_scene s(922); proc_data data{s.target,73,SPELL_FIREBALL,SPLDAM_NODEFLECT,nullptr};
      assert(native_artifact_owns(922)); assert(!intercept_mirrored_ioun(s.source,s.actor,data)); assert(!debits); }
    { // Legacy route still reflects without any new mana debit, in a nonzero room.
        native_scene s(922,false); s.actor->in_room=s.target->in_room=1;
        world[1].people=s.actor; choices={0,0}; assert(s.mirror() && !debits && reflected==1);
        world[1].people=nullptr;
    }
}

static void necroplasm_cases() {
    for(bool enabled:{false,true}) {
        native_scene s(67243,enabled); choices={1,1};
        living_necroplasm(s.source,s.actor,CMD_PERIODIC,nullptr);
        if(enabled) { assert(!form_spells && debits==1); advance(); }
        assert(form_spells==2 && affected_by_spell(s.actor,SPELL_VAMPIRE));
        assert(bool(s.actor->obj_linked)==enabled);
        if(enabled) {
            for(auto *af=s.actor->affected;af;af=af->next) assert(af->flags&AFFTYPE_NOSAVE);
            const auto uid=s.source->obj_uid; auto *object=unequip_char(s.actor,WIELD,true);
            equip_char(s.actor,object,WIELD,0); assert(s.actor->obj_linked && object->obj_uid==uid && reserves[uid].reserve==70);
            object=unequip_char(s.actor,WIELD,false); obj_to_char(object,s.actor);
            assert(!s.actor->obj_linked && !affected_by_spell(s.actor,SPELL_VAMPIRE) && off_messages==2);
        }
    }
    for(int failure=0;failure<7;++failure) {
        native_scene s(67243); assert(native_artifact_owns(67243));
        if(failure==0) reserves[100].reserve=29;
        if(failure==1) prime_allowed=false;
        if(failure==2) s.actor->equipment[HOLD]=s.source;
        if(failure==3) {
            affected_type af{}; af.type=SPELL_VAMPIRE; affect_to_char(s.actor,&af); form_spells=0;
        }
        if(failure==4) storage_ready=false;
        if(failure==5) world[0].room_flags=ROOM_NO_MAGIC;
        if(failure==6) { s.target->equipment[WIELD]=nullptr; s.actor->equipment[WEAR_BODY]=new obj_data{};
            auto *other=s.actor->equipment[WEAR_BODY]; other->extra_flags=ITEM_ARTIFACT; other->next=object_list; object_list=other; }
        begin_necroplasm_form(s.source,s.actor); advance(); assert(!form_spells);
    }
    for(int transition=0;transition<5;++transition) {
        native_scene s(67243); begin_necroplasm_form(s.source,s.actor);
        if(transition==0) item_actions_source_leaving(s.source);
        if(transition==1) { properties["itemActions.artifact.67243.enabled"]=0; update_native_artifact_properties(); }
        if(transition==2) mutation=6;
        if(transition==3) { theurgist=true; }
        advance();
        if(transition<3) assert(!s.actor->obj_linked && !affected_by_spell(s.actor,SPELL_VAMPIRE));
        else {
            assert(affected_by_spell(s.actor,transition==3?SPELL_ANGELIC_COUNTENANCE:SPELL_VAMPIRE));
            properties["itemActions.enabled"]=0; update_item_action_properties(); update_native_artifact_properties();
            assert(!s.actor->obj_linked && !affected_by_spell(s.actor,SPELL_VAMPIRE) &&
                   !affected_by_spell(s.actor,SPELL_ANGELIC_COUNTENANCE));
        }
        assert(reserves[100].reserve==70);
    }
    for(int race:{RACE_HUMAN,RACE_CENTAUR,RACE_DRIDER}) {
        native_scene s(67243); s.actor->player.race=race;
        obj_to_char(unequip_char(s.actor,WIELD,false),s.actor);
        assert(living_necroplasm(s.source,s.actor,CMD_PERIODIC,nullptr));
        assert(worn_slot==(race==RACE_CENTAUR?WEAR_HORSE_BODY:race==RACE_DRIDER?WEAR_SPIDER_BODY:WEAR_BODY));
        assert(s.source->obj_uid==100 && !debits); // Equipment stats/custody are outside resource gating.
    }
    { native_scene s(67243); obj_to_char(unequip_char(s.actor,WIELD,false),s.actor);
      mutation=7; assert(living_necroplasm(s.source,s.actor,CMD_PERIODIC,nullptr)); }
    { // Actual clear_links middle removal: message before freeing affect; honor break flag.
        native_scene s(67243); begin_necroplasm_form(s.source,s.actor); advance();
        affected_type af{}; af.type=SPELL_CURSE; af.flags=AFFTYPE_LINKED_OBJ;
        auto *unrelated=affect_to_char(s.actor,&af);
        link_char_obj_with_affect(s.actor,s.source,LNK_CEGILUNE,unrelated);
        link_types[LNK_CEGILUNE].flags=LNKFLG_OBJECT|LNKFLG_REMOVE_AFF; // No break-on-remove flag.
        clear_links(s.actor,s.source,LNKFLG_BREAK_REMOVE);
        assert(!affected_by_spell(s.actor,SPELL_VAMPIRE) && affected_by_spell(s.actor,SPELL_CURSE));
        assert(off_messages==2 && s.actor->obj_linked && !s.actor->obj_linked->next);
    }
}

int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool;
    fake_clock_ns=1000000000ULL; ne_events();
    tsunami_cases(); mirror_cases(); necroplasm_cases();
    std::puts("Native artifacts: Tsunami, ioun and necroplasm parity, mana, terrain, legality, linked forms, cancellation and extraction passed");
}
'''

callbacks = '\n'.join(function(ROOT / 'src/specs/specs.underworld.c', signature) for signature in (
    'void event_tsunamiwave(', 'int SeaKingdom_Tsunami(', 'int deflect_ioun('))
callbacks += '\n' + function(ROOT / 'src/specs/specs.object.c', 'int living_necroplasm(')
callbacks += '\n' + function(ROOT / 'src/magic/magic.c', 'void spell_vampire(')
affects = '\n'.join(function(ROOT / 'src/magic/affects.c', signature) for signature in (
    'void clear_links(P_char ch, P_obj obj, int flag)',
    'void unlink_char_obj_affect(P_char ch, struct affected_type *af)'))
with tempfile.TemporaryDirectory(prefix='duris-native-artifacts-') as directory:
    source = Path(directory) / 'harness.cpp'; binary = Path(directory) / 'harness'
    source.write_text(platform + fixture + boundary + HARNESS.replace('// INSERT_CALLBACKS', callbacks).replace('// INSERT_AFFECTS', affects))
    subprocess.run(['g++', '-std=c++20', '-O1', '-g', '-ffunction-sections', '-fdata-sections',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-pthread',
                    '-I' + str(ROOT / 'src'), str(source), str(ROOT / 'src/persistence/latency_trace.c'),
                    '-Wl,--gc-sections', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=dict(os.environ,
                   ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1',
                   DURIS_NEVENT_ANALYTICS='0', DURIS_NEVENT_BUDGET_USEC='0',
                   DURIS_NEVENT_MAX_CALLBACKS='0', DURIS_NEVENT_PLAYER_PRIORITY='1'))
