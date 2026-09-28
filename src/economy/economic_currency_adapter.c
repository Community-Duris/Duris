#include "economy/economic_currency_adapter.h"

#include <cerrno>
#include <new>
#include <utility>

namespace
{
uint32_t writer_for(currency_reason_type reason)
{
	if (reason == currency_reason_type::atm_deposit)
		return ECONOMIC_WRITER_BANK_DEPOSIT;
	if (reason == currency_reason_type::atm_withdraw)
		return ECONOMIC_WRITER_BANK_WITHDRAW;
	return 0;
}
bool account_pair(const economic_account_key &wallet, const economic_account_key &bank)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet &&
	       bank.kind == economic_account_kind::bank && wallet.context_id == 0 &&
	       wallet.lineage.bytes == bank.lineage.bytes;
}
std::vector<uint8_t> bank_facts(const economic_account_key &wallet,
				const economic_account_key &bank)
{
	std::vector<uint8_t> result(ECONOMIC_BANK_FACT_BYTES);
	const uint64_t fields[] = { wallet.authority_id, bank.authority_id, bank.context_id };
	for (size_t field = 0; field < 3; ++field)
		for (size_t byte = 0; byte < 8; ++byte)
			result[field * 8 + byte] =
				static_cast<uint8_t>(fields[field] >> (byte * 8));
	return result;
}
economic_accounting_error mutation_error(unsigned int error)
{
	switch (error)
	{
	case 0:
		return economic_accounting_error::ok;
	case ESTALE:
		return economic_accounting_error::stale_revision;
	case ERANGE:
		return economic_accounting_error::overflow;
	case ENOSPC:
		return economic_accounting_error::negative_holding;
	case EILSEQ:
		return economic_accounting_error::corrupt_evidence;
	default:
		return economic_accounting_error::invalid_identity;
	}
}
economic_accounting_error transfer_policy(const currency_command_payload &payload)
{
	const auto writer = writer_for(payload.reason);
	if (!writer)
		return economic_accounting_error::unauthorized;
	// ATM writers transfer matching denominations; conversion requires a separate
	// capability even when the resulting copper value balances.
	for (size_t part = 0; part < payload.wallet_delta.amount.size(); ++part)
	{
		const auto debit = payload.wallet_delta.amount[part];
		const auto credit = payload.bank_delta.amount[part];
		if ((writer == ECONOMIC_WRITER_BANK_DEPOSIT && (debit > 0 || credit < 0)) ||
		    (writer == ECONOMIC_WRITER_BANK_WITHDRAW && (debit < 0 || credit > 0)))
			return economic_accounting_error::unauthorized;
		// The direction check makes this sum overflow-safe without negation.
		if (debit + credit != 0)
			return economic_accounting_error::unbalanced;
	}
	int64_t wallet = 0, bank = 0;
	auto result = economic_coin_value(payload.wallet_delta.amount, &wallet);
	if (result != economic_accounting_error::ok)
		return result;
	result = economic_coin_value(payload.bank_delta.amount, &bank);
	if (result != economic_accounting_error::ok)
		return result;
	// Avoid negating an unbounded integer. Opposite nonzero signs ensure the
	// sum cannot overflow, even at the endpoints of the signed range.
	if ((writer == ECONOMIC_WRITER_BANK_DEPOSIT && !(wallet < 0 && bank > 0)) ||
	    (writer == ECONOMIC_WRITER_BANK_WITHDRAW && !(wallet > 0 && bank < 0)))
		return economic_accounting_error::unauthorized;
	if (wallet + bank != 0)
		return economic_accounting_error::unbalanced;
	return economic_accounting_error::ok;
}
}

economic_prepared_currency::economic_prepared_currency(currency_prepared_mutation mutation,
						       economic_accounting_plan plan,
						       std::vector<uint8_t> encoded)
	: mutation_(std::move(mutation))
	, plan_(std::move(plan))
	, encoded_(std::move(encoded))
{
}

economic_accounting_error
economic_prepared_currency::agrees_with(const economic_accounting_plan &candidate) const
{
	std::vector<uint8_t> encoded;
	const auto result = economic_plan_encode(candidate, &encoded);
	if (result != economic_accounting_error::ok)
		return result;
	return encoded == encoded_ ? economic_accounting_error::ok :
				     economic_accounting_error::payload_conflict;
}

economic_accounting_error economic_bank_transfer_intent(const critical_command &command,
							const critical_operation_id &epoch,
							const economic_account_key &wallet,
							const economic_account_key &bank,
							std::vector<uint8_t> *encoded)
{
	if (!encoded || !account_pair(wallet, bank))
		return economic_accounting_error::invalid_identity;
	currency_command_payload payload = {};
	if (!currency_command_decode_payload(command, &payload))
		return economic_accounting_error::corrupt_evidence;
	auto result = transfer_policy(payload);
	if (result != economic_accounting_error::ok)
		return result;
	if (bank.context_id != payload.racewar)
		return economic_accounting_error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = wallet.authority_id;
		facts.metadata.writer_id = writer_for(payload.reason);
		facts.metadata.reason = economic_reason::bank_transfer;
		facts.facts = bank_facts(wallet, bank);
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}

economic_accounting_error economic_bank_transfer_prepare(
	const critical_command &command, const economic_frozen_intent &intent,
	const economic_currency_authority &authority, currency_revision_policy revision_policy,
	std::optional<economic_prepared_currency> *prepared)
{
	if (!prepared || !account_pair(authority.wallet_account, authority.bank_account))
		return economic_accounting_error::invalid_identity;
	try
	{
		auto result = economic_intent_verify_binding(command, intent);
		if (result != economic_accounting_error::ok)
			return result;
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload))
			return economic_accounting_error::corrupt_evidence;
		result = transfer_policy(payload);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &meta = intent.admission.metadata;
		if (meta.epoch.bytes != authority.epoch.bytes ||
		    meta.writer_id != writer_for(payload.reason) ||
		    meta.reason != economic_reason::bank_transfer ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != authority.wallet_account.authority_id || meta.source_event ||
		    !critical_operation_id_is_zero(meta.original_operation_id) ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    authority.bank_account.context_id != payload.racewar ||
		    !critical_entity_key_equal(authority.player_fence, command.keys[0]) ||
		    !critical_entity_key_equal(authority.bank_fence, command.keys[1]) ||
		    intent.admission.facts !=
			    bank_facts(authority.wallet_account, authority.bank_account))
			return economic_accounting_error::unauthorized;
		std::optional<currency_prepared_mutation> mutation;
		const auto domain_error = currency_prepare_mutation(
			payload, authority.state, command.expected_revisions[0].revision,
			command.expected_revisions[1].revision, revision_policy, &mutation);
		if (domain_error)
			return mutation_error(domain_error);
		economic_accounting_plan plan;
		result = economic_intent_plan_metadata(command, intent, &plan.metadata);
		if (result != economic_accounting_error::ok)
			return result;
		const auto &before = mutation->before();
		const auto &after = mutation->after();
		plan.accounts = { { authority.wallet_account, before.wallet.amount,
				    after.wallet.amount, before.wallet_revision,
				    after.wallet_revision },
				  { authority.bank_account, before.bank.amount, after.bank.amount,
				    before.bank_revision, after.bank_revision } };
		int64_t wallet = 0, bank = 0;
		result = economic_coin_value(payload.wallet_delta.amount, &wallet);
		if (result != economic_accounting_error::ok)
			return result;
		result = economic_coin_value(payload.bank_delta.amount, &bank);
		if (result != economic_accounting_error::ok)
			return result;
		plan.postings = { { 0, 0, 0, payload.wallet_delta.amount, wallet },
				  { 1, 1, 0, payload.bank_delta.amount, bank } };
		result = economic_plan_normalize(&plan);
		if (result != economic_accounting_error::ok)
			return result;
		std::vector<uint8_t> encoded;
		result = economic_plan_encode(plan, &encoded);
		if (result != economic_accounting_error::ok)
			return result;
		*prepared =
			economic_prepared_currency(*mutation, std::move(plan), std::move(encoded));
		return economic_accounting_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return economic_accounting_error::capacity;
	}
}
