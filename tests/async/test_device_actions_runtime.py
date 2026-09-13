#!/usr/bin/env python3
"""Real use/recite commands, device adapters and scheduler under ASan/UBSan."""
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
commands = "\n".join([
    function(ROOT / "src/core/utility.c", "void cast_as_area("),
    function(ROOT / "src/cmd/actoth.c", "void do_use("),
    function(ROOT / "src/cmd/actoth.c", "void do_recite("),
])

HARNESS = r'''
#include "item/device_actions.c"
void update_wonder_action_properties() {}
#include "sql/sql.h"
Skill skills[MAX_SKILLS] = {};
static std::vector<std::string> output;
struct spell_call {
    int id, power, type;
    uint64_t actor, target, object;
    std::string arguments;
};
static std::vector<spell_call> calls;
static int parser_calls = 0, mutation = 0, extracts = 0, appearances = 0, law_calls = 0;
static bool visible = true, silent = false, legal = true, bound_owner = true, parse_ok = true;
static P_char chosen_char;
static P_obj chosen_obj;
void send_to_char(const char *text, P_char) { output.emplace_back(text); }
void send_to_room(const char *text, int) { output.emplace_back(text); }
void act(const char *text, int, P_char, P_obj, void *, int) { output.emplace_back(text); }
bool ac_can_see(P_char, P_char, bool) { return visible; }
bool ac_can_see_obj(P_char, P_obj, int) { return visible; }
bool is_silent(P_char, bool) { return silent; }
bool is_in_safe(P_char actor) { return IS_ROOM(actor->in_room, ROOM_SAFE); }
bool account_bound_reward_owner(P_char, P_obj) { return bound_owner; }
bool should_not_kill(P_char, P_char) { ++law_calls; return !legal; }
int CanDoFightMove(P_char, P_char) { return legal; }
bool should_area_hit(P_char, P_char) { return true; }
bool AdjacentInRoom(P_char, P_char) { return true; }
bool is_linked_to(P_char, P_char, unsigned short) { return false; }
bool affected_by_spell(P_char, int) { return false; }
int char_in_list(const P_char actor) {
    for (auto p = character_list; p; p = p->next) if (p == actor) return true;
    return false;
}
bool isname(const char *a, const char *b) { return a && b && !strcmp(a,b); }
void appear(P_char, bool) { ++appearances; }
void wizlog(int, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
char *one_argument(const char *text, char *first) {
    while (*text == ' ') ++text;
    while (*text && *text != ' ') *first++ = *text++;
    *first = 0; while (*text == ' ') ++text;
    return const_cast<char *>(text);
}
P_obj get_obj_in_list_vis(P_char, const char *name, P_obj list, bool) {
    for (; list; list = list->next_content) if (isname(name,list->name)) return list;
    return nullptr;
}
P_obj get_object_in_equip_vis(P_char actor, char *name, int *slot) {
    for (int i=0;i<MAX_WEAR;++i) if (actor->equipment[i] && isname(name,actor->equipment[i]->name)) {
        *slot=i; return actor->equipment[i];
    }
    return nullptr;
}
P_obj unequip_char(P_char actor, int slot, bool) {
    auto source = actor->equipment[slot]; item_actions_source_leaving(source);
    actor->equipment[slot] = nullptr; source->loc_p = LOC_NOWHERE; return source;
}
void extract_obj(P_obj source, int) {
    ++extracts; item_actions_source_leaving(source);
    for (P_char actor=character_list;actor;actor=actor->next) {
        P_obj *entry=&actor->carrying;
        while (*entry && *entry!=source) entry=&(*entry)->next_content;
        if (*entry) *entry=source->next_content;
        for (int i=0;i<MAX_WEAR;++i) if(actor->equipment[i]==source) actor->equipment[i]=nullptr;
    }
    remove_source(source, character_list);
}
bool parse_spell_arguments(P_char actor, spell_target_data *data, char *arg) {
    ++parser_calls; data->arg=arg; data->t_obj=nullptr; data->t_char=nullptr;
    if (!parse_ok) return false;
    if (data->ttype==2) data->t_char=actor;
    else if (data->ttype==3) data->t_obj=chosen_obj;
    else if (!IS_SET(skills[data->ttype].targets,TAR_IGNORE|TAR_AREA)) data->t_char=chosen_char;
    return true;
}
void discard_character(P_char target) {
    P_char *entry=&world[0].people;
    while(*entry && *entry!=target) entry=&(*entry)->next_in_room;
    if (*entry) *entry=target->next_in_room;
    remove_character(target);
}
void invoke(int id,int power,P_char actor,char *arg,int type,P_char target,P_obj object) {
    calls.push_back({id,power,type,actor?actor->runtime_id:0,target?target->runtime_id:0,
                     object?object->obj_uid:0,arg?arg:""});
    if (mutation==1 && target) depart(target);
    if (mutation==2 && target) discard_character(target);
    if (mutation==3 && object) extract_obj(object);
    if (mutation==4) { properties["itemActions.enabled"]=0; update_item_action_properties(); }
    if (mutation==5) depart(actor);
}
void first_spell(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(1,p,a,s,t,v,o); }
void second_spell(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(2,p,a,s,t,v,o); }
void third_spell(int p,P_char a,char *s,int t,P_char v,P_obj o) { invoke(3,p,a,s,t,v,o); }
// INSERT_COMMANDS

struct device_scene : scene {
    P_obj target_object = new obj_data{};
    explicit device_scene(int type=ITEM_WAND, bool enabled=true) {
        assert(consumed_scrolls.empty()); device_bindings.clear(); calls.clear(); output.clear();
        parser_calls=mutation=extracts=appearances=law_calls=0;
        visible=legal=bound_owner=parse_ok=true; silent=false;
        world[0].room_flags=0; world[0].people=actor; actor->next_in_room=target;
        chosen_char=target; chosen_obj=target_object;
        target_object->obj_uid=200; target_object->loc_p=LOC_CARRIED; target_object->loc.carrying=actor;
        actor->carrying=target_object; target_object->next=object_list; object_list=target_object;
        source->type=type; source->name=const_cast<char *>("device"); source->short_description=const_cast<char *>("a test device");
        source->value[0]=17; source->value[1]=10; source->value[2]=1; source->value[3]=1;
        obj_index[0].virtual_number=777;
        properties["itemActions.wands.enabled"]=enabled;
        properties["itemActions.staves.enabled"]=enabled;
        properties["itemActions.scrolls.enabled"]=enabled;
        skills[1].spell_pointer=first_spell; skills[1].targets=TAR_AGGRO|TAR_CHAR_ROOM;
        skills[2].spell_pointer=second_spell; skills[2].targets=TAR_SELF_ONLY;
        skills[3].spell_pointer=third_spell; skills[3].targets=TAR_OBJ_INV;
        if(type==ITEM_SCROLL) {
            actor->equipment[WIELD]=nullptr; source->loc_p=LOC_CARRIED; source->loc.carrying=actor;
            source->next_content=actor->carrying; actor->carrying=source;
            source->value[1]=1; source->value[2]=2; source->value[3]=3;
        }
    }
    ~device_scene() { item_actions_reload(); device_actions_pulse(); assert(consumed_scrolls.empty()); world[0].people=nullptr; }
    void use() { char arg[]="device original words"; do_use(actor,arg,CMD_USE); }
    void recite() { char arg[]="device original words"; do_recite(actor,arg,CMD_RECITE); }
};

static void wand_parity() {
    {
        device_scene s; skills[1].targets |= TAR_CHAR_RANGE;
        s.use(); advance(); assert(calls.size()==1); // Ranged-capable spell, local target.
    }
    {
        device_scene s; skills[1].targets |= TAR_CHAR_RANGE; s.target->in_room=1;
        s.use(); advance(); assert(calls.empty() && s.source->value[2]==1);
    }
    for(bool enabled:{false,true}) {
        device_scene s(ITEM_WAND,enabled); const int wait=unrelated_wait_changes;
        s.use(); assert(s.source->value[2]==0 && parser_calls==1);
        if(enabled) {
            assert(item_action_active(s.actor) && calls.empty());
            s.use(); assert(calls.empty() && parser_calls==1); // Empty charge no second parse/cost.
            s.source->value[0]=99; s.source->value[3]=2; chosen_char=s.actor;
            advance(20,false); assert(calls.empty()); advance();
            assert(unrelated_wait_changes==wait);
        } else assert(unrelated_wait_changes==wait+1);
        assert(calls.size()==1 && calls[0].power==17 && calls[0].type==SPELL_TYPE_SPELL);
        assert(calls[0].target==s.target->runtime_id && calls[0].object==0 && calls[0].arguments.empty());
    }
    for(int failure=0;failure<8;++failure) {
        device_scene s;
        if(failure==0) s.source->value[2]=0;
        if(failure==1) s.source->value[3]=MAX_SKILLS;
        if(failure==2) parse_ok=false;
        if(failure==3) legal=false;
        if(failure==4) { SET_BIT(s.source->extra2_flags,ITEM2_ACCOUNT_BOUND); bound_owner=false; }
        if(failure==5) SET_BIT(s.actor->specials.affected_by2,AFF2_CASTING);
        if(failure==6) SET_POS(s.actor,STAT_SLEEPING+POS_PRONE);
        if(failure==7) properties["itemActions.wands.enabled"]=1.5;
        const int charges=s.source->value[2]; s.use(); advance();
        assert(calls.empty() && !item_actions_pending() && s.source->value[2]==charges);
    }
    {
        device_scene s; const auto sequence=ne_event_sequence; ne_event_sequence=ULLONG_MAX;
        s.use(); ne_event_sequence=sequence;
        assert(!item_actions_pending() && s.source->value[2]==1 && calls.empty());
    }
    for(int change=0;change<7;++change) {
        device_scene s; s.use(); assert(s.source->value[2]==0);
        switch(change) {
        case 0: depart(s.target); s.target->in_room=0; break;
        case 1: item_actions_source_leaving(s.source); break;
        case 2: visible=false; break;
        case 3: SET_POS(s.actor,STAT_SLEEPING+POS_PRONE); break;
        case 4: legal=false; break;
        case 5: { char empty[]=""; do_abort(s.actor,empty,CMD_ABORT); break; }
        case 6: properties["itemActions.device.777.enabled"]=0; update_device_action_properties(); break;
        }
        advance(); assert(calls.empty() && s.source->value[2]==0 && !item_action_active(s.actor));
    }
    {
        device_scene s; silent=true; s.use(); advance(); assert(calls.size()==1); // Wands are not verbal.
    }
    {
        device_scene s; s.source->value[3]=3; s.use(); assert(calls.empty());
        item_actions_source_leaving(s.target_object); // Object away/return cannot revive the invocation.
        advance(); assert(calls.empty() && s.source->value[2]==0);
    }
    {
        device_scene s; s.source->value[3]=3; s.use(); advance();
        assert(calls.size()==1 && calls[0].object==200 && !calls[0].target);
    }
    {
        device_scene s; skills[1].targets=TAR_IGNORE; s.use(); advance();
        assert(calls.size()==1 && !calls[0].target && calls[0].arguments=="original words");
    }
}

static void scroll_consumption() {
    for(bool enabled:{false,true}) {
        device_scene s(ITEM_SCROLL,enabled); s.recite();
        if(enabled) {
            assert(calls.empty() && !extracts && s.source->value[1]==0 && s.source->value[2]==0 && s.source->value[3]==0);
            advance(); assert(extracts==0); device_actions_pulse();
        }
        assert(extracts==1 && calls.size()==3);
        for(int i=0;i<3;++i) assert(calls[i].id==i+1 && calls[i].power==17 && calls[i].type==SPELL_TYPE_SPELL);
        assert(calls[0].target==s.target->runtime_id && calls[1].target==s.actor->runtime_id && calls[2].object==200);
        assert(calls[0].arguments=="original words" && calls[1].arguments=="original words");
    }
    for(int change=0;change<5;++change) {
        device_scene s(ITEM_SCROLL); s.recite();
        switch(change) {
        case 0: { char empty[]=""; do_abort(s.actor,empty,CMD_ABORT); break; }
        case 1: silent=true; break;
        case 2: item_actions_source_leaving(s.source); break;
        case 3: extract_obj(s.source); break;
        case 4: item_actions_source_leaving(s.target_object); break;
        }
        advance(); assert(calls.empty()); device_actions_pulse(); assert(extracts==1);
    }
    for(int change:{1,2,4,5}) {
        device_scene s(ITEM_SCROLL); mutation=change; s.recite(); advance();
        assert(calls.size()==1); device_actions_pulse(); assert(extracts==1);
    }
    {
        device_scene s(ITEM_SCROLL); silent=true; s.recite();
        assert(!extracts && !item_actions_pending() && s.source->value[1]==1);
    }
    {
        device_scene s(ITEM_SCROLL); parse_ok=false; s.recite();
        assert(!extracts && s.source->value[1]==1); // New admission rejects before consuming.
    }
    {
        device_scene s(ITEM_SCROLL,false); parse_ok=false; s.recite();
        assert(extracts==1 && calls.empty()); // Legacy failure still consumes.
    }
}

static void staff_area() {
    for(bool enabled:{false,true}) {
        device_scene s(ITEM_STAFF,enabled);
        s.use(); if(enabled) { assert(calls.empty()); advance(); }
        assert(calls.size()==1 && calls[0].target==s.target->runtime_id && calls[0].type==SPELL_TYPE_SPELL);
    }
    {
        device_scene s(ITEM_STAFF);
        auto ally=new char_data{}; ally->runtime_id=allocate_character_runtime_id();
        ally->only.pc=s.actor->only.pc; ally->in_room=0; SET_POS(ally,STAT_NORMAL+POS_STANDING);
        ally->next=character_list; character_list=ally;
        ally->next_in_room=world[0].people; world[0].people=ally;
        std::remove_pointer_t<decltype(s.actor->group)> group{}; s.actor->group=ally->group=&group;
        s.use(); advance(); assert(calls.size()==1 && calls[0].target==s.target->runtime_id);
    }
    {
        device_scene s(ITEM_STAFF); s.use();
        discard_character(s.target); chosen_char=nullptr;
        auto newcomer=new char_data{}; newcomer->runtime_id=allocate_character_runtime_id();
        newcomer->only.pc=s.actor->only.pc; newcomer->in_room=0; SET_POS(newcomer,STAT_NORMAL+POS_STANDING);
        newcomer->next=character_list; character_list=newcomer;
        newcomer->next_in_room=world[0].people; world[0].people=newcomer;
        advance(); assert(calls.size()==1 && calls[0].target==newcomer->runtime_id);
    }
    for(int change:{2,4,5}) {
        device_scene s(ITEM_STAFF); s.source->value[3]=2; skills[2].targets=TAR_CHAR_ROOM;
        mutation=change; s.use(); advance(); assert(calls.size()==1);
    }
    {
        device_scene s(ITEM_STAFF); s.source->value[3]=2; s.use(); advance();
        assert(calls.size()==2 && calls[0].actor==calls[0].target && calls[1].actor==calls[1].target);
    }
}
// Separate legacy hardening regression: previously the early guard observed
// initialized-null tmp_char and never checked the actual parsed PC victim.
static void legacy_wand_permission() {
    {
        device_scene s(ITEM_WAND,false); legal=false; s.use();
        assert(parser_calls==1 && law_calls==1 && calls.empty() && s.source->value[2]==0);
    }
    {
        device_scene s(ITEM_WAND,false); legal=false; parse_ok=false; s.use();
        assert(parser_calls==1 && law_calls==0 && calls.empty() && s.source->value[2]==0);
    }
    {
        device_scene s(ITEM_WAND,false); legal=false; skills[1].targets=TAR_CHAR_ROOM; s.use();
        assert(parser_calls==1 && law_calls==0 && calls.size()==1 && s.source->value[2]==0);
    }
    {
        device_scene s(ITEM_WAND,false); legal=false; chosen_char=s.actor; s.use();
        assert(law_calls==0 && calls.size()==1 && s.source->value[2]==0);
    }
}
int main() {
    nevent_bind_game_thread(); ne_dead_event_pool=&test_pool; fake_clock_ns=1000000000ULL; ne_events();
    wand_parity(); scroll_consumption(); staff_area(); legacy_wand_permission();
    std::puts("Device actions: real wrappers, charge/scroll consumption, identities, area lifetime and cancellation passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-device-actions-") as directory:
    source = Path(directory) / "harness.cpp"; binary = Path(directory) / "harness"
    source.write_text(platform + fixture + HARNESS.replace("// INSERT_COMMANDS", commands))
    subprocess.run(["g++", "-std=c++20", "-O1", "-g", "-D__NO_MYSQL__", "-ffunction-sections", "-fdata-sections",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-pthread", "-no-pie",
                    "-I" + str(ROOT / "src"), str(source), str(ROOT / "src/persistence/latency_trace.c"),
                    "-Wl,--gc-sections", "-o", str(binary)], check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1", DURIS_NEVENT_ANALYTICS="0",
                       DURIS_NEVENT_BUDGET_USEC="0", DURIS_NEVENT_MAX_CALLBACKS="0", DURIS_NEVENT_PLAYER_PRIORITY="1")
    subprocess.run([str(binary)], check=True, env=environment)
