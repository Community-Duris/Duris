#include "economy/collector_accounting.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id result = {};
	result.bytes[0] = first;
	return result;
}

collector::record available_listing()
{
	collector::rules rules;
	rules.enabled = true;
	rules.collection_delay = 10;
	rules.sale_delay = 20;
	rules.holding_duration = 30;
	rules.minimum_value = 30;
	collector::record entry;
	assert(collector::enroll(42, "0123456789abcdef0123456789abcdef", 55, 900, 1, 1000, rules,
				 &entry) == collector::outcome::applied);
	assert(collector::collect(&entry, entry.revision, entry.item_revision, 1, true, 0, 1010) ==
	       collector::outcome::applied);
	assert(collector::activate(&entry, entry.revision, 1020) == collector::outcome::applied);
	assert(entry.price_value == 30 && collector::valid_record(entry));
	return entry;
}

collector_command_payload purchase(const collector::record &entry)
{
	collector_command_payload payload;
	payload.action = collector_action::purchase;
	payload.target_state = item_custody_state::active;
	payload.capacity_admitted = true;
	payload.listing = entry.listing;
	payload.expected_listing_revision = entry.revision;
	payload.observed_at = 1021;
	payload.actor_pid = 55;
	payload.racewar = 1;
	std::memcpy(payload.account_name.data(), "CollectorTest", 14);
	payload.expected_wallet_revision = 4;
	payload.expected_bank_revision = 5;
	payload.from_owner = { item_owner_type::collector, item_collector_owner_id(entry.listing),
			       0 };
	payload.to_owner = { item_owner_type::player, payload.actor_pid, 0 };
	payload.expected_from_owner_revision = 8;
	payload.expected_to_owner_revision = 9;
	payload.selected_item_uid = entry.uid;
	payload.item_count = 1;
	payload.items[0] = { entry.uid,		  entry.uid, 0,
			     entry.item_revision, 1234,	     item_custody_state::active };
	payload.item_blob_size = 1;
	payload.item_blob[0] = 7;
	return payload;
}

collector_command_result settled(const collector_command_payload &payload,
				 const collector::record &entry)
{
	collector_command_result result;
	result.action = collector_action::purchase;
	result.record_present = true;
	result.catalog_revision = 8;
	result.from_owner_revision = 9;
	result.to_owner_revision = 10;
	result.wallet.amount = { 0, 7, 0, 0 };
	result.bank.amount = { 3, 0, 0, 0 };
	result.wallet_revision = 5;
	result.bank_revision = 6;
	result.materialized_item_id = 123;
	result.entry = entry;
	assert(collector::purchase(&result.entry, payload.expected_listing_revision,
				   payload.actor_pid, 100, true,
				   payload.observed_at) == collector::outcome::applied);
	return result;
}
} // namespace

int main()
{
	const auto listing = available_listing();
	const auto payload = purchase(listing);
	critical_command command = {};
	assert(collector_command_build(&command, id(3), payload, critical_source_site::command,
				       critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	const auto lineage = id(1), epoch = id(2);
	const economic_account_key wallet = { lineage, economic_account_kind::wallet, 100, 0 };
	const economic_account_key bank = { lineage, economic_account_kind::bank, 101, 1 };
	std::vector<uint8_t> encoded;
	assert(collector_purchase_accounting_intent(command, epoch, wallet, bank, listing,
						    &encoded) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = encoded;
	economic_frozen_intent intent;
	assert(economic_intent_decode(encoded, &intent) == economic_accounting_error::ok);
	economic_frozen_intent checked_intent;
	collector_command_payload checked_payload = {};
	collector::record checked_listing;
	economic_account_key checked_wallet, checked_bank;
	assert(collector_purchase_accounting_decode(
		       command, &checked_intent, &checked_payload, &checked_listing,
		       &checked_wallet, &checked_bank) == economic_accounting_error::ok);
	assert(checked_intent.admission.facts == intent.admission.facts &&
	       checked_payload.listing == payload.listing && checked_listing.uid == listing.uid &&
	       economic_account_key_equal(checked_wallet, wallet) &&
	       economic_account_key_equal(checked_bank, bank));
	auto damaged_command = command;
	damaged_command.accounting_intent[0] ^= 1;
	checked_wallet.authority_id = 999;
	assert(collector_purchase_accounting_decode(
		       damaged_command, &checked_intent, &checked_payload, &checked_listing,
		       &checked_wallet, &checked_bank) != economic_accounting_error::ok &&
	       checked_wallet.authority_id == 999);
	critical_operation_id death = {};
	assert(critical_operation_id_from_hex(listing.death_operation.data(), &death));
	assert(intent.admission.metadata.original_operation_id.bytes == death.bytes);
	collector_purchase_accounting_authority authority;
	authority.epoch = epoch;
	authority.wallet_account = wallet;
	authority.bank_account = bank;
	authority.balances_before.wallet.amount = { 0, 0, 1, 0 };
	authority.balances_before.bank.amount = { 3, 0, 0, 0 };
	authority.balances_before.wallet_revision = 4;
	authority.balances_before.bank_revision = 5;
	authority.listing_before = listing;
	authority.item_before = { listing.uid,
				  { payload.from_owner, listing.uid, 0, listing.item_revision,
				    item_custody_state::active } };
	authority.catalog_revision_before = 7;
	authority.from_owner_revision_before = 8;
	authority.to_owner_revision_before = 9;
	const auto result = settled(payload, listing);
	economic_accounting_plan plan;
	assert(collector_purchase_accounting_plan(command, intent, authority, result, &plan) ==
	       economic_accounting_error::ok);
	assert(plan.metadata.original_operation_id.bytes == death.bytes);
	assert(plan.metadata.reason == economic_reason::collector_purchase);
	assert(plan.accounts.size() == 3 && plan.postings.size() == 2);
	assert(plan.accounts[0].key.kind == economic_account_kind::wallet);
	assert(plan.accounts[1].key.kind == economic_account_kind::bank);
	assert(plan.accounts[2].key.kind == economic_account_kind::sink);
	assert(plan.accounts[2].key.authority_id == ECONOMIC_COLLECTOR_PURCHASE_SINK_ID);
	assert(plan.postings[0].copper == -30 && plan.postings[1].copper == 30);
	assert(plan.postings[0].delta == (economic_coin_vector{ 0, 7, -1, 0 }));
	assert(plan.item_events.size() == 1 && plan.item_events[0].uid == listing.uid);
	assert(plan.item_events[0].before.owner.type == item_owner_type::collector);
	assert(plan.item_events[0].after.owner.type == item_owner_type::player);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);
	auto flatfile_result = result;
	flatfile_result.materialized_item_id = 0;
	assert(collector_purchase_accounting_plan(command, intent, authority, flatfile_result,
						  &plan) == economic_accounting_error::ok);
	auto first_catalog = authority;
	first_catalog.catalog_revision_before = 0;
	auto first_catalog_result = result;
	first_catalog_result.catalog_revision = 1;
	assert(collector_purchase_accounting_plan(command, intent, first_catalog,
						  first_catalog_result,
						  &plan) == economic_accounting_error::ok);

	const auto original_digest = plan.metadata.intent_digest;
	auto damaged_result = result;
	damaged_result.wallet.amount[1] = 6;
	assert(collector_purchase_accounting_plan(command, intent, authority, damaged_result,
						  &plan) ==
	       economic_accounting_error::corrupt_evidence);
	assert(plan.metadata.intent_digest == original_digest);
	damaged_result = result;
	damaged_result.entry.price_value++;
	assert(collector_purchase_accounting_plan(command, intent, authority, damaged_result,
						  &plan) ==
	       economic_accounting_error::corrupt_evidence);
	auto stale = authority;
	stale.item_before.position.revision++;
	assert(collector_purchase_accounting_plan(command, intent, stale, result, &plan) ==
	       economic_accounting_error::stale_revision);
	stale = authority;
	stale.listing_before.price_value++;
	assert(collector_purchase_accounting_plan(command, intent, stale, result, &plan) ==
	       economic_accounting_error::unauthorized);
	stale = authority;
	stale.listing_before.death_operation[0] = 'f';
	assert(collector_purchase_accounting_plan(command, intent, stale, result, &plan) ==
	       economic_accounting_error::unauthorized);
	stale = authority;
	stale.balances_before.wallet.amount = { 0, 2, 0, 0 };
	assert(collector_purchase_accounting_plan(command, intent, stale, result, &plan) ==
	       economic_accounting_error::negative_holding);

	for (const bool quarantine : { false, true })
	{
		auto held_payload = payload;
		held_payload.action = quarantine ? collector_action::cancel :
						   collector_action::expire;
		held_payload.cancel_reason = quarantine ? collector::reason::quarantined :
							  collector::reason::none;
		held_payload.target_state = quarantine ? item_custody_state::quarantined :
							 item_custody_state::destroyed;
		held_payload.to_owner = { quarantine ? item_owner_type::system :
						       item_owner_type::destruction,
					  0, 0 };
		held_payload.actor_pid = 0;
		held_payload.racewar = 0;
		held_payload.account_name = {};
		held_payload.capacity_admitted = false;
		held_payload.expected_wallet_revision = 0;
		held_payload.expected_bank_revision = 0;
		held_payload.observed_at = 1050;
		critical_command held_command = {};
		assert(collector_command_build(&held_command, id(quarantine ? 5 : 4), held_payload,
					       critical_source_site::zone_event,
					       critical_deadline_class::background));
		held_command.accepted_at_usec = 1;
		std::vector<uint8_t> held_encoded;
		assert(collector_held_accounting_intent(held_command, lineage, epoch, listing,
							&held_encoded) ==
		       economic_accounting_error::ok);
		held_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
		held_command.accounting_intent = held_encoded;
		economic_frozen_intent held_intent;
		assert(economic_intent_decode(held_encoded, &held_intent) ==
		       economic_accounting_error::ok);
		collector_command_payload decoded_payload = {};
		collector::record decoded_listing;
		economic_frozen_intent decoded_intent;
		assert(collector_held_accounting_decode(held_command, &decoded_intent,
							&decoded_payload, &decoded_listing) ==
		       economic_accounting_error::ok);
		assert(decoded_payload.action == held_payload.action &&
		       decoded_listing.listing == listing.listing &&
		       decoded_intent.admission.metadata.writer_id ==
			       ECONOMIC_WRITER_COLLECTOR_HELD);
		const auto valid_action = decoded_payload.action;
		const auto valid_listing = decoded_listing.listing;
		critical_command changed = held_command;
		changed.accounting_intent.back() ^= 1;
		assert(collector_held_accounting_decode(changed, &decoded_intent, &decoded_payload,
							&decoded_listing) !=
		       economic_accounting_error::ok);
		assert(decoded_payload.action == valid_action &&
		       decoded_listing.listing == valid_listing);
		collector_held_accounting_authority held_authority;
		held_authority.lineage = lineage;
		held_authority.epoch = epoch;
		held_authority.listing_before = listing;
		held_authority.item_before = authority.item_before;
		held_authority.catalog_revision_before = 7;
		held_authority.from_owner_revision_before = 8;
		held_authority.to_owner_revision_before = 9;
		collector_command_result held_result;
		held_result.action = held_payload.action;
		held_result.record_present = true;
		held_result.catalog_revision = 8;
		held_result.from_owner_revision = 9;
		held_result.to_owner_revision = 10;
		held_result.entry = listing;
		const auto outcome = quarantine ?
					     collector::cancel(&held_result.entry, listing.revision,
							       collector::reason::quarantined) :
					     collector::expire(&held_result.entry, listing.revision,
							       held_payload.observed_at);
		assert(outcome == collector::outcome::applied);
		economic_accounting_plan held_plan;
		assert(collector_held_accounting_plan(held_command, held_intent, held_authority,
						      held_result,
						      &held_plan) == economic_accounting_error::ok);
		assert(held_plan.accounts.empty() && held_plan.postings.empty());
		assert(held_plan.item_events.size() == 1);
		assert(held_plan.item_events[0].after.state == held_payload.target_state);
		assert(held_plan.metadata.original_operation_id.bytes == death.bytes);
		assert(economic_plan_validate_structure(held_plan) ==
		       economic_accounting_error::ok);
		auto first_held_catalog = held_authority;
		first_held_catalog.catalog_revision_before = 0;
		auto first_held_result = held_result;
		first_held_result.catalog_revision = 1;
		assert(collector_held_accounting_plan(held_command, held_intent, first_held_catalog,
						      first_held_result,
						      &held_plan) == economic_accounting_error::ok);
		held_result.entry.item_revision++;
		assert(collector_held_accounting_plan(held_command, held_intent, held_authority,
						      held_result, &held_plan) ==
		       economic_accounting_error::corrupt_evidence);
	}
	return 0;
}
