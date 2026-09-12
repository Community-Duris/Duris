#ifndef PET_RESTORE_STATE_H
#define PET_RESTORE_STATE_H

#include <array>
#include <cstdint>
#include <string>

// Zero means an ordinary area creature. These values identify a creation path,
// never a name or a prototype shared by several kinds of raised creatures.
enum class summoned_pet_kind : uint32_t
{
	ordinary = 0,
	undead_first = 1, // 1..14: necromancer/theurgist undead_data index + 1
	golem_first = 15, // 15..18: golem_data index + 15
	titan = 19,
	dracolich = 20,
	avatar = 21,
	greater_dracolich = 22,
};

enum class pet_hold_reason : uint32_t
{
	none = 0,
	legacy_summon = 1,
	invalid_state = 2,
	expired = 3,
	over_capacity = 4,
	missing_prototype = 5,
};

constexpr size_t PET_RESTORE_STATE_MAX_BYTES = 32768;
constexpr size_t PET_RESTORE_STATE_MAX_TEXT = 4096;
constexpr size_t PET_RESTORE_STATE_SLOTS = 16;

struct pet_restore_state
{
	uint32_t version = 1;
	summoned_pet_kind kind = summoned_pet_kind::ordinary;
	int64_t charm_expires_at = 0; // zero is explicitly permanent
	int64_t death_expires_at = 0;
	std::string name, short_description, long_description;
	std::array<int32_t, 10> base_stats = {};
	std::array<int32_t, 7> base_points = {}; // HP, mana, vitality, AC, hit, damage, ward
	std::array<int32_t, 2> damage_dice = {};
	std::array<int32_t, PET_RESTORE_STATE_SLOTS> spell_slots = {};
	std::array<uint64_t, 5> intrinsic_affects = {};
	std::array<uint64_t, 3> aggression = {};
	uint64_t act = 0;
	uint32_t primary_class = 0, secondary_class = 0;
	int32_t level = 0, race = 0, sex = 0, size = 0, alignment = 0;
};

// Bounded, endian-independent ASCII payload: safe in SQL text and binary
// checkpoints alike. Unknown versions are retained for review, never guessed.
bool pet_restore_state_encode(const pet_restore_state &state, std::string *encoded);
bool pet_restore_state_decode(const std::string &encoded, pet_restore_state *state);
int summoned_pet_cost(summoned_pet_kind kind);
bool legacy_summon_prototype(int vnum);
bool summoned_pet_matches_prototype(summoned_pet_kind kind, int vnum);

#endif
