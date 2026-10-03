/*
 * Shared formatting for readable flag definitions.
 */

#include "core/prototypes.h"
#include "core/structs.h"

#include <stdio.h>
#include <string.h>

/* buf is caller-owned and every caller provides a MAX_STRING_LENGTH buffer. */
void concat_which_flagsde(const char *flagType, const flagDef flagNames[], char *buf)
{
	int j;

	strcat(buf, flagType);
	strcat(buf, ":\n\n");

	for (j = 0; flagNames[j].flagShort != NULL; j++)
	{
		if (j && !(j % 3))
			strcat(buf, "\n");

		snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "%-20s",
			 flagNames[j].flagShort);
	}

	strcat(buf, "\n\n");
}
