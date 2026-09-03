#include "flatfile/flatfile_locker_repository.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <type_traits>
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
	value.name = "locker item";
	value.short_description = "a locker item";
	value.dynamic_affects.push_back({ 3, 4, 5 });
	value.extra_descriptions.push_back({ "runes", "small runes", true, { 7, 8 } });
	return value;
}

static flatfile_locker_record player_locker()
{
	flatfile_locker_chest_record private_chest = {};
	private_chest.chest_id = 12;
	private_chest.chest_name = "Private";
	private_chest.password_hash = "$hash";
	private_chest.sort_config = "weapons armor";
	private_chest.revision = 4;
	private_chest.items = { item(102, PLAYER_SNAPSHOT_NO_PARENT, 302) };
	flatfile_locker_chest_record public_chest = {};
	public_chest.chest_id = 11;
	public_chest.chest_name = "Public";
	public_chest.is_public = true;
	public_chest.revision = 3;
	public_chest.items = { item(100, PLAYER_SNAPSHOT_NO_PARENT, 300), item(101, 0, 301) };
	flatfile_locker_record locker = {};
	locker.locker_id = 2;
	locker.locker_name = "Hero.Locker";
	locker.owner_pid = 42;
	locker.racewar = 1;
	locker.race = 2;
	locker.revision = 9;
	locker.chests = { private_chest, public_chest };
	return locker;
}

static flatfile_locker_record guild_locker()
{
	flatfile_locker_chest_record public_chest = {};
	public_chest.chest_id = 20;
	public_chest.chest_name = "public";
	public_chest.is_public = true;
	public_chest.revision = 1;
	flatfile_locker_record locker = {};
	locker.locker_id = 1;
	locker.locker_name = "guild.7.locker";
	locker.owner_assoc_id = 7;
	locker.revision = 2;
	locker.chests = { public_chest };
	return locker;
}

static flatfile_locker_record account_locker()
{
	flatfile_locker_chest_record public_chest = {};
	public_chest.chest_id = 30;
	public_chest.chest_name = "public";
	public_chest.is_public = true;
	public_chest.revision = 2;
	public_chest.items = { item(200, PLAYER_SNAPSHOT_NO_PARENT, 400) };
	flatfile_locker_record locker = {};
	locker.locker_id = 3;
	locker.locker_name = "account.user.1.locker";
	locker.account_owner = flatfile_account_locker_identity{ "user", 1 };
	locker.racewar = 1;
	locker.revision = 3;
	locker.chests = { public_chest };
	return locker;
}

template <typename T> static void append_number(std::vector<uint8_t> *bytes, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
	{
		bytes->push_back(static_cast<uint8_t>(bits & 0xff));
		bits >>= 8;
	}
}

static void append_string(std::vector<uint8_t> *bytes, const std::string &value)
{
	append_number<uint32_t>(bytes, value.size());
	bytes->insert(bytes->end(), value.begin(), value.end());
}

static void append_legacy_locker(std::vector<uint8_t> *payload,
				 const flatfile_locker_record &locker)
{
	append_number(payload, locker.locker_id);
	append_string(payload, locker.locker_name);
	append_number(payload, locker.owner_pid);
	append_number(payload, locker.owner_assoc_id);
	append_number(payload, locker.racewar);
	append_number(payload, locker.race);
	append_number(payload, locker.revision);
	append_number<uint32_t>(payload, locker.chests.size());
	for (const auto &chest : locker.chests)
	{
		std::vector<uint8_t> items;
		require(player_item_snapshot_list_encode(chest.items, &items) ==
				player_snapshot_codec_result::ok,
			"could not encode legacy locker items");
		append_number(payload, chest.chest_id);
		append_string(payload, chest.chest_name);
		append_string(payload, chest.password_hash);
		append_number<uint8_t>(payload, chest.is_public ? 1 : 0);
		append_string(payload, chest.sort_config);
		append_number(payload, chest.revision);
		append_number<uint32_t>(payload, items.size());
		payload->insert(payload->end(), items.begin(), items.end());
	}
}

static void write_legacy_catalog(const fs::path &root,
				 const std::vector<flatfile_locker_record> &lockers)
{
	auto canonical_lockers = lockers;
	for (auto &locker : canonical_lockers)
	{
		std::transform(locker.locker_name.begin(), locker.locker_name.end(),
			       locker.locker_name.begin(), [](unsigned char character)
			       { return static_cast<char>(std::tolower(character)); });
		for (auto &chest : locker.chests)
			std::transform(chest.chest_name.begin(), chest.chest_name.end(),
				       chest.chest_name.begin(), [](unsigned char character)
				       { return static_cast<char>(std::tolower(character)); });
		std::sort(locker.chests.begin(), locker.chests.end(),
			  [](const auto &left, const auto &right)
			  { return left.chest_id < right.chest_id; });
	}
	std::sort(canonical_lockers.begin(), canonical_lockers.end(),
		  [](const auto &left, const auto &right)
		  { return left.locker_id < right.locker_id; });
	std::vector<uint8_t> payload;
	append_number<uint32_t>(&payload, canonical_lockers.size());
	append_number<uint32_t>(&payload, 0);
	for (const auto &locker : canonical_lockers)
		append_legacy_locker(&payload, locker);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.data(), payload.size(), digest.data());
	std::vector<uint8_t> file = { 'D', 'U', 'R', 'L', 'O', 'C', 'K', 0 };
	append_number<uint32_t>(&file, 1);
	append_number<uint32_t>(&file, payload.size());
	append_number<uint64_t>(&file, 1);
	file.insert(file.end(), digest.begin(), digest.end());
	file.insert(file.end(), payload.begin(), payload.end());
	const fs::path catalog = root / "domains/locker_catalog";
	std::ofstream output(catalog, std::ios::binary);
	output.write(reinterpret_cast<const char *>(file.data()), file.size());
	output.close();
	fs::permissions(catalog, fs::perms::owner_read | fs::perms::owner_write,
			fs::perm_options::replace);
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = fs::path(argv[1]) / "catalog";
	prepare_root(root);
	std::string error;
	std::vector<flatfile_locker_record> lockers;
	std::vector<flatfile_locker_access_record> access;
	const auto missing = flatfile_locker_list(root.string(), &lockers, &access, &error);
	require(missing == flatfile_locker_result::not_found,
		"missing locker authority did not fail closed: result=" +
			std::to_string(static_cast<int>(missing)) + " error=" + error);
	const auto player = player_locker();
	const auto guild = guild_locker();
	const auto account = account_locker();
	const fs::path legacy_root = fs::path(argv[1]) / "legacy";
	prepare_root(legacy_root);
	write_legacy_catalog(legacy_root, { player, guild });
	require(flatfile_locker_list(legacy_root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 2 && lockers[0].owner_assoc_id == 7 &&
			!lockers[0].account_owner && lockers[1].owner_pid == 42 &&
			!lockers[1].account_owner,
		"version-one player/guild catalog did not decode");
	const std::vector<flatfile_locker_access_record> source_access = {
		{ "Hero.Locker", "Guest", 6 },
		{ "guild.7.locker", "Hero", 2 },
		{ "account.user.1.locker", "AccountGuest", 3 },
		{ "guild.7.locker", "User", 4 }
	};
	require(flatfile_locker_establish(root.string(), { player, guild, account }, source_access,
					  &error) == flatfile_locker_result::ok,
		"locker establishment failed: " + error);
	require(flatfile_locker_establish(root.string(), { account, guild, player },
					  { source_access[3], source_access[2], source_access[1],
					    source_access[0] },
					  &error) == flatfile_locker_result::already_exists,
		"canonical locker establishment retry was not idempotent");
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 3 && lockers[0].locker_id == 1 &&
			lockers[1].locker_name == "hero.locker" && lockers[1].owner_pid == 42 &&
			lockers[1].chests.size() == 2 && lockers[1].chests[0].chest_id == 11 &&
			lockers[1].chests[0].items.size() == 2 &&
			lockers[1].chests[0].items[1].parent_index == 0 &&
			lockers[1].chests[0].items[0].dynamic_affects[0].extra2 == 5 &&
			lockers[1].chests[1].password_hash == "$hash" && lockers[2].account_owner &&
			lockers[2].account_owner->account_name == "user" &&
			lockers[2].account_owner->racewar_side == 1 &&
			lockers[2].chests[0].items[0].object_uid == 200 && access.size() == 4 &&
			access[0].owner_name == "account.user.1.locker" &&
			access[0].visitor_name == "accountguest",
		"locker catalog was not canonical or did not round trip nested state");
	auto conflicting = player;
	conflicting.revision++;
	require(flatfile_locker_establish(root.string(), { guild, conflicting, account },
					  source_access, &error) == flatfile_locker_result::invalid,
		"conflicting locker establishment was accepted");

	const fs::path invalid_root = fs::path(argv[1]) / "invalid";
	prepare_root(invalid_root);
	auto untyped_account = player;
	untyped_account.locker_name = "account.user.1.locker";
	require(flatfile_locker_establish(invalid_root.string(), { untyped_account }, {}, &error) ==
			flatfile_locker_result::invalid,
		"account locker was accepted without typed account authority");
	auto malformed_account = account;
	malformed_account.locker_name = "account.other.1.locker";
	require(flatfile_locker_establish(invalid_root.string(), { malformed_account }, {},
					  &error) == flatfile_locker_result::invalid,
		"account locker name was trusted over its typed owner");
	auto malformed_side = account;
	malformed_side.account_owner->racewar_side = 5;
	malformed_side.racewar = 5;
	malformed_side.locker_name = "account.user.5.locker";
	require(flatfile_locker_establish(invalid_root.string(), { malformed_side }, {}, &error) ==
			flatfile_locker_result::invalid,
		"out-of-range account locker side was accepted");
	auto duplicate_account = account;
	duplicate_account.locker_id = 4;
	duplicate_account.chests[0].chest_id = 31;
	require(flatfile_locker_establish(invalid_root.string(), { account, duplicate_account }, {},
					  &error) == flatfile_locker_result::invalid,
		"duplicate account/side locker owner was accepted");
	auto duplicate_uid = player;
	duplicate_uid.chests[1].items[0].object_uid = 102;
	require(flatfile_locker_establish(invalid_root.string(), { duplicate_uid }, {}, &error) ==
			flatfile_locker_result::invalid,
		"duplicate item UID was accepted across locker chests");
	auto no_public = player;
	for (auto &chest : no_public.chests)
		chest.is_public = false;
	require(flatfile_locker_establish(invalid_root.string(), { no_public }, {}, &error) ==
			flatfile_locker_result::invalid,
		"locker without one public chest was accepted");
	require(flatfile_locker_establish(invalid_root.string(), { player },
					  { { "missing.locker", "guest", 1 } },
					  &error) == flatfile_locker_result::invalid,
		"dangling locker access was accepted");
	auto duplicate_owner = guild;
	duplicate_owner.locker_id = 3;
	duplicate_owner.locker_name = "guild.7.second";
	duplicate_owner.chests[0].chest_id = 30;
	require(flatfile_locker_establish(invalid_root.string(), { guild, duplicate_owner }, {},
					  &error) == flatfile_locker_result::invalid,
		"duplicate association locker owner was accepted");

	flatfile_locker_player_removal removal;
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire account locker authority");
		require(flatfile_locker_prepare_account_remove(root.string(), lock, "UsEr",
							       &removal, &error) ==
					flatfile_locker_result::ok &&
				removal.custody.size() == 1 && removal.custody[0].owner.id == 3 &&
				removal.custody[0].owner.context_id == 30 &&
				removal.custody[0].items.size() == 1 &&
				removal.custody[0].items[0].item_uid == 200,
			"account locker removal did not prepare exact custody metadata");
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire account locker authority for commit");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock, { removal.operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"account locker removal transaction failed: " + error);
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 2 && access.size() == 2 &&
			access[0].owner_name == "guild.7.locker" &&
			access[0].visitor_name == "hero",
		"account locker removal stranded owned or visitor access rows");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire locker authority");
		require(flatfile_locker_prepare_player_remove(root.string(), lock, 42, "Hero",
							      &removal, &error) ==
					flatfile_locker_result::ok &&
				removal.operation.filename == "locker_catalog" &&
				removal.custody.size() == 2 &&
				removal.custody[0].owner.type == item_owner_type::locker &&
				removal.custody[0].owner.id == 2 &&
				removal.custody[0].owner.context_id == 11 &&
				removal.custody[0].items.size() == 2 &&
				removal.custody[0].items[0].item_uid == 100 &&
				removal.custody[1].owner.context_id == 12 &&
				removal.custody[1].items[0].vnum == 302,
			"player locker removal did not prepare exact custody metadata");
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 2 && access.size() == 2,
		"prepared locker removal published before transaction commit");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire locker authority for commit");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock, { removal.operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"locker removal transaction failed: " + error);
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
				flatfile_locker_result::ok &&
			lockers.size() == 1 && lockers[0].owner_assoc_id == 7 && access.empty(),
		"locker removal did not remove owned locker and player access rows");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire locker authority for retry");
		require(flatfile_locker_prepare_player_remove(root.string(), lock, 42, "Hero",
							      &removal, &error) ==
				flatfile_locker_result::unchanged,
			"player locker removal retry was not idempotent");
	}

	const fs::path catalog = root / "domains/locker_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open locker catalog for corruption");
		file.seekg(-1, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&byte, 1);
	}
	require(flatfile_locker_list(root.string(), &lockers, &access, &error) ==
			flatfile_locker_result::invalid,
		"corrupt locker authority was exposed");
	std::cout << "flat-file locker repository passed\n";
	return 0;
}
