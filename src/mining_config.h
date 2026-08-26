#ifndef _MINING_CONFIG_H_
#define _MINING_CONFIG_H_

/* Boot-time configuration for mine placement and rewards. */
void mining_config_boot(void);
int mining_config_region_value(int region, const char *field, int fallback);
int mining_config_gem_vnum(int mine_quality);

#endif
