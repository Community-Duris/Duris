#ifndef COIN_PHYSICAL_PUBLICATION_H
#define COIN_PHYSICAL_PUBLICATION_H

#include "economy/currency_transaction.h"

struct obj_data;
typedef struct obj_data *P_obj;

// Ordinary, single-root room piles only. This is an in-process publication
// adapter, not authority to hydrate a wallet or ACK a callback-free cold replay.
bool coin_physical_publication_room_safe(int room, P_obj money);
bool coin_physical_publication_publish(P_char actor, const critical_operation_id &operation_id,
				       const coin_transfer_payload &payload,
				       const coin_transfer_result &result, const uint8_t *context,
				       size_t context_size);
// Called by the currency owner only after its original durable publication ACK.
void coin_physical_publication_release(const critical_operation_id &operation_id) noexcept;

#endif
