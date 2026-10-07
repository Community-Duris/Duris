#ifndef DURIS_ENHANCEMENT_STAT_RULES_H
#define DURIS_ENHANCEMENT_STAT_RULES_H

#include <climits>
#include <cmath>

inline int enhancement_stat_cap(int base_modifier, double multiplier)
{
	if (base_modifier <= 0 || !std::isfinite(multiplier) || multiplier <= 0.0)
		return 0;
	const double cap = base_modifier * multiplier;
	return cap >= SCHAR_MAX ? SCHAR_MAX : static_cast<int>(cap);
}

#endif
