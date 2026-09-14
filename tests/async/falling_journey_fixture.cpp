// Offline mutator for synthetic falling journey state only.
#include "core/defines.h"
#include "magic/spells.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include <algorithm>
#include <cassert>
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
	if (mode == "inspect")
	{
		for (const auto &field : snapshot.status_integers)
			if (field.field == player_status_field::hit_difference)
				std::cout << snapshot.room_vnum << ' ' << field.signed_value
					  << '\n';
		return 0;
	}
	assert(mode == "unskilled" || mode == "safe" || mode == "climb-zero");
	snapshot.room_vnum = 22800;
	for (auto &field : snapshot.status_integers)
	{
		if (field.field == player_status_field::class_primary)
			field.signed_value = field.unsigned_value = CLASS_THIEF;
		if (field.field == player_status_field::base_hit)
			field.signed_value = field.unsigned_value = 200000;
		if (field.field == player_status_field::hit_difference)
			field.signed_value = field.unsigned_value = 0;
	}
	snapshot.skills.erase(std::remove_if(snapshot.skills.begin(), snapshot.skills.end(),
					     [](const auto &row) {
						     return row.skill_id == SKILL_SAFE_FALL ||
							    row.skill_id == SKILL_CLIMB;
					     }),
			      snapshot.skills.end());
	// Login enforces class eligibility and clamps learned to the class maximum.
	// A level-one thief keeps 1 (always fails the strict check) or 100. The
	// journey accepts a failed skill roll and retries that case, but never
	// accepts increased damage as a failed roll.
	snapshot.skills.push_back(
		{ SKILL_SAFE_FALL, static_cast<uint8_t>(mode == "safe" ? 100 : 1), 100 });
	snapshot.skills.push_back({ SKILL_CLIMB, 0, 0 });
	snapshot.affects.clear();
	if (mode == "climb-zero")
	{
		player_affect_snapshot climb = {};
		climb.type = SKILL_CLIMB;
		climb.duration = -1;
		climb.level = 1;
		snapshot.affects.push_back(climb);
	}
	std::vector<uint8_t> payload, bytes;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	assert(player_snapshot_encode(snapshot, &payload) == player_snapshot_codec_result::ok);
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
	assert(flatfile_atomic_write(player_directory(root), player_filename(1), bytes, &error));
}
