#ifndef CRITICAL_COMMAND_REPOSITORY_H
#define CRITICAL_COMMAND_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <mysql/mysql.h>

constexpr size_t CRITICAL_COMMAND_RESULT_MAX_BYTES = 4096;
static_assert(CRITICAL_COMMAND_RESULT_MAX_BYTES <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
constexpr size_t CRITICAL_OUTBOX_PAYLOAD_MAX_BYTES = 65535;

critical_apply_result critical_command_repository_apply(MYSQL *connection,
							const critical_command &command);
critical_apply_result critical_command_repository_reconcile(MYSQL *connection,
							    const critical_command &command);
critical_apply_result critical_command_repository_apply_from_pool(const critical_command &command,
								  void *context);

// Shared transaction-finalization helpers for command-specific repositories.
bool critical_command_repository_insert_outbox_event(MYSQL *connection,
						     const critical_operation_id &operation_id,
						     uint16_t event_index, uint16_t destination,
						     uint16_t event_type, uint16_t payload_version,
						     const uint8_t *payload, size_t payload_size);
bool critical_command_repository_finish_inbox(
	MYSQL *connection, const critical_command &command, uint64_t durable_revision,
	unsigned int result_code, const uint8_t *payload, size_t payload_size,
	critical_failure_stage failure_stage = critical_failure_stage::none);

#endif
