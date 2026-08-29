#ifndef DURIS_FLATFILE_PLAYER_DOMAIN_REPOSITORY_H
#define DURIS_FLATFILE_PLAYER_DOMAIN_REPOSITORY_H

#include "flatfile_authority_transaction.h"
#include "player_load_repository.h"
#include "critical_command_coordinator.h"
#include "currency_command.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_player_domain_record
{
	int32_t pid = 0;
	std::string account_name;
	int8_t racewar = 0;
	player_load_domain_state domains = {};
	std::vector<int64_t> recent_pvp_deaths;
	std::vector<int32_t> completed_epic_zones;
};

enum class flatfile_player_domain_result
{
	ok,
	not_found,
	conflict,
	invalid,
	io_error
};

struct flatfile_wallet_mutation
{
	currency_vector wallet = {};
	currency_vector bank = {};
	uint64_t wallet_revision = 0;
	uint64_t bank_revision = 0;
	std::vector<flatfile_authority_after_image> after_images;
};

struct flatfile_base_stat_mutation
{
	int16_t stat_value = 0;
	uint64_t stat_revision = 0;
	flatfile_authority_after_image after_image;
};

flatfile_player_domain_result
flatfile_player_domain_establish(const std::string &root,
				 const flatfile_player_domain_record &record, std::string *error);
flatfile_player_domain_result flatfile_player_domain_establish_initial_player(
	const std::string &root, const flatfile_player_domain_record &record, std::string *error);
flatfile_player_domain_result flatfile_player_domain_load(const std::string &root, int32_t pid,
							  const std::string &account_name,
							  int8_t racewar,
							  flatfile_player_domain_record *record,
							  std::string *error);
critical_apply_result flatfile_player_domain_apply(const std::string &root,
						   const critical_command &command);
flatfile_player_domain_result flatfile_player_domain_prepare_wallet(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	const std::string &account_name, int8_t racewar, uint64_t expected_wallet_revision,
	uint64_t expected_bank_revision, int64_t value_delta, bool apply_delta,
	flatfile_wallet_mutation *mutation, unsigned int *result_code, std::string *error);
flatfile_player_domain_result flatfile_player_domain_prepare_resurrection_wallet(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	uint64_t expected_wallet_revision, const std::array<int32_t, 4> &expected_wallet,
	const std::array<int32_t, 4> &replacement_wallet, flatfile_wallet_mutation *mutation,
	std::string *error);
flatfile_player_domain_result flatfile_player_domain_prepare_base_stat(
	const std::string &root, const flatfile_authority_lock &lock, uint32_t pid,
	uint8_t stat_index, bool apply_increment, flatfile_base_stat_mutation *mutation,
	unsigned int *result_code, std::string *error);
flatfile_player_domain_result
flatfile_player_domain_prepare_remove(const std::string &root, const flatfile_authority_lock &lock,
				      uint32_t pid, flatfile_authority_operation *operation,
				      std::string *error);

#endif
