#ifndef SHOP_TRADE_TRANSACTION_H
#define SHOP_TRADE_TRANSACTION_H

#include "persistence/critical_command_coordinator.h"
#include "economy/shop_trade_command.h"
#include "core/structs.h"

#include <cstddef>
#include "economy/economic_gameplay_authority.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot.h"
#include "economy/shop_trade_flat_native_checkpoint.h"
#include <memory>
#ifndef __NO_MYSQL__
#include "persistence/shop_item_runtime_payload.h"
#endif

class shop_trade_native_checkpoint_owner;
struct flatfile_accounting_shop_projection;

// Local preparation cancellation only, after the exact player hold/slot has
// been released before admission. Literal bytes belong to the original selected
// tree; no fabricated receipt, save revision or SQL authority is supplied.
// False/throw retains the original owner and is never automatically repeated.
using shop_trade_preparation_refusal_fn = bool (*)(P_char, const shop_trade_payload &,
						   std::span<const uint8_t>, unsigned int);

constexpr size_t SHOP_TRADE_PENDING_MAX = 128;

using shop_trade_completion_fn = void (*)(P_char character, bool committed,
					  const shop_trade_result &result, unsigned int error_code,
					  const shop_trade_payload &payload);

// Accounted publication receives the freshly witnessed ORIGINAL keeper body.
// Never resolve a v6 trade by prototype or configured room; these pointers are
// transient and grant no SQL, reservation or ACK authority.
using shop_trade_accounted_publication_fn = bool (*)(P_char character, P_char keeper,
						     const shop_trade_result &result,
						     const shop_trade_payload &payload,
						     uint32_t &stages);

// Original startup journal observer. Stages the exact command/map node before
// registering the existing typed player hold; never enters the coordinator.
bool shop_trade_transaction_restore_replayed_command(const critical_command &) noexcept;
// Actor-independent, bounded original cold entries on the normal game pulse.
void shop_trade_transaction_restore_pulse() noexcept;

enum class shop_trade_cold_native_step : uint8_t
{
	detach,
	place_player,
	place_keeper,
	nest,
	destroy,
	retire_copy,
	reject_produced
};
// One normally returned native handler leg. A started/unreturned effect is
// retained forever until its actual owner resolves it, never inferred from UID
// placement. This carries no SQL, receipt, publication or ACK authority.
struct shop_trade_cold_native_effect
{
	shop_trade_cold_native_step step = shop_trade_cold_native_step::detach;
	bool started = false, returned = false, succeeded = false;
};

bool shop_trade_transaction_submit(P_char character, const shop_trade_payload &payload,
				   shop_trade_completion_fn completion);
void shop_trade_transaction_handle_completions(const critical_completion *completions,
					       size_t count);
void shop_trade_transaction_player_ready(P_char character);
bool shop_trade_transaction_player_busy(P_char character);
// Main game-thread ownership, from original selection through player checkpoint.
// An ID or an observed mapping is not SQL checkpoint/publication authority.
class shop_trade_preparation_token final
{
    public:
	shop_trade_preparation_token() = default;

    private:
	friend class shop_trade_preparation_owner;
	critical_operation_id operation_id_{};
	uint64_t generation_ = 0;
};

// Original regular-flat identity; never converts to the SQL preparation token.
class shop_trade_flat_preparation_token final
{
    public:
	shop_trade_flat_preparation_token() = default;

    private:
	friend class shop_trade_preparation_owner;
	critical_operation_id operation_id_{};
	uint64_t generation_ = 0;
};
enum class shop_trade_preparation_state : uint8_t
{
	refused,
	pending,
	ready
};

struct shop_trade_checkpoint_context
{
	critical_operation_id operation_id{};
	uint64_t generation = 0, actor_runtime_id = 0, keeper_runtime_id = 0;
	uint32_t player_pid = 0, shop_id = 0;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name{};
	uint8_t racewar = 0, keeper_roaming = 0;
	int64_t keeper_cash = 0;
	int32_t keeper_vnum = 0;
	economic_shop_checkpoint_projection mapping{};
	player_shop_checkpoint_stage player{};
	std::vector<player_item_snapshot> keeper_items;
};

// Original held save/source values only; no native write permission.
struct shop_trade_flat_checkpoint_context
{
	shop_trade_checkpoint_context original;
	player_flat_shop_checkpoint_token player_token;
	player_snapshot original_queued_ack;
	player_flat_shop_checkpoint_cut player_hold_cut;
};
#ifndef __NO_MYSQL__
// Retained original native facts only. Public construction grants no write or
// completion authority: only the native owner can seal/resolve these values.
struct shop_trade_native_checkpoint_stage
{
	uint64_t keeper_id = 0, bank_id = 0;
	uint64_t player_owner_revision = 0, keeper_owner_revision = 0;
	currency_vector wallet{}, bank{};
	uint64_t wallet_revision = 0, bank_revision = 0;
	uint64_t shop_revision_before = 0, shop_revision_after = 0;
	bool payload_checkpoint_recorded_before = false;
	uint64_t payload_checkpoint_revision_before = 0;
	shop_item_runtime_image keeper_image;
};
enum class shop_trade_native_checkpoint_disposition : uint8_t
{
	committed,
	rolled_back,
	never_mutated_retired,
	uncertain
};
#endif

class shop_trade_preparation_owner final
{
    public:
	// Read-only route availability before producer callbacks/RNG/allocation.
	// It grants no source, checkpoint, admission or publication authority.
	static bool production_available() noexcept;
	// True transfers this one original selection to the bounded pending map.
	// Neither notification is invoked synchronously by this function.
	static bool start(P_char actor, P_char keeper, P_obj selected, P_obj stock,
			  P_obj destination, uint32_t shop_id, shop_trade_action action,
			  int64_t price, shop_trade_accounted_publication_fn publication,
			  shop_trade_completion_fn completion,
			  shop_trade_preparation_refusal_fn refusal) noexcept;
	// One bounded attempt per original entry on the existing gameplay pulse.
	static void pulse() noexcept;
	static shop_trade_preparation_state begin(P_char actor, P_char keeper, P_obj selected,
						  P_obj stock, P_obj destination, uint32_t shop_id,
						  shop_trade_action action, int64_t price,
						  shop_trade_preparation_token *output) noexcept;
	static shop_trade_preparation_state poll(const shop_trade_preparation_token &, P_char actor,
						 P_char keeper, P_obj selected, P_obj stock,
						 P_obj destination) noexcept;
	// Freezes one exact v6 accounting command from the original proved native
	// checkpoint. No SQL revision/ACK can be supplied by an arbitrary caller.
	static bool build_accounted_command(const shop_trade_preparation_token &, P_char actor,
					    P_char keeper, P_obj selected, P_obj stock,
					    P_obj destination, critical_command *) noexcept;
	// Convert this original prepared checkpoint to journal/publication ownership.
	// An exact retry uses the retained command and callbacks; it cannot select a
	// new decision, clock, runtime checkpoint or operation identity.
	static critical_submit_result
	submit_accounted(const shop_trade_preparation_token &, P_char actor, P_char keeper,
			 P_obj selected, P_obj stock, P_obj destination,
			 shop_trade_accounted_publication_fn publication,
			 shop_trade_completion_fn completion) noexcept;
	// Only before native keeper attempt/admission; never an economic rollback.
	static bool cancel(const shop_trade_preparation_token &) noexcept;
	// Value copy only. The original native transaction must prove custody,
	// physical rows, account ownership, epoch, revisions and session exclusion.
	static bool checkpoint_context(const shop_trade_preparation_token &,
				       shop_trade_checkpoint_context *output) noexcept;
	// Explicit flat capture/readiness only. Production driver/admission stay closed.
	static shop_trade_preparation_state
	begin_flat(P_char actor, P_char keeper, P_obj selected, P_obj stock, P_obj destination,
		   uint32_t shop, shop_trade_action, int64_t price,
		   shop_trade_flat_preparation_token *) noexcept;
	static shop_trade_preparation_state poll_flat(const shop_trade_flat_preparation_token &,
						      P_char actor, P_char keeper, P_obj selected,
						      P_obj stock, P_obj destination) noexcept;
	static bool cancel_flat(const shop_trade_flat_preparation_token &) noexcept;
	// Freeze only this original proved READY flat source. Exact retries retain
	// its original acceptance time; no submission/publication route follows.
	static bool build_accounted_command_flat(const shop_trade_flat_preparation_token &,
						 P_char actor, P_char keeper, P_obj selected,
						 P_obj stock, P_obj destination,
						 critical_command *) noexcept;

	// Retain and submit this original READY flat command with its existing hold.
	// This does not enable admission, producer driving, publication or ACK.
	static critical_submit_result
	submit_accounted_flat(const shop_trade_flat_preparation_token &, P_char actor,
			      P_char keeper, P_obj selected, P_obj stock, P_obj destination,
			      shop_trade_accounted_publication_fn,
			      shop_trade_completion_fn) noexcept;

    private:
	// Retry borrows the same original stage without recapture or new write permission.
	static bool
	begin_native_checkpoint_flat(const shop_trade_flat_preparation_token &, P_char actor,
				     P_char keeper, P_obj selected, P_obj stock, P_obj destination,
				     shop_trade_flat_checkpoint_context *,
				     const shop_trade_flat_native_checkpoint_stage **,
				     shop_trade_flat_native_checkpoint_phase *) noexcept;
	static bool seal_native_checkpoint_flat(
		const shop_trade_flat_preparation_token &,
		std::unique_ptr<shop_trade_flat_native_checkpoint_stage> &&) noexcept;
	static bool
	start_native_checkpoint_flat(const shop_trade_flat_preparation_token &) noexcept;
	static bool retain_native_outcome_flat(const shop_trade_flat_preparation_token &,
					       flatfile_authority_transaction_result,
					       flatfile_authority_commit_outcome) noexcept;
	static bool resolve_native_checkpoint_flat(
		const shop_trade_flat_preparation_token &,
		const shop_trade_flat_native_checkpoint_resolution &) noexcept;
	static bool
	completed_native_checkpoint_flat(const shop_trade_flat_preparation_token &,
					 const shop_trade_flat_native_checkpoint_stage **) noexcept;

    private:
	static void drive(const shop_trade_preparation_token &) noexcept;
	static void notify_refusal(const shop_trade_preparation_token &) noexcept;
	friend class shop_trade_native_checkpoint_owner;
#ifndef __NO_MYSQL__
	// Original recapture and ownership precede SQL. All output copies complete
	// before installing a new attempt; failed retry copies retain the old attempt.
	static bool begin_native_checkpoint(const shop_trade_preparation_token &, P_char actor,
					    P_char keeper, P_obj selected, P_obj stock,
					    P_obj destination, shop_trade_checkpoint_context *,
					    shop_trade_native_checkpoint_stage *,
					    bool *retry) noexcept;
	// Only observes previously proved clean native completion; failed copies
	// preserve the original retained stage and never release its player hold.
	static bool completed_native_checkpoint(const shop_trade_preparation_token &,
						shop_trade_native_checkpoint_stage *) noexcept;
	// Must precede the first native mutation. Move retains the complete locked
	// after-image and actual revision pair in the original pending map entry.
	static bool seal_native_checkpoint(const shop_trade_preparation_token &,
					   shop_trade_native_checkpoint_stage &&) noexcept;
	// Native owner must independently prove clean commit/readback or rollback.
	// Uncertain cleanup cannot reopen cancellation or remove the original entry.
	static bool finish_native_checkpoint(const shop_trade_preparation_token &,
					     shop_trade_native_checkpoint_disposition) noexcept;
#endif
};

class shop_trade_native_checkpoint_owner final
{
    private:
	friend class shop_trade_native_publication_owner;
	// Physical read-only reuse. Only the actual live publisher supplies the
	// authenticated CURRENT projection under its original borrowed root lock.
	static bool observe_flat_publication_sources_locked(
		const std::string &, const flatfile_authority_lock &, P_char actor, P_char keeper,
		uint32_t shop, const flatfile_accounting_shop_projection &current,
		const std::vector<player_item_snapshot> &physical_pc,
		const std::vector<player_item_snapshot> &physical_keeper,
		int64_t runtime_keeper_cash) noexcept;

	// Supplement the mandatory original cold world/literal witness. Null bodies
	// require actual complete identity absence; UID-zero pet absence stays unresolved.
	// Cash is the caller-authenticated original BEFORE/CURRENT publication phase.
	static bool observe_flat_cold_publication_sources_locked(
		const std::string &, const flatfile_authority_lock &, P_char actor, P_char keeper,
		uint32_t shop, const flatfile_accounting_shop_projection &current,
		const std::vector<player_item_snapshot> &physical_pc,
		const std::vector<player_item_snapshot> &physical_keeper,
		int64_t authenticated_stage_keeper_cash) noexcept;

    public:
	static shop_trade_preparation_state attempt(const shop_trade_preparation_token &,
						    P_char actor, P_char keeper, P_obj selected,
						    P_obj stock, P_obj destination) noexcept;
	static shop_trade_preparation_state attempt_flat(const shop_trade_flat_preparation_token &,
							 P_char actor, P_char keeper,
							 P_obj selected, P_obj stock,
							 P_obj destination) noexcept;
};

bool shop_trade_transaction_keeper_busy(uint32_t shop_id);
bool shop_trade_transaction_item_busy(uint64_t uid);
void shop_trade_transaction_reset_for_tests(void);

#endif
