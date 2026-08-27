#ifndef PLAYER_LOAD_REPOSITORY_H
#define PLAYER_LOAD_REPOSITORY_H

#include "item_transfer_command.h"
#include "player_snapshot.h"

#include <array>
#include <cstdint>
#include <string>

struct st_mysql;
typedef struct st_mysql MYSQL;

constexpr uint32_t PLAYER_LOAD_SCHEMA_VERSION = 1;
constexpr size_t PLAYER_LOAD_ACCOUNT_MAX = 50;
constexpr size_t PLAYER_LOAD_NAME_MAX = 32;
constexpr size_t PLAYER_LOAD_QUERY_MAX = 20;
constexpr uint64_t PLAYER_LOAD_TIMEOUT_USEC = UINT64_C(3000000);
constexpr size_t PLAYER_LOAD_ITEM_MAX = PLAYER_SNAPSHOT_MAX_OBJECTS;
constexpr size_t PLAYER_LOAD_ITEM_AFFECT_MAX = 4;
constexpr size_t PLAYER_LOAD_ITEM_DESCRIPTION_MAX = 64;
constexpr size_t PLAYER_LOAD_ITEM_OPERATIONS_PER_ITEM = 96;

constexpr player_component_mask_t PLAYER_LOAD_SESSION01_COMPONENTS =
	PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_LANGUAGES | PLAYER_COMPONENT_INTRODUCTIONS |
	PLAYER_COMPONENT_TIMERS | PLAYER_COMPONENT_UNDEAD_SLOTS | PLAYER_COMPONENT_FORGED_ITEMS |
	PLAYER_COMPONENT_GRANTED_COMMANDS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS |
	PLAYER_COMPONENT_SHAPECHANGES;
constexpr player_component_mask_t PLAYER_LOAD_SESSION02_COMPONENTS =
	PLAYER_LOAD_SESSION01_COMPONENTS | PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;

enum player_load_item_override : uint16_t
{
	PLAYER_LOAD_ITEM_OVERRIDE_WEAR_FLAGS = UINT16_C(1) << 0,
	PLAYER_LOAD_ITEM_OVERRIDE_TYPE = UINT16_C(1) << 1,
	PLAYER_LOAD_ITEM_OVERRIDE_MATERIAL = UINT16_C(1) << 2,
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR1 = UINT16_C(1) << 3,
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR2 = UINT16_C(1) << 4,
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR3 = UINT16_C(1) << 5,
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR4 = UINT16_C(1) << 6,
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR5 = UINT16_C(1) << 7,
	PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS = UINT16_C(1) << 8,
};

constexpr uint16_t PLAYER_LOAD_ITEM_OVERRIDE_ALL =
	PLAYER_LOAD_ITEM_OVERRIDE_WEAR_FLAGS | PLAYER_LOAD_ITEM_OVERRIDE_TYPE |
	PLAYER_LOAD_ITEM_OVERRIDE_MATERIAL | PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR1 |
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR2 | PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR3 |
	PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR4 | PLAYER_LOAD_ITEM_OVERRIDE_BITVECTOR5 |
	PLAYER_LOAD_ITEM_OVERRIDE_AFFECTS;

struct player_load_item_identity
{
	uint64_t database_id = 0;
	uint64_t serialized_parent_id = 0;
	uint32_t quantity = 0;
	uint16_t override_mask = 0;
	uint64_t item_uid = 0;
	uint64_t root_item_uid = 0;
	uint64_t parent_item_uid = 0;
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t item_revision = 0;
	uint64_t owner_revision = 0;
	item_custody_state state = item_custody_state::absent;
};

enum class player_load_outcome : uint8_t
{
	applied,
	not_found,
	retryable_failure,
	component_failure,
	limit_exceeded,
	timed_out,
	cancelled,
	stale,
};

struct player_load_request
{
	uint32_t schema_version = PLAYER_LOAD_SCHEMA_VERSION;
	uint64_t request_id = 0;
	int32_t pid = 0;
	std::string account_name;
	uint64_t deadline_usec = 0;
	std::string player_name;
	bool include_items = true;
};

struct player_load_domain_state
{
	uint64_t wallet_revision = 0;
	uint64_t epic_revision = 0;
	uint64_t frag_revision = 0;
	uint64_t bank_revision = 0;
	std::array<uint64_t, 4> wallet = {};
	std::array<uint64_t, 4> bank = {};
	int64_t epics = 0;
	int64_t frags = 0;
	int64_t old_frags = 0;
};

struct player_load_metrics
{
	uint32_t query_count = 0;
	uint32_t row_count = 0;
	uint64_t byte_count = 0;
	uint64_t transaction_usec = 0;
};

struct player_load_result
{
	uint64_t request_id = 0;
	int32_t pid = 0;
	player_load_outcome outcome = player_load_outcome::component_failure;
	unsigned int error_code = 0;
	player_snapshot snapshot = {};
	player_load_domain_state domains = {};
	uint64_t item_owner_revision = 0;
	std::vector<player_load_item_identity> item_identities;
	player_load_metrics metrics = {};
	int64_t saved_at = 0;
	std::string account_name;
};

bool player_load_request_valid(const player_load_request &request, uint64_t now_usec);
player_load_result player_load_repository_execute(MYSQL *connection,
						  const player_load_request &request);

#endif
