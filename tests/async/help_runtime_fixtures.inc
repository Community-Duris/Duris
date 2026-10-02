// Isolated engine boundaries for the actual help renderer. Name/color tables,
// creation configuration, and specialization admission are linked from production.
#include "core/prototypes.h"
#include "cmd/help_cache.h"
#include "flatfile/flatfile_help_catalog.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <strings.h>

extern const struct race_names race_names_table[];
extern const struct class_names class_names_table[];
extern const char *specdata[][MAX_SPEC];

static std::map<string, float> help_properties;
static int help_list_version = 1;

void logit(const char *, const char *, ...) {}
void debug(const char *, ...) {}

static void load_help_properties()
{
	std::ifstream input("lib/duris.properties");
	string line;
	while (std::getline(input, line))
	{
		const auto equals = line.find('=');
		if (equals == string::npos || line.empty() || line.front() == '#')
			continue;
		try
		{
			help_properties[line.substr(0, equals)] =
				std::stof(line.substr(equals + 1));
		}
		catch (const std::exception &)
		{
			// This fixture uses only numeric properties.
		}
	}
}

float get_property(const char *key, double fallback)
{
	const auto found = help_properties.find(key);
	return found == help_properties.end() ? fallback : found->second;
}

int get_property(const char *key, int fallback)
{
	return static_cast<int>(get_property(key, static_cast<double>(fallback)));
}

string strip_ansi(const char *text)
{
	string result;
	for (size_t i = 0; text[i]; ++i)
	{
		if (text[i] == '&' && text[i + 1])
		{
			if (text[i + 1] == '+' && text[i + 2])
				i += 2;
			else
				++i;
		}
		else
			result += text[i];
	}
	return result;
}

int ansi_strlen(const char *text)
{
	return strip_ansi(text).size();
}

string pad_ansi(const char *text, int width, bool /*trim_to_length*/)
{
	return string(text) + string(std::max(0, width - ansi_strlen(text)), ' ');
}

int flag2idx(int flag)
{
	int index = 0;
	while (flag > 0)
	{
		++index;
		flag >>= 1;
	}
	return index;
}

const char *stat_to_string_damage_pulse(float value)
{
	static string result;
	result = "pulse " + std::to_string(value);
	return result.c_str();
}

const char *stat_to_string_spell_pulse(float value)
{
	return stat_to_string_damage_pulse(value);
}

string list_innates(int race, int cls, int spec)
{
	return "&+cLIVE_INNATES " + std::to_string(race) + "/" + std::to_string(cls) + "/" +
	       std::to_string(spec) + " v" + std::to_string(help_list_version) + "&N\n";
}

string list_skills(int cls, int spec)
{
	return "\n&+cLIVE_SKILLS " + std::to_string(cls) + "/" + std::to_string(spec) + " v" +
	       std::to_string(help_list_version) + "&N\n";
}

string list_spells(int cls, int spec)
{
	return "\n&+cLIVE_SPELLS " + std::to_string(cls) + "/" + std::to_string(spec) + " v" +
	       std::to_string(help_list_version) + "&N\n";
}

string list_songs(int cls, int spec)
{
	return "\n&+cLIVE_SONGS " + std::to_string(cls) + "/" + std::to_string(spec) + " v" +
	       std::to_string(help_list_version) + "&N\n==Instruments==\n&+charp&N\n";
}

#ifndef __NO_MYSQL__
static help_catalog help_pages;

const help_catalog *help_cache_get()
{
	if (help_pages.empty())
	{
		flatfile_help_catalog catalog;
		string error;
		if (!flatfile_help_catalog_load(".", &catalog, &error))
			return nullptr;
		for (const auto &entry : catalog.entries)
			help_pages.push_back(
				{ { entry.title, entry.text, "0", "2026-10-02", "Fixture" } });
	}
	return &help_pages;
}

bool help_title_equal(const string &left, const string &right)
{
	return strcasecmp(left.c_str(), right.c_str()) == 0;
}

bool help_title_matches(const string &title, const string &query)
{
	return std::search(title.begin(), title.end(), query.begin(), query.end(),
			   [](unsigned char left, unsigned char right)
			   { return tolower(left) == tolower(right); }) != title.end();
}
#endif
