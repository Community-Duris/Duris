#ifndef DURIS_FLATFILE_IDENTITY_REPOSITORY_H
#define DURIS_FLATFILE_IDENTITY_REPOSITORY_H

#include <stdint.h>

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
	invalid,
	exhausted,
	io_error
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
flatfile_identity_result flatfile_identity_rename(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  const std::string &new_name, std::string *error);
flatfile_identity_result flatfile_identity_set_blocked(const std::string &root, int32_t pid,
						       bool blocked, std::string *error);
flatfile_identity_result flatfile_identity_remove(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  std::string *error);

#endif
