#include "economy/boon_shop_transaction.h"

#include "persistence/persistence_mode.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <cerrno>
#include <new>
#include <string>
#include <unordered_map>

extern const struct attr_names_struct attr_names[];

namespace
{
struct pending_purchase
{
	uint32_t pid = 0;
};

std::unordered_map<std::string, pending_purchase> pending;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

void publish(P_char character, const boon_shop_result &result)
{
	if (!character || result.stat_index >= BOON_SHOP_BASE_STAT_COUNT)
		return;
	character->base_stats[result.stat_index] = result.stat_value;
	affect_total(character, TRUE);
	send_to_char_f(character, "Your %s rises to %d. You have %lld stat point%s left.\r\n",
		       attr_names[result.stat_index + 1].name, result.stat_value,
		       static_cast<long long>(result.remaining_stat_points),
		       result.remaining_stat_points == 1 ? "" : "s");
}
} // namespace

bool boon_shop_transaction_submit(P_char character, uint8_t stat_index)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    stat_index >= BOON_SHOP_BASE_STAT_COUNT ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY)
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !boon_shop_command_build(&command, operation_id,
				     { static_cast<uint32_t>(GET_PID(character)), stat_index }))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, pending_purchase{ static_cast<uint32_t>(GET_PID(character)) });
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const auto submitted = critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		return false;
	}
	return true;
}

void boon_shop_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		boon_shop_result result = {};
		const bool decoded =
			boon_shop_command_decode_result(completions[index].result_payload.data(),
							completions[index].result_size, &result);
		P_char character = find_player_by_pid(found->second.pid);
		if (decoded &&
		    (completions[index].outcome == critical_apply_outcome::applied ||
		     completions[index].outcome == critical_apply_outcome::already_applied))
			publish(character, result);
		else if (character && completions[index].error_code == ENOSPC)
			send_to_char("You don't have any stat points available.\r\n", character);
		else if (character && completions[index].error_code == EALREADY)
			send_to_char("You already have 100 points in that stat.\r\n", character);
		else if (character)
			send_to_char("The boon shop could not complete that purchase.\r\n",
				     character);
		pending.erase(found);
	}
}

void boon_shop_transaction_reset_for_tests(void)
{
	pending.clear();
}
