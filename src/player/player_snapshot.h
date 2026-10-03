#ifndef PLAYER_SNAPSHOT_H
#define PLAYER_SNAPSHOT_H

#include "player/player_revision_state.h"
#include "item/item_transfer_command.h"
#include "player/pet_restore_state.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

constexpr uint32_t PLAYER_SNAPSHOT_SCHEMA_VERSION = 7;
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION = 8;
// Readable evidence envelope only. Existing capture/admission stays on version 8
// until the atomic retention and player-visible recovery path is integrated.
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION = 10;
// Ordinary save frames carrying a quest progression receipt. Kept separate from
// the death envelopes so the receipt can be committed with player progression.
constexpr uint32_t PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION = 11;
// Ordinary save frames that retain operation-scoped spell effect application receipts.
constexpr uint32_t PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION = 12;
// Death dispositions retain applied spell receipts, including conflict evidence.
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION = 13;
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION = 14;
// Death frames carrying quest XP and optionally applied spell receipts.
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION = 15;
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION = 16;

// Recipe progression receipts commit with status, skills, notch affects and trophies.
constexpr uint32_t PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION = 17;
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION = 18;
constexpr uint32_t PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION = 19;
constexpr size_t PLAYER_CRAFT_RECEIPT_MAX = 64;
constexpr bool player_snapshot_has_craft_receipt_schema(uint32_t version)
{
	return version == PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION;
}

constexpr bool player_snapshot_is_death_request_schema(uint32_t version)
{
	return version == PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
}
constexpr bool player_snapshot_is_death_evidence_schema(uint32_t version)
{
	return version == PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION;
}
constexpr bool player_snapshot_has_spell_receipt_schema(uint32_t version)
{
	return player_snapshot_has_craft_receipt_schema(version) ||
	       version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION;
}
constexpr bool player_snapshot_has_quest_receipt_schema(uint32_t version)
{
	return player_snapshot_has_craft_receipt_schema(version) ||
	       version == PLAYER_SNAPSHOT_QUEST_REWARD_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_SPELL_EFFECT_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION ||
	       version == PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION;
}
constexpr uint32_t player_snapshot_death_request_schema(uint32_t version)
{
	if (version == PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION)
		return PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION;
	if (version == PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION)
		return PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION;
	return version == PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION ?
		       PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION :
		       PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
}
constexpr uint32_t player_snapshot_death_evidence_schema(uint32_t version)
{
	if (version == PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION)
		return PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION;
	if (version == PLAYER_SNAPSHOT_DEATH_QUEST_RECEIPT_SCHEMA_VERSION)
		return PLAYER_SNAPSHOT_DEATH_QUEST_EVIDENCE_SCHEMA_VERSION;
	return version == PLAYER_SNAPSHOT_DEATH_SPELL_RECEIPT_SCHEMA_VERSION ?
		       PLAYER_SNAPSHOT_DEATH_SPELL_EVIDENCE_SCHEMA_VERSION :
		       PLAYER_SNAPSHOT_DEATH_EVIDENCE_SCHEMA_VERSION;
}
constexpr uint32_t PLAYER_SPELL_EFFECT_RECEIPT_EFFECT_MAX = 6;
constexpr size_t PLAYER_SPELL_EFFECT_RECEIPT_MAX = 4096;
constexpr size_t PLAYER_DEATH_EVIDENCE_MAX_COLUMNS = 64;
constexpr size_t PLAYER_DEATH_EVIDENCE_MAX_COLUMN_NAME_BYTES = 64;
// Ward-bearing wire envelopes use the existing normalized schema plus 13.
// This allocates versions 20, 21 and 23-32 without reusing accounting formats
// 7-19 (including death evidence 10). Receipts retain their existing schemas.
constexpr uint32_t PLAYER_SNAPSHOT_WARD_WIRE_OFFSET = 13;
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
	uint64_t ward_source_uid;
	int32_t ward_full_duration;
	int64_t ward_capacity;
	int64_t ward_capacity_max;
	int32_t ward_refresh_remaining;
	uint8_t ward_source_type;
	uint8_t ward_source_worn;
	uint8_t ward_active;
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
	uint64_t pet_uid = 0;
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
	std::string restore_state;
	pet_hold_reason hold_reason = pet_hold_reason::none;
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

struct player_quest_xp_receipt_snapshot
{
	critical_operation_id offering_operation;
	uint32_t reward_index;
	uint32_t amount;
};

struct player_craft_receipt_snapshot
{
	critical_operation_id operation_id = {};
	uint32_t discipline = 0;
	uint32_t experience = 0;
};

struct player_spell_effect_receipt_snapshot
{
	critical_operation_id operation_id;
	uint32_t effect_id;
};

// Stored with the post-death terminal snapshot, outside active inventory. The
// corpse tree includes every captured asset; custody records ownership evidence
// for that captured graph, including an explicit absent row when its runtime
// catalog entry is missing. Live observations outside the captured graph are
// intentionally not retained as death custody.
struct player_death_custody_snapshot
{
	item_transfer_entry item;
	item_owner_identity owner;
	uint64_t owner_revision;
};

// Raw observations are evidence, never an ownership grant or loadable inventory.
// Preserve SQL NULL separately from empty strings and keep field bytes verbatim.
using player_death_evidence_row = std::vector<std::optional<std::string>>;

struct player_death_evidence_table
{
	std::vector<std::string> columns;
	std::vector<player_death_evidence_row> rows;
};

struct player_death_conflict_evidence
{
	player_death_evidence_table player_items;
	player_death_evidence_table player_item_affects;
	player_death_evidence_table player_item_extra_descr;
	player_death_evidence_table item_current_owner;
	player_death_evidence_table item_owner_revision;
};

struct player_death_snapshot
{
	critical_operation_id operation_id;
	int32_t corpse_room_vnum;
	uint64_t wallet_revision;
	std::array<int32_t, 4> wallet_before;
	uint64_t wallet_pile_uid;
	std::vector<player_item_snapshot> corpse;
	std::vector<player_death_custody_snapshot> custody;
	std::vector<critical_operation_id> unresolved_operations;
	std::optional<player_death_conflict_evidence> conflict_evidence;
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
	std::vector<player_quest_xp_receipt_snapshot> quest_xp_receipts;
	std::vector<player_spell_effect_receipt_snapshot> spell_effect_receipts;
	std::vector<player_craft_receipt_snapshot> craft_receipts = {};
	bool recipes_are_external;
	std::string output_preferences;
	std::optional<player_death_snapshot> death;
};

#endif
