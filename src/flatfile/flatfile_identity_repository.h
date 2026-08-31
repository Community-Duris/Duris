#ifndef DURIS_FLATFILE_IDENTITY_REPOSITORY_H
#define DURIS_FLATFILE_IDENTITY_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

struct flatfile_identity_record
{
	uint64_t catalog_revision = 0;
	int32_t pid = 0;
	std::string name;
	std::string account;
	uint64_t login_count = 0;
	int64_t last_login = 0;
	bool blocked = false;
	bool active = false;
	int8_t racewar = 0;
	int32_t level = 0;
	int32_t race = 0;
	uint32_t primary_class = 0;
	uint32_t secondary_class = 0;
	int32_t last_room = 0;
	int64_t last_save = 0;
};

enum class flatfile_identity_result
{
	ok,
	not_found,
	conflict,
	unchanged,
	invalid,
	exhausted,
	io_error
};

class flatfile_identity_lock
{
    public:
	flatfile_identity_lock() noexcept;
	~flatfile_identity_lock();
	flatfile_identity_lock(const flatfile_identity_lock &) = delete;
	flatfile_identity_lock &operator=(const flatfile_identity_lock &) = delete;

	bool acquire(const std::string &root, std::string *error);
	bool matches(const std::string &root) const;

    private:
	struct state;
	std::unique_ptr<state> state_;
	bool owns(const std::string &root) const;
	friend flatfile_identity_result
	flatfile_identity_prepare_remove(const std::string &, const flatfile_identity_lock &,
					 const flatfile_authority_lock &, int32_t,
					 const std::string &, flatfile_authority_operation *,
					 std::string *);
	friend flatfile_identity_result
	flatfile_identity_prepare_sync_account(const std::string &, const flatfile_identity_lock &,
					       const flatfile_authority_lock &, const std::string &,
					       const std::vector<flatfile_identity_record> &,
					       flatfile_authority_operation *, std::string *);
};

flatfile_identity_result flatfile_identity_allocate_pid(const std::string &root, int32_t *pid,
							std::string *error);
flatfile_identity_result flatfile_identity_current_highest_pid(const std::string &root,
							       int32_t *pid, std::string *error);
flatfile_identity_result flatfile_identity_claim(const std::string &root, int32_t pid,
						 const std::string &name,
						 const std::string &account, std::string *error);
flatfile_identity_result flatfile_identity_lookup_name(const std::string &root,
						       const std::string &name,
						       flatfile_identity_record *record,
						       std::string *error);
flatfile_identity_result flatfile_identity_lookup_pid(const std::string &root, int32_t pid,
						      flatfile_identity_record *record,
						      std::string *error);
flatfile_identity_result
flatfile_identity_list_account(const std::string &root, const std::string &account,
			       std::vector<flatfile_identity_record> *records, std::string *error);
flatfile_identity_result
flatfile_identity_sync_account(const std::string &root, const std::string &account,
			       const std::vector<flatfile_identity_record> &records,
			       std::string *error);
/*
 * Encode the membership after-image for one authority transaction instead of
 * publishing the catalog on its own; see flatfile_account_prepare_save().
 * Returns unchanged when the catalog already matches the desired membership.
 */
flatfile_identity_result flatfile_identity_prepare_sync_account(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock, const std::string &account,
	const std::vector<flatfile_identity_record> &records,
	flatfile_authority_operation *operation, std::string *error);
flatfile_identity_result flatfile_identity_rename(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  const std::string &new_name, std::string *error);
flatfile_identity_result flatfile_identity_set_blocked(const std::string &root, int32_t pid,
						       bool blocked, std::string *error);
flatfile_identity_result flatfile_identity_remove(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  std::string *error);
flatfile_identity_result
flatfile_identity_prepare_remove(const std::string &root,
				 const flatfile_identity_lock &identity_lock,
				 const flatfile_authority_lock &authority_lock, int32_t pid,
				 const std::string &expected_name,
				 flatfile_authority_operation *operation, std::string *error);

#endif
