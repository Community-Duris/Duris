#include "core/random.h"
#include <algorithm>
#include <limits>
#include <unistd.h>
#include <stdint.h>
#ifdef _WIN32
#include <bcrypt.h>
#include <process.h>
#endif

static uint64_t rng_state[4];

namespace
{
struct native_constructor_random_scope
{
	std::array<uint64_t, 4> state{};
	uint64_t draws = 0, expected_draws = 0;
	bool replay = false, invalid = false;
};
thread_local native_constructor_random_scope *native_constructor_random = nullptr;

class native_constructor_random_lease final
{
    public:
	explicit native_constructor_random_lease(native_constructor_random_scope &scope) noexcept
	{
		native_constructor_random = &scope;
	}
	~native_constructor_random_lease() { native_constructor_random = nullptr; }
	native_constructor_random_lease(const native_constructor_random_lease &) = delete;
	native_constructor_random_lease &
	operator=(const native_constructor_random_lease &) = delete;
};
bool nonzero_random_state(const std::array<uint64_t, 4> &state) noexcept
{
	return state[0] || state[1] || state[2] || state[3];
}
} // namespace

uint64_t hash64(uint64_t x)
{
	x += 0x9e3779b97f4a7c15;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	return x ^ (x >> 31);
}

/*
 * xoshiro256** random generator
 *
 * Fastest available good PRNG as of 2018 (sub-nanosecond per entry), produces
 * much better output than old stuff like rand() or Mersenne's Twister.
 *
 * By David Blackman and Sebastiano Vigna; PD/CC0 2018.
 *
 * It has a period of 2²⁵⁶-1, excluding all-zero state; it must always get
 * initialized to avoid that zero.
 */

static inline uint64_t rotl(const uint64_t x, int k)
{
	/* optimized to a single instruction on x86 */
	return (x << k) | (x >> (64 - k));
}

static uint64_t rnd64(void)
{
	auto *scope = native_constructor_random;
	uint64_t *state = scope && scope->replay ? scope->state.data() : rng_state;
	if (scope)
	{
		if (scope->draws == std::numeric_limits<uint64_t>::max())
			scope->invalid = true;
		else
			++scope->draws;
		if (scope->replay && scope->draws > scope->expected_draws)
			scope->invalid = true;
	}
	const uint64_t result = rotl(state[1] * 5, 7) * 9;
	const uint64_t t = state[1] << 17;

	state[2] ^= state[0];
	state[3] ^= state[1];
	state[1] ^= state[2];
	state[0] ^= state[3];

	state[2] ^= t;

	state[3] = rotl(state[3], 45);

	return result;
}

void randomize(uint64_t seed)
{
	if (native_constructor_random)
	{
		// A constructor recipe cannot reproduce reseeding. In replay, prevent
		// that unsupported path from touching the process generator at all.
		native_constructor_random->invalid = true;
		if (native_constructor_random->replay)
			return;
	}
	if (!seed)
	{
#if defined(_WIN32) && _WIN32
#pragma comment(lib, "Bcrypt.lib")
		if (BCryptGenRandom(NULL, (PUCHAR)rng_state, sizeof rng_state,
				    BCRYPT_USE_SYSTEM_PREFERRED_RNG))
		{
			return;
		}
#else
		if (!getentropy(rng_state, sizeof rng_state))
			return;
#endif
		seed = (uint64_t)getpid();
	}

	rng_state[0] = hash64(seed);
	rng_state[1] = hash64(rng_state[0]);
	rng_state[2] = hash64(rng_state[1]);
	rng_state[3] = hash64(rng_state[2]);
}

int number(int from, int to)
{
	// Biased in theory, but we won't hit a single biased roll during
	// the game's lifetime, thus no need to bother with rejection
	// sampling.
	if (from < to)
		return from + rnd64() % (to - from + 1);
	if (from == to)
		return from;
	else
		return from - rnd64() % (from - to + 1);
}

bool native_mobile_birth_random_owner::capture(bool (*construct)(void *), void *context,
					       native_mobile_birth_random_recipe *output) noexcept
{
	if (!construct || !output || native_constructor_random)
		return false;
	native_mobile_birth_random_recipe recipe;
	std::copy_n(rng_state, 4, recipe.initial.begin());
	if (!nonzero_random_state(recipe.initial))
		return false;
	native_constructor_random_scope scope;
	native_constructor_random_lease lease(scope);
	try
	{
		if (!construct(context) || scope.invalid)
			return false;
		std::copy_n(rng_state, 4, recipe.terminal.begin());
		if (!nonzero_random_state(recipe.terminal))
			return false;
		recipe.draws = scope.draws;
		*output = recipe;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool native_mobile_birth_random_owner::replay(const native_mobile_birth_random_recipe &recipe,
					      bool (*construct)(void *), void *context) noexcept
{
	if (!construct || native_constructor_random || !nonzero_random_state(recipe.initial) ||
	    !nonzero_random_state(recipe.terminal))
		return false;
	native_constructor_random_scope scope;
	scope.state = recipe.initial;
	scope.expected_draws = recipe.draws;
	scope.replay = true;
	native_constructor_random_lease lease(scope);
	try
	{
		return construct(context) && !scope.invalid && scope.draws == recipe.draws &&
		       scope.state == recipe.terminal;
	}
	catch (...)
	{
		return false;
	}
}
