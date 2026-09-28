/*
 ***************************************************************************
 *  File: specs.monster_attacks.c                           Part of Duris  *
 *  Usage: shared monster attack helpers                                  *
 ***************************************************************************
 */

#include <stdio.h>
#include <string.h>

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "world/handler.h"
#include "core/utility.h"
#include "core/utils.h"
#include "combat/attack_continuation.h"
#include "combat/damage.h"
#include "combat/justice.h"
#include "magic/spells.h"

extern P_room world;
extern struct str_app_type str_app[];

void fetid_breath(P_char ch, P_char victim)
{
	int dam, level;
	bool knock = FALSE;
	bool feint = FALSE;

	struct damage_messages messages = {
		"$N &+gis hit by your fetid breath.",
		"$n &+gbreathes a stream of &+Gfetid gases&n&+g at you.",
		"$N &+gis covered with a stream of &+Gfetid gases&n&+g breathed by $n.",
		"$N &+gis dead, &+Gsuffocated&n&+g by your fetid breath.",
		"&+gYou are &+Gsuffocated&n&+g by the fetid gasses that $n&n&+g breathes on you.",
		"$n's &+gbreath kills $N &+gon the spot.",
		0
	};

	if (resists_spell(ch, victim))
		return;

	level = GET_LEVEL(ch);

	dam = dice(level, 8) + level;

	if (StatSave(victim, APPLY_CON, 0))
		dam >>= 1;

	if (!StatSave(victim, APPLY_AGI, 0))
		knock = TRUE;

	if (!StatSave(victim, APPLY_WIS, 0))
		feint = TRUE;

	spell_damage(ch, victim, dam, SPLDAM_GAS, SPLDAM_BREATH | SPLDAM_NOSHRUG, &messages);

	if (IS_ALIVE(victim) && knock)
	{
		SET_POS(victim, POS_SITTING + GET_STAT(victim));
		act("$n &+Lis knocked down by the power of &+gthe &+Gfetid&n&+g breath!", FALSE,
		    victim, 0, 0, TO_ROOM);
		send_to_char(
			"&+LYou are knocked down by the power of &+gthe &+Gfetid&n&+g breath!\n",
			victim);
	}

	if (IS_ALIVE(victim) && feint)
	{
		struct affected_type af;

		if (!check_freedom_of_movement(victim, number(0, 1)))
		{
			bzero(&af, sizeof(af));
			af.type = SPELL_MINOR_PARALYSIS;
			af.flags = AFFTYPE_SHORT;
			af.duration = WAIT_SEC;
			af.bitvector2 = AFF2_MINOR_PARALYSIS;

			affect_to_char(victim, &af);

			act("$n &+Wturns pale as some magical force occupies $s body, causing all motion to halt.",
			    FALSE, victim, 0, 0, TO_ROOM);
			send_to_char(
				"&+LYour body becomes like stone as the paralyzation takes effect.\n",
				victim);
			if (IS_FIGHTING(victim))
				stop_fighting(victim);
		}
	}
}

int do_fetid_breath(P_char ch)
{
	P_char tch, tch_next;

	for (tch = world[ch->in_room].people; tch; tch = tch_next)
	{
		tch_next = tch->next_in_room;

		if (IS_TRUSTED(tch))
			continue;

		if ((GET_OPPONENT(ch) == tch) || (GET_OPPONENT(tch) == ch))
		{
			fetid_breath(ch, tch);
			continue;
		}

		if ((get_linking_char(ch, LNK_RIDING) == tch) || (ch == tch) || grouped(ch, tch))
			continue;

		if (IS_NPC(tch) && !IS_PC_PET(tch))
			continue;

		fetid_breath(ch, tch);
	}
	return 0;
}

void hyena_bite(P_char ch, P_char victim)
{
	struct damage_messages messages = {
		"Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
		"$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
		"$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
		"Your shape blurs as you lash towards $N and sink your fangs in $S flesh.",
		"$n's shape blurs as $e lashes towards you and sinks $s fangs in your flesh.",
		"$n's shape blurs as $e lashes towards $N and sinks $s fangs in $S flesh.",
	};
	if (!ch || !char_in_list(ch) || !IS_ALIVE(ch) || !victim || !char_in_list(victim) ||
	    !IS_ALIVE(victim))
		return;

	int level = GET_LEVEL(ch);

	int dam = dice(level, 10);
	const attack_continuation bite_continuation = begin_attack_continuation(ch, victim);

	if (raw_damage(ch, victim, dam, RAWDAM_DEFAULT, &messages) != DAM_NONEDEAD)
		return;
	auto refresh_bite_participants = [&](const attack_continuation &continuation)
	{
		const attack_continuation_result after_callback =
			check_attack_continuation(continuation);
		if (!after_callback.can_continue())
			return false;

		ch = after_callback.actor;
		victim = after_callback.target;
		return true;
	};
	if (!refresh_bite_participants(bite_continuation))
		return;

	int i = 1 + GET_LEVEL(ch) / 12;
	while (i-- && !affected_by_spell(victim, SPELL_DISEASE))
	{
		const attack_continuation disease_continuation =
			begin_attack_continuation(ch, victim);
		spell_disease(GET_LEVEL(ch), ch, 0, 0, victim, 0);
		if (!refresh_bite_participants(disease_continuation))
			return;
	}
}

int devour(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_obj i, temp, next_obj;

	// Check for periodic event calls
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd != CMD_PERIODIC || !IS_AWAKE(ch))
		return FALSE;

	for (i = world[ch->in_room].contents; i; i = i->next_content)
	{
		if ((GET_ITEM_TYPE(i) == ITEM_FOOD) || (GET_ITEM_TYPE(i) == ITEM_CORPSE))
		{
			if (persistence_defer_corpse_room_release(i))
				return TRUE;
			if (GET_ITEM_TYPE(i) == ITEM_CORPSE)
				for (temp = i->contains; temp; temp = next_obj)
				{
					next_obj = temp->next_content;
					obj_from_obj(temp);
					obj_to_room(temp, ch->in_room);
				}
			if (IS_SET(i->value[1], PC_CORPSE))
			{
				logit(LOG_CORPSE, "%s devoured in room %d.", i->short_description,
				      world[i->loc.room].number);
			}
			act("$n savagely devours $p.", FALSE, ch, i, 0, TO_ROOM);
			extract_obj(i, TRUE); // Just food/empty corpse, but 'in game.'
			return (TRUE);
		}
	}
	return (FALSE);
}

/* Number of rooms archers can shoot from. */

#define NUM_ARCHERS 21
/* Number of rooms an archer can target. */
#define NUM_TARGETS 3
/* Archer accuracy percentage. */
#define HIT_CHANCE 30
/* Archer damage dice: 2d5 per hit. */
#define ARCHER_NUM_DICE 2
#define ARCHER_SIZE_DICE 5

int archer(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char targ;
	int i, j, k, gottem = FALSE;
	char buf[MAX_STRING_LENGTH], buf1[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];

	int to_from_rooms[NUM_ARCHERS][NUM_TARGETS + 1] = {
		/*
	     * archer room     target room #1     #2       #3
	     */
		{ 95518, 95509, 95508, 95507 }, { 95518, 95512, 95509, 95510 },
		{ 95522, 95509, 95508, 95507 }, { 95522, 95512, 95509, 95510 },
		{ 95516, 95512, 95510, 95511 }, { 95516, 95512, 95510, 95511 },
		{ 66005, 66095, 66130, 66129 }, { 8087, 3651, 3650, 3649 },
		{ 8213, 8214, 8215, 8216 },	{ 17367, 17024, 17023, 17022 },
		{ 17366, 17024, 17023, 17022 }, { 17052, 17021, 17022, 17032 },
		{ 17053, 17021, 17022, 17032 }, { 17054, 17021, 17022, 17032 },
		{ 17272, 17030, 17031, 17032 }, { 17273, 17030, 17031, 17032 },
		{ 75081, 75076, 75075, -1 },	{ 75088, 75087, 75084, -1 },
		{ 75094, 75093, 75095, -1 },	{ 77231, 77230, 77229, 77225 },
		{ 77237, 77234, 77232, 77233 }
	};

	if (cmd)
		return FALSE;

	if (GET_POS(ch) != POS_STANDING)
		return FALSE;

	snprintf(buf, MAX_STRING_LENGTH,
		 "You feel a sharp pain in your side as an arrow finds its mark!");
	snprintf(buf1, MAX_STRING_LENGTH, "You hear a dull thud as an arrow pierces $n!");
	snprintf(buf2, MAX_STRING_LENGTH, "An arrow whistles by your ear, barely missing you!");
	snprintf(buf3, MAX_STRING_LENGTH, "An arrow narrowly misses $n!");

	for (i = 0; i < NUM_ARCHERS; i++)
	{
		if (real_room(to_from_rooms[i][0]) == ch->in_room)
		{
			for (j = 1; j <= NUM_TARGETS; j++)
			{
				if ((k = real_room(to_from_rooms[i][j])) >= 0)
				{
					for (targ = world[k].people; targ;
					     targ = targ->next_in_room)
					{
						if (is_aggr_to(ch, targ))
						{
							if (number(1, 100) <= HIT_CHANCE)
							{
								act(buf, 1, ch, 0, targ, TO_VICT);
								act(buf1, 1, targ, 0, 0,
								    TO_NOTVICT);
								if (!IS_TRUSTED(targ))
									GET_HIT(targ) -= dice(
										ARCHER_NUM_DICE,
										ARCHER_SIZE_DICE);
								if (number(1, 100) <
								    (HIT_CHANCE / 4))
								{
									GET_HIT(targ) -=
										((GET_HIT(targ) /
										  10) *
										 8);
									send_to_char(
										"The arrow pierces extremely deep!\r\n",
										targ);
								}
								if (GET_HIT(targ) < -10)
								{
									send_to_char(
										"Alas, your wounds prove too much for you...\r\n",
										targ);
									die(targ, ch);
									return TRUE;
								}
								StartRegen(targ,
									   regen_resource::hit);
								update_pos(targ);
								gottem = TRUE;
							}
							else
							{
								act(buf2, 1, ch, 0, targ, TO_VICT);
								act(buf3, 1, targ, 0, 0,
								    TO_NOTVICT);
							}
						}
					}
				}
			}
		}
	}
	return gottem;
}

int poison(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd)
		return FALSE;

	if (!IS_FIGHTING(ch))
		return FALSE;

	if (GET_OPPONENT(ch) && (GET_OPPONENT(ch)->in_room == ch->in_room) &&
	    !number(0, (MAXLVL - GET_LEVEL(ch))))
	{
		act("$n bites $N!", 1, ch, 0, GET_OPPONENT(ch), TO_NOTVICT);
		act("$n bites you!", 1, ch, 0, GET_OPPONENT(ch), TO_VICT);
		poison_neurotoxin(GET_LEVEL(ch), ch, 0, 0, GET_OPPONENT(ch), 0);
		return TRUE;
	}
	return FALSE;
}

void event_tentacles(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	P_char tch;

	for (tch = world[ch->in_room].people; tch; tch = tch->next)
		if ((GET_OPPONENT(tch) == ch) && (tch->points.damnodice == 4) &&
		    (tch->points.damsizedice == 4) && (tch->specials.alignment == -200))
			break;

	if (!tch)
		REMOVE_BIT(ch->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
}

int tentacle(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	int dam = cmd / 1000;

	if (cmd == CMD_SET_PERIODIC)
		return FALSE;
	if (!dam || dam < 1)
		return FALSE;
	if (NewSaves(pl, SAVING_SPELL, 0) && (GET_POS(pl) == POS_STANDING))
	{
		REMOVE_BIT(ch->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
		act("$N breaks free of $n's grip and quickly eliminates $m!", TRUE, ch, 0, pl,
		    TO_NOTVICT);
		act("You finally manage to get free of $n's grip and kick $s ass!", TRUE, pl, 0, 0,
		    TO_CHAR);
		die(ch, pl);
		return TRUE;
	}
	else
	{
		add_event(event_tentacles, PULSE_VIOLENCE, pl, 0, 0, 0, 0, 0);
	}
	return FALSE;
}

/*
 * combat special for warhorses
 */
int warhorse(P_char ch, P_char /*pl*/, int cmd, char * /*arg*/)
{
	P_char vict, rider;
	int i;
	bool dropped_through = FALSE;

	/*
	 * check for periodic event call
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (!ch || !IS_AWAKE(ch) || cmd || !IS_FIGHTING(ch) || !CAN_ACT(ch))
		return FALSE;

	/*
	 * warhorses have 2 special attack forms, first is overbearing and trample,
	 * this will work on everything smaller than dragons or demons.  This attack
	 * works whether warhorse has a rider or not.  Second attack is a mule kick,
	 * this attack work on everything, but doesn't work very well if the warhorse
	 *
	 * has a rider.
	 */
	rider = get_linking_char(ch, LNK_RIDING);
	vict = GET_OPPONENT(ch);

	switch (number(1, 3))
	{
	case 1: /*
		         * overbear and trample
		         */
		if (IS_DEMON(vict) || IS_DRAGON(vict))
			return FALSE;

		if ((world[ch->in_room].sector_type != SECT_FIREPLANE) &&
		    (world[ch->in_room].sector_type > SECT_WATER_SWIM))
		{
			send_to_char("You have no footing here!\r\n", ch);
			return FALSE;
		}
		CharWait(ch, PULSE_VIOLENCE);
		if (!StatSave(vict, APPLY_AGI, (GET_LEVEL(vict) - GET_LEVEL(ch)) / 3))
		{
			/*
				 * a hit! basically does about what a bodyslam would, but not quite as
				 * bad
				 */
			if (GET_POS(vict) == POS_STANDING)
			{
				act("$n charges $N and knocks $M flat!", FALSE, ch, 0, vict,
				    TO_NOTVICT);
				act("$n charges you and knocks you flat!", FALSE, ch, 0, vict,
				    TO_VICT);
				act("You charge $N and knock $M flat!", FALSE, ch, 0, vict,
				    TO_CHAR);
				CharWait(vict, PULSE_VIOLENCE * 3);
				if (!damage(ch, vict, str_app[STAT_INDEX(GET_C_STR(ch))].todam + 5,
					    SKILL_BASH))
				{
					SET_POS(vict, POS_PRONE + GET_STAT(vict));
					if (!number(0, 2))
						Stun(vict, ch,
						     number(PULSE_VIOLENCE, PULSE_VIOLENCE * 5 / 2),
						     TRUE);
				}
				else
				{
					return TRUE;
				}
			}
			else
			{
				act("$n charges $N!", FALSE, ch, 0, vict, TO_NOTVICT);
				act("$n charges you!", FALSE, ch, 0, vict, TO_VICT);
				act("You charge $N!", FALSE, ch, 0, vict, TO_CHAR);
			}
			act("$n then starts a deadly little dance on $N's prone form!", FALSE, ch,
			    0, vict, TO_NOTVICT);
			act("$n then starts a deadly little dance on your tender body!", FALSE, ch,
			    0, vict, TO_VICT);
			act("Then you start a deadly little dance on $N's prone form!", FALSE, ch,
			    0, vict, TO_CHAR);

			/*
				 * up to 3 'normal' attacks
				 */
			for (i = 1; i < 4; i++)
			{
				if (!IS_AWAKE(vict) || !StatSave(vict, APPLY_AGI, -2))
				{
					if (damage(ch, vict,
						   (dice(ch->points.damnodice,
							 ch->points.damsizedice) +
						    TRUE_DAMROLL(ch)),
						   TYPE_UNDEFINED))
					{
						break;
					}
				}
			}
			return TRUE;
			break;
		}
		else
		{
			if (rider || number(0, 4) || StatSave(ch, APPLY_AGI, -2))
			{
				act("$n charges toward $N, who quickly dodges aside!", FALSE, ch, 0,
				    vict, TO_NOTVICT);
				act("$n charges you, but you manage to scramble out of the way!",
				    FALSE, ch, 0, vict, TO_VICT);
				act("You charge $N, but $E dodges away!", FALSE, ch, 0, vict,
				    TO_CHAR);
				return TRUE;
			}
			/*
				 * Muhahaha, you thought you escaped!  Drops through to mulekick
				 * section!
				 */
			act("As $N dodges $n's charge, $n whirls, and both of $s rear feet connect with crushing force!",
			    FALSE, ch, 0, vict, TO_NOTVICT);
			act("You dodge a charge from $n, only to be slammed with both of $s rear feet!",
			    FALSE, ch, 0, vict, TO_VICT);
			act("$N dodges your charge, you whirl around and connect with both rear feet!",
			    FALSE, ch, 0, vict, TO_CHAR);
			dropped_through = TRUE;
			[[fallthrough]];
		}

	case 2: /*
		         * mulekick
		         */
		if (!dropped_through)
		{
			if (rider && number(0, 4))
				return FALSE;
			if (StatSave(vict, APPLY_AGI, -2))
			{
				act("$n's rear feet whistle over $N's head, missing by scant inches!",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("$n's rear feet whistle past your scalp, as you duck frantically!",
				    FALSE, ch, 0, vict, TO_VICT);
				act("Your feet whistle over $N's head, missing by scant inches!",
				    FALSE, ch, 0, vict, TO_CHAR);
				CharWait(ch, PULSE_VIOLENCE);
				return TRUE;
			}
			else
			{
				act("$n lashes out with both rear feet, they connect with $N, generating meaty THUDS!",
				    FALSE, ch, 0, vict, TO_NOTVICT);
				act("$n kicks you with both rear feet, sending you back gasping in pain!",
				    FALSE, ch, 0, vict, TO_VICT);
				act("Your kicks connect with $N, generating meaty THUDS!", FALSE,
				    ch, 0, vict, TO_CHAR);
			}
		}
		/*
			 * ok, do the deed, VERY high chance of stunning victim in this case, in
			 * addition to nasty damage!
			 */
		if (damage(ch, vict, GET_LEVEL(ch), TYPE_UNDEFINED))
			return TRUE;
		if (!number(0, 10))
			Stun(vict, ch, PULSE_VIOLENCE * 2, FALSE);
		update_pos(vict);
		if (IS_AWAKE(vict))
			CharWait(vict, 2 * PULSE_VIOLENCE);
		CharWait(ch, PULSE_VIOLENCE);

		break;
	default: /*
		          * do nothing special
		          */
		return FALSE;
		break;
	}
	return FALSE;
}

int water_elemental(P_char ch, P_char pl, int cmd, char * /*arg*/)
{
	P_char vict = NULL, next_ch;
	bool found = FALSE;

	/*
	 * check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return TRUE;

	if (cmd || pl || !CAN_ACT(ch))
		return FALSE;

	/*
	 * ok, basically, this is a area bash attack, but not quite
	 */

	/*  LOOP_THRU_PEOPLE(vict, ch) {*/
	for (vict = world[ch->in_room].people; vict; vict = next_ch)
	{
		next_ch = vict->next_in_room;

		if (!should_area_hit(ch, vict))
			continue;

		if (!found)
		{
			found = TRUE;
			act("$n shimmers and the waters of the pool lift in a towering wall!", TRUE,
			    ch, 0, 0, TO_ROOM);
		}
		CharWait(vict, PULSE_VIOLENCE * 2);
		SET_POS(vict, POS_PRONE + GET_STAT(vict));
		if (GET_RACE(vict) == RACE_F_ELEMENTAL)
		{
			act("$N vanishes in a cloud of steam!", FALSE, ch, 0, vict, TO_NOTVICT);
			act("A wall of water crashes on top of you, you feel your lifefires being quenched!",
			    FALSE, ch, 0, vict, TO_VICT);
			damage(ch, vict, dice(10, 6), TYPE_UNDEFINED);
		}
		else
		{
			act("A wave crashes over you, pounding you into the ground!", FALSE, ch, 0,
			    vict, TO_VICT);
			damage(ch, vict, dice(1, 5), TYPE_UNDEFINED);
		}
		/*    update_pos(vict);*/
	}

	if (found)
		return TRUE;

	return FALSE;
}

/* Negative Material */
