#ifndef PLAYER_SAVE_PIPELINE_H
#define PLAYER_SAVE_PIPELINE_H

#include "player/player_revision_state.h"
#include "persistence/critical_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_coordinator.h"
#include "player/player_save_replay_ownership.h"

#include "item/item_transfer_command.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <string>
#include <vector>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;
struct player_quest_xp_receipt_snapshot;
struct player_spell_effect_receipt_snapshot;
struct player_item_snapshot;
struct player_snapshot;
class collector_purchase_publication_owner;

constexpr size_t PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS = 256;
constexpr size_t PLAYER_SAVE_PIPELINE_MAX_BYTES = 32 * 1024 * 1024;
constexpr size_t PLAYER_SAVE_PIPELINE_PULSE_BUDGET = 32;

enum class player_save_pipeline_result : uint8_t
{
	queued,
	coalesced,
	unchanged,
	invalid,
	capture_failed,
	overloaded,
	unavailable,
};

enum class player_save_terminal_result : uint8_t
{
	database_acknowledged,
	journal_durable,
	invalid,
	unavailable,
	timed_out,
	not_pending,
};

struct player_save_pipeline_health
{
	uint64_t pending_append;
	uint64_t durable_ready;
	uint64_t retained_bytes;
	uint64_t high_water_snapshots;
	uint64_t high_water_bytes;
	uint64_t marked;
	uint64_t captured;
	uint64_t coalesced;
	uint64_t unchanged;
	uint64_t capture_failures;
	uint64_t append_failures;
	uint64_t overloads;
	uint64_t dispatched;
	uint64_t durable_spills;
	uint64_t completions;
	uint64_t terminal_fences;
	uint64_t terminal_death_requeues;
	uint64_t terminal_database_acks;
	uint64_t terminal_journal_handoffs;
	uint64_t terminal_timeouts;
	uint64_t drain_failures;
	bool initialized;
	bool accepting;
	bool append_inflight;
	bool dispatcher_running;
	bool replay_complete;
	bool replay_blocked;
};

// Resident coordinator metadata only; journal/worker/revision observations are separate.
struct player_save_pipeline_diagnostic
{
	player_save_pipeline_health health = {};
	bool available = false, pid_admission_open = false, retained_save = false;
};
player_save_pipeline_diagnostic player_save_pipeline_diagnostic_copy(int pid);

// Publishes startup replay readiness to normal player-load callers. False
// covers not-started, in-progress, failed, and stopped pipeline states.
class player_save_pipeline_replay_gate
{
    public:
	void begin_replay() noexcept { replay_complete_.store(false, std::memory_order_release); }
	void finish_replay(bool succeeded) noexcept
	{
		replay_complete_.store(succeeded, std::memory_order_release);
	}
	bool loads_allowed() const noexcept
	{
		return replay_complete_.load(std::memory_order_acquire);
	}

    private:
	std::atomic<bool> replay_complete_{ false };
};

// Game-thread lifecycle API. Preparation establishes journal/recovery metadata
// with admission and loads closed, without starting persistence threads. The
// caller must complete restored-command ownership before opting into start.
// Preparation refuses pre-existing execution owners or an incomplete shutdown;
// a failed drain requires an explicit successful shutdown retry.
// Start preserves prepared holds; shutdown also accepts an unstarted pipeline.
bool player_save_pipeline_prepare(const char *journal_directory,
				  void (*verify_resolved_recovery)() = nullptr);
bool player_save_pipeline_start(void);
// Existing callers retain immediate preparation/start behavior.
bool player_save_pipeline_init(const char *journal_directory,
			       void (*verify_resolved_recovery)() = nullptr);
void player_save_pipeline_shutdown(void);
bool player_save_pipeline_mark(int pid, player_component_mask_t components);
player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum);
player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum);
// An opt-in SQL ordinary-drop checkpoint retains literal capture policy through
// newer saves and coalescing. Database acknowledgment is separate from the
// physical source proof required by the eventual transfer transaction.
struct player_literal_inventory_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0;
	uint64_t root_uid = 0;
	uint64_t generation = 0;
	bool operator==(const player_literal_inventory_token &) const = default;
};

enum class player_literal_inventory_state : uint8_t
{
	pending,
	database_acknowledged,
	refused,
};

player_literal_inventory_state
player_save_pipeline_literal_inventory_begin(P_char actor, P_obj root, int room_vnum,
					     player_literal_inventory_token *token_out);
player_literal_inventory_state
player_save_pipeline_literal_inventory_poll(const player_literal_inventory_token &token,
					    P_char actor);
// Hold blocks inventory capture, while dirty marks continue advancing normally.
// Release requires the bound original operation ID; pre-admission cancel cannot
// release an operation's publication obligation.
bool player_save_pipeline_literal_inventory_hold(const player_literal_inventory_token &token,
						 const critical_operation_id &operation_id);
bool player_save_pipeline_literal_inventory_release(const player_literal_inventory_token &token,
						    const critical_operation_id &operation_id);
bool player_save_pipeline_literal_inventory_cancel(const player_literal_inventory_token &token);
// A distinct typed profile: root zero selects no literal tree, including empty
// inventory. All resulting persisted EQ/INV items and captured STATUS level are
// frozen. This is save readiness, not keeper/source or economic-write authority.
struct player_shop_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0;
	uint64_t root_uid = 0;
	uint64_t generation = 0;
	bool operator==(const player_shop_checkpoint_token &) const = default;
};

struct player_shop_checkpoint_stage
{
	player_revision_t save_revision = 0;
	uint32_t level = 0;
};

player_literal_inventory_state
player_save_pipeline_shop_checkpoint_begin(P_char actor, P_obj optional_carried_root, int room_vnum,
					   player_shop_checkpoint_token *token_out);
// Output stays unchanged until exact STATUS/EQ/INV acknowledgment and fresh
// runtime-body, whole-item-list and level equality. Other status may progress.
player_literal_inventory_state
player_save_pipeline_shop_checkpoint_poll(const player_shop_checkpoint_token &token, P_char actor,
					  player_shop_checkpoint_stage *stage_out = nullptr);
bool player_save_pipeline_shop_checkpoint_hold(const player_shop_checkpoint_token &token,
					       const critical_operation_id &operation_id);
bool player_save_pipeline_shop_checkpoint_release(const player_shop_checkpoint_token &token,
						  const critical_operation_id &operation_id);
bool player_save_pipeline_shop_checkpoint_cancel(const player_shop_checkpoint_token &token);

// Separate SQL and regular-flat Smith readiness profiles. Root zero selects no PC input tree;
// the original checkpoint retains the acknowledged filtered EQ/INV and STATUS
// level. Complete physical PC images, runtime grant grouping and wallet/native
// authority are independently frozen by the original Smith owner.
struct player_smith_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0, root_uid = 0, generation = 0;
	bool operator==(const player_smith_checkpoint_token &) const = default;
};
struct player_smith_checkpoint_stage
{
	player_revision_t save_revision = 0;
	uint32_t level = 0;
};
class smith_native_compound_owner;
struct economic_native_money_checkpoint_projection;
struct player_smith_flat_checkpoint_cut
{
	std::string selected_root;
	uint64_t ownership_epoch = 0, execution_hold_generation = 0;
};
class player_save_smith_checkpoint_owner final
{
    private:
	friend class smith_native_compound_owner;
	static player_literal_inventory_state begin(P_char, uint64_t original_runtime,
						    int room_vnum, player_smith_checkpoint_token *);
	static player_literal_inventory_state poll(const player_smith_checkpoint_token &, P_char,
						   player_smith_checkpoint_stage * = nullptr);
	static bool hold(const player_smith_checkpoint_token &, const critical_operation_id &);
	// Only an unheld pre-admission checkpoint can be cancelled here. Held
	// publication/recovery release remains closed until its full owner exists.
	static bool cancel(const player_smith_checkpoint_token &);
	// Optional original body is the exact queued/coalesced snapshot whose real
	// worker ACK supplied the recorded revision. Preserves its captured component
	// mask and receipts; never recreates uncaptured components or a full physical
	// PC forest. No submission, release, source or publication capability follows.
	static bool observe_held(
		const player_smith_checkpoint_token &, P_char, const critical_operation_id &,
		player_smith_checkpoint_stage *, std::vector<player_item_snapshot> * = nullptr,
		player_snapshot *original_acknowledged_body = nullptr,
		player_smith_flat_checkpoint_cut *flat_cut_out = nullptr,
		economic_native_money_checkpoint_projection *flat_wallet_out = nullptr) noexcept;
};

// A separate regular-flat profile; SQL tokens cannot select this slot.
struct player_flat_shop_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0, root_uid = 0, generation = 0;
	bool operator==(const player_flat_shop_checkpoint_token &) const = default;
};
struct economic_shop_checkpoint_projection;
struct player_snapshot;
struct player_flat_shop_checkpoint_cut
{
	std::string selected_root;
	uint64_t ownership_epoch = 0, execution_hold_generation = 0;
};
player_literal_inventory_state
player_save_pipeline_flat_shop_checkpoint_begin(P_char, P_obj optional_carried_root, int room_vnum,
						player_flat_shop_checkpoint_token *);
player_literal_inventory_state
player_save_pipeline_flat_shop_checkpoint_poll(const player_flat_shop_checkpoint_token &, P_char,
					       player_shop_checkpoint_stage * = nullptr);
bool player_save_pipeline_flat_shop_checkpoint_cancel(const player_flat_shop_checkpoint_token &);

class shop_trade_preparation_owner;
class shop_trade_native_checkpoint_owner;
class shop_trade_native_publication_owner;
class player_save_restored_publication_owner;
class player_save_shop_checkpoint_owner final
{
    private:
	friend class shop_trade_preparation_owner;
	friend class shop_trade_native_publication_owner;
	// Observation of the original held save only. No native keeper mutation,
	// economic admission or publication capability follows from this value.
	static bool observe_held(const player_shop_checkpoint_token &, P_char actor,
				 const critical_operation_id &, player_shop_checkpoint_stage *,
				 std::vector<player_item_snapshot> *items_out = nullptr) noexcept;
	// Retain the original whole-player body and immutable command in one slot.
	// The preparation owner must first prove its original native checkpoint.
	static critical_submit_result submit_owned(const player_shop_checkpoint_token &,
						   critical_command,
						   bool *checkpoint_released) noexcept;
	// Values only, copied from the exact original live slot. Neither matching
	// checkpoint bytes nor this image can authorize native publication or ACK.
	// Restored replay slots require their own durable checkpoint recovery route.
	static bool original_held_body(const player_shop_checkpoint_token &,
				       const critical_command &,
				       std::vector<player_item_snapshot> *,
				       player_shop_checkpoint_stage *) noexcept;
	friend class shop_trade_native_checkpoint_owner;
	// The leaf precedes native preparation. No slot flag replaces exclusion.
	static bool hold_flat(const player_flat_shop_checkpoint_token &,
			      const critical_operation_id &) noexcept;
	// One complete original native source stage is charged to the unchanged
	// shared literal budget before retention/attempt. A size is not source proof;
	// the native/preparation owner calculates every retained body and allocation.
	static bool reserve_flat_native_checkpoint(const player_flat_shop_checkpoint_token &,
						   const critical_operation_id &, size_t) noexcept;
	// Separate immutable command allocation after the original native attempt.
	// The genuine preparation owner proves READY; bytes grant no readiness.
	static bool reserve_flat_command_checkpoint(const player_flat_shop_checkpoint_token &,
						    const critical_operation_id &,
						    const player_flat_shop_checkpoint_cut &,
						    size_t) noexcept;

	// Charge the once-retained decoded recovery manifest independently.
	static bool reserve_flat_payload_checkpoint(const player_flat_shop_checkpoint_token &,
						    const critical_operation_id &,
						    const player_flat_shop_checkpoint_cut &,
						    size_t) noexcept;
	// Exact frozen command retention under the original native-attempt hold.
	// No new hold or synchronous-refusal release of an attempted source.
	// Charge the original live publisher's actual retained allocations once.
	// Borrow the authentic outer publication owner; no nested ticket or root.
	static bool reserve_flat_publication_checkpoint(
		const player_flat_shop_checkpoint_token &, const critical_operation_id &,
		const player_flat_shop_checkpoint_cut &,
		const player_save_restored_publication_owner &, size_t) noexcept;
	static critical_submit_result submit_owned_flat(const player_flat_shop_checkpoint_token &,
							const player_flat_shop_checkpoint_cut &,
							critical_command) noexcept;
	// Irreversible owner handoff BEFORE the first native attempt. Repeated
	// calls refuse; this is not evidence of a commit, rollback or safe retry.
	static bool begin_native_attempt_flat(const player_flat_shop_checkpoint_token &,
					      const critical_operation_id &) noexcept;
	// Only the never-handed-off original slot may release. Native attempts,
	// uncertainty and publication need a separate genuine outcome owner.
	static bool release_unattempted_flat(const player_flat_shop_checkpoint_token &,
					     const critical_operation_id &) noexcept;
	// Values only: actual enqueued/ACKed STATUS/EQ/INV and pinned source cut.
	// Fresh literal equality and the exact original leaf are independently checked.
	static bool observe_held_flat(const player_flat_shop_checkpoint_token &, P_char,
				      const critical_operation_id &, player_snapshot *,
				      player_shop_checkpoint_stage *,
				      economic_shop_checkpoint_projection *,
				      player_flat_shop_checkpoint_cut *) noexcept;

	// Values from the original ACK/body after physical publication may have
	// changed runtime items. No recapture, new root lock or authority follows.
	static bool original_held_body_flat(const player_flat_shop_checkpoint_token &,
					    const player_flat_shop_checkpoint_cut &,
					    const player_save_restored_publication_owner &,
					    player_snapshot *,
					    player_shop_checkpoint_stage *) noexcept;
};

// Auction readiness owns the original whole persistent player forest. It grants
// neither SQL mutation nor publication or ACK authority. Root UID stays zero.
struct player_auction_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0, root_uid = 0, generation = 0;
	bool operator==(const player_auction_checkpoint_token &) const = default;
};
struct player_auction_checkpoint_stage
{
	player_revision_t save_revision = 0;
	uint32_t level = 0;
};
player_literal_inventory_state
player_save_pipeline_auction_checkpoint_begin(P_char, int room_vnum,
					      std::span<const uint64_t> selected_roots,
					      player_auction_checkpoint_token *);
player_literal_inventory_state
player_save_pipeline_auction_checkpoint_poll(const player_auction_checkpoint_token &, P_char,
					     player_auction_checkpoint_stage * = nullptr);
bool player_save_pipeline_auction_checkpoint_hold(const player_auction_checkpoint_token &,
						  const critical_operation_id &);
bool player_save_pipeline_auction_checkpoint_release(const player_auction_checkpoint_token &,
						     const critical_operation_id &);
bool player_save_pipeline_auction_checkpoint_cancel(const player_auction_checkpoint_token &);
class auction_preparation_owner;
class auction_native_publication_owner;
class player_save_auction_checkpoint_owner final
{
    public:
	static bool original_held_bodies(const critical_command &,
					 std::vector<player_item_snapshot> *,
					 std::vector<player_item_snapshot> *,
					 player_auction_checkpoint_stage *) noexcept;

    private:
	friend class auction_preparation_owner;
	friend class auction_native_publication_owner;
	static bool observe_held(const player_auction_checkpoint_token &, P_char,
				 const critical_operation_id &, player_auction_checkpoint_stage *,
				 std::vector<player_item_snapshot> * = nullptr) noexcept;
	static critical_submit_result submit_owned(const player_auction_checkpoint_token &,
						   critical_command,
						   std::span<const uint8_t> original_native_before,
						   bool *checkpoint_released) noexcept;
};

class player_save_auction_publication_owner final
{
    private:
	friend class auction_native_publication_owner;
	// Passive original phase1 replay only; no actor/publication/ACK authority.
	static bool restore_recovery_checkpoint(const critical_native_recovery_envelope &) noexcept;
	// Bind only an exact original restored hold to an actual loaded player.
	// The native owner must first prove current SQL/world custody and confirmed
	// rollback. This creates local identity only, never publication authority.
	static bool rebind_recovery_checkpoint(const critical_native_recovery_envelope &,
					       uint64_t actual_player_runtime_id) noexcept;
	static bool copy_recovery_context(const critical_command &,
					  critical_native_recovery_envelope *) noexcept;
	static bool
	checkpoint_recovery_context(const critical_native_recovery_envelope &expected,
				    const critical_native_recovery_envelope &successor) noexcept;
	// Exact acknowledged whole-player forests and original level/revision.
	// No cold/source/ACK authority.
	static bool publication_held_bodies(const critical_command &,
					    std::vector<player_item_snapshot> *,
					    std::vector<player_item_snapshot> *,
					    player_auction_checkpoint_stage *) noexcept;

	// The real native owner must authenticate historical receipt, current SQL
	// cut and physical BEFORE/AFTER before this private callback succeeds.
	// Never-admitted remains closed until exact absence/BEFORE proof exists.
	static bool publish_auction(const critical_command &, const critical_completion &,
				    bool (*)(const critical_command &, const critical_completion &,
					     void *) noexcept,
				    void *) noexcept;
};

// Native quest readiness is distinct from SHOP authorization. The actual saved
// EQ/INV body is acknowledged before the original operation can hold it.
struct player_native_quest_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0, root_uid = 0, generation = 0;
	// Explicit zero-root money checkpoint; ordinary native item tokens stay false.
	bool money_only = false;
	bool operator==(const player_native_quest_checkpoint_token &) const = default;
};
struct player_native_quest_checkpoint_stage
{
	player_revision_t save_revision = 0;
};
player_literal_inventory_state player_save_pipeline_native_quest_checkpoint_begin(
	P_char, P_obj optional_carried_root, int room_vnum, player_native_quest_checkpoint_token *);
// Saves actual STATUS and the full unchanged EQ/INV forest before money admission.
player_literal_inventory_state
player_save_pipeline_native_money_checkpoint_begin(P_char, int room_vnum,
						   player_native_quest_checkpoint_token *);
player_literal_inventory_state
player_save_pipeline_native_quest_checkpoint_poll(const player_native_quest_checkpoint_token &,
						  P_char,
						  player_native_quest_checkpoint_stage * = nullptr);
bool player_save_pipeline_native_quest_checkpoint_hold(const player_native_quest_checkpoint_token &,
						       const critical_operation_id &);
bool player_save_pipeline_native_quest_checkpoint_release(
	const player_native_quest_checkpoint_token &, const critical_operation_id &);
bool player_save_pipeline_native_quest_checkpoint_cancel(
	const player_native_quest_checkpoint_token &);
class item_native_quest_preparation_owner;
class player_save_native_quest_checkpoint_owner final
{
    public:
	// Read-only genuine retained fee/acceptance correlation, before SQL locks.
	// No source, transaction, physical publication or ACK authority.
	static bool original_fee_acceptance(
		const critical_command &actual_fee_action, critical_command *original_acceptance,
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> *original_typed48) noexcept;
	static bool original_held_bodies(const critical_command &,
					 std::vector<player_item_snapshot> *before,
					 std::vector<player_item_snapshot> *after,
					 player_native_quest_checkpoint_stage *) noexcept;

    private:
	friend class item_native_quest_preparation_owner;
	static bool observe_held(const player_native_quest_checkpoint_token &, P_char,
				 const critical_operation_id &,
				 player_native_quest_checkpoint_stage *,
				 std::vector<player_item_snapshot> * = nullptr) noexcept;
	static critical_submit_result submit_owned(const player_native_quest_checkpoint_token &,
						   critical_command,
						   std::span<const uint8_t> original_native_before,
						   bool *checkpoint_released) noexcept;
};

// Separate capability boundary: preparation cannot invoke publication or ACK.
class player_save_native_quest_publication_owner final
{
    private:
	friend class item_native_quest_publication_owner;
	// Passive original phase1 replay only; no actor/publication/ACK authority.
	static bool restore_recovery_checkpoint(const critical_native_recovery_envelope &) noexcept;
	// Bind only an exact original restored hold to an actual loaded player.
	// The native owner must first prove current SQL/world custody and confirmed
	// rollback. This creates local identity only, never publication authority.
	static bool rebind_recovery_checkpoint(const critical_native_recovery_envelope &,
					       uint64_t actual_player_runtime_id) noexcept;
	static bool copy_recovery_context(const critical_command &,
					  critical_native_recovery_envelope *) noexcept;
	static bool
	checkpoint_recovery_context(const critical_native_recovery_envelope &expected,
				    const critical_native_recovery_envelope &successor) noexcept;
	// Exact live original bodies for native publication, including unrelated
	// final-giver inventory for consumption. No cold/source/ACK authority.
	static bool publication_held_bodies(const critical_command &,
					    std::vector<player_item_snapshot> *,
					    std::vector<player_item_snapshot> *,
					    player_native_quest_checkpoint_stage *) noexcept;

	// The real native owner must authenticate historical receipt, current SQL
	// cut and physical BEFORE/AFTER before this private callback succeeds.
	// Never-admitted remains closed until exact absence/BEFORE proof exists.
	static bool publish_native_quest(const critical_command &, const critical_completion &,
					 bool (*)(const critical_command &,
						  const critical_completion &, void *) noexcept,
					 void *) noexcept;
};

// Actual parentless HOLD source over complete root-zero persisted EQ/INV.
struct player_held_retirement_checkpoint_token
{
	int32_t pid = 0;
	uint64_t actor_runtime_id = 0, root_uid = 0, generation = 0, selected_uid = 0;
	bool operator==(const player_held_retirement_checkpoint_token &) const = default;
};
struct player_held_retirement_checkpoint_stage
{
	player_revision_t save_revision = 0;
};
player_literal_inventory_state
player_save_pipeline_held_retirement_checkpoint_begin(P_char, P_obj actual_held_pick, int room_vnum,
						      player_held_retirement_checkpoint_token *);
player_literal_inventory_state player_save_pipeline_held_retirement_checkpoint_poll(
	const player_held_retirement_checkpoint_token &, P_char,
	player_held_retirement_checkpoint_stage * = nullptr);
bool player_save_pipeline_held_retirement_checkpoint_hold(
	const player_held_retirement_checkpoint_token &, const critical_operation_id &);
bool player_save_pipeline_held_retirement_checkpoint_release(
	const player_held_retirement_checkpoint_token &, const critical_operation_id &);
bool player_save_pipeline_held_retirement_checkpoint_cancel(
	const player_held_retirement_checkpoint_token &);
struct held_retirement_publication_snapshot;
class item_held_retirement_preparation_owner;
class item_held_retirement_publication_owner;
class player_save_held_retirement_checkpoint_owner final
{
    public:
	// Original values only, callable by SQL worker. No publication/ACK authority.
	static bool original_held_bodies(const critical_command &,
					 std::vector<player_item_snapshot> *,
					 std::vector<player_item_snapshot> *,
					 player_held_retirement_checkpoint_stage *) noexcept;

    private:
	friend class item_held_retirement_preparation_owner;
	static bool observe_held(const player_held_retirement_checkpoint_token &, P_char,
				 const critical_operation_id &,
				 player_held_retirement_checkpoint_stage *,
				 std::vector<player_item_snapshot> * = nullptr) noexcept;
	static critical_submit_result submit_owned(const player_held_retirement_checkpoint_token &,
						   critical_command,
						   bool *checkpoint_released) noexcept;
};
class player_save_held_retirement_publication_owner final
{
    private:
	friend class item_held_retirement_publication_owner;
	static bool restore_recovery_checkpoint(const critical_native_recovery_envelope &) noexcept;
	static bool rebind_recovery_checkpoint(const critical_native_recovery_envelope &,
					       uint64_t actual_runtime_id) noexcept;
	static bool copy_recovery_context(const critical_command &,
					  critical_native_recovery_envelope *) noexcept;
	static bool resume_recovery_context(const critical_command &) noexcept;
	static bool copy_publication_context(const critical_command &, const critical_completion &,
					     held_retirement_publication_snapshot *) noexcept;
	static bool checkpoint_recovery_context(const critical_native_recovery_envelope &,
						const critical_native_recovery_envelope &) noexcept;
	static bool publication_held_bodies(const critical_command &,
					    std::vector<player_item_snapshot> *,
					    std::vector<player_item_snapshot> *,
					    player_held_retirement_checkpoint_stage *) noexcept;
	static bool publish_held_retirement(const critical_command &, const critical_completion &,
					    const held_retirement_publication_snapshot &,
					    bool (*)(const critical_command &,
						     const critical_completion &, void *) noexcept,
					    void *) noexcept;
};

// Critical replay restores a SQL ordinary-drop obligation without inventing a
// live runtime token or checkpoint revision. Identical immutable commands are
// idempotent; conflicting identity or capacity refuses before admission.
// Registration is restricted to the prepared/no-execution phase. A private
// generation fences ordinary save apply and journal retirement; broader native
// mutation coverage, clean census and critical-ACK reservation remain required.
bool player_save_pipeline_restore_sql_drop_obligation(const critical_command &command);
// The same prepared original-command slot/reservation binds the wallet PID and
// pile UID for supported single ordinary-room SQL coin drop/pickup replay.
// Classification or a matching ID grants neither native proof nor ACK authority.
bool player_save_pipeline_restore_sql_coin_obligation(const critical_command &command);
// The restored slot owns publication and ACK together. No caller can construct
// an ACK capability from a graph observation, operation ID or readiness bool.
// Typed singleton purchase admission retains exact bytes before coordinator
// admission. Flatfile stays closed until its native publication owner is present.
critical_submit_result collector_purchase_submit_owned(critical_command command);
bool player_save_pipeline_restore_sql_collector_purchase_obligation(const critical_command &);
bool player_save_pipeline_restore_sql_shop_obligation(const critical_command &);

struct player_save_journal_retained_frame;
class player_save_restored_publication_owner final
{
	friend class player_save_shop_checkpoint_owner;

    public:
	static bool publish(const critical_completion &completion) noexcept;

    private:
	friend class player_save_native_quest_publication_owner;
	friend class player_save_held_retirement_publication_owner;
	friend bool critical_command_coordinator_cancel_held_retirement_publication(
		player_save_restored_publication_owner &, const critical_native_recovery_envelope &,
		bool (*)(const critical_command &, const critical_completion &, void *) noexcept,
		void *) noexcept;
	friend class player_save_auction_publication_owner;
	friend class collector_purchase_publication_owner;
	friend class shop_trade_native_publication_owner;
	friend bool critical_command_coordinator_restored_shop_publication_current(
		const player_save_restored_publication_owner &) noexcept;
	static bool publish_shop(const critical_command &, const critical_completion &,
				 bool (*native_publish)(const critical_command &,
							const critical_completion &,
							void *) noexcept,
				 void *) noexcept;

	// Private passive replay registration before execution starts. The native
	// owner supplies its genuine retained command/UID-union allocation census.
	// No live token, source stage, historical ACK or publication proof is restored.
	static bool restore_shop_flat_obligation(const critical_command &,
						 size_t native_owner_retained_bytes) noexcept;

	// Distinct cold holder: no live token, READY body or attempted-native charge.
	static bool publish_shop_flat_restored(const critical_command &,
					       const critical_completion &,
					       bool (*)(player_save_restored_publication_owner &,
							void *) noexcept,
					       void *) noexcept;
	// One immutable complete cold-stage capacity charge, borrowing this ticket.
	bool reserve_shop_flat_restored_publication_checkpoint(size_t) noexcept;

	// The same original live flat slot retains command and ACK/body separately.
	// Only the genuine native owner may provide publication/refusal cleanup.
	static bool publish_shop_flat(const critical_command &, const critical_completion &,
				      bool (*)(player_save_restored_publication_owner &,
					       void *) noexcept,
				      void *) noexcept;
	friend bool critical_command_coordinator_cancel_shop_publication(
		player_save_restored_publication_owner &,
		bool (*)(const critical_command &, const critical_completion &, void *) noexcept,
		void *);
	friend bool critical_command_coordinator_cancel_auction_publication(
		player_save_restored_publication_owner &,
		bool (*)(const critical_command &, const critical_completion &, void *) noexcept,
		void *);
	// Only the integrated native collector owner can call this; a public true
	// callback must never fabricate authority to consume a save reservation.
	static bool publish_collector(const critical_command &, const critical_completion &,
				      bool (*native_publish)(const critical_command &,
							     const critical_completion &,
							     void *) noexcept,
				      void *) noexcept;
	friend bool critical_command_coordinator_cancel_collector_publication(
		player_save_restored_publication_owner &);
	friend bool critical_command_coordinator_acknowledge_publication(
		player_save_restored_publication_owner &owner);
	player_save_restored_publication_owner(critical_command &&command,
					       std::vector<uint8_t> &&frozen,
					       const critical_completion &completion,
					       uint64_t epoch, int pid,
					       uint64_t generation) noexcept
		: command_(std::move(command))
		, frozen_(std::move(frozen))
		, completion_(completion)
		, reservation_(epoch, pid, completion.operation_id, generation)
		, pid_(pid)
		, generation_(generation)
	{
	}
	// Construction of covered-revision evidence remains with this original
	// owner; native publication cannot write the evidence fields.
	static bool
	retire_covered_ordinary(player_save_restored_publication_owner &,
				const std::vector<player_save_journal_retained_frame> &) noexcept;
	bool consume_acknowledged_hold() noexcept;
	bool consume_acknowledged_flat_shop_hold() noexcept;
	bool consume_acknowledged_flat_shop_restored_hold() noexcept;
	critical_command command_;
	std::vector<uint8_t> frozen_;
	critical_completion completion_;
	player_save_execution_guard::held_publication_reservation reservation_;
	int pid_ = 0;
	uint64_t generation_ = 0, coordinator_generation_ = 0;
	bool publication_proven_ = false, acknowledged_ = false;
	bool flat_shop_ = false;
	// Set only after exact canonical command and real passive slot correlation.
	bool flat_shop_restored_ = false;
	uint64_t flat_shop_restored_root_uid_ = 0, flat_shop_restored_epoch_ = 0;
};

// A restored drop may hydrate authoritative state while saves/lifecycle remain
// held. All other recovery, target-login and pinned-death fences still refuse.
bool player_save_pipeline_authoritative_hydration_admitted(int pid);
// Live-token cleanup after durable coordinator ACK. Restored holds refuse this
// ID-only assertion; their private owner consumes the exact checked reservation
// and emits the worker/replay notice after guarded ACK.
void player_save_pipeline_sql_drop_publication_acknowledged(
	const critical_operation_id &operation_id) noexcept;

// Capture progression and its quest reward identities in one save-journal frame.
// SQL applies the experience snapshot and receipt mask in the same transaction.
player_save_pipeline_result
player_save_pipeline_request_quest_xp(P_char ch, player_component_mask_t components,
				      const player_quest_xp_receipt_snapshot *receipts,
				      size_t receipt_count, int room_vnum);
player_save_pipeline_result
player_save_pipeline_request_spell_effect(P_char ch, player_component_mask_t components,
					  const player_spell_effect_receipt_snapshot *receipt,
					  int room_vnum);
player_save_terminal_result player_save_pipeline_terminal(P_char ch, int save_intent, int room_vnum,
							  uint64_t timeout_msec,
							  bool allow_journal_handoff);
// Resume the exact in-memory format-8 death request for corpse_uid (zero uses
// the pinned corpse). Returns not_pending only when no pinned death exists;
// journal durability never releases it.
player_save_terminal_result
player_save_pipeline_terminal_death_resume(P_char ch, uint64_t corpse_uid, uint64_t timeout_msec);
// Capture the immutable death disposition for ch and wait for its database ACK.
// wallet_pile may be null; when the wallet still holds coins it must be an
// unattached pile carrying the complete remaining wallet. allow_journal_handoff
// is retained for call compatibility; death requests never accept journal-only ACKs.
player_save_terminal_result
player_save_pipeline_terminal_death(P_char ch, P_obj corpse, P_obj wallet_pile,
				    const critical_operation_id &operation_id, int room_vnum,
				    uint64_t timeout_msec, bool allow_journal_handoff);
void player_save_pipeline_pulse(void);
void player_save_pipeline_quiesce(void);
void player_save_pipeline_resume(void);
bool player_save_pipeline_drain(uint64_t timeout_msec);
// Enabled lifecycle owner on the game thread, with the coordinator guard held.
// Failed close retains originals and closes load/save admission for retry.
bool player_save_pipeline_drain_owned(uint64_t timeout_msec);
bool player_save_pipeline_shutdown_owned(void);
player_save_pipeline_health player_save_pipeline_health_copy(void);
// Normal account, legacy, and copyover materialization is forbidden until the
// startup save-journal replay has completed successfully.
bool player_save_pipeline_loads_allowed(void);
size_t player_save_pipeline_dirty_count(void);
bool player_save_pipeline_is_nonterminal_type(int save_intent);
// Exact-PID save/login barrier used by offline critical commands.  A target
// fence rejects new saves for that PID without quiescing unrelated players.
bool player_save_pipeline_target_save_pending(int pid);
// Game-thread creation admission waits for sealed saves, while dirty components
// remain eligible for capture after the grant is published.
bool player_save_pipeline_sealed_save_pending(int pid);
bool player_save_pipeline_acquire_target_save_login_fence(int pid,
							  player_revision_t expected_revision);
void player_save_pipeline_release_target_save_login_fence(int pid,
							  player_revision_t expected_revision);
bool player_save_pipeline_target_save_login_fenced(int pid);
bool player_save_pipeline_save_admitted(int pid);
void player_save_pipeline_reset_for_tests(void);

// Passive exact current physical literal checkpoint pool observation under
// pipeline_mutex. Includes complete pool inline, its 13 byte-vector capacities,
// two string heaps per slot and both generations. Excludes all other pipeline
// workers/queues/mutex/health and save execution-guard storage. Strong output;
// no readiness, hold, admission or authority. Do not call while holding the
// same pipeline mutex; owning bounded relays use the genuine locked provider.
bool player_save_pipeline_literal_replay_storage_bytes(size_t *) noexcept;

// Genuine same-lock passive currency replay memory owner. Only the complete
// mixed currency replay counterpart can construct it. Outer excludes this
// literal pool, complete other-pipeline owner, prepared worker and live scope;
// includes authentic currency/coordinator/journal/input/execution-guard owners.
// Scope adds each physical CURRENT owner exactly once at every cut under its
// actual pipeline mutex; the prepared worker uses its separate leaf mutex.
// Prepared startup excludes dispatcher/worker execution. The ROOT startup
// owner must separately exclude concurrent lifecycle callers; idle is not proof.
// Reserve MUST NOT acquire pipeline/coordinator/journal or mutate replay owners.
// Startup/main-thread coordinator ownership supplies currency pending stability.
// Scope is required before first allocating proof, including wallet-only replay.
// Original selected functions and backend/activation gates remain unchanged.
class player_save_coin_replay_budget_scope_owner final
{
    private:
	friend class critical_mixed_startup_replay_owner;
	friend class player_save_sql_drop_replay_owner;
	friend class player_save_shop_replay_owner;
	friend class player_save_sql_collector_replay_owner;
	friend class player_save_native_recovery_replay_owner;
	friend class player_save_auction_replay_owner;
	friend bool player_save_pipeline_replay_current_storage_bytes(size_t *) noexcept;
	friend bool player_save_pipeline_replay_current_storage_source_frames(size_t *) noexcept;
	friend bool
	player_save_pipeline_restore_sql_drop_obligation_bounded(const critical_command &,
								 bool (*)(size_t, void *) noexcept,
								 void *, size_t) noexcept;
	friend class currency_transaction_replay_owner;
	friend bool currency_transaction_restore_replayed_command_bounded(const critical_command &,
									  bool (*)(size_t,
										   void *) noexcept,
									  void *, size_t) noexcept;
	player_save_coin_replay_budget_scope_owner(bool (*)(size_t, void *) noexcept, void *);
	~player_save_coin_replay_budget_scope_owner() noexcept;
	player_save_coin_replay_budget_scope_owner(
		const player_save_coin_replay_budget_scope_owner &) = delete;
	player_save_coin_replay_budget_scope_owner &
	operator=(const player_save_coin_replay_budget_scope_owner &) = delete;
	bool locked() const noexcept;
	bool prepared() const noexcept;
	bool prepared_worker_storage_bytes(size_t *) const noexcept;

    public:
	// Pure complete fixed source-carrier handoff: ROOT admits these before
	// constructing/scanning this scope. No storage/readiness is inferred.
	static size_t observer_frame_bytes() noexcept;
	static constexpr size_t observer_source_profile_query_frames() noexcept
	{
		// This accessor's result, both genuine observer getter results, nine
		// worker getter scalar locals, and final maximum operands/result/bool.
		// Pure query only: no scope construction, readiness, lock or census.
		return 15 * sizeof(size_t) + sizeof(bool);
	}

    private:
	static size_t prepared_worker_observer_frame_bytes() noexcept;
	// ROOT holds authentic startup coordinator/lifecycle exclusion and this
	// SAME pipeline scope across fresh bootstrap and complete replay. No DTO,
	// cache, logical-byte baseline or first-call qualification is supplied.
	bool bootstrap_storage_bytes(size_t *) const noexcept;
	bool prefix(size_t exclusive, size_t &full) const noexcept;
	bool admit(size_t exclusive) const noexcept;
	static bool reserve_exclusive(size_t exclusive, void *context) noexcept;
	bool restore(const critical_command &, size_t exclusive_outer) const noexcept;
	bool restore_obligation(const critical_command &, int, uint64_t,
				size_t exclusive_outer) const;
	bool restore_publication_profile(const critical_command &, int, uint64_t,
					 size_t exclusive_outer, bool shop) const;
	std::unique_lock<std::mutex> lock_;
	bool (*reserve_)(size_t, void *) noexcept;
	void *context_;
};

// ONE real process-wide lifecycle exclusion. C++20 inline linkage makes the
// coordinator and pipeline use the same mutex even in minimal link targets.
// Its retained inline storage is counted once in pipeline OTHER, not CURRENT C.
namespace player_save_pipeline_lifecycle_detail
{
inline std::mutex mutex;
}

// Genuine creation/join exclusion, acquired BEFORE coordinator/pipeline locks.
// Only the actual mixed startup owner can construct it. It is an owning lock,
// not readiness, a byte snapshot, an activation permit or a worker-idle flag.
class player_save_prepared_startup_lifecycle_owner final
{
    public:
	~player_save_prepared_startup_lifecycle_owner() noexcept = default;

    private:
	friend class critical_mixed_startup_replay_owner;
	player_save_prepared_startup_lifecycle_owner()
		: lock_(player_save_pipeline_lifecycle_detail::mutex)
	{
	}
	player_save_prepared_startup_lifecycle_owner(
		const player_save_prepared_startup_lifecycle_owner &) = delete;
	player_save_prepared_startup_lifecycle_owner &
	operator=(const player_save_prepared_startup_lifecycle_owner &) = delete;
	bool held() const noexcept
	{
		return lock_.mutex() == &player_save_pipeline_lifecycle_detail::mutex &&
		       lock_.owns_lock();
	}
	std::unique_lock<std::mutex> lock_;
};

// Genuine same-scope SQL ordinary-drop bridge for complete mixed startup.
// ROOT retains real coordinator/lifecycle exclusion and this same pipeline
// scope; exclusive_outer excludes CURRENT reported by borrowed getter. Every
// prospective request refreshes scope storage once. No post-hold reservation.
class player_save_sql_drop_replay_owner final
{
    public:
	static size_t frame_bytes() noexcept;
	// Includes actual scope inline; fixed observer frames separately admitted.
	static bool current_storage_bytes(player_save_coin_replay_budget_scope_owner &,
					  size_t *) noexcept;
	static bool restore(const critical_command &, player_save_coin_replay_budget_scope_owner &,
			    size_t exclusive_outer) noexcept;
};
// External unheld observer counts retained pipeline/worker owners, excludes
// transient scope inline. Must not be called while pipeline_mutex is held;
// use borrowed getter for mixed replay. All getters preserve output on refusal.
bool player_save_pipeline_replay_current_storage_bytes(size_t *) noexcept;
// Pure source profile, called and admitted BEFORE the external unheld observer.
// It creates no scope and takes no lock; it includes the real temporary scope
// and complete held observer closure, rather than any retained-storage baseline.
constexpr size_t player_save_pipeline_replay_current_storage_source_profile_query_frames() noexcept
{
	return sizeof(size_t *) + 15 * sizeof(size_t) + 2 * sizeof(bool);
}
bool player_save_pipeline_replay_current_storage_source_frames(size_t *) noexcept;

bool player_save_pipeline_restore_sql_drop_obligation_bounded(const critical_command &,
							      bool (*)(size_t, void *) noexcept,
							      void *,
							      size_t exclusive_outer) noexcept;

// Same-scope passive SHOP/Collector replay, paired with full bounded decoders.
class player_save_shop_replay_owner final
{
    public:
	static size_t frame_bytes() noexcept;
	// Additional wrapper only; ROOT owns the complete same-scope observer.
	static size_t current_observer_frame_bytes() noexcept;
	static bool current_storage_bytes(player_save_coin_replay_budget_scope_owner &,
					  size_t *) noexcept;
	static bool restore_sql(const critical_command &,
				player_save_coin_replay_budget_scope_owner &,
				size_t exclusive_outer) noexcept;
	// Complete original flat hold law; full provider/source joins required.
	static bool restore_flat(const critical_command &, size_t native_owner_retained_bytes,
				 player_save_coin_replay_budget_scope_owner &,
				 size_t exclusive_outer) noexcept;
};
class player_save_sql_collector_replay_owner final
{
    public:
	static size_t frame_bytes() noexcept;
	static bool current_storage_bytes(player_save_coin_replay_budget_scope_owner &,
					  size_t *) noexcept;
	static bool restore(const critical_command &, player_save_coin_replay_budget_scope_owner &,
			    size_t exclusive_outer) noexcept;
};

// Full original native/held passive checkpoints on the SAME genuine pipeline
// lock. The caller's reserve relay refreshes provider and pipeline CURRENT.
// No stage, gameplay, receipt, publication or ACK authority is introduced.
class item_native_recovery_replay_owner;
class player_save_native_recovery_replay_owner final
{
    public:
	static bool restore_held_source_frames(size_t *) noexcept;
	static bool restore_native_source_frames(size_t *) noexcept;
	static constexpr size_t source_profile_query_frames() noexcept
	{
		return 2 * sizeof(void *) + 42 * sizeof(size_t) + 6 * sizeof(bool);
	}

	static size_t current_observer_frame_bytes() noexcept;
	static bool current_storage_bytes(player_save_coin_replay_budget_scope_owner &,
					  size_t *) noexcept;

    private:
	friend class item_native_recovery_replay_owner;
	static bool restore_held_checkpoint(const critical_native_recovery_envelope &,
					    player_save_coin_replay_budget_scope_owner &,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t) noexcept;
	static bool restore_native_checkpoint(const critical_native_recovery_envelope &,
					      player_save_coin_replay_budget_scope_owner &,
					      bool (*)(size_t, void *) noexcept, void *,
					      size_t) noexcept;
};

class player_save_auction_replay_owner final
{
    public:
	static bool restore_source_frames(size_t *) noexcept;
	static constexpr size_t source_profile_query_frames() noexcept
	{
		return 2 * sizeof(void *) + 42 * sizeof(size_t) + 6 * sizeof(bool);
	}

	static size_t current_observer_frame_bytes() noexcept;
	static bool current_storage_bytes(player_save_coin_replay_budget_scope_owner &,
					  size_t *) noexcept;
	static bool restore_recovery_checkpoint(const critical_native_recovery_envelope &,
						player_save_coin_replay_budget_scope_owner &,
						bool (*)(size_t, void *) noexcept, void *,
						size_t) noexcept;
};

#endif
