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

#include <type_traits>
namespace
{
bool collector_account_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
struct collector_account_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *projection = nullptr;
	const economic_frozen_intent *intent = nullptr;
	const economic_admission_facts *facts = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	mutable bool denied = false;
	bool prefix(size_t &bytes, size_t extra = 0) const noexcept
	{
		bytes = outer;
		size_t heap = 0;
		const bool observed =
			collector_account_add(bytes, sizeof(*this)) &&
			collector_account_add(bytes, frames) &&
			collector_account_add(bytes, critical_command_copy_frame_bytes()) &&
			collector_account_add(bytes, 8 * sizeof(void *) + 8 * sizeof(size_t) +
							     4 * sizeof(bool)) &&
			(!projection || (critical_command_current_heap_bytes(*projection, &heap) &&
					 collector_account_add(bytes, heap))) &&
			(!intent ||
			 collector_account_add(bytes, intent->admission.facts.capacity())) &&
			(!facts || collector_account_add(bytes, facts->facts.capacity())) &&
			(!expected || collector_account_add(bytes, expected->capacity())) &&
			collector_account_add(bytes, extra);
		if (!observed)
			denied = true;
		return observed;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		if (!prefix(bytes, extra) || !reserve || !reserve(bytes, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool forward(size_t bytes, void *opaque) noexcept
	{
		auto &budget = *static_cast<collector_account_budget *>(opaque);
		if (!budget.reserve || !budget.reserve(bytes, budget.context))
		{
			budget.denied = true;
			return false;
		}
		return true;
	}
};
// Original listing_facts owns its encoded array; record_encode owns a separate
// candidate array. record_decode owns reader/candidate/flags. These genuine
// nested fixed owners are additional to the caller's output record and facts.
constexpr size_t collector_record_source_frames =
	sizeof(std::array<uint8_t, collector::encoded_record_bytes>) + sizeof(collector::record) +
	// encode/decode, put/get, put/get_operation, hexadecimal, validity/ID decode.
	19 * sizeof(void *) + 11 * sizeof(size_t) + 7 * sizeof(uint8_t) + 5 * sizeof(int) +
	3 * sizeof(uint64_t) + 8 * sizeof(bool) + sizeof(std::string_view) +
	// get_operation's genuine hexadecimal_digits array; all_of/any_of/
	// find_if/find_if_not iterator/predicate/category and return carriers.
	17 * sizeof(char) + 22 * sizeof(void *) + 6 * sizeof(char) + 5 * sizeof(bool);
}
size_t collector_purchase_accounting_replay_observer_frame_bytes() noexcept
{
	return 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
}
bool collector_purchase_accounting_replay_heap_bytes(const economic_frozen_intent &intent,
						     size_t *output) noexcept
{
	// Payload, record, wallet and bank contain only fixed arrays/scalars. Their
	// authentic inline objects remain caller-owned, not duplicated in this heap.
	static_assert(std::is_trivially_copyable_v<collector_command_payload>);
	static_assert(std::is_trivially_copyable_v<collector::record>);
	static_assert(std::is_trivially_copyable_v<economic_account_key>);
	if (!output)
		return false;
	*output = intent.admission.facts.capacity();
	return true;
}

economic_accounting_error collector_purchase_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const collector::record &listing, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	const size_t frames = sizeof(collector_command_payload) + sizeof(economic_admission_facts) +
			      sizeof(critical_operation_id) +
			      sizeof(std::array<uint8_t, collector::encoded_record_bytes>) +
			      10 * sizeof(void *) + 8 * sizeof(size_t) + 5 * sizeof(bool) +
			      sizeof(error) + 2 * sizeof(uint64_t) +
			      critical_command_valid_frame_bytes() + collector_record_source_frames;
	collector_account_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!budget.peak())
		return error::capacity;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	collector_command_payload payload = {};
	if (!budget.prefix(nested) || !collector_command_decode_payload_bounded(
					      command, &payload, collector_account_budget::forward,
					      &budget, nested, &budget.denied))
		return budget.denied ? error::capacity : error::corrupt_evidence;
	critical_operation_id death = {};
	if (!account_pair(wallet, bank, payload.racewar) || !death_id(listing, &death) ||
	    critical_operation_id_equal(death, command.operation_id) ||
	    !command_matches_listing(payload, listing))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		budget.facts = &facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = death;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_COLLECTOR_PURCHASE;
		facts.metadata.reason = economic_reason::collector_purchase;
		// Genuine fresh reserve is the original 16 mapping bytes plus encoded record.
		const size_t facts_request = 16 + collector::encoded_record_bytes;
		if (!budget.peak(facts_request + critical_command_copy_frame_bytes()))
			return error::capacity;
		auto status = listing_facts(listing, wallet, bank, &facts.facts);
		if (status != error::ok)
			return status;
		if (!budget.prefix(nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(command, facts, encoded,
							    collector_account_budget::forward,
							    &budget, nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_purchase_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	collector_command_payload *payload, collector::record *listing,
	economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	const size_t frames = sizeof(collector_command_payload) + sizeof(economic_frozen_intent) +
			      sizeof(collector::record) + 2 * sizeof(economic_account_key) +
			      sizeof(critical_command) + sizeof(std::vector<uint8_t>) +
			      16 * sizeof(void *) + 9 * sizeof(size_t) + 7 * sizeof(bool) +
			      2 * sizeof(error) + sizeof(std::span<const uint8_t>) +
			      3 * sizeof(uint64_t) + critical_command_valid_frame_bytes() +
			      collector_record_source_frames;
	collector_account_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!budget.peak())
		return error::capacity;
	if (!intent || !payload || !listing || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		collector_command_payload parsed_payload = {};
		if (!budget.prefix(nested) ||
		    !collector_command_decode_payload_bounded(command, &parsed_payload,
							      collector_account_budget::forward,
							      &budget, nested, &budget.denied) ||
		    parsed_payload.action != collector_action::purchase)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		const auto decoded_status =
			!budget.prefix(nested) ?
				error::capacity :
				economic_intent_decode_bounded(command.accounting_intent,
							       &parsed_intent,
							       collector_account_budget::forward,
							       &budget, nested);
		if (decoded_status != error::ok)
			return budget.denied || decoded_status == error::capacity ?
				       error::capacity :
				       error::corrupt_evidence;
		const auto binding_status = !budget.prefix(nested) ?
						    error::capacity :
						    economic_intent_verify_binding_bounded(
							    command, parsed_intent,
							    collector_account_budget::forward,
							    &budget, nested);
		if (binding_status != error::ok)
			return budget.denied || binding_status == error::capacity ?
				       error::capacity :
				       error::corrupt_evidence;
		const auto &facts = parsed_intent.admission.facts;
		if (facts.size() != 16 + collector::encoded_record_bytes)
			return error::invalid_identity;
		collector::record parsed_listing;
		if (collector::record_decode(facts.data() + 16, collector::encoded_record_bytes,
					     &parsed_listing) != collector::codec_result::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto &meta = parsed_intent.admission.metadata;
		const economic_account_key parsed_wallet = { meta.lineage,
							     economic_account_kind::wallet,
							     read_u64(facts, 0), 0 };
		const economic_account_key parsed_bank = { meta.lineage,
							   economic_account_kind::bank,
							   read_u64(facts, 8),
							   parsed_payload.racewar };
		size_t request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &request) ||
		    !budget.peak(request))
			return error::capacity;
		critical_command projected = command;
		budget.projection = &projected;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		const auto status = !budget.prefix(nested) ?
					    error::capacity :
					    collector_purchase_accounting_intent_bounded(
						    projected, meta.epoch, parsed_wallet,
						    parsed_bank, parsed_listing, &expected,
						    collector_account_budget::forward, &budget,
						    nested);
		if (status != error::ok || expected != command.accounting_intent)
			return budget.denied || status == error::capacity ? error::capacity :
									    error::unauthorized;
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

// Unsealed named source ledger: no runtime/profile selection until complete join.
namespace
{
bool collector_fixed_source_policy() noexcept;
constexpr size_t collector_source_P = sizeof(void *);
constexpr size_t collector_source_N = sizeof(size_t);
constexpr size_t collector_source_B = sizeof(bool);

// Selected typed GNU13 byte-vector lifecycle. This exact constructor/cleanup/
// move graph is reused from reviewed stock14, never its unrelated codec total.
template <class T> constexpr size_t collector_vector_cleanup_source() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	constexpr size_t destruction = sizeof(V *) + sizeof(V *) + sizeof(A *) + 2 * sizeof(T *) +
				       sizeof(A *) + 2 * sizeof(T *) + 2 * sizeof(T *) +
				       sizeof(bool);
	constexpr size_t element = std::is_trivially_destructible_v<T> ? 0 : 4 * sizeof(T *);
	constexpr size_t deallocation =
		sizeof(void *) + sizeof(V *) + sizeof(T *) + sizeof(size_t) + sizeof(A *) +
		sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) + sizeof(size_t) +
		sizeof(A *) + sizeof(T *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
		sizeof(bool) + sizeof(A *);
	// _Vector_base's member/base cleanup really reaches implicit ~_Vector_impl,
	// ~_Vector_impl_data and ~__new_allocator, each with its own this carrier.
	return destruction + element + deallocation + 3 * collector_source_P;
}

template <class T> constexpr size_t collector_vector_default_source() noexcept
{
	// vector, _Vector_base, _Vector_impl, allocator, __new_allocator and
	// _Vector_impl_data default constructors: six genuine this carriers.
	return 6 * collector_source_P;
}
template <class T> constexpr size_t collector_vector_get_allocator_source() noexcept
{
	// _Vector_base::get_allocator(this); _M_get_Tp_allocator(this,returned-ref);
	// allocator(const&) and __new_allocator(const&): this/source for each.
	// The returned allocator value is not a member of the caller vector.
	return 7 * collector_source_P + sizeof(std::allocator<T>);
}
template <class T> constexpr size_t collector_vector_const_allocator_ctor_source() noexcept
{
	// vector(alloc), _Vector_base(alloc), _Vector_impl(alloc), allocator copy,
	// new_allocator copy each this/source; _Vector_impl_data default this.
	return 11 * collector_source_P;
}
template <class T> constexpr size_t collector_vector_data_swap_source() noexcept
{
	// _M_swap_data(this,source), its real three-pointer __tmp, data default
	// ctor(this), three _M_copy_data(this,source) calls, implicit tmp dtor(this).
	return 2 * collector_source_P + 3 * sizeof(T *) + collector_source_P +
	       3 * (2 * collector_source_P) + collector_source_P;
}
template <class T> constexpr size_t collector_vector_move_constructor_source() noexcept
{
	// Defaulted vector/base moves, impl move, allocator/new_allocator const
	// copies, data move: six this/source pairs. Impl makes two std::move calls
	// (reference/result each); data move's pointer() null-reset result is real.
	return 6 * (2 * collector_source_P) + 2 * (2 * collector_source_P) + sizeof(T *);
}
template <class T> constexpr size_t collector_vector_move_assignment_source() noexcept
{
	// operator=(this,source,returned-ref), named constexpr __move_storage,
	// _S_propagate_on_move_assign bool result (true short-circuits _S_always_equal),
	// std::move(ref,result), _M_move_assign(this,source,actual true_type value),
	// generated true_type ctor/dtor this. The actual __tmp vector is separate
	// from input/output vectors. get_allocator's value dies via allocator and
	// new_allocator dtors after __tmp's const-allocator construction.
	constexpr size_t entry = 3 * collector_source_P + 2 * sizeof(bool) +
				 2 * collector_source_P + 2 * collector_source_P +
				 sizeof(std::true_type) + 2 * collector_source_P;
	// C++20 __alloc_on_move(one,two), std::move(ref,result), generated allocator
	// assignment(this,source,returned-ref), generated new_allocator assignment
	// (this,source,returned-ref):10P, plus both _M_get_Tp_allocator(this,ref):4P.
	constexpr size_t allocator_move = 10 * collector_source_P + 2 * (2 * collector_source_P);
	return entry + sizeof(std::vector<T>) + collector_vector_get_allocator_source<T>() +
	       2 * collector_source_P + collector_vector_const_allocator_ctor_source<T>() +
	       2 * collector_vector_data_swap_source<T>() + allocator_move +
	       collector_vector_cleanup_source<T>();
}

// Genuine unreserved-to-reserved byte allocation route. Listing facts start
// empty and reserve exactly 16+record bytes; no old element relocation executes.
constexpr size_t collector_byte_allocator_source =
	// _M_allocate(this,n,result), _M_get_Tp_allocator(this,ref), traits::allocate
	// (alloc,n,result), allocator::allocate(this,n,result,__is_constant_evaluated),
	// new_allocator::allocate(this,n,hint,result), _M_max_size(this,result), new.
	(2 * collector_source_P + collector_source_N) + 2 * collector_source_P +
	(2 * collector_source_P + collector_source_N) +
	(2 * collector_source_P + collector_source_N + collector_source_B) +
	(3 * collector_source_P + collector_source_N) + (collector_source_P + collector_source_N) +
	collector_source_N + collector_source_P;
constexpr size_t collector_vector_query_source =
	// size/capacity receivers and actual returned integer; data receiver/result.
	2 * (collector_source_P + collector_source_N) + 2 * collector_source_P;
constexpr size_t collector_byte_max_size_source =
	// max_size(this,N), _M_get_Tp_allocator(this,ref); _S_max_size(alloc,returnedN)
	// with real diffmax/allocmax; traits::max_size, allocator::max_size and
	// new_allocator::_M_max_size; min's two references and returned reference.
	(collector_source_P + collector_source_N) + 2 * collector_source_P +
	(collector_source_P + 3 * collector_source_N) +
	3 * (collector_source_P + collector_source_N) + 3 * collector_source_P;
constexpr size_t collector_byte_reserve_source =
	// reserve(this,n), old_size and tmp; real _S_use_relocate/_S_nothrow_relocate
	// bool returns; old empty-range bitwise relocation still calls all controllers.
	2 * collector_source_P + 2 * collector_source_N + 2 * collector_source_B +
	collector_vector_query_source + collector_byte_max_size_source +
	collector_byte_allocator_source +
	// _S_relocate(4 pointers,returned pointer), __relocate_a(4 pointers,return),
	// three raw niter_base pointer/results; bitwise __relocate_a_1 owns first,
	// last,result,allocator-ref,return and n; runtime constant-evaluated bool.
	5 * collector_source_P + 5 * collector_source_P + 6 * collector_source_P +
	5 * collector_source_P + collector_source_N + collector_source_B +
	// Empty-range skips memmove. Allocator receiver/returned-ref plus actual
	// old _M_deallocate(null,n) formals still execute but no deallocation child.
	2 * collector_source_P + 2 * collector_source_P + collector_source_N;

constexpr size_t collector_byte_reserved_push_source =
	// rvalue push -> move, emplace -> forward, traits::construct -> forward,
	// construct_at -> forward, actual placement-new(size,where,result).
	2 * collector_source_P + 2 * collector_source_P + 3 * collector_source_P +
	2 * collector_source_P + 3 * collector_source_P + 2 * collector_source_P +
	3 * collector_source_P + 2 * collector_source_P + collector_source_N +
	2 * collector_source_P +
	// back -> end+normal ctor -> iterator subtract+normal ctor -> dereference;
	// subtraction pointer materializes for const-ref constructor; two dtors.
	2 * collector_source_P + 2 * collector_source_P + 2 * collector_source_P +
	2 * collector_source_P + sizeof(std::ptrdiff_t) + 2 * collector_source_P +
	2 * collector_source_P + 2 * collector_source_P + collector_source_P;

constexpr size_t collector_raw_distance_advance_source =
	// distance(first,last,returnedN) + category(first,tag) + random distance;
	// advance(first-ref,n), real named d, category, random advance(first-ref,n,tag).
	2 * collector_source_P + collector_source_N + collector_source_P +
	sizeof(std::random_access_iterator_tag) + 2 * collector_source_P + collector_source_N +
	sizeof(std::random_access_iterator_tag) + collector_source_P + 2 * collector_source_N +
	collector_source_P + sizeof(std::random_access_iterator_tag) + collector_source_P +
	collector_source_N + sizeof(std::random_access_iterator_tag) +
	// Each actual category value reaches random/bidirectional/forward/input
	// generated constructor/destructor this carriers, reused per sequential call.
	8 * collector_source_P;
constexpr size_t collector_raw_copy_source =
	// copy, __copy_move_a, __copy_move_a1, __copy_move_a2 and bitwise __copy_m:
	// each input/output/result; actual is_constant_evaluated and difference n.
	5 * (4 * collector_source_P) + collector_source_B + collector_source_N +
	// Two __miter_base, three raw __niter_base and __niter_wrap(from,res,result).
	2 * (2 * collector_source_P) + 3 * (2 * collector_source_P) + 3 * collector_source_P +
	// Actual memmove input/result/length, selected for nonempty record byte copy.
	3 * collector_source_P + collector_source_N;
constexpr size_t collector_byte_uninitialized_copy_source =
	// Allocator-specialized uninitialized_copy_a(first,last,result,alloc,return),
	// constant-evaluated result; public uninitialized_copy with can_memcpy/trivial;
	// __uninitialized_copy<true>::__uninit_copy -> genuine raw copy chain.
	5 * collector_source_P + collector_source_B + 4 * collector_source_P +
	2 * collector_source_B + 4 * collector_source_P + collector_raw_copy_source;
constexpr size_t collector_empty_move_copy_source =
	// Empty __uninitialized_move_a still creates two actual move_iterators;
	// both constructors/std::move/reference/results and generated cleanup.
	5 * collector_source_P +
	2 * (2 * collector_source_P + 2 * collector_source_P + 2 * collector_source_P +
	     collector_source_P) +
	// uninitialized_copy_a/public/__uninit_copy wrappers select copy; raw
	// copy's miter_base unwraps both iterator values via base and copy/cleanup.
	collector_byte_uninitialized_copy_source + 2 * (5 * collector_source_P);
constexpr size_t collector_byte_end_insert_source =
	// vector::insert(this,const-position,first,last,returned-iterator), real
	// offset; cbegin, both begin calls and normal constructors; const conversion.
	5 * collector_source_P + sizeof(std::ptrdiff_t) +
	3 * (2 * collector_source_P + 2 * collector_source_P) + 4 * collector_source_P +
	// Iterator difference(two refs,resultN, both base receiver/ref); begin()+
	// offset receiver/N/result, normal ctor and genuine pointer temporary.
	2 * collector_source_P + sizeof(std::ptrdiff_t) + 2 * (2 * collector_source_P) +
	2 * collector_source_P + sizeof(std::ptrdiff_t) + 2 * collector_source_P +
	collector_source_P +
	// _M_range_insert(this,position,first,last,forward-tag), n/elems_after,
	// old_finish/mid. At end elems_after=0, so the fitting-range else arm is real.
	5 * collector_source_P + 2 * collector_source_N + sizeof(std::forward_iterator_tag) +
	collector_raw_distance_advance_source +
	// end-position difference, position.base and allocator observers.
	2 * collector_source_P + 2 * collector_source_P + 2 * collector_source_P +
	sizeof(std::ptrdiff_t) + 2 * (2 * collector_source_P) + 2 * collector_source_P +
	2 * collector_source_P + collector_byte_uninitialized_copy_source +
	collector_empty_move_copy_source +
	// Final copy(first,mid,position) is empty but retains normal-iterator
	// niter_base/base/wrap/copy/dtor controllers; no byte memmove executed there.
	collector_raw_copy_source + 6 * collector_source_P +
	// Generated lifetime for actual position/return/category iterator temporaries.
	8 * collector_source_P;

constexpr size_t collector_byte_vector_equal_source =
	// vector equality(two refs,bool); two size and three begin/end queries,
	// three normal constructors; std::equal/aux1/aux raw pointer chains and memcmp.
	2 * collector_source_P + collector_source_B +
	2 * (collector_source_P + collector_source_N) +
	3 * (2 * collector_source_P + 2 * collector_source_P) +
	3 * (3 * collector_source_P + collector_source_B) +
	// Normal-iterator niter_base owns its real by-value iterator, raw result,
	// base() receiver/ref; named aux parameters and niter_base arguments copy
	// through six real iterator copy/dtor chains. Public prvalues elide copies,
	// but their three actual argument/result temporaries still destruct.
	3 * (4 * collector_source_P) + 6 * (3 * collector_source_P) + 3 * collector_source_P +
	3 * collector_source_P + sizeof(std::ptrdiff_t) + collector_source_B +
	// __equal_aux1's __simple, __memcmp constant-evaluated bool, then actual
	// __memcmp wrapper and builtin memcmp's input/length/integer-return scopes.
	2 * collector_source_B + 2 * (2 * collector_source_P + collector_source_N + sizeof(int));

constexpr size_t collector_fixed_array_equal_source =
	// array equality(two refs,bool), three begin/end -> direct array.data,
	// actual std::equal, __equal_aux/__equal_aux1 and pointer __equal dispatch.
	2 * collector_source_P + collector_source_B + 3 * (4 * collector_source_P) +
	3 * (3 * collector_source_P + collector_source_B) + 3 * (2 * collector_source_P) +
	3 * collector_source_P + sizeof(std::ptrdiff_t) + collector_source_B +
	2 * collector_source_B + 2 * (2 * collector_source_P + collector_source_N + sizeof(int));

constexpr size_t collector_record_array_source =
	// Direct array.data(this,pointer-result), begin/end(this,result) each
	// calling that data leaf; size() and subscript -> array_traits::_S_ref.
	// Source values, not duplicated fixed array storage.
	2 * collector_source_P + 2 * (4 * collector_source_P) + collector_source_P +
	collector_source_N + (2 * collector_source_P + collector_source_N) +
	(2 * collector_source_P + collector_source_N);
// This profile-only closure has the same genuine no-capture shape as the
// original zero predicate. It never runs and creates no gameplay callback.
using collector_hex_zero_predicate = decltype([](char value) { return value != '0'; });
constexpr size_t collector_hex_L = sizeof(collector_hex_zero_predicate);
// Actual GNU13 _Iter_pred<L> contains exactly one L member (no base/EBO).
constexpr size_t collector_hex_I = collector_hex_L;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13
static_assert(sizeof(__gnu_cxx::__ops::_Iter_pred<collector_hex_zero_predicate>) ==
	      collector_hex_I);
static_assert(sizeof(__gnu_cxx::__ops::_Iter_pred<bool (*)(char)>) == sizeof(void *));
static_assert(sizeof(__gnu_cxx::__ops::_Iter_negate<bool (*)(char)>) == sizeof(void *));
#endif
constexpr size_t collector_hex_function_pointer_lane_source =
	// all_of; find_if_not; __find_if_not; FOUR-arg random-access helper.
	3 * collector_source_P + collector_source_B + 4 * collector_source_P +
	4 * collector_source_P + 4 * collector_source_P + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) +
	// __iterator_category owns its const iterator ref and actual returned tag.
	collector_source_P + sizeof(std::random_access_iterator_tag) +
	// __pred_iter(arg/result), _Iter_pred(this,pred), both real std::move calls.
	2 * collector_source_P + 2 * collector_source_P + 2 * (2 * collector_source_P) +
	// __negate(arg/result), _Iter_negate(this,pred), both real std::move calls.
	2 * collector_source_P + 2 * collector_source_P + 2 * (2 * collector_source_P) +
	// Negated adapter operator(this,it,bool), hexadecimal(char,bool).
	2 * collector_source_P + collector_source_B + sizeof(char) + collector_source_B +
	// __negate copies its named _Iter_pred argument; actual adapter destructors
	// are the __find_if_not parameter, __negate parameter and RA negate parameter.
	2 * collector_source_P + 3 * collector_source_P;
constexpr size_t collector_hex_lambda_lane_source =
	// any_of; none_of; find_if, then its genuine THREE-arg __find_if wrapper.
	2 * (2 * collector_source_P + collector_hex_L + collector_source_B) +
	3 * collector_source_P + collector_hex_L + 3 * collector_source_P + collector_hex_I +
	// FOUR-arg RA helper: first/last/result, actual adapter I, trip_count and tag.
	3 * collector_source_P + collector_hex_I + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + collector_source_P +
	sizeof(std::random_access_iterator_tag) +
	// Adapter factory(L,I), ctor(this,L), two moves; adapter op and lambda call.
	collector_hex_L + collector_hex_I + collector_source_P + collector_hex_L +
	2 * (2 * collector_source_P) + 2 * collector_source_P + collector_source_B +
	collector_source_P + sizeof(char) + collector_source_B +
	// Three named-lvalue lambda copies(any->none->find->factory), two lambda
	// moves into ctor/member, final adapter-copy member lambda copy: six pairs.
	6 * (2 * collector_source_P) +
	// THREE-arg wrapper copies its named adapter into the FOUR-arg helper.
	2 * collector_source_P +
	// Five scalar lambda parameters plus both adapter lambda members destroy;
	// both real adapters also have their own implicit destructor receivers.
	7 * collector_source_P + 2 * collector_source_P;
constexpr size_t collector_hex_tag_lifetime_source =
	// Actual random_access -> bidirectional -> forward -> input ctor/dtor.
	// All-of and any-of dispatches are sequential, never nested: reuse this
	// exact identical four-level tag-lifetime lane maximum, not spare margin.
	4 * collector_source_P + 4 * collector_source_P;
constexpr size_t collector_hex_predicate_source = collector_hex_function_pointer_lane_source +
						  collector_hex_lambda_lane_source +
						  collector_hex_tag_lifetime_source;
constexpr size_t collector_record_policy_source =
	// valid_record(entry,collect_at,sale_at,available,availability,before),
	// valid_rules, both valid_death_operation overloads, string_view pointer/count
	// ctor/size/begin/end/dtor, actual valid_reason/cancellation_reason and add.
	collector_source_P + 2 * sizeof(uint64_t) + 4 * collector_source_B + collector_source_P +
	collector_source_B + collector_source_P + collector_source_B + sizeof(std::string_view) +
	collector_source_B + 2 * collector_source_P + collector_source_N + collector_source_P +
	collector_source_N + 2 * (2 * collector_source_P) + collector_source_P +
	2 * (sizeof(collector::reason) + collector_source_B) + 2 * sizeof(uint64_t) +
	collector_source_P + collector_source_B + collector_hex_predicate_source +
	collector_record_array_source;
constexpr size_t collector_record_codec_source =
	// encode(entry,out,codec result,candidate array,offset); original local
	// candidate array is distinct from listing_facts's array/caller output.
	2 * collector_source_P + sizeof(collector::codec_result) +
	sizeof(std::array<uint8_t, collector::encoded_record_bytes>) + collector_source_N +
	// put(output,offset,widest value,bits,byte,bool); operation loop/high/low,
	// hexadecimal(input/result); complete array assignment receiver/source/ref.
	2 * collector_source_P + 2 * sizeof(uint64_t) + collector_source_N + collector_source_B +
	3 * collector_source_P + collector_source_N + 2 * sizeof(int) + collector_source_B +
	sizeof(unsigned char) + sizeof(int) + 3 * collector_source_P +
	// decode(encoded,size,out,reader{cursor,end},candidate,4 flag bytes,status);
	// reader get(this,value,bits,byte,bool), get_operation(input,operation,hex17).
	2 * collector_source_P + collector_source_N + 2 * collector_source_P +
	sizeof(collector::record) + 4 * sizeof(uint8_t) + sizeof(collector::codec_result) +
	2 * collector_source_P + sizeof(uint64_t) + collector_source_N + collector_source_B +
	2 * collector_source_P + 17 * sizeof(char) + collector_source_N + sizeof(uint8_t) +
	collector_source_B +
	// candidate default/rules default, record/rules/reader cleanup and assignment.
	2 * collector_source_P + 3 * collector_source_P + 3 * collector_source_P +
	collector_record_policy_source + collector_record_array_source;
}

namespace
{
struct collector_fixed_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *projection = nullptr;
	const economic_frozen_intent *intent = nullptr;
	const economic_admission_facts *facts = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	mutable bool denied = false;
	bool prefix(size_t &bytes, size_t extra = 0) const noexcept
	{
		bytes = outer;
		size_t heap = 0;
		if (!collector_account_add(bytes, sizeof(*this)) ||
		    !collector_account_add(bytes, frames) ||
		    (projection && (!critical_command_current_heap_bytes(*projection, &heap) ||
				    !collector_account_add(bytes, heap))) ||
		    (intent && !collector_account_add(bytes, intent->admission.facts.capacity())) ||
		    (facts && !collector_account_add(bytes, facts->facts.capacity())) ||
		    (expected && !collector_account_add(bytes, expected->capacity())) ||
		    !collector_account_add(bytes, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		if (!prefix(bytes, extra) || !reserve || !reserve(bytes, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool forward(size_t bytes, void *opaque) noexcept
	{
		auto &budget = *static_cast<collector_fixed_budget *>(opaque);
		if (!budget.reserve || !budget.reserve(bytes, budget.context))
		{
			budget.denied = true;
			return false;
		}
		return true;
	}
};
}

namespace
{
// Genuine child query costs are constant expressions before the first callback.
constexpr size_t collector_fixed_decode_query = economic_intent_decode_source_query_frame_bytes();
constexpr size_t collector_fixed_binding_query =
	economic_intent_verify_binding_fixed_source_query_frame_bytes();
constexpr size_t collector_fixed_freeze_query =
	economic_intent_freeze_fixed_source_query_frame_bytes();
struct collector_fixed_preflight
{
	size_t source = 0, initial = 0, supplement = 0, request = 0;
};
// Actual helper formals, returned result, query accessor result and checked-add
// carriers plus actual working-object constructor/destructor receivers;
// not an emitted-stack claim or substitute for the full parent graph.
constexpr size_t collector_fixed_preflight_source =
	7 * sizeof(void *) + 3 * sizeof(size_t) + 4 * sizeof(bool);

bool collector_fixed_preflight_peak(collector_fixed_budget &budget,
				    const collector_fixed_preflight &work) noexcept
{
	size_t request = work.request;
	if (!collector_account_add(request, sizeof(work)) ||
	    !collector_account_add(request, collector_fixed_preflight_source))
	{
		budget.denied = true;
		return false;
	}
	return budget.peak(request);
}
bool collector_fixed_payload_preflight(collector_fixed_budget &budget, size_t outer,
				       size_t *child_outer) noexcept
{
	constexpr size_t query =
		collector_command_decode_payload_source_query_frame_bytes() + sizeof(size_t);
	if (!budget.peak(sizeof(collector_fixed_preflight) + collector_fixed_preflight_source +
			 query))
		return false;
	collector_fixed_preflight work;
	if (!collector_command_decode_payload_source_frame_bytes(&work.source) ||
	    !collector_command_decode_payload_initial_inline_bytes(&work.initial) ||
	    !collector_command_decode_payload_source_supplement_frame_bytes(&work.supplement) ||
	    !collector_account_add(work.request, work.source) ||
	    !collector_account_add(work.request, work.initial) ||
	    !collector_fixed_preflight_peak(budget, work) ||
	    !collector_account_add(outer, work.supplement))
	{
		budget.denied = true;
		return false;
	}
	*child_outer = outer;
	return true;
}
bool collector_fixed_decode_preflight(collector_fixed_budget &budget, size_t outer,
				      size_t *child_outer) noexcept
{
	constexpr size_t query = collector_fixed_decode_query + sizeof(size_t);
	if (!budget.peak(sizeof(collector_fixed_preflight) + collector_fixed_preflight_source +
			 query))
		return false;
	collector_fixed_preflight work;
	if (!economic_intent_decode_source_frame_bytes(&work.source) ||
	    !economic_intent_decode_initial_inline_bytes(&work.initial) ||
	    !economic_intent_decode_source_supplement_frame_bytes(&work.supplement) ||
	    !collector_account_add(work.request, work.source) ||
	    !collector_account_add(work.request, work.initial) ||
	    !collector_fixed_preflight_peak(budget, work) ||
	    !collector_account_add(outer, work.supplement))
	{
		budget.denied = true;
		return false;
	}
	// Old decoder owns DTO/span/request; only its uncovered SOURCE stays live.
	*child_outer = outer;
	return true;
}
bool collector_fixed_binding_preflight(collector_fixed_budget &budget) noexcept
{
	constexpr size_t query = collector_fixed_binding_query + sizeof(size_t);
	if (!budget.peak(sizeof(collector_fixed_preflight) + collector_fixed_preflight_source +
			 query))
		return false;
	collector_fixed_preflight work;
	if (!economic_intent_verify_binding_fixed_source_frame_bytes(&work.source) ||
	    !economic_intent_verify_binding_fixed_initial_inline_bytes(&work.initial) ||
	    !collector_account_add(work.request, work.source) ||
	    !collector_account_add(work.request, work.initial) ||
	    !collector_fixed_preflight_peak(budget, work))
	{
		budget.denied = true;
		return false;
	}
	// New fixed proof owns SOURCE/inline; prospective terms die before entry.
	return true;
}
bool collector_fixed_freeze_preflight(collector_fixed_budget &budget, size_t outer,
				      size_t *child_outer) noexcept
{
	constexpr size_t query = collector_fixed_freeze_query + sizeof(size_t);
	if (!budget.peak(sizeof(collector_fixed_preflight) + collector_fixed_preflight_source +
			 query))
		return false;
	collector_fixed_preflight work;
	if (!economic_intent_freeze_fixed_source_frame_bytes(&work.source) ||
	    !economic_intent_freeze_fixed_initial_inline_bytes(&work.initial) ||
	    !economic_intent_freeze_fixed_source_supplement_frame_bytes(&work.supplement) ||
	    !collector_account_add(work.request, work.source) ||
	    !collector_account_add(work.request, work.initial) ||
	    !collector_fixed_preflight_peak(budget, work) ||
	    !collector_account_add(outer, work.supplement))
	{
		budget.denied = true;
		return false;
	}
	// Old fixed freeze owns controllers/DTO; only missing named SOURCE stays.
	*child_outer = outer;
	return true;
}
bool collector_fixed_rebuild_preflight(collector_fixed_budget &budget) noexcept
{
	constexpr size_t query =
		collector_purchase_accounting_fixed_source_query_frame_bytes() + sizeof(size_t);
	if (!budget.peak(sizeof(collector_fixed_preflight) + collector_fixed_preflight_source +
			 query))
		return false;
	collector_fixed_preflight work;
	if (!collector_purchase_accounting_intent_fixed_source_frame_bytes(&work.source) ||
	    !collector_purchase_accounting_intent_fixed_initial_inline_bytes(&work.initial) ||
	    !collector_account_add(work.request, work.source) ||
	    !collector_account_add(work.request, work.initial) ||
	    !collector_fixed_preflight_peak(budget, work))
	{
		budget.denied = true;
		return false;
	}
	return true;
}

}

namespace
{
constexpr size_t collector_identity_source =
	// account_pair(wallet,bank,racewar,result) -> key_valid(key,result),
	// kind_valid(kind,result) -> operation_is_zero(ref,result,value,range/iterators).
	2 * collector_source_P + sizeof(uint8_t) + collector_source_B + collector_source_P +
	collector_source_B + sizeof(economic_account_kind) + collector_source_B +
	collector_source_P + collector_source_B + sizeof(uint8_t) + 3 * collector_source_P +
	collector_record_array_source +
	// death_id(listing,out,result) -> operation_from_hex(input,out,index,high,low),
	// actual strlen input/result and hex_value(value,result), trailing zero proof.
	2 * collector_source_P + collector_source_B + 2 * collector_source_P + collector_source_N +
	2 * sizeof(int) + collector_source_B + collector_source_P + collector_source_N +
	sizeof(char) + sizeof(int) +
	// Operation/lineage array equality uses genuine byte equality controllers.
	2 * collector_source_P + collector_source_B + collector_fixed_array_equal_source +
	// Original command_matches_listing(payload,listing,bool), array index helper.
	2 * collector_source_P + collector_source_B + 2 * collector_source_P + collector_source_N;
constexpr size_t collector_span_source =
	// Explicit pointer/count span ctor -> to_address -> __to_address and extent
	// ctor; physical span value stays prospective inline, not source duplication.
	(2 * collector_source_P + collector_source_N) + 2 * (2 * collector_source_P) +
	collector_source_P + collector_source_N +
	// read_u64(span,offset): by-value span owns cursor value separately; actual
	// value/index. operator[](this,index,ref), size->extent read and returned N.
	collector_source_N + sizeof(uint64_t) + collector_source_N + 2 * collector_source_P +
	collector_source_N + 2 * (collector_source_P + collector_source_N) +
	// Generated copied span/extent this/source and both cleanup this carriers.
	4 * collector_source_P + 2 * collector_source_P;
constexpr size_t collector_intent_value_source =
	// Actual frozen/admission/metadata default-member construction and cleanup.
	// facts vector default + cleanup. Trivial optional default: optional/base/
	// base_impl/payload/payload_base/storage/empty/enable plus generated cleanup.
	6 * collector_source_P + collector_vector_default_source<uint8_t>() +
	collector_vector_cleanup_source<uint8_t>() + 16 * collector_source_P +
	// Frozen/admission/metadata move assignment(this,source,returned-ref);
	// trivial optional/member/base assignment receivers are real source calls.
	3 * (3 * collector_source_P) + 8 * (3 * collector_source_P) + 2 * collector_source_P +
	collector_vector_move_assignment_source<uint8_t>();
constexpr size_t collector_facts_value_source =
	// Actual facts/metadata ctor+dtor, optional default/cleanup and byte vector.
	4 * collector_source_P + 16 * collector_source_P +
	collector_vector_default_source<uint8_t>() + collector_vector_cleanup_source<uint8_t>() +
	// Three actual operation-ID assignment receivers/source/returned-reference.
	3 * (3 * collector_source_P);
constexpr size_t collector_listing_facts_source =
	// listing_facts(listing,wallet,bank,out,status): four refs/result plus its
	// original fixed record array. Array object belongs builder inline below.
	4 * collector_source_P + sizeof(error) +
	// append_u64(bytes,value) owns its index and actual rvalue byte passed to push.
	collector_source_P + sizeof(uint64_t) + collector_source_N + sizeof(uint8_t) +
	collector_byte_reserve_source + collector_byte_reserved_push_source +
	collector_byte_end_insert_source + collector_record_codec_source +
	collector_record_array_source;
constexpr size_t collector_budget_source =
	// New prefix(this,out,extra,result) total aliases output; heap local,
	// critical CURRENT getter local size. peak(this,extra,total,result),
	// forward(bytes,opaque,actual budget ref,result), generated budget lifetime.
	2 * collector_source_P + collector_source_N + collector_source_N + collector_source_B +
	collector_source_P + 2 * collector_source_N + collector_source_B + 3 * collector_source_P +
	collector_source_N + collector_source_B + 2 * collector_source_P +
	// Checked addition(ref,extra,bool), vector facts/expected capacity receivers;
	// each actual selected inline-size/capacity declaration chain is nonallocating.
	collector_source_P + collector_source_N + collector_source_B +
	3 * (collector_source_P + collector_source_N);
constexpr size_t collector_fixed_intent_own_source =
	// Entry eight reference/callback/context parameters, outer,N frames,N nested,
	// N facts_request,N child_outer; actual status and catch-reference carrier.
	8 * collector_source_P + 5 * collector_source_N + sizeof(error) + collector_source_P +
	collector_budget_source + collector_identity_source + collector_facts_value_source +
	collector_listing_facts_source +
	// Three genuine helper preflight closures: payload, fixed-freeze and peak;
	// helper working storage is separately prospective inline during that call.
	3 * collector_fixed_preflight_source;
constexpr size_t collector_fixed_decode_own_source =
	// Entry eight pointer/reference/callback/context formals and outer; frames,
	// nested,request; facts/meta refs; decoded/binding/rebuild statuses and catch.
	8 * collector_source_P + 4 * collector_source_N + 2 * collector_source_P +
	3 * sizeof(error) + collector_source_P + collector_budget_source +
	collector_record_codec_source + collector_span_source + collector_intent_value_source +
	collector_vector_default_source<uint8_t>() + collector_vector_cleanup_source<uint8_t>() +
	collector_byte_vector_equal_source +
	// Original projected.accounting_intent.clear(): clear(this) then
	// _M_erase_at_end(this,pos) and genuine __n local =3P+N. Its actual
	// _M_get_Tp_allocator/_Destroy descendants reuse the complete selected
	// byte cleanup closure above; no new vector heap or copy graph is added.
	3 * collector_source_P + collector_source_N +
	// Five output assignments: moved intent above, payload/record/two keys each
	// this/source/returned-reference; key/listing generated cleanup receivers.
	4 * (3 * collector_source_P) + 5 * collector_source_P +
	// Parsed record/rules default constructors, independently of record_decode's
	// distinct private candidate. Payload assignment reaches three real array
	// assignments, entry and two owner assignments; keys reach copied operation
	// IDs/byte arrays. These are source receivers, not repeated DTO heap.
	2 * collector_source_P + 3 * (3 * collector_source_P) + 3 * collector_source_P +
	2 * (3 * collector_source_P) + 2 * (3 * collector_source_P + 3 * collector_source_P) +
	// Parsed key aggregate initialization copies a critical ID and its real
	// bytes array; actual const-object cleanup does not mutate any authority.
	2 * (2 * collector_source_P + 2 * collector_source_P) +
	// Four genuine preflight helper closures plus peak controller.
	5 * collector_fixed_preflight_source;
constexpr size_t collector_fixed_intent_inline =
	sizeof(collector_command_payload) + sizeof(critical_operation_id) +
	sizeof(economic_admission_facts) +
	sizeof(std::array<uint8_t, collector::encoded_record_bytes>);
constexpr size_t collector_fixed_decode_inline =
	sizeof(collector_command_payload) + sizeof(economic_frozen_intent) +
	sizeof(collector::record) + 2 * sizeof(economic_account_key) + sizeof(critical_command) +
	sizeof(std::vector<uint8_t>) + sizeof(std::span<const uint8_t>);
}

economic_accounting_error collector_purchase_accounting_intent_fixed_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const collector::record &listing, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!collector_fixed_source_policy())
		return error::capacity;
	const size_t frames = collector_fixed_intent_inline + collector_fixed_intent_own_source;
	collector_fixed_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!budget.peak())
		return error::capacity;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	collector_command_payload payload = {};
	if (!budget.prefix(nested) || !collector_fixed_payload_preflight(budget, nested, &nested) ||
	    !collector_command_decode_payload_bounded(command, &payload,
						      collector_fixed_budget::forward, &budget,
						      nested, &budget.denied))
		return budget.denied ? error::capacity : error::corrupt_evidence;
	critical_operation_id death = {};
	if (!account_pair(wallet, bank, payload.racewar) || !death_id(listing, &death) ||
	    critical_operation_id_equal(death, command.operation_id) ||
	    !command_matches_listing(payload, listing))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		budget.facts = &facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = death;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_COLLECTOR_PURCHASE;
		facts.metadata.reason = economic_reason::collector_purchase;
		// Genuine fresh reserve is the original 16 mapping bytes plus encoded record.
		const size_t facts_request = 16 + collector::encoded_record_bytes;
		if (!budget.peak(facts_request))
			return error::capacity;
		auto status = listing_facts(listing, wallet, bank, &facts.facts);
		if (status != error::ok)
			return status;
		if (!budget.prefix(nested))
			return error::capacity;
		size_t child_outer = 0;
		if (!collector_fixed_freeze_preflight(budget, nested, &child_outer))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(command, facts, encoded,
							    collector_fixed_budget::forward,
							    &budget, child_outer);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error collector_purchase_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	collector_command_payload *payload, collector::record *listing,
	economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	if (!collector_fixed_source_policy())
		return error::capacity;
	const size_t frames = collector_fixed_decode_inline + collector_fixed_decode_own_source +
			      critical_command_copy_frame_bytes() +
			      critical_command_valid_frame_bytes();
	collector_fixed_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!budget.peak())
		return error::capacity;
	if (!intent || !payload || !listing || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		collector_command_payload parsed_payload = {};
		if (!budget.prefix(nested) ||
		    !collector_fixed_payload_preflight(budget, nested, &nested) ||
		    !collector_command_decode_payload_bounded(command, &parsed_payload,
							      collector_fixed_budget::forward,
							      &budget, nested, &budget.denied) ||
		    parsed_payload.action != collector_action::purchase)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		const auto decoded_status =
			!budget.prefix(nested) ?
				error::capacity :
			!collector_fixed_decode_preflight(budget, nested, &nested) ?
				error::capacity :
				economic_intent_decode_bounded(
					std::span<const uint8_t>(command.accounting_intent.data(),
								 command.accounting_intent.size()),
					&parsed_intent, collector_fixed_budget::forward, &budget,
					nested);
		if (decoded_status != error::ok)
			return budget.denied || decoded_status == error::capacity ?
				       error::capacity :
				       error::corrupt_evidence;
		const auto binding_status = !budget.prefix(nested) ?
						    error::capacity :
					    !collector_fixed_binding_preflight(budget) ?
						    error::capacity :
						    economic_intent_verify_binding_fixed_bounded(
							    command, parsed_intent,
							    collector_fixed_budget::forward,
							    &budget, nested);
		if (binding_status != error::ok)
			return budget.denied || binding_status == error::capacity ?
				       error::capacity :
				       error::corrupt_evidence;
		const auto &facts = parsed_intent.admission.facts;
		if (facts.size() != 16 + collector::encoded_record_bytes)
			return error::invalid_identity;
		collector::record parsed_listing;
		if (collector::record_decode(facts.data() + 16, collector::encoded_record_bytes,
					     &parsed_listing) != collector::codec_result::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto &meta = parsed_intent.admission.metadata;
		const economic_account_key parsed_wallet = {
			meta.lineage, economic_account_kind::wallet,
			read_u64(std::span<const uint8_t>(facts.data(), facts.size()), 0), 0
		};
		const economic_account_key parsed_bank = {
			meta.lineage, economic_account_kind::bank,
			read_u64(std::span<const uint8_t>(facts.data(), facts.size()), 8),
			parsed_payload.racewar
		};
		size_t request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &request) ||
		    !budget.peak(request))
			return error::capacity;
		critical_command projected = command;
		budget.projection = &projected;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		const auto status = !budget.prefix(nested) ?
					    error::capacity :
				    !collector_fixed_rebuild_preflight(budget) ?
					    error::capacity :
					    collector_purchase_accounting_intent_fixed_bounded(
						    projected, meta.epoch, parsed_wallet,
						    parsed_bank, parsed_listing, &expected,
						    collector_fixed_budget::forward, &budget,
						    nested);
		if (status != error::ok || expected != command.accounting_intent)
			return budget.denied || status == error::capacity ? error::capacity :
									    error::unauthorized;
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

namespace
{
bool collector_fixed_source_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 && \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&   \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                    \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                 \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&              \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(unsigned long) == 8 && sizeof(std::allocator<uint8_t>) == 1;
#else
	return false;
#endif
}
}
bool collector_purchase_accounting_intent_fixed_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !collector_fixed_source_policy())
		return false;
	size_t source = 0, initial = 0, total = collector_fixed_intent_own_source;
	if (!collector_command_decode_payload_source_frame_bytes(&source) ||
	    !collector_command_decode_payload_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial) ||
	    !economic_intent_freeze_fixed_source_frame_bytes(&source) ||
	    !economic_intent_freeze_fixed_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial))
		return false;
	// Child query work is actual live inline inside each helper, not a second
	// external output. Getter preentry is transient; child runtime owns itself.
	if (!collector_account_add(total, sizeof(collector_fixed_preflight)) ||
	    !collector_account_add(
		    total, collector_fixed_freeze_query +
				   collector_command_decode_payload_source_query_frame_bytes() +
				   2 * sizeof(size_t)))
		return false;
	*output = total;
	return true;
}
bool collector_purchase_accounting_intent_fixed_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !collector_fixed_source_policy())
		return false;
	*output = sizeof(collector_fixed_budget);
	return true;
}
bool collector_purchase_accounting_decode_fixed_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !collector_fixed_source_policy())
		return false;
	size_t source = 0, initial = 0, total = collector_fixed_decode_own_source;
	if (!collector_account_add(total, critical_command_copy_frame_bytes()) ||
	    !collector_account_add(total, critical_command_valid_frame_bytes()) ||
	    !collector_command_decode_payload_source_frame_bytes(&source) ||
	    !collector_command_decode_payload_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial) ||
	    !economic_intent_decode_source_frame_bytes(&source) ||
	    !economic_intent_decode_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial) ||
	    !economic_intent_verify_binding_fixed_source_frame_bytes(&source) ||
	    !economic_intent_verify_binding_fixed_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial) ||
	    !collector_purchase_accounting_intent_fixed_source_frame_bytes(&source) ||
	    !collector_purchase_accounting_intent_fixed_initial_inline_bytes(&initial) ||
	    !collector_account_add(total, source) || !collector_account_add(total, initial) ||
	    !collector_account_add(total, sizeof(collector_fixed_preflight)) ||
	    !collector_account_add(
		    total, collector_fixed_decode_query + collector_fixed_binding_query +
				   collector_command_decode_payload_source_query_frame_bytes() +
				   3 * sizeof(size_t)))
		return false;
	*output = total;
	return true;
}
bool collector_purchase_accounting_decode_fixed_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !collector_fixed_source_policy())
		return false;
	*output = sizeof(collector_fixed_budget);
	return true;
}
