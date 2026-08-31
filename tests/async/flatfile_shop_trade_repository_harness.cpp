#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_shop_trade_repository.h"
#include "flatfile/flatfile_shop_trade_materialization.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "player_snapshot_codec.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
	id.bytes[0] = 0x73;
	id.bytes.back() = value;
	return id;
}

static player_item_snapshot shop_item()
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = 200;
	item.generated_key = 1200;
	item.vnum = 800;
	item.name = "trade test item";
	item.short_description = "a trade test item";
	return item;
}

static player_item_snapshot shop_child()
{
	player_item_snapshot item = {};
	item.parent_index = 0;
	item.object_uid = 201;
	item.generated_key = 1201;
	item.vnum = 801;
	item.name = "nested trade test item";
	item.short_description = "a nested trade test item";
	return item;
}

static shop_trade_payload purchase(const std::vector<player_item_snapshot> &items)
{
	require(!items.empty(), "shop purchase needs an item tree");
	shop_trade_payload payload = {};
	payload.action = shop_trade_action::buy_existing;
	payload.player_pid = 42;
	payload.shop_id = 0;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "shop-account");
	payload.price = 100;
	payload.expected_wallet_revision = 0;
	payload.expected_bank_revision = 1;
	payload.expected_shop_revision = 1;
	payload.selected_item_uid = items.front().object_uid;
	payload.target_root_item_uid = items.front().object_uid;
	payload.stock_item_uid = items.front().object_uid;
	payload.expected_stock_item_revision = 1;
	payload.stock_vnum = items.front().vnum;
	payload.item_count = static_cast<uint16_t>(items.size());
	for (size_t index = 0; index < items.size(); ++index)
	{
		const uint64_t parent_uid =
			items[index].parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
				0 :
				items[static_cast<size_t>(items[index].parent_index)].object_uid;
		payload.items[index] = {
			items[index].object_uid, items.front().object_uid,  parent_uid, 1,
			items[index].vnum,	 item_custody_state::active
		};
	}
	std::vector<uint8_t> blob;
	require(player_item_snapshot_list_encode(items, &blob) == player_snapshot_codec_result::ok,
		"could not encode shop purchase item");
	require(blob.size() <= payload.item_blob.size(), "shop purchase item exceeded payload");
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	return payload;
}

static critical_command command(const shop_trade_payload &payload, uint8_t operation_value)
{
	critical_command value = {};
	require(shop_trade_command_build(&value, operation(operation_value), payload,
					 critical_source_site::command,
					 critical_deadline_class::interactive),
		"could not build shop trade command");
	value.accepted_at_usec = operation_value;
	require(critical_command_normalize(&value), "could not normalize shop trade command");
	return value;
}

static shop_trade_result result_of(const critical_apply_result &applied)
{
	shop_trade_result result = {};
	require(shop_trade_command_decode_result(applied.result_payload.data(), applied.result_size,
						 &result),
		"could not decode shop trade result");
	return result;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	{
		flatfile_authority_lock health_lock;
		flatfile_shop_trade_materialization_health empty_health = {};
		require(health_lock.acquire(root.string(), &error),
			"could not lock empty materialization health read: " + error);
		require(flatfile_shop_trade_materialization_read_health(root.string(), health_lock,
									&empty_health, &error) ==
					flatfile_shop_trade_materialization_result::ok &&
				empty_health.revision == 0 && empty_health.events == 0 &&
				empty_health.encoded_bytes == 0 &&
				empty_health.maximum_events > 0 && empty_health.maximum_bytes > 0 &&
				!empty_health.near_capacity,
			"empty materialization health was not available: " + error);
	}

	flatfile_player_domain_record player = {};
	player.pid = 42;
	player.account_name = "shop-account";
	player.racewar = 1;
	player.domains.wallet = { 0, 0, 0, 10 };
	require(flatfile_player_domain_establish(root.string(), player, &error) ==
			flatfile_player_domain_result::ok,
		"could not establish shop player: " + error);

	const player_item_snapshot item = shop_item();
	const player_item_snapshot child = shop_child();
	const std::vector<player_item_snapshot> trade_items = { item, child };
	flatfile_shopkeeper_record shop = {};
	shop.mob_vnum = 1000;
	shop.room_vnum = 2000;
	shop.saved_at = 1;
	shop.revision = 1;
	shop.items = trade_items;
	require(flatfile_shopkeeper_establish(root.string(), { shop }, &error) ==
			flatfile_shopkeeper_result::ok,
		"could not establish shopkeeper: " + error);

	const item_owner_identity player_owner = { item_owner_type::player, 42, 0 };
	const item_owner_identity shop_owner = { item_owner_type::shopkeeper,
						 item_shopkeeper_owner_id(0), 0 };
	player_item_snapshot container = {};
	container.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	container.object_uid = 700;
	container.vnum = 1700;
	container.name = "purchase container";
	require(flatfile_item_repository_establish_owner(
			root.string(), player_owner,
			{ { 700, 700, 0, player_owner, 1, 1700, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish player custody: " + error);
	const std::vector<flatfile_item_ownership_record> shop_items = {
		{ 200, 200, 0, shop_owner, 1, 800, item_custody_state::active },
		{ 201, 200, 200, shop_owner, 1, 801, item_custody_state::active },
	};
	require(flatfile_item_repository_establish_owner(root.string(), shop_owner, shop_items,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish shop custody: " + error);

	const shop_trade_payload payload = purchase(trade_items);
	const critical_command purchase_command = command(payload, 1);
	const std::string root_path = root.string();
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	critical_apply_result applied = flatfile_critical_command_repository_apply_selected(
		purchase_command, const_cast<char *>(root_path.c_str()));
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".critical-authority-transaction"),
		"interrupted purchase did not preserve its cross-authority intent");

	flatfile_player_domain_record loaded_player;
	require(flatfile_player_domain_load(root.string(), 42, "shop-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet == std::array<uint64_t, 4>{ 0, 0, 9, 9 } &&
			loaded_player.domains.wallet_revision == 1 &&
			loaded_player.domains.bank_revision == 2 &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"player load did not recover interrupted purchase after-images");

	applied = flatfile_shop_trade_repository_apply(root.string(), purchase_command);
	const shop_trade_result purchased = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			purchased.wallet.amount == std::array<int64_t, 4>{ 0, 0, 9, 9 } &&
			purchased.wallet_revision == 1 && purchased.bank_revision == 2 &&
			purchased.shop_revision == 2 && purchased.player_owner_revision == 2 &&
			purchased.counterparty_owner_revision == 2 && purchased.item_count == 2 &&
			purchased.item_uids[0] == 200 && purchased.item_revisions[0] == 2 &&
			purchased.item_uids[1] == 201 && purchased.item_revisions[1] == 2,
		"recovered purchase did not replay its exact result");

	std::vector<flatfile_shopkeeper_record> shops;
	require(flatfile_shopkeeper_list(root.string(), &shops, &error) ==
				flatfile_shopkeeper_result::ok &&
			shops.size() == 1 && shops[0].revision == 2 && shops[0].items.empty(),
		"purchase did not remove the item from shopkeeper inventory");
	uint64_t player_revision = 0, shop_revision = 0;
	std::vector<flatfile_item_ownership_record> player_items, remaining_shop_items;
	require(flatfile_item_repository_load_owner(root.string(), player_owner, &player_revision,
						    &player_items, &error) ==
				flatfile_item_repository_result::ok &&
			flatfile_item_repository_load_owner(
				root.string(), shop_owner, &shop_revision, &remaining_shop_items,
				&error) == flatfile_item_repository_result::ok &&
			player_revision == 2 && shop_revision == 2 && player_items.size() == 3 &&
			player_items[0].item_uid == 200 && player_items[0].item_revision == 2 &&
			player_items[1].item_uid == 201 && player_items[1].parent_item_uid == 200 &&
			player_items[1].item_revision == 2 && player_items[2].item_uid == 700 &&
			remaining_shop_items.empty(),
		"purchase did not transfer global item custody");
	player_snapshot stale_snapshot = {};
	stale_snapshot.pid = 42;
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(root.string(), &error),
			"could not lock purchase reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, player_items,
				&stale_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_snapshot.items.size() == 2 &&
				stale_snapshot.items[0].object_uid == item.object_uid &&
				stale_snapshot.items[0].short_description ==
					item.short_description &&
				stale_snapshot.items[1].object_uid == child.object_uid &&
				stale_snapshot.items[1].parent_index == 0,
			"restart reconciliation did not materialize the committed purchase: " +
				error);
		stale_snapshot.items[0].cost = 777;
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, player_items,
				&stale_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_snapshot.items[0].cost == 777,
			"reconciliation overwrote a newer materialized object snapshot");
	}

	shop_trade_payload conflicting_payload = payload;
	conflicting_payload.price = 101;
	require(flatfile_shop_trade_repository_apply(root.string(), command(conflicting_payload, 1))
				.error_code == EEXIST,
		"operation ID reuse with a different command was accepted");

	const critical_command stale_command = command(payload, 2);
	applied = flatfile_shop_trade_repository_apply(root.string(), stale_command);
	require(applied.outcome == critical_apply_outcome::terminal_failure &&
			applied.error_code == ESTALE &&
			flatfile_shop_trade_repository_apply(root.string(), stale_command).outcome ==
				critical_apply_outcome::terminal_failure &&
			flatfile_shop_trade_repository_apply(root.string(), stale_command)
					.error_code == ESTALE,
		"stale purchase rejection was not durable and idempotent");

	require(flatfile_player_domain_load(root.string(), 42, "shop-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet == std::array<uint64_t, 4>{ 0, 0, 9, 9 },
		"durable rejection changed player money");

	shop_trade_payload sale = payload;
	sale.action = shop_trade_action::sell_store;
	sale.price = 50;
	sale.expected_wallet_revision = 1;
	sale.expected_bank_revision = 2;
	sale.expected_shop_revision = 2;
	sale.stock_item_uid = 0;
	sale.expected_stock_item_revision = 0;
	sale.stock_vnum = 0;
	for (size_t index = 0; index < sale.item_count; ++index)
		sale.items[index].expected_item_revision = 2;
	applied = flatfile_shop_trade_repository_apply(root.string(), command(sale, 3));
	require(applied.outcome == critical_apply_outcome::applied,
		"could not commit retained sale for restart reconciliation");
	player_items.clear();
	require(flatfile_item_repository_load_owner(root.string(), player_owner, &player_revision,
						    &player_items, &error) ==
				flatfile_item_repository_result::ok &&
			player_items.size() == 1 && player_items[0].item_uid == 700,
		"retained sale did not remove player custody");
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(root.string(), &error),
			"could not lock sale reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, player_items,
				&stale_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_snapshot.items.empty(),
			"restart reconciliation did not remove the committed sale: " + error);
	}

	player_item_snapshot produced_item = item;
	produced_item.object_uid = 300;
	produced_item.generated_key = 1300;
	shop_trade_payload produced = purchase({ produced_item });
	produced.action = shop_trade_action::buy_produced;
	produced.expected_wallet_revision = 2;
	produced.expected_bank_revision = 3;
	produced.expected_shop_revision = 3;
	produced.selected_item_uid = 300;
	produced.target_root_item_uid = 700;
	produced.target_parent_item_uid = 700;
	produced.expected_target_parent_revision = 1;
	produced.stock_item_uid = 200;
	produced.expected_stock_item_revision = 3;
	produced.items[0] = { 300, 300,
			      0,   ITEM_TRANSFER_ABSENT_REVISION,
			      800, item_custody_state::absent };
	applied = flatfile_shop_trade_repository_apply(root.string(), command(produced, 4));
	require(applied.outcome == critical_apply_outcome::applied,
		"could not commit produced container purchase");
	player_items.clear();
	require(flatfile_item_repository_load_owner(root.string(), player_owner, &player_revision,
						    &player_items, &error) ==
				flatfile_item_repository_result::ok &&
			player_items.size() == 2 && player_items[0].item_uid == 300 &&
			player_items[0].root_item_uid == 700 &&
			player_items[0].parent_item_uid == 700 && player_items[1].item_uid == 700,
		"produced purchase did not commit container custody");
	stale_snapshot.items = { container };
	{
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(root.string(), &error),
			"could not lock produced purchase reconciliation: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, player_items,
				&stale_snapshot,
				&error) == flatfile_shop_trade_materialization_result::ok &&
				stale_snapshot.items.size() == 2 &&
				stale_snapshot.items[0].object_uid == 700 &&
				stale_snapshot.items[1].object_uid == 300 &&
				stale_snapshot.items[1].parent_index == 0,
			"restart reconciliation did not restore produced container placement: " +
				error);
	}
	flatfile_shop_trade_materialization_health health_before = {};
	{
		flatfile_authority_lock health_lock;
		require(health_lock.acquire(root.string(), &error),
			"could not lock materialization health read: " + error);
		require(flatfile_shop_trade_materialization_read_health(root.string(), health_lock,
									&health_before, &error) ==
					flatfile_shop_trade_materialization_result::ok &&
				health_before.revision == 3 && health_before.events == 3 &&
				health_before.encoded_bytes > 0 &&
				health_before.maximum_events >= health_before.events &&
				health_before.maximum_bytes >= health_before.encoded_bytes &&
				!health_before.near_capacity,
			"materialization health did not report bounded catalog usage: " + error);
	}
	shop_trade_payload repurchase = payload;
	repurchase.expected_wallet_revision = 3;
	repurchase.expected_bank_revision = 4;
	repurchase.expected_shop_revision = 4;
	repurchase.expected_stock_item_revision = 3;
	for (size_t index = 0; index < repurchase.item_count; ++index)
		repurchase.items[index].expected_item_revision = 3;
	const critical_command repurchase_command = command(repurchase, 5);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_critical_command_repository_apply_selected(
		repurchase_command, const_cast<char *>(root_path.c_str()));
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".critical-authority-transaction"),
		"interrupted compaction did not preserve its cross-authority intent");
	applied = flatfile_shop_trade_repository_apply(root.string(), repurchase_command);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"compacted repurchase did not recover and replay exactly");
	{
		flatfile_authority_lock health_lock;
		flatfile_shop_trade_materialization_health compacted_health = {};
		require(health_lock.acquire(root.string(), &error),
			"could not lock compacted materialization health read: " + error);
		require(flatfile_shop_trade_materialization_read_health(
				root.string(), health_lock, &compacted_health, &error) ==
					flatfile_shop_trade_materialization_result::ok &&
				compacted_health.revision == 4 && compacted_health.events == 2 &&
				compacted_health.events < health_before.events &&
				compacted_health.reclaimable_events == 0,
			"materialization catalog did not compact superseded events: " + error);
	}
	shop_trade_payload resale = payload;
	resale.action = shop_trade_action::sell_store;
	resale.price = 50;
	resale.expected_wallet_revision = 4;
	resale.expected_bank_revision = 5;
	resale.expected_shop_revision = 5;
	resale.stock_item_uid = 0;
	resale.expected_stock_item_revision = 0;
	resale.stock_vnum = 0;
	for (size_t index = 0; index < resale.item_count; ++index)
		resale.items[index].expected_item_revision = 4;
	applied = flatfile_shop_trade_repository_apply(root.string(), command(resale, 6));
	require(applied.outcome == critical_apply_outcome::applied,
		"could not return repurchased stock for cleanup");
	shop_trade_payload cleanup = resale;
	cleanup.action = shop_trade_action::discard_invalid;
	cleanup.price = 0;
	cleanup.expected_wallet_revision = 5;
	cleanup.expected_bank_revision = 6;
	cleanup.expected_shop_revision = 6;
	cleanup.stock_item_uid = 200;
	cleanup.expected_stock_item_revision = 5;
	cleanup.stock_vnum = 800;
	for (size_t index = 0; index < cleanup.item_count; ++index)
		cleanup.items[index].expected_item_revision = 5;
	const critical_command cleanup_command = command(cleanup, 7);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
	applied = flatfile_critical_command_repository_apply_selected(
		cleanup_command, const_cast<char *>(root_path.c_str()));
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	require(applied.outcome == critical_apply_outcome::retryable_failure &&
			fs::exists(domains / ".critical-authority-transaction"),
		"interrupted invalid-stock cleanup did not retain its authority intent");
	applied = flatfile_shop_trade_repository_apply(root.string(), cleanup_command);
	const shop_trade_result cleaned = result_of(applied);
	require(applied.outcome == critical_apply_outcome::already_applied &&
			cleaned.action == shop_trade_action::discard_invalid &&
			cleaned.wallet_revision == 5 && cleaned.bank_revision == 6 &&
			cleaned.shop_revision == 7 && cleaned.player_owner_revision == 6 &&
			cleaned.counterparty_owner_revision == 1 && cleaned.item_count == 2 &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"invalid-stock cleanup did not recover and replay exactly");
	require(flatfile_player_domain_load(root.string(), 42, "shop-account", 1, &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet_revision == 5 &&
			loaded_player.domains.bank_revision == 6,
		"invalid-stock cleanup changed player money revisions");
	shops.clear();
	require(flatfile_shopkeeper_list(root.string(), &shops, &error) ==
				flatfile_shopkeeper_result::ok &&
			shops.size() == 1 && shops[0].revision == 7 && shops[0].items.empty(),
		"invalid-stock cleanup did not remove the durable shop subtree");
	player_items.clear();
	require(flatfile_item_repository_load_owner(root.string(), player_owner, &player_revision,
						    &player_items, &error) ==
				flatfile_item_repository_result::ok &&
			player_items.size() == 2 && player_items[0].item_uid == 300 &&
			player_items[1].item_uid == 700,
		"invalid-stock cleanup left destroyed rows in active player custody");
	const item_owner_identity destruction = { item_owner_type::destruction, 0, 0 };
	uint64_t destruction_revision = 0;
	std::vector<flatfile_item_ownership_record> destroyed_items;
	require(flatfile_item_repository_load_owner(
			root.string(), destruction, &destruction_revision, &destroyed_items,
			&error) == flatfile_item_repository_result::ok &&
			destruction_revision == 1 && destroyed_items.empty(),
		"invalid-stock cleanup did not commit destruction custody");
	{
		flatfile_authority_lock health_lock;
		flatfile_shop_trade_materialization_health cleanup_health = {};
		require(health_lock.acquire(root.string(), &error),
			"could not lock cleanup materialization health read: " + error);
		require(flatfile_shop_trade_materialization_read_health(root.string(), health_lock,
									&cleanup_health, &error) ==
					flatfile_shop_trade_materialization_result::ok &&
				cleanup_health.revision == 5 && cleanup_health.events == 3,
			"invalid-stock cleanup unexpectedly wrote player materialization evidence");
	}

	const fs::path complimentary_root = fs::path(argv[1]).string() + "-complimentary";
	fs::create_directories(complimentary_root / "domains");
	fs::permissions(complimentary_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(complimentary_root / "domains", fs::perms::owner_all,
			fs::perm_options::replace);
	flatfile_player_domain_record complimentary_player = player;
	require(flatfile_player_domain_establish(complimentary_root.string(), complimentary_player,
						 &error) == flatfile_player_domain_result::ok,
		"could not establish complimentary-purchase player: " + error);
	flatfile_shopkeeper_record complimentary_shop = shop;
	complimentary_shop.revision = 1;
	complimentary_shop.items = trade_items;
	require(flatfile_shopkeeper_establish(complimentary_root.string(), { complimentary_shop },
					      &error) == flatfile_shopkeeper_result::ok,
		"could not establish complimentary-purchase shop: " + error);
	require(flatfile_item_repository_establish_owner(complimentary_root.string(), player_owner,
							 {}, &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish empty complimentary player custody: " + error);
	require(flatfile_item_repository_establish_owner(
			complimentary_root.string(), shop_owner,
			{ { 200, 200, 0, shop_owner, 1, 800, item_custody_state::active },
			  { 201, 200, 200, shop_owner, 1, 801, item_custody_state::active } },
			&error) == flatfile_item_baseline_result::applied,
		"could not establish complimentary shop custody: " + error);
	shop_trade_payload complimentary_purchase = purchase(trade_items);
	complimentary_purchase.price = 0;
	applied = flatfile_shop_trade_repository_apply(complimentary_root.string(),
						       command(complimentary_purchase, 1));
	const shop_trade_result complimentary_result = result_of(applied);
	require(applied.outcome == critical_apply_outcome::applied &&
			complimentary_result.wallet.amount ==
				std::array<int64_t, 4>{ 0, 0, 0, 10 } &&
			complimentary_result.wallet_revision == 1 &&
			complimentary_result.bank_revision == 2,
		"complimentary purchase did not preserve player money");
	require(flatfile_player_domain_load(complimentary_root.string(), 42, "shop-account", 1,
					    &loaded_player,
					    &error) == flatfile_player_domain_result::ok &&
			loaded_player.domains.wallet == std::array<uint64_t, 4>{ 0, 0, 0, 10 },
		"complimentary purchase changed durable player money");
	player_items.clear();
	remaining_shop_items.clear();
	require(flatfile_item_repository_load_owner(complimentary_root.string(), player_owner,
						    &player_revision, &player_items, &error) ==
				flatfile_item_repository_result::ok &&
			flatfile_item_repository_load_owner(complimentary_root.string(), shop_owner,
							    &shop_revision, &remaining_shop_items,
							    &error) ==
				flatfile_item_repository_result::ok &&
			player_items.size() == 2 && remaining_shop_items.empty(),
		"complimentary purchase did not transfer durable item custody");
	{
		std::fstream catalog(domains / "shop_trade_materializations",
				     std::ios::binary | std::ios::in | std::ios::out);
		require(catalog.good(),
			"could not open materialization catalog for corruption test");
		catalog.seekg(-1, std::ios::end);
		char byte = 0;
		catalog.read(&byte, 1);
		byte ^= 0x5a;
		catalog.seekp(-1, std::ios::end);
		catalog.write(&byte, 1);
		catalog.close();
		flatfile_authority_lock reconciliation_lock;
		require(reconciliation_lock.acquire(root.string(), &error),
			"could not lock corrupt reconciliation test: " + error);
		require(flatfile_shop_trade_materialization_reconcile(
				root.string(), reconciliation_lock, 42, player_items,
				&stale_snapshot,
				&error) == flatfile_shop_trade_materialization_result::invalid,
			"corrupt materialization catalog was accepted");
		flatfile_shop_trade_materialization_health corrupt_health = {};
		error.clear();
		require(flatfile_shop_trade_materialization_read_health(
				root.string(), reconciliation_lock, &corrupt_health, &error) ==
				flatfile_shop_trade_materialization_result::invalid,
			"corrupt materialization catalog reported healthy capacity");
	}
	std::cout << "flat-file shop trade repository passed\n";
}
