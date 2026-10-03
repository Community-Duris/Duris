/* Staff commands for character and mobile moderation. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "net/comm.h"
#include "sql/sql.h"
#include "world/db.h"

#include <stdio.h>
#include <string.h>

extern P_room world;
void do_freeze(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict = NULL;
	P_obj dummy;
	char buf[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);
	level = MIN(62, GET_LEVEL(ch));

	if (!*buf)
		send_to_char("Usage: freeze <player>\n", ch);
	else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
		send_to_char("Couldn't find any such creature.\n", ch);
	else if (IS_NPC(vict))
		send_to_char("You can't freeze a mobile.\n", ch);
	else if (GET_LEVEL(vict) >= level)
		act("$E is too hot for you to freeze..", 0, ch, 0, vict, TO_CHAR);
	else if (IS_SET(vict->specials.act, PLR_FROZEN))
	{
		send_to_char("You have thawed out and can move again freely.\n", vict);
		send_to_char("Player has thawed out.\n", ch);
		REMOVE_BIT(vict->specials.act, PLR_FROZEN);
		if (GET_LEVEL(ch) > 50)
		{
			wizlog(GET_LEVEL(ch), "%s was just unfrozen by %s.", GET_NAME(vict),
			       GET_NAME(ch));
			logit(LOG_WIZ, "%s was just unfrozen by %s.", GET_NAME(vict), GET_NAME(ch));
			sql_log(ch, WIZLOG, "Unfroze %s", GET_NAME(vict));
		}
	}
	else
	{
		send_to_char("You suddenly feel very frozen and can't move.\n", vict);
		send_to_char("FROZEN set.\n", ch);
		SET_BIT(vict->specials.act, PLR_FROZEN);
		if (GET_LEVEL(ch) > 50)
		{
			wizlog(GET_LEVEL(ch), "%s was just frozen by %s.", GET_NAME(vict),
			       GET_NAME(ch));
			logit(LOG_WIZ, "%s was just frozen by %s.", GET_NAME(vict), GET_NAME(ch));
			sql_log(ch, WIZLOG, "Froze %s", GET_NAME(vict));
		}
	}
}

void do_silence(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict;
	P_obj dummy;
	char buf[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);
	level = MIN(62, GET_LEVEL(ch));
	if (!*buf)
		send_to_char("Usage: silence <player>\n", ch);
	else if (!generic_find(argument, FIND_CHAR_WORLD, ch, &vict, &dummy))
		send_to_char("Couldn't find any such creature.\n", ch);
	else if (IS_NPC(vict))
		send_to_char("Can't do that to a mobile.\n", ch);
	else if (GET_LEVEL(vict) > level)
		act("$E might object to that.. better not.", 0, ch, 0, vict, TO_CHAR);
	else if (IS_SET(vict->specials.act, PLR_SILENCE))
	{
		send_to_char(
			"You can use communications channels again...but don't piss off the gods again!\n",
			vict);
		send_to_char("SILENCE removed.\n", ch);
		REMOVE_BIT(vict->specials.act, PLR_SILENCE);
		if (GET_LEVEL(ch) > 56)
		{
			wizlog(GET_LEVEL(ch), "%s just removed the silence on %s.", GET_NAME(ch),
			       GET_NAME(vict));
			logit(LOG_WIZ, "%s just removed the silence on %s.", GET_NAME(ch),
			      GET_NAME(vict));
			sql_log(ch, WIZLOG, "Unsilenced %s", GET_NAME(vict));
		}
	}
	else
	{
		send_to_char(
			"&+WThe gods take away your ability to use ANY communications channel!\n",
			vict);
		send_to_char("SILENCE set.\n", ch);
		SET_BIT(vict->specials.act, PLR_SILENCE);
		if (GET_LEVEL(ch) > 56)
		{
			wizlog(GET_LEVEL(ch), "%s was just silenced by %s.", GET_NAME(vict),
			       GET_NAME(ch));
			logit(LOG_WIZ, "%s was just silenced by %s.", GET_NAME(vict), GET_NAME(ch));
			sql_log(ch, WIZLOG, "Silenced %s", GET_NAME(vict));
		}
	}
}

static void tranquilize(P_char ch, P_char victim)
{
	if (!victim)
		return;

	if (!IS_TRUSTED(ch))
	{
		if (GET_LEVEL(victim) >= GET_LEVEL(ch))
			return;
	}

	if (GET_OPPONENT(victim))
	{
		for (P_char tch = world[victim->in_room].people; tch; tch = tch->next_in_room)
		{
			if (GET_OPPONENT(tch) && (GET_OPPONENT(tch) == victim))
			{
				stop_fighting(tch);
				clearMemory(tch);
			}
		}

		stop_fighting(victim);
		clearMemory(victim);
	}

	act("$n begins to speak in a monotone voice. You nod off immediately out of boredom.",
	    FALSE, ch, 0, victim, TO_VICT);
	if (!IS_TRUSTED(victim))
	{
		do_sleep(victim, 0, 0);
	}
}

void do_tranquilize(P_char ch, char *argument, int /*cmd*/)
{
	if (!ch || !IS_PC(ch) || !IS_TRUSTED(ch))
		return;

	char buf[100];

	one_argument(argument, buf);

	if (*buf)
	{
		// one target
		P_char victim = get_char_room_vis(ch, buf);

		if (!victim)
		{
			send_to_char("Nobody with that name here.\n", ch);
			return;
		}

		act("You begin to speak in a monotone voice. $N nods off immediately out of boredom.",
		    FALSE, ch, 0, victim, TO_CHAR);
		tranquilize(ch, victim);
	}
	else
	{
		act("You begin to speak in a monotone voice. The entire room nods off immediately out of boredom.",
		    FALSE, ch, 0, 0, TO_CHAR);
		// all
		for (P_char tch = world[ch->in_room].people; tch; tch = tch->next_in_room)
		{
			if (tch == ch || IS_TRUSTED(tch))
				continue;

			tranquilize(ch, tch);
		}
	}
}

ACMD(do_depiss)
{
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
	P_char vic = NULL, mob = NULL;

	half_chop(argument, buf1, buf2);

	if (!*buf1 || !*buf2)
	{
		send_to_char("Usage: depiss <player> <mobile>\n", ch);
		return;
	}
	if (((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic)))
	{
		if (((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob)))
		{
			if (IS_SET(mob->specials.act, ACT_MEMORY))
				forget(mob, vic);
			else
			{
				send_to_char("Mobile does not have the memory flag set!\n", ch);
				return;
			}
		}
		else
		{
			send_to_char("Sorry, Player Not Found!\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("Sorry, Mobile Not Found!\n", ch);
		return;
	}
	snprintf(buf3, MAX_STRING_LENGTH, "%s has been removed from %s's pissed list.\n",
		 J_NAME(vic), J_NAME(mob));
	send_to_char(buf3, ch);
}

ACMD(do_repiss)
{
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
	P_char vic = NULL, mob = NULL;

	half_chop(argument, buf1, buf2);

	if (!*buf1 || !*buf2)
	{
		send_to_char("Usage: repiss <player> <mobile>\n", ch);
		return;
	}
	if (((vic = get_char_vis(ch, buf1))) && (!IS_NPC(vic)))
	{
		if (((mob = get_char_vis(ch, buf2))) && (IS_NPC(mob)))
		{
			if (IS_SET(mob->specials.act, ACT_MEMORY))
				remember(mob, vic);
			else
			{
				send_to_char("Mobile does not have the memory flag set!\n", ch);
				return;
			}
		}
		else
		{
			send_to_char("Sorry, Player Not Found!\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("Sorry, Mobile Not Found!\n", ch);
		return;
	}
	snprintf(buf3, MAX_STRING_LENGTH, "%s has been added to %s's pissed list.\n", J_NAME(vic),
		 J_NAME(mob));
	send_to_char(buf3, ch);
}
