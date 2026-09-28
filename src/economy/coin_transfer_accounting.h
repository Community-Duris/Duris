#ifndef DURIS_COIN_TRANSFER_ACCOUNTING_H
#define DURIS_COIN_TRANSFER_ACCOUNTING_H

#include "economy/coin_transfer_command.h"
#include <mysql/mysql.h>

// Record double-entry economic accounting evidence for a completed coin_transfer composite:
// - Inserts economic_accounting_operation root
// - Inserts 2 child operations (wallet and pile) in economic_accounting_child
// - Inserts 2 account effects (wallet and pile) in economic_accounting_account_effect
// - Inserts 2 balanced postings (wallet debit, pile credit) in economic_accounting_coin_posting
// Net copper delta = 0.
bool coin_transfer_accounting_record(MYSQL *connection, const critical_command &root_command,
				     const coin_transfer_payload &payload,
				     const coin_transfer_result &result);

#endif
