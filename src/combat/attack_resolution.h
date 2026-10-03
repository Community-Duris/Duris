#ifndef DURIS_COMBAT_ATTACK_RESOLUTION_H
#define DURIS_COMBAT_ATTACK_RESOLUTION_H

#include "core/structs.h"

enum
{
	PROCCING_SLOTS_COUNT = 31
};
extern int proccing_slots[PROCCING_SLOTS_COUNT];

int pv_common(P_char ch, P_char opponent, const P_obj wpn, int *damAccumulator);
bool monk_superhit(P_char ch, P_char victim, int *damAccumulator);

#endif
