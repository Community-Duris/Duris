#ifndef PLAYER_SAVE_PIPELINE_H
#define PLAYER_SAVE_PIPELINE_H

#include "player/player_revision_state.h"
#include "persistence/critical_command.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;
struct player_quest_xp_receipt_snapshot;
struct player_spell_effect_receipt_snapshot;

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

bool player_save_pipeline_init(const char *journal_directory);
void player_save_pipeline_shutdown(void);
bool player_save_pipeline_mark(int pid, player_component_mask_t components);
player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum);
player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum);
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
