#include "boon_reward_transaction.h"

#include "boon.h"
#include "flatfile/flatfile_boon_repository.h"
#include "persistence/persistence_mode.h"
#include "prototypes.h"
#include "spells.h"
#include "utils.h"

#include <algorithm>
#include <chrono>
#include <new>
#include <string>
#include <unordered_map>

namespace
{
struct pending_reward
{
	uint32_t pid;
	boon_reward_payload payload;
	uint64_t submitted_at_msec;
};

std::unordered_map<std::string, pending_reward> pending;
boon_reward_health health = {};

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

bool acknowledge_flat_reward(const critical_operation_id &operation_id)
{
	if (persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return true;
	const char *root = persistence_mode_flatfile_root();
	std::string error;
	if (root && flatfile_boon_acknowledge_reward(root, operation_id, &error) ==
			    flatfile_boon_result::ok)
		return true;
	persistence_alert(AVATAR, "boon_reward", "player", "unknown", "acknowledge",
			  "flat_write_failed", "error=%s", error.c_str());
	return false;
}
} // namespace

bool boon_reward_transaction_submit(P_char character, P_char victim, double data, int option)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 || option < 0 ||
	    option >= MAX_BOPT || pending.size() >= BOON_REWARD_PENDING_MAX)
		return false;
	boon_reward_payload payload = {
		.pid = static_cast<uint32_t>(GET_PID(character)),
		.racewar = static_cast<uint8_t>(GET_RACEWAR(character)),
		.level = static_cast<uint16_t>(std::max(0, GET_LEVEL(character))),
		.zone_number = character->in_room >= 0 ? ROOM_ZONE_NUMBER(character->in_room) : 0,
		.option = static_cast<uint8_t>(option),
		.data = data,
		.victim_vnum = victim && IS_NPC(victim) ? GET_VNUM(victim) : 0,
		.victim_race = static_cast<int16_t>(victim ? GET_RACE(victim) : 0),
		.victim_flags = 0
	};
	if (victim && IS_NPC(victim) && !IS_PC_PET(victim))
		payload.victim_flags |= 1;
	if (victim && ((IS_NPC(victim) && !get_linked_char(victim, LNK_PET) &&
			!affected_by_spell(victim, TAG_CONJURED_PET)) ||
		       (IS_PC(victim) && GET_RACEWAR(character) != GET_RACEWAR(victim))))
		payload.victim_flags |= 2;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !boon_reward_command_build(&command, operation_id, payload))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, pending_reward{ payload.pid, payload, monotonic_msec() });
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
	health.pending = pending.size();
	return true;
}

void boon_reward_transaction_handle_completions(const critical_completion *completions,
						size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		boon_reward_result result = {};
		const bool decoded =
			boon_reward_command_decode_result(completions[index].result_payload.data(),
							  completions[index].result_size, &result);
		const bool committed =
			decoded &&
			(completions[index].outcome == critical_apply_outcome::applied ||
			 completions[index].outcome == critical_apply_outcome::already_applied);
		if (committed)
		{
			P_char character = find_player_by_pid(found->second.pid);
			if (character)
			{
				boon_publish_transaction_result(character,
								found->second.payload.data, result);
				acknowledge_flat_reward(completions[index].operation_id);
			}
			health.max_results =
				std::max<uint64_t>(health.max_results, result.entry_count);
			++health.committed;
		}
		else
			++health.rejected;
		pending.erase(found);
	}
	health.pending = pending.size();
}

void boon_reward_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return;
	const char *root = persistence_mode_flatfile_root();
	if (!root)
		return;
	for (size_t delivered = 0; delivered < 64; ++delivered)
	{
		flatfile_boon_pending_reward reward;
		std::string error;
		const auto found = flatfile_boon_find_pending_reward(
			root, static_cast<uint32_t>(GET_PID(character)), &reward, &error);
		if (found == flatfile_boon_result::not_found)
			break;
		if (found != flatfile_boon_result::ok)
		{
			persistence_alert(AVATAR, "boon_reward", "player", "unknown",
					  "load_pending", "flat_read_failed", "error=%s",
					  error.c_str());
			break;
		}
		boon_publish_transaction_result(character, reward.event_data, reward.result);
		if (!acknowledge_flat_reward(reward.operation_id))
			break;
	}
}

critical_outbox_delivery_result
boon_reward_transaction_outbox_delivery(const critical_outbox_record &record, void *)
{
	if (record.destination != 8 || record.event_type != 1 || record.payload_version != 1)
		return critical_outbox_delivery_result::terminal_failure;
	boon_reward_result result = {};
	return boon_reward_command_decode_result(record.payload.data(), record.payload.size(),
						 &result) ?
		       critical_outbox_delivery_result::delivered :
		       critical_outbox_delivery_result::terminal_failure;
}

boon_reward_health boon_reward_transaction_health_copy(void)
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

void boon_reward_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
}
