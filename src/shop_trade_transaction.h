#ifndef SHOP_TRADE_TRANSACTION_H
#define SHOP_TRADE_TRANSACTION_H

#include "critical_command_coordinator.h"
#include "shop_trade_command.h"
#include "structs.h"

#include <cstddef>

constexpr size_t SHOP_TRADE_PENDING_MAX = 128;

using shop_trade_completion_fn = void (*)(P_char character, bool committed,
					  const shop_trade_result &result, unsigned int error_code,
					  const shop_trade_payload &payload);

bool shop_trade_transaction_submit(P_char character, const shop_trade_payload &payload,
				   shop_trade_completion_fn completion);
void shop_trade_transaction_handle_completions(const critical_completion *completions,
					       size_t count);
void shop_trade_transaction_player_ready(P_char character);
bool shop_trade_transaction_player_busy(P_char character);
void shop_trade_transaction_reset_for_tests(void);

#endif
