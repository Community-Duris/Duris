#include "telemetry/telemetry_battle_build_observation.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace
{
using observation = telemetry_battle_build_observation;

using telemetry_battle_build_detail::zero;

template <std::size_t Width, bool Signed, typename T>
void encode_value(T value, std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	const auto number = static_cast<std::uint64_t>(value);
	for (std::size_t index = 0U; index < Width; ++index)
		*bytes++ = static_cast<std::uint8_t>(number >> ((Width - 1U - index) * 8U));
}

template <std::size_t Width, bool Signed, typename T>
void decode_value(T &value, const std::uint8_t *&bytes) noexcept
{
	static_assert(sizeof(T) == Width);
	static_assert(std::is_signed_v<T> == Signed);
	std::uint64_t number = 0U;
	for (std::size_t index = 0U; index < Width; ++index)
		number = (number << 8U) | *bytes++;
	if constexpr (Signed)
		value = std::bit_cast<T>(static_cast<std::make_unsigned_t<T>>(number));
	else
		value = static_cast<T>(number);
}
} // namespace

bool telemetry_battle_build_observation_same_key(const observation &a,
						 const observation &b) noexcept
{
	return a.sequence != 0U && a.sequence == b.sequence &&
	       telemetry_producer_id_is_valid(a.battle.producer) &&
	       a.battle.producer.boot_id == b.battle.producer.boot_id &&
	       a.battle.producer.process_id == b.battle.producer.process_id;
}

bool telemetry_battle_build_observation_equal(const observation &a, const observation &b) noexcept
{
	if (!telemetry_battle_build_observation_is_valid(a) ||
	    !telemetry_battle_build_observation_is_valid(b))
		return false;
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) \
	if (a.member != b.member)                                \
		return false;
#define TELEMETRY_BUILD_BYTES(name, member, width)                                       \
	if (!std::equal(std::begin(a.member), std::end(a.member), std::begin(b.member))) \
		return false;
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	return true;
}

bool telemetry_battle_build_observation_encode(const observation &v, std::uint8_t *bytes,
					       std::size_t length) noexcept
{
	if (bytes == nullptr || length != TELEMETRY_BATTLE_BUILD_WIRE_BYTES ||
	    !telemetry_battle_build_observation_is_valid(v))
		return false;
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) \
	encode_value<width, signed_value>(v.member, bytes);
#define TELEMETRY_BUILD_BYTES(name, member, width) \
	std::memcpy(bytes, v.member, width);       \
	bytes += width;
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	return true;
}

bool telemetry_battle_build_observation_decode(const std::uint8_t *bytes, std::size_t length,
					       observation *output) noexcept
{
	if (output == nullptr)
		return false;
	*output = {};
	if (bytes == nullptr || length != TELEMETRY_BATTLE_BUILD_WIRE_BYTES)
		return false;
	observation v{};
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) \
	decode_value<width, signed_value>(v.member, bytes);
#define TELEMETRY_BUILD_BYTES(name, member, width) \
	std::memcpy(v.member, bytes, width);       \
	bytes += width;
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	if (!telemetry_battle_build_observation_is_valid(v))
		return false;
	*output = v;
	return true;
}

bool telemetry_battle_build_observation_from_context(
	const telemetry_battle_build_context &native,
	const telemetry_battle_contribution_context &association, telemetry_sequence sequence,
	telemetry_monotonic_usec at, telemetry_utc_usec utc,
	telemetry_battle_build_boundary boundary, observation *output) noexcept
{
	if (output == nullptr)
		return false;
	*output = {};
	if (!telemetry_battle_contribution_context_is_valid(association) ||
	    native.version != TELEMETRY_BATTLE_BUILD_CONTEXT_VERSION ||
	    native.config_id != association.scope.config_id ||
	    native.actor.id != association.actor.actor.actor_id ||
	    native.actor.kind != association.actor.actor.kind || !zero(native.reserved) ||
	    !zero(native.reserved_saves) || native.equipment.reserved != 0U ||
	    !zero(native.listed_affects.reserved) || !zero(native.arena.reserved))
		return false;
	observation v{};
	v.battle = association.battle;
	v.environment_id = association.scope.environment_id;
	v.season_id = association.scope.season_id;
	v.config_id = native.config_id;
	v.actor_id = native.actor.id;
	v.actor_kind = native.actor.kind;
	v.sequence = sequence;
	v.association_revision = association.association_revision;
	v.association_fact_sequence = association.association_fact_sequence;
	v.at_monotonic_usec = at;
	v.at_utc_usec = utc;
	v.definition_version = TELEMETRY_BATTLE_BUILD_DEFINITION_VERSION;
	v.native_context_version = native.version;
	v.boundary = boundary;
	v.status = telemetry_battle_build_status::snapshot;
	v.primary_class_mask = native.primary_class_mask;
	v.secondary_class_mask = native.secondary_class_mask;
	v.build_version = native.build_version;
	v.content_version = native.content_version;
	v.available = native.available;
	v.context_quality = native.quality;
	v.quality_flags = association.quality_flags | association.actor.quality_flags;
	v.level = native.level;
	v.race = native.race;
	v.faction = native.faction;
	v.specialization = native.specialization;
	std::copy_n(native.base.stats, 10U, v.base_stats);
	std::copy_n(native.effective.stats, 10U, v.effective_stats);
	std::copy_n(native.base.resources, 4U, v.base_resources);
	std::copy_n(native.effective.resources, 4U, v.effective_resources);
	std::copy_n(native.current_resources, 4U, v.current_resources);
	std::copy_n(native.base.combat, 3U, v.base_combat);
	std::copy_n(native.effective.combat, 3U, v.effective_combat);
	std::copy_n(native.equipment.direct_modifiers, 5U, v.equipment_modifiers);
	std::copy_n(native.saving_modifiers, 5U, v.saving_modifiers);
	std::copy_n(native.effective_flags, 5U, v.effective_flags);
	std::copy_n(native.equipment.flags, 5U, v.equipment_flags);
	std::copy_n(native.equipment.fixed_feature_digest, 32U, v.equipment_digest);
	std::copy_n(native.epics.learned_build_digest, 32U, v.epic_digest);
	v.equipment_counts[0] = native.equipment.occupied_slots;
	v.equipment_counts[1] = native.equipment.melee_weapons;
	v.equipment_counts[2] = native.equipment.ranged_weapons;
	v.equipment_counts[3] = native.equipment.shields;
	v.equipment_counts[4] = native.equipment.armor;
	v.equipment_counts[5] = native.equipment.other_items;
	v.equipment_counts[6] = native.equipment.items_with_dynamic_affects;
	v.epic_catalog_skills = native.epics.catalog_skills;
	v.epic_learned_skills = native.epics.learned_skills;
	v.affect_nodes = native.listed_affects.observed_nodes;
	v.offensive_modifier_nodes = native.listed_affects.offensive_modifier_nodes;
	v.armor_modifier_nodes = native.listed_affects.armor_modifier_nodes;
	v.resource_modifier_nodes = native.listed_affects.resource_modifier_nodes;
	v.unapplied_nodes = native.listed_affects.unapplied_nodes;
	v.affects_complete = native.listed_affects.complete;
	v.arena_membership = static_cast<std::uint8_t>(native.arena.membership);
	v.arena_player_flags = native.arena.player_flags;
	v.arena_room = native.arena.room_is_arena;
	v.arena_enabled = native.arena.enabled;
	v.arena_type = native.arena.type;
	v.arena_stage = native.arena.stage;
	v.arena_team = native.arena.team;
	if (!telemetry_battle_build_observation_is_valid(v))
		return false;
	*output = v;
	return true;
}
