#ifndef PLAYER_SAVE_PIPELINE_H
#define PLAYER_SAVE_PIPELINE_H

#include "player/player_revision_state.h"
#include "persistence/critical_command.h"
#include "persistence/critical_command_completion.h"
#include "persistence/critical_command_coordinator.h"
#include "player/player_save_replay_ownership.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;
struct player_quest_xp_receipt_snapshot;
struct player_spell_effect_receipt_snapshot;
struct player_item_snapshot;
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

class shop_trade_preparation_owner;
class shop_trade_native_publication_owner;
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

class player_save_restored_publication_owner final
{
    public:
	static bool publish(const critical_completion &completion) noexcept;

    private:
	friend class collector_purchase_publication_owner;
	friend class shop_trade_native_publication_owner;
	static bool publish_shop(const critical_command &, const critical_completion &,
				 bool (*native_publish)(const critical_command &,
							const critical_completion &,
							void *) noexcept,
				 void *) noexcept;
	friend bool critical_command_coordinator_cancel_shop_publication(
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
	bool consume_acknowledged_hold() noexcept;
	critical_command command_;
	std::vector<uint8_t> frozen_;
	critical_completion completion_;
	player_save_execution_guard::held_publication_reservation reservation_;
	int pid_ = 0;
	uint64_t generation_ = 0, coordinator_generation_ = 0;
	bool publication_proven_ = false, acknowledged_ = false;
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

#endif
