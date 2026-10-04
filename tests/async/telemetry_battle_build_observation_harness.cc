#include "telemetry/telemetry_battle_build_observation.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string_view>
#include <type_traits>

namespace
{
telemetry_battle_build_context profile()
{
	telemetry_battle_build_context v{};
	v.actor = { 8571U, telemetry_combat_actor_kind::player };
	v.config_id = 333U;
	v.build_version = 7U;
	v.content_version = 11U;
	v.version = 1U;
	v.available = 1023U;
	v.quality = TELEMETRY_BUILD_SUPPORT_ORIGIN_UNKNOWN;
	v.primary_class_mask = 0x20000001U;
	v.secondary_class_mask = 0x10000002U;
	v.specialization = 3U;
	v.level = 56U;
	v.race = 1U;
	v.faction = 2U;
	for (std::size_t index = 0U; index < 10U; ++index)
	{
		v.base.stats[index] = 110 + index;
		v.effective.stats[index] = 230 + index;
	}
	v.base.resources[0] = 70000;
	v.effective.resources[0] = 123456;
	v.current_resources[0] = -2;
	v.base.combat[0] = 100;
	v.effective.combat[0] = -15;
	v.saving_modifiers[0] = -128;
	v.saving_modifiers[4] = 127;
	v.effective_flags[0] = 0x1234U;
	v.equipment.occupied_slots = 3U;
	v.equipment.melee_weapons = 1U;
	v.equipment.shields = 1U;
	v.equipment.armor = 1U;
	v.equipment.items_with_dynamic_affects = 1U;
	v.equipment.direct_modifiers[0] = 13;
	v.equipment.direct_modifiers[1] = -2;
	v.equipment.flags[4] = std::numeric_limits<std::uint64_t>::max();
	for (std::size_t index = 0U; index < 32U; ++index)
	{
		v.equipment.fixed_feature_digest[index] = index + 1U;
		v.epics.learned_build_digest[index] = 255U - index;
	}
	v.epics.catalog_skills = 2U;
	v.epics.learned_skills = 1U;
	v.listed_affects.observed_nodes = 3U;
	v.listed_affects.offensive_modifier_nodes = 1U;
	v.listed_affects.armor_modifier_nodes = 1U;
	v.listed_affects.unapplied_nodes = 1U;
	v.listed_affects.complete = 1U;
	v.arena.membership = telemetry_battle_arena_membership::absent;
	return v;
}

telemetry_battle_contribution_context association()
{
	telemetry_battle_contribution_context v{};
	v.battle = { { 101U, 201U }, 301U };
	v.scope = { 1U, 2U, 333U, 1U, 1U, -1, 0U };
	v.actor.actor = { 8571U, 8571, 8571U, telemetry_combat_actor_kind::player, {}, 56U };
	v.actor.dimensions = { 56U, 1U, 1U, 2U, 1703, 1U };
	v.actor.context_version = 1U;
	v.actor.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	v.association_revision = 2U;
	v.association_fact_sequence = 5U;
	v.available_metrics = TELEMETRY_BC_METRICS;
	v.side_status = telemetry_battle_side_status::partial;
	v.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	assert(telemetry_battle_contribution_context_is_valid(v));
	return v;
}

telemetry_battle_build_observation snapshot()
{
	const auto native = profile();
	const auto link = association();
	telemetry_battle_build_observation v{};
	assert(telemetry_battle_build_observation_from_context(
		native, link, 1U, 1000U, TELEMETRY_UTC_UNKNOWN,
		telemetry_battle_build_boundary::actor_entry, &v));
	return v;
}

void roundtrip(const telemetry_battle_build_observation &v)
{
	std::uint8_t wire[TELEMETRY_BATTLE_BUILD_WIRE_BYTES]{};
	assert(telemetry_battle_build_observation_encode(v, wire, sizeof(wire)));
	telemetry_battle_build_observation decoded{};
	assert(telemetry_battle_build_observation_decode(wire, sizeof(wire), &decoded));
	assert(telemetry_battle_build_observation_equal(v, decoded));
}

void invalid(const telemetry_battle_build_observation &v)
{
	assert(!telemetry_battle_build_observation_is_valid(v));
	std::uint8_t wire[TELEMETRY_BATTLE_BUILD_WIRE_BYTES]{};
	assert(!telemetry_battle_build_observation_encode(v, wire, sizeof(wire)));
}

template <bool Signed, typename T> void export_number(bool &first, const char *name, T value)
{
	std::printf("%s\"%s\":", first ? "" : ",", name);
	if constexpr (Signed)
		std::printf("%lld", static_cast<long long>(value));
	else
		std::printf("%llu", static_cast<unsigned long long>(value));
	first = false;
}

void export_snapshot(const telemetry_battle_build_observation &v)
{
	std::uint8_t wire[TELEMETRY_BATTLE_BUILD_WIRE_BYTES]{};
	assert(telemetry_battle_build_observation_encode(v, wire, sizeof(wire)));
	std::printf("BUILD_OBSERVATION_JSON {\"wire\":\"");
	for (auto byte : wire)
		std::printf("%02x", byte);
	std::printf("\",\"fields\":{");
	bool first = true;
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) \
	export_number<signed_value>(first, #name, v.member);
#define TELEMETRY_BUILD_BYTES(name, member, width)           \
	std::printf("%s\"%s\":\"", first ? "" : ",", #name); \
	for (auto byte : v.member)                           \
		std::printf("%02x", byte);                   \
	std::printf("\"");                                   \
	first = false;
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	std::puts("}}");
}
} // namespace

int main(int argc, char **argv)
{
	if (argc == 3 && std::string_view(argv[1]) == "--verify")
	{
		auto *input = std::fopen(argv[2], "rb");
		assert(input);
		std::uint8_t length[4]{};
		while (std::fread(length, 1U, sizeof(length), input) == sizeof(length))
		{
			std::uint32_t count = 0U;
			for (auto byte : length)
				count = (count << 8U) | byte;
			assert(count <= TELEMETRY_BATTLE_BUILD_WIRE_BYTES + 1U);
			std::uint8_t wire[TELEMETRY_BATTLE_BUILD_WIRE_BYTES + 1U]{};
			assert(std::fread(wire, 1U, count, input) == count);
			telemetry_battle_build_observation value{};
			std::puts(telemetry_battle_build_observation_decode(wire, count, &value) ?
					  "1" :
					  "0");
		}
		assert(std::feof(input) && !std::ferror(input));
		std::fclose(input);
		return 0;
	}
	assert(argc == 1);
	const auto original = snapshot();
	roundtrip(original);
	assert(sizeof(original) == 448U && TELEMETRY_BATTLE_BUILD_WIRE_BYTES == 447U);
	assert(sizeof(telemetry_record) == 488U);
	telemetry_record record{};
	record.header.key = { { 101U, 201U }, 1U };
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::battle_build;
	record.header.occurrence_utc_usec = TELEMETRY_UTC_UNKNOWN;
	record.payload.battle_build = original;
	assert(telemetry_record_is_valid(record));
	assert(telemetry_record_kind_is_control(record.header.kind));
	++record.header.key.producer.process_id;
	assert(!telemetry_record_is_valid(record));
	--record.header.key.producer.process_id;
	record.header.occurrence_utc_usec = 2000U;
	assert(!telemetry_record_is_valid(record));
	export_snapshot(original);
	auto native = profile();
	native.listed_affects.flags[0] = std::numeric_limits<std::uint64_t>::max();
	telemetry_battle_build_observation selected{};
	assert(telemetry_battle_build_observation_from_context(
		native, association(), 1U, 1000U, TELEMETRY_UTC_UNKNOWN,
		telemetry_battle_build_boundary::actor_entry, &selected));
	assert(telemetry_battle_build_observation_equal(original, selected));
	auto refused = [&](const telemetry_battle_build_context &bad_native,
			   telemetry_sequence sequence = 1U,
			   telemetry_battle_build_boundary boundary =
				   telemetry_battle_build_boundary::actor_entry)
	{
		selected = original;
		assert(!telemetry_battle_build_observation_from_context(
			bad_native, association(), sequence, 1000U, 2000U, boundary, &selected));
		assert(selected.definition_version == 0U && selected.available == 0U &&
		       selected.actor_id == 0U && selected.sequence == 0U);
	};
	native = profile();
	native.config_id = 444U;
	refused(native);
	native = profile();
	++native.actor.id;
	refused(native);
	native = profile();
	++native.version;
	refused(native);
	native = profile();
	native.reserved[0] = 1U;
	refused(native);
	refused(profile(), 0U);
	refused(profile(), 1U, telemetry_battle_build_boundary::rate_limit);
	std::uint8_t bytes[TELEMETRY_BATTLE_BUILD_WIRE_BYTES]{};
	selected = original;
	assert(!telemetry_battle_build_observation_decode(bytes, sizeof(bytes) - 1U, &selected));
	assert(selected.sequence == 0U && selected.definition_version == 0U);
	assert(!telemetry_battle_build_observation_decode(nullptr, sizeof(bytes), &selected));
	assert(!telemetry_battle_build_observation_decode(bytes, sizeof(bytes), nullptr));
	assert(!telemetry_battle_build_observation_encode(original, bytes, sizeof(bytes) - 1U));
	assert(!telemetry_battle_build_observation_encode(original, nullptr, sizeof(bytes)));
	auto bad = original;
	bad.definition_version = 2U;
	invalid(bad);
	bad = original;
	bad.available |= 1024U;
	invalid(bad);
	bad = original;
	bad.equipment_counts[0] = 4U;
	invalid(bad);
	bad = original;
	bad.epic_learned_skills = 3U;
	invalid(bad);
	bad = original;
	bad.affects_complete = 0U;
	invalid(bad);
	bad = original;
	bad.context_quality |= TELEMETRY_BUILD_ARENA_INVALID;
	invalid(bad);
	auto partial = original;
	partial.available &= ~TELEMETRY_BUILD_LEARNED_EPICS;
	invalid(partial);
	partial.epic_catalog_skills = partial.epic_learned_skills = 0U;
	std::memset(partial.epic_digest, 0, sizeof(partial.epic_digest));
	partial.context_quality |= TELEMETRY_BUILD_EPICS_UNAVAILABLE;
	roundtrip(partial);
	partial = original;
	partial.affect_nodes = 64U;
	partial.affects_complete = 0U;
	partial.context_quality |= TELEMETRY_BUILD_AFFECTS_TRUNCATED;
	roundtrip(partial);
	partial = original;
	partial.available &= ~TELEMETRY_BUILD_ARENA_ROSTER;
	partial.arena_membership = 3U;
	partial.context_quality |= TELEMETRY_BUILD_ARENA_INVALID;
	roundtrip(partial);
	telemetry_battle_build_observation gap{};
	gap.battle = original.battle;
	gap.environment_id = original.environment_id;
	gap.season_id = original.season_id;
	gap.config_id = original.config_id;
	gap.build_version = original.build_version;
	gap.content_version = original.content_version;
	gap.actor_id = original.actor_id;
	gap.actor_kind = original.actor_kind;
	gap.sequence = 2U;
	gap.association_revision = original.association_revision;
	gap.association_fact_sequence = original.association_fact_sequence;
	gap.at_monotonic_usec = 1100U;
	gap.at_utc_usec = TELEMETRY_UTC_UNKNOWN;
	gap.definition_version = gap.native_context_version = 1U;
	gap.boundary = telemetry_battle_build_boundary::rate_limit;
	gap.status = telemetry_battle_build_status::unavailable;
	gap.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	gap.context_quality = TELEMETRY_BUILD_SUPPORT_ORIGIN_UNKNOWN;
	roundtrip(gap);
	bad = gap;
	bad.level = original.level;
	invalid(bad);
	gap.boundary = telemetry_battle_build_boundary::configuration_unavailable;
	gap.config_id = gap.build_version = gap.content_version = 0U;
	roundtrip(gap);
	bad = original;
	++bad.current_resources[0];
	assert(telemetry_battle_build_observation_same_key(original, bad));
	assert(!telemetry_battle_build_observation_equal(original, bad));
	roundtrip(bad);
	std::puts(
		"build observation factory, exact wire, unavailable clearing and partial-family contracts passed");
}
