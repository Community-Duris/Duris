#include "cmd/divine_refusal_policy.h"

#include <cassert>
#include <cmath>
#include <limits>

namespace
{
int draws = 0;
int next_roll = DIVINE_REFUSAL_ROLL_SCALE;

int controlled_roll(int minimum, int maximum)
{
	assert(minimum == 1);
	assert(maximum == DIVINE_REFUSAL_ROLL_SCALE);
	++draws;
	return next_roll;
}

void expect_bypass_without_draw(const divine_refusal_config &config, bool eligible, bool recognized,
				bool exempt, bool blocked)
{
	divine_refusal_tick deadline = 91;
	draws = 0;
	assert(divine_refusal_decide(config, eligible, recognized, exempt, blocked, 50, &deadline,
				     controlled_roll) == divine_refusal_outcome::bypassed);
	assert(draws == 0);
	assert(deadline == 91);
}
} // namespace

int main()
{
	const divine_refusal_config enabled =
		divine_refusal_make_config(1.0f, 1.0f, 10.0f, 4.0f, 4);
	assert(enabled.enabled);
	assert(enabled.summoner_only);
	assert(enabled.percent == 10);
	assert(enabled.lock_pulses == 16);

	const divine_refusal_config disabled =
		divine_refusal_make_config(0.0f, 1.0f, 10.0f, 4.0f, 4);
	expect_bypass_without_draw(disabled, true, true, false, false);
	expect_bypass_without_draw(enabled, false, true, false, false);
	expect_bypass_without_draw(enabled, true, false, false, false);
	expect_bypass_without_draw(enabled, true, true, true, false);
	// The issue requires the ordinary casting/item gate even during an active lock.
	expect_bypass_without_draw(enabled, true, true, false, true);

	const divine_refusal_config zero = divine_refusal_make_config(1.0f, 1.0f, 0.0f, 4.0f, 4);
	expect_bypass_without_draw(zero, true, true, false, false);
	const divine_refusal_config invalid_lock = divine_refusal_make_config(
		1.0f, 1.0f, 10.0f, std::numeric_limits<float>::quiet_NaN(), 4);
	assert(!invalid_lock.enabled);
	const divine_refusal_config negative_lock =
		divine_refusal_make_config(1.0f, 1.0f, 10.0f, -1.0f, 4);
	assert(!negative_lock.enabled);
	const divine_refusal_config zero_rate =
		divine_refusal_make_config(1.0f, 1.0f, 10.0f, 4.0f, 0);
	assert(!zero_rate.enabled);
	const divine_refusal_config fractional =
		divine_refusal_make_config(1.0f, 1.0f, 10.0f, 0.1f, 4);
	assert(fractional.lock_pulses == 1);
	const divine_refusal_config negative_percent =
		divine_refusal_make_config(1.0f, 1.0f, -20.0f, 4.0f, 4);
	assert(negative_percent.percent == 0);
	const divine_refusal_config fractional_percent =
		divine_refusal_make_config(1.0f, 1.0f, 0.8f, 4.0f, 4);
	assert(std::fabs(fractional_percent.percent - 0.8f) < 0.0001f);
	const divine_refusal_config bounded =
		divine_refusal_make_config(1.0f, 0.0f, 120.0f, 600.0f, 4);
	assert(!bounded.summoner_only);
	assert(bounded.percent == 100);
	assert(bounded.lock_pulses == DIVINE_REFUSAL_MAX_LOCK_SECONDS * 4);
	const divine_refusal_config nonfinite = divine_refusal_make_config(
		std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::quiet_NaN(), 4.0f, 4);
	assert(!nonfinite.enabled);
	assert(nonfinite.summoner_only);
	assert(nonfinite.percent == 0);

	divine_refusal_tick deadline = 0;
	draws = 0;
	next_roll = 1;
	assert(divine_refusal_decide(enabled, true, true, false, false, 100, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_new);
	assert(draws == 1);
	assert(deadline == 116);

	// Changing the command/target while the window is active cannot reroll or extend it.
	next_roll = DIVINE_REFUSAL_ROLL_SCALE;
	assert(divine_refusal_decide(enabled, true, true, false, false, 115, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_active);
	assert(draws == 1);
	assert(deadline == 116);

	// The exact boundary is expired and receives one fresh decision.
	assert(divine_refusal_decide(enabled, true, true, false, false, 116, &deadline,
				     controlled_roll) == divine_refusal_outcome::allowed);
	assert(draws == 2);
	assert(deadline == 0);

	// An allowed command cannot bank acceptance for the next command.
	next_roll = DIVINE_REFUSAL_ROLL_SCALE;
	assert(divine_refusal_decide(enabled, true, true, false, false, 200, &deadline,
				     controlled_roll) == divine_refusal_outcome::allowed);
	next_roll = 1;
	assert(divine_refusal_decide(enabled, true, true, false, false, 201, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_new);
	assert(draws == 4);

	// Disabling is immediate but preserves the dormant live-NPC deadline.
	const divine_refusal_tick preserved = deadline;
	draws = 0;
	assert(divine_refusal_decide(disabled, true, true, false, false, 202, &deadline,
				     controlled_roll) == divine_refusal_outcome::bypassed);
	assert(draws == 0);
	assert(deadline == preserved);
	assert(divine_refusal_decide(enabled, true, true, false, false, 202, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_active);
	assert(deadline == preserved);
	assert(draws == 0);

	// 100 percent still performs exactly one draw, then refuses.
	deadline = 0;
	draws = 0;
	next_roll = DIVINE_REFUSAL_ROLL_SCALE;
	assert(divine_refusal_decide(bounded, true, true, false, false, 300, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_new);
	assert(draws == 1);

	// Fractional percentages retain sub-one-percent precision (0.8% = 80/10000).
	deadline = 0;
	draws = 0;
	next_roll = 80;
	assert(divine_refusal_decide(fractional_percent, true, true, false, false, 400, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_new);
	assert(draws == 1);
	deadline = 0;
	next_roll = 81;
	assert(divine_refusal_decide(fractional_percent, true, true, false, false, 401, &deadline,
				     controlled_roll) == divine_refusal_outcome::allowed);
	assert(draws == 2);

	// Saturating addition cannot wrap a deadline into the past.
	deadline = 0;
	next_roll = 1;
	const divine_refusal_tick near_max = std::numeric_limits<divine_refusal_tick>::max() - 2;
	assert(divine_refusal_decide(enabled, true, true, false, false, near_max, &deadline,
				     controlled_roll) == divine_refusal_outcome::refused_new);
	assert(deadline == std::numeric_limits<divine_refusal_tick>::max());

	return 0;
}
