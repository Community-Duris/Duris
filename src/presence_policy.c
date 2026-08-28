#include "presence_policy.h"

#include "utils.h"

#include <cstdlib>
#include <cstring>

bool durisweb_private_presence_enabled(void)
{
	const char *value = getenv("DURISWEB_PRIVATE_PRESENCE");
	return value && !strcmp(value, "TRUE");
}

bool durisweb_presence_character_visible(P_char ch)
{
	return ch && IS_PC(ch) && (GET_WIZINVIS(ch) <= 0 || durisweb_private_presence_enabled());
}
