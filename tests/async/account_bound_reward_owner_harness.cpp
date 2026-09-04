#include "account/account.h"
#include "account/account_reward.h"
#include "core/structs.h"

#include <cassert>

const char *get_account_name_safe(P_char ch)
{
	if (ch && ch->desc && ch->desc->account)
		return ch->desc->account->acct_name;
	return "Unknown";
}

int main()
{
	acct_entry account = {};
	descriptor_data descriptor = {};
	char_data character = {};
	obj_data reward = {};

	account.acct_name = const_cast<char *>("SharedAccount");
	descriptor.account = &account;
	character.desc = &descriptor;
	reward.extra2_flags = ITEM2_ACCOUNT_BOUND;
	reward.name = const_cast<char *>("account_reward:42:SharedAccount divine wand");

	assert(account_bound_reward_owner(&character, &reward));
	account.acct_name = const_cast<char *>("sharedaccount");
	assert(account_bound_reward_owner(&character, &reward));
	account.acct_name = const_cast<char *>("OtherAccount");
	assert(!account_bound_reward_owner(&character, &reward));

	account.acct_name = const_cast<char *>("SharedAccount");
	reward.extra2_flags = 0;
	assert(!account_bound_reward_owner(&character, &reward));
	reward.extra2_flags = ITEM2_ACCOUNT_BOUND;
	reward.name = const_cast<char *>("account_reward:SharedAccount legacy divine wand");
	assert(account_bound_reward_owner(&character, &reward));
	reward.name = const_cast<char *>("ordinary divine wand");
	assert(!account_bound_reward_owner(&character, &reward));
	assert(!account_bound_reward_owner(nullptr, &reward));
	assert(!account_bound_reward_owner(&character, nullptr));

	return 0;
}
