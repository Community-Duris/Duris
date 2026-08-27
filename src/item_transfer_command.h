#ifndef ITEM_TRANSFER_COMMAND_H
#define ITEM_TRANSFER_COMMAND_H

#include "critical_command.h"

#include <array>
#include <cstdint>

constexpr uint16_t ITEM_TRANSFER_PAYLOAD_VERSION = 2;
constexpr size_t ITEM_TRANSFER_MAX_ITEMS = 12;
constexpr size_t ITEM_TRANSFER_HEADER_BYTES = 96;
constexpr size_t ITEM_TRANSFER_ENTRY_BYTES = 40;
constexpr size_t ITEM_TRANSFER_PAYLOAD_BYTES =
	ITEM_TRANSFER_HEADER_BYTES + ITEM_TRANSFER_MAX_ITEMS * ITEM_TRANSFER_ENTRY_BYTES;
constexpr size_t ITEM_TRANSFER_RESULT_BYTES = 40;
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
};

struct item_transfer_result
{
	uint64_t root_item_uid;
	uint16_t item_count;
	uint64_t from_owner_revision;
	uint64_t to_owner_revision;
	uint64_t max_item_revision;
};

bool item_owner_identity_valid(const item_owner_identity &owner);
bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right);
uint64_t item_corpse_owner_id(uint32_t player_pid, uint32_t corpse_save_id);
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
