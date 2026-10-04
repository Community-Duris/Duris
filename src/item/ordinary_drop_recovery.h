#ifndef ORDINARY_DROP_RECOVERY_H
#define ORDINARY_DROP_RECOVERY_H

#include "persistence/critical_command_completion.h"

enum class ordinary_drop_observation_status : uint8_t
{
	verified_existing,
	absent,
	conflict,
	unsupported,
	refused,
	unavailable,
	published,
};

struct ordinary_drop_observation
{
	ordinary_drop_observation_status status = ordinary_drop_observation_status::unavailable;
	unsigned int error_code = 0;
	uint64_t witness_item_uid = 0;
	uint64_t room_revision = 0;
};

// Read-only, actor-independent game-thread observation of one retained ordinary
// SQL drop. Success describes the graph at this call, after confirmed SQL lock
// cleanup; it is not a publication/ACK capability or a reusable SQL proof.
// Absent never authorizes construction. No object or registry state is changed.
ordinary_drop_observation
ordinary_drop_recovery_observe_existing(const critical_command &original_command,
					const critical_completion &sealed_completion) noexcept;

// Opt-in game-thread projection reconstruction. Reacquires fresh native SQL
// authority internally; an earlier absent observation grants no permission.
// Exact existing graphs are unchanged. Entirely absent eligible graphs use only
// trusted cached prototypes and private inert enrollment. Cache misses and
// unsupported activity/procedure/timer representations remain held. A published
// result requires confirmed SQL cleanup; uncertain cleanup retains any enrolled
// graph for exact re-observation. No economic mutation or publication ACK occurs.
ordinary_drop_observation
ordinary_drop_recovery_publish(const critical_command &original_command,
			       const critical_completion &sealed_completion) noexcept;

#endif
