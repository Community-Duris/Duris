#pragma once

#include "net/output_preference_state.h"
#include <charconv>
#include <string>
#include <string_view>

inline bool valid_output_preference_choice(unsigned value)
{
	return value <= 3 || (value >= 17 && value <= 31);
}

inline std::string encode_output_preferences(const OutputPreferenceState &state)
{
	std::string result = "v1";
	if (state.motion_off)
		result += ";m=1";
	for (size_t channel = 1; channel < (size_t)OutputChannel::Count; ++channel)
		if (state.choices[channel] &&
		    valid_output_preference_choice(state.choices[channel]))
			result += ";" + std::to_string(channel) + "=" +
				  std::to_string(state.choices[channel]);
	return result == "v1" ? std::string{} : result;
}

// Bounded, versioned, independent fields. Ignore obsolete IDs/values, never
// interpreting arbitrary markup. Duplicate known fields reset just that field.
inline OutputPreferenceState decode_output_preferences(std::string_view text)
{
	OutputPreferenceState state{};
	if (text.empty() || text.size() > OUTPUT_PREFERENCE_MAX_BYTES ||
	    (text != "v1" && !text.starts_with("v1;")))
		return state;
	bool seen[(size_t)OutputChannel::Count]{};
	bool motion_seen = false;
	auto number = [](std::string_view input, unsigned &value)
	{
		auto parsed = std::from_chars(input.data(), input.data() + input.size(), value);
		return parsed.ec == std::errc{} && parsed.ptr == input.data() + input.size();
	};
	text.remove_prefix(2);
	while (!text.empty())
	{
		text.remove_prefix(1); // semicolon
		auto end = text.find(';');
		auto field = text.substr(0, end);
		auto equals = field.find('=');
		if (equals != std::string_view::npos)
		{
			auto key = field.substr(0, equals);
			auto value = field.substr(equals + 1);
			unsigned decoded = 0, channel = 0;
			if (key == "m")
			{
				state.motion_off = !motion_seen && value == "1";
				motion_seen = true;
			}
			else if (number(key, channel) && channel > 0 &&
				 channel < (size_t)OutputChannel::Count)
			{
				state.choices[channel] =
					!seen[channel] && number(value, decoded) &&
							valid_output_preference_choice(decoded) ?
						decoded :
						0;
				seen[channel] = true;
			}
		}
		if (end == std::string_view::npos)
			break;
		text.remove_prefix(end);
	}
	return state;
}
