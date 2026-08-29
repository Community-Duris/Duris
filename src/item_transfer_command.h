#ifndef ITEM_TRANSFER_COMMAND_H
#define ITEM_TRANSFER_COMMAND_H

#include "critical_command.h"

#include <array>
#include <cstdint>
#include <string>

constexpr uint16_t ITEM_TRANSFER_PAYLOAD_VERSION = 5;
constexpr uint16_t ITEM_TRANSFER_EXACT_PAYLOAD_VERSION = 4;
constexpr uint16_t ITEM_TRANSFER_PREVIOUS_PAYLOAD_VERSION = 3;
constexpr uint16_t ITEM_TRANSFER_LEGACY_PAYLOAD_VERSION = 2;
constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 12;
constexpr size_t ITEM_TRANSFER_HEADER_BYTES = 96;
constexpr size_t ITEM_TRANSFER_ENTRY_BYTES = 40;
constexpr size_t ITEM_TRANSFER_PAYLOAD_BYTES =
	ITEM_TRANSFER_HEADER_BYTES + ITEM_TRANSFER_MAX_ITEMS * ITEM_TRANSFER_ENTRY_BYTES;
constexpr size_t ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES = 128 * 1024;
constexpr size_t ITEM_TRANSFER_CORPSE_NAME_MAX_BYTES = 255;
constexpr size_t ITEM_TRANSFER_CORPSE_SHORT_DESCRIPTION_MAX_BYTES = 512;
constexpr size_t ITEM_TRANSFER_CORPSE_DESCRIPTION_MAX_BYTES = 64 * 1024;
constexpr size_t ITEM_TRANSFER_CORPSE_KEYWORDS_MAX_BYTES = 512;
constexpr size_t ITEM_TRANSFER_LEGACY_RESULT_BYTES = 40;
constexpr size_t ITEM_TRANSFER_RESULT_BYTES = 48;
constexpr uint64_t ITEM_TRANSFER_ABSENT_REVISION = UINT64_MAX;

enum class item_owner_type : uint8_t
{
	unknown = 0,
	player,
	container,
	room,
	corpse,
	locker,
	auction,
	system,
	destruction,
	shopkeeper,
};

enum class item_transfer_reason : uint16_t
{
	unknown = 0,
	synthetic,
	creation,
	destruction,
	operator_repair,
	player_get,
	player_drop,
	player_put,
	player_give,
	corpse_create,
	corpse_restore,
	corpse_loot,
	locker_deposit,
	locker_withdraw,
	auction_list,
	auction_claim,
	shop_buy,
	shop_sell,
};

enum class item_custody_state : uint8_t
{
	absent = 0,
	active,
	destroyed,
	quarantined,
};

struct item_owner_identity
{
	item_owner_type type;
	uint64_t id;
	uint64_t context_id;
};

struct item_transfer_entry
{
	uint64_t item_uid;
	uint64_t root_item_uid;
	uint64_t parent_item_uid;
	uint64_t expected_item_revision;
	int32_t vnum;
	item_custody_state expected_state;
};

struct item_corpse_metadata
{
	bool present = false;
	int32_t room_vnum = 0;
	int32_t weight = 0;
	uint8_t actor_racewar = 0;
	std::array<int32_t, 8> values = {};
	std::string owner_name;
	std::string short_description;
	std::string description;
	std::string keywords;
};

struct item_transfer_payload
{
	item_owner_identity from_owner;
	item_owner_identity to_owner;
	item_transfer_reason reason;
	int64_t reason_id;
	uint64_t expected_from_revision;
	uint64_t expected_to_revision;
	uint64_t selected_item_uid;
	uint64_t target_root_item_uid;
	uint64_t target_parent_item_uid;
	uint64_t expected_target_parent_revision;
	uint16_t item_count;
	std::array<item_transfer_entry, ITEM_TRANSFER_MAX_ITEMS> items;
	uint32_t item_blob_size;
	std::array<uint8_t, ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES> item_blob;
	item_corpse_metadata corpse;
};

struct item_transfer_result
{
	uint64_t root_item_uid;
	uint16_t item_count;
	uint64_t from_owner_revision;
	uint64_t to_owner_revision;
	uint64_t max_item_revision;
	uint64_t corpse_revision;
};

bool item_owner_identity_valid(const item_owner_identity &owner);
bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right);
uint64_t item_corpse_owner_id(uint32_t player_pid, uint32_t corpse_save_id);
uint64_t item_shopkeeper_owner_id(uint32_t shop_id);
bool item_owner_key(const item_owner_identity &owner, critical_entity_key *key);
bool item_transfer_command_encode_payload(const item_transfer_payload &payload,
					  std::vector<uint8_t> *encoded);
bool item_transfer_command_decode_payload(const critical_command &command,
					  item_transfer_payload *payload);
bool item_transfer_command_encode_result(const item_transfer_result &result,
					 std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> *encoded);
bool item_transfer_command_decode_result(const uint8_t *encoded, size_t size,
					 item_transfer_result *result);
bool item_transfer_command_build(critical_command *command, critical_operation_id operation_id,
				 const item_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class);

#endif
