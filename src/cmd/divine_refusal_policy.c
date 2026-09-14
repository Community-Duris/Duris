#include "cmd/divine_refusal_policy.h"

#include <algorithm>
#include <cmath>
#include <limits>

divine_refusal_config divine_refusal_make_config(float enabled, float summoner_only, float percent,
						 float retry_lock_seconds,
						 unsigned int pulses_per_second)
{
	divine_refusal_config config;
	config.enabled = std::isfinite(enabled) && enabled >= 0.5f;
	config.summoner_only = !std::isfinite(summoner_only) || summoner_only >= 0.5f;

	if (std::isfinite(percent))
		config.percent = std::clamp(percent, 0.0f, 100.0f);

	if (!config.enabled)
		return config;
	if (!std::isfinite(retry_lock_seconds) || retry_lock_seconds <= 0.0f ||
	    pulses_per_second == 0)
	{
		config.enabled = false;
		return config;
	}

	const double bounded_seconds = std::min(static_cast<double>(retry_lock_seconds),
						DIVINE_REFUSAL_MAX_LOCK_SECONDS * 1.0);
	config.lock_pulses = static_cast<divine_refusal_tick>(
		std::ceil(bounded_seconds * static_cast<double>(pulses_per_second)));
	return config;
}

divine_refusal_outcome divine_refusal_decide(const divine_refusal_config &config, bool eligible,
					     bool command_recognized, bool command_exempt,
					     bool command_blocked, divine_refusal_tick now,
					     divine_refusal_tick *refusal_until,
					     divine_refusal_roll_fn roll)
{
	if (!config.enabled || config.percent <= 0 || config.lock_pulses == 0 || !eligible ||
	    !command_recognized || command_exempt || command_blocked || !refusal_until || !roll)
		return divine_refusal_outcome::bypassed;

	if (*refusal_until > now)
		return divine_refusal_outcome::refused_active;

	const float refusal_threshold =
		config.percent * (static_cast<float>(DIVINE_REFUSAL_ROLL_SCALE) / 100.0f);
	if (static_cast<float>(roll(1, DIVINE_REFUSAL_ROLL_SCALE)) > refusal_threshold)
	{
		*refusal_until = 0;
		return divine_refusal_outcome::allowed;
	}

	const divine_refusal_tick maximum = std::numeric_limits<divine_refusal_tick>::max();
	*refusal_until = maximum - now < config.lock_pulses ? maximum : now + config.lock_pulses;
	return divine_refusal_outcome::refused_new;
}
