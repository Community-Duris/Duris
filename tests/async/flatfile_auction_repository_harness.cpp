#include "flatfile_auction_repository.h"
#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static critical_operation_id operation(uint8_t value)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xc7;
	id.bytes.back() = value;
	return id;
}

static flatfile_player_domain_record player(uint32_t pid, const char *account)
{
	flatfile_player_domain_record record;
	record.pid = pid;
	record.account_name = account;
	record.racewar = 1;
	record.domains.wallet = { 0, 0, 0, 10 };
	record.domains.bank = { 0, 0, 0, 0 };
	return record;
}

static void actor(auction_command_payload *payload, uint32_t pid, const char *account,
		  const char *name, uint64_t wallet_revision, uint64_t bank_revision)
{
	payload->actor_pid = pid;
	payload->racewar = 1;
	payload->expected_wallet_revision = wallet_revision;
	payload->expected_bank_revision = bank_revision;
	strcpy(payload->account_name.data(), account);
	strcpy(payload->actor_name.data(), name);
}

static critical_command command(const auction_command_payload &payload, uint8_t operation_value)
{
	critical_command value = {};
	require(auction_command_build(&value, operation(operation_value), payload,
				      critical_source_site::command,
				      critical_deadline_class::interactive),
		"could not build auction command");
	value.accepted_at_usec = operation_value;
	require(critical_command_normalize(&value), "could not normalize auction command");
	return value;
}

static auction_command_result result_of(const critical_apply_result &applied)
{
	auction_command_result result = {};
	require(auction_command_decode_result(applied.result_payload.data(), applied.result_size,
					      &result),
		"could not decode auction result");
	return result;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const std::string root_path = root.string();
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	require(flatfile_player_domain_establish(root.string(), player(42, "seller-account"),
						 &error) == flatfile_player_domain_result::ok &&
			flatfile_player_domain_establish(root.string(),
							 player(43, "bidder-account"), &error) ==
				flatfile_player_domain_result::ok &&
			flatfile_player_domain_establish(root.string(), player(44, "buyer-account"),
							 &error) ==
				flatfile_player_domain_result::ok,
		"could not establish auction players: " + error);
	const item_owner_identity seller = { item_owner_type::player, 42, 0 };
	const std::vector<flatfile_item_ownership_record> seller_items = {
		{ 700, 700, 0, seller, 1, 1700, item_custody_state::active },
		{ 701, 701, 0, seller, 1, 1701, item_custody_state::active },
		{ 702, 702, 0, seller, 1, 1702, item_custody_state::active },
	};
	require(flatfile_item_repository_establish_owner(root.string(), seller, seller_items,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish auction custody: " + error);

	auction_command_payload listing = {};
	listing.action = auction_action::list;
	actor(&listing, 42, "seller-account", "Seller", 0, 1);
	listing.start_price = 1000;
	listing.buy_price = 5000;
	listing.listing_fee = 100;
	listing.closing_fee_basis_points = 1000;
	listing.end_time = static_cast<uint64_t>(time(nullptr)) + 3600;
	listing.item_count = 1;
	listing.items[0] = { 700, 1, 1700 };
	listing.object_blob[0] = 0x5a;
	listing.object_blob_size = 1;
	strcpy(listing.object_short.data(), "an auction test item");
	strcpy(listing.id_keywords.data(), "auction test item");
	strcpy(listing.object_info.data(), "durable test object");
	const critical_command list_command = command(listing, 1);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	critical_apply_result applied = flatfile_critical_command_repository_apply_selected(
		list_command, const_cast<char *>(root_path.c_str()));
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".critical-authority-transaction"),
		"interrupted listing did not preserve its cross-authority intent");
	flatfile_player_domain_record loaded_player;
	require(flatfile_player_domain_load(root.string(), 42, "seller-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet == std::array<uint64_t, 4>{ 0, 0, 9, 9 } &&
			loaded_player.domains.wallet_revision == 1 &&
			loaded_player.domains.bank_revision == 2 &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"player load did not recover interrupted listing after-images");
	applied = flatfile_auction_repository_apply(root.string(), list_command);
	auction_command_result listed = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			listed.event_type == auction_event_type::listed && listed.auction_id != 0 &&
			listed.item_revisions[0] == 2,
		"recovered listing did not replay its result");
	auction_command_payload conflicting_listing = listing;
	conflicting_listing.listing_fee = 101;
	require(flatfile_auction_repository_apply(root.string(), command(conflicting_listing, 1))
				.error_code == EEXIST,
		"conflicting auction operation ID was accepted");
	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> owned;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::auction, listed.auction_id, 0 },
			&owner_revision, &owned, &error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && owned.size() == 1 && owned[0].item_uid == 700 &&
			owned[0].item_revision == 2,
		"listing did not transfer authoritative custody");

	auction_command_payload stale_bid = {};
	stale_bid.action = auction_action::bid;
	stale_bid.auction_id = listed.auction_id;
	stale_bid.value = 3000;
	stale_bid.closing_fee_basis_points = 1000;
	actor(&stale_bid, 43, "bidder-account", "Bidder", 9, 1);
	critical_command stale_command = command(stale_bid, 2);
	applied = flatfile_auction_repository_apply(root.string(), stale_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE &&
			flatfile_auction_repository_apply(root.string(), stale_command).error_code ==
				ESTALE,
		"stale bid decision was not durably replayed");

	auction_command_payload bid = stale_bid;
	actor(&bid, 43, "bidder-account", "Bidder", 0, 1);
	applied = flatfile_auction_repository_apply(root.string(), command(bid, 3));
	auction_command_result bid_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			bid_result.event_type == auction_event_type::bid_placed &&
			bid_result.final_price == 3000 && bid_result.wallet_value_delta == -3000 &&
			bid_result.auction_revision == 2,
		"valid bid did not apply");

	auction_command_payload buy = {};
	buy.action = auction_action::bid;
	buy.auction_id = listed.auction_id;
	buy.value = 6000;
	buy.closing_fee_basis_points = 1000;
	actor(&buy, 44, "buyer-account", "Buyer", 0, 1);
	applied = flatfile_auction_repository_apply(root.string(), command(buy, 4));
	auction_command_result buy_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			buy_result.event_type == auction_event_type::sold &&
			buy_result.final_price == 5000 && buy_result.previous_bidder_pid == 43 &&
			buy_result.auction_revision == 3,
		"buy-now settlement did not apply");

	auction_command_payload bidder_money = {};
	bidder_money.action = auction_action::claim_money;
	actor(&bidder_money, 43, "bidder-account", "Bidder", 1, 2);
	applied = flatfile_auction_repository_apply(root.string(), command(bidder_money, 5));
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).wallet_value_delta == 3000,
		"outbid refund was not claimable");
	auction_command_payload seller_money = {};
	seller_money.action = auction_action::claim_money;
	actor(&seller_money, 42, "seller-account", "Seller", 1, 2);
	applied = flatfile_auction_repository_apply(root.string(), command(seller_money, 6));
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).wallet_value_delta == 4500,
		"seller proceeds were not claimable");

	auction_command_payload claim = {};
	claim.action = auction_action::claim_item;
	claim.auction_id = listed.auction_id;
	actor(&claim, 44, "buyer-account", "Buyer", 1, 2);
	claim.item_count = 1;
	claim.items[0] = { 700, 2, 1700 };
	const critical_command claim_command = command(claim, 7);
	applied = flatfile_auction_repository_apply(root.string(), claim_command);
	auction_command_result claim_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			claim_result.event_type == auction_event_type::item_claimed &&
			claim_result.item_revisions[0] == 3 && claim_result.auction_revision == 4,
		"sold item claim did not apply");
	owned.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 44, 0 }, &owner_revision, &owned,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && owned.size() == 1 && owned[0].item_uid == 700 &&
			owned[0].item_revision == 3,
		"claimed item did not return to player custody");
	require(flatfile_auction_repository_apply(root.string(), claim_command).outcome ==
			critical_apply_outcome::already_applied,
		"item claim did not replay idempotently");

	auction_command_payload expiring = listing;
	actor(&expiring, 42, "seller-account", "Seller", 2, 3);
	expiring.listing_fee = 0;
	expiring.end_time = static_cast<uint64_t>(time(nullptr)) - 1;
	expiring.items[0] = { 701, 1, 1701 };
	applied = flatfile_auction_repository_apply(root.string(), command(expiring, 8));
	const auction_command_result expiring_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			expiring_result.event_type == auction_event_type::listed,
		"expiring listing did not apply");
	auction_command_payload finalize = {};
	finalize.action = auction_action::finalize;
	finalize.auction_id = expiring_result.auction_id;
	finalize.closing_fee_basis_points = 1000;
	applied = flatfile_auction_repository_apply(root.string(), command(finalize, 9));
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::expired,
		"expired auction did not finalize");
	auction_command_payload expired_claim = {};
	expired_claim.action = auction_action::claim_item;
	expired_claim.auction_id = expiring_result.auction_id;
	actor(&expired_claim, 42, "seller-account", "Seller", 3, 4);
	expired_claim.item_count = 1;
	expired_claim.items[0] = { 701, 2, 1701 };
	require(flatfile_auction_repository_apply(root.string(), command(expired_claim, 10))
				.outcome == critical_apply_outcome::applied,
		"expired item was not reclaimable by its seller");

	auction_command_payload removable = listing;
	actor(&removable, 42, "seller-account", "Seller", 3, 4);
	removable.listing_fee = 0;
	removable.buy_price = 0;
	removable.items[0] = { 702, 1, 1702 };
	applied = flatfile_auction_repository_apply(root.string(), command(removable, 11));
	const auction_command_result removable_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied,
		"removable listing did not apply");
	auction_command_payload unaffordable = {};
	unaffordable.action = auction_action::bid;
	unaffordable.auction_id = removable_result.auction_id;
	unaffordable.value = 6000;
	unaffordable.closing_fee_basis_points = 1000;
	actor(&unaffordable, 44, "buyer-account", "Buyer", 1, 2);
	const critical_command unaffordable_command = command(unaffordable, 12);
	require(flatfile_auction_repository_apply(root.string(), unaffordable_command).error_code ==
				ENOSPC &&
			flatfile_auction_repository_apply(root.string(), unaffordable_command)
					.error_code == ENOSPC,
		"insufficient auction bid was not rolled back and durably rejected");
	auction_command_payload remove = {};
	remove.action = auction_action::remove;
	remove.auction_id = removable_result.auction_id;
	actor(&remove, 42, "seller-account", "Seller", 4, 5);
	applied = flatfile_auction_repository_apply(root.string(), command(remove, 13));
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::removed &&
			result_of(applied).winner_pid == 0,
		"open auction removal did not stage seller custody");
	auction_command_payload removed_claim = {};
	removed_claim.action = auction_action::claim_item;
	removed_claim.auction_id = removable_result.auction_id;
	actor(&removed_claim, 42, "seller-account", "Seller", 4, 5);
	removed_claim.item_count = 1;
	removed_claim.items[0] = { 702, 2, 1702 };
	require(flatfile_auction_repository_apply(root.string(), command(removed_claim, 14))
				.outcome == critical_apply_outcome::applied,
		"removed item was not reclaimable by its seller");
	const fs::path catalog = domains / "auction_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open auction catalog for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x24;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_auction_repository_apply(root.string(), claim_command).error_code ==
			EILSEQ,
		"corrupt auction catalog was accepted or overwritten");
	for (const fs::directory_entry &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary auction authority file was left behind");

	std::cout << "flat-file auction repository passed\n";
	return 0;
}
