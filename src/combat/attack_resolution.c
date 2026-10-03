/* Per-swing attack resolution shared by combat cadence. */
#include "combat/attack_resolution.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "combat/attack_continuation.h"
#include "combat/defense_resolution.h"
#include "combat/damage.h"
#include "combat/grapple.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/map.h"
#include <stdio.h>

/* items worn on those slots by the victim will be checked for defensive procs,
   order is important - if weapon absorbs, ioun cant deflect etc. */
int proccing_slots[] = { PRIMARY_WEAPON, SECONDARY_WEAPON, THIRD_WEAPON,   FOURTH_WEAPON,
			 WEAR_SHIELD,	 WEAR_HANDS,	   WEAR_HANDS_2,   WEAR_BODY,
			 WEAR_ABOUT,	 WEAR_IOUN,	   WEAR_HEAD,	   WEAR_HORN,
			 WEAR_NECK_1,	 WEAR_NECK_2,	   WEAR_ARMS,	   WEAR_ARMS_2,
			 WEAR_WRIST_LR,	 WEAR_WRIST_LL,	   WEAR_FINGER_R,  WEAR_FINGER_L,
			 WEAR_NOSE,	 WEAR_TAIL,	   WEAR_LEGS,	   WEAR_FEET,
			 WEAR_EYES,	 WEAR_FACE,	   WEAR_EARRING_R, WEAR_EARRING_L,
			 WEAR_WAIST,	 WEAR_QUIVER,	   GUILD_INSIGNIA };

extern bool divine_blessing_parry(P_char, P_char);
extern P_room world;
extern Skill skills[];

void double_strike(P_char ch, P_char victim, P_obj wpn)
{
	P_char tch;
	int count;
	int chosen;

	if (!wpn)
		return;

	if (wpn == ch->equipment[PRIMARY_WEAPON])
		wpn = ch->equipment[SECONDARY_WEAPON];
	else
		wpn = ch->equipment[PRIMARY_WEAPON];

	if (!wpn)
		return;

	for (tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room)
	{
		if (IS_TRUSTED(tch) || !on_front_line(tch) || tch == ch || !IS_FIGHTING(tch) ||
		    tch == victim || (ch->group && ch->group == tch->group))
			continue;
		count++;
	}

	if (count == 0)
		return;

	chosen = number(0, count - 1);

	for (tch = world[ch->in_room].people, count = 0; tch; tch = tch->next_in_room)
	{
		if (IS_TRUSTED(tch) || !on_front_line(tch) || tch == ch || !IS_FIGHTING(tch) ||
		    tch == victim || (ch->group && ch->group == tch->group))
			continue;
		if (count == chosen)
			break;
		count++;
	}

	if (tch == NULL)
		return;

	act("With a swift turn you lash at $N.", FALSE, ch, 0, tch, TO_CHAR | ACT_NOTTERSE);
	act("With a swift turn $n lashes at $N.", FALSE, ch, 0, tch, TO_NOTVICT | ACT_NOTTERSE);
	act("With a swift turn $n lashes at you!", FALSE, ch, 0, tch, TO_VICT | ACT_NOTTERSE);
	hit(ch, tch, wpn);
}

// This does % maxhealth damage, costs the monk 2 attacks and is not blockable.
bool monk_superhit(P_char ch, P_char victim, int *damAccumulator)
{
	static struct damage_messages monk_superhit_messages = {
		"You hit a pressure point on $N with additional &+rforce&n.",
		"$n's finger drives &+Rdeep&N inside you.",
		"$n pokes $N &+Rreally&n hard, making you wince.",
		"As you reach inside $N, you feel $S life force fade.",
		"$n reaches inside you and things begin to &+wfade &+Rred &+rthen &+Lblack&+W.&N",
		"As $n reaches inside $N, $N collapses."
	};
	double dam;
	int mindam;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return FALSE;

	if (!SanityCheck(ch, "monk_superhit") || !SanityCheck(victim, "monk_superhit"))
		return FALSE;

	if (!can_hit_target(ch, victim))
	{
		send_to_char("Seems that it's too crowded!\r\n", ch);
		return FALSE;
	}

	if ((IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim) && !IS_AFFECTED5(ch, AFF5_NOT_OFFENSIVE))
	{
		if (on_front_line(ch))
		{
			if (!on_front_line(victim))
			{
				act("$N tries to attack $n but can't quite reach!", TRUE, victim, 0,
				    ch, TO_NOTVICT);
				act("You try to attack $n but can't quite reach!", TRUE, victim, 0,
				    ch, TO_VICT);
				act("$N tries to attack you but can't quite reach!", TRUE, victim,
				    0, ch, TO_CHAR);
				return FALSE;
			}
		}
		else
		{
			send_to_char("Sorry, you can't quite reach them!\r\n", ch);
			return FALSE;
		}
	}

	// 1/2% max health damage _squared_ (min 10 damage).
	// This equates to 100 (5%) damage to a 2k hp target, or 25 dam (2.5%) to a 1000 hp target.
	dam = GET_MAX_HIT(victim) * .005;
	dam *= dam;
	// We do at least the damage a regular hit would do.
	// Note: This attack cost 2 attacks though, so they do less damage to low hp targets overall.
	mindam = MonkDamage(ch);
	if (dam < mindam)
		dam = mindam;
	// Caps at 225 on a 3k hps target (7.5% max health).
	else if (dam > 225)
		dam = 225;
	melee_damage(ch, victim, dam, PHSDAM_NOREDUCE | PHSDAM_TOUCH, &monk_superhit_messages,
		     damAccumulator);

	return TRUE;
}

//
// called once per attack in perform_violence - returns false if the ch dies somehow or another
//
// ch        = attacker
// opponent  = target
// weaponpos = weapon pos used, if -1 nothing is swapped
// canDodge  = self-explanatory
// num_hits  = incremented if opponent did not dodge somehow (as such, it is only somewhat accurate, eh?)
//

int pv_common(P_char ch, P_char opponent, const P_obj wpn, int *damAccumulator)
{
	int room, success = FALSE, wpn_skill, spell;
	P_obj item;
	struct proc_data data;

	if (!char_in_list(ch) || !IS_ALIVE(ch) || !char_in_list(opponent) || !IS_ALIVE(opponent))
		return FALSE;

	if (!SanityCheck(ch, "pv_common") || !SanityCheck(opponent, "pv_common"))
		return FALSE;

	room = ch->in_room;
	const attack_continuation continuation = begin_attack_continuation(ch, opponent, wpn);
	const auto refresh_attack_participants = [&]()
	{
		const attack_continuation_result checked = check_attack_continuation(continuation);
		if (!checked.can_continue())
			return false;
		ch = checked.actor;
		opponent = checked.target;
		return true;
	};

	/* weapon skill notch, check for automatic defensive skills */
	if (!((wpn_skill = required_weapon_skill(wpn)) &&
	      ((wpn_skill != SKILL_1H_FLAYING) && (wpn_skill != SKILL_2H_FLAYING)) &&
	      notch_skill(ch, wpn_skill, get_property("skill.notch.offensive.auto", 4))) &&
	    GET_STAT(opponent) == STAT_NORMAL && !IS_IMMOBILE(ch) &&
	    (has_innate(ch, INNATE_EYELESS) || CAN_SEE(opponent, ch) ||
	     GET_CHAR_SKILL(opponent, SKILL_BLINDFIGHTING) / 3 > number(0, 100)))
	{
		if (affected_by_spell(ch, SKILL_SHADOW_MOVEMENT))
		{
		}
		else if (!IS_IMMOBILE(opponent))
		{
			if (mangleSucceed(opponent, ch, wpn))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (parrySucceed(opponent, ch, wpn))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (divine_blessing_parry(opponent, ch))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (blockSucceed(opponent, ch, wpn))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (dodgeSucceed(opponent, ch, wpn))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (leapSucceed(opponent, ch))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
			if (MonkRiposte(opponent, ch, wpn))
				return FALSE;
			if (!refresh_attack_participants())
				return FALSE;
		}
	}
	/* defensive hit hook for equipped items - Tharkun */
	for (size_t proc_index = 0; proc_index < ARRAY_SIZE(proccing_slots); proc_index++)
	{
		if ((item = opponent->equipment[proccing_slots[proc_index]]) == NULL)
			continue;

		if (IS_PC_PET(opponent) && OBJ_VNUM(item) == 1251)
			continue;

		if (obj_index[item->R_num].func.obj != NULL)
		{
			data.victim = ch;
			if (invoke_object_special(item, opponent, CMD_GOTHIT, (char *)&data))
			{
				return FALSE;
			}
			if (!refresh_attack_participants())
				return FALSE;
		}
	}

	/* defensive hit hook for mob procs - Torgal */
	if (IS_ALIVE(opponent) && IS_NPC(opponent) && mob_index[GET_RNUM(opponent)].func.mob &&
	    !affected_by_spell(opponent, TAG_CONJURED_PET))
	{
		data.victim = ch;

		if ((*mob_index[GET_RNUM(opponent)].func.mob)(opponent, ch, CMD_GOTHIT,
							      (char *)&data))
		{
			return FALSE;
		}
		if (!refresh_attack_participants())
			return FALSE;
	}

	if (hit(ch, opponent, wpn, damAccumulator))
		success = TRUE;
	if (!refresh_attack_participants())
		return success;

	if (success && IS_ALIVE(opponent) && GET_POS(opponent) == POS_STANDING &&
	    GET_CHAR_SKILL(opponent, SKILL_ARMLOCK))
	{
		armlock_check(ch, opponent);
		if (!refresh_attack_participants())
			return success;
	}

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_ZEALOT) && success && IS_ALIVE(ch))
	{
		int devotion = GET_CHAR_SKILL(ch, SKILL_DEVOTION);
		int chance1 = 0, bonus1 = 0;

		if (IS_NPC(ch) && IS_ELITE(ch) && !IS_PC_PET(ch))
			devotion = GET_LEVEL(ch) * 2;

		if (devotion && devotion > 0)
		{
			if (devotion >= 100)
				bonus1 += 3;
			else if (devotion >= 50)
				bonus1 += 2;
			else
				bonus1 += 1;
		}

		chance1 = (int)(get_property("zealots.memHitChance", 3)) + bonus1;

		//    debug("chance to recover spell is (%d).", chance1);

		if (number(1, 100) <= chance1)
		{
			if ((spell = memorize_last_spell(ch)))
			{
				if (!refresh_attack_participants())
					return success;
				char buf[256];
				snprintf(
					buf, 256,
					"%s's essence &+Cempowers you&n and you are rewarded with &+G%s!\n",
					get_god_name(ch), skills[spell].name);
				send_to_char(buf, ch);
			}
		}
	}

	if (!refresh_attack_participants() || !is_char_in_room(ch, room))
	{
		return success;
	}
	bool double_strike_ready =
		notch_skill(ch, SKILL_DOUBLE_STRIKE, get_property("skill.notch.offensive.auto", 4));
	if (!refresh_attack_participants())
		return success;
	if (!double_strike_ready)
		double_strike_ready = GET_CHAR_SKILL(ch, SKILL_DOUBLE_STRIKE) / 20 >
					      number(0, 100) ||
				      (affected_by_spell(ch, SKILL_WHIRLWIND) && !number(0, 2));
	if (double_strike_ready)
	{
		double_strike(ch, opponent, wpn);
	}

	return success;
}
