#ifndef DURIS_WEAPON_ACTIONS_H
#define DURIS_WEAPON_ACTIONS_H

#include "item/item_actions.h"

// Call only after the legacy path has selected its proc chance. A non-legacy
// result owns that selection even when it is suppressed by resource or busy gates.
item_action_start selected_avernus_action(P_obj, P_char, P_char, int damage, int heal_cap);
item_action_start selected_packed_weapon_action(P_obj, P_char, P_char);
item_action_start selected_random_weapon_action(P_obj, P_char, P_char, int spell, int selected_at);
void update_weapon_action_properties();

// A typed effect shared with the immediate legacy path; never re-enters the
// callback that performs RNG, speech powers or periodic behavior.
void resolve_avernus_drain(P_obj, P_char, P_char, int damage, int heal_cap);

#endif
