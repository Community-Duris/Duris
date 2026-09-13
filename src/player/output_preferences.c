#include "player/output_preferences.h"
#include "core/prototypes.h"
#include "core/files.h"
#include "core/utils.h"
#include "player/player_save_pipeline.h"
#include "world/db.h"

namespace
{
P_char preference_owner(P_char recipient)
{
	if (!recipient)
		return nullptr;
	P_char owner = GET_PLYR(recipient);
	return owner && IS_PC(owner) && owner->only.pc ? owner : nullptr;
}
}

OutputProfilePreferences player_output_preferences(P_char recipient)
{
	P_char owner = preference_owner(recipient);
	return owner ? OutputProfilePreferences::from_state(owner->only.pc->output_preferences) :
		       OutputProfilePreferences{};
}

OutputPreferenceUpdate update_player_output_preferences(P_char recipient,
							const OutputProfilePreferences &preferences)
{
	P_char owner = preference_owner(recipient);
	if (!owner || GET_PID(owner) <= 0)
		return OutputPreferenceUpdate::Unavailable;
	const auto before = owner->only.pc->output_preferences;
	const auto after = preferences.state();
	if (before == after)
		return OutputPreferenceUpdate::Unchanged;
	owner->only.pc->output_preferences = after;
	const int room = owner->in_room == NOWHERE ? NOWHERE : world[owner->in_room].number;
	const auto result =
		player_save_pipeline_request(owner, PLAYER_COMPONENT_STATUS, RENT_CRASH, room);
	if (result == player_save_pipeline_result::queued ||
	    result == player_save_pipeline_result::coalesced)
		return OutputPreferenceUpdate::PendingSave;
	// No immutable candidate was admitted. Leave the character's old settings
	// active; any retained dirty marker can safely checkpoint those originals.
	owner->only.pc->output_preferences = before;
	return OutputPreferenceUpdate::Unavailable;
}

ResolvedOutputProfile player_output_profile(P_char recipient, OutputChannel channel,
					    OutputPolicy caller_policy)
{
	return resolve_output_profile(output_profile_registry().snapshot(), channel, caller_policy,
				      player_output_preferences(recipient));
}
