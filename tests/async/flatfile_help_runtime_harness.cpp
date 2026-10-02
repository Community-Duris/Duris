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
int main(int argc, char **argv)
{
	if (argc == 2 && string(argv[1]) == "search-fixture")
	{
		const string result = wiki_help("  BULK  ");
		require(result.find("EXACT_AFTER_CAP") != string::npos,
			"exact title beyond the search cap was not rendered");
		require(result.find("aaa bulk099\n") != string::npos &&
				result.find("aaa bulk100\n") == string::npos,
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
