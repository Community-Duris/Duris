#ifndef DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H

#include "persistence/critical_command_completion.h"
#include <string>

// Typed coin root. Native wallets, pile ownership, UID-keyed pile state,
// item references, and accounting evidence share one authority transaction.
class flatfile_accounting_coin_transaction
{
    public:
	static critical_apply_result apply(const std::string &root,
					   const critical_command &command);
};
#endif
