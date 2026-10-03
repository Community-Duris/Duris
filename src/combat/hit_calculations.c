#include <stdio.h>
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "combat/damage.h"
#include "combat/dam_mods.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "world/difficulty.h"

extern int is_wearing_necroplasm(P_char ch);
int calculate_ac(P_char ch)
{
	int victim_ac = GET_AC(ch);

	// Only drop PCs ac since we don't want to mess with the ac set in zone files.
	if (victim_ac < 0 && IS_PC(ch))
		victim_ac = ((float)victim_ac * dam_factor[DF_NEG_AC_MULT]);

	// This is strange since load_modifier ranges from 75-300.
	//  if( GET_AC(ch) < 1 && load_modifier(ch) > 50 )
	// This ranges from 0 to 225, much more reasonable.
	victim_ac += load_modifier(ch) - 75;

	if (GET_CLASS(ch, CLASS_MONK))
		victim_ac += MonkAcBonus(ch);

	victim_ac += io_agi_defense(ch);

	if (has_innate(ch, INNATE_RRAKKMA) && ch->group)
	{
		for (struct group_list *gl = ch->group; gl; gl = gl->next)
		{
			if (ch != gl->ch && IS_PC(gl->ch) && ch->in_room == gl->ch->in_room &&
			    has_innate(gl->ch, INNATE_RRAKKMA))
			{
				victim_ac -= 10;
			}
		}
	}

	return BOUNDED(-750, victim_ac, 100);
}

int calculate_thac_zero(P_char ch, int skill)
{
	int to_hit = 0;

	if (IS_GREATER_RACE(ch) || IS_SET(ch->specials.act, ACT_PROTECTOR))
	{
		to_hit = get_property("to.hit.GreaterRaceTypes", 13);
	}
	else if (GET_CLASS(ch, CLASS_WARRIOR) || GET_CLASS(ch, CLASS_DREADLORD) ||
		 GET_CLASS(ch, CLASS_AVENGER) || GET_CLASS(ch, CLASS_PALADIN) ||
		 GET_CLASS(ch, CLASS_ANTIPALADIN) || affected_by_spell(ch, SPELL_COMBAT_MIND) ||
		 is_wearing_necroplasm(ch) || GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_LANCER))
	{
		to_hit = get_property("to.hit.WarriorTypes", 10);
	}
	else if (GET_CLASS(ch, CLASS_MERCENARY) || GET_CLASS(ch, CLASS_REAVER) ||
		 GET_CLASS(ch, CLASS_RANGER) || GET_CLASS(ch, CLASS_BERSERKER) ||
		 GET_SPEC(ch, CLASS_NECROMANCER, SPEC_REAPER) ||
		 GET_SPEC(ch, CLASS_THEURGIST, SPEC_THAUMATURGE) ||
		 GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) ||
		 GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_HUNTER))
	{
		to_hit = get_property("to.hit.HitterTankTypes", 8);
	}
	else if (GET_CLASS(ch, CLASS_THIEF) || GET_CLASS(ch, CLASS_BARD) ||
		 GET_CLASS(ch, CLASS_ROGUE) || GET_CLASS(ch, CLASS_ASSASSIN) ||
		 GET_CLASS(ch, CLASS_MONK))
	{
		to_hit = get_property("to.hit.RogueTypes", 7);
	}
	else if (GET_CLASS(ch, CLASS_CLERIC) || GET_CLASS(ch, CLASS_DRUID) ||
		 GET_CLASS(ch, CLASS_BLIGHTER) || GET_CLASS(ch, CLASS_SHAMAN) ||
		 GET_CLASS(ch, CLASS_WARLOCK) || GET_CLASS(ch, CLASS_ETHERMANCER) ||
		 GET_CLASS(ch, CLASS_ALCHEMIST) || GET_CLASS(ch, CLASS_PSIONICIST) ||
		 GET_SPEC(ch, CLASS_DRAGOON, SPEC_DRAGON_PRIEST))
	{
		to_hit = get_property("to.hit.ClericTypes", 6);
	}
	else if (GET_CLASS(ch, CLASS_SORCERER) || GET_CLASS(ch, CLASS_CONJURER) ||
		 GET_CLASS(ch, CLASS_NECROMANCER) || GET_CLASS(ch, CLASS_THEURGIST) ||
		 GET_CLASS(ch, CLASS_ILLUSIONIST) || GET_CLASS(ch, CLASS_MINDFLAYER) ||
		 GET_CLASS(ch, CLASS_SUMMONER) || GET_CLASS(ch, CLASS_DRAGOON))
	{
		to_hit = get_property("to.hit.MageTypes", 4);
	}
	else if (IS_NPC(ch))
	{
		to_hit = get_property("to.hit.npcTypes", 10);
	}
	else
	{
		if (IS_PC(ch) && !number(0, 100))
		{
			statuslog(
				AVATAR,
				"The THAC0 for %s's class is not defined, update calculate_thac_zero().",
				GET_NAME(ch));
		}
		to_hit = get_property("to.hit.npcTypes", 10);
	}

	to_hit = (int)(to_hit * GET_LEVEL(ch) / 6);

	to_hit += IS_NPC(ch) ? BOUNDED(3, (GET_LEVEL(ch) * 2), 95) : skill;

	to_hit = MAX((2 * to_hit) / 3, 30);

	to_hit += BOUNDED(-10, GET_HITROLL(ch) * 2, 90);
	// hard cap for benefit to hitroll(45hr max) Jexni 10/05/08

	if (ch->equipment[PRIMARY_WEAPON] &&
	    IS_SET(ch->equipment[PRIMARY_WEAPON]->extra2_flags, ITEM2_BLESS))
	{
		to_hit = (int)(to_hit * get_property("to.hit.BlessBonus", 1.100));
	}

	if (GET_C_LUK(ch) / 2 > number(0, 100))
		to_hit = (int)(to_hit * get_property("to.hit.LuckBonus", 1.100));

	if (to_hit < 0)
	{
		wizlog(56, "(%s) has less than 0 thac0: fight.c calculate_thac_zero()",
		       GET_NAME(ch));
		to_hit = 0; // Shouldn't need this, but rather debug the code than pass errors.
	}

	return to_hit;
}

int chance_to_hit(P_char ch, P_char victim, int skill, P_obj weapon)
{
	int to_hit, victim_ac;

	if (!IS_ALIVE(ch))
		return 0;

	// New function to calculate thac0 - Dec08 - Lucrot
	to_hit = calculate_thac_zero(ch, skill);

	if (((IS_EVIL(ch) && !IS_EVIL(victim) && IS_AFFECTED(victim, AFF_PROTECT_EVIL)) ||
	     (IS_GOOD(ch) && !IS_GOOD(victim) && IS_AFFECTED(victim, AFF_PROTECT_GOOD))))
	{
		to_hit -= GET_LEVEL(victim) / 4;
	}

	// This makes no sense commenting it out. Dec08 - Lucrot
	// if((IS_EVIL(ch) && IS_EVIL(victim)) ||
	// (IS_GOOD(ch) && IS_GOOD(victim)))
	// {
	// if (affected_by_spell(ch, SPELL_VIRTUE))
	// {
	// to_hit = (int) (to_hit * get_property("to.hit.VirtueBonus", 1.030));
	// }
	// }

	if ((IS_EVIL(ch) && !IS_EVIL(victim)) || (IS_GOOD(ch) && !IS_GOOD(victim)))
	{
		if (affected_by_spell(ch, SPELL_VIRTUE))
			to_hit = (int)(to_hit * get_property("to.hit.VirtueBonus", 1.100));
	}

	if (IS_UNDEADRACE(ch) && !IS_UNDEADRACE(victim) && IS_AFFECTED5(victim, AFF5_PROT_UNDEAD))
		to_hit = (int)to_hit * get_property("to.hit.ProtUndead", 0.700);

	if (!CAN_SEE(ch, victim))
	{
		if (IS_NPC(ch))
		{
			to_hit -= 40 * MIN(100, (120 - 2 * GET_LEVEL(ch))) / 100;
		}
		else
		{
			to_hit -= 40 * (120 - GET_CHAR_SKILL(ch, SKILL_BLINDFIGHTING)) / 100;
			notch_skill(ch, SKILL_BLINDFIGHTING,
				    get_property("skill.notch.blindFighting", 6.25));
		}
	}

	victim_ac = MAX(-100, MIN(calculate_ac(victim), 100));

	if (IS_AFFECTED(victim, AFF_BLIND))
	{
		victim_ac +=
			(int)(40 * (120 - GET_CHAR_SKILL_P(victim, SKILL_BLINDFIGHTING)) / 100);
	}

#ifdef FIGHT_DEBUG
	char buf[MAX_STRING_LENGTH];
	snprintf(buf, MAX_STRING_LENGTH, "&+Rvictim ac: %d&n ", victim_ac);
	send_to_char(buf, ch);
#endif

	if (!weapon && affected_by_spell(ch, SPELL_VAMPIRIC_TOUCH) && !GET_CLASS(ch, CLASS_MONK))
		to_hit = (int)(to_hit * get_property("to.hit.VampTouch", 1.150));

	if (!IS_AWAKE(victim) || IS_IMMOBILE(victim))
		to_hit += 100;

	if (GET_POS(victim) < POS_STANDING)
		to_hit += (POS_STANDING - GET_POS(victim)) * 10;

	if (GET_POS(ch) < POS_STANDING)
		to_hit -= (POS_STANDING - GET_POS(ch)) * 15;

	const int hit_chance = BOUNDED(1, (to_hit + (victim_ac * 85 / 100)), 100);
	if (difficulty_world_npc(ch))
		return difficulty_scale_percent(hit_chance,
						difficulty_multiplier(DIFFICULTY_MOB_ACCURACY));
	return hit_chance;
}

int required_weapon_skill(P_obj wpn)
{
	if (!wpn)
		return SKILL_UNARMED_DAMAGE;
	else if (wpn->type != ITEM_WEAPON)
		return 0;

	switch (wpn->value[0])
	{
	case WEAPON_AXE:
	case WEAPON_SHORTSWORD:
	case WEAPON_2HANDSWORD:
	case WEAPON_SICKLE:
	case WEAPON_LONGSWORD:
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_SLASHING :
								 SKILL_1H_SLASHING;
		break;
	case WEAPON_DAGGER:
	case WEAPON_HORN:
		if (IS_SET(wpn->extra_flags, ITEM_TWOHANDS))
		{
			char Gbuf[MAX_STRING_LENGTH];
			snprintf(Gbuf, MAX_STRING_LENGTH,
				 "Weapon '%s' [%d] has 2h flag set and is a %s (%d).",
				 wpn->short_description, OBJ_VNUM(wpn),
				 (wpn->value[0] == WEAPON_DAGGER) ? "Dagger" : "Horn",
				 wpn->value[0]);
			debug("%s", Gbuf);
			logit(LOG_OBJ, "%s", Gbuf);
		}
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? 0 : SKILL_1H_PIERCING;
		break;
	case WEAPON_HAMMER:
	case WEAPON_CLUB:
	case WEAPON_SPIKED_CLUB:
	case WEAPON_MACE:
	case WEAPON_SPIKED_MACE:
	case WEAPON_STAFF:
	case WEAPON_LANCE:
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_BLUDGEON :
								 SKILL_1H_BLUDGEON;
		break;
	case WEAPON_WHIP:
	case WEAPON_FLAIL:
	case WEAPON_NUMCHUCKS:
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_2H_FLAYING :
								 SKILL_1H_FLAYING;
		break;
	case WEAPON_TRIDENT:
	case WEAPON_SPEAR:
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_REACH_WEAPONS :
								 SKILL_1H_PIERCING;
		break;
	case WEAPON_POLEARM:
		return IS_SET(wpn->extra_flags, ITEM_TWOHANDS) ? SKILL_REACH_WEAPONS :
								 SKILL_1H_SLASHING;
		break;
	default:
		return 0;
		break;
	}
}
