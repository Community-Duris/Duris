#include "economy/economic_currency_adapter.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstring>
#include <iostream>
#include <random>
#include <type_traits>

using error = economic_accounting_error;
critical_operation_id id(uint8_t value)
{
	critical_operation_id result = {};
	result.bytes[0] = value;
	return result;
}
critical_command transfer(currency_reason_type reason = currency_reason_type::atm_deposit)
{
	currency_command_payload payload = {};
	payload.pid = 7;
	payload.racewar = 1;
	payload.reason = reason;
	std::strcpy(payload.account_name.data(), "fixture");
	payload.wallet_delta.amount = { -7, -3, -1, 0 };
	payload.bank_delta.amount = { 7, 3, 1, 0 };
	if (reason == currency_reason_type::atm_withdraw)
		for (size_t index = 0; index < 4; ++index)
		{
			payload.wallet_delta.amount[index] = -payload.wallet_delta.amount[index];
			payload.bank_delta.amount[index] = -payload.bank_delta.amount[index];
		}
	critical_command command = {};
	assert(currency_command_build(&command, id(3), payload, 4, 9, critical_source_site::command,
				      critical_deadline_class::interactive));
	return command;
}
economic_currency_authority authority(const critical_command &command)
{
	economic_currency_authority result;
	result.epoch = id(2);
	result.wallet_account = { id(1), economic_account_kind::wallet, 1001, 0 };
	result.bank_account = { id(1), economic_account_kind::bank, 2001, 1 };
	result.player_fence = command.keys[0];
	result.bank_fence = command.keys[1];
	result.state = { { { 7, 3, 1, 1 } }, { { 0, 0, 0, 0 } }, 4, 9 };
	return result;
}
economic_frozen_intent attach(critical_command &command, const economic_currency_authority &state)
{
	assert(economic_bank_transfer_intent(command, state.epoch, state.wallet_account,
					     state.bank_account,
					     &command.accounting_intent) == error::ok);
	command.schema_version = 2;
	economic_frozen_intent result;
	assert(economic_intent_decode(command.accounting_intent, &result) == error::ok);
	return result;
}
void prepare_and_agree()
{
	static_assert(!std::is_default_constructible_v<currency_prepared_mutation>);
	static_assert(!std::is_default_constructible_v<economic_prepared_currency>);
	static_assert(!std::is_aggregate_v<economic_prepared_currency>);
	auto command = transfer();
	auto state = authority(command);
	auto intent = attach(command, state);
	assert(intent.admission.metadata.writer_id == ECONOMIC_WRITER_BANK_DEPOSIT);
	assert(intent.admission.metadata.actor_id == 1001 && intent.admission.facts.size() == 24);
	std::optional<economic_prepared_currency> prepared;
	assert(economic_bank_transfer_prepare(command, intent, state,
					      currency_revision_policy::sql_legacy,
					      &prepared) == error::ok);
	const auto &after = prepared->mutation().after();
	assert((after.wallet.amount == economic_coin_vector{ 0, 0, 0, 1 }));
	assert((after.bank.amount == economic_coin_vector{ 7, 3, 1, 0 }));
	assert(after.wallet_revision == 5 && after.bank_revision == 10);
	assert(prepared->agrees_with(prepared->plan()) == error::ok);
	std::optional<economic_prepared_currency> flat;
	assert(economic_bank_transfer_prepare(command, intent, state,
					      currency_revision_policy::flatfile_legacy,
					      &flat) == error::ok);
	assert(prepared->agrees_with(flat->plan()) == error::ok);
	auto candidate = prepared->plan();
	std::swap(candidate.accounts[0], candidate.accounts[1]);
	for (auto &posting : candidate.postings)
		posting.account_index = 1 - posting.account_index;
	std::reverse(candidate.postings.begin(), candidate.postings.end());
	assert(prepared->agrees_with(candidate) == error::ok);
	candidate = prepared->plan();
	candidate.postings.push_back({ 2, 0, 0, { 1, 0, 0, 0 }, 1 });
	candidate.postings.push_back({ 3, 0, 0, { -1, 0, 0, 0 }, -1 });
	assert(economic_plan_validate_structure(candidate) == error::ok);
	assert(prepared->agrees_with(candidate) == error::payload_conflict);
	candidate = prepared->plan();
	++candidate.accounts[0].after_revision;
	assert(economic_plan_validate_structure(candidate) == error::ok);
	assert(prepared->agrees_with(candidate) == error::payload_conflict);
	candidate = prepared->plan();
	candidate.metadata.actor_id = 2001;
	assert(prepared->agrees_with(candidate) == error::payload_conflict);
	candidate = prepared->plan();
	candidate.metadata.intent_digest[0] ^= 1;
	assert(prepared->agrees_with(candidate) == error::payload_conflict);
	candidate = prepared->plan();
	candidate.accounts.push_back(
		{ { id(1), economic_account_kind::wallet, 3001, 0 }, {}, {}, 0, 1 });
	assert(economic_plan_normalize(&candidate) == error::ok);
	assert(prepared->agrees_with(candidate) == error::payload_conflict);
	command.accepted_at_usec = 9876;
	assert(economic_bank_transfer_prepare(command, intent, state,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::ok);
	assert(prepared->agrees_with(flat->plan()) == error::ok);
	auto changed = state;
	++changed.state.wallet_revision;
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::stale_revision);
	assert(prepared->agrees_with(flat->plan()) == error::ok);
	changed = state;
	changed.state.wallet.amount = {};
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::negative_holding);
	changed = state;
	changed.epoch = id(9);
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::unauthorized);
	changed = state;
	++changed.bank_account.authority_id;
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::unauthorized);
	changed = state;
	++changed.bank_fence.id;
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::unauthorized);
	changed = state;
	changed.wallet_account.lineage = id(8);
	changed.bank_account.lineage = id(8);
	assert(economic_bank_transfer_prepare(command, intent, changed,
					      currency_revision_policy::sql_legacy,
					      &flat) == error::unauthorized);
	const auto original = intent;
	auto rejects = [&]
	{
		assert(economic_intent_encode(intent, &command.accounting_intent) == error::ok);
		assert(economic_bank_transfer_prepare(command, intent, state,
						      currency_revision_policy::sql_legacy,
						      &flat) == error::unauthorized);
		intent = original;
	};
	intent.admission.metadata.epoch = id(9);
	rejects();
	intent.admission.metadata.writer_id = 999;
	rejects();
	intent.admission.metadata.actor_id = 2001;
	rejects();
	intent.admission.metadata.original_operation_id = id(8);
	rejects();
	intent.admission.metadata.source_event =
		economic_source_event{ economic_source_kind::quest_completion, id(8), id(9), 1, 1 };
	rejects();
	intent.admission.facts.push_back(0);
	rejects();
	command = transfer(currency_reason_type::atm_withdraw);
	state = authority(command);
	state.state.wallet.amount = { 0, 0, 0, 1 };
	state.state.bank.amount = { 7, 3, 1, 0 };
	intent = attach(command, state);
	assert(intent.admission.metadata.writer_id == ECONOMIC_WRITER_BANK_WITHDRAW);
	assert(economic_bank_transfer_prepare(command, intent, state,
					      currency_revision_policy::sql_legacy,
					      &prepared) == error::ok);
	assert((prepared->mutation().after().wallet.amount == economic_coin_vector{ 7, 3, 1, 1 }));
	assert((prepared->mutation().after().bank.amount == economic_coin_vector{}));
	command = transfer(currency_reason_type::wallet_reward);
	state = authority(command);
	std::vector<uint8_t> sentinel = { 99 };
	assert(economic_bank_transfer_intent(command, id(2), state.wallet_account,
					     state.bank_account, &sentinel) == error::unauthorized);
	assert(sentinel == std::vector<uint8_t>{ 99 });
	// Balanced change-making and simultaneous reverse transfers are not ATM capabilities.
	for (const auto direction :
	     { currency_reason_type::atm_deposit, currency_reason_type::atm_withdraw })
	{
		command = transfer(direction);
		currency_command_payload changed_payload = {};
		assert(currency_command_decode_payload(command, &changed_payload));
		changed_payload.wallet_delta.amount = { 3, 6, 8, -1 };
		changed_payload.bank_delta.amount = { 7, 3, 1, 0 };
		assert(currency_command_encode_payload(changed_payload, &command.payload));
		assert(economic_bank_transfer_intent(command, state.epoch, state.wallet_account,
						     state.bank_account,
						     &sentinel) == error::unauthorized);
		changed_payload.wallet_delta.amount = { 1, -1, 0, 0 };
		changed_payload.bank_delta.amount = { -1, 1, 0, 0 };
		assert(currency_command_encode_payload(changed_payload, &command.payload));
		assert(economic_bank_transfer_intent(command, state.epoch, state.wallet_account,
						     state.bank_account,
						     &sentinel) == error::unauthorized);
	}
	command = transfer();
	currency_command_payload payload = {};
	assert(currency_command_decode_payload(command, &payload));
	++payload.bank_delta.amount[0];
	assert(currency_command_encode_payload(payload, &command.payload));
	assert(economic_bank_transfer_intent(command, id(2), state.wallet_account,
					     state.bank_account, &sentinel) == error::unbalanced);
}
void mutation_policy()
{
	auto command = transfer();
	currency_command_payload payload = {};
	assert(currency_command_decode_payload(command, &payload));
	auto state = authority(command).state;
	std::optional<currency_prepared_mutation> prepared;
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == 0);
	// Generic domain preparation still supports existing change-making writers.
	payload.wallet_delta.amount = { 3, 6, 8, -1 };
	payload.bank_delta.amount = { 7, 3, 1, 0 };
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == 0);
	const auto prior = prepared->after();
	assert(currency_prepare_mutation(payload, state, 0, 0, currency_revision_policy::sql_legacy,
					 &prepared) == ESTALE);
	assert(prepared->after().wallet.amount == prior.wallet.amount);
	assert(currency_prepare_mutation(payload, state, UINT64_MAX, UINT64_MAX,
					 currency_revision_policy::flatfile_legacy,
					 &prepared) == 0);
	payload.reason = currency_reason_type::wallet_reward;
	payload.wallet_delta.amount = { 1, 0, 0, 0 };
	payload.bank_delta.amount = {};
	assert(currency_prepare_mutation(payload, state, 0, 0, currency_revision_policy::sql_legacy,
					 &prepared) == 0);
	assert(prepared->after().bank.amount == state.bank.amount &&
	       prepared->after().bank_revision == 10);
	assert(currency_prepare_mutation(payload, state, 0, 0,
					 currency_revision_policy::flatfile_legacy,
					 &prepared) == ESTALE);
	payload.reason = currency_reason_type::chaos_starter_reward;
	payload.wallet_delta.amount = {};
	payload.bank_delta.amount = { 1, 0, 0, 0 };
	assert(currency_prepare_mutation(payload, state, 0, 0, currency_revision_policy::sql_legacy,
					 &prepared) == 0);
	assert(currency_prepare_mutation(payload, state, 0, 0,
					 currency_revision_policy::flatfile_legacy,
					 &prepared) == ESTALE);
	payload.reason = currency_reason_type::bank_reward;
	assert(currency_prepare_mutation(payload, state, 0, 0, currency_revision_policy::sql_legacy,
					 &prepared) == ESTALE);
	state.wallet.amount[0] = INT_MAX;
	payload.wallet_delta.amount = { 1, 0, 0, 0 };
	payload.bank_delta.amount = { -1, 0, 0, 0 };
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == ENOSPC);
	assert(currency_prepare_mutation(payload, state, 4, 9,
					 currency_revision_policy::flatfile_legacy,
					 &prepared) == ERANGE);
	state = authority(command).state;
	payload.wallet_delta.amount = { INT64_MAX, 0, 0, 0 };
	payload.bank_delta.amount = {};
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == ERANGE);
	payload.wallet_delta.amount = { -INT64_MAX, 0, 0, 0 };
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == ENOSPC);
	payload.wallet_delta.amount = { INT64_MIN, 0, 0, 0 };
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == EINVAL);
	payload.wallet_delta.amount = { 1, 0, 0, 0 };
	state.wallet_revision = UINT64_MAX;
	assert(currency_prepare_mutation(payload, state, UINT64_MAX, 9,
					 currency_revision_policy::sql_legacy,
					 &prepared) == ERANGE);
	state = authority(command).state;
	state.wallet.amount[0] = -1;
	assert(currency_prepare_mutation(payload, state, 4, 9, currency_revision_policy::sql_legacy,
					 &prepared) == EILSEQ);
	state.wallet.amount[0] = static_cast<int64_t>(INT_MAX) + 1;
	for (auto policy :
	     { currency_revision_policy::sql_legacy, currency_revision_policy::flatfile_legacy })
		assert(currency_prepare_mutation(payload, state, 4, 9, policy, &prepared) ==
		       EILSEQ);
	std::mt19937_64 random(480);
	for (size_t iteration = 0; iteration < 5000; ++iteration)
	{
		state = {};
		payload.wallet_delta = {};
		payload.bank_delta = {};
		for (size_t part = 0; part < 4; ++part)
		{
			state.wallet.amount[part] = random() % 50000;
			state.bank.amount[part] = random() % 50000;
			payload.wallet_delta.amount[part] =
				static_cast<int64_t>(random() % 6001) - 3000;
			payload.bank_delta.amount[part] = -payload.wallet_delta.amount[part];
		}
		bool negative = false;
		for (size_t part = 0; part < 4; ++part)
			negative = negative ||
				   state.wallet.amount[part] + payload.wallet_delta.amount[part] <
					   0 ||
				   state.bank.amount[part] + payload.bank_delta.amount[part] < 0;
		for (auto policy : { currency_revision_policy::sql_legacy,
				     currency_revision_policy::flatfile_legacy })
		{
			const auto result =
				currency_prepare_mutation(payload, state, 0, 0, policy, &prepared);
			assert(result == static_cast<unsigned>(negative ? ENOSPC : 0));
			if (!result)
				for (size_t part = 0; part < 4; ++part)
				{
					assert(prepared->after().wallet.amount[part] ==
					       state.wallet.amount[part] +
						       payload.wallet_delta.amount[part]);
					assert(prepared->after().bank.amount[part] ==
					       state.bank.amount[part] +
						       payload.bank_delta.amount[part]);
				}
		}
	}
}
int main()
{
	prepare_and_agree();
	mutation_policy();
	std::cout
		<< "typed bank transfers and shared currency preparation: exact effects, authority rejection, legacy policies and 5000 random cases passed\n";
}
