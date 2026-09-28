/* Staff command grant and revoke handlers. */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/mm.h"
#include "net/comm.h"
#include "sql/sql.h"

#include <stdio.h>
#include <string.h>

extern const char *command[];
extern struct command_info cmd_info[];

void do_grant(P_char ch, char *args, int cmd)
{
	char victname[MAX_INPUT_LENGTH], cmdname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH],
		buf2[MAX_STRING_LENGTH];
	int *new_arr, i, s;
	P_char vict;

	args = one_argument(args, victname);
	one_argument(args, cmdname);

	if (!ch || (GET_LEVEL(ch) < FORGER))
	{
		send_to_char("Granted, you want to grant, but not here!\r\n", ch);
		return;
	}

	if (!victname[0])
	{
		send_to_char("Syntax: grant <char> <command name>\n", ch);
		return;
	}

	/* get target, check all sorts of shit */

	vict = get_char(victname);
	if (!vict)
	{
		send_to_char("Target not found.\n", ch);
		return;
	}

	if ((ch == vict) && cmdname[0])
	{
		send_to_char("Grant to yourself?  That's odd.\n", ch);
		return;
	}

	if (!IS_PC(vict))
	{
		send_to_char("Need a PC, my friend.\n", ch);
		return;
	}

	if (!IS_TRUSTED(vict))
	{
		send_to_char("That person does not appear to be godly enough.\n", ch);
		return;
	}

	/* if no command arg, list currently granted commands */

	if (!cmdname[0])
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+WCommands currently granted to %s:\n\n",
			 GET_NAME(vict));

		if (!vict->only.pc->gcmd_arr)
		{
			strcat(buf, "None!\n");
		}
		else
		{
			s = vict->only.pc->numb_gcmd;

			for (i = 0; i < s; i++)
			{
				snprintf(buf2, MAX_STRING_LENGTH, "[&+Y%d&n] &+c%-20s&n",
					 cmd_info[vict->only.pc->gcmd_arr[i]].minimum_level,
					 command[vict->only.pc->gcmd_arr[i] - 1]);

				strcat(buf, buf2);

				if (!((i + 1) % 3))
					strcat(buf, "\n");
			}

			strcat(buf, "\n");
		}

		page_string(ch->desc, buf, 1);

		return;
	}

	/* let's get the command */

	cmd = old_search_block(cmdname, 0, strlen(cmdname), command, 2);

	if (cmd <= 0)
	{
		send_to_char("Sorry, that command does not seem to exist.\n", ch);
		return;
	}

	if (!cmd_info[cmd].grantable)
	{
		send_to_char("Sorry, but that command is not grantable.\n", ch);
		return;
	}

	if (cmd_info[cmd].minimum_level > GET_LEVEL(ch))
	{
		send_to_char("You are not worthy of that command yourself!\n", ch);
		return;
	}

	/* okay let's go zany.  ZANY! */

	vict->only.pc->numb_gcmd++;
	s = vict->only.pc->numb_gcmd;

	CREATE(new_arr, int, s, MEM_TAG_ARRAY);

	for (i = 0; i < (s - 1); i++)
		new_arr[i] = vict->only.pc->gcmd_arr[i];

	new_arr[i] = cmd;

	if (vict->only.pc->gcmd_arr)
		FREE(vict->only.pc->gcmd_arr);

	vict->only.pc->gcmd_arr = new_arr;

	snprintf(buf, MAX_STRING_LENGTH, "You have granted the command '%s' (level %d) to %s.\n",
		 command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));

	send_to_char(buf, ch);

	snprintf(buf, MAX_STRING_LENGTH, "%s has granted you the use of the '%s' command.\n",
		 GET_NAME(ch), command[cmd - 1]);

	send_to_char(buf, vict);

	logit(LOG_WIZ, "%s granted %s the '%s' (%d) command.", GET_NAME(ch), GET_NAME(vict),
	      command[cmd - 1], cmd_info[cmd].minimum_level);
	wizlog(GET_LEVEL(ch), "%s granted %s the '%s' (%d) command.", GET_NAME(ch), GET_NAME(vict),
	       command[cmd - 1], cmd_info[cmd].minimum_level);
	sql_log(ch, WIZLOG, "Granted %s the '%s' (%d) command.", GET_NAME(vict), command[cmd - 1],
		cmd_info[cmd].minimum_level);
}

void do_revoke(P_char ch, char *args, int cmd)
{
	char victname[MAX_INPUT_LENGTH], cmdname[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	int *new_arr, i, j, s;
	P_char vict;

	args = one_argument(args, victname);
	one_argument(args, cmdname);

	if (!victname[0])
	{
		send_to_char("Syntax: revoke <char> <command name>\n", ch);
		return;
	}

	/* get target, check all sorts of shit */

	vict = get_char(victname);
	if (!vict)
	{
		send_to_char("Target not found.\n", ch);
		return;
	}

	if ((ch == vict) && cmdname[0])
		send_to_char("Revoke from yourself?  Well, if you say so..\n", ch);

	if (!IS_PC(vict))
	{
		send_to_char("Need a PC, my friend.\n", ch);
		return;
	}

	if (!IS_TRUSTED(vict))
	{
		send_to_char("That person does not appear to be godly enough.\n", ch);
		return;
	}

	if (!cmdname[0])
	{
		send_to_char("Use 'grant' to see what commands a char currently has granted.\n",
			     ch);
		return;
	}

	if (!vict->only.pc->gcmd_arr)
	{
		send_to_char("They have no commands granted to them.\n", ch);
		return;
	}

	/* let's get the command */

	cmd = old_search_block(cmdname, 0, strlen(cmdname), command, 2);

	if (cmd <= 0)
	{
		send_to_char("Sorry, that command does not seem to exist.\n", ch);
		return;
	}

	/* okay let's go zany.  ZANY! */

	s = vict->only.pc->numb_gcmd;

	for (i = 0; i < s; i++)
	{
		if (vict->only.pc->gcmd_arr[i] == cmd)
		{
			if (s > 1)
			{
				CREATE(new_arr, int, s - 1, MEM_TAG_ARRAY);

				/* copy up to here .. */

				for (j = 0; j < i; j++)
				{
					new_arr[j] = vict->only.pc->gcmd_arr[j];
				}

				/* copy past revoked command */

				for (j = i + 1; j < s; j++)
				{
					new_arr[j - 1] = vict->only.pc->gcmd_arr[j];
				}

				FREE(vict->only.pc->gcmd_arr);

				vict->only.pc->gcmd_arr = new_arr;
			}
			else /* only had one command, g'bye array */
			{
				FREE(vict->only.pc->gcmd_arr);

				vict->only.pc->gcmd_arr = NULL;
			}

			vict->only.pc->numb_gcmd--;

			snprintf(buf, MAX_STRING_LENGTH,
				 "You have revoked the command '%s' (level %d) from %s.\n",
				 command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));

			send_to_char(buf, ch);

			snprintf(buf, MAX_STRING_LENGTH,
				 "%s has revoked your ability to use the '%s' command.\n",
				 GET_NAME(ch), command[cmd - 1]);

			send_to_char(buf, vict);

			logit(LOG_WIZ, "%s revoked '%s' (%d) command from %s.", GET_NAME(ch),
			      command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));
			wizlog(GET_LEVEL(ch), "%s revoked '%s' (%d) command from %s.", GET_NAME(ch),
			       command[cmd - 1], cmd_info[cmd].minimum_level, GET_NAME(vict));
			sql_log(ch, WIZLOG, "Revoked '%s' (%d) command from %s.", command[cmd - 1],
				cmd_info[cmd].minimum_level, GET_NAME(vict));

			return;
		}
	}

	send_to_char("That character does not have that command granted.\n", ch);
}
