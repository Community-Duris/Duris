#ifndef DURIS_FLATFILE_ACCOUNT_REPOSITORY_H
#define DURIS_FLATFILE_ACCOUNT_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

struct flatfile_account_ip
{
	std::string hostname;
	std::string address;
	uint64_t count = 0;
};

struct flatfile_account_character
{
	int32_t pid = 0;
	std::string name;
	uint64_t login_count = 0;
	int64_t last_login = 0;
	int8_t blocked = 0;
	int8_t racewar = 0;
	int32_t level = 0;
	int32_t race = 0;
	uint32_t primary_class = 0;
	uint32_t secondary_class = 0;
	int32_t last_room = 0;
	int64_t last_save = 0;
};

struct flatfile_account_record
{
	uint64_t revision = 0;
	std::string name;
	std::string email;
	std::string password_hash;
	std::string confirmation;
	int8_t blocked = 0;
	int8_t confirmed = 0;
	int8_t confirmation_sent = 0;
	int64_t last_login = 0;
	int64_t last_good = 0;
	int64_t last_evil = 0;
	uint64_t flags[4] = {};
	std::vector<flatfile_account_ip> ips;
	std::vector<flatfile_account_character> characters;
};

enum class flatfile_account_result
{
	ok,
	not_found,
	conflict,
	invalid,
	io_error
};

/*
 * Exclusive hold on the account store, so an account after-image can be
 * prepared and committed inside one authority transaction without another
 * writer publishing a conflicting revision in between.
 */
class flatfile_account_lock
{
    public:
	flatfile_account_lock() noexcept;
	~flatfile_account_lock();
	flatfile_account_lock(const flatfile_account_lock &) = delete;
	flatfile_account_lock &operator=(const flatfile_account_lock &) = delete;

	bool acquire(const std::string &root, std::string *error);
	bool matches(const std::string &root) const;

    private:
	struct state;
	std::unique_ptr<state> state_;
};

flatfile_account_result flatfile_account_load(const std::string &root, const std::string &name,
					      flatfile_account_record *record, std::string *error);
flatfile_account_result flatfile_account_save(const std::string &root,
					      const flatfile_account_record &record,
					      uint64_t expected_revision,
					      uint64_t *committed_revision, std::string *error);
/*
 * Encode the account after-image for one authority transaction. The caller owns
 * both locks and commits the returned operation together with the identity
 * membership after-image, so account scalars and membership publish or fail as
 * one revision.
 */
flatfile_account_result
flatfile_account_prepare_save(const std::string &root, const flatfile_account_lock &account_lock,
			      const flatfile_authority_lock &authority_lock,
			      const flatfile_account_record &record, uint64_t expected_revision,
			      flatfile_authority_operation *operation, uint64_t *committed_revision,
			      std::string *error);
flatfile_account_result
flatfile_account_prepare_remove(const std::string &root, const flatfile_account_lock &account_lock,
				const flatfile_authority_lock &authority_lock,
				const std::string &name, uint64_t expected_revision,
				flatfile_authority_operation *operation, std::string *error);
flatfile_account_result flatfile_account_exists(const std::string &root, const std::string &name,
						bool *exists, std::string *error);

#endif
