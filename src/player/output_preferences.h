#pragma once

#include "net/output_profiles.h"

struct char_data;

enum class OutputPreferenceUpdate
{
	Unchanged,
	PendingSave,
	Unavailable
};

OutputProfilePreferences player_output_preferences(char_data *recipient);
OutputPreferenceUpdate
update_player_output_preferences(char_data *recipient, const OutputProfilePreferences &preferences);
ResolvedOutputProfile player_output_profile(char_data *recipient, OutputChannel channel,
					    OutputPolicy caller_policy);

ResolvedOutputProfile player_output_profile(char_data *recipient, const OutputContext &context);
