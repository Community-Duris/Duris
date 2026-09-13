#ifndef DURIS_DEVICE_ACTIONS_H
#define DURIS_DEVICE_ACTIONS_H

#include "item/item_actions.h"

item_action_start begin_device_action(P_obj source, P_char actor, const char *arguments);
void update_device_action_properties();
// Physical removal of committed scrolls occurs after command/event callbacks
// unwind. The ink is already consumed synchronously at acceptance.
void device_actions_pulse();

#endif
