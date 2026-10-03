/*
 * Staff broadcast and announcement commands.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "net/comm.h"
#include "net/gmcp.h"
#include "net/listen.h"
#include "magic/spells.h"
#include "world/db.h"
#include "sql/sql.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern P_desc descriptor_list;

void do_emote(P_char ch, char *argument, int /*cmd*/)
{
	int i;
	P_char k;
	static char buf[MAX_STRING_LENGTH];

	/*
	   if(IS_SET(ch->specials.act, PLR_NOEMOTE) && !IS_NPC(ch)) {
	   send_to_char("You have NoEmote on.\n", ch);
	   return;
	   }
	 */

	if (!IS_ALIVE(ch))
	{
		if (ch)
			send_to_char("Lay still, you seem to be dead.\r\n", ch);
		return;
	}

	if (IS_IMMOBILE(ch))
	{
		act("In your present state just relax and make the best of it.", FALSE, ch, 0, 0,
		    TO_CHAR);
		return;
	}

	if (IS_MORPH(ch))
	{
		send_to_char("So much for that idea!\n", ch);
		return;
	}
	for (i = 0; *(argument + i) == ' '; i++)
		;

	if (IS_ROOM(ch->in_room, ROOM_UNDERWATER) && !IS_TRUSTED(ch) && !IS_NPC(GET_PLYR(ch)))
	{
		send_to_char("You cannot emote while swimming around in water...", ch);
		return;
	}
	if (IS_AFFECTED2(ch, AFF2_SILENCED))
		send_to_char("You seem unable to make your emotions known.\n", ch);
	else if (IS_AFFECTED(ch, AFF_WRAITHFORM))
		send_to_char("You cannot speak in this form.\n", ch);
	else if (is_silent(ch, FALSE))
		send_to_char("For some reason, that doesn't seem possible here.\n", ch);
	else if (!*(argument + i))
		send_to_char("Yes... But what?\n", ch);
	else
	{
		/*
		 * snprintf(buf, MAX_STRING_LENGTH, "$n %s", argument + i);
		 */
		for (k = world[ch->in_room].people; k; k = k->next_in_room)
			if (IS_AWAKE(k))
			{
				snprintf(buf, MAX_STRING_LENGTH, "$n %s",
					 language_CRYPT(ch, k, argument + i));
				act(buf, FALSE, ch, 0, k, TO_VICT | ACT_SILENCEABLE);
			}
		listen_broadcast(ch, (const char *)buf, LISTEN_EMOTE);

		if (IS_SET(ch->specials.act, PLR_ECHO) || IS_NPC(GET_PLYR(ch)))
		{
			snprintf(buf, MAX_STRING_LENGTH, "$N %s", argument + i);
			act(buf, FALSE, ch, 0, ch, TO_CHAR);
		}
		else
			send_to_char("Ok.\n", ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s emotes '%s'", GET_NAME(ch), argument + i);
	}
}

void do_echo(P_char ch, char *argument, int /*cmd*/)
{
	P_desc d;
	int i;
	static char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	for (i = 0; *(argument + i) == ' '; i++)
		;

	if (!*(argument + i))
		send_to_char("That must be a mistake...\n", ch);
	else
	{
		snprintf(buf, MAX_STRING_LENGTH, "%s\n", argument + i);
		send_to_room(buf, ch->in_room);

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING && ch->in_room == d->character->in_room)
			{
				write_to_pc_log(d->character, buf, LOG_PRIVATE);
			}
		}
		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echo's '%s'", GET_NAME(ch), argument + i);
	}
}

void do_echoa(P_char ch, char *argument, int /*cmd*/)
{
	P_desc d;
	char Gbuf1[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	while (*argument == ' ' && *argument != '\0')
		argument++;

	if (!*argument)
		send_to_char("Yes, fine, we must echoa something, but what!?\n", ch);
	else
	{
		level = GET_LEVEL(ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echoa's '%s'", GET_NAME(ch), argument);

		strcat(argument, "\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				if (GET_LEVEL(d->character) >= level)
				{
					snprintf(Gbuf1, MAX_STRING_LENGTH, "A[%s]", GET_NAME(ch));
					send_to_char(Gbuf1, d->character);
				}
				send_to_char(argument, d->character);

				write_to_pc_log(d->character, argument, LOG_PRIVATE);
			}
		}
	}
}

void do_echoz(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	char Gbuf1[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	while (*arg == ' ' && *arg != '\0')
		arg++;

	if (!*arg)
	{
		send_to_char("Yes, fine, we must echoz something, but what?!\n", ch);
		return;
	}
	else
	{
		level = GET_LEVEL(ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echoz's '%s'", GET_NAME(ch), arg);

		strcat(arg, "\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				if (world[ch->in_room].zone == world[d->character->in_room].zone)
				{
					if (GET_LEVEL(d->character) >= level)
					{
						snprintf(Gbuf1, MAX_STRING_LENGTH, "Z[%s]",
							 GET_NAME(ch));
						send_to_char(Gbuf1, d->character);
					}
					send_to_char(arg, d->character);
					write_to_pc_log(d->character, arg, LOG_PRIVATE);
				}
			}
		}
	}
	return;
}

void do_echog(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	char Gbuf1[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	while (*arg == ' ' && *arg != '\0')
		arg++;

	if (!*arg)
	{
		send_to_char("Yes, fine, we must inform the goods of something, but what?!\n", ch);
		return;
	}
	else
	{
		level = GET_LEVEL(ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echog's '%s'", GET_NAME(ch), arg);

		strcat(arg, "\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				if (IS_RACEWAR_GOOD(d->character) || IS_TRUSTED(d->character))
				{
					if (GET_LEVEL(d->character) >= level)
					{
						snprintf(Gbuf1, MAX_STRING_LENGTH, "G[%s]",
							 GET_NAME(ch));
						send_to_char(Gbuf1, d->character);
					}
					send_to_char(arg, d->character);
					write_to_pc_log(d->character, arg, LOG_PRIVATE);
				}
			}
		}
	}
	return;
}

void do_echoe(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	char Gbuf1[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	while (*arg == ' ' && *arg != '\0')
		arg++;

	if (!*arg)
	{
		send_to_char("Yes, fine, we must inform the evils of something, but what?!\n", ch);
		return;
	}
	else
	{
		level = GET_LEVEL(ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echoe's '%s'", GET_NAME(ch), arg);

		strcat(arg, "\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				if ((EVIL_RACE(d->character) || IS_TRUSTED(d->character)) &&
				    (!(IS_RACEWAR_UNDEAD(d->character))))
				{
					if (GET_LEVEL(d->character) >= level)
					{
						snprintf(Gbuf1, MAX_STRING_LENGTH, "E[%s]",
							 GET_NAME(ch));
						send_to_char(Gbuf1, d->character);
					}
					send_to_char(arg, d->character);
					write_to_pc_log(d->character, arg, LOG_PRIVATE);
				}
			}
		}
	}
	return;
}

void do_echou(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	char Gbuf1[MAX_STRING_LENGTH];
	int level;

	if (IS_NPC(ch))
		return;

	while (*arg == ' ' && *arg != '\0')
		arg++;

	if (!*arg)
	{
		send_to_char("Yes, fine, we must inform the undead of something, but what?!\n", ch);
		return;
	}
	else
	{
		level = GET_LEVEL(ch);

		if (get_property("logs.chat.status", 0.000) && IS_PC(ch))
			logit(LOG_CHAT, "%s echou's '%s'", GET_NAME(ch), arg);

		strcat(arg, "\n");

		for (d = descriptor_list; d; d = d->next)
		{
			if (d->connected == CON_PLAYING)
			{
				if (IS_RACEWAR_UNDEAD(d->character) || IS_TRUSTED(d->character))
				{
					if (GET_LEVEL(d->character) >= level)
					{
						snprintf(Gbuf1, MAX_STRING_LENGTH, "U[%s]",
							 GET_NAME(ch));
						send_to_char(Gbuf1, d->character);
					}
					send_to_char(arg, d->character);
					write_to_pc_log(d->character, arg, LOG_PRIVATE);
				}
			}
		}
	}
	return;
}

void do_wizmsg(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	P_char realChar, toChar;
	char Gbuf1[MAX_STRING_LENGTH], color[4];
	char Gbuf2[MAX_STRING_LENGTH], Gbuf3[MAX_STRING_LENGTH];
	char *send_string;
	int min_level = 0;

	// Changed this to ch->desc from IS_NPC(ch) so that switched Imms still can wizchat.
	if (!IS_ALIVE(ch) || !ch->desc || !IS_TRUSTED(ch))
	{
		return;
	}

	if (ch->desc && ch->desc->original)
	{
		realChar = ch->desc->original;
	}
	else
	{
		realChar = ch;
	}

	if (IS_NPC(realChar))
	{
		debug("Please tell Lohrr, \"There's a NPC with a descriptor and no original.\"  He doesn't think it's possible");
		logit(LOG_STATUS,
		      "Please tell Lohrr, \"There's a NPC with a descriptor and no original.\"");
	}

	if (IS_SET(realChar->specials.act, PLR_WIZMUFFED))
	{
		send_to_char("You have the &+Wwiz&n channel toggled &+WOFF&n.\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("Yes, yes, but WHAT do you want to tell them all?\n", ch);
		return;
	}
	half_chop(arg, Gbuf1, Gbuf2);
	min_level = atoi(Gbuf1);

	// If we have a valid level proceeded by a message.
	if (is_number(Gbuf1) && *Gbuf2 && min_level >= MINLVLIMMORTAL && min_level <= MAXLVL)
	{
		send_string = Gbuf2;
	}
	else
	{
		min_level = MINLVLIMMORTAL;
		send_string = arg;
	}

	if (min_level > AVATAR)
	{
		snprintf(color, 4, "&+R");
	}
	else
	{
		snprintf(color, 4, "&+r");
	}

	// If God is visible - Show real chars name, not the switched...
	checked_snprintf(Gbuf1, MAX_STRING_LENGTH, "%s : (%s%d&n) [ %s &n]\n\r", GET_NAME(realChar),
			 color, min_level, send_string);

	// If God is invisible
	checked_snprintf(Gbuf3, MAX_STRING_LENGTH, "Someone : (%s%d&n) [ %s &n]\n\r", color,
			 min_level, send_string);

	for (d = descriptor_list; d; d = d->next)
	{
		toChar = (d->original) ? d->original : d->character;
		// For descriptors in game and of appropriate level and listening to wiz channel.
		if ((d->connected == CON_PLAYING) && (GET_LEVEL(toChar) >= min_level) &&
		    !PLR_FLAGGED(toChar, PLR_WIZMUFFED))
		{
			// Check to make sure wizinvis Gods stay anonymous.
			if (CAN_SEE(toChar, realChar))
			{
				send_to_char(Gbuf1, toChar, LOG_PRIVATE);
				/* Send to web client via GMCP */
				gmcp_comm_channel(toChar, "wizmsg", GET_NAME(realChar),
						  send_string);
			}
			else
			{
				send_to_char(Gbuf3, toChar, LOG_PRIVATE);
				/* Send to web client via GMCP (anonymous) */
				gmcp_comm_channel(toChar, "wizmsg", "Someone", send_string);
			}
		}
	}
	if (get_property("logs.chat.status", 0.000))
	{
		logit(LOG_CHAT, "%s wizmsg's '%s'", GET_NAME(realChar), Gbuf1);
	}
}

void do_echot(P_char ch, char *argument, int /*cmd*/)
{
	P_char vict;
	P_desc d;
	char name[MAX_INPUT_LENGTH], message[MAX_STRING_LENGTH];
	char Gbuf1[MAX_STRING_LENGTH];

	half_chop(argument, name, message);

	if (!*name || !*message)
	{
		send_to_char("Who do you wish to echot to??\n", ch);
		return;
	}

	vict = NULL;
	/*
	 * switching to descriptor list, rather than get_char_vis, since it
	 * was lagging hell out of things. JAB
	 */
	for (d = descriptor_list; d; d = d->next)
	{
		if (!d->character || d->connected || !d->character->player.name)
			continue;
		if (!isname(d->character->player.name, name))
			continue;
		vict = d->character;
		break;
	}
	if (!vict)
	{
		send_to_char("No-one by that name here...\n", ch);
		return;
	}
	else if (ch == vict)
	{
		send_to_char("You try to echot yourself something.\n", ch);
		return;
	}
	else if (!vict->desc)
	{
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	if (ch->desc)
	{
		if (IS_SET(ch->specials.act, PLR_ECHO))
		{
			checked_snprintf(Gbuf1, MAX_STRING_LENGTH, "&+WYou echot %s '&n%s&+W'.&n\n",
					 GET_NAME(vict), message);
			send_to_char(Gbuf1, ch);
		}
		else
		{
			send_to_char("Ok.\n", ch);
		}
	}
	if (get_property("logs.chat.status", 0.000) && IS_PC(ch) && IS_PC(vict))
		logit(LOG_CHAT, "%s echot's to %s '%s'", GET_NAME(ch), GET_NAME(vict), message);
	strcat(message, "\n");
	send_to_char(message, vict);
	write_to_pc_log(vict, message, LOG_PRIVATE);
}

// Does pretty much what it sounds like: Sends a message to a character.
//   If that character is online, it sends them the message right away.
//   If that character is ld/offline, it adds the message to the offline_messages
//     table which, in turn, shows the message at next login.
void do_offlinemsg(P_char ch, char *arg, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH], *rest;
	char message[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int pid;
	P_desc d;
	P_char recipient;

	// Separate name from message.
	rest = one_argument(arg, name);
	rest = skip_spaces(rest);

	pid = get_player_pid_from_name(name);

	if (!*name || *name == '?')
	{
		snprintf(buf, MAX_STRING_LENGTH,
			 "&+YSyntax:&N offlinemsg <player's name> <message to send>\n\r");
		send_to_char(buf, ch);
		return;
	}

	if (pid == 0)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+YCould not find player '&+w%s&+Y'.&n\n\r",
			 name);
		send_to_char(buf, ch);
		return;
	}

	if (!rest || !*rest)
	{
		send_to_char("&+YYes, but what message do you want to send them?&n\n\r", ch);
		return;
	}

	// This needs a carriage return.. *sigh*
	snprintf(message, MAX_INPUT_LENGTH, "&+W%s&n: %s\n\r", GET_NAME(ch), rest);

	// Walk through the connected players, looking for the recipient online first.
	for (d = descriptor_list; d; d = d->next)
	{
		// Need an in-game descriptor w/a character attached.
		if (!d->character || d->connected)
		{
			continue;
		}

		// Handles switched gods / morphed players.
		recipient = (d->original) ? d->original : d->character;

		// This should never be the case, but ...
		if (IS_NPC(recipient))
		{
			debug("do_offlinemsg: NPC char '%s' %d on descriptor list??",
			      GET_NAME(recipient),
			      (recipient->only.npc != NULL) ? GET_VNUM(recipient) : -1);
			continue;
		}

		// If we have the right pid
		if (GET_PID(recipient) == pid)
		{
			if (CAN_SEE_Z_CORD(ch, recipient))
			{
				snprintf(
					buf, MAX_STRING_LENGTH,
					"&+YSending &=LWin-game&n&+Y message '&n%s&n&+Y' to '&+w%s&+Y' (pid: &+w%d&+Y).&n\n\r",
					rest, name, pid);
				send_to_char(buf, ch);
				snprintf(buf, MAX_STRING_LENGTH,
					 "&+Y%s&+Y sends you an in-game message '&n%s&n&+W'&N\r\n",
					 CAN_SEE(recipient, ch) ? J_NAME(ch) : "Someone", rest);
				SEND_TO_Q(buf, d);
				return;
			}
			else
			{
				break;
			}
		}
	}
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+YSending offline message '&n%s&+Y' to '&+w%s&n&+Y' (pid: &+w%d&+Y).&n\n\r",
		 rest, name, pid);
	send_to_char(buf, ch);
	send_to_pid_offline(message, pid);
}
