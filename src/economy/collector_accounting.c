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
