#include "player_snapshot_codec.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static std::vector<player_item_snapshot> fixture()
{
	player_item_snapshot parent = {};
	parent.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	parent.equipment_slot = -1;
	parent.object_uid = 100;
	parent.generated_key = 200;
	parent.vnum = 300;
	parent.type = 4;
	parent.string_mask = 15;
	parent.name = "container";
	parent.short_description = "a container";
	parent.description = "A container is here.";
	parent.action_description = "open";
	parent.values[0] = 11;
	parent.timers[0] = 12;
	parent.wear_flags = 13;
	parent.extra_flags = 14;
	parent.anti_flags = 15;
	parent.anti2_flags = 16;
	parent.extra2_flags = 17;
	parent.weight = 18;
	parent.material = 19;
	parent.cost = 20;
	parent.condition = 21;
	parent.craftsmanship = 22;
	parent.bitvectors[0] = 23;
	parent.affects[0] = { 24, 25 };
	parent.dynamic_affects.push_back({ 26, 27, 28 });
	parent.extra_descriptions.push_back({ "runes", "glowing runes", true, { 29, 30 } });
	player_item_snapshot child = {};
	child.parent_index = 0;
	child.equipment_slot = -1;
	child.object_uid = 101;
	child.generated_key = 201;
	child.vnum = 301;
	child.name = "child";
	return { parent, child };
}

int main()
{
	const auto original = fixture();
	std::vector<uint8_t> encoded;
	require(player_item_snapshot_list_encode(original, &encoded) ==
				player_snapshot_codec_result::ok &&
			!encoded.empty(),
		"item list did not encode");
	std::vector<player_item_snapshot> decoded;
	require(player_item_snapshot_list_decode(encoded.data(), encoded.size(), &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded.size() == 2 && decoded[0].object_uid == 100 &&
			decoded[0].name == "container" && decoded[0].values[0] == 11 &&
			decoded[0].timers[0] == 12 && decoded[0].bitvectors[0] == 23 &&
			decoded[0].affects[0][1] == 25 && decoded[0].dynamic_affects.size() == 1 &&
			decoded[0].dynamic_affects[0].extra2 == 28 &&
			decoded[0].extra_descriptions.size() == 1 &&
			decoded[0].extra_descriptions[0].spell_ids ==
				std::vector<int32_t>{ 29, 30 } &&
			decoded[1].parent_index == 0 && decoded[1].object_uid == 101,
		"item list did not round trip every nested field");
	require(player_item_snapshot_list_decode(encoded.data(), encoded.size() - 1, &decoded) ==
			player_snapshot_codec_result::truncated,
		"truncated item list was accepted");
	auto trailing = encoded;
	trailing.push_back(0);
	require(player_item_snapshot_list_decode(trailing.data(), trailing.size(), &decoded) ==
			player_snapshot_codec_result::invalid_value,
		"trailing item-list bytes were accepted");
	auto invalid_parent = original;
	invalid_parent[1].parent_index = 1;
	require(player_item_snapshot_list_encode(invalid_parent, &encoded) ==
			player_snapshot_codec_result::invalid_value,
		"self-parented item was encoded");
	std::vector<player_item_snapshot> too_deep;
	for (size_t index = 0; index <= PLAYER_SNAPSHOT_MAX_DEPTH; ++index)
	{
		player_item_snapshot item = {};
		item.parent_index = index ? static_cast<int32_t>(index - 1) :
					    PLAYER_SNAPSHOT_NO_PARENT;
		too_deep.push_back(item);
	}
	require(player_item_snapshot_list_encode(too_deep, &encoded) ==
			player_snapshot_codec_result::invalid_value,
		"over-depth item tree was encoded");
	auto oversized = original;
	oversized[0].name.assign(PLAYER_SNAPSHOT_MAX_STRING_BYTES + 1, 'x');
	require(player_item_snapshot_list_encode(oversized, &encoded) ==
			player_snapshot_codec_result::limit_exceeded,
		"oversized item string was encoded");
	std::vector<player_item_snapshot> empty;
	require(player_item_snapshot_list_encode(empty, &encoded) ==
				player_snapshot_codec_result::ok &&
			player_item_snapshot_list_decode(encoded.data(), encoded.size(),
							 &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded.empty(),
		"empty item list did not round trip");
	std::cout << "player item snapshot codec passed\n";
	return 0;
}
