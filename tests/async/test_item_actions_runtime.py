#!/usr/bin/env python3
"""Execute the item-action runtime with the production nevent scheduler under sanitizers.

Reuse only the scheduler test's C++ platform doubles, without importing/running
that test. All scheduling, payload ownership, cancellation and item-action logic
in this harness are compiled directly from production sources.
"""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
fixture = ast.parse((ROOT / "tests/async/test_nevent_scheduler_runtime.py").read_text())
scheduler = next(ast.literal_eval(node.value) for node in fixture.body
                 if isinstance(node, ast.Assign)
                 and any(isinstance(t, ast.Name) and t.id == "HARNESS" for t in node.targets))
platform = scheduler.split("struct record_payload\n", 1)[0]
platform = platform.replace("DEFINE_LABEL_CALLBACK(event_item_action_active)", "")
platform = platform.replace('void panic_corruption(const char *, const char *, ...)\n{\n\tthrow panic_signal{};\n}',
                           'void panic_corruption(const char *, const char *format, ...) { '
                           'va_list args; va_start(args, format); vfprintf(stderr, format, args); '
                           'va_end(args); std::abort(); }')


def function_body(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


abort_command = function_body((ROOT / "src/net/sparser.c").read_text(), "void do_abort(P_char ch,")

HARNESS = r'''
#define clock_gettime nevent_test_clock_gettime
#include "item/item_actions.c"
#undef clock_gettime
#include "account/character_identity.c"
#include "magic/spells.h"
#include <cassert>

void event_wait(P_char, P_char, P_obj, void *) {}
void event_ward_regen(P_char, P_char, P_obj, void *) {}
static int unrelated_wait_changes = 0;
void CharWait(P_char, int) { ++unrelated_wait_changes; }
void StopCasting(P_char) { ++unrelated_wait_changes; }
void update_pos(P_char) {}
void send_to_char(const char *, P_char) {}
void act(const char *, int, P_char, P_obj, void *, int) {}
void affect_from_char(P_char, int) {}
// INSERT_PRODUCTION_ABORT

static std::map<std::string, float> properties;
float get_property(const char *key, double fallback, bool) {
    auto found = properties.find(key);
    return found == properties.end() ? static_cast<float>(fallback) : found->second;
}

struct evidence {
    int commits = 0, announces = 0, progress = 0, effects = 0, finishes = 0, destroys = 0;
    int reserve = 0, spent = 0;
    bool permitted = true, reject_cost = false;
    int mutation = 0;
    uint64_t target_id = 0;
    int power = 0;
    int auxiliary = 0;
    item_action_effect_target effect_target = item_action_effect_target::original;
    item_action_call call = item_action_call::weapon;
    item_action_consumption cost = item_action_consumption::committed;
    item_action_outcome outcome = item_action_outcome::interrupted;
};

static void depart(P_char ch) {
    item_actions_character_leaving(ch);
    ch->in_room = NOWHERE;
}
static void remove_character(P_char ch) {
    item_actions_character_leaving(ch);
    disarm_char_nevents(ch, nullptr);
    while (ch->linked) {
        auto *link = ch->linked;
        event_broken(link);
        remove_link(link->linking, link);
    }
    P_char *cursor = &character_list;
    while (*cursor && *cursor != ch) cursor = &(*cursor)->next;
    if (*cursor) *cursor = ch->next;
    delete ch;
}
static void remove_source(P_obj obj, P_char owner) {
    item_actions_source_leaving(obj);
    disarm_obj_nevents(obj, nullptr);
    for (int slot = 0; slot < MAX_WEAR; ++slot)
        if (owner->equipment[slot] == obj) owner->equipment[slot] = nullptr;
    P_obj *cursor = &object_list;
    while (*cursor && *cursor != obj) cursor = &(*cursor)->next;
    if (*cursor) *cursor = obj->next;
    delete obj;
}

struct probe_adapter final : item_action_adapter {
    evidence &report;
    explicit probe_adapter(evidence &value) : report(value) {}
    ~probe_adapter() override { ++report.destroys; }
    bool validate(const item_action_context &) const noexcept override {
        return report.permitted;
    }
    item_action_consumption commit(const item_action_context &) const noexcept override {
        ++report.commits;
        if (report.reject_cost) return item_action_consumption::rejected;
        if (report.cost == item_action_consumption::reserved) ++report.reserve;
        if (report.cost == item_action_consumption::committed) ++report.spent;
        return report.cost;
    }
    void announce(const item_action_context &) const noexcept override { ++report.announces; }
    void progress(const item_action_context &) const noexcept override { ++report.progress; }
    void resolve(const item_action_context &context, const item_action_effect &effect) const noexcept override {
        ++report.effects;
        report.target_id = context.target->runtime_id;
        report.power = effect.power;
        report.call = effect.call;
        report.auxiliary = effect.auxiliary;
        report.effect_target = effect.target;
        switch (report.mutation) {
        case 1: depart(context.target); break;
        case 2: SET_POS(context.target, STAT_DEAD); break;
        case 3: remove_character(context.target); break;
        case 4: remove_character(context.actor); break;
        case 5: remove_source(context.source, context.actor); break;
        case 6: properties["itemActions.enabled"] = 0; update_item_action_properties(); break;
        case 7: item_actions_reload(); break;
        case 8: event_item_action_passive(nullptr, nullptr, nullptr, current_nevent->data); break;
        default: break;
        }
    }
    void finish(const item_action_identity &, item_action_consumption cost,
                item_action_outcome outcome) const noexcept override {
        ++report.finishes;
        report.outcome = outcome;
        if (cost == item_action_consumption::reserved) {
            --report.reserve;
            if (outcome != item_action_outcome::interrupted) ++report.spent;
        }
    }
};

static item_action_definition definition(uint32_t id = 1, bool active = false) {
    item_action_definition result;
    result.id = id;
    result.revision = 1;
    result.mode = active ? item_action_mode::active : item_action_mode::passive;
    result.windup_pulses = 4;
    result.effect_count = 1;
    result.effects[0] = { 19, 42, active ? item_action_call::wand : item_action_call::weapon };
    return result;
}

struct scene {
    evidence report;
    P_char actor = new char_data{};
    P_char target = new char_data{};
    P_obj source = new obj_data{};
    pc_only_data actor_pc = {}, target_pc = {};
    scene(bool active = false, int effects = 1) {
        assert(ne_event_counter == 0 && item_actions_pending() == 0);
        actor->runtime_id = allocate_character_runtime_id();
        target->runtime_id = allocate_character_runtime_id();
        actor->only.pc = &actor_pc;
        target->only.pc = &target_pc;
        actor->player.name = const_cast<char *>("Actor");
        target->player.name = const_cast<char *>("Target");
        SET_POS(actor, STAT_NORMAL + POS_STANDING);
        SET_POS(target, STAT_NORMAL + POS_STANDING);
        actor->in_room = target->in_room = 0;
        actor->next = target;
        character_list = actor;
        object_list = source;
        source->obj_uid = 100;
        source->loc_p = LOC_WORN;
        source->loc.wearing = actor;
        actor->equipment[WIELD] = source;
        static index_data index[1] = {};
        obj_index = index;
        properties.clear();
        properties["itemActions.enabled"] = 1;
        update_item_action_properties();
        auto def = definition(1, active);
        def.effect_count = effects;
        for (int i = 1; i < effects; ++i) def.effects[i] = def.effects[0];
        assert(item_actions_publish(def, std::make_unique<probe_adapter>(report)));
    }
    ~scene() {
        item_actions_reload();
        assert(item_actions_pending() == 0 && ne_event_counter == 0 && test_pool.objs_used == 0);
        while (character_list) {
            P_char ch = character_list;
            assert(!ch->nevents && !ch->linked && !ch->linking);
            character_list = ch->next;
            delete ch;
        }
        while (object_list) {
            P_obj obj = object_list;
            assert(!obj->nevents);
            object_list = obj->next;
            delete obj;
        }
    }
    item_action_start start() { return start_item_action(1, actor, target, source); }
};

static void advance(int pulses = 12, bool real_time = true) {
    for (int i = 0; i < pulses; ++i) {
        nevent_advance_tick();
        if (real_time) fake_clock_ns += static_cast<uint64_t>(OPT_USEC) * 1000;
        ne_events();
    }
}

static void test_default_off_and_rejection() {
    scene s(true);
    properties.clear();
    update_item_action_properties();
    assert(s.start() == item_action_start::legacy);
    assert(!s.report.commits && !s.report.announces && !item_action_active(s.actor));
    properties["itemActions.enabled"] = 1;
    update_item_action_properties();
    assert(start_item_action(999, s.actor, s.target, s.source) == item_action_start::legacy);
    const auto sequence = ne_event_sequence;
    ne_event_sequence = ULLONG_MAX;
    assert(s.start() == item_action_start::suppressed);
    assert(!s.report.commits && !s.report.announces && !item_actions_pending());
    assert(!item_action_active(s.actor) && CAN_ACT(s.actor));
    ne_event_sequence = sequence;
    s.report.reject_cost = true;
    assert(s.start() == item_action_start::suppressed);
    assert(s.report.commits == 1 && !s.report.announces && !s.report.finishes);
    assert(!item_action_active(s.actor) && !item_actions_pending());
}

static void test_resolve_and_caps() {
    scene s;
    assert(s.start() == item_action_start::scheduled);
    assert(!item_action_active(s.actor) && !IS_AFFECTED2(s.actor, AFF2_CASTING));
    assert(CAN_ACT(s.actor) && s.report.commits == 1 && s.report.announces == 1);
    for (int i = 0; i < 30; ++i) assert(s.start() == item_action_start::suppressed);
    auto *second = new obj_data{};
    auto *third = new obj_data{};
    *second = *s.source;
    *third = *s.source;
    second->obj_uid = 101; third->obj_uid = 102;
    second->nevents = second->nevents_tail = nullptr;
    third->nevents = third->nevents_tail = nullptr;
    s.source->next = second; second->next = third;
    s.actor->equipment[WIELD + 1] = second;
    s.actor->equipment[WIELD + 2] = third;
    assert(start_item_action(1, s.actor, s.target, second) == item_action_start::scheduled);
    assert(start_item_action(1, s.actor, s.target, third) == item_action_start::suppressed);
    assert(item_actions_pending() == 2 && s.report.commits == 2);
    advance(3);
    assert(!s.report.effects);
    // An opponent change cannot retarget either immutable action.
    s.actor->specials.fighting = s.actor;
    advance();
    assert(s.report.effects == 2 && s.report.finishes == 2 && s.report.spent == 2);
    assert(s.report.target_id == s.target->runtime_id && s.report.power == 42);
    assert(s.report.outcome == item_action_outcome::completed);
    advance();
    assert(s.report.effects == 2);
}

static void test_transition_cancel() {
    for (int kind = 0; kind < 9; ++kind) {
        scene s(true);
        s.report.cost = item_action_consumption::reserved;
        assert(s.start() == item_action_start::scheduled);
        assert(item_action_active(s.actor) && s.report.reserve == 1);
        switch (kind) {
        case 0: depart(s.actor); s.actor->in_room = 0; break;
        case 1: depart(s.target); s.target->in_room = 0; break;
        case 2: item_actions_source_leaving(s.source); break; // unequip/re-equip
        case 3: disarm_obj_nevents(s.source, nullptr); break; // owned-payload destructor
        case 4: disarm_char_nevents(s.actor, nullptr); break;
        case 5: { // Generic victim link destruction, without the explicit hook.
            auto *link = s.target->linked;
            event_broken(link);
            remove_link(link->linking, link);
            break;
        }
        case 6: remove_character(s.target); break;
        case 7: remove_character(s.actor); break;
        case 8: remove_source(s.source, s.actor); break;
        }
        assert(!item_actions_pending() && !s.report.reserve && s.report.finishes == 1);
        advance();
        assert(!s.report.effects && !s.report.spent);
    }
}

static void test_completion_validation() {
    for (int kind = 0; kind < 5; ++kind) {
        scene s;
        assert(s.start() == item_action_start::scheduled);
        switch (kind) {
        case 0: s.report.permitted = false; break;
        case 1: SET_POS(s.target, STAT_DEAD); break;
        case 2: s.target->runtime_id = allocate_character_runtime_id(); break;
        case 3: s.source->obj_uid = 777; break;
        case 4: s.source->loc.wearing = s.target; break;
        }
        advance();
        assert(!s.report.effects && s.report.finishes == 1 && s.report.spent == 1);
        assert(s.report.outcome == item_action_outcome::interrupted);
    }
}

static void test_abort_and_unrelated_wait() {
    scene s(true);
    SET_BIT(s.actor->specials.act2, PLR2_WAIT);
    assert(s.start() == item_action_start::suppressed);
    REMOVE_BIT(s.actor->specials.act2, PLR2_WAIT);
    assert(s.start() == item_action_start::scheduled);
    assert(item_action_active(s.actor));
    assert(get_scheduled(s.actor, event_item_action_active)->priority == NEVENT_PRIORITY_PLAYER);
    const auto wait = add_event(event_wait, 100, s.actor, nullptr, nullptr, 0, nullptr, 0);
    SET_BIT(s.actor->specials.act2, PLR2_WAIT);
    do_abort(s.actor, nullptr, 0);
    assert(!abort_item_action(s.actor));
    assert(!item_action_active(s.actor) && !IS_AFFECTED2(s.actor, AFF2_CASTING));
    assert(!CAN_ACT(s.actor) && nevent_handle_is_active(wait.handle));
    assert(unrelated_wait_changes == 0);
    nevent_cancel(wait.handle);
    advance();
    assert(!s.report.effects && s.report.finishes == 1);
}

static void test_config_and_reload() {
    {
        scene s;
        properties["itemActions.mana.enabled"] = 1;
        update_item_action_properties();
        assert(s.start() == item_action_start::scheduled);
        properties["itemActions.mana.enabled"] = 0;
        update_item_action_properties();
        assert(!item_actions_pending() && s.report.spent == 1);
        properties["itemActions.mana.enabled"] = 1;
        update_item_action_properties();
        advance();
        assert(!s.report.effects && s.report.finishes == 1 && s.report.spent == 1);
    }
    for (float value : {0.0f, -1.0f, 0.5f, 2.0f,
                       std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        scene s;
        assert(s.start() == item_action_start::scheduled);
        properties["itemActions.enabled"] = value;
        update_item_action_properties();
        assert(!item_actions_pending() && s.report.finishes == 1 && s.report.spent == 1);
        properties["itemActions.enabled"] = 1;
        update_item_action_properties();
        advance();
        assert(!s.report.effects && s.report.spent == 1);
    }
    for (const char *key : {"itemActions.reactionPulses", "itemActions.maxPulses",
                           "itemActions.maxPerWielder", "itemActions.maxPending"}) {
        scene s;
        properties[key] = std::numeric_limits<float>::quiet_NaN();
        update_item_action_properties();
        assert(s.start() == item_action_start::legacy);
    }
    scene s;
    assert(s.start() == item_action_start::scheduled);
    auto def = definition();
    assert(!item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
    assert(item_actions_pending() == 1);
    def.revision = 2;
    def.effects[0].power = 57;
    assert(item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
    assert(!item_actions_pending());
    def.effects[0].power = 999; // Caller mutation cannot affect published selection.
    assert(s.start() == item_action_start::scheduled);
    advance();
    assert(s.report.effects == 1 && s.report.power == 57);
    assert(s.start() == item_action_start::scheduled);
    item_actions_disable(1);
    assert(s.start() == item_action_start::legacy);
    advance();
    assert(s.report.effects == 1);
    item_actions_reload();
    assert(s.start() == item_action_start::legacy);
}

static void test_real_reaction_floor() {
    scene s;
    auto def = definition(); def.revision = 2; def.windup_pulses = 0;
    assert(item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
    assert(s.start() == item_action_start::scheduled);
    assert(get_scheduled(s.actor, event_item_action_passive)->priority == NEVENT_PRIORITY_NORMAL);
    advance(20, false); // Game ticks race ahead; real time has barely moved.
    assert(!s.report.effects && item_actions_pending() == 1);
    advance();
    assert(s.report.effects == 1 && !item_actions_pending());
}

static void test_selected_effects_and_progress() {
    scene s;
    item_action_selection selection;
    selection.effect_count = 1;
    selection.effects[0] = { 88, 155, item_action_call::spell,
                            item_action_effect_target::actor, 277 };
    assert(start_selected_item_action(1, s.actor, s.target, s.source, selection) ==
           item_action_start::suppressed); // fixed definitions reject overrides
    auto def = definition();
    def.revision = 2; def.selected_effects = true; def.effect_count = 0;
    def.progress_pulses = 4;
    assert(!item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
    def.progress_pulses = 2;
    assert(item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
    assert(s.start() == item_action_start::suppressed); // dynamic definitions need selection
    selection.effects[0].auxiliary = -1;
    assert(start_selected_item_action(1, s.actor, s.target, s.source, selection) ==
           item_action_start::suppressed);
    selection.effects[0].auxiliary = 277;
    assert(start_selected_item_action(1, s.actor, s.target, s.source, selection) ==
           item_action_start::scheduled);
    selection.effects[0] = { 99, 999, item_action_call::wand };
    advance(20, false); // accelerated ticks cannot reveal progress or release early
    assert(!s.report.progress && !s.report.effects);
    advance(3);
    assert(s.report.progress == 1 && !s.report.effects);
    advance();
    assert(s.report.effects == 1 && s.report.progress == 1);
    assert(s.report.power == 155 && s.report.auxiliary == 277);
    assert(s.report.call == item_action_call::spell);
    assert(s.report.effect_target == item_action_effect_target::actor);
    assert(s.report.commits == 1 && s.report.finishes == 1);
}

static void test_effect_transitions() {
    for (int mutation = 1; mutation <= 8; ++mutation) {
        scene s(false, 3);
        s.report.cost = item_action_consumption::reserved;
        s.report.mutation = mutation;
        assert(s.start() == item_action_start::scheduled);
        advance();
        assert(s.report.effects == (mutation == 8 ? 3 : 1));
        assert(s.report.finishes == 1 && !item_actions_pending());
        assert(s.report.reserve == 0 && s.report.spent == 1);
        assert(s.report.outcome == (mutation == 8 ? item_action_outcome::completed :
                                                     item_action_outcome::partially_resolved));
    }
}

static void test_carry_self_reload_and_rearm_rejection() {
    {
        scene s(true);
        s.actor->equipment[WIELD] = nullptr;
        s.actor->carrying = s.source;
        s.source->loc_p = LOC_CARRIED;
        s.source->loc.carrying = s.actor;
        auto def = definition(1, true);
        def.source = item_action_source::carried;
        def.revision = 2;
        assert(item_actions_publish(def, std::make_unique<probe_adapter>(s.report)));
        assert(start_item_action(1, s.actor, s.actor, s.source) == item_action_start::scheduled);
        update_item_action_properties(); // Unchanged configuration preserves this action.
        assert(item_actions_pending() == 1);
        advance();
        assert(s.report.effects == 1 && s.report.target_id == s.actor->runtime_id);
        assert(s.start() == item_action_start::scheduled);
        item_actions_source_leaving(s.source); // Transfer-away/return cancels permanently.
        advance();
        assert(s.report.effects == 1);
    }
    {
        scene s;
        s.report.cost = item_action_consumption::reserved;
        assert(s.start() == item_action_start::scheduled);
        const auto sequence = ne_event_sequence;
        ne_event_sequence = ULLONG_MAX;
        advance(5, false); // Failure while transferring a still-owned reaction-window payload.
        ne_event_sequence = sequence;
        assert(!item_actions_pending() && !s.report.reserve && !s.report.spent);
        assert(!s.report.effects && s.report.finishes == 1);
    }
    {
        scene s;
        evidence other;
        auto def = definition(2);
        assert(item_actions_publish(def, std::make_unique<probe_adapter>(other)));
        assert(s.start() == item_action_start::scheduled);
        item_actions_disable(2); // Independent rollback must not cancel ability 1.
        assert(item_actions_pending() == 1);
        advance();
        assert(s.report.effects == 1 && other.effects == 0);
        item_actions_reload(); // Destroy adapter before its evidence leaves scope.
    }
}

static uint64_t metric(item_action_metric value) {
    return item_actions_telemetry_snapshot().counters[static_cast<size_t>(value)];
}
static uint64_t cancelled(item_action_cancel_reason value) {
    return item_actions_telemetry_snapshot().cancelled[static_cast<size_t>(value)];
}
static void observe() {
    properties["itemActions.telemetry.enabled"]=1; update_item_action_properties();
}
static void test_bounded_telemetry() {
    { scene s; s.start(); advance(); assert(!metric(item_action_metric::selected)); }
    { scene s; observe();
      assert(s.start()==item_action_start::scheduled);
      assert(s.start()==item_action_start::suppressed);
      auto snapshot=item_actions_telemetry_snapshot(); assert(snapshot.pending==1 && snapshot.peak_pending==1);
      advance(); assert(metric(item_action_metric::selected)==2 && metric(item_action_metric::started)==1);
      assert(metric(item_action_metric::busy)==1 && metric(item_action_metric::completed)==1);
      assert(metric(item_action_metric::effects_invoked)==1 && item_actions_telemetry_snapshot().callbacks>=1); }
    for(int reason=0;reason<8;++reason) {
        scene s(true); observe(); s.start();
        item_action_cancel_reason expected;
        switch(reason) {
        case 0: depart(s.actor); expected=item_action_cancel_reason::actor_departure; break;
        case 1: depart(s.target); expected=item_action_cancel_reason::target_departure; break;
        case 2: item_actions_source_leaving(s.source); expected=item_action_cancel_reason::source_departure; break;
        case 3: item_actions_disable(1); expected=item_action_cancel_reason::definition_change; break;
        case 4: item_actions_reload(); expected=item_action_cancel_reason::reload; break;
        case 5: abort_item_action(s.actor); expected=item_action_cancel_reason::abort; break;
        case 6: s.report.permitted=false; expected=item_action_cancel_reason::invalid_context; break;
        default: properties["itemActions.enabled"]=0; update_item_action_properties();
                 expected=item_action_cancel_reason::configuration_change;
        }
        advance(); assert(cancelled(expected)==1 && !item_actions_pending());
        assert(metric(item_action_metric::completed)==0 && !s.report.effects);
    }
    { scene s; observe(); s.report.reject_cost=true; s.start();
      assert(metric(item_action_metric::consumption_rejected)==1 && !metric(item_action_metric::started)); }
    { scene s; observe(); const auto sequence=ne_event_sequence; ne_event_sequence=ULLONG_MAX;
      s.start(); ne_event_sequence=sequence;
      assert(metric(item_action_metric::scheduling_rejected)==1 && !metric(item_action_metric::started)); }
    { scene s; observe(); s.start(); const auto sequence=ne_event_sequence; ne_event_sequence=ULLONG_MAX;
      advance(5,false); ne_event_sequence=sequence;
      assert(cancelled(item_action_cancel_reason::scheduling_rejected)==1); }
    { scene s(false,2); observe(); s.report.mutation=1; s.start(); advance();
      assert(metric(item_action_metric::partial)==1 && metric(item_action_metric::effect_failures)==1);
      assert(metric(item_action_metric::effects_invoked)==1 && cancelled(item_action_cancel_reason::target_departure)==1); }
    { scene s; s.start(); observe(); assert(item_actions_pending()==1); advance();
      assert(s.report.effects==1); // Merely enabling observation must not cancel work.
      telemetry.counters[static_cast<size_t>(item_action_metric::selected)]=UINT64_MAX;
      item_actions_note(item_action_metric::selected); assert(metric(item_action_metric::selected)==UINT64_MAX);
      for(float value:{0.0f,1.5f,std::numeric_limits<float>::quiet_NaN()}) {
          properties["itemActions.telemetry.enabled"]=value; update_item_action_properties();
          item_actions_note(item_action_metric::selected); assert(!item_actions_telemetry_enabled());
          assert(!metric(item_action_metric::selected));
      } }
}

int main() {
    nevent_bind_game_thread();
    ne_dead_event_pool = &test_pool;
    fake_clock_ns = 1000000000ULL;
    ne_events();
    test_default_off_and_rejection();
    test_resolve_and_caps();
    test_transition_cancel();
    test_completion_validation();
    test_abort_and_unrelated_wait();
    test_config_and_reload();
    test_real_reaction_floor();
    test_selected_effects_and_progress();
    test_effect_transitions();
    test_carry_self_reload_and_rearm_rejection();
    test_bounded_telemetry();
    std::puts("Item actions: scheduler, identities, costs, cancellation, timing and effect lifetime passed");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-item-actions-") as directory:
    source = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    source.write_text(platform + HARNESS.replace("// INSERT_PRODUCTION_ABORT", abort_command))
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

# Exercise placement contracts for full-server boundaries whose dependencies are
# intentionally outside the focused runtime harness.
handler = (ROOT / "src/world/handler.c").read_text()
assert handler.index("item_actions_character_leaving(ch);") > handler.index("ROOM_PROC_LEAVE_VETO")
for signature, hook in [("void char_from_room(", "item_actions_character_leaving(ch)"),
                        ("void extract_char(P_char", "item_actions_character_leaving(ch)"),
                        ("void obj_from_char(", "item_actions_source_leaving(object)"),
                        ("P_obj unequip_char(", "item_actions_source_leaving(obj)"),
                        ("void extract_obj(", "item_actions_source_leaving(obj)")]:
    assert hook in handler.split(signature, 1)[1].split("\n}", 1)[0]
assert "item_action_active(ch)" in (ROOT / "src/combat/fight.c").read_text()
properties = (ROOT / "src/world/properties.c").read_text()
assert "update_item_action_properties();" in properties.split("void apply_properties()", 1)[1]
assert "item_actions_reload();" in properties.split("void initialize_properties()", 1)[1]
assert "itemActions.enabled=0" in (ROOT / "lib/duris.properties").read_text()
print("Item-action transition/configuration hooks and default-off routing passed")
