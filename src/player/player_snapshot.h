#ifndef PLAYER_SNAPSHOT_H
#define PLAYER_SNAPSHOT_H

#include "player/player_revision_state.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr uint32_t PLAYER_SNAPSHOT_SCHEMA_VERSION = 1;
constexpr size_t PLAYER_SNAPSHOT_MAX_BYTES = 4 * 1024 * 1024;
constexpr size_t PLAYER_SNAPSHOT_MAX_ROWS = 8192;
constexpr size_t PLAYER_SNAPSHOT_MAX_OBJECTS = 4096;
constexpr size_t PLAYER_SNAPSHOT_MAX_DEPTH = 32;
constexpr size_t PLAYER_SNAPSHOT_MAX_STRING_BYTES = 4096;
constexpr int32_t PLAYER_SNAPSHOT_NO_PARENT = -1;

enum class player_snapshot_capture_result : uint8_t
{
	ok,
	invalid_identity,
	retryable_allocation_failure,
	limit_exceeded,
	object_cycle,
	malformed_source,
};

enum class player_status_field : uint16_t
{
	class_primary,
	class_secondary,
	specialization,
	race,
	racewar,
	level,
	sex,
	weight,
	height,
	size,
	hometown,
	birthplace,
	original_birthplace,
	birth_time,
	played_time,
	base_strength,
	base_dexterity,
	base_agility,
	base_constitution,
	base_power,
	base_intelligence,
	base_wisdom,
	base_charisma,
	base_karma,
	base_luck,
	mana,
	base_mana,
	hit_difference,
	base_hit,
	vitality,
	base_vitality,
	extra_memorization,
	copper,
	silver,
	gold,
	platinum,
	experience,
	epics,
	epic_skill_points,
	skill_points,
	spell_bind_used,
	action_flags,
	action_flags_2,
	action_flags_3,
	vote,
	alignment,
	prestige,
	guild_id,
	guild_status,
	time_left_guild,
	times_left_guild,
	time_unspecialized,
	frags,
	old_frags,
	deaths,
	echo,
	prompt,
	wizard_invisibility,
	wimpy,
	aggressive,
	highest_level,
	screen_length,
	last_ip,
};

enum class player_status_string_field : uint8_t
{
	name,
	short_description,
	long_description,
	description,
	title,
	poof_in,
	poof_out,
};

struct player_snapshot_integer
{
	player_status_field field;
	int64_t signed_value;
	uint64_t unsigned_value;
	bool is_unsigned;
};

struct player_snapshot_string
{
	player_status_string_field field;
	std::string value;
};

struct player_index_value_snapshot
{
	int32_t index;
	int64_t value;
	uint64_t auxiliary;
};

struct player_skill_snapshot
{
	int32_t skill_id;
	uint8_t learned;
	uint8_t taught;
};

struct player_affect_snapshot
{
	int16_t type;
	int32_t duration;
	uint32_t flags;
	int32_t modifier;
	uint8_t location;
	uint16_t level;
	std::array<uint64_t, 5> bitvectors;
	std::string wear_off_character;
	std::string wear_off_room;
};

struct player_item_extra_description_snapshot
{
	std::string keyword;
	std::string description;
	bool spellbook;
	std::vector<int32_t> spell_ids;
};

struct player_item_dynamic_affect_snapshot
{
	int16_t type;
	int16_t data;
	uint64_t extra2;
};

struct player_item_snapshot
{
	int32_t parent_index;
	int16_t equipment_slot;
	uint64_t object_uid;
	int64_t generated_key;
	int32_t vnum;
	int8_t type;
	uint8_t string_mask;
	std::string name;
	std::string short_description;
	std::string description;
	std::string action_description;
	std::array<int32_t, 8> values;
	std::array<int64_t, 6> timers;
	uint32_t wear_flags;
	uint32_t extra_flags;
	uint32_t anti_flags;
	uint32_t anti2_flags;
	uint32_t extra2_flags;
	int32_t weight;
	int8_t material;
	int32_t cost;
	int16_t condition;
	int16_t craftsmanship;
	std::array<uint64_t, 5> bitvectors;
	std::array<std::array<int16_t, 2>, 4> affects;
	std::vector<player_item_dynamic_affect_snapshot> dynamic_affects;
	std::vector<player_item_extra_description_snapshot> extra_descriptions;
};

struct player_pet_snapshot
{
	int32_t mob_vnum;
	int32_t order;
	int32_t hit;
	int32_t max_hit;
	int32_t mana;
	int32_t max_mana;
	int32_t vitality;
	int32_t max_vitality;
	int32_t charm_duration;
	int32_t room_vnum;
	std::vector<player_item_snapshot> items;
};

struct player_shape_snapshot
{
	int32_t mob_vnum;
	int32_t times_researched;
	int64_t last_researched;
	int64_t last_shapechanged;
};

struct player_trophy_snapshot
{
	int32_t zone_number;
	int32_t experience;
};

struct player_snapshot
{
	uint32_t schema_version;
	int32_t pid;
	player_revision_t revision;
	player_component_mask_t components;
	int32_t save_intent;
	int32_t room_vnum;
	size_t encoded_size_bound;
	std::vector<player_snapshot_integer> status_integers;
	std::vector<player_snapshot_string> status_strings;
	std::array<int32_t, 5> conditions;
	std::array<int32_t, 14> quest_values;
	std::vector<player_index_value_snapshot> languages;
	std::vector<player_index_value_snapshot> introductions;
	std::vector<player_index_value_snapshot> timers;
	std::vector<player_index_value_snapshot> undead_slots;
	std::vector<player_index_value_snapshot> forged_items;
	std::vector<int32_t> granted_commands;
	std::vector<player_skill_snapshot> skills;
	std::vector<player_affect_snapshot> affects;
	std::vector<player_item_snapshot> items;
	std::vector<player_pet_snapshot> pets;
	std::vector<player_shape_snapshot> shapes;
	std::vector<player_trophy_snapshot> trophies;
	bool recipes_are_external;
};

#endif
