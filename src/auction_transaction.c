#include "auction_transaction.h"

#include "auction_houses.h"
#include "currency_transaction.h"
#include "net/gmcp.h"
#include "item_ownership_runtime.h"
#include "prototypes.h"
#include "sql/sql_player.h"
#include "utils.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <new>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
struct pending_auction
{
	uint32_t actor_pid;
	auction_command_payload payload;
	auction_completion_fn completion;
	bool completion_ready;
	critical_completion completed;
};

std::unordered_map<std::string, pending_auction> pending;

enum class outbox_publication_state : uint8_t
{
	queued,
	publishing,
	published,
};

struct pending_outbox_publication
{
	auction_command_result result;
	outbox_publication_state state;
};

std::mutex outbox_mutex;
std::unordered_map<uint64_t, pending_outbox_publication> outbox_publications;
constexpr size_t AUCTION_OUTBOX_PENDING_MAX = 1024;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool publish(std::unordered_map<std::string, pending_auction>::iterator found, P_char character)
{
	pending_auction &entry = found->second;
	auction_command_result result = {};
	const bool decoded = auction_command_decode_result(entry.completed.result_payload.data(),
							   entry.completed.result_size, &result);
	const bool committed = decoded &&
			       (entry.completed.outcome == critical_apply_outcome::applied ||
				entry.completed.outcome == critical_apply_outcome::already_applied);
	if (committed && character && result.wallet_revision &&
	    !currency_transaction_publish_balances(
		    character, entry.payload.account_name.data(), entry.payload.racewar,
		    result.wallet, result.bank, result.wallet_revision, result.bank_revision))
	{
		if (entry.completion)
			entry.completion(character, false, {}, ERANGE, entry.payload);
		pending.erase(found);
		return false;
	}
	if (committed && result.item_count)
	{
		const item_owner_identity owner = {
			result.action == auction_action::list ? item_owner_type::auction :
								item_owner_type::player,
			result.action == auction_action::list ? result.auction_id : entry.actor_pid,
			0
		};
		const uint64_t owner_revision = result.action == auction_action::list ?
							result.auction_owner_revision :
							result.player_owner_revision;
		for (size_t index = 0; index < result.item_count; ++index)
			if (!item_ownership_runtime_hydrate(
				    { result.item_uids[index], result.item_uids[index], 0, owner,
				      result.item_revisions[index], owner_revision,
				      entry.payload.items[index].vnum,
				      item_custody_state::active }))
			{
				if (entry.completion)
					entry.completion(character, false, {}, ESTALE,
							 entry.payload);
				pending.erase(found);
				return false;
			}
	}
	if (entry.completion)
		entry.completion(character, committed, decoded ? result : auction_command_result{},
				 entry.completed.error_code, entry.payload);
	pending.erase(found);
	return committed;
}

bool submit(P_char character, const auction_command_payload &payload,
	    auction_completion_fn completion, critical_source_site source,
	    critical_deadline_class deadline)
{
	if (pending.size() >= AUCTION_PENDING_MAX ||
	    (payload.actor_pid && (!character || IS_NPC(character) ||
				   static_cast<uint32_t>(GET_PID(character)) != payload.actor_pid)))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !auction_command_build(&command, operation_id, payload, source, deadline))
		return false;
	pending_auction entry = { .actor_pid = payload.actor_pid,
				  .payload = payload,
				  .completion = completion,
				  .completion_ready = false,
				  .completed = {} };
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, std::move(entry));
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
		return false;
	}
	return true;
}
} // namespace

bool auction_transaction_submit(P_char character, const auction_command_payload &payload,
				auction_completion_fn completion, critical_deadline_class deadline)
{
	return submit(character, payload, completion, critical_source_site::command, deadline);
}

bool auction_transaction_submit_background(const auction_command_payload &payload,
					   auction_completion_fn completion)
{
	return !payload.actor_pid &&
	       submit(nullptr, payload, completion, critical_source_site::zone_event,
		      critical_deadline_class::background);
}

void auction_transaction_handle_completions(const critical_completion *completions, size_t count)
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
		P_char character = found->second.actor_pid ?
					   find_player_by_pid(found->second.actor_pid) :
					   nullptr;
		if (!found->second.actor_pid || character)
			publish(found, character);
	}
}

void auction_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character))
		return;
	for (auto found = pending.begin(); found != pending.end();)
	{
		auto current = found++;
		if (current->second.actor_pid == static_cast<uint32_t>(GET_PID(character)) &&
		    current->second.completion_ready)
			publish(current, character);
	}
}

bool auction_transaction_player_busy(P_char character)
{
	if (!character || IS_NPC(character))
		return false;
	return std::any_of(
		pending.begin(), pending.end(), [&](const auto &entry)
		{ return entry.second.actor_pid == static_cast<uint32_t>(GET_PID(character)); });
}

critical_outbox_delivery_result
auction_transaction_outbox_delivery(const critical_outbox_record &record, void *context)
{
	if (record.destination != 5)
		return critical_outbox_test_destination(record, context);
	auction_command_result result = {};
	if (record.event_type != 1 || record.payload_version != 1 ||
	    !auction_command_decode_result(record.payload.data(), record.payload.size(), &result))
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
		if (outbox_publications.size() >= AUCTION_OUTBOX_PENDING_MAX)
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

void auction_transaction_publish_outbox(void)
{
	std::vector<std::pair<uint64_t, auction_command_result>> work;
	{
		std::lock_guard<std::mutex> lock(outbox_mutex);
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
	bool published = false;
	for (const auto &[outbox_id, result] : work)
	{
		const bool succeeded = auction_publish_committed_event(result, outbox_id);
		std::lock_guard<std::mutex> lock(outbox_mutex);
		auto found = outbox_publications.find(outbox_id);
		if (found != outbox_publications.end())
			found->second.state = succeeded ? outbox_publication_state::published :
							  outbox_publication_state::queued;
		published = published || succeeded;
	}
	if (published)
		critical_outbox_resume();
}

void auction_transaction_reset_for_tests(void)
{
	pending.clear();
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_publications.clear();
}
