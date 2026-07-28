#ifndef _ACCOUNT_REWARD_SNAPSHOT_H_
#define _ACCOUNT_REWARD_SNAPSHOT_H_

#include "structs.h"
#include "account_reward.h"

char *account_reward_snapshot_serialize(P_obj obj);
bool account_reward_snapshot_apply(P_obj obj, const char *json, int template_version);

#endif
