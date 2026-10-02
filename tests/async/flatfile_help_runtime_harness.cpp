#include <string>
using namespace std;

#include "cmd/wikihelp.h"

#include <cstdlib>
#include <iostream>

#include "help_runtime_fixtures.inc"

/** Exit the test harness with a diagnostic when a requirement fails. */
static void require(bool condition, const string &message)
{
	if (!condition)
	{
		cerr << message << '\n';
		exit(1);
	}
}

static size_t occurrences(const string &text, const string &needle)
{
	size_t count = 0;
	for (size_t at = 0; (at = text.find(needle, at)) != string::npos; at += needle.size())
		++count;
	return count;
}

/** Exercise flat-file help lookup, including the supported No Locate aliases. */
int main(int argc, char **argv)
{
	load_help_properties();
	if (argc == 2 && string(argv[1]) == "dynamic-catalog")
	{
		for (int race = 1; race <= RACE_PLAYER_MAX; ++race)
		{
			const string page = wiki_help_single(race_names_table[race].normal);
			require(page.find(race_names_table[race].ansi) == 0,
				"tracked race topic lost its canonical color");
			require(occurrences(page, "==Racial Statistics==") == 1 &&
					occurrences(page, "==Class list==") == 1 &&
					occurrences(page, "==Innate abilities==") == 1 &&
					occurrences(page, "Last Edited:") <= 1,
				"tracked race topic has duplicate captured/live sections or headers");
		}
		for (int cls = 1; cls <= CLASS_COUNT; ++cls)
		{
			const string title = string(class_names_table[cls].normal) + " Skills";
			const string page = wiki_help_single(title);
			require(page.find(class_names_table[cls].ansi) == 0 &&
					occurrences(page, "==Skills==") == 1 &&
					occurrences(page, "==Spells==") == 1,
				"tracked class skillset lost colors or has duplicate skill/spell sections");
		}
		const string warrior = wiki_help_single("Warrior");
		require(warrior.find(class_names_table[1].ansi) == 0 &&
				occurrences(warrior, "==Allowed races==") == 1 &&
				occurrences(warrior, "==Specializations==") == 1 &&
				warrior.find(specdata[1][0]) != string::npos,
			"class help did not replace captured races/specs with colored current choices");
		require(wiki_help_single("Planetbound Illithid").find("(Planetbound Illithid)") !=
				string::npos,
			"canonical short color name concealed the full help-topic identity");
		for (int cls = 1; cls <= CLASS_COUNT; ++cls)
			for (int spec = 0; spec < MAX_SPEC; ++spec)
			{
				const string name = strip_ansi(specdata[cls][spec]);
				if (name.empty() || name == "Not Used")
					continue;
				const string page = wiki_help_single(name);
				require(page.find(specdata[cls][spec]) == 0 &&
						occurrences(page, "==Skills==") == 1 &&
						occurrences(page, "==Spells==") == 1,
					"registered specialization lost colors or has duplicate generated sections");
			}
		cout << "37 race pages, 30 class skillsets and all registered specs render live sections once\n";
		return 0;
	}
	if (argc == 2 && string(argv[1]) == "dynamic-fixture")
	{
		const string human = wiki_help_single("Human");
		require(human.find(race_names_table[RACE_HUMAN].ansi) == 0,
			"race help did not use its canonical colors");
		require(human.find("Strength    : &+c100&n") != string::npos,
			"race help did not read current properties");
		require(human.find("STALE") == string::npos &&
				human.find("NARRATIVE_AFTER") != string::npos,
			"captured sections were retained or authored sections were lost");
		require(human.find(class_names_table[1].ansi) != string::npos &&
				human.find(class_names_table[30].ansi) == string::npos,
			"class choices did not use canonical colors and current creation policy");
		help_properties["stats.str.Human"] = 137;
		help_properties["damage.pulse.racial.Human"] = 16.75;
		help_list_version = 2;
		const string changed = wiki_help_single("Human");
		require(changed.find("Strength    : &+c137&n") != string::npos &&
				changed.find("pulse 16.750000") != string::npos &&
				changed.find("LIVE_INNATES 1/0/0 v2") != string::npos,
			"race facts were cached instead of recomputed");
		setenv("CREATION_ALL_CLASSES", "TRUE", 1);
		require(wiki_help_single("Human").find(class_names_table[30].ansi) != string::npos,
			"creation overrides did not update live class choices");
		unsetenv("CREATION_ALL_CLASSES");
		require(wiki_help_single("Lich").find("not currently offered") != string::npos,
			"progression race was incorrectly offered for new characters");
		setenv("CREATION_ALL_RACES", "TRUE", 1);
		require(wiki_help_single("Lich").find(class_names_table[11].ansi) != string::npos,
			"restricted creation override did not use its current class row");
		unsetenv("CREATION_ALL_RACES");
		for (const char *query : { "Warrior Skills", "SKILL_WARRIOR" })
		{
			const string skills = wiki_help_single(query);
			require(skills.find("LIVE_SKILLS 1/0 v2") != string::npos &&
					skills.find("LIVE_SPELLS 1/0 v2") != string::npos &&
					skills.find("STALE") == string::npos &&
					skills.find("See also") != string::npos,
				"class skillset aliases did not render current lists and preserve links");
		}
		const string bard = wiki_help_single("Bard Skills");
		require(bard.find("LIVE_SONGS 16/0 v2") != string::npos &&
				bard.find("STALE") == string::npos,
			"bard songs/instruments were not replaced by the live provider");
		const string spec = wiki_help("  Mentalist  ");
		require(spec.find("not yet been authored") != string::npos &&
				spec.find("LIVE_SKILLS 29/2 v2") != string::npos,
			"missing registered specialization returned an unrelated partial match");
		require(wiki_help_single("Assassin").find("LIVE_SKILLS 13/1 v2") != string::npos,
			"current specialization did not take precedence over the retired class name");
#ifndef __NO_MYSQL__
		for (auto &page : help_pages)
		{
			if (page.fields[0] == "Assassin")
				page.fields[2] = "9";
			if (page.fields[0] == "Races")
				page.fields[2] =
					"6"; // The literal index works in any SQL category.
		}
		require(wiki_help_single("Assassin").find("LIVE_INNATES 0/14/0 v2") != string::npos,
			"explicit SQL category did not take precedence over inferred current topics");
#endif
		const string races = wiki_help_single("RACES");
		require(races.find(race_names_table[RACE_TIEFLING].ansi) != string::npos &&
				races.find("STALE") == string::npos,
			"case-insensitive race index did not reflect the current roster");
		require(wiki_help_single("Multiclass").find("STALE") == string::npos,
			"multiclass captures were not replaced");
		cout << "live properties, colors, creation choices, aliases, lists and capture replacement passed\n";
		return 0;
	}
	if (argc == 2 && string(argv[1]) == "search-fixture")
	{
		const string result = wiki_help("  BULK  ");
		require(result.find("EXACT_AFTER_CAP") != string::npos,
			"exact title beyond the search cap was not rendered");
		require(result.find("aaa bulk099&N\n") != string::npos &&
				result.find("aaa bulk100&N\n") == string::npos,
			"topic list did not respect the 100-result display limit");
		require(result.find("limited to 100 topics") != string::npos &&
				result.find("Type HELP <topic>") != string::npos,
			"bounded results did not explain how to refine or read a topic");
		require(wiki_help(" \t\r\n").find("DEFAULT_MENU") != string::npos,
			"blank help did not use the default menu");
		require(wiki_help("missing").find("HELP COMMANDS") != string::npos,
			"missing help did not suggest a recovery action");
		return 0;
	}
	const string default_help = wiki_help("");
	require(default_help.find("temporarily disabled") == string::npos &&
			default_help.find("help") != string::npos,
		"default help did not use the flat catalog");
	const string exact = wiki_help("cHaRiSmA");
	require(exact.find("major attributes") != string::npos,
		"exact help did not render flat content");
	const string multiple = wiki_help("celest");
	require(multiple.find("following help topics") != string::npos,
		"multiple-match help did not render a topic list");
	const string missing = wiki_help("definitely missing topic");
	require(missing.find("no help topics") != string::npos,
		"missing help did not preserve the user-facing result");
	for (const char *query : { "nolocate", "no locate", "no-locate", "toggle_nolocate" })
	{
		const string no_locate = wiki_help(query);
		require(no_locate.find("introduction system") != string::npos,
			"No Locate help alias did not render the dedicated topic");
	}
	cout << "flat-file help runtime passed\n";
	return 0;
}
