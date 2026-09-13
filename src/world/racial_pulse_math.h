#pragma once

// Value rules for the 'pulse' command, kept free of engine types so a harness can test them.

#include <cctype>
#include <cstdlib>

enum racial_pulse_table
{
	RACIAL_PULSE_CAST,
	RACIAL_PULSE_MELEE
};

// Casting multiplies every spell's cast time; melee is the base round in beats. Lower is
// faster in both.
constexpr double RACIAL_PULSE_CAST_MIN = 0.010;
constexpr double RACIAL_PULSE_CAST_MAX = 5.000;
constexpr double RACIAL_PULSE_MELEE_MIN = 1.000;
constexpr double RACIAL_PULSE_MELEE_MAX = 40.000;

// An absolute value written with exactly three decimals: one to three digits, a point and
// three digits ("0.895", "12.000"). A sign, spaces or any other form is refused.
inline bool racial_pulse_parse_value(const char *text, double *value)
{
	if (!text || !value)
		return false;
	const char *p = text;
	int whole = 0;
	while (std::isdigit(static_cast<unsigned char>(*p)))
	{
		++p;
		++whole;
	}
	if (whole == 0 || whole > 3 || *p != '.')
		return false;
	++p;
	for (int decimal = 0; decimal < 3; ++decimal, ++p)
	{
		if (!std::isdigit(static_cast<unsigned char>(*p)))
			return false;
	}
	if (*p)
		return false;
	*value = std::strtod(text, nullptr);
	return true;
}

inline bool racial_pulse_in_range(racial_pulse_table table, double value)
{
	if (table == RACIAL_PULSE_CAST)
		return value >= RACIAL_PULSE_CAST_MIN && value <= RACIAL_PULSE_CAST_MAX;
	return value >= RACIAL_PULSE_MELEE_MIN && value <= RACIAL_PULSE_MELEE_MAX;
}

inline const char *racial_pulse_property_prefix(racial_pulse_table table)
{
	return table == RACIAL_PULSE_CAST ? "spellcast.pulse.racial." : "damage.pulse.racial.";
}
