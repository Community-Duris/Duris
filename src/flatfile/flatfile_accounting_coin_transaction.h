#ifndef DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H

#include "persistence/critical_command_completion.h"
#include <string>

class flatfile_authority_lock;

// Typed coin root. Native wallets, pile ownership, UID-keyed pile state,
// item references, and accounting evidence share one authority transaction.
class flatfile_accounting_coin_transaction
{
    public:
	static critical_apply_result apply(const std::string &root,
					   const critical_command &command);
	// Borrow the caller's original authority lock and verify the exact retained
	// canonical command/result, historical root/plan and pile/item references.
	// Existing lookup may finish recovery of a previously published authority
	// bundle. Never applies, stages, commits a new bundle, reacquires the lock,
	// proves current native state, projects a pile, or grants publication ACK.
	// Only a verified retained completion can return terminal_failure; proof
	// refusals are retryable (EAGAIN missing, EEXIST conflicting, helper errors).
	static critical_apply_result
	verify_retained_locked(const std::string &root, const flatfile_authority_lock &lock,
			       const critical_command &original) noexcept;
};
#endif
