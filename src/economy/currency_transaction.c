#include "economy/currency_transaction.h"

#include "net/gmcp.h"
#include "core/prototypes.h"
#include "sql/sql_player.h"
#include "core/utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <new>
#include <optional>
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
	std::optional<coin_transfer_payload> coin = std::nullopt;
	coin_completion_fn coin_completion = nullptr;
	bool coin_wallets_published = false;
	unsigned int publication_attempts = 0;
};

std::unordered_map<std::string, pending_currency> pending;
currency_transaction_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool publish_coin(std::unordered_map<std::string, pending_currency>::iterator found, P_char actor)
{
	auto &entry = found->second;
	const auto &completed = entry.completed;
	const bool committed = completed.outcome == critical_apply_outcome::applied ||
			       completed.outcome == critical_apply_outcome::already_applied;
	coin_transfer_result result;
	if (committed &&
	    !coin_transfer_command_decode_result(*entry.coin, completed.result_payload.data(),
						 completed.result_size, &result))
	{
		// An unparseable acknowledgement is not a rejected transaction. Preserve its
		// identity and do not invoke a failure/refund callback for committed money.
		++health.malformed_completions;
		return false;
	}
	if (committed && !entry.coin_wallets_published)
	{
		const coin_transfer_endpoint *endpoints[] = { &entry.coin->source,
							      &entry.coin->destination };
		for (size_t index = 0; index < 2; ++index)
		{
			if (endpoints[index]->change.type != critical_command_type::account_bank)
				continue;
			currency_command_payload wallet;
			if (!currency_command_decode_payload(endpoints[index]->change, &wallet))
				return false;
			P_char character = find_player_by_pid(wallet.pid);
			const auto &balances = result.wallets[index];
			// Offline endpoints load the same committed state on re-entry. They must
			// not hold up publication to the other endpoint or require a refund.
			if (character && !currency_transaction_publish_balances(
						 character, wallet.account_name.data(),
						 wallet.racewar, balances.wallet, balances.bank,
						 balances.wallet_revision, balances.bank_revision))
				return false;
		}
		entry.coin_wallets_published = true;
	}
	// Extract the node so a successful callback can advance a bulk command. A
	// temporarily unavailable live destination keeps this same operation for retry.
	auto node = pending.extract(found);
	auto &finished = node.mapped();
	bool published = true;
	try
	{
		if (finished.coin_completion)
			published = finished.coin_completion(actor, committed, *finished.coin,
							     result, finished.completed.error_code,
							     finished.context.data(),
							     finished.context_size);
	}
	catch (const std::bad_alloc &)
	{
		published = false;
	}
	if (!published)
	{
		if (++finished.publication_attempts < CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS)
		{
			pending.insert(std::move(node));
			return false;
		}
		// The authority already owns this result. Never report a rejected debit or
		// refund it merely because a live container vanished. Retire the hot retry;
		// the durable command and custody payload remain available for recovery.
		++health.publication_abandoned;
		char operation[33];
		critical_operation_id_to_hex(finished.completed.operation_id, operation,
					     sizeof(operation));
		persistence_alert(AVATAR, "currency", "coin_publication", operation, "none",
				  "publication_abandoned", "pid=%u committed=%d attempts=%u",
				  finished.pid, committed, finished.publication_attempts);
		if (committed && finished.coin_completion)
		{
			try
			{
				(void)finished.coin_completion(actor, true, *finished.coin, result,
							       EOWNERDEAD, finished.context.data(),
							       finished.context_size);
			}
			catch (const std::bad_alloc &)
			{
			}
		}
		if (actor)
			send_to_char(
				"Your coin transfer was saved, but its items could not be updated here. Please contact staff for recovery.\r\n",
				actor);
	}
	if (committed)
		++health.committed;
	else
		++health.rejected;
	return true;
}

bool publish(std::unordered_map<std::string, pending_currency>::iterator found, P_char character)
{
	pending_currency &entry = found->second;
	if (entry.coin)
		return publish_coin(found, character);
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
	const bool committed = completion.outcome == critical_apply_outcome::applied ||
			       completion.outcome == critical_apply_outcome::already_applied;
	if (!currency_transaction_publish_balances(character, entry.account_name.data(),
						   entry.racewar, result.wallet, result.bank,
						   result.wallet_revision, result.bank_revision))
	{
		++health.malformed_completions;
		if (entry.completion)
			entry.completion(character, false, {}, ERANGE, entry.context.data(),
					 entry.context_size);
		++health.rejected;
		pending.erase(found);
		return false;
	}
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
bool wallet_value_delta(P_char character, int64_t value_delta, currency_vector *delta)
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
	*delta = wallet_delta;
	return true;
}
} // namespace

bool currency_transaction_publish_balances(P_char character, const char *account_name,
					   uint8_t racewar, const currency_vector &wallet,
					   const currency_vector &bank, uint64_t wallet_revision,
					   uint64_t bank_revision)
{
	if (!character || !character->only.pc || !account_name)
		return false;
	for (int64_t amount : bank.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	if (!currency_transaction_publish_wallet(character, wallet, wallet_revision))
		return false;
	const AccountBankBalances balances = { static_cast<int>(bank.amount[0]),
					       static_cast<int>(bank.amount[1]),
					       static_cast<int>(bank.amount[2]),
					       static_cast<int>(bank.amount[3]) };
	publish_account_bank_balances_revision(account_name, racewar, &balances, bank_revision);
	return true;
}

bool currency_transaction_publish_wallet(P_char character, const currency_vector &wallet,
					 uint64_t wallet_revision)
{
	if (!character || !character->only.pc)
		return false;
	for (int64_t amount : wallet.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	// Re-entry may have loaded a later commit while this completion was retained
	// offline. Finish its callback without rolling the authoritative wallet back.
	if (wallet_revision < character->only.pc->wallet_revision)
		return true;
	GET_COPPER(character) = static_cast<int>(wallet.amount[0]);
	GET_SILVER(character) = static_cast<int>(wallet.amount[1]);
	GET_GOLD(character) = static_cast<int>(wallet.amount[2]);
	GET_PLATINUM(character) = static_cast<int>(wallet.amount[3]);
	character->only.pc->wallet_revision = wallet_revision;
	gmcp_char_vitals(character);
	return true;
}

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

bool currency_transaction_player_busy(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return false;
	const char *account_name = get_account_name_safe(character);
	const bool account_known = account_name && strcmp(account_name, "Unknown");
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	const uint8_t racewar = static_cast<uint8_t>(GET_RACEWAR(character));
	return std::any_of(
		pending.begin(), pending.end(),
		[pid, racewar, account_known, account_name](const auto &item)
		{
			const pending_currency &entry = item.second;
			if (entry.coin && entry.coin_wallets_published)
				return false;
			if (entry.coin)
			{
				for (const auto *endpoint :
				     { &entry.coin->source, &entry.coin->destination })
				{
					currency_command_payload wallet;
					if (endpoint->change.type ==
						    critical_command_type::account_bank &&
					    currency_command_decode_payload(endpoint->change,
									    &wallet) &&
					    (wallet.pid == pid ||
					     (account_known && wallet.racewar == racewar &&
					      !strcasecmp(wallet.account_name.data(),
							  account_name))))
						return true;
				}
			}
			return entry.pid == pid ||
			       (account_known && entry.racewar == racewar &&
				!strcasecmp(entry.account_name.data(), account_name));
		});
}

bool currency_transaction_coin_item_busy(uint64_t item_uid)
{
	if (!item_uid)
		return false;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (!entry.coin)
			continue;
		for (const auto *endpoint : { &entry.coin->source, &entry.coin->destination })
		{
			if (endpoint->change.type != critical_command_type::item_transfer)
				continue;
			item_transfer_payload pile;
			if (item_transfer_command_decode_payload(endpoint->change, &pile) &&
			    (pile.selected_item_uid == item_uid ||
			     pile.target_parent_item_uid == item_uid ||
			     pile.target_root_item_uid == item_uid ||
			     pile.items[0].root_item_uid == item_uid))
				return true;
		}
	}
	return false;
}

bool currency_transaction_coin_wallet(P_char character, int64_t value_delta,
				      coin_transfer_endpoint *endpoint)
{
	if (!endpoint || !currency_transaction_can_submit(character))
		return false;
	coin_transfer_endpoint candidate;
	candidate.before = { GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
			     GET_PLATINUM(character) };
	currency_vector delta;
	if (!wallet_value_delta(character, value_delta, &delta))
		return false;
	currency_command_payload wallet = {};
	wallet.pid = GET_PID(character);
	wallet.racewar = GET_RACEWAR(character);
	wallet.reason = currency_reason_type::coin_transfer;
	const char *account = get_account_name_safe(character);
	memcpy(wallet.account_name.data(), account, strlen(account));
	for (size_t index = 0; index < 4; ++index)
	{
		const int64_t after = int64_t(candidate.before[index]) + delta.amount[index];
		if (candidate.before[index] < 0 || after < 0 || after > INT32_MAX)
			return false;
		candidate.after[index] = after;
		wallet.wallet_delta.amount[index] = delta.amount[index];
	}
	critical_operation_id id;
	if (!critical_operation_id_generate(&id) ||
	    !currency_command_build(
		    &candidate.change, id, wallet, character->only.pc->wallet_revision,
		    character->only.pc->bank_revision, critical_source_site::command,
		    critical_deadline_class::interactive))
		return false;
	*endpoint = std::move(candidate);
	return true;
}

bool currency_transaction_submit_coin(P_char actor, const coin_transfer_payload &payload,
				      coin_completion_fn completion, const void *context,
				      size_t context_size)
{
	if (!currency_transaction_can_submit(actor) || currency_transaction_player_busy(actor) ||
	    context_size > CURRENCY_PENDING_CONTEXT_MAX_BYTES || (context_size && !context))
		return false;
	critical_operation_id id;
	critical_command command;
	if (!critical_operation_id_generate(&id) ||
	    !coin_transfer_command_build(&command, id, payload, critical_source_site::command,
					 critical_deadline_class::interactive))
		return false;
	for (const auto &key : command.keys)
		if (critical_command_coordinator_is_fenced(key, nullptr))
			return false;
	for (const auto *endpoint : { &payload.source, &payload.destination })
		if (endpoint->change.type == critical_command_type::account_bank)
		{
			currency_command_payload wallet;
			if (!currency_command_decode_payload(endpoint->change, &wallet))
				return false;
			P_char character = find_player_by_pid(wallet.pid);
			if (!character || currency_transaction_player_busy(character))
				return false;
		}
	const std::string key = operation_key(id);
	try
	{
		pending_currency entry = {};
		entry.pid = GET_PID(actor);
		entry.racewar = GET_RACEWAR(actor);
		const char *account = get_account_name_safe(actor);
		memcpy(entry.account_name.data(), account, strlen(account));
		entry.coin = payload;
		entry.coin_completion = completion;
		entry.context_size = context_size;
		if (context_size)
			memcpy(entry.context.data(), context, context_size);
		pending.emplace(key, std::move(entry));
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
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	if (submitted == critical_submit_result::attached)
		publish_completed_if_available(key, id, actor);
	update_retained_health();
	return true;
}

bool currency_transaction_submit(P_char character, const currency_vector &wallet_delta,
				 const currency_vector &bank_delta, currency_reason_type reason,
				 int64_t reason_id, critical_source_site source_site,
				 critical_deadline_class deadline_class,
				 currency_completion_fn completion, const void *context,
				 size_t context_size)
{
	critical_operation_id operation_id = {};
	return critical_operation_id_generate(&operation_id) &&
	       currency_transaction_submit_identified(character, operation_id, wallet_delta,
						      bank_delta, reason, reason_id, source_site,
						      deadline_class, completion, context,
						      context_size);
}

bool currency_transaction_submit_identified(
	P_char character, const critical_operation_id &operation_id,
	const currency_vector &wallet_delta, const currency_vector &bank_delta,
	currency_reason_type reason, int64_t reason_id, critical_source_site source_site,
	critical_deadline_class deadline_class, currency_completion_fn completion,
	const void *context, size_t context_size)
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
	currency_command_payload payload = { .pid = static_cast<uint32_t>(GET_PID(character)),
					     .racewar =
						     static_cast<uint8_t>(GET_RACEWAR(character)),
					     .reason = reason,
					     .reason_id = reason_id,
					     .account_name = {},
					     .wallet_delta = wallet_delta,
					     .bank_delta = bank_delta };
	memcpy(payload.account_name.data(), account_name, strlen(account_name));
	const bool rebasable_reward = currency_command_is_rebasable_reward(payload);
	if (!currency_account_key(account_name, static_cast<uint8_t>(GET_RACEWAR(character)),
				  &account_key) ||
	    (!rebasable_reward && (critical_command_coordinator_is_fenced(player_key, nullptr) ||
				   critical_command_coordinator_is_fenced(account_key, nullptr))))
		return false;
	if (critical_operation_id_is_zero(operation_id))
		return false;
	critical_command command = {};
	const uint64_t expected_bank_revision = currency_command_is_rebasable_bank_reward(payload) ?
							UINT64_MAX :
							character->only.pc->bank_revision;
	if (!currency_command_build(&command, operation_id, payload,
				    character->only.pc->wallet_revision, expected_bank_revision,
				    source_site, deadline_class))
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
	if (submitted == critical_submit_result::attached)
		publish_completed_if_available(key, operation_id, character);
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
	currency_vector wallet_delta;
	if (!wallet_value_delta(character, value_delta, &wallet_delta))
		return false;
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
	}
	// Callbacks may submit the next bulk operation, so do not retain map iterators
	// across them. Coin publication retries once per ordinary coordinator pulse.
	std::array<critical_operation_id, CURRENCY_PENDING_MAX> ready;
	size_t ready_count = 0;
	for (const auto &[key, entry] : pending)
		if (entry.completion_ready && ready_count < ready.size())
			memcpy(ready[ready_count++].bytes.data(), key.data(), key.size());
	for (size_t index = 0; index < ready_count; ++index)
	{
		auto found = pending.find(operation_key(ready[index]));
		if (found == pending.end())
			continue;
		P_char character = find_player_by_pid(found->second.pid);
		if (character || found->second.coin)
			publish(found, character);
	}
	update_retained_health();
}

void currency_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character))
		return;
	std::array<critical_operation_id, CURRENCY_PENDING_MAX> ready;
	size_t ready_count = 0;
	for (const auto &[key, entry] : pending)
		if (entry.pid == static_cast<uint32_t>(GET_PID(character)) &&
		    entry.completion_ready && ready_count < ready.size())
			memcpy(ready[ready_count++].bytes.data(), key.data(), key.size());
	for (size_t index = 0; index < ready_count; ++index)
	{
		auto found = pending.find(operation_key(ready[index]));
		if (found != pending.end())
			publish(found, character);
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
