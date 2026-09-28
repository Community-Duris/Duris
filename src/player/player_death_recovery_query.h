#ifndef PLAYER_DEATH_RECOVERY_QUERY_H
#define PLAYER_DEATH_RECOVERY_QUERY_H

#include "persistence/critical_command.h"
#include "player/player_death_conflict_repository.h"

#include <mysql/mysql.h>
#include <string>
#include <vector>

enum class player_death_recovery_query_kind : uint8_t
{
	none,
	list,
	detail,
};

enum class player_death_recovery_query_outcome : uint8_t
{
	read,
	unauthorized,
	not_found,
	failed,
};

constexpr size_t PLAYER_DEATH_RECOVERY_SUMMARY_MAX = 768;
constexpr size_t PLAYER_DEATH_RECOVERY_ITEM_LABEL_MAX = 48;
constexpr size_t PLAYER_DEATH_RECOVERY_ITEM_LABEL_COUNT = 5;

struct player_death_recovery_query_request
{
	player_death_recovery_query_kind kind = player_death_recovery_query_kind::none;
	player_revision_t after_revision = 0;
	critical_operation_id operation_id = {};
};

struct player_death_recovery_query_result
{
	player_death_recovery_query_kind kind = player_death_recovery_query_kind::none;
	player_death_recovery_query_outcome outcome = player_death_recovery_query_outcome::failed;
	unsigned int error_code = 0;
	std::vector<player_death_conflict_case> cases;
	player_death_conflict_case detail_identity = {};
	std::string detail_summary;
};

// The caller supplies pid only after resolving a character from the authenticated account's
// trusted account_characters entry. The adapter independently checks active SQL membership
// and never returns raw snapshot payloads.
bool player_death_recovery_query_request_valid(const player_death_recovery_query_request &request,
					       int32_t pid, const std::string &account_name,
					       const std::string &character_name);
bool player_death_recovery_detail_identity_valid(int32_t pid,
						 const critical_operation_id &requested,
						 const player_death_conflict_case &identity,
						 const player_snapshot &snapshot);

// Formats only bounded numeric facts and sanitized corpse item labels; archive evidence cells
// and payload bytes are never returned to the account-menu caller.
bool player_death_recovery_summary_build(const player_death_conflict_case &identity,
					 const player_snapshot &snapshot, std::string *summary);

player_death_recovery_query_result
player_death_recovery_query_execute(MYSQL *connection, int32_t pid, const std::string &account_name,
				    const std::string &character_name,
				    const player_death_recovery_query_request &request) noexcept;

#endif
