#ifndef DURIS_FLATFILE_ACCOUNT_ADAPTER_H
#define DURIS_FLATFILE_ACCOUNT_ADAPTER_H

#include "account/account.h"

#include <string>

P_acct flatfile_account_state_load(const char *name, std::string *error);
void flatfile_account_state_release(P_acct account);
bool flatfile_account_state_save(P_acct account, std::string *error);
enum class flatfile_account_fence_result
{
	ready,
	refused,
	pending_recovery
};
// A pending publication retains the live fence and must be resolved before
// erasure. Retry recovers and reads native authority without another revision.
flatfile_account_fence_result flatfile_account_state_fence(P_acct account, bool retry,
							   std::string *error);
bool flatfile_account_state_exists(const char *name, bool *exists, std::string *error);

#endif
