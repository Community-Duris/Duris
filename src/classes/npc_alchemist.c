#include "classes/npc_alchemist.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/object_template.h"
#include "world/vnum.obj.h"
#include <algorithm>
#include <array>
#include <climits>

extern Skill skills[];
extern P_room world;
extern const int top_of_world;
extern unsigned long long ne_event_tick;

namespace
{
struct effect_profile
{
	int minimum_level;
	// Nitrogen, dispel, wither, slow, grease, napalm, glass, acid, greater living stone.
	std::array<int, 9> weights;
};
constexpr std::array<int, 9> potion_vnums = { 868, 866, 865, 863, 859, 857, 855, 853, 850 };
constexpr effect_profile profiles[] = {
	{ 6, { 1, 0, 0, 0, 0, 0, 0, 0, 0 } },	{ 11, { 4, 1, 0, 0, 0, 0, 0, 0, 0 } },
	{ 16, { 10, 1, 1, 0, 0, 0, 0, 0, 0 } }, { 21, { 16, 2, 1, 1, 0, 0, 0, 0, 0 } },
	{ 26, { 24, 2, 1, 1, 4, 0, 0, 0, 0 } }, { 31, { 8, 1, 1, 2, 4, 48, 0, 0, 0 } },
	{ 36, { 0, 1, 0, 1, 4, 2, 32, 0, 0 } }, { 41, { 0, 1, 0, 1, 4, 0, 2, 40, 0 } },
	{ 51, { 0, 3, 0, 3, 0, 0, 6, 9, 2 } },
};
const effect_profile *profile_for(int level)
{
	const effect_profile *selected = nullptr;
	for (const auto &profile : profiles)
		if (level >= profile.minimum_level)
			selected = &profile;
	return selected;
}
int reference_spell(int level)
{
	// Representative single-class sorcerer, selecting an offensive spell at its
	// current tier, with sufficient slots and a valid unprotected target.
	if (level >= 41)
		return SPELL_PRISMATIC_RAY;
	if (level >= 26)
		return SPELL_FIREBALL;
	if (level >= 21)
		return SPELL_CONE_OF_COLD;
	if (level >= 16)
		return SPELL_LIGHTNING_BOLT;
	if (level >= 11)
		return SPELL_ACID_BLAST;
	return SPELL_BURNING_HANDS;
}
bool usable_template(const object_template *prototype)
{
	if (!prototype || prototype->type != ITEM_POTION)
		return false;
	bool usable = false;
	for (int slot = 1; slot <= 3; ++slot)
	{
		const int spell = prototype->value[slot];
		if (spell <= 0)
			continue;
		if (spell >= MAX_SKILLS || !skills[spell].spell_pointer)
			return false;
		const auto targets = skills[spell].targets;
		if (!(targets & TAR_IGNORE) && !(targets & TAR_CHAR_ROOM))
			return false;
		usable = true;
	}
	return usable;
}
}

void npc_alchemist_cache_templates()
{
	for (int vnum : potion_vnums)
		if (!cache_object_template(vnum))
			logit(LOG_STATUS, "NPC alchemist template VNUM %d is unavailable", vnum);
}

int npc_alchemist_caster_interval(P_char ch)
{
	// MobCombat runs once per personal melee round. CharWait must expire before
	// a caster can begin its next spell, so round up cast time to that opportunity.
	const int round = std::max(1, static_cast<int>(ch->specials.base_combat_round) + 1);
	const int cast = std::max(1, SpellCastTime(ch, reference_spell(GET_LEVEL(ch))));
	return ((cast + round - 1) / round) * round;
}

bool npc_alchemist_combat(P_char ch)
{
	if (!ch || !IS_NPC(ch) || !ch->only.npc || !GET_CLASS(ch, CLASS_ALCHEMIST) ||
	    !IS_ALIVE(ch) || !IS_AWAKE(ch) || !CAN_ACT(ch) || !IS_FIGHTING(ch) ||
	    ch->in_room <= NOWHERE || ch->in_room > top_of_world ||
	    ne_event_tick < ch->only.npc->alchemist_action_until_pulse)
		return false;
	const auto *profile = profile_for(GET_LEVEL(ch));
	if (!profile)
		return false;
	P_char target = pick_target(ch, PT_NUKETARGET | PT_WEAKEST);
	if (!target)
		target = GET_OPPONENT(ch);
	if (!target || target == ch || !char_in_list(target) || !IS_ALIVE(target) ||
	    target->in_room != ch->in_room)
		return false;
	std::array<int, 9> weights = profile->weights;
	int total = 0;
	for (size_t index = 0; index < weights.size(); ++index)
	{
		if (!usable_template(find_object_template(potion_vnums[index])))
			weights[index] = 0;
		total += weights[index];
	}
	if (!total)
		return false;
	int choice = number(1, total);
	size_t selected = 0;
	for (; selected < weights.size() - 1; ++selected)
	{
		choice -= weights[selected];
		if (choice <= 0)
			break;
	}
	const auto *prototype = find_object_template(potion_vnums[selected]);
	const int interval = npc_alchemist_caster_interval(ch);
	ch->only.npc->alchemist_action_until_pulse = ne_event_tick + 3ULL * interval;
	// One action budget is shared by both AI entry points.
	CharWait(ch, std::min(PULSE_VIOLENCE, interval));
	if (number(1, 140) > GET_C_AGI(ch) || !number(0, GET_LEVEL(ch) * 2))
	{
		act("$n throws an alchemical mixture, but it splashes harmlessly away!", TRUE, ch,
		    0, target, TO_ROOM);
		return true;
	}
	act("$n throws an alchemical mixture at $N!", FALSE, ch, 0, target, TO_NOTVICT);
	act("$n's alchemical mixture strikes you!", FALSE, ch, 0, target, TO_VICT);
	if (!IS_TRUSTED(ch) &&
	    (IS_ROOM(ch->in_room, ROOM_NO_MAGIC) ||
	     (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE) && !AdjacentInRoom(ch, target))))
		return true;
	const int level = std::min(50, GET_LEVEL(ch));
	for (int slot = 1; slot <= 3; ++slot)
	{
		const int spell = prototype->value[slot];
		if (spell <= 0)
			continue;
		if (IS_AGG_SPELL(spell) && target != ch &&
		    (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT)))
			appear(ch);
		(*skills[spell].spell_pointer)(level, ch, nullptr, SPELL_TYPE_SPELL, target,
					       nullptr);
		// Spells can remove either character. Never dereference them afterward.
		if (!char_in_list(ch) || !char_in_list(target))
			return true;
		if (IS_AGG_SPELL(spell) && target != ch)
		{
			if (affected_by_spell(target, SPELL_SLEEP))
				affect_from_char(target, SPELL_SLEEP);
			if (GET_STAT(target) == STAT_SLEEPING)
			{
				send_to_char("Your rest is violently disturbed!\r\n", target);
				SET_POS(target, GET_POS(target) + STAT_NORMAL);
			}
		}
		if (target->in_room != ch->in_room || !IS_ALIVE(ch) || !IS_ALIVE(target))
			return true;
	}
	return true;
}

void npc_alchemist_world_spawn(P_char ch)
{
	if (!ch || !IS_NPC(ch) || !ch->only.npc || !GET_CLASS(ch, CLASS_ALCHEMIST) ||
	    ch->only.npc->summoned_instance || GET_MASTER(ch) || ch->in_room <= NOWHERE ||
	    ch->in_room > top_of_world || ch->only.npc->alchemist_vial_roll_done)
		return;
	ch->only.npc->alchemist_vial_roll_done = true;
	// Mark before allocation. Failure or depletion never triggers another roll.
	if (number(1, 100) > 10)
		return;
	if (P_obj vial = read_object(VOBJ_POISON_VIALS, VIRTUAL))
		obj_to_char(vial, ch);
}
