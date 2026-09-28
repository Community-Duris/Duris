#ifndef DURIS_FLATFILE_AUCTION_REPOSITORY_H
#define DURIS_FLATFILE_AUCTION_REPOSITORY_H

#include "economy/auction_command.h"
#include "persistence/critical_command_coordinator.h"
#include "flatfile/flatfile_authority_transaction.h"

#include <cstdint>
#include <string>
#include <vector>

enum class flatfile_auction_query_result
{
	ok,
	not_found,
	invalid,
	io_error,
};

enum class flatfile_auction_player_reference_result
{
	clear,
	referenced,
	invalid,
	io_error,
};

struct flatfile_auction_item_projection
{
	uint64_t item_uid = 0;
	uint64_t item_revision = 0;
	int32_t vnum = 0;
};

struct flatfile_auction_listing_projection
{
	uint32_t auction_id = 0;
	uint32_t seller_pid = 0;
	uint32_t winner_pid = 0;
	int64_t current_price = 0;
	int64_t buy_price = 0;
	uint64_t revision = 0;
	uint64_t end_time = 0;
	std::string seller_name;
	std::string winner_name;
	std::string object_short;
	std::string id_keywords;
	std::string object_info;
	std::vector<uint8_t> object_blob;
	std::vector<flatfile_auction_item_projection> items;
};

struct flatfile_auction_pickup_projection
{
	int64_t money = 0;
	uint64_t money_revision = 0;
	bool has_item_claim = false;
	flatfile_auction_listing_projection item_claim;
};

struct flatfile_auction_event_projection
{
	critical_operation_id operation_id = {};
	uint64_t outbox_id = 0;
	auction_command_result result = {};
	flatfile_auction_listing_projection listing;
};

flatfile_auction_query_result
flatfile_auction_list_open(const std::string &root,
			   std::vector<flatfile_auction_listing_projection> *listings,
			   std::string *error);
flatfile_auction_query_result
flatfile_auction_find_open(const std::string &root, uint32_t auction_id,
			   flatfile_auction_listing_projection *listing, std::string *error);
flatfile_auction_query_result
flatfile_auction_find_pickup(const std::string &root, uint32_t pid,
			     flatfile_auction_pickup_projection *pickup, std::string *error);
flatfile_auction_query_result
flatfile_auction_find_pending_event(const std::string &root,
				    flatfile_auction_event_projection *event, std::string *error);
flatfile_auction_query_result
flatfile_auction_acknowledge_event(const std::string &root,
				   const critical_operation_id &operation_id, std::string *error);
flatfile_auction_player_reference_result
flatfile_auction_check_player_unreferenced(const std::string &root,
					   const flatfile_authority_lock &lock, uint32_t pid,
					   std::string *error);

critical_apply_result flatfile_auction_repository_apply(const std::string &root,
							const critical_command &command);

// Inactive schema-2 item claim owner. Its native custody and EAP1 record share
// the auction authority journal.
critical_apply_result
flatfile_auction_repository_apply_accounted_item_claim(const std::string &root,
						       const critical_command &command);

// Inactive schema-2 listing owner. Native listing, new escrow lifetime, and
// EAP1 evidence share the auction authority journal.
critical_apply_result
flatfile_auction_repository_apply_accounted_listing(const std::string &root,
						    const critical_command &command);

// Inactive schema-2 bid owner. Native bid, escrow, pending claims, and EAP1
// evidence share the auction authority journal.
critical_apply_result
flatfile_auction_repository_apply_accounted_bid(const std::string &root,
						const critical_command &command);

// Inactive schema-2 auction closure owners. Native claim rights and EAP1
// settlement evidence share the auction authority journal.
critical_apply_result
flatfile_auction_repository_apply_accounted_finalize(const std::string &root,
						     const critical_command &command);
critical_apply_result
flatfile_auction_repository_apply_accounted_remove(const std::string &root,
						   const critical_command &command);

// Inactive schema-2 collection owner. Every source allocation is consumed in
// the same journal as the native pickup, wallet credit, and EAP1 evidence.
critical_apply_result
flatfile_auction_repository_apply_accounted_money_claim(const std::string &root,
							const critical_command &command);

#endif
