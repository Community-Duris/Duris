#ifndef DURIS_ECONOMY_CURRENCY_TRANSACTION_H
#define DURIS_ECONOMY_CURRENCY_TRANSACTION_H

#include "persistence/critical_command_coordinator.h"
#include "economy/currency_command.h"
#include "economy/coin_transfer_command.h"
#include "core/structs.h"

#include <cstddef>
#include <cstdint>

constexpr size_t CURRENCY_PENDING_MAX = 1024;
constexpr size_t CURRENCY_PENDING_CONTEXT_MAX_BYTES = 64;

// A final notification: false means an explicit terminal rejection, never an
// ambiguous outcome or failure to publish an acknowledged commit. Unresolved
// receipts retain this same continuation and block non-rebasable admission.
using currency_completion_fn = void (*)(P_char character, bool committed,
					const currency_command_result &result,
					unsigned int error_code, const uint8_t *context,
					size_t context_size);

// The legacy composite callback returns false when committed live publication
// needs another pulse. When used as coin_publication_callbacks::notify, false
// reports a notification failure after ACK; it cannot retry economic effects.
using coin_completion_fn = bool (*)(P_char actor, bool committed,
				    const coin_transfer_payload &payload,
				    const coin_transfer_result &result, unsigned int error_code,
				    const uint8_t *context, size_t context_size);

// Schema-2 pile publication is separate from post-ACK notification. The publisher
// must verify/materialize every physical endpoint against this original operation
// and result, without notifying players, advancing bulk work, submitting commands
// or erasing pending owners. Resolve native objects anew on every attempt. False
// or an exception retains the original obligation; retry must be idempotent.
using coin_physical_publication_fn = bool (*)(P_char actor,
					      const critical_operation_id &operation_id,
					      const coin_transfer_payload &payload,
					      const coin_transfer_result &result,
					      const uint8_t *context, size_t context_size);

struct coin_publication_callbacks
{
	coin_physical_publication_fn publish = nullptr;
	coin_completion_fn notify = nullptr;
	// Optional producer-owned staging cleanup, called only after durable ACK and
	// pending-owner extraction. It must not publish effects or submit new work.
	void (*release)(const critical_operation_id &) noexcept = nullptr;
};

// A committed callback receives EOWNERDEAD on its final cleanup notification if
// bounded legacy live publication fails. Schema-2 obligations never use this
// cleanup path or retire merely because publication retries are exhausted.
constexpr unsigned int CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS = 8;

struct currency_transaction_health
{
	uint64_t pending;
	uint64_t retained_offline;
	uint64_t publication_blocked;
	uint64_t publication_retrying;
	uint64_t submitted;
	uint64_t committed;
	uint64_t rejected;
	uint64_t submission_failures;
	uint64_t malformed_completions;
	uint64_t publication_abandoned;
};

// Guard for commands built from the character's current wallet or bank view:
// valid identity/capacity, no coordinator fence, and no unpublished predecessor.
bool currency_transaction_can_submit_nonrebasable(P_char character);
bool currency_transaction_player_busy(P_char character);
bool currency_transaction_coin_item_busy(uint64_t item_uid);
bool currency_transaction_coin_wallet(P_char character, int64_t value_delta,
				      coin_transfer_endpoint *endpoint);
bool currency_transaction_coin_wallet_exact(P_char character, uint8_t denomination, int32_t amount,
					    bool debit, coin_transfer_endpoint *endpoint);
// Schema-1 uses the original composite completion. Schema-2 item endpoints need
// an explicit verified publisher before admission; its notification runs once
// after durable ACK and owner release. Wallet-only schema-2 uses completion.
bool currency_transaction_submit_coin(P_char actor, const coin_transfer_payload &payload,
				      coin_completion_fn completion, const void *context,
				      size_t context_size,
				      coin_publication_callbacks publication = {});
bool currency_transaction_publish_wallet(P_char character, const currency_vector &wallet,
					 uint64_t wallet_revision);
bool currency_transaction_publish_balances(P_char character, const char *account_name,
					   uint8_t racewar, const currency_vector &wallet,
					   const currency_vector &bank, uint64_t wallet_revision,
					   uint64_t bank_revision);
bool currency_transaction_submit(P_char character, const currency_vector &wallet_delta,
				 const currency_vector &bank_delta, currency_reason_type reason,
				 int64_t reason_id, critical_source_site source_site,
				 critical_deadline_class deadline_class,
				 currency_completion_fn completion, const void *context,
				 size_t context_size);
bool currency_transaction_submit_identified(
	P_char character, const critical_operation_id &operation_id,
	const currency_vector &wallet_delta, const currency_vector &bank_delta,
	currency_reason_type reason, int64_t reason_id, critical_source_site source_site,
	critical_deadline_class deadline_class, currency_completion_fn completion,
	const void *context, size_t context_size);
bool currency_transaction_submit_wallet_value_identified(
	P_char character, const critical_operation_id &operation_id, int64_t value_delta,
	currency_reason_type reason, int64_t reason_id, critical_source_site source_site,
	critical_deadline_class deadline_class, currency_completion_fn completion,
	const void *context, size_t context_size);
// Prepare an immutable locker payment without submitting it. A durable receipt
// must store this exact command before submit_prepared is called. Admission is
// checked before that durable boundary; unsupported active-mode payments refuse.
bool currency_transaction_prepare_identify(P_char character, int64_t cost,
					   critical_command *command);
// New prepared submissions in active mode must already contain typed intent.
// Producer-written timestamps never authorize a schema-1 admission bypass.
bool currency_transaction_submit_prepared(P_char character, const critical_command &command,
					  currency_completion_fn completion, const void *context,
					  size_t context_size);
bool currency_transaction_submit_wallet_value(P_char character, int64_t value_delta,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size);
bool currency_transaction_submit_bank_reward(P_char character, int64_t value,
					     currency_reason_type reason, int64_t reason_id,
					     critical_source_site source_site,
					     critical_deadline_class deadline_class,
					     currency_completion_fn completion, const void *context,
					     size_t context_size);
bool currency_transaction_submit_bank_payment(P_char character, int64_t value,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size);
void currency_transaction_handle_completions(const critical_completion *completions, size_t count);
// Called only from the coordinator's validated journal-replay observer under
// its mutex. Must not call coordinator APIs or issue a replacement operation.
bool currency_transaction_restore_replayed_command(const critical_command &command);
// Primary-owned native room-coin recovery handoff, invoked only for a restored
// schema-2 ordinary single-root drop/pickup. It must verify the original retained
// receipt and current wallet/bank/pile authority, publish idempotently under the
// original save reservation, then perform guarded ACK and consume that hold.
// True means that entire original obligation has completed; false or an exception
// retains it. No implementation or native proof is supplied by this domain owner.
bool coin_physical_publication_restore_and_acknowledge(
	const critical_command &original_command, const critical_completion &sealed_completion);
// Allocation-free synchronous reentry check for that owner immediately before
// its guarded ACK. This observation is NOT native proof or an ACK capability.
bool currency_transaction_restored_coin_receipt_current(
	const critical_command &original_command,
	const critical_completion &sealed_completion) noexcept;
void currency_transaction_player_ready(P_char character);
currency_transaction_health currency_transaction_health_copy(void);
void currency_transaction_reset_for_tests(void);

// Explicit original bank ATM passive replay owner only, never selected here.
// Schema/publication skips remain original; a publication-required non-bank
// refuses this capability. Caller outer excludes CURRENT currency owner once,
// and includes authentic coordinator/journal/input/output/dispatcher frames.
// Reserve must not reenter locked coordinator/journal or mutate this table.
bool currency_transaction_restore_bank_replayed_command_bounded(
	const critical_command &, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept;
// Passive complete CURRENT table/health/buckets/nodes/keys and coin-command heaps.
// Value observation only; strong output. Genuine caller refreshes every return.
bool currency_transaction_current_storage_bytes(size_t *output) noexcept;

// Complete original mixed schema/publication/bank ATM/coin passive replay.
// Genuine startup coordinator/main-thread caller owns pending stability. Outer
// excludes CURRENT currency owner, complete literal replay pool and private
// stack scope; includes actual coordinator/journal/input/caller and every
// OTHER pipeline, worker and execution-guard owner with fresh genuine census.
// Real private pipeline scope lives across first allocating proof, wallet-only
// too, and adds its same-lock CURRENT pool once to each absolute request.
// Reserve must not acquire pipeline/coordinator/journal or mutate replay owners.
// No native publication, ACK, alternate operation or selected route is granted.
bool currency_transaction_restore_replayed_command_bounded(const critical_command &,
							   bool (*)(size_t, void *) noexcept,
							   void *, size_t outer_live) noexcept;

#endif
