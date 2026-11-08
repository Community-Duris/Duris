#include <string>
using namespace std;

#include "cmd/wikihelp.h"

#include <cstdlib>
#include <iostream>

/** Provide the logging symbol required by the isolated help runtime. */
void logit(const char *, const char *, ...) {}

/** Exit the test harness with a diagnostic when a requirement fails. */
static void require(bool condition, const string &message)
{
	if (!condition)
	{
		cerr << message << '\n';
		exit(1);
	}
}

/** Exercise flat-file help lookup, including the supported No Locate aliases. */
int main()
{
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
	const string ferries = wiki_help("ferries");
	for (const char *name : { "WaveDancer", "Seaspray", "Stalval", "Crimson Fulgur",
				  "Ironhold Grudge", "Old Rickety Ferry", "Stromvok" })
		require(ferries.find(name) != string::npos,
			"ferry help did not render an active ferry");
	require(ferries.find("SeaStalker") == string::npos &&
			ferries.find("Fenaline") == string::npos &&
			ferries.find("Menden-on-the-Deep") != string::npos,
		"ferry help still renders obsolete routes or stop names");
	for (const char *query : { "nolocate", "no locate", "no-locate", "toggle_nolocate" })
	{
		const string no_locate = wiki_help(query);
		require(no_locate.find("introduction system") != string::npos,
			"No Locate help alias did not render the dedicated topic");
	}
	const string quick_step = wiki_help("quick step");
	for (const char *query : { "quickstep", "QuIcKsTeP" })
		require(wiki_help(query) == quick_step &&
				quick_step.find("powerful blow") != string::npos,
			"Quickstep did not redirect to the full Quick Step topic");
	for (const char *query : { "seadog", "SeAdOg" })
	{
		const string seadog = wiki_help(query);
		for (const char *meaning :
		     { "passive seafaring", "owner with Seadog", "anywhere aboard",
		       "person operating the panel", "bonus will not persist", "10% more proceeds",
		       "Humans and Orcs", "level 1", "Swashbucklers", "level 30" })
			require(seadog.find(meaning) != string::npos,
				"Seadog did not render its ownership, cargo and availability rules");
	}
	cout << "flat-file help runtime passed\n";
	return 0;
}
