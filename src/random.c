#include <unistd.h>
#include <stdint.h>
#ifdef _WIN32
#include <bcrypt.h>
#include <process.h>
#endif

static uint64_t rng_state[4];

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
	const uint64_t result = rotl(rng_state[1] * 5, 7) * 9;
	const uint64_t t = rng_state[1] << 17;

	rng_state[2] ^= rng_state[0];
	rng_state[3] ^= rng_state[1];
	rng_state[1] ^= rng_state[2];
	rng_state[0] ^= rng_state[3];

	rng_state[2] ^= t;

	rng_state[3] = rotl(rng_state[3], 45);

	return result;
}

void randomize(uint64_t seed)
{
	if (!seed) {
#if _WIN32
#pragma comment(lib, "Bcrypt.lib")
		if (BCryptGenRandom(NULL, (PUCHAR)rng_state, sizeof rng_state,
			BCRYPT_USE_SYSTEM_PREFERRED_RNG)) {
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
