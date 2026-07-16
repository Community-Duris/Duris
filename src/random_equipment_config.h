#ifndef _RANDOM_EQUIPMENT_CONFIG_H_
#define _RANDOM_EQUIPMENT_CONFIG_H_

struct random_equipment_config
{
	float drop_piece_percentage;
	float drop_equipment_percentage;
	float drop_luck_divisor;
	float drop_hardcore_multiplier;
	float quality_level_multiplier;
	int drop_max_level_gap;
	int drop_neutral_roll_max;
	int drop_base_chance;
	int drop_jitter_min;
	int drop_jitter_max;
	int drop_low_level_threshold;
	int drop_low_level_bonus_max;
	int drop_elite_bonus_min;
	int drop_elite_bonus_max;
	int stat_medium_level;
	int stat_high_level;
	int stat_elite_level;
	int stat_max_low;
	int stat_max_medium;
	int stat_max_high;
	int stat_max_elite;
	int stat_secondary_min_level;
	int stat_secondary_roll_max;
	int stat_tertiary_min_level;
	int stat_tertiary_roll_max;
	int stat_primary_divisor;
	int stat_secondary_divisor;
	int stat_tertiary_divisor;
	int stat_random_bonus_max;
	float weight_base_multiplier;
	float weight_divisor;
};

void boot_random_equipment_config(void);
const struct random_equipment_config *random_equipment_config_get(void);
int random_equipment_stat_max(int mob_level);

#endif
