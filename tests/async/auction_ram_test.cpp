#include "economy/auction_ram.h"
#include "economy/economic_state.h"
#include "economy/auction_website.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot_codec.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>

// Custody is unused by bid/refund/settlement; list/claim fail unless a test
// explicitly supplies it. The full custody adapter is covered separately.
bool item_ownership_runtime_owner_revision(const item_owner_identity &, uint64_t *revision)
{
	*revision = 0;
	return true;
}
bool item_ownership_runtime_lookup(uint64_t, item_ownership_runtime_entry *)
{
	return false;
}

bool item_ownership_runtime_patch_ready(const item_ownership_runtime_patch &p)
{
	return p.before.empty();
}
bool item_ownership_runtime_patch_publish(const item_ownership_runtime_patch &p)
{
	return p.before.empty();
}

static critical_command bid(uint32_t pid, const char *account, uint8_t id, int64_t value)
{
	auction_command_payload payload{};
	payload.action = auction_action::bid;
	payload.actor_pid = pid;
	payload.auction_id = 7;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), account);
	payload.expected_wallet_revision = UINT64_MAX;
	payload.expected_bank_revision = UINT64_MAX;
	payload.value = value;
	payload.closing_fee_basis_points = 300;
	payload.bid_extension_seconds = 300;
	critical_operation_id operation{};
	operation.bytes[0] = id;
	critical_command command{};
	assert(auction_command_build(&command, operation, payload, critical_source_site::command,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1000ULL * 1000000;
	return command;
}

static auction_command_result accept(critical_command command)
{
	const auto result = auction_ram_prepare_command(command);
	assert(result.outcome == critical_apply_outcome::applied && !result.error_code);
	auction_command_result decoded{};
	assert(auction_command_decode_result(result.result_payload.data(), result.result_size,
					     &decoded));
	command.schema_version = CRITICAL_COMMAND_OUTCOME_SCHEMA_VERSION;
	command.outcome_revision = result.durable_revision;
	command.outcome_payload.assign(result.result_payload.begin(),
				       result.result_payload.begin() + result.result_size);
	assert(auction_ram_publish(command));
	return decoded;
}

int main()
{
	// The durable listing carries each stack member's complete UID-bound state.
	{
		auction_command_payload payload{};
		payload.action = auction_action::list;
		payload.actor_pid = 1;
		strcpy(payload.account_name.data(), "seller");
		payload.end_time = 2000;
		payload.item_count = 2;
		payload.items[0] = { 91, 1, 390 };
		payload.items[1] = { 92, 2, 390 };
		const uint8_t legacy[]{ 4, 1, 0, 0, 0, 0, 32 };
		memcpy(payload.object_blob.data(), legacy, sizeof(legacy));
		payload.object_blob_size = 7;
		std::vector<player_item_snapshot> items(2);
		for (size_t i = 0; i < items.size(); ++i)
		{
			items[i].object_uid = 91 + i;
			items[i].vnum = 390;
			items[i].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			items[i].equipment_slot = -1;
			items[i].condition = 71 + i;
			items[i].timers[4] = 1234 + i;
		}
		std::vector<uint8_t> runtime;
		assert(player_item_snapshot_list_encode(items, &runtime) ==
		       player_snapshot_codec_result::ok);
		assert(auction_blob_pack(&payload, runtime));
		item_transfer_payload transfer;
		assert(auction_item_payload(payload, 7, &transfer));
		std::vector<player_item_snapshot> recovered;
		assert(player_item_snapshot_list_decode(transfer.item_blob.data(),
							transfer.item_blob_size, &recovered) ==
		       player_snapshot_codec_result::ok);
		assert(recovered.size() == 2 && recovered[1].condition == 72 &&
		       recovered[1].timers[4] == 1235);
		critical_command command;
		critical_operation_id id{};
		id.bytes[0] = 99;
		assert(auction_command_build(&command, id, payload, critical_source_site::command,
					     critical_deadline_class::interactive));
		command.accepted_at_usec = UINT64_C(1000000000);
		auction_ram_listing listing;
		auction_command_result listed;
		assert(!auction_ram_prepare(command, nullptr, {}, {}, 0, 0, &listing, &listed));
		for (auto &item : listing.items)
			item.claimant = 1;
		payload.action = auction_action::claim_item;
		payload.auction_id = listing.id;
		++payload.items[0].expected_item_revision;
		++payload.items[1].expected_item_revision;
		assert(auction_command_build(&command, id, payload, critical_source_site::command,
					     critical_deadline_class::interactive));
		command.accepted_at_usec = UINT64_C(1000000000);
		auction_ram_listing next;
		assert(!auction_ram_prepare(command, &listing, {}, {}, 1, 1, &next, &listed));
		++items[0].condition;
		assert(player_item_snapshot_list_encode(items, &runtime) ==
			       player_snapshot_codec_result::ok &&
		       auction_blob_pack(&payload, runtime));
		assert(auction_command_build(&command, id, payload, critical_source_site::command,
					     critical_deadline_class::interactive));
		command.accepted_at_usec = UINT64_C(1000000000);
		assert(auction_ram_prepare(command, &listing, {}, {}, 1, 1, &next, &listed) ==
		       ESTALE);
		payload.items[1].item_uid = 91;
		assert(!auction_item_payload(payload, listing.id, &transfer));
	}
	economic_state_reset();
	auction_ram_reset();
	currency_command_result balance{ { { 0, 0, 0, 10 } }, { { 0, 0, 0, 3 } }, 4, 8 };
	assert(economic_state_seed({ 1, "seller", "Seller", 1, balance }));
	assert(economic_state_seed({ 2, "bidder", "Bidder", 1, balance }));
	assert(economic_state_seed({ 3, "other", "Other", 1, balance }));
	assert(economic_state_seed({ 4, "BIDDER", "Alt", 1, balance }));
	// Deleted characters cannot remain available to a trusted website session.
	assert(economic_state_seed({ 5, "bidder", "Deleted", 1, balance }));
	economic_state_forget(5);
	critical_command deleted_bid{};
	assert(!auction_website_build_bid("bidder", 5, 7, 1000, "deleted", &deleted_bid));
	auction_ram_listing listing{ 7,	   1, 0,    1,	      1000,
				     5000, 1, 2000, "seller", { { 99, 2, 10, 0, false } } };
	assert(auction_ram_seed(listing));
	critical_command web{}, retry{}, conflicting{};
	assert(auction_website_build_bid("BIDDER", 2, 7, 1000, "request-1", &web));
	assert(auction_website_build_bid("bidder", 2, 7, 1000, "request-1", &retry));
	web.accepted_at_usec = retry.accepted_at_usec = 1;
	assert(critical_command_equal(web, retry));
	assert(!auction_website_build_bid("other", 2, 7, 1000, "request-1", &retry));
	assert(auction_website_build_bid("bidder", 2, 7, 2000, "request-1", &conflicting));
	assert(critical_operation_id_equal(web.operation_id, conflicting.operation_id));
	conflicting.accepted_at_usec = 1;
	assert(!critical_command_equal(web, conflicting));
	// No descriptor or character exists. Preparation must not spend anything.
	const auto first = bid(2, "bidder", 1, 1000);
	assert(auction_ram_prepare_command(first).outcome == critical_apply_outcome::applied);
	currency_command_result current{};
	assert(economic_state_balances(2, "bidder", 1, &current));
	assert(current.wallet_revision == 4 && current.wallet.amount[3] == 10);
	assert(auction_ram_find(7, &listing) && listing.winner == 0);
	auto receipt = accept(first);
	assert(receipt.wallet.amount[3] == 9 && receipt.wallet_revision == 5);
	assert(receipt.end_time == 2300 && receipt.previous_bidder_pid == 0);
	assert(economic_state_balances(4, "bidder", 1, &current) && current.bank_revision == 9);
	// A delayed login/checkpoint cannot restore pre-bid wallet or shared bank.
	assert(economic_state_seed({ 2, "bidder", "Bidder", 1, balance }));
	assert(economic_state_balances(2, "bidder", 1, &current));
	assert(current.wallet.amount[3] == 9 && current.wallet_revision == 5 &&
	       current.bank_revision == 9);
	auction_ram_set_policy(400, 600);
	assert(auction_website_build_bid("bidder", 2, 7, 1000, "request-1", &retry));
	web.accepted_at_usec = retry.accepted_at_usec = 1;
	assert(critical_command_equal(web,
				      retry)); // no balances, clocks or mutable policy in identity
	auction_ram_set_policy(300, 300);
	// Same revision with conflicting values is corruption, never an overwrite.
	auto corrupt = current;
	corrupt.wallet.amount[3]++;
	assert(!economic_state_publish(2, "bidder", 1, corrupt));
	assert(!economic_state_balances(2, "other", 1, &current));
	assert(auction_ram_prepare_command(bid(1, "seller", 2, 2000)).error_code == EACCES);
	assert(auction_ram_prepare_command(bid(3, "other", 3, 50000)).outcome ==
	       critical_apply_outcome::applied); // capped buyout
	// Replacing an unadmitted candidate never mutates a prior bidder or pickup.
	receipt = accept(bid(3, "other", 4, 2000));
	assert(receipt.refund == 1000 && receipt.end_time == 2600);
	auction_ram_pickup pickup{};
	assert(auction_ram_find_pickup(2, &pickup) && pickup.amount == 1000 &&
	       pickup.revision == 1);
	receipt = accept(bid(3, "other", 5, 3000));
	assert(receipt.wallet_value_delta == -1000 && receipt.refund == 0 &&
	       receipt.end_time == 2600);
	assert(auction_ram_find_pickup(2, &pickup) && pickup.amount == 1000);
	auto expired = bid(2, "bidder", 6, 4000);
	uint32_t due[1]{};
	assert(auction_ram_expired(2599, due, 1) == 0);
	assert(auction_ram_expired(2600, due, 1) == 1 && due[0] == 7);
	expired.accepted_at_usec = 2600ULL * 1000000;
	assert(auction_ram_prepare_command(expired).error_code == EACCES);
	receipt = accept(bid(2, "bidder", 7, 5000));
	assert(receipt.status == 2 && receipt.refund == 3000 && receipt.seller_proceeds == 4850);
	assert(auction_ram_expired(9999, due, 1) == 0);
	assert(auction_ram_find(7, &listing) && listing.winner == 2 &&
	       listing.items[0].claimant == 2);
	assert(auction_ram_find_pickup(1, &pickup) && pickup.amount == 4850);
	assert(auction_ram_find_pickup(3, &pickup) && pickup.amount == 3000);
	assert(auction_ram_prepare_command(bid(3, "other", 8, 6000)).error_code == EACCES);
	// Tail fields survive a full nine-item receipt and never overlap custody.
	receipt.item_count = AUCTION_COMMAND_MAX_ITEMS;
	for (size_t i = 0; i < receipt.item_count; ++i)
	{
		receipt.item_uids[i] = i + 1;
		receipt.item_revisions[i] = i + 2;
	}
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> bytes{};
	assert(auction_command_encode_result(receipt, &bytes));
	auction_command_result decoded{};
	assert(auction_command_decode_result(bytes.data(), bytes.size(), &decoded));
	assert(decoded.item_uids[8] == 9 && decoded.item_revisions[8] == 10 &&
	       decoded.refund == 3000 && decoded.seller_proceeds == 4850);
	std::cout << "RAM auction and offline wallet regressions passed\n";
	// Pickup storage is unsigned 32-bit; large accumulated refunds remain claimable.
	auction_command_payload money_claim{};
	money_claim.action = auction_action::claim_money;
	money_claim.actor_pid = 2;
	money_claim.racewar = 1;
	strcpy(money_claim.account_name.data(), "bidder");
	money_claim.expected_wallet_revision = money_claim.expected_bank_revision = UINT64_MAX;
	critical_command claim;
	critical_operation_id claim_id{};
	claim_id.bytes[0] = 99;
	assert(auction_command_build(&claim, claim_id, money_claim, critical_source_site::command,
				     critical_deadline_class::interactive));
	claim.accepted_at_usec = 1000000000;
	auction_ram_listing next;
	auction_command_result claimed;
	assert(!auction_ram_prepare(claim, nullptr, balance, { UINT32_MAX, 3 }, 0, 0, &next,
				    &claimed));
	assert(claimed.wallet_value_delta == UINT32_MAX);
	auction_command_payload bounds = money_claim;
	bounds.action = auction_action::list;
	bounds.item_count = 1;
	bounds.items[0] = { 777, 1, 1234 };
	bounds.start_price = 1;
	bounds.buy_price = INT64_C(2147483648);
	bounds.end_time = 10001;
	strcpy(bounds.object_short.data(), "fixture");
	memcpy(bounds.object_blob.data(), "x", 2);
	bounds.object_blob_size = 2;
	assert(auction_command_build(&claim, claim_id, bounds, critical_source_site::command,
				     critical_deadline_class::interactive));
	claim.accepted_at_usec = 1000000000;
	assert(auction_ram_prepare(claim, nullptr, balance, {}, 0, 0, &next, &claimed) == EINVAL);
	bounds.buy_price = 1;
	bounds.end_time = UINT64_C(2147483648);
	assert(auction_command_build(&claim, claim_id, bounds, critical_source_site::command,
				     critical_deadline_class::interactive));
	claim.accepted_at_usec = 1000000000;
	assert(auction_ram_prepare(claim, nullptr, balance, {}, 0, 0, &next, &claimed) == EINVAL);
}
