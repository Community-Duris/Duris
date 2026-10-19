#include "world/epic_transaction.h"

#include "world/epic.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <array>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>

namespace
{
struct pending_epic
{
	uint32_t pid;
	epic_completion_fn completion;
	std::array<uint8_t, EPIC_PENDING_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	critical_completion completed;
};

std::unordered_map<std::string, pending_epic> pending;
epic_transaction_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool publish(std::unordered_map<std::string, pending_epic>::iterator found, P_char character)
{
	pending_epic &entry = found->second;
	const critical_completion &completion = entry.completed;
	epic_command_result result = {};
	const bool committed = completion.outcome == critical_apply_outcome::applied ||
			       completion.outcome == critical_apply_outcome::already_applied;
	const bool decoded = epic_command_decode_result(completion.result_payload.data(),
							completion.result_size, &result);
	if (!decoded &&
	    (committed || completion.outcome != critical_apply_outcome::terminal_failure))
	{
		++health.malformed_completions;
		// The accepted result remains fenced until its exact receipt can publish.
		return false;
	}
	if (decoded &&
	    !epic_transaction_publish_balance(character, result.balance, result.revision))
		return false;
	const auto id = completion.operation_id;
	const auto error = completion.error_code;
	auto node = pending.extract(found);
	critical_command_coordinator_acknowledge_publication(id);
	if (committed)
		++health.committed;
	else
		++health.rejected;
	const auto &finished = node.mapped();
	if (finished.completion)
		finished.completion(character, committed, result, error, finished.context.data(),
				    finished.context_size);
	return true;
}

bool publish_completed_if_available(const std::string &key,
				    const critical_operation_id &operation_id, P_char character)
{
	critical_completion completion = {};
	if (!critical_command_coordinator_get_completed(operation_id, &completion))
		return false;
	auto found = pending.find(key);
	if (found == pending.end())
		return false;
	found->second.completed = completion;
	found->second.completion_ready = true;
	publish(found, character);
	return true;
}
} // namespace

bool epic_transaction_publish_balance(P_char character, int64_t balance, uint64_t revision)
{
	if (!character || IS_NPC(character))
		return false;
	if (revision < character->only.pc->epic_revision)
		return true;
	if (revision == character->only.pc->epic_revision && balance != character->only.pc->epics)
		return false;
	character->only.pc->epics = balance;
	character->only.pc->epic_revision = revision;
	return true;
}

bool epic_transaction_submit_identified(P_char character, const critical_operation_id &operation_id,
					int64_t delta, epic_reason_type reason, int64_t reason_id,
					uint16_t flags, critical_source_site source_site,
					critical_deadline_class deadline_class,
					epic_completion_fn completion, const void *context,
					size_t context_size)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 || !delta ||
	    context_size > EPIC_PENDING_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    pending.size() >= EPIC_PENDING_MAX || critical_operation_id_is_zero(operation_id))
		return false;
	critical_command command = {};
	if (!epic_command_build(&command, operation_id,
				{ .pid = static_cast<uint32_t>(GET_PID(character)),
				  .delta = delta,
				  .reason = reason,
				  .flags = flags,
				  .reason_id = reason_id },
				UINT64_MAX, source_site, deadline_class))
		return false;
	pending_epic entry = { .pid = static_cast<uint32_t>(GET_PID(character)),
			       .completion = completion,
			       .context = {},
			       .context_size = context_size,
			       .completion_ready = false,
			       .completed = {} };
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit_for_publication(std::move(command));
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	if (submitted == critical_submit_result::attached)
		publish_completed_if_available(key, operation_id, character);
	health.pending = pending.size();
	return true;
}

bool epic_transaction_submit(P_char character, int64_t delta, epic_reason_type reason,
			     int64_t reason_id, uint16_t flags, critical_source_site source_site,
			     critical_deadline_class deadline_class, epic_completion_fn completion,
			     const void *context, size_t context_size)
{
	critical_operation_id operation_id = {};
	return critical_operation_id_generate(&operation_id) &&
	       epic_transaction_submit_identified(character, operation_id, delta, reason, reason_id,
						  flags, source_site, deadline_class, completion,
						  context, context_size);
}

void epic_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		found->second.completed = completions[index];
		found->second.completion_ready = true;
		P_char character = find_player_by_pid(found->second.pid);
		if (character)
			publish(found, character);
	}
	health.pending = pending.size();
	health.retained_offline = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.completion_ready)
			++health.retained_offline;
	}
}

void epic_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character))
		return;
	std::array<critical_operation_id, EPIC_PENDING_MAX> ready;
	size_t count = 0;
	for (const auto &[key, entry] : pending)
		if (entry.pid == static_cast<uint32_t>(GET_PID(character)) &&
		    entry.completion_ready && count < ready.size())
			memcpy(ready[count++].bytes.data(), key.data(), key.size());
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(ready[index]));
		if (found != pending.end())
			publish(found, character);
	}
	health.pending = pending.size();
	health.retained_offline = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.completion_ready)
			++health.retained_offline;
	}
}

epic_transaction_health epic_transaction_health_copy(void)
{
	health.pending = pending.size();
	return health;
}

void epic_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
}
