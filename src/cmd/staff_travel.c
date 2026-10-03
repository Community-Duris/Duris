/* Staff travel, teleport, and arrival/departure handlers. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "net/comm.h"
#include "sql/sql.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;

void do_trans(P_char ch, char *argument, int /*cmd*/)
{
	P_desc i;
	P_char victim;
	char buf[MAX_INPUT_LENGTH];
	int target, old_room;
	int level = GET_LEVEL(ch);

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{
		send_to_char("Who do you wish to transfer?\n", ch);
		return;
	}
	if (str_cmp("all", buf))
	{
		if (!(victim = get_char_vis(ch, buf)))
		{
			send_to_char("No-one by that name around.\n", ch);
			return;
		}
		if ((GET_LEVEL(victim) > level) && IS_TRUSTED(victim))
		{
			send_to_char("You cannot transfer someone higher level than you\n", ch);
			return;
		}
		if (!can_enter_room(victim, ch->in_room, FALSE) && (GET_LEVEL(ch) < 59))
		{
			send_to_char("That person can't come here.\n", ch);
			return;
		}
		act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
		target = ch->in_room;
		old_room = victim->in_room;
		act("$n demands your presence NOW!", FALSE, ch, 0, victim, TO_VICT);
		char_from_room(victim);
		room_light(old_room, REAL);
		char_to_room(victim, target, -1);
		char_light(victim);
		room_light(victim->in_room, REAL);
		act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
		send_to_char("Ok.\n", ch);

		logit(LOG_WIZ, "%s transferred %s from %d to %d", ch->player.name,
		      victim->player.name, world[old_room].number, world[victim->in_room].number);
		sql_log(ch, WIZLOG, "Transferred %s from %d to %d", victim->player.name,
			world[old_room].number, world[victim->in_room].number);
	}
	else
	{ /*
	   * Trans All
	   */

		if (level < 58)
		{
			send_to_char("Sorry, 'trans all' is a level 58 command.\n", ch);
			return;
		}
		for (i = descriptor_list; i; i = i->next)
		{
			if (i->character != ch && !i->connected &&
			    (GET_LEVEL(i->character) <= level))
			{
				victim = i->character;
				act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0,
				    TO_ROOM);
				target = ch->in_room;
				old_room = victim->in_room;
				char_from_room(victim);
				room_light(old_room, REAL);
				act("$n demands your presence NOW!", FALSE, ch, 0, victim, TO_VICT);
				char_to_room(victim, target, -1);
				char_light(victim);
				room_light(victim->in_room, REAL);
				act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0,
				    TO_ROOM);
			}
		}

		send_to_char("Ok.\n", ch);

		logit(LOG_WIZ, "%s transferred all to %d", ch->player.name,
		      world[ch->in_room].number);
		sql_log(ch, WIZLOG, "Transferred all to %d", world[ch->in_room].number);
	}
}

void do_at(P_char ch, char *argument, int /*cmd*/)
{
	char loc_str[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
	int loc_nr, location, original_loc, zc, original_zc;
	P_char target_mob, next;
	P_obj target_obj;

	if (IS_NPC(ch))
		return;

	half_chop(argument, loc_str, buf);
	if (!*loc_str)
	{
		send_to_char("You must supply a room number or a name.\n", ch);
		return;
	}
	if (is_number(loc_str))
	{
		loc_nr = atoi(loc_str);
		location = real_room(loc_nr);
		if (location == NOWHERE)
		{
			send_to_char("No room exists with that number.\n", ch);
			return;
		}
		else
		{
			zc = 0;
		}
	}
	else if ((target_mob = get_char_vis(ch, loc_str)))
	{
		if (target_mob->in_room != NOWHERE)
		{
			location = target_mob->in_room;
			zc = target_mob->specials.z_cord;
		}
		else
		{
			logit(LOG_DEBUG, "%s in NOWHERE!!",
			      (IS_PC(target_mob) ? GET_NAME(target_mob) :
						   target_mob->player.short_descr));
			send_to_char("That person is in NOWHERE!!", ch);
			return;
		}
	}
	else if ((target_obj = get_obj_vis(ch, loc_str)))
	{
		if (OBJ_ROOM(target_obj))
		{
			location = target_obj->loc.room;
			zc = target_obj->z_cord;
		}
		else
		{
			send_to_char("The object is not available.\n", ch);
			return;
		}
	}
	else
	{
		send_to_char("No such creature or object around.\n", ch);
		return;
	}

	if (IS_ROOM(location, ROOM_PRIVATE) && (GET_LEVEL(ch) < OVERLORD))
	{
		send_to_char("It is far too private there.\n", ch);
		return;
	}

	original_zc = ch->specials.z_cord;
	ch->specials.z_cord = zc;

	if (!can_enter_room(ch, location, TRUE))
	{
		ch->specials.z_cord = original_zc;
		return;
	}

	/*
	 * a location has been found.
	 */

	original_loc = ch->in_room;
	char_from_room(ch);
	char_to_room(ch, location, -2); /*
	                                 * avoid triggering aggros
	                                 */
	command_interpreter(ch, buf);

	/*
	 * check if the guy's still there
	 */
	for (target_mob = world[location].people; target_mob; target_mob = next)
	{
		next = target_mob->next_in_room;

		if (ch == target_mob)
		{
			char_from_room(ch);
			ch->specials.z_cord = original_zc;
			char_to_room(ch, original_loc, -2);
			return;
		}
	}
}

void do_goto(P_char ch, char *argument, int /*cmd*/)
{
	char buf[MAX_STRING_LENGTH], output[MAX_STRING_LENGTH];
	int location = NOWHERE, old_room, bits, zcoord = 0;
	size_t i;
	P_char target_mob = NULL;
	P_obj target_obj = NULL;

	if (IS_NPC(ch))
	{
		send_to_char("MOBs don't have such power.\n\r", ch);
		return;
	}

	one_argument(argument, buf);
	if (!*buf)
	{
		send_to_char("You must supply a room number or a name.\n", ch);
		return;
	}
	if (is_number(buf))
	{
		location = real_room(atoi(buf));
		if ((location == NOWHERE) || (location > top_of_world))
		{
			send_to_char("No room exists with that number.\n", ch);
			return;
		}
	}
	else
	{
		bits = generic_find(buf, (FIND_CHAR_WORLD | FIND_OBJ_WORLD | FIND_IGNORE_ZCOORD),
				    ch, &target_mob, &target_obj);
		if (!bits)
		{
			send_to_char("Nothing by that name.\n", ch);
			return;
		}
		if (target_mob)
		{
			location = target_mob->in_room;
			zcoord = target_mob->specials.z_cord;
		}
		else
		{
			while (location == NOWHERE)
			{
				if (OBJ_ROOM(target_obj))
				{
					location = target_obj->loc.room;
					zcoord = target_obj->z_cord;
				}
				else if (OBJ_CARRIED(target_obj))
				{
					location = target_obj->loc.carrying->in_room;
					zcoord = target_obj->loc.carrying->specials.z_cord;
				}
				else if (OBJ_WORN(target_obj))
				{
					location = target_obj->loc.wearing->in_room;
					zcoord = target_obj->loc.wearing->specials.z_cord;
				}
				else if (OBJ_INSIDE(target_obj))
				{
					target_obj = target_obj->loc.inside;
				}
				else
				{
					send_to_char(
						"&+RThat object is BUGGED! can't find a location for it.&n\n",
						ch);
					return;
				}
			}
		}
	}

	if ((location == NOWHERE) || (location > top_of_world))
	{
		send_to_char("No such creature or object around.\n", ch);
		return;
	}
	/* a location has been found.  */
	if (IS_ROOM(location, ROOM_PRIVATE) && (GET_LEVEL(ch) < MAXLVL))
	{
		if (GET_LEVEL(ch) < MAXLVL)
		{
			send_to_char("That room is private.\n", ch);
			return;
		}
	}
	if (!can_enter_room(ch, location, TRUE))
	{
		send_to_char("You try, but you can't seem to get there.\n\r", ch);
		return;
	}
	if (ch->only.pc->poofOut == NULL)
	{
		strcpy(output, "$n disappears in a puff of smoke.");
	}
	else
	{
		if (!strstr(ch->only.pc->poofOut, "%n"))
		{
			strcpy(output, "$n ");
			strcat(output, ch->only.pc->poofOut);
		}
		else
		{
			strcpy(output, ch->only.pc->poofOut);

			for (i = 0; i < strlen(output); i++)
				if ((*(output + i) == '%') && (*(output + i + 1) == 'n'))
					*(output + i) = '$';
		}
	}

	act(output, TRUE, ch, 0, 0, TO_ROOM);

	old_room = ch->in_room;
	char_from_room(ch);
	room_light(old_room, REAL);
	ch->specials.z_cord = zcoord;

	if (!char_to_room(ch, location, -1))
	{
		// If ch didn't make it to the new location, and is still alive, try to put them back in the old room.
		if (IS_ALIVE(ch))
		{
			send_to_char(
				"You didn't make it there, trying to put you back where you started...\n\r",
				ch);
			if (!char_to_room(ch, old_room, -1))
			{
				return;
			}
		}
	}

	if (ch->only.pc->poofIn == NULL)
	{
		strcpy(output, "$n appears with an ear-splitting bang.");
	}
	else
	{
		if (!strstr(ch->only.pc->poofIn, "%n"))
		{
			strcpy(output, "$n ");
			strcat(output, ch->only.pc->poofIn);
		}
		else
		{
			strcpy(output, ch->only.pc->poofIn);

			for (i = 0; i < strlen(output); i++)
				if ((*(output + i) == '%') && (*(output + i + 1) == 'n'))
					*(output + i) = '$';
		}
	}

	act(output, TRUE, ch, 0, 0, TO_ROOM);
}

void do_poofIn(P_char ch, char *argument, int /*cmd*/)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
	size_t i;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		if (ch->only.pc->poofIn != NULL)
			str_free(ch->only.pc->poofIn);
		ch->only.pc->poofIn = NULL;
	}
	else if (!str_cmp("?", arg))
	{
		if (ch->only.pc->poofIn == NULL)
		{
			strcpy(buf, "$n appears with an ear-splitting bang.");
		}
		else
		{
			if (!strstr(ch->only.pc->poofIn, "%n"))
			{
				strcpy(buf, "$n ");
				strcat(buf, ch->only.pc->poofIn);
			}
			else
			{
				strcpy(buf, ch->only.pc->poofIn);

				/* bleah, code doubles $ to prevent entering 'act' strings */
				for (i = 0; i < strlen(buf); i++)
					if ((*(buf + i) == '%') && (*(buf + i + 1) == 'n'))
						*(buf + i) = '$';
			}
		}
		act(buf, TRUE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		if (ch->only.pc->poofIn != NULL)
			str_free(ch->only.pc->poofIn);

		if (*argument == ' ')
			argument++;
		ch->only.pc->poofIn = str_dup(argument);
	}
}

void do_poofOut(P_char ch, char *argument, int /*cmd*/)
{
	char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
	size_t i;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, arg);

	if (!*arg)
	{
		if (ch->only.pc->poofOut != NULL)
			str_free(ch->only.pc->poofOut);

		ch->only.pc->poofOut = NULL;
	}
	else if (!str_cmp("?", arg))
	{
		if (ch->only.pc->poofOut == NULL)
		{
			strcpy(buf, "$n disappears in a puff of smoke.");
		}
		else
		{
			if (!strstr(ch->only.pc->poofOut, "%n"))
			{
				strcpy(buf, "$n ");
				strcat(buf, ch->only.pc->poofOut);
			}
			else
			{
				strcpy(buf, ch->only.pc->poofOut);
				/* bleah, code doubles $ to prevent entering 'act' strings */
				for (i = 0; i < strlen(buf); i++)
					if ((*(buf + i) == '%') && (*(buf + i + 1) == 'n'))
						*(buf + i) = '$';
			}
		}
		act(buf, TRUE, ch, 0, 0, TO_CHAR);
	}
	else
	{
		if (ch->only.pc->poofOut != NULL)
			str_free(ch->only.pc->poofOut);

		if (*argument == ' ')
			argument++;
		ch->only.pc->poofOut = str_dup(argument);
	}
}

void do_teleport(P_char ch, char *argument, int /*cmd*/)
{
	P_char victim, target_mob;
	char person[MAX_INPUT_LENGTH], room[MAX_INPUT_LENGTH];

	int target, old_room;
	int loop;

	if (!IS_TRUSTED(ch))
		return;

	half_chop(argument, person, room);

	if (!*person)
	{
		send_to_char("Who do you wish to teleport?\n", ch);
		return;
	}
	if (!*room)
	{
		send_to_char("Where do you wish to send this person?\n", ch);
		return;
	}
	if (!(victim = get_char_vis(ch, person)))
	{
		send_to_char("No-one by that name around.\n", ch);
		return;
	}
	if ((GET_LEVEL(victim) > MIN(62, GET_LEVEL(ch))) &&
	    (!IS_NPC(victim) || (GET_LEVEL(ch) < 60)))
	{
		send_to_char("You can't do that!\n", ch);
		return;
	}
	if (isdigit(*room))
	{
		target = atoi(&room[0]);
		for (loop = 0; loop <= top_of_world; loop++)
		{
			if (world[loop].number == target)
			{
				target = loop;
				break;
			}
			else if (loop == top_of_world)
			{
				send_to_char("No room exists with that number.\n", ch);
				return;
			}
		}
	}
	else if (!strcmp(room, "home"))
	{
		target = real_room(GET_HOME(victim));
	}
	else if (!strcmp(room, "return"))
	{
		target = real_room(victim->specials.was_in_room);
		if (target == NOWHERE)
		{
			send_to_char("Return them to where?  The main menu?\n", ch);
			return;
		}
	}
	else if ((target_mob = get_char_vis(ch, room)))
	{
		target = target_mob->in_room;
	}
	else
	{
		send_to_char("No such target (person) can be found.\n", ch);
		return;
	}
	if (IS_ROOM(target, ROOM_PRIVATE) && (GET_LEVEL(ch) < MAXLVL))
	{
		send_to_char("That room is private.\n", ch);
		return;
	}
	if (!can_enter_room(victim, target, FALSE) && GET_LEVEL(ch) < 59)
	{
		send_to_char("That person can't go there.\n", ch);
		return;
	}
	act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	old_room = victim->in_room;
	act("$n has teleported you!", FALSE, ch, 0, (char *)victim, TO_VICT);
	char_from_room(victim);
	room_light(old_room, REAL);
	char_to_room(victim, target, -1);
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	send_to_char("Teleport completed.\n", ch);

	logit(LOG_WIZ, "%s teleported %s from %d to %d", ch->player.name, victim->player.name,
	      world[old_room].number, world[victim->in_room].number);
	sql_log(ch, WIZLOG, "Teleported %s from %d to %d", victim->player.name,
		world[old_room].number, world[victim->in_room].number);
}

void do_knock(P_char ch, char *arg, int /*cmd*/)
{
	P_char victim;

	if (!ch || !arg || IS_NPC(ch) || !IS_TRUSTED(ch))
		return;

	if (!*arg)
	{
		send_to_char("At whose door do you wish to knock?\n", ch);
		return;
	}
	if (!(victim = get_char_vis(ch, arg)) || IS_NPC(victim))
	{
		send_to_char("Sorry, no one around that fits that description.\n", ch);
		return;
	}
	if (ch == victim)
	{
		send_to_char("You don't have to ask yourself to come in, silly!\n", ch);
		return;
	}
	if (!IS_TRUSTED(victim))
	{
		act("$N's not a god, why knock?", FALSE, ch, 0, victim, TO_CHAR);
		return;
	}
	act("You knock at $N's door...", FALSE, ch, 0, victim, TO_CHAR);
	act("&+R*KNOCK KNOCK*&n  $n is knocking at your door.  Can $e come in?", FALSE, ch, 0,
	    victim, TO_VICT);

	return;
}

/*
 * Guild-God command.
 *
 * This command acts as a filter, calling do_setbit() after it rebuilds
 * the command to set the hometown.
 */

#define SYNTAX_SETHOME "Syntax:\n   sethome <char> <home flags> <room>\n"

void do_sethome(P_char ch, char *args, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH], hflg[MAX_INPUT_LENGTH];
	char val[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	static char homeflags[3][16] = { "hometown", "birthplace", "\n" };
	static char setbit_flags[2][16] = { "home", "orighome" };
	char *p;
	int i, hflg_index = -1;

	args = one_argument(args, name);
	args = one_argument(args, hflg);
	(void)one_argument(args, val);

	if (!name[0])
	{
		send_to_char(SYNTAX_SETHOME, ch);
		return;
	}
	if (hflg[0])
	{
		for (p = hflg; *p; p++)
			*p = tolower(*p);

		for (i = 0; homeflags[i][0] != '\n'; i++)
		{
			if (is_abbrev(hflg, homeflags[i]))
			{
				hflg_index = i;
				break;
			}
		}
	}
	if (!hflg[0] || hflg_index == -1)
	{
		send_to_char(SYNTAX_SETHOME, ch);
		send_to_char("\nValid home flags are:\n", ch);
		for (i = 0; homeflags[i][0] != '\n'; i++)
		{
			snprintf(buf, MAX_STRING_LENGTH, "\t%s\n", homeflags[i]);
			send_to_char(buf, ch);
		}
		return;
	}
	/*
	 * here we have at least 2 arguments, the 2nd being a valid 'home
	 * flag'
	 */

	snprintf(buf, MAX_STRING_LENGTH, "char %s %s %s", name, setbit_flags[hflg_index], val);
	do_setbit(ch, buf, CMD_SETHOME);
	// a hack :)
	if (1 == hflg_index)
	{
		// set the origbp too
		snprintf(buf, MAX_STRING_LENGTH, "char %s origbp %s", name, val);
		do_setbit(ch, buf, CMD_SETHOME);
	}
}

void revert_sethome(P_char member)
{
	char buf[MAX_STRING_LENGTH];

	// Revert their home to their original birthplace.
	snprintf(buf, MAX_STRING_LENGTH, "char %s home %d", J_NAME(member),
		 GET_ORIG_BIRTHPLACE(member));
	do_setbit(member, buf, CMD_SETHOME);
	snprintf(buf, MAX_STRING_LENGTH, "char %s orighome %d", J_NAME(member),
		 GET_ORIG_BIRTHPLACE(member));
	do_setbit(member, buf, CMD_SETHOME);
}
