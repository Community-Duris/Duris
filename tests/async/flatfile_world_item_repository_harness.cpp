#include "economy/collector_command.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
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

static player_item_snapshot item(uint64_t uid, int32_t parent, int32_t vnum)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = -1;
	value.object_uid = uid;
	value.generated_key = static_cast<int64_t>(uid + 1000);
	value.vnum = vnum;
	value.name = "world item";
	value.short_description = "a world item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "runes", "small runes", true, { 7, 8 } });
	return value;
}

static flatfile_corpse_record corpse(uint32_t pid, uint32_t save_id, uint64_t uid)
{
	flatfile_corpse_record value = {};
	value.owner_pid = pid;
	value.owner_name = pid == 42 ? "Hero" : "Other";
	value.save_id = save_id;
	value.room_vnum = 500;
	value.short_description = "the corpse of Hero";
	value.description = "The corpse of Hero is lying here.";
	value.keywords = "corpse hero _pcorpse_";
	value.weight = 90;
	value.values = { 1, 2, 3, 4, 5, 6, 0, 8 };
	value.money = { 10, 20, 30, 40 };
	value.revision = 4;
	value.items = { item(uid, PLAYER_SNAPSHOT_NO_PARENT, 300), item(uid + 1, 0, 301) };
	return value;
}

static uint32_t read_u32(const std::vector<uint8_t> &bytes, size_t *offset)
{
	require(offset && *offset <= bytes.size() && bytes.size() - *offset >= 4,
		"legacy catalog fixture was truncated");
	uint32_t value = 0;
	for (size_t index = 0; index < 4; ++index)
		value |= static_cast<uint32_t>(bytes[(*offset)++]) << (index * 8);
	return value;
}

static void write_u32(std::vector<uint8_t> *bytes, size_t offset, uint32_t value)
{
	require(bytes && offset <= bytes->size() && bytes->size() - offset >= 4,
		"legacy catalog fixture header was truncated");
	for (size_t index = 0; index < 4; ++index)
	{
		(*bytes)[offset + index] = static_cast<uint8_t>(value & 0xff);
		value >>= 8;
	}
}

static void skip_text(const std::vector<uint8_t> &bytes, size_t *offset)
{
	const uint32_t size = read_u32(bytes, offset);
	require(*offset <= bytes.size() && bytes.size() - *offset >= size,
		"legacy catalog fixture text was truncated");
	*offset += size;
}

static void convert_world_catalog_to_version_one(const fs::path &catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	std::ifstream input(catalog, std::ios::binary);
	require(input.good(), "could not open world item catalog for legacy conversion");
	std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
				   std::istreambuf_iterator<char>());
	require(bytes.size() >= header_size, "world item catalog header was truncated");
	size_t offset = header_size;
	const uint32_t corpse_count = read_u32(bytes, &offset);
	read_u32(bytes, &offset);
	bytes.erase(bytes.begin() + offset, bytes.begin() + offset + 4);
	std::vector<size_t> money_offsets;
	for (uint32_t index = 0; index < corpse_count; ++index)
	{
		offset += 4;
		skip_text(bytes, &offset);
		offset += 8;
		skip_text(bytes, &offset);
		skip_text(bytes, &offset);
		skip_text(bytes, &offset);
		offset += 4 + 8 * 4;
		require(offset <= bytes.size() && bytes.size() - offset >= 16,
			"world item money aggregate was truncated");
		money_offsets.push_back(offset);
		offset += 16 + 8;
		const uint32_t item_blob_size = read_u32(bytes, &offset);
		require(offset <= bytes.size() && bytes.size() - offset >= item_blob_size,
			"world item snapshot was truncated");
		offset += item_blob_size;
	}
	for (auto iterator = money_offsets.rbegin(); iterator != money_offsets.rend(); ++iterator)
		bytes.erase(bytes.begin() + *iterator, bytes.begin() + *iterator + 16);
	write_u32(&bytes, 8, 1);
	write_u32(&bytes, 12, static_cast<uint32_t>(bytes.size() - header_size));
	SHA256(bytes.data() + header_size, bytes.size() - header_size, bytes.data() + 24);
	std::ofstream output(catalog, std::ios::binary | std::ios::trunc);
	require(output.good(), "could not rewrite legacy world item catalog");
	output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	require(output.good(), "could not flush legacy world item catalog");
}

static flatfile_saved_world_item_record saved_item()
{
	flatfile_saved_world_item_record value = {};
	value.item_key = "item.statue.1";
	value.room_vnum = 700;
	value.revision = 3;
	value.items = { item(200, PLAYER_SNAPSHOT_NO_PARENT, 400), item(201, 0, 401) };
	return value;
}

static std::vector<uint8_t> encode_items(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> bytes;
	require(player_item_snapshot_list_encode(items, &bytes) == player_snapshot_codec_result::ok,
		"could not encode saved item fixture");
	return bytes;
}

static std::vector<uint8_t> read_catalog(const fs::path &root)
{
	std::ifstream input(root / "domains/world_item_catalog", std::ios::binary);
	require(input.good(), "could not read persisted world item catalog");
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}

static void require_saved_items(const flatfile_saved_world_item_record &actual,
				const flatfile_saved_world_item_record &expected)
{
	require(actual.item_key == expected.item_key && actual.room_vnum == expected.room_vnum &&
			actual.revision == expected.revision &&
			actual.items.size() == expected.items.size(),
		"saved item forest lost its key, room, revision or members");
	require(encode_items(actual.items) == encode_items(expected.items),
		"saved item forest changed serialized payload fields");
	for (size_t index = 0; index < expected.items.size(); ++index)
	{
		const auto &value = actual.items[index];
		const auto &original = expected.items[index];
		require(value.object_uid == original.object_uid &&
				value.parent_index == original.parent_index &&
				value.weight == original.weight &&
				value.generated_key == original.generated_key &&
				value.vnum == original.vnum && value.name == original.name &&
				value.short_description == original.short_description,
			"saved item forest lost topology, weights or rich item fields");
	}
}

static void test_saved_root_collection(const fs::path &root)
{
	prepare_root(root);
	std::string error;
	auto saved = saved_item();
	saved.items = { item(200, PLAYER_SNAPSHOT_NO_PARENT, 400), item(201, 0, 401),
			item(202, 0, 402), item(203, 2, 403) };
	saved.items[0].weight = 30;
	saved.items[1].weight = 5;
	saved.items[2].weight = 6;
	saved.items[3].weight = 2;
	require(flatfile_world_item_establish(root.string(), {}, { saved }, &error) ==
			flatfile_world_item_result::ok,
		"saved container collection fixture establishment failed: " + error);
	const auto before = read_catalog(root);
	collector_command_payload payload = {};
	payload.action = collector_action::collect;
	payload.from_owner = { item_owner_type::room, static_cast<uint64_t>(saved.room_vnum), 0 };
	payload.to_owner = { item_owner_type::collector, 1, 0 };
	payload.target_state = item_custody_state::active;
	payload.expected_from_owner_revision = saved.revision;
	payload.selected_item_uid = 200;
	payload.item_count = 4;
	payload.items[0] = { 200, 200, 0, 1, 400, item_custody_state::active };
	payload.items[1] = { 201, 200, 200, 1, 401, item_custody_state::active };
	payload.items[2] = { 202, 200, 200, 1, 402, item_custody_state::active };
	payload.items[3] = { 203, 200, 202, 1, 403, item_custody_state::active };
	auto shell = saved.items[0];
	shell.weight = 19; // Direct child aggregate weights stay with the saved contents.
	shell.equipment_slot = 0;
	const auto blob = encode_items({ shell });
	require(blob.size() <= payload.item_blob.size(), "collector shell fixture exceeds payload");
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not lock saved container authority");
		flatfile_collector_world_mutation mutation;
		unsigned int result_code = 0;
		payload.expected_from_owner_revision = saved.revision + 1;
		require(flatfile_world_item_prepare_collector_transfer(
				root.string(), lock, payload, &mutation, &result_code, &error) ==
					flatfile_world_item_result::ok &&
				result_code == ESTALE && !mutation.changed,
			"flat-file collection accepted a stale source-owner revision");
		require(read_catalog(root) == before,
			"stale collection attempt changed the saved-item catalog");
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire saved container authority");
		flatfile_collector_world_mutation mutation;
		unsigned int result_code = 1;
		payload.expected_from_owner_revision = saved.revision;
		require(flatfile_world_item_prepare_collector_transfer(
				root.string(), lock, payload, &mutation, &result_code, &error) ==
					flatfile_world_item_result::ok &&
				result_code == 0 && mutation.changed,
			"saved root collection did not prepare a forest after-image: " + error);
		require(read_catalog(root) == before,
			"preparing collection published before commit");
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"saved root collection did not commit: " + error);
	}
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty() && saved_items.size() == 1,
		"saved root collection did not retain exactly one saved key: " + error);
	saved.items.erase(saved.items.begin());
	saved.items[0].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	saved.items[1].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	saved.items[2].parent_index = 1;
	++saved.revision;
	require_saved_items(saved_items[0], saved);
	require(std::none_of(saved_items[0].items.begin(), saved_items[0].items.end(),
			     [](const auto &value) { return value.object_uid == 200; }),
		"collected shell remained in saved room custody");
}

static void test_nested_saved_container_collection(const fs::path &root)
{
	prepare_root(root);
	std::string error;
	auto saved = saved_item();
	saved.room_vnum = 701;
	saved.items = { item(300, PLAYER_SNAPSHOT_NO_PARENT, 500), item(301, 0, 501),
			item(302, 0, 502), item(303, 2, 503) };
	saved.items[0].weight = 30;
	saved.items[1].weight = 5;
	saved.items[2].weight = 8;
	saved.items[3].weight = 2;
	require(flatfile_world_item_establish(root.string(), {}, { saved }, &error) ==
			flatfile_world_item_result::ok,
		"nested saved container fixture establishment failed: " + error);

	collector_command_payload payload = {};
	payload.action = collector_action::collect;
	payload.from_owner = { item_owner_type::room, static_cast<uint64_t>(saved.room_vnum), 0 };
	payload.to_owner = { item_owner_type::collector, 2, 0 };
	payload.target_state = item_custody_state::active;
	payload.expected_from_owner_revision = saved.revision;
	payload.selected_item_uid = 302;
	payload.item_count = 4;
	payload.items[0] = { 300, 300, 0, 1, 500, item_custody_state::active };
	payload.items[1] = { 301, 300, 300, 1, 501, item_custody_state::active };
	payload.items[2] = { 302, 300, 300, 1, 502, item_custody_state::active };
	payload.items[3] = { 303, 300, 302, 1, 503, item_custody_state::active };
	auto shell = saved.items[2];
	shell.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	shell.weight = 6; // 8 aggregate weight minus the nested child weight of 2.
	shell.equipment_slot = 0;
	const auto blob = encode_items({ shell });
	payload.item_blob_size = static_cast<uint32_t>(blob.size());
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());

	flatfile_collector_world_mutation mutation;
	unsigned int result_code = 1;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not lock nested saved container authority");
		require(flatfile_world_item_prepare_collector_transfer(
				root.string(), lock, payload, &mutation, &result_code, &error) ==
					flatfile_world_item_result::ok &&
				result_code == 0 && mutation.changed,
			"nested saved container collection did not prepare: " + error);
		require(flatfile_authority_transaction_commit(root.string(), lock,
							      { mutation.after_image }, &error) ==
				flatfile_authority_transaction_result::ok,
			"nested saved container collection did not commit: " + error);
	}

	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			saved_items.size() == 1,
		"nested saved container collection lost its saved key: " + error);
	saved.items.erase(saved.items.begin() + 2);
	saved.items[2].parent_index = 0;
	saved.items[0].weight = 24;
	++saved.revision;
	require_saved_items(saved_items[0], saved);
	require(std::none_of(saved_items[0].items.begin(), saved_items[0].items.end(),
			     [](const auto &value) { return value.object_uid == 302; }),
		"nested collected container remained in saved room custody");
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "world";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_saved_world_item_record> saved_items;
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
			flatfile_world_item_result::not_found,
		"missing world item authority did not fail closed");
	const auto first = corpse(42, 20, 100);
	const auto second = corpse(77, 10, 110);
	const auto saved = saved_item();
	require(flatfile_world_item_establish(root.string(), { second, first }, { saved },
					      &error) == flatfile_world_item_result::ok,
		"world item establishment failed: " + error);
	require(flatfile_world_item_establish(root.string(), { first, second }, { saved },
					      &error) == flatfile_world_item_result::already_exists,
		"canonical world item establishment retry was not idempotent");
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2 && corpses[0].owner_pid == 42 &&
			corpses[0].owner_name == "hero" && corpses[0].values[7] == 8 &&
			corpses[0].money == std::array<int32_t, 4>{ 10, 20, 30, 40 } &&
			corpses[0].items.size() == 2 && corpses[0].items[1].parent_index == 0 &&
			corpses[0].items[0].dynamic_affects[0].extra2 == 5 &&
			saved_items.size() == 1 && saved_items[0].item_key == "item.statue.1" &&
			saved_items[0].items[1].extra_descriptions[0].spell_ids[1] == 8,
		"world item catalog was not canonical or did not round trip nested state");
	auto conflicting = first;
	conflicting.weight++;
	require(flatfile_world_item_establish(root.string(), { conflicting, second }, { saved },
					      &error) == flatfile_world_item_result::invalid,
		"conflicting world item establishment was accepted");

	const fs::path legacy_root = fs::path(argv[1]) / "legacy";
	prepare_root(legacy_root);
	require(flatfile_world_item_establish(legacy_root.string(), { first }, {}, &error) ==
			flatfile_world_item_result::ok,
		"legacy world item fixture establishment failed");
	convert_world_catalog_to_version_one(legacy_root / "domains/world_item_catalog");
	corpses.clear();
	saved_items.clear();
	require(flatfile_world_item_list(legacy_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 &&
			corpses[0].money == std::array<int32_t, 4>{ 0, 0, 0, 0 } &&
			corpses[0].items.size() == 2 && saved_items.empty(),
		"version one world item catalog did not decode with an empty money aggregate");

	const fs::path transfer_root = fs::path(argv[1]) / "transfer";
	prepare_root(transfer_root);
	require(!fs::exists(transfer_root / "domains/world_item_catalog"),
		"first item-bearing corpse fixture unexpectedly has a world catalog");
	item_transfer_payload transfer = {};
	transfer.from_owner = { item_owner_type::player, 9, 0 };
	transfer.to_owner = { item_owner_type::corpse, item_corpse_owner_id(9, 33), 0 };
	transfer.reason = item_transfer_reason::corpse_create;
	transfer.selected_item_uid = 300;
	transfer.target_root_item_uid = 300;
	transfer.item_count = 1;
	transfer.items[0] = { 300, 300, 0, 1, 500, item_custody_state::active };
	std::vector<player_item_snapshot> transferred_items = { item(300, PLAYER_SNAPSHOT_NO_PARENT,
								     500) };
	transferred_items[0].equipment_slot = 0;
	std::vector<uint8_t> transfer_blob;
	require(player_item_snapshot_list_encode(transferred_items, &transfer_blob) ==
			player_snapshot_codec_result::ok,
		"could not encode corpse transfer item");
	transfer.item_blob_size = static_cast<uint32_t>(transfer_blob.size());
	std::copy(transfer_blob.begin(), transfer_blob.end(), transfer.item_blob.begin());
	transfer.corpse.present = true;
	transfer.corpse.room_vnum = 900;
	transfer.corpse.weight = 55;
	transfer.corpse.actor_racewar = 1;
	transfer.corpse.values[3] = 9;
	transfer.corpse.values[5] = 1;
	transfer.corpse.values[6] = 33;
	transfer.corpse.owner_name = "TransferOwner";
	transfer.corpse.short_description = "the transfer corpse";
	transfer.corpse.description = "The transfer corpse is lying here.";
	transfer.corpse.keywords = "corpse transferowner _pcorpse_";
	flatfile_corpse_transfer_mutation transfer_mutation;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(transfer_root.string(), &error),
			"could not acquire corpse creation authority");
		auto missing_update = transfer;
		missing_update.expected_to_revision = 1;
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, missing_update, &transfer_mutation,
				&error) == flatfile_world_item_result::not_found,
			"missing corpse update initialized a catalog");
		auto missing_loot = transfer;
		missing_loot.from_owner = transfer.to_owner;
		missing_loot.to_owner = transfer.from_owner;
		missing_loot.reason = item_transfer_reason::corpse_loot;
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, missing_loot, &transfer_mutation,
				&error) == flatfile_world_item_result::not_found,
			"missing corpse loot initialized a catalog");
		require(!fs::exists(transfer_root / "domains/world_item_catalog"),
			"rejected corpse operation wrote a catalog");
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, transfer, &transfer_mutation,
				&error) == flatfile_world_item_result::ok &&
				transfer_mutation.created &&
				transfer_mutation.expected_items.empty() &&
				transfer_mutation.corpse_revision == 1,
			"first corpse transfer did not prepare establishment");
		require(flatfile_authority_transaction_commit(
				transfer_root.string(), lock, { transfer_mutation.after_image },
				&error) == flatfile_authority_transaction_result::ok,
			"first corpse transfer did not commit: " + error);
	}
	corpses.clear();
	saved_items.clear();
	require(flatfile_world_item_list(transfer_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].owner_name == "transferowner" &&
			corpses[0].room_vnum == 900 && corpses[0].weight == 55 &&
			corpses[0].items.size() == 1 && corpses[0].items[0].object_uid == 300 &&
			corpses[0].items[0].equipment_slot == -1,
		"first corpse transfer did not preserve metadata and item state");
	transfer.from_owner = transfer.to_owner;
	transfer.to_owner = { item_owner_type::player, 10, 0 };
	transfer.reason = item_transfer_reason::corpse_loot;
	transfer.corpse.weight = 40;
	transfer.corpse.actor_racewar = 2;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(transfer_root.string(), &error),
			"could not acquire corpse loot authority");
		require(flatfile_world_item_prepare_corpse_transfer(
				transfer_root.string(), lock, transfer, &transfer_mutation,
				&error) == flatfile_world_item_result::ok &&
				!transfer_mutation.created &&
				transfer_mutation.expected_items.size() == 1 &&
				transfer_mutation.expected_items[0].item_uid == 300 &&
				transfer_mutation.corpse_revision == 2,
			"corpse loot did not prepare exact prestate evidence");
		require(flatfile_authority_transaction_commit(
				transfer_root.string(), lock, { transfer_mutation.after_image },
				&error) == flatfile_authority_transaction_result::ok,
			"corpse loot did not commit: " + error);
	}
	corpses.clear();
	require(flatfile_world_item_list(transfer_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].revision == 2 &&
			corpses[0].weight == 40 && corpses[0].items.empty(),
		"corpse loot did not retain the empty metadata aggregate");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto duplicate_uid = saved;
	duplicate_uid.items[0].object_uid = 100;
	require(flatfile_world_item_establish(invalid_root.string(), { first }, { duplicate_uid },
					      &error) == flatfile_world_item_result::invalid,
		"duplicate UID across corpse and saved room custody was accepted");
	auto malformed = saved;
	malformed.items[1].parent_index = 1;
	require(flatfile_world_item_establish(invalid_root.string(), {}, { malformed }, &error) ==
			flatfile_world_item_result::invalid,
		"malformed saved item nesting was accepted");
	auto duplicate_corpse = first;
	duplicate_corpse.owner_name = "Impostor";
	require(flatfile_world_item_establish(invalid_root.string(), { first, duplicate_corpse },
					      {}, &error) == flatfile_world_item_result::invalid,
		"duplicate corpse owner/save identity was accepted");
	auto empty_forest = saved;
	empty_forest.items.clear();
	require(flatfile_world_item_establish(invalid_root.string(), {}, { empty_forest },
					      &error) == flatfile_world_item_result::invalid,
		"empty saved item forest was accepted");
	const fs::path forest_root = fs::path(argv[1]) / "forest";
	prepare_root(forest_root);
	auto two_roots = saved;
	two_roots.items[1].parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	require(flatfile_world_item_establish(forest_root.string(), {}, { two_roots }, &error) ==
			flatfile_world_item_result::ok,
		"saved item forest establishment failed: " + error);
	corpses.clear();
	saved_items.clear();
	require(flatfile_world_item_list(forest_root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.empty() && saved_items.size() == 1,
		"saved item forest did not round trip as one saved key: " + error);
	require_saved_items(saved_items[0], two_roots);
	test_saved_root_collection(fs::path(argv[1]) / "collector-root");
	test_nested_saved_container_collection(fs::path(argv[1]) / "collector-nested");

	flatfile_world_item_player_removal removal;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire world item authority");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "wrong",
								  &removal, &error) ==
				flatfile_world_item_result::conflict,
			"corpse owner name mismatch did not conflict");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "Hero",
								  &removal, &error) ==
					flatfile_world_item_result::ok &&
				removal.operation.filename == "world_item_catalog" &&
				removal.custody.size() == 1 &&
				removal.custody[0].owner.type == item_owner_type::corpse &&
				removal.custody[0].owner.id == item_corpse_owner_id(42, 20) &&
				removal.custody[0].items.size() == 2 &&
				removal.custody[0].items[0].item_uid == 100 &&
				removal.custody[0].items[1].vnum == 301,
			"corpse removal did not prepare exact custody evidence");
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 2,
		"prepared corpse removal published before commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire world item authority");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock, { removal.operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"corpse removal transaction failed: " + error);
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
				flatfile_world_item_result::ok &&
			corpses.size() == 1 && corpses[0].owner_pid == 77 &&
			saved_items.size() == 1 && saved_items[0].items[0].object_uid == 200,
		"corpse removal did not preserve unrelated corpse and saved room state");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire world item authority for retry");
		require(flatfile_world_item_prepare_player_remove(root.string(), lock, 42, "hero",
								  &removal, &error) ==
				flatfile_world_item_result::unchanged,
			"corpse removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/world_item_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open world item catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_world_item_list(root.string(), &corpses, &saved_items, &error) ==
			flatfile_world_item_result::invalid,
		"corrupt world item authority was exposed");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not lock corrupt catalog");
		transfer.from_owner = { item_owner_type::player, 9, 0 };
		transfer.to_owner = { item_owner_type::corpse, item_corpse_owner_id(9, 33), 0 };
		transfer.reason = item_transfer_reason::corpse_create;
		require(flatfile_world_item_prepare_corpse_transfer(root.string(), lock, transfer,
								    &transfer_mutation, &error) ==
				flatfile_world_item_result::invalid,
			"new corpse creation replaced a corrupt catalog");
	}
	std::cout << "flat-file world item repository passed\n";
	return 0;
}
