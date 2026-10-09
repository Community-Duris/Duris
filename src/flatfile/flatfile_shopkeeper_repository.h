#ifndef DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H
#define DURIS_FLATFILE_SHOPKEEPER_REPOSITORY_H

#include "flatfile/flatfile_authority_transaction.h"
#include "player/player_snapshot.h"
#include "economy/shop_trade_command.h"

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
	// -1 marks a catalog decoded from v1, before keeper cash was retained.
	int64_t cash = 0;
	bool roaming = false;
	std::vector<flatfile_shopkeeper_affect_record> affects;
	std::vector<player_item_snapshot> items;
};

// Pure passive one-record INITIAL checkpoint, using the existing DURSHOPv2 bytes.
// Catalog revision1 is canonical framing only: it is neither the actual catalog
// clock nor proof of a file AFTER/write. Contained record revision must equal1
// and cash must be observed (not historical -1). Encode applies only the original
// affect ordering; decode requires exact canonical bytes. Strong outputs; no I/O,
// source/admission/publication/ACK capability follows from these values.
bool flatfile_shopkeeper_initial_checkpoint_encode(const flatfile_shopkeeper_record &,
						   std::vector<uint8_t> *output) noexcept;
bool flatfile_shopkeeper_initial_checkpoint_decode(const std::vector<uint8_t> &,
						   flatfile_shopkeeper_record *output) noexcept;

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
	int64_t keeper_cash = 0;
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
