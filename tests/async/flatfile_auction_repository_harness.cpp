#include "flatfile/flatfile_auction_repository.h"
#include "economy/auction_item_claim_accounting.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_player_domain_repository.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string>

namespace fs = std::filesystem;

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto initialize_evidence =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto create_mapping =
		&flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
};

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

static size_t native_bucket(uint16_t kind, uint64_t context, uint64_t pid, const std::string &name)
{
	std::vector<uint8_t> bytes;
	const auto add = [&](uint64_t value, size_t width)
	{
		for (size_t index = 0; index < width; ++index)
			bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
	};
	add(kind, 2);
	add(context, 8);
	add(kind, 2);
	if (kind == 1)
		add(pid, 8);
	else
		bytes.insert(bytes.end(), name.begin(), name.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest[0];
}

static void enable_accounting(const std::string &root, economic_account_key *wallet,
			      economic_account_key *bank, std::string *error)
{
	fs::create_directories(fs::path(root) / "economic-evidence");
	fs::permissions(fs::path(root) / "economic-evidence", fs::perms::owner_all,
			fs::perm_options::replace);
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction accounting setup");
	std::vector<flatfile_authority_operation> operations;
	const auto commit = [&]
	{
		require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
				flatfile_authority_transaction_result::ok,
			"could not commit auction accounting setup: " + *error);
		operations.clear();
	};
	const auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root, lock, &control, error) == 0,
			"could not read auction accounting control: " + *error);
		return control.revision;
	};
	require(flatfile_accounting_test_access::bootstrap(root, lock, operation(70), operation(71),
							   &operations, error) == 0,
		"could not bootstrap auction accounting: " + *error);
	commit();
	for (const size_t bucket :
	     { native_bucket(1, 0, 42, {}), native_bucket(2, 1, 0, "seller-account") })
	{
		const auto initialized = flatfile_accounting_test_access::initialize_native(
			root, lock, revision(), bucket, operation(72), &operations, error);
		require(initialized == 0 || initialized == EALREADY,
			"could not initialize auction native bucket: " + *error);
		if (!initialized)
			commit();
	}
	flatfile_economic_mapping mapping;
	require(flatfile_accounting_test_access::create_mapping(
			root, lock, revision(), economic_account_kind::wallet, 0, { 1, 42, {} },
			operation(73), &mapping, &operations, error) == 0,
		"could not map auction wallet: " + *error);
	*wallet = mapping.account;
	commit();
	require(flatfile_accounting_test_access::create_mapping(
			root, lock, revision(), economic_account_kind::bank, 1,
			{ 2, 0, "seller-account" }, operation(74), &mapping, &operations,
			error) == 0,
		"could not map auction bank: " + *error);
	*bank = mapping.account;
	commit();
	flatfile_economic_epoch epoch;
	epoch.epoch = operation(75);
	epoch.creating_operation = operation(76);
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	require(flatfile_accounting_test_access::append_epoch(root, lock, revision(), epoch,
							      &operations, error) == 0,
		"could not append auction accounting epoch: " + *error);
	commit();
	require(flatfile_accounting_test_access::select_epoch(
			root, lock, revision(), true, operation(77), &operations, error) == 0,
		"could not activate auction accounting epoch: " + *error);
	commit();
	require(flatfile_accounting_test_access::initialize_evidence(
			root, lock, revision(), operation(17).bytes[0], operation(78), &operations,
			error) == 0,
		"could not initialize auction evidence bucket: " + *error);
	commit();
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

static void expect_event(const std::string &root, auction_event_type type, uint32_t auction_id,
			 std::string *error)
{
	flatfile_auction_event_projection event;
	require(flatfile_auction_find_pending_event(root, &event, error) ==
				flatfile_auction_query_result::ok &&
			event.outbox_id != 0 && event.result.event_type == type &&
			event.result.auction_id == auction_id &&
			event.listing.auction_id == auction_id,
		"expected auction event was not durably pending");
	require(flatfile_auction_acknowledge_event(root, event.operation_id, error) ==
				flatfile_auction_query_result::ok &&
			flatfile_auction_acknowledge_event(root, event.operation_id, error) ==
				flatfile_auction_query_result::ok &&
			flatfile_auction_find_pending_event(root, &event, error) ==
				flatfile_auction_query_result::not_found,
		"auction event acknowledgement was not durable and idempotent");
}

static uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t *offset)
{
	require(*offset + 4 <= bytes.size(), "legacy conversion exceeded catalog payload");
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(bytes[(*offset)++]) << (index * 8);
	return value;
}

static void write_u32(std::vector<uint8_t> *bytes, size_t offset, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		(*bytes)[offset + index] = static_cast<uint8_t>(value >> (index * 8));
}

static void convert_catalog_to_legacy_v1(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	std::vector<uint8_t> file((std::istreambuf_iterator<char>(input)),
				  std::istreambuf_iterator<char>());
	require(file.size() >= 56, "auction catalog too short for legacy conversion");
	std::vector<uint8_t> payload(file.begin() + 56, file.end());
	size_t offset = 0;
	const uint32_t listing_count = read_u32(payload, &offset);
	for (uint32_t listing = 0; listing < listing_count; ++listing)
	{
		offset += 48;
		for (size_t text = 0; text < 6; ++text)
			offset += read_u32(payload, &offset);
		offset += read_u32(payload, &offset);
		require(offset + 2 <= payload.size(), "legacy conversion missed item count");
		const uint16_t item_count = static_cast<uint16_t>(payload[offset]) |
					    static_cast<uint16_t>(payload[offset + 1]) << 8;
		offset += 2 + static_cast<size_t>(item_count) * 25;
	}
	const uint32_t money_count = read_u32(payload, &offset);
	offset += static_cast<size_t>(money_count) * 20;
	const size_t operations_header = offset;
	const uint32_t operation_count = read_u32(payload, &offset);
	constexpr size_t version_two_operation_size =
		16 + 32 + 4 + AUCTION_RESULT_PAYLOAD_BYTES + 1;
	constexpr size_t version_one_operation_size = version_two_operation_size - 1;
	require(offset + static_cast<size_t>(operation_count) * version_two_operation_size ==
			payload.size(),
		"legacy conversion did not locate operation records");
	std::vector<uint8_t> legacy(payload.begin(), payload.begin() + operations_header + 4);
	for (uint32_t operation = 0; operation < operation_count; ++operation)
	{
		legacy.insert(legacy.end(), payload.begin() + offset,
			      payload.begin() + offset + version_one_operation_size);
		offset += version_two_operation_size;
	}
	write_u32(&file, 8, 1);
	write_u32(&file, 12, legacy.size());
	file.resize(56);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(legacy.data(), legacy.size(), digest.data());
	std::copy(digest.begin(), digest.end(), file.begin() + 24);
	file.insert(file.end(), legacy.begin(), legacy.end());
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(reinterpret_cast<const char *>(file.data()), file.size());
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
		{ 703, 703, 0, seller, 1, 1703, item_custody_state::active },
	};
	require(flatfile_item_repository_establish_owner(root.string(), seller, seller_items,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish auction custody: " + error);
	std::vector<flatfile_auction_listing_projection> open_listings;
	require(flatfile_auction_list_open(root.string(), &open_listings, &error) ==
				flatfile_auction_query_result::ok &&
			open_listings.empty(),
		"missing auction catalog was not projected as an empty list");

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
	economic_accounting_item_reference interrupted_reference = {};
	require(flatfile_item_accounting_reference_find_by_legacy(
			root.string(), list_command.operation_id, 0, &interrupted_reference,
			&error) == flatfile_item_accounting_status::not_found,
		"interrupted listing published an item reference before recovery");
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
	std::vector<economic_accounting_item_reference> listing_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			root.string(), list_command.operation_id, &listing_references, &error) ==
				flatfile_item_accounting_status::ok &&
			listing_references.size() == 1 && listing_references[0].item_uid == 700 &&
			listing_references[0].before_revision == 1 &&
			listing_references[0].after_revision == 2,
		"recovered listing did not atomically retain its item reference");
	expect_event(root.string(), auction_event_type::listed, listed.auction_id, &error);
	flatfile_auction_listing_projection listing_view;
	require(flatfile_auction_list_open(root.string(), &open_listings, &error) ==
				flatfile_auction_query_result::ok &&
			open_listings.size() == 1 &&
			open_listings[0].auction_id == listed.auction_id &&
			open_listings[0].seller_name == "Seller" &&
			open_listings[0].object_short == "an auction test item" &&
			open_listings[0].object_info == "durable test object" &&
			open_listings[0].items.size() == 1 &&
			flatfile_auction_find_open(root.string(), listed.auction_id, &listing_view,
						   &error) == flatfile_auction_query_result::ok &&
			listing_view.object_blob == std::vector<uint8_t>{ 0x5a },
		"open listing query did not project the durable catalog");
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
	expect_event(root.string(), auction_event_type::bid_placed, listed.auction_id, &error);

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
	expect_event(root.string(), auction_event_type::sold, listed.auction_id, &error);
	flatfile_auction_pickup_projection bidder_pickup, seller_pickup, buyer_pickup;
	require(flatfile_auction_list_open(root.string(), &open_listings, &error) ==
				flatfile_auction_query_result::ok &&
			open_listings.empty() &&
			flatfile_auction_find_open(root.string(), listed.auction_id, &listing_view,
						   &error) ==
				flatfile_auction_query_result::not_found &&
			flatfile_auction_find_pickup(root.string(), 43, &bidder_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			bidder_pickup.money == 3000 && !bidder_pickup.has_item_claim &&
			flatfile_auction_find_pickup(root.string(), 42, &seller_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			seller_pickup.money == 4500 && !seller_pickup.has_item_claim &&
			flatfile_auction_find_pickup(root.string(), 44, &buyer_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			buyer_pickup.money == 0 && buyer_pickup.has_item_claim &&
			buyer_pickup.item_claim.auction_id == listed.auction_id &&
			buyer_pickup.item_claim.items.size() == 1 &&
			buyer_pickup.item_claim.items[0].item_uid == 700 &&
			buyer_pickup.item_claim.items[0].item_revision == 2 &&
			buyer_pickup.item_claim.object_blob == std::vector<uint8_t>{ 0x5a },
		"settled auction projections did not expose refunds, proceeds, and custody");

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
	expect_event(root.string(), auction_event_type::listed, expiring_result.auction_id, &error);
	auction_command_payload finalize = {};
	finalize.action = auction_action::finalize;
	finalize.auction_id = expiring_result.auction_id;
	finalize.closing_fee_basis_points = 1000;
	applied = flatfile_auction_repository_apply(root.string(), command(finalize, 9));
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::expired,
		"expired auction did not finalize");
	expect_event(root.string(), auction_event_type::expired, expiring_result.auction_id,
		     &error);
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
	expect_event(root.string(), auction_event_type::listed, removable_result.auction_id,
		     &error);
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
	expect_event(root.string(), auction_event_type::removed, removable_result.auction_id,
		     &error);
	auction_command_payload removed_claim = {};
	removed_claim.action = auction_action::claim_item;
	removed_claim.auction_id = removable_result.auction_id;
	actor(&removed_claim, 42, "seller-account", "Seller", 4, 5);
	removed_claim.item_count = 1;
	removed_claim.items[0] = { 702, 2, 1702 };
	require(flatfile_auction_repository_apply(root.string(), command(removed_claim, 14))
				.outcome == critical_apply_outcome::applied,
		"removed item was not reclaimable by its seller");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire authority lock for player reference check");
		require(flatfile_auction_check_player_unreferenced(root.string(), lock, 42,
								   &error) ==
				flatfile_auction_player_reference_result::referenced,
			"auction history did not fence referenced player deletion");
		require(flatfile_auction_check_player_unreferenced(root.string(), lock, 99,
								   &error) ==
				flatfile_auction_player_reference_result::clear,
			"unreferenced player was fenced by auction authority");
	}
	flatfile_player_domain_record seller_account;
	require(flatfile_player_domain_load(root_path, 42, "seller-account", 1, &seller_account,
					    &error) == flatfile_player_domain_result::ok,
		"could not read seller before accounted auction claim");
	auction_command_payload accounted_listing = listing;
	actor(&accounted_listing, 42, "seller-account", "Seller",
	      seller_account.domains.wallet_revision, seller_account.domains.bank_revision);
	accounted_listing.listing_fee = 0;
	accounted_listing.buy_price = 0;
	accounted_listing.items[0] = { 703, 1, 1703 };
	applied = flatfile_auction_repository_apply(root_path, command(accounted_listing, 15));
	const auction_command_result staged_listing = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			staged_listing.item_revisions[0] == 2,
		"could not stage auction item for accounted claim");
	expect_event(root_path, auction_event_type::listed, staged_listing.auction_id, &error);
	auction_command_payload accounted_remove = {};
	accounted_remove.action = auction_action::remove;
	accounted_remove.auction_id = staged_listing.auction_id;
	actor(&accounted_remove, 42, "seller-account", "Seller", staged_listing.wallet_revision,
	      staged_listing.bank_revision);
	applied = flatfile_auction_repository_apply(root_path, command(accounted_remove, 16));
	const auction_command_result staged_removal = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			staged_removal.event_type == auction_event_type::removed,
		"could not stage seller claim after auction removal");
	expect_event(root_path, auction_event_type::removed, staged_listing.auction_id, &error);
	economic_account_key wallet_account, bank_account;
	enable_accounting(root_path, &wallet_account, &bank_account, &error);
	auction_command_payload accounted_claim = {};
	accounted_claim.action = auction_action::claim_item;
	accounted_claim.auction_id = staged_listing.auction_id;
	actor(&accounted_claim, 42, "seller-account", "Seller", staged_listing.wallet_revision,
	      staged_listing.bank_revision);
	accounted_claim.item_count = 1;
	accounted_claim.items[0] = { 703, 2, 1703 };
	auction_item_claim_state frozen_claim;
	frozen_claim.auction_id = staged_listing.auction_id;
	frozen_claim.seller_pid = 42;
	frozen_claim.claimant_pid = 42;
	frozen_claim.status = 3;
	frozen_claim.custody_state = 1;
	frozen_claim.auction_revision = staged_removal.auction_revision;
	frozen_claim.listing_operation = operation(15);
	frozen_claim.claim_source_operation = operation(16);
	frozen_claim.item_count = 1;
	frozen_claim.rows[0] = { 703, 2, 0, 1703, 42, false };
	critical_command accounted_command = command(accounted_claim, 17);
	require(auction_item_claim_accounting_intent(accounted_command, operation(75),
						     wallet_account, bank_account, frozen_claim,
						     &accounted_command.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze accounted auction claim");
	accounted_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	auction_item_claim_state wrong_source = frozen_claim;
	wrong_source.claim_source_operation = operation(13);
	critical_command stale_source_command = command(accounted_claim, 18);
	require(auction_item_claim_accounting_intent(stale_source_command, operation(75),
						     wallet_account, bank_account, wrong_source,
						     &stale_source_command.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze mismatched auction claim source");
	stale_source_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_item_claim(root_path,
									 stale_source_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"accounted auction claim admitted a different staging source");
	owned.clear();
	require(flatfile_item_repository_load_owner(
			root_path, { item_owner_type::auction, staged_listing.auction_id, 0 },
			&owner_revision, &owned, &error) == flatfile_item_repository_result::ok &&
			owned.size() == 1 && owned[0].item_uid == 703 &&
			owned[0].item_revision == 2,
		"mismatched auction claim source moved native custody");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_auction_repository_apply_accounted_item_claim(root_path,
									 accounted_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".critical-authority-transaction"),
		"accounted auction claim did not retain its interrupted journal");
	flatfile_accounting_record retained;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root_path, &error), "could not lock auction claim lookup");
		require(flatfile_accounting_lookup(root_path, lock, accounted_command, &retained,
						   &error) == flatfile_accounting_status::ok &&
				!retained.plan.empty() && retained.result_code == 0,
			"accounted auction claim did not recover its EAP1 plan: " + error);
	}
	economic_accounting_plan retained_plan;
	require(economic_plan_decode(retained.plan, &retained_plan) ==
				economic_accounting_error::ok &&
			retained_plan.accounts.empty() && retained_plan.postings.empty() &&
			retained_plan.item_events.size() == 1 &&
			retained_plan.item_events[0].uid == 703 &&
			retained_plan.metadata.source_event &&
			retained_plan.metadata.source_event->source.bytes == operation(16).bytes,
		"accounted auction claim lost custody or source evidence");
	applied = flatfile_auction_repository_apply_accounted_item_claim(root_path,
									 accounted_command);
	const auction_command_result accounted_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			accounted_result.item_revisions[0] == 3,
		"recovered accounted auction claim did not replay its result");
	std::vector<economic_accounting_item_reference> accounted_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			root_path, accounted_command.operation_id, &accounted_references, &error) ==
				flatfile_item_accounting_status::ok &&
			accounted_references.size() == 1 &&
			accounted_references[0].child_index == 0 &&
			accounted_references[0].item_uid == 703 &&
			accounted_references[0].after_revision == 3,
		"accounted auction claim did not retain its exact item reference");
	const fs::path catalog = domains / "auction_catalog";
	convert_catalog_to_legacy_v1(catalog);
	require(flatfile_auction_list_open(root.string(), &open_listings, &error) ==
				flatfile_auction_query_result::ok &&
			open_listings.empty(),
		"legacy v1 auction catalog was not readable after the event-state upgrade");
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
	require(flatfile_auction_list_open(root.string(), &open_listings, &error) ==
			flatfile_auction_query_result::invalid,
		"corrupt auction catalog was exposed through the query projection");
	for (const fs::directory_entry &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary auction authority file was left behind");

	std::cout << "flat-file auction repository passed\n";
	return 0;
}
