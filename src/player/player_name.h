#ifndef PLAYER_NAME_H
#define PLAYER_NAME_H

#include "core/utils.h"

static inline void normalize_player_name_case(char *name)
{
	if (!name || !*name)
		return;

	name[0] = UPPER(name[0]);
	for (char *p = name + 1; *p; ++p)
	{
		*p = LOWER(*p);
	}
}

#endif
