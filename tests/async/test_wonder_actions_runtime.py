#!/usr/bin/env python3
"""Execute the actual wonder callback and owned runtime for every native branch."""
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
fixture = literal(ROOT / "tests/async/test_item_actions_runtime.py", "HARNESS")
fixture = fixture.replace("int main() {", "void foundation_regression_main() {")
fixture = fixture.replace("void send_to_char(const char *, P_char) {}", "")
fixture = fixture.replace("void act(const char *, int, P_char, P_obj, void *, int) {}", "")
fixture = fixture.replace("// INSERT_PRODUCTION_ABORT", function(ROOT / "src/net/sparser.c", "void do_abort(P_char ch,"))
boundary = literal(ROOT / "tests/async/test_device_actions_runtime.py", "HARNESS").split("// INSERT_COMMANDS", 1)[0]
boundary = boundary.replace("void update_wonder_action_properties() {}", '#include "item/wonder_actions.c"')

HARNESS = r'''
bool native_artifact_control_enabled(int) { return true; }
bool native_artifact_variant_enabled(int, P_char, const char *) { return true; }
bool native_artifact_power_enabled(int, const char *) { return true; }
int native_artifact_power_level(int, const char *, int fallback) { return fallback; }
static int choice=1, rolls=0, native_mutation=0, placements=0, gem_damage=0, years=0;
static std::vector<int> mobile_types, gem_types;
int number(int low,int high) {
    ++rolls;
    if(low==20 && high==40) return 30;
    if(low==1 && high==20) return choice;
    if(low==10 && high==40) return 12;
    if(low==0 && high==9) return 4;
    if(low==1 && high==50) return 23;
    assert(false); return low;
}
P_char get_char_vis(P_char, const char *) { return chosen_char; }
P_char read_mobile(int vnum,int) {
    mobile_types.push_back(vnum);
    auto result=new char_data{}; result->runtime_id=allocate_character_runtime_id();
    result->in_room=NOWHERE; SET_POS(result,STAT_NORMAL+POS_STANDING);
    result->next=character_list; character_list=result;
    if(native_mutation==1) depart(chosen_char);
    return result;
}
void extract_char(P_char target) { discard_character(target); }
bool char_to_room(P_char target,int room,int) {
    target->in_room=room; ++placements;
    if(native_mutation==2) { properties["itemActions.enabled"]=0; update_item_action_properties(); }
    return true;
}
P_obj read_object(int vnum,int) {
    gem_types.push_back(vnum);
    auto result=new obj_data{}; result->obj_uid=1000+gem_types.size();
    result->next=object_list; object_list=result;
    if(native_mutation==1) depart(chosen_char);
    return result;
}
void obj_to_room(P_obj object,int room) {
    object->loc_p=LOC_ROOM; object->loc.room=room; ++placements;
    if(native_mutation==2) { properties["itemActions.enabled"]=0; update_item_action_properties(); }
}
bool damage(P_char,P_char,double amount,int type) {
    assert(type==TYPE_UNDEFINED); gem_damage=amount; return false;
}
void AgeChar(P_char actor,int amount) { years=amount; if(native_mutation==1) discard_character(actor); }
#define SPELL_STUB(name,id) void name(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(id,p,a,s,t,v,o); }
SPELL_STUB(spell_minor_paralysis,1)
SPELL_STUB(spell_cyclone,2)
SPELL_STUB(spell_lightning_bolt,3)
SPELL_STUB(spell_darkness,4)
SPELL_STUB(spell_concealment,5)
SPELL_STUB(spell_fireball,6)
// INSERT_CALLBACK
struct wonder_scene : scene {
    wonder_scene(bool enabled=true) {
        calls.clear(); output.clear(); mobile_types.clear(); gem_types.clear();
        rolls=native_mutation=placements=gem_damage=years=mutation=0;
        visible=legal=bound_owner=true; world[0].room_flags=0;
        source->type=ITEM_WAND; source->short_description=const_cast<char *>("the wand of wonder");
        source->value[2]=1; obj_index[0].virtual_number=41350;
        actor->equipment[WIELD]=nullptr; actor->equipment[HOLD]=source;
        chosen_char=target;
        properties["itemActions.wonder.enabled"]=enabled;
        for(int spell:{SPELL_MINOR_PARALYSIS,SPELL_CYCLONE,SPELL_LIGHTNING_BOLT,SPELL_FIREBALL})
            skills[spell].targets=TAR_AGGRO|TAR_CHAR_ROOM;
    }
    void use() { char argument[]="target"; assert(wand_of_wonder(source,actor,CMD_USE,argument)); }
};
static void all_branches() {
    for(choice=1;choice<=20;++choice) for(bool enabled:{false,true}) {
        wonder_scene s(enabled); auto target_id=s.target->runtime_id, actor_id=s.actor->runtime_id;
        s.use(); assert(s.source->value[2]==0);
        const auto selected_rolls=rolls;
        if(enabled) {
            assert(calls.empty() && mobile_types.empty() && gem_types.empty() && !years);
            // Empty custom callback returns false; ordinary wrapper sees no charges.
            char arg[]="target"; assert(!wand_of_wonder(s.source,s.actor,CMD_USE,arg));
            assert(rolls==selected_rolls+1); // Legacy callback's level roll is unconditional.
            const auto after_empty=rolls; advance(); assert(rolls==after_empty);
        }
        if(choice<=7) {
            assert(calls.size()==1 && calls[0].power==30 && calls[0].type==SPELL_TYPE_WAND);
            assert(calls[0].id==(choice==7?5:choice));
            assert(calls[0].target==(choice==4?0:choice==7?actor_id:target_id));
        } else assert(calls.empty());
        if(choice>=8 && choice<=11) {
            assert(mobile_types.size()==(choice==10?40:1));
            for(int type:mobile_types) assert(type==(choice<=9?5710:choice==10?12803:97514));
            assert(placements==static_cast<int>(mobile_types.size()));
        } else assert(mobile_types.empty());
        if(choice==12) {
            assert(gem_types.size()==12 && placements==12 && gem_damage==12);
            for(int gem:gem_types) assert(gem==66038);
            assert(selected_rolls==27); // level + choice + 13 bound rolls + 12 gem rolls.
        } else assert(gem_types.empty() && !gem_damage);
        assert(years==((choice==15 || choice==16)?23:0));
        assert(!item_actions_pending());
    }
}
static void interruptions() {
    for(int fail=0;fail<7;++fail) {
        wonder_scene s; choice=1;
        if(fail==0) legal=false;
        if(fail==1) { SET_BIT(s.source->extra2_flags,ITEM2_ACCOUNT_BOUND); bound_owner=false; }
        if(fail==2) SET_BIT(world[0].room_flags,ROOM_SAFE);
        if(fail==3) properties["itemActions.wonder.enabled"]=1.5;
        if(fail==4) SET_BIT(s.actor->specials.affected_by2,AFF2_CASTING);
        if(fail==5) SET_POS(s.actor,STAT_SLEEPING+POS_PRONE);
        if(fail==6) visible=false;
        s.use(); advance(); assert(calls.empty() && s.source->value[2]==1 && !item_actions_pending());
    }
    for(int change=0;change<5;++change) {
        wonder_scene s; choice=1; s.use();
        if(change==0) { depart(s.target); s.target->in_room=0; }
        if(change==1) item_actions_source_leaving(s.source);
        if(change==2) legal=false;
        if(change==3) { char empty[]=""; do_abort(s.actor,empty,CMD_ABORT); }
        if(change==4) { properties["itemActions.wonder.enabled"]=0; update_wonder_action_properties(); }
        advance(); assert(calls.empty() && !item_actions_pending() && s.source->value[2]==0);
    }
    for(int selected:{10,12}) for(int change:{1,2}) {
        wonder_scene s; choice=selected; chosen_char=s.actor; native_mutation=change;
        s.use(); advance();
        assert(mobile_types.size()+gem_types.size()==1 && placements==(change==1?0:1));
        assert(!gem_damage && !item_actions_pending());
    }
    {
        wonder_scene s; choice=15; native_mutation=1; s.use(); advance();
        assert(years==23 && !item_actions_pending()); // Aging may remove the actor.
    }
}
int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool; fake_clock_ns=1000000000ULL; ne_events();
    all_branches(); interruptions();
    std::puts("Wand of wonder: all twenty native outcomes, captured RNG, legality, consumption and lifetime passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-wonder-actions-") as directory:
    source = Path(directory) / "harness.cpp"; binary = Path(directory) / "harness"
    source.write_text(platform + fixture + boundary + HARNESS.replace(
        "// INSERT_CALLBACK", function(ROOT / "src/specs/specs.highway.c", "int wand_of_wonder(")))
    subprocess.run(["g++", "-std=c++20", "-O1", "-g", "-D__NO_MYSQL__", "-ffunction-sections", "-fdata-sections",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-pthread", "-no-pie",
                    "-I" + str(ROOT / "src"), str(source), str(ROOT / "src/persistence/latency_trace.c"),
                    "-Wl,--gc-sections", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=dict(os.environ,
        ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
        DURIS_NEVENT_ANALYTICS="0", DURIS_NEVENT_BUDGET_USEC="0", DURIS_NEVENT_MAX_CALLBACKS="0",
        DURIS_NEVENT_PLAYER_PRIORITY="1"))
