#ifndef DURIS_STUDIO_ABILITY_MODEL_H
#define DURIS_STUDIO_ABILITY_MODEL_H

#include "item/artifact_mana_model.h"
#include <array>
#include <cstdint>
#include <map>
#include <string>

enum class studio_ability_trigger
{
	hit,
	use
};
enum class studio_ability_target
{
	activator,
	holder,
	victim,
	item,
	room
};
enum class studio_ability_call
{
	spell,
	wand
};
struct studio_ability_effect
{
	int spell = 0, power = 0;
	studio_ability_target target = studio_ability_target::victim;
	studio_ability_call call = studio_ability_call::spell;
	bool operator==(const studio_ability_effect &) const = default;
};
struct studio_ability_definition
{
	uint32_t id = 0;
	uint64_t revision = 0;
	int vnum = 0, min_level = 0, windup = 8, progress = 4, cooldown_ms = 0;
	studio_ability_trigger trigger = studio_ability_trigger::hit;
	bool carried = false;
	uint64_t cost = 0;
	artifact_mana_profile mana;
	std::array<studio_ability_effect, 3> effects{};
	size_t effect_count = 0;
	// Literal, bounded text. Presentation adds actor/item/victim through act().
	std::string begin, beat, complete, cancel;
	bool operator==(const studio_ability_definition &) const = default;
};
using studio_ability_catalog = std::map<uint32_t, studio_ability_definition>;

// Pure strict parser. Output is replaced only after the entire document passes.
bool parse_studio_ability_catalog(const std::string &, studio_ability_catalog &,
				  std::string &error);

#endif
