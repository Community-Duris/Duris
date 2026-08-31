#include "combat_outcome_transaction.h"

#include "redis/redis_report_cache.h"
#include "currency_transaction.h"
#include "epic_transaction.h"
#include "sql_player.h"
#include "utils.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
struct pending_combat_outcome
{
	combat_outcome_payload payload;
	combat_outcome_completion_fn completion;
	uint64_t submitted_at_msec;
};

std::unordered_map<std::string, pending_combat_outcome> pending;
combat_outcome_health health = {};
std::mutex outbox_mutex;
std::unordered_map<uint64_t, bool> outbox_publications;

uint64_t monotonic_msec()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		       std::chrono::steady_clock::now().time_since_epoch())
		.count();
}

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

void publish_live(const combat_outcome_result &result, const combat_outcome_payload &payload)
{
	for (size_t index = 0; index < result.participant_count; ++index)
	{
		const auto &entry = result.participants[index];
		P_char character = find_player_by_pid(entry.pid);
		if (!character || IS_NPC(character))
			continue;
		character->only.pc->oldfrags = character->only.pc->frags;
		character->only.pc->frags = entry.frags;
		character->only.pc->frag_revision = entry.frag_revision;
		epic_transaction_publish_balance(character, entry.epics, entry.epic_revision);
		if (entry.wallet_value >= 0)
		{
			currency_vector wallet = {};
			int64_t value = entry.wallet_value;
			wallet.amount[3] = value / 1000;
			value %= 1000;
			wallet.amount[2] = value / 100;
			value %= 100;
			wallet.amount[1] = value / 10;
			wallet.amount[0] = value % 10;
			currency_transaction_publish_balances(
				character, payload.participants[index].account_name.data(),
				payload.participants[index].racewar, wallet, entry.bank,
				entry.wallet_revision, entry.bank_revision);
		}
	}
}
} // namespace

bool combat_outcome_transaction_submit(const combat_outcome_payload &payload,
				       combat_outcome_completion_fn completion,
				       critical_operation_id *submitted_operation_id)
{
	if (pending.size() >= COMBAT_OUTCOME_PENDING_MAX)
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !combat_outcome_command_build(&command, operation_id, payload))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key,
				pending_combat_outcome{ payload, completion, monotonic_msec() });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	health.max_participants =
		std::max<uint64_t>(health.max_participants, payload.participant_count);
	health.pending = pending.size();
	if (submitted_operation_id)
		*submitted_operation_id = operation_id;
	return true;
}

void combat_outcome_transaction_handle_completions(const critical_completion *completions,
						   size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		combat_outcome_result result = {};
		const bool decoded = combat_outcome_command_decode_result(
			completions[index].result_payload.data(), completions[index].result_size,
			&result);
		const bool committed =
			decoded &&
			(completions[index].outcome == critical_apply_outcome::applied ||
			 completions[index].outcome == critical_apply_outcome::already_applied);
		if (committed)
		{
			publish_live(result, found->second.payload);
			++health.committed;
		}
		else
		{
			++health.rejected;
			if (!decoded)
				++health.malformed_completions;
		}
		if (found->second.completion)
			found->second.completion(committed,
						 decoded ? result : combat_outcome_result{},
						 completions[index].error_code,
						 found->second.payload);
		pending.erase(found);
	}
	health.pending = pending.size();
}

critical_outbox_delivery_result
combat_outcome_transaction_outbox_delivery(const critical_outbox_record &record, void *)
{
	if (record.destination != 6 || record.event_type != 1 || record.payload_version != 1)
		return critical_outbox_delivery_result::terminal_failure;
	combat_outcome_result result = {};
	if (!combat_outcome_command_decode_result(record.payload.data(), record.payload.size(),
						  &result))
		return critical_outbox_delivery_result::terminal_failure;
	std::lock_guard<std::mutex> lock(outbox_mutex);
	auto found = outbox_publications.find(record.outbox_id);
	if (found != outbox_publications.end())
	{
		if (found->second)
		{
			outbox_publications.erase(found);
			return critical_outbox_delivery_result::delivered;
		}
		return critical_outbox_delivery_result::retryable_failure;
	}
	if (outbox_publications.size() >= 1024)
		return critical_outbox_delivery_result::retryable_failure;
	outbox_publications.emplace(record.outbox_id, false);
	return critical_outbox_delivery_result::retryable_failure;
}

void combat_outcome_transaction_publish_outbox(void)
{
	bool publish = false;
	{
		std::lock_guard<std::mutex> lock(outbox_mutex);
		for (auto &[outbox_id, published] : outbox_publications)
		{
			(void)outbox_id;
			if (!published)
			{
				published = true;
				publish = true;
			}
		}
	}
	if (publish)
	{
		redis_invalidate_fraglist();
		critical_outbox_resume();
	}
}

combat_outcome_health combat_outcome_transaction_health_copy(void)
{
	health.pending = pending.size();
	health.oldest_age_msec = 0;
	const uint64_t now = monotonic_msec();
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		health.oldest_age_msec =
			std::max(health.oldest_age_msec, now - entry.submitted_at_msec);
	}
	return health;
}

void combat_outcome_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_publications.clear();
}
