#ifndef ZONE_TOUCH_TRANSACTION_H
#define ZONE_TOUCH_TRANSACTION_H

#include "critical_command_coordinator.h"
#include "critical_outbox.h"
#include "zone_touch_command.h"

constexpr size_t ZONE_TOUCH_PENDING_MAX = 64;

bool zone_touch_transaction_submit(const zone_touch_payload &payload);
void zone_touch_transaction_handle_completions(const critical_completion *completions,
					       size_t count);
critical_outbox_delivery_result
zone_touch_transaction_outbox_delivery(const critical_outbox_record &record, void *context);
void zone_touch_transaction_reset_for_tests(void);

#endif
