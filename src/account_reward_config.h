#ifndef _ACCOUNT_REWARD_CONFIG_H_
#define _ACCOUNT_REWARD_CONFIG_H_

#include <stdbool.h>

void boot_account_reward_config(void);
int account_reward_config_cooldown_seconds(void);
int account_reward_config_max_active_rewards(void);
bool account_reward_config_show_claim_ids(void);
bool account_reward_config_preserve_on_pwipe(void);

#endif
