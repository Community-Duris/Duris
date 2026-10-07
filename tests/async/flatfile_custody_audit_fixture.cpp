#include "../../scripts/qualify_flatfile_native_custody.h"
#include "../../scripts/qualify_flatfile_native_world.h"
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

int main(int argc, char **argv)
{
	assert(argc == 4 || argc == 5);
	const std::string root = argv[1];
	std::ifstream stream(argv[2], std::ios::binary);
	const std::vector<uint8_t> encoded((std::istreambuf_iterator<char>(stream)), {});
	const bool expected = std::string(argv[3]) == "1";
	if (argc == 5)
	{
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
