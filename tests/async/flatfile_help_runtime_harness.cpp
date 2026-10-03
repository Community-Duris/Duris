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
				human.find("NARRATIVE_AFTER") != string::npos &&
				human.find("NARRATIVE_WEAKNESS") != string::npos,
			"captured sections were retained or authored sections were lost");
		const string warrior = wiki_help_single("Warrior");
		require(warrior.find("STALE") == string::npos &&
				warrior.find("NARRATIVE_CLASS") != string::npos &&
				warrior.find("NARRATIVE_EQUIPMENT") != string::npos,
			"plain class tables were retained or equipment narrative was lost");
		const string huntsman = wiki_help_single("Huntsman");
		require(huntsman.find("STALE") == string::npos &&
				huntsman.find("NARRATIVE_SPEC") != string::npos &&
				huntsman.find("See also: Ranger") != string::npos,
			"plain specialization lists were retained or narrative was lost");
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
	if (argc == 2 && string(argv[1]) == "index-fixture")
	{
		const string partial = wiki_help("ment");
		require(strip_ansi(partial.c_str()).find("Mentalist") != string::npos &&
				partial.find("[live; narrative missing]") != string::npos,
			"partial search omitted an exact registered topic without narrative");
		const string races = wiki_help(" INDEX races ");
		require(races.find("37 topics") != string::npos &&
				races.find(race_names_table[RACE_HUMAN].ansi) != string::npos &&
				races.find("Warrior") == string::npos,
			"race index lost its complete registry, colors or group filter");
		const string specs = wiki_help("index specs");
		require(strip_ansi(specs.c_str()).find("Mentalist") != string::npos &&
				specs.find("[live; narrative missing]") != string::npos,
			"specialization index did not label missing narrative");
		const string classes = wiki_help("index classes");
		require(classes.find(class_names_table[1].ansi) != string::npos &&
				classes.find("Human") == string::npos,
			"class index mixed race entries or lost canonical colors");
		const string first = wiki_help("index");
		const string second = wiki_help("index all 2");
		require(occurrences(first, "\n ") == 50 &&
				first.find("Next: HELP INDEX all 2") != string::npos &&
				second.find("page 2/") != string::npos,
			"index pagination failed to bound the first page or provide navigation");
		for (const char *query :
		     { "index nonsense", "index races 0", "index races -1", "index races 1x",
		       "index races 99999999999999999999999", "index all 1 extra" })
			require(wiki_help(query).find("Usage:") != string::npos,
				"invalid index request did not report its syntax");
		require(wiki_help("index races 2").find("between 1 and 1") != string::npos,
			"index accepted a nonexistent page");
		const string typo = wiki_help("huamn");
		require(typo.find("no help topics") != string::npos &&
				typo.find("HELP Human") != string::npos &&
				occurrences(typo, "&+L HELP ") <= 5 &&
				typo.find("==Racial Statistics==") == string::npos,
			"typo suggestions were missing, unbounded or automatically selected");
		require(wiki_help("xy").find("Did you mean?") == string::npos &&
				wiki_help(string(65, 'x')).find("Did you mean?") == string::npos,
			"unsupported typo queries entered fuzzy discovery");
#ifndef __NO_MYSQL__
		require(wiki_help("refreshfixture").find("no help topics") != string::npos,
			"refresh fixture was present before publication");
		help_pages.push_back(
			{ { "refreshfixture", "FRESH_NARRATIVE", "0", "today", "Fixture" } });
		++help_catalog_generation;
		require(wiki_help("refreshfixture").find("FRESH_NARRATIVE") != string::npos,
			"index did not follow a newly published generation");
		help_pages.pop_back();
		++help_catalog_generation;
		require(wiki_help("refreshfixture").find("no help topics") != string::npos,
			"index retained a removed title after refresh");
#endif
		cout << "shared discovery, generated topics, colors, pagination, typo bounds and refresh passed\n";
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
	for (const char *query : { "named equipment", "namedreport", "racewar", "introduce",
				   "refine", "soulbind", "prestige" })
		require(wiki_help_single(query).find("==Syntax==") != string::npos,
			"new help entry did not survive source precedence");
	cout << "flat-file help runtime passed\n";
	return 0;
}
