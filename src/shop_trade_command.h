#ifndef SHOP_TRADE_COMMAND_H
#define SHOP_TRADE_COMMAND_H

#include "currency_command.h"
#include "item_transfer_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t SHOP_TRADE_PAYLOAD_VERSION = 4;
constexpr uint16_t SHOP_TRADE_PREVIOUS_PAYLOAD_VERSION = 3;
constexpr uint16_t SHOP_TRADE_STOCK_PAYLOAD_VERSION = 2;
constexpr uint16_t SHOP_TRADE_LEGACY_PAYLOAD_VERSION = 1;
constexpr size_t SHOP_TRADE_MAX_ITEMS = ITEM_TRANSFER_MAX_ITEMS;
constexpr size_t SHOP_TRADE_ITEM_BLOB_MAX_BYTES = 128 * 1024;
constexpr size_t SHOP_TRADE_RESULT_BYTES = 304;
constexpr uint8_t SHOP_TRADE_RESULT_VERSION = 1;

enum class shop_trade_action : uint8_t
{
	unknown = 0,
	buy_existing,
	buy_produced,
	sell_store,
	sell_destroy,
	discard_invalid,
};

struct shop_trade_item_entry
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	uint64_t expected_item_revision;
	int32_t vnum;
	item_custody_state expected_state;
};

struct shop_trade_payload
{
	shop_trade_action action;
	uint32_t player_pid;
	uint32_t shop_id;
	uint8_t racewar;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	int64_t price;
	uint64_t expected_wallet_revision;
	uint64_t expected_bank_revision;
	uint64_t expected_shop_revision;
	uint64_t selected_item_uid;
	uint64_t target_root_item_uid;
	uint64_t target_parent_item_uid;
	uint64_t expected_target_parent_revision;
	uint64_t stock_item_uid;
	uint64_t expected_stock_item_revision;
	int32_t stock_vnum;
	uint16_t item_count;
	std::array<shop_trade_item_entry, SHOP_TRADE_MAX_ITEMS> items;
	uint32_t item_blob_size;
	std::array<uint8_t, SHOP_TRADE_ITEM_BLOB_MAX_BYTES> item_blob;
};

struct shop_trade_result
{
	shop_trade_action action;
	currency_vector wallet;
	currency_vector bank;
	uint64_t wallet_revision;
	uint64_t bank_revision;
	uint64_t shop_revision;
	uint64_t player_owner_revision;
	uint64_t counterparty_owner_revision;
	uint16_t item_count;
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_uids;
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_revisions;
};

bool shop_trade_command_encode_payload(const shop_trade_payload &payload,
				       std::vector<uint8_t> *encoded);
bool shop_trade_command_decode_payload(const critical_command &command,
				       shop_trade_payload *payload);
bool shop_trade_command_encode_result(const shop_trade_result &result,
				      std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> *encoded);
bool shop_trade_command_decode_result(const uint8_t *encoded, size_t size,
				      shop_trade_result *result);
bool shop_trade_command_build(critical_command *command, critical_operation_id operation_id,
			      const shop_trade_payload &payload, critical_source_site source_site,
			      critical_deadline_class deadline_class);

#endif
