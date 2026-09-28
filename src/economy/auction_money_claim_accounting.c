#include "economy/auction_money_claim_accounting.h"

#include <openssl/sha.h>

#include <algorithm>
#include <climits>
#include <new>
#include <span>
#include <utility>

namespace
{
using error = economic_accounting_error;

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u16(std::vector<uint8_t> *bytes, uint16_t value)
{
	for (size_t index = 0; index < 2; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

bool valid(const auction_command_payload &payload, const economic_account_key &wallet,
	   const economic_account_key &bank, const economic_account_key &claim_account,
	   const auction_money_claim_state &claim)
{
	if (payload.action != auction_action::claim_money || !payload.actor_pid ||
	    payload.actor_pid != claim.beneficiary_pid || claim.money <= 0 ||
	    claim.money > INT_MAX || claim.revision == UINT64_MAX || claim.sources.empty() ||
	    claim.sources.size() > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES ||
	    !economic_account_key_valid(wallet) || wallet.kind != economic_account_kind::wallet ||
	    wallet.context_id || !economic_account_key_valid(bank) ||
	    bank.kind != economic_account_kind::bank || bank.context_id != payload.racewar ||
	    !economic_account_key_valid(claim_account) ||
	    claim_account.kind != economic_account_kind::pending_claim ||
	    claim_account.context_id || wallet.lineage.bytes != bank.lineage.bytes ||
	    wallet.lineage.bytes != claim_account.lineage.bytes ||
	    wallet.authority_id == bank.authority_id ||
	    wallet.authority_id == claim_account.authority_id ||
	    bank.authority_id == claim_account.authority_id)
		return false;
	uint64_t total = 0;
	for (size_t index = 0; index < claim.sources.size(); ++index)
	{
		const auto &source = claim.sources[index];
		if (critical_operation_id_is_zero(source.operation) || !source.slot ||
		    source.beneficiary_pid != payload.actor_pid ||
		    source.claim_mapping_id != claim_account.authority_id || !source.amount ||
		    source.amount > static_cast<uint64_t>(INT_MAX) - total)
			return false;
		if (index)
		{
			const auto &previous = claim.sources[index - 1];
			if (source.operation.bytes < previous.operation.bytes ||
			    (source.operation.bytes == previous.operation.bytes &&
			     source.slot <= previous.slot))
				return false;
		}
		total += source.amount;
	}
	return total == static_cast<uint64_t>(claim.money);
}

economic_digest source_digest(const auction_money_claim_state &claim)
{
	static constexpr char tag[] = "DURIS-PENDING-CLAIM-SOURCES-V1";
	std::vector<uint8_t> bytes(tag, tag + sizeof(tag) - 1);
	append_u32(&bytes, static_cast<uint32_t>(claim.sources.size()));
	for (const auto &source : claim.sources)
	{
		bytes.insert(bytes.end(), source.operation.bytes.begin(),
			     source.operation.bytes.end());
		append_u16(&bytes, source.slot);
		append_u32(&bytes, source.beneficiary_pid);
		append_u64(&bytes, source.claim_mapping_id);
		append_u64(&bytes, source.amount);
	}
	economic_digest digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest;
}

std::vector<uint8_t> facts(const economic_account_key &wallet, const economic_account_key &bank,
			   const economic_account_key &claim_account,
			   const auction_money_claim_state &claim)
{
	std::vector<uint8_t> result;
	result.reserve(84);
	append_u64(&result, wallet.authority_id);
	append_u64(&result, bank.authority_id);
	append_u64(&result, claim_account.authority_id);
	append_u32(&result, claim.beneficiary_pid);
	append_u64(&result, static_cast<uint64_t>(claim.money));
	append_u64(&result, claim.revision);
	append_u32(&result, static_cast<uint32_t>(claim.sources.size()));
	const auto digest = source_digest(claim);
	result.insert(result.end(), digest.begin(), digest.end());
	return result;
}

economic_coin_vector canonical_wallet(int64_t amount)
{
	economic_coin_vector coins = {};
	constexpr int64_t units[] = { 1, 10, 100, 1000 };
	for (size_t index = coins.size(); index-- > 0;)
	{
		coins[index] = amount / units[index];
		amount %= units[index];
	}
	return coins;
}
} // namespace

economic_accounting_error auction_money_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const economic_account_key &claim_account, const auction_money_claim_state &claim,
	std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!valid(payload, wallet, bank, claim_account, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.sources.front().operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts admission;
		admission.metadata.lineage = wallet.lineage;
		admission.metadata.epoch = epoch;
		admission.metadata.original_operation_id = claim.sources.front().operation;
		admission.metadata.actor_kind = economic_actor_kind::domain;
		admission.metadata.actor_id = payload.actor_pid;
		admission.metadata.writer_id = ECONOMIC_WRITER_AUCTION_MONEY_CLAIM;
		admission.metadata.reason = economic_reason::auction_claim;
		admission.metadata.source_event = { economic_source_kind::service,
						    claim.sources.front().operation,
						    claim.sources.front().operation, claim.revision,
						    claim.sources.front().slot };
		admission.facts = facts(wallet, bank, claim_account, claim);
		return economic_intent_freeze(command, admission, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_money_claim_accounting_decode(const critical_command &command,
								economic_frozen_intent *intent,
								auction_command_payload *payload,
								economic_account_key *wallet,
								economic_account_key *bank,
								economic_account_key *claim_account)
{
	if (!intent || !payload || !wallet || !bank || !claim_account ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::claim_money ||
		    !parsed_payload.actor_pid)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() != 80)
			return error::invalid_identity;
		const auto number = [&](size_t offset, size_t width)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &meta = parsed_intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != parsed_payload.actor_pid ||
		    number(24, 4) != parsed_payload.actor_pid || !number(28, 8) ||
		    number(28, 8) > INT_MAX || !number(44, 4) ||
		    number(44, 4) > ECONOMIC_AUCTION_CLAIM_MAX_SOURCES || !meta.source_event ||
		    meta.source_event->kind != economic_source_kind::service ||
		    meta.source_event->source.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->generation.bytes != meta.original_operation_id.bytes ||
		    meta.source_event->sequence != number(36, 8) || !meta.source_event->slot)
			return error::invalid_identity;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     number(0, 8), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   number(8, 8), parsed_payload.racewar };
		const economic_account_key parsed_claim = { meta.lineage,
							    economic_account_kind::pending_claim,
							    number(16, 8), 0 };
		if (!economic_account_key_valid(parsed_wallet) ||
		    !economic_account_key_valid(parsed_bank) ||
		    !economic_account_key_valid(parsed_claim) ||
		    parsed_wallet.authority_id == parsed_bank.authority_id ||
		    parsed_wallet.authority_id == parsed_claim.authority_id ||
		    parsed_bank.authority_id == parsed_claim.authority_id)
			return error::invalid_identity;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		*claim_account = parsed_claim;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_money_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_money_claim_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		const auto &meta = intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_MONEY_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.lineage.bytes != authority.wallet.lineage.bytes)
			return error::unauthorized;
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		status = auction_money_claim_accounting_intent(projected, authority.epoch,
							       authority.wallet, authority.bank,
							       authority.claim_account,
							       authority.claim, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::stale_revision;
		const auto &before = authority.balances_before;
		int64_t wallet_before = 0;
		status = economic_coin_value(before.wallet.amount, &wallet_before);
		if (status != error::ok)
			return status;
		auction_command_payload payload = {};
		if (!auction_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		if (wallet_before > INT64_MAX - authority.claim.money)
			return error::overflow;
		if (before.wallet_revision != payload.expected_wallet_revision ||
		    before.bank_revision != payload.expected_bank_revision ||
		    before.wallet_revision == UINT64_MAX || before.bank_revision == UINT64_MAX)
			return error::stale_revision;
		const auto after = canonical_wallet(wallet_before + authority.claim.money);
		if (result.action != auction_action::claim_money ||
		    result.event_type != auction_event_type::money_claimed || result.auction_id ||
		    result.status || result.seller_pid || result.winner_pid ||
		    result.previous_bidder_pid || result.final_price ||
		    result.wallet_value_delta != authority.claim.money ||
		    result.wallet.amount != after || result.bank.amount != before.bank.amount ||
		    result.wallet_revision != before.wallet_revision + 1 ||
		    result.bank_revision != before.bank_revision + 1 ||
		    result.auction_revision != result.wallet_revision ||
		    result.player_owner_revision || result.auction_owner_revision ||
		    result.item_count ||
		    std::any_of(result.item_uids.begin(), result.item_uids.end(),
				[](uint64_t uid) { return uid != 0; }) ||
		    std::any_of(result.item_revisions.begin(), result.item_revisions.end(),
				[](uint64_t rev) { return rev != 0; }))
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back({ authority.wallet, before.wallet.amount, after,
					       before.wallet_revision, result.wallet_revision });
		candidate.accounts.push_back({ authority.claim_account,
					       { authority.claim.money, 0, 0, 0 },
					       {},
					       authority.claim.revision,
					       authority.claim.revision + 1 });
		economic_coin_vector wallet_delta = {};
		status = economic_coin_delta(before.wallet.amount, after, &wallet_delta);
		if (status != error::ok)
			return status;
		candidate.postings.push_back({ 0, 0, 0, wallet_delta, authority.claim.money });
		candidate.postings.push_back(
			{ 1, 1, 0, { -authority.claim.money, 0, 0, 0 }, -authority.claim.money });
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}
