#!/usr/bin/env python3
"""Selected weapon procs against the real item runtime and scheduler, ASan/UBSan.

Compile the actual Avernus/packed/random callbacks, not copies of their routing.
Only world presentation, permission, RNG and effect endpoints are doubled here.
Artifact persistence has its own game/worker/repository tests; this fixture uses
the real resource arithmetic at the debit boundary and checks its call ordering.
"""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def literal(path, name):
    tree = ast.parse(path.read_text())
    return next(ast.literal_eval(n.value) for n in tree.body
                if isinstance(n, ast.Assign)
                and any(isinstance(t, ast.Name) and t.id == name for t in n.targets))


def function(path, signature):
    text = path.read_text()
    start = text.index(signature)
    end = text.index("{", start) + 1
    depth = 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


platform = literal(ROOT / "tests/async/test_nevent_scheduler_runtime.py", "HARNESS")
platform = platform.split("struct record_payload\n", 1)[0]
platform = platform.replace("DEFINE_LABEL_CALLBACK(event_item_action_active)", "")
fixture = literal(ROOT / "tests/async/test_item_actions_runtime.py", "HARNESS")
fixture = fixture.replace("int main() {", "void foundation_regression_main() {")
fixture = fixture.replace('void send_to_char(const char *, P_char) {}', '')
fixture = fixture.replace('void act(const char *, int, P_char, P_obj, void *, int) {}', '')
fixture = fixture.replace("// INSERT_PRODUCTION_ABORT",
                          function(ROOT / "src/net/sparser.c", "void do_abort(P_char ch,"))
callbacks = "\n".join([
    function(ROOT / "src/specs/specs.underworld.c", "void resolve_avernus_drain("),
    function(ROOT / "src/specs/specs.underworld.c", "int avernus("),
    function(ROOT / "src/combat/fight.c", "bool weapon_proc("),
    function(ROOT / "src/item/randomeq.c", "int random_eq_proc("),
])

DOUBLES = r'''
#include "item/weapon_actions.c"
#include "item/artifact_mana_model.c"
#include "combat/damage.h"
#include <deque>
Skill skills[MAX_SKILLS] = {};
random_spells spells_data[61] = {};
struct message { std::string text; int audience; };
static std::vector<message> messages;
static std::vector<std::pair<int, int>> rolls;
static std::deque<int> choices;
struct effect_call {
    int id, power, type; uint64_t target; uint flags;
    bool operator==(const effect_call &) const = default;
};
static std::vector<effect_call> effects;
static bool can_fight = true, storage_ready = true;
static int stone_calls = 0, hum_calls = 0, debit_calls = 0, mutation = 0;
static int affected_spell = 0;
static artifact_mana_profile mana_profile;
static std::map<uint64_t, artifact_mana_record> reserves;
static std::map<uint64_t, uint64_t> tokens;
static time_t wall_time = 1000;
time_t selected_test_time(time_t *) { return wall_time; }
void send_to_char(const char *text, P_char) { messages.push_back({text, -1}); }
void send_to_room(const char *text, int) { messages.push_back({text, -2}); }
void act(const char *text, int, P_char, P_obj, void *, int audience) {
    messages.push_back({text, audience});
}
int number(int low, int high) {
    rolls.emplace_back(low, high);
    assert(!choices.empty());
    const int result = choices.front(); choices.pop_front();
    assert(result >= low && result <= high); return result;
}
int CanDoFightMove(P_char, P_char) { return can_fight; }
int BOUNDED(int low, int value, int high) { return std::clamp(value, low, high); }
float BOUNDEDF(float low, float value, float high) { return std::clamp(value, low, high); }
float get_property(const char *key, double fallback) { return get_property(key, fallback, false); }
int GET_PRIME_CLASS(P_char, uint) { return 0; }
bool isname(const char *a, const char *b) { return a && b && !strcmp(a, b); }
bool affected_by_spell(P_char, int spell) { return spell == affected_spell; }
int is_char_in_room(const P_char ch, int room) { return ch && ch->in_room == room; }
void hummer(P_obj) { ++hum_calls; }
void spell_stone_skin(int, P_char, char *, int, P_char, P_obj) { ++stone_calls; }
int vamp(P_char ch, double amount, double cap) {
    effects.push_back({-1, static_cast<int>(amount), static_cast<int>(cap), ch->runtime_id, 0});
    return 0;
}
int spell_damage(P_char, P_char victim, double damage, int type, uint flags,
                 damage_messages *, int *) {
    effects.push_back({-2, static_cast<int>(damage), type, victim->runtime_id, flags}); return 0;
}
void spell_endpoint(int id, int power, int type, P_char target) {
    effects.push_back({id, power, type, target->runtime_id, 0});
    if (mutation == 1) depart(target);
    if (mutation == 2) remove_character(target);
}
void spell_one(int power, P_char, char *, int type, P_char target, P_obj) {
    spell_endpoint(1, power, type, target);
}
void spell_two(int power, P_char, char *, int type, P_char target, P_obj) {
    spell_endpoint(2, power, type, target);
}
void spell_three(int power, P_char, char *, int type, P_char target, P_obj) {
    spell_endpoint(3, power, type, target);
}
bool artifact_mana_publish(int, const artifact_mana_profile &profile) {
    mana_profile = profile; return artifact_mana_valid(profile);
}
bool artifact_mana_debit(P_obj source, uint64_t cost, bool passive, uint64_t token) {
    ++debit_calls;
    assert(item_actions_pending() > 0); // Scheduler owns the action before payment.
    if (!storage_ready || token <= tokens[source->obj_uid]) return false;
    auto found = reserves.find(source->obj_uid);
    if (found == reserves.end()) return false;
    if (!artifact_mana_spend(found->second, mana_profile, wall_time, cost, passive)) return false;
    tokens[source->obj_uid] = token; return true;
}
#define time selected_test_time
// INSERT_CALLBACKS
#undef time

struct weapon_scene : scene {
    explicit weapon_scene(const char *category = "avernus", bool enabled = true) {
        bindings.clear(); messages.clear(); effects.clear(); rolls.clear(); choices.clear();
        reserves.clear(); tokens.clear(); can_fight = storage_ready = true;
        debit_calls = mutation = affected_spell = stone_calls = hum_calls = 0;
        obj_index[0].virtual_number = 19730;
        obj_index[0].func.obj = avernus;
        GET_HIT(target) = 170; GET_MAX_HIT(actor) = 1000;
        properties[std::string("itemActions.") + category + ".enabled"] = enabled;
        skills[1].spell_pointer = spell_one; skills[1].targets = TAR_AGGRO;
        skills[2].spell_pointer = spell_two; skills[2].targets = 0;
        skills[3].spell_pointer = spell_three; skills[3].targets = TAR_AGGRO;
        spells_data[1].spell = 1; spells_data[1].self_only = false;
        spells_data[2].spell = 2; spells_data[2].self_only = true;
    }
    void paid(int reserve = 60) {
        properties["itemActions.weapon.19730.manaCost"] = 30;
        properties["itemActions.weapon.19730.manaCapacity"] = 100;
        mana_profile = {19730, 1, 100, 0, 0};
        auto row = artifact_mana_empty(source->obj_uid, mana_profile, wall_time);
        row.reserve = reserve; reserves[source->obj_uid] = row;
    }
    bool hit(int roll = 0) { choices.push_back(roll); return weapon_proc(source, actor, target); }
    void packed(bool random = false) {
        obj_index[0].func.obj = nullptr;
        source->value[5] = 3002001 + (random ? 1000000000 : 0);
        source->value[6] = 57; source->value[7] = 25;
    }
    void named() {
        obj_index[0].func.obj = random_eq_proc;
        source->value[6] = 999; source->value[7] = 1;
        source->wear_flags = ITEM_WIELD; GET_OPPONENT(actor) = target;
    }
};
static void audience_count(const char *part, size_t expected) {
    for (int audience : {TO_CHAR, TO_VICT, TO_NOTVICT}) {
        size_t count = 0;
        for (auto &m : messages)
            if (m.audience == audience && m.text.find(part) != std::string::npos) ++count;
        assert(count == expected);
    }
}

static void avernus_parity() {
    for (int hitpoints : {170, 500, -8, -9}) {
        for (bool enabled : {false, true}) {
            weapon_scene s("avernus", enabled);
            GET_HIT(s.target) = hitpoints;
            assert(s.hit());
            assert((rolls == std::vector<std::pair<int,int>>{{0,24}}));
            if (enabled) {
                assert(effects.empty()); audience_count("begins", 1);
                advance(20, false); assert(effects.empty()); audience_count("intensifies", 0);
                advance(5); assert(effects.empty()); audience_count("intensifies", 1);
                GET_HIT(s.target) = 1000; GET_MAX_HIT(s.actor) = 2;
                GET_OPPONENT(s.actor) = s.actor; // Completion cannot retarget/recompute.
                advance(8);
            }
            const int damage = std::clamp(hitpoints + 9, 0, 200);
            assert(effects.size() == 2);
            assert((effects[0] == effect_call{-1, damage / 2, 1100, s.actor->runtime_id, 0}));
            assert((effects[1] == effect_call{-2, damage, SPLDAM_NEGATIVE, s.target->runtime_id,
                SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | PHSDAM_NOREDUCE}));
            assert(choices.empty() && rolls.size() == 1);
        }
    }
    weapon_scene s;
    assert(!s.hit(24) && effects.empty() && !item_actions_pending());
    assert(avernus(s.source, nullptr, CMD_SET_PERIODIC, nullptr));
    assert(avernus(s.source, nullptr, CMD_PERIODIC, nullptr));
    assert(hum_calls == 1);
    char stone[] = "stone";
    assert(avernus(s.source, s.actor, CMD_SAY, stone));
    assert(stone_calls == 1 && s.source->timer[0] == wall_time);
    choices.push_back(0);
    // Same direct callback invocation used by do_backstab, bypassing weapon_proc.
    assert(obj_index[0].func.obj(s.source, s.actor, CMD_MELEE_HIT, reinterpret_cast<char *>(s.target)));
    assert(effects.empty()); advance(); assert(effects.size() == 2);
}

static void cancellation_and_payment() {
    for (int transition = 0; transition < 8; ++transition) {
        weapon_scene s; s.paid();
        assert(s.hit() && effects.empty());
        assert(debit_calls == 1 && reserves[100].reserve == 30);
        assert(s.hit()); assert(debit_calls == 1); // Busy selection never falls back.
        switch (transition) {
        case 0: depart(s.target); s.target->in_room = 0; break;
        case 1: depart(s.actor); s.actor->in_room = 0; break;
        case 2: SET_POS(s.target, STAT_DEAD); break;
        case 3: remove_character(s.target); break;
        case 4: remove_source(s.source, s.actor); break;
        case 5: can_fight = false; break;
        case 6: properties["itemActions.weapon.19730.enabled"] = 0; update_weapon_action_properties(); break;
        case 7: properties["itemActions.enabled"] = 0; update_item_action_properties(); break;
        }
        advance();
        assert(effects.empty() && !item_actions_pending() && reserves[100].reserve == 30);
        assert(!messages.empty());
        bool fizzle = false;
        for (const auto &m : messages) fizzle |= m.text.find("fades") != std::string::npos;
        assert(fizzle);
    }
    {
        weapon_scene s; s.paid(30);
        const auto sequence = ne_event_sequence; ne_event_sequence = ULLONG_MAX;
        assert(s.hit()); ne_event_sequence = sequence;
        assert(debit_calls == 0 && reserves[100].reserve == 30 && effects.empty());
        assert(s.hit()); advance(); assert(effects.size() == 2 && reserves[100].reserve == 0);
        assert(s.hit()); advance(); assert(effects.size() == 2 && debit_calls == 2);
        properties["itemActions.avernus.enabled"] = 0; update_weapon_action_properties();
        assert(s.hit() && effects.size() == 4); // Explicit rollback restores legacy.
    }
    {
        weapon_scene s; s.paid(); storage_ready = false;
        assert(s.hit()); advance();
        assert(effects.empty() && messages.empty() && reserves[100].reserve == 60);
    }
    {
        weapon_scene s;
        SET_BIT(s.actor->specials.affected_by2, AFF2_CASTING);
        const auto wait_changes = unrelated_wait_changes;
        assert(s.hit());
        auto other = new obj_data{};
        other->R_num = s.source->R_num; other->loc_p = LOC_WORN; other->loc.wearing = s.actor;
        other->obj_uid = 101; other->next = object_list;
        object_list = other; s.actor->equipment[WIELD2] = other;
        choices.push_back(0); assert(weapon_proc(other, s.actor, s.target));
        assert(item_actions_pending() == 2 && !item_action_active(s.actor));
        advance();
        assert(effects.size() == 4 && IS_AFFECTED2(s.actor, AFF2_CASTING));
        assert(wait_changes == unrelated_wait_changes);
    }
}

static void packed_parity() {
    for (bool enabled : {false, true}) {
        weapon_scene s("weapons", enabled); s.packed();
        assert(s.hit());
        if (enabled) { assert(effects.empty()); advance(); }
        assert((effects == std::vector<effect_call>{
            {3,57,SPELL_TYPE_SPELL,s.target->runtime_id,0},
            {2,57,SPELL_TYPE_SPELL,s.actor->runtime_id,0},
            {1,57,SPELL_TYPE_SPELL,s.target->runtime_id,0}}));
        assert(rolls.size() == 1 && choices.empty());
    }
    for (int choice = 0; choice < 3; ++choice) {
        weapon_scene s("weapons"); s.packed(true);
        choices = {0, choice}; assert(weapon_proc(s.source, s.actor, s.target));
        s.source->value[5] = 3; s.source->value[6] = 999;
        assert(effects.empty()); advance();
        assert(effects.size() == 1 && effects[0].id == choice + 1 && effects[0].power == 57);
        assert(effects[0].target == (choice == 1 ? s.actor : s.target)->runtime_id);
        assert((rolls == std::vector<std::pair<int,int>>{{0,24},{0,2}}));
    }
    {
        weapon_scene s("weapons"); s.packed(); s.source->value[5] = 2002;
        assert(s.hit() && effects.size() == 2 && !item_actions_pending()); // Pure buffs stay instant.
    }
    for (bool enabled : {false, true}) {
        weapon_scene s("weapons", enabled); s.packed(); s.source->value[5] = 3000001;
        assert(s.hit()); advance();
        assert(effects.size() == 1 && effects[0].id == 1); // Preserve legacy sparse-slot/count quirk.
    }
    {
        weapon_scene s("weapons"); s.packed(); affected_spell = 2;
        assert(s.hit()); advance(); assert(effects.size() == 2 && effects[0].id == 3 && effects[1].id == 1);
    }
    for (int change : {1,2}) {
        weapon_scene s("weapons"); s.packed(); mutation = change;
        assert(s.hit()); advance(); assert(effects.size() == 1 && !item_actions_pending());
    }
}

static void random_parity() {
    for (bool enabled : {false, true}) {
        weapon_scene s("randomWeapons", enabled); s.named();
        assert(!s.hit());
        assert(s.source->timer[0] == wall_time);
        if (enabled) {
            assert(effects.empty()); GET_OPPONENT(s.actor) = s.actor;
            advance();
        }
        assert((effects == std::vector<effect_call>{{1,50,SPELL_TYPE_SPELL,s.target->runtime_id,0}}));
        assert((rolls == std::vector<std::pair<int,int>>{{0,10}}));
        assert(!weapon_proc(s.source, s.actor, s.target)); assert(rolls.size() == 1);
    }
    {
        weapon_scene s("randomWeapons"); s.named(); s.paid(0);
        assert(!s.hit()); assert(!s.source->timer[0] && effects.empty());
    }
    {
        weapon_scene s("randomWeapons"); s.named();
        assert(!s.hit()); depart(s.target); advance();
        assert(effects.empty() && s.source->timer[0] == wall_time);
    }
    {
        weapon_scene s("randomWeapons"); s.named(); s.source->value[7] = 2;
        assert(!s.hit());
        assert(effects.size() == 1 && effects[0].id == 2 && effects[0].target == s.actor->runtime_id);
    }
    for (int command : {CMD_GOTHIT, CMD_GOTNUKED}) {
        weapon_scene s("randomWeapons"); s.named();
        // Three matching pieces satisfy the unchanged defensive selection path.
        s.actor->equipment[6] = s.actor->equipment[19] = s.actor->equipment[21] = s.source;
        choices.push_back(0);
        assert(!random_eq_proc(s.source,s.actor,command,reinterpret_cast<char *>(s.target)));
        assert(effects.size() == 1 && !item_actions_pending());
        assert(rolls.size() == 1 && rolls[0].second == (command == CMD_GOTHIT ? 100 : 50));
    }
}

static void suppression_measurement() {
    for (bool enabled : {false, true}) {
        weapon_scene s("avernus", enabled);
        for (int i = 0; i < 200; ++i) { assert(s.hit()); advance(1); }
        advance();
        const size_t completed = effects.size() / 2;
        assert(rolls.size() == 200 && (enabled ? completed > 0 && completed < 200 : completed == 200));
        std::printf("Deterministic Avernus: enabled=%d, selected=200 at 4/sec, completed=%zu, suppressed=%zu\n",
                    enabled, completed, 200 - completed);
    }
    weapon_scene s;
    properties["itemActions.avernus.windupPulses"] = 600;
    properties["itemActions.maxPulses"] = 8; update_item_action_properties();
    assert(s.hit()); advance(5); audience_count("intensifies", 1);
    advance(8); assert(effects.size() == 2);
}

int main() {
    nevent_bind_game_thread(); ne_dead_event_pool = &test_pool;
    fake_clock_ns = 1000000000ULL; ne_events();
    avernus_parity(); cancellation_and_payment(); packed_parity(); random_parity(); suppression_measurement();
    std::puts("Weapon actions: real selection callbacks, parity, payment, targets, cancellation and timing passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-weapon-actions-") as directory:
    source = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    doubles = DOUBLES.replace("messages", "captured_messages").replace("damage_captured_messages", "damage_messages")
    source.write_text(platform + fixture + doubles.replace("// INSERT_CALLBACKS", callbacks))
    subprocess.run([
        "g++", "-std=c++20", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-pthread",
        "-I" + str(ROOT / "src"), str(source), str(ROOT / "src/persistence/latency_trace.c"),
        "-Wl,--gc-sections", "-o", str(binary),
    ], check=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
               UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1",
               DURIS_NEVENT_ANALYTICS="0", DURIS_NEVENT_BUDGET_USEC="0",
               DURIS_NEVENT_MAX_CALLBACKS="0", DURIS_NEVENT_PLAYER_PRIORITY="1")
    subprocess.run([str(binary)], check=True, env=env)

backstab = function(ROOT / "src/cmd/actoff.c", "bool single_stab(")
assert "obj_index[weapon->R_num].func.obj" in backstab and "CMD_MELEE_HIT" in backstab
assert "weapon_proc(" not in "\n".join(line.split("//", 1)[0] for line in backstab.splitlines())
print("Backstab retains the direct callback route covered by the Avernus pilot")
