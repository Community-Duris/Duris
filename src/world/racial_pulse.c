/*
 * Racial pulse: each race's casting multiplier and melee round, adjustable in game.
 *
 * The rates live in lib/duris.properties. spellcast.pulse.racial.<Race> multiplies every
 * spell's cast time; damage.pulse.racial.<Race> is the base melee round in beats, before
 * damage.pulse.class.all and the class adjustment. Lower is faster in both. The 'pulse'
 * command lists them and sets one at a time through 'properties set', which re-reads both
 * tables at once. Like the difficulty dials, a change stays in memory until 'pulse save'.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/racial_pulse_math.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

extern const struct race_names race_names_table[];
extern P_char character_list;

namespace
{
std::string pulse_property(racial_pulse_table table, int race)
{
	return std::string(racial_pulse_property_prefix(table)) + race_names_table[race].no_spaces;
}

// The engine's own fallbacks when a race has no property: one combat round, and 1.0.
double pulse_value(racial_pulse_table table, int race)
{
	const double fallback = table == RACIAL_PULSE_CAST ? 1.0 : (double)PULSE_VIOLENCE;
	return get_property(pulse_property(table, race).c_str(), fallback);
}

// Letters and digits only, in lower case, so "Grey Elf", "GreyElf" and "greyelf" all match.
std::string race_key(const char *name)
{
	std::string key;
	for (; name && *name; ++name)
	{
		if (isalnum(static_cast<unsigned char>(*name)))
			key += static_cast<char>(tolower(static_cast<unsigned char>(*name)));
	}
	return key;
}

int find_race(const char *typed)
{
	const std::string wanted = race_key(typed);
	if (wanted.empty())
		return -1;
	for (int race = 1; race <= LAST_RACE; ++race)
	{
		if (race_key(race_names_table[race].no_spaces) == wanted ||
		    race_key(race_names_table[race].normal) == wanted)
			return race;
	}
	return -1;
}

void show_race_line(P_char ch, int race, const char *side, double class_all)
{
	char line[MAX_STRING_LENGTH];
	const double melee = pulse_value(RACIAL_PULSE_MELEE, race);
	snprintf(line, sizeof line, "  %-18s %-8s %8.3f  %8.3f  %6.3f\r\n",
		 race_names_table[race].normal, side, melee, melee + class_all,
		 pulse_value(RACIAL_PULSE_CAST, race));
	send_to_char(line, ch);
}

void show_pulse(P_char ch, bool every_race)
{
	char line[MAX_STRING_LENGTH];
	const double class_all = get_property("damage.pulse.class.all", 1.000);
	send_to_char("&+WRacial pulse&n (lower is faster)\r\n\r\n", ch);
	snprintf(line, sizeof line, "  %-18s %-8s %8s  %8s  %6s\r\n", "Race", "Side", "Melee",
		 "Round", "Cast");
	send_to_char(line, ch);
	if (every_race)
	{
		for (int race = 1; race <= LAST_RACE; ++race)
			show_race_line(ch, race, "", class_all);
	}
	else
	{
		for (const playable_race_info *entry = playable_races; entry->race_id >= 0; ++entry)
			show_race_line(ch, entry->race_id, entry->faction, class_all);
	}
	snprintf(line, sizeof line,
		 "\r\n  Melee is the base round in beats; Round adds the %.3f every class gets\r\n"
		 "  before its own class adjustment. Cast multiplies every spell's cast time.\r\n",
		 class_all);
	send_to_char(line, ch);
	send_to_char(
		"\r\n  pulse list all   pulse adjust <cast|melee> <race> <value>   pulse save\r\n"
		"  Values are absolute and take exactly three decimals, e.g. 0.900 or 12.000.\r\n",
		ch);
}
} // namespace

// pulse                                     list the creation races' rates
// pulse list [all]                          the same, or every race
// pulse adjust <cast|melee> <race> <value>  set one race's rate (Forger and up)
// pulse save                                write the current properties to disk (Forger and up)
void do_pulse(P_char ch, char *argument, int /*cmd*/)
{
	char command[16] = "", table_word[16] = "", race_word[64] = "", value_word[32] = "",
	     extra[16] = "";
	if (argument)
		sscanf(argument, " %15s %15s %63s %31s %15s", command, table_word, race_word,
		       value_word, extra);

	if (!*command || !strcmp(command, "list"))
	{
		show_pulse(ch, !strcmp(table_word, "all"));
		return;
	}
	if (strcmp(command, "adjust") && strcmp(command, "save"))
	{
		send_to_char(
			"Usage: pulse [list [all] | adjust <cast|melee> <race> <value> | save]\r\n",
			ch);
		return;
	}
	if (GET_LEVEL(ch) < FORGER)
	{
		send_to_char("Only a Forger or higher can change racial pulse.\r\n", ch);
		return;
	}
	if (!strcmp(command, "save"))
	{
		char request[] = "save";
		do_properties(ch, request, 0);
		return;
	}

	racial_pulse_table table = RACIAL_PULSE_CAST;
	if (!strcmp(table_word, "melee"))
		table = RACIAL_PULSE_MELEE;
	else if (strcmp(table_word, "cast"))
	{
		send_to_char("Usage: pulse adjust <cast|melee> <race> <value>\r\n", ch);
		return;
	}
	const int race = find_race(race_word);
	if (race < 0)
	{
		send_to_char("No such race. Type the name without spaces, for example 'greyelf'; "
			     "'pulse list all' shows every race.\r\n",
			     ch);
		return;
	}
	double value = 0.0;
	if (*extra || !racial_pulse_parse_value(value_word, &value))
	{
		send_to_char(
			"Enter an absolute value with exactly three decimals, for example 0.900 "
			"or 12.000.\r\n",
			ch);
		return;
	}
	char line[MAX_STRING_LENGTH];
	if (!racial_pulse_in_range(table, value))
	{
		const bool cast = table == RACIAL_PULSE_CAST;
		snprintf(line, sizeof line, "%s pulse must be from %.3f to %.3f.\r\n",
			 cast ? "Cast" : "Melee",
			 cast ? RACIAL_PULSE_CAST_MIN : RACIAL_PULSE_MELEE_MIN,
			 cast ? RACIAL_PULSE_CAST_MAX : RACIAL_PULSE_MELEE_MAX);
		send_to_char(line, ch);
		return;
	}
	const std::string key = pulse_property(table, race);
	const double before = get_property(key.c_str(), -1.0, false);
	if (before < 0.0)
	{
		snprintf(line, sizeof line,
			 "%s is not in duris.properties, so there is nothing to "
			 "adjust.\r\n",
			 key.c_str());
		send_to_char(line, ch);
		return;
	}

	// 'properties set' logs the change and re-applies every property, so both racial tables
	// are re-read at once. It changes memory only; 'pulse save' writes the file. The value
	// goes through as typed: it has already been checked to be exactly three decimals.
	std::string request = "set " + key + " " + value_word;
	do_properties(ch, request.data(), 0);

	// The melee round is fixed into a character when their affects are totalled, so
	// re-total everyone of the race; the cast multiplier is read at every cast.
	if (table == RACIAL_PULSE_MELEE)
	{
		for (P_char tch = character_list; tch; tch = tch->next)
		{
			if (GET_RACE(tch) == race)
				balance_affects(tch);
		}
	}

	snprintf(line, sizeof line, "%s %s pulse: %.3f -> %.3f (in memory until 'pulse save').\r\n",
		 race_names_table[race].normal, table == RACIAL_PULSE_CAST ? "cast" : "melee",
		 before, pulse_value(table, race));
	send_to_char(line, ch);
}
