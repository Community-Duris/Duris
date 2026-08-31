#ifndef AUCTION_TRANSACTION_H
#define AUCTION_TRANSACTION_H

#include "auction_command.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t AUCTION_PENDING_MAX = 128;

using auction_completion_fn = void (*)(P_char character, bool committed,
				       const auction_command_result &result,
				       unsigned int error_code,
				       const auction_command_payload &payload);

bool auction_transaction_submit(
	P_char character, const auction_command_payload &payload, auction_completion_fn completion,
	critical_deadline_class deadline = critical_deadline_class::interactive);
bool auction_transaction_submit_background(const auction_command_payload &payload,
					   auction_completion_fn completion);
void auction_transaction_handle_completions(const critical_completion *completions, size_t count);
void auction_transaction_player_ready(P_char character);
bool auction_transaction_player_busy(P_char character);
critical_outbox_delivery_result
auction_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void auction_transaction_publish_outbox(void);
void auction_transaction_reset_for_tests(void);

#endif
