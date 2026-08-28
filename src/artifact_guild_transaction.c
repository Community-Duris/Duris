#include "artifact_guild_transaction.h"

#include "assocs.h"
#include "comm.h"
#include "prototypes.h"
#include "redis_report_cache.h"
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>

namespace
{
constexpr uint32_t ARTIFACT_GUILD_DERIVATION_DOMAIN = 0x41475431;

struct pending_outcome
{
	uint32_t actor_pid;
	artifact_guild_payload payload;
	uint64_t submitted_at_msec;
};

std::unordered_map<std::string, pending_outcome> pending;
artifact_guild_health health = {};
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

void publish_messages(P_char character, const artifact_guild_payload &payload)
{
	if (!character)
		return;
	if (payload.guild_id && payload.prestige_delta > 0)
		send_to_char("&+bYour guild gained prestige!\n", character);
	if (payload.construction_delta > 0)
	{
		Guild *guild = GET_ASSOC(character);
		if (guild)
		{
			statuslog(GREATER_G, "Association %s &ngained %lld construction points.",
				  guild->get_name().c_str(),
				  static_cast<long long>(payload.construction_delta));
			logit(LOG_STATUS, "Association %s &ngained %lld construction points.",
			      guild->get_name().c_str(),
			      static_cast<long long>(payload.construction_delta));
		}
	}
	if (payload.artifact_count)
		send_to_char("&+RYou feel a sense of satisfaction from your artifact(s).&n\r\n",
			     character);
}
} // namespace

bool artifact_guild_transaction_submit(P_char character,
				       const critical_operation_id &parent_operation_id, int epics,
				       int epic_type)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    pending.size() >= ARTIFACT_GUILD_PENDING_MAX)
		return false;
	artifact_guild_payload payload = {};
	const artifact_guild_capture_status captured = artifact_guild_state_capture(
		character, epics, epic_type, parent_operation_id, &payload);
	if (captured == artifact_guild_capture_status::no_effect)
		return true;
	if (captured != artifact_guild_capture_status::ready)
	{
		++health.unavailable;
		logit(LOG_FILE,
		      "artifact_guild: component=capture outcome=unavailable actor=redacted");
		return false;
	}
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_derive(parent_operation_id, ARTIFACT_GUILD_DERIVATION_DOMAIN,
					  static_cast<uint64_t>(GET_PID(character)),
					  &operation_id) ||
	    !artifact_guild_command_build(&command, operation_id, payload))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, pending_outcome{ static_cast<uint32_t>(GET_PID(character)),
						      payload, monotonic_msec() });
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
	health.max_artifacts = std::max<uint64_t>(health.max_artifacts, payload.artifact_count);
	health.pending = pending.size();
	return true;
}

void artifact_guild_transaction_handle_completions(const critical_completion *completions,
						   size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		artifact_guild_result result = {};
		const bool decoded = artifact_guild_command_decode_result(
			completions[index].result_payload.data(), completions[index].result_size,
			&result);
		const bool committed =
			decoded &&
			(completions[index].outcome == critical_apply_outcome::applied ||
			 completions[index].outcome == critical_apply_outcome::already_applied);
		if (committed)
		{
			artifact_guild_state_publish(result);
			publish_messages(find_player_by_pid(found->second.actor_pid),
					 found->second.payload);
			++health.committed;
		}
		else
		{
			++health.rejected;
			if (!decoded)
				++health.malformed_completions;
		}
		pending.erase(found);
	}
	health.pending = pending.size();
}

critical_outbox_delivery_result
artifact_guild_transaction_outbox_delivery(const critical_outbox_record &record, void *)
{
	if (record.destination != 7 || record.event_type != 1 || record.payload_version != 1)
		return critical_outbox_delivery_result::terminal_failure;
	artifact_guild_result result = {};
	if (!artifact_guild_command_decode_result(record.payload.data(), record.payload.size(),
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

void artifact_guild_transaction_publish_outbox(void)
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
		redis_invalidate_artifact_cache();
		critical_outbox_resume();
	}
}

artifact_guild_health artifact_guild_transaction_health_copy(void)
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

void artifact_guild_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
	artifact_guild_state_reset_for_tests();
	std::lock_guard<std::mutex> lock(outbox_mutex);
	outbox_publications.clear();
}
