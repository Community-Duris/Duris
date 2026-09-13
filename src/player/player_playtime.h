#ifndef DURIS_PLAYER_PLAYTIME_H
#define DURIS_PLAYER_PLAYTIME_H

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <limits>

// Compatibility time is resident-character time, not active-input telemetry.
// Keep the loaded/admin-set baseline and logon unchanged across checkpoints.
inline unsigned int player_playtime_total(unsigned int baseline, time_t logon, time_t now)
{
	const uint64_t elapsed = logon > 0 && now > logon ? static_cast<uint64_t>(now - logon) : 0;
	// The durable SQL column is signed INT; never wrap or reject a save at its limit.
	return static_cast<unsigned int>(std::min<uint64_t>(
		static_cast<uint64_t>(baseline) + elapsed, std::numeric_limits<int>::max()));
}

#endif
