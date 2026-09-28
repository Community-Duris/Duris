#ifndef DURIS_SQL_ACCOUNT_H_INCLUDED
#define DURIS_SQL_ACCOUNT_H_INCLUDED

#include "core/structs.h"
#include "economy/account_bank_balances.h"

struct acct_ip;

bool sql_save_account(struct acct_entry *acc);
struct acct_entry *sql_load_account(const char *name);
int sql_repair_account_character_projection(const char *account_name);
bool sql_account_exists(const char *name);
bool sql_delete_account(const char *name);

bool sql_load_account_bank(const char *account_name, int racewar, P_char ch);
long long sql_account_bank_deposit(const char *account_name, int racewar, int coin_type,
				   int amount);
bool sql_account_bank_deposit_balances(const char *account_name, int racewar,
				       const AccountBankBalances *amounts,
				       AccountBankBalances *committed);
long long sql_account_bank_withdraw(const char *account_name, int racewar, int coin_type,
				    int amount);
int sql_account_bank_withdraw_value(const char *account_name, int racewar, int amount,
				    AccountBankBalances *committed, int *change);
bool sql_ensure_account_bank(const char *account_name, int racewar);

bool sql_save_account_ips(const char *account_name, struct acct_ip *ips);
struct acct_ip *sql_load_account_ips(const char *account_name);
bool sql_delete_account_ips(const char *account_name);

#endif // DURIS_SQL_ACCOUNT_H_INCLUDED
