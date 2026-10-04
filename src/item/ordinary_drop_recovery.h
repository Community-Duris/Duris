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

#endif
