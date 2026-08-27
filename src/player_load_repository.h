#ifndef PLAYER_LOAD_REPOSITORY_H
#define PLAYER_LOAD_REPOSITORY_H

#include "player_snapshot.h"

#include <array>
#include <cstdint>
#include <string>

struct st_mysql;
typedef struct st_mysql MYSQL;

constexpr uint32_t PLAYER_LOAD_SCHEMA_VERSION = 1;
constexpr size_t PLAYER_LOAD_ACCOUNT_MAX = 50;
constexpr size_t PLAYER_LOAD_NAME_MAX = 32;
constexpr size_t PLAYER_LOAD_QUERY_MAX = 16;
constexpr uint64_t PLAYER_LOAD_TIMEOUT_USEC = UINT64_C(3000000);

constexpr player_component_mask_t PLAYER_LOAD_SESSION01_COMPONENTS =
	PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_LANGUAGES | PLAYER_COMPONENT_INTRODUCTIONS |
	PLAYER_COMPONENT_TIMERS | PLAYER_COMPONENT_UNDEAD_SLOTS | PLAYER_COMPONENT_FORGED_ITEMS |
	PLAYER_COMPONENT_GRANTED_COMMANDS | PLAYER_COMPONENT_SKILLS | PLAYER_COMPONENT_AFFECTS |
	PLAYER_COMPONENT_SHAPECHANGES;

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
	player_load_metrics metrics = {};
	int64_t saved_at = 0;
	std::string account_name;
};

bool player_load_request_valid(const player_load_request &request, uint64_t now_usec);
player_load_result player_load_repository_execute(MYSQL *connection,
						  const player_load_request &request);

#endif
