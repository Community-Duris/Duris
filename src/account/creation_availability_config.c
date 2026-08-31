#include "core/prototypes.h"
#include "account/creation_availability_config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct named_id
{
	const char *name;
	int id;
};

static const struct named_id class_names[] = {
	{ "warrior", 1 },     { "ranger", 2 },	     { "psionicist", 3 },
	{ "paladin", 4 },     { "anti-paladin", 5 }, { "cleric", 6 },
	{ "monk", 7 },	      { "druid", 8 },	     { "shaman", 9 },
	{ "sorcerer", 10 },   { "necromancer", 11 }, { "conjurer", 12 },
	{ "rogue", 13 },      { "assassin", 14 },    { "mercenary", 15 },
	{ "bard", 16 },	      { "thief", 17 },	     { "warlock", 18 },
	{ "mindflayer", 19 }, { "alchemist", 20 },   { "berserker", 21 },
	{ "reaver", 22 },     { "illusionist", 23 }, { "blighter", 24 },
	{ "dreadlord", 25 },  { "ethermancer", 26 }, { "avenger", 27 },
	{ "theurgist", 28 },  { "summoner", 29 },    { "dragoon", 30 },
	{ NULL, 0 }
};

static const struct named_id race_names[] = {
	{ "human", RACE_HUMAN },
	{ "barbarian", RACE_BARBARIAN },
	{ "grey-elf", RACE_GREY },
	{ "mountain-dwarf", RACE_MOUNTAIN },
	{ "halfling", RACE_HALFLING },
	{ "gnome", RACE_GNOME },
	{ "centaur", RACE_CENTAUR },
	{ "githzerai", RACE_GITHZERAI },
	{ "firbolg", RACE_FIRBOLG },
	{ "drow", RACE_DROW },
	{ "duergar", RACE_DUERGAR },
	{ "ogre", RACE_OGRE },
	{ "troll", RACE_TROLL },
	{ "orc", RACE_ORC },
	{ "githyanki", RACE_GITHYANKI },
	{ "goblin", RACE_GOBLIN },
	{ "kobold", RACE_KOBOLD },
	{ "drider", RACE_DRIDER },
	{ "thri-kreen", RACE_THRIKREEN },
	{ "minotaur", RACE_MINOTAUR },
	{ "tiefling", RACE_TIEFLING },
	/* Restricted races: only offered when CREATION_ALL_RACES=TRUE, but still
       individually switchable here once it is on. */
	{ "lich", RACE_LICH },
	{ "vampire", RACE_PVAMPIRE },
	{ "death-knight", RACE_PDKNIGHT },
	{ "wight", RACE_WIGHT },
	{ "revenant", RACE_REVENANT },
	{ "shadow-beast", RACE_PSBEAST },
	{ "phantom", RACE_PHANTOM },
	{ "shade", RACE_SHADE },
	{ "half-elf", RACE_HALFELF },
	{ "wood-elf", RACE_WOODELF },
	{ "kuo-toa", RACE_KUOTOA },
	{ "orog", RACE_OROG },
	{ "harpy", RACE_HARPY },
	{ "illithid", RACE_ILLITHID },
	{ "planetbound-illithid", RACE_PILLITHID },
	{ "storm-giant", RACE_SGIANT },
	{ NULL, 0 }
};

static bool class_enabled[CLASS_COUNT + 1];
static bool race_enabled[LAST_RACE + 1];
static bool initialized;

static bool parse_bool(const char *text, bool *result)
{
	if (!strcmp(text, "true") || !strcmp(text, "1"))
	{
		*result = true;
		return true;
	}
	if (!strcmp(text, "false") || !strcmp(text, "0"))
	{
		*result = false;
		return true;
	}
	return false;
}

static bool extract_name(const char *key, const char *prefix, char *name, size_t name_size)
{
	const char *start;
	const char *suffix = ".enabled";
	size_t name_length;

	if (strncmp(key, prefix, strlen(prefix)) != 0)
		return false;
	start = key + strlen(prefix);
	if (strlen(start) <= strlen(suffix) ||
	    strcmp(start + strlen(start) - strlen(suffix), suffix) != 0)
		return false;
	name_length = strlen(start) - strlen(suffix);
	if (name_length >= name_size)
		return false;
	memcpy(name, start, name_length);
	name[name_length] = '\0';
	return true;
}

static const struct named_id *find_named_id(const struct named_id *entries, const char *name)
{
	for (; entries->name; entries++)
		if (!strcmp(entries->name, name))
			return entries;
	return NULL;
}

static void apply_value(const char *key, const char *value)
{
	const struct named_id *entry;
	bool enabled;
	char name[64];

	if (!parse_bool(value, &enabled))
	{
		logit(LOG_STATUS, "Invalid creation availability value %s=%s; retaining default.",
		      key, value);
		return;
	}

	if (extract_name(key, "creation.class.", name, sizeof(name)))
	{
		entry = find_named_id(class_names, name);
		if (entry)
			class_enabled[entry->id] = enabled;
		else
			logit(LOG_STATUS, "Unknown creation class: %s", name);
		return;
	}
	if (extract_name(key, "creation.race.", name, sizeof(name)))
	{
		entry = find_named_id(race_names, name);
		if (entry)
			race_enabled[entry->id] = enabled;
		else
			logit(LOG_STATUS, "Unknown creation race: %s", name);
		return;
	}
	logit(LOG_STATUS, "Unknown creation availability key: %s", key);
}

void boot_creation_availability_config(void)
{
	FILE *fp;
	char line[256], key[128], value[128];
	int i;

	for (i = 0; i <= CLASS_COUNT; i++)
		class_enabled[i] = true;
	for (i = 0; i <= LAST_RACE; i++)
		race_enabled[i] = true;
	initialized = true;

	fp = fopen("lib/creation_availability.cfg", "r");
	if (!fp)
	{
		logit(LOG_STATUS, "Creation availability config unavailable; using defaults.");
		return;
	}
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
		{
			logit(LOG_STATUS, "Ignoring malformed creation availability line: %s",
			      line);
			continue;
		}
		apply_value(key, value);
	}
	fclose(fp);
	logit(LOG_STATUS, "Loaded creation availability config; Dragoon enabled=%s.",
	      class_enabled[30] ? "true" : "false");
}

bool creation_class_enabled(int class_id)
{
	if (!initialized)
		boot_creation_availability_config();
	return class_id > 0 && class_id <= CLASS_COUNT &&
	       (creation_all_classes_enabled() || class_enabled[class_id]);
}

bool creation_race_enabled(int race_id)
{
	if (!initialized)
		boot_creation_availability_config();
	return race_id >= 0 && race_id <= LAST_RACE && race_enabled[race_id];
}

/*
 * CREATION_ALL_RACES=TRUE (.env) opens the normally unavailable player races
 * -- the descend-only undead forms and the other non-roster races -- to
 * character creation.  Intended for local testing; off unless set.
 */
bool creation_all_races_enabled(void)
{
	const char *value = getenv("CREATION_ALL_RACES");

	return value && strcasecmp(value, "TRUE") == 0;
}

/*
 * CREATION_ALL_CLASSES=TRUE (.env) opens every defined player class to
 * character creation, including classes normally reached through progression
 * and classes disabled in creation_availability.cfg.
 */
bool creation_all_classes_enabled(void)
{
	const char *value = getenv("CREATION_ALL_CLASSES");

	return value && strcasecmp(value, "TRUE") == 0;
}

static int normal_creation_class_align(int race_id, int class_id)
{
	extern int class_table[LAST_RACE + 1][CLASS_COUNT + 1];
	int i;

	if (race_id < 0 || race_id > LAST_RACE || class_id < 0 || class_id > CLASS_COUNT)
		return 5;

	if (creation_all_races_enabled())
	{
		for (i = 0; restricted_class_rows[i].race_id != -1; i++)
		{
			if (restricted_class_rows[i].race_id == race_id)
				return restricted_class_rows[i].align[class_id];
		}
	}

	return class_table[race_id][class_id];
}

bool creation_class_normally_available(int race_id, int class_id)
{
	if (!initialized)
		boot_creation_availability_config();

	return class_id > 0 && class_id <= CLASS_COUNT && class_enabled[class_id] &&
	       normal_creation_class_align(race_id, class_id) != 5;
}

/*
 * Alignment code for a race/class pair as character creation should see it:
 * class_table[] normally, but a restricted race whose class_table row is empty
 * uses its restricted_class_rows[] stand-in while CREATION_ALL_RACES is on.
 * Creation paths only -- random mob generation keeps reading class_table[].
 */
int creation_class_align(int race_id, int class_id)
{
	int align = normal_creation_class_align(race_id, class_id);

	if (align == 5 && creation_all_classes_enabled() && race_id >= 0 && race_id <= LAST_RACE &&
	    class_id > 0 && class_id <= CLASS_COUNT)
		return 2;

	return align;
}
