#ifndef _HARDCORE_CONFIG_H_
#define _HARDCORE_CONFIG_H_

#include <stdbool.h>

struct hardcore_config
{
    bool creation_enabled;
    bool creation_veterans_only;
    int death_max_count;
    bool death_count_arena_deaths;
    bool death_permadeath;
    bool death_record_killer;
    bool death_hall_of_fame;
    bool death_messages_enabled;
    bool level_exp_bypass_property_cap;
    int level_loss_protected_at;
    int bonus_hp_per_level;
    float bonus_healing_multiplier;
    float bonus_damage_outgoing_multiplier;
    float bonus_damage_incoming_multiplier;
    int bonus_mass_heal_base;
    float bonus_skill_notch_multiplier;
    float bonus_random_equipment_multiplier;
    int score_level_points;
    int score_experience_divisor;
    int score_frag_points;
    int score_multiclass_multiplier;
    int score_killer_bonus;
    int score_death_penalty_points;
    int score_invalid_frag_threshold;
    bool disable_in_ctf;
    bool disable_in_chaos;
    int score_display_divisor;
};

void boot_hardcore_config(void);
const struct hardcore_config *hardcore_config_get(void);
bool hardcore_config_death_is_final(int counted_deaths);
bool hardcore_config_level_loss_allowed(int level);

#endif
