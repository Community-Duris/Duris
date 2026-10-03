#ifndef DURIS_CHARACTER_MAINTENANCE_H
#define DURIS_CHARACTER_MAINTENANCE_H

#include "core/structs.h"

void character_maintenance_init();
void character_maintenance_enter(P_char character);
void character_maintenance_leave(P_char character);
void character_maintenance_changed(P_char character);

#endif
