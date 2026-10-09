#ifndef DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_COIN_TRANSACTION_H

#include "persistence/critical_command_completion.h"
#include "item/item_ownership_runtime.h"
#include "player/player_snapshot.h"
#include <string>

// Borrowed-lock cold value only: current native literal/custody and authenticated
// original selected baseline or ordinary COIN proof. Not an original envelope, admission token,
// publication/save reservation or ACK. Flat evidence keeps zero child receipts;
// pile_endpoint_operation is the original embedded COIN endpoint ID, not a new child.
// Baseline values retain zero endpoint/result fields; no COIN is reconstructed.
struct flatfile_room_coin_pile
{
	critical_operation_id lineage = {}, epoch = {}, root_operation = {},
			      pile_endpoint_operation = {};
	uint64_t lineage_revision = 0, retained_root_revision = 0;
	item_ownership_runtime_entry identity = {};
	player_item_snapshot item;
	item_transfer_result retained_pile_result = {};
};

// Values for the original whole-world detached boot owner. Successful typed
// history fences old aggregate room money even after full consumption. Every
// active pile is independently authenticated by read_room_pile_locked. No
// command, native write, enrollment capability or ACK is reconstructed here.
struct flatfile_room_coin_boot_view
{
	std::vector<uint64_t> history_uids;
	std::vector<int32_t> fenced_rooms;
	std::vector<flatfile_room_coin_pile> piles;
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
	// Complete catalog-required retained history plus EPH1 census under the
	// caller's original cut. Rejected attempts grant no pile/head authority.
	// Missing/corrupt successful proof refuses before any physical staging;
	// outputs remain unchanged. maximum is the existing physical census bound.
	static unsigned int read_room_boot_locked(const std::string &root,
		const flatfile_authority_lock &lock, size_t maximum,
		flatfile_room_coin_boot_view *output, std::string *error) noexcept;
	// Original cold boot only: select the actual current head's retained proof.
	// Baseline and typed COIN use their own native/witness contracts; values
	// cannot authorize admission, mutation, activation or publication ACK.
	static unsigned int read_room_boot_pile_locked(const std::string &root,
		const flatfile_authority_lock &lock, uint64_t uid,
		flatfile_room_coin_pile *output, std::string *error) noexcept;
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
