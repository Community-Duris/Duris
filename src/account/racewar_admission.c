#include "account/racewar_admission.h"

#include <limits.h>

namespace
{
long account_racewar_cooldown_remaining(long opposite_admission, long now, long cooldown_seconds)
{
	if (opposite_admission <= 0 || cooldown_seconds <= 0)
		return 0;

	if (now >= opposite_admission)
	{
		const long elapsed = now - opposite_admission;
		return elapsed < cooldown_seconds ? cooldown_seconds - elapsed : 0;
	}

	if (now < 0 && opposite_admission > LONG_MAX + now)
		return LONG_MAX;
	const long clock_skew = opposite_admission - now;
	return clock_skew > LONG_MAX - cooldown_seconds ? LONG_MAX : cooldown_seconds + clock_skew;
}
} // namespace

account_racewar_admission account_racewar_evaluate(enum account_racewar_side side, bool blocked,
						   long last_good, long last_evil, long now,
						   long cooldown_seconds)
{
	account_racewar_admission result = { true, ACCOUNT_RACEWAR_DENIAL_NONE, side, 0 };

	if (blocked)
	{
		result.allowed = false;
		result.denial = ACCOUNT_RACEWAR_DENIAL_BLOCKED;
		return result;
	}

	if (side == ACCOUNT_RACEWAR_EXEMPT || side == ACCOUNT_RACEWAR_UNRESTRICTED)
		return result;

	const long opposite_admission = side == ACCOUNT_RACEWAR_GOOD ? last_evil : last_good;
	result.remaining_seconds =
		account_racewar_cooldown_remaining(opposite_admission, now, cooldown_seconds);
	if (result.remaining_seconds > 0)
	{
		result.allowed = false;
		result.denial = ACCOUNT_RACEWAR_DENIAL_COOLDOWN;
	}
	return result;
}
