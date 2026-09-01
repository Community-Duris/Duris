#include "combat/chaos_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
