#ifndef SHOP_TRADE_COMMAND_H
#define SHOP_TRADE_COMMAND_H

#include "economy/currency_command.h"
#include "economy/shop_trade_destination_weight.h"
#include "economy/shop_trade_recovery_manifest.h"
#include "item/item_transfer_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t SHOP_TRADE_PAYLOAD_VERSION = 5;
// Explicit accounted-only capability; legacy builders continue to emit v5.
constexpr uint16_t SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION = 6;
constexpr size_t SHOP_TRADE_ACCOUNTED_TAIL_BYTES = 16;
// Explicit new native witness capability; historical v1-v6 stay byte-exact.
constexpr uint16_t SHOP_TRADE_NATIVE_PAYLOAD_VERSION = 7;
constexpr size_t SHOP_TRADE_NATIVE_TAIL_BYTES = 32;
// Exact whole-forest recovery bindings; v1-v7 encodings remain unchanged.
constexpr uint16_t SHOP_TRADE_RECOVERY_PAYLOAD_VERSION = 8;
constexpr bool shop_trade_payload_version_is_accounted(uint16_t version) noexcept
{
	return version == SHOP_TRADE_ACCOUNTED_PAYLOAD_VERSION ||
	       version == SHOP_TRADE_NATIVE_PAYLOAD_VERSION ||
	       version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION;
}
constexpr uint16_t SHOP_TRADE_PREVIOUS_PAYLOAD_VERSION = 4;
constexpr uint16_t SHOP_TRADE_CONTAINER_PAYLOAD_VERSION = 3;
constexpr uint16_t SHOP_TRADE_STOCK_PAYLOAD_VERSION = 2;
constexpr uint16_t SHOP_TRADE_LEGACY_PAYLOAD_VERSION = 1;
constexpr size_t SHOP_TRADE_MAX_ITEMS = ITEM_TRANSFER_LEGACY_MAX_ITEMS;
constexpr size_t SHOP_TRADE_ITEM_BLOB_MAX_BYTES = 128 * 1024;
constexpr size_t SHOP_TRADE_PREVIOUS_RESULT_BYTES = 304;
constexpr size_t SHOP_TRADE_RESULT_BYTES = 312;
constexpr uint8_t SHOP_TRADE_RESULT_VERSION = 2;

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
	int32_t keeper_vnum;
	int64_t expected_keeper_cash;
	uint8_t keeper_roaming;
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
	// v6 only: exact original player status preimage, independently verified under
	// the native player row lock. Frozen placement transforms never read today's level.
	uint64_t expected_player_save_revision = 0;
	uint32_t expected_player_level = 0;
	// v7 only. Version determines presence; no facts are invented for v6.
	bool native_destination_weight_recorded = false;
	shop_trade_destination_weight destination_weight{};
	// v8 only. Full bodies remain with native/recovery owners; these are value bindings.
	bool recovery_manifest_recorded = false;
	shop_trade_recovery_manifest recovery_manifest{};
};

struct shop_trade_result
{
	shop_trade_action action;
	currency_vector wallet;
	currency_vector bank;
	uint64_t wallet_revision;
	uint64_t bank_revision;
	uint64_t shop_revision;
	int64_t keeper_cash;
	bool keeper_cash_recorded;
	uint64_t player_owner_revision;
	uint64_t counterparty_owner_revision;
	uint16_t item_count;
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_uids;
	std::array<uint64_t, SHOP_TRADE_MAX_ITEMS> item_revisions;
};

bool shop_trade_command_encode_payload(const shop_trade_payload &payload,
				       std::vector<uint8_t> *encoded);
bool shop_trade_command_encode_accounted_payload(const shop_trade_payload &payload,
						 std::vector<uint8_t> *encoded);
bool shop_trade_command_encode_native_payload(const shop_trade_payload &payload,
					      std::vector<uint8_t> *encoded);
bool shop_trade_command_encode_recovery_payload(const shop_trade_payload &payload,
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

// Source status checkpoint/hold is a native producer prerequisite, not a caller
// assertion granted by this pure codec. This does not freeze an accounting intent.
bool shop_trade_command_build_accounted(critical_command *command,
					critical_operation_id operation_id,
					const shop_trade_payload &payload,
					critical_source_site source_site,
					critical_deadline_class deadline_class);

bool shop_trade_command_build_native(critical_command *command, critical_operation_id operation_id,
				     const shop_trade_payload &payload,
				     critical_source_site source_site,
				     critical_deadline_class deadline_class);

bool shop_trade_command_build_recovery(critical_command *command,
				       critical_operation_id operation_id,
				       const shop_trade_payload &payload,
				       critical_source_site source_site,
				       critical_deadline_class deadline_class);

#endif
