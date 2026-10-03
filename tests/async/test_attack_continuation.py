#!/usr/bin/env python3
"""Exercise the checked combat continuation contract under destructive callbacks."""

import re
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function, source


PREFIX = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include <cassert>
#include <cstdio>

static char_data actor = {}, target = {};
static pc_only_data actor_pc = {}, target_pc = {};
static obj_data weapon = {}, replacement = {};
static bool actor_listed = true, target_listed = true;
P_obj object_list = nullptr;

P_char find_character_by_runtime_id(uint64_t id) {
    if (actor_listed && actor.runtime_id == id) return &actor;
    if (target_listed && target.runtime_id == id) return &target;
    return nullptr;
}

static void reset() {
    actor = {};
    target = {};
    weapon = {};
    replacement = {};
    actor.only.pc = &actor_pc;
    target.only.pc = &target_pc;
    actor.runtime_id = 10;
    target.runtime_id = 20;
    actor.in_room = target.in_room = 1;
    actor.specials.z_cord = target.specials.z_cord = 0;
    SET_POS(&actor, STAT_NORMAL + POS_STANDING);
    SET_POS(&target, STAT_NORMAL + POS_STANDING);
    weapon.obj_uid = 100;
    replacement.obj_uid = 200;
    weapon.next = &replacement;
    replacement.next = nullptr;
    object_list = &weapon;
    actor.equipment[PRIMARY_WEAPON] = &weapon;
    actor_listed = target_listed = true;
}

static attack_continuation guard() {
    return begin_attack_continuation(&actor, &target, &weapon, PRIMARY_WEAPON);
}

static void expect(attack_continuation guard, attack_continuation_outcome outcome) {
    const attack_continuation_result checked = check_attack_continuation(guard);
    assert(checked.outcome == outcome);
    if (outcome == attack_continuation_outcome::continue_attack) {
        assert(checked.actor == &actor);
        assert(checked.target == &target);
        assert(checked.weapon == &weapon);
    }
}
'''

SUFFIX = r'''
int main() {
    expect(begin_attack_continuation(nullptr, &target),
           attack_continuation_outcome::cancelled);
    expect(begin_attack_continuation(&actor, nullptr),
           attack_continuation_outcome::cancelled);
    reset();
    expect(guard(), attack_continuation_outcome::continue_attack);
    const attack_continuation actor_only = begin_attack_continuation(&actor, &actor);
    assert(check_attack_continuation(actor_only).can_continue());

    auto captured = guard();
    actor_listed = false;
    expect(captured, attack_continuation_outcome::actor_gone);
    reset();
    captured = guard();
    captured.weapon_slot = MAX_WEAR;
    expect(captured, attack_continuation_outcome::cancelled);

    reset();
    captured = guard();
    SET_POS(&actor, STAT_DEAD);
    expect(captured, attack_continuation_outcome::actor_gone);
    reset();
    captured = guard();
    ++actor.runtime_id;
    expect(captured, attack_continuation_outcome::actor_gone);

    reset();
    captured = guard();
    target_listed = false;
    expect(captured, attack_continuation_outcome::target_gone);
    reset();
    captured = guard();
    SET_POS(&target, STAT_DEAD);
    expect(captured, attack_continuation_outcome::target_gone);
    reset();
    captured = guard();
    ++target.runtime_id;
    expect(captured, attack_continuation_outcome::target_gone);

    reset();
    captured = guard();
    ++actor.in_room;
    expect(captured, attack_continuation_outcome::relocated);
    reset();
    actor.specials.z_cord = 1;
    target.specials.z_cord = 3;
    captured = guard();
    expect(captured, attack_continuation_outcome::continue_attack);
    ++target.specials.z_cord;
    expect(captured, attack_continuation_outcome::relocated);

    reset();
    captured = guard();
    actor.equipment[PRIMARY_WEAPON] = nullptr;
    expect(captured, attack_continuation_outcome::weapon_changed);
    reset();
    captured = guard();
    actor.equipment[PRIMARY_WEAPON] = &replacement;
    expect(captured, attack_continuation_outcome::weapon_changed);
    reset();
    captured = guard();
    ++weapon.obj_uid;
    expect(captured, attack_continuation_outcome::weapon_changed);

    // An unrelated removal does not cancel a valid continuation.
    reset();
    actor_listed = target_listed = true;
    expect(guard(), attack_continuation_outcome::continue_attack);

    puts("combat continuation outcomes and live-pointer revalidation passed");
}
'''


vicious_section = source("fight.c").read_text(encoding="utf-8")
vicious_section = vicious_section[
    vicious_section.index("if (GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK)") :
    vicious_section.index("/* calculate the damage */", vicious_section.index(
        "if (GET_CHAR_SKILL(ch, SKILL_VICIOUS_ATTACK)"
    ))
]
assert "begin_attack_continuation" in vicious_section
assert "check_attack_continuation" in vicious_section
assert "is_char_in_room(ch, room)" not in vicious_section


def continuation_caller(filename, signature):
    return extract_function(filename, signature)


flurry = continuation_caller("actnew.c", "void do_flurry_of_blows(P_char ch, char *arg)")
flurry_hit = flurry.index("hit(ch, tch, NULL)")
flurry_check = flurry.index("check_attack_continuation", flurry_hit)
assert flurry.rfind("begin_attack_continuation(ch, tch)", 0, flurry_hit) >= 0
assert flurry.index("find_character_by_runtime_id(actor_runtime_id)", flurry_check) > flurry_check
assert "find_character_by_runtime_id(next_tch_runtime_id)" in flurry
assert flurry.rindex("find_character_by_runtime_id(actor_runtime_id)") < flurry.index(
    "CharWait(ch, PULSE_VIOLENCE * 3)"
)

hitall = continuation_caller("actnew.c", "void do_hitall(P_char ch, char *arg, int /*cmd*/)")
assert "check_attack_continuation(continuation)" in hitall
assert "find_character_by_runtime_id(next_mob_runtime_id)" in hitall

stormcaller = continuation_caller(
    "reavers.c", "bool stormcallers_fury(P_char ch, P_char victim, P_obj wpn)"
)
storm_hit = stormcaller.index("hit(ch, victim, wpn)")
storm_check = stormcaller.index("check_attack_continuation", storm_hit)
assert storm_check > storm_hit
assert stormcaller.index("P_char live_actor =", storm_check) > storm_check
assert stormcaller.index(
    "get_spell_from_char(ch, SPELL_STORMCALLERS_FURY)", storm_check
) > storm_check

bleed = continuation_caller(
    "actoff.c", "void event_bleedproc(P_char ch, P_char victim, P_obj /*obj*/, void *data)"
)
bleed_damage = bleed.index("raw_damage(ch, victim")
bleed_actor = bleed.index("ch = find_character_by_runtime_id(actor_runtime_id)", bleed_damage)
bleed_victim = bleed.index("victim = find_character_by_runtime_id(victim_runtime_id)", bleed_actor)
assert bleed_damage < bleed_actor < bleed_victim < bleed.index("// Show messages..")
assert "ch->in_room" not in bleed and "victim->in_room" not in bleed

takedown = continuation_caller(
    "actoff.c",
    "float takedown_check(P_char ch, P_char victim, float chance, int skill, ulong applicable)",
)
takedown_compact = "".join(takedown.split())
assert (
    takedown_compact.index("!char_in_list(ch)")
    < takedown_compact.index("!IS_ALIVE(ch)")
)
assert (
    takedown_compact.index("!char_in_list(victim)")
    < takedown_compact.index("!IS_ALIVE(victim)")
)
sleeping_takedown = takedown[
    takedown.index("if (GET_STAT(victim) <= STAT_SLEEPING)") :
    takedown.index("if (check_freedom_of_movement")
]
assert sleeping_takedown.index("actor_runtime_id = ch->runtime_id") < sleeping_takedown.index("hit(")
assert sleeping_takedown.index("hit(") < sleeping_takedown.index(
    "ch = find_character_by_runtime_id(actor_runtime_id)"
) < sleeping_takedown.index("CharWait(ch, PULSE_VIOLENCE)")

surprise = continuation_caller("actoff.c", "int surprise(P_char ch, P_char victim)")
surprise_first_hit = surprise.index("hit(ch, victim, primary_weapon)")
surprise_check = surprise.index("check_attack_continuation(continuation)", surprise_first_hit)
surprise_second_hit = surprise.index("hit(ch, victim, ch->equipment[SECONDARY_WEAPON])")
assert surprise_first_hit < surprise_check < surprise_second_hit

combination = continuation_caller(
    "actoff.c", "void event_combination(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)"
)
combination_damage = combination.index("result = melee_damage(ch, victim")
combination_check = combination.index("check_attack_continuation(continuation)", combination_damage)
combination_stun = combination.index("Stun(victim, ch,", combination_check)
stun_check = combination.index("check_attack_continuation(stun_continuation)", combination_stun)
assert combination_damage < combination_check < combination_stun < stun_check
assert combination.rindex("find_character_by_runtime_id(actor_runtime_id)") < combination.index(
    "notch_skill(ch, SKILL_COMBINATION"
)

rush = continuation_caller("actoff.c", "void rush(P_char ch, P_char victim)")
rush_actor_guard = rush.index("if (!ch || !char_in_list(ch) || !IS_ALIVE(ch))")
rush_victim_guard = rush.index("if (!victim || !char_in_list(victim) || !IS_ALIVE(victim))")
rush_opponent_guard = rush.index("if (char_in_list(opponent) && IS_ALIVE(opponent))")
rush_begin = rush.index("begin_attack_continuation(ch, victim)", rush_opponent_guard)
rush_retaliation = rush.index("hit(opponent, ch,")
rush_check = rush.index("check_attack_continuation(continuation)", rush_retaliation)
rush_height_guard = rush.index("checked.target->specials.z_cord != continuation.height", rush_check)
rush_fighting = rush.index("stop_fighting(ch)", rush_height_guard)
rush_second_hit = rush.index("hit(ch, victim, ch->equipment[PRIMARY_WEAPON])", rush_fighting)
assert rush_actor_guard < rush_victim_guard < rush_opponent_guard < rush_begin
assert rush_begin < rush_retaliation < rush_check < rush_height_guard < rush_fighting < rush_second_hit

do_rush = continuation_caller("actoff.c", "void do_rush(P_char ch, char *argument, int /*cmd*/)")
assert do_rush.index("!char_in_list(ch)") < do_rush.index("!IS_ALIVE(ch)")

spell_damage = continuation_caller(
    "fight.c", "int spell_damage(P_char ch, P_char victim, double dam, int type, uint flags,"
)
spell_damage_compact = "".join(spell_damage.split())
spell_damage_actor_capture = spell_damage_compact.index("constuint64_tch_runtime_id=ch->runtime_id;")
spell_damage_victim_capture = spell_damage_compact.index(
    "constuint64_tvictim_runtime_id=victim->runtime_id;", spell_damage_actor_capture
)
spell_damage_refresh = spell_damage_compact.index(
    "autorefresh_spell_damage_participants=[&]()", spell_damage_victim_capture
)
spell_damage_refresh_actor_lookup = spell_damage_compact.index(
    "ch=find_character_by_runtime_id(ch_runtime_id)", spell_damage_refresh
)
spell_damage_refresh_actor_liveness = spell_damage_compact.index(
    "constboolattacker_alive=ch&&IS_ALIVE(ch)", spell_damage_refresh_actor_lookup
)
spell_damage_callback = spell_damage_compact.index(
    "spell_damage_modifiers[modifier_index](ch,victim,dam,type,flags,&dam_mod,messages);"
)
spell_damage_revalidation = spell_damage_compact.index(
    "if(dam_mod.requires_participant_revalidation)", spell_damage_callback
)
spell_damage_refresh_call = spell_damage_compact.index(
    "result=refresh_spell_damage_participants();", spell_damage_revalidation
)
spell_damage_modifier_apply = spell_damage_compact.index(
    "switch(dam_mod.type)", spell_damage_refresh_call
)
assert spell_damage_actor_capture < spell_damage_victim_capture < spell_damage_refresh
assert spell_damage_refresh_actor_lookup < spell_damage_refresh_actor_liveness
assert spell_damage_refresh < spell_damage_callback < spell_damage_revalidation
assert spell_damage_revalidation < spell_damage_refresh_call < spell_damage_modifier_apply

spell_modifiers_source = source("dam_mods.c").read_text(encoding="utf-8")
spell_modifiers_start = spell_modifiers_source.index("dam_mod_predicate spell_damage_modifiers[] =")
spell_modifiers_end = spell_modifiers_source.index(
    "static_assert(ARRAY_SIZE(spell_damage_modifiers)", spell_modifiers_start
)
ethereal_modifier = spell_modifiers_source[
    spell_modifiers_source.index(
        "if (get_linked_char(victim, LNK_ETHEREAL) ||", spell_modifiers_start
    ) : spell_modifiers_end
]
ethereal_damage_call = ethereal_modifier.index("raw_damage(caster, eth_ch")
ethereal_revalidation_flag = ethereal_modifier.index(
    "dam_mod->requires_participant_revalidation = true"
)
ethereal_damage_modifier = ethereal_modifier.index("dam_mod->type = dam_mod_type::More")
assert ethereal_modifier.index("char_in_list(eth_ch)") < ethereal_modifier.index("IS_ALIVE(eth_ch)")
assert ethereal_damage_call < ethereal_revalidation_flag < ethereal_damage_modifier

legacy_damage = continuation_caller(
    "fight.c", "bool damage(P_char ch, P_char victim, double dam, int attacktype)"
)
legacy_damage_compact = "".join(legacy_damage.split())
legacy_damage_nonspell = legacy_damage_compact.index(
    "else{constboolactor_listed=ch&&char_in_list(ch);"
)
legacy_damage_actor_listed = legacy_damage_compact.index(
    "constboolactor_listed=ch&&char_in_list(ch);", legacy_damage_nonspell
)
legacy_damage_victim_listed = legacy_damage_compact.index(
    "constboolvictim_listed=victim&&char_in_list(victim);", legacy_damage_actor_listed
)
legacy_damage_entry_guard = legacy_damage_compact.index(
    "if(!actor_listed||!victim_listed)returnTRUE;", legacy_damage_victim_listed
)
legacy_damage_actor_capture = legacy_damage_compact.index(
    "constuint64_tactor_runtime_id=ch->runtime_id;", legacy_damage_entry_guard
)
assert (
    legacy_damage_nonspell
    < legacy_damage_actor_listed
    < legacy_damage_victim_listed
    < legacy_damage_entry_guard
    < legacy_damage_actor_capture
)
legacy_damage_raw_call = legacy_damage_compact.index(
    "constintraw_result=raw_damage(ch,victim,dam,RAWDAM_DEFAULT,&tmsg);"
)
legacy_damage_result_guard = legacy_damage_compact.index(
    "if(raw_result!=DAM_NONEDEAD)returnTRUE;", legacy_damage_raw_call
)
legacy_damage_actor_resolve = legacy_damage_compact.index(
    "ch=find_character_by_runtime_id(actor_runtime_id)", legacy_damage_result_guard
)
legacy_damage_victim_resolve = legacy_damage_compact.index(
    "victim=find_character_by_runtime_id(victim_runtime_id)", legacy_damage_actor_resolve
)
legacy_damage_liveness_guard = legacy_damage_compact.index(
    "if(!ch||!victim||!IS_ALIVE(ch)||!IS_ALIVE(victim))returnTRUE;",
    legacy_damage_victim_resolve,
)
legacy_damage_engagement = legacy_damage_compact.index(
    "if(!IS_FIGHTING(ch)&&(ch->in_room==victim->in_room))", legacy_damage_liveness_guard
)
legacy_damage_retaliation = legacy_damage_compact.index(
    "constintretaliation_result=attack_back(ch,victim,attacktype>FIRST_SKILL);",
    legacy_damage_engagement,
)
assert legacy_damage_raw_call < legacy_damage_result_guard < legacy_damage_actor_resolve
assert legacy_damage_actor_resolve < legacy_damage_victim_resolve < legacy_damage_liveness_guard
assert legacy_damage_liveness_guard < legacy_damage_engagement < legacy_damage_retaliation

attack_back = continuation_caller(
    "fight_state.c", "int attack_back(P_char ch, P_char victim, int physical)"
)
attack_back_compact = "".join(attack_back.split())
attack_back_actor_membership = attack_back_compact.index(
    "constboolattacker_in_list=ch&&char_in_list(ch);"
)
attack_back_victim_membership = attack_back_compact.index(
    "constboolvictim_in_list=victim&&char_in_list(victim);",
    attack_back_actor_membership,
)
attack_back_membership_return = attack_back_compact.index(
    "if(!attacker_in_list&&!victim_in_list)returnDAM_BOTHDEAD;",
    attack_back_victim_membership,
)
attack_back_dummy_check = attack_back_compact.index(
    "if(training_dummy_is(ch)||training_dummy_is(victim))", attack_back_membership_return
)
attack_back_initial_liveness = attack_back_compact.index(
    "if(!IS_ALIVE(ch))", attack_back_dummy_check
)
attack_back_actor_capture = attack_back_compact.index(
    "constuint64_tactor_runtime_id=ch->runtime_id;", attack_back_initial_liveness
)
attack_back_victim_capture = attack_back_compact.index(
    "constuint64_tvictim_runtime_id=victim->runtime_id;", attack_back_actor_capture
)
attack_back_retaliation = attack_back_compact.index(
    "MobRetaliateRange(victim,ch);", attack_back_victim_capture
)
attack_back_actor_refresh = attack_back_compact.index(
    "ch=find_character_by_runtime_id(actor_runtime_id)", attack_back_retaliation
)
attack_back_victim_refresh = attack_back_compact.index(
    "victim=find_character_by_runtime_id(victim_runtime_id)", attack_back_actor_refresh
)
attack_back_post_callback_liveness = attack_back_compact.index(
    "constboolattacker_alive=ch&&IS_ALIVE(ch)", attack_back_victim_refresh
)
attack_back_followup = attack_back_compact.index(
    "if(!IS_ALIVE(ch))", attack_back_post_callback_liveness
)
assert attack_back_actor_membership < attack_back_victim_membership < attack_back_membership_return
assert attack_back_membership_return < attack_back_dummy_check < attack_back_initial_liveness
assert attack_back_initial_liveness < attack_back_actor_capture < attack_back_victim_capture
assert attack_back_victim_capture < attack_back_retaliation < attack_back_actor_refresh
assert attack_back_actor_refresh < attack_back_victim_refresh < attack_back_post_callback_liveness
assert attack_back_post_callback_liveness < attack_back_followup

mob_retaliate_range = continuation_caller(
    "mobact.c", "void MobRetaliateRange(P_char ch, P_char vict)"
)
mob_retaliate_compact = "".join(mob_retaliate_range.split())
mob_retaliate_entry_membership = mob_retaliate_compact.index(
    "!ch||!vict||!char_in_list(ch)||!char_in_list(vict)"
)
mob_retaliate_sanity = mob_retaliate_compact.index(
    "if(!SanityCheck(ch,\"MobRetaliateRange\"))", mob_retaliate_entry_membership
)
mob_retaliate_post_sanity_membership = mob_retaliate_compact.index(
    "if(!char_in_list(ch)||!char_in_list(vict))", mob_retaliate_sanity
)
mob_retaliate_actor_capture = mob_retaliate_compact.index(
    "constuint64_tch_runtime_id=ch->runtime_id;", mob_retaliate_post_sanity_membership
)
mob_retaliate_victim_capture = mob_retaliate_compact.index(
    "constuint64_tvictim_runtime_id=vict->runtime_id;", mob_retaliate_actor_capture
)
mob_retaliate_refresh = mob_retaliate_compact.index(
    "autorefresh_retaliation_participants=[&]()", mob_retaliate_victim_capture
)
mob_retaliate_flee_positions = []
mob_retaliate_offset = 0
while True:
    try:
        mob_retaliate_offset = mob_retaliate_compact.index(
            "do_flee(ch,0,0);", mob_retaliate_offset
        )
    except ValueError:
        break
    mob_retaliate_flee_positions.append(mob_retaliate_offset)
    mob_retaliate_offset += 1
assert len(mob_retaliate_flee_positions) == 3
mob_retaliate_first_flee_check = mob_retaliate_compact.index(
    "if(!refresh_retaliation_participants())return;", mob_retaliate_flee_positions[0]
)
mob_retaliate_range_check = mob_retaliate_compact.index(
    "!mob_can_range_att(ch,vict)", mob_retaliate_first_flee_check
)
mob_retaliate_second_flee_check = mob_retaliate_compact.index(
    "if(!refresh_retaliation_participants())return;", mob_retaliate_flee_positions[1]
)
mob_retaliate_cover_check = mob_retaliate_compact.index(
    "if((!IS_AFFECTED3(ch,AFF3_COVER)))", mob_retaliate_second_flee_check
)
assert mob_retaliate_entry_membership < mob_retaliate_sanity
assert mob_retaliate_sanity < mob_retaliate_post_sanity_membership
assert mob_retaliate_post_sanity_membership < mob_retaliate_actor_capture
assert mob_retaliate_actor_capture < mob_retaliate_victim_capture < mob_retaliate_refresh
assert mob_retaliate_flee_positions[0] < mob_retaliate_first_flee_check < mob_retaliate_range_check
assert mob_retaliate_flee_positions[1] < mob_retaliate_second_flee_check < mob_retaliate_cover_check

celestia = continuation_caller(
    "specs.celestia.c",
    "int Malevolence_vapor(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)",
)
celestia_sweep = celestia[celestia.index("case 4:") : celestia.index("case 6:")]
celestia_sweep_compact = "".join(celestia_sweep.split())
celestia_actor_identity = celestia_sweep_compact.index(
    "actor_runtime_id=ch->runtime_id"
)
celestia_room_identity = celestia_sweep_compact.index("actor_room=ch->in_room")
celestia_height_identity = celestia_sweep_compact.index(
    "actor_height=ch->specials.z_cord"
)
celestia_next_identity = celestia_sweep_compact.index(
    "next_victim_runtime_id=tch?tch->runtime_id:0"
)
celestia_hit = celestia_sweep_compact.index(
    "hit(ch,vict,ch->equipment[PRIMARY_WEAPON])"
)
celestia_actor_refresh = celestia_sweep_compact.index(
    "ch=find_character_by_runtime_id(actor_runtime_id)", celestia_hit
)
celestia_actor_guard = celestia_sweep_compact.index(
    "if(!ch||!IS_ALIVE(ch)||ch->in_room!=actor_room||ch->specials.z_cord!=actor_height)returnTRUE;",
    celestia_actor_refresh,
)
celestia_next_refresh = celestia_sweep_compact.index(
    "tch=find_character_by_runtime_id(next_victim_runtime_id)", celestia_actor_guard
)
celestia_next_room_guard = celestia_sweep_compact.index(
    "!is_char_in_room(tch,actor_room)", celestia_next_refresh
)
assert (
    celestia_actor_identity
    < celestia_room_identity
    < celestia_height_identity
    < celestia_next_identity
    < celestia_hit
    < celestia_actor_refresh
    < celestia_actor_guard
    < celestia_next_refresh
    < celestia_next_room_guard
)

madman_mangler = continuation_caller(
    "specs.tikitt.c", "int madman_mangler(P_obj obj, P_char ch, int cmd, char *arg)"
)
madman_mangler_compact = "".join(madman_mangler.split())
assert madman_mangler_compact.index("!char_in_list(ch)") < madman_mangler_compact.index(
    "!IS_ALIVE(ch)"
)
assert madman_mangler_compact.index("!char_in_list(victim)") < madman_mangler_compact.index(
    "!IS_ALIVE(victim)"
)
madman_hit = madman_mangler_compact.index("hit(ch,victim,obj)")
madman_guard = madman_mangler_compact.rindex(
    "begin_attack_continuation(ch,victim,obj)", 0, madman_hit
)
madman_check = madman_mangler_compact.index(
    "check_attack_continuation(continuation)", madman_hit
)
madman_equipment_guard = madman_mangler_compact.index(
    "!OBJ_WORN(after_hit.weapon)||after_hit.weapon->loc.wearing!=after_hit.actor",
    madman_check,
)
madman_actor_refresh = madman_mangler_compact.index(
    "ch=after_hit.actor", madman_equipment_guard
)
madman_target_refresh = madman_mangler_compact.index(
    "victim=after_hit.target", madman_actor_refresh
)
madman_object_refresh = madman_mangler_compact.index(
    "obj=after_hit.weapon", madman_target_refresh
)
assert (
    madman_guard
    < madman_hit
    < madman_check
    < madman_equipment_guard
    < madman_actor_refresh
    < madman_target_refresh
    < madman_object_refresh
)

lucky_weapon = continuation_caller(
    "specs.githzer.c", "int lucky_weapon(P_obj obj, P_char ch, int cmd, char *arg)"
)
lucky_weapon_compact = "".join(lucky_weapon.split())
assert lucky_weapon_compact.index("!char_in_list(ch)") < lucky_weapon_compact.index(
    "!IS_ALIVE(ch)"
)
assert lucky_weapon_compact.index("!char_in_list(vict)") < lucky_weapon_compact.index(
    "!IS_ALIVE(vict)"
)
lucky_branch_start = lucky_weapon_compact.index("elseif(!number(0,100))")
lucky_branch = lucky_weapon_compact[lucky_branch_start:]
lucky_spell = lucky_branch.index("spell_serendipity(60,ch,NULL,SPELL_TYPE_SPELL,ch,0)")
lucky_spell_guard = lucky_branch.rindex(
    "begin_attack_continuation(ch,vict,obj)", 0, lucky_spell
)
lucky_spell_check = lucky_branch.index(
    "check_attack_continuation(luck_continuation)", lucky_spell
)
lucky_spell_actor_refresh = lucky_branch.index("ch=after_luck.actor", lucky_spell_check)
lucky_spell_target_refresh = lucky_branch.index(
    "vict=after_luck.target", lucky_spell_actor_refresh
)
lucky_spell_weapon_refresh = lucky_branch.index(
    "obj=after_luck.weapon", lucky_spell_target_refresh
)
lucky_bloodstain = lucky_branch.index("make_bloodstain(ch)", lucky_spell_weapon_refresh)
lucky_hit = lucky_branch.index("hit(ch,vict,obj)", lucky_bloodstain)
lucky_hit_guard = lucky_branch.rindex(
    "begin_attack_continuation(ch,vict,obj)", 0, lucky_hit
)
lucky_hit_check = lucky_branch.index("check_attack_continuation(continuation)", lucky_hit)
lucky_hit_refresh = lucky_branch.index("ch=after_hit.actor", lucky_hit_check)
assert (
    lucky_spell_guard
    < lucky_spell
    < lucky_spell_check
    < lucky_spell_actor_refresh
    < lucky_spell_target_refresh
    < lucky_spell_weapon_refresh
    < lucky_bloodstain
    < lucky_hit_guard
    < lucky_hit
    < lucky_hit_check
    < lucky_hit_refresh
)

pv_common = continuation_caller(
    "attack_resolution.c",
    "int pv_common(P_char ch, P_char opponent, const P_obj wpn, int *damAccumulator)",
)
pv_common_compact = "".join(pv_common.split())
assert pv_common_compact.index("!char_in_list(ch)") < pv_common_compact.index(
    "!IS_ALIVE(ch)"
)
assert pv_common_compact.index("!char_in_list(opponent)") < pv_common_compact.index(
    "!IS_ALIVE(opponent)"
)
pv_common_guard = pv_common_compact.index(
    "begin_attack_continuation(ch,opponent,wpn)"
)
pv_defense_callbacks = (
    "mangleSucceed(opponent,ch,wpn)",
    "parrySucceed(opponent,ch,wpn)",
    "divine_blessing_parry(opponent,ch)",
    "blockSucceed(opponent,ch,wpn)",
    "dodgeSucceed(opponent,ch,wpn)",
    "leapSucceed(opponent,ch)",
    "MonkRiposte(opponent,ch,wpn)",
)
pv_callback_offset = pv_common_guard
for pv_callback in pv_defense_callbacks:
    pv_callback_offset = pv_common_compact.index(pv_callback, pv_callback_offset)
    pv_callback_check = pv_common_compact.index(
        "if(!refresh_attack_participants())returnFALSE;", pv_callback_offset
    )
    assert pv_callback_offset < pv_callback_check
    pv_callback_offset = pv_callback_check + 1
pv_item_callback = pv_common_compact.index(
    "invoke_object_special(item,opponent,CMD_GOTHIT,(char*)&data)"
)
pv_item_check = pv_common_compact.index(
    "if(!refresh_attack_participants())returnFALSE;", pv_item_callback
)
pv_mob_callback = pv_common_compact.index(
    "(*mob_index[GET_RNUM(opponent)].func.mob)(opponent,ch,CMD_GOTHIT,(char*)&data)"
)
pv_mob_check = pv_common_compact.index(
    "if(!refresh_attack_participants())returnFALSE;", pv_mob_callback
)
pv_hit = pv_common_compact.index("hit(ch,opponent,wpn,damAccumulator)")
pv_hit_check = pv_common_compact.index(
    "if(!refresh_attack_participants())returnsuccess;", pv_hit
)
pv_armlock = pv_common_compact.index("armlock_check(ch,opponent)", pv_hit_check)
pv_armlock_check = pv_common_compact.index(
    "if(!refresh_attack_participants())returnsuccess;", pv_armlock
)
pv_zealot = pv_common_compact.index("GET_SPEC(ch,CLASS_CLERIC,SPEC_ZEALOT)")
assert (
    pv_common_guard
    < pv_item_callback
    < pv_item_check
    < pv_mob_callback
    < pv_mob_check
    < pv_hit
    < pv_hit_check
    < pv_armlock
    < pv_armlock_check
    < pv_zealot
)

sevenoaks_longsword = continuation_caller(
    "specs.underworld.c",
    "int sevenoaks_longsword(P_obj obj, P_char ch, int cmd, char *arg)",
)
sevenoaks_compact = "".join(sevenoaks_longsword.split())
assert sevenoaks_compact.index("!char_in_list(ch)") < sevenoaks_compact.index(
    "!IS_ALIVE(ch)"
)
assert sevenoaks_compact.index("!char_in_list(vict)") < sevenoaks_compact.index(
    "!IS_ALIVE(vict)"
)
sevenoaks_first_hit = sevenoaks_compact.index("hit(ch,vict,obj)")
sevenoaks_first_hit_guard = sevenoaks_compact.rindex(
    "begin_attack_continuation(ch,vict,obj)", 0, sevenoaks_first_hit
)
sevenoaks_first_hit_check = sevenoaks_compact.index(
    "check_attack_continuation(hit_continuation)", sevenoaks_first_hit
)
sevenoaks_first_actor_refresh = sevenoaks_compact.index(
    "ch=after_hit.actor", sevenoaks_first_hit_check
)
sevenoaks_first_target_refresh = sevenoaks_compact.index(
    "vict=after_hit.target", sevenoaks_first_actor_refresh
)
sevenoaks_first_weapon_refresh = sevenoaks_compact.index(
    "obj=after_hit.weapon", sevenoaks_first_target_refresh
)
sevenoaks_spell = sevenoaks_compact.index(
    "spell_damage(ch,vict,dice(10,24),SPLDAM_COLD,"
)
sevenoaks_spell_result = sevenoaks_compact.index("!=DAM_NONEDEAD", sevenoaks_spell)
sevenoaks_spell_handled_return = sevenoaks_compact.index(
    "returnTRUE;", sevenoaks_spell_result
)
sevenoaks_final_return = sevenoaks_compact.rindex("returnFALSE;")
assert sevenoaks_compact.count("hit(ch,vict,obj)") == 1
assert (
    sevenoaks_first_hit_guard
    < sevenoaks_first_hit
    < sevenoaks_first_hit_check
    < sevenoaks_first_actor_refresh
    < sevenoaks_first_target_refresh
    < sevenoaks_first_weapon_refresh
    < sevenoaks_spell
    < sevenoaks_spell_result
    < sevenoaks_spell_handled_return
    < sevenoaks_final_return
)

doombringer = continuation_caller(
    "specs.underworld.c",
    "int doombringer(P_obj obj, P_char ch, int cmd, char *arg)"
)
doombringer_compact = "".join(doombringer.split())
assert doombringer_compact.index("!char_in_list(ch)") < doombringer_compact.index(
    "!IS_ALIVE(ch)"
)
assert doombringer_compact.index("!char_in_list(vict)") < doombringer_compact.index(
    "!IS_ALIVE(vict)"
)
doombringer_spells = [
    doombringer_compact.index("spell_damage(ch,vict,number(100,200)," + kind)
    for kind in ("SPLDAM_LIGHTNING", "SPLDAM_FIRE", "SPLDAM_COLD")
]
doombringer_refreshes = [
    doombringer_compact.index(
        "refresh_doombringer_continuation(continuation)", spell
    )
    for spell in doombringer_spells
]
assert doombringer_spells == sorted(doombringer_spells)
assert all(
    spell < refresh
    for spell, refresh in zip(doombringer_spells, doombringer_refreshes)
)
doombringer_hit = doombringer_compact.index("hit(ch,vict,obj)")
doombringer_hit_check = doombringer_compact.index(
    "refresh_doombringer_continuation(continuation)", doombringer_hit
)
assert doombringer_compact.count("hit(ch,vict,obj)") == 1
assert doombringer_hit < doombringer_hit_check
assert doombringer_refreshes[-1] < doombringer_compact.index(
    "for(i=0;i<3;i++)", doombringer_refreshes[-1]
)

barb = continuation_caller("specs.underworld.c", "int barb(P_obj obj, P_char ch, int cmd, char *arg)")
barb_compact = "".join(barb.split())
barb_melee = barb_compact[barb_compact.index("if(cmd!=CMD_MELEE_HIT)") :]
assert barb_melee.index("char_in_list(ch)") < barb_melee.index("IS_ALIVE(ch)")
assert barb_melee.index("char_in_list(vict)") < barb_melee.index("IS_ALIVE(vict)")
barb_chain = barb_melee.index("spell_chain_lightning")
barb_chain_check = barb_melee.index(
    "refresh_barb_continuation(continuation)", barb_chain
)
barb_forked = barb_melee.index("spell_forked_lightning", barb_chain_check)
barb_forked_check = barb_melee.index(
    "refresh_barb_continuation(continuation)", barb_forked
)
barb_hits = [m.start() for m in re.finditer(r"hit\(ch,vict,obj\)", barb_melee)]
assert len(barb_hits) == 3
for hit_position in barb_hits:
    check_position = barb_melee.index(
        "refresh_barb_continuation(continuation)", hit_position
    )
    assert check_position > hit_position
assert barb_chain < barb_chain_check < barb_forked < barb_forked_check < barb_hits[0]

barb_nuked = barb_compact[
    barb_compact.index("if((cmd==CMD_GOTNUKED)") : barb_compact.index(
        "if(cmd!=CMD_MELEE_HIT)"
    )
]
barb_nuked_damage = barb_nuked.index(
    "result=spell_damage(ch,vict,BarbProcData.damage"
)
barb_attack_back = barb_nuked.index("attack_back(vict,ch,FALSE)", barb_nuked_damage)
assert (
    "if(result==DAM_NONEDEAD&&refresh_barb_continuation(continuation))"
    in barb_nuked[barb_nuked_damage:barb_attack_back]
)

barb_dwarven = barb_melee[
    barb_melee.index("elseif((!number(0,49)") : barb_melee.index(
        "elseif(!number(0,24)&&GET_CLASS(ch,CLASS_BERSERKER)"
    )
]
barb_dwarven_damage = barb_dwarven.index(
    "result=spell_damage(ch,vict,BarbProcData.damage"
)
barb_dwarven_death = barb_dwarven.index(
    "if(result==DAM_VICTDEAD)", barb_dwarven_damage
)
assert "find_character_by_runtime_id(actor_runtime_id)" in barb_dwarven[
    barb_dwarven_death : barb_dwarven.index("returnDAM_VICTDEAD", barb_dwarven_death)
]
barb_dwarven_refresh = barb_dwarven.index(
    "if(!refresh_barb_continuation(continuation))", barb_dwarven_damage
)
assert barb_dwarven_refresh > barb_dwarven_damage
barb_dwarven_return = barb_dwarven.index("returnTRUE", barb_dwarven_refresh)
barb_dwarven_update = barb_dwarven.index("update_pos(vict)", barb_dwarven_refresh)
assert barb_dwarven_return < barb_dwarven_update

barb_berserker = barb_melee[
    barb_melee.index("elseif(!number(0,24)&&GET_CLASS(ch,CLASS_BERSERKER)") :
]
barb_berserker_damage = barb_berserker.index("spell_damage(ch,vict,200")
barb_berserker_refresh = barb_berserker.index(
    "if(!refresh_barb_continuation(continuation))", barb_berserker_damage
)
assert barb_berserker_refresh > barb_berserker_damage

blur_shortsword = continuation_caller(
    "specs.winterhaven.c",
    "int blur_shortsword(P_obj obj, P_char ch, int cmd, char *arg)"
)
blur_compact = "".join(blur_shortsword.split())
assert blur_compact.index("!char_in_list(ch)") < blur_compact.index("!IS_ALIVE(ch)")
assert blur_compact.index("!char_in_list(vict)") < blur_compact.index("!IS_ALIVE(vict)")
blur_say = blur_compact[blur_compact.index("if(arg&&(cmd==CMD_SAY))") : blur_compact.index(
    "if(cmd==CMD_GOTHIT"
)]
assert "for(intstrike=0;strike<3;++strike)" in blur_say
blur_say_hit = blur_say.index("hit(ch,vict,obj)")
blur_say_hit_guard = blur_say.rindex(
    "begin_attack_continuation(ch,vict,obj)", 0, blur_say_hit
)
blur_say_hit_check = blur_say.index("refresh_blur_continuation(continuation)", blur_say_hit)
assert blur_say_hit_guard < blur_say_hit < blur_say_hit_check
assert blur_say.index("switch(number(0,3))") < blur_say.rindex("set_blur_cooldown()")
blur_gothit = blur_compact[blur_compact.index("if(cmd==CMD_GOTHIT") :]
blur_gothit_hit = blur_gothit.index("hit(ch,vict,obj)")
assert blur_gothit.rindex(
    "begin_attack_continuation(ch,vict,obj)", 0, blur_gothit_hit
) < blur_gothit_hit < blur_gothit.index(
    "refresh_blur_continuation(hit_continuation)", blur_gothit_hit
)
blur_double_spell = blur_gothit.index("spell_chill_touch")
assert blur_gothit.index(
    "refresh_blur_continuation(continuation)", blur_double_spell
) < blur_gothit.index("spell_chill_touch", blur_double_spell + 1)

fumblegaunts = continuation_caller(
    "specs.heavens.c",
    "int fumblegaunts(P_obj obj, P_char ch, int cmd, char * /*arg*/)"
)
fumblegaunts_compact = "".join(fumblegaunts.split())
assert fumblegaunts_compact.index("!char_in_list(ch)") < fumblegaunts_compact.index(
    "!IS_ALIVE(ch)"
)
assert fumblegaunts_compact.index("!char_in_list(vict)") < fumblegaunts_compact.index(
    "!IS_ALIVE(vict)"
)
fumblegaunts_hit = fumblegaunts_compact.index(
    "hit(ch,vict,ch->equipment[PRIMARY_WEAPON])"
)
fumblegaunts_guard = fumblegaunts_compact.rindex(
    "begin_attack_continuation(ch,vict)", 0, fumblegaunts_hit
)
fumblegaunts_check = fumblegaunts_compact.index(
    "check_attack_continuation(continuation)", fumblegaunts_hit
)
assert fumblegaunts_compact.count("hit(ch,vict,ch->equipment[PRIMARY_WEAPON])") == 1
assert (
    fumblegaunts_compact.index("for(intstrike=0;strike<5;++strike)")
    < fumblegaunts_guard
    < fumblegaunts_hit
    < fumblegaunts_check
    < fumblegaunts_compact.index("ch=after_hit.actor", fumblegaunts_check)
)

good_evil_sword = continuation_caller(
    "specs.heavens.c",
    "int good_evil_sword(P_obj obj, P_char ch, int cmd, char *arg)"
)
good_evil_compact = "".join(good_evil_sword.split())
assert good_evil_compact.index("!char_in_list(ch)") < good_evil_compact.index(
    "!IS_ALIVE(ch)"
)
assert good_evil_compact.index("num_attacks=number(3,5)") < good_evil_compact.index(
    "!char_in_list(victim)"
)
assert good_evil_compact.index("!char_in_list(victim)") < good_evil_compact.index(
    "!IS_ALIVE(victim)"
)
good_evil_hit = good_evil_compact.index("hit(ch,victim,obj)")
good_evil_guard = good_evil_compact.rindex(
    "begin_attack_continuation(ch,victim,obj)", 0, good_evil_hit
)
good_evil_check = good_evil_compact.index(
    "check_attack_continuation(continuation)", good_evil_hit
)
assert good_evil_compact.count("hit(ch,victim,obj)") == 1
assert (
    good_evil_compact.index("for(i=0;i<num_attacks;i++)")
    < good_evil_guard
    < good_evil_hit
    < good_evil_check
    < good_evil_compact.index("ch=after_hit.actor", good_evil_check)
)

barrage = continuation_caller(
    "actoff.c", "void event_barrage(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)"
)
barrage_actor_guard = barrage.index("if (!char_in_list(ch) || !IS_ALIVE(ch))")
barrage_opponent = barrage.index("victim = GET_OPPONENT(ch)")
barrage_hit = barrage.index("hit(ch, victim, continuation.weapon)")
barrage_check = barrage.index("check_attack_continuation(continuation)", barrage_hit)
assert barrage_actor_guard < barrage_opponent
assert barrage.index("if (!char_in_list(victim) || !IS_ALIVE(victim))") < barrage.index(
    "victim = guard_check(ch, victim)"
)
assert barrage_hit < barrage_check < barrage.index("notch_skill(ch, SKILL_BLADE_BARRAGE")
assert "if (!(IS_ALIVE(ch) && IS_ALIVE(victim)))" not in barrage

sweeping = continuation_caller(
    "actoff.c", "void do_sweeping_thrust(P_char ch, char *argument, int /*cmd*/)"
)
sweep_helper = sweeping.index("auto hit_and_engage")
sweep_hit = sweeping.index("hit(ch, victim, weapon)", sweep_helper)
sweep_check = sweeping.index("check_attack_continuation(continuation)", sweep_hit)
sweep_engage = sweeping.index("engage(ch, victim)", sweep_check)
assert sweep_helper < sweep_hit < sweep_check < sweep_engage
assert sweeping.count("hit(ch, victim, weapon)") == 1
assert sweeping.count("hit_and_engage(PRIMARY_WEAPON)") == 2
assert sweeping.count("hit_and_engage(SECONDARY_WEAPON)") == 1

backstab = continuation_caller("actoff.c", "bool backstab(P_char ch, P_char victim)")
backstab_guard = backstab.index("auto revalidate_backstab_participants")
backstab_first_hit = backstab.index("hit(ch, victim, first_w)")
backstab_first_check = backstab.index(
    "if (!revalidate_backstab_participants())", backstab_first_hit
)
backstab_weapon_refresh = backstab.index(
    "second_w = ch->equipment[SECONDARY_WEAPON]", backstab_first_check
)
backstab_second_hit = backstab.index("hit(ch, victim, second_w)", backstab_weapon_refresh)
backstab_final_check = backstab.rindex("if (!revalidate_backstab_participants())")
assert "find_character_by_runtime_id(actor_runtime_id)" in backstab
assert "find_character_by_runtime_id(victim_runtime_id)" in backstab
assert backstab_guard < backstab_first_hit < backstab_first_check < backstab_weapon_refresh
assert backstab_weapon_refresh < backstab_second_hit < backstab_final_check
assert backstab_final_check < backstab.index("if (IS_PC(victim))")

single_stab = continuation_caller(
    "actoff.c", "bool single_stab(P_char ch, P_char victim, P_obj weapon)"
)
assert single_stab.index("!char_in_list(ch)") < single_stab.index("!IS_ALIVE(ch)")
assert "initial_continuation" in single_stab
assert "begin_attack_continuation(ch, victim, weapon)" in single_stab
stab_damage_positions = []
stab_offset = 0
while True:
    try:
        stab_offset = single_stab.index("melee_damage(ch, victim", stab_offset)
    except ValueError:
        break
    stab_damage_positions.append(stab_offset)
    stab_offset += 1
assert len(stab_damage_positions) == 4
for stab_damage in stab_damage_positions:
    assert single_stab.rfind("begin_attack_continuation(ch, victim, weapon)", 0, stab_damage) >= 0
    assert single_stab.index("refresh_stab_participants(continuation)", stab_damage) > stab_damage
stab_object_proc = single_stab.index("invoke_object_special(weapon,")
stab_object_check = single_stab.index("refresh_stab_participants(continuation)", stab_object_proc)
stab_poison = single_stab.index("(skills[poison].spell_pointer)")
stab_poison_check = single_stab.index("refresh_stab_participants(continuation)", stab_poison)
assert stab_object_proc < stab_object_check < single_stab.index("if (weapon->value[4])")
assert stab_poison < stab_poison_check < single_stab.index("weapon->value[4] = 0")

mob_warrior = continuation_caller("mobact.c", "bool MobWarrior(P_char ch)")
assert mob_warrior.index("!char_in_list(ch)") < mob_warrior.index("!IS_ALIVE(ch)")
warrior_loop = mob_warrior[
    mob_warrior.index("for (tch = world[ch->in_room].people; tch; tch = next_ch)") :
]
warrior_compact = "".join(warrior_loop.split())
warrior_hit = warrior_compact.index("hit(ch,tch,ch->equipment[PRIMARY_WEAPON])")
assert warrior_compact.index("next_ch_runtime_id=next_ch?next_ch->runtime_id:0") < warrior_hit
assert warrior_compact.index("begin_attack_continuation(ch,ch)") < warrior_hit
warrior_actor_check = warrior_compact.index("check_attack_continuation(actor_continuation)", warrior_hit)
warrior_next_check = warrior_compact.index(
    "find_character_by_runtime_id(next_ch_runtime_id)", warrior_actor_check
)
assert warrior_hit < warrior_actor_check < warrior_next_check

mob_retaliate = continuation_caller(
    "sparser.c", "bool check_mob_retaliate(P_char ch, P_char tar_char, int spl)"
)
retaliate_compact = "".join(mob_retaliate.split())
assert retaliate_compact.index("caster_in_list=ch&&char_in_list(ch)") < retaliate_compact.index(
    "caster_alive=caster_in_list&&IS_ALIVE(ch)"
)
retaliation_hit = retaliate_compact.index("hit(tch,ch,tch->equipment[PRIMARY_WEAPON])")
retaliation_caster_check = retaliate_compact.index(
    "find_character_by_runtime_id(caster_runtime_id)", retaliation_hit
)
retaliation_next_check = retaliate_compact.index(
    "find_character_by_runtime_id(tch2_runtime_id)", retaliation_caster_check
)
assert retaliate_compact.rfind(
    "tch2_runtime_id=tch2?tch2->runtime_id:0", 0, retaliation_hit
) >= 0
assert retaliation_hit < retaliation_caster_check < retaliation_next_check
assert retaliate_compact.count("find_character_by_runtime_id(caster_runtime_id)") == 2

wind_blade = continuation_caller(
    "ethermancer.c", "void wind_blade_attack_routine(P_char ch, P_char victim)"
)
wind_blade_compact = "".join(wind_blade.split())
assert wind_blade_compact.index("!char_in_list(ch)") < wind_blade_compact.index("!IS_ALIVE(ch)")
assert wind_blade_compact.index("!char_in_list(victim)") < wind_blade_compact.index(
    "!IS_ALIVE(victim)"
)
wind_blade_hit = wind_blade_compact.index("hit(ch,victim,obj)")
wind_blade_check = wind_blade_compact.index(
    "check_attack_continuation(continuation)", wind_blade_hit
)
assert wind_blade_compact.index("begin_attack_continuation(ch,victim,obj,PRIMARY_WEAPON)") < (
    wind_blade_hit
)
assert wind_blade_hit < wind_blade_check
assert "ch=after_hit.actor;victim=after_hit.target;obj=after_hit.weapon;" in wind_blade_compact
assert "find_character_by_runtime_id(actor_runtime_id)" in wind_blade_compact
assert "affected_by_spell(live_actor,SPELL_WIND_BLADE)" in wind_blade_compact

rapier_dirk = continuation_caller(
    "defense_resolution.c", "static bool rapier_dirk(P_char victim, P_char attacker)"
)
rapier_compact = "".join(rapier_dirk.split())
assert rapier_compact.index("!char_in_list(victim)") < rapier_compact.index("!IS_ALIVE(victim)")
assert rapier_compact.index("!char_in_list(attacker)") < rapier_compact.index(
    "!IS_ALIVE(attacker)"
)
rapier_hit_positions = []
rapier_offset = 0
while True:
    try:
        rapier_offset = rapier_compact.index("hit(victim,attacker,wep1)", rapier_offset)
    except ValueError:
        break
    rapier_hit_positions.append(rapier_offset)
    rapier_offset += 1
assert len(rapier_hit_positions) == 3
for rapier_hit in rapier_hit_positions:
    assert rapier_compact.rfind(
        "begin_attack_continuation(victim,attacker,wep1,PRIMARY_WEAPON)", 0, rapier_hit
    ) >= 0
    assert rapier_compact.index("refresh_dirk_participants(", rapier_hit) > rapier_hit
assert rapier_compact.count("IS_ALIVE(victim)") == 1
assert rapier_compact.count("IS_ALIVE(attacker)") == 1

parry_succeed = continuation_caller(
    "defense_resolution.c", "int parrySucceed(P_char victim, P_char attacker, P_obj wpn)"
)
parry_compact = "".join(parry_succeed.split())
assert parry_compact.index("!char_in_list(victim)") < parry_compact.index("!IS_ALIVE(victim)")
assert parry_compact.index("!char_in_list(attacker)") < parry_compact.index("!IS_ALIVE(attacker)")
assert parry_compact.index("learnedvictim=GET_CHAR_SKILL(victim,SKILL_PARRY)") > parry_compact.index(
    "!IS_ALIVE(attacker)"
)

hyena_bite = continuation_caller(
    "specs.monster_attacks.c", "void hyena_bite(P_char ch, P_char victim)"
)
hyena_compact = "".join(hyena_bite.split())
assert hyena_compact.index("!char_in_list(ch)") < hyena_compact.index("!IS_ALIVE(ch)")
assert hyena_compact.index("!char_in_list(victim)") < hyena_compact.index("!IS_ALIVE(victim)")
hyena_damage = hyena_compact.index("raw_damage(ch,victim,dam,RAWDAM_DEFAULT,&messages)")
hyena_damage_check = hyena_compact.index(
    "refresh_bite_participants(bite_continuation)", hyena_damage
)
hyena_disease = hyena_compact.index("spell_disease(GET_LEVEL(ch),ch,0,0,victim,0)")
hyena_disease_check = hyena_compact.index(
    "refresh_bite_participants(disease_continuation)", hyena_disease
)
assert hyena_compact.index("begin_attack_continuation(ch,victim)") < hyena_damage
assert hyena_damage < hyena_damage_check < hyena_disease
assert hyena_compact.rfind("begin_attack_continuation(ch,victim)", 0, hyena_disease) > hyena_damage
assert hyena_disease < hyena_disease_check

bite_poison = continuation_caller(
    "innates.c", "int bite_poison(P_char ch, P_char victim, int mod)"
)
bite_poison_compact = "".join(bite_poison.split())
assert bite_poison_compact.index("!char_in_list(ch)") < bite_poison_compact.index("!IS_ALIVE(ch)")
assert bite_poison_compact.index("!char_in_list(victim)") < bite_poison_compact.index(
    "!IS_ALIVE(victim)"
)
bite_spell = bite_poison_compact.index("spell_pointer)(GET_LEVEL(ch),ch,0,0,victim,0)")
bite_spell_check = bite_poison_compact.index("refresh_bite_pair(", bite_spell)
assert bite_spell < bite_spell_check < bite_poison_compact.index("act(", bite_spell_check)
assert bite_spell_check < bite_poison_compact.index("send_to_char(", bite_spell_check)

for bite_signature in (
    "void insectbite(P_char ch, P_char victim)",
    "void event_snakebite(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)",
):
    bite = continuation_caller("innates.c", bite_signature)
    bite_compact = "".join(bite.split())
    assert bite_compact.index("!char_in_list(ch)") < bite_compact.index("!IS_ALIVE(ch)")
    assert bite_compact.index("!char_in_list(victim)") < bite_compact.index(
        "!IS_ALIVE(victim)"
    )
    bite_damage = bite_compact.index("raw_damage(ch,victim,")
    bite_damage_check = bite_compact.index("refresh_bite_pair(", bite_damage)
    assert bite_compact.index("ch_runtime_id=ch->runtime_id") < bite_damage
    assert bite_compact.index("victim_runtime_id=victim->runtime_id") < bite_damage
    assert bite_damage < bite_damage_check < bite_compact.index("bite_poison(ch,victim,i)")
    bite_wait = bite_compact.rindex("CharWait(ch,PULSE_VIOLENCE)")
    assert bite_compact.rfind("find_character_by_runtime_id(ch_runtime_id)", 0, bite_wait) > (
        bite_damage_check
    )

tainted_event = continuation_caller(
    "attack_effects.c",
    "void event_tainted_blade(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)",
)
tainted_event_compact = "".join(tainted_event.split())
assert tainted_event_compact.index("!char_in_list(ch)") < tainted_event_compact.index(
    "!IS_ALIVE(ch)"
)
assert tainted_event_compact.index("!char_in_list(victim)") < tainted_event_compact.index(
    "!IS_ALIVE(victim)"
)
tainted_event_damage = tainted_event_compact.index("raw_damage(ch,victim,")
tainted_event_pair_check = tainted_event_compact.index(
    "refresh_tainted_blade_pair(", tainted_event_damage
)
tainted_event_affect_refresh = tainted_event_compact.index(
    "af=get_spell_from_char(victim,blade_skill)", tainted_event_pair_check
)
assert tainted_event_damage < tainted_event_pair_check < tainted_event_affect_refresh
assert tainted_event_affect_refresh < tainted_event_compact.index("af->modifier--")

tainted_apply = continuation_caller(
    "attack_effects.c", "bool tainted_blade(P_char ch, P_char victim)"
)
tainted_apply_compact = "".join(tainted_apply.split())
assert tainted_apply_compact.index("!char_in_list(ch)") < tainted_apply_compact.index(
    "!IS_ALIVE(ch)"
)
assert tainted_apply_compact.index("!char_in_list(victim)") < tainted_apply_compact.index(
    "!IS_ALIVE(victim)"
)
tainted_apply_damage = tainted_apply_compact.index(
    "raw_damage(ch,victim,60,RAWDAM_DEFAULT,messages)"
)
tainted_apply_pair_check = tainted_apply_compact.index(
    "refresh_tainted_blade_pair(", tainted_apply_damage
)
tainted_apply_affect = tainted_apply_compact.index(
    "get_spell_from_char(victim,blade_skill)", tainted_apply_pair_check
)
assert tainted_apply_damage < tainted_apply_pair_check < tainted_apply_affect

weapon_proc = continuation_caller(
    "attack_effects.c", "bool weapon_proc(P_obj obj, P_char ch, P_char victim)"
)
weapon_proc_compact = "".join(weapon_proc.split())
single_spell_start = weapon_proc_compact.index("if(obj->value[5]>999999999)")
legacy_spell_start = weapon_proc_compact.index(
    "else{while(count--", single_spell_start
)
single_spell_branch = weapon_proc_compact[single_spell_start:legacy_spell_start]
assert "begin_attack_continuation" not in single_spell_branch
legacy_spell_end = weapon_proc_compact.rindex("returnTRUE;") + len("returnTRUE;")
legacy_spell_loop = weapon_proc_compact[legacy_spell_start:legacy_spell_end]
legacy_spell_guard = legacy_spell_loop.index("begin_attack_continuation(ch,victim,obj)")
legacy_aggregate_call = legacy_spell_loop.index(
    "SPELL_TYPE_SPELL,victim,obj", legacy_spell_guard
)
legacy_self_call = legacy_spell_loop.index(
    "SPELL_TYPE_SPELL,ch,obj", legacy_aggregate_call
)
legacy_spell_check = legacy_spell_loop.index(
    "check_attack_continuation(continuation)", legacy_self_call
)
legacy_spell_stop = legacy_spell_loop.index(
    "if(!after_spell.can_continue())returnTRUE;", legacy_spell_check
)
legacy_actor_refresh = legacy_spell_loop.index(
    "ch=after_spell.actor", legacy_spell_stop
)
legacy_target_refresh = legacy_spell_loop.index(
    "victim=after_spell.target", legacy_actor_refresh
)
legacy_weapon_refresh = legacy_spell_loop.index(
    "obj=after_spell.weapon", legacy_target_refresh
)
assert (
    legacy_spell_guard
    < legacy_aggregate_call
    < legacy_self_call
    < legacy_spell_check
    < legacy_spell_stop
    < legacy_actor_refresh
    < legacy_target_refresh
    < legacy_weapon_refresh
)

poison_heart = continuation_caller(
    "handler.c",
    "void poison_heart_toxin(int level, P_char ch, char * /*arg*/",
)
poison_heart_compact = "".join(poison_heart.split())
assert poison_heart_compact.index("!char_in_list(victim)") < poison_heart_compact.index(
    "!IS_ALIVE(victim)"
)
poison_heart_ch_member = poison_heart_compact.index("if(ch&&!char_in_list(ch))ch=NULL")
assert poison_heart_ch_member < poison_heart_compact.index("if(ch)level=GET_LEVEL(ch)")
poison_heart_damage = poison_heart_compact.index("raw_damage(ch,victim,")
poison_heart_victim = poison_heart_compact.index(
    "victim=find_character_by_runtime_id(victim_runtime_id)", poison_heart_damage
)
poison_heart_affect = poison_heart_compact.index(
    "af=get_spell_from_char(victim,POISON_HEART_TOXIN)", poison_heart_victim
)
poison_heart_attacker = poison_heart_compact.index(
    "ch=find_character_by_runtime_id(ch_runtime_id)", poison_heart_victim
)
assert poison_heart_damage < poison_heart_victim < poison_heart_attacker < poison_heart_affect
assert "af->" not in poison_heart_compact[poison_heart_damage:poison_heart_affect]
assert poison_heart_affect < poison_heart_compact.index(
    "affect_remove(victim,af)", poison_heart_affect
)

illesarus = continuation_caller(
    "specs.hoa.c", "int illesarus(P_obj obj, P_char ch, int cmd, char *arg)"
)
illesarus_compact = "".join(illesarus.split())
assert illesarus_compact.index("!char_in_list(ch)") < illesarus_compact.index(
    "!IS_ALIVE(ch)"
)
assert illesarus_compact.index("!char_in_list(vict)") < illesarus_compact.index(
    "!IS_ALIVE(vict)"
)
illesarus_damage = illesarus_compact.index("damage_result=raw_damage(ch,vict,")
illesarus_result_check = illesarus_compact.index(
    "if(damage_result!=DAM_NONEDEAD)returnTRUE;", illesarus_damage
)
illesarus_actor = illesarus_compact.index(
    "ch=find_character_by_runtime_id(ch_runtime_id)", illesarus_result_check
)
illesarus_target = illesarus_compact.index(
    "vict=find_character_by_runtime_id(victim_runtime_id)", illesarus_actor
)
illesarus_object = illesarus_compact.index(
    "obj=find_illesarus_object_by_uid(object_uid)", illesarus_target
)
illesarus_severing = illesarus_compact.index("limb=number(0,LAST_LIMB)", illesarus_object)
assert (
    illesarus_damage
    < illesarus_result_check
    < illesarus_actor
    < illesarus_target
    < illesarus_object
    < illesarus_severing
)

def assert_grapple_membership_before_alive(block, character_names):
    compact = "".join(block.split())
    for character in character_names:
        assert compact.index(f"!char_in_list({character})") < compact.index(
            f"!IS_ALIVE({character})"
        )


grapple_refresh = continuation_caller("grapple.c", "static bool refresh_grapple_pair(")
grapple_refresh_compact = "".join(grapple_refresh.split())
assert "check_attack_continuation(continuation)" in grapple_refresh_compact
assert "if(!after_callback.can_continue())returnfalse;" in grapple_refresh_compact
assert "actor=after_callback.actor;victim=after_callback.target;" in grapple_refresh_compact

headlock = continuation_caller(
    "grapple.c", "void event_headlock(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)"
)
assert_grapple_membership_before_alive(headlock, ("ch", "victim"))
headlock_compact = "".join(headlock.split())
headlock_damage = headlock_compact.index("raw_damage(ch,victim,")
headlock_refresh = headlock_compact.index(
    "refresh_grapple_pair(ch,victim,continuation)", headlock_damage
)
headlock_notch = headlock_compact.index("notch_skill(ch,SKILL_HEADLOCK", headlock_refresh)
headlock_notch_refresh = headlock_compact.index(
    "refresh_grapple_pair(ch,victim,continuation)", headlock_notch
)
headlock_shields = headlock_compact.index("check_shields(ch,victim,", headlock_notch_refresh)
headlock_shield_refresh = headlock_compact.index(
    "refresh_grapple_pair(ch,victim,continuation)", headlock_shields
)
headlock_next = headlock_compact.index("add_event(event_headlock,", headlock_shield_refresh)
assert (
    headlock_damage
    < headlock_refresh
    < headlock_notch
    < headlock_notch_refresh
    < headlock_shields
    < headlock_shield_refresh
    < headlock_next
)

armlock = continuation_caller(
    "grapple.c", "void armlock_check(P_char attacker, P_char grappler)"
)
assert_grapple_membership_before_alive(armlock, ("attacker", "grappler"))
armlock_compact = "".join(armlock.split())
armlock_damage_positions = []
armlock_offset = 0
while True:
    try:
        armlock_offset = armlock_compact.index("raw_damage(grappler,attacker,", armlock_offset)
    except ValueError:
        break
    armlock_damage_positions.append(armlock_offset)
    armlock_offset += 1
assert len(armlock_damage_positions) == 2
for index, armlock_damage in enumerate(armlock_damage_positions):
    continuation_name = "continuation" if index == 0 else "break_continuation"
    refresh_call = f"refresh_grapple_pair(grappler,attacker,{continuation_name})"
    armlock_pair = armlock_compact.index(refresh_call, armlock_damage)
    armlock_shields = armlock_compact.index("check_shields(grappler,attacker,", armlock_pair)
    armlock_shield_pair = armlock_compact.index(
        refresh_call, armlock_shields
    )
    assert armlock_damage < armlock_pair < armlock_shields < armlock_shield_pair

leglock = continuation_caller(
    "grapple.c", "void event_leglock(P_char ch, P_char victim, P_obj /*obj*/, void * /*data*/)"
)
assert_grapple_membership_before_alive(leglock, ("ch", "victim"))
leglock_compact = "".join(leglock.split())
leglock_damage_positions = []
leglock_offset = 0
while True:
    try:
        leglock_offset = leglock_compact.index("raw_damage(ch,victim,", leglock_offset)
    except ValueError:
        break
    leglock_damage_positions.append(leglock_offset)
    leglock_offset += 1
assert len(leglock_damage_positions) == 2
for leglock_damage in leglock_damage_positions:
    leglock_pair = leglock_compact.index(
        "refresh_grapple_pair(ch,victim,continuation)", leglock_damage
    )
    leglock_notch = leglock_compact.index("notch_skill(ch,SKILL_LEGLOCK", leglock_pair)
    leglock_notch_pair = leglock_compact.index(
        "refresh_grapple_pair(ch,victim,continuation)", leglock_notch
    )
    leglock_shields = leglock_compact.index("check_shields(ch,victim,", leglock_notch_pair)
    leglock_shield_pair = leglock_compact.index(
        "refresh_grapple_pair(ch,victim,continuation)", leglock_shields
    )
    assert (
        leglock_damage
        < leglock_pair
        < leglock_notch
        < leglock_notch_pair
        < leglock_shields
        < leglock_shield_pair
    )

groundslam = continuation_caller("grapple.c", "void do_groundslam(P_char ch, char *argument")
groundslam_compact = "".join(groundslam.split())
assert groundslam_compact.index("!char_in_list(ch)") < groundslam_compact.index(
    "!IS_ALIVE(ch)"
)
groundslam_damage = groundslam_compact.index("raw_damage(ch,victim,")
groundslam_pair = groundslam_compact.index(
    "refresh_grapple_pair(ch,victim,continuation)", groundslam_damage
)
groundslam_shields = groundslam_compact.index("check_shields(ch,victim,", groundslam_pair)
groundslam_shield_pair = groundslam_compact.index(
    "refresh_grapple_pair(ch,victim,continuation)", groundslam_shields
)
groundslam_position = groundslam_compact.index("SET_POS(ch,POS_PRONE+GET_STAT(ch))", groundslam_shield_pair)
assert groundslam_damage < groundslam_pair < groundslam_shields < groundslam_shield_pair < groundslam_position

spell_damage = continuation_caller(
    "fight.c", "int spell_damage(P_char ch, P_char victim, double dam, int type, uint flags,"
)
spell_damage_compact = "".join(spell_damage.split())
spell_null_guard = spell_damage_compact.index("if(!ch||!victim)returnDAM_NONEDEAD;")
spell_actor_membership = spell_damage_compact.index(
    "constboolch_listed=char_in_list(ch);", spell_null_guard
)
spell_victim_membership = spell_damage_compact.index(
    "constboolvictim_listed=char_in_list(victim);", spell_actor_membership
)
spell_both_missing = spell_damage_compact.index(
    "if(!ch_listed&&!victim_listed)returnDAM_BOTHDEAD;", spell_victim_membership
)
spell_actor_missing = spell_damage_compact.index(
    "if(!ch_listed)returnDAM_CHARDEAD;", spell_both_missing
)
spell_victim_missing = spell_damage_compact.index(
    "if(!victim_listed)returnDAM_VICTDEAD;", spell_actor_missing
)
spell_training_dummy = spell_damage_compact.index(
    "if(training_dummy_is(ch))", spell_victim_missing
)
assert spell_null_guard < spell_actor_membership < spell_victim_membership
assert spell_victim_membership < spell_both_missing < spell_actor_missing
assert spell_actor_missing < spell_victim_missing < spell_training_dummy
spell_raw_damage = spell_damage_compact.index("result=raw_damage(ch,victim,dam,")
assert spell_damage_compact.index(
    "constuint64_tch_runtime_id=ch->runtime_id"
) < spell_raw_damage
assert spell_damage_compact.index(
    "constuint64_tvictim_runtime_id=victim->runtime_id"
) < spell_raw_damage
spell_acid_roll = spell_damage_compact.index(
    "constboolacid_item_damage=type==SPLDAM_ACID&&!number(0,3)", spell_raw_damage
)
spell_acid_lookup = spell_damage_compact.index(
    "P_characid_victim=find_character_by_runtime_id(victim_runtime_id)", spell_acid_roll
)
spell_acid_damage = spell_damage_compact.index(
    "DamageStuff(acid_victim,SPLDAM_ACID)", spell_acid_lookup
)
spell_nonlethal = spell_damage_compact.index("if(result==DAM_NONEDEAD)", spell_acid_damage)
spell_first_refresh = spell_damage_compact.index(
    "result=refresh_spell_damage_participants()", spell_nonlethal
)
spell_typed_damage = spell_damage_compact.index("DamageStuff(victim,type)", spell_first_refresh)
spell_typed_refresh = spell_damage_compact.index(
    "result=refresh_spell_damage_participants()", spell_typed_damage
)
spell_attack_back = spell_damage_compact.index("attack_back(ch,victim,FALSE)", spell_typed_refresh)
spell_attack_refresh = spell_damage_compact.index(
    "result=refresh_spell_damage_participants()", spell_attack_back
)
spell_wet_effect = spell_damage_compact.index("IS_AFFECTED5(victim,AFF5_WET)", spell_attack_refresh)
assert (
    spell_raw_damage
    < spell_acid_roll
    < spell_acid_lookup
    < spell_acid_damage
    < spell_nonlethal
    < spell_first_refresh
    < spell_typed_damage
    < spell_typed_refresh
    < spell_attack_back
    < spell_attack_refresh
    < spell_wet_effect
)

check_shields = continuation_caller(
    "fight.c", "int check_shields(P_char ch, P_char victim, int dam, int flags)"
)
check_shields_compact = "".join(check_shields.split())
shields_ch_membership = check_shields_compact.index(
    "constboolch_listed=ch&&char_in_list(ch)"
)
shields_victim_membership = check_shields_compact.index(
    "constboolvictim_listed=victim&&char_in_list(victim)"
)
shields_ch_liveness = check_shields_compact.index(
    "constboolch_initially_alive=ch_listed&&IS_ALIVE(ch)"
)
shields_victim_liveness = check_shields_compact.index(
    "constboolvictim_initially_alive=victim_listed&&IS_ALIVE(victim)"
)
shields_training_dummy = check_shields_compact.index(
    "if(training_dummy_is(victim))"
)
assert (
    shields_ch_membership
    < shields_ch_liveness
    < shields_training_dummy
    and shields_victim_membership
    < shields_victim_liveness
    < shields_training_dummy
)

negative_shield = check_shields_compact.index("&negshield)")
negative_shield_refresh = check_shields_compact.index(
    "result=refresh_check_shields_participants()", negative_shield
)
infernal_followup = check_shields_compact.index(
    "IS_AFFECTED(victim,AFF_INFERNAL_FURY)", negative_shield_refresh
)
assert negative_shield < negative_shield_refresh < infernal_followup

holy_soulshield = check_shields_compact.index("&soulshield_spec)")
holy_soulshield_refresh = check_shields_compact.index(
    "result=refresh_check_shields_participants()", holy_soulshield
)
holy_devotion_read = check_shields_compact.index(
    "GET_CHAR_SKILL(ch,SKILL_DEVOTION)", holy_soulshield_refresh
)
assert holy_soulshield < holy_soulshield_refresh < holy_devotion_read

thornskin_damage = check_shields_compact.index("result=raw_damage(victim,ch,thornDamage,")
thornskin_refresh = check_shields_compact.index(
    "result=refresh_check_shields_participants()", thornskin_damage
)
acid_blood_followup = check_shields_compact.index(
    "has_innate(victim,INNATE_ACID_BLOOD)", thornskin_refresh
)
assert thornskin_damage < thornskin_refresh < acid_blood_followup

raw_damage = continuation_caller(
    "fight.c",
    "int raw_damage(P_char ch, P_char victim, double dam, uint flags, struct damage_messages *messages,",
)
raw_damage_compact = "".join(raw_damage.split())
raw_actor_membership = raw_damage_compact.index("if(!char_in_list(ch))")
raw_actor_dummy_check = raw_damage_compact.index("if(training_dummy_is(ch))")
raw_victim_null_check = raw_damage_compact.index("if(!victim)")
raw_victim_membership = raw_damage_compact.index("if(!char_in_list(victim))")
raw_actor_id_capture = raw_damage_compact.index(
    "constuint64_tch_runtime_id=ch->runtime_id"
)
raw_update_groupies = raw_damage_compact.index("update_groupies(ch,true)")
raw_victim_stat = raw_damage_compact.index("GET_STAT(victim)==STAT_DEAD")
assert (
    raw_actor_membership
    < raw_actor_dummy_check
    < raw_victim_null_check
    < raw_victim_membership
    < raw_update_groupies
    < raw_victim_stat
)
assert "if(victim&&!char_in_list(victim))returnDAM_BOTHDEAD;" in raw_damage_compact
raw_die = raw_damage_compact.index("die(victim,ch)")
raw_death_actor_refresh = raw_damage_compact.index(
    "ch=find_character_by_runtime_id(ch_runtime_id)", raw_die
)
raw_death_room_check = raw_damage_compact.index(
    "!is_char_in_room(ch,room)", raw_death_actor_refresh
)
assert (
    raw_actor_id_capture
    < raw_die
    < raw_death_actor_refresh
    < raw_death_room_check
)

melee_damage = continuation_caller(
    "fight.c",
    "int melee_damage(P_char ch, P_char victim, double dam, int flags, struct damage_messages *messages,",
)
melee_damage_compact = "".join(melee_damage.split())
melee_ch_membership = melee_damage_compact.index(
    "constboolch_listed=ch&&char_in_list(ch)"
)
melee_victim_membership = melee_damage_compact.index(
    "constboolvictim_listed=victim&&char_in_list(victim)"
)
melee_ch_liveness = melee_damage_compact.index(
    "constboolch_initially_alive=ch_listed&&IS_ALIVE(ch)"
)
melee_victim_liveness = melee_damage_compact.index(
    "constboolvictim_initially_alive=victim_listed&&IS_ALIVE(victim)"
)
assert (
    "if(!ch_initially_alive)returnDAM_CHARDEAD;"
    "if(!victim_initially_alive)returnDAM_VICTDEAD;"
) in melee_damage_compact
melee_raw_damage = melee_damage_compact.index(
    "result=raw_damage(ch,victim,dam,RAWDAM_DEFAULT|flags|RAWDAM_NOWARD,"
)
assert (
    melee_ch_membership
    < melee_ch_liveness
    < melee_raw_damage
    and melee_victim_membership
    < melee_victim_liveness
    < melee_raw_damage
)
melee_raw_guard = melee_damage_compact.index(
    "if(result!=DAM_NONEDEAD)returnresult;", melee_raw_damage
)
melee_raw_refresh = melee_damage_compact.index(
    "result=refresh_melee_damage_participants()", melee_raw_guard
)
melee_warring_zeal = melee_damage_compact.index(
    "if(affected_by_spell(ch,SPELL_WARRING_ZEAL))", melee_raw_refresh
)
assert melee_raw_damage < melee_raw_guard < melee_raw_refresh < melee_warring_zeal

melee_warring_damage = melee_damage_compact.index(
    "result=spell_damage(ch,victim,local_dam", melee_warring_zeal
)
melee_warring_guard = melee_damage_compact.index(
    "if(result!=DAM_NONEDEAD)returnresult;", melee_warring_damage
)
melee_warring_refresh = melee_damage_compact.index(
    "result=refresh_melee_damage_participants()", melee_warring_guard
)
melee_item_damage = melee_damage_compact.index(
    "DamageStuff(victim,SPLDAM_GENERIC)", melee_warring_refresh
)
melee_item_refresh = melee_damage_compact.index(
    "result=refresh_melee_damage_participants()", melee_item_damage
)
melee_shields = melee_damage_compact.index("check_shields(ch,victim,dam,flags)", melee_item_refresh)
melee_shields_refresh = melee_damage_compact.index(
    "result=refresh_melee_damage_participants()", melee_shields
)
melee_attack_back = melee_damage_compact.index(
    "returnattack_back(ch,victim,TRUE)", melee_shields_refresh
)
assert (
    melee_warring_zeal
    < melee_warring_damage
    < melee_warring_guard
    < melee_warring_refresh
    < melee_item_damage
    < melee_item_refresh
    < melee_shields
    < melee_shields_refresh
    < melee_attack_back
)

monk_critic = continuation_caller(
    "attack_effects.c", "bool monk_critic(P_char ch, P_char victim, int *damAccumulator)"
)
monk_compact = "".join(monk_critic.split())
assert monk_compact.index("!char_in_list(ch)") < monk_compact.index("!IS_ALIVE(ch)")
assert monk_compact.index("!char_in_list(victim)") < monk_compact.index("!IS_ALIVE(victim)")
monk_damage = monk_compact.index("DAM_NONEDEAD!=melee_damage(ch,victim,")
monk_actor_refresh = monk_compact.index(
    "ch=find_character_by_runtime_id(ch_runtime_id)", monk_damage
)
monk_victim_refresh = monk_compact.index(
    "victim=find_character_by_runtime_id(victim_runtime_id)", monk_actor_refresh
)
monk_live_check = monk_compact.index(
    "if(!ch||!IS_ALIVE(ch)||!victim||!IS_ALIVE(victim))", monk_victim_refresh
)
monk_affect_refresh = monk_compact.index(
    "af=get_spell_from_char(victim,TAG_PRESSURE_POINTS)", monk_live_check
)
monk_modifier_cleanup = monk_compact.index("if(af->modifier==6)", monk_affect_refresh)
monk_affect_remove = monk_compact.index(
    "affect_from_char(ch,TAG_PRESSURE_POINTS)", monk_modifier_cleanup
)
assert (
    monk_damage
    < monk_actor_refresh
    < monk_victim_refresh
    < monk_live_check
    < monk_affect_refresh
    < monk_modifier_cleanup
    < monk_affect_remove
)
assert "af->" not in monk_compact[monk_damage:monk_affect_refresh]

hit = continuation_caller(
    "fight.c", "bool hit(P_char ch, P_char victim, P_obj weapon, int *damAccumulator)"
)
hit_compact = "".join(hit.split())
hit_entry_membership = hit_compact.index("!char_in_list(ch)")
hit_victim_membership = hit_compact.index("!char_in_list(victim)", hit_entry_membership)
hit_actor_liveness = hit_compact.index("!IS_ALIVE(ch)", hit_victim_membership)
hit_victim_liveness = hit_compact.index("!IS_ALIVE(victim)", hit_actor_liveness)
hit_first_skill_read = hit_compact.index("GET_CHAR_SKILL(ch,SKILL_VICIOUS_STRIKE)")
assert hit_entry_membership < hit_victim_membership < hit_actor_liveness < hit_victim_liveness
assert hit_victim_liveness < hit_first_skill_read
weapon_damage = hit_compact.index("DamageOneItem(ch,1,weapon,FALSE)")
weapon_damage_guard = hit_compact.rindex(
    "begin_attack_continuation(ch,victim,weapon)", 0, weapon_damage
)
weapon_damage_check = hit_compact.index(
    "check_attack_continuation(damage_continuation)", weapon_damage
)
weapon_change = hit_compact.index(
    "after_item_damage.outcome==attack_continuation_outcome::weapon_changed",
    weapon_damage_check,
)
weapon_clear = hit_compact.index("weapon=nullptr", weapon_change)
message_object_clear = hit_compact.index("messages.obj=weapon", weapon_clear)
melee_after_item_damage = hit_compact.index("melee_damage(ch,victim,dam,", weapon_damage)
assert (
    weapon_damage_guard
    < weapon_damage
    < weapon_damage_check
    < weapon_change
    < weapon_clear
    < message_object_clear
    < melee_after_item_damage
)
assert "else{returnTRUE;}" in hit_compact[weapon_damage_check:message_object_clear]

critical_attack_call = hit_compact.index("critical_attack(ch,victim,msg)")
critical_guard = hit_compact.rindex(
    "begin_attack_continuation(ch,victim,weapon)", 0, critical_attack_call
)
critical_check = hit_compact.index(
    "check_attack_continuation(critical_continuation)", critical_attack_call
)
critical_actor_refresh = hit_compact.index("ch=after_critical.actor", critical_check)
critical_target_refresh = hit_compact.index("victim=after_critical.target", critical_actor_refresh)
critical_weapon_refresh = hit_compact.index("weapon=after_critical.weapon", critical_target_refresh)
battle_frenzy = hit_compact.index("has_innate(ch,INNATE_BATTLE_FRENZY)", critical_attack_call)
assert (
    critical_guard
    < critical_attack_call
    < critical_check
    < critical_actor_refresh
    < critical_target_refresh
    < critical_weapon_refresh
    < battle_frenzy
)
assert "if(!after_critical.can_continue())returnTRUE;" in hit_compact[
    critical_check:critical_actor_refresh
]

vampiric_touch = hit_compact.index(
    "if(!weapon&&affected_by_spell(ch,SPELL_VAMPIRIC_TOUCH)"
)
vampiric_actor_capture = hit_compact.index("actor_runtime_id=ch->runtime_id", vampiric_touch)
vampiric_damage = hit_compact.index(
    "damage(ch,victim,to_hit,SPELL_VAMPIRIC_TOUCH)", vampiric_actor_capture
)
vampiric_actor_refresh = hit_compact.index(
    "ch=find_character_by_runtime_id(actor_runtime_id)", vampiric_damage
)
vampiric_actor_check = hit_compact.index(
    "if(!ch||!IS_ALIVE(ch))", vampiric_actor_refresh
)
vampiric_affect_cleanup = hit_compact.index(
    "affect_from_char(ch,SPELL_VAMPIRIC_TOUCH)", vampiric_actor_check
)
vampiric_heal = hit_compact.index("vamp(ch,to_hit,", vampiric_affect_cleanup)
assert (
    vampiric_touch
    < vampiric_actor_capture
    < vampiric_damage
    < vampiric_actor_refresh
    < vampiric_actor_check
    < vampiric_affect_cleanup
    < vampiric_heal
)

battle_frenzy = hit_compact.index("if(has_innate(ch,INNATE_BATTLE_FRENZY)")
frenzy_guard = hit_compact.index(
    "begin_attack_continuation(ch,victim,weapon)", battle_frenzy
)
frenzy_call = hit_compact.index("battle_frenzy(ch,victim)", frenzy_guard)
frenzy_result_check = hit_compact.index(
    "if(frenzy_result!=DAM_NONEDEAD)returnFALSE;", frenzy_call
)
frenzy_continuation_check = hit_compact.index(
    "check_attack_continuation(frenzy_continuation)", frenzy_result_check
)
frenzy_actor_refresh = hit_compact.index("ch=after_frenzy.actor", frenzy_continuation_check)
frenzy_target_refresh = hit_compact.index("victim=after_frenzy.target", frenzy_actor_refresh)
frenzy_weapon_refresh = hit_compact.index("weapon=after_frenzy.weapon", frenzy_target_refresh)
vicious_after_frenzy = hit_compact.index(
    "GET_CHAR_SKILL(ch,SKILL_VICIOUS_ATTACK)", frenzy_call
)
assert (
    battle_frenzy
    < frenzy_guard
    < frenzy_call
    < frenzy_result_check
    < frenzy_continuation_check
    < frenzy_actor_refresh
    < frenzy_target_refresh
    < frenzy_weapon_refresh
    < vicious_after_frenzy
)
assert "if(!after_frenzy.can_continue())returnFALSE;" in hit_compact[
    frenzy_continuation_check:frenzy_actor_refresh
]

main_melee = hit_compact.index("if(melee_damage(ch,victim,dam,")
main_melee_guard = hit_compact.rindex(
    "begin_attack_continuation(ch,victim,weapon)", 0, main_melee
)
main_melee_result_guard = hit_compact.index(
    ")!=DAM_NONEDEAD){returnTRUE;}", main_melee
)
main_melee_check = hit_compact.index("check_attack_continuation(melee_continuation)", main_melee)
main_melee_actor = hit_compact.index("ch=after_melee.actor", main_melee_check)
main_melee_target = hit_compact.index("victim=after_melee.target", main_melee_actor)
main_melee_weapon = hit_compact.index("weapon=after_melee.weapon", main_melee_target)
main_melee_weapon_change = hit_compact.index(
    "after_melee.outcome==attack_continuation_outcome::weapon_changed", main_melee_check
)
main_melee_weapon_clear = hit_compact.index("weapon=nullptr", main_melee_weapon_change)
main_melee_return = hit_compact.index("else{returnTRUE;}", main_melee_weapon_clear)
reaver_after_main_melee = hit_compact.index(
    "reaver_hit_proc(ch,victim,weapon)", main_melee
)
assert (
    main_melee_guard
    < main_melee
    < main_melee_result_guard
    < main_melee_check
    < main_melee_actor
    < main_melee_target
    < main_melee_weapon
    < main_melee_weapon_change
    < main_melee_weapon_clear
    < main_melee_return
    < reaver_after_main_melee
)

reaver_continuation_guard = hit_compact.rindex(
    "begin_attack_continuation(ch,victim,weapon)", 0, reaver_after_main_melee
)
reaver_call = hit_compact.index("reaver_hit_proc(ch,victim,weapon)", reaver_continuation_guard)
reaver_handled_return = hit_compact.index(
    "if(reaver_hit_handled)returnTRUE;", reaver_call
)
reaver_continuation_check = hit_compact.index(
    "check_attack_continuation(reaver_continuation)", reaver_handled_return
)
reaver_actor_refresh = hit_compact.index("ch=after_reaver.actor", reaver_continuation_check)
reaver_target_refresh = hit_compact.index("victim=after_reaver.target", reaver_actor_refresh)
reaver_weapon_refresh = hit_compact.index("weapon=after_reaver.weapon", reaver_target_refresh)
reaver_weapon_change = hit_compact.index(
    "after_reaver.outcome==attack_continuation_outcome::weapon_changed",
    reaver_continuation_check,
)
reaver_weapon_clear = hit_compact.index("weapon=nullptr", reaver_weapon_change)
reaver_invalid_return = hit_compact.index("else{returnTRUE;}", reaver_weapon_clear)
dread_blade_after_reaver = hit_compact.index(
    "affected_by_spell(ch,SPELL_DREAD_BLADE)", reaver_call
)
assert (
    reaver_continuation_guard
    < reaver_call
    < reaver_handled_return
    < reaver_continuation_check
    < reaver_actor_refresh
    < reaver_target_refresh
    < reaver_weapon_refresh
    < reaver_weapon_change
    < reaver_weapon_clear
    < reaver_invalid_return
    < dread_blade_after_reaver
)

dread_blade_condition = hit_compact.index("if(affected_by_spell(ch,SPELL_DREAD_BLADE)")
dread_continuation_guard = hit_compact.index(
    "begin_attack_continuation(ch,victim,weapon)", dread_blade_condition
)
dread_call = hit_compact.index("dread_blade_proc(ch,victim)", dread_continuation_guard)
dread_handled_return = hit_compact.index("if(dread_handled)returnTRUE;", dread_call)
dread_continuation_check = hit_compact.index(
    "check_attack_continuation(dread_continuation)", dread_handled_return
)
dread_actor_refresh = hit_compact.index("ch=after_dread.actor", dread_continuation_check)
dread_target_refresh = hit_compact.index("victim=after_dread.target", dread_actor_refresh)
dread_weapon_refresh = hit_compact.index("weapon=after_dread.weapon", dread_target_refresh)
dread_weapon_change = hit_compact.index(
    "after_dread.outcome==attack_continuation_outcome::weapon_changed",
    dread_continuation_check,
)
dread_weapon_clear = hit_compact.index("weapon=nullptr", dread_weapon_change)
dread_invalid_return = hit_compact.index("else{returnTRUE;}", dread_weapon_clear)
paladin_after_dread = hit_compact.index("if(GET_CLASS(ch,CLASS_PALADIN)", dread_call)
assert (
    dread_blade_condition
    < dread_continuation_guard
    < dread_call
    < dread_handled_return
    < dread_continuation_check
    < dread_actor_refresh
    < dread_target_refresh
    < dread_weapon_refresh
    < dread_weapon_change
    < dread_weapon_clear
    < dread_invalid_return
    < paladin_after_dread
)

lightbringer = continuation_caller(
    "drannak.c", "bool lightbringer_proc(P_char ch, P_char victim, bool phys)"
)
lightbringer_compact = "".join(lightbringer.split())
assert lightbringer_compact.index("!char_in_list(ch)") < lightbringer_compact.index(
    "!IS_ALIVE(ch)"
)
assert lightbringer_compact.index("!char_in_list(victim)") < lightbringer_compact.index(
    "!IS_ALIVE(victim)"
)
lightbringer_spell = lightbringer_compact.index("(spells[number(0,4)])")
lightbringer_actor_refresh = lightbringer_compact.index(
    "ch=find_character_by_runtime_id(attacker_runtime_id)", lightbringer_spell
)
lightbringer_target_refresh = lightbringer_compact.index(
    "victim=find_character_by_runtime_id(victim_runtime_id)", lightbringer_actor_refresh
)
lightbringer_live_check = lightbringer_compact.index(
    "if(!ch||!IS_ALIVE(ch)||!victim||!IS_ALIVE(victim))", lightbringer_target_refresh
)
lightbringer_room_check = lightbringer_compact.index(
    "returnch->in_room!=room||victim->in_room!=room", lightbringer_live_check
)
assert (
    lightbringer_spell
    < lightbringer_actor_refresh
    < lightbringer_target_refresh
    < lightbringer_live_check
    < lightbringer_room_check
)

lightbringer_hit = hit_compact.index("if(affected_by_spell(ch,ACH_YOUSTRAHDME)")
lightbringer_hit_guard = hit_compact.index(
    "begin_attack_continuation(ch,victim,weapon)", lightbringer_hit
)
lightbringer_hit_call = hit_compact.index(
    "lightbringer_proc(ch,victim,TRUE)", lightbringer_hit_guard
)
lightbringer_hit_return = hit_compact.index(
    "if(lightbringer_handled)returnTRUE;", lightbringer_hit_call
)
lightbringer_hit_check = hit_compact.index(
    "check_attack_continuation(lightbringer_continuation)", lightbringer_hit_return
)
lightbringer_hit_actor = hit_compact.index(
    "ch=after_lightbringer.actor", lightbringer_hit_check
)
lightbringer_hit_target = hit_compact.index(
    "victim=after_lightbringer.target", lightbringer_hit_actor
)
lightbringer_hit_weapon = hit_compact.index(
    "weapon=after_lightbringer.weapon", lightbringer_hit_target
)
assert (
    lightbringer_hit
    < lightbringer_hit_guard
    < lightbringer_hit_call
    < lightbringer_hit_return
    < lightbringer_hit_check
    < lightbringer_hit_actor
    < lightbringer_hit_target
    < lightbringer_hit_weapon
)

weapon_poison = hit_compact.index("if(weapon&&weapon->value[4]!=0)")
poison_spell_snapshot = hit_compact.index(
    "constintpoison_spell=weapon->value[4]", weapon_poison
)
poison_continuation_guard = hit_compact.index(
    "begin_attack_continuation(ch,victim,weapon)", poison_spell_snapshot
)
poison_spell_call = hit_compact.index(
    "(skills[poison_spell].spell_pointer)(10,ch,0,0,victim,0)",
    poison_continuation_guard,
)
poison_lifeleak_call = hit_compact.index(
    "poison_lifeleak(10,ch,0,0,victim,0)", poison_spell_call
)
poison_continuation_check = hit_compact.index(
    "check_attack_continuation(poison_continuation)", poison_lifeleak_call
)
assert "weapon->" not in hit_compact[poison_spell_call:poison_continuation_check]
poison_continue_branch = hit_compact.index(
    "if(after_poison.can_continue())", poison_continuation_check
)
poison_actor_refresh = hit_compact.index("ch=after_poison.actor", poison_continue_branch)
poison_target_refresh = hit_compact.index("victim=after_poison.target", poison_actor_refresh)
poison_weapon_refresh = hit_compact.index("weapon=after_poison.weapon", poison_target_refresh)
poison_clear = hit_compact.index("weapon->value[4]=0", poison_weapon_refresh)
poison_fallback_loop = hit_compact.index(
    "P_objlive_poison_weapon=nullptr", poison_clear
)
poison_live_match = hit_compact.index(
    "object==poison_continuation.weapon&&object->obj_uid==poison_continuation.weapon_uid",
    poison_fallback_loop,
)
poison_fallback_clear = hit_compact.index(
    "live_poison_weapon->value[4]=0", poison_live_match
)
poison_stop = hit_compact.index("returnTRUE;", poison_fallback_clear)
poison_weapon_proc = hit_compact.index("weapon_proc(weapon,ch,victim)", poison_stop)
assert (
    weapon_poison
    < poison_spell_snapshot
    < poison_continuation_guard
    < poison_spell_call
    < poison_lifeleak_call
    < poison_continuation_check
    < poison_continue_branch
    < poison_actor_refresh
    < poison_target_refresh
    < poison_weapon_refresh
    < poison_clear
    < poison_fallback_loop
    < poison_live_match
    < poison_fallback_clear
    < poison_stop
    < poison_weapon_proc
)

blood_alliance_event = continuation_caller(
    "reavers.c",
    "void event_blood_alliance(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)",
)
blood_alliance_compact = "".join(blood_alliance_event.split())
blood_alliance_actor_guard = blood_alliance_compact.index(
    "if(!ch||!char_in_list(ch)||!IS_ALIVE(ch))return;"
)
blood_alliance_link_lookup = blood_alliance_compact.index(
    "linked=get_linking_char(ch,LNK_BLOOD_ALLIANCE);"
)
blood_alliance_link_guard = blood_alliance_compact.index(
    "if(!linked||!char_in_list(linked)||!IS_ALIVE(linked))return;",
    blood_alliance_link_lookup,
)
blood_alliance_room_read = blood_alliance_compact.index(
    "if(linked->in_room!=ch->in_room)return;", blood_alliance_link_guard
)
blood_alliance_hit_read = blood_alliance_compact.index(
    "if(GET_HIT(linked)<GET_MAX_HIT(linked)*0.7)return;",
    blood_alliance_link_guard,
)
assert blood_alliance_actor_guard < blood_alliance_link_lookup
assert blood_alliance_link_lookup < blood_alliance_link_guard < blood_alliance_room_read
assert blood_alliance_link_guard < blood_alliance_hit_read

mob_cast_spell = continuation_caller(
    "mobact.c", "bool MobCastSpell(P_char ch, P_char victim, P_obj object, int spl, int lvl)"
)
mob_cast_compact = "".join(mob_cast_spell.split())
mob_cast_room_capture = mob_cast_compact.index("constintcaster_room=ch->in_room")
mob_cast_height_capture = mob_cast_compact.index(
    "constintcaster_height=ch->specials.z_cord", mob_cast_room_capture
)
mob_cast_area_loop = mob_cast_compact.index(
    "for(tch=world[ch->in_room].people;tch;tch=tch2)"
)
mob_cast_next_target_capture = mob_cast_compact.index(
    "constuint64_tnext_tch_runtime_id=tch2?tch2->runtime_id:0;", mob_cast_area_loop
)
mob_cast_reactive_hit = mob_cast_compact.index(
    "hit(tch,ch,tch->equipment[PRIMARY_WEAPON]);", mob_cast_next_target_capture
)
mob_cast_owner_refresh = mob_cast_compact.index(
    "ch=find_character_by_runtime_id(caster_runtime_id)", mob_cast_reactive_hit
)
mob_cast_owner_guard = mob_cast_compact.index(
    "if(!ch||!IS_ALIVE(ch)||ch->in_room!=caster_room||ch->specials.z_cord!=caster_height||!live_target)",
    mob_cast_owner_refresh,
)
mob_cast_next_target_refresh = mob_cast_compact.index(
    "tch2=find_character_by_runtime_id(next_tch_runtime_id)", mob_cast_owner_guard
)
assert mob_cast_room_capture < mob_cast_height_capture < mob_cast_area_loop
assert mob_cast_area_loop < mob_cast_next_target_capture < mob_cast_reactive_hit
assert mob_cast_reactive_hit < mob_cast_owner_refresh < mob_cast_owner_guard
assert mob_cast_owner_guard < mob_cast_next_target_refresh

assist_core = continuation_caller(
    "actoff.c", "void do_assist_core(P_char ch, P_char victim)"
)
assist_core_compact = "".join(assist_core.split())
assist_actor_capture = assist_core_compact.index(
    "constuint64_tactor_runtime_id=ch->runtime_id;"
)
assist_mob_start = assist_core_compact.index(
    "MobStartFight(ch,GET_OPPONENT(victim));", assist_actor_capture
)
assist_reactive_hit = assist_core_compact.index(
    "hit(ch,GET_OPPONENT(victim),ch->equipment[PRIMARY_WEAPON]);", assist_mob_start
)
assist_actor_refresh = assist_core_compact.index(
    "ch=find_character_by_runtime_id(actor_runtime_id)", assist_reactive_hit
)
assist_wait_guard = assist_core_compact.index(
    "if(ch&&IS_ALIVE(ch))CharWait(ch,(int)(PULSE_VIOLENCE*0.5));",
    assist_actor_refresh,
)
assert assist_actor_capture < assist_mob_start < assist_reactive_hit
assert assist_reactive_hit < assist_actor_refresh < assist_wait_guard

perform_violence = continuation_caller("attack_cadence.c", "void perform_violence(void)")
perform_violence_compact = "".join(perform_violence.split())
cadence_room_capture = perform_violence_compact.index("room=ch->in_room")
cadence_guard_capture = perform_violence_compact.index(
    "begin_attack_continuation(ch,opponent)", cadence_room_capture
)
cadence_attack_loop = perform_violence_compact.index(
    "for(i=0;i<real_attacks;i++)", cadence_guard_capture
)
cadence_hit = perform_violence_compact.index("pv_common(ch,opponent,", cadence_attack_loop)
cadence_revalidate = perform_violence_compact.index(
    "check_attack_continuation(cadence_continuation)", cadence_hit
)
cadence_target_room_guard = perform_violence_compact.index(
    "!is_char_in_room(after_attack.target,room)", cadence_revalidate
)
cadence_abort = perform_violence_compact.index(
    "if(!cadence_can_continue)continue;", cadence_revalidate
)
assert cadence_room_capture < cadence_guard_capture < cadence_attack_loop
assert cadence_attack_loop < cadence_hit < cadence_revalidate
assert cadence_revalidate < cadence_target_room_guard < cadence_abort
assert "ch=after_attack.actor;opponent=after_attack.target;" in perform_violence_compact[
    cadence_revalidate:cadence_abort
]

with tempfile.TemporaryDirectory(prefix="duris-attack-continuation-") as temporary:
    source_path = Path(temporary) / "attack_continuation.cpp"
    binary = Path(temporary) / "attack_continuation"
    source_path.write_text(PREFIX + SUFFIX)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-g",
            "-O0",
            "-D__NO_MYSQL__",
            "-Isrc",
            "-Isrc/no_mysql",
            "-I/usr/include/libxml2",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-fno-pie",
            "-no-pie",
            str(source_path),
            str(ROOT / "src" / "combat" / "attack_continuation.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True, timeout=30)
