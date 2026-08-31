#ifndef EPIC_TRANSACTION_H
#define EPIC_TRANSACTION_H

#include "persistence/critical_command_coordinator.h"
#include "world/epic_command.h"
#include "structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t EPIC_PENDING_MAX = 1024;
constexpr size_t EPIC_PENDING_CONTEXT_MAX_BYTES = 64;

using epic_completion_fn = void (*)(P_char character, bool committed,
				    const epic_command_result &result, unsigned int error_code,
				    const uint8_t *context, size_t context_size);

struct epic_transaction_health
{
	uint64_t pending;
	uint64_t retained_offline;
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t submission_failures;
	uint64_t malformed_completions;
};

bool epic_transaction_submit(P_char character, int64_t delta, epic_reason_type reason,
			     int64_t reason_id, uint16_t flags, critical_source_site source_site,
			     critical_deadline_class deadline_class, epic_completion_fn completion,
			     const void *context, size_t context_size);
bool epic_transaction_submit_identified(P_char character, const critical_operation_id &operation_id,
					int64_t delta, epic_reason_type reason, int64_t reason_id,
					uint16_t flags, critical_source_site source_site,
					critical_deadline_class deadline_class,
					epic_completion_fn completion, const void *context,
					size_t context_size);
void epic_transaction_handle_completions(const critical_completion *completions, size_t count);
void epic_transaction_player_ready(P_char character);
epic_transaction_health epic_transaction_health_copy(void);
void epic_transaction_reset_for_tests(void);
bool epic_transaction_publish_balance(P_char character, int64_t balance, uint64_t revision);

#endif
