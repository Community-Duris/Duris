#include "economy/collector_transaction.h"

#include "economy/collector_runtime.h"
#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <cerrno>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
struct pending_collector
{
	uint32_t actor_pid = 0;
	collector_command_payload payload = {};
	collector_completion_fn completion = nullptr;
	bool completion_ready = false;
	critical_completion completed = {};
};

std::unordered_map<std::string, pending_collector> pending;

enum class outbox_publication_state : uint8_t
{
	queued,
	publishing,
	published,
};

struct pending_outbox_publication
{
	collector_command_result result = {};
	outbox_publication_state state = outbox_publication_state::queued;
};

std::mutex outbox_mutex;
std::unordered_map<uint64_t, pending_outbox_publication> outbox_publications;
constexpr size_t COLLECTOR_OUTBOX_PENDING_MAX = 1024;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool player_pending(uint32_t pid)
{
	return std::any_of(pending.begin(), pending.end(),
			   [pid](const auto &entry) { return entry.second.actor_pid == pid; });
}

bool publishes_authority(const collector_command_payload &payload)
{
	return payload.action == collector_action::collect ||
	       payload.action == collector_action::purchase ||
	       payload.action == collector_action::expire ||
	       (payload.action == collector_action::cancel && payload.item_count);
}

bool publish(std::unordered_map<std::string, pending_collector>::iterator found, P_char character)
{
	pending_collector &entry = found->second;
	collector_command_result result = {};
	const bool decoded = collector_command_decode_result(entry.completed.result_payload.data(),
							     entry.completed.result_size, &result);
	const bool durable_commit = entry.completed.outcome == critical_apply_outcome::applied ||
				    entry.completed.outcome ==
					    critical_apply_outcome::already_applied;
	const bool committed = decoded && durable_commit;
	bool published = decoded;
	unsigned int publication_error = entry.completed.error_code;
	if (!decoded)
		publication_error = entry.completed.error_code ? entry.completed.error_code :
								 EBADMSG;
	if (published && committed &&
	    (!result.record_present || result.action != entry.payload.action))
	{
		published = false;
		publication_error = EBADMSG;
	}
	if (published && committed && publishes_authority(entry.payload) &&
	    !item_ownership_runtime_apply_collector(entry.payload, result))
	{
		published = false;
		publication_error = ESTALE;
	}
	if (published && committed && entry.payload.action == collector_action::purchase &&
	    (!character ||
	     !currency_transaction_publish_balances(
		     character, entry.payload.account_name.data(), entry.payload.racewar,
		     result.wallet, result.bank, result.wallet_revision, result.bank_revision)))
	{
		published = false;
		publication_error = ESTALE;
	}
	if (published && committed && !collector_runtime_publish(result))
	{
		published = false;
		publication_error = ESTALE;
	}

	const auto completion = entry.completion;
	const collector_command_payload payload = entry.payload;
	pending.erase(found);
	if (completion)
		completion(character, durable_commit, decoded ? result : collector_command_result{},
			   publication_error, payload);
	return committed && published;
}

bool submit(P_char character, const collector_command_payload &payload,
	    collector_completion_fn completion, critical_source_site source,
	    critical_deadline_class deadline)
{
	if (pending.size() >= COLLECTOR_PENDING_MAX ||
	    (payload.actor_pid && (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
				   static_cast<uint32_t>(GET_PID(character)) != payload.actor_pid ||
				   player_pending(payload.actor_pid))))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !collector_command_build(&command, operation_id, payload, source, deadline))
		return false;
	std::string key;
	try
	{
		key = operation_key(operation_id);
		const auto inserted = pending.emplace(
			key,
			pending_collector{ payload.actor_pid, payload, completion, false, {} });
		if (!inserted.second)
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		return false;
	}
	return true;
}
}

bool collector_transaction_submit(P_char character, const collector_command_payload &payload,
				  collector_completion_fn completion,
				  critical_deadline_class deadline)
{
	return payload.actor_pid &&
	       submit(character, payload, completion, critical_source_site::command, deadline);
}

bool collector_transaction_submit_background(const collector_command_payload &payload,
					     collector_completion_fn completion)
{
	return !payload.actor_pid &&
	       submit(nullptr, payload, completion, critical_source_site::zone_event,
		      critical_deadline_class::background);
}

void collector_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		std::string key;
		try
		{
			key = operation_key(completions[index].operation_id);
		}
		catch (const std::bad_alloc &)
		{
			continue;
		}
		auto found = pending.find(key);
		if (found == pending.end())
			continue;
		found->second.completed = completions[index];
		found->second.completion_ready = true;
		P_char character = found->second.actor_pid ?
					   find_player_by_pid(found->second.actor_pid) :
					   nullptr;
		if (!found->second.actor_pid || character)
			publish(found, character);
	}
}

void collector_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return;
	for (auto found = pending.begin(); found != pending.end();)
	{
		auto current = found++;
		if (current->second.actor_pid == static_cast<uint32_t>(GET_PID(character)) &&
		    current->second.completion_ready)
			publish(current, character);
	}
}

bool collector_transaction_player_busy(P_char character)
{
	return character && !IS_NPC(character) && GET_PID(character) > 0 &&
	       player_pending(static_cast<uint32_t>(GET_PID(character)));
}

critical_outbox_delivery_result
collector_transaction_outbox_delivery(const critical_outbox_record &record, void *context)
{
	if (record.destination != COLLECTOR_OUTBOX_DESTINATION)
		return critical_outbox_test_destination(record, context);
	collector_command_result result = {};
	if (record.event_type != COLLECTOR_OUTBOX_EVENT_MUTATED ||
	    record.payload_version != COLLECTOR_COMMAND_RESULT_VERSION ||
	    !collector_command_decode_result(record.payload.data(), record.payload.size(),
					     &result) ||
	    !result.record_present)
		return critical_outbox_delivery_result::terminal_failure;
	std::lock_guard<std::mutex> lock(outbox_mutex);
	auto found = outbox_publications.find(record.outbox_id);
	if (found != outbox_publications.end())
	{
		if (found->second.state == outbox_publication_state::published)
		{
			outbox_publications.erase(found);
			return critical_outbox_delivery_result::delivered;
		}
		return critical_outbox_delivery_result::retryable_failure;
	}
	try
	{
		if (!record.outbox_id || outbox_publications.size() >= COLLECTOR_OUTBOX_PENDING_MAX)
			return critical_outbox_delivery_result::retryable_failure;
		outbox_publications.emplace(
			record.outbox_id,
			pending_outbox_publication{ result, outbox_publication_state::queued });
	}
	catch (const std::bad_alloc &)
	{
		return critical_outbox_delivery_result::retryable_failure;
	}
	return critical_outbox_delivery_result::retryable_failure;
}

void collector_transaction_publish_outbox(void)
{
	std::vector<std::pair<uint64_t, collector_command_result>> work;
	{
		std::lock_guard<std::mutex> lock(outbox_mutex);
		try
		{
			work.reserve(std::min<size_t>(64, outbox_publications.size()));
			for (auto &[outbox_id, publication] : outbox_publications)
			{
				if (work.size() >= 64)
					break;
				if (publication.state == outbox_publication_state::queued)
				{
					publication.state = outbox_publication_state::publishing;
					work.emplace_back(outbox_id, publication.result);
				}
			}
		}
		catch (const std::bad_alloc &)
		{
			for (auto &[outbox_id, publication] : outbox_publications)
			{
				(void)outbox_id;
				if (publication.state == outbox_publication_state::publishing)
					publication.state = outbox_publication_state::queued;
			}
			return;
		}
	}
	bool published_any = false;
	for (const auto &[outbox_id, result] : work)
	{
		const bool published = collector_publish_committed_event(result, outbox_id);
		std::lock_guard<std::mutex> lock(outbox_mutex);
		auto found = outbox_publications.find(outbox_id);
		if (found != outbox_publications.end())
			found->second.state = published ? outbox_publication_state::published :
							  outbox_publication_state::queued;
		published_any = published_any || published;
	}
	if (published_any)
		critical_outbox_resume();
}

void collector_transaction_reset_for_tests(void)
{
	pending.clear();
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_publications.clear();
}
