#include "flatfile/flatfile_auction_repository.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_player_domain_repository.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <thread>

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

static uint32_t auction_id_for(const critical_operation_id &operation_id)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(operation_id.bytes.data(), operation_id.bytes.size(), digest.data());
	uint32_t id = 0;
	for (size_t byte = 0; byte < 4; ++byte)
		id |= static_cast<uint32_t>(digest[byte]) << (byte * 8);
	return id ? id : 1;
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
	if (kind != 2)
		add(pid, 8);
	else
		bytes.insert(bytes.end(), name.begin(), name.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest[0];
}

static void enable_accounting(const std::string &root, economic_account_key *wallet,
			      economic_account_key *bank, std::string *error,
			      uint32_t auction_id = 0)
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
	std::vector<size_t> native_buckets = { native_bucket(1, 0, 42, {}),
					       native_bucket(2, 1, 0, "seller-account") };
	if (auction_id)
		native_buckets.push_back(native_bucket(4, 0, auction_id, {}));
	for (const size_t bucket : native_buckets)
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

static economic_account_key map_account(const std::string &root, economic_account_kind kind,
					uint64_t context, const flatfile_economic_locator &locator,
					uint8_t initialize_operation, uint8_t mapping_operation,
					std::string *error)
{
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction mapping setup");
	const auto revision = [&]
	{
		flatfile_economic_control control;
		require(flatfile_economic_control_read(root, lock, &control, error) == 0,
			"could not read auction mapping control: " + *error);
		return control.revision;
	};
	std::vector<flatfile_authority_operation> operations;
	const size_t bucket = native_bucket(locator.kind, context, locator.native_id, locator.name);
	const auto initialized = flatfile_accounting_test_access::initialize_native(
		root, lock, revision(), bucket, operation(initialize_operation), &operations,
		error);
	require(initialized == 0 || initialized == EALREADY,
		"could not initialize auction account bucket: " + *error);
	if (!initialized)
	{
		require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
				flatfile_authority_transaction_result::ok,
			"could not commit auction account bucket: " + *error);
		operations.clear();
	}
	flatfile_economic_mapping mapping;
	require(flatfile_accounting_test_access::create_mapping(
			root, lock, revision(), kind, context, locator,
			operation(mapping_operation), &mapping, &operations, error) == 0,
		"could not map auction account: " + *error);
	require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
			flatfile_authority_transaction_result::ok,
		"could not commit auction account mapping: " + *error);
	return mapping.account;
}

static void initialize_auction_bucket(const std::string &root, uint32_t auction_id,
				      uint8_t operation_value, std::string *error)
{
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction bucket setup");
	flatfile_economic_control control;
	require(flatfile_economic_control_read(root, lock, &control, error) == 0,
		"could not read auction bucket control: " + *error);
	std::vector<flatfile_authority_operation> operations;
	const auto initialized = flatfile_accounting_test_access::initialize_native(
		root, lock, control.revision, native_bucket(4, 0, auction_id, {}),
		operation(operation_value), &operations, error);
	require(initialized == 0 || initialized == EALREADY,
		"could not initialize auction escrow bucket: " + *error);
	if (!initialized)
		require(flatfile_accounting_test_access::commit(root, lock, operations, error) ==
				flatfile_authority_transaction_result::ok,
			"could not commit auction escrow bucket: " + *error);
}

static economic_account_key escrow_account(const std::string &root, uint32_t auction_id,
					   std::string *error)
{
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction escrow lookup");
	flatfile_economic_mapping mapping;
	require(flatfile_economic_native_lookup(root, lock, economic_account_kind::auction_escrow,
						0, { 4, auction_id, {} }, &mapping, error) == 0,
		"could not find auction escrow mapping: " + *error);
	return mapping.account;
}

static void expect_escrow_lifetime(const std::string &root, uint32_t auction_id,
				   const economic_account_key &key,
				   const critical_operation_id &retiring_operation,
				   std::string *error)
{
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction lifetime lookup");
	flatfile_economic_mapping retained, active;
	require(flatfile_economic_mapping_read(root, lock, key, &retained, error) == 0 &&
			retained.retiring_operation.bytes == retiring_operation.bytes &&
			retained.revision ==
				(critical_operation_id_is_zero(retiring_operation) ? 0U : 1U),
		"auction escrow lifetime did not retain its closure operation");
	const auto lookup =
		flatfile_economic_native_lookup(root, lock, economic_account_kind::auction_escrow,
						0, { 4, auction_id, {} }, &active, error);
	require(critical_operation_id_is_zero(retiring_operation) ?
			lookup == 0 && active.account.authority_id == key.authority_id :
			lookup == ENODATA,
		"auction escrow lifetime has the wrong active native mapping");
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

static critical_command accounted_bid(const auction_command_payload &payload,
				      uint8_t operation_value,
				      const auction_bid_accounting_listing &listing,
				      const auction_bid_accounting_accounts &accounts)
{
	critical_command value = command(payload, operation_value);
	require(auction_bid_accounting_intent(value, operation(75), listing, accounts,
					      &value.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze accounted auction bid");
	value.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	return value;
}

static critical_command accounted_settlement(const auction_command_payload &payload,
					     uint8_t operation_value,
					     const auction_settlement_listing &listing,
					     const auction_settlement_accounts &accounts)
{
	critical_command value = command(payload, operation_value);
	require(auction_settlement_accounting_intent(value, operation(75), listing, accounts,
						     &value.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze accounted auction settlement");
	value.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	return value;
}

static critical_command accounted_money_claim(const auction_command_payload &payload,
					      uint8_t operation_value,
					      const economic_account_key &wallet,
					      const economic_account_key &bank,
					      const economic_account_key &claim_account,
					      const auction_money_claim_state &state)
{
	critical_command value = command(payload, operation_value);
	require(auction_money_claim_accounting_intent(
			value, operation(75), wallet, bank, claim_account, state,
			&value.accounting_intent) == economic_accounting_error::ok,
		"could not freeze accounted auction money claim");
	value.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	return value;
}

static std::vector<uint8_t> source_catalog_bytes(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
				   std::istreambuf_iterator<char>());
	require(bytes.size() >= 60 && std::equal(bytes.begin(), bytes.begin() + 7, "DURAUSR") &&
			bytes[7] == 0,
		"accounted auction source catalog is missing");
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data() + 56, bytes.size() - 56, digest.data());
	require(std::equal(digest.begin(), digest.end(), bytes.begin() + 24),
		"accounted auction source catalog digest is invalid");
	return bytes;
}

static economic_accounting_plan recorded_plan(const std::string &root,
					      const critical_command &command, std::string *error)
{
	flatfile_authority_lock lock;
	require(lock.acquire(root, error), "could not lock auction EAP1 lookup");
	flatfile_accounting_record record;
	require(flatfile_accounting_lookup(root, lock, command, &record, error) ==
				flatfile_accounting_status::ok &&
			record.result_code == 0 && !record.plan.empty(),
		"auction operation did not retain its EAP1 plan: " + *error);
	economic_accounting_plan plan;
	require(economic_plan_decode(record.plan, &plan) == economic_accounting_error::ok,
		"auction operation retained an invalid EAP1 plan");
	return plan;
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
	const fs::path typed_root = root / "accounted-listing";
	const std::string typed_path = typed_root.string();
	const fs::path typed_domains = typed_root / "domains";
	fs::create_directories(typed_domains);
	fs::permissions(typed_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(typed_domains, fs::perms::owner_all, fs::perm_options::replace);
	require(flatfile_player_domain_establish(typed_path, player(42, "seller-account"),
						 &error) == flatfile_player_domain_result::ok &&
			flatfile_player_domain_establish(typed_path, player(43, "bidder-account"),
							 &error) ==
				flatfile_player_domain_result::ok &&
			flatfile_player_domain_establish(typed_path, player(44, "buyer-account"),
							 &error) ==
				flatfile_player_domain_result::ok &&
			flatfile_item_repository_establish_owner(
				typed_path, seller,
				{ { 800, 800, 0, seller, 1, 1800, item_custody_state::active },
				  { 900, 900, 0, seller, 1, 1900, item_custody_state::active },
				  { 901, 900, 900, seller, 1, 1901, item_custody_state::active },
				  { 902, 902, 0, seller, 1, 1902, item_custody_state::active },
				  { 903, 903, 0, seller, 1, 1903, item_custody_state::active },
				  { 904, 904, 0, seller, 1, 1904, item_custody_state::active } },
				&error) == flatfile_item_baseline_result::applied,
		"could not establish accounted listing fixture: " + error);
	const uint32_t expected_auction_id = auction_id_for(operation(19));
	economic_account_key typed_wallet, typed_bank;
	enable_accounting(typed_path, &typed_wallet, &typed_bank, &error, expected_auction_id);
	auction_command_payload typed_listing = listing;
	typed_listing.items[0] = { 800, 1, 1800 };
	critical_command typed_command = command(typed_listing, 19);
	require(auction_listing_accounting_intent(typed_command, operation(75), typed_wallet,
						  typed_bank, &typed_command.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze accounted auction listing");
	typed_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	critical_command wrong_mapping = command(typed_listing, 20);
	auto unmapped_bank = typed_bank;
	unmapped_bank.authority_id += 100;
	require(auction_listing_accounting_intent(
			wrong_mapping, operation(75), typed_wallet, unmapped_bank,
			&wrong_mapping.accounting_intent) == economic_accounting_error::ok,
		"could not freeze wrong auction mapping");
	wrong_mapping.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path, wrong_mapping);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"accounted auction listing admitted an unmapped bank");
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path, typed_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(typed_domains / ".critical-authority-transaction"),
		"accounted auction listing did not retain its interrupted journal: " +
			std::to_string(static_cast<int>(applied.outcome)) + "/" +
			std::to_string(applied.error_code) + " " + error);
	flatfile_accounting_record typed_record;
	flatfile_economic_mapping typed_escrow;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(typed_path, &error), "could not lock listing lookup");
		require(flatfile_accounting_lookup(typed_path, lock, typed_command, &typed_record,
						   &error) == flatfile_accounting_status::ok &&
				!typed_record.plan.empty() && typed_record.result_code == 0,
			"accounted auction listing did not recover its EAP1 plan: " + error);
		require(flatfile_economic_native_lookup(
				typed_path, lock, economic_account_kind::auction_escrow, 0,
				{ 4, expected_auction_id, {} }, &typed_escrow, &error) == 0 &&
				typed_escrow.creating_operation.bytes ==
					typed_command.operation_id.bytes,
			"accounted auction listing did not allocate its escrow mapping: " + error);
	}
	economic_accounting_plan typed_plan;
	require(economic_plan_decode(typed_record.plan, &typed_plan) ==
				economic_accounting_error::ok &&
			typed_plan.accounts.size() == 3 && typed_plan.postings.size() == 2 &&
			typed_plan.item_events.size() == 1 &&
			economic_account_key_equal(typed_plan.accounts[1].key,
						   typed_escrow.account) &&
			typed_plan.item_events[0].uid == 800 &&
			typed_plan.postings[0].copper == -100 &&
			typed_plan.postings[1].copper == 100,
		"accounted auction listing lost fee, escrow, or custody evidence");
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path, typed_command);
	const auction_command_result typed_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			typed_result.auction_id == expected_auction_id &&
			typed_result.item_revisions[0] == 2 &&
			typed_result.wallet_value_delta == -100,
		"recovered accounted auction listing did not replay its result");
	std::vector<economic_accounting_item_reference> typed_references;
	require(flatfile_item_accounting_reference_find_by_operation(
			typed_path, typed_command.operation_id, &typed_references, &error) ==
				flatfile_item_accounting_status::ok &&
			typed_references.size() == 1 && typed_references[0].item_uid == 800 &&
			typed_references[0].child_index == 0 &&
			typed_references[0].after_revision == 2,
		"accounted auction listing lost its exact item reference");
	require(flatfile_player_domain_load(typed_path, 42, "seller-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet == std::array<uint64_t, 4>{ 0, 0, 9, 9 },
		"accounted auction listing did not commit its wallet fee");
	expect_event(typed_path, auction_event_type::listed, expected_auction_id, &error);
	auction_command_payload nested_listing = typed_listing;
	actor(&nested_listing, 42, "seller-account", "Seller", typed_result.wallet_revision,
	      typed_result.bank_revision);
	nested_listing.items[0] = { 900, 1, 1900 };
	critical_command nested_command = command(nested_listing, 21);
	require(auction_listing_accounting_intent(nested_command, operation(75), typed_wallet,
						  typed_bank, &nested_command.accounting_intent) ==
			economic_accounting_error::ok,
		"could not freeze nested auction listing");
	nested_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path, nested_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EOPNOTSUPP,
		"accounted auction listing moved a root with descendants");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(typed_path, &error), "could not lock nested listing lookup");
		require(flatfile_accounting_lookup(typed_path, lock, nested_command, &typed_record,
						   &error) == flatfile_accounting_status::ok &&
				typed_record.result_code == EOPNOTSUPP && typed_record.plan.empty(),
			"nested auction listing did not retain an empty-plan refusal");
	}
	owned.clear();
	require(flatfile_item_repository_load_owner(typed_path, seller, &owner_revision, &owned,
						    &error) ==
				flatfile_item_repository_result::ok &&
			owned.size() == 5 && owned[0].item_uid == 900 &&
			owned[0].item_revision == 1 && owned[1].item_uid == 901 &&
			owned[1].item_revision == 1 &&
			flatfile_player_domain_load(typed_path, 42, "seller-account", 1,
						    &loaded_player,
						    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet_revision == typed_result.wallet_revision,
		"refused nested auction listing changed native custody or wallet");
	const auto bidder_wallet = map_account(typed_path, economic_account_kind::wallet, 0,
					       { 1, 43, {} }, 80, 81, &error);
	const auto bidder_bank = map_account(typed_path, economic_account_kind::bank, 1,
					     { 2, 0, "bidder-account" }, 82, 83, &error);
	const auto buyer_wallet = map_account(typed_path, economic_account_kind::wallet, 0,
					      { 1, 44, {} }, 84, 85, &error);
	const auto buyer_bank = map_account(typed_path, economic_account_kind::bank, 1,
					    { 2, 0, "buyer-account" }, 86, 87, &error);
	const auto bidder_claim = map_account(typed_path, economic_account_kind::pending_claim, 0,
					      { 5, 43, {} }, 88, 89, &error);
	const auto buyer_claim = map_account(typed_path, economic_account_kind::pending_claim, 0,
					     { 5, 44, {} }, 92, 93, &error);
	const auto seller_claim = map_account(typed_path, economic_account_kind::pending_claim, 0,
					      { 5, 42, {} }, 90, 91, &error);
	auction_bid_accounting_listing bid_listing;
	bid_listing.auction_id = expected_auction_id;
	bid_listing.seller_pid = 42;
	bid_listing.status = 1;
	bid_listing.custody_state = 1;
	bid_listing.current_price = 1000;
	bid_listing.buy_price = 5000;
	bid_listing.revision = 1;
	bid_listing.listing_operation = typed_command.operation_id;
	auction_bid_accounting_accounts bid_accounts;
	bid_accounts.wallet = bidder_wallet;
	bid_accounts.bank = bidder_bank;
	bid_accounts.escrow = typed_escrow.account;
	bid_accounts.bidder_claim = bidder_claim;
	auction_command_payload opening_bid = {};
	opening_bid.action = auction_action::bid;
	opening_bid.auction_id = expected_auction_id;
	opening_bid.value = 2000;
	opening_bid.closing_fee_basis_points = 1000;
	actor(&opening_bid, 43, "bidder-account", "Bidder", 0, 1);
	const auto opening_command = accounted_bid(opening_bid, 22, bid_listing, bid_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, opening_command);
	const auto opening_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			opening_result.event_type == auction_event_type::bid_placed &&
			opening_result.final_price == 2000 &&
			opening_result.wallet_value_delta == -2000 &&
			opening_result.auction_revision == 2,
		"accounted opening bid did not apply");
	auto bid_plan = recorded_plan(typed_path, opening_command, &error);
	require(bid_plan.accounts.size() == 2 && bid_plan.postings.size() == 2 &&
			bid_plan.item_events.empty() && bid_plan.postings[0].copper == -2000 &&
			bid_plan.postings[1].copper == 2000 &&
			bid_plan.accounts[1].after == (economic_coin_vector{ 2000, 0, 0, 0 }) &&
			bid_plan.metadata.source_event &&
			bid_plan.metadata.source_event->source.bytes ==
				typed_command.operation_id.bytes,
		"accounted opening bid lost escrow or source evidence");
	require(flatfile_auction_repository_apply_accounted_bid(typed_path, opening_command)
				.outcome == critical_apply_outcome::already_applied,
		"accounted opening bid did not replay");
	expect_event(typed_path, auction_event_type::bid_placed, expected_auction_id, &error);
	bid_listing.winning_bidder_pid = 43;
	bid_listing.current_price = 2000;
	bid_listing.revision = 2;
	bid_listing.previous_bid_operation = opening_command.operation_id;
	actor(&opening_bid, 43, "bidder-account", "Bidder", opening_result.wallet_revision,
	      opening_result.bank_revision);
	opening_bid.value = 3000;
	const auto raise_command = accounted_bid(opening_bid, 23, bid_listing, bid_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, raise_command);
	const auto raise_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			raise_result.wallet_value_delta == -1000 &&
			raise_result.auction_revision == 3,
		"accounted same-bidder raise charged more than its increment");
	bid_plan = recorded_plan(typed_path, raise_command, &error);
	require(bid_plan.accounts.size() == 2 && bid_plan.postings.size() == 2 &&
			bid_plan.postings[0].copper == -1000 &&
			bid_plan.accounts[1].before == (economic_coin_vector{ 2000, 0, 0, 0 }) &&
			bid_plan.accounts[1].after == (economic_coin_vector{ 3000, 0, 0, 0 }) &&
			bid_plan.metadata.source_event &&
			bid_plan.metadata.source_event->source.bytes ==
				opening_command.operation_id.bytes,
		"accounted same-bidder raise lost incremental escrow evidence");
	expect_event(typed_path, auction_event_type::bid_placed, expected_auction_id, &error);
	auction_command_payload outbid = opening_bid;
	actor(&outbid, 44, "buyer-account", "Buyer", 0, 1);
	outbid.value = 4000;
	auction_bid_accounting_accounts outbid_accounts = bid_accounts;
	outbid_accounts.wallet = buyer_wallet;
	outbid_accounts.bank = buyer_bank;
	outbid_accounts.bidder_claim = buyer_claim;
	outbid_accounts.previous_claim = bidder_claim;
	const auto typed_stale_command = accounted_bid(outbid, 24, bid_listing, outbid_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, typed_stale_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"accounted auction bid accepted a stale prior bid source");
	bid_listing.current_price = 3000;
	bid_listing.revision = 3;
	bid_listing.previous_bid_operation = raise_command.operation_id;
	const auto outbid_command = accounted_bid(outbid, 25, bid_listing, outbid_accounts);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, outbid_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(typed_domains / ".critical-authority-transaction"),
		"accounted outbid did not retain its interrupted journal");
	bid_plan = recorded_plan(typed_path, outbid_command, &error);
	require(bid_plan.accounts.size() == 3 && bid_plan.postings.size() == 4 &&
			bid_plan.postings[0].copper == -4000 &&
			bid_plan.postings[1].copper == 4000 &&
			bid_plan.postings[2].copper == -3000 &&
			bid_plan.postings[3].copper == 3000 &&
			bid_plan.accounts[2].after == (economic_coin_vector{ 3000, 0, 0, 0 }) &&
			bid_plan.metadata.source_event &&
			bid_plan.metadata.source_event->source.bytes ==
				raise_command.operation_id.bytes,
		"accounted outbid lost refund or source evidence");
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, outbid_command);
	const auto outbid_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			outbid_result.auction_revision == 4 &&
			outbid_result.previous_bidder_pid == 43,
		"recovered accounted outbid did not replay");
	flatfile_auction_pickup_projection typed_bidder_pickup, typed_seller_pickup;
	require(flatfile_auction_find_pickup(typed_path, 43, &typed_bidder_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			typed_bidder_pickup.money == 3000 &&
			typed_bidder_pickup.money_revision == 1,
		"accounted outbid did not retain the former bidder claim");
	expect_event(typed_path, auction_event_type::bid_placed, expected_auction_id, &error);
	bid_listing.winning_bidder_pid = 44;
	bid_listing.current_price = 4000;
	bid_listing.revision = 4;
	bid_listing.previous_bid_operation = outbid_command.operation_id;
	auction_bid_accounting_accounts buy_accounts = outbid_accounts;
	buy_accounts.previous_claim = {};
	buy_accounts.seller_claim = seller_claim;
	actor(&outbid, 44, "buyer-account", "Buyer", outbid_result.wallet_revision,
	      outbid_result.bank_revision);
	outbid.value = 6000;
	const auto buy_command = accounted_bid(outbid, 26, bid_listing, buy_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, buy_command);
	const auto typed_buy_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			typed_buy_result.event_type == auction_event_type::sold &&
			typed_buy_result.final_price == 5000 &&
			typed_buy_result.wallet_value_delta == -1000 &&
			typed_buy_result.auction_revision == 5,
		"accounted buy-now did not settle the incremental bid");
	bid_plan = recorded_plan(typed_path, buy_command, &error);
	require(bid_plan.accounts.size() == 4 && bid_plan.postings.size() == 5 &&
			bid_plan.postings[0].copper == -1000 &&
			bid_plan.postings[1].copper == 1000 &&
			bid_plan.postings[2].copper == -5000 &&
			bid_plan.postings[3].copper == 4500 && bid_plan.postings[4].copper == 500 &&
			bid_plan.accounts[1].after == economic_coin_vector{} &&
			bid_plan.metadata.source_event &&
			bid_plan.metadata.source_event->source.bytes ==
				outbid_command.operation_id.bytes,
		"accounted buy-now lost seller proceeds, fee, or escrow evidence");
	require(flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 4500 &&
			typed_seller_pickup.money_revision == 1 &&
			flatfile_auction_find_pickup(typed_path, 44, &buyer_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			buyer_pickup.has_item_claim && buyer_pickup.item_claim.items.size() == 1 &&
			buyer_pickup.item_claim.items[0].item_uid == 800,
		"accounted buy-now did not retain seller money or buyer item claim");
	require(flatfile_auction_repository_apply_accounted_bid(typed_path, buy_command).outcome ==
			critical_apply_outcome::already_applied,
		"accounted buy-now did not replay");
	expect_escrow_lifetime(typed_path, expected_auction_id, typed_escrow.account,
			       buy_command.operation_id, &error);
	expect_event(typed_path, auction_event_type::sold, expected_auction_id, &error);
	const uint32_t sale_auction_id = auction_id_for(operation(27));
	initialize_auction_bucket(typed_path, sale_auction_id, 92, &error);
	auction_command_payload sale_listing = typed_listing;
	actor(&sale_listing, 42, "seller-account", "Seller", typed_result.wallet_revision,
	      typed_result.bank_revision);
	sale_listing.items[0] = { 902, 1, 1902 };
	sale_listing.listing_fee = 0;
	sale_listing.buy_price = 0;
	sale_listing.end_time = static_cast<uint64_t>(time(nullptr)) + 4;
	critical_command sale_listing_command = command(sale_listing, 27);
	require(auction_listing_accounting_intent(
			sale_listing_command, operation(75), typed_wallet, typed_bank,
			&sale_listing_command.accounting_intent) == economic_accounting_error::ok,
		"could not freeze timed-sale listing");
	sale_listing_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path,
								      sale_listing_command);
	const auto sale_listing_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			sale_listing_result.auction_id == sale_auction_id,
		"could not list timed-sale fixture");
	expect_event(typed_path, auction_event_type::listed, sale_auction_id, &error);
	const auto sale_escrow = escrow_account(typed_path, sale_auction_id, &error);
	auction_bid_accounting_listing sale_bid_listing;
	sale_bid_listing.auction_id = sale_auction_id;
	sale_bid_listing.seller_pid = 42;
	sale_bid_listing.status = 1;
	sale_bid_listing.custody_state = 1;
	sale_bid_listing.current_price = 1000;
	sale_bid_listing.revision = 1;
	sale_bid_listing.listing_operation = sale_listing_command.operation_id;
	auction_bid_accounting_accounts sale_bid_accounts;
	sale_bid_accounts.wallet = bidder_wallet;
	sale_bid_accounts.bank = bidder_bank;
	sale_bid_accounts.escrow = sale_escrow;
	sale_bid_accounts.bidder_claim = bidder_claim;
	auction_command_payload sale_bid_payload = opening_bid;
	actor(&sale_bid_payload, 43, "bidder-account", "Bidder", raise_result.wallet_revision,
	      raise_result.bank_revision);
	sale_bid_payload.auction_id = sale_auction_id;
	sale_bid_payload.value = 3000;
	const auto sale_bid_command =
		accounted_bid(sale_bid_payload, 28, sale_bid_listing, sale_bid_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, sale_bid_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::bid_placed &&
			result_of(applied).claim_credit_used == 3000 &&
			result_of(applied).wallet_value_delta == 0,
		"could not stage accounted timed-sale bid outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	expect_event(typed_path, auction_event_type::bid_placed, sale_auction_id, &error);
	auction_settlement_listing sale_state;
	sale_state.auction_id = sale_auction_id;
	sale_state.seller_pid = 42;
	sale_state.winner_pid = 43;
	sale_state.status = 1;
	sale_state.custody_state = 1;
	sale_state.quantity = 1;
	sale_state.current_price = 3000;
	sale_state.revision = 2;
	sale_state.end_time = sale_listing.end_time;
	sale_state.listing_operation = sale_listing_command.operation_id;
	sale_state.winning_bid_operation = sale_bid_command.operation_id;
	sale_state.item_count = 1;
	sale_state.items[0] = { 902, 2, 0, 1902, 0, false };
	auction_settlement_accounts sale_accounts;
	sale_accounts.escrow = sale_escrow;
	sale_accounts.seller_claim = seller_claim;
	auction_command_payload sale_finalize = {};
	sale_finalize.action = auction_action::finalize;
	sale_finalize.auction_id = sale_auction_id;
	sale_finalize.closing_fee_basis_points = 1000;
	const auto sale_command =
		accounted_settlement(sale_finalize, 29, sale_state, sale_accounts);
	while (static_cast<uint64_t>(time(nullptr)) < sale_listing.end_time)
		std::this_thread::sleep_for(std::chrono::seconds(1));
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_auction_repository_apply_accounted_finalize(typed_path, sale_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(typed_domains / ".critical-authority-transaction"),
		"accounted timed sale did not retain its interrupted journal");
	const auto sale_plan = recorded_plan(typed_path, sale_command, &error);
	require(sale_plan.accounts.size() == 3 && sale_plan.postings.size() == 3 &&
			sale_plan.postings[0].copper == -3000 &&
			sale_plan.postings[1].copper == 2700 &&
			sale_plan.postings[2].copper == 300 &&
			sale_plan.accounts[0].after == economic_coin_vector{} &&
			sale_plan.accounts[1].before == (economic_coin_vector{ 4500, 0, 0, 0 }) &&
			sale_plan.accounts[1].after == (economic_coin_vector{ 7200, 0, 0, 0 }) &&
			sale_plan.items_before.size() == 1 && sale_plan.items_after.size() == 1 &&
			sale_plan.item_events.empty() && sale_plan.items_before[0].uid == 902 &&
			sale_plan.items_after[0].position.revision == 2 &&
			sale_plan.metadata.source_event &&
			sale_plan.metadata.source_event->source.bytes ==
				sale_bid_command.operation_id.bytes,
		"accounted timed sale lost escrow, proceeds, fee, or custody witness");
	applied = flatfile_auction_repository_apply_accounted_finalize(typed_path, sale_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			result_of(applied).event_type == auction_event_type::sold &&
			flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup,
						     &error) == flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 7200 &&
			typed_seller_pickup.money_revision == 2 &&
			flatfile_auction_find_pickup(typed_path, 43, &typed_bidder_pickup,
						     &error) == flatfile_auction_query_result::ok &&
			typed_bidder_pickup.has_item_claim &&
			typed_bidder_pickup.item_claim.items[0].item_uid == 902,
		"recovered accounted sale lost seller money or winner item claim");
	expect_escrow_lifetime(typed_path, sale_auction_id, sale_escrow, sale_command.operation_id,
			       &error);
	expect_event(typed_path, auction_event_type::sold, sale_auction_id, &error);
	const uint32_t remove_auction_id = auction_id_for(operation(30));
	initialize_auction_bucket(typed_path, remove_auction_id, 93, &error);
	auction_command_payload remove_listing = sale_listing;
	actor(&remove_listing, 42, "seller-account", "Seller", sale_listing_result.wallet_revision,
	      sale_listing_result.bank_revision);
	remove_listing.items[0] = { 903, 1, 1903 };
	remove_listing.end_time = static_cast<uint64_t>(time(nullptr)) + 3600;
	critical_command remove_listing_command = command(remove_listing, 30);
	require(auction_listing_accounting_intent(
			remove_listing_command, operation(75), typed_wallet, typed_bank,
			&remove_listing_command.accounting_intent) == economic_accounting_error::ok,
		"could not freeze removal listing");
	remove_listing_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path,
								      remove_listing_command);
	const auto remove_listing_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			remove_listing_result.auction_id == remove_auction_id,
		"could not list removal fixture");
	expect_event(typed_path, auction_event_type::listed, remove_auction_id, &error);
	const auto remove_escrow = escrow_account(typed_path, remove_auction_id, &error);
	auction_bid_accounting_listing remove_bid_listing;
	remove_bid_listing.auction_id = remove_auction_id;
	remove_bid_listing.seller_pid = 42;
	remove_bid_listing.status = 1;
	remove_bid_listing.custody_state = 1;
	remove_bid_listing.current_price = 1000;
	remove_bid_listing.revision = 1;
	remove_bid_listing.listing_operation = remove_listing_command.operation_id;
	auction_bid_accounting_accounts remove_bid_accounts;
	remove_bid_accounts.wallet = buyer_wallet;
	remove_bid_accounts.bank = buyer_bank;
	remove_bid_accounts.escrow = remove_escrow;
	remove_bid_accounts.bidder_claim = buyer_claim;
	auction_command_payload remove_bid_payload = outbid;
	actor(&remove_bid_payload, 44, "buyer-account", "Buyer", typed_buy_result.wallet_revision,
	      typed_buy_result.bank_revision);
	remove_bid_payload.auction_id = remove_auction_id;
	remove_bid_payload.value = 2000;
	const auto remove_bid_command =
		accounted_bid(remove_bid_payload, 31, remove_bid_listing, remove_bid_accounts);
	applied = flatfile_auction_repository_apply_accounted_bid(typed_path, remove_bid_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).wallet_value_delta == -2000,
		"could not stage removal bid");
	expect_event(typed_path, auction_event_type::bid_placed, remove_auction_id, &error);
	auction_settlement_listing remove_state;
	remove_state.auction_id = remove_auction_id;
	remove_state.seller_pid = 42;
	remove_state.winner_pid = 44;
	remove_state.status = 1;
	remove_state.custody_state = 1;
	remove_state.quantity = 1;
	remove_state.current_price = 2000;
	remove_state.revision = 2;
	remove_state.end_time = remove_listing.end_time;
	remove_state.listing_operation = remove_listing_command.operation_id;
	remove_state.winning_bid_operation = remove_bid_command.operation_id;
	remove_state.item_count = 1;
	remove_state.items[0] = { 903, 2, 0, 1903, 0, false };
	auction_settlement_accounts remove_accounts;
	remove_accounts.escrow = remove_escrow;
	remove_accounts.actor_wallet = typed_wallet;
	remove_accounts.actor_bank = typed_bank;
	auction_command_payload remove_payload = {};
	remove_payload.action = auction_action::remove;
	remove_payload.auction_id = remove_auction_id;
	actor(&remove_payload, 42, "seller-account", "Seller",
	      remove_listing_result.wallet_revision, remove_listing_result.bank_revision);
	const auto remove_command =
		accounted_settlement(remove_payload, 32, remove_state, remove_accounts);
	applied = flatfile_auction_repository_apply_accounted_remove(typed_path, remove_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::removed,
		"accounted trusted removal did not apply");
	const auto remove_plan = recorded_plan(typed_path, remove_command, &error);
	require(remove_plan.accounts.size() == 1 && remove_plan.postings.empty() &&
			remove_plan.accounts[0].before == (economic_coin_vector{ 2000, 0, 0, 0 }) &&
			remove_plan.accounts[0].after == (economic_coin_vector{ 2000, 0, 0, 0 }) &&
			remove_plan.items_before.size() == 1 &&
			remove_plan.items_after.size() == 1 && remove_plan.item_events.empty() &&
			remove_plan.metadata.source_event &&
			remove_plan.metadata.source_event->source.bytes ==
				remove_bid_command.operation_id.bytes,
		"accounted removal changed held escrow or lost custody witness");
	require(flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 7200 &&
			typed_seller_pickup.money_revision == 2 &&
			typed_seller_pickup.has_item_claim &&
			typed_seller_pickup.item_claim.items[0].item_uid == 903 &&
			flatfile_auction_find_pickup(typed_path, 44, &buyer_pickup, &error) ==
				flatfile_auction_query_result::ok &&
			buyer_pickup.money == 0 &&
			flatfile_auction_repository_apply_accounted_remove(typed_path,
									   remove_command)
					.outcome == critical_apply_outcome::already_applied,
		"accounted removal changed claim balance or failed replay");
	expect_escrow_lifetime(typed_path, remove_auction_id, remove_escrow, {}, &error);
	expect_event(typed_path, auction_event_type::removed, remove_auction_id, &error);
	const uint32_t expire_auction_id = auction_id_for(operation(33));
	initialize_auction_bucket(typed_path, expire_auction_id, 94, &error);
	auction_command_payload expire_listing = remove_listing;
	actor(&expire_listing, 42, "seller-account", "Seller",
	      remove_listing_result.wallet_revision, remove_listing_result.bank_revision);
	expire_listing.items[0] = { 904, 1, 1904 };
	expire_listing.end_time = 1;
	critical_command expire_listing_command = command(expire_listing, 33);
	require(auction_listing_accounting_intent(
			expire_listing_command, operation(75), typed_wallet, typed_bank,
			&expire_listing_command.accounting_intent) == economic_accounting_error::ok,
		"could not freeze no-bid expiry listing");
	expire_listing_command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	applied = flatfile_auction_repository_apply_accounted_listing(typed_path,
								      expire_listing_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).auction_id == expire_auction_id,
		"could not list no-bid expiry fixture");
	expect_event(typed_path, auction_event_type::listed, expire_auction_id, &error);
	auction_settlement_listing expire_state;
	expire_state.auction_id = expire_auction_id;
	expire_state.seller_pid = 42;
	expire_state.status = 1;
	expire_state.custody_state = 1;
	expire_state.quantity = 1;
	expire_state.current_price = 1000;
	expire_state.revision = 1;
	expire_state.end_time = 1;
	expire_state.listing_operation = expire_listing_command.operation_id;
	expire_state.item_count = 1;
	expire_state.items[0] = { 904, 2, 0, 1904, 0, false };
	auction_settlement_accounts expire_accounts;
	expire_accounts.escrow = escrow_account(typed_path, expire_auction_id, &error);
	auction_command_payload expire_payload = {};
	expire_payload.action = auction_action::finalize;
	expire_payload.auction_id = expire_auction_id;
	auto stale_expire = expire_state;
	stale_expire.revision = 2;
	const auto stale_expire_command =
		accounted_settlement(expire_payload, 35, stale_expire, expire_accounts);
	applied = flatfile_auction_repository_apply_accounted_finalize(typed_path,
								       stale_expire_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"accounted expiry accepted a stale listing revision");
	const auto expire_command =
		accounted_settlement(expire_payload, 34, expire_state, expire_accounts);
	applied = flatfile_auction_repository_apply_accounted_finalize(typed_path, expire_command);
	require(applied.outcome == critical_apply_outcome::applied &&
			result_of(applied).event_type == auction_event_type::expired,
		"accounted no-bid expiry did not apply");
	const auto expire_plan = recorded_plan(typed_path, expire_command, &error);
	require(expire_plan.accounts.size() == 1 && expire_plan.postings.empty() &&
			expire_plan.accounts[0].before == economic_coin_vector{} &&
			expire_plan.accounts[0].after == economic_coin_vector{} &&
			expire_plan.items_before.size() == 1 &&
			expire_plan.items_after.size() == 1 && expire_plan.item_events.empty() &&
			expire_plan.items_before[0].uid == 904 &&
			expire_plan.metadata.source_event &&
			expire_plan.metadata.source_event->source.bytes ==
				expire_listing_command.operation_id.bytes &&
			flatfile_auction_repository_apply_accounted_finalize(typed_path,
									     expire_command)
					.outcome == critical_apply_outcome::already_applied,
		"accounted no-bid expiry lost source or custody evidence");
	expect_escrow_lifetime(typed_path, expire_auction_id, expire_accounts.escrow,
			       expire_command.operation_id, &error);
	expect_event(typed_path, auction_event_type::expired, expire_auction_id, &error);
	const fs::path source_path = typed_domains / "auction_claim_sources";
	auto source_bytes = source_catalog_bytes(source_path);
	const auto number_at = [&](size_t offset, size_t width)
	{
		uint64_t value = 0;
		for (size_t index = 0; index < width; ++index)
			value |= static_cast<uint64_t>(source_bytes[offset + index]) << (index * 8);
		return value;
	};
	require(number_at(8, 4) == 1 && number_at(56, 4) == 3 && source_bytes.size() == 60 + 3 * 70,
		"accounted auction credits did not retain three exact source rows");
	const auto check_source = [&](size_t index, const critical_operation_id &source,
				      uint16_t slot, uint32_t pid,
				      const economic_account_key &claim, uint64_t amount,
				      const critical_operation_id &consumer)
	{
		const size_t offset = 60 + index * 70;
		require(std::equal(source.bytes.begin(), source.bytes.end(),
				   source_bytes.begin() + offset) &&
				number_at(offset + 16, 2) == slot &&
				std::equal(claim.lineage.bytes.begin(), claim.lineage.bytes.end(),
					   source_bytes.begin() + offset + 18) &&
				number_at(offset + 34, 4) == pid &&
				number_at(offset + 38, 8) == claim.authority_id &&
				number_at(offset + 46, 8) == amount &&
				std::equal(consumer.bytes.begin(), consumer.bytes.end(),
					   source_bytes.begin() + offset + 54),
			"accounted auction source row lost identity, value, or consumption");
	};
	check_source(0, outbid_command.operation_id, 1, 43, bidder_claim, 3000,
		     sale_bid_command.operation_id);
	check_source(1, buy_command.operation_id, 2, 42, seller_claim, 4500, {});
	check_source(2, sale_command.operation_id, 2, 42, seller_claim, 2700, {});
	auction_money_claim_state seller_state;
	seller_state.beneficiary_pid = 42;
	seller_state.money = 7200;
	seller_state.revision = 2;
	seller_state.sources = {
		{ buy_command.operation_id, 2, 42, seller_claim.authority_id, 4500 },
		{ sale_command.operation_id, 2, 42, seller_claim.authority_id, 2700 }
	};
	require(flatfile_player_domain_load(typed_path, 42, "seller-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok,
		"could not load seller wallet before accounted collection");
	auction_command_payload seller_claim_payload = {};
	seller_claim_payload.action = auction_action::claim_money;
	actor(&seller_claim_payload, 42, "seller-account", "Seller",
	      loaded_player.domains.wallet_revision, loaded_player.domains.bank_revision);
	auto stale_seller_state = seller_state;
	stale_seller_state.money = 4500;
	stale_seller_state.revision = 1;
	stale_seller_state.sources.pop_back();
	const auto stale_seller_claim = accounted_money_claim(seller_claim_payload, 38,
							      typed_wallet, typed_bank,
							      seller_claim, stale_seller_state);
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  stale_seller_claim);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE &&
			flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup,
						     &error) == flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 7200,
		"accounted auction collection accepted a stale source set");
	const auto seller_claim_command = accounted_money_claim(
		seller_claim_payload, 37, typed_wallet, typed_bank, seller_claim, seller_state);
	{
		std::fstream file(source_path, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open source catalog for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x24;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  seller_claim_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EILSEQ,
		"accounted auction collection accepted a corrupt source catalog");
	{
		std::ofstream file(source_path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char *>(source_bytes.data()),
			   source_bytes.size());
	}
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  seller_claim_command);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(typed_domains / ".critical-authority-transaction"),
		"accounted auction collection did not retain its interrupted journal");
	const auto seller_claim_plan = recorded_plan(typed_path, seller_claim_command, &error);
	require(seller_claim_plan.accounts.size() == 2 && seller_claim_plan.postings.size() == 2 &&
			seller_claim_plan.postings[0].copper == 7200 &&
			seller_claim_plan.postings[1].copper == -7200 &&
			seller_claim_plan.metadata.original_operation_id.bytes ==
				buy_command.operation_id.bytes,
		"recovered auction proceeds collection lost balanced source evidence");
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  seller_claim_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup,
						     &error) == flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 0 && typed_seller_pickup.money_revision == 3,
		"recovered auction proceeds collection did not replay");
	const auto claimed_source_bytes = source_catalog_bytes(source_path);
	{
		std::fstream file(source_path, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open consumed sources for replay corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x24;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  seller_claim_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EILSEQ,
		"accounted auction claim replay accepted a corrupt source catalog");
	{
		std::ofstream file(source_path, std::ios::binary | std::ios::trunc);
		file.write(reinterpret_cast<const char *>(claimed_source_bytes.data()),
			   claimed_source_bytes.size());
	}
	applied = flatfile_auction_repository_apply_accounted_money_claim(typed_path,
									  seller_claim_command);
	const auto seller_claim_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			seller_claim_result.wallet_value_delta == 7200 &&
			flatfile_auction_find_pickup(typed_path, 42, &typed_seller_pickup,
						     &error) == flatfile_auction_query_result::ok &&
			typed_seller_pickup.money == 0 && typed_seller_pickup.money_revision == 3 &&
			flatfile_auction_repository_apply_accounted_money_claim(
				typed_path, seller_claim_command)
					.outcome == critical_apply_outcome::already_applied,
		"accounted auction proceeds collection did not consume both sources");
	source_bytes = source_catalog_bytes(source_path);
	check_source(0, outbid_command.operation_id, 1, 43, bidder_claim, 3000,
		     sale_bid_command.operation_id);
	check_source(1, buy_command.operation_id, 2, 42, seller_claim, 4500,
		     seller_claim_command.operation_id);
	check_source(2, sale_command.operation_id, 2, 42, seller_claim, 2700,
		     seller_claim_command.operation_id);
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
