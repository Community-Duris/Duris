#ifndef AUCTION_COMMAND_H
#define AUCTION_COMMAND_H

#include "persistence/critical_command.h"
#include "currency_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t AUCTION_COMMAND_PAYLOAD_VERSION = 1;
constexpr size_t AUCTION_COMMAND_MAX_ITEMS = 9;
constexpr size_t AUCTION_NAME_MAX_BYTES = 32;
constexpr size_t AUCTION_SHORT_MAX_BYTES = 255;
constexpr size_t AUCTION_KEYWORDS_MAX_BYTES = 1024;
constexpr size_t AUCTION_INFO_MAX_BYTES = 8192;
constexpr size_t AUCTION_BLOB_MAX_BYTES = 32768;
constexpr size_t AUCTION_RESULT_PAYLOAD_BYTES = 320;

enum class auction_action : uint8_t
{
	unknown = 0,
	list,
	bid,
	finalize,
	claim_money,
	claim_item,
	remove,
};

enum class auction_event_type : uint8_t
{
	none = 0,
	listed,
	bid_placed,
	sold,
	expired,
	money_claimed,
	item_claimed,
	removed,
};

struct auction_item_entry
{
	uint64_t item_uid;
	uint64_t expected_item_revision;
	int32_t vnum;
};

struct auction_command_payload
{
	auction_action action;
	uint32_t actor_pid;
	uint32_t auction_id;
	uint8_t racewar;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	std::array<char, AUCTION_NAME_MAX_BYTES + 1> actor_name;
	uint64_t expected_wallet_revision;
	uint64_t expected_bank_revision;
	int64_t value;
	int64_t start_price;
	int64_t buy_price;
	int64_t listing_fee;
	uint32_t closing_fee_basis_points;
	uint32_t bid_extension_seconds;
	uint64_t end_time;
	uint16_t item_count;
	std::array<auction_item_entry, AUCTION_COMMAND_MAX_ITEMS> items;
	std::array<uint8_t, AUCTION_BLOB_MAX_BYTES> object_blob;
	uint32_t object_blob_size;
	std::array<char, AUCTION_SHORT_MAX_BYTES + 1> object_short;
	std::array<char, AUCTION_KEYWORDS_MAX_BYTES + 1> id_keywords;
	std::array<char, AUCTION_INFO_MAX_BYTES + 1> object_info;
};

struct auction_command_result
{
	auction_action action;
	auction_event_type event_type;
	uint32_t auction_id;
	uint32_t status;
	uint32_t seller_pid;
	uint32_t winner_pid;
	uint32_t previous_bidder_pid;
	int64_t final_price;
	int64_t wallet_value_delta;
	currency_vector wallet;
	currency_vector bank;
	uint64_t wallet_revision;
	uint64_t bank_revision;
	uint64_t auction_revision;
	uint64_t player_owner_revision;
	uint64_t auction_owner_revision;
	uint16_t item_count;
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_uids;
	std::array<uint64_t, AUCTION_COMMAND_MAX_ITEMS> item_revisions;
};

bool auction_command_encode_payload(const auction_command_payload &payload,
				    std::vector<uint8_t> *encoded);
bool auction_command_decode_payload(const critical_command &command,
				    auction_command_payload *payload);
bool auction_command_encode_result(const auction_command_result &result,
				   std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> *encoded);
bool auction_command_decode_result(const uint8_t *encoded, size_t size,
				   auction_command_result *result);
bool auction_command_build(critical_command *command, critical_operation_id operation_id,
			   const auction_command_payload &payload, critical_source_site source_site,
			   critical_deadline_class deadline_class);

#endif
