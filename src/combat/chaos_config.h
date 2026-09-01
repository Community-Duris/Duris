#ifndef CHAOS_CONFIG_H
#define CHAOS_CONFIG_H

#include <stdbool.h>

/* CHAOS_MUD is enabled only by the exact .env value TRUE. */
bool chaos_mud_enabled(void);

/* CHAOS_EQ_PROFILE accepts standard (default) or enhanceable. */
bool chaos_eq_use_enhanceable_profile(void);

#endif
