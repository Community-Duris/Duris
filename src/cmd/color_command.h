#pragma once

#include "net/output_profiles.h"
#include <string>

struct char_data;

struct ColorCommandChannel
{
	OutputChannel channel;
	std::string_view name;
	std::string_view alias;
	std::string_view description;
	std::string_view sample;
	bool modes;
};

std::span<const ColorCommandChannel> color_command_channels();
std::string output_preference_label(const OutputProfilePreferences &, OutputChannel);

struct ColorCommandResult
{
	OutputProfilePreferences preferences;
	std::string text;
	bool mutation = false;
};

// Pure command planning and preview. The adapter below owns save admission.
ColorCommandResult evaluate_color_command(std::string_view arguments,
					  const OutputProfilePreferences &preferences,
					  std::shared_ptr<const OutputProfileSnapshot> snapshot);
void do_color_preferences(char_data *recipient, const char *arguments);
