/* Staff controls for snooping, switching, returning, and forcing commands. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/mm.h"
#include "net/comm.h"
#include "sql/sql.h"
#include "world/db.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

extern P_desc descriptor_list;
extern P_room world;

void do_snoop(P_char ch, char *argument, int /*cmd*/)
{
	static char arg[MAX_STRING_LENGTH];
	P_char victim;
	P_desc point;
	int level;
	snoop_by_data *snoop_by_ptr;

	if (!ch->desc)
		return;
	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Snoop who ?\n", ch);
		return;
	}
	if (!(victim = get_char_vis(ch, arg)))
	{
		send_to_char("No such person around.\n", ch);
		return;
	}
	if (!victim->desc)
	{
		send_to_char("There's no link.. nothing to snoop.\n", ch);
		return;
	}
	if (victim == ch->desc->snoop.snooping)
	{
		send_to_char("Duh!  You already ARE snooping that person!\n", ch);
		return;
	}
	level = MIN(62, GET_LEVEL(ch));
	if (victim == ch)
	{
		send_to_char("Ok, you just snoop yourself.\n", ch);
		if (ch->desc->snoop.snooping)
		{
			if (level < 59)
				send_to_char("&+CYou are no longer being snooped.&N\n",
					     ch->desc->snoop.snooping);
			if (GET_LEVEL(ch) < FORGER)
			{
				sql_log(ch, WIZLOG, "Stopped snooping %s",
					GET_NAME(ch->desc->snoop.snooping));
			}
			rem_char_from_snoopby_list(
				&ch->desc->snoop.snooping->desc->snoop.snoop_by_list, ch);
			ch->desc->snoop.snooping = 0;
		}
		return;
	}

	if ((GET_LEVEL(victim) >= level))
	{
		send_to_char("You failed.\n", ch);
		return;
	}
	send_to_char("Ok. \n", ch);

	if (ch->desc->snoop.snooping)
	{
		if (level < 58)
			send_to_char("&+CYou are no longer being snooped.&N\n",
				     ch->desc->snoop.snooping);
		/*
		    ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;
		*/
		rem_char_from_snoopby_list(&ch->desc->snoop.snooping->desc->snoop.snoop_by_list,
					   ch);

		sql_log(ch, WIZLOG, "Stopped snooping %s", GET_NAME(ch->desc->snoop.snooping));
		logit(LOG_WIZ, "(%s) stopped snooping (%s)", GET_NAME(ch),
		      GET_NAME(ch->desc->snoop.snooping));
	}
	ch->desc->snoop.snooping = victim;
	/*
	  victim->desc->snoop.snoop_by = ch;
	*/
	CREATE(snoop_by_ptr, snoop_by_data, 1, MEM_TAG_SNOOP);
	bzero(snoop_by_ptr, sizeof(snoop_by_data));

	snoop_by_ptr->next = victim->desc->snoop.snoop_by_list;
	snoop_by_ptr->snoop_by = ch;

	victim->desc->snoop.snoop_by_list = snoop_by_ptr;

	// We need to move down past our victim on the descriptor list so things display properly.
	// First we pull ch->desc from the list:
	// If we're pulling from the head of the list
	if (descriptor_list == ch->desc)
	{
		descriptor_list = descriptor_list->next;
	}
	// If we're pulling from the middle of the list
	else
	{
		// Find the previous item and set it's next to the next item.
		point = descriptor_list;
		while (point->next != ch->desc)
		{
			point = point->next;
		}
		point->next = ch->desc->next;
	}
	// Point is now who we're snooping.
	point = ch->desc->snoop.snooping->desc;
	// Now we want to put ch->desc after point
	ch->desc->next = point->next;
	point->next = ch->desc;

	if (level < 58)
		send_to_char("&+CSomeone starts snooping you.&N\n", victim);

	if (GET_LEVEL(ch) < FORGER)
	{
		sql_log(ch, WIZLOG, "Started snooping %s", GET_NAME(victim));
	}
}

void do_switch(P_char ch, char *argument, int cmd)
{
	static char arg[MAX_STRING_LENGTH];
	snoop_by_data *snoop_by_ptr, *next;
	P_char victim;

	// If you're already switched, we un-switch you first.
	if (ch->desc && ch->desc->original)
	{
		do_return(ch, NULL, cmd);
	}

	if (IS_NPC(ch) || !ch->desc)
	{
		send_to_char("Sorry, no mobs or LD chars allowed.\n", ch);
		return;
	}
	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Switch with who?\n", ch);
		send_to_char(
			"&+YSyntax: &+wswitch <target> [silent]&n\n"
			"Where &+w<target>&n is the MOB / LD char you want to switch into.\n"
			"And &+w[silent]&n is an option to turn off messages to your Imm while switched.\n\r",
			ch);
		send_to_char(
			"&+RPlease note that the &+w[silent]&+R option might cause crashes.&n\n",
			ch);
	}
	else
	{
		if (!(victim = get_char_vis(ch, arg)))
		{
			send_to_char("They aren't here.\n", ch);
		}
		else
		{
			if (ch == victim)
			{
				send_to_char("He he he... We are jolly funny today, eh?\n", ch);
				return;
			}
			if (ch->desc->snoop.snooping)
			{
				send_to_char("Mixing snoop & switch is bad for your health.\n", ch);
				return;
			}
			// Can only switch into mobs and LD chars of lesser level.
			if (victim->desc || (IS_PC(victim) && (GET_LEVEL(ch) < GET_LEVEL(victim))))
			{
				send_to_char("You can't do that, the body is already in use!\n",
					     ch);
				return;
			}
			else if ((GET_LEVEL(ch) < OVERLORD))
			{
				wizlog(GET_LEVEL(ch), "%s has switched into '%s'.", GET_NAME(ch),
				       GET_NAME(victim));
				logit(LOG_WIZ, "%s has switched into '%s'.", GET_NAME(ch),
				      GET_NAME(victim));
			}

			// We send this message to the descriptor since ch had its desc removed at the end of this fn.
			SEND_TO_Q("Ok.\n", ch->desc);

			if (ch->desc->snoop.snoop_by_list)
			{
				snoop_by_ptr = ch->desc->snoop.snoop_by_list;
				while (snoop_by_ptr)
				{
					send_to_char(
						"Your victim has switched into something, killing your snoop.\n",
						snoop_by_ptr->snoop_by);
					snoop_by_ptr->snoop_by->desc->snoop.snooping = NULL;

					next = snoop_by_ptr->next;
					FREE(snoop_by_ptr);

					snoop_by_ptr = next;
				}
				ch->desc->snoop.snoop_by_list = NULL;
			}

			if (IS_TRUSTED(ch) && !IS_FIGHTING(ch))
			{
				if (GET_WIZINVIS(ch) < GET_LEVEL(ch))
				{
					act("$n's &+Wyeyes&n slowly &+wglaze&n over, and then $n slowly fades out &+wof view&+L...&n",
					    FALSE, ch, 0, 0, TO_ROOM);
					// The - 1 is because we don't want to be invis to others of the same level, esp for Overlords.
					GET_WIZINVIS(ch) = GET_LEVEL(ch) - 1;
				}
			}

			// Almost the same as the blink social... Just slightly different color.
			act("&+w$n&+w blinks in disbelief.&n", FALSE, victim, 0, 0, TO_ROOM);
			ch->desc->character = victim;
			ch->desc->original = ch;
			ch->only.pc->switched = victim;

			victim->desc = ch->desc;

			// We could, at this point, pull ch from room and leave the was_in_room, and put them back there
			//   upon do_return, but that's not really necessary.
			one_argument(argument, arg);
			if (!strcmp(arg, "silent"))
			{
				ch->desc = NULL;
			}
		}
	}
}

void do_return(P_char ch, char * /*argument*/, int /*cmd*/)
{
	/*  if(CHAR_POLYMORPH_OBJ(ch))
	  {
	    return_from_poly_obj(ch);
	    return;
	  }*/

	if (IS_AFFECTED(ch, AFF_WRAITHFORM))
	{
		BackToUsualForm(ch);
		return;
	}
	if (!ch->desc)
		return;

	if (!ch->desc->original || IS_NPC(ch->desc->original))
	{
		send_to_char("Pardon?\n", ch);
		return;
	}
	if (IS_MORPH(ch))
	{
		send_to_char("Use \"shape me\" to return to your normal form.\n", ch);
		return;
	}
	if (ch->desc->original->only.pc->switched)
	{
		P_char switched_mob = ch->desc->character; // Save reference to the mob

		send_to_char("You return to your original body.\n", ch);

		ch->desc->character = ch->desc->original;
		ch->desc->original = 0;
		ch->desc->character->only.pc->switched = 0;

		ch->desc->character->desc = ch->desc;
		switched_mob->desc = 0; // Clear the MOB's desc, not ch->desc
	}
	else /* switched body due to shape change  */
		send_to_char("No effect.\n", ch);
}

int forced_command = 0;

void do_force(P_char ch, char *argument, int /*cmd*/)
{
	P_desc i;
	P_char vict = NULL;
	int level;
	char name[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH + 60];

	if (IS_NPC(ch))
		return;

	level = MIN(62, GET_LEVEL(ch));

	half_chop(argument, name, to_force);

	if (!*name || !*to_force)
		send_to_char("Who do you wish to force to do what?\n", ch);
	else if (str_cmp("all", name))
	{
		if (!(vict = get_char_vis(ch, name)))
		{
			send_to_char("No-one by that name here..\n", ch);
			return;
		}
		else if (!str_cmp("quit", to_force))
		{
			send_to_char("Cannot force that player to quit.\n", ch);
			return;
		}
		else if (!strn_cmp("jun", to_force, 3))
		{
			send_to_char("Cannot force that player to junk.\n", ch);
			return;
		}
		else if (!str_cmp("fafhrd", name))
		{
			send_to_room(
				"&+yThe ground begins to shake...&n &=LBLightning&n&+B streaks towards your head...&n\n",
				ch->in_room);
			act("And Fafhrd turns to $n with a wicked grin... 'I don't think so'",
			    FALSE, ch, 0, 0, TO_ROOM);
			send_to_char(
				"And Fafhrd turns to you with a wicked grin... 'I don't think so'\n",
				ch);
			return;
		}
		else
		{
			if ((level <= GET_LEVEL(vict)) && IS_PC(vict))
				send_to_char("Ok.\n", ch);
			else
			{
				snprintf(buf, sizeof buf, "$n has forced you to '%s'.", to_force);
				act(buf, FALSE, ch, 0, vict, TO_VICT);
				if (level < 62)
					wizlog(GET_LEVEL(ch), "%s has forced %s to '%s' [%d/%d]",
					       GET_NAME(ch), GET_NAME(vict), to_force,
					       world[ch->in_room].number,
					       world[vict->in_room].number);
				logit(LOG_FORCE, "%s has forced %s to '%s' [%d/%d]", GET_NAME(ch),
				      GET_NAME(vict), to_force, world[ch->in_room].number,
				      world[vict->in_room].number);
				sql_log(ch, WIZLOG, "Forced %s to '%s'", GET_NAME(vict), to_force);
				send_to_char("Ok.\n", ch);
				forced_command = 1;
				command_interpreter(vict, to_force);
				forced_command = 0;
			}
		}
	}
	else
	{ /* force all  */
		wizlog(level, "%s has forced all to '%s'", GET_NAME(ch), to_force);
		logit(LOG_FORCE, "%s has forced all to '%s'", GET_NAME(ch), to_force);
		sql_log(ch, WIZLOG, "Forced all to '%s'", to_force);
		for (i = descriptor_list; i; i = i->next)
			if (i->character != ch && !i->connected)
			{
				vict = i->character;
				if ((level > GET_LEVEL(vict)))
				{
					snprintf(buf, sizeof buf, "$n has forced you to '%s'.",
						 to_force);
					act(buf, FALSE, ch, 0, vict, TO_VICT);
					forced_command = 1;
					command_interpreter(vict, to_force);
					forced_command = 0;
				}
			}
		send_to_char("Ok.\n", ch);
	}
}
