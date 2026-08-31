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
