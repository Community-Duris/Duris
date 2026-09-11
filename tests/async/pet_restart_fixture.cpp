// Offline mutator for synthetic journey fixtures only. Never use on live state.
#include "core/structs.h"
#include "core/defines.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include "player/pet_restore_state.h"
#include <algorithm>
#include <cassert>
#include <ctime>
#include <iostream>
#include <openssl/sha.h>

template <class T> void number(std::vector<uint8_t> &out, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		out.push_back(static_cast<uint64_t>(value) >> (i * 8));
}

int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string root = argv[1], mode = argv[2];
	player_snapshot snapshot;
	std::string error;
	assert(flatfile_player_snapshot_read(root, 1, &snapshot, &error) ==
	       flatfile_player_load_result::ok);
	if (mode == "seed")
	{
		assert(snapshot.pets.empty());
		for (auto &field : snapshot.status_integers)
			if (field.field == player_status_field::level ||
			    field.field == player_status_field::highest_level)
				field.signed_value = field.unsigned_value = 56;
		for (int i = 0; i < 3; ++i)
		{
			player_pet_snapshot pet = {};
			pet.mob_vnum = 1201;
			pet.order = i;
			pet.room_vnum = snapshot.room_vnum;
			pet.hit = pet.max_hit = 1234;
			pet.mana = pet.max_mana = 55;
			pet.vitality = pet.max_vitality = 100;
			pet.charm_duration = i == 1 ? 5 : -1;
			pet_restore_state state;
			state.kind = summoned_pet_kind::undead_first;
			state.name = "skeleton fixture" + std::to_string(i) + " _Taverek_";
			state.short_description = "the skeleton of fixture" + std::to_string(i);
			state.long_description =
				"The skeleton of fixture" + std::to_string(i) + " waits here.\r\n";
			state.level = 21;
			state.race = RACE_SKELETON;
			state.size = SIZE_MEDIUM;
			state.primary_class = CLASS_WARRIOR;
			state.base_stats.fill(95);
			state.base_points = { 1234, 55, 100, -50, 17, 19, 0 };
			state.damage_dice = { 2, 8 };
			state.spell_slots[2] = 7;
			state.act = ACT_ISNPC | ACT_SENTINEL;
			if (i == 1)
			{
				state.charm_expires_at = time(nullptr) + 240;
				state.death_expires_at = time(nullptr) + 300;
			}
			if (i != 2)
				assert(pet_restore_state_encode(state, &pet.restore_state));
			// Move existing authoritative leaf UIDs; never mint or copy custody.
			auto item = std::find_if(snapshot.items.begin(), snapshot.items.end(),
						 [](const auto &row) {
							 return row.parent_index ==
									PLAYER_SNAPSHOT_NO_PARENT &&
								row.type != ITEM_CONTAINER;
						 });
			assert(item != snapshot.items.end());
			const auto index = item - snapshot.items.begin();
			pet.items.push_back(*item);
			snapshot.items.erase(item);
			for (auto &row : snapshot.items)
				if (row.parent_index > index)
					--row.parent_index;
			pet.items[0].equipment_slot = i == 0 ? WEAR_BODY + 1 : 0;
			pet.items[0].affects[0] = { APPLY_HIT, 19 };
			snapshot.pets.push_back(std::move(pet));
		}
	}
	if (mode == "arm-expiry")
	{
		bool armed = false;
		for (auto &pet : snapshot.pets)
		{
			pet_restore_state state;
			if (!pet_restore_state_decode(pet.restore_state, &state) ||
			    !state.death_expires_at)
				continue;
			state.charm_expires_at = time(nullptr) + 12;
			state.death_expires_at = time(nullptr) + 18;
			assert(pet_restore_state_encode(state, &pet.restore_state));
			armed = true;
		}
		assert(armed);
	}
	if (mode == "seed" || mode == "arm-expiry")
	{
		std::vector<uint8_t> payload, bytes;
		snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
		assert(player_snapshot_encode(snapshot, &payload) ==
		       player_snapshot_codec_result::ok);
		using namespace flatfile_player_snapshot_file;
		bytes.insert(bytes.end(), player_magic.begin(), player_magic.end());
		number(bytes, player_file_version);
		number<uint32_t>(bytes, payload.size());
		number(bytes, snapshot.pid);
		number(bytes, snapshot.revision);
		number(bytes, snapshot.components);
		unsigned char digest[SHA256_DIGEST_LENGTH];
		SHA256(payload.data(), payload.size(), digest);
		bytes.insert(bytes.end(), digest, digest + sizeof(digest));
		bytes.insert(bytes.end(), payload.begin(), payload.end());
		assert(flatfile_atomic_write(player_directory(root), player_filename(1), bytes,
					     &error));
	}
	for (const auto &pet : snapshot.pets)
	{
		std::cout << static_cast<unsigned>(pet.hold_reason) << '|' << pet.restore_state
			  << '|' << pet.max_hit;
		for (const auto &item : pet.items)
			std::cout << '|' << item.object_uid;
		std::cout << '\n';
	}
}
