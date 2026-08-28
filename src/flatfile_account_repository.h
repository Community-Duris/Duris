#ifndef DURIS_FLATFILE_ACCOUNT_REPOSITORY_H
#define DURIS_FLATFILE_ACCOUNT_REPOSITORY_H

#include <stdint.h>

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

flatfile_account_result flatfile_account_load(const std::string &root, const std::string &name,
					      flatfile_account_record *record, std::string *error);
flatfile_account_result flatfile_account_save(const std::string &root,
					      const flatfile_account_record &record,
					      uint64_t expected_revision,
					      uint64_t *committed_revision, std::string *error);
flatfile_account_result flatfile_account_exists(const std::string &root, const std::string &name,
						bool *exists, std::string *error);

#endif
