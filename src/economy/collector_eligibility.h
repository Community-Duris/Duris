#ifndef DURIS_COLLECTOR_ELIGIBILITY_H
#define DURIS_COLLECTOR_ELIGIBILITY_H

#include "player/player_snapshot.h"

// Shared by game-thread intake and both persistence backends. Eligibility is
// evaluated from the immutable item snapshot carried by the custody command.
bool collector_death_item_snapshot_eligible(const player_item_snapshot &item);

#endif
