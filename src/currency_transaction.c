#include "currency_transaction.h"

#include "gmcp.h"
#include "prototypes.h"
#include "sql_player.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>

namespace
{
struct pending_currency
{
	uint32_t pid;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	uint8_t racewar;
	currency_completion_fn completion;
	std::array<uint8_t, CURRENCY_PENDING_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	bool completion_ready;
	critical_completion completed;
};

std::unordered_map<std::string, pending_currency> pending;
currency_transaction_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool publish(std::unordered_map<std::string, pending_currency>::iterator found, P_char character)
{
	pending_currency &entry = found->second;
	const critical_completion &completion = entry.completed;
	currency_command_result result = {};
	if (!currency_command_decode_result(completion.result_payload.data(),
					    completion.result_size, &result))
	{
		++health.malformed_completions;
		if (entry.completion)
			entry.completion(character, false, {}, completion.error_code,
					 entry.context.data(), entry.context_size);
		++health.rejected;
		pending.erase(found);
		return false;
	}
	for (int64_t amount : result.wallet.amount)
		if (amount < 0 || amount > INT_MAX)
		{
			++health.malformed_completions;
			if (entry.completion)
				entry.completion(character, false, {}, ERANGE, entry.context.data(),
						 entry.context_size);
			++health.rejected;
			pending.erase(found);
			return false;
		}
	for (int64_t amount : result.bank.amount)
		if (amount < 0 || amount > INT_MAX)
		{
			++health.malformed_completions;
			if (entry.completion)
				entry.completion(character, false, {}, ERANGE, entry.context.data(),
						 entry.context_size);
			++health.rejected;
			pending.erase(found);
			return false;
		}
	const bool committed = completion.outcome == critical_apply_outcome::applied ||
			       completion.outcome == critical_apply_outcome::already_applied;
	GET_COPPER(character) = static_cast<int>(result.wallet.amount[0]);
	GET_SILVER(character) = static_cast<int>(result.wallet.amount[1]);
	GET_GOLD(character) = static_cast<int>(result.wallet.amount[2]);
	GET_PLATINUM(character) = static_cast<int>(result.wallet.amount[3]);
	character->only.pc->wallet_revision = result.wallet_revision;
	const AccountBankBalances balances = { static_cast<int>(result.bank.amount[0]),
					       static_cast<int>(result.bank.amount[1]),
					       static_cast<int>(result.bank.amount[2]),
					       static_cast<int>(result.bank.amount[3]) };
	publish_account_bank_balances_revision(entry.account_name.data(), entry.racewar, &balances,
					       result.bank_revision);
	gmcp_char_vitals(character);
	if (entry.completion)
		entry.completion(character, committed, result, completion.error_code,
				 entry.context.data(), entry.context_size);
	if (committed)
		++health.committed;
	else
		++health.rejected;
	pending.erase(found);
	return true;
}

void update_retained_health()
{
	health.pending = pending.size();
	health.retained_offline = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.completion_ready)
			++health.retained_offline;
	}
}

currency_vector canonical_value(int64_t value)
{
	currency_vector result = {};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	for (size_t index = values.size(); index-- > 0;)
	{
		result.amount[index] = value / values[index];
		value %= values[index];
	}
	return result;
}
} // namespace

bool currency_transaction_can_submit(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	const char *account_name = get_account_name_safe(character);
	if (!account_name || !strcmp(account_name, "Unknown") ||
	    strlen(account_name) > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player,
						 static_cast<uint64_t>(GET_PID(character)) };
	return currency_account_key(account_name, static_cast<uint8_t>(GET_RACEWAR(character)),
				    &account_key) &&
	       !critical_command_coordinator_is_fenced(player_key, nullptr) &&
	       !critical_command_coordinator_is_fenced(account_key, nullptr);
}

bool currency_transaction_submit(P_char character, const currency_vector &wallet_delta,
				 const currency_vector &bank_delta, currency_reason_type reason,
				 int64_t reason_id, critical_source_site source_site,
				 critical_deadline_class deadline_class,
				 currency_completion_fn completion, const void *context,
				 size_t context_size)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    context_size > CURRENCY_PENDING_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	const char *account_name = get_account_name_safe(character);
	if (!account_name || !strcmp(account_name, "Unknown") ||
	    strlen(account_name) > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player,
						 static_cast<uint64_t>(GET_PID(character)) };
	if (!currency_account_key(account_name, static_cast<uint8_t>(GET_RACEWAR(character)),
				  &account_key) ||
	    critical_command_coordinator_is_fenced(player_key, nullptr) ||
	    critical_command_coordinator_is_fenced(account_key, nullptr))
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	currency_command_payload payload = { .pid = static_cast<uint32_t>(GET_PID(character)),
					     .racewar =
						     static_cast<uint8_t>(GET_RACEWAR(character)),
					     .reason = reason,
					     .reason_id = reason_id,
					     .account_name = {},
					     .wallet_delta = wallet_delta,
					     .bank_delta = bank_delta };
	memcpy(payload.account_name.data(), account_name, strlen(account_name));
	if (!critical_operation_id_generate(&operation_id) ||
	    !currency_command_build(&command, operation_id, payload,
				    character->only.pc->wallet_revision,
				    character->only.pc->bank_revision, source_site, deadline_class))
		return false;
	pending_currency entry = { .pid = static_cast<uint32_t>(GET_PID(character)),
				   .account_name = payload.account_name,
				   .racewar = payload.racewar,
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
		critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	update_retained_health();
	return true;
}

bool currency_transaction_submit_wallet_value(P_char character, int64_t value_delta,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size)
{
	if (!value_delta || value_delta == INT64_MIN)
		return false;
	currency_vector wallet_delta = {};
	if (value_delta > 0)
		wallet_delta = canonical_value(value_delta);
	else
	{
		if (!character || IS_NPC(character))
			return false;
		const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
			GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
			GET_PLATINUM(character)
		};
		static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10,
											     100,
											     1000 };
		int64_t total = 0;
		for (size_t index = 0; index < current.size(); ++index)
		{
			if (current[index] > (INT64_MAX - total) / values[index])
				return false;
			total += current[index] * values[index];
		}
		const int64_t spend = -value_delta;
		if (total < spend)
			return false;
		const currency_vector after = canonical_value(total - spend);
		for (size_t index = 0; index < current.size(); ++index)
			wallet_delta.amount[index] = after.amount[index] - current[index];
	}
	return currency_transaction_submit(character, wallet_delta, {}, reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

bool currency_transaction_submit_bank_reward(P_char character, int64_t value,
					     currency_reason_type reason, int64_t reason_id,
					     critical_source_site source_site,
					     critical_deadline_class deadline_class,
					     currency_completion_fn completion, const void *context,
					     size_t context_size)
{
	if (value <= 0)
		return false;
	return currency_transaction_submit(character, {}, canonical_value(value), reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

bool currency_transaction_submit_bank_payment(P_char character, int64_t value,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size)
{
	if (!character || IS_NPC(character) || value <= 0)
		return false;
	const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
		GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
		GET_BALANCE_GOLD(character), GET_BALANCE_PLATINUM(character)
	};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	int64_t total = 0;
	for (size_t index = 0; index < current.size(); ++index)
	{
		if (current[index] > (INT64_MAX - total) / values[index])
			return false;
		total += current[index] * values[index];
	}
	if (total < value)
		return false;
	currency_vector bank_delta = {};
	int64_t remaining = value;
	for (size_t index = 0; index < current.size() && remaining > 0; ++index)
	{
		const int64_t needed = (remaining + values[index] - 1) / values[index];
		const int64_t used = std::min(current[index], needed);
		bank_delta.amount[index] = -used;
		remaining -= used * values[index];
	}
	currency_vector wallet_delta = {};
	if (remaining < 0)
		wallet_delta = canonical_value(-remaining);
	return currency_transaction_submit(character, wallet_delta, bank_delta, reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

void currency_transaction_handle_completions(const critical_completion *completions, size_t count)
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
	update_retained_health();
}

void currency_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character))
		return;
	for (auto found = pending.begin(); found != pending.end();)
	{
		if (found->second.pid == static_cast<uint32_t>(GET_PID(character)) &&
		    found->second.completion_ready)
		{
			auto current = found++;
			publish(current, character);
		}
		else
			++found;
	}
	update_retained_health();
}

currency_transaction_health currency_transaction_health_copy(void)
{
	update_retained_health();
	return health;
}

void currency_transaction_reset_for_tests(void)
{
	pending.clear();
	health = {};
}
