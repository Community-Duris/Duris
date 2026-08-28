#include "flatfile_item_repository.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_shop_trade_repository.h"
#include "flatfile_shopkeeper_repository.h"
#include "player_snapshot_codec.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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

static shop_trade_payload purchase(const player_item_snapshot &item)
{
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
	payload.selected_item_uid = item.object_uid;
	payload.stock_item_uid = item.object_uid;
	payload.expected_stock_item_revision = 1;
	payload.stock_vnum = item.vnum;
	payload.item_count = 1;
	payload.items[0] = { item.object_uid, item.object_uid,		 0, 1,
			     item.vnum,	      item_custody_state::active };
	std::vector<uint8_t> blob;
	require(player_item_snapshot_list_encode({ item }, &blob) ==
			player_snapshot_codec_result::ok,
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

	flatfile_player_domain_record player = {};
	player.pid = 42;
	player.account_name = "shop-account";
	player.racewar = 1;
	player.domains.wallet = { 0, 0, 0, 10 };
	require(flatfile_player_domain_establish(root.string(), player, &error) ==
			flatfile_player_domain_result::ok,
		"could not establish shop player: " + error);

	const player_item_snapshot item = shop_item();
	flatfile_shopkeeper_record shop = {};
	shop.mob_vnum = 1000;
	shop.room_vnum = 2000;
	shop.saved_at = 1;
	shop.revision = 1;
	shop.items = { item };
	require(flatfile_shopkeeper_establish(root.string(), { shop }, &error) ==
			flatfile_shopkeeper_result::ok,
		"could not establish shopkeeper: " + error);

	const item_owner_identity player_owner = { item_owner_type::player, 42, 0 };
	const item_owner_identity shop_owner = { item_owner_type::shopkeeper,
						 item_shopkeeper_owner_id(0), 0 };
	require(flatfile_item_repository_establish_owner(root.string(), player_owner, {}, &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish player custody: " + error);
	const std::vector<flatfile_item_ownership_record> shop_items = {
		{ 200, 200, 0, shop_owner, 1, 800, item_custody_state::active },
	};
	require(flatfile_item_repository_establish_owner(root.string(), shop_owner, shop_items,
							 &error) ==
			flatfile_item_baseline_result::applied,
		"could not establish shop custody: " + error);

	const shop_trade_payload payload = purchase(item);
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
			purchased.counterparty_owner_revision == 2 && purchased.item_count == 1 &&
			purchased.item_uids[0] == 200 && purchased.item_revisions[0] == 2,
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
			player_revision == 2 && shop_revision == 2 && player_items.size() == 1 &&
			player_items[0].item_uid == 200 && player_items[0].item_revision == 2 &&
			remaining_shop_items.empty(),
		"purchase did not transfer global item custody");

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
	std::cout << "flat-file shop trade repository passed\n";
}
