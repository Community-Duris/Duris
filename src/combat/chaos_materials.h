#ifndef CHAOS_MATERIALS_H
#define CHAOS_MATERIALS_H

#include "combat/chaos_config.h"
#include "core/structs.h"
#include "core/utils.h"
#include "world/vnum.obj.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS = 8;
constexpr size_t CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES = 3500;

struct chaos_material_pouch_usage
{
	int vnum;
	uint64_t count;
};

static inline bool chaos_material_pouch_is(P_obj object)
{
	return object && OBJ_VNUM(object) == VOBJ_CHAOS_CRAFT_POUCH;
}

constexpr size_t CHAOS_MATERIAL_POUCH_SEARCH_BUDGET = 128;
static inline P_obj
chaos_material_pouch_find_nested(P_obj object, size_t budget = CHAOS_MATERIAL_POUCH_SEARCH_BUDGET)
{
	if (!object || !budget)
		return nullptr;
	if (chaos_material_pouch_is(object))
		return object;
	for (P_obj child = object->contains; child && budget > 1;
	     child = child->next_content, --budget)
		if (P_obj pouch = chaos_material_pouch_find_nested(child, budget - 1))
			return pouch;
	return nullptr;
}

static inline bool chaos_material_pouch_in(P_obj object)
{
	return chaos_material_pouch_find_nested(object) != nullptr;
}

static inline bool chaos_material_pouch_is_active(P_obj object)
{
	return chaos_material_pouch_is(object) && chaos_starter_materials_enabled();
}

P_obj chaos_material_pouch_find(P_char ch);
bool chaos_material_pouch_available(P_char ch);
bool chaos_material_pouch_vnum_supported(int vnum);
bool chaos_material_pouch_can_record_generated(P_char ch, const chaos_material_pouch_usage *usage,
					       size_t usage_count);
void chaos_material_pouch_report_generated_failure(P_char ch, const char *operation);

bool chaos_material_pouch_record_generated(P_char ch, const chaos_material_pouch_usage *usage,
					   size_t usage_count);
bool chaos_material_pouch_record_collected(P_obj pouch, const chaos_material_pouch_usage *usage,
					   size_t usage_count);
bool chaos_material_pouch_revert_collected(P_obj pouch, const chaos_material_pouch_usage *usage,
					   size_t usage_count);
bool chaos_material_pouch_collect_object(P_char ch, P_obj pouch, P_obj material);
/* Returns 0 for no matches, a positive count for a queued collection, or -1 on submission failure. */
int chaos_material_pouch_collect_inventory(P_char ch, P_obj pouch, const char *filter);

const char *chaos_material_pouch_scoreboard(P_obj pouch);
static inline const char *chaos_material_pouch_contents_description(P_obj pouch)
{
	return chaos_material_pouch_scoreboard(pouch);
}

#endif
