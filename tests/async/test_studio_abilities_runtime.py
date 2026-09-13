#!/usr/bin/env python3
"""Typed Studio action, actual object bridge and shared runtime under ASan/UBSan."""
import ast
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def literal(path, name):
    return next(ast.literal_eval(n.value) for n in ast.parse(path.read_text()).body
                if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == name for t in n.targets))


def function(text, signature):
    start = text.index(signature); end = text.index("{", start)+1; depth = 1
    while depth:
        depth += (text[end] == "{")-(text[end] == "}"); end += 1
    return text[start:end]


platform = literal(ROOT / "tests/async/test_nevent_scheduler_runtime.py", "HARNESS").split("struct record_payload\n", 1)[0]
platform = platform.replace("DEFINE_LABEL_CALLBACK(event_item_action_active)", "")
fixture = literal(ROOT / "tests/async/test_item_actions_runtime.py", "HARNESS")
fixture = fixture.replace("int main() {", "void foundation_regression_main() {")
fixture = fixture.replace("void send_to_char(const char *, P_char) {}", "")
fixture = fixture.replace("void act(const char *, int, P_char, P_obj, void *, int) {}", "")
fixture = fixture.replace("// INSERT_PRODUCTION_ABORT", function((ROOT / "src/net/sparser.c").read_text(), "void do_abort(P_char ch,"))
boundary = literal(ROOT / "tests/async/test_device_actions_runtime.py", "HARNESS").split("// INSERT_COMMANDS", 1)[0]
studio = (ROOT / "src/mob/studioproc.c").read_text()
structures = studio[studio.index("struct sp_cond\n"):studio.index("#define SP_HASH")]
structures += function(studio, "struct sp_ctx\n")+";\n"
sample = json.loads((ROOT / "docs/examples/studio-item-abilities.json").read_text())
sample["abilities"] = sample["abilities"][:1]
definition = sample["abilities"][0]
definition.update(id=123, vnum=777, cooldownMs=10000, cost=1000)
definition["effects"] = [dict(type="spell", spell=1, power=17, target="victim", call="spell")]
definition["mana"].update(id=777, capacity=10000, passiveFloor=0)

HARNESS = r'''
#define clock_gettime nevent_test_clock_gettime
#include "item/studio_abilities.c"
#undef clock_gettime
#include <cjson/cJSON.h>
static uint64_t mana_reserve=1000, paid_token=0;
static int debits=0, publications=0, previous_calls=0, dispatches=0;
static bool profile_ok=true, previous_wins=false;
static uint64_t old_actor_id=0, original_actor_id=0, victim_id=0;
int real_object(int vnum) { return vnum==777?0:-1; }
P_char get_char_room_vis(P_char,const char *) { return chosen_char; }
bool artifact_mana_can_publish(int,const artifact_mana_profile &) { return profile_ok; }
bool artifact_mana_publish(int,const artifact_mana_profile &) { ++publications; return profile_ok; }
bool artifact_mana_debit(P_obj,uint64_t cost,bool,uint64_t token) {
    ++debits;
    if(mana_reserve<cost) return false;
    assert(token && token!=paid_token); paid_token=token; mana_reserve-=cost; return true;
}
// INSERT_STRUCTURES
static sp_rec record{};
static sp_trig trigger{};
static int sp_depth=0;
static bool sp_conds_pass(sp_trig *,sp_ctx *) { return true; }
int number(int low,int) { return low; }
// INSERT_EXECUTOR
static int sp_execute(sp_trig *t,sp_ctx *cx) { sp_execute_item_ability(t,cx,&t->actions[0]); return 1; }
// INSERT_FIRE
static bool sp_on_game_thread() { return true; }
static void sp_arm_hour() {}
static sp_rec *sp_find(int,int) { return &record; }
static int previous(P_obj,P_char,int,char *) { ++previous_calls; return previous_wins; }
static int sp_dispatch(sp_rec *,sp_ctx *cx,int,char *) {
    ++dispatches; old_actor_id=cx->actor?cx->actor->runtime_id:0;
    original_actor_id=cx->original_activator_id; victim_id=cx->struck_victim_id;
    sp_fire(&trigger,cx); return cx->blocked;
}
// INSERT_OBJECT_BRIDGE
static std::string document() { return R"json(INSERT_JSON)json"; }
static std::string change(const std::string &input,const char *field,cJSON *value) {
    auto root=cJSON_Parse(input.c_str()); assert(root);
    auto entry=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"abilities"),0);
    assert(cJSON_ReplaceItemInObjectCaseSensitive(entry,field,value));
    char *json=cJSON_PrintUnformatted(root); std::string output=json; cJSON_free(json); cJSON_Delete(root); return output;
}
struct studio_scene : scene {
    bool active;
    explicit studio_scene(bool use=false):active(use) {
        catalog.clear(); versions.clear(); cooldowns.clear(); ability_enabled.clear();
        calls.clear(); output.clear(); mutation=debits=publications=previous_calls=dispatches=0;
        mana_reserve=1000; paid_token=0; profile_ok=visible=legal=bound_owner=true; previous_wins=false;
        old_actor_id=original_actor_id=victim_id=0; world[0].room_flags=0;
        chosen_char=target;
        source->name=const_cast<char *>("device"); source->short_description=const_cast<char *>("a Studio device");
        obj_index[0].virtual_number=777;
        skills[1].spell_pointer=first_spell; skills[1].targets=TAR_AGGRO|TAR_CHAR_ROOM;
        properties["itemActions.studio.enabled"]=1;
        record={}; record.prev_obj=previous; trigger={}; trigger.chance=100;
        trigger.event=use?SP_EV_CMD:SP_EV_HIT; trigger.num_actions=1;
        trigger.actions[0].op=SP_A_ITEM_ABILITY; trigger.actions[0].num=123;
        std::string text=document(),error;
        if(use) { text=change(text,"trigger",cJSON_CreateString("use")); text=change(text,"mode",cJSON_CreateString("active")); }
        assert(studio_abilities_load(text,error)); assert(publications==1);
    }
    int fire() {
        char argument[]="device target";
        return studioproc_obj(source,actor,active?CMD_USE:CMD_MELEE_HIT,active?argument:reinterpret_cast<char *>(target));
    }
};
static void bridge_and_payment() {
    for(bool active:{false,true}) {
        studio_scene s(active); s.fire(); assert(debits==1 && mana_reserve==0 && calls.empty());
        assert(previous_calls==1 && dispatches==1 && original_actor_id==s.actor->runtime_id);
        assert(old_actor_id==(active?s.actor->runtime_id:s.target->runtime_id));
        assert(victim_id==(active?0:s.target->runtime_id));
        assert(item_action_active(s.actor)==active);
        const auto token=paid_token; s.fire(); assert(debits==1);
        advance(); assert(calls.size()==1 && calls[0].actor==s.actor->runtime_id && calls[0].target==s.target->runtime_id);
        assert(calls[0].type==SPELL_TYPE_SPELL && calls[0].power==17 && paid_token==token);
        mana_reserve=1000; s.fire(); assert(debits==1); // Stable cooldown persists after completion.
    }
    {
        studio_scene s(true); mana_reserve=0;
        assert(s.fire()); assert(calls.empty() && !item_actions_pending() && cooldowns.empty());
        mana_reserve=1000; assert(s.fire()); assert(item_actions_pending()==1 && mana_reserve==0);
    }
    {
        studio_scene s; previous_wins=true; assert(s.fire()); assert(!dispatches && !debits);
    }
    {
        studio_scene s(true); char unrelated[]="another target";
        assert(!studioproc_obj(s.source,s.actor,CMD_USE,unrelated)); assert(!debits);
    }
    {
        studio_scene s; properties["itemActions.studio.enabled"]=0; update_studio_ability_properties();
        s.fire(); assert(!debits && !item_actions_pending());
    }
    {
        studio_scene s; sp_depth=SP_MAX_DEPTH; s.fire(); sp_depth=0; assert(!debits);
        trigger.running=true; s.fire(); trigger.running=false; assert(!debits);
    }
}
static void reload_and_targets() {
    for(int change_kind=0;change_kind<7;++change_kind) {
        studio_scene s; s.fire();
        if(change_kind==0) { depart(s.target); s.target->in_room=0; }
        if(change_kind==1) item_actions_source_leaving(s.source);
        if(change_kind==2) legal=false;
        if(change_kind==3) { properties["itemActions.ability.123.enabled"]=0; update_studio_ability_properties(); }
        if(change_kind==4) { std::string error; assert(studio_abilities_load("{\"schemaVersion\":1,\"abilities\":[]}",error)); }
        if(change_kind==5) SET_POS(s.actor,STAT_SLEEPING+POS_PRONE);
        if(change_kind==6) visible=false;
        advance(); assert(calls.empty() && !item_actions_pending() && mana_reserve==0);
    }
    {
        studio_scene s; s.fire(); std::string error;
        auto invalid=change(document(),"windupPulses",cJSON_CreateNumber(0));
        assert(!studio_abilities_load(invalid,error) && item_actions_pending()==1);
        assert(!studio_abilities_load(change(document(),"windupPulses",cJSON_CreateNumber(9)),error));
        profile_ok=false; assert(!studio_abilities_load(document(),error)); profile_ok=true;
        skills[1].spell_pointer=nullptr; assert(!studio_abilities_load(document(),error)); skills[1].spell_pointer=first_spell;
        skills[1].targets=TAR_OBJ_ROOM; assert(!studio_abilities_load(document(),error)); skills[1].targets=TAR_AGGRO|TAR_CHAR_ROOM;
        assert(studio_abilities_load(document(),error) && item_actions_pending()==1); // Unchanged reload.
        advance(); assert(calls.size()==1 && mana_reserve==0);
    }
    {
        studio_scene s; s.fire(); std::string error;
        auto updated=change(document(),"revision",cJSON_CreateNumber(2));
        assert(studio_abilities_load(updated,error) && !item_actions_pending());
        assert(!studio_abilities_load(document(),error)); // Revision cannot regress.
        assert(cooldowns.size()==1 && mana_reserve==0);
    }
    for(const char *target:{"activator","holder","item","room"}) {
        studio_scene s; auto root=cJSON_Parse(document().c_str());
        auto entry=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"abilities"),0);
        auto effect=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(entry,"effects"),0);
        cJSON_ReplaceItemInObjectCaseSensitive(entry,"revision",cJSON_CreateNumber(2));
        cJSON_ReplaceItemInObjectCaseSensitive(effect,"target",cJSON_CreateString(target));
        cJSON_ReplaceItemInObjectCaseSensitive(effect,"call",cJSON_CreateString("wand"));
        skills[1].targets=std::string(target)=="item"?TAR_OBJ_EQUIP:std::string(target)=="room"?TAR_IGNORE:TAR_SELF_ONLY;
        char *json=cJSON_PrintUnformatted(root); std::string error;
        assert(studio_abilities_load(json,error)); cJSON_free(json); cJSON_Delete(root);
        s.fire(); advance(); assert(calls.size()==1 && calls[0].type==SPELL_TYPE_WAND);
        assert(calls[0].object==(std::string(target)=="item"?100:0));
        assert(calls[0].target==((std::string(target)=="activator" || std::string(target)=="holder")?s.actor->runtime_id:0));
    }
}
static void multi_effect_lifetime() {
    for(int transition:{1,2,4,5}) {
        studio_scene s;
        auto root=cJSON_Parse(document().c_str());
        auto entry=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"abilities"),0);
        cJSON_ReplaceItemInObjectCaseSensitive(entry,"revision",cJSON_CreateNumber(2));
        auto effects=cJSON_GetObjectItemCaseSensitive(entry,"effects");
        cJSON_AddItemToArray(effects,cJSON_Duplicate(effects->child,1));
        cJSON_AddItemToArray(effects,cJSON_Duplicate(effects->child,1));
        char *json=cJSON_PrintUnformatted(root); std::string error;
        assert(studio_abilities_load(json,error)); cJSON_free(json); cJSON_Delete(root);
        mutation=transition; s.fire(); advance(); assert(calls.size()==1 && mana_reserve==0 && !item_actions_pending());
    }
    {
        studio_scene s; std::string error;
        auto text=change(document(),"minimumLevel",cJSON_CreateNumber(60));
        text=change(text,"revision",cJSON_CreateNumber(2)); assert(studio_abilities_load(text,error));
        s.fire(); assert(!debits && !item_actions_pending());
    }
}
static void parser() {
    studio_scene s; uint32_t id=0; std::string error;
    assert(parse_studio_ability_action(SP_T_OBJ,777,SP_EV_HIT,0,"123",id,error) && id==123);
    for(int target:{SP_T_MOB,SP_T_ROOM}) assert(!parse_studio_ability_action(target,777,SP_EV_HIT,0,"123",id,error));
    for(const char *bad:{"", "-1", "+123", "123 garbage", "999", "99999999999999999999"})
        assert(!parse_studio_ability_action(SP_T_OBJ,777,SP_EV_HIT,0,bad,id,error));
    assert(!parse_studio_ability_action(SP_T_OBJ,778,SP_EV_HIT,0,"123",id,error));
    assert(!parse_studio_ability_action(SP_T_OBJ,777,SP_EV_DAMAGED,0,"123",id,error));
    assert(!parse_studio_ability_action(SP_T_OBJ,777,SP_EV_CMD,CMD_LOOK,"123",id,error));
}
int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool; fake_clock_ns=1000000000ULL; ne_events();
    bridge_and_payment(); reload_and_targets(); multi_effect_lifetime(); parser();
    std::puts("Studio abilities: parser, original identities, legacy precedence, payment, cooldown, reload and targets passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-studio-runtime-") as directory:
    source = Path(directory) / "harness.cpp"; binary = Path(directory) / "harness"
    harness = HARNESS.replace("// INSERT_STRUCTURES", structures)
    harness = harness.replace("// INSERT_EXECUTOR", function(studio,"static void sp_execute_item_ability("))
    harness = harness.replace("// INSERT_FIRE", function(studio,"static int sp_fire("))
    harness = harness.replace("// INSERT_OBJECT_BRIDGE", function(studio,"int studioproc_obj("))
    source.write_text(platform+fixture+boundary+harness.replace("INSERT_JSON",json.dumps(sample)))
    subprocess.run(["g++", "-std=c++20", "-O1", "-g", "-D__NO_MYSQL__", "-ffunction-sections", "-fdata-sections",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-pthread", "-no-pie",
                    "-I"+str(ROOT / "src"), str(source), str(ROOT / "src/persistence/latency_trace.c"),
                    str(ROOT / "src/item/studio_ability_model.c"), "-lcjson", "-Wl,--gc-sections", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=dict(os.environ,
        ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
        DURIS_NEVENT_ANALYTICS="0", DURIS_NEVENT_BUDGET_USEC="0", DURIS_NEVENT_MAX_CALLBACKS="0", DURIS_NEVENT_PLAYER_PRIORITY="1"))
