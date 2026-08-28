#include "flatfile_item_repository.h"

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static critical_operation_id operation(uint8_t discriminator)
{
	critical_operation_id id = {};
	id.bytes[0] = 0xa5;
	id.bytes.back() = discriminator;
	return id;
}

static critical_command creation(uint8_t discriminator, int64_t reason_id = 7)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = reason_id;
	payload.expected_from_revision = 0;
	payload.expected_to_revision = 0;
	payload.selected_item_uid = 100;
	payload.target_root_item_uid = 100;
	payload.item_count = 2;
	payload.items[0] = { 100, 100,
			     0,	  ITEM_TRANSFER_ABSENT_REVISION,
			     500, item_custody_state::absent };
	payload.items[1] = { 101, 100,
			     100, ITEM_TRANSFER_ABSENT_REVISION,
			     501, item_custody_state::absent };
	critical_command command = {};
	require(item_transfer_command_build(&command, operation(discriminator), payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build creation command");
	command.accepted_at_usec = 1;
	return command;
}

static critical_command single_creation(uint8_t discriminator, uint64_t item_uid,
					uint64_t player_pid, uint64_t system_revision)
{
	item_transfer_payload payload = {};
	payload.from_owner = { item_owner_type::system, 0, 0 };
	payload.to_owner = { item_owner_type::player, player_pid, 0 };
	payload.reason = item_transfer_reason::creation;
	payload.reason_id = 11;
	payload.expected_from_revision = system_revision;
	payload.expected_to_revision = 0;
	payload.selected_item_uid = item_uid;
	payload.target_root_item_uid = item_uid;
	payload.item_count = 1;
	payload.items[0] = { item_uid, item_uid,
			     0,	       ITEM_TRANSFER_ABSENT_REVISION,
			     600,      item_custody_state::absent };
	critical_command command = {};
	require(item_transfer_command_build(&command, operation(discriminator), payload,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build single-item creation command");
	command.accepted_at_usec = 4;
	return command;
}

static item_transfer_result result_of(const critical_apply_result &applied)
{
	item_transfer_result result = {};
	require(item_transfer_command_decode_result(applied.result_payload.data(),
						    applied.result_size, &result),
		"could not decode item repository result");
	return result;
}

static shop_trade_payload shop_trade(shop_trade_action action, uint64_t item_uid,
				     uint64_t item_revision, int32_t vnum, uint64_t stock_uid = 0,
				     uint64_t stock_revision = 0)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 42;
	payload.shop_id = 0;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "ShopTester");
	payload.price = 100;
	payload.expected_wallet_revision = 1;
	payload.expected_bank_revision = 1;
	payload.expected_shop_revision = 1;
	payload.selected_item_uid = item_uid;
	payload.stock_item_uid = stock_uid;
	payload.expected_stock_item_revision = stock_revision;
	payload.stock_vnum = stock_uid ? vnum : 0;
	payload.item_count = 1;
	payload.items[0] = { item_uid,
			     item_uid,
			     0,
			     item_revision,
			     vnum,
			     action == shop_trade_action::buy_produced ?
				     item_custody_state::absent :
				     item_custody_state::active };
	payload.item_blob_size = 1;
	payload.item_blob[0] = 0xa5;
	return payload;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity baseline_owner = { item_owner_type::player, 999, 0 };
	std::vector<flatfile_item_ownership_record> baseline = {
		{ 300, 300, 0, baseline_owner, 1, 700, item_custody_state::active },
		{ 301, 300, 300, baseline_owner, 1, 701, item_custody_state::active },
	};
	std::string error;
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"owner baseline did not apply: " + error);
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::already_applied,
		"owner baseline retry was not idempotent");
	baseline[1].vnum = 702;
	require(flatfile_item_repository_establish_owner(root.string(), baseline_owner, baseline,
							 &error) ==
			flatfile_item_baseline_result::conflict,
		"conflicting owner baseline was accepted");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_item_repository_prepare_player_remove(
					root.string(), lock, 999, &operation, &error) ==
					flatfile_item_repository_result::ok &&
				operation.filename == "item_ownership" && !operation.bytes.empty(),
			"player item destruction was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"prepared player item destruction did not commit: " + error);
	}
	uint64_t removed_revision = 0;
	std::vector<flatfile_item_ownership_record> removed_items;
	require(flatfile_item_repository_load_owner(root.string(), baseline_owner,
						    &removed_revision, &removed_items, &error) ==
			flatfile_item_repository_result::not_found,
		"deleted player item owner remained authoritative");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_item_repository_prepare_player_remove(
					root.string(), lock, 999, &operation, &error) ==
					flatfile_item_repository_result::unchanged,
			"prepared player item destruction was not idempotent");
	}

	const critical_command create = creation(1);
	critical_apply_result applied = flatfile_item_repository_apply(root.string(), create);
	require(applied.outcome == critical_apply_outcome::applied && applied.error_code == 0,
		"item creation did not apply: outcome=" +
			std::to_string(static_cast<unsigned int>(applied.outcome)) +
			" error=" + std::to_string(applied.error_code));
	item_transfer_result result = result_of(applied);
	require(result.root_item_uid == 100 && result.item_count == 2 &&
			result.from_owner_revision == 1 && result.to_owner_revision == 1 &&
			result.max_item_revision == 1,
		"item creation returned incorrect revisions");

	uint64_t owner_revision = 0;
	std::vector<flatfile_item_ownership_record> items;
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 42, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 2 && items[0].item_uid == 100 &&
			items[0].parent_item_uid == 0 && items[1].parent_item_uid == 100 &&
			items[1].item_revision == 1,
		"created ownership topology did not round trip: " + error);

	applied = flatfile_item_repository_apply(root.string(), create);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			result_of(applied).to_owner_revision == 1,
		"replayed creation was not idempotent");
	applied = flatfile_item_repository_apply(root.string(), creation(1, 8));
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EEXIST,
		"operation ID reuse with different content was accepted");

	item_transfer_payload move = {};
	move.from_owner = { item_owner_type::player, 42, 0 };
	move.to_owner = { item_owner_type::player, 77, 0 };
	move.reason = item_transfer_reason::player_give;
	move.reason_id = 9;
	move.expected_from_revision = 1;
	move.expected_to_revision = 0;
	move.selected_item_uid = 100;
	move.target_root_item_uid = 100;
	move.item_count = 2;
	for (size_t index = 0; index < items.size(); ++index)
		move.items[index] = { items[index].item_uid,
				      items[index].root_item_uid,
				      items[index].parent_item_uid,
				      items[index].item_revision,
				      items[index].vnum,
				      items[index].state };
	critical_command transfer = {};
	require(item_transfer_command_build(&transfer, operation(2), move,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build transfer command");
	transfer.accepted_at_usec = 2;
	applied = flatfile_item_repository_apply(root.string(), transfer);
	result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			result.from_owner_revision == 2 && result.to_owner_revision == 1 &&
			result.max_item_revision == 2,
		"cross-owner transfer did not apply");
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 2 && items[0].item_revision == 2,
		"destination owner did not receive the complete topology");

	move.expected_from_revision = 1;
	move.expected_to_revision = 0;
	critical_command stale = {};
	require(item_transfer_command_build(&stale, operation(3), move,
					    critical_source_site::command,
					    critical_deadline_class::interactive),
		"could not build stale transfer command");
	stale.accepted_at_usec = 3;
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale owner revisions were accepted");
	applied = flatfile_item_repository_apply(root.string(), stale);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE,
		"stale rejection was not durably replayable");

	const critical_command concurrent = single_creation(4, 200, 88, 1);
	for (int child = 0; child < 2; ++child)
	{
		const pid_t pid = fork();
		require(pid >= 0, "ownership writer fork failed");
		if (!pid)
		{
			const critical_apply_result child_result =
				flatfile_item_repository_apply(root.string(), concurrent);
			_exit(child_result.outcome == critical_apply_outcome::applied ||
					      child_result.outcome ==
						      critical_apply_outcome::already_applied ?
				      0 :
				      2);
		}
	}
	for (int child = 0; child < 2; ++child)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent idempotent ownership writer failed");
	}
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 88, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::ok &&
			owner_revision == 1 && items.size() == 1 && items[0].item_uid == 200,
		"concurrent replay duplicated or lost item creation");

	const fs::path shop_root = fs::path(argv[1]).string() + "-shop";
	fs::create_directories(shop_root / "domains");
	fs::permissions(shop_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(shop_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity shop_player = { item_owner_type::player, 42, 0 };
	const item_owner_identity shop_owner = { item_owner_type::shopkeeper,
						 item_shopkeeper_owner_id(0), 0 };
	require(flatfile_item_repository_establish_owner(
			shop_root.string(), shop_player,
			{ { 100, 100, 0, shop_player, 1, 700, item_custody_state::active },
			  { 101, 101, 0, shop_player, 1, 701, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"shop player custody baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(
			shop_root.string(), shop_owner,
			{ { 200, 200, 0, shop_owner, 1, 800, item_custody_state::active },
			  { 201, 201, 0, shop_owner, 1, 801, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"shop inventory custody baseline failed: " + error);
	flatfile_item_shop_trade_mutation shop_mutation;
	unsigned int shop_result_code = 0;
	auto produced = shop_trade(shop_trade_action::buy_produced, 300,
				   ITEM_TRANSFER_ABSENT_REVISION, 801, 201, 1);
	{
		auto stale_stock = produced;
		stale_stock.expected_stock_item_revision = 2;
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire stale stock lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, stale_stock, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == ESTALE &&
				shop_mutation.after_image.bytes.empty(),
			"stale produced-stock exemplar was accepted");
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire produced lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, produced, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 2 &&
				shop_mutation.shop_owner_revision == 1 &&
				shop_mutation.item_revisions[0] == 1,
			"produced custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"produced custody mutation did not commit: " + error);
	}
	auto purchase = shop_trade(shop_trade_action::buy_existing, 200, 1, 800, 200, 1);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire purchase lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, purchase, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 3 &&
				shop_mutation.shop_owner_revision == 2 &&
				shop_mutation.item_revisions[0] == 2,
			"purchase custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"purchase custody mutation did not commit: " + error);
	}
	auto sale = shop_trade(shop_trade_action::sell_store, 100, 1, 700);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error), "could not acquire sale lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, sale, &shop_mutation, &shop_result_code,
				&error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 4 &&
				shop_mutation.shop_owner_revision == 3 &&
				shop_mutation.item_revisions[0] == 2,
			"sale custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"sale custody mutation did not commit: " + error);
	}
	auto destruction = shop_trade(shop_trade_action::sell_destroy, 101, 1, 701);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(shop_root.string(), &error),
			"could not acquire destruction lock");
		require(flatfile_item_repository_prepare_shop_trade(
				shop_root.string(), lock, destruction, &shop_mutation,
				&shop_result_code, &error) == flatfile_item_repository_result::ok &&
				shop_result_code == 0 && shop_mutation.player_owner_revision == 5 &&
				shop_mutation.shop_owner_revision == 3 &&
				shop_mutation.item_revisions[0] == 2,
			"destruction custody mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(
				shop_root.string(), lock, { shop_mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"destruction custody mutation did not commit: " + error);
	}
	items.clear();
	require(flatfile_item_repository_load_owner(shop_root.string(), shop_player,
						    &owner_revision, &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 5 && items.size() == 2 && items[0].item_uid == 200 &&
			items[1].item_uid == 300,
		"shop trades did not publish exact player custody");
	items.clear();
	require(flatfile_item_repository_load_owner(shop_root.string(), shop_owner, &owner_revision,
						    &items, &error) ==
				flatfile_item_repository_result::ok &&
			owner_revision == 3 && items.size() == 2 && items[0].item_uid == 100 &&
			items[1].item_uid == 201,
		"shop trades did not preserve exact shop custody");

	const fs::path combined_root = fs::path(argv[1]).string() + "-combined";
	fs::create_directories(combined_root / "domains");
	fs::permissions(combined_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(combined_root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	const item_owner_identity combined_player = { item_owner_type::player, 42, 0 };
	const item_owner_identity locker_public = { item_owner_type::locker, 2, 11 };
	const item_owner_identity locker_private = { item_owner_type::locker, 2, 12 };
	require(flatfile_item_repository_establish_owner(
			combined_root.string(), combined_player,
			{ { 400, 400, 0, combined_player, 1, 800, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"combined player owner baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(
			combined_root.string(), locker_public,
			{ { 500, 500, 0, locker_public, 1, 900, item_custody_state::active },
			  { 501, 500, 500, locker_public, 1, 901, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"combined public locker owner baseline failed: " + error);
	require(flatfile_item_repository_establish_owner(combined_root.string(), locker_private, {},
							 &error) ==
			flatfile_item_baseline_result::applied,
		"combined empty private locker owner baseline failed: " + error);
	std::vector<flatfile_locker_custody_owner> custody = {
		{ locker_public, { { 500, 900 }, { 501, 901 } } }, { locker_private, {} }
	};
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation prepared;
		auto mismatch = custody;
		mismatch[0].items[1].vnum = 999;
		require(lock.acquire(combined_root.string(), &error) &&
				flatfile_item_repository_prepare_player_and_locker_remove(
					combined_root.string(), lock, 42, mismatch, &prepared,
					&error) == flatfile_item_repository_result::invalid,
			"locker payload/custody mismatch was accepted");
		require(flatfile_item_repository_prepare_player_and_locker_remove(
				combined_root.string(), lock, 42, custody, &prepared, &error) ==
					flatfile_item_repository_result::ok &&
				prepared.filename == "item_ownership",
			"combined player/locker removal was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(
				combined_root.string(), lock, { prepared }, &error) ==
				flatfile_authority_transaction_result::ok,
			"combined player/locker removal did not commit: " + error);
	}
	for (const auto &owner : { combined_player, locker_public, locker_private })
		require(flatfile_item_repository_load_owner(combined_root.string(), owner,
							    &owner_revision, &items, &error) ==
				flatfile_item_repository_result::not_found,
			"combined removal left an item owner authoritative");

	const fs::path authority = domains / "item_ownership";
	{
		std::fstream file(authority, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open ownership authority for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5c;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	items.clear();
	require(flatfile_item_repository_load_owner(
			root.string(), { item_owner_type::player, 77, 0 }, &owner_revision, &items,
			&error) == flatfile_item_repository_result::invalid,
		"corrupt ownership checksum was accepted");
	applied = flatfile_item_repository_apply(root.string(), transfer);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == EILSEQ,
		"corrupt ownership authority was overwritten");
	for (const fs::directory_entry &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary ownership file was left behind");

	std::cout << "flat-file item ownership repository passed\n";
	return 0;
}
