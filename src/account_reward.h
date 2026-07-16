#ifndef DURIS_ACCOUNT_REWARD_H
#define DURIS_ACCOUNT_REWARD_H

#define DEFAULT_ACCOUNT_REWARD_VNUM 36419
#define ACCOUNT_REWARD_ACCOUNT_MAX 50
#define ACCOUNT_REWARD_MARKER "account_reward:"

void do_divineclaim(P_char ch, char *argument, int cmd);
void account_bound_reward_on_login(P_char ch);
bool account_bound_reward_owner(P_char ch, P_obj obj);

#endif
