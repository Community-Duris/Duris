#ifndef QUEST_REWARD_OBLIGATION_REPOSITORY_H
#define QUEST_REWARD_OBLIGATION_REPOSITORY_H

#include "item/quest_reward_continuation.h"
#include "persistence/critical_command.h"

#include <mysql/mysql.h>

#include <cstdint>
#include <span>
#include <vector>

// Existing journal-carrier authentication only. The coordinator grants this
// friend no publication, admission or journal mutation authority.
class quest_reward_obligation_native_fee_owner final
{
    public:
	static bool verify_in_transaction(MYSQL *, const critical_operation_id &,
					  std::span<const uint8_t>) noexcept;
	static bool ready(const critical_operation_id &,
			  const quest_reward_continuation &) noexcept;
};

struct quest_reward_obligation_record
{
	critical_operation_id offering_operation = {};
	std::vector<uint8_t> continuation;
	quest_reward_continuation terms;
	uint64_t xp_applied_mask = 0;
	// Verified native item/cash receipts, indexed by the frozen reward slot.
	uint64_t economic_applied_mask = 0;
};

struct quest_reward_xp_entitlement_record
{
	critical_operation_id offering_operation = {};
	std::vector<uint8_t> continuation;
	quest_reward_continuation terms;
	uint32_t reward_index = 0;
	uint32_t amount = 0;
};

enum class quest_reward_obligation_result
{
	ok,
	invalid,
	database_error,
	corrupt,
	limit_exceeded,
	pending_effects,
	already_acknowledged,
	not_found,
};

// Counts attempted SELECT executions and fetched native payloads, including
// receipts and rows that subsequently fail validation. Callers aggregate these
// into their load budget even when the read refuses publication.
struct quest_reward_read_metrics
{
	uint32_t query_count = 0;
	uint32_t row_count = 0;
	uint64_t byte_count = 0;
};

// ACK preflight is a distinct bounded literal read, not fee-success authority.
// One attempted SELECT per call; v6 rereads once on its trusted transaction.
constexpr size_t QUEST_REWARD_ACK_PREFLIGHT_QUERY_MAX = 1;
quest_reward_obligation_result quest_reward_obligation_repository_read_ack_terms(
	MYSQL *, uint32_t player_pid, const critical_operation_id &, std::vector<uint8_t> *,
	quest_reward_continuation *, unsigned int *,
	quest_reward_read_metrics * = nullptr) noexcept;

constexpr size_t QUEST_REWARD_PENDING_MAX = 64;
constexpr size_t QUEST_REWARD_PENDING_QUERY_MAX = 3;

// Read pending obligations for one player from a worker-owned connection.
// An inconsistent or oversized row fails the whole read without changing output.
quest_reward_obligation_result
quest_reward_obligation_repository_pending(MYSQL *connection, uint32_t player_pid,
					   std::vector<quest_reward_obligation_record> *obligations,
					   unsigned int *database_error_code,
					   quest_reward_read_metrics *metrics = nullptr);

quest_reward_obligation_result quest_reward_xp_entitlement_repository_pending(
	MYSQL *connection, uint32_t recipient_pid,
	std::vector<quest_reward_xp_entitlement_record> *entitlements,
	unsigned int *database_error_code, quest_reward_read_metrics *metrics = nullptr);

// Acknowledge only the exact retained obligation after all of its reward
// effects have durable receipts. Safe to retry from a repository worker.
quest_reward_obligation_result
quest_reward_obligation_repository_acknowledge(MYSQL *connection, uint32_t player_pid,
					       const critical_operation_id &offering_operation,
					       unsigned int *database_error_code);

struct quest_reward_obligation_readback
{
	quest_reward_obligation_record record;
	bool acknowledged = false;
	// Indexed by the original frozen XP award, not by reward slot.
	uint64_t xp_entitlement_applied_mask = 0;
};

constexpr size_t QUEST_REWARD_EXACT_QUERY_MAX = 4;
// Read the exact original obligation, including an acknowledged row, on the
// caller's reconnect-disabled, autocommit-on IN_TRANS session. Validate literal retained terms,
// the applied inbox, native item/cash witnesses and the complete original XP set.
// An ACKed row must have all required receipts. No writes, actor publication or
// reward/skill repetition. Caller separately authenticates the historical native
// quest core and confirms transaction cleanup before terminal journal mutation.
// Missing/invalid evidence never changes output; attempted reads count in metrics.
quest_reward_obligation_result quest_reward_obligation_repository_read_exact_in_transaction(
	MYSQL *, uint32_t player_pid, const critical_operation_id &offering_operation,
	std::span<const uint8_t> original_continuation, quest_reward_obligation_readback *,
	unsigned int *database_error_code, quest_reward_read_metrics *metrics = nullptr) noexcept;

#endif
