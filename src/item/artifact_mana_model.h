#ifndef DURIS_ARTIFACT_MANA_MODEL_H
#define DURIS_ARTIFACT_MANA_MODEL_H

#include <cstdint>

// All amounts are thousandths of one mana point. Rates are per whole second.
constexpr uint64_t ARTIFACT_MANA_MAXIMUM = 1000000000000ULL;
constexpr uint64_t ARTIFACT_MANA_MAX_RATE = 1000000000ULL;
constexpr uint64_t ARTIFACT_MANA_MAX_OFFLINE_SECONDS = 86400;

struct artifact_mana_profile
{
	uint64_t id = 0;
	uint64_t revision = 0;
	uint64_t capacity = 0;
	uint64_t regeneration = 0;
	uint64_t passive_floor = 0;
	bool operator==(const artifact_mana_profile &) const = default;
};

// Independent authority keyed by physical UID, never embedded in an owner save.
struct artifact_mana_record
{
	uint64_t uid = 0;
	uint64_t profile_id = 0;
	uint64_t profile_revision = 0;
	uint64_t version = 0;
	uint64_t capacity = 0;
	uint64_t regeneration = 0;
	uint64_t reserve = 0;
	uint64_t settled_at = 0;
	bool operator==(const artifact_mana_record &) const = default;
};

bool artifact_mana_valid(const artifact_mana_profile &);
bool artifact_mana_valid(const artifact_mana_record &);
artifact_mana_record artifact_mana_empty(uint64_t uid, const artifact_mana_profile &, uint64_t now);
// Read projection; does not increment the durable version. Old rates settle
// before a revision change, then capacity is clamped. Never fills on conversion.
bool artifact_mana_project(artifact_mana_record &, const artifact_mana_profile &, uint64_t now);
bool artifact_mana_spend(artifact_mana_record &, const artifact_mana_profile &, uint64_t now,
			 uint64_t cost, bool passive);

#endif
