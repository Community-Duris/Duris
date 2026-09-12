#ifndef PET_RESTORE_RUNTIME_H
#define PET_RESTORE_RUNTIME_H

#include "player/player_snapshot.h"

struct char_data;
typedef char_data *P_char;

struct player_held_pet_state
{
	std::vector<player_pet_snapshot> pets;
};

void summoned_pet_mark(P_char pet, summoned_pet_kind kind);
bool summoned_pet_capture(P_char pet, std::string *encoded);
bool summoned_pet_apply(P_char pet, const pet_restore_state &state);
int summoned_pet_capacity(P_char owner);
void summoned_pet_restore_lifetime(P_char pet, P_char owner, const pet_restore_state &state);

#endif
