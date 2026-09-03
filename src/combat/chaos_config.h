#ifndef CHAOS_CONFIG_H
#define CHAOS_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

/* CHAOS_MUD is enabled only by the exact .env value TRUE. */
bool chaos_mud_enabled(void);

/* Production cannot enable local-only Chaos or creation overrides. */
bool chaos_config_validate_environment(char *error, size_t error_size);

/* CHAOS_EQ_PROFILE accepts standard (default) or enhanceable. */
bool chaos_eq_use_enhanceable_profile(void);

/* New-character Chaos starter grants.  Every accessor also requires CHAOS_MUD. */
bool chaos_starter_bonuses_enabled(void);
bool chaos_starter_frigate_enabled(void);
bool chaos_starter_epic_skills_enabled(void);
bool chaos_starter_epic_points_enabled(void);
bool chaos_starter_bank_platinum_enabled(void);
bool chaos_starter_materials_enabled(void);

/* Local integration-test commands require an explicit, exact opt-in. */
bool chaos_test_commands_enabled(void);

#endif
