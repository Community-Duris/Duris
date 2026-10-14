#include "player/player_snapshot_codec.h"
#include "core/files.h"
#include "core/defines.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

// The baseline mode selects only the test harness's API calls. It does not
// change any codec/provider, production ABI or unsupported Source-policy macro.
namespace
{
using result = player_snapshot_codec_result;

void require(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

critical_operation_id operation(uint8_t tag)
{
	critical_operation_id id = {};
	for (size_t index = 0; index < id.bytes.size(); ++index)
		id.bytes[index] = static_cast<uint8_t>(tag + index + 1);
	return id;
}

std::vector<player_item_snapshot> tree(uint64_t first_uid)
{
	std::vector<player_item_snapshot> items(4);
	for (size_t index = 0; index < items.size(); ++index)
	{
		auto &item = items[index];
		item.parent_index = index == 0 ? PLAYER_SNAPSHOT_NO_PARENT : index == 2 ? 1 : 0;
		item.equipment_slot = index == 0 ? 3 : -1;
		item.object_uid = first_uid + index;
		item.generated_key = static_cast<int64_t>(700 + index);
		item.vnum = static_cast<int32_t>(300 + index);
		item.type = ITEM_CONTAINER;
		item.string_mask = 15;
		item.name = "nested runic container " + std::to_string(index);
		item.short_description = "a layered container";
		item.description = "A layered container lies here.\n";
		item.action_description = std::string("open\0close", 10);
		for (size_t field = 0; field < item.values.size(); ++field)
			item.values[field] = static_cast<int32_t>(field * 13 + index);
		for (size_t field = 0; field < item.timers.size(); ++field)
			item.timers[field] = static_cast<int64_t>(9000000000ULL + field + index);
		item.wear_flags = 3;
		item.extra_flags = 5;
		item.anti_flags = 7;
		item.anti2_flags = 11;
		item.extra2_flags = 13;
		item.weight = 17;
		item.material = 2;
		item.cost = 123456;
		item.condition = 91;
		item.craftsmanship = 37;
		item.bitvectors = { 1, 2, 4, 8, UINT64_C(0x8000000000000000) };
		item.affects[0] = { 1, -2 };
		item.affects[1] = { 3, 4 };
		item.affects[2] = { 5, -6 };
		item.affects[3] = { 7, 8 };
		item.dynamic_affects = { { 9, -10, UINT64_C(0xfedcba9876543210) }, { 11, 12, 13 } };
		item.extra_descriptions = { { "runes", "first\nsecond", true, { 29, 31 } },
					    { "seal", std::string("a\0b", 3), false, {} } };
	}
	return items;
}

player_death_evidence_table table(std::initializer_list<const char *> names)
{
	player_death_evidence_table value;
	for (const auto *name : names)
		value.columns.emplace_back(name);
	player_death_evidence_row row;
	for (size_t index = 0; index < value.columns.size(); ++index)
		row.emplace_back(
			index == 0 ? std::optional<std::string>{} :
			index == 1 ? std::optional<std::string>{ "" } :
				     std::optional<std::string>{ std::string("raw\0bytes", 9) });
	value.rows.push_back(row);
	value.rows.push_back(std::move(row));
	return value;
}

player_snapshot fixture(uint32_t schema, bool wards)
{
	player_snapshot snapshot = {};
	snapshot.schema_version = schema;
	snapshot.pid = 137;
	snapshot.revision = 23;
	snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
	snapshot.save_intent = 1;
	snapshot.room_vnum = 401;
	snapshot.encoded_size_bound = 16384;
	snapshot.status_integers = { { player_status_field::mana, -91, 0, false },
				     { player_status_field::experience, 0,
				       UINT64_C(0xfedcba9876543210), true },
				     { player_status_field::played_time, 9000000000LL, 0, false } };
	snapshot.status_strings = { { player_status_string_field::name, "Wire Keeper" },
				    { player_status_string_field::description,
				      std::string("long\0text", 9) },
				    { player_status_string_field::title, "the keeper\n" } };
	snapshot.conditions = { -1, 2, -3, 4, 5 };
	for (size_t index = 0; index < snapshot.quest_values.size(); ++index)
		snapshot.quest_values[index] = static_cast<int32_t>(index * 17);
	snapshot.languages = { { 1, -2, 3 }, { 4, 5, 6 } };
	snapshot.introductions = { { 7, 8, 9 } };
	snapshot.timers = { { 10, -9000000000LL, 11 } };
	snapshot.undead_slots = { { 12, 13, 14 } };
	snapshot.forged_items = { { 15, 16, 17 } };
	snapshot.granted_commands = { 18, 19, 20 };
	snapshot.skills = { { 21, 78, 79 }, { 22, 80, 81 } };
	player_affect_snapshot affect = {};
	affect.type = 23;
	affect.duration = 24;
	affect.flags = 25;
	affect.modifier = -26;
	affect.location = 2;
	affect.level = 28;
	affect.bitvectors = { 31, 32, 33, 34, UINT64_C(0x8000000000000000) };
	affect.wear_off_character = "A ward fades.\n";
	affect.wear_off_room = std::string("room\0echo", 9);
	if (wards)
	{
		affect.ward_source_uid = 901;
		affect.ward_full_duration = 41;
		affect.ward_capacity = 42;
		affect.ward_capacity_max = 43;
		affect.ward_refresh_remaining = 44;
		affect.ward_source_type = 1;
		affect.ward_source_worn = 1;
		affect.ward_active = 1;
	}
	snapshot.affects.push_back(affect);
	affect.ward_source_type = 0;
	affect.ward_source_uid = 0;
	snapshot.affects.push_back(affect);
	snapshot.items = tree(1000);
	player_pet_snapshot pet = {};
	pet.pet_uid = 2000;
	pet.mob_vnum = 701;
	pet.order = 2;
	pet.hit = 30;
	pet.max_hit = 40;
	pet.mana = 50;
	pet.max_mana = 60;
	pet.vitality = 70;
	pet.max_vitality = 80;
	pet.charm_duration = 90;
	pet.room_vnum = 401;
	pet.items = tree(2100);
	pet.restore_state = std::string("state\0bytes", 11);
	snapshot.pets.push_back(pet);
	pet.pet_uid = 2200;
	pet.items = tree(2300);
	pet.hold_reason = pet_hold_reason::missing_prototype;
	snapshot.pets.push_back(std::move(pet));
	snapshot.shapes = { { 801, 3, 9000000000LL, 9000000001LL } };
	snapshot.trophies = { { 17, 1234 }, { 18, 4321 } };
	snapshot.recipes_are_external = true;
	snapshot.output_preferences = "{\"ansi\":true,\"width\":100}";
	if (player_snapshot_has_quest_receipt_schema(schema))
		snapshot.quest_xp_receipts = { { operation(10), 3, 100 },
					       { operation(11), 4, 200 } };
	if (player_snapshot_has_spell_receipt_schema(schema))
		snapshot.spell_effect_receipts = { { operation(20), 1 }, { operation(21), 6 } };
	if (player_snapshot_has_craft_receipt_schema(schema))
		snapshot.craft_receipts = { { operation(30), 1, 123 }, { operation(31), 6, 0 } };
	if (player_snapshot_is_death_request_schema(schema) ||
	    player_snapshot_is_death_evidence_schema(schema))
	{
		snapshot.save_intent = RENT_DEATH;
		snapshot.items.clear();
		for (auto &held : snapshot.pets)
			held.hold_reason = pet_hold_reason::custody_pending;
		player_death_snapshot death = {};
		death.operation_id = operation(40);
		death.corpse_room_vnum = 402;
		death.wallet_revision = 24;
		death.wallet_before = { 2, 3, 5, 7 };
		death.corpse = tree(3000);
		auto &corpse = death.corpse.front();
		corpse.equipment_slot = -1;
		corpse.vnum = VOBJ_CORPSE;
		corpse.type = ITEM_CORPSE;
		corpse.values[CORPSE_PID] = snapshot.pid;
		corpse.values[CORPSE_SAVEID] = 123;
		corpse.values[CORPSE_FLAGS] = PC_CORPSE;
		auto &coins = death.corpse.back();
		coins.vnum = VOBJ_COINS;
		coins.type = ITEM_MONEY;
		std::copy(death.wallet_before.begin(), death.wallet_before.end(),
			  coins.values.begin());
		death.wallet_pile_uid = coins.object_uid;
		for (size_t index = 1; index < death.corpse.size(); ++index)
		{
			const auto &item = death.corpse[index];
			player_death_custody_snapshot custody = {};
			custody.item = {
				item.object_uid,
				corpse.object_uid,
				death.corpse[static_cast<size_t>(item.parent_index)].object_uid,
				3,
				item.vnum,
				item_custody_state::active
			};
			custody.owner = { item_owner_type::player,
					  static_cast<uint64_t>(snapshot.pid), 0 };
			custody.owner_revision = 2;
			if (index == 2)
			{
				custody.item.expected_state = item_custody_state::absent;
				custody.item.expected_item_revision = ITEM_TRANSFER_ABSENT_REVISION;
				custody.owner = {};
				custody.owner_revision = 0;
			}
			death.custody.push_back(custody);
		}
		death.unresolved_operations = { operation(50), operation(51) };
		if (player_snapshot_is_death_evidence_schema(schema))
		{
			player_death_conflict_evidence evidence;
			evidence.player_items =
				table({ "id", "pid", "obj_uid", "vnum", "container_id" });
			evidence.player_item_affects =
				table({ "id", "item_id", "location", "modifier" });
			evidence.player_item_extra_descr =
				table({ "id", "item_id", "keyword", "description" });
			evidence.item_current_owner = table(
				{ "item_uid", "root_item_uid", "parent_item_uid", "item_revision",
				  "vnum", "state", "owner_type", "owner_id", "owner_context_id" });
			evidence.item_owner_revision =
				table({ "owner_type", "owner_id", "owner_context_id", "revision" });
			death.conflict_evidence = std::move(evidence);
		}
		snapshot.death = std::move(death);
	}
	return snapshot;
}

std::vector<std::pair<std::string, player_snapshot>> cases()
{
	std::vector<std::pair<std::string, player_snapshot>> values;
	for (uint32_t schema : { 7U, 8U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U })
		for (bool wards : { false, true })
			values.emplace_back("schema" + std::to_string(schema) +
						    (wards ? "-ward" : "-plain"),
					    fixture(schema, wards));
	auto changed = fixture(17, true);
	changed.pid += 29;
	changed.revision += 31;
	changed.room_vnum += 37;
	changed.encoded_size_bound += 41;
	changed.conditions[1] = -500;
	changed.status_strings[1].value += "changed";
	values.emplace_back("changed-metadata", std::move(changed));
	changed = fixture(17, true);
	changed.items[2].parent_index = 0;
	changed.pets[0].items[2].parent_index = 0;
	values.emplace_back("changed-relationships", std::move(changed));
	return values;
}

std::vector<uint8_t> original(const player_snapshot &snapshot)
{
	std::vector<uint8_t> bytes;
	require(player_snapshot_encode(snapshot, &bytes) == result::ok,
		"complete original encode failed");
	return bytes;
}

void roundtrip(const player_snapshot &snapshot, const std::vector<uint8_t> &bytes)
{
	player_snapshot decoded = {};
	require(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) == result::ok,
		"full decode failed");
	require(original(decoded) == bytes, "full wire roundtrip changed bytes");
	require(decoded.pid == snapshot.pid && decoded.revision == snapshot.revision &&
			decoded.components == snapshot.components &&
			decoded.schema_version == snapshot.schema_version &&
			decoded.status_strings[1].value == snapshot.status_strings[1].value &&
			decoded.affects[0].ward_source_type ==
				snapshot.affects[0].ward_source_type &&
			decoded.pets[0].items[2].parent_index ==
				snapshot.pets[0].items[2].parent_index &&
			decoded.pets[0].restore_state == snapshot.pets[0].restore_state &&
			decoded.quest_xp_receipts.size() == snapshot.quest_xp_receipts.size() &&
			decoded.spell_effect_receipts.size() ==
				snapshot.spell_effect_receipts.size() &&
			decoded.craft_receipts.size() == snapshot.craft_receipts.size(),
		"complete branch readback mismatch");
	if (snapshot.death)
		require(decoded.death &&
				decoded.death->custody.size() == snapshot.death->custody.size() &&
				decoded.death->wallet_pile_uid == snapshot.death->wallet_pile_uid &&
				decoded.death->conflict_evidence.has_value() ==
					snapshot.death->conflict_evidence.has_value(),
			"death branch readback mismatch");
	else
		require(decoded.items[2].parent_index == snapshot.items[2].parent_index &&
				decoded.items[0].extra_descriptions[1].description ==
					snapshot.items[0].extra_descriptions[1].description,
			"nested player item readback mismatch");
}

#if !defined(DURIS_FULL_ENCODER_BASELINE)
struct reservation
{
	size_t calls = 0, deny = 0, peak = 0, outer = 0;
	bool rejected = false;
	static bool admit(size_t bytes, void *opaque) noexcept
	{
		auto &budget = *static_cast<reservation *>(opaque);
		++budget.calls;
		budget.peak = std::max(budget.peak, bytes);
		require(bytes >= budget.outer, "callback lost outer-live bytes");
		budget.rejected = budget.rejected || (budget.deny && budget.calls == budget.deny);
		return !budget.rejected;
	}
};

size_t outer(const player_snapshot &snapshot, const std::vector<uint8_t> &output)
{
	size_t heap = 0;
	require(player_snapshot_current_heap_bytes(snapshot, &heap),
		"actual complete CURRENT provider refused");
	return sizeof(snapshot) + heap + sizeof(output) + output.capacity() + sizeof(reservation);
}

void refusal_controls(const player_snapshot &snapshot)
{
	const auto input_wire = original(snapshot);
	std::vector<uint8_t> successful;
	reservation accepted;
	accepted.outer = outer(snapshot, successful);
	require(player_snapshot_encode_bounded(snapshot, &successful, reservation::admit, &accepted,
					       accepted.outer) == result::ok &&
			successful == input_wire && accepted.calls,
		"complete callback trace failed");
	for (size_t ordinal = 1; ordinal <= accepted.calls; ++ordinal)
	{
		std::vector<uint8_t> output = { 9, 8, 7, 6 };
		output.reserve(67);
		const auto prior = output;
		const auto *data = output.data();
		const size_t capacity = output.capacity();
		reservation denied;
		denied.deny = ordinal;
		denied.outer = outer(snapshot, output);
		errno = 0;
		require(player_snapshot_encode_bounded(snapshot, &output, reservation::admit,
						       &denied, denied.outer) ==
					result::allocation_failure &&
				denied.rejected && denied.calls >= ordinal,
			"an actual callback denial was not propagated");
		require(output == prior && output.data() == data && output.capacity() == capacity,
			"callback refusal altered prior output/storage");
	}
	require(original(snapshot) == input_wire, "callback refusals altered complete input");
	std::cout << "all-callback-denials schema=" << snapshot.schema_version
		  << " count=" << accepted.calls << '\n';
}

void invalid_controls()
{
	auto snapshot = fixture(17, true);
	const auto input_wire = original(snapshot);
	for (unsigned negative = 0; negative != 9; ++negative)
	{
		auto invalid = snapshot;
		std::vector<uint8_t> output = { 1, 3, 5 };
		output.reserve(71);
		const auto prior = output;
		const auto *data = output.data();
		const size_t capacity = output.capacity();
		reservation budget;
		budget.outer = outer(invalid, output);
		switch (negative)
		{
		case 0:
			invalid.pid = 0;
			break;
		case 1:
			invalid.revision = 0;
			break;
		case 2:
			invalid.components |= UINT64_C(1) << 63;
			break;
		case 3:
			invalid.items[1].parent_index = 1;
			break;
		case 4:
			invalid.pets[0].items[1].parent_index = 3;
			break;
		case 5:
			invalid.quest_xp_receipts[1] = invalid.quest_xp_receipts[0];
			break;
		case 6:
			invalid.spell_effect_receipts[1] = invalid.spell_effect_receipts[0];
			break;
		case 7:
			invalid.craft_receipts[0].discipline = 0;
			break;
		case 8:
			invalid.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES + 1;
			break;
		}
		require(player_snapshot_encode_bounded(invalid, &output, reservation::admit,
						       &budget, budget.outer) != result::ok,
			"invalid metadata/relationships/receipt accepted");
		require(output == prior && output.data() == data && output.capacity() == capacity,
			"invalid encode changed output");
	}
	std::vector<uint8_t> output = { 2, 4, 6 };
	output.reserve(79);
	const auto prior = output;
	const auto *data = output.data();
	const size_t capacity = output.capacity();
	reservation budget;
	require(player_snapshot_encode_bounded(snapshot, nullptr, reservation::admit, &budget, 0) ==
				result::invalid_value &&
			budget.calls == 0,
		"null output was not refused before callback");
	require(player_snapshot_encode_bounded(snapshot, &output, nullptr, &budget, 0) ==
				result::allocation_failure &&
			output == prior && budget.calls == 0,
		"null callback changed output");
	require(player_snapshot_encode_bounded(snapshot, &output, reservation::admit, &budget,
					       std::numeric_limits<size_t>::max()) ==
				result::allocation_failure &&
			output == prior && budget.calls == 0,
		"outer-live overflow was not refused before callback");
	require(output.data() == data && output.capacity() == capacity,
		"null/overflow refusal altered prior output storage");
	require(original(snapshot) == input_wire, "invalid/null/overflow controls altered input");
}
#endif
} // namespace

int main(int argc, char **argv)
{
	require(argc == 2, "expected golden directory argument");
	const std::filesystem::path directory(argv[1]);
	const auto snapshots = cases();
	const auto unchanged = original(fixture(17, true));
	require(original(snapshots[snapshots.size() - 2].second) != unchanged &&
			original(snapshots.back().second) != unchanged,
		"metadata/relationship variants did not change original canonical bytes");
#if defined(DURIS_FULL_ENCODER_BASELINE)
	std::filesystem::create_directories(directory);
#else
	size_t frame = 0;
	require(player_snapshot_encode_source_frame_bytes(&frame) && frame &&
			player_snapshot_encode_initial_inline_bytes(&frame),
		"ordinary GNU13 Source policy unavailable");
#endif
	for (const auto &[name, snapshot] : snapshots)
	{
		const auto reference = original(snapshot);
		roundtrip(snapshot, reference);
		const auto path = directory / (name + ".bin");
#if defined(DURIS_FULL_ENCODER_BASELINE)
		std::ofstream stream(path, std::ios::binary);
		stream.write(reinterpret_cast<const char *>(reference.data()),
			     static_cast<std::streamsize>(reference.size()));
		require(stream.good(), "original golden write failed");
#else
		std::ifstream stream(path, std::ios::binary);
		require(stream.good(), "independent original golden is missing");
		const std::vector<uint8_t> golden{ std::istreambuf_iterator<char>(stream),
						   std::istreambuf_iterator<char>() };
		require(golden == reference,
			"source209 original traversal changed canonical golden bytes");
		std::vector<uint8_t> bounded = { 91, 92 };
		reservation budget;
		budget.outer = outer(snapshot, bounded);
		require(player_snapshot_encode_bounded(snapshot, &bounded, reservation::admit,
						       &budget, budget.outer) == result::ok &&
				bounded == golden,
			"bounded complete wire differs from independent canonical golden");
		roundtrip(snapshot, bounded);
#endif
		std::cout << name << " bytes=" << reference.size() << '\n';
	}
#if !defined(DURIS_FULL_ENCODER_BASELINE)
	invalid_controls();
	refusal_controls(fixture(17, true));
	refusal_controls(fixture(19, true));
#endif
	std::cout
		<< "complete codec/value regression PASS; no ownership/SQL/gameplay qualification\n";
}
