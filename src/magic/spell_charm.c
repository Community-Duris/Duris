#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/training_dummy.h"
#include "magic/spells.h"

void charm_generic(int level, P_char ch, P_char victim)
{
	int i;
	int c; // count of druid followers
	bool failed = FALSE;
	struct follow_type *followers;
	int num;

	if (GET_STAT(ch) == STAT_DEAD)
		return;

	if (training_dummy_is(victim))
	{
		send_to_char("The training dummy has no mind to charm.\r\n", ch);
		return;
	}

	if (resists_spell(ch, victim))
		return;

	if (victim == ch)
	{
		send_to_char("You contemplate how charming you are.\n", ch);
		return;
	}
	if (circle_follow(victim, ch))
	{
		send_to_char("Sorry, following in circles cannot be allowed.\n", ch);
		return;
	}
	if (CHAR_IN_SAFE_ROOM(ch))
	{
		send_to_char("You wouldn't even consider that, would you?\n", ch);
		return;
	}
	if (IS_PC(ch))
		CharWait(ch, PULSE_VIOLENCE);

	if (!CAN_SEE(victim, ch))
		failed = TRUE;

	if (IS_SHOPKEEPER(victim))
		failed = TRUE;

	num = 0;
	c = 0;
	/* can only have 1 charmie now... because the charmies have full mob
	   stats */
	for (i = 0; (i < MAX_WEAR) && !failed; i++)
		if (victim->equipment[i] && IS_SET(victim->equipment[i]->extra_flags, ITEM_NOCHARM))
			failed = TRUE;

	// druids get 15 lvls and below animals, everyone else 2/3 lvl
	if (GET_CLASS(ch, CLASS_DRUID) ||
	    (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
	{
		if ((GET_LEVEL(victim) > (GET_LEVEL(ch) - 10)))
		{
			failed = TRUE;
		}
	}
	else if ((level * 2 / 3) < (int)GET_LEVEL(victim))
	{
		failed = TRUE;
	}

	/*
	 * Moved the # of followers check below - Clav
	 */

	if (!failed && GET_MASTER(victim))
	{
		/*
		 * several possibilities if victim is already charmed
		 */
		if (!victim->following)
		{
			/*
			 * should only happen when bit is set by other than spell
			 */
			failed = TRUE;
		}
		else if (level > (int)GET_LEVEL(victim->following))
		{
			/*
			 * victim gets another save versus initial charm (with bonus)
			 */
			if (NewSaves(victim, SAVING_PARA, GET_LEVEL(victim->following) - level))
			{
				clear_links(victim, LNK_PET);
			}
			else
				failed = TRUE;
		}
		else
			failed = TRUE;
	}
	if (!failed && saves_spell(victim, SAVING_PARA))
		failed = TRUE;

	/*
	 * If a druid is 25 levels higher than an animal, it'll be an
	 * automatic innate charm animal
	 */
	if ((GET_CLASS(ch, CLASS_DRUID) ||
	     (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID))) &&
	    ((GET_LEVEL(victim) < (GET_LEVEL(ch) - 25))) && is_natural_creature(victim))
	{
		failed = FALSE;
	}

	// druids can have more then one charmed animal, and it's level based!!
	for (followers = ch->followers; followers; followers = followers->next)
	{
		if (GET_CLASS(ch, CLASS_DRUID) ||
		    (IS_MULTICLASS_PC(ch) && GET_SECONDARY_CLASS(ch, CLASS_DRUID)))
		{
			c++;
			if (GET_LEVEL(ch) <= 30)
			{
				if (c >= 4)
				{
					failed = TRUE;
				}
			}
			if ((GET_LEVEL(ch) > 30) && (GET_LEVEL(ch) <= 50))
			{
				if (c >= 5)
				{
					failed = TRUE;
				}
			}

			if ((GET_LEVEL(ch) >= 51) && (GET_LEVEL(ch) < 56))
			{
				if (c >= 6)
				{
					failed = TRUE;
				}
			}

			if ((GET_LEVEL(ch) == 56))
			{
				if (c >= 7)
				{
					failed = TRUE;
				}
			}
		}
		else if (followers->follower &&
			 affected_by_spell(followers->follower, SPELL_CHARM_PERSON))
		{
			failed = TRUE;
			/*    everyone else can have one follower .. */
		}
	}

	if (IS_TRUSTED(ch))
	{
		failed = FALSE;
	}

	if (failed)
	{
		send_to_char("Your victim doesn't seem to find you particularly charming...\n", ch);
		act("$n tried to charm you, but failed!", FALSE, ch, 0, victim, TO_VICT);

		if (CAN_SEE(victim, ch))
		{
			/* if they fail, wham! */
			remember(victim, ch);

			hit(victim, ch, ch->equipment[PRIMARY_WEAPON]);
		}
		return;
	}
	/*
	 * if get here, spell worked, so do the nasty thing
	 */

	if (victim->following && (victim->following != ch))
		stop_follower(victim);

	// Uncharm victim's pets.
	while (victim->followers)
	{
		clear_links(victim->followers->follower, LNK_PET);
	}

	if (!victim->following)
		add_follower(victim, ch);

	/* the duration maxes out at GET_LEVEL(ch)/10 mud days. */
	if (GET_C_INT(victim))
		num = MIN((GET_LEVEL(ch) / 10 * 24) + 1, 200 / STAT_INDEX(GET_C_INT(victim)));
	else
		num = 4;

	if (IS_TRUSTED(ch))
		num = (num > 40) ? num : 40;

	setup_pet(victim, ch, num, 0);

	act("You stand enthralled by $n's charming personality...", FALSE, ch, 0, victim, TO_VICT);

	if (IS_FIGHTING(victim))
		stop_fighting(victim);

	/*
	 * stop all non-vicious/agg attackers
	 */
	StopMercifulAttackers(victim);
}

void spell_animal_friendship(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	act("You feel overwhelmed with &+Mcompassion&n...\n", FALSE, ch, 0, 0, TO_CHAR);

	if (is_natural_creature(victim))
	{
		if (IS_FIGHTING(victim))
		{
			if (GET_OPPONENT(victim))
			{
				stop_fighting(victim);
				stop_fighting(ch);
				clearMemory(victim);
				return;
			}
		}
		else
		{
			charm_generic(level, ch, victim);
		}
	}
	else
		act("Apparently, this creature is not close enough to the nature!\n", FALSE, ch, 0,
		    0, TO_CHAR);
}

void spell_charm_person(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	if (!IS_HUMANOID(victim))
	{
		send_to_char("That doesn't look like a person to me...\n", ch);
		return;
	}
	charm_generic(level, ch, victim);
}

void spell_charm_animal(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			P_obj /*obj*/)
{
	if (IS_HUMANOID(victim))
	{
		send_to_char("Just because they SMELL like an animal doesn't mean they are one!\n",
			     ch);
		return;
	}
	charm_generic(level, ch, victim);
}
