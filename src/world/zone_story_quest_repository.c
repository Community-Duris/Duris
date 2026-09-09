#include "world/zone_story_quest_repository.h"

#include <algorithm>
#include <utility>

namespace zone_story_quest_repository
{
namespace
{
void set_error(std::string *error, const char *message)
{
	if (error)
		*error = message;
}
} // namespace

record_result in_memory_completion_repository::record(
	const zone_story_quest_tracking::completion_transaction &transaction, std::string *error)
{
	if (!zone_story_quest_tracking::validate_transaction(transaction, error))
		return record_result::invalid;

	const auto existing = transactions_.find(transaction.transaction_id);
	if (existing == transactions_.end())
	{
		transactions_.emplace(transaction.transaction_id, transaction);
		return record_result::applied;
	}

	if (zone_story_quest_tracking::serialize_transaction(existing->second) ==
	    zone_story_quest_tracking::serialize_transaction(transaction))
		return record_result::already_applied;

	set_error(error, "transaction_id already contains different data");
	return record_result::conflict;
}

bool in_memory_completion_repository::get_transaction(
	std::string_view transaction_id,
	zone_story_quest_tracking::completion_transaction *transaction, std::string *error) const
{
	if (!transaction)
	{
		set_error(error, "transaction output must not be null");
		return false;
	}
	const auto existing = transactions_.find(std::string(transaction_id));
	if (existing == transactions_.end())
	{
		set_error(error, "transaction was not found");
		return false;
	}
	*transaction = existing->second;
	return true;
}

std::vector<zone_story_quest_tracking::completion_transaction>
in_memory_completion_repository::list_for_pid(uint32_t pid, uint32_t season_id, int32_t zone_number,
					      uint32_t content_revision) const
{
	std::vector<zone_story_quest_tracking::completion_transaction> matching;
	for (const auto &entry : transactions_)
	{
		const auto &transaction = entry.second;
		if (transaction.season_id == season_id && transaction.zone_number == zone_number &&
		    transaction.content_revision == content_revision &&
		    std::find(transaction.credited_pids.begin(), transaction.credited_pids.end(),
			      pid) != transaction.credited_pids.end())
			matching.push_back(transaction);
	}
	return matching;
}

std::size_t in_memory_completion_repository::size() const
{
	return transactions_.size();
}
} // namespace zone_story_quest_repository
