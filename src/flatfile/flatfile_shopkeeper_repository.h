#ifndef DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H
#define DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"
#include "player_snapshot.h"
#include "shop_trade_command.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct flatfile_shopkeeper_affect_record
{
	int32_t type = 0;
	int32_t duration = 0;
	int32_t modifier = 0;
	int32_t location = 0;
	std::array<uint64_t, 5> bitvectors = {};
};

struct flatfile_shopkeeper_record
{
	uint32_t shop_id = 0;
	int32_t mob_vnum = 0;
	int32_t room_vnum = 0;
	int64_t saved_at = 0;
	uint64_t revision = 0;
	std::vector<flatfile_shopkeeper_affect_record> affects;
	std::vector<player_item_snapshot> items;
};

enum class flatfile_shopkeeper_result
{
	ok,
	not_found,
	already_exists,
	stale,
	invalid,
	io_error
};

struct flatfile_shopkeeper_trade_mutation
{
	uint64_t shop_revision = 0;
	flatfile_authority_after_image after_image;
};

flatfile_shopkeeper_result
flatfile_shopkeeper_establish(const std::string &root,
			      const std::vector<flatfile_shopkeeper_record> &records,
			      std::string *error);
flatfile_shopkeeper_result
flatfile_shopkeeper_list(const std::string &root, std::vector<flatfile_shopkeeper_record> *records,
			 std::string *error);
flatfile_shopkeeper_result flatfile_shopkeeper_replace(const std::string &root,
						       const flatfile_shopkeeper_record &record,
						       uint64_t expected_revision,
						       std::string *error);
flatfile_shopkeeper_result
flatfile_shopkeeper_prepare_trade(const std::string &root, const flatfile_authority_lock &lock,
				  const shop_trade_payload &payload,
				  flatfile_shopkeeper_trade_mutation *mutation,
				  unsigned int *result_code, std::string *error);

#endif
