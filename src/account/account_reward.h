#ifndef DURIS_ACCOUNT_REWARD_H
#define DURIS_ACCOUNT_REWARD_H

#define DEFAULT_ACCOUNT_REWARD_VNUM 36419
#define ACCOUNT_REWARD_ACCOUNT_MAX 50
#define ACCOUNT_REWARD_MARKER "account_reward:"
#define ACCOUNT_REWARD_TEMPLATE_VERSION 1

#include "item/item_transfer_command.h"

void do_divineclaim(P_char ch, char *argument, int cmd);
void account_bound_reward_on_login(P_char ch);
void account_bound_reward_prepare_player_corpse(P_char ch, P_obj corpse);
bool account_bound_reward_owner(P_char ch, P_obj obj);
bool account_bound_rewards_on_successful_pwipe(void);
bool account_reward_retirement_publication(const critical_operation_id &operation_id, P_char actor,
					   bool committed, const item_transfer_result &result,
					   unsigned int error_code, const uint8_t *context,
					   size_t context_size);
bool account_reward_duplicate_promotion_publication(const critical_operation_id &operation_id,
						    P_char actor, bool committed,
						    const item_transfer_result &result,
						    unsigned int error_code, const uint8_t *context,
						    size_t context_size);
void account_reward_retirement_completion(P_char actor, bool committed,
					  const item_transfer_result &result,
					  unsigned int error_code, const uint8_t *context,
					  size_t context_size);

#endif
