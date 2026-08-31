#ifndef BOON_SHOP_TRANSACTION_H
#define BOON_SHOP_TRANSACTION_H

#include "boon_shop_command.h"
#include "persistence/critical_command_coordinator.h"
#include "structs.h"

bool boon_shop_transaction_submit(P_char character, uint8_t stat_index);
void boon_shop_transaction_handle_completions(const critical_completion *completions, size_t count);
void boon_shop_transaction_reset_for_tests(void);

#endif
