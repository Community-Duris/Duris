/* Staff account access rules and their legacy file storage. */

#include "core/prototypes.h"
#include "cmd/interp.h"
#include "core/structs.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/mm.h"
#include "core/files.h"
#include "net/comm.h"
#include "world/db.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern P_desc descriptor_list;
extern struct ban_t *ban_list;
extern struct wizban_t *wizconnect;

void do_wizhost(P_char ch, char *argument, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH];
	char ip[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct wizban_t *tmp;
	int count;

	if (!IS_TRUSTED(ch))
		return;

	argument_interpreter(argument, name, ip);

	if (!*name)
	{
		/* list the sites  */
		send_to_char(
			"Who:              Allowed Host\n--------------------------------------\n",
			ch);
		for (count = 0, tmp = wizconnect; tmp; count++, tmp = tmp->next)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%16s %s\n", tmp->name, tmp->ban_str);
			send_to_char(buf, ch);
		}
		return;
	}
	logit(LOG_WIZ, "(%s) allows wizconnect: %s", ch->player.name, argument);

	for (tmp = wizconnect; tmp; tmp = tmp->next)
	{
		if ((!str_cmp(name, tmp->name)) && (!str_cmp(ip, tmp->ban_str)))
		{
			send_to_char("That site is already allowed!\n", ch);
			return;
		}
	}
	CREATE(tmp, struct wizban_t, 1, MEM_TAG_WIZBAN);
	CREATE(tmp->name, char, strlen(name) + 1, MEM_TAG_STRING);
	CREATE(tmp->ban_str, char, strlen(ip) + 1, MEM_TAG_STRING);

	strcpy(tmp->name, name);
	strcpy(tmp->ban_str, ip);

	tmp->next = wizconnect;
	wizconnect = tmp;
	save_wizconnect_file();
}

void do_ban(P_char ch, char *argument, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct ban_t *tmp;
	int count;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, name);

	if (!*name)
	{
		/* list the sites  */
		send_to_char(
			"Lvl By:              Banned substrings\n--------------------------------------\n",
			ch);
		if (ban_list == (struct ban_t *)NULL)
		{
			send_to_char("Empty list!\n", ch);
			return;
		}
		for (count = 0, tmp = ban_list; tmp; count++, tmp = tmp->next)
		{
			snprintf(buf, MAX_STRING_LENGTH, "%3d %16s %s\n", tmp->lvl, tmp->name,
				 tmp->ban_str);
			send_to_char(buf, ch);
		}
		if (count == 1)
		{
			snprintf(buf, MAX_STRING_LENGTH, "\nThere is 1 banned site string.\n");
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "\nThere are %d banned site strings.\n",
				 count);
		}
		send_to_char(buf, ch);
		return;
	}
	logit(LOG_WIZ, "(%s) ban %s", ch->player.name, argument);
	for (tmp = ban_list; tmp; tmp = tmp->next)
	{
		if (!str_cmp(name, tmp->name))
		{
			send_to_char("That site is already banned!\n", ch);
			return;
		}
	}
	if (!strn_cmp("localhost", name, strlen(name)))
	{
		send_to_char("'localhost' may not be banned.\n", ch);
		return;
	}

	CREATE(tmp, struct ban_t, 1, MEM_TAG_BAN);
	CREATE(tmp->name, char, strlen(GET_NAME(ch)) + 1, MEM_TAG_STRING);
	CREATE(tmp->ban_str, char, strlen(name) + 1, MEM_TAG_STRING);

	strcpy(tmp->name, GET_NAME(ch));
	strcpy(tmp->ban_str, name);
	tmp->lvl = MIN(62, GET_LEVEL(ch));

	tmp->next = ban_list;
	ban_list = tmp;
	save_ban_file();
}

void do_allow(P_char ch, char *argument, int /*cmd*/)
{
	char name[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	struct ban_t *curr, *prev;

	if (!IS_TRUSTED(ch))
		return;

	one_argument(argument, name);

	if (!*name)
	{
		send_to_char("Remove which string from the ban list?\n", ch);
		return;
	}
	if (ban_list == NULL)
	{
		send_to_char("No sites are banned currently.\n", ch);
		return;
	}
	curr = prev = ban_list;
	if (!str_cmp(curr->ban_str, name))
	{
		if (curr->lvl > GET_LEVEL(ch))
		{
			send_to_char("Sorry, you are not high enough level to remove that ban.\n",
				     ch);
			return;
		}
		ban_list = ban_list->next;
		FREE(curr->ban_str);
		FREE(curr->name);
		FREE(curr);
		curr = NULL;
		send_to_char("Ok.\n", ch);
		logit(LOG_WIZ, "(%s) allow %s", ch->player.name, argument);
		save_ban_file();
		return;
	}
	curr = curr->next;
	while (curr)
	{
		if (!str_cmp(curr->ban_str, name))
		{
			if (curr->lvl > GET_LEVEL(ch))
			{
				send_to_char(
					"Sorry, you are not high enough level to remove that ban.\n",
					ch);
				return;
			}
			if (curr->next)
			{
				prev->next = curr->next;
				FREE(curr->name);
				FREE(curr->ban_str);
				FREE(curr);
				curr = NULL;
				send_to_char("Ok.\n", ch);
				save_ban_file();
				return;
			}
			prev->next = (struct ban_t *)NULL;
			FREE(curr->name);
			FREE(curr->ban_str);
			FREE(curr);
			curr = NULL;
			send_to_char("Ok.\n", ch);
			snprintf(buf, MAX_STRING_LENGTH, "WIZ: (%s) allow %s", ch->player.name,
				 argument);
			logit(LOG_WIZ, "%s", buf);
			save_ban_file();
			return;
		}
		curr = curr->next;
		prev = prev->next;
	}
	send_to_char("String not found in list!\n", ch);
}

void read_ban_file(void)
{
	FILE *f;
	char buf[MAX_STRING_LENGTH];
	int tmp;
	struct ban_t *ban;

	f = fopen(BAN_FILE, "r");
	if (!f)
	{
		/* The ban file is only written once a ban exists; absence just means
		   "no bans" and is not a failure worth logging. */
		if (errno != ENOENT)
			logit(LOG_FILE, "Could not open %s to read ban info.\n", BAN_FILE);
		return;
	}
	while (fscanf(f, "%s\n", buf) != EOF)
	{
		CREATE(ban, struct ban_t, 1, MEM_TAG_BAN);
		CREATE(ban->name, char, sizeof(buf) + 1, MEM_TAG_STRING);

		strcpy(ban->name, buf);
		REQUIRED_FSCANF(f, "%d\n", &tmp);
		ban->lvl = tmp;
		REQUIRED_FSCANF(f, "%s\n", buf);
		CREATE(ban->ban_str, char, sizeof(buf) + 1, MEM_TAG_STRING);

		strcpy(ban->ban_str, buf);
		ban->next = ban_list;
		ban_list = ban;
	}
	fclose(f);
}

void save_ban_file(void)
{
	struct ban_t *i;
	FILE *f;

	f = fopen(BAN_FILE, "w");
	if (!f)
	{
		logit(LOG_FILE, "Could not open %s to save ban info.\n", BAN_FILE);
		return;
	}
	for (i = ban_list; i; i = i->next)
	{
		fprintf(f, "%s\n", i->name);
		fprintf(f, "%d\n", i->lvl);
		fprintf(f, "%s\n", i->ban_str);
	}
	fclose(f);
}

void read_wizconnect_file(void)
{
	FILE *f;
	char buf[MAX_STRING_LENGTH];
	struct wizban_t *ban;

	f = fopen(WIZCONNECT_FILE, "r");
	if (!f)
	{
		logit(LOG_FILE, "Could not open %s to read wizconnect info.\n", WIZCONNECT_FILE);
		return;
	}
	while (fscanf(f, "%s\n", buf) != EOF)
	{
		CREATE(ban, struct wizban_t, 1, MEM_TAG_WIZBAN);
		CREATE(ban->name, char, sizeof(buf) + 1, MEM_TAG_STRING);

		strcpy(ban->name, buf);
		REQUIRED_FSCANF(f, "%s\n", buf);
		CREATE(ban->ban_str, char, sizeof(buf) + 1, MEM_TAG_STRING);

		strcpy(ban->ban_str, buf);
		ban->next = wizconnect;
		wizconnect = ban;
	}
	fclose(f);
}

void save_wizconnect_file(void)
{
	struct wizban_t *i;
	FILE *f;

	f = fopen(WIZCONNECT_FILE, "w");
	if (!f)
	{
		logit(LOG_FILE, "Could not open %s to save wizconnect info.\n", WIZCONNECT_FILE);
		return;
	}
	for (i = wizconnect; i; i = i->next)
	{
		fprintf(f, "%s\n", i->name);
		fprintf(f, "%s\n", i->ban_str);
	}
	fclose(f);
}

void do_wizlock(P_char ch, char *arg, int /*cmd*/)
{
	P_desc d;
	char buf[MAX_STRING_LENGTH];
	char buf1[MAX_STRING_LENGTH];

	if (!*arg || !str_cmp(arg, "?"))
	{
		send_to_char_f(
			ch,
			"Status: \nCreation   : %s\nConnections: %s\nMaxplayers : %s %d / %d\nLevel      : %s %d\n",
			YESNO(IS_SET(game_locked, LOCK_CREATION)),
			YESNO(IS_SET(game_locked, LOCK_CONNECTIONS)),
			YESNO(IS_SET(game_locked, LOCK_MAX_PLAYERS)), number_of_players(),
			game_locked_players, YESNO(IS_SET(game_locked, LOCK_LEVEL)),
			game_locked_level);
		send_to_char(
			"Usage: wizlock <creation | connections | maxplayers | level | ?> [value]\n",
			ch);
		return;
	}
	arg = one_argument(arg, buf1);

	if (is_abbrev(buf1, "creation"))
	{
		if (IS_SET(game_locked, LOCK_CREATION))
		{
			REMOVE_BIT(game_locked, LOCK_CREATION);
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+GDuris DikuMUD ->&n Restrictions on new character creations lifted.\n");
		}
		else
		{
			SET_BIT(game_locked, LOCK_CREATION);
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+RDuris DikuMUD ->&n Game is being locked;  no more character creation.\n");
		}
	}
	else if (is_abbrev(buf1, "connections"))
	{
		if (IS_SET(game_locked, LOCK_CONNECTIONS))
		{
			REMOVE_BIT(game_locked, LOCK_CONNECTIONS);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+GDuris DikuMUD ->&n Restrictions on new connections lifted.\n");
		}
		else
		{
			SET_BIT(game_locked, LOCK_CONNECTIONS);
			snprintf(
				buf, MAX_STRING_LENGTH,
				"&+RDuris DikuMUD ->&n Game is being locked; no more connections.\n");
		}
	}
	else if (is_abbrev(buf1, "maxplayers"))
	{
		if (IS_SET(game_locked, LOCK_MAX_PLAYERS))
		{
			REMOVE_BIT(game_locked, LOCK_MAX_PLAYERS);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+GDuris DikuMUD ->&n Restrictions on max players lifted.\n");
		}
		else
		{
			one_argument(arg, buf1);
			if (is_number(buf1) && (atoi(buf1) >= 0))
			{
				SET_BIT(game_locked, LOCK_MAX_PLAYERS);
				game_locked_players = atoi(buf1);
				snprintf(
					buf, MAX_STRING_LENGTH,
					"&+RDuris DikuMUD ->&n Game is limited to a MAX of %d players.\n",
					game_locked_players);
			}
			else
			{
				send_to_char(
					"To lock a number of players, please supply the number that's at least 0.\n",
					ch);
			}
		}
	}
	else if (is_abbrev(buf1, "level"))
	{
		if (IS_SET(game_locked, LOCK_LEVEL))
		{
			REMOVE_BIT(game_locked, LOCK_LEVEL);
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+GDuris DikuMUD ->&n Restrictions on level lifted.\n");
		}
		else
		{
			one_argument(arg, buf1);
			if (is_number(buf1) && (atoi(buf1) > 0))
			{
				SET_BIT(game_locked, LOCK_LEVEL);
				game_locked_level = atoi(buf1);
				snprintf(
					buf, MAX_STRING_LENGTH,
					"&+RDuris DikuMUD ->&n Game is limited to level %d players.\n",
					game_locked_level);
			}
			else
			{
				send_to_char(
					"To level-lock the game, please supply the number that's at least 1.\n",
					ch);
			}
		}
	}
	else
	{
		send_to_char(
			"Usage: wizlock <creation | connections | maxplayers | level | ?> [value].\n",
			ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next)
	{
		if (!d->connected)
		{
			send_to_char(buf, d->character);
		}
	}
}

void do_invite(P_char ch, char *arg, int /*cmd*/)
{
	char f_a[MAX_STRING_LENGTH];

	arg = skip_spaces(arg);
	if (!*arg)
	{
		send_to_char("Usage: invite <charname> or invite <on|off>\n", ch);
		return;
	}

	if (*arg && (isname(arg, "on") || isname(arg, "off")))
	{
		if (isname(arg, "on"))
		{
			invitemode = 1;
			wizlog(57, "%s has turned on Evil Invite", GET_NAME(ch));
			return;
		}
		else
		{
			invitemode = 0;
			wizlog(57, "%s has turned off Evil Invite", GET_NAME(ch));
			return;
		}
	}

	arg = one_argument(arg, f_a);

	create_denied_file("Players/Invited", f_a);

	send_to_char("Invited.\n", ch);

	wizlog(GET_LEVEL(ch), "%s has invited %s", GET_NAME(ch), f_a);
}

void do_uninvite(P_char ch, char *arg, int /*cmd*/)
{
	char f_a[MAX_STRING_LENGTH], path[2048];

	arg = skip_spaces(arg);
	if (!*arg)
	{
		send_to_char("Usage: uninvite <charname>\n", ch);
		return;
	}

	arg = one_argument(arg, f_a);

	checked_snprintf(path, 2048, "Players/Invited/%c/%s", f_a[0], f_a);

	// -1 is failure, 0 is success.
	if (unlink(path) == -1)
	{
		send_to_char("Failed.\n\r", ch);
		debug("Couldn't delete file '%s'.", path);
		return;
	}

	send_to_char("Uninvited.\n", ch);

	wizlog(GET_LEVEL(ch), "%s has uninvited %s", GET_NAME(ch), f_a);
}
