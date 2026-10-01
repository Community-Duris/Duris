/* Combat attack cadence and per-tick violence dispatch. */
#include "core/prototypes.h"
#include "combat/attack_cadence.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "combat/attack_continuation.h"
#include "combat/attack_resolution.h"
#include "combat/grapple.h"
#include "item/item_actions.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/map.h"
#include "world/weather.h"
#include "magic/spells.h"
#include "mob/studioproc.h"
#include "account/account_reward.h"
#include <math.h>
#include <set>
#include <stdio.h>
#include <string.h>

extern P_char combat_list;
extern P_char combat_next_ch;
extern P_room world;
extern int pulse;
extern struct time_info_data time_info;
extern bool is_dragoon_mounted(P_char ch);
extern P_char misfire_check(P_char ch, P_char spell_target, int flag);

static bool frightening_presence(P_char ch, P_char victim)
{
	int chance;

	if (GET_OPPONENT(victim) == ch)
		return FALSE;

	if (GET_LEVEL(victim) >= GET_LEVEL(ch))
		chance = 15 + dice(1, 10);
	else
		chance = 15;

	if (number(0, 100) < chance && !fear_check(ch))
	{
		act("&+rTerror&+L unlike anything you have ever felt overwhelms you.", TRUE, ch, 0,
		    victim, TO_CHAR);
		act("Unable to stand the aura of fear surrounding $N $n turns to flee.", TRUE, ch,
		    0, victim, TO_ROOM);

		do_flee(ch, 0, 0);
		return TRUE;
	}

	return FALSE;
}

#define ADD_ATTACK(slot) (attacks[number_attacks++] = (slot))

static int calculate_attacks(P_char ch, int attacks[])
{
	int number_attacks = 0;
	P_obj weapon;

	if (IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE) || !IS_ALIVE(GET_OPPONENT(ch)))
		return 0;

	if (GET_CLASS(ch, CLASS_MONK))
	{
		bool fighting_pc = GET_OPPONENT(ch) ? IS_PC(GET_OPPONENT(ch)) : FALSE;
		int num_atts = MonkNumberOfAttacks(ch);
		int weight_threshold = GET_C_STR(ch) / 2;

		if (!IS_TRUSTED(ch) && (total_carried_weight(ch) >= weight_threshold) && IS_PC(ch))
		{
			num_atts -= MIN(num_atts - 1, (int)((total_carried_weight(ch) - 30) / 10));

			// if (!number(0, 4))
			send_to_char(
				"&+LYou feel weighed down, which is causing you to lose attacks.&n\r\n",
				ch);
		}

		if (IS_AFFECTED2(ch, AFF2_SLOW) && num_atts > 1)
			num_atts /= 2;

		while (num_atts--)
		{
			// We're not on the last attack and up to 35% chance at level 56 (% max health for 2 hits damage).
			if (fighting_pc && (num_atts > 0) &&
			    (((GET_LEVEL(ch) / 2 + 9)) >= number(1, 100)))
			{
				// Costs 3 attacks worth.
				num_atts--;
				// Hack for %max health damage.
				ADD_ATTACK(WEAR_NONE);
			}
			else
			{
				ADD_ATTACK(PRIMARY_WEAPON);
			}
		}
	}
	else
	{ // not MONK
		if (!IS_AFFECTED2(ch, AFF2_SLOW) || IS_AFFECTED(ch, AFF_HASTE))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			// Can swing both primary hands at once - 50% for newbs, 100% for maxxed dual wield.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50) >= number(1, 100)))
			{
				ADD_ATTACK(THIRD_WEAPON);
			}
		}

		// I hate it when both my hands are trying to swing the same 1h sword.
		if (ch->equipment[PRIMARY_WEAPON] && ch->equipment[SECONDARY_WEAPON] &&
		    (ch->equipment[PRIMARY_WEAPON] != ch->equipment[SECONDARY_WEAPON]))
		{
			if (notch_skill(ch, SKILL_DUAL_WIELD,
					get_property("skill.notch.offensive.auto", 4)) ||
			    number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD))
			{
				ADD_ATTACK(SECONDARY_WEAPON);
				// Can swing both secondary hands at once - 50% for newbs, 100% for maxxed dual wield.
				if (HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON] &&
				    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50) >=
				     number(1, 100)))
				{
					ADD_ATTACK(FOURTH_WEAPON);
				}

				if (GET_CHAR_SKILL(ch, SKILL_IMPROVED_TWOWEAPON) >= number(1, 100))
				{
					ADD_ATTACK(SECONDARY_WEAPON);
					// Can swing both secondary hands at once - 50% for newbs, 100% for maxxed dual wield.
					if (HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON] &&
					    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50) >=
					     number(1, 100)))
					{
						ADD_ATTACK(FOURTH_WEAPON);
					}
				}
			}
		}

		/*
			    if(ch->player.race == RACE_KOBOLD || ch->player.race == RACE_GNOME)
			    {
			    if (number(1, 160) < GET_C_AGI(ch))
			    {
			    ADD_ATTACK(PRIMARY_WEAPON);
			    send_to_char("&nYou move swiftly and execute an extra attack against your foe!&n\n\r", ch);
			    }
			    }

			    if(ch->player.race == RACE_GOBLIN || ch->player.race == RACE_HALFLING)
			    {
			    if (number(1, 220) < GET_C_AGI(ch))
			    {
			    ADD_ATTACK(PRIMARY_WEAPON);
			    send_to_char("&nYou move swiftly and execute an extra attack against your foe!&n\n\r", ch);
			    }
			    }
			    */

		/* No longer giving minos an extra attack.
			    //loop through affects - Drannak
			    struct affected_type *findaf, *next_af;  //initialize affects

			    for(findaf = ch->affected; findaf; findaf = next_af)
			    {
			      next_af = findaf->next;
			      if(findaf && findaf->type == TAG_MINOTAUR_RAGE)
			        ADD_ATTACK(PRIMARY_WEAPON);
			    }
			*/

		// This randomness might should go too.
		if (GET_SPEC(ch, CLASS_ANTIPALADIN, SPEC_VIOLATOR))
			ADD_ATTACK(PRIMARY_WEAPON);

		if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT))
		{
			int zealproc;
			struct affected_type af;
			if (GET_C_STR(ch) > number(1, 340))
			{
				zealproc = (number(1, 5));
				switch (zealproc)
				{
				case 1:
					bzero(&af, sizeof(af));
					int bonus;
					if (GET_C_STR(ch) >= 300)
						bonus = 0;
					else
						bonus = number(5, 10);
					af.duration = 50;
					af.location = APPLY_STR_MAX;
					af.modifier = bonus;
					af.flags = AFFTYPE_SHORT;
					affect_to_char(ch, &af);

					act("&nThe power of your &+Wgod&n suddenly fills your body and you feel &+BMUCH&n stronger!&n",
					    FALSE, ch, 0, 0, TO_CHAR);
					act("&n$n gasps suddenly as their body is &+Benhanced&n by some &+Wother-worldly &npower!&n",
					    FALSE, ch, 0, 0, TO_ROOM);
					do_say(ch, writable_arg("Fill me with your strength!"),
					       CMD_SAY);
					break;
				case 2:
					act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
					    FALSE, ch, NULL, GET_OPPONENT(ch), TO_CHAR);
					act("$n says 'The non-believer must be punished!", FALSE,
					    ch, 0, 0, TO_ROOM);
					ADD_ATTACK(PRIMARY_WEAPON);
					ADD_ATTACK(PRIMARY_WEAPON);
					do_say(ch,
					       writable_arg(
						       "What once was lost, is now found upon your skull!"),
					       CMD_SAY);
					break;
				case 3:
					bzero(&af, sizeof(af));
					if (GET_C_STR(ch) >= 300)
						bonus = 0;
					else
						bonus = number(5, 10);
					af.duration = 50;
					af.location = APPLY_STR_MAX;
					af.modifier = bonus;
					af.flags = AFFTYPE_SHORT;
					affect_to_char(ch, &af);

					act("&nThe power of your &+Wgod&n suddenly fills your body and you feel &+BMUCH&n stronger!&n",
					    FALSE, ch, 0, 0, TO_CHAR);
					act("&n$n gasps suddenly as their body is &+Benhanced&n by some &+Wother-worldly &npower!&n",
					    FALSE, ch, 0, 0, TO_NOTVICT);
					act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
					    FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
					do_say(ch,
					       writable_arg("The non-believer must be punished!"),
					       CMD_SAY);
					ADD_ATTACK(PRIMARY_WEAPON);
					ADD_ATTACK(PRIMARY_WEAPON);
					break;
				case 4:
					act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
					    FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
					ADD_ATTACK(PRIMARY_WEAPON);
					do_say(ch,
					       writable_arg("Beg for forgiveness and be saved!"),
					       CMD_SAY);
					ADD_ATTACK(PRIMARY_WEAPON);
					break;
				case 5:
					act("&+WYour devotion to your god causes you to unleash a &+rF&+Rlurr&+ry&+W of attacks against $N!&n",
					    FALSE, ch, 0, GET_OPPONENT(ch), TO_CHAR);
					do_say(ch,
					       writable_arg(
						       "May your bloodshed be a willing sacrifice!"),
					       CMD_SAY);
					ADD_ATTACK(PRIMARY_WEAPON);
					ADD_ATTACK(PRIMARY_WEAPON);
					break;
				}
			}
		}

		if (GET_CLASS(ch, CLASS_PSIONICIST) && affected_by_spell(ch, SPELL_COMBAT_MIND))
		{
			ADD_ATTACK(PRIMARY_WEAPON);

			if (GET_LEVEL(ch) > 51)
				ADD_ATTACK(PRIMARY_WEAPON);
		}

		if (notch_skill(ch, SKILL_DOUBLE_ATTACK,
				get_property("skill.notch.offensive.auto", 4)) ||
		    GET_CHAR_SKILL(ch, SKILL_DOUBLE_ATTACK) >= number(1, 100))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			// Can swing both primary hands at once - 99% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50) > number(1, 100)))
			{
				ADD_ATTACK(THIRD_WEAPON);
			}
			// Can swing second secondary hand - 94% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[FOURTH_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45) > number(1, 100)))
			{
				ADD_ATTACK(FOURTH_WEAPON);
			}
		}

		if (notch_skill(ch, SKILL_TRIPLE_ATTACK,
				get_property("skill.notch.offensive.auto", 4)) ||
		    GET_CHAR_SKILL(ch, SKILL_TRIPLE_ATTACK) >= number(1, 100))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			// Can swing both primary hands at once - 94% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45) > number(1, 100)))
			{
				ADD_ATTACK(THIRD_WEAPON);
			}
			// Can swing primary secondary hand - 94% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[SECONDARY_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45) > number(1, 100)))
			{
				ADD_ATTACK(SECONDARY_WEAPON);
			}
		}

		if (notch_skill(ch, SKILL_QUADRUPLE_ATTACK,
				get_property("skill.notch.offensive.auto", 4)) ||
		    GET_CHAR_SKILL(ch, SKILL_QUADRUPLE_ATTACK) > number(1, 100))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			// Can swing both primary hands at once - 94% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 45) > number(1, 100)))
			{
				ADD_ATTACK(THIRD_WEAPON);
			}
		}
	}

	// both monks and others below
	if (IS_AFFECTED(ch, AFF_HASTE))
	{
		if (!IS_AFFECTED2(ch, AFF2_SLOW))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			// Can swing both primary hands at once - 100% chance max.
			if (HAS_FOUR_HANDS(ch) && ch->equipment[THIRD_WEAPON] &&
			    ((GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) / 2 + 50) >= number(1, 100)))
			{
				ADD_ATTACK(THIRD_WEAPON);
			}
		}
	}

	// High dex now grants extra attacks (does not include bare hands).
	// Dex primary weapon
	weapon = ch->equipment[PRIMARY_WEAPON];
	if ((weapon == NULL) || (weapon->type == ITEM_WEAPON))
	{
		int actpct =
			(100 * ((weapon == NULL) ? 0 : GET_OBJ_WEIGHT(weapon))) / GET_C_STR(ch);

		if (((actpct <= 6) && (GET_C_DEX(ch) >= 125)) ||
		    ((actpct <= 20) && (GET_C_DEX(ch) >= 150)))
		{
			if (number(1, GET_C_DEX(ch)) > 60)
			{
				send_to_char(
					"&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r",
					ch);
				ADD_ATTACK(PRIMARY_WEAPON);
			}
		}
	}
	// Dex secondary weapon
	weapon = ch->equipment[SECONDARY_WEAPON];
	if ((weapon == NULL) || (weapon->type == ITEM_WEAPON))
	{
		if (ch->equipment[PRIMARY_WEAPON] &&
		    IS_SET(ch->equipment[PRIMARY_WEAPON]->extra_flags, ITEM_TWOHANDS) &&
		    weapon == NULL)
		{
			// swap to primary for actpct weight check if 2hander
			weapon = ch->equipment[PRIMARY_WEAPON];
		}

		int actpct =
			(100 * ((weapon == NULL) ? 0 : GET_OBJ_WEIGHT(weapon))) / GET_C_STR(ch);

		if ((actpct <= 6) && (GET_C_DEX(ch) >= 150))
		{
			if (number(1, GET_C_DEX(ch)) > 60)
			{
				// Swap back to secondary if it was changed due to 2hander so the check below works as original
				weapon = ch->equipment[SECONDARY_WEAPON];
				send_to_char(
					"&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r",
					ch);
				if (ch->equipment[PRIMARY_WEAPON] &&
				    IS_SET(ch->equipment[PRIMARY_WEAPON]->extra_flags,
					   ITEM_TWOHANDS) &&
				    weapon == NULL)
					ADD_ATTACK(PRIMARY_WEAPON);
				else
					ADD_ATTACK(SECONDARY_WEAPON);
			}
		}
	}

	/* Keeping the old 3 tier code for dex attacks, but reducing it to two.
		  if(actpct <= 5)
		  {
		    if(GET_C_DEX(ch) >= 155)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		    else if(GET_C_DEX(ch) >=140)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		    else if(GET_C_DEX(ch) >=125)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		  }
		  else if(actpct <= 10)
		  {
		    if(GET_C_DEX(ch) >= 155)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		    else if(GET_C_DEX(ch) >=140)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		  }
		  else if(actpct <= 20)
		  {
		    if(GET_C_DEX(ch) >= 155)
		    {
		      if(number(1, GET_C_DEX(ch)) > 60)
		      {
		        send_to_char("&nYour improved &+gdexterity&n grants you an additional attack!&n\n\r", ch);
		        ADD_ATTACK(PRIMARY_WEAPON);
		      }
		    }
		  }
		*/

	if (GET_CLASS(ch, CLASS_CLERIC) && affected_by_spell(ch, SPELL_DIVINE_FURY))
		ADD_ATTACK(PRIMARY_WEAPON);

	if (GET_CLASS(ch, CLASS_DREADLORD | CLASS_AVENGER) &&
	    GET_CHAR_SKILL(ch, required_weapon_skill(ch->equipment[PRIMARY_WEAPON])) > 69)
		ADD_ATTACK(PRIMARY_WEAPON);

	if (affected_by_spell(ch, SPELL_HOLY_SWORD))
		ADD_ATTACK(PRIMARY_WEAPON);

	if (IS_AFFECTED4(ch, AFF4_VAMPIRE_FORM))
	{
		ADD_ATTACK(PRIMARY_WEAPON);
		if (number(0, 2))
			ADD_ATTACK(PRIMARY_WEAPON);
	}

	if (get_linking_char(ch, LNK_ESSENCE_OF_WOLF))
	{
		int extra = number(2, 4);

		extra = MIN(extra, GET_LEVEL(ch) / 10);

		if (ch->equipment[SECONDARY_WEAPON])
		{
			ADD_ATTACK(SECONDARY_WEAPON);
			if (number(0, 1))
			{
				extra--;
				ADD_ATTACK(SECONDARY_WEAPON);
			}
		}

		while (extra-- > 0)
			ADD_ATTACK(PRIMARY_WEAPON);
	}

	// This is a reaver-only bonus, as it goddamned should have been originally
	if (affected_by_spell(ch, SPELL_KANCHELSIS_FURY) && GET_CLASS(ch, CLASS_REAVER))
	{
		ADD_ATTACK(PRIMARY_WEAPON);

		if (GET_LEVEL(ch) >= 41 && ch->equipment[SECONDARY_WEAPON] &&
		    number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD))
			ADD_ATTACK(SECONDARY_WEAPON);

		if (GET_LEVEL(ch) >= 51)
			ADD_ATTACK(PRIMARY_WEAPON);

		if (GET_LEVEL(ch) >= 56) // Long into wipe, give them something to strive for
			ADD_ATTACK(SECONDARY_WEAPON);
	}

	/* Commented out - allow flurry to use max attacks. -- Eikel.
		  if (IS_AFFECTED2(ch, AFF2_FLURRY) && number_attacks > 4)
		  {
		    int maxattacks = number_attacks;
		    number_attacks = number(4, maxattacks);
		  }*/

	if (IS_AFFECTED3(ch, AFF3_VIVERNAE_CONCORDIA))
	{
		if (IS_DRAGOON(ch) && is_dragoon_mounted(ch))
		{
			ADD_ATTACK(PRIMARY_WEAPON);
			int blurattackchance = (GET_LEVEL(ch) / 2);
			if (number(1, 100) < blurattackchance && ch->equipment[SECONDARY_WEAPON])
				ADD_ATTACK(SECONDARY_WEAPON);
		}
	}
	else if (IS_AFFECTED3(ch, AFF3_BLUR))
	{
		ADD_ATTACK(PRIMARY_WEAPON);
		if ((GET_CLASS(ch, CLASS_RANGER) || GET_SECONDARY_CLASS(ch, CLASS_RANGER)) &&
		    number(1, 100) < GET_CHAR_SKILL(ch, SKILL_DUAL_WIELD) &&
		    ch->equipment[SECONDARY_WEAPON])
			ADD_ATTACK(SECONDARY_WEAPON);

		int blurattackchance = (GET_LEVEL(ch) / 2);
		if (GET_CLASS(ch, CLASS_RANGER) && (number(1, 100) < blurattackchance) &&
		    ch->equipment[SECONDARY_WEAPON])
			ADD_ATTACK(SECONDARY_WEAPON);
	}

	// Not-standing? half attacks - Drannak 7/22/13
	// Don't you mean less attacks?
	if (!MIN_POS(ch, POS_STANDING + STAT_NORMAL) && !GROUNDFIGHTING_CHECK(ch))
	{
		if (GET_POS(ch) == POS_KNEELING)
			// 30% reduction for kneeling
			number_attacks = (int)(number_attacks - (number_attacks / 3));
		else
			// 50% reduction for sitting/prone
			number_attacks = (int)(number_attacks - (number_attacks / 2));
	}

	return number_attacks;
}
#undef ADD_ATTACK

// ATTACK_DIVISOR is the amount of attacks lost due to inert barrier, armlock, etc.
//   ie 2 -> loss of 1/2 of attacks, 3 -> loss of 2/3 of attacks, etc.
// Note: The current limit for one round of battle is 256 attacks for one character.
#define ATTACK_DIVISOR 2
#define MAX_ATTACKS 256
void perform_violence(void)
{
	P_char ch, opponent;
	char GBuf1[MAX_STRING_LENGTH];
	int attacks[MAX_ATTACKS];
	int number_attacks, real_attacks, div_attacks;
	int num_hits, damAccumulator;
	int i, room;
	std::set<int> room_rnums;
	std::set<int>::iterator it;
	int door, nearby_room;
	P_char tmp_ch;
	bool melee_exp_pulse;
	// loop through everyone fighting

	melee_exp_pulse = ((pulse % PULSE_VIOLENCE) == 0);

	for (ch = combat_list; ch; ch = combat_next_ch)
	{
		combat_next_ch = ch->specials.next_fighting;

		if (!IS_ALIVE(ch))
			continue;

		room_rnums.insert(ch->in_room);

		opponent = GET_OPPONENT(ch);

		if (!opponent)
		{
			if (ch->in_room != NOWHERE)
			{
				debug("perform_violence: %s fighting null opponent in %s (%d)!",
				      GET_NAME(ch), world[ch->in_room].name,
				      world[ch->in_room].number);
				logit(LOG_DEBUG,
				      "perform_violence: %s fighting null opponent in %s (%d)!",
				      GET_NAME(ch), world[ch->in_room].name,
				      world[ch->in_room].number);
				extract_char(ch);
				return;
			}
			else
			{
				debug("%s fighting null opponent in NOWHERE!", GET_NAME(ch));
				logit(LOG_DEBUG, "%s fighting null opponent in NOWHERE!",
				      GET_NAME(ch));
				stop_fighting(ch);
				continue;
			}
		}

		// If someone's hitting on opponent (ch is) then give opponent melee exp.
		if (melee_exp_pulse && opponent && !IS_IMMOBILE(ch) && (opponent != ch) &&
		    IS_PC(opponent) && (opponent->in_room == ch->in_room))
		{
			// Make sure we're gaining a positive amount.
			if (GET_LEVEL(ch) * 2 > GET_LEVEL(opponent))
				gain_exp(opponent, ch, GET_LEVEL(ch) * 2 - GET_LEVEL(opponent),
					 EXP_MELEE);
		}

		if (ch->specials.combat_tics-- > 0)
		{
			continue;
		}
		else
		{
			ch->specials.combat_tics = (int)ch->specials.base_combat_round;
			if (affected_by_spell(ch, SKILL_WHIRLWIND))
			{
				GET_VITALITY(ch) -=
					get_property("skill.whirlwind.movement.drain", 10);
				if (GET_VITALITY(ch) > 0)
					ch->specials.combat_tics -= (ch->specials.combat_tics >> 1);
				else
					affect_from_char(ch, SKILL_WHIRLWIND);
			}
		}

		if (IS_PC(ch) && IS_PC(opponent))
		{
			startPvP(ch, GET_RACEWAR(ch) != GET_RACEWAR(opponent));
			startPvP(opponent, GET_RACEWAR(ch) != GET_RACEWAR(opponent));
		}

		if (!FightingCheck(ch, opponent, "perform_violence"))
			continue;

		/* misfire */
		if (IS_PC(ch))
		{
			opponent = misfire_check(ch, opponent, DISALLOW_SELF | DISALLOW_BACKRANK);
			if (!opponent)
				continue;
		}

		if (HOLD_CANT_ATTACK(ch))
			continue;

		/** handle paralysis and slowness --TAM 2/94 **/

		if (IS_AFFECTED2(ch, AFF2_MAJOR_PARALYSIS))
		{
			act("You remain paralyzed and can't do a thing to defend yourself.", FALSE,
			    ch, 0, 0, TO_CHAR);
			act("$n strains to respond to $N's attack, but the paralysis is too overpowering.",
			    FALSE, ch, 0, opponent, TO_ROOM);
			continue;
		}

		else if (IS_AFFECTED2(ch, AFF2_MINOR_PARALYSIS))
		{
			act("You couldn't budge a feather in your present condition.", FALSE, ch, 0,
			    0, TO_CHAR);
			act("$n is too preoccupied with $s nervous system problem to fight.", FALSE,
			    ch, 0, 0, TO_ROOM);
			continue;
		}

		if (item_action_active(ch) ||
		    (IS_AFFECTED2(ch, AFF2_CASTING) && !affected_by_spell(ch, SPELL_BATTLEMAGE)))
			continue;

		if (IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE))
		{
			if (!number(0, 4))
			{
				send_to_char(
					"You are being non-offensive... just a random reminder.\r\n",
					ch);
			}
			continue;
		}

		number_attacks = calculate_attacks(ch, attacks);
		if (number_attacks > MAX_ATTACKS)
			number_attacks = MAX_ATTACKS;
		if (number_attacks <= 0)
		{
			send_to_char("You can't seem to get a single swing in.\n", ch);
			continue;
		}

		float attacksMultiplier = get_property("attacks.default.multiplier", 1.0);

		if (IS_AFFECTED3(opponent, AFF3_INERTIAL_BARRIER))
			attacksMultiplier = get_property("attacks.inertialBarrier.multiplier", 0.5);
		else if (IS_ARMLOCK(ch))
			attacksMultiplier = get_property("attacks.armLock.multiplier", 0.5);
		else if (affected_by_spell(ch, TAG_INTERCEPT))
			attacksMultiplier = get_property("attacks.intercept.multiplier", 0.5);

		if (affected_by_spell(ch, SPELL_COMBAT_MIND))
		{
			float cmMulti = get_property("attacks.combatMind.multiplier", 0.75);
			if (attacksMultiplier < cmMulti)
				attacksMultiplier = cmMulti;
		}

		// we ceil to not round off attacks
		real_attacks = (int)ceil(number_attacks * attacksMultiplier);
		div_attacks = (int)(1 / attacksMultiplier);

		if (!affected_by_spell(opponent, SKILL_BATTLE_SENSES) &&
		    GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES) &&
		    GET_POS(opponent) == POS_STANDING && !IS_STUNNED(opponent) &&
		    !IS_BLIND(opponent))
		{
			if (notch_skill(opponent, SKILL_BATTLE_SENSES,
					get_property("skill.notch.defensive", 17)))
			{
			}
			else if ((1 + (GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES) / 10)) >=
				 number(1, 100))
			{
				act("&+wA sense of awareness &+bflows &+wover you as you move into &+rbattle.&n",
				    FALSE, opponent, 0, 0, TO_CHAR);
				act("$n maneuvers around you like a &+yviper!&n", FALSE, opponent,
				    0, 0, TO_VICT);
				act("$n &+bflows into battle with the speed of a &+yviper.&n",
				    FALSE, opponent, 0, 0, TO_NOTVICT);

				set_short_affected_by(opponent, SKILL_BATTLE_SENSES, 1);
			}
		}

		if (!affected_by_spell(opponent, SKILL_BATTLE_SENSES) &&
		    GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES) &&
		    GET_POS(opponent) == POS_STANDING && !IS_STUNNED(opponent) &&
		    !IS_BLIND(opponent))
		{
			if ((1 + (GET_CHAR_SKILL(opponent, SKILL_BATTLE_SENSES) / 10)) >=
			    number(1, 100))
			{
				act("&+wA sense of awareness &+bflows &+wover you as you move into &+rbattle.&n",
				    FALSE, opponent, 0, 0, TO_CHAR);
				act("$n maneuvers around you like a &+yviper!&n", FALSE, opponent,
				    0, 0, TO_VICT);
				act("$n &+bflows into battle with the speed of a &+yviper.&n",
				    FALSE, opponent, 0, 0, TO_NOTVICT);

				set_short_affected_by(opponent, SKILL_BATTLE_SENSES, 1);
			}
		}

		if (IS_DRAGOON(opponent) && is_dragoon_mounted(opponent))
		{
			if (affected_by_spell(opponent, SPELL_SANGUINIS_IGNIS) &&
			    !affected_by_spell(opponent, SPELL_SANGUINIS_IGNIS_AFF) &&
			    GET_POS(opponent) == POS_STANDING && !IS_STUNNED(opponent) &&
			    !IS_BLIND(opponent))
			{
				// 10% chance on shaman spell knowledge + 5% POW modifier
				if (((1 +
				      (GET_CHAR_SKILL(opponent, SKILL_SPELL_KNOWLEDGE_SHAMAN) / 10) +
				      (GET_C_POW(ch) / 20))) >= number(1, 100))
				{
					act("You smell &+rblood&n as you maneuver your &+Gdr&+Lag&+Gon&n into battle!",
					    FALSE, opponent, 0, 0, TO_CHAR);
					act("$n maneuvers $s &+Gdr&+Lag&+Gon&n around you, tracking your movements!&n",
					    FALSE, opponent, 0, 0, TO_VICT);
					act("$n maneuvers $s &+Gdr&+Lag&+Gon&n into battle, tracking $s prey!&n",
					    FALSE, opponent, 0, 0, TO_NOTVICT);

					set_short_affected_by(opponent, SPELL_SANGUINIS_IGNIS_AFF,
							      1);
				}
			}
		}

		if (!IS_SUNLIT(ch->in_room))
		{
			if (GET_CHAR_SKILL(ch, SKILL_SHADOW_MOVEMENT) && !IS_BLIND(ch) &&
			    !IS_STUNNED(ch) && GET_POS(ch) == POS_STANDING &&
			    (notch_skill(ch, SKILL_SHADOW_MOVEMENT,
					 get_property("skill.notch.offensive.auto", 4)) ||
			     (1 + GET_CHAR_SKILL(ch, SKILL_SHADOW_MOVEMENT) / 4 > number(1, 100))))
			{
				act("$n &+wblinks out of existence ... then reappears &+ybehind&n $N!&n",
				    FALSE, ch, 0, opponent, TO_NOTVICT);
				act("&+wYou blink out of existence and reappear behind&n $N.&n",
				    FALSE, ch, 0, opponent, TO_CHAR);
				if (!IS_BLIND(opponent))
					act("$n &+wblinks out of existence ... then reappears &+ybehind you!&n",
					    FALSE, ch, 0, opponent, TO_VICT);
				set_short_affected_by(ch, SKILL_SHADOW_MOVEMENT, 4);
			}
		}

		if (has_innate(opponent, INNATE_FPRESENCE))
			frightening_presence(ch, opponent);

		room = ch->in_room;

		if (room == NOWHERE)
		{
			logit(LOG_DEBUG, "perfom_violence by (%s) in fight.c in NOWHERE!",
			      GET_NAME(opponent));
		}

		num_hits = 0;
		damAccumulator = 0;
		const attack_continuation cadence_continuation =
			begin_attack_continuation(ch, opponent);
		bool cadence_can_continue = true;

		for (i = 0; i < real_attacks; i++)
		{
			if (!GET_OPPONENT(ch) || !is_char_in_room(ch, room) ||
			    !is_char_in_room(opponent, room))
			{
				break;
			}
			// Monk % maxhealth damage.
			if (attacks[i / div_attacks] == WEAR_NONE)
			{
				if (monk_superhit(ch, opponent, &damAccumulator))
					num_hits++;
			}
			else if (pv_common(ch, opponent, ch->equipment[attacks[i / div_attacks]],
					   &damAccumulator))
			{
				num_hits++;
			}

			const attack_continuation_result after_attack =
				check_attack_continuation(cadence_continuation);
			if (!after_attack.can_continue() ||
			    !is_char_in_room(after_attack.actor, room) ||
			    !is_char_in_room(after_attack.target, room))
			{
				cadence_can_continue = false;
				break;
			}
			ch = after_attack.actor;
			opponent = after_attack.target;
		}

		if (!cadence_can_continue)
			continue;

		if (!is_char_in_room(opponent, room) || !is_char_in_room(ch, room))
			continue;

		if (is_char_in_room(ch, room) && IS_NPC(ch) && IS_AWAKE(ch) && CAN_ACT(ch))
			MobCombat(ch);

		// NPC effects can kill or move either participant.
		if (!is_char_in_room(opponent, room) || !is_char_in_room(ch, room))
			continue;

		appear(ch);
		appear(opponent);

		snprintf(GBuf1, MAX_STRING_LENGTH,
			 "%sYou attack $N.%s [&+R%d&n hits] [&+R%d&n damage]",
			 (IS_PC(ch) && !IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G-=[&n" :
										       "",
			 (IS_PC(ch) && !IS_SET(ch->specials.act2, PLR2_BATTLEALERT)) ? "&+G]=-&n" :
										       "",
			 num_hits, damAccumulator);
		act(GBuf1, FALSE, ch, 0, opponent, TO_CHAR | ACT_TERSE);
		snprintf(GBuf1, MAX_STRING_LENGTH,
			 "%s$n attacks you.%s [&+R%d&n hits] [&+R%d&n damage]",
			 (IS_PC(opponent) && !IS_SET(opponent->specials.act2, PLR2_BATTLEALERT)) ?
				 "&+R-=[&n" :
				 "",
			 (IS_PC(opponent) && !IS_SET(opponent->specials.act2, PLR2_BATTLEALERT)) ?
				 "&+R]=-&n" :
				 "",
			 num_hits, damAccumulator);
		act(GBuf1, FALSE, ch, 0, opponent, TO_VICT | ACT_TERSE);
		snprintf(GBuf1, MAX_STRING_LENGTH, "$n attacks $N. [&+R%d&n hits]", num_hits);
		act(GBuf1, FALSE, ch, 0, opponent, TO_NOTVICT | ACT_TERSE);
	}

	/* Now room_rnums contains rooms where combat took place. Handle
		   each room once. We don't add any checks here, since they will
		   be done by the flagged mobs. */
	for (it = room_rnums.begin(); it != room_rnums.end(); it++)
	{
		for (door = 0; door < NUM_EXITS; door++)
		{
			if (!VIRTUAL_EXIT(*it, door))
				continue;
			nearby_room = VIRTUAL_EXIT(*it, door)->to_room;
			if (nearby_room == NOWHERE)
				continue;
			for (tmp_ch = world[nearby_room].people; tmp_ch;
			     tmp_ch = tmp_ch->next_in_room)
			{
				if (IS_NPC(tmp_ch))
					SET_BIT(tmp_ch->specials.act2, ACT2_COMBAT_NEARBY);
			}
		}
	}
}
