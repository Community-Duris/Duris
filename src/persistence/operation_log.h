#ifndef DURIS_OPERATION_LOG_H
#define DURIS_OPERATION_LOG_H

#include "persistence/critical_command_coordinator.h"

// One ordered redo log. Admission is serialized through append and writer
// reservation, so a later checkpoint cannot overtake a transfer. Call init
// before hydrating any world/player state and shutdown after stopping the writer.
bool operation_log_init(const char *directory, critical_apply_fn project, void *context);
void operation_log_shutdown();
critical_submit_result operation_log_submit(const critical_command &outcome);
struct player_snapshot;
critical_submit_result operation_log_submit_player(const critical_command &outcome,
						   const player_snapshot &snapshot);
bool operation_log_ready();
bool operation_log_projection_caught_up();
// Rejections have no state mutation to replay, but their request identity and
// result still survive retries, cache eviction and restart.
bool operation_log_remember_rejection(const critical_command &, const critical_apply_result &);
// Consult durable identity before preparing an outcome, including after cache
// eviction/restart. The request's acceptance timestamp is not its identity.
critical_receipt_status operation_log_lookup(const critical_command &request,
					     critical_apply_result *receipt);

// A sealed outcome is a promise, not a request for the database to decide again.
bool operation_log_result_matches(const critical_command &, const critical_apply_result &) noexcept;

#endif
