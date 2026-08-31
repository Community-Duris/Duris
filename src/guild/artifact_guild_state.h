#ifndef ARTIFACT_GUILD_STATE_H
#define ARTIFACT_GUILD_STATE_H

#include "guild/artifact_guild_command.h"
#include "structs.h"

enum class artifact_guild_capture_status : uint8_t
{
	ready,
	no_effect,
	unavailable,
};

bool artifact_guild_state_hydrate(void);
artifact_guild_capture_status
artifact_guild_state_capture(P_char character, int epics, int epic_type,
			     const critical_operation_id &parent_operation_id,
			     artifact_guild_payload *payload);
void artifact_guild_state_publish(const artifact_guild_result &result);
bool artifact_guild_state_ready(void);
void artifact_guild_state_reset_for_tests(void);

#endif
