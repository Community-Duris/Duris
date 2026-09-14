#include "net/casting_pulse_policy.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
float finite_or(float value, float fallback)
{
	return std::isfinite(value) ? value : fallback;
}

float bounded_percent(float value)
{
	return std::clamp(value, 0.0f, 100.0f);
}
} // namespace

casting_pulse_config casting_pulse_make_config(float quick_multiplier, float tank_success_percent,
					       float skill_base_percent,
					       float skill_percent_per_point,
					       float abort_base_percent,
					       float abort_agility_reduction,
					       float abort_cap_percent)
{
	casting_pulse_config config;
	config.quick_multiplier = std::clamp(
		finite_or(quick_multiplier, CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER), 0.1f, 1.0f);
	config.tank_success_percent = bounded_percent(
		finite_or(tank_success_percent, CASTING_PULSE_DEFAULT_TANK_SUCCESS_PERCENT));
	config.skill_base_percent =
		std::clamp(finite_or(skill_base_percent, CASTING_PULSE_DEFAULT_SKILL_BASE_PERCENT),
			   -100.0f, 100.0f);
	config.skill_percent_per_point = std::clamp(
		finite_or(skill_percent_per_point, CASTING_PULSE_DEFAULT_SKILL_PERCENT_PER_POINT),
		0.0f, 5.0f);
	config.abort_base_percent = bounded_percent(
		finite_or(abort_base_percent, CASTING_PULSE_DEFAULT_ABORT_BASE_PERCENT));
	config.abort_agility_reduction = std::clamp(
		finite_or(abort_agility_reduction, CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION),
		0.0f, 5.0f);
	config.abort_cap_percent = bounded_percent(
		finite_or(abort_cap_percent, CASTING_PULSE_DEFAULT_ABORT_CAP_PERCENT));
	return config;
}

float casting_pulse_quick_chant_percent(const casting_pulse_config &config, int skill)
{
	const int bounded_skill = std::clamp(skill, 0, 100);
	const float base = std::clamp(finite_or(config.skill_base_percent,
						CASTING_PULSE_DEFAULT_SKILL_BASE_PERCENT),
				      -100.0f, 100.0f);
	const float per_point = std::clamp(finite_or(config.skill_percent_per_point,
						     CASTING_PULSE_DEFAULT_SKILL_PERCENT_PER_POINT),
					   0.0f, 5.0f);
	return bounded_percent(base + per_point * static_cast<float>(bounded_skill));
}

float casting_pulse_max_circle_abort_percent(const casting_pulse_config &config, int agility)
{
	const float base = bounded_percent(
		finite_or(config.abort_base_percent, CASTING_PULSE_DEFAULT_ABORT_BASE_PERCENT));
	const float reduction = std::clamp(finite_or(config.abort_agility_reduction,
						     CASTING_PULSE_DEFAULT_ABORT_AGILITY_REDUCTION),
					   0.0f, 5.0f);
	const float cap = bounded_percent(
		finite_or(config.abort_cap_percent, CASTING_PULSE_DEFAULT_ABORT_CAP_PERCENT));
	const float adjusted = base - reduction * static_cast<float>(std::max(0, agility));
	return std::min(cap, bounded_percent(adjusted));
}

bool casting_pulse_percent_roll(float percent, int roll)
{
	if (!std::isfinite(percent) || roll < 1 || roll > CASTING_PULSE_ROLL_SCALE)
		return false;
	const long threshold = std::lround(static_cast<double>(bounded_percent(percent)) *
					   (static_cast<double>(CASTING_PULSE_ROLL_SCALE) / 100.0));
	return roll <= threshold;
}

casting_pulse_timing casting_pulse_timing_for(const casting_pulse_config &config, int base_beats,
					      bool quick_chant_succeeded)
{
	const int bounded_base = std::max(1, base_beats);
	const float multiplier = std::clamp(finite_or(config.quick_multiplier,
						      CASTING_PULSE_DEFAULT_QUICK_MULTIPLIER),
					    0.1f, 1.0f);
	const double scaled = static_cast<double>(bounded_base) * multiplier;
	const int landing_beats =
		quick_chant_succeeded ?
			std::max(1, static_cast<int>(std::min(
					    scaled, static_cast<double>(
							    std::numeric_limits<int>::max())))) :
			bounded_base;
	return { landing_beats, landing_beats };
}
