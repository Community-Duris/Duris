#ifndef DURIS_TELEMETRY_BATTLE_BUILD_CONTEXT_H
#define DURIS_TELEMETRY_BATTLE_BUILD_CONTEXT_H

#include "telemetry/telemetry_types.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/* Native snapshot contract, independently versioned from the sealed kind-10/11
 * records. This is a value reader, not a transport/persistence record. */
inline constexpr std::size_t TELEMETRY_BATTLE_BUILD_STATS = 10U;
inline constexpr std::size_t TELEMETRY_BATTLE_BUILD_RESOURCES = 4U;
inline constexpr std::size_t TELEMETRY_BATTLE_BUILD_FLAG_BANKS = 5U;
inline constexpr std::size_t TELEMETRY_BATTLE_BUILD_MAX_AFFECTS = 64U;

/* Array mappings are explicit: stats = Str,Dex,Agi,Con,Pow,Int,Wis,Cha,Kar,Luk;
 * resources = hit,mana,vitality,ward; combat = armor,hitroll,damroll;
 * equipment modifiers = APPLY_HIT,APPLY_MANA,APPLY_AC,APPLY_HITROLL,APPLY_DAMROLL;
 * saving modifiers = para,rod,fear,breath,spell; flags = native banks 1..5.
 * These are observed modifiers/values, not probabilities or a power score. */
struct telemetry_battle_build_values
{
	std::int16_t stats[TELEMETRY_BATTLE_BUILD_STATS];
	std::int32_t resources[TELEMETRY_BATTLE_BUILD_RESOURCES];
	std::int32_t combat[3];
};

struct telemetry_battle_equipment_context
{
	/* Fixed loaded declarations only. Eligibility, racial/material scaling,
	 * suppression and proc behavior are not inferred as applied gear effects. */
	std::uint8_t occupied_slots;
	std::uint8_t melee_weapons;
	std::uint8_t ranged_weapons;
	std::uint8_t shields;
	std::uint8_t armor;
	std::uint8_t other_items;
	std::uint8_t items_with_dynamic_affects;
	std::uint8_t reserved;
	std::int32_t direct_modifiers[5];
	std::uint64_t flags[TELEMETRY_BATTLE_BUILD_FLAG_BANKS];
	/* SHA-256 of the reviewed fixed loaded features in slot order. No item
	 * identities, strings, prices or dynamic/prototype proc interpretation. */
	std::uint8_t fixed_feature_digest[32];
};

struct telemetry_battle_epic_context
{
	std::uint16_t catalog_skills;
	std::uint16_t learned_skills;
	/* SHA-256 of content version and compiled epic skill IDs/learned ranks.
	 * Wealth, unspent epic points and contribution-bonus choices are excluded. */
	std::uint8_t learned_build_digest[32];
};

struct telemetry_battle_listed_affects
{
	std::uint64_t flags[TELEMETRY_BATTLE_BUILD_FLAG_BANKS];
	std::uint16_t observed_nodes;
	std::uint16_t offensive_modifier_nodes;
	std::uint16_t armor_modifier_nodes;
	std::uint16_t resource_modifier_nodes;
	std::uint16_t unapplied_nodes;
	std::uint8_t complete;
	/* No caster identity is inferred from the generic affect context pointer.
	 * Listed effects may be beneficial, harmful, intrinsic or externally cast. */
	std::uint8_t reserved[5];
};

enum class telemetry_battle_arena_membership : std::uint8_t
{
	unavailable = 0,
	absent = 1,
	member = 2,
	ambiguous = 3,
};

struct telemetry_battle_arena_context
{
	std::int32_t player_flags;
	telemetry_battle_arena_membership membership;
	std::uint8_t room_is_arena;
	std::uint8_t enabled;
	std::uint8_t type;
	std::uint8_t stage;
	std::uint8_t team; // 1..3 when a unique member; zero otherwise.
	std::uint8_t reserved[2];
};

struct telemetry_battle_build_context
{
	telemetry_battle_actor_key actor;
	telemetry_config_id config_id;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t primary_class_mask;
	std::uint32_t secondary_class_mask;
	std::uint32_t available;
	std::uint32_t quality;
	std::uint16_t version;
	std::uint16_t level;
	std::uint16_t race;
	std::uint16_t faction;
	std::uint8_t specialization; // The game has no separate secondary spec field.
	std::uint8_t reserved[7];
	telemetry_battle_build_values base;
	telemetry_battle_build_values effective;
	std::int32_t current_resources[TELEMETRY_BATTLE_BUILD_RESOURCES];
	std::int8_t saving_modifiers[5];
	std::uint8_t reserved_saves[3];
	std::uint64_t effective_flags[TELEMETRY_BATTLE_BUILD_FLAG_BANKS];
	telemetry_battle_equipment_context equipment;
	telemetry_battle_epic_context epics;
	telemetry_battle_listed_affects listed_affects;
	telemetry_battle_arena_context arena;
};

static_assert(std::is_trivially_copyable_v<telemetry_battle_build_context>);
static_assert(std::is_standard_layout_v<telemetry_battle_build_context>);
static_assert(sizeof(telemetry_battle_build_context) <= 448U);

#endif
