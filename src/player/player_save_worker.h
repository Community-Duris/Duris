#ifndef PLAYER_SAVE_WORKER_H
#define PLAYER_SAVE_WORKER_H

#include "player/player_snapshot.h"
#include "persistence/persistence_diagnostics.h"

#include <cstddef>
#include <cstdint>
#include <vector>

constexpr size_t PLAYER_SAVE_WORKER_MAX_PIDS = 256;
constexpr size_t PLAYER_SAVE_WORKER_MAX_RESULTS = 256;
constexpr size_t PLAYER_SAVE_WORKER_MAX_BYTES = 32 * 1024 * 1024;
constexpr uint64_t PLAYER_SAVE_WORKER_MAX_AGE_MSEC = 5 * 60 * 1000;
constexpr unsigned int PLAYER_SAVE_WORKER_MAX_RETRIES = 8;
constexpr unsigned int PLAYER_SAVE_WORKER_DEFAULT_THREADS = 2;
/* Application-specific repository error: a replacement player-item graph did
 * not exactly match authoritative active custody. */
constexpr unsigned int PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH = 10001;

// Low-cardinality, redacted reasons for rejecting a custody payload. These are
// diagnostic context only; the public save error remains the generic mismatch.
enum class player_save_custody_diagnosis : uint8_t
{
	none = 0,
	invalid_snapshot_item = 1,
	invalid_snapshot_parent = 2,
	duplicate_snapshot_uid = 3,
	malformed_active_custody_row = 4,
	duplicate_equipment_slot = 5,
	active_custody_absent_from_snapshot = 6,
	custody_vnum_mismatch = 7,
	duplicate_custody_match = 8,
	snapshot_item_absent_from_custody = 9,
	invalid_custody_topology = 10,
	invalid_death_payload = 11,
	saved_item_absent_from_death_payload = 12,
	orphaned_saved_item = 13,
	orphaned_saved_pet_item = 14,
};

inline const char *player_save_custody_diagnosis_name(player_save_custody_diagnosis diagnosis)
{
	switch (diagnosis)
	{
	case player_save_custody_diagnosis::invalid_snapshot_item:
		return "invalid_snapshot_item";
	case player_save_custody_diagnosis::invalid_snapshot_parent:
		return "invalid_snapshot_parent";
	case player_save_custody_diagnosis::duplicate_snapshot_uid:
		return "duplicate_snapshot_uid";
	case player_save_custody_diagnosis::malformed_active_custody_row:
		return "malformed_active_custody_row";
	case player_save_custody_diagnosis::duplicate_equipment_slot:
		return "duplicate_equipment_slot";
	case player_save_custody_diagnosis::active_custody_absent_from_snapshot:
		return "active_custody_absent_from_snapshot";
	case player_save_custody_diagnosis::custody_vnum_mismatch:
		return "custody_vnum_mismatch";
	case player_save_custody_diagnosis::duplicate_custody_match:
		return "duplicate_custody_match";
	case player_save_custody_diagnosis::snapshot_item_absent_from_custody:
		return "snapshot_item_absent_from_custody";
	case player_save_custody_diagnosis::invalid_custody_topology:
		return "invalid_custody_topology";
	case player_save_custody_diagnosis::invalid_death_payload:
		return "invalid_death_payload";
	case player_save_custody_diagnosis::saved_item_absent_from_death_payload:
		return "saved_item_absent_from_death_payload";
	case player_save_custody_diagnosis::orphaned_saved_item:
		return "orphaned_saved_item";
	case player_save_custody_diagnosis::orphaned_saved_pet_item:
		return "orphaned_saved_pet_item";
	case player_save_custody_diagnosis::none:
	default:
		return "none";
	}
}

enum class player_save_apply_outcome : uint8_t
{
	applied,
	already_applied,
	stale_revision,
	retryable_failure,
	terminal_failure,
	ambiguous_commit,
	// An authority gate has not admitted repository execution. Retain the exact
	// request without a result, ACK, failure retry or quarantine.
	deferred,
};

struct player_save_apply_result
{
	player_save_apply_outcome outcome;
	player_revision_t durable_revision;
	unsigned int error_code;
	player_save_custody_diagnosis custody_diagnosis = player_save_custody_diagnosis::none;
	// Replay may retire an obsolete non-death frame only after the repository
	// verifies every attached operation receipt. This does not ACK a live save.
	bool operation_receipts_verified = false;
	persistence_custody_witness custody_witness = {};
};

// A newer revision alone cannot prove that death disposition or an attached
// operation receipt committed. Only the exact successful save can ACK live state.
inline bool player_save_result_matches_exact_request(const player_snapshot &snapshot,
						     const player_save_apply_result &result)
{
	const bool exact_required = snapshot.death || !snapshot.quest_xp_receipts.empty() ||
				    !snapshot.spell_effect_receipts.empty() ||
				    !snapshot.craft_receipts.empty();
	return !exact_required || ((result.outcome == player_save_apply_outcome::applied ||
				    result.outcome == player_save_apply_outcome::already_applied) &&
				   result.durable_revision == snapshot.revision);
}

struct player_save_completion
{
	int32_t pid;
	player_revision_t revision;
	player_component_mask_t components;
	player_save_apply_outcome outcome;
	player_revision_t durable_revision;
	unsigned int error_code;
	player_save_custody_diagnosis custody_diagnosis = player_save_custody_diagnosis::none;
	unsigned int retry_count;
	uint64_t queued_at_usec;
	uint64_t started_at_usec;
	uint64_t completed_at_usec;
	// Populated only after the exact snapshot succeeds and its revision is
	// acknowledged. Owners can match operation IDs without inferring from a counter.
	std::vector<player_quest_xp_receipt_snapshot> quest_xp_receipts;
	std::vector<player_spell_effect_receipt_snapshot> spell_effect_receipts;
	// A final failed attempt releases the owner's retry gate without granting an ACK.
	std::vector<player_spell_effect_receipt_snapshot> failed_spell_effect_receipts;
	std::vector<player_craft_receipt_snapshot> craft_receipts = {};
	std::vector<player_craft_receipt_snapshot> failed_craft_receipts = {};
	persistence_custody_witness custody_witness = {};
};

enum class player_save_submit_result : uint8_t
{
	accepted,
	coalesced,
	invalid,
	stale,
	capacity_exceeded,
	revision_state_mismatch,
	worker_unavailable,
	journal_failure,
	durably_spilled,
};

struct player_save_worker_health
{
	uint64_t queued_pids;
	uint64_t inflight_pids;
	uint64_t deferred_pids;
	uint64_t queued_bytes;
	uint64_t high_water_pids;
	uint64_t high_water_bytes;
	uint64_t oldest_age_msec;
	bool age_limit_exceeded;
	uint64_t submitted;
	uint64_t coalesced;
	uint64_t applied;
	uint64_t stale;
	uint64_t retryable_failures;
	uint64_t journal_ack_failures;
	uint64_t terminal_failures;
	uint64_t custody_payload_mismatches;
	uint64_t retries_exhausted;
	uint64_t max_capture_to_apply_usec;
	uint64_t max_apply_usec;
	uint64_t max_ack_latency_usec;
	uint64_t max_revision_gap;
	unsigned int worker_threads;
	unsigned int running_workers;
	bool running;
	bool stop_pending;
};

using player_save_apply_fn = player_save_apply_result (*)(const player_snapshot &snapshot,
							  void *context);
using player_save_journal_append_fn = bool (*)(const player_snapshot &snapshot, void *context);
using player_save_journal_ack_fn = bool (*)(const player_snapshot &snapshot,
					    player_revision_t durable_revision, void *context);
// A terminal hook must establish a login/save fence even when archive I/O
// fails. It runs on the worker before publishing the completion.
using player_save_journal_terminal_fn = void (*)(const player_snapshot &snapshot,
						 void *context) noexcept;

bool player_save_worker_init(player_save_apply_fn apply, void *context,
			     unsigned int worker_threads = PLAYER_SAVE_WORKER_DEFAULT_THREADS);
void player_save_worker_shutdown(void);
bool player_save_worker_set_journal_hooks(player_save_journal_append_fn append,
					  player_save_journal_ack_fn acknowledge, void *context,
					  player_save_journal_terminal_fn terminal = nullptr);
player_save_submit_result player_save_worker_submit(player_snapshot snapshot);
player_save_submit_result player_save_worker_submit_retained(player_snapshot *snapshot);
size_t player_save_worker_pulse(player_save_completion *completions_out, size_t capacity);
// Return true while this PID has a queued or executing worker snapshot.  The
// player save/login fence uses this exact-PID query; aggregate health is not a
// sufficient admission check for a recipient-only operation.
bool player_save_worker_pid_pending(int pid);
// Schedule one parked original request. This is a wakeup, not authority to apply:
// the callback must recheck its gate. False leaves the request parked, including
// on allocation failure. Call outside pipeline/journal locks; repeated wakeups
// cannot queue duplicate execution. A wake before parking returns false: its
// owner must retain the wake and retry, rather than consume a one-shot event.
// The normal pipeline does not use this yet. Journal replay has its own deferred
// result; this wake schedules only the retained worker request.
bool player_save_worker_resume_deferred(int pid) noexcept;
player_save_worker_health player_save_worker_health_copy(void);
void player_save_worker_reset_for_tests(void);

#endif
