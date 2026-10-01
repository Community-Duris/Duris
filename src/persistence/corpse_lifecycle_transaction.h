#ifndef DURIS_CORPSE_LIFECYCLE_TRANSACTION_H
#define DURIS_CORPSE_LIFECYCLE_TRANSACTION_H

#include "persistence/corpse_lifecycle_command.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CORPSE_LIFECYCLE_PENDING_MAX = 1024;
// A stale corpse action gets one authoritative-revision retry.  The delay is
// expressed in transaction pulses so the game thread remains non-blocking and
// tests can exercise the state machine without sleeping.
constexpr unsigned int CORPSE_LIFECYCLE_STALE_RETRY_LIMIT = 1;
constexpr unsigned int CORPSE_LIFECYCLE_STALE_RETRY_BACKOFF_PULSES = 4;

using corpse_lifecycle_release_completion_fn = void (*)(bool committed,
							const corpse_lifecycle_result &result,
							unsigned int error_code,
							const corpse_lifecycle_payload &payload);

struct corpse_lifecycle_transaction_health
{
	uint64_t tracked = 0;
	uint64_t pending = 0;
	uint64_t dirty = 0;
	uint64_t fenced = 0;
	uint64_t submitted = 0;
	uint64_t committed = 0;
	uint64_t rejected = 0;
	uint64_t submission_failures = 0;
};

bool corpse_lifecycle_transaction_stage(const corpse_lifecycle_payload &payload);
bool corpse_lifecycle_transaction_release(const corpse_lifecycle_payload &payload,
					  corpse_lifecycle_release_completion_fn completion);
bool corpse_lifecycle_transaction_destroy(const corpse_lifecycle_payload &payload,
					  corpse_lifecycle_release_completion_fn completion);
bool corpse_lifecycle_transaction_resurrect(const corpse_lifecycle_payload &payload,
					    corpse_lifecycle_release_completion_fn completion);
bool corpse_lifecycle_transaction_raise_follower(const corpse_lifecycle_payload &payload,
						 corpse_lifecycle_release_completion_fn completion);
bool corpse_lifecycle_transaction_raise_world_follower(
	const corpse_lifecycle_payload &payload, uint64_t source_item_revision,
	corpse_lifecycle_release_completion_fn completion);
bool corpse_lifecycle_transaction_hydrate(uint32_t owner_pid, uint32_t save_id,
					  uint64_t corpse_revision);
bool corpse_lifecycle_transaction_note_item_transfer(uint32_t owner_pid, uint32_t save_id,
						     uint64_t corpse_revision);
bool corpse_lifecycle_transaction_forget(uint32_t owner_pid, uint32_t save_id);
void corpse_lifecycle_transaction_pulse(void);
void corpse_lifecycle_transaction_handle_completions(const critical_completion *completions,
						     size_t count);
critical_outbox_delivery_result
corpse_lifecycle_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void corpse_lifecycle_transaction_publish_outbox(void);
bool corpse_lifecycle_transaction_busy(uint32_t owner_pid, uint32_t save_id);
corpse_lifecycle_transaction_health corpse_lifecycle_transaction_health_copy(void);
void corpse_lifecycle_transaction_reset_for_tests(void);

#endif
