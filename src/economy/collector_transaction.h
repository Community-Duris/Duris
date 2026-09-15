#ifndef DURIS_COLLECTOR_TRANSACTION_H
#define DURIS_COLLECTOR_TRANSACTION_H

#include "economy/collector_command.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "core/structs.h"

#include <cstddef>

constexpr size_t COLLECTOR_PENDING_MAX = 128;

// committed reports the durable command outcome. A nonzero error on a committed
// callback means local publication needs recovery; it never turns a committed
// custody or wallet mutation into a rejected operation.
using collector_completion_fn = void (*)(P_char character, bool committed,
					 const collector_command_result &result,
					 unsigned int error_code,
					 const collector_command_payload &payload);

bool collector_transaction_submit(
	P_char character, const collector_command_payload &payload,
	collector_completion_fn completion,
	critical_deadline_class deadline = critical_deadline_class::interactive);
bool collector_transaction_submit_identified(
	P_char character, const critical_operation_id &operation_id,
	const collector_command_payload &payload, collector_completion_fn completion,
	critical_deadline_class deadline = critical_deadline_class::interactive);
bool collector_transaction_submit_background(const collector_command_payload &payload,
					     collector_completion_fn completion);
bool collector_transaction_submit_background_identified(const critical_operation_id &operation_id,
							const collector_command_payload &payload,
							collector_completion_fn completion);
void collector_transaction_handle_completions(const critical_completion *completions, size_t count);
void collector_transaction_player_ready(P_char character);
bool collector_transaction_player_busy(P_char character);
// A listing may have at most one command between durable submission and
// game-thread publication. This also protects due work from ephemeral lease
// loss when a periodic authoritative cache snapshot rebuilds the runtime.
bool collector_transaction_listing_busy(uint64_t listing);
// Holds the live object graph stable from collection submission through game-thread
// publication, including the interval after the coordinator releases its worker fence.
bool collector_transaction_item_busy(uint64_t item_uid);
critical_outbox_delivery_result
collector_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void collector_transaction_publish_outbox(void);
void collector_transaction_reset_for_tests(void);

#endif
