#ifndef DURIS_TROPHY_STATE_H
#define DURIS_TROPHY_STATE_H

#include <cstddef>

// Raw accepted XP for observation, retaining the existing snapshot/SQL format.
// Normalized familiarity and timestamped recovery require a versioned successor.
constexpr size_t ZONE_TROPHY_MAX_ZONES = 1024;

struct zone_trophy_data
{
	int zone_number;
	int exp;
};

#endif
