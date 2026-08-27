#ifndef ARTIFACT_GUILD_TRANSACTION_H
#define ARTIFACT_GUILD_TRANSACTION_H

#include "artifact_guild_state.h"
#include "critical_command_coordinator.h"
#include "critical_outbox.h"

constexpr size_t ARTIFACT_GUILD_PENDING_MAX = 256;

struct artifact_guild_health
{
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t unavailable;
	uint64_t submission_failures;
	uint64_t malformed_completions;
	uint64_t pending;
	uint64_t max_artifacts;
	uint64_t oldest_age_msec;
};

bool artifact_guild_transaction_submit(P_char character,
				       const critical_operation_id &parent_operation_id, int epics,
				       int epic_type);
void artifact_guild_transaction_handle_completions(const critical_completion *completions,
						   size_t count);
critical_outbox_delivery_result
artifact_guild_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void artifact_guild_transaction_publish_outbox(void);
artifact_guild_health artifact_guild_transaction_health_copy(void);
void artifact_guild_transaction_reset_for_tests(void);

#endif
