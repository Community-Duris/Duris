#ifndef DURIS_WONDER_ACTIONS_H
#define DURIS_WONDER_ACTIONS_H

#include "item/item_actions.h"

item_action_start begin_wonder_action(P_obj, P_char actor, P_char original_target,
				      int selected_level);
void update_wonder_action_properties();

#endif
