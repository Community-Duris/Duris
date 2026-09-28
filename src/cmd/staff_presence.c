/* Staff commands for player visibility and presence. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/utils.h"
#include "net/comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern P_desc descriptor_list;
extern P_room world;

void do_ingame(P_char ch, char *args, int /*cmd*/)
{
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
	char name[MAX_NAME_LENGTH + 1];
	P_desc desc;
	char *pIn, *pOut;
	int count, i;
	bool percent;

	count = 0;
	percent = FALSE;
	pIn = args;
	pOut = buf1;
	while (*pIn != '\0')
	{
		// Leading %.
		if (*pIn == '%')
		{
			percent = TRUE;
			*(pOut++) = '%';
			pIn++;
		}
		// %p -> substitute.
		else if (percent && (*pIn == 'p'))
		{
			percent = FALSE;
			// We have the first % from initial if.
			// For each additional %p, we need to add %'s to total 2^count.
			for (i = (1 << (count++)) - 1; i > 0; i--)
			{
				*(pOut++) = '%';
			}
			*(pOut++) = 's';
			pIn++;
		}
		// Regular character.
		else
		{
			percent = FALSE;
			*(pOut++) = *(pIn++);
		}
	}
	*pOut = '\0';

	if (count == 0)
	{
		send_to_char("Bleah. Try ingame <string>, and include a %p somewhere.\n", ch);
		return;
	}

	for (desc = descriptor_list; desc; desc = desc->next)
	{
		// Not self, in game, visible and lower lvl.
		if ((desc->character != ch) && (desc->connected == CON_PLAYING) &&
		    CAN_SEE(ch, desc->character) && (GET_LEVEL(desc->character) < GET_LEVEL(ch)))
		{
			snprintf(name, sizeof name, "%s", GET_TRUE_NAME(desc->character));
			// We know there's at least one %s in the string that needs substituting.
			checked_snprintf_runtime(buf2, MAX_STRING_LENGTH, buf1, name);
			i = 1;
			// Substitute the rest if there are any.
			while (i++ < count)
			{
				// Swap back and forth between buffers.
				if ((i % 2) == 0)
					checked_snprintf_runtime(buf3, MAX_STRING_LENGTH, buf2,
								 name);
				else
					checked_snprintf_runtime(buf2, MAX_STRING_LENGTH, buf3,
								 name);
			}
			// count % 2 tells us which buffer we have the final string.
			if ((count % 2) == 0)
			{
				command_interpreter(ch, buf3);
			}
			else
			{
				command_interpreter(ch, buf2);
			}
		}
	}
}

void do_inroom(P_char ch, char *args, int /*cmd*/)
{
	char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	char *p;
	P_char v, v_next;
	int target;

	strcpy(buf1, args); /* don't wanna change args directly */

	if ((p = strstr(buf1, " %p")))
		target = 1;
	else if ((p = strstr(buf1, " %m")))
		target = 2;
	else if ((p = strstr(buf1, " %a")))
		target = 3;
	else
	{
		/* error.. no token provided  */
		send_to_char("Eh?  try: \"inroom <string>\"\n", ch);
		send_to_char("  <string> should contain 1 occurance of ONE of the following:\n",
			     ch);
		send_to_char("    %p - all players in the room\n", ch);
		send_to_char("    %m - all mobs in the room\n", ch);
		send_to_char("    %a - both mobs and players in the room\n", ch);
		send_to_char("  <string> will then be executed once for each pc and/or npc\n", ch);
		send_to_char("  in the room, replacing the % token with the char name\n", ch);
		return;
	}

	/* okay... now we have p pointing to the space before the % token.
	   move it forward two places, and replace the letter with an 's' for
	   use in sprintf  */

	p++;
	p++;
	*p = 's';

	/* okay.. now buf1 is setup as an arguement for sprintf...   */

	for (v = world[ch->in_room].people; v; v = v_next)
	{
		v_next = v->next_in_room;
		if ((!CAN_SEE(ch, v)) || (IS_PC(v) && (target == 2)) ||
		    (IS_NPC(v) && (target == 1)) || (ch == v))
			continue;

		/*
		 * Serious flaw in this (and MANY other functions): if the users
		 * does "inroom grin %pc %s" that extra %s is going to cause
		 * problems... For now, I'm going to just "hope" that people
		 * aren't that stupid (considering thats what the code does
		 * everywhere else)
		 */

		checked_snprintf_runtime(buf2, MAX_STRING_LENGTH, buf1, FirstWord(GET_NAME(v)));

		/* okay.. now just dump buf2 to the command interpretter  */

		command_interpreter(ch, buf2);
	}
}

/* Make oneself visible only to players above certain levels. */
void do_vis(P_char ch, char *argument, int /*cmd*/)
{
	char buf[MAX_STRING_LENGTH];

	if (IS_NPC(ch))
		return;

	one_argument(argument, buf);

	if (!*buf)
	{ /* Inquire visibility */
		if (IS_TRUSTED(ch))
			snprintf(buf, MAX_STRING_LENGTH,
				 "You are currently visible only to players with level > %d.\n",
				 ch->only.pc->wiz_invis);
		if (IS_AFFECTED(ch, AFF_INVISIBLE) || IS_AFFECTED2(ch, AFF2_CONCEALMENT))
			if (strlen(buf) > 0)
				strcat(buf, "You are also affected by invisibility.\n");
			else
				strcpy(buf, "You are affected by invisibility.\n");
		else if (strlen(buf) == 0)
			strcpy(buf, "You are not invisible.\n");
		send_to_char(buf, ch);
	}
	else
	{
		/** Set new visibility  */
		int min_level;

		if IS_AFFECTED (ch, AFF_WRAITHFORM)
		{
			BackToUsualForm(ch);
			return;
		}
		if (!IS_TRUSTED(ch))
		{
			appear(ch);
			return;
		}
		min_level = MAX(0, atoi(buf));
		if (min_level >= GET_LEVEL(ch))
		{
			send_to_char(
				"Sorry... but you cannot be invis to your peers or superiors.\n",
				ch);
			return;
		}
		snprintf(buf, MAX_STRING_LENGTH,
			 "You are now visible only to PCs with level > %d.\n", min_level);

		ch->only.pc->wiz_invis = (ubyte)min_level;
		send_to_char(buf, ch);
	}
}
