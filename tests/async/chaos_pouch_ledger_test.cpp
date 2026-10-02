#include "combat/chaos_pouch_ledger.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/vnum.obj.h"
#include "economy/tradeskill.h"
#include "player/player_snapshot_codec.h"
#include "item/craft_pouch_mutation.h"

#include <cassert>
#include <limits>
#include <string_view>

using result = chaos_pouch_ledger_result;
using mode = chaos_pouch_usage_mode;

static player_item_snapshot pouch()
{
	player_item_snapshot item = {};
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.object_uid = 912;
	item.vnum = VOBJ_CHAOS_CRAFT_POUCH;
	item.generated_key = 73;
	item.type = ITEM_CONTAINER;
	item.name = "a pouch with a preserved name";
	item.values[2] = 84;
	item.dynamic_affects.push_back({ 31, 41, 51 });
	item.extra_descriptions.push_back({ "inscription", "preserve this", false, {} });
	return item;
}

static std::vector<uint8_t> bytes(const player_item_snapshot &item)
{
	std::vector<uint8_t> value;
	assert(player_item_snapshot_list_encode({ item }, &value) ==
	       player_snapshot_codec_result::ok);
	return value;
}

static void refused(const player_item_snapshot &before,
		    const std::vector<chaos_material_pouch_usage> &usage, mode kind,
		    result expected = result::invalid)
{
	player_item_snapshot after = pouch();
	after.object_uid = 777;
	const auto untouched = bytes(after);
	assert(chaos_pouch_ledger_prepare(before, usage, kind, &after) == expected);
	assert(bytes(after) == untouched);
}

int main()
{
	const auto before = pouch();
	const auto original = bytes(before);
	const std::vector<chaos_material_pouch_usage> usage = { { LOWEST_MAT_VNUM, 2 },
								{ ENCRUST_VNUM_END, 3 },
								{ LOWEST_MAT_VNUM, 7 } };
	player_item_snapshot after;
	assert(chaos_pouch_ledger_prepare(before, usage, mode::generated, &after) == result::ok);
	assert(bytes(before) == original);
	assert(after.object_uid == before.object_uid &&
	       after.generated_key == before.generated_key);
	assert(after.extra_descriptions.size() == 2);
	assert(after.extra_descriptions[0].keyword == "inscription");
	assert(after.extra_descriptions[0].description == "preserve this");
	assert(after.extra_descriptions[1].keyword == "CHAOS_POUCH_LEDGER_0");
	assert(after.extra_descriptions[1].description == "0:9:0;218:3:0;");
	assert(chaos_pouch_ledger_verify(before, after, usage, mode::generated) == result::ok);
	auto native = before;
	native.generated_key = 84;
	native.values[2] = 99;
	native.extra_descriptions[0].description = "new native inscription";
	player_item_snapshot native_after;
	assert(chaos_pouch_ledger_apply_native(before, after, usage, mode::generated, native,
					       &native_after) == result::ok);
	assert(native_after.generated_key == 84 && native_after.values[2] == 99);
	assert(native_after.extra_descriptions[0].description == "new native inscription");
	assert(native_after.extra_descriptions[1].description == "0:9:0;218:3:0;");
	const auto native_untouched = bytes(native_after);
	assert(chaos_pouch_ledger_apply_native(before, after, usage, mode::generated, after,
					       &native_after) == result::invalid);
	assert(bytes(native_after) == native_untouched);
	craft_pouch_mutation mutation;
	mutation.mode = mode::generated;
	mutation.usage = { { LOWEST_MAT_VNUM, 9 }, { ENCRUST_VNUM_END, 3 } };
	mutation.before = before;
	mutation.after = after;
	std::vector<uint8_t> envelope;
	assert(craft_pouch_mutation_encode(mutation, &envelope));
	craft_pouch_mutation decoded;
	assert(craft_pouch_mutation_decode(envelope, &decoded));
	assert(bytes(decoded.before) == bytes(before) && bytes(decoded.after) == bytes(after));
	for (size_t index = 0; index < envelope.size(); ++index)
	{
		auto corrupt = envelope;
		corrupt[index] ^= 0x80;
		assert(!craft_pouch_mutation_decode(corrupt, &decoded));
	}
	for (size_t length = 0; length < envelope.size(); ++length)
		assert(!craft_pouch_mutation_decode(std::span(envelope).first(length), &decoded));
	auto corrupt = envelope;
	corrupt.push_back(0);
	assert(!craft_pouch_mutation_decode(corrupt, &decoded));
	mutation.usage.push_back(mutation.usage.back());
	assert(!craft_pouch_mutation_encode(mutation, &corrupt));
	assert(corrupt.size() == envelope.size() + 1);
	assert(chaos_pouch_ledger_verify(before, after, usage, mode::collected) == result::invalid);
	for (int mutation = 0; mutation < 5; ++mutation)
	{
		auto changed = after;
		switch (mutation)
		{
		case 0:
			++changed.object_uid;
			break;
		case 1:
			++changed.generated_key;
			break;
		case 2:
			++changed.values[2];
			break;
		case 3:
			changed.extra_descriptions[0].description = "changed";
			break;
		case 4:
			++changed.dynamic_affects[0].data;
			break;
		}
		assert(chaos_pouch_ledger_verify(before, changed, usage, mode::generated) ==
		       result::invalid);
	}
	player_item_snapshot collected;
	assert(chaos_pouch_ledger_prepare(after, usage, mode::collected, &collected) == result::ok);
	assert(collected.extra_descriptions[1].description == "0:9:9;218:3:3;");
	refused(before, {}, mode::generated);
	refused(before, { { LOWEST_MAT_VNUM, 0 } }, mode::generated);
	refused(before, { { HIGHEST_MAT_VNUM + 1, 1 } }, mode::generated);
	refused(before, usage, static_cast<mode>(0));
	auto wrong = before;
	wrong.vnum = LOWEST_MAT_VNUM;
	refused(wrong, usage, mode::generated);
	wrong = before;
	wrong.object_uid = 0;
	refused(wrong, usage, mode::generated);
	wrong = before;
	wrong.parent_index = 0;
	refused(wrong, usage, mode::generated);
	for (const auto record : { "", "-1:0:0;", "219:0:0;", "0:1:0", "0:1:0;garbage",
				   "0:1:0;0:2:0;", "0:18446744073709551616:0;", "0:+1:0;" })
	{
		if (std::string_view(record).empty())
			continue; // Empty legacy ledger chunks are valid.
		wrong = before;
		wrong.extra_descriptions.push_back({ "CHAOS_POUCH_LEDGER_0", record, false, {} });
		refused(wrong, usage, mode::generated);
	}
	for (const auto key :
	     { "CHAOS_POUCH_LEDGER_", "CHAOS_POUCH_LEDGER_00", "CHAOS_POUCH_LEDGER_8",
	       "CHAOS_POUCH_LEDGER_1", "CHAOS_POUCH_LEDGER_0garbage" })
	{
		wrong = before;
		wrong.extra_descriptions.push_back({ key, "0:1:0;", false, {} });
		refused(wrong, usage, mode::generated);
	}
	wrong = after;
	wrong.extra_descriptions.push_back(wrong.extra_descriptions.back());
	refused(wrong, usage, mode::generated);
	wrong = after;
	wrong.extra_descriptions.back().spellbook = true;
	refused(wrong, usage, mode::generated);
	wrong = after;
	wrong.extra_descriptions.back().spell_ids.push_back(1);
	refused(wrong, usage, mode::generated);
	wrong = before;
	wrong.extra_descriptions.push_back(
		{ "CHAOS_POUCH_LEDGER_0", "0:18446744073709551615:0;", false, {} });
	refused(wrong, usage, mode::generated, result::overflow);
	refused(before, { { LOWEST_MAT_VNUM, UINT64_MAX }, { LOWEST_MAT_VNUM, 1 } },
		mode::generated, result::overflow);
	std::vector<chaos_material_pouch_usage> all;
	for (int vnum = LOWEST_MAT_VNUM; vnum <= HIGHEST_MAT_VNUM; ++vnum)
		all.push_back({ vnum, UINT64_MAX });
	for (int vnum = ENCRUST_VNUM_BEGIN; vnum <= ENCRUST_VNUM_END; ++vnum)
		all.push_back({ vnum, UINT64_MAX });
	assert(all.size() == 219);
	assert(chaos_pouch_ledger_prepare(before, all, mode::generated, &after) == result::ok);
	assert(chaos_pouch_ledger_prepare(after, all, mode::collected, &collected) == result::ok);
	assert(collected.extra_descriptions.size() > 2);
	assert(collected.extra_descriptions.size() <= CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS + 1);
	for (size_t index = 1; index < collected.extra_descriptions.size(); ++index)
		assert(collected.extra_descriptions[index].description.size() <=
		       CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES);
	assert(chaos_pouch_ledger_verify(after, collected, all, mode::collected) == result::ok);
	mutation = { mode::collected, all, after, collected };
	assert(craft_pouch_mutation_encode(mutation, &envelope));
	assert(envelope.size() > ITEM_TRANSFER_CONTINUATION_MAX_BYTES);
	assert(craft_pouch_mutation_decode(envelope, &decoded));
	assert(bytes(decoded.after) == bytes(collected));
	all.push_back({ LOWEST_MAT_VNUM, 1 });
	refused(before, all, mode::generated);
	return 0;
}
