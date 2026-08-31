#include "flatfile/flatfile_account_adapter.h"

#include "flatfile/flatfile_account_repository.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "flatfile/flatfile_identity_repository.h"
#include "persistence/persistence_mode.h"

#include <cstdlib>
#include <new>
#include <cstring>
#include <utility>

namespace
{
char *copy_string(const std::string &value)
{
	return strdup(value.c_str());
}

void release_account(P_acct account)
{
	if (!account)
		return;
	free(account->acct_name);
	free(account->acct_email);
	free(account->acct_password);
	free(account->acct_confirmation);
	while (account->acct_unique_ips)
	{
		struct acct_ip *next = account->acct_unique_ips->next;
		free(account->acct_unique_ips->hostname);
		free(account->acct_unique_ips->ip_address);
		free(account->acct_unique_ips);
		account->acct_unique_ips = next;
	}
	while (account->acct_character_list)
	{
		struct acct_chars *next = account->acct_character_list->next;
		free(account->acct_character_list->charname);
		free(account->acct_character_list);
		account->acct_character_list = next;
	}
	free(account);
}

bool to_record(P_acct account, flatfile_account_record *record)
{
	if (!account || !record || !account->acct_name)
		return false;
	record->name = account->acct_name;
	record->email = account->acct_email ? account->acct_email : "";
	record->password_hash = account->acct_password ? account->acct_password : "";
	record->confirmation = account->acct_confirmation ? account->acct_confirmation : "";
	record->blocked = account->acct_blocked;
	record->confirmed = account->acct_confirmed;
	record->confirmation_sent = account->acct_confirmation_sent;
	record->last_login = account->acct_last;
	record->last_good = account->acct_good;
	record->last_evil = account->acct_evil;
	record->flags[0] = account->acct_flags1;
	record->flags[1] = account->acct_flags2;
	record->flags[2] = account->acct_flags3;
	record->flags[3] = account->acct_flags4;
	for (struct acct_ip *ip = account->acct_unique_ips; ip; ip = ip->next)
		record->ips.push_back({ ip->hostname ? ip->hostname : "",
					ip->ip_address ? ip->ip_address : "", ip->count });
	return true;
}

bool membership_records(P_acct account, std::vector<flatfile_identity_record> *records)
{
	if (!account || !records || !account->acct_name)
		return false;
	for (struct acct_chars *character = account->acct_character_list; character;
	     character = character->next)
	{
		if (!character->charname)
			return false;
		flatfile_identity_record value;
		value.pid = character->pid;
		value.name = character->charname;
		value.account = account->acct_name;
		value.login_count = character->count;
		value.last_login = character->last;
		value.blocked = character->blocked;
		value.active = true;
		value.racewar = character->racewar;
		value.level = character->level;
		value.race = character->race;
		value.primary_class = character->m_class;
		value.secondary_class = character->secondary_class;
		value.last_room = character->last_room;
		value.last_save = character->last_save;
		records->push_back(std::move(value));
	}
	return true;
}

P_acct from_record(const flatfile_account_record &record, std::string *error)
{
	const char *root = persistence_mode_flatfile_root();
	std::vector<flatfile_identity_record> memberships;
	if (!root || flatfile_identity_list_account(root, record.name, &memberships, error) !=
			     flatfile_identity_result::ok)
		return NULL;
	P_acct account = static_cast<P_acct>(calloc(1, sizeof(struct acct_entry)));
	if (!account)
		return NULL;
	account->acct_name = copy_string(record.name);
	account->acct_email = copy_string(record.email);
	account->acct_password = copy_string(record.password_hash);
	account->acct_confirmation = copy_string(record.confirmation);
	if (!account->acct_name || !account->acct_email || !account->acct_password ||
	    !account->acct_confirmation)
	{
		release_account(account);
		return NULL;
	}
	account->acct_blocked = record.blocked;
	account->acct_confirmed = record.confirmed;
	account->acct_confirmation_sent = record.confirmation_sent;
	account->acct_last = record.last_login;
	account->acct_good = record.last_good;
	account->acct_evil = record.last_evil;
	account->acct_flags1 = record.flags[0];
	account->acct_flags2 = record.flags[1];
	account->acct_flags3 = record.flags[2];
	account->acct_flags4 = record.flags[3];
	account->persistence_revision = record.revision;

	struct acct_ip **ip_tail = &account->acct_unique_ips;
	for (const flatfile_account_ip &source : record.ips)
	{
		struct acct_ip *ip =
			static_cast<struct acct_ip *>(calloc(1, sizeof(struct acct_ip)));
		if (!ip)
		{
			release_account(account);
			return NULL;
		}
		ip->hostname = copy_string(source.hostname);
		ip->ip_address = copy_string(source.address);
		ip->count = source.count;
		if (!ip->hostname || !ip->ip_address)
		{
			free(ip->hostname);
			free(ip->ip_address);
			free(ip);
			release_account(account);
			return NULL;
		}
		*ip_tail = ip;
		ip_tail = &ip->next;
		account->num_ips++;
	}

	struct acct_chars **character_tail = &account->acct_character_list;
	for (const flatfile_identity_record &source : memberships)
	{
		struct acct_chars *character =
			static_cast<struct acct_chars *>(calloc(1, sizeof(struct acct_chars)));
		if (!character)
		{
			release_account(account);
			return NULL;
		}
		character->pid = source.pid;
		character->charname = copy_string(source.name);
		character->count = source.login_count;
		character->last = source.last_login;
		character->blocked = source.blocked;
		character->racewar = source.racewar;
		character->level = source.level;
		character->race = source.race;
		character->m_class = source.primary_class;
		character->secondary_class = source.secondary_class;
		character->last_room = source.last_room;
		character->last_save = source.last_save;
		if (!character->charname)
		{
			free(character);
			release_account(account);
			return NULL;
		}
		*character_tail = character;
		character_tail = &character->next;
		account->num_chars++;
	}
	return account;
}
} // namespace

P_acct flatfile_account_state_load(const char *name, std::string *error)
{
	const char *root = persistence_mode_flatfile_root();
	if (!root || !name)
		return NULL;
	flatfile_account_record record;
	if (flatfile_account_load(root, name, &record, error) != flatfile_account_result::ok)
		return NULL;
	return from_record(record, error);
}

void flatfile_account_state_release(P_acct account)
{
	release_account(account);
}

bool flatfile_account_state_save(P_acct account, std::string *error)
{
	const char *root = persistence_mode_flatfile_root();
	flatfile_account_record record;
	if (!root || !to_record(account, &record))
		return false;
	std::vector<flatfile_identity_record> memberships;
	if (!membership_records(account, &memberships))
		return false;

	/*
	 * Account scalars and identity membership are two authorities; publish
	 * both after-images through one transaction so a failure between them
	 * cannot leave the account revision acknowledged on its own.
	 */
	flatfile_identity_lock identity_lock;
	if (!identity_lock.acquire(root, error))
		return false;
	flatfile_authority_lock authority_lock;
	if (!authority_lock.acquire(root, error))
		return false;
	flatfile_account_lock account_lock;
	if (!account_lock.acquire(root, error))
		return false;

	std::vector<flatfile_authority_operation> operations;
	flatfile_authority_operation account_operation;
	uint64_t committed_revision = 0;
	if (flatfile_account_prepare_save(
		    root, account_lock, authority_lock, record, account->persistence_revision,
		    &account_operation, &committed_revision, error) != flatfile_account_result::ok)
		return false;
	flatfile_authority_operation identity_operation;
	const auto identity = flatfile_identity_prepare_sync_account(
		root, identity_lock, authority_lock, account->acct_name, memberships,
		&identity_operation, error);
	if (identity != flatfile_identity_result::ok)
		return false;
	try
	{
		operations.push_back(std::move(account_operation));
		operations.push_back(std::move(identity_operation));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (flatfile_authority_transaction_commit_operations(root, authority_lock, operations,
							     error) !=
	    flatfile_authority_transaction_result::ok)
		return false;
	account->persistence_revision = committed_revision;
	return true;
}

bool flatfile_account_state_exists(const char *name, bool *exists, std::string *error)
{
	const char *root = persistence_mode_flatfile_root();
	return root && name &&
	       flatfile_account_exists(root, name, exists, error) == flatfile_account_result::ok;
}
