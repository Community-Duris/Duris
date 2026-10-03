#include "economy/collector_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <utility>

namespace
{
using error = economic_accounting_error;

bool account_pair(const economic_account_key &wallet, const economic_account_key &bank,
		  uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet &&
	       bank.kind == economic_account_kind::bank && wallet.context_id == 0 &&
	       bank.context_id == racewar && wallet.lineage.bytes == bank.lineage.bytes;
}

economic_account_key sink_for(const economic_account_key &wallet)
{
	return { wallet.lineage, economic_account_kind::sink, ECONOMIC_COLLECTOR_PURCHASE_SINK_ID,
		 0 };
}

bool death_id(const collector::record &listing, critical_operation_id *operation)
{
	return collector::valid_record(listing) &&
	       critical_operation_id_from_hex(listing.death_operation.data(), operation);
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t index = 0; index < sizeof(value); ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (8 * index)));
}

uint64_t read_u64(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= uint64_t(bytes[offset + index]) << (8 * index);
	return value;
}

error listing_facts(const collector::record &listing, const economic_account_key &wallet,
		    const economic_account_key &bank, std::vector<uint8_t> *facts)
{
	std::array<uint8_t, collector::encoded_record_bytes> record = {};
	if (collector::record_encode(listing, &record) != collector::codec_result::ok)
		return error::corrupt_evidence;
	facts->reserve(16 + record.size());
	append_u64(facts, wallet.authority_id);
	append_u64(facts, bank.authority_id);
	facts->insert(facts->end(), record.begin(), record.end());
	return error::ok;
}

bool command_matches_listing(const collector_command_payload &payload,
			     const collector::record &listing)
{
	return payload.action == collector_action::purchase &&
	       listing.status == collector::state::available &&
	       listing.listing == payload.listing &&
	       listing.revision == payload.expected_listing_revision &&
	       listing.uid == payload.selected_item_uid &&
	       listing.uid == payload.items[0].item_uid &&
	       listing.item_revision == payload.items[0].expected_item_revision &&
	       listing.beneficiary == payload.actor_pid && listing.price_value &&
	       listing.price_value <= static_cast<uint64_t>(INT64_MAX);
}

bool command_matches_held(const collector_command_payload &payload,
			  const collector::record &listing)
{
	return (payload.action == collector_action::expire ||
		payload.action == collector_action::cancel) &&
	       (listing.status == collector::state::collected ||
		listing.status == collector::state::available) &&
	       listing.listing == payload.listing &&
	       listing.revision == payload.expected_listing_revision &&
	       listing.uid == payload.selected_item_uid &&
	       listing.uid == payload.items[0].item_uid &&
	       listing.item_revision == payload.items[0].expected_item_revision;
}

bool same_record(const collector::record &left, const collector::record &right)
{
	std::array<uint8_t, collector::encoded_record_bytes> a = {}, b = {};
	return collector::record_encode(left, &a) == collector::codec_result::ok &&
	       collector::record_encode(right, &b) == collector::codec_result::ok && a == b;
}

economic_coin_vector canonical_wallet(int64_t copper)
{
	economic_coin_vector result = {};
	constexpr int64_t units[] = { 1, 10, 100, 1000 };
	for (size_t index = result.size(); index-- > 0;)
	{
		result[index] = copper / units[index];
		copper %= units[index];
	}
	return result;
}
} // namespace

economic_accounting_error collector_purchase_accounting_intent(const critical_command &command,
							       const critical_operation_id &epoch,
							       const economic_account_key &wallet,
							       const economic_account_key &bank,
							       const collector::record &listing,
							       std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	collector_command_payload payload = {};
	if (!collector_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	critical_operation_id death = {};
	if (!account_pair(wallet, bank, payload.racewar) || !death_id(listing, &death) ||
	    critical_operation_id_equal(death, command.operation_id) ||
	    !command_matches_listing(payload, listing))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = death;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_COLLECTOR_PURCHASE;
		facts.metadata.reason = economic_reason::collector_purchase;
		auto status = listing_facts(listing, wallet, bank, &facts.facts);
		if (status != error::ok)
			return status;
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_purchase_accounting_decode(const critical_command &command,
							       economic_frozen_intent *intent,
							       collector_command_payload *payload,
							       collector::record *listing,
							       economic_account_key *wallet,
							       economic_account_key *bank)
{
	if (!intent || !payload || !listing || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		collector_command_payload parsed_payload = {};
		if (!collector_command_decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != collector_action::purchase)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto &facts = parsed_intent.admission.facts;
		if (facts.size() != 16 + collector::encoded_record_bytes)
			return error::invalid_identity;
		collector::record parsed_listing;
		if (collector::record_decode(facts.data() + 16, collector::encoded_record_bytes,
					     &parsed_listing) != collector::codec_result::ok)
			return error::corrupt_evidence;
		const auto &meta = parsed_intent.admission.metadata;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     read_u64(facts, 0), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   read_u64(facts, 8),
							   parsed_payload.racewar };
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto status = collector_purchase_accounting_intent(projected, meta.epoch,
									 parsed_wallet, parsed_bank,
									 parsed_listing, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*listing = parsed_listing;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_purchase_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const collector_purchase_accounting_authority &authority,
	const collector_command_result &result, economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		collector_command_payload payload = {};
		if (!collector_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		critical_operation_id death = {};
		if (!account_pair(authority.wallet_account, authority.bank_account,
				  payload.racewar) ||
		    !death_id(authority.listing_before, &death) ||
		    !command_matches_listing(payload, authority.listing_before))
			return error::invalid_identity;
		const auto &meta = intent.admission.metadata;
		std::vector<uint8_t> facts;
		status = listing_facts(authority.listing_before, authority.wallet_account,
				       authority.bank_account, &facts);
		if (status != error::ok)
			return status;
		if (meta.writer_id != ECONOMIC_WRITER_COLLECTOR_PURCHASE ||
		    meta.reason != economic_reason::collector_purchase ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || meta.source_event ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != death.bytes ||
		    intent.admission.facts != facts)
			return error::unauthorized;
		const auto &item = authority.item_before;
		const auto &position = item.position;
		if (item.uid != payload.selected_item_uid ||
		    !item_owner_identity_equal(position.owner, payload.from_owner) ||
		    position.root_uid != item.uid || position.parent_uid ||
		    position.revision != payload.items[0].expected_item_revision ||
		    position.state != item_custody_state::active ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision ||
		    authority.from_owner_revision_before != payload.expected_from_owner_revision ||
		    authority.to_owner_revision_before != payload.expected_to_owner_revision)
			return error::stale_revision;
		if (authority.catalog_revision_before == UINT64_MAX ||
		    authority.from_owner_revision_before == UINT64_MAX ||
		    authority.to_owner_revision_before == UINT64_MAX ||
		    authority.balances_before.wallet_revision == UINT64_MAX ||
		    authority.balances_before.bank_revision == UINT64_MAX)
			return error::overflow;
		int64_t wallet_value = 0;
		status =
			economic_coin_value(authority.balances_before.wallet.amount, &wallet_value);
		if (status != error::ok)
			return status;
		const int64_t price = static_cast<int64_t>(authority.listing_before.price_value);
		if (wallet_value < price)
			return error::negative_holding;
		collector::record purchased = authority.listing_before;
		if (collector::purchase(&purchased, payload.expected_listing_revision,
					payload.actor_pid, static_cast<uint64_t>(wallet_value),
					payload.capacity_admitted,
					payload.observed_at) != collector::outcome::applied)
			return error::stale_revision;
		const auto wallet_after = canonical_wallet(wallet_value - price);
		if (!result.record_present || result.action != collector_action::purchase ||
		    !same_record(result.entry, purchased) ||
		    result.catalog_revision != authority.catalog_revision_before + 1 ||
		    result.from_owner_revision != authority.from_owner_revision_before + 1 ||
		    result.to_owner_revision != authority.to_owner_revision_before + 1 ||
		    result.wallet.amount != wallet_after ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision + 1 ||
		    result.bank_revision != authority.balances_before.bank_revision + 1)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		const auto sink = sink_for(authority.wallet_account);
		candidate.accounts = {
			{ authority.wallet_account, authority.balances_before.wallet.amount,
			  result.wallet.amount, authority.balances_before.wallet_revision,
			  result.wallet_revision },
			{ authority.bank_account, authority.balances_before.bank.amount,
			  result.bank.amount, authority.balances_before.bank_revision,
			  result.bank_revision },
			{ sink, {}, {}, 0, 0 }
		};
		economic_coin_vector wallet_delta = {};
		status = economic_coin_delta(candidate.accounts[0].before,
					     candidate.accounts[0].after, &wallet_delta);
		if (status != error::ok)
			return status;
		candidate.postings = { { 0, 0, 0, wallet_delta, -price },
				       { 1, 2, 0, { price, 0, 0, 0 }, price } };
		economic_item_position after = position;
		after.owner = payload.to_owner;
		after.revision = purchased.item_revision;
		candidate.items_before = { item };
		candidate.items_after = { { item.uid, after } };
		candidate.item_events = { { 0, 0, item.uid, position, after } };
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

economic_accounting_error collector_held_accounting_intent(const critical_command &command,
							   const critical_operation_id &lineage,
							   const critical_operation_id &epoch,
							   const collector::record &listing,
							   std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(lineage) || critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	collector_command_payload payload = {};
	if (!collector_command_decode_payload(command, &payload))
		return error::corrupt_evidence;
	critical_operation_id death = {};
	if (!death_id(listing, &death) ||
	    critical_operation_id_equal(death, command.operation_id) ||
	    !command_matches_held(payload, listing))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = death;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = listing.listing;
		facts.metadata.writer_id = ECONOMIC_WRITER_COLLECTOR_HELD;
		facts.metadata.reason = economic_reason::collector_custody;
		std::array<uint8_t, collector::encoded_record_bytes> record = {};
		if (collector::record_encode(listing, &record) != collector::codec_result::ok)
			return error::corrupt_evidence;
		facts.facts.assign(record.begin(), record.end());
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_held_accounting_decode(const critical_command &command,
							   economic_frozen_intent *intent,
							   collector_command_payload *payload,
							   collector::record *listing)
{
	if (!intent || !payload || !listing ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		collector_command_payload parsed_payload = {};
		if (!collector_command_decode_payload(command, &parsed_payload) ||
		    (parsed_payload.action != collector_action::expire &&
		     parsed_payload.action != collector_action::cancel))
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto &facts = parsed_intent.admission.facts;
		if (facts.size() != collector::encoded_record_bytes)
			return error::invalid_identity;
		collector::record parsed_listing;
		if (collector::record_decode(facts.data(), facts.size(), &parsed_listing) !=
		    collector::codec_result::ok)
			return error::corrupt_evidence;
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto &meta = parsed_intent.admission.metadata;
		const auto status = collector_held_accounting_intent(
			projected, meta.lineage, meta.epoch, parsed_listing, &expected);
		if (status != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*listing = parsed_listing;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_held_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const collector_held_accounting_authority &authority,
	const collector_command_result &result, economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		collector_command_payload payload = {};
		if (!collector_command_decode_payload(command, &payload))
			return error::corrupt_evidence;
		critical_operation_id death = {};
		const auto &listing = authority.listing_before;
		if (!death_id(listing, &death) || !command_matches_held(payload, listing))
			return error::invalid_identity;
		std::array<uint8_t, collector::encoded_record_bytes> record = {};
		if (collector::record_encode(listing, &record) != collector::codec_result::ok)
			return error::corrupt_evidence;
		const auto &meta = intent.admission.metadata;
		if (meta.writer_id != ECONOMIC_WRITER_COLLECTOR_HELD ||
		    meta.reason != economic_reason::collector_custody ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != listing.listing || meta.source_event ||
		    meta.lineage.bytes != authority.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != death.bytes ||
		    intent.admission.facts.size() != record.size() ||
		    !std::equal(record.begin(), record.end(), intent.admission.facts.begin()))
			return error::unauthorized;
		const auto &item = authority.item_before;
		const auto &position = item.position;
		if (item.uid != payload.selected_item_uid ||
		    !item_owner_identity_equal(position.owner, payload.from_owner) ||
		    position.root_uid != item.uid || position.parent_uid ||
		    position.revision != payload.items[0].expected_item_revision ||
		    position.state != item_custody_state::active ||
		    authority.from_owner_revision_before != payload.expected_from_owner_revision ||
		    authority.to_owner_revision_before != payload.expected_to_owner_revision)
			return error::stale_revision;
		if (authority.catalog_revision_before == UINT64_MAX ||
		    authority.from_owner_revision_before == UINT64_MAX ||
		    authority.to_owner_revision_before == UINT64_MAX)
			return error::overflow;
		collector::record updated = listing;
		const auto outcome =
			payload.action == collector_action::expire ?
				collector::expire(&updated, payload.expected_listing_revision,
						  payload.observed_at) :
				collector::cancel(&updated, payload.expected_listing_revision,
						  payload.cancel_reason);
		if (outcome != collector::outcome::applied)
			return error::stale_revision;
		if (!result.record_present || result.action != payload.action ||
		    result.materialized_item_id || !same_record(result.entry, updated) ||
		    result.catalog_revision != authority.catalog_revision_before + 1 ||
		    result.from_owner_revision != authority.from_owner_revision_before + 1 ||
		    result.to_owner_revision != authority.to_owner_revision_before + 1 ||
		    result.wallet_revision || result.bank_revision ||
		    result.wallet.amount != currency_vector{}.amount ||
		    result.bank.amount != currency_vector{}.amount)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		economic_item_position after = position;
		after.owner = payload.to_owner;
		after.revision = updated.item_revision;
		after.state = payload.target_state;
		candidate.items_before = { item };
		candidate.items_after = { { item.uid, after } };
		candidate.item_events = { { 0, 0, item.uid, position, after } };
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
