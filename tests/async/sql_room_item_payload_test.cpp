#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <memory>

static player_item_snapshot item(uint64_t uid, int32_t parent)
{
	player_item_snapshot result{};
	result.parent_index = parent;
	result.object_uid = uid;
	result.vnum = 1200;
	result.type = ITEM_CONTAINER;
	result.string_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3;
	result.name = "literal keys ' quote";
	result.short_description = "literal short";
	result.description = "literal room\r\ntext";
	result.action_description = "literal action";
	result.generated_key = INT64_C(9223372036854700000);
	result.values = { 1, 2, 3, 4, 5, 6, 7, 8 };
	result.timers = { 1, 2, 3, 4, 5, INT64_C(9007199254740993) };
	result.wear_flags = 17;
	result.extra_flags = 19;
	result.anti_flags = 23;
	result.anti2_flags = 29;
	result.extra2_flags = 31;
	result.weight = 37;
	result.material = 2;
	result.cost = 41;
	result.condition = 43;
	result.craftsmanship = 47;
	result.bitvectors = { 53, 59, 61, 67, 71 };
	result.affects = { { { 1, 3 }, { 2, 5 }, { 3, 7 }, { 4, 11 } } };
	result.dynamic_affects = { { 5, 13, UINT64_C(9007199254740995) } };
	result.extra_descriptions.push_back({ "literal keyword", "literal extra text", false, {} });
	return result;
}

static std::unique_ptr<item_transfer_payload>
command(const std::vector<player_item_snapshot> &items)
{
	auto result = std::make_unique<item_transfer_payload>();
	result->reason = item_transfer_reason::player_drop;
	result->reason_id = 120;
	result->from_owner = { item_owner_type::player, 7, 0 };
	result->to_owner = { item_owner_type::room, 120, 0 };
	result->selected_item_uid = items[0].object_uid;
	result->target_root_item_uid = items[0].object_uid;
	result->item_count = items.size();
	std::vector<uint8_t> bytes;
	assert(player_item_snapshot_list_encode(items, &bytes) == player_snapshot_codec_result::ok);
	assert(bytes.size() <= result->item_blob.size());
	result->item_blob_size = bytes.size();
	std::copy(bytes.begin(), bytes.end(), result->item_blob.begin());
	for (size_t index = 0; index < items.size(); ++index)
	{
		auto &entry = result->items[index];
		entry.item_uid = items[index].object_uid;
		entry.root_item_uid = items[0].object_uid;
		entry.parent_item_uid = items[index].parent_index < 0 ?
						0 :
						items[items[index].parent_index].object_uid;
		entry.vnum = items[index].vnum;
		entry.expected_item_revision = 10 + index;
		entry.expected_state = item_custody_state::active;
	}
	return result;
}

static void unchanged_refusal(const item_transfer_payload &payload)
{
	sql_room_item_payload_batch sentinel;
	sentinel.session_id = 999;
	sentinel.season_epoch = 123;
	sentinel.payloads = { { 4, 5, 6 } };
	assert(!sql_room_item_payload_capture(payload, &sentinel));
	assert(sentinel.session_id == 999 && sentinel.season_epoch == 123 &&
	       sentinel.payloads == std::vector<std::vector<uint8_t>>({ { 4, 5, 6 } }));
}

int main()
{
	std::vector<player_item_snapshot> items = { item(100, -1), item(101, 0), item(102, 1) };
	auto payload = command(items);
	sql_room_item_payload_batch batch;
	assert(sql_room_item_payload_capture(*payload, &batch));
	assert(batch.items.size() == 3 && batch.payloads.size() == 3);
	for (size_t index = 0; index < items.size(); ++index)
	{
		std::vector<player_item_snapshot> decoded;
		assert(player_item_snapshot_list_decode(batch.payloads[index].data(),
							batch.payloads[index].size(), &decoded) ==
		       player_snapshot_codec_result::ok);
		auto expected = items[index];
		expected.parent_index = -1;
		std::vector<uint8_t> bytes;
		assert(player_item_snapshot_list_encode({ expected }, &bytes) ==
		       player_snapshot_codec_result::ok);
		assert(bytes == batch.payloads[index] &&
		       decoded[0].object_uid == items[index].object_uid);
	}
	payload->reason_id = 0;
	unchanged_refusal(*payload);
	payload->reason_id = 120;
	payload->items[1].parent_item_uid = 102;
	unchanged_refusal(*payload);
	payload->items[1].parent_item_uid = 100;
	payload->items[2].item_uid = 101;
	unchanged_refusal(*payload);
	payload->items[2].item_uid = 102;
	payload->items[1].expected_item_revision = UINT64_MAX;
	unchanged_refusal(*payload);
	payload->items[1].expected_item_revision = 11;
	payload->multi_root = true;
	unchanged_refusal(*payload);
	payload->multi_root = false;
	payload->target_parent_item_uid = 400;
	unchanged_refusal(*payload);
	payload->target_parent_item_uid = 0;
	payload->from_owner.context_id = 1;
	unchanged_refusal(*payload);
	payload->from_owner.context_id = 0;
	payload->item_blob[0] ^= 1;
	unchanged_refusal(*payload);
	payload->item_blob[0] ^= 1;
	items[1].string_mask = 0;
	items[1].name.clear();
	items[1].short_description.clear();
	items[1].description.clear();
	items[1].action_description.clear();
	unchanged_refusal(*command(items));
	items[1] = item(101, 0);
	items[1].type = ITEM_MONEY;
	unchanged_refusal(*command(items));
	items[1] = item(101, 0);
	items[1].type = ITEM_CORPSE;
	unchanged_refusal(*command(items));
	items[1] = item(101, 0);
	items[1].extra_flags |= ITEM_ARTIFACT;
	unchanged_refusal(*command(items));
	items[1] = item(101, 0);
	items[1].equipment_slot = 1;
	unchanged_refusal(*command(items));
	items = { item(100, -1), item(101, 0), item(102, 0) };
	payload = command(items);
	items[2].object_uid = 101;
	std::vector<uint8_t> duplicate_capture;
	assert(player_item_snapshot_list_encode(items, &duplicate_capture) ==
	       player_snapshot_codec_result::ok);
	payload->item_blob_size = duplicate_capture.size();
	std::copy(duplicate_capture.begin(), duplicate_capture.end(), payload->item_blob.begin());
	unchanged_refusal(*payload); // entries remain distinct; captured graph does not

	// The aggregate transfer at its exact cap remains readable when separately
	// stored rows add repeated count headers. Capture/read share the same budget.
	items.clear();
	for (size_t index = 0; index < 64; ++index)
	{
		auto next = item(1000 + index, index ? 0 : -1);
		next.name = next.short_description = next.description = next.action_description =
			std::string(400, 'x');
		items.push_back(std::move(next));
	}
	std::vector<uint8_t> bytes;
	assert(player_item_snapshot_list_encode(items, &bytes) == player_snapshot_codec_result::ok);
	assert(bytes.size() < ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES);
	size_t remaining = ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES - bytes.size();
	for (auto &next : items)
	{
		const size_t add =
			std::min(remaining, PLAYER_SNAPSHOT_MAX_STRING_BYTES - next.name.size());
		next.name.append(add, 'z');
		remaining -= add;
	}
	assert(remaining == 0);
	payload = command(items);
	assert(payload->item_blob_size == ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES);
	assert(sql_room_item_payload_capture(*payload, &batch));
	size_t stored = 0;
	for (const auto &blob : batch.payloads)
		stored += blob.size();
	assert(stored > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES &&
	       stored <= SQL_ROOM_ITEM_GRAPH_MAX_BYTES);
	std::puts(
		"exact payload capture, complete nested graph, refusal preservation and stored-byte boundary passed");
}
