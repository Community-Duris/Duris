#ifndef DURIS_NATIVE_ARTIFACT_ACTIONS_H
#define DURIS_NATIVE_ARTIFACT_ACTIONS_H

#include "item/item_actions.h"
#include <cstdint>

struct native_artifact_config
{
	bool valid = false, enabled = false;
	int cost = 0, capacity = 0, regeneration = 0, floor = 0, mana_revision = 1, windup = 8;
	uint64_t revision = 1;
	bool operator==(const native_artifact_config &) const = default;
};

uint32_t native_artifact_ability(int vnum, int power);
void update_native_artifact_properties();
// Invalid opt-in settings suppress the new power, never fall through to legacy.
bool native_artifact_owns(int vnum);
native_artifact_config native_artifact_settings(int vnum);
bool native_artifact_prepare(int vnum, const native_artifact_config &);
bool native_artifact_actor(P_char);
bool native_artifact_hostile(P_char actor, P_char target);
bool native_artifact_current(const item_action_identity &, P_char &, P_obj &);

item_action_start begin_tsunami_action(P_obj, P_char, int command);
bool intercept_mirrored_ioun(P_obj, P_char defender, const proc_data &);
item_action_start begin_necroplasm_form(P_obj, P_char);
void release_necroplasm_forms();

#endif
