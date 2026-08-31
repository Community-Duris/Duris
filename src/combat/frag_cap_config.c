#include "core/prototypes.h"
#include "core/config.h"
#include "combat/frag_cap_config.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct frag_cap_config defaults = { 25, 25, 31, 56, 1, 0.429f, 5, 7,    7, 7,
						 7,  7,	 7,  7,	 7, 7,	    2, 2880, 2 };

static struct frag_cap_config active;
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

static void set_int(const char *key, const char *value, int *field, int low, int high)
{
	long parsed;
	if (!parse_long_value(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_STATUS, "Invalid frag-cap config value %s=%s; retaining default.", key,
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
		logit(LOG_STATUS, "Invalid frag-cap config value %s=%s; retaining default.", key,
		      value);
		return;
	}
	*field = parsed;
}

static void apply_value(const char *key, const char *value)
{
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

	INT_KEY("cap.floor.level", cap_floor_level, 1, MAXLVLMORTAL)
	INT_KEY("cap.frag.base.level", cap_frag_base_level, 1, MAXLVLMORTAL)
	INT_KEY("cap.reset.level", cap_reset_level, 1, MAXLVLMORTAL)
	INT_KEY("cap.maximum.level", cap_maximum_level, 1, MAXLVLMORTAL)
	INT_KEY("cap.level.step", cap_level_step, 1, 20)
	FLOAT_KEY("cap.frags.per.level", cap_frags_per_level, 0.001f, 1000.0f)
	INT_KEY("timer.default.days", timer_default_days, 0, 3650)
	INT_KEY("timer.circle.level.35.days", timer_circle_level_35_days, 0, 3650)
	INT_KEY("timer.circle.level.40.days", timer_circle_level_40_days, 0, 3650)
	INT_KEY("timer.circle.level.45.days", timer_circle_level_45_days, 0, 3650)
	INT_KEY("timer.level.50.days", timer_level_50_days, 0, 3650)
	INT_KEY("timer.level.51.days", timer_level_51_days, 0, 3650)
	INT_KEY("timer.level.52.days", timer_level_52_days, 0, 3650)
	INT_KEY("timer.level.53.days", timer_level_53_days, 0, 3650)
	INT_KEY("timer.level.54.days", timer_level_54_days, 0, 3650)
	INT_KEY("timer.level.55.days", timer_level_55_days, 0, 3650)
	INT_KEY("hardcore.levels.beyond.cap", hardcore_levels_beyond_cap, 0, 56)
	INT_KEY("boon.duration.minutes", boon_duration_minutes, 0, 100000)
	INT_KEY("boon.bonus", boon_bonus, 0, 100)

#undef INT_KEY
#undef FLOAT_KEY
	logit(LOG_STATUS, "Unknown frag-cap config key: %s", key);
}

void boot_frag_cap_config(void)
{
	FILE *fp;
	char line[256], key[128], value[128];

	reset_to_defaults();
	fp = fopen("lib/frag_cap.cfg", "r");
	if (!fp)
	{
		logit(LOG_STATUS, "Frag-cap config unavailable; using compiled defaults.");
		return;
	}

	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
		{
			logit(LOG_STATUS, "Ignoring malformed frag-cap config line: %s", line);
			continue;
		}
		apply_value(key, value);
	}
	fclose(fp);

	if (active.cap_floor_level > active.cap_frag_base_level ||
	    active.cap_frag_base_level > active.cap_reset_level ||
	    active.cap_reset_level > active.cap_maximum_level || active.cap_level_step <= 0 ||
	    active.cap_frags_per_level <= 0.0f)
	{
		logit(LOG_STATUS, "Invalid frag-cap level ordering; restoring level defaults.");
		active.cap_floor_level = defaults.cap_floor_level;
		active.cap_frag_base_level = defaults.cap_frag_base_level;
		active.cap_reset_level = defaults.cap_reset_level;
		active.cap_maximum_level = defaults.cap_maximum_level;
		active.cap_level_step = defaults.cap_level_step;
		active.cap_frags_per_level = defaults.cap_frags_per_level;
	}

	logit(LOG_STATUS,
	      "Loaded frag-cap config: floor %d, frag base %d, reset %d, ceiling %d, step %d, %.3f frags/level, default timer %d days, circle timers %d/%d/%d days, level 50-55 timers %d/%d/%d/%d/%d/%d, hardcore allowance %d.",
	      active.cap_floor_level, active.cap_frag_base_level, active.cap_reset_level,
	      active.cap_maximum_level, active.cap_level_step, active.cap_frags_per_level,
	      active.timer_default_days, active.timer_circle_level_35_days,
	      active.timer_circle_level_40_days, active.timer_circle_level_45_days,
	      active.timer_level_50_days, active.timer_level_51_days, active.timer_level_52_days,
	      active.timer_level_53_days, active.timer_level_54_days, active.timer_level_55_days,
	      active.hardcore_levels_beyond_cap);
}

const struct frag_cap_config *frag_cap_config_get(void)
{
	if (!config_initialized)
		reset_to_defaults();
	return &active;
}

int frag_cap_config_cap_level_from_frags(double frags)
{
	const struct frag_cap_config *config = frag_cap_config_get();
	int level = (int)(frags / config->cap_frags_per_level) + config->cap_frag_base_level;
	if (level < config->cap_frag_base_level)
		return config->cap_frag_base_level;
	if (level > config->cap_maximum_level)
		return config->cap_maximum_level;
	return level;
}

double frag_cap_config_frags_for_level(int level)
{
	const struct frag_cap_config *config = frag_cap_config_get();
	if (level <= config->cap_frag_base_level)
		return 0.0;
	return (double)(level - config->cap_frag_base_level) * config->cap_frags_per_level;
}

int frag_cap_config_timer_days(int old_level)
{
	const struct frag_cap_config *config = frag_cap_config_get();
	switch (old_level)
	{
	case 35:
		return config->timer_circle_level_35_days;
	case 40:
		return config->timer_circle_level_40_days;
	case 45:
		return config->timer_circle_level_45_days;
	case 50:
		return config->timer_level_50_days;
	case 51:
		return config->timer_level_51_days;
	case 52:
		return config->timer_level_52_days;
	case 53:
		return config->timer_level_53_days;
	case 54:
		return config->timer_level_54_days;
	case 55:
		return config->timer_level_55_days;
	default:
		return config->timer_default_days;
	}
}

int frag_cap_config_reset_level(void)
{
	return frag_cap_config_get()->cap_reset_level;
}

int frag_cap_config_reset_timer_days(void)
{
	return frag_cap_config_get()->timer_default_days;
}

int frag_cap_config_hardcore_level_cap(int normal_cap)
{
	int level_cap = normal_cap + frag_cap_config_get()->hardcore_levels_beyond_cap;
	return (level_cap > MAXLVLMORTAL) ? MAXLVLMORTAL : level_cap;
}

int frag_cap_config_boon_duration_minutes(void)
{
	return frag_cap_config_get()->boon_duration_minutes;
}

int frag_cap_config_boon_bonus(void)
{
	return frag_cap_config_get()->boon_bonus;
}
