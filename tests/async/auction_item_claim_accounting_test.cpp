#include "economy/auction_item_claim_accounting.h"

#include <cassert>
#include <cstring>

namespace
{
critical_operation_id id(uint8_t first)
{
	critical_operation_id value = {};
	value.bytes[0] = first;
	return value;
}

auction_command_payload payload()
{
	auction_command_payload command = {};
	command.action = auction_action::claim_item;
	command.actor_pid = 20;
	command.auction_id = 400;
	command.racewar = 1;
	std::memcpy(command.account_name.data(), "auction_claim", 14);
	command.expected_wallet_revision = 4;
	command.expected_bank_revision = 5;
	command.item_count = 2;
	command.items[0] = { 600, 1, 77 };
	command.items[1] = { 601, 2, 78 };
	return command;
}

auction_item_claim_state claim()
{
	auction_item_claim_state state;
	state.auction_id = 400;
	state.seller_pid = 10;
	state.winner_pid = 20;
	state.claimant_pid = 20;
	state.status = 2;
	state.custody_state = 1;
	state.auction_revision = 3;
	state.listing_operation = id(1);
	state.claim_source_operation = id(3);
	state.item_count = 2;
	state.rows[0] = { 600, 1, 0, 77, 20, false };
	state.rows[1] = { 601, 2, 1, 78, 20, false };
	return state;
}

auction_command_result result()
{
	auction_command_result value = {};
	value.action = auction_action::claim_item;
	value.event_type = auction_event_type::item_claimed;
	value.auction_id = 400;
	value.status = 2;
	value.seller_pid = 10;
	value.winner_pid = 20;
	value.wallet.amount = { 0, 0, 0, 5 };
	value.wallet_revision = 4;
	value.bank_revision = 5;
	value.auction_revision = 4;
	value.player_owner_revision = 6;
	value.auction_owner_revision = 7;
	value.item_count = 2;
	value.item_uids[0] = 600;
	value.item_uids[1] = 601;
	value.item_revisions[0] = 2;
	value.item_revisions[1] = 3;
	return value;
}

auction_item_claim_accounting_authority authority(const auction_item_claim_state &state)
{
	auction_item_claim_accounting_authority value;
	value.epoch = id(2);
	value.wallet_account = { id(9), economic_account_kind::wallet, 100, 0 };
	value.bank_account = { id(9), economic_account_kind::bank, 101, 1 };
	value.claim = state;
	value.balances_before.wallet.amount = { 0, 0, 0, 5 };
	value.balances_before.wallet_revision = 4;
	value.balances_before.bank_revision = 5;
	value.player_owner_revision_before = 5;
	value.auction_owner_revision_before = 6;
	value.items_before = {
		{ 600,
		  { { item_owner_type::auction, 400, 0 }, 600, 0, 1, item_custody_state::active } },
		{ 601,
		  { { item_owner_type::auction, 400, 0 }, 601, 0, 2, item_custody_state::active } }
	};
	return value;
}
} // namespace

int main()
{
	const auto selected = payload();
	const auto staged = claim();
	auto before = authority(staged);
	critical_command command = {};
	assert(auction_command_build(&command, id(4), selected, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	std::vector<uint8_t> encoded;
	assert(auction_item_claim_accounting_intent(command, before.epoch, before.wallet_account,
						    before.bank_account, staged,
						    &encoded) == economic_accounting_error::ok);
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = encoded;
	economic_frozen_intent intent;
	assert(economic_intent_decode(encoded, &intent) == economic_accounting_error::ok);
	economic_frozen_intent decoded_intent;
	auction_command_payload decoded_payload = {};
	auction_item_claim_state decoded_claim;
	economic_account_key decoded_wallet, decoded_bank;
	assert(auction_item_claim_accounting_decode(
		       command, &decoded_intent, &decoded_payload, &decoded_claim, &decoded_wallet,
		       &decoded_bank) == economic_accounting_error::ok);
	assert(decoded_claim.claim_source_operation.bytes == staged.claim_source_operation.bytes &&
	       decoded_claim.rows[1].uid == 601 && decoded_payload.items[1].item_uid == 601 &&
	       economic_account_key_equal(decoded_wallet, before.wallet_account) &&
	       economic_account_key_equal(decoded_bank, before.bank_account));
	auto altered = command;
	altered.accounting_intent[0] ^= 1;
	decoded_wallet.authority_id = 999;
	assert(auction_item_claim_accounting_decode(
		       altered, &decoded_intent, &decoded_payload, &decoded_claim, &decoded_wallet,
		       &decoded_bank) != economic_accounting_error::ok &&
	       decoded_wallet.authority_id == 999);
	economic_accounting_plan plan;
	const auto applied = result();
	assert(auction_item_claim_accounting_plan(command, intent, before, applied, &plan) ==
	       economic_accounting_error::ok);
	assert(plan.accounts.empty() && plan.postings.empty());
	assert(plan.item_events.size() == 2 && plan.items_before.size() == 2 &&
	       plan.items_after.size() == 2);
	assert(plan.metadata.original_operation_id.bytes == staged.listing_operation.bytes);
	assert(plan.metadata.source_event &&
	       plan.metadata.source_event->source.bytes == staged.claim_source_operation.bytes);
	assert(plan.item_events[0].uid == 600 && plan.item_events[1].uid == 601);
	assert(plan.item_events[0].before.owner.type == item_owner_type::auction &&
	       plan.item_events[0].after.owner.type == item_owner_type::player);
	assert(economic_plan_validate_structure(plan) == economic_accounting_error::ok);
	const auto digest = plan.metadata.intent_digest;
	auto damaged_result = applied;
	damaged_result.item_revisions[0]++;
	assert(auction_item_claim_accounting_plan(command, intent, before, damaged_result, &plan) ==
	       economic_accounting_error::corrupt_evidence);
	assert(plan.metadata.intent_digest == digest);
	auto stale = before;
	stale.claim.rows[0].claim_pid = 10;
	assert(auction_item_claim_accounting_plan(command, intent, stale, applied, &plan) ==
	       economic_accounting_error::invalid_identity);
	stale = before;
	stale.claim.claim_source_operation = id(5);
	assert(auction_item_claim_accounting_plan(command, intent, stale, applied, &plan) ==
	       economic_accounting_error::unauthorized);
	stale = before;
	stale.items_before[1].position.owner.id = 10;
	assert(auction_item_claim_accounting_plan(command, intent, stale, applied, &plan) ==
	       economic_accounting_error::stale_revision);
	auto duplicate = staged;
	duplicate.rows[1].slot = duplicate.rows[0].slot;
	critical_command projected = command;
	projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	projected.accounting_intent.clear();
	assert(auction_item_claim_accounting_intent(projected, before.epoch, before.wallet_account,
						    before.bank_account, duplicate, &encoded) ==
	       economic_accounting_error::invalid_identity);
	auto removed_to_winner = staged;
	removed_to_winner.status = 3;
	assert(auction_item_claim_accounting_intent(
		       projected, before.epoch, before.wallet_account, before.bank_account,
		       removed_to_winner, &encoded) == economic_accounting_error::invalid_identity);
	return 0;
}
