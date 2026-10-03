#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <strings.h>

#define MACE_OF_EARTH_VNUM 23805
void spell_armor(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		 P_obj /*obj*/)
{
	struct affected_type af;
	double mod;
	bool shown;

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
	{
		return;
	}

	if (GET_SPEC(ch, CLASS_CLERIC, SPEC_HOLYMAN))
	{
		mod = 1.25;
	}
	else
	{
		mod = 1;
	}

	if (!IS_AFFECTED(victim, AFF_ARMOR))
	{
		bzero(&af, sizeof(af));
		af.type = SPELL_ARMOR;
		af.duration = 20;
		af.modifier = (int)(-1 * (mod * level) - number(0, 10));
		af.location = APPLY_AC;
		af.bitvector = AFF_ARMOR;
		affect_to_char(victim, &af);
		send_to_char("&+WBands of magic armor wrap around you!\n", victim);
	}
	else
	{
		struct affected_type *af1;
		shown = FALSE;
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_ARMOR)
			{
				if (!shown)
				{
					send_to_char("&+WThe bands of magic armor glow as new!\n",
						     victim);
					shown = TRUE;
				}
				af1->duration = 20;
			}
		}
		if (!shown)
		{
			send_to_char("&+WYou're already affected by an armor-type spell.\n",
				     victim);
		}
	}
}

void spell_group_stone_skin(int level, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
			    P_obj /*obj*/)
{
	struct group_list *gl;

	if (ch && ch->group)
	{
		gl = ch->group;
		/* leader first */
		if (gl->ch->in_room == ch->in_room)
			spell_stone_skin((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		/* followers */
		for (gl = gl->next; gl; gl = gl->next)
		{
			if (gl->ch->in_room == ch->in_room)
				spell_stone_skin((level / 3) * 2, ch, 0, 0, gl->ch, 0);
		}
	}
}

bool has_skin_spell(P_char ch)
{
	if (!affected_by_spell(ch, SPELL_STONE_SKIN) && !IS_AFFECTED(ch, AFF_STONE_SKIN) &&
	    !affected_by_spell(ch, SPELL_SHADOW_SHIELD) &&
	    !affected_by_spell(ch, SPELL_BIOFEEDBACK) && !IS_AFFECTED(ch, AFF_BIOFEEDBACK) &&
	    !affected_by_spell(ch, SPELL_IRONWOOD) && !affected_by_spell(ch, SPELL_VINES) &&
	    !IS_AFFECTED5(ch, AFF5_VINES) && !affected_by_spell(ch, SPELL_ICE_ARMOR) &&
	    !affected_by_spell(ch, SPELL_NEG_ARMOR) &&
	    !affected_by_spell(ch, SPELL_DRAKESCALE_AEGIS))
		return false;
	else
		return true;
}

void spell_stone_skin(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type,
		      P_char victim, P_obj /*obj*/)
{
	struct affected_type af;
	int absorb = (level / 4) + number(1, 4);

	if (!has_skin_spell(victim))
	{
		if (has_innate(ch, INNATE_LIVING_STONE) ||
		    (victim->equipment[WIELD] &&
		     (obj_index[victim->equipment[WIELD]->R_num].virtual_number ==
		      MACE_OF_EARTH_VNUM)))
		{
			absorb = (int)(absorb * 1.5);
			act("&+LLiving stone sprouts up and covers $n's flesh.", TRUE, victim, 0, 0,
			    TO_ROOM);
			act("&+LLiving stone sprouts up and covers your flesh.", TRUE, victim, 0, 0,
			    TO_CHAR);
		}
		else if (GET_CLASS(ch, CLASS_CONJURER))
		{
			absorb = (int)(absorb * 1.2);
			act("&+L$n's flesh melds with conjured rocks, turning it to stone.", TRUE,
			    victim, 0, 0, TO_ROOM);
			act("&+LYour flesh melds with conjured rocks, turning it to stone.", TRUE,
			    victim, 0, 0, TO_CHAR);
		}
		else if (GET_CLASS(ch, CLASS_SORCERER))
		{
			absorb = (int)(absorb * 1.0);
			act("&+L$n's flesh magically hardens, turning to stone.", TRUE, victim, 0,
			    0, TO_ROOM);
			act("&+LYour flesh magically hardens, turning to stone.", TRUE, victim, 0,
			    0, TO_CHAR);
		}
		else
		{
			absorb = (int)(absorb * .8);
			act("&+L$n&+L's skin seems to turn to stone.", TRUE, victim, 0, 0, TO_ROOM);
			act("&+LYou feel your skin harden to stone.", TRUE, victim, 0, 0, TO_CHAR);
		}
	}
	else
	{
		send_to_char("Their skin is already hard as a rock!\n", ch);
		return;
	}

	if (GET_OPPONENT(victim))
		gain_exp(ch, victim, 50 + GET_LEVEL(ch) * 2,
			 EXP_HEALING); // stoning the tank equal to small heal in exp -Odorf

	bzero(&af, sizeof(af));
	af.type = SPELL_STONE_SKIN;
	af.duration = 4;
	af.modifier = absorb;
	affect_to_char(victim, &af);
}

void spell_ironwood(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		    P_obj /*obj*/)
{
	struct affected_type af;

	if (!IS_AFFECTED(victim, AFF_BARKSKIN) && !IS_AFFECTED5(victim, AFF5_THORNSKIN))
	{
		send_to_char(
			"They're not even made of wood! How can you begin to make it resemble iron!\n",
			ch);
		return;
	}
	if (has_skin_spell(victim))
	{
		send_to_char("Their skin is already hard as a rock!\n", ch);
		return;
	}

	act("&+y$n's &+ybarkskin seems to take on the texture of &+Liron.", TRUE, victim, 0, 0,
	    TO_ROOM);
	act("&+yYou feel your barkskin harden to &+Liron.", TRUE, victim, 0, 0, TO_CHAR);

	// barkskin twice as effective as thornskin at absorbing
	int absorb = (level / (IS_AFFECTED(victim, AFF_BARKSKIN) ? 5 : 10)) + number(1, 4);

	// Stoning the tank equals to heal in exp -Odorf
	if (GET_OPPONENT(victim))
		gain_exp(ch, victim, absorb, EXP_HEALING);

	bzero(&af, sizeof(af));
	af.type = SPELL_IRONWOOD;
	af.duration = 4;
	af.modifier = absorb;
	affect_to_char(victim, &af);
}

void spell_barkskin(int level, P_char ch, char * /*arg*/, [[maybe_unused]] int type, P_char victim,
		    P_obj /*obj*/)
{
	struct affected_type af1;
	double mod = 1;
	bool shown;

	if (!ch)
	{
		logit(LOG_EXIT, "spell_barkskin called in magic.c with no ch");
		return;
	}
	if (!IS_ALIVE(ch))
	{
		act("Lay still, you seem to be dead!", TRUE, ch, 0, 0, TO_CHAR);
		return;
	}
	if (!IS_ALIVE(victim))
	{
		act("$N is not a valid target.", TRUE, ch, 0, victim, TO_CHAR);
		return;
	}
	if (racewar(ch, victim))
	{
		if (NewSaves(victim, SAVING_PARA, 0))
		{
			act("$N evades your spell!", TRUE, ch, 0, victim, TO_CHAR);
			return;
		}
	}
	if (GET_SPEC(ch, CLASS_DRUID, SPEC_WOODLAND))
	{
		mod = 1.25;
	}
	else
	{
		mod = 1;
	}
	if (!IS_AFFECTED(victim, AFF_ARMOR))
	{
		bzero(&af1, sizeof(af1));
		af1.type = SPELL_BARKSKIN;
		af1.duration = 25;
		af1.modifier = (int)(-1 * mod * level);
		af1.location = APPLY_AC;
		af1.bitvector = AFF_BARKSKIN | AFF_ARMOR;

		affect_to_char(victim, &af1);
		act("$n's skin gains the texture and toughness of &+ybark.&n", FALSE, victim, 0, 0,
		    TO_ROOM);
		act("Your skin gains the texture and toughness of &+ybark.&n", FALSE, victim, 0, 0,
		    TO_CHAR);
	}
	else if (!IS_AFFECTED(victim, AFF_BARKSKIN) && !IS_AFFECTED5(victim, AFF5_THORNSKIN))
	{
		bzero(&af1, sizeof(af1));
		af1.type = SPELL_BARKSKIN;
		af1.duration = 25;
		af1.bitvector = AFF_BARKSKIN;

		affect_to_char(victim, &af1);
		act("$n's skin gains the texture of &+ybark.&n", FALSE, victim, 0, 0, TO_ROOM);
		act("Your skin gains the texture of &+ybark.&n", FALSE, victim, 0, 0, TO_CHAR);
	}
	else
	{
		struct affected_type *af1;

		shown = FALSE;
		for (af1 = victim->affected; af1; af1 = af1->next)
		{
			if (af1->type == SPELL_BARKSKIN)
			{
				if (!shown)
				{
					send_to_char("&+yYour skin rehardens.\n", victim);
					shown = TRUE;
				}
				af1->duration = 25;
			}
		}
		if (!shown)
		{
			send_to_char("&+WYou're already affected by an armor-type spell.\n",
				     victim);
		}
	}
}

void spell_mass_barkskin(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	LOOP_THRU_PEOPLE(victim, ch)
	{
		if (ch->specials.z_cord == victim->specials.z_cord)
			spell_barkskin(level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}
}
