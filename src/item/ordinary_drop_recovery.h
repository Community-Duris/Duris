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

// Retained by the original movement entry. Native handlers may have completed
// only part of their bookkeeping when they throw; graph placement alone cannot
// erase that distinction. Only the synchronous publisher mutates these stages.
class ordinary_drop_live_publication_state
{
    public:
	ordinary_drop_live_publication_state() noexcept = default;

    private:
	friend class ordinary_drop_live_publication_owner;
	critical_operation_id operation_ = {};
	uint64_t actor_runtime_id_ = 0;
	bool bound_ = false;
	bool departure_started_ = false, departure_returned_ = false;
	bool placement_started_ = false, placement_returned_ = false;
};

// Publish the original carried graph under fresh native custody and receipt
// authority. No all-absent construction occurs. Retained stages prevent repeating
// native handler effects across cleanup, registry or ACK retries.
ordinary_drop_observation ordinary_drop_recovery_publish_live(
	const critical_command &original_command, const critical_completion &sealed_completion,
	uint64_t actor_runtime_id, ordinary_drop_live_publication_state &state) noexcept;

#endif
