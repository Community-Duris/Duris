#ifndef DURIS_FRAG_CAP_CONFIG_H
#define DURIS_FRAG_CAP_CONFIG_H

struct frag_cap_config
{
    int cap_minimum_level;
    int cap_reset_level;
    int cap_maximum_level;
    int cap_level_step;
    float cap_frags_per_level;
    int timer_first_days;
    int timer_level_26_29_days;
    int timer_level_30_39_days;
    int timer_level_40_49_days;
    int timer_level_50_56_days;
    int boon_duration_minutes;
    int boon_bonus;
};

void boot_frag_cap_config(void);
const struct frag_cap_config *frag_cap_config_get(void);
int frag_cap_config_cap_level_from_frags(double frags);
double frag_cap_config_frags_for_level(int level);
int frag_cap_config_timer_days(int old_level);
int frag_cap_config_reset_level(void);
int frag_cap_config_first_timer_days(void);
int frag_cap_config_boon_duration_minutes(void);
int frag_cap_config_boon_bonus(void);

#endif
