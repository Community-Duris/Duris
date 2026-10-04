#ifndef SHOP_TRADE_PUBLICATION_H
#define SHOP_TRADE_PUBLICATION_H

#include "economy/shop_trade_transaction.h"

// Shop-local live publication only. This does not change durable admission,
// publication ACK classification, or cold replay. A false return retains the
// original committed receipt; stages must describe idempotent effects, never
// retain native object/character pointers.
using shop_trade_physical_publication_fn = bool (*)(P_char character,
						    const shop_trade_result &result,
						    const shop_trade_payload &payload,
						    uint32_t &stages);

bool shop_trade_transaction_submit_with_publication(P_char character,
						    const shop_trade_payload &payload,
						    shop_trade_physical_publication_fn publication,
						    shop_trade_completion_fn completion);

#endif
