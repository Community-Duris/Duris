#ifndef DURIS_WORLD_HANDLER_H
#define DURIS_WORLD_HANDLER_H

#include "core/structs.h"

enum class obj_to_char_result
{
	placed,
	deferred,
	rejected,
	destroyed,
};

// Only placed permits callers to keep using the object without another live lookup.
obj_to_char_result obj_to_char_checked(P_obj object, P_char character);

int can_prime_class_use_item(P_char, P_obj);

// A committed corpse raise may need to defer player serialization until the
// durable item rows have been rehydrated.  The login hook clears this fence
// only after the player's authoritative snapshot has been loaded.
bool corpse_raise_player_save_fenced(P_char);
void corpse_raise_player_ready(P_char, bool inventory_reloaded);
bool corpse_has_death_conflict(P_obj);

#endif
