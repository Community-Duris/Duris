/* Combat engagement state and target management. */
#include "core/prototypes.h"
#include "world/character_maintenance.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "combat/damage.h"
#include "combat/training_dummy.h"
#include "classes/dreadlord.h"
#include "item/objmisc.h"
#include "economy/collector_presence.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "telemetry/telemetry_runtime.h"
#include "world/map.h"
#include "world/world_activity.h"
#include <stdio.h>
#include <string.h>

P_char combat_list = 0; /* head of l-list of fighting chars  */
P_char combat_next_ch = 0; /* Next in combat global trick    */

void StopAllAttackers(P_char ch)
{
	P_char t_ch, hold;

	if (!ch)
		return;

	for (t_ch = combat_list; t_ch; t_ch = hold)
	{
		hold = t_ch->specials.next_fighting;
		if (GET_OPPONENT(t_ch) == ch)
		{
			t_ch->specials.was_fighting = ch;
			stop_fighting(t_ch);
		}
	}
}

void StopMercifulAttackers(P_char ch)
{
	P_char t_ch, hold;

	if (!ch)
		return;

	for (t_ch = combat_list; t_ch; t_ch = hold)
	{
		hold = t_ch->specials.next_fighting;
		if ((GET_OPPONENT(t_ch) == ch) && !affected_by_spell(t_ch, SKILL_BERSERK) &&
		    ((IS_PC(t_ch) && !IS_SET(t_ch->specials.act, PLR_VICIOUS)) ||
		     (IS_NPC(t_ch) && !is_aggr_to(t_ch, ch))))
		{
			if (GET_CLASS(t_ch, CLASS_PALADIN) && ch->specials.alignment < -349)
				continue;
			stop_fighting(t_ch);
		}
	}
}

static void telemetry_combat_context_changed(P_char ch)
{
	if (!ch)
		return;
	if (IS_PC(ch) && ch->desc && ch->desc->connected == CON_PLAYING)
		(void)telemetry_runtime_game_context(ch, ch->desc);
	telemetry_runtime_game_combat_context(ch);
}

/* start one char fighting another (yes, it is horrible, I know... ) */
void set_fighting(P_char ch, P_char vict)
{
	P_char victim = vict;
	char Gbuf[10];
	if (collector_presence_is_npc(ch) || collector_presence_is_npc(victim))
		return;

	if ((ch == victim) || !SanityCheck(ch, "set_fighting - ch") ||
	    !SanityCheck(victim, "set_fighting - victim"))
	{
		return;
	}
	if (training_dummy_is(ch))
		return;
	if (!training_dummy_target_allowed(ch, victim))
	{
		training_dummy_retarget_nonpet(ch, victim);
		return;
	}

	if (IS_FIGHTING(ch) || ch->specials.next_fighting)
	{
		logit(LOG_EXIT, "assert: set_fighting() when already fighting");
		return;
	}
	/*
		 * new reality mode, unless they are adjacent in a SINGLE_FILE room,
		 * they can't hit each other, except with spells/breath, so they don't
		 * start fighting either.  Make sure this check is first, as it can
		 * change the intended victim. JAB
		 */

	if ((IS_PC(ch) || IS_PC_PET(ch)) && // IS_PC(vict) &&
	    (!on_front_line(ch) || !on_front_line(vict)) &&
	    !(ch->equipment[PRIMARY_WEAPON] && IS_REACH_WEAPON(ch->equipment[PRIMARY_WEAPON])))
		return;

	if (IS_ROOM(ch->in_room, ROOM_SINGLE_FILE) && !AdjacentInRoom(ch, victim))
	{
		if (IS_PC(ch) || !(victim = PickTarget(ch)))
			return;
	}

	if (!can_hit_target(ch, victim))
	{
		send_to_char("You can't seem to find room!\r\n", ch);
		return;
	}

	if (IS_IMMOBILE(ch) || !IS_AWAKE(ch))
		return;

	if (IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_AGGIMMUNE))
		return;

	if (IS_TRUSTED(victim) && IS_SET(victim->specials.act, PLR_AGGIMMUNE))
	{
		if (IS_FIGHTING(victim))
			stop_fighting(victim);
		return;
	}
	if (affected_by_spell(ch, SONG_CHARMING) &&
	    ((ch->following && (ch->following == victim)) ||
	     (victim->following && (victim->following == ch))))
		affect_from_char(ch, SONG_CHARMING);

	if (affected_by_spell(ch, SONG_SLEEP))
		affect_from_char(ch, SONG_SLEEP);

	if (affected_by_spell(ch, SPELL_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);

	if (IS_AFFECTED(ch, AFF_SLEEP))
	{
		REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
		telemetry_runtime_game_control_changed(ch);
	}

	if (IS_AFFECTED(ch, AFF_SNEAK))
	{
		if (affected_by_spell(ch, SKILL_SNEAK))
			affect_from_char(ch, SKILL_SNEAK);
		if (IS_AFFECTED(ch, AFF_SNEAK))
			REMOVE_BIT(ch->specials.affected_by, AFF_SNEAK);
	}
	if (IS_AFFECTED(ch, AFF_HIDE))
	{
		act("$n comes out of hiding!", TRUE, ch, 0, 0, TO_ROOM);
		REMOVE_BIT(ch->specials.affected_by, AFF_HIDE);
	}

	appear(ch);

	if (IS_AFFECTED(victim, AFF_MEDITATE))
	{
		act("$n is disrupted from meditation.", TRUE, victim, 0, 0, TO_ROOM);
		stop_meditation(victim);
	}

	if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE))
	{
		struct affected_type *afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
		afp->modifier = 0;
	}

	/*
		 * paging mode was doing weird things when you got attacked, so nuke
		 * it (for victim) when attacked.  JAB
		 */
	if (victim->desc && victim->desc->showstr_vector)
	{
		strcpy(Gbuf, "q\n");
		show_string(victim->desc, Gbuf);
	}
	GET_OPPONENT(ch) = victim;
	ch->specials.next_fighting = combat_list;
	combat_list = ch;
	world_activity_promote_character(ch);
	world_activity_promote_character(victim);
	telemetry_combat_context_changed(ch);
	(void)telemetry_runtime_game_combat_engage(ch, victim);

	if (ch->in_room >= 0)
		gmcp_mark_room_dirty(ch->in_room);

	if (!IS_DRAGOON(ch))
		stop_memorizing(ch);
	if (!IS_DRAGOON(victim))
		stop_memorizing(victim);

	/* call for initial 'dragon fear' check.  -JAB */

	if (IS_NPC(ch) && (IS_DRAGON(ch) || IS_AVATAR(ch)) && !IS_MORPH(ch) && !IS_PC_PET(ch))
	{
		if (!number(0, 2)) // Attack and roar.
			DragonCombat(ch, TRUE);
		else
			DragonCombat(ch, FALSE); // Attack but don't roar.
	}

	/* Code for memory */

	if (ch && victim)
	{
		if (HAS_MEMORY(victim))
		{
			if (IS_PC(ch))
			{
				if (!(IS_TRUSTED(ch) && IS_SET(ch->specials.act, PLR_AGGIMMUNE)))
					if (GET_STAT(victim) > STAT_INCAP)
						remember(victim, ch);
			}
			else if (IS_PC_PET(ch) && (GET_MASTER(ch)->in_room == ch->in_room) &&
				 CAN_SEE(victim, GET_MASTER(ch)))
			{
				if (!(IS_TRUSTED(GET_MASTER(ch)) &&
				      IS_SET(GET_MASTER(ch)->specials.act, PLR_AGGIMMUNE)))
					if (GET_STAT(victim) > STAT_INCAP)
						remember(victim, GET_MASTER(ch));
			}
		}
	}
	/*
		 * used to change position directly to POSITION_FIGHTING, causing some
		 * strange things.  This is better.  JAB
		 */

	if (GET_STAT(ch) == STAT_SLEEPING)
	{
		send_to_char("You are VERY rudely awakened!\r\n", ch);
		act("$n has a RUDE awakening!", TRUE, ch, 0, 0, TO_ROOM);
		SET_POS(ch, POS_SITTING + GET_STAT(ch));
		do_wake(ch, 0, -4);
	}
	if (P_char mount = get_linked_char(ch, LNK_RIDING))
	{
		if (!GET_CHAR_SKILL(ch, SKILL_MOUNTED_COMBAT) /* && !is_natural_mount(ch, mount)*/)
		{
			send_to_char("I'm afraid you aren't quite up to mounted combat.\r\n", ch);
			act("$n quickly slides off $N's back.", TRUE, ch, 0, mount, TO_NOTVICT);
			stop_riding(ch);
		}
	}

	if (has_innate(ch, INNATE_ANTI_EVIL) && IS_EVIL(vict))
		do_innate_anti_evil(ch, vict);
	if (has_innate(ch, INNATE_DECREPIFY))
		do_innate_decrepify(ch, vict);
	if (GET_CHAR_SKILL(ch, SKILL_DREAD_WRATH))
		do_dread_wrath(ch, vict);
}

void MoveAllAttackers(P_char ch, P_char v)
{
	P_char t_ch, hold;

	if (!ch || !v)
		return;

	for (t_ch = combat_list; t_ch; t_ch = hold)
	{
		hold = t_ch->specials.next_fighting;
		if (GET_OPPONENT(t_ch) == ch)
		{
			t_ch->specials.was_fighting = ch;
			stop_fighting(t_ch);
			set_fighting(t_ch, v);
		}
	}
}

/* picks a new random target for mayhem */

void retarget_event(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	P_char target;
	char buf[255];

	if (!ch)
		return;

	/* no need to retarget when target _Does_ exist */
	if (GET_OPPONENT(ch))
		return;

	/* retarget blues: */
	if ((ch->in_room < 0) /* || (ch->in_room > 65536) */)
		return;
	if (!MIN_POS(ch, POS_STANDING + STAT_RESTING))
		return;
	target = PickTarget(ch);
	if (!target)
		return;
	if (IS_NPC(ch))
		MobStartFight(ch, target);
	else
	{
		snprintf(buf, 255, "hit %s", GET_NAME(target));
		command_interpreter(ch, buf);
	}
}

/* remove a char from the list of fighting chars */

void stop_fighting(P_char ch)
{
	P_char tmp;

	if (!SanityCheck(ch, "stop_fighting") || !IS_FIGHTING(ch))
		return;

	if (IS_SET(ch->specials.affected_by3, AFF3_TRACKING)) // needed for new track *Alv*
		REMOVE_BIT(ch->specials.affected_by3, AFF3_TRACKING);

	ch->specials.was_fighting = GET_OPPONENT(ch);

	if (ch == combat_next_ch)
		combat_next_ch = ch->specials.next_fighting;
	if (combat_list == ch)
		combat_list = ch->specials.next_fighting;
	else
	{
		for (tmp = combat_list; tmp && (tmp->specials.next_fighting != ch);
		     tmp = tmp->specials.next_fighting)
			;
		if (!tmp)
		{
			logit(LOG_EXIT, "%s not found in combat_list stop_fighting()",
			      GET_NAME(ch));
		}
		else
			tmp->specials.next_fighting = ch->specials.next_fighting;
	}

	ch->specials.next_fighting = NULL;
	GET_OPPONENT(ch) = NULL;
	telemetry_combat_context_changed(ch);

	if (affected_by_spell(ch, SPELL_CEGILUNE_BLADE))
	{
		struct affected_type *afp = get_spell_from_char(ch, SPELL_CEGILUNE_BLADE);
		afp->modifier = 0;
	}

	if (GET_CHAR_SKILL(ch, SKILL_LANCE_CHARGE) != 0)
		set_short_affected_by(ch, SKILL_LANCE_CHARGE,
				      PULSE_VIOLENCE / 2); // to prevent flee/charge

	if (ch->in_room >= 0)
		gmcp_mark_room_dirty(ch->in_room);

	/* Notify web client that combat has ended */
	gmcp_combat_end(ch);

	update_pos(ch);
}

/*
 * used to restrict targeting in SINGLE_FILE rooms, was simple, but got
 * uglier when I started skipping immortals.  Even worse now that I have
 * to skip 'wraithform' chars too.  JAB
 */
bool AdjacentInRoom(P_char ch, P_char ch2)
{
	P_char t_ch, t_ch2;

	if (!ch || !ch2 || (ch->in_room != ch2->in_room) || (ch->in_room == NOWHERE))
		return FALSE;

	if (IS_TRUSTED(ch) || IS_TRUSTED(ch2) || IS_AFFECTED(ch, AFF_WRAITHFORM) ||
	    IS_AFFECTED(ch2, AFF_WRAITHFORM))
		return TRUE;

	t_ch = world[ch->in_room].people;

	if (!t_ch)
		return FALSE;

	/*
	 * find first of ch/ch2 in room list
	 */

	while (t_ch && (t_ch != ch) && (t_ch != ch2))
		t_ch = t_ch->next_in_room;

	if (!t_ch)
		return FALSE;

	t_ch2 = t_ch->next_in_room;

	/*
	 * find the second of ch/ch2 in room list, skipping immorts
	 */

	while (t_ch2 && (IS_TRUSTED(t_ch2) || IS_AFFECTED(t_ch2, AFF_WRAITHFORM)))
		t_ch2 = t_ch2->next_in_room;

	if (!t_ch2)
		return FALSE;

	/*
	 * yup, they are effectively adjacent
	 */
	if (((t_ch == ch) && (t_ch2 == ch2)) || (t_ch2 == ch))
		return TRUE;

	return FALSE;
}

/* new function to check max attacker */
bool can_hit_target(P_char ch, P_char vict)
{
	int table_1[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
	int table_2[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };

	P_char opponent, next_ch;
	int size_total = 0;
	int size_max = 0;
	int num_opponents = 0;

	if (IS_NPC(ch) || IS_NPC(vict))
		return TRUE;

	size_max = table_2[GET_ALT_SIZE(vict)];

	for (opponent = combat_list; opponent; opponent = next_ch)
	{
		next_ch = opponent->specials.next_fighting;
		if ((GET_OPPONENT(opponent) == vict) && (opponent != ch) && IS_PC(ch) &&
		    IS_PC(opponent))
		{
			num_opponents++;
			size_total += table_1[GET_ALT_SIZE(opponent)];
		}
	}

	if (num_opponents > 3)
		return FALSE;

	if (size_total > size_max)
		return FALSE;

	return TRUE;
}

void engage(P_char ch, P_char victim)
{
	if (!IS_FIGHTING(ch))
		set_fighting(ch, victim);

	if (!IS_FIGHTING(victim))
		set_fighting(victim, ch);
}

unsigned int calculate_ch_state(P_char ch)
{
	if (ch)
	{
		if (GET_HIT(ch) < -10)
			return STAT_DEAD;
		else if (GET_HIT(ch) <= -6)
			return STAT_DYING;
		else if (GET_HIT(ch) <= -3)
			return STAT_INCAP;
		else if (((GET_STAT(ch) == STAT_SLEEPING) || (GET_STAT(ch) == STAT_RESTING)) &&
			 (IS_FIGHTING(ch) || NumAttackers(ch)))
			return STAT_NORMAL;
		else if (GET_STAT(ch) < STAT_SLEEPING)
			return STAT_RESTING;
		else
			return GET_STAT(ch);
	}
	return STAT_DEAD;
}

void update_pos(P_char ch)
{
	int pos, tmp;
	unsigned int stat;
	P_char mount;

	if (!ch)
	{
		logit(LOG_EXIT, "assert: update_pos() - no ch");
		return;
	}
	if ((IS_NPC(ch) && ch->only.npc == NULL) || (IS_PC(ch) && ch->only.pc == NULL))
		return;

	if (IS_FIGHTING(ch))
		if (ch->in_room != GET_OPPONENT(ch)->in_room)
			stop_fighting(ch);

	mount = get_linked_char(ch, LNK_RIDING);
	if (mount && mount->in_room != ch->in_room)
		stop_riding(ch);

	stat = calculate_ch_state(ch);
	pos = GET_POS(ch);

	/*
	 * quadrupeds are much more stable than bipeds, so they skip this
	 * little indignity. JAB
	 */

	if ((IS_HUMANOID(ch) || IS_GIANT(ch)) && (stat < STAT_RESTING))
	{
		tmp = 0;
		switch (GET_POS(ch))
		{
		case POS_PRONE:
			/*
				 * nada
				 */
			break;
		case POS_KNEELING:
			/*
				 * fairly stable, (except riding) but there is that chance...
				 */
			switch (stat)
			{
			case STAT_DEAD:
				if (IS_RIDING(ch))
					tmp = 1;
				else if (!number(0, 9))
					tmp = 1;
				break;
			case STAT_DYING:
			case STAT_INCAP:
			case STAT_SLEEPING:
				if (IS_RIDING(ch))
				{
					if (!number(0, 1))
						tmp = 1;
				}
				else
				{
					if (!number(0, 9))
						tmp = 1;
				}
				break;
			}
			break;
		case POS_SITTING:
			/*
				 * moderately stable, esp. if mounted, but not completely...
				 */
			switch (stat)
			{
			case STAT_DEAD:
				if (IS_RIDING(ch))
				{
					if (!number(0, 1))
						tmp = 1;
				}
				else
				{
					if (!number(0, 9))
						tmp = 1;
				}
				break;
			case STAT_DYING:
			case STAT_INCAP:
			case STAT_SLEEPING:
				if (IS_RIDING(ch))
				{
					if (!number(0, 4))
						tmp = 1;
				}
				else
				{
					if (!number(0, 7))
						tmp = 1;
				}
				break;
			}
			break;
		case POS_STANDING:
			/*
				 * very unstable
				 */
			switch (stat)
			{
			case STAT_DEAD:
				/*
						 * they can die on their feet, but they aren't staying
						 * standing...
						 */
				tmp = 1;
				break;
			case STAT_DYING:
			case STAT_INCAP:
			case STAT_SLEEPING:
				if (IS_RIDING(ch))
					tmp = 1;
				else if (!number(0, 5))
					tmp = 1;
				break;
			}
			break;
		}
		if (tmp)
		{
			tmp = 0;
			/*
			 * they involuntarily changed position, can be real bad if
			 * they were mounted at the time. (damage + stun).  JAB
			 */
			if (mount)
			{
				tmp = 1;
				switch (GET_POS(mount))
				{
				case POS_PRONE:
				case POS_KNEELING:
				case POS_SITTING:
					/*
						 * not far to fall, no damage, just stop riding
						 */
					break;
				case POS_STANDING:
					/*
						 * ouchness
						 */
					Stun(ch, ch, dice(3, 4) * 4,
					     FALSE); /* 3 rounds max, avg, ~1.5 */
					break;
				}
				act("$n falls off $s mount!", TRUE, ch, 0, 0, TO_ROOM);
				stop_riding(ch);
			}
			else
			{
				switch (pos)
				{
				case POS_PRONE:
				case POS_KNEELING:
				case POS_SITTING:
					/*
						 * no damage, just go prone.
						 */
					break;
				case POS_STANDING:
					/* if they are dying, DON'T have them take damage from
						   this slumping BS, or we end up with chars that are
						   "dead" until hit_regen kills them */
					if (stat <= STAT_DYING)
						break;

					/*
						 * minor owie, maybe
						 */
					tmp = 0;
					if (!number(0, 2))
					{
						if (GET_STAT(ch) > STAT_INCAP)
							Stun(ch, ch, dice(2, 3) * 4, FALSE); /*
								                                      * 1.5 rounds max,
								                                      * avg, ~1
								                                      */
						tmp = 1; /*
							                                          * will wake normal sleepers
							                                          */
					}
					act("$n slumps to the ground.", TRUE, ch, 0, 0, TO_ROOM);
					break;
				}
			}
			pos = POS_PRONE;

			/*
			 * since they can take damage in this routine, check again.
			 * JAB
			 */
			if (GET_HIT(ch) < -10)
				stat = STAT_DEAD;
			else if (GET_HIT(ch) <= -6)
				stat = STAT_DYING;
			else if (GET_HIT(ch) <= -3)
				stat = STAT_INCAP;
			else if (((GET_STAT(ch) == STAT_SLEEPING) ||
				  (GET_STAT(ch) == STAT_RESTING)) &&
				 (IS_FIGHTING(ch) || NumAttackers(ch)))
				stat = STAT_NORMAL;
			else if (GET_STAT(ch) < STAT_SLEEPING)
				stat = STAT_RESTING;
			else
				stat = GET_STAT(ch); /*
				                      * SLEEPING/RESTING/NORMAL
				                      */

			/*
			 * if they are just normally asleep, falling will wake them
			 * (mostly), if they were magically asleep, taking damage from
			 * falling down will break the spell.  If they wind up worse
			 * than sleeping, ah well...
			 */

			if (tmp && (stat < STAT_RESTING))
			{
				if (affected_by_spell(ch, SPELL_SLEEP))
				{
					REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
					telemetry_runtime_game_control_changed(ch);
					affect_from_char(ch, SPELL_SLEEP);
				}
				if (affected_by_spell(ch, SONG_SLEEP))
				{
					REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
					telemetry_runtime_game_control_changed(ch);
					affect_from_char(ch, SONG_SLEEP);
				}
				if (stat == STAT_SLEEPING)
				{
					stat = STAT_NORMAL;
					send_to_char(
						"Huh?  What!?  You find yourself laying on the ground!\r\n",
						ch);
				}
			}
		}
	}
	if ((GET_STAT(ch) == STAT_SLEEPING) && (stat > STAT_SLEEPING))
	{
		act("$n has a RUDE awakening!", TRUE, ch, 0, 0, TO_ROOM);
		if (affected_by_spell(ch, SPELL_SLEEP))
		{
			REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
			telemetry_runtime_game_control_changed(ch);
			affect_from_char(ch, SPELL_SLEEP);
		}
		if (affected_by_spell(ch, SONG_SLEEP))
		{
			REMOVE_BIT(ch->specials.affected_by, AFF_SLEEP);
			telemetry_runtime_game_control_changed(ch);
			affect_from_char(ch, SONG_SLEEP);
		}
		do_wake(ch, 0, -4);
		if (IS_NPC(ch))
		{
			do_stand(ch, 0, 0);
			do_alert(ch, 0, 0);
		}
	}
	/*
	 * finally, set new position and status
	 */

	SET_POS(ch, pos + stat);
	if (stat == STAT_DEAD)
	{
		if (IS_FIGHTING(ch))
			stop_fighting(ch);
		StopAllAttackers(ch);
		stat = STAT_DYING; /*
		                    * reason being, killing people in
		                    * update_pos, would cause horrible
		                    * logistic nightmares.  If they are dead,
		                    * setting them dying will handle most
		                    * cases. If THIS causes problems, may
		                    * have to bite the bullet and change the
		                    * code in LOTS of places. Hopefully this
		                    * will suffice.  JAB
		                    */
		/*    SET_POS(ch, pos + stat);*/
	}
	if (pos != POS_STANDING)
	{
		clear_links(ch, LNK_FLANKING);
		clear_links(ch, LNK_CIRCLING);
	}

	if (IS_IMMOBILE(ch))
	{
		if (IS_FIGHTING(ch))
			stop_fighting(ch);
		StopMercifulAttackers(ch);
	}
	/*
	 * final check for mobs, if they can assume their default position
	 */
	/*
	 * added check - DTS 7/11/95
	 */
	if (IS_NPC(ch) && (stat > STAT_SLEEPING) && !IS_FIGHTING(ch) && CAN_ACT(ch) &&
	    ((ch->only.npc->default_pos & STAT_MASK) >= STAT_SLEEPING) &&
	    (!HAS_MEMORY(ch) || !GET_MEMORY(ch)))
		ch->specials.position = ch->only.npc->default_pos;
	character_maintenance_changed(ch);
}

/*
 * This routine is here to solve some message timing problems, called from
 * several places in damage(), checks to see if victim should start
 * fighting ch.  JAB
 * Returns one of DAM_NONEDEAD, DAM_VICTDEAD, DAM_CHARDEAD, DAM_BOTHDEAD.
 */
int attack_back(P_char ch, P_char victim, int physical)
{
	const bool attacker_in_list = ch && char_in_list(ch);
	const bool victim_in_list = victim && char_in_list(victim);
	if (!attacker_in_list && !victim_in_list)
		return DAM_BOTHDEAD;
	if (!attacker_in_list)
		return DAM_CHARDEAD;
	if (!victim_in_list)
		return DAM_VICTDEAD;

	if (training_dummy_is(ch) || training_dummy_is(victim))
	{
		if (victim && IS_NPC(ch) && !IS_PC_PET(ch) && training_dummy_is(victim))
			training_dummy_retarget_nonpet(ch, victim);
		return DAM_NONEDEAD;
	}

	if (!IS_ALIVE(ch))
	{
		if (!IS_ALIVE(victim))
			return DAM_BOTHDEAD;
		return DAM_CHARDEAD;
	}
	if (victim)
		update_pos(victim);
	if (!IS_ALIVE(victim))
		return DAM_VICTDEAD;
	if (IS_FIGHTING(victim))
		return DAM_NONEDEAD;
	const uint64_t actor_runtime_id = ch->runtime_id;
	const uint64_t victim_runtime_id = victim->runtime_id;

	if (ch->in_room != victim->in_room || ch->specials.z_cord != victim->specials.z_cord)
	{
		if (IS_NPC(victim))
		{
			MobRetaliateRange(victim, ch);
			ch = find_character_by_runtime_id(actor_runtime_id);
			victim = find_character_by_runtime_id(victim_runtime_id);
			const bool attacker_alive = ch && IS_ALIVE(ch);
			const bool victim_alive = victim && IS_ALIVE(victim);
			if (!attacker_alive && !victim_alive)
				return DAM_BOTHDEAD;
			if (!attacker_alive)
				return DAM_CHARDEAD;
			if (!victim_alive)
				return DAM_VICTDEAD;
		}
	}
	// Can't very well attack back if either ch or victim is back ranked!
	else if (!physical && (IS_PC(ch) || IS_PC_PET(ch)) && IS_PC(victim) &&
		 (!IS_SET(ch->specials.act, PLR_VICIOUS) || IS_BACKRANKED(ch) ||
		  IS_BACKRANKED(victim)))
	{
		return DAM_NONEDEAD;
	}
	else if (!IS_IMMOBILE(ch))
	{
		set_fighting(victim, ch);
	}

	if (!IS_ALIVE(ch))
	{
		if (!IS_ALIVE(victim))
			return DAM_BOTHDEAD;
		return DAM_CHARDEAD;
	}
	if (!IS_ALIVE(victim))
		return DAM_VICTDEAD;
	return DAM_NONEDEAD;
}
