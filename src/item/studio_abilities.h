#ifndef DURIS_STUDIO_ABILITIES_H
#define DURIS_STUDIO_ABILITIES_H

#include "item/item_actions.h"
#include "item/studio_ability_model.h"

// Atomic catalog publication. A bad candidate retains the last valid catalog
// and its pending actions; replacements/removals cancel only affected IDs.
bool studio_abilities_load(const std::string &json, std::string &error);
bool studio_abilities_reload_file(std::string &error);
void update_studio_ability_properties();
bool studio_ability_reference(uint32_t id, int vnum, studio_ability_trigger, std::string &error);
bool parse_studio_ability_action(int target_type, int vnum, int event, int command,
				 const char *argument, uint32_t &id, std::string &error);
item_action_start begin_studio_ability(uint32_t id, studio_ability_trigger event, P_obj source,
				       P_char original_activator, P_char struck_victim,
				       const char *use_arguments);

#endif
