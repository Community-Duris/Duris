#ifndef DURIS_FLATFILE_ACCOUNT_ADAPTER_H
#define DURIS_FLATFILE_ACCOUNT_ADAPTER_H

#include "account.h"

#include <string>

P_acct flatfile_account_state_load(const char *name, std::string *error);
void flatfile_account_state_release(P_acct account);
bool flatfile_account_state_save(P_acct account, std::string *error);
bool flatfile_account_state_exists(const char *name, bool *exists, std::string *error);

#endif
