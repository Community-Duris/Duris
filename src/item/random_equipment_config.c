#include "core/prototypes.h"
#include "item/random_equipment_config.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct random_equipment_config defaults = { 15.0f, 8.0f, 4.0f, 1.0f, 10,	9,   50, -5,
							 5,	26,   10,   5,	  15,	35,  49, 60,
							 4,	5,    6,    8,	  20,	2,   49, 5,
							 46,	36,   30,   20,	  2.0f, 3.0f };

static struct random_equipment_config active;
static bool config_initialized;

static void reset_to_defaults(void)
{
	active = defaults;
	config_initialized = true;
}

static bool parse_long_value(const char *text, long *result)
{
	char *end;
	errno = 0;
	long value = strtol(text, &end, 10);
	if (errno || end == text || *end != '\0')
		return false;
	*result = value;
	return true;
}

static bool parse_float_value(const char *text, float *result)
{
	char *end;
	errno = 0;
	float value = strtof(text, &end);
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
		logit(LOG_STATUS, "Invalid random equipment config value %s=%s; retaining default.",
		      key, value);
		return;
	}
	*field = (int)parsed;
}

static void set_float(const char *key, const char *value, float *field, float low, float high)
{
	float parsed;
	if (!parse_float_value(value, &parsed) || parsed < low || parsed > high)
	{
		logit(LOG_STATUS, "Invalid random equipment config value %s=%s; retaining default.",
		      key, value);
		return;
	}
	*field = parsed;
}

static void apply_value(const char *key, const char *value)
{
#define FLOAT_KEY(name, field, low, high)                        \
	if (!strcmp(key, name))                                  \
	{                                                        \
		set_float(key, value, &active.field, low, high); \
		return;                                          \
	}
#define INT_KEY(name, field, low, high)                        \
	if (!strcmp(key, name))                                \
	{                                                      \
		set_int(key, value, &active.field, low, high); \
		return;                                        \
	}
	FLOAT_KEY("drop.piece.percentage", drop_piece_percentage, 0.0f, 100.0f)
	FLOAT_KEY("drop.equipment.percentage", drop_equipment_percentage, 0.0f, 100.0f)
	FLOAT_KEY("drop.luck.divisor", drop_luck_divisor, 0.1f, 100.0f)
	FLOAT_KEY("quality.level.multiplier", quality_level_multiplier, 0.0f, 5.0f)
	INT_KEY("drop.maximum.level.gap", drop_max_level_gap, 0, 100)
	INT_KEY("drop.neutral.roll.max", drop_neutral_roll_max, 0, 9999)
	INT_KEY("drop.base.chance", drop_base_chance, 0, 100)
	INT_KEY("drop.jitter.minimum", drop_jitter_min, -100, 100)
	INT_KEY("drop.jitter.maximum", drop_jitter_max, -100, 100)
	INT_KEY("drop.low_level.threshold", drop_low_level_threshold, 0, 100)
	INT_KEY("drop.low_level.bonus.max", drop_low_level_bonus_max, 0, 100)
	INT_KEY("drop.elite.bonus.minimum", drop_elite_bonus_min, -100, 100)
	INT_KEY("drop.elite.bonus.maximum", drop_elite_bonus_max, -100, 100)
	INT_KEY("stat.cap.medium.level", stat_medium_level, 0, 100)
	INT_KEY("stat.cap.high.level", stat_high_level, 0, 100)
	INT_KEY("stat.cap.elite.level", stat_elite_level, 0, 100)
	INT_KEY("stat.cap.low", stat_max_low, 1, 100)
	INT_KEY("stat.cap.medium", stat_max_medium, 1, 100)
	INT_KEY("stat.cap.high", stat_max_high, 1, 100)
	INT_KEY("stat.cap.elite", stat_max_elite, 1, 100)
	INT_KEY("stat.secondary.minimum.level", stat_secondary_min_level, 0, 100)
	INT_KEY("stat.secondary.roll.max", stat_secondary_roll_max, 0, 10000)
	INT_KEY("stat.tertiary.minimum.level", stat_tertiary_min_level, 0, 100)
	INT_KEY("stat.tertiary.roll.max", stat_tertiary_roll_max, 0, 10000)
	INT_KEY("stat.primary.divisor", stat_primary_divisor, 1, 10000)
	INT_KEY("stat.secondary.divisor", stat_secondary_divisor, 1, 10000)
	INT_KEY("stat.tertiary.divisor", stat_tertiary_divisor, 1, 10000)
	INT_KEY("stat.random.bonus.max", stat_random_bonus_max, 0, 10000)
	FLOAT_KEY("weight.base.multiplier", weight_base_multiplier, 0.0f, 100.0f)
	FLOAT_KEY("weight.divisor", weight_divisor, 0.1f, 100.0f)
#undef FLOAT_KEY
#undef INT_KEY
	logit(LOG_STATUS, "Unknown random equipment config key: %s", key);
}

void boot_random_equipment_config(void)
{
	reset_to_defaults();
	FILE *fp = fopen("lib/random_equipment.cfg", "r");
	if (!fp)
	{
		logit(LOG_STATUS, "Random equipment config unavailable; using compiled defaults.");
		return;
	}
	char line[256], key[128], value[128];
	while (fgets(line, sizeof(line), fp))
	{
		if (line[0] == '#' || line[0] == '\n')
			continue;
		if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
		{
			logit(LOG_STATUS, "Ignoring malformed random equipment config line: %s",
			      line);
			continue;
		}
		apply_value(key, value);
	}
	fclose(fp);
	if (active.drop_jitter_min > active.drop_jitter_max)
	{
		active.drop_jitter_min = defaults.drop_jitter_min;
		active.drop_jitter_max = defaults.drop_jitter_max;
	}
	if (active.drop_elite_bonus_min > active.drop_elite_bonus_max)
	{
		active.drop_elite_bonus_min = defaults.drop_elite_bonus_min;
		active.drop_elite_bonus_max = defaults.drop_elite_bonus_max;
	}
	if (!(active.stat_medium_level < active.stat_high_level &&
	      active.stat_high_level < active.stat_elite_level))
	{
		active.stat_medium_level = defaults.stat_medium_level;
		active.stat_high_level = defaults.stat_high_level;
		active.stat_elite_level = defaults.stat_elite_level;
	}
	if (!(active.stat_max_low <= active.stat_max_medium &&
	      active.stat_max_medium <= active.stat_max_high &&
	      active.stat_max_high <= active.stat_max_elite))
	{
		active.stat_max_low = defaults.stat_max_low;
		active.stat_max_medium = defaults.stat_max_medium;
		active.stat_max_high = defaults.stat_max_high;
		active.stat_max_elite = defaults.stat_max_elite;
	}
	logit(LOG_STATUS,
	      "Loaded random equipment config: piece %.2f%%, equipment %.2f%%, quality %.3fx, stat caps %d/%d/%d/%d.",
	      active.drop_piece_percentage, active.drop_equipment_percentage,
	      active.quality_level_multiplier, active.stat_max_low, active.stat_max_medium,
	      active.stat_max_high, active.stat_max_elite);
}

const struct random_equipment_config *random_equipment_config_get(void)
{
	if (!config_initialized)
		reset_to_defaults();
	return &active;
}

int random_equipment_stat_max(int mob_level)
{
	const struct random_equipment_config *config = random_equipment_config_get();
	if (mob_level > config->stat_elite_level)
		return config->stat_max_elite;
	if (mob_level > config->stat_high_level)
		return config->stat_max_high;
	if (mob_level > config->stat_medium_level)
		return config->stat_max_medium;
	return config->stat_max_low;
}
