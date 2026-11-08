#include "player/player_snapshot_codec.h"

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
	player_snapshot status = {};
	status.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	status.pid = 42;
	status.revision = 1;
	status.components = PLAYER_COMPONENT_STATUS;
	status.encoded_size_bound = 4096;
	status.status_integers.push_back({ player_status_field::level, 1, 0, false });
	status.status_strings.push_back({ player_status_string_field::name, "Fixture" });
	std::vector<uint8_t> status_bytes;
	require(player_snapshot_encode(status, &status_bytes) == player_snapshot_codec_result::ok,
		"status fixture did not encode");
	player_snapshot decoded_status;
	require(player_snapshot_decode(status_bytes.data(), status_bytes.size(), &decoded_status) ==
				player_snapshot_codec_result::ok &&
			decoded_status.pid == status.pid,
		"status fixture did not decode");
	// Header, integer row count, then the first uint16 field identifier.
	constexpr size_t integer_field_offset = 4 + 4 + 8 + 8 + 4 + 4 + 8 + 4;
	// One integer row has a field, two 64-bit values and a boolean, then string count.
	constexpr size_t string_field_offset = integer_field_offset + 2 + 8 + 8 + 1 + 4;
	for (size_t offset : { integer_field_offset, string_field_offset })
	{
		auto malformed = status_bytes;
		malformed[offset] = 255;
		decoded_status.pid = -123;
		require(player_snapshot_decode(malformed.data(), malformed.size(),
					       &decoded_status) ==
					player_snapshot_codec_result::invalid_value &&
				decoded_status.pid == -123,
			"invalid status identifier reported success or modified the output");
	}
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
	auto forest = original;
	auto other_root = original[0];
	other_root.object_uid = 200;
	other_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	auto other_child = original[1];
	other_child.object_uid = 201;
	other_child.parent_index = 2;
	forest.push_back(other_root);
	forest.push_back(other_child);
	std::vector<player_item_snapshot> selected, remaining;
	require(player_item_snapshot_extract_subtree(forest, 100, &selected, &remaining) ==
				player_snapshot_codec_result::ok &&
			selected.size() == 2 && selected[0].object_uid == 100 &&
			selected[1].parent_index == 0 && remaining.size() == 2 &&
			remaining[0].object_uid == 200 &&
			remaining[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			remaining[1].parent_index == 0,
		"subtree extraction did not normalize both resulting forests");
	auto third_root = original[0];
	third_root.object_uid = 300;
	third_root.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	forest.push_back(third_root);
	require(player_item_snapshot_extract_forest(forest, { 100, 300 }, &selected, &remaining) ==
				player_snapshot_codec_result::ok &&
			selected.size() == 3 && selected[0].object_uid == 100 &&
			selected[1].parent_index == 0 && selected[2].object_uid == 300 &&
			selected[2].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			remaining.size() == 2 && remaining[0].object_uid == 200 &&
			remaining[0].parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
			remaining[1].parent_index == 0,
		"forest extraction did not normalize selected and remaining roots");
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
	// The world's longest item description, 8411 bytes, is saved like any other.
	auto described = original;
	described[0].extra_descriptions[0].spellbook = false;
	described[0].extra_descriptions[0].spell_ids.clear();
	described[0].extra_descriptions[0].description.assign(8411, 'x');
	require(player_item_snapshot_list_encode(described, &encoded) ==
				player_snapshot_codec_result::ok &&
			player_item_snapshot_list_decode(encoded.data(), encoded.size(),
							 &decoded) ==
				player_snapshot_codec_result::ok &&
			decoded[0].extra_descriptions[0].description.size() == 8411,
		"an 8411-byte item description did not round trip");
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
