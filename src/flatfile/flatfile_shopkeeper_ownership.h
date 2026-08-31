#ifndef DURIS_FLATFILE_SHOPKEEPER_OWNERSHIP_H
#define DURIS_FLATFILE_SHOPKEEPER_OWNERSHIP_H

#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "player/player_load_repository.h"

#include <string>
#include <vector>

enum class flatfile_shopkeeper_ownership_result
{
	ok,
	not_found,
	invalid,
	io_error,
};

item_owner_identity flatfile_shopkeeper_item_owner(uint32_t shop_id);
flatfile_shopkeeper_ownership_result flatfile_shopkeeper_reconcile_item_ownership(
	const flatfile_shopkeeper_record &record, uint64_t owner_revision,
	const std::vector<flatfile_item_ownership_record> &custody,
	std::vector<player_load_item_identity> *identities);
flatfile_shopkeeper_ownership_result flatfile_shopkeeper_load_item_ownership(
	const std::string &root, const flatfile_shopkeeper_record &record, uint64_t *owner_revision,
	std::vector<player_load_item_identity> *identities, std::string *error);

#endif
