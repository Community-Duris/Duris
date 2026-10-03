#include "core/prototypes.h"
#include "core/structs.h"
#include "core/defines.h"
#include "core/mm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/safe_format.h"
#include "net/comm.h"
#include "sql/sql.h"
#include "world/db.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;
extern const struct race_names race_names_table[];
void do_lookup(P_char ch, char *argument, int /*cmd*/)
{
	FILE *fp;
	char *irc;
	char arg[MAX_STRING_LENGTH], pattern[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH], file[MAX_INPUT_LENGTH];
	char o_buf[MAX_STRING_LENGTH];
	int found, length;

	if (IS_NPC(ch) || !ch->desc)
		return;

	half_chop(argument, arg, buf);
	//  one_argument(buf, pattern);
	o_buf[0] = 0;
	strcpy(pattern, buf);

	if (!*arg || !*pattern)
	{
		send_to_char(
			"Syntax : lookup <pfile | room | zone | mob | obj | random> <pattern>\n",
			ch);
		return;
	}
	/*
	 * Search the files created via the gawk script (during boot), * for
	 * the desired pattern.  Output goes to a temp file, which * is then
	 * read in.
	 */

	if (strncasecmp(arg, "random", 1) == 0)
	{
		if (strncasecmp(pattern, "zone", 1) == 0 && GET_LEVEL(ch) >= FORGER)
		{
			display_random_zones(ch);
			return;
		}
	}
	if (strncasecmp(arg, "mob", 1) == 0 || strncasecmp(arg, "char", 1) == 0)
	{
		strcpy(file, MOB_LOOKUP);
	}
	else if (strncasecmp(arg, "obj", 1) == 0)
	{
		strcpy(file, OBJ_LOOKUP);
	}
	else if (strncasecmp(arg, "room", 1) == 0)
	{
		strcpy(file, WLD_LOOKUP);
	}
	else if (strncasecmp(arg, "zone", 1) == 0)
	{
		strcpy(file, ZON_LOOKUP);
	}
	else if (strncasecmp(arg, "pfile", 1) == 0)
	{
		char m_class[MAX_STRING_LENGTH];
		char race[MAX_STRING_LENGTH];
		char level[MAX_STRING_LENGTH];
		char start_letter[MAX_STRING_LENGTH];

		half_chop(pattern, start_letter, buf);
		if (isname("*", start_letter))
		{
			send_to_char("start letter must be a or b etc can't use * (Couse of lag)\n",
				     ch);
			return;
		}

		half_chop(buf, level, pattern);
		half_chop(pattern, m_class, buf);
		half_chop(buf, race, pattern);
		checked_snprintf(
			buf, MAX_STRING_LENGTH,
			"&+RQuery:\n&+WFind all ch with first letter:&+R%s&+W Level:&+R%s&+W Class:&+R%s&+W Race:&+R%s&+W\n",
			start_letter, level, m_class, race);
		send_to_char(buf, ch);
		if (!*start_letter || !*level || !*race || !*m_class)
		{
			send_to_char("Syntax : 'lookup pfile a 50 Human Warrior'\n", ch);
			send_to_char("Syntax : 'lookup pfile d * Ogre *'\n", ch);
			return;
		}

		FILE *flist;
		char Gbuf2[MAX_STRING_LENGTH];
		char Gbuf3[MAX_STRING_LENGTH];
		char buffer[MAX_STRING_LENGTH];
		char tbuf[MAX_STRING_LENGTH];
		char tbuf2[MAX_STRING_LENGTH];

		int how_many = 0;
		P_char owner;

		snprintf(buf, MAX_STRING_LENGTH, "&+W%-12s %s %-10s\t %-10s&n\n", "Name", "Lev",
			 "Class", "Race");
		send_to_char(buf, ch);

		checked_snprintf(Gbuf3, MAX_STRING_LENGTH, "/bin/ls -1 Players/%s > %s",
				 start_letter, "temp_letterfile");
		if (system(Gbuf3) != 0) /* ls a list of Players into the temp_file */
		{
			logit(LOG_FILE, "do_players: failed to list Players/%s", start_letter);
			return;
		}
		flist = fopen("temp_letterfile", "r");
		if (!flist)
			return;

		while (fscanf(flist, " %s \n", Gbuf2) != EOF)
		{
			owner = (struct char_data *)mm_get(dead_mob_pool);
			ensure_pconly_pool();
			owner->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);

			if (restoreCharOnly(owner, skip_spaces(Gbuf2)) >= 0)
			{
				stripansi_2(race_to_string(owner), tbuf);
				half_chop(tbuf, tbuf, buf);
				stripansi_2(get_class_string(owner, buffer), tbuf2);

				tbuf2[strlen(tbuf2) - 1] = '\0';
				if (GET_LEVEL(owner) == (atoi(level)) || isname("*", level))
				{ // LEVEL
					if (isname(tbuf2, m_class) || isname("*", m_class))
					{ // CLASS
						if (isname(tbuf, race) || isname("*", race))
						{ // RACE
							how_many++;
							snprintf(buf, MAX_STRING_LENGTH,
								 "%-12s %d\t %-10s\t %-10s\n",
								 GET_NAME(owner), GET_LEVEL(owner),
								 get_class_string(owner, buffer),
								 race_to_string(owner));
							send_to_char(buf, ch);
							if (how_many > 20)
							{
								send_to_char(
									"To many results narrow down your search",
									ch);
								fclose(flist);
								return;
							} // End spam check

						} // end race
					} // end class

				} // end Level

			} // end restore
		} // End while
		fclose(flist);
		return;
	}
	else
	{
		send_to_char("Syntax : lookup <pfile room | zone | mob | obj> <pattern>\n", ch);
		return;
	}

	if ((fp = fopen(file, "r")) == NULL)
	{
		snprintf(buf, MAX_STRING_LENGTH, "Error opening %s", file);
		logit(LOG_FILE, "%s", buf);
		snprintf(buf, MAX_STRING_LENGTH, "Error opening %s...tell an implementor.\n", file);
		send_to_char(buf, ch);
		return;
	}
	else
	{
		/* Read in each line of the file.  See if the pattern
		   is in the line.  If so, pass it to the user. */
		found = 0;
		length = 0;
		strToLower(pattern); /* lower case all values for comparison */
		do
		{
			irc = fgets(buf, MAX_STRING_LENGTH - 1, fp);
			strcpy(arg, buf);
			strcpy(buf, strip_ansi(buf).c_str());
			strToLower(buf);
			if (strstr(buf, pattern) != NULL)
			{
				found = 1;
				if ((length + strlen(arg) + 40) > MAX_STRING_LENGTH)
				{
					strcat(o_buf, "...and the list goes on...\n");
					irc = NULL;
				}
				else
				{
					length += strlen(arg) + 1;
					strcat(o_buf, arg);
					strcat(o_buf, "");
				}
			}
		} while (irc != NULL);

		fclose(fp);

		if (!found)
		{
			snprintf(buf, MAX_STRING_LENGTH, "No matches found for pattern '%s'\n",
				 pattern);
			send_to_char(buf, ch);
		}
		else
		{
			page_string(ch->desc, o_buf, 1);
		}
	}
}

void GetMIA(char *playerName, char *returned)
{
	unsigned long laston, minutesgone;
	P_char finger_foo;

	if (!playerName || !*playerName)
	{
		snprintf(returned, MAX_STRING_LENGTH, "NoArgs");
		return;
	}

	finger_foo = (struct char_data *)mm_get(dead_mob_pool);
	ensure_pconly_pool();
	finger_foo->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);

	if (restoreCharOnly(finger_foo, skip_spaces(playerName)) < 0 || !finger_foo)
	{
		if (finger_foo)
			free_char(finger_foo);
		snprintf(returned, MAX_STRING_LENGTH, "NoPfile: '%s'.", playerName);
		return;
	}

	laston = finger_foo->player.time.saved;
	minutesgone = (time(0) - laston) / 60;

	snprintf(returned, MAX_STRING_LENGTH, "  &n(&+cMIA: &+w");
	if (minutesgone > 0)
	{
		// 1440 min / day = 24 hrs/day * 60 min / hr
		if (minutesgone > 1440)
		{
			snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
				 "%ld day%s%s", (minutesgone / 1440),
				 ((minutesgone / 1440) > 1) ? "s" : "",
				 (minutesgone % 1440) ? ", " : "");
		}
		// % 1440 -> removes days.  .. / 60 -> hours MIA.
		if ((minutesgone % 1440) / 60 > 0)
		{
			snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
				 "%ld hour%s%s", (minutesgone % 1440) / 60,
				 (((minutesgone % 1440) / 60) > 1) ? "s" : "",
				 (minutesgone % 60) ? ", " : "");
		}
		// % 60 cuts out hours, just leaving minutes MIA.
		if (minutesgone % 60)
		{
			snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
				 "%ld minute%s", (minutesgone % 60),
				 ((minutesgone % 60) > 1) ? "s" : "");
		}
	}
	else
	{
		minutesgone = time(0) - laston;
		snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
			 "%ld second%s", minutesgone, (minutesgone > 1) ? "s" : "");
	}

	strcat(returned, "&n)");

	return;
}

void GetMIA2(char *playerName, char *returned)
{
	unsigned long timegone;
	int days, hours, minutes, seconds;
	time_t laston;
	P_char finger_foo;

	finger_foo = (struct char_data *)mm_get(dead_mob_pool);
	ensure_pconly_pool();
	finger_foo->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (restoreCharOnly(finger_foo, skip_spaces(playerName)) < 0 || !finger_foo)
	{
		if (finger_foo)
			free_char(finger_foo);
		debug("Pfile does not exist or is invalid.\n");
		return;
	}

	laston = finger_foo->player.time.saved;
	timegone = time(0) - laston;

	days = (timegone) / (3600 * 24);
	hours = (timegone % (3600 * 24)) / (3600);
	minutes = (timegone % (3600)) / (60);
	seconds = (timegone % (60));

	snprintf(returned, MAX_STRING_LENGTH, "&+cMIA:&n ");

	if (days)
	{
		snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
			 "%d day%s%s", days, (days > 1) ? "s" : "", (hours || minutes) ? ", " : "");
	}
	if (hours)
	{
		snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
			 "%d hour%s%s", hours, (hours > 1) ? "s" : "",
			 (minutes || seconds) ? ", " : "");
	}
	if (minutes)
	{
		snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
			 "%d minute%s%s", minutes, (minutes > 1) ? "s" : "", (seconds) ? ", " : "");
	}
	// display seconds only if there are no days/hours/minutes
	if (seconds)
	{
		snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
			 "%d second%s", seconds, (seconds > 1) ? "s" : "");
	}
	snprintf(returned + strlen(returned), MAX_STRING_LENGTH - strlen(returned),
		 " - %ld mud hour%s.", timegone / SECS_PER_MUD_HOUR,
		 (timegone / SECS_PER_MUD_HOUR) > 1 ? "s" : "");
}

void do_finger(P_char ch, char *arg, int /*cmd*/)
{
	unsigned long timegone;
	time_t laston;
	char Gbuf1[MAX_STRING_LENGTH], Gbuf2[512];
	P_char finger_foo;
	bool in_game;
	int pid;

	if (!*arg)
	{
		send_to_char("Usage:\n  finger playername.\n", ch);
		return;
	}
	finger_foo = (struct char_data *)mm_get(dead_mob_pool);
	ensure_pconly_pool();
	finger_foo->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);

	if (restoreCharOnly(finger_foo, skip_spaces(arg)) < 0 || !finger_foo)
	{
		if (finger_foo)
			free_char(finger_foo);
		send_to_char("Pfile does not exist or is invalid.\n", ch);
		return;
	}
	if (GET_LEVEL(finger_foo) > GET_LEVEL(ch))
	{
		send_to_char("Sorry, you cannot finger those higher level than you.\n", ch);
		if (finger_foo)
			free_char(finger_foo);
		return;
	}
	snprintf(Gbuf1, MAX_STRING_LENGTH,
		 "&+cName:&n %s %s &n&+cLevel:&n %d &+cClass:&n %s &n&+cRace:&n %s\n",
		 GET_NAME(finger_foo), GET_TITLE(finger_foo), GET_LEVEL(finger_foo),
		 get_class_string(finger_foo, Gbuf2),
		 race_names_table[(int)GET_RACE(finger_foo)].ansi);
	laston = finger_foo->player.time.saved;
	timegone = (time(0) - laston) / 60;
	send_to_char(Gbuf1, ch);
	pid = GET_PID(finger_foo);
	snprintf(Gbuf1, MAX_STRING_LENGTH, "&+cPID:&n %d &+cLast saved:&n %s", pid,
		 asctime(localtime(&laston)));
	Gbuf1[strlen(Gbuf1) - 1] = 0;
	send_to_char(Gbuf1, ch);
	send_to_char("\n", ch);
	time_t lastConnect = 0, lastDisconnect = 0;
	strcpy(Gbuf1, "Unrecorded IP");
	sql_select_IP_info(finger_foo, Gbuf1, sizeof(Gbuf1), &lastConnect, &lastDisconnect);

	// If they just logged in, or logged out just after logging in (at menu: 1/rent)
	if (lastConnect == lastDisconnect)
	{
		in_game = is_pid_online(pid, FALSE);
	}
	else if (lastConnect > lastDisconnect)
	{
		in_game = FALSE;
	}
	else
	{
		in_game = TRUE;
	}

	if (in_game)
	{
		send_to_char("&+cPlaying from: &n", ch);
	}
	else
	{
		send_to_char("&+cLast played from: &n", ch);
	}
	send_to_char(Gbuf1, ch);

	if (in_game)
	{
		timegone = lastConnect;
		int hours = (timegone / 3600), minutes = (timegone % 3600) / 60,
		    seconds = (timegone % 60);

		send_to_char("  &n(&+cPlaying:&+w ", ch);
		*Gbuf1 = '\0';
		if (hours > 0)
			snprintf(Gbuf1, MAX_STRING_LENGTH, "%d hour%s", hours,
				 (hours > 1) ? "s" : "");
		if (minutes > 0)
			checked_snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1),
					 "%s%d minute%s", (hours > 0) ? ", " : "", minutes,
					 (minutes > 1) ? "s" : "");
		// display seconds only if there are no hours/minutes
		if (timegone < 60)
			checked_snprintf(Gbuf1 + strlen(Gbuf1), MAX_STRING_LENGTH - strlen(Gbuf1),
					 "%d second%s", seconds, (seconds > 1) ? "s" : "");
		strcat(Gbuf1, "&n)&n\n");
	}
	else
	{
		/* Handled in GetMIA
		if((lastConnect == 0) && (lastDisconnect == 0))
		  timegone = (time(0) - laston);
		else
		  timegone = lastDisconnect;
		*/

		GetMIA(finger_foo->player.name, Gbuf1);
		send_to_char(Gbuf1, ch);
		Gbuf1[0] = '\0';
	}
	send_to_char(Gbuf1, ch);

	snprintf(
		Gbuf1, MAX_STRING_LENGTH,
		"\n&+cLast Rented: &n%s&n &+W[&+C%d&+W]\n&n&+cBirthplace: &n%s&n &+W[&+C%d&+W]&n\n",
		world[real_room0(GET_HOME(finger_foo))].name, GET_HOME(finger_foo),
		world[real_room0(GET_BIRTHPLACE(finger_foo))].name, GET_BIRTHPLACE(finger_foo));
	send_to_char(Gbuf1, ch);
	if (finger_foo)
		free_char(finger_foo);
}

int race_lookup(char *raceStr)
{
	int i;

	// If the argument isn't a positive integer, check to see if it's a race name.
	if (!is_number(raceStr))
	{
		// Check for an exact match first...
		for (i = 0; i < LAST_RACE; i++)
		{
			// Check race, ignoring case
			if (!strcasecmp(race_names_table[i].normal, raceStr))
			{
				return i;
			}
		}
		// Check for a short version of race name iff not found above. (i.e. just 'grey' instead of 'grey elf')
		for (i = 0; i <= LAST_RACE; i++)
		{
			// Mob should always load, but just in case...
			if (is_abbrev(raceStr, race_names_table[i].normal))
			{
				return i;
			}
		}
	}
	else
	{
		// If it's a number, return it as an integer
		return atoi(raceStr);
	}

	return -1;
}
