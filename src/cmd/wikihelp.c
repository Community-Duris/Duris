#include "core/prototypes.h"
#include "core/utility.h"
#include "core/utils.h"
#include "cmd/wikihelp.h"
#include "cmd/help_cache.h"
#include "account/creation_availability_config.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_help_catalog.h"
#endif
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include "classes/specializations.h"
#include "sql/sql.h"
#include "string.h"
using namespace std;

extern struct race_names race_names_table[];
extern struct class_names class_names_table[];
extern char *specdata[][MAX_SPEC];
extern const char *stat_to_string3(int);
extern int allowed_secondary_classes[][5];
extern const mcname multiclass_names[];

void debug(const char *format, ...);

/* strip characters from front and back of string */
string trim(string const &str, char const *sep_chars)
{
	string::size_type const first = str.find_first_not_of(sep_chars);
	return (first == string::npos) ?
		       string() :
		       str.substr(first, str.find_last_not_of(sep_chars) - first + 1);
}

/* replace a string with another string in a string */
string str_replace(string haystack_, const char *needle_, const char *replace_)
{
	const string needle(needle_);
	if (needle.empty())
		return haystack_;
	string result;
	result.reserve(haystack_.size());
	size_t begin = 0;
	for (size_t pos = haystack_.find(needle); pos != string::npos;
	     pos = haystack_.find(needle, begin))
	{
		result.append(haystack_, begin, pos - begin);
		result += replace_;
		begin = pos + needle.size();
	}
	result.append(haystack_, begin, string::npos);
	return result;
}

/* clean up some of the wiki formatting */
string dewikify(string str_)
{
	string str(str_);
	str = str_replace(str, "[[", "&+c");
	str = str_replace(str, "]]", "&n");
	str = str_replace(str, "'''", "");
	return str;
}

string tolower(string str_)
{
	string str(str_);
	for (char &character : str)
		if (character >= 'A' && character <= 'Z')
			character = static_cast<char>(character - 'A' + 'a');
	return str;
}

namespace
{
enum class dynamic_help_type
{
	none,
	race,
	class_topic,
	specialization,
	skillset,
	race_index,
	multiclass,
};

struct dynamic_help_topic
{
	dynamic_help_type type = dynamic_help_type::none;
	string subject;
	string colored_title;
};

string colored_race_name(int race)
{
	const auto &name = race_names_table[race];
	string display = name.ansi;
	if (tolower(strip_ansi(name.ansi)) != tolower(name.normal))
		display += " &+L(" + string(name.normal) + ")&N";
	return display;
}

// Explicit SQL categories take precedence. Category-zero imports and flat files
// can bind only to exact names in the game registries, never to arbitrary prose.
dynamic_help_topic dynamic_topic(const string &title, int category = 0)
{
	const string key = tolower(title);
	if (key == "races")
		return { dynamic_help_type::race_index, title, "&+WRaces&N" };
	if (key == "multiclass")
		return { dynamic_help_type::multiclass, title, "&+WMulticlass&N" };
	if (category == 0 || category == 25)
		for (int race = 1; race <= RACE_PLAYER_MAX; ++race)
			if (key == tolower(race_names_table[race].normal))
				return { dynamic_help_type::race, race_names_table[race].normal,
					 colored_race_name(race) };
	// Assassin/Thief are current Rogue specializations as well as legacy class
	// names. An explicit class category still selects the historical class.
	if (category == 0 || category == 16)
		for (int cls = 1; cls <= CLASS_COUNT; ++cls)
			for (int spec = 0; spec < MAX_SPEC; ++spec)
			{
				const string name = strip_ansi(specdata[cls][spec]);
				if (!name.empty() && name != "Not Used" && key == tolower(name))
					return { dynamic_help_type::specialization, name,
						 specdata[cls][spec] };
			}
	for (int cls = 1; cls <= CLASS_COUNT; ++cls)
	{
		const string name = class_names_table[cls].normal;
		if ((category == 0 || category == 9) && key == tolower(name))
			return { dynamic_help_type::class_topic, name,
				 class_names_table[cls].ansi };
		if ((category == 0 || category == 10) &&
		    (key == tolower(name + " Skills") || key == tolower("SKILL_" + name)))
			return { dynamic_help_type::skillset, name,
				 string(class_names_table[cls].ansi) + " Skills" };
	}
	return {};
}

string help_display_title(const string &title, int category = 0)
{
	const auto topic = dynamic_topic(title, category);
	return topic.type == dynamic_help_type::none ? "&+c" + title : topic.colored_title;
}

bool dynamic_section(const dynamic_help_topic &topic, const string &heading)
{
	const string key = tolower(heading);
	if (topic.type == dynamic_help_type::race)
		return key == "class list" || key == "racial statistics" ||
		       key == "racial traits" || key == "innate abilities";
	if (topic.type == dynamic_help_type::race_index)
		return key == "good races" || key == "evil races" || key == "neutral races" ||
		       key == "restricted races";
	if (topic.type == dynamic_help_type::multiclass)
		return key == "multi-class names" || key == "multiclass options";
	if (key == "innate abilities" || key == "innates abilities")
		return true;
	if (topic.type == dynamic_help_type::class_topic)
		return key == "allowed races" || key == "allowable races" ||
		       key == "specializations" ||
		       key == tolower(topic.subject + " Specializations");
	return key == "skills" || key == "spells" || key == "songs" || key == "spec songs" ||
	       key == "instruments" || key == "additional instruments" ||
	       (topic.type == dynamic_help_type::specialization &&
		(key == "allowed races" || key == "allowable races")) ||
	       key == tolower(topic.subject + " Specializations");
}

// Replace only sections owned by this provider. Keep descriptions, examples,
// strengths/weaknesses, equipment notes, and See also sections around them.
string help_narrative(const string &title, const string &text, const dynamic_help_topic &topic)
{
	if (topic.type == dynamic_help_type::none)
		return text;
	std::istringstream input(text);
	string line, output;
	bool first = true, skip_rule = false, generated = false;
	while (std::getline(input, line))
	{
		const string plain = trim(strip_ansi(line.c_str()), " \t\r\n");
		const string key = tolower(plain);
		if (first && key.rfind(tolower(title) + " - last edited:", 0) == 0)
		{
			first = false;
			skip_rule = true;
			continue;
		}
		first = false;
		if (skip_rule)
		{
			skip_rule = false;
			if (!plain.empty() && plain.find_first_not_of('=') == string::npos)
				continue;
		}
		if (key.rfind("the following help topics", 0) == 0)
			break; // Captured search results are display chrome, not authored text.
		if (plain.size() > 4 && plain.rfind("==", 0) == 0 &&
		    plain.compare(plain.size() - 2, 2, "==") == 0 &&
		    plain.find_first_not_of('=') != string::npos)
			generated = dynamic_section(topic,
						    trim(plain.substr(2, plain.size() - 4), " "));
		else if (topic.type == dynamic_help_type::skillset && dynamic_section(topic, plain))
			generated = true; // The SKILL_<class> index uses plain section names.
		else if (key.rfind("see also:", 0) == 0)
			generated = false;
		else if ((topic.type == dynamic_help_type::multiclass &&
			  key == "here are the options available to each class:") ||
			 (topic.type == dynamic_help_type::race_index &&
			  key == "the following races are currently available on duris dikumud:"))
			generated = true;
		if (!generated)
			output += line + "\n";
	}
	return trim(output, " \t\r\n");
}

string render_help_content(const string &title, const string &text, int category = 0)
{
	const auto topic = dynamic_topic(title, category);
	string result = dewikify(trim(help_narrative(title, text, topic), " \t\r\n"));
	if (topic.type != dynamic_help_type::none)
		result += "\n\n&+LCurrent game data&N\n";
	switch (topic.type)
	{
	case dynamic_help_type::race:
		result += wiki_classes(topic.subject) + "\n" + wiki_racial_stats(topic.subject) +
			  "\n" + wiki_innates(topic.subject, WIKI_RACE);
		break;
	case dynamic_help_type::class_topic:
		result += wiki_races(topic.subject, WIKI_CLASS) + "\n" +
			  wiki_innates(topic.subject, WIKI_CLASS) + "\n" +
			  wiki_specs(topic.subject);
		break;
	case dynamic_help_type::specialization:
		result += wiki_races(topic.subject, WIKI_SPEC) + "\n" +
			  wiki_innates(topic.subject, WIKI_SPEC) + "\n" +
			  wiki_skills(topic.subject, WIKI_SPEC) + "\n" +
			  wiki_spells(topic.subject, WIKI_SPEC);
		break;
	case dynamic_help_type::skillset:
		result += wiki_innates(topic.subject, WIKI_CLASS) + "\n" +
			  wiki_skills(topic.subject, WIKI_CLASS) + "\n" +
			  wiki_spells(topic.subject, WIKI_CLASS);
		break;
	case dynamic_help_type::race_index:
		result += wiki_pcraces(topic.subject);
		break;
	case dynamic_help_type::multiclass:
		result += wiki_multiclass(topic.subject);
		break;
	case dynamic_help_type::none:
		break;
	}
	return result;
}

string generated_help(const string &title)
{
	if (dynamic_topic(title).type == dynamic_help_type::none)
		return {};
	return help_display_title(title) + "&N\n" +
	       render_help_content(
		       title, "A narrative help entry has not yet been authored for this topic.");
}

bool creation_help_race(int race)
{
	if (!creation_race_enabled(race))
		return false;
	for (int i = 0; playable_races[i].race_id != -1; ++i)
		if (playable_races[i].race_id == race)
			return true;
	if (creation_all_races_enabled())
		for (int i = 0; restricted_races[i].race_id != -1; ++i)
			if (restricted_races[i].race_id == race)
				return true;
	return false;
}
} // namespace

#ifdef __NO_MYSQL__

namespace
{
struct cached_flat_help
{
	flatfile_help_catalog catalog;
	string error;
	bool ready = false;
};

const cached_flat_help &flat_help()
{
	static const cached_flat_help cached = []
	{
		cached_flat_help value;
		value.ready = flatfile_help_catalog_load(".", &value.catalog, &value.error);
		return value;
	}();
	return cached;
}

string render_flat_help(const flatfile_help_entry &entry, unsigned int depth)
{
	if (depth < 8)
	{
		const string content = trim(entry.text, " \t\r\n");
		const string prefix = "Redirect:";
		if (content.rfind(prefix, 0) == 0)
		{
			const string target = trim(content.substr(prefix.size()), " \t\r\n");
			const flatfile_help_entry *redirect =
				flatfile_help_catalog_find(flat_help().catalog, target);
			if (redirect)
				return render_flat_help(*redirect, depth + 1);
			const string generated = generated_help(target);
			if (!generated.empty())
				return generated;
		}
	}
	string rendered = help_display_title(entry.title) + "&N\n&+L";
	rendered.append(ansi_strlen(help_display_title(entry.title).c_str()), '=');
	rendered += "&N\n";
	rendered += render_help_content(entry.title, entry.text);
	return rendered;
}
} // namespace

string wiki_help_single(string str)
{
	const auto &help = flat_help();
	if (!help.ready)
	{
		logit(LOG_DEBUG, "flat-file help catalog unavailable: %s", help.error.c_str());
		return string("&+GSorry, but there was an error with the help system.");
	}
	const flatfile_help_entry *entry = flatfile_help_catalog_find(help.catalog, str);
	if (entry)
		return render_flat_help(*entry, 0);
	const string generated = generated_help(str);
	return generated.empty() ? string("&+GHelp topic not found.") : generated;
}

string wiki_help(string str)
{
	str = trim(str, " \t\r\n");
	if (str.empty())
		return wiki_help_single("help");
	const auto &help = flat_help();
	if (!help.ready)
	{
		logit(LOG_DEBUG, "flat-file help catalog unavailable: %s", help.error.c_str());
		return string("&+GSorry, but there was an error with the help system.");
	}
	const flatfile_help_entry *exact = flatfile_help_catalog_find(help.catalog, str);
	if (!exact)
	{
		const string generated = generated_help(str);
		if (!generated.empty())
			return generated;
	}
	const auto matches = flatfile_help_catalog_search(
		help.catalog, str, static_cast<size_t>(WIKIHELP_RESULTS_LIMIT) + 1);
	if (matches.empty())
	{
		logit(LOG_HELP, "%s", str.c_str());
		return string("&+GSorry, but there are no help topics that match your search.\n"
			      "Try HELP <shorter keyword>, HELP COMMANDS, or COMMANDS.");
	}
	if (matches.size() == 1)
		return render_flat_help(*matches.front(), 0);
	string result;
	if (exact)
	{
		result = render_flat_help(*exact, 0);
		result += "\n\n&+GThe following help topics also matched your search:\n";
	}
	else
		result = "&+GThe following help topics matched your search:\n";
	size_t listed = 0;
	for (const auto *entry : matches)
		if (entry != exact)
		{
			if (listed == WIKIHELP_RESULTS_LIMIT)
				break;
			result += " " + help_display_title(entry->title) + "&N\n";
			++listed;
		}
	if (matches.size() > WIKIHELP_RESULTS_LIMIT)
		result +=
			"&+GThe list is limited to 100 topics; use a longer keyword to narrow your search.\n";
	result += "&+GType HELP <topic> to read an entry.&N\n";
	return result;
}

#else

string wiki_help(string str)
{
	str = trim(str, " \t\r\n");
	const auto *catalog = help_cache_get();
	if (!catalog)
		return "&+GHelp is temporarily unavailable while its catalog loads. Please try again shortly.";
	if (str.empty())
		return wiki_help_single("help");
	std::vector<const help_page *> matches;
	const help_page *exact = nullptr;
	for (const auto &page : *catalog)
	{
		if (help_title_equal(page.fields[0], str))
			exact = &page;
		if (matches.size() < WIKIHELP_RESULTS_LIMIT + 1 &&
		    help_title_matches(page.fields[0], str))
		{
			matches.push_back(&page);
		}
	}
	if (!exact)
	{
		const string generated = generated_help(str);
		if (!generated.empty())
			return generated;
	}
	if (matches.empty())
	{
		logit(LOG_HELP, "%s", str.c_str());
		return "&+GSorry, but there are no help topics that match your search.\n"
		       "Try HELP <shorter keyword>, HELP COMMANDS, or COMMANDS.";
	}
	if (matches.size() == 1)
		return wiki_help_single(matches.front()->fields[0]);
	std::string result;
	if (exact)
		result = wiki_help_single(exact->fields[0]) +
			 "\n\n&+GThe following help topics also matched your search:\n";
	else
		result = "&+GThe following help topics matched your search:\n";
	size_t listed = 0;
	for (const auto *page : matches)
		if (page != exact)
		{
			if (listed == WIKIHELP_RESULTS_LIMIT)
				break;
			result +=
				" " +
				help_display_title(page->fields[0], atoi(page->fields[2].c_str())) +
				"&N\n";
			++listed;
		}
	if (matches.size() > WIKIHELP_RESULTS_LIMIT)
		result +=
			"&+GThe list is limited to 100 topics; use a longer keyword to narrow your search.\n";
	result += "&+GType HELP <topic> to read an entry.&N\n";
	return result;
}

#endif

// display racial stats for a race category help file
string wiki_racial_stats(string title)
{
	string return_str, race_str;
	char race[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH] = "";
	int i;

	for (i = 0; i <= RACE_PLAYER_MAX; i++)
	{
		if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
		{
			race_str += race_names_table[i].no_spaces;
			break;
		}
	}

	return_str += "&+W==Racial Statistics==&N\n";

	if (i > RACE_PLAYER_MAX)
	{
		return_str += "No data found for race '";
		return_str += title;
		return_str += "&n'\n";
		return return_str;
	}

	return_str += "Strength    : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.str.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Agility     : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.agi.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Dexterity   : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.dex.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Constitution: &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.con.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Power       : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.pow.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Intelligence: &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.int.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Wisdom      : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.wis.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Charisma    : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.cha.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Luck        : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.luc.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "Karma       : &+c";
	snprintf(race, MAX_STRING_LENGTH, "stats.kar.%s", race_str.c_str());
	// return_str += stat_to_string3((int)get_property(race, 100));
	snprintf(buf, MAX_STRING_LENGTH, "%d", (int)get_property(race, 100));
	return_str += buf;
	return_str += "&n\n";
	return_str += "\n&+W==Racial Traits==&N\n";
	// Removing damage output because this is more of a fine tuning function
	// for imm's, and will only confuse players.
	// return_str += "Damage Output: &+c";
	// snprintf(race, MAX_STRING_LENGTH, "damage.totalOutput.racial.%s", race_str.c_str());
	// return_str += stat_to_string_spell_pulse(get_property(race, 1.000));
	// return_str += "&n\n";
	return_str += "Combat Pulse : &+c";
	snprintf(race, MAX_STRING_LENGTH, "damage.pulse.racial.%s", race_str.c_str());
	return_str += stat_to_string_damage_pulse(get_property(race, 14.000));
	return_str += "&n\n";
	return_str += "Spell Pulse  : &+c";
	snprintf(race, MAX_STRING_LENGTH, "spellcast.pulse.racial.%s", race_str.c_str());
	return_str += stat_to_string_spell_pulse(get_property(race, 1.000));
	return_str += "&n\n";
	return return_str;
}

// Display Classes and specs based on whats allowed in the code.
string wiki_classes(string title)
{
	string return_str;
	int i, found = 0;

	for (i = 0; i <= RACE_PLAYER_MAX; i++)
	{
		if (!strcmp(tolower(race_names_table[i].normal).c_str(), tolower(title).c_str()))
		{
			break;
		}
	}

	return_str += "&+W==Class list==&N\n";

	if (i > RACE_PLAYER_MAX)
	{
		return_str += "No data found for race '";
		return_str += title;
		return_str += "&n'\n";
		return return_str;
	}
	return_str += "&+LCurrent character-creation choices.&N\n";
	if (!creation_help_race(i))
	{
		return_str += "This race is not currently offered at character creation.\n";
		return return_str;
	}

	for (int cls = 1; cls <= CLASS_COUNT; cls++)
	{
		if (creation_class_enabled(cls) && creation_class_align(i, cls) != 5)
		{
			found = 1;
			return_str += "* ";
			return_str += pad_ansi(class_names_table[cls].ansi, 12);
			return_str += "&n: ";
			return_str += single_spec_list(i, cls);
			return_str += "\n";
		}
	}
	if (!found)
		return_str += "No classes available.\n";

	return return_str;
}

string wiki_specs(string title)
{
	string return_str;
	int i, j, found = 0;

	for (i = 0; i <= CLASS_COUNT; i++)
	{
		if (!strcmp(tolower(class_names_table[i].normal).c_str(), tolower(title).c_str()))
		{
			break;
		}
	}

	return_str += "&+W==Specializations==&N\n";

	if (i > CLASS_COUNT)
	{
		return_str += "No data found for class '";
		return_str += title;
		return_str += "'&n\n";
		return return_str;
	}

	for (j = 0; j < MAX_SPEC; j++)
	{
		if (!strcmp(specdata[i][j], "") || !strcmp(specdata[i][j], "Not Used"))
		{
			continue;
		}
		found = TRUE;
		return_str += "* ";
		return_str += string(specdata[i][j]);
		return_str += "\n";
	}

	if (!found)
	{
		return_str += "No specializations found.\n";
	}

	return return_str;
}

string wiki_innates(string title, int type)
{
	string return_str;
	int i = 0, j = 0, found = 0;

	if (type == WIKI_RACE)
	{
		for (i = 0; i <= RACE_PLAYER_MAX; i++)
		{
			if (!strcmp(tolower(race_names_table[i].normal).c_str(),
				    tolower(title).c_str()))
			{
				found = 1;
				break;
			}
		}
	}

	if (type == WIKI_CLASS)
	{
		for (i = 0; i <= CLASS_COUNT; i++)
		{
			if (!strcmp(tolower(class_names_table[i].normal).c_str(),
				    tolower(title).c_str()))
			{
				found = 2;
				break;
			}
		}
	}

	if (type == WIKI_SPEC)
	{
		// Skip "CLASS_NONE"
		for (i = 1; i <= CLASS_COUNT; i++)
		{
			for (j = 0; j < MAX_SPEC; j++)
			{
				if (!strcmp(tolower(strip_ansi(specdata[i][j])).c_str(),
					    tolower(title).c_str()))
				{
					found = 2;
					break;
				}
			}
			if (found == 2)
			{
				// Specs range from 1-4 not 0-3.
				j++;
				break;
			}
		}
	}

	return_str += "&+W==Innate abilities==&N\n";

	if (!found)
	{
		return_str += "No entries found.\n";
		return return_str;
	}

	return_str += list_innates(((found == 1) ? i : 0), ((found == 2) ? i : 0), j);

	return return_str;
}

string wiki_races(string title, int type)
{
	string return_str;
	int cls = CLASS_COUNT + 1, spec = 0, race;
	bool found = false;

	if (type == WIKI_CLASS)
	{
		spec = 0;
		// Find class to search for.
		for (cls = 0; cls <= CLASS_COUNT; cls++)
		{
			if (!strcmp(tolower(class_names_table[cls].normal).c_str(),
				    tolower(title).c_str()))
			{
				break;
			}
		}
	}
	else if (type == WIKI_SPEC)
	{
		found = FALSE;
		for (cls = 0; cls <= CLASS_COUNT; cls++)
		{
			for (spec = 0; spec < MAX_SPEC; spec++)
			{
				if (!strcmp(tolower(strip_ansi(specdata[cls][spec])).c_str(),
					    tolower(title).c_str()))
				{
					found = TRUE;
					break;
				}
			}
			if (found == TRUE)
			{
				// Specs range from 1-4 not 0-3.
				spec++;
				break;
			}
		}
	}

	return_str += "&+W==Allowed races==&N\n";
	return_str += "&+LCurrent character-creation choices.&N\n";

	if (cls > CLASS_COUNT)
	{
		if (type == WIKI_CLASS)
		{
			return_str += "No data found for class '";
		}
		else if (type == WIKI_SPEC)
		{
			return_str += "No data found for spec '";
		}
		else
		{
			return_str += "Unknown type.  Plz report to a God.\n";
			return return_str;
		}
		return_str += title;
		return_str += "&n'\n";
		return return_str;
	}

	found = FALSE;
	for (race = 1; race <= RACE_PLAYER_MAX; race++)
	{
		// Class not allowed for race.
		if (!creation_help_race(race) || !creation_class_enabled(cls) ||
		    creation_class_align(race, cls) == 5)
		{
			continue;
		}
		// Spec not allowed for race.
		if (type == WIKI_SPEC && !is_allowed_race_spec(race, 1 << (cls - 1), spec))
		{
			continue;
		}
		if (!found)
		{
			return_str += "&+W*&n ";
			found = TRUE;
		}
		else
		{
			return_str += ", ";
		}
		return_str += colored_race_name(race);
		return_str += "&n";
	}

	if (!found)
	{
		return_str += "None.\n";
		return return_str;
	}

	return_str += "\n";
	return return_str;
}

#ifndef __NO_MYSQL__

// Display a single help topic. Resolve redirects without recursion or I/O.
string wiki_help_single(string str)
{
	const auto *catalog = help_cache_get();
	if (!catalog)
		return "&+GHelp is temporarily unavailable while its catalog loads. Please try again shortly.";
	const help_page *selected = nullptr;
	for (unsigned depth = 0; depth < 8; ++depth)
	{
		selected = nullptr;
		for (const auto &page : *catalog)
			if (help_title_equal(page.fields[0], str))
			{
				selected = &page;
				break;
			}
		if (!selected)
		{
			const string generated = generated_help(str);
			return generated.empty() ? "&+GHelp topic not found." : generated;
		}
		const auto &fields = selected->fields;
		if (fields[2] != "1" || fields[1].rfind("Redirect: ", 0) != 0)
			break;
		str = trim(fields[1].substr(10), " \t\r\n");
		selected = nullptr;
	}
	if (!selected)
		return "&+GHelp redirect limit exceeded.";
	const char *row[5];
	for (size_t i = 0; i < 5; ++i)
		row[i] = selected->fields[i].c_str();
	string return_str;
	int dashes;

	return_str = help_display_title(row[0], atoi(row[2]));
	return_str += "&N - Last Edited: &+w";
	return_str += row[3];
	return_str += "&n by &+w";
	return_str += (row[4] == NULL) ? "Unknown" : row[4];

	dashes = ansi_strlen(return_str.c_str());
	return_str += "&N\n&+L";
	while (dashes-- > 0)
	{
		return_str += "=";
	}
	return_str += "&N\n";

	return_str += render_help_content(row[0], row[1], atoi(row[2]));

	return return_str;
}

#endif

struct cmd_attrib_data cmd_attribs[CMD_ATTRIB_MAX];

void load_cmd_attributes()
{
	FILE *cmd_file;
	char line[MAX_STRING_LENGTH];
	char attributes[MAX_STRING_LENGTH];
	int count = 0, i = 0;
	bool ch_attributes[ATT_MAX];
	bool vi_attributes[ATT_MAX];

	for (count = 0; count < CMD_ATTRIB_MAX; count++)
	{
		cmd_attribs[count].name = cmd_attribs[count].attributes = NULL;
	}

	cmd_file = fopen("docs/lib/information/command_attributes.txt", "r");
	if (!cmd_file)
	{
		logit(LOG_DEBUG, "Could not open command_attributes.txt.");
		return;
	}

	count = 0;
	while (fgets(line, sizeof line, cmd_file) != NULL)
	{
		if (count >= CMD_ATTRIB_MAX)
		{
			logit(LOG_DEBUG,
			      "command_attributes.txt: entry limit reached, remaining entries skipped.");
			break;
		}
		// First line is the name.
		cmd_attribs[count].name = strdup(line);
		attributes[0] = '\0';
#define APPEND_ATTR(text)                                         \
	do                                                        \
	{                                                         \
		size_t __attr_len = strlen(attributes);           \
		size_t __attr_add = strlen(text);                 \
		if (__attr_len + __attr_add < sizeof(attributes)) \
			strcat(attributes, text);                 \
	} while (0)
		// Set all attributes to false
		for (i = 0; i < ATT_MAX; i++)
		{
			ch_attributes[i] = FALSE;
			vi_attributes[i] = FALSE;
		}
		// Following lines are attributes until '~' is encountered.
		REQUIRED_FGETS(line, sizeof line, cmd_file);
		while (line[0] != '~')
		{
			// If GET_C_STR
			if (line[6] == 'S')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_STR] = TRUE;
				else
					ch_attributes[ATT_STR] = TRUE;
			}
			else if (line[6] == 'D')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_DEX] = TRUE;
				else
					ch_attributes[ATT_DEX] = TRUE;
			}
			else if (line[6] == 'A')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_AGI] = TRUE;
				else
					ch_attributes[ATT_AGI] = TRUE;
			}
			else if (line[6] == 'C' && line[7] == 'O')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_CON] = TRUE;
				else
					ch_attributes[ATT_CON] = TRUE;
			}
			else if (line[6] == 'P')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_POW] = TRUE;
				else
					ch_attributes[ATT_POW] = TRUE;
			}
			else if (line[6] == 'I')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_INT] = TRUE;
				else
					ch_attributes[ATT_INT] = TRUE;
			}
			else if (line[6] == 'W')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_WIS] = TRUE;
				else
					ch_attributes[ATT_WIS] = TRUE;
			}
			else if (line[6] == 'C' && line[7] == 'H')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_CHA] = TRUE;
				else
					ch_attributes[ATT_CHA] = TRUE;
			}
			else if (line[6] == 'K')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_KAR] = TRUE;
				else
					ch_attributes[ATT_KAR] = TRUE;
			}
			else if (line[6] == 'L')
			{
				if (line[10] == 'v')
					vi_attributes[ATT_LUK] = TRUE;
				else
					ch_attributes[ATT_LUK] = TRUE;
			}
			else
			{
				logit(LOG_DEBUG, "Bad line in command_attributes.txt: ");
				logit(LOG_DEBUG, "%s", line);
			}
			REQUIRED_FGETS(line, sizeof line, cmd_file);
		}
		// create the list of attributes.
		attributes[0] = '\0';
		APPEND_ATTR(
			"The following character attributes are used in execution of this ability (if any):\n");
		if (ch_attributes[ATT_STR])
			APPEND_ATTR("Char's Strength.\n");
		if (vi_attributes[ATT_STR])
			APPEND_ATTR("Victim's Strength.\n");
		if (ch_attributes[ATT_DEX])
			APPEND_ATTR("Char's Dexterity.\n");
		if (vi_attributes[ATT_DEX])
			APPEND_ATTR("Victim's Dexterity.\n");
		if (ch_attributes[ATT_AGI])
			APPEND_ATTR("Char's Agility.\n");
		if (vi_attributes[ATT_AGI])
			APPEND_ATTR("Victim's Agility.\n");
		if (ch_attributes[ATT_CON])
			APPEND_ATTR("Char's Constitution.\n");
		if (vi_attributes[ATT_CON])
			APPEND_ATTR("Victim's Constitution.\n");
		if (ch_attributes[ATT_POW])
			APPEND_ATTR("Char's Power.\n");
		if (vi_attributes[ATT_POW])
			APPEND_ATTR("Victim's Power.\n");
		if (ch_attributes[ATT_INT])
			APPEND_ATTR("Char's Intelligence.\n");
		if (vi_attributes[ATT_INT])
			APPEND_ATTR("Victim's Intelligence.\n");
		if (ch_attributes[ATT_WIS])
			APPEND_ATTR("Char's Wisdom.\n");
		if (vi_attributes[ATT_WIS])
			APPEND_ATTR("Victim's Wisdom.\n");
		if (ch_attributes[ATT_CHA])
			APPEND_ATTR("Char's Charisma.\n");
		if (vi_attributes[ATT_CHA])
			APPEND_ATTR("Victim's Charisma.\n");
		if (ch_attributes[ATT_KAR])
			APPEND_ATTR("Char's Karma.\n");
		if (vi_attributes[ATT_KAR])
			APPEND_ATTR("Victim's Karma.\n");
		if (ch_attributes[ATT_LUK])
			APPEND_ATTR("Char's Luck.\n");
		if (vi_attributes[ATT_LUK])
			APPEND_ATTR("Victim's Luck.\n");

		// add list to the array.
		cmd_attribs[count++].attributes = strdup(attributes);
	}
}

char *attrib_help(char *arg)
{
	int count = 0;

	while (cmd_attribs[count].name != NULL)
	{
		if (strstr(cmd_attribs[count].name, arg))
		{
			return cmd_attribs[count].attributes;
		}
		count++;
	}

	return NULL;
}

string wiki_spells(string title, int type)
{
	string return_str;
	int i = 0, j = 0;
	bool found = FALSE;

	if (type == WIKI_CLASS)
	{
		for (i = 0; i <= CLASS_COUNT; i++)
		{
			if (!strcmp(tolower(class_names_table[i].normal).c_str(),
				    tolower(title).c_str()))
			{
				found = TRUE;
				j = 0;
				break;
			}
		}
	}

	if (type == WIKI_SPEC)
	{
		for (i = 0; i <= CLASS_COUNT; i++)
		{
			for (j = 0; j < MAX_SPEC; j++)
			{
				if (!strcmp(tolower(strip_ansi(specdata[i][j])).c_str(),
					    tolower(title).c_str()))
				{
					found = TRUE;
					break;
				}
			}
			if (found == TRUE)
			{
				// Specs range from 1-4 not 0-3.
				j++;
				break;
			}
		}
	}

	return_str += "&+W==Spells==&N";

	if (!found)
	{
		return_str += "\nNo entries found.\n";
		return return_str;
	}

	// List spells( class, spec )
	return_str += list_spells(i, j);

	// If Bard, then show songs after spells.
	if (i == flag2idx(CLASS_BARD))
	{
		if (j == 0)
		{
			return_str += "\n&+W==Songs==&N";
		}
		else
		{
			return_str += "\n&+W==Spec Songs==&N";
		}
		return_str += list_songs(i, j);
	}
	return return_str;
}

string wiki_skills(string title, int type)
{
	string return_str;
	int i = 0, j = 0;
	bool found = FALSE;

	if (type == WIKI_CLASS)
	{
		for (i = 0; i <= CLASS_COUNT; i++)
		{
			if (!strcmp(tolower(class_names_table[i].normal).c_str(),
				    tolower(title).c_str()))
			{
				found = TRUE;
				j = 0;
				break;
			}
		}
	}

	if (type == WIKI_SPEC)
	{
		for (i = 0; i <= CLASS_COUNT; i++)
		{
			for (j = 0; j < MAX_SPEC; j++)
			{
				if (!strcmp(tolower(strip_ansi(specdata[i][j])).c_str(),
					    tolower(title).c_str()))
				{
					found = TRUE;
					break;
				}
			}
			if (found == TRUE)
			{
				// Specs range from 1-4 not 0-3.
				j++;
				break;
			}
		}
	}

	return_str += "&+W==Skills==&N";

	if (!found)
	{
		return_str += "\nNo entries found.\n";
		return return_str;
	}

	// List spells( class, spec )
	return_str += list_skills(i, j);

	return return_str;
}

string wiki_multiclass(string /*title*/)
{
	string return_str;
	int i, j, k;
	bool found;

	return_str = "\n&+W==Multiclass options==&N\nHere are the options available to each class:";
	for (i = 1; i <= CLASS_COUNT; i++)
	{
		found = FALSE;
		for (j = 0; j < 5; j++)
		{
			// allowed_secondary_classes ends with a -1.
			if (allowed_secondary_classes[i][j] == -1)
				break;
			if (!found)
			{
				return_str += "\n\r* ";
				return_str += pad_ansi(class_names_table[i].ansi, 12);
				return_str += "&n: ";
			}
			else
			{
				return_str += ", ";
			}
			return_str +=
				class_names_table[flag2idx(allowed_secondary_classes[i][j])].ansi;
			return_str += "&n";
			found = TRUE;
		}
	}

	// This works, but we're missing a lotta multiclass names?
	return_str += "\n\r\n\r&+W==Multi-Class Names==&N";

	// For each class (skipping CLASS_NONE)..
	for (i = 1; i <= CLASS_COUNT; i++)
	{
		// For each allowed secondary class
		for (j = 0; j < 5; j++)
		{
			// allowed_secondary_classes ends with a -1.
			if (allowed_secondary_classes[i][j] == -1)
			{
				break;
			}
			// i : 1 (warrior), j : 0, allowed_secondary_classes[i][j] : CLASS_MERCENARY
			// Find the corresponding multi-class name..
			for (k = 0; multiclass_names[k].cls1 != -1; k++)
			{
				// If cls1 and cls2 match..
				if (((multiclass_names[k].cls1 == (1 << (i - 1))) &&
				     (multiclass_names[k].cls2 ==
				      allowed_secondary_classes[i][j])) ||
				    ((multiclass_names[k].cls2 == (1 << (i - 1)) &&
				      (multiclass_names[k].cls1 ==
				       allowed_secondary_classes[i][j]))))
				{
					return_str += "\n\r* ";
					return_str += multiclass_names[k].mc_name;
					return_str += "&n: ";
					return_str += class_names_table[i].ansi;
					return_str += "&n / ";
					return_str +=
						class_names_table
							[flag2idx(allowed_secondary_classes[i][j])]
								.ansi;
					return_str += "&n";
					break;
				}
			}
			if (multiclass_names[k].cls1 == -1)
			{
				return_str += "\n\r&+RMulticlass: &n";
				return_str += "&n not found!";
			}
		}
	}
	return_str += "\n\r";

	return return_str;
}

string wiki_pcraces(string /*title*/)
{
	string result;
	for (const char *faction : { "good", "evil", "neutral" })
	{
		result += !strcmp(faction, "good") ? "\n&+Y==Good Races==&N\n" :
			  !strcmp(faction, "evil") ? "\n&+R==Evil Races==&N\n" :
						     "\n&+W==Neutral Races==&N\n";
		bool found = false;
		for (int i = 0; playable_races[i].race_id != -1; ++i)
		{
			const int race = playable_races[i].race_id;
			if (strcmp(playable_races[i].faction, faction) || !creation_help_race(race))
				continue;
			result += "* " + colored_race_name(race) + "&N\n";
			found = true;
		}
		if (!found)
			result += "None currently offered.\n";
	}
	result += "\n&+W==Restricted races==&N\n";
	result +=
		creation_all_races_enabled() ?
			"These additional races are currently offered at character creation.\n" :
			"These races require progression or are unavailable at character creation.\n";
	for (int i = 0; restricted_races[i].race_id != -1; ++i)
	{
		const int race = restricted_races[i].race_id;
		if (!creation_race_enabled(race))
			continue;
		result += "* " + colored_race_name(race) + "&N: " + restricted_races[i].note + "\n";
	}
	return result;
}
