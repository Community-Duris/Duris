#include "combat/chaos_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
bool parse_bool_switch(const char *name, bool default_value)
{
	const char *value = getenv(name);
	if (!value || !*value)
		return default_value;
	if (!strcmp(value, "TRUE"))
		return true;
	if (!strcmp(value, "FALSE"))
		return false;
	fprintf(stderr, "%s must be TRUE or FALSE; disabling this Chaos starter grant.\n", name);
	return false;
}

bool chaos_starter_feature(const char *name)
{
	return chaos_starter_bonuses_enabled() && parse_bool_switch(name, true);
}
} // namespace
bool chaos_mud_enabled(void)
{
	static int enabled = -1;
	const char *value = getenv("CHAOS_MUD");

	if (enabled >= 0)
		return enabled;
	if (!value || strcmp(value, "FALSE") == 0)
		enabled = false;
	else if (strcmp(value, "TRUE") == 0)
		enabled = true;
	else
	{
		fprintf(stderr, "CHAOS_MUD must be TRUE or FALSE; disabling chaos mode.\n");
		enabled = false;
	}
	return enabled;
}

bool chaos_eq_use_enhanceable_profile(void)
{
	static int enhanceable = -1;
	const char *value = getenv("CHAOS_EQ_PROFILE");

	if (enhanceable >= 0)
		return enhanceable;
	if (!value || strcmp(value, "standard") == 0)
		enhanceable = false;
	else if (strcmp(value, "enhanceable") == 0)
		enhanceable = true;
	else
	{
		fprintf(stderr,
			"CHAOS_EQ_PROFILE must be standard or enhanceable; using standard.\n");
		enhanceable = false;
	}
	return enhanceable;
}

bool chaos_starter_bonuses_enabled(void)
{
	static int enabled = -1;
	if (enabled >= 0)
		return enabled;
	enabled = chaos_mud_enabled() && parse_bool_switch("CHAOS_STARTER_BONUSES", true);
	return enabled;
}

bool chaos_starter_frigate_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = chaos_starter_feature("CHAOS_STARTER_FRIGATE");
	return enabled;
}

bool chaos_starter_epic_skills_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = chaos_starter_feature("CHAOS_STARTER_EPIC_SKILLS");
	return enabled;
}

bool chaos_starter_epic_points_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = chaos_starter_feature("CHAOS_STARTER_EPIC_POINTS");
	return enabled;
}

bool chaos_starter_bank_platinum_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = chaos_starter_feature("CHAOS_STARTER_BANK_PLATINUM");
	return enabled;
}

bool chaos_starter_materials_enabled(void)
{
	static int enabled = -1;
	if (enabled < 0)
		enabled = chaos_starter_feature("CHAOS_STARTER_MATERIALS");
	return enabled;
}

bool chaos_test_commands_enabled(void)
{
	const char *environment = getenv("ENVIRONMENT");
	const char *value = getenv("CHAOS_TEST_COMMANDS");
	return environment && !strcmp(environment, "local") && value && !strcmp(value, "TRUE");
}
