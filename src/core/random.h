#ifndef DURIS_CORE_RANDOM_H
#define DURIS_CORE_RANDOM_H

#include <array>
#include <cstdint>

int number(int from, int to);

// Values only: original constructor draws, never economic or runtime authority.
struct native_mobile_birth_random_recipe
{
	std::array<uint64_t, 4> initial{}, terminal{};
	uint64_t draws = 0;
};

class quest_mobile_native_stage;
class native_mobile_birth_random_owner final
{
    private:
	friend class quest_mobile_native_stage;
	// Capture actual global draws in the original synchronous detached constructor.
	// Replay uses only that original substream, leaving the process RNG unchanged.
	static bool capture(bool (*construct)(void *), void *,
			    native_mobile_birth_random_recipe *) noexcept;
	static bool replay(const native_mobile_birth_random_recipe &, bool (*construct)(void *),
			   void *) noexcept;
};

#endif
