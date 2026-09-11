#ifndef DURIS_ITEM_LORE_H
#define DURIS_ITEM_LORE_H
#include "core/structs.h"
#include <string>
// Capture the current item description without sending output or imposing recovery.
std::string item_lore_description(P_char ch, P_obj obj);
#endif
