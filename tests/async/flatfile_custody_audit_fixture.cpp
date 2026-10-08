#include "../../scripts/qualify_flatfile_native_custody.h"
#include "../../scripts/qualify_flatfile_native_world.h"
#include "../../scripts/qualify_flatfile_native_locker.h"
#include "../../scripts/qualify_flatfile_native_shopkeeper.h"
#include "../../scripts/qualify_flatfile_native_player.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_locker_repository.h"
#include "flatfile/flatfile_shopkeeper_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_store.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "player/player_snapshot_codec.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sys/stat.h>

void logit(const char *, const char *, ...) {}

static int world(const std::string &root, const std::vector<uint8_t> &encoded, bool expected)
{
	std::filesystem::create_directories(root + "/domains");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0);
	std::string error;
	assert(flatfile_atomic_write(root + "/domains", "world_item_catalog", encoded, &error));
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_room_item_record> rooms;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		const auto result = flatfile_world_item_recovery_list_locked(root, lock, &corpses,
									     &rooms, &error);
		assert((result == flatfile_world_item_result::ok) == expected);
	}
	bool accepted = false;
	size_t item_count = 0;
	try
	{
		const auto independent = restore_native_world::decode_world(encoded);
		accepted = true;
		assert(expected && independent.corpses == corpses.size() &&
		       independent.rooms == rooms.size());
		std::vector<flatfile_saved_world_item_record> saved;
		// Original native full-list API, only in this initialized private fixture.
		// Its journal recovery is a no-op; the independent operator never calls it.
		assert(flatfile_world_item_list(root, &corpses, &saved, &error) ==
		       flatfile_world_item_result::ok);
		assert(independent.saved == saved.size());
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		for (size_t i = 0; i < independent.records.size(); ++i)
		{
			const auto &record = independent.records[i];
			const std::vector<player_item_snapshot> *native_items = nullptr;
			if (i < corpses.size())
			{
				assert(record.revision == corpses[i].revision &&
				       record.money == corpses[i].money);
				assert(record.location.id ==
				       item_corpse_owner_id(corpses[i].owner_pid,
							    corpses[i].save_id));
				assert(record.items.size() == corpses[i].items.size());
				native_items = &corpses[i].items;
			}
			else if (i < corpses.size() + saved.size())
			{
				const auto &native = saved[i - corpses.size()];
				assert(record.revision == native.revision &&
				       record.location.id ==
					       static_cast<uint64_t>(native.room_vnum));
				assert(record.items.size() == native.items.size());
				native_items = &native.items;
			}
			else
			{
				const auto &native = rooms[i - corpses.size() - saved.size()];
				assert(record.revision == native.revision &&
				       record.money == native.money);
				assert(record.location.id ==
				       static_cast<uint64_t>(native.room_vnum));
				assert(record.items.size() == native.items.size());
				native_items = &native.items;
			}
			for (size_t j = 0; j < record.items.size(); ++j)
			{
				const auto &literal = record.items[j];
				++item_count;
				auto native = (*native_items)[j];
				assert(native.parent_index == literal.parent &&
				       native.equipment_slot == literal.equipment);
				native.parent_index = -1;
				std::vector<uint8_t> canonical;
				assert(player_item_snapshot_list_encode({ native }, &canonical) ==
				       player_snapshot_codec_result::ok);
				assert(restore_economic_authority::same(
					std::span(canonical).subspan(8),
					literal.encoded.subspan(4)));
			}
		}
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		(void)restore_native_world::audit(root, budget);
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_world::audit(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << item_count << ",\"item_fields_match\":true}\n";
	return 0;
}

static int locker(const std::string &root, const std::vector<uint8_t> &encoded, bool expected)
{
	std::filesystem::create_directories(root + "/domains");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0);
	std::string error;
	assert(flatfile_atomic_write(root + "/domains", "locker_catalog", encoded, &error));
	std::vector<flatfile_locker_record> native;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		const auto result =
			flatfile_locker_recovery_list_locked(root, lock, &native, &error);
		assert((result == flatfile_locker_result::ok) == expected);
	}
	bool accepted = false;
	size_t item_count = 0;
	try
	{
		const auto independent = restore_native_locker::decode_locker(encoded);
		accepted = true;
		assert(expected && independent.lockers == native.size());
		std::vector<flatfile_locker_access_record> access;
		// Original full native API only in this initialized private journal-free fixture.
		assert(flatfile_locker_list(root, &native, &access, &error) ==
		       flatfile_locker_result::ok);
		assert(independent.access == access.size());
		size_t index = 0;
		for (const auto &record : native)
			for (const auto &chest : record.chests)
			{
				assert(index < independent.chests.size());
				const auto &decoded = independent.chests[index++];
				assert(decoded.location.type == 5 &&
				       decoded.location.id == record.locker_id &&
				       decoded.location.context == chest.chest_id &&
				       decoded.locker_revision == record.revision &&
				       decoded.revision == chest.revision &&
				       decoded.items.size() == chest.items.size());
				for (size_t i = 0; i < decoded.items.size(); ++i)
				{
					++item_count;
					const auto &literal = decoded.items[i];
					auto item = chest.items[i];
					assert(item.parent_index == literal.parent &&
					       item.equipment_slot == literal.equipment);
					item.parent_index = -1;
					std::vector<uint8_t> canonical;
					assert(player_item_snapshot_list_encode({ item },
										&canonical) ==
					       player_snapshot_codec_result::ok);
					assert(restore_economic_authority::same(
						std::span(canonical).subspan(8),
						literal.encoded.subspan(4)));
				}
			}
		assert(index == independent.chests.size());
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		(void)restore_native_locker::audit(root, budget);
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_locker::audit(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << item_count << ",\"item_fields_match\":true}\n";
	return 0;
}

static int shopkeeper(const std::string &root, const std::vector<uint8_t> &encoded, bool expected)
{
	std::filesystem::create_directories(root + "/domains");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0);
	std::string error;
	assert(flatfile_atomic_write(root + "/domains", "shopkeeper_catalog", encoded, &error));
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
	}
	std::vector<flatfile_shopkeeper_record> native;
	// The full native API is only used in this initialized private journal-free fixture.
	assert((flatfile_shopkeeper_list(root, &native, &error) ==
		flatfile_shopkeeper_result::ok) == expected);
	bool accepted = false;
	size_t item_count = 0;
	try
	{
		const auto independent = restore_native_shopkeeper::decode_shopkeeper(encoded);
		accepted = true;
		assert(expected && independent.records.size() == native.size());
		for (size_t index = 0; index < native.size(); ++index)
		{
			const auto &record = native[index];
			const auto &decoded = independent.records[index];
			assert(decoded.location.type == 9 &&
			       decoded.location.id == item_shopkeeper_owner_id(record.shop_id) &&
			       decoded.location.context == 0 && decoded.mobile == record.mob_vnum &&
			       decoded.room == record.room_vnum &&
			       decoded.saved_at == record.saved_at &&
			       decoded.revision == record.revision && decoded.cash == record.cash &&
			       decoded.roaming == record.roaming &&
			       decoded.affects.size() == record.affects.size() * 56 &&
			       decoded.items.size() == record.items.size());
			restore_economic_authority::reader affects{ decoded.affects };
			for (const auto &affect : record.affects)
			{
				assert(restore_native_custody::signed32(affects) == affect.type &&
				       restore_native_custody::signed32(affects) ==
					       affect.duration &&
				       restore_native_custody::signed32(affects) ==
					       affect.modifier &&
				       restore_native_custody::signed32(affects) ==
					       affect.location);
				for (auto bits : affect.bitvectors)
					assert(affects.number(8) == bits);
			}
			affects.done();
			for (size_t i = 0; i < decoded.items.size(); ++i)
			{
				++item_count;
				const auto &literal = decoded.items[i];
				auto item = record.items[i];
				assert(item.parent_index == literal.parent &&
				       item.equipment_slot == literal.equipment);
				item.parent_index = -1;
				std::vector<uint8_t> canonical;
				assert(player_item_snapshot_list_encode({ item }, &canonical) ==
				       player_snapshot_codec_result::ok);
				assert(restore_economic_authority::same(
					std::span(canonical).subspan(8),
					literal.encoded.subspan(4)));
			}
		}
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		(void)restore_native_shopkeeper::audit(root, budget);
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_shopkeeper::audit(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << item_count << ",\"item_fields_match\":true}\n";
	return 0;
}

static int player(const std::string &root, const std::vector<uint8_t> &encoded, bool expected,
		  int32_t pid)
{
	std::filesystem::create_directories(root + "/domains");
	std::filesystem::create_directories(root + "/players");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0 &&
	       chmod((root + "/players").c_str(), 0700) == 0);
	std::string error;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
	}
	assert(flatfile_atomic_write(root + "/players", std::to_string(pid) + ".snapshot", encoded,
				     &error));
	player_snapshot native{};
	const auto loaded = flatfile_player_snapshot_read(root, pid, &native, &error);
	assert((loaded == flatfile_player_load_result::ok) == expected);
	bool accepted = false;
	size_t item_count = 0;
	try
	{
		const auto independent = restore_native_player::decode_player(encoded, pid);
		accepted = true;
		assert(expected && independent.pid == static_cast<uint32_t>(native.pid) &&
		       independent.revision == native.revision &&
		       independent.death == native.death.has_value() &&
		       independent.pets.size() == native.pets.size());
		const auto compare = [&](const auto &literals, const auto &items)
		{
			assert(literals.size() == items.size());
			for (size_t i = 0; i < items.size(); ++i)
			{
				++item_count;
				const auto &literal = literals[i];
				assert(literal.parent == items[i].parent_index &&
				       literal.equipment == items[i].equipment_slot &&
				       literal.uid == items[i].object_uid &&
				       literal.vnum == items[i].vnum &&
				       literal.type == static_cast<uint8_t>(items[i].type) &&
				       literal.values == items[i].values);
				auto item = items[i];
				item.parent_index = -1;
				std::vector<uint8_t> canonical;
				assert(player_item_snapshot_list_encode({ item }, &canonical) ==
				       player_snapshot_codec_result::ok);
				assert(restore_economic_authority::same(
					std::span(canonical).subspan(8),
					literal.encoded.subspan(4)));
			}
		};
		compare(independent.items, native.items);
		for (size_t i = 0; i < native.pets.size(); ++i)
		{
			assert(independent.pets[i].uid == native.pets[i].pet_uid &&
			       independent.pets[i].hold_reason ==
				       static_cast<uint32_t>(native.pets[i].hold_reason));
			compare(independent.pets[i].items, native.pets[i].items);
		}
		std::vector<uint8_t> generated;
		assert(flatfile_player_snapshot_encode_file(native, &generated));
		const auto roundtrip = restore_native_player::decode_player(generated, pid);
		assert(roundtrip.pid == independent.pid &&
		       roundtrip.revision == independent.revision &&
		       roundtrip.items.size() == independent.items.size() &&
		       roundtrip.pets.size() == independent.pets.size());
		std::ofstream output(root + "/native-generated-player.bin", std::ios::binary);
		output.write(reinterpret_cast<const char *>(generated.data()), generated.size());
		assert(output);
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		(void)restore_native_player::audit(root, budget);
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_player::audit(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << item_count << ",\"item_fields_match\":true}\n";
	return 0;
}

int main(int argc, char **argv)
{
	assert(argc == 4 || argc == 5 || argc == 6);
	const std::string root = argv[1];
	std::ifstream stream(argv[2], std::ios::binary);
	const std::vector<uint8_t> encoded((std::istreambuf_iterator<char>(stream)), {});
	const bool expected = std::string(argv[3]) == "1";
	if (argc >= 5)
	{
		if (std::string(argv[4]) == "player")
			return player(root, encoded, expected, argc == 6 ? std::stoi(argv[5]) : 7);
		if (std::string(argv[4]) == "shopkeeper")
			return shopkeeper(root, encoded, expected);
		if (std::string(argv[4]) == "locker")
			return locker(root, encoded, expected);
		assert(std::string(argv[4]) == "world");
		return world(root, encoded, expected);
	}
	std::filesystem::create_directories(root + "/domains");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0);
	std::string error;
	assert(flatfile_atomic_write(root + "/domains", "item_ownership", encoded, &error));
	std::vector<flatfile_item_ownership_record> native;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		const auto result = flatfile_item_repository_recovery_catalog_locked(
			root, lock, &native, &error);
		assert((result == flatfile_item_repository_result::ok) == expected);
	}
	bool accepted = false;
	try
	{
		const auto independent = restore_native_custody::decode_catalog(encoded);
		accepted = true;
		assert(expected && independent.items.size() == native.size());
		for (size_t i = 0; i < native.size(); ++i)
		{
			const auto &left = independent.items[i];
			const auto &right = native[i];
			assert(left.uid == right.item_uid && left.root == right.root_item_uid &&
			       left.parent == right.parent_item_uid &&
			       left.revision == right.item_revision &&
			       left.location.type == static_cast<uint8_t>(right.owner.type) &&
			       left.location.id == right.owner.id &&
			       left.location.context == right.owner.context_id &&
			       left.vnum == right.vnum &&
			       left.state == static_cast<uint8_t>(right.state) &&
			       left.equipment == right.equipment_slot &&
			       restore_economic_authority::same(left.coin_payload,
								right.coin_payload));
		}
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		const auto summary = restore_native_custody::audit_catalog(root, budget);
		assert(summary.present && summary.items == native.size());
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_custody::audit_catalog(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << native.size() << ",\"item_fields_match\":true}\n";
}
