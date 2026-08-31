#ifndef COMBAT_OUTCOME_TRANSACTION_H
#define COMBAT_OUTCOME_TRANSACTION_H

#include "combat/combat_outcome_command.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "core/prototypes.h"

using combat_outcome_completion_fn = void (*)(bool committed, const combat_outcome_result &result,
					      unsigned int error_code,
					      const combat_outcome_payload &payload);

constexpr size_t COMBAT_OUTCOME_PENDING_MAX = 128;

struct combat_outcome_health
{
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t malformed_completions;
	uint64_t submission_failures;
	uint64_t pending;
	uint64_t max_participants;
	uint64_t oldest_age_msec;
};

bool combat_outcome_transaction_submit(const combat_outcome_payload &payload,
				       combat_outcome_completion_fn completion,
				       critical_operation_id *submitted_operation_id = nullptr);
void combat_outcome_transaction_handle_completions(const critical_completion *completions,
						   size_t count);
critical_outbox_delivery_result
combat_outcome_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void combat_outcome_transaction_publish_outbox(void);
combat_outcome_health combat_outcome_transaction_health_copy(void);
void combat_outcome_transaction_reset_for_tests(void);

#endif
