#include "prototypes.h"
#include "world/hardcore_config.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct hardcore_config defaults = { true, true,  1,      false, true,  true,  true,
						 true, true,  50,     2,     1.20f, 1.09f, 0.91f,
						 110,  1.60f, 1.50f,  1000,  10000, 100,   2,
						 100,  25,    100000, true,  true,  100 };

static struct hardcore_config active;
static bool config_initialized;

static void reset_to_defaults(void)
{
	active = defaults;
	config_initialized = true;
}

static bool parse_long_value(const char *text, long *result)
{
	char *end;
	long value;

	errno = 0;
	value = strtol(text, &end, 10);
	if (errno || end == text || *end != '\0')
		return false;
	*result = value;
	return true;
}

static bool parse_float_value(const char *text, float *result)
{
	char *end;
	float value;

	errno = 0;
	value = strtof(text, &end);
	if (errno || end == text || *end != '\0' || !isfinite(value))
		return false;
	*result = value;
	return true;
}

static bool parse_bool_value(const char *text, bool *result)
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

static void set_int(const char *key, const char *value, int *field, int low, int high)
{
	long parsed;

	if (!parse_long_value(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_STATUS, "Invalid hardcore config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = (int)parsed;
}

static void set_float(const char *key, const char *value, float *field, float low, float high)
{
	float parsed;

	if (!parse_float_value(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_STATUS, "Invalid hardcore config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = parsed;
}

static void set_bool(const char *key, const char *value, bool *field)
{
	bool parsed;

	if (!parse_bool_value(value, &parsed))
	{
		logit(LOG_STATUS, "Invalid hardcore config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = parsed;
}

static void apply_value(const char *key, const char *value)
{
#define BOOL_KEY(name, field)                        \
	if (!strcmp(key, name))                      \
	{                                            \
		set_bool(key, value, &active.field); \
		return;                              \
	}
#define INT_KEY(name, field, low, high)                        \
	if (!strcmp(key, name))                                \
	{                                                      \
		set_int(key, value, &active.field, low, high); \
		return;                                        \
	}
#define FLOAT_KEY(name, field, low, high)                        \
	if (!strcmp(key, name))                                  \
	{                                                        \
		set_float(key, value, &active.field, low, high); \
		return;                                          \
	}
	BOOL_KEY("creation.enabled", creation_enabled)
	BOOL_KEY("creation.veterans.only", creation_veterans_only)
	INT_KEY("death.max.count", death_max_count, 1, 100)
	BOOL_KEY("death.count.arena.deaths", death_count_arena_deaths)
	BOOL_KEY("death.permadeath", death_permadeath)
	BOOL_KEY("death.record.killer", death_record_killer)
	BOOL_KEY("death.hall.of.fame", death_hall_of_fame)
	BOOL_KEY("death.messages.enabled", death_messages_enabled)
	BOOL_KEY("level.exp.bypass.property.cap", level_exp_bypass_property_cap)
	INT_KEY("level.loss.protected.at", level_loss_protected_at, 1, 56)
	INT_KEY("bonus.hp.per.level", bonus_hp_per_level, 0, 100)
	FLOAT_KEY("bonus.healing.multiplier", bonus_healing_multiplier, 0.0f, 10.0f)
	FLOAT_KEY("bonus.damage.outgoing.multiplier", bonus_damage_outgoing_multiplier, 0.0f, 10.0f)
	FLOAT_KEY("bonus.damage.incoming.multiplier", bonus_damage_incoming_multiplier, 0.0f, 10.0f)
	INT_KEY("bonus.mass.heal.base", bonus_mass_heal_base, 0, 10000)
	FLOAT_KEY("bonus.skill.notch.multiplier", bonus_skill_notch_multiplier, 0.0f, 10.0f)
	FLOAT_KEY("bonus.random.equipment.multiplier", bonus_random_equipment_multiplier, 0.0f,
		  10.0f)
	INT_KEY("score.level.points", score_level_points, 0, 1000000)
	INT_KEY("score.experience.divisor", score_experience_divisor, 1, 100000000)
	INT_KEY("score.frag.points", score_frag_points, 0, 1000000)
	INT_KEY("score.multiclass.multiplier", score_multiclass_multiplier, 0, 100)
	INT_KEY("score.killer.bonus", score_killer_bonus, 0, 1000000)
	INT_KEY("score.death.penalty.points", score_death_penalty_points, 0, 1000000)
	INT_KEY("score.invalid.frag.threshold", score_invalid_frag_threshold, 0, 1000000000)
	BOOL_KEY("mode.disable.in.ctf", disable_in_ctf)
	BOOL_KEY("mode.disable.in.chaos", disable_in_chaos)
	INT_KEY("score.display.divisor", score_display_divisor, 1, 1000000000)
#undef BOOL_KEY
#undef INT_KEY
#undef FLOAT_KEY
	logit(LOG_STATUS, "Unknown hardcore config key: %s", key);
}

void boot_hardcore_config(void)
{
	FILE *fp;
	char line[256], key[128], value[128];

	reset_to_defaults();
	fp = fopen("lib/hardcore.cfg", "r");
	if (!fp)
	{
		logit(LOG_STATUS, "Hardcore config unavailable; using compiled defaults.");
		return;
	}
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
		{
			logit(LOG_STATUS, "Ignoring malformed hardcore config line: %s", line);
			continue;
		}
		apply_value(key, value);
	}
	fclose(fp);
	logit(LOG_STATUS,
	      "Loaded hardcore config: death limit %d, HP/level %d, healing %.2fx, damage %.2fx/%.2fx.",
	      active.death_max_count, active.bonus_hp_per_level, active.bonus_healing_multiplier,
	      active.bonus_damage_outgoing_multiplier, active.bonus_damage_incoming_multiplier);
}

const struct hardcore_config *hardcore_config_get(void)
{
	if (!config_initialized)
		reset_to_defaults();
	return &active;
}

bool hardcore_config_death_is_final(int counted_deaths)
{
	const struct hardcore_config *config = hardcore_config_get();
	return config->death_permadeath && counted_deaths >= config->death_max_count;
}

bool hardcore_config_level_loss_allowed(int level)
{
	return level < hardcore_config_get()->level_loss_protected_at;
}
