#ifndef DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H

#include "persistence/critical_command_completion.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot.h"
#include <string>

// Borrowed-lock cold value only: current native literal/custody and authenticated
// existing ordinary COIN proof. Not an original envelope, admission token,
// publication/save reservation or ACK. Flat evidence keeps zero child receipts;
// pile_endpoint_operation is the original embedded endpoint ID, not a new child.
struct flatfile_room_coin_pile
{
	critical_operation_id lineage = {}, epoch = {}, root_operation = {},
			      pile_endpoint_operation = {};
	uint64_t lineage_revision = 0, retained_root_revision = 0;
	item_ownership_runtime_entry identity = {};
	player_item_snapshot item;
	item_transfer_result retained_pile_result = {};
};

class flatfile_authority_lock;

// Typed coin root. Native wallets, pile ownership, UID-keyed pile state,
// item references, and accounting evidence share one authority transaction.
class flatfile_accounting_coin_transaction
{
    public:
	// Caller owns original authority lock; recover existing journal, authenticate
	// retained indexed root/intent/plan/native receipt/item references and exact
	// current active-epoch head/custody/full literal. No wallet-current adoption,
	// mutation, application, activation, boot publication, or legacy fallback.
	// errno-style refusal; output remains unchanged on every failure.
	static unsigned int read_room_pile_locked(const std::string &root,
						  const flatfile_authority_lock &lock, uint64_t uid,
						  flatfile_room_coin_pile *output,
						  std::string *error) noexcept;
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
