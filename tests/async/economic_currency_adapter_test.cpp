#include "economy/economic_currency_adapter.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <openssl/sha.h>
#include <random>
#include <type_traits>

using error = economic_accounting_error;
// Component evidence only; the SQL/gameplay gate must supply committed receipts.
void export_telemetry_plan(const critical_command &command,
			   const economic_prepared_currency &prepared)
{
	if (!std::getenv("DURIS_TELEMETRY_PLAN_EXPORT"))
		return;
	std::vector<uint8_t> plan, encoded, keys;
	assert(economic_plan_encode(prepared.plan(), &plan) == error::ok);
	// Native intent freezes before admission supplies a timestamp. This remains
	// a component fixture and does not assert an actual SQL commit.
	auto admitted = command;
	admitted.accepted_at_usec = 1;
	assert(critical_command_encode(admitted, &encoded) == critical_command_codec_result::ok);
	std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> result;
	assert(currency_command_encode_result(prepared.mutation().after(), &result));
	for (const auto &key : command.keys)
	{
		keys.push_back(static_cast<uint8_t>(key.type));
		for (size_t byte = 0; byte < 8; ++byte)
			keys.push_back(static_cast<uint8_t>(key.id >> (8 * byte)));
	}
	economic_digest command_hash = {}, keys_hash = {};
	SHA256(encoded.data(), encoded.size(), command_hash.data());
	SHA256(keys.data(), keys.size(), keys_hash.data());
	static constexpr char digits[] = "0123456789abcdef";
	std::cout << "BANK_HEX";
	for (const std::span<const uint8_t> value :
	     { std::span<const uint8_t>(plan), std::span<const uint8_t>(command.accounting_intent),
	       std::span<const uint8_t>(command.payload), std::span<const uint8_t>(result),
	       std::span<const uint8_t>(command_hash), std::span<const uint8_t>(keys_hash) })
	{
		std::cout << ' ';
		for (uint8_t byte : value)
			std::cout << digits[byte >> 4] << digits[byte & 15];
	}
	std::cout << '\n';
}
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
	export_telemetry_plan(command, *prepared);
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
	export_telemetry_plan(command, *prepared);
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
void quest_wallet_reward()
{
	auto command = transfer(currency_reason_type::wallet_reward);
	currency_command_payload payload = {};
	assert(currency_command_decode_payload(command, &payload));
	payload.reason_id = 1;
	payload.wallet_delta.amount = { 100, 0, 0, 0 };
	payload.bank_delta.amount = {};
	assert(currency_command_encode_payload(payload, &command.payload));
	command.source_site = critical_source_site::recovery;
	command.deadline_class = critical_deadline_class::recovery;
	command.expected_revisions[0].revision = UINT64_MAX;
	command.expected_revisions[1].revision = UINT64_MAX;
	auto state = authority(command);
	std::vector<uint8_t> encoded;
	assert(economic_quest_wallet_reward_intent(command, state.epoch, state.wallet_account,
						   state.bank_account, &encoded) == error::ok);
	command.accounting_intent = encoded;
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	economic_frozen_intent intent;
	assert(economic_intent_decode(encoded, &intent) == error::ok);
	assert(intent.admission.metadata.writer_id == ECONOMIC_WRITER_QUEST_WALLET_REWARD &&
	       intent.admission.metadata.reason == economic_reason::quest_reward &&
	       intent.admission.metadata.source_event &&
	       intent.admission.metadata.source_event->source.bytes == command.operation_id.bytes);
	std::optional<economic_prepared_currency> prepared;
	assert(economic_quest_wallet_reward_prepare(command, intent, state,
						    currency_revision_policy::sql_legacy,
						    &prepared) == error::ok);
	assert(prepared->plan().accounts.size() == 3 && prepared->plan().postings.size() == 2 &&
	       prepared->plan().metadata.source_event &&
	       prepared->plan().accounts[0].key.kind == economic_account_kind::wallet &&
	       prepared->plan().accounts[2].key.kind == economic_account_kind::issuance);
	assert(prepared->agrees_with(prepared->plan()) == error::ok);
	std::optional<economic_prepared_currency> flatfile;
	assert(economic_quest_wallet_reward_prepare(command, intent, state,
						    currency_revision_policy::flatfile_legacy,
						    &flatfile) == error::ok);
	assert(prepared->agrees_with(flatfile->plan()) == error::ok);
	export_telemetry_plan(command, *prepared);
	command.source_site = critical_source_site::command;
	assert(economic_quest_wallet_reward_intent(command, state.epoch, state.wallet_account,
						   state.bank_account,
						   &encoded) == error::unauthorized);
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
					 &prepared) == 0);
	assert(prepared->after().wallet.amount[0] == state.wallet.amount[0] + 1 &&
	       prepared->after().bank_revision == state.bank_revision + 1);
	payload.reason = currency_reason_type::chaos_starter_reward;
	payload.wallet_delta.amount = {};
	payload.bank_delta.amount = { 1, 0, 0, 0 };
	assert(currency_prepare_mutation(payload, state, 0, 0, currency_revision_policy::sql_legacy,
					 &prepared) == 0);
	assert(currency_prepare_mutation(payload, state, 0, 0,
					 currency_revision_policy::flatfile_legacy,
					 &prepared) == 0);
	assert(prepared->after().wallet_revision == state.wallet_revision + 1 &&
	       prepared->after().bank_revision == state.bank_revision + 1);
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
void chaos_starter_bank_supply()
{
	critical_operation_id seed = {};
	std::memcpy(seed.bytes.data(), "CHAOSEED", 8);
	seed.bytes[8] = 7;
	critical_operation_id operation = {};
	assert(critical_operation_id_derive(seed, 0x43484250, 1, &operation));
	currency_command_payload payload = {};
	payload.pid = 7;
	payload.racewar = 1;
	payload.reason = currency_reason_type::chaos_starter_reward;
	payload.reason_id = 7;
	std::strcpy(payload.account_name.data(), "fixture");
	payload.bank_delta.amount = { 0, 0, 0, 1000000 };
	critical_command command = {};
	assert(currency_command_build(&command, operation, payload, 4, UINT64_MAX,
				      critical_source_site::login,
				      critical_deadline_class::recovery));
	auto state = authority(command);
	state.state.bank.amount = { 0, 0, 0, 2 };
	std::vector<uint8_t> frozen;
	assert(economic_chaos_starter_bank_intent(command, state.epoch, state.wallet_account,
						  state.bank_account, &frozen) == error::ok);
	auto other_epoch = frozen;
	assert(economic_chaos_starter_bank_intent(command, id(9), state.wallet_account,
						  state.bank_account, &other_epoch) == error::ok);
	economic_frozen_intent intent, next_epoch;
	assert(economic_intent_decode(frozen, &intent) == error::ok);
	assert(economic_intent_decode(other_epoch, &next_epoch) == error::ok);
	const auto &source = intent.admission.metadata.source_event;
	assert(source && source->kind == economic_source_kind::starter_grant &&
	       source->source.bytes == seed.bytes && source->generation.bytes == seed.bytes &&
	       source->sequence == 1 && source->slot == 1);
	assert(next_epoch.admission.metadata.source_event->source.bytes == seed.bytes);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = frozen;
	std::optional<economic_prepared_currency> prepared;
	assert(economic_chaos_starter_bank_prepare(command, intent, state,
						   currency_revision_policy::bank_only,
						   &prepared) == error::ok);
	const auto &after = prepared->mutation().after();
	assert(after.wallet.amount == state.state.wallet.amount &&
	       after.wallet_revision == state.state.wallet_revision);
	assert((after.bank.amount == economic_coin_vector{ 0, 0, 0, 1000002 }) &&
	       after.bank_revision == 10);
	std::optional<currency_prepared_mutation> bank_only;
	assert(currency_prepare_mutation(payload, state.state, 0, UINT64_MAX,
					 currency_revision_policy::bank_only, &bank_only) == 0);
	assert(bank_only->after().wallet_revision == 4 && bank_only->after().bank_revision == 10);
	auto maximum_wallet_revision = state.state;
	maximum_wallet_revision.wallet_revision = UINT64_MAX;
	assert(currency_prepare_mutation(payload, maximum_wallet_revision, 0, UINT64_MAX,
					 currency_revision_policy::bank_only, &bank_only) == 0);
	assert(bank_only->after().wallet_revision == UINT64_MAX);
	auto mixed = payload;
	mixed.wallet_delta.amount[0] = 1;
	assert(currency_prepare_mutation(mixed, state.state, 4, UINT64_MAX,
					 currency_revision_policy::bank_only,
					 &bank_only) == EINVAL);
	const auto &plan = prepared->plan();
	assert(plan.accounts.size() == 2 && plan.postings.size() == 2 &&
	       plan.accounts[0].key.kind == economic_account_kind::bank &&
	       plan.accounts[1].key.kind == economic_account_kind::issuance);
	assert(plan.accounts[0].before == state.state.bank.amount &&
	       plan.accounts[0].after == after.bank.amount &&
	       plan.accounts[0].before_revision == 9 && plan.accounts[0].after_revision == 10);
	assert((plan.postings[0].delta == economic_coin_vector{ 0, 0, 0, 1000000 }) &&
	       plan.postings[0].copper == 1000000000 &&
	       (plan.postings[1].delta == economic_coin_vector{ 0, 0, 0, -1000000 }) &&
	       plan.postings[1].copper == -1000000000);
	assert(economic_plan_validate_structure(plan) == error::ok);
	assert(prepared->agrees_with(plan) == error::ok);
	auto changed_plan = plan;
	++changed_plan.metadata.source_event->slot;
	assert(prepared->agrees_with(changed_plan) == error::payload_conflict);
	std::optional<economic_prepared_currency> sql_prepared;
	assert(economic_chaos_starter_bank_prepare(command, intent, state,
						   currency_revision_policy::sql_legacy,
						   &sql_prepared) == error::ok);
	const auto &sql_after = sql_prepared->mutation().after();
	const auto &sql_plan = sql_prepared->plan();
	assert(sql_after.wallet_revision == state.state.wallet_revision + 1 &&
	       sql_after.wallet.amount == state.state.wallet.amount &&
	       sql_after.bank_revision == state.state.bank_revision + 1);
	assert(sql_plan.accounts.size() == 3 && sql_plan.postings.size() == 2 &&
	       sql_plan.accounts[0].key.kind == economic_account_kind::wallet &&
	       sql_plan.accounts[0].before == sql_plan.accounts[0].after &&
	       sql_plan.accounts[0].after_revision == state.state.wallet_revision + 1 &&
	       sql_plan.accounts[1].key.kind == economic_account_kind::bank &&
	       sql_plan.accounts[2].key.kind == economic_account_kind::issuance &&
	       sql_plan.postings[0].account_index == 1 && sql_plan.postings[1].account_index == 2);
	assert(economic_plan_validate_structure(sql_plan) == error::ok);
	export_telemetry_plan(command, *sql_prepared);
	auto wrong_source = intent;
	++wrong_source.admission.metadata.source_event->slot;
	assert(economic_intent_encode(wrong_source, &command.accounting_intent) == error::ok);
	assert(economic_chaos_starter_bank_prepare(command, wrong_source, state,
						   currency_revision_policy::bank_only,
						   &prepared) == error::unauthorized);
	auto wrong_writer = intent;
	wrong_writer.admission.metadata.writer_id = ECONOMIC_WRITER_BANK_DEPOSIT;
	assert(economic_intent_encode(wrong_writer, &command.accounting_intent) == error::ok);
	assert(economic_chaos_starter_bank_prepare(command, wrong_writer, state,
						   currency_revision_policy::bank_only,
						   &prepared) == error::unauthorized);
	command.accounting_intent = frozen;
	auto changed = state;
	changed.state.bank.amount[3] = INT_MAX - 999999;
	assert(economic_chaos_starter_bank_prepare(command, intent, changed,
						   currency_revision_policy::bank_only,
						   &prepared) == error::overflow);
	changed = state;
	++changed.state.bank_revision;
	command.expected_revisions[1].revision = 9;
	assert(economic_chaos_starter_bank_prepare(command, intent, changed,
						   currency_revision_policy::bank_only,
						   &prepared) == error::payload_conflict);
	command.expected_revisions[1].revision = UINT64_MAX;
	critical_command stale_command = {};
	assert(currency_command_build(&stale_command, operation, payload, 4, 9,
				      critical_source_site::login,
				      critical_deadline_class::recovery));
	assert(economic_chaos_starter_bank_intent(stale_command, state.epoch, state.wallet_account,
						  state.bank_account,
						  &stale_command.accounting_intent) == error::ok);
	stale_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	economic_frozen_intent stale_intent;
	assert(economic_intent_decode(stale_command.accounting_intent, &stale_intent) == error::ok);
	assert(economic_chaos_starter_bank_prepare(stale_command, stale_intent, changed,
						   currency_revision_policy::bank_only,
						   &prepared) == error::stale_revision);
	changed = state;
	changed.epoch = id(9);
	assert(economic_chaos_starter_bank_prepare(command, intent, changed,
						   currency_revision_policy::bank_only,
						   &prepared) == error::unauthorized);
	command.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	command.accounting_intent.clear();
	auto refused = std::vector<uint8_t>{ 99 };
	command.operation_id = id(99);
	assert(economic_chaos_starter_bank_intent(command, state.epoch, state.wallet_account,
						  state.bank_account,
						  &refused) == error::unauthorized);
	assert(refused == std::vector<uint8_t>{ 99 });
	command.operation_id = operation;
	command.source_site = critical_source_site::command;
	assert(economic_chaos_starter_bank_intent(command, state.epoch, state.wallet_account,
						  state.bank_account,
						  &refused) == error::unauthorized);
	command.source_site = critical_source_site::login;
	++payload.bank_delta.amount[3];
	assert(currency_command_encode_payload(payload, &command.payload));
	assert(economic_chaos_starter_bank_intent(command, state.epoch, state.wallet_account,
						  state.bank_account,
						  &refused) == error::unauthorized);
}
int main()
{
	prepare_and_agree();
	quest_wallet_reward();
	mutation_policy();
	chaos_starter_bank_supply();
	std::cout
		<< "typed bank transfers, starter supply and shared currency preparation passed\n";
}
