#include "core/prototypes.h"
#include "combat/defense_resolution.h"
#include "core/structs.h"
#include "world/db.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "world/graph.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include <string.h>
#include <strings.h>

extern Skill skills[];
extern P_char character_list;

void spell_blindness(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		     P_obj /*obj*/)
{
	if (GET_STAT(ch) == STAT_DEAD)
		return;
	/*
	  if(affected_by_spell(victim, SPELL_BLINDNESS))
	    return;
	*/

	if (IS_TRUSTED(victim))
		return;

	if (resists_spell(ch, victim))
		return;

	blind(ch, victim, number(4, 12) * WAIT_SEC);

	if (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT))
		appear(ch);

	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}

/** TAM 1/94 -- paralyze a PC/MOB so they can't perform actions which     **/
/**             require actual physical movement AND not respond to       **/
/**             attacker until wears off **/

void spell_major_paralysis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char victim, P_obj obj)
{
	struct affected_type af;
	int lev = level;

	if (!((victim || obj) && ch))
	{
		return;
	}
	if (GET_STAT(ch) == STAT_DEAD)
		return;

	appear(ch);

	if (!IS_TRUSTED(ch) &&
	    (resists_spell(ch, victim) ||
	     (IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA))))
		return;

	if (check_freedom_of_movement(victim, number(0, 1)) && !IS_TRUSTED(ch))
	{
		send_to_char("&+CTheir movement magic prevented your spell!&n\r\n", ch);
		return;
	}

	/*
	 * calling with negative level negates save
	 */
	/*
	 * indeed it does, but magic resistance counts even to that.
	 */

	if (IS_TRUSTED(ch) || (lev < 0) || !NewSaves(victim, SAVING_PARA, -4))
	{
		if (lev < 0)
			lev = -lev;
		bzero(&af, sizeof(af));
		af.type = SPELL_MAJOR_PARALYSIS;
		af.flags = AFFTYPE_SHORT;
		af.duration = 50 * WAIT_SEC / 2;
		af.bitvector2 = AFF2_MAJOR_PARALYSIS;

		affect_to_char(victim, &af);

		act("$n &+Mceases to move.. still and lifeless.", FALSE, victim, 0, 0, TO_ROOM);
		send_to_char("&+LYour body becomes like stone as the paralyzation takes effect.\n",
			     victim);
		if (IS_FIGHTING(victim))
			stop_fighting(victim);

		/*
		 * stop all non-vicious/agg attackers
		 */
		StopMercifulAttackers(victim);

		remember(victim, ch);
	}
	else if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}

/** TAM 1/94 -- paralyze a PC/MOB so they can't perform actions which require **/
/**             actual physical movement BUT wear off when attacked.          **/

void spell_minor_paralysis(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			   P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (resists_spell(ch, victim) ||
	    (IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)))
		return;

	if (check_freedom_of_movement(victim, false) && !IS_TRUSTED(ch))
	{
		send_to_char("&+CTheir movement magic prevented your spell!&n\r\n", ch);
		return;
	}

	if (!NewSaves(victim, SAVING_PARA, 0))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_MINOR_PARALYSIS;
		af.flags = AFFTYPE_SHORT;
		af.duration = (int)(level * WAIT_SEC * 0.75);
		af.bitvector2 = AFF2_MINOR_PARALYSIS;

		affect_to_char(victim, &af);

		act("$n &+Wturns pale as some magical force occupies $s body, causing all motion to halt.",
		    FALSE, victim, 0, 0, TO_ROOM);
		send_to_char("&+LYour body becomes like stone as the paralyzation takes effect.\n",
			     victim);
		if (IS_FIGHTING(victim))
			stop_fighting(victim);

		/*
		 * stop all non-vicious/agg attackers
		 */
		StopMercifulAttackers(victim);
	}
} /*
   * spell_paralyze
   */

/** TAM 1/94 -- slow a PC/MOB down  so that commands are processed at a reduced rate **/

void spell_slow(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		P_obj /*obj*/)
{
	struct affected_type af;

	if (GET_STAT(ch) == STAT_DEAD)
		return;

	appear(ch);

	if (IS_AFFECTED2(victim, AFF2_SLOW))
	{
		act("Just between you and me, $N looks pretty slow already.", FALSE, ch, 0, victim,
		    TO_CHAR);
		return;
	}

	if (GET_CLASS(victim, CLASS_MONK))
	{
		act("$N's intense concentration means that $E cannot be slowed!", TRUE, ch, 0,
		    victim, TO_CHAR);
		return;
	}

	if (resists_spell(ch, victim))
		return;

	if (!saves_spell(victim, SAVING_PARA))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_SLOW;
		af.duration = (level >> 4) + 1;
		af.modifier = 2;
		af.bitvector2 = AFF2_SLOW;

		affect_to_char(victim, &af);

		act("&+m$n begins to sllooowwww down.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("&+mYou feel yourself slowing down.\n", victim);
	}

	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
} /*
   * spell_slow
   */

void spell_sleep(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj /*obj*/)
{
	struct affected_type af;
	int i;

	if (GET_STAT(ch) == STAT_DEAD)
	{
		send_to_char("They are already... quite... asleep... for good.&n\n", ch);
		return;
	}

	if (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT))
		appear(ch);

	if (resists_spell(ch, victim))
	{
		send_to_char("Your victim resists your attempt to make them sleep.&n\n", ch);
		return;
	}

	if (level > 0)
		for (i = 0; i < MAX_WEAR; i++)
		{
			if (victim->equipment[i] &&
			    IS_SET(victim->equipment[i]->extra_flags, ITEM_NOSLEEP))
			{
				send_to_char(
					"&+CYour target appears to be protected against sleeping!\n",
					ch);
				if (IS_PC(victim))
					send_to_char("You stifle a yawn.\n", victim);
				if (IS_NPC(victim) && CAN_SEE(victim, ch))
				{
					remember(victim, ch);
					if (!IS_FIGHTING(victim))
						MobStartFight(victim, ch);
					send_to_char(
						"Your victim does not want to sleep right now!&n\n",
						ch);
				}
				return;
			}
		}
	if ((level < 0) || (!saves_spell(victim, SAVING_SPELL) && (GET_LEVEL(victim) < 56) &&
			    !IS_DEMON(victim) && !IS_UNDEADRACE(victim) && !IS_ELEMENTAL(victim)))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_SLEEP;
		af.duration = 4 + (level < 0 ? -level : level);
		af.duration /= 10;
		if (af.duration > 1)
			af.duration--;
		else
			af.duration = 1;

		af.bitvector = AFF_SLEEP;

		act("&+LYou feel very sleepy ..... zzzzzz", FALSE, victim, 0, 0, TO_CHAR);
		if (GET_OPPONENT(victim))
			stop_fighting(victim);
		if (GET_STAT(victim) > STAT_SLEEPING)
		{
			act("&+W$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
			SET_POS(victim, GET_POS(victim) + STAT_SLEEPING);
		}
		affect_join(victim, &af, FALSE, FALSE);
		/*
		 * stop all non-vicious/agg attackers
		 */
		StopMercifulAttackers(victim);
		return;
	}
	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
		send_to_char("Your victim does not want to sleep right now!&n\n", ch);
	}
}

void spell_fear(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		P_obj /*obj*/)
{
	if (GET_STAT(ch) == STAT_DEAD)
		return;

	if (affected_by_spell(victim, SKILL_BERSERK) || resists_spell(ch, victim))
		return;

	/*
	 * Added by DTS 7/18/95
	 */
	if (IS_DEMON(victim) || IS_DRAGON(victim) || IS_UNDEADRACE(victim) || IS_TRUSTED(victim))
	{
		act("Your attempt to scare $N only makes $M laugh!", FALSE, ch, 0, victim, TO_CHAR);

		return;
	}
	if (!NewSaves(victim, SAVING_FEAR, (IS_ELITE(ch) ? 20 : 0)) && !fear_check(victim))
	{
		act("You scare the bejesus out of $N!", FALSE, ch, 0, victim, TO_CHAR);
		act("A wave of utter terror overcomes you as $n's power fully hits you!", FALSE, ch,
		    0, victim, TO_VICT);
		act("$n scares the bejesus out of $N!", FALSE, ch, 0, victim, TO_NOTVICT);

		do_flee(victim, 0, 2);
	}
	else
	{
		act("$N doesn't look frightened..  $E DOES look kinda mad though.", FALSE, ch, 0,
		    victim, TO_CHAR);
		act("$N &+wsteels $sself against the fearsome visage and presses $s attack.", TRUE,
		    ch, 0, victim, TO_NOTVICT);
		act("$n &+Lseems to grow in size and power, but you ignore the danger and press your attack.",
		    TRUE, ch, 0, victim, TO_VICT);
	}
	if (ch->in_room == victim->in_room)
	{
		/*
		 * they didn't flee, know what happens when you corner a scared
		 * rat?
		 */
		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
	}
}

void spell_haste(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj /*obj*/)
{
	if (affected_by_spell(victim, SPELL_SLOW))
	{
		affect_from_char(victim, SPELL_SLOW);

		act("$n regains $s former speed.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("&+mYou feel your speed return to normal.\r\n", victim);
		return;
	}

	if (IS_AFFECTED(victim, AFF_HASTE))
	{
		act("$N is already under the influence of &+Yhaste.&n", TRUE, ch, 0, victim,
		    TO_CHAR);
		return;
	}

	if (!affected_by_spell(victim, SPELL_HASTE))
	{
		if (GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR))
		{
			send_to_char(
				"&+RYou feel the speed of the &+Cwind&+R rushing through your heart!\n",
				victim);
			act("$n &+Rstarts to move with the speed of the &+Cwind&+R!", TRUE, victim,
			    0, 0, TO_ROOM);
		}
		else
		{
			send_to_char("&+RYou feel your heart start to race REAL FAST!\n", victim);
			act("$n &+Rstarts to move with uncanny speed!", TRUE, victim, 0, 0,
			    TO_ROOM);
		}

		struct affected_type af;
		bzero(&af, sizeof(af));
		af.type = SPELL_HASTE;
		af.bitvector = AFF_HASTE;

		if (GET_SPEC(ch, CLASS_CONJURER, SPEC_AIR))
			af.duration = (int)(level / 2);
		else if (GET_CLASS(ch, CLASS_RANGER) || GET_CLASS(ch, CLASS_REAVER) ||
			 GET_CLASS(ch, CLASS_CONJURER) || GET_CLASS(ch, CLASS_SORCERER))
			af.duration = 15;
		else
			af.duration = 10;

		affect_to_char(victim, &af);
	}
}

/* if it looks alot like gstone...that's because i mercilessly ripped it off of that! */
void spell_group_haste(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_haste((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_haste((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		}
	}
}

void spell_virtue(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		  P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_VIRTUE))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_VIRTUE;
		af.modifier = level;
		af.duration = level / 2;
		affect_to_char(victim, &af);
		send_to_char("&+WYou feel overwhelmed to honor the gods!\n", victim);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_VIRTUE)
			{
				af1->duration = level / 2;
			}
		}
	}
}

void spell_alter_energy_polarity(int /*level*/, P_char /*ch*/, char * /*arg*/,
				 [[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_ALTER_ENERGY_POLARITY) &&
	    !NewSaves(victim, SAVING_PARA, 0))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_ALTER_ENERGY_POLARITY;
		af.duration = 7;
		af.bitvector4 = AFF4_REV_POLARITY;
		affect_to_char(victim, &af);

		act("&+L$n&+L fades from view for a split second, then reappears.", FALSE, victim,
		    0, 0, TO_ROOM);
		act("&+LYour vision goes black for a brief moment..... hrmmm.", FALSE, victim, 0, 0,
		    TO_CHAR);
	}
}

void spell_lifelust(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_LIFELUST))
	{
		bzero(&af, sizeof(af));
		send_to_char("&+LYou begin to drool... &+rmust kill the living!\n", victim);
		af.type = SPELL_LIFELUST;
		af.duration = MAX(5, level / 2);
		af.modifier = ((int)(level / 25)) + 2;
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.location = APPLY_DAMROLL;
		af.modifier = ((int)(level / 25)) + 2;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_LIFELUST)
			{
				af1->duration = MAX(5, level / 2);
			}
	}
}

void spell_hellfire(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		    P_char /*victim*/, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED4(ch, AFF4_HELLFIRE))
	{
		act("You are already burning with hatred!", FALSE, ch, 0, ch, TO_CHAR);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_HELLFIRE;
	af.location = APPLY_NONE;
	af.duration = 15;
	af.bitvector4 = AFF4_HELLFIRE;
	affect_to_char(ch, &af);

	act("&+R$n&+R bursts into a flaming mass!!", FALSE, ch, 0, 0, TO_ROOM);
	act("&+RYou burst into a flaming mass of hate!!", FALSE, ch, 0, 0, TO_CHAR);
}

void spell_battletide(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (IS_AFFECTED4(victim, AFF4_BATTLETIDE))
	{
		act("Alas, the taste of battle is already with you.", FALSE, ch, 0, victim,
		    TO_CHAR);
		return;
	}

	bzero(&af, sizeof(af));
	af.type = SPELL_BATTLETIDE;
	af.location = APPLY_DAMROLL;
	af.modifier = 10;
	af.duration = 10;
	af.bitvector4 = AFF4_BATTLETIDE;
	affect_to_char(ch, &af);
	af.location = APPLY_HITROLL;
	af.modifier = 10;
	af.duration = 10;
	af.bitvector4 = 0;
	affect_to_char(ch, &af);

	act("&+R$n&+R thrusts $s arms skyward, &+rscreaming&+R forth a call to arms!&n", FALSE, ch,
	    0, 0, TO_ROOM);
	act("&+RYou thrust your arms skyward, rallying your comrades for battle!&n", FALSE, ch, 0,
	    0, TO_CHAR);
}

void spell_disease(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		   P_obj /*obj*/)
{
	struct affected_type af;
	int temp;

	temp = (int)(level / 10);

	if (get_spell_from_room(&world[ch->in_room], SPELL_SUMMON_INSECTS))
	{
		temp += 3;
	}

	if (get_spell_from_room(&world[ch->in_room], SPELL_CONSECRATE_LAND))
	{
		temp -= 3;
	}

	if (NewSaves(victim, SAVING_PARA, temp))
		return;

	if (IS_UNDEADRACE(victim))
		return;

	if (!affected_by_spell(victim, SPELL_DISEASE))
	{
		bzero(&af, sizeof(af));
		send_to_char("&+yYou suddenly don't feel so well!\n", victim);
		act("&+y$n &+ysuddenly does not look so well.", FALSE, victim, 0, 0, TO_ROOM);
		af.type = SPELL_DISEASE;
		af.duration = 3 * (1 + temp);
		af.modifier = -(5 * (1 + temp));
		af.location = APPLY_STR;
		affect_to_char(victim, &af);
		af.location = APPLY_DEX;
		affect_to_char(victim, &af);
		af.location = APPLY_AGI;
		affect_to_char(victim, &af);
		af.location = APPLY_CON;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_DISEASE)
			{
				af1->duration = MAX(5, level / 4);
			}
		send_to_char("&+yYou suddenly feel the disease in your body growing stronger!\n",
			     victim);
		act("&+y$n &+ysuddenly looks even worse than before.", FALSE, victim, 0, 0,
		    TO_ROOM);
	}
}

void spell_freedom_of_movement(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_FREEDOM_OF_MOVEMENT))
	{
		act("$n &+gappears to move unhindered through the world.&n", TRUE, victim, 0, 0,
		    TO_ROOM);
		act("You &+gbegin to move unhindered through the world.&n", TRUE, victim, 0, 0,
		    TO_CHAR);
		bzero(&af, sizeof(af));
		af.type = SPELL_FREEDOM_OF_MOVEMENT;
		af.duration = (GET_LEVEL(ch) / 28) + 1; // 1-3 minutes depends on level of caster
		af.bitvector = AFF_FREEDOM_OF_MVMNT;
		affect_to_char(victim, &af);

		// maybe need an affects_from_char to make this a single call
		affect_from_char(victim, SPELL_MINOR_PARALYSIS);
		affect_from_char(victim, SPELL_MAJOR_PARALYSIS);
		affect_from_char(victim, SPELL_EARTHEN_GRASP);
		affect_from_char(victim, SPELL_ENTANGLE);
	}
}

void spell_levitate(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type,
		    P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_LEVITATE))
	{
		if (!IS_AFFECTED(victim, AFF_LEVITATE))
		{
			act("&+WYou float up in the air!", FALSE, victim, 0, 0, TO_CHAR);
			act("$n &+Wfloats up in the air!", TRUE, victim, 0, 0, TO_ROOM);
		}
		memset(&af, 0, sizeof(af));
		af.type = SPELL_LEVITATE;
		af.duration = MAX(level, 10);
		af.bitvector = AFF_LEVITATE;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_LEVITATE)
			{
				af1->duration = MAX(level, 10);
			}
	}
}

void spell_fly(int level, P_char /*ch*/, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
	       P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_FLY))
	{
		if (!IS_AFFECTED(victim, AFF_FLY))
		{
			act("&+WYou fly through the air, free as a bird!", FALSE, victim, 0, 0,
			    TO_CHAR);
			act("$n &+Wflies through the air, free as a bird!", TRUE, victim, 0, 0,
			    TO_ROOM);
		}
		bzero(&af, sizeof(af));
		af.type = SPELL_FLY;
		af.duration = level * 2;
		af.bitvector = AFF_FLY;
		affect_to_char(victim, &af);
	}
	else
	{
		struct affected_type *af1;

		for (af1 = victim->affected; af1; af1 = af1->next)
			if (af1->type == SPELL_FLY)
			{
				af1->duration = level * 2;
			}
	}
}

void spell_silence(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		   P_char victim, P_obj /*obj*/)
{
	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	int save = victim->specials.apply_saving_throw[SAVING_SPELL];

	if (GET_LEVEL(victim) > 57)
	{
		if (ch != victim)
		{
			save = -15;
		}
		// For Imms testing..
		else
		{
			save += 15;
		}
	}

	int percent = BOUNDED(0,
			      (int)((GET_C_WIS(ch) / 2) + (save * 2) +
				    (GET_LEVEL(ch) - GET_LEVEL(victim)) - GET_C_WIS(victim) / 4),
			      100);

	//  debug("Silence percent is: %d", percent);

	if ((IS_TRUSTED(victim) && victim != ch) || (IS_GREATER_RACE(victim)) ||
	    (IS_ELITE(victim)) || (percent < 10))
	{
		return;
	}

	if (IS_AFFECTED2(victim, AFF2_SILENCED))
	{
		send_to_char("They are already quiet!\n", ch);
		return;
	}

	if (resists_spell(ch, victim))
		return;

	struct affected_type af;
	bzero(&af, sizeof(af));

	if (percent > 90)
	{
		send_to_char("You suddenly feel completely quiet!\n", victim);
		act("$n is suddenly at a great loss for words.", TRUE, victim, 0, 0, TO_ROOM);

		af.type = SPELL_SILENCE;
		af.duration = 10 * WAIT_SEC;
		af.flags = AFFTYPE_SHORT;
		af.bitvector2 = AFF2_SILENCED;
		affect_to_char(victim, &af);
	}
	else if (percent > 70)
	{
		send_to_char("You suddenly feel much quieter!\n", victim);
		act("$n suddenly grows silent.", TRUE, victim, 0, 0, TO_ROOM);

		af.type = SPELL_SILENCE;
		af.duration = 8 * WAIT_SEC;
		af.flags = AFFTYPE_SHORT;
		af.bitvector2 = AFF2_SILENCED;
		affect_to_char(victim, &af);
	}
	else if (percent > 40)
	{
		send_to_char("You suddenly feel quiet!\n", victim);
		act("$n is suddenly at a loss for words.", TRUE, victim, 0, 0, TO_ROOM);

		af.type = SPELL_SILENCE;
		af.flags = AFFTYPE_SHORT;
		af.duration = 5 * WAIT_SEC;
		af.bitvector2 = AFF2_SILENCED;
		affect_to_char(victim, &af);
	}
	else if (percent > 5)
	{
		send_to_char("You feel your voice soften slightly!\n", victim);
		act("$n is suddenly at a slight loss for words.", TRUE, victim, 0, 0, TO_ROOM);

		af.type = SPELL_SILENCE;
		af.flags = AFFTYPE_SHORT;
		af.duration = 3 * WAIT_SEC;
		af.bitvector2 = AFF2_SILENCED;
		affect_to_char(victim, &af);
	}
}
void spell_feeblemind(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	char Gbuffer_1[24];
	int i = 0, j = 0, k = 0, percentch, percentvictim, save = 0, percent, intmod, wismod;

	if (ch) // Just making sure.
	{
		if (!IS_ALIVE(ch))
		{
			send_to_char("Take a look around. You're dead...\r\n", ch);
			return;
		}
		if (!victim || !IS_ALIVE(victim))
		{
			act("Who/what do you wish to feeb?", TRUE, ch, 0, victim, TO_CHAR);
			return;
		}

		if (IS_NPC(ch))
			return; // drannak - disabling while i debug

		if (affected_by_spell(victim, SPELL_FEEBLEMIND))
		{
			act("$N is already dumb!", TRUE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (IS_GREATER_RACE(victim) || IS_ELITE(victim) || IS_TRUSTED(victim))
		{
			act("$N is immune to your pathetic spell.", TRUE, ch, 0, victim, TO_CHAR);
			return;
		}
		if ((GET_RACE(victim) == RACE_UNDEAD) || (GET_RACE(victim) == RACE_PLANT))
		{
			act("$N's mind is too foreign for you to alter!", TRUE, ch, 0, victim,
			    TO_CHAR);
			return;
		}
		if (resists_spell(ch, victim))
		{
			return;
		}

		save = victim->specials.apply_saving_throw[SAVING_SPELL];

		if (save < 0)
		{
			save = (int)(save * 1.5);
		}
		if (IS_PC(ch) || (IS_NPC(ch) && GET_C_WIS(ch) >= 75))
		{
			percentch = (int)((GET_C_WIS(ch) + GET_LEVEL(ch)) *
					  get_property("spell.richness.feeblemind", 1.5));
		}
		else
		{
			percentch = (int)((75 + GET_LEVEL(ch)) *
					  get_property("spell.richness.feeblemind", 1.5));
		}

		percentvictim = GET_C_WIS(victim) + GET_LEVEL(victim) - save;
		percent = percentch + number(-15, 15) - percentvictim;

		if (percent < 30)
		{
			act("Your spell has no effect!", TRUE, ch, 0, victim, TO_CHAR);
			return;
		}

		send_to_char("You feel &+creally, &+Creally &+cdumb!&n\n", victim);
		act("$n looks &+c_really_ dumb.&n", TRUE, victim, 0, 0, TO_ROOM);

		// At 100 int and level 50 caster, int drop approx 43.
		intmod = (int)((GET_C_INT(victim) / 3) + (level / 5) + number(-15, 5));
		wismod = (int)((GET_C_WIS(victim) / 3) + (level / 5) + number(-15, 5));

		struct affected_type af;
		bzero(&af, sizeof(af));
		af.type = SPELL_FEEBLEMIND;
		if (percent < 70)
		{
			af.flags = AFFTYPE_SHORT;
			af.duration = 80;
		}
		else
		{
			af.duration = 1;
		}
		af.modifier = (-1 * intmod);
		af.location = APPLY_INT;
		affect_to_char(victim, &af);

		af.modifier = (-1 * wismod);
		af.location = APPLY_WIS;
		affect_to_char(victim, &af);

		// for i greater than 10, returns 1, 2, 3, ...
		// tweak divisor value higher for fewer spell losses.
		i = (int)(percent /
			  get_property("spell.richness.feeblemind.spellLoss.divisor", 10));

		if (ch && victim && IS_ALIVE(ch) && IS_ALIVE(victim) && (i >= 1))
		{
			stop_memorizing(victim); // Need to stop otherwise possible crash.

			// Is it a slot caster?
			if (USES_SPELL_SLOTS(victim))
			{
				if (!victim)
				{
					return;
				}
				for (BOUNDED(1, (int)(j = get_max_circle(victim)), 12); j >= 1; j--)
				{
					for (k = victim->specials.undead_spell_slots[j]; k >= 1;
					     k--)
					{
						if (i >= 1)
						{
							victim->specials.undead_spell_slots[j]--;
							i--;
							continue;
						}
						else
						{
							break;
						}
					}
				}
			}
			else // We guessing it's a regular caster.
			{
				if (!victim || !IS_PC(victim))
				{
					return;
				}
				struct affected_type *af2, *next_af2;
				for (af2 = victim->affected; af2; af2 = next_af2)
				{
					if (!af2)
						break;

					next_af2 = af2->next;
					if (af2->type == TAG_MEMORIZE)
					{
						if (i >= 1)
						{
							affect_remove(victim, af2);
							i--;
							snprintf(Gbuffer_1, sizeof Gbuffer_1,
								 "You forget %s!\n",
								 skills[af2->modifier].name);
							send_to_char(Gbuffer_1, victim);
							continue;
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
}
void spell_pword_blind(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (IS_BLIND(victim))
	{
		send_to_char("Your target is already blind!\r\n", ch);
		return;
	}

	if (has_innate(victim, INNATE_EYELESS) || IS_TRUSTED(victim))
		return;

	act("$N gropes around, blinded after hearing $n's powerful word!", FALSE, ch, 0, victim,
	    TO_NOTVICT);
	act("&+rSuddenly, the world goes &+Lblack!", FALSE, ch, 0, victim, TO_VICT);
	act("$N won't be seeing much in the near future...", TRUE, ch, 0, victim, TO_CHAR);
	blind(ch, victim, MAX(4, level - GET_LEVEL(victim)) * WAIT_SEC);
}
/* rocking spell now, stun is extremely unpleasant */

void spell_pword_stun(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		      P_obj /*obj*/)
{
	// int percent = 50;
	int percent;

	if (!IS_ALIVE(victim) || GET_HIT(victim) < 1 || !IS_ALIVE(ch) || IS_TRUSTED(victim) ||
	    IS_ZOMBIE(victim) || GET_RACE(victim) == RACE_GOLEM || GET_RACE(victim) == RACE_PLANT ||
	    GET_RACE(victim) == RACE_CONSTRUCT || IS_GREATER_RACE(victim))
	{
		send_to_char("&+rYour target is immune!\r\n", ch);
		return;
	}

	if (resists_spell(ch, victim))
		return;

	if (IS_STUNNED(victim))
	{
		send_to_char("Your target is already stunned!\r\n", ch);
		return;
	}

	percent = (int)(GET_LEVEL(ch) - GET_LEVEL(victim));

	percent += (int)(GET_C_POW(ch) - GET_C_POW(victim)) * .75;

	if (IS_PC_PET(victim))
		percent *= 2;

	if (affected_by_spell(ch, SPELL_FEEBLEMIND))
		percent /= 3;

	if (affected_by_spell(victim, SPELL_FEEBLEMIND))
		percent *= 1.5;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_PWORD_STUN);
	if (NewSaves(victim, SAVING_SPELL, mod))
	{
		percent = (int)percent * .75;
	}
	else
	{
		percent = (int)percent * 1.15;
	}

	if (percent < 30)
	{
		send_to_char("&+rYour spell has no effect!\n", ch);
		return;
	}

	if (percent > 90)
	{
		act("$N&n &+yis &+rstunned &+yinto complete submission by the power of&n $n's &+yword!&n",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		act("$n's&n &+yword of power sends you reeling in utter confusion and pain!&n",
		    FALSE, ch, 0, victim, TO_VICT);
		act("$N&n &+yis &+rstunned &+yinto complete submission by your powerful word!&n",
		    TRUE, ch, 0, victim, TO_CHAR);
		Stun(victim, ch, (number(3, 4) * PULSE_VIOLENCE), FALSE);
	}
	else if (percent > 70)
	{
		act("$N&n &+yis heavily &+rstunned &+yby the power of&n $n's &+yyword!&n", TRUE, ch,
		    0, victim, TO_NOTVICT);
		act("$n's&n &+yword of power sends you reeling!&n", FALSE, ch, 0, victim, TO_VICT);
		act("$N&n &+yis sent reeling by your very powerful word!&n", TRUE, ch, 0, victim,
		    TO_CHAR);
		Stun(victim, ch, (number(2, 3) * PULSE_VIOLENCE), FALSE);
	}
	else if (percent > 50)
	{
		act("$N&n &+yis &+rstunned &+yby the power of&n $n's &+yword!&n", TRUE, ch, 0,
		    victim, TO_NOTVICT);
		act("$n's&n &+yword of power confuses and disorients you!&n", FALSE, ch, 0, victim,
		    TO_VICT);
		act("$N&n &+yis sent reeling by your powerful word!&n", TRUE, ch, 0, victim,
		    TO_CHAR);
		Stun(victim, ch, (number(1, 2) * PULSE_VIOLENCE), FALSE);
	}
	else
	{
		act("$N&n &+yis &+wdazed &+yby the power of&n $n's &+yword!&n", TRUE, ch, 0, victim,
		    TO_NOTVICT);
		act("$n's&n &+yword of power &+wdazes &+yyou!&n", FALSE, ch, 0, victim, TO_VICT);
		act("$N&n &+yis &+wdazed &+yby your powerful word!&n", TRUE, ch, 0, victim,
		    TO_CHAR);
		Stun(victim, ch, PULSE_VIOLENCE, FALSE);
	}
}

void spell_command_undead(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			  P_obj /*obj*/)
{
	if (!victim)
	{
		return;
	}
	if (training_dummy_is(victim))
	{
		send_to_char("The training dummy cannot be commanded.\r\n", ch);
		return;
	}
	if (/*(GET_CLASS(ch) != CLASS_NECROMANCER) || */ resists_spell(ch, victim))
		return;

	if (IS_GOOD(ch))
	{
		send_to_char("You don't even _consider_ such an evil act, meddling with undead!",
			     ch);
		return;
	}
	if (!IS_UNDEADRACE(victim))
	{
		act("$n just tried to command you, what a moron!", TRUE, ch, 0, victim, TO_VICT);
		act("$N thinks $n is really strange.", TRUE, ch, 0, victim, TO_NOTVICT);
		act("Um... $N isn't undead...", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	/* okay, we can charm them IF they are a 1201 undead and not charmed, OR
	   they fail a saving throw versus spell */

	if (victim->following && (victim->following == ch))
		return; /* they're already following us */

	if ((IS_NPC(victim) && (GET_RNUM(victim) == real_mobile(1201)) && !GET_MASTER(victim)) &&
	    !NewSaves(victim, SAVING_PARA, -4) && ((GET_LEVEL(ch) - 10) > GET_LEVEL(victim)))
	{ /* para save is deathmagic
  save right? */
		/* got em */
		if (victim->following)
			stop_follower(victim);
		add_follower(victim, ch);
		setup_pet(victim, ch, level / 4, 0);
		if (IS_FIGHTING(victim))
			stop_fighting(victim);
		StopMercifulAttackers(victim);
	}
	else if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}
void spell_command_horde(int level, P_char ch, char *arg, int type, P_char /*victim*/, P_obj obj)
{
	P_char tar, tar2;

	if (IS_GOOD(ch))
	{
		send_to_char("You don't even _consider_ such an evil act as meddling with undead!",
			     ch);
		return;
	}
	for (tar = world[ch->in_room].people; tar; tar = tar2)
	{
		tar2 = tar->next_in_room;
		if (!IS_UNDEADRACE(tar))
			continue;
		spell_command_undead(level, ch, arg, type, tar, obj);
	}
}
void spell_turn_undead(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*tch*/,
		       P_obj /*obj*/)
{
	int diff;
	P_char victim, next;

	act("$n raises $s holy symbol and shouts 'Begone!'", TRUE, ch, 0, 0, TO_ROOM);
	act("You raise your holy symbol and shout 'Begone!'", TRUE, ch, 0, 0, TO_CHAR);
	for (victim = world[ch->in_room].people; victim; victim = next)
	{
		next = victim->next_in_room;

		if ((IS_UNDEADRACE(victim) || IS_ANGEL(victim)) && should_area_hit(ch, victim))
		{
			diff = level - GET_LEVEL(victim);
			if (diff <= 0 && !IS_PC_PET(victim))
			{
				act("You are powerless to affect $N!", TRUE, ch, 0, victim,
				    TO_CHAR);
				return;
			}
			else if (diff <= 30)
			{
				if (!NewSaves(victim, SAVING_FEAR, 3 + diff / 4) &&
				    !fear_check(victim))
				{
					act("$n forces $N from this room.", TRUE, ch, 0, victim,
					    TO_NOTVICT);
					act("You force $N from this room.", TRUE, ch, 0, victim,
					    TO_CHAR);
					act("$n forces you from this room.", TRUE, ch, 0, victim,
					    TO_VICT);
					do_flee(victim, 0, 2);
				}
				else
				{
					act("You laugh at $n.", TRUE, ch, 0, victim, TO_VICT);
					act("$N laughs at $n.", TRUE, ch, 0, victim, TO_NOTVICT);
					act("$N laughs at you.", TRUE, ch, 0, victim, TO_CHAR);
				}
			}
			else if (diff > number(0, 100))
			{
				act("As the magic that once brought $n to life has been dispelled, "
				    "$e falls apart.",
				    FALSE, victim, 0, 0, TO_ROOM);
				act("As a powerful force emanating from the symbol drains the magic "
				    "animating you, your body starts falling apart!",
				    FALSE, victim, 0, 0, TO_CHAR);
				die(victim, ch);
			}
			else
			{
				do_flee(victim, 0, 2);
			}
		}
	}
}

void spell_nether_touch(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!victim)
		victim = ch;

	if ((GET_RACE(victim) == RACE_DRAGON) || (GET_RACE(victim) == RACE_DEVIL) ||
	    (GET_RACE(victim) == RACE_UNDEAD) || (GET_RACE(victim) == RACE_GHOST) ||
	    (GET_RACE(victim) == RACE_DEMON) || (GET_RACE(victim) == RACE_PLANT))
	{
		send_to_char("You sense that they would be unaffected by your touch.\n", ch);
		return;
	}

	if (saves_spell(victim, SAVING_SPELL) || affected_by_spell(victim, SPELL_NETHER_TOUCH))
		return;

	bzero(&af, sizeof(af));

	af.type = SPELL_NETHER_TOUCH;
	af.duration = 100;
	af.flags = AFFTYPE_SHORT;
	af.modifier = -3;
	af.location = APPLY_HITROLL;
	affect_to_char(victim, &af);
	af.modifier = 10;
	af.location = APPLY_CURSE;
	affect_to_char(victim, &af);
	af.modifier = 2;
	af.location = APPLY_SAVING_PARA;
	affect_to_char(victim, &af);

	act("&+r$n briefly reveals a &+Lblack&n&+r aura!", FALSE, victim, 0, 0, TO_ROOM);
	act("&+rYou get a strong sinking feeling.", FALSE, victim, 0, 0, TO_CHAR);
}

void spell_dazzle(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		  P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (IS_AFFECTED4(victim, AFF4_DAZZLER))
		return;
	else
	{
		send_to_char(
			"&+cYou feel a surge of energy, as &+wsparks &+carc between your fingers.&N\n",
			ch);
		act("$n begins to &+Wsparkle&N!", TRUE, victim, 0, 0, TO_ROOM);
		bzero(&af, sizeof(af));
		af.type = SPELL_DAZZLE;
		af.duration = (int)(level / 2);
		af.bitvector4 = AFF4_DAZZLER;
		affect_to_char(victim, &af);
	}
	return;
}

void spell_command(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		   P_obj /*obj*/)
{
	if (!NewSaves(victim, SAVING_SPELL, -((GET_LEVEL(victim) / 10) + 3)))
	{
		act("With but a single word, you stun $N into submission.", FALSE, ch, 0, victim,
		    TO_CHAR);
		act("With a single indecipherable word, $n stuns you into submission!", FALSE, ch,
		    0, victim, TO_VICT);
		act("With a single indecipherable word, $n stuns $N into submission!", FALSE, ch, 0,
		    victim, TO_NOTVICT);

		Stun(victim, ch, (GET_LEVEL(ch) > 50) ? PULSE_VIOLENCE * 2 : PULSE_VIOLENCE, FALSE);

		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
	}
	else
	{
		act("With but a single word, you... don't seem to do much of anything.", FALSE, ch,
		    0, victim, TO_CHAR);
		act("$n points at you while screaming.  You don't feel any different.", FALSE, ch,
		    0, victim, TO_VICT);
		act("With a single indecipherable word, $n does nothing in particular to $N.",
		    FALSE, ch, 0, victim, TO_NOTVICT);

		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
	}
}

void spell_stornogs_lowered_magical_res(int level, P_char ch, char * /*arg*/,
					[[maybe_unused]] int type, P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (GET_STAT(ch) == STAT_DEAD)
		return;

	if (resists_spell(ch, victim))
		return;

	//  if(!NewSaves(victim, SAVING_SPELL, 0)) {
	if (!saves_spell(victim, SAVING_PARA))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_STORNOGS_LOWERED_RES;
		af.duration = level / 10;
		af.location = APPLY_SAVING_SPELL;
		af.modifier = (level / 10) + 3;

		affect_to_char(victim, &af);

		act("&+mYou sense that $n&n&+m's magical resistance has been affected.", TRUE,
		    victim, 0, 0, TO_ROOM);
		send_to_char("&+mYou feel more susceptible to magic.\n", victim);
	}

	if (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT))
		appear(ch);

	if (IS_NPC(victim) && CAN_SEE(victim, ch))
	{
		remember(victim, ch);
		if (!IS_FIGHTING(victim))
			MobStartFight(victim, ch);
	}
}

void spell_blackmantle(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		       P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!ch)
	{
		logit(LOG_EXIT, "spell_blackmantle called in magic.c with no ch");
		return;
	}

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (ch == victim)
	{
		send_to_char("Stop wasting time and go kill someone!\r\n", ch);
		return;
	}

	if (affected_by_spell(victim, SPELL_BMANTLE))
	{
		send_to_char("They are already afflicted by blackmantle!!\n", ch);
		return;
	}

	// Made the save level based for PC casters.. maxxes at 14 (lvl 56) instead of hard 3.
	if (!NewSaves(victim, SAVING_SPELL, IS_NPC(ch) ? 16 : level / 4))
	{
		act("&+LA blanketing shroud of &+bnegative energy &+Lcoalesces around $N&+L...",
		    FALSE, ch, 0, victim, TO_CHAR);
		act("&+LA blanketing shroud of &+bnegative energy &+Lcoalesces around $N&+L...",
		    FALSE, ch, 0, victim, TO_NOTVICT);
		act("&+LA cloud of &+bnegative energy &+Ldroplets cloak you, slowly draining the &+wlife &+Lfrom your body...",
		    FALSE, ch, 0, victim, TO_VICT);

		bzero(&af, sizeof(af));
		af.type = SPELL_BMANTLE;
		// Maxxes at 8 ticks at lvl 56.
		af.duration = level / 7;
		// Modifier is how many hps the blackmantle can absorb.
		af.modifier = IS_NPC(ch) ? 4000 : 60 * level;
		affect_to_char(victim, &af);
	}
	else
	{
		act("&+LYour spell fails to afflict $N&+L!", FALSE, ch, 0, victim, TO_CHAR);
	}
}

void spell_prayer(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		  P_obj /*obj*/)
{
	P_char tch;
	P_char tvict;
	const char *god_name;
	struct group_list *group;
	char Gbuf1[MAX_STRING_LENGTH];
	bool prayer;

	if (!IS_ALIVE(ch))
	{
		return;
	}

	god_name = get_god_name(ch);
	snprintf(Gbuf1, MAX_STRING_LENGTH, "$n &+wopenly prays out to %s, asking for intervention!",
		 god_name);
	act(Gbuf1, FALSE, ch, 0, 0, TO_ROOM);
	snprintf(Gbuf1, MAX_STRING_LENGTH, "&+wYou pray to&n %s.", god_name);
	act(Gbuf1, FALSE, ch, 0, 0, TO_CHAR);

	if (!ch->group && !IS_NPC(ch))
	{
		prayer = FALSE;

		for (tvict = character_list; tvict; tvict = tvict->next)
		{
			if (IS_NPC(tvict))
			{
				if (forget(tvict, ch))
				{
					prayer = TRUE;
				}
			}
		}

		if (prayer)
			send_to_char("&+WYour past transgressions will no longer haunt you.&n\n",
				     ch);
		return;
	}

	for (group = ch->group; group; group = group->next)
	{
		tch = group->ch;
		prayer = FALSE;

		if (IS_PC(tch) && tch->in_room == ch->in_room)
		{
			for (tvict = character_list; tvict; tvict = tvict->next)
			{
				if (IS_NPC(tvict))
				{
					if (forget(tvict, tch))
					{
						prayer = TRUE;
					}
				}
			}
		}

		if (prayer)
		{
			send_to_char("&+WYour past transgressions will no longer haunt you.&n\n",
				     tch);
		}
	}
}

void spell_tranquility(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	P_char d = NULL;
	int skl_lvl;
	P_char oponent = NULL;

	skl_lvl = (int)(level * 1.15);

	act("&+g$n calls upon nature, &+ca calming wind passes through&n.\n", FALSE, ch, 0, 0,
	    TO_ROOM);
	act("&+gYou call upon nature, &+ca calming wind passes through&n.\n", FALSE, ch, 0, 0,
	    TO_CHAR);
	for (d = world[ch->in_room].people; d; d = d->next_in_room)
	{
		if (GET_OPPONENT(d))
			if (number(1, 130) < skl_lvl)
			{
				oponent = GET_OPPONENT(d);
				// Lom: made them forget each other in pairs. as StopMercifulAttackers dont clear memories
				if (IS_PC(oponent))
					send_to_char("A sense of calm comes upon you.\n", oponent);
				if (IS_PC(d))
					send_to_char("A sense of calm comes upon you.\n", d);
				if (IS_NPC(d) && IS_PC(oponent))
					forget(d, oponent);
				if (IS_NPC(oponent) && IS_PC(d))
					forget(oponent, d);
				stop_fighting(oponent);
				stop_fighting(d);
				update_pos(oponent);
				update_pos(d);
				//        StopMercifulAttackers(d);
			}
	}
	CharWait(ch, PULSE_VIOLENCE);
}

void spell_entangle(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		    P_obj /*obj*/)
{
	struct affected_type af;
	int skl_lvl, chance, sect;

	chance = 5;
	// 56 / 5 - 1 = 10 => -50 svpara to always save
	//   .. and 0 svpara to save 50% of the time.
	//   .. and 50 svpara to never save.
	skl_lvl = MAX(1, ((level / 5) - 1));

	sect = SECTOR_TYPE(ch->in_room);
	if (!IS_OUTSIDE(ch->in_room) || !HAS_VEGETATION(sect))
	{
		send_to_char("Not too much to entangle yer opponent with here..\n", ch);
		return;
	}

	if (!IS_ALIVE(victim) || IS_TRUSTED(victim))
	{
		return;
	}

	if (affected_by_spell(victim, SPELL_ENTANGLE) || IS_AFFECTED2(victim, AFF2_MINOR_PARALYSIS))
	{
		send_to_char("Nothing happens.\n", ch);
		return;
	}

	if (check_freedom_of_movement(victim, number(0, 1)) && !IS_TRUSTED(ch))
	{
		send_to_char("&+CTheir movement magic prevented your spell!&n\r\n", ch);
		return;
	}

	/*
	if( (!(IS_NPC(victim)) && (world[ch->in_room].sector_type == SECT_FOREST)  && (CASTING_MOD(ch) > 2) )) {
	    if(!(StatSave(victim, APPLY_AGI, (-1 * (-4+CASTING_MOD(ch))) ))) {
	  SET_BIT(victim->specials.affected_by, AFF_BOUND);
	  act("&+GVegetation bursts out of the ground, TOTALLY entangling $N!", TRUE, ch, 0, victim, TO_NOTVICT);
	  act("&+GVegetation bursts out of the ground, TOTALLY entangling you!", TRUE, ch, 0, victim, TO_VICT);
	  act("&+GYou call the vegetation to burst out and completely entangle $N.", TRUE, ch, 0, victim, TO_CHAR);
	  CharWait(ch, 4 * PULSE_VIOLENCE);
	  return; }
	}
	*/

	bzero(&af, sizeof(af));

	if (!NewSaves(victim, SAVING_PARA, skl_lvl) &&
	    !(IS_NPC(victim) && IS_SET(victim->specials.act, ACT_IMMUNE_TO_PARA)))
	{
		send_to_char(
			"&+GVegetation bursts out of the ground, entangling you to the point of paralysis.&N\n",
			victim);
		act("&+GVegetation bursts out of the ground, entangling $n!&N", TRUE, victim, 0, 0,
		    TO_ROOM);

		if (IS_FIGHTING(victim))
		{
			stop_fighting(victim);
		}
		StopMercifulAttackers(victim);

		if (world[ch->in_room].sector_type == SECT_FOREST && number(1, 100) < chance)
		{
			act("&+GThe vegetation closes tightly, completely entangling $n!", TRUE,
			    victim, 0, 0, TO_ROOM);
			send_to_char(
				"&+GThe vegetation closes tightly, completely entangling you!\n",
				victim);
			SET_BIT(victim->specials.affected_by, AFF_BOUND);
		}
		else
		{
			af.type = SPELL_ENTANGLE;
			// 56 / 2 - 5 = 23 sec and 12 / 2 - 5 = 1 sec
			af.duration = WAIT_SEC * (level / 2 - 5);
			af.flags = AFFTYPE_SHORT | AFFTYPE_NOSHOW;
			af.bitvector2 = AFF2_MINOR_PARALYSIS;
			//      af.bitvector2 = AFF2_SLOW;
			af.location = APPLY_NONE;
			af.modifier = 0;

			send_to_char(
				"&+gVegetation &+Gbursts&+g from the ground, impeding your progress.&N\n",
				victim);
			act("&+gVegetation &+Gbursts&+g from the ground, impeding $n.&N", TRUE,
			    victim, 0, 0, TO_ROOM);
			affect_to_char(victim, &af);
		}
	}
}

void spell_contain_being(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!victim)
	{
		send_to_char("You must specify a target for this spell!\r\n", ch);
		return;
	}

	if (victim == ch)
	{
		send_to_char("Haha, very funny.\r\n", ch);
		return;
	}

	if (IS_PC(victim))
	{
		send_to_char("You cannot learn to contain players.\r\n", ch);
		return;
	}

	if (IS_PC_PET(victim))
	{
		send_to_char("You cannot contain other's pets.\r\n", ch);
		return;
	}

	act("&n$n &npoints at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...",
	    FALSE, ch, 0, victim, TO_NOTVICT);
	act("&nYou &npoint at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...",
	    FALSE, ch, 0, victim, TO_CHAR);
	act("&n$n &npoints at &+L$N &nwhose form begins to &+Lp&+Mh&+Wa&+ms&+Be &nin and out from this plane of existence...",
	    FALSE, ch, 0, victim, TO_VICT);

	memset(&af, 0, sizeof(af));
	af.type = SPELL_CONTAIN_BEING;
	af.duration = 100;
	af.flags = AFFTYPE_SHORT;
	affect_to_char_with_messages(victim, &af, "You your physical composure returns to normal.",
				     "$n's &+Yphysical composure slowly returns to &+Lnormal&n.");
}

void spell_cloak_of_fear(int level, P_char ch, char * /*arg*/, int /*type*/, P_char vict,
			 P_obj /*obj*/)
{
	P_char tch;

	if (!ch || !IS_ALIVE(ch))
	{
		return;
	}

	appear(ch);
	act("&+LThe very essence of death flows out from you...&N", 0, ch, 0, 0, TO_CHAR);
	act("&+L$n assumes a terrifying visage of death!", 1, ch, 0, 0, TO_ROOM);

	for (vict = world[ch->in_room].people; vict; vict = tch)
	{
		tch = vict->next_in_room;
		if (vict == ch)
			continue;
		if (IS_GREATER_RACE(vict) || IS_TRUSTED(vict) || IS_ELITE(vict) ||
		    IS_UNDEADRACE(vict) || !IS_ALIVE(vict))
		{
			continue;
		}
		if (should_area_hit(ch, vict) &&
		    !NewSaves(vict, SAVING_FEAR, (int)(level / 11 - 2)) && !fear_check(vict))
		{
			do_flee(vict, 0, 1);
		}
	}
}

void spell_pleasantry(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;

	if (!affected_by_spell(victim, SPELL_PLEASANTRY))
	{
		send_to_char("You create a warm, pleasant feeling in your victim.\n", ch);
		send_to_char("You are suddenly overcome with a warm, pleasant feeling.\n", victim);
		bzero(&af, sizeof(af));
		af.type = SPELL_PLEASANTRY;
		af.duration = 99;
		affect_to_char(victim, &af);
	}
	else
	{
		send_to_char("The bozo has already been made as pleasant as possible.\n", ch);
	}
}

void do_pleasantry(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict;
	P_obj dummy;
	char buf[MAX_STRING_LENGTH];
	struct affected_type af;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
		send_to_char("Usage: pleasant <player>\n", ch);
	else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
		send_to_char("Couldn't find any such creature.\n", ch);
	else if (GET_LEVEL(vict) >= AVATAR)
		act("$E doesn't get any more pleasant than this.", 0, ch, 0, vict, TO_CHAR);
	else if (!affected_by_spell(vict, SPELL_PLEASANTRY))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_PLEASANTRY;
		af.duration = 25;
		affect_to_char(vict, &af);
		act("You make $M tense up as they think about the game.", 0, ch, 0, vict, TO_CHAR);
	}
	else
	{
		act("You let $M relax.", 0, ch, 0, vict, TO_CHAR);
		affect_from_char(vict, SPELL_PLEASANTRY);
	}
}

void pleasantry(P_char ch)
{
	char buf[256];
	int x;

	x = number(1, 24);

	snprintf(buf, 256, "say I'm buggy!"); /* just in case */
	switch (x)
	{
	case 1:
		snprintf(buf, 256, "say Duris is the reason I can't pay child-support!");
		break;
	case 2:
		snprintf(
			buf, 256,
			"say I think I'll donate 50 dollars for all the endless volunteer work that goes into Duris!");
		break;
	case 3:
		snprintf(buf, 256, "say I love this game, I'm teaching my kids how to play!");
		break;
	case 4:
		snprintf(
			buf, 256,
			"say I shouldn't be pressuring Zion, he knows what he's doing and it takes time.");
		break;
	case 5:
		snprintf(
			buf, 256,
			"say I may die a virgin, but at least I'll be the best Duris player ever!");
		break;
	case 6:
		snprintf(buf, 256, "say One of the immortals totally rick-rolled me.");
		break;
	case 7:
		snprintf(buf, 256, "say Isn't this a REALLY cool zone?  Duris has the BEST areas!");
		break;
	case 8:
		snprintf(
			buf, 256,
			"say I heard Torgal spends money on the Duris link. I can't believe I was such a twink as to complain about anything!");
		break;
	case 9:
		snprintf(buf, 256, "dance");
		command_interpreter(ch, buf);
		snprintf(buf, 256, "say Footloose, I gotta cut footloose!");
		command_interpreter(ch, buf);
		snprintf(buf, 256, "dance");
		break;
	case 10:
		snprintf(buf, 256, "sload");
		command_interpreter(ch, buf);
		snprintf(buf, 256, "say Oops! I guess I was having TOO much fun on Duris!");
		command_interpreter(ch, buf);
		snprintf(buf, 256, "blush");
		break;
	case 11:
		snprintf(buf, 256, "say If I had to choose between sex and Duris, I choose Duris!");
		break;
	case 12:
		snprintf(
			buf, 256,
			"say Has anyone seen Aycer? I want to zone, but I'm just too lazy to learn how to lead!");
		break;
	case 13:
		snprintf(buf, 256, "omg");
		command_interpreter(ch, buf);
		snprintf(
			buf, 256,
			"say Torgal rules! I hope I'm just like him when I grow up, minus the fu-man-chu!");
		break;
	case 14:
		snprintf(buf, 256,
			 "say Someday, I hope I'm remembered as a great player! Like Vuthen!");
		command_interpreter(ch, buf);
		snprintf(buf, 256, "rofl me");
		break;
	case 15:
		snprintf(buf, 256,
			 "say People who whine about anything here should just be deleted!");
		break;
	case 16:
		snprintf(
			buf, 256,
			"say Try not to get too good at Duris, or Torgal will code lag to your character!");
		break;
	case 17:
		snprintf(buf, 256, "say Lions, Tigers and Cerif's, Oh My!");
		break;
	case 18:
		snprintf(
			buf, 256,
			"say Thank god this isn't Toril!  I'd hate to play a game with no development!");
		break;
	case 19:
		snprintf(
			buf, 256,
			"say Man, Lohrr really has done a lot of work on here.  We should cut him some slack for all the sleep he's lost.");
		break;
	case 20:
		snprintf(buf, 256, "say I wish exp was harder so we wouldn't level so quickly.");
		break;
	case 21:
		snprintf(
			buf, 256,
			"say Just think, if zones were worth less epics, we could do even more zones!");
		break;
	case 22:
		snprintf(buf, 256,
			 "say I wish mobs were more difficult.  I haven't died enough today.");
		break;
	case 23:
		snprintf(buf, 256, "say I love how Immortal's handle cheating here.");
		break;
	case 24:
		snprintf(
			buf, 256,
			"say I wish Lohrr would go out with me, but Immortals don't date mortals..");
		break;
	}
	command_interpreter(ch, buf);
}

bool check_freedom_of_movement(P_char ch, bool clear)
{
	// only the spell works for these checks
	if (affected_by_spell(ch, SPELL_FREEDOM_OF_MOVEMENT))
	{
		if (clear)
		{
			act("&+yYour movement is no longer unhindered!&n", TRUE, ch, 0, 0, TO_CHAR);
			affect_from_char(ch, SPELL_FREEDOM_OF_MOVEMENT);
		}
		return true;
	}
	return false;
}

void spell_curse(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj obj)
{
	struct affected_type af;

	if (!victim)
		victim = ch;

	if (victim)
		if (!IS_TRUSTED(ch) && resists_spell(ch, victim))
			return;

	if (IS_TRUSTED(victim) || affected_by_spell(victim, SPELL_CURSE))
	{
		send_to_char("Aren't they already cursed enough?\n", ch);
		return;
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_CURSE);
	if (obj)
	{
		SET_BIT(obj->extra_flags, ITEM_NODROP);
		/* LOWER ATTACK DICE BY -1 */
		if (obj->type == ITEM_WEAPON)
			obj->value[2]--;
		if (obj->value[2] < 1)
			obj->value[2] = 1;
		act("&+r$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
	}
	else if (IS_GREATER_RACE(victim) || IS_ELITE(victim))
	{
		if (NewSaves(victim, SAVING_SPELL, mod + 5))
			send_to_char("Your victim has saved against your curse spell!&n\n", ch);
		return;
	}
	else if (IS_NPC(victim) && NewSaves(victim, SAVING_SPELL, mod))
	{
		send_to_char("Your victim has saved against your curse spell!&n\n", ch);
		return;
	}
	else if (IS_PC(victim) && NewSaves(victim, SAVING_SPELL, mod))
	{
		send_to_char("Your victim has saved against your curse spell!&n\n", ch);
		return;
	}
	else
	{
		bzero(&af, sizeof(af));

		af.type = SPELL_CURSE;
		af.duration = GET_LEVEL(ch);
		af.modifier = (IS_NPC(victim) ? -10 : -5);
		af.location = APPLY_HITROLL;
		affect_to_char(victim, &af);
		af.modifier = 10;
		af.location = APPLY_CURSE;
		affect_to_char(victim, &af);

		act("&+r$n &+rbriefly reveals a red aura!", FALSE, victim, 0, 0, TO_ROOM);
		act("&+rYou suddenly feel very uncomfortable.", FALSE, victim, 0, 0, TO_CHAR);
	}
}

void spell_poison(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		  P_obj obj)
{
	bool was_poisoned;

	if (victim)
	{
		if (!IS_ALIVE(victim) || IS_TRUSTED(victim) || resists_spell(ch, victim))
		{
			return;
		}

		int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_POISON);
		if ((level > 0) && NewSaves(victim, SAVING_SPELL, mod))
		{
			return;
		}

		was_poisoned = IS_SET(victim->specials.affected_by2, AFF2_POISONED);

		if (!IS_TRUSTED(victim) && !IS_UNDEADRACE(victim))
		{
			level = abs(level);
			(skills[number(FIRST_POISON, LAST_POISON)].spell_pointer)(level, ch, 0, 0,
										  victim, 0);

			act("&+G$n shivers slightly.", TRUE, victim, 0, 0, TO_ROOM);
			if (was_poisoned)
				send_to_char("&+GYou feel even more ill.\n", victim);
			else
				send_to_char("&+GYou feel very sick.\n", victim);
		}
		appear(ch);

		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
			{
				MobStartFight(victim, ch);
			}
		}
	}
	else
	{ /* Object poison */
		if ((obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD))
		{
			obj->value[3] = number(1, MAX(1, GET_LEVEL(ch) / 10));
		}
		else
			send_to_char("The spell seems to have no effect on that object.\n", ch);
	}
}

void spell_remove_poison(int /*level*/, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
			 P_char victim, P_obj obj)
{
	if (victim)
	{
		if (poison_common_remove(victim))
		{
			act("&+WThe power of your god removes the poison.", FALSE, ch, 0, 0,
			    TO_CHAR);
			act("&+WYou suddenly feel healthier!", FALSE, ch, 0, victim, TO_VICT);
		}
		else
			send_to_char("You do your best, but it seems there is no poison to cure.\n",
				     ch);
	}
	else
	{
		if (obj->value[3])
		{
			if ((obj->type == ITEM_DRINKCON) || (obj->type == ITEM_FOOD))
			{
				obj->value[3] = 0;
				act("The $q steams briefly.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
		else
			send_to_char("It seems unchanged.\n", ch);
	}
}
