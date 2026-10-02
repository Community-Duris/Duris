#include "flatfile/flatfile_help_catalog.h"

#include <cstdlib>
#include <iostream>
#include <string>

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static void dump_hex(const std::string &value)
{
	const char hex[] = "0123456789abcdef";
	for (unsigned char character : value)
	{
		std::cout.put(hex[character >> 4]);
		std::cout.put(hex[character & 15]);
	}
}

int main(int argc, char **argv)
{
	require(argc == 2 || (argc == 3 && std::string(argv[2]) == "--dump"),
		"project root and optional --dump argument required");
	flatfile_help_catalog catalog;
	std::string error;
	require(flatfile_help_catalog_load(argv[1], &catalog, &error),
		"catalog load failed: " + error);
	if (argc == 3)
	{
		for (const auto &entry : catalog.entries)
		{
			dump_hex(entry.title);
			std::cout.put('\t');
			dump_hex(entry.text);
			std::cout.put('\n');
		}
		return 0;
	}
	require(catalog.entries.size() > 1500, "catalog did not load the complete help sources");

	const flatfile_help_entry *charisma = flatfile_help_catalog_find(catalog, "cHaRiSmA");
	require(charisma && charisma->text.find("major attributes") != std::string::npos,
		"case-insensitive exact lookup failed");
	const flatfile_help_entry *help = flatfile_help_catalog_find(catalog, "help advanced");
	require(help && help->text.find("SOCIETY EXAMPLES") != std::string::npos,
		"individual help source was not loaded");

	const auto matches = flatfile_help_catalog_search(catalog, "celest", 101);
	require(matches.size() > 1, "substring search did not find multiple topics");
	for (size_t index = 1; index < matches.size(); ++index)
		require(matches[index - 1]->title <= matches[index]->title,
			"search results were not title sorted");
	require(flatfile_help_catalog_search(catalog, "", 7).size() == 7,
		"search limit was not enforced");
	require(!flatfile_help_catalog_find(catalog, "definitely missing topic"),
		"missing exact lookup returned an entry");

	std::string information;
	require(flatfile_information_read(argv[1], "NeWs", &information, &error) &&
			information.find("Added 'taunt'") != std::string::npos,
		"tracked news was not available through mud_info");
	require(flatfile_information_read(argv[1], "motd", &information, &error) &&
			!information.empty(),
		"tracked motd was not available through mud_info");
	require(flatfile_information_read(argv[1], "credits", &information, &error) &&
			!information.empty(),
		"tracked credits were not available through mud_info");
	require(!flatfile_information_read(argv[1], "../unsafe", &information, &error),
		"unknown mud_info name escaped the source allow-list");

	std::cout << "flat-file help catalog passed\n";
	return 0;
}
