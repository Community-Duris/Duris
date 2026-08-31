#ifndef ITEM_TRANSFER_SYNTHETIC_H
#define ITEM_TRANSFER_SYNTHETIC_H

#include "persistence/critical_command_coordinator.h"
#include "item_transfer_command.h"

/* Session 05 proof adapter. Live object pointers and movement routes are intentionally
 * excluded; Session 06 owns their cutover. */
bool item_transfer_synthetic_submit(const item_transfer_payload &payload,
				    critical_operation_id *operation_id_out);
bool item_transfer_synthetic_decode_completion(const critical_completion &completion,
					       item_transfer_result *result, bool *committed);

#endif
