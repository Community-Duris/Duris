#include "flatfile_shopkeeper_repository.h"
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

static void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "domains", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "players", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities/names", fs::perms::owner_all, fs::perm_options::replace);
}

static player_item_snapshot item(uint64_t uid, int32_t parent, int16_t slot, int32_t vnum)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = slot;
	value.object_uid = uid;
	value.generated_key = uid + 1000;
	value.vnum = vnum;
	value.name = "shop item";
	value.short_description = "a shop item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "mark", "a maker's mark", false, {} });
	return value;
}

static flatfile_shopkeeper_record shop(uint32_t id, uint64_t uid)
{
	flatfile_shopkeeper_record value = {};
	value.shop_id = id;
	value.mob_vnum = 1000 + id;
	value.room_vnum = 2000 + id;
	value.saved_at = 3000 + id;
	value.revision = 2;
	value.affects = { { 9, 10, 11, 12, { 1, 2, 3, 4, 5 } },
			  { 1, 2, 3, 4, { 6, 7, 8, 9, 10 } } };
	value.items = { item(uid, PLAYER_SNAPSHOT_NO_PARENT, 3, 400), item(uid + 1, 0, 0, 401),
			item(uid + 2, PLAYER_SNAPSHOT_NO_PARENT, 0, 402) };
	return value;
}

static shop_trade_payload trade(shop_trade_action action, uint32_t shop_id, uint64_t shop_revision,
				const player_item_snapshot &object, uint64_t item_revision,
				uint64_t stock_uid = 0, uint64_t stock_revision = 0)
{
	shop_trade_payload payload = {};
	payload.action = action;
	payload.player_pid = 42;
	payload.shop_id = shop_id;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "ShopTester");
	payload.price = 100;
	payload.expected_wallet_revision = 4;
	payload.expected_bank_revision = 5;
	payload.expected_shop_revision = shop_revision;
	payload.selected_item_uid = object.object_uid;
	payload.target_root_item_uid = object.object_uid;
	payload.stock_item_uid = stock_uid;
	payload.expected_stock_item_revision = stock_revision;
	payload.stock_vnum = stock_uid ? object.vnum : 0;
	payload.item_count = 1;
	payload.items[0] = { object.object_uid,
			     object.object_uid,
			     0,
			     item_revision,
			     object.vnum,
			     action == shop_trade_action::buy_produced ?
				     item_custody_state::absent :
				     item_custody_state::active };
	std::vector<uint8_t> blob;
	require(player_item_snapshot_list_encode({ object }, &blob) ==
			player_snapshot_codec_result::ok,
		"could not encode shop trade item");
	require(blob.size() <= payload.item_blob.size(), "shop trade item exceeded command bound");
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	return payload;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "shops";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_shopkeeper_record> records;
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
			flatfile_shopkeeper_result::not_found,
		"missing shopkeeper authority did not fail closed");
	const auto first = shop(2, 100);
	const auto second = shop(1, 200);
	require(flatfile_shopkeeper_establish(root.string(), { first, second }, &error) ==
			flatfile_shopkeeper_result::ok,
		"shopkeeper establishment failed: " + error);
	require(flatfile_shopkeeper_establish(root.string(), { second, first }, &error) ==
			flatfile_shopkeeper_result::already_exists,
		"canonical shopkeeper establishment retry was not idempotent");
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records.size() == 2 && records[0].shop_id == 1 &&
			records[1].affects[0].type == 1 &&
			records[1].affects[1].bitvectors[4] == 5 && records[1].items.size() == 3 &&
			records[1].items[0].equipment_slot == 3 &&
			records[1].items[1].parent_index == 0 &&
			records[1].items[1].dynamic_affects[0].extra2 == 5,
		"shopkeeper catalog was not canonical or did not round trip state");
	auto conflicting = first;
	conflicting.room_vnum++;
	require(flatfile_shopkeeper_establish(root.string(), { conflicting, second }, &error) ==
			flatfile_shopkeeper_result::invalid,
		"conflicting shopkeeper establishment was accepted");
	auto replacement = records[1];
	replacement.revision = 3;
	replacement.saved_at++;
	replacement.room_vnum++;
	require(flatfile_shopkeeper_replace(root.string(), replacement, 2, &error) ==
			flatfile_shopkeeper_result::ok,
		"revision-guarded shopkeeper replacement failed: " + error);
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records[1].revision == 3 && records[1].room_vnum == replacement.room_vnum,
		"shopkeeper replacement did not round trip");
	require(flatfile_shopkeeper_replace(root.string(), replacement, 2, &error) ==
			flatfile_shopkeeper_result::stale,
		"stale shopkeeper replacement was accepted");
	auto skipped_revision = replacement;
	skipped_revision.revision = 5;
	require(flatfile_shopkeeper_replace(root.string(), skipped_revision, 3, &error) ==
			flatfile_shopkeeper_result::invalid,
		"nonconsecutive shopkeeper revision was accepted");

	auto produced_item = item(300, PLAYER_SNAPSHOT_NO_PARENT, 0, 402);
	auto produced = trade(shop_trade_action::buy_produced, 2, 3, produced_item,
			      ITEM_TRANSFER_ABSENT_REVISION, 102, 7);
	flatfile_shopkeeper_trade_mutation mutation;
	unsigned int result_code = 0;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire shop trade lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, produced, &mutation,
							  &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == 0 && mutation.shop_revision == 4 &&
				!mutation.after_image.bytes.empty(),
			"produced trade mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"produced trade mutation did not commit: " + error);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records[1].revision == 4 && records[1].items.size() == 3,
		"produced trade changed persistent exemplar inventory");

	auto purchased_item = item(102, PLAYER_SNAPSHOT_NO_PARENT, 0, 402);
	auto purchase = trade(shop_trade_action::buy_existing, 2, 4, purchased_item, 8, 102, 8);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire purchase lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, purchase, &mutation,
							  &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == 0 && mutation.shop_revision == 5,
			"existing purchase mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"existing purchase mutation did not commit: " + error);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records[1].revision == 5 && records[1].items.size() == 2 &&
			records[1].items[0].object_uid == 100 &&
			records[1].items[1].parent_index == 0,
		"existing purchase did not remove exactly its authoritative subtree");

	auto unsupported_item = item(399, PLAYER_SNAPSHOT_NO_PARENT, 0, 699);
	auto unsupported_sale = trade(shop_trade_action::sell_store, 2, 5, unsupported_item, 9);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire unsupported sale lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, unsupported_sale,
							  &mutation, &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == EOPNOTSUPP && mutation.after_image.bytes.empty(),
			"dynamic-affect sale was admitted to complete shop state");
	}
	auto sold_item = item(400, PLAYER_SNAPSHOT_NO_PARENT, 0, 700);
	sold_item.dynamic_affects.clear();
	auto sale = trade(shop_trade_action::sell_store, 2, 5, sold_item, 9);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire sale lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, sale, &mutation,
							  &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == 0 && mutation.shop_revision == 6,
			"stored sale mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"stored sale mutation did not commit: " + error);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records[1].revision == 6 && records[1].items.size() == 3 &&
			records[1].items.back().object_uid == 400,
		"stored sale did not append the transferred graph");

	auto destroyed_item = item(500, PLAYER_SNAPSHOT_NO_PARENT, 0, 800);
	auto destruction = trade(shop_trade_action::sell_destroy, 2, 6, destroyed_item, 2);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire destruction lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, destruction,
							  &mutation, &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == 0 && mutation.shop_revision == 7,
			"destroyed sale mutation did not prepare: " + error);
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"destroyed sale mutation did not commit: " + error);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
				flatfile_shopkeeper_result::ok &&
			records[1].revision == 7 && records[1].items.size() == 3,
		"destroyed sale changed shop inventory");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire stale trade lock");
		require(flatfile_shopkeeper_prepare_trade(root.string(), lock, destruction,
							  &mutation, &result_code, &error) ==
					flatfile_shopkeeper_result::ok &&
				result_code == ESTALE && mutation.after_image.bytes.empty(),
			"stale shop trade was not fenced");
	}

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate_uid = second;
	duplicate_uid.items[0].object_uid = 100;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { first, duplicate_uid },
					      &error) == flatfile_shopkeeper_result::invalid,
		"duplicate item UID across shopkeepers was accepted");
	auto nested_equipment = first;
	nested_equipment.items[1].equipment_slot = 4;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { nested_equipment },
					      &error) == flatfile_shopkeeper_result::invalid,
		"nested shopkeeper equipment was accepted");
	auto duplicate_slot = first;
	duplicate_slot.items[2].equipment_slot = 3;
	require(flatfile_shopkeeper_establish(invalid_root.string(), { duplicate_slot }, &error) ==
			flatfile_shopkeeper_result::invalid,
		"duplicate shopkeeper equipment slot was accepted");

	const fs::path catalog = root / "domains/shopkeeper_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open shopkeeper catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_shopkeeper_list(root.string(), &records, &error) ==
			flatfile_shopkeeper_result::invalid,
		"corrupt shopkeeper authority was exposed");
	std::cout << "flat-file shopkeeper repository passed\n";
	return 0;
}
