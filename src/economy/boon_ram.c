#include "economy/boon_ram.h"
#include "economy/boon.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cmath>
#include <memory>

namespace
{
boon_ram_state state;
struct prepared_state
{
	critical_operation_id id;
	boon_ram_state next;
	critical_apply_result result;
};
std::unique_ptr<prepared_state> prepared;

bool eligible(const flatfile_boon_definition &definition, const boon_reward_payload &payload)
{
	if (!definition.active || definition.option != payload.option ||
	    (definition.racewar && definition.racewar != payload.racewar) ||
	    (definition.target_pid && definition.target_pid != payload.pid))
		return false;
	switch (payload.option)
	{
	case BOPT_NONE:
		return definition.criteria == payload.zone_number;
	case BOPT_ZONE:
	case BOPT_OP:
	case BOPT_NEXUS:
	case BOPT_CTF:
	case BOPT_CTFB:
		return definition.criteria == payload.data;
	case BOPT_LEVEL:
		return definition.criteria == payload.level || definition.criteria == 0;
	case BOPT_MOB:
		return (payload.victim_flags & 1) &&
		       (definition.criteria2 == payload.victim_vnum || definition.criteria2 == -1);
	case BOPT_RACE:
		return (payload.victim_flags & 2) && definition.criteria2 == payload.victim_race;
	case BOPT_FRAG:
		return definition.criteria <= payload.data;
	default:
		return true;
	}
}

unsigned int reward(boon_ram_state &next, const boon_reward_payload &payload,
		    boon_reward_result *result)
{
	result->pid = payload.pid;
	for (auto &[id, definition] : next.definitions)
	{
		if (!eligible(definition, payload))
			continue;
		if (result->entry_count == BOON_REWARD_MAX_RESULTS)
			return E2BIG;
		const bool progress = definition.option == BOPT_MOB ||
				      definition.option == BOPT_RACE ||
				      definition.option == BOPT_FRAGS;
		const double increment = definition.option == BOPT_FRAGS ? payload.data :
					 progress			 ? 1.0 :
									   0.0;
		auto [entry, inserted] = next.progress.try_emplace({ id, payload.pid }, increment);
		double &counter = entry->second;
		if (!inserted && progress && counter != -1)
			counter += increment;
		if (!std::isfinite(counter))
			return ERANGE;
		const bool completed = counter != -1 &&
				       (!progress || counter >= definition.criteria);
		if (completed)
			counter = definition.repeat ? 0.0 : -1.0;
		if (completed && (definition.type == BTYPE_STATS || definition.type == BTYPE_POINT))
		{
			if (!std::isfinite(definition.bonus) || definition.bonus < INT_MIN ||
			    definition.bonus > INT_MAX)
				return ERANGE;
			auto &shop = next.shops[payload.pid];
			auto &balance = definition.type == BTYPE_STATS ? shop.stats : shop.points;
			const int64_t amount = static_cast<int>(definition.bonus);
			if ((amount > 0 && balance > INT64_MAX - amount) ||
			    (amount < 0 && balance < INT64_MIN - amount))
				return ERANGE;
			balance += amount;
			if (balance < INT32_MIN || balance > INT32_MAX)
				return ERANGE;
		}
		if (completed && definition.target_pid == payload.pid && !definition.repeat)
		{
			definition.active = false;
			definition.duration = 0;
		}
		result->entries[result->entry_count++] = {
			id,
			definition.type,
			definition.option,
			static_cast<uint8_t>((progress ? BOON_RESULT_PROGRESS : 0) |
					     (completed ? BOON_RESULT_COMPLETED : 0)),
			static_cast<uint8_t>(definition.repeat),
			definition.criteria,
			definition.criteria2,
			definition.bonus,
			definition.bonus2,
			counter
		};
	}
	return 0;
}

template <size_t N>
critical_apply_result receipt(const std::array<uint8_t, N> &bytes, uint64_t revision)
{
	critical_apply_result result{ critical_apply_outcome::applied, revision, 0 };
	result.result_size = N;
	std::copy(bytes.begin(), bytes.end(), result.result_payload.begin());
	return result;
}
}

void boon_ram_seed(boon_ram_state value)
{
	state = std::move(value);
	prepared.reset();
}
bool boon_ram_player(uint32_t pid, flatfile_boon_player_projection *result)
{
	if (!result)
		return false;
	const auto found = state.shops.find(pid);
	*result = found == state.shops.end() ? flatfile_boon_player_projection{} : found->second;
	return true;
}
bool boon_ram_definition(uint32_t id, flatfile_boon_definition *result)
{
	const auto found = state.definitions.find(id);
	if (!result || found == state.definitions.end())
		return false;
	*result = found->second;
	return true;
}
bool boon_ram_progress(uint32_t boon, uint32_t pid, double *counter)
{
	const auto found = state.progress.find({ boon, pid });
	if (!counter || found == state.progress.end())
		return false;
	*counter = found->second;
	return true;
}
bool boon_ram_configure(const flatfile_boon_definition &definition)
try
{
	state.definitions[definition.id] = definition;
	return true;
}
catch (...)
{
	return false;
}

critical_apply_result boon_ram_prepare(const critical_command &command, int16_t current_stat)
try
{
	prepared.reset();
	auto candidate = std::make_unique<prepared_state>();
	candidate->id = command.operation_id;
	candidate->next = state;
	if (command.type == critical_command_type::boon_reward)
	{
		boon_reward_payload payload{};
		boon_reward_result result{};
		std::array<uint8_t, BOON_REWARD_RESULT_BYTES> bytes{};
		if (!boon_reward_command_decode_payload(command, &payload))
			return { critical_apply_outcome::terminal_failure, 0, EINVAL };
		if (auto error = reward(candidate->next, payload, &result))
			return { critical_apply_outcome::terminal_failure, 0, error };
		if (!boon_reward_command_encode_result(result, &bytes))
			return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
		candidate->result = receipt(bytes, 0);
	}
	else if (command.type == critical_command_type::boon_shop)
	{
		boon_shop_payload payload{};
		if (!boon_shop_command_decode_payload(command, &payload))
			return { critical_apply_outcome::terminal_failure, 0, EINVAL };
		auto &shop = candidate->next.shops[payload.pid];
		auto &revision = candidate->next.stat_revisions[payload.pid];
		if (current_stat < 0 || revision == UINT64_MAX)
			return { critical_apply_outcome::terminal_failure, 0, ERANGE };
		if (current_stat >= 100)
			return { critical_apply_outcome::terminal_failure, 0, EALREADY };
		if (shop.stats <= 0)
			return { critical_apply_outcome::terminal_failure, 0, ENOSPC };
		boon_shop_result result{ payload.pid, payload.stat_index,
					 static_cast<int16_t>(current_stat + 1), --shop.stats,
					 ++revision };
		candidate->next.stat_changes[{ payload.pid, payload.stat_index }] = {
			result.stat_revision, result.stat_value
		};
		std::array<uint8_t, BOON_SHOP_RESULT_BYTES> bytes{};
		if (!boon_shop_command_encode_result(result, &bytes))
			return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
		candidate->result = receipt(bytes, revision);
	}
	else
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	const auto result = candidate->result;
	prepared = std::move(candidate);
	return result;
}
catch (...)
{
	return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
}

bool boon_ram_publish(const critical_command &command) noexcept
{
	if (command.schema_version != CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION || !prepared ||
	    !critical_operation_id_equal(prepared->id, command.operation_id) ||
	    command.outcome_revision != prepared->result.durable_revision ||
	    command.outcome_payload.size() != prepared->result.result_size ||
	    !std::equal(command.outcome_payload.begin(), command.outcome_payload.end(),
			prepared->result.result_payload.begin()))
		return false;
	std::swap(state, prepared->next);
	prepared.reset();
	return true;
}

void boon_ram_overlay_stats(uint32_t pid, uint64_t observed_revision,
			    std::array<int16_t, 10> *stats)
{
	if (!stats)
		return;
	for (size_t i = 0; i < stats->size(); ++i)
	{
		const auto found = state.stat_changes.find({ pid, static_cast<uint8_t>(i) });
		if (found != state.stat_changes.end() && found->second.first > observed_revision)
			(*stats)[i] = found->second.second;
	}
}
