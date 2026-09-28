#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "core/defines.h"
#include "magic/spells.h"
#include <strings.h>

void spell_animal_growth(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			 P_obj /*obj*/)
{
	int size, size2;

	switch (GET_RACE(victim))
	{
	case RACE_HERBIVORE:
	case RACE_CARNIVORE:
	case RACE_REPTILE:
	case RACE_SNAKE:
	case RACE_INSECT:
	case RACE_ARACHNID:
	case RACE_AQUATIC_ANIMAL:
	case RACE_FLYING_ANIMAL:
	case RACE_QUADRUPED:
	case RACE_ANIMAL:
	case RACE_PRIMATE:
		break;
	default:
		send_to_char("Your target must be an animal!\n", ch);
		return;
		break;
	}
	size = GET_SIZE(victim);
	size2 = GET_ALT_SIZE(ch);

	if ((size + 1 < size2) || (size - 1 > size2))
	{
		send_to_char("You get the feeling your body couldn't change quite that much.\r\n",
			     ch);
		return;
	}
	else if (size < size2)
	{
		spell_reduce(GET_LEVEL(ch), ch, 0, SPELL_ANIMAL_GROWTH, ch, NULL);
	}
	else if (size > size2)
	{
		spell_enlarge(GET_LEVEL(ch), ch, 0, SPELL_ANIMAL_GROWTH, ch, NULL);
	}
	else if (size == size2)
	{
		send_to_char("It's the same size as you, duh!\r\n", ch);
	}
}

void spell_enlarge(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj /*obj*/)
{
	bool animal_growth = FALSE;
	struct affected_type af;

	if (IS_AFFECTED3(victim, AFF3_ELEMENTAL_FORM) || IS_AFFECTED5(victim, AFF5_TITAN_FORM))
	{
		send_to_char("Nah.  They're quite big already.\n", ch);
		return;
	}

	if (ch != victim)
		if (!is_linked_to(ch, victim, LNK_CONSENT))
			return;

	if (type == SPELL_ANIMAL_GROWTH)
		animal_growth = TRUE;

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_ENLARGE);
	if (!NewSaves(victim, SAVING_SPELL, mod) || (ch == victim) ||
	    is_linked_to(ch, victim, LNK_CONSENT))
	{
		if (IS_SET(victim->specials.affected_by3, AFF3_REDUCE))
		{
			REMOVE_BIT(victim->specials.affected_by3, AFF3_REDUCE);
			if (affected_by_spell(victim, SPELL_REDUCE))
				affect_from_char(victim, SPELL_REDUCE);
			act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
			act("You return to your normal size!", TRUE, ch, 0, victim, TO_VICT);
			act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
			return;
		}
		if (GET_SIZE(victim) == SIZE_MAXIMUM)
			return;

		if (!affected_by_spell(victim, SPELL_ENLARGE))
		{
			bzero(&af, sizeof(af));
			af.type = SPELL_ENLARGE;
			af.duration = 3;
			af.bitvector3 = AFF3_ENLARGE;
			affect_to_char(victim, &af);

			af.bitvector3 = 0;
			af.modifier = (victim->base_stats.Str / 10);
			af.location = APPLY_STR_MAX;
			affect_to_char(victim, &af);

			af.modifier = (victim->base_stats.Con / 4);
			af.location = APPLY_CON_MAX;
			affect_to_char(victim, &af);

			af.modifier = -(victim->base_stats.Agi / 5);
			af.location = APPLY_AGI;
			affect_to_char(victim, &af);

			af.modifier = -(victim->base_stats.Dex / 5);
			af.location = APPLY_DEX;
			affect_to_char(victim, &af);

			if (animal_growth) /*  messages for animal growth */
			{
				act("You &+Gconcentrate&N on your target and &+Ytransform&n your body accordingly!",
				    TRUE, ch, 0, 0, TO_CHAR);
				act("$n &+Gconcentrates&N for a few seconds as $s body takes on a &+Ynew size!&N",
				    TRUE, ch, 0, 0, TO_ROOM);
			}
			else
			{
				act("$N grows to about twice $S normal size!", TRUE, ch, 0, victim,
				    TO_CHAR);
				act("You grow to about twice your normal size!", TRUE, ch, 0,
				    victim, TO_VICT);
				act("$N grows to about twice $S normal size!", TRUE, ch, 0, victim,
				    TO_NOTVICT);
			}
		}
		else
		{
			struct affected_type *af1;

			for (af1 = victim->affected; af1; af1 = af1->next)
			{
				if (af1->type == SPELL_ENLARGE)
				{
					af1->duration = level;
				}
			}
		}

		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
	}
}

void spell_reduce(int level, P_char ch, char * /*arg*/, int type, P_char victim, P_obj /*obj*/)
{
	bool animal_growth = FALSE;
	struct affected_type af;

	if (type == SPELL_ANIMAL_GROWTH)
		animal_growth = TRUE;

	if (GET_SIZE(victim) == SIZE_TINY && !(victim->specials.affected_by3 & AFF3_REDUCE))
	{
		send_to_char("Why would you want to reduce them? They are tiny enough!&n\n\r", ch);
		return; // check to make sure they are not racial tiny.
	}

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_REDUCE);
	if (!NewSaves(victim, SAVING_SPELL, mod) || (ch == victim) ||
	    is_linked_to(ch, victim, LNK_CONSENT))
	{
		if (IS_SET(victim->specials.affected_by3, AFF3_ENLARGE))
		{
			REMOVE_BIT(victim->specials.affected_by3, AFF3_ENLARGE);
			if (affected_by_spell(victim, SPELL_ENLARGE))
				affect_from_char(victim, SPELL_ENLARGE);
			act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_CHAR);
			act("You return to your normal size!", TRUE, ch, 0, victim, TO_VICT);
			act("$N returns to $S normal size!", TRUE, ch, 0, victim, TO_NOTVICT);
			return;
		}
		if (GET_SIZE(victim) == SIZE_MINIMUM)
			return;

		if (!affected_by_spell(victim, SPELL_REDUCE))
		{
			bzero(&af, sizeof(af));
			af.type = SPELL_REDUCE;
			af.duration = 3;
			af.bitvector3 = AFF3_REDUCE;
			affect_to_char(victim, &af);

			af.bitvector3 = 0;
			af.modifier = -(victim->base_stats.Str / 4);
			af.location = APPLY_STR;
			affect_to_char(victim, &af);

			af.modifier = -(victim->base_stats.Con / 5);
			af.location = APPLY_CON;
			affect_to_char(victim, &af);

			af.modifier = (victim->base_stats.Agi / 5);
			af.location = APPLY_AGI_MAX;
			affect_to_char(victim, &af);

			af.modifier = (victim->base_stats.Dex / 5);
			af.location = APPLY_DEX_MAX;
			affect_to_char(victim, &af);

			if (animal_growth) /*  messages for animal growth */
			{
				act("You &+Gconcentrate&N on your target and &+Ytransform&n your body accordingly!",
				    TRUE, ch, 0, 0, TO_CHAR);
				act("$n &+Gconcentrates&N for a few seconds as $s body takes on a &+Ynew size!&N",
				    TRUE, ch, 0, 0, TO_ROOM);
			}
			else
			{
				act("$N shrinks to about half $S normal size!", TRUE, ch, 0, victim,
				    TO_CHAR);
				act("You shrink to about half your normal size!", TRUE, ch, 0,
				    victim, TO_VICT);
				act("$N shrinks to about half $S normal size!", TRUE, ch, 0, victim,
				    TO_NOTVICT);
			}
		}
		else
		{
			struct affected_type *af1;

			for (af1 = victim->affected; af1; af1 = af1->next)
				if (af1->type == SPELL_REDUCE)
				{
					af1->duration = level;
				}
		}
		if (IS_NPC(victim) && CAN_SEE(victim, ch))
		{
			remember(victim, ch);
			if (!IS_FIGHTING(victim))
				MobStartFight(victim, ch);
		}
	}
}
