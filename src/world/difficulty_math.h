#ifndef DURIS_DIFFICULTY_MATH_H
#define DURIS_DIFFICULTY_MATH_H

#include <cmath>
#include <limits>

// Server-wide difficulty dials run 1..10. DIFFICULTY_NEUTRAL is the game exactly as it
// was before the dials existed; every other setting reads a multiplier from the curve
// (difficulty.curve.NN in lib/duris.properties).
constexpr int DIFFICULTY_MIN = 1;
constexpr int DIFFICULTY_MAX = 10;
constexpr int DIFFICULTY_NEUTRAL = 5;
inline constexpr double DIFFICULTY_DEFAULT_CURVE[DIFFICULTY_MAX] = {
	0.50, 0.60, 0.70, 0.85, 1.00, 1.15, 1.30, 1.50, 1.75, 2.00,
};

// A property holds a float; round it to the nearest whole setting inside 1..10.
inline int difficulty_clamp_setting(double value)
{
	if (!std::isfinite(value))
		return DIFFICULTY_NEUTRAL;
	const double rounded = std::round(value);
	if (rounded < DIFFICULTY_MIN)
		return DIFFICULTY_MIN;
	if (rounded > DIFFICULTY_MAX)
		return DIFFICULTY_MAX;
	return static_cast<int>(rounded);
}

// The curve's multiplier for a setting. The neutral setting is always exactly 1.0, so a
// dial left at 5 can never change the game whatever the curve says; an unusable curve
// entry also falls back to 1.0.
inline double difficulty_curve_multiplier(const double *curve, int setting)
{
	if (!curve || setting == DIFFICULTY_NEUTRAL || setting < DIFFICULTY_MIN ||
	    setting > DIFFICULTY_MAX)
		return 1.0;
	const double multiplier = curve[setting - 1];
	return std::isfinite(multiplier) && multiplier > 0.0 ? multiplier : 1.0;
}

// Dials on something that hurts players use the multiplier as it is; dials on something
// players want (experience, loot, regeneration) use its reciprocal, so 10 is always the
// hardest setting for players.
inline double difficulty_effective_multiplier(double curve_multiplier, bool favours_players)
{
	if (curve_multiplier == 1.0)
		return 1.0;
	return favours_players ? 1.0 / curve_multiplier : curve_multiplier;
}

// Scaling helpers return their input untouched at a multiplier of exactly 1.0, so a
// neutral dial is byte-for-byte the old arithmetic rather than a round trip through it.
inline int difficulty_scale_int(int value, double multiplier)
{
	if (multiplier == 1.0)
		return value;
	const double scaled = std::round(static_cast<double>(value) * multiplier);
	if (scaled >= static_cast<double>(std::numeric_limits<int>::max()))
		return std::numeric_limits<int>::max();
	if (scaled <= static_cast<double>(std::numeric_limits<int>::min()))
		return std::numeric_limits<int>::min();
	return static_cast<int>(scaled);
}

inline long difficulty_scale_long(long value, double multiplier)
{
	if (multiplier == 1.0)
		return value;
	const double scaled = std::round(static_cast<double>(value) * multiplier);
	if (scaled >= static_cast<double>(std::numeric_limits<long>::max()))
		return std::numeric_limits<long>::max();
	if (scaled <= static_cast<double>(std::numeric_limits<long>::min()))
		return std::numeric_limits<long>::min();
	return static_cast<long>(scaled);
}

// A percentage chance, scaled and kept inside 1..100.
inline int difficulty_scale_percent(int percent, double multiplier)
{
	if (multiplier == 1.0)
		return percent;
	const int scaled = difficulty_scale_int(percent, multiplier);
	return scaled < 1 ? 1 : scaled > 100 ? 100 : scaled;
}

#endif // DURIS_DIFFICULTY_MATH_H
