#ifndef PLAYER_SAVE_PIPELINE_H
#define PLAYER_SAVE_PIPELINE_H

#include "player/player_revision_state.h"
#include "persistence/critical_command.h"

#include <cstddef>
#include <cstdint>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;

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

bool player_save_pipeline_init(const char *journal_directory);
void player_save_pipeline_shutdown(void);
bool player_save_pipeline_mark(int pid, player_component_mask_t components);
player_save_pipeline_result player_save_pipeline_checkpoint_dirty(P_char ch, int save_intent,
								  int room_vnum);
player_save_pipeline_result player_save_pipeline_request(P_char ch,
							 player_component_mask_t components,
							 int save_intent, int room_vnum);
player_save_terminal_result player_save_pipeline_terminal(P_char ch, int save_intent, int room_vnum,
							  uint64_t timeout_msec,
							  bool allow_journal_handoff);
// Capture the immutable death disposition for ch and wait for it to become
// durable. wallet_pile may be null; when the wallet still holds coins it must be
// an unattached pile carrying the complete remaining wallet.
player_save_terminal_result
player_save_pipeline_terminal_death(P_char ch, P_obj corpse, P_obj wallet_pile,
				    const critical_operation_id &operation_id, int room_vnum,
				    uint64_t timeout_msec, bool allow_journal_handoff);
void player_save_pipeline_pulse(void);
void player_save_pipeline_quiesce(void);
void player_save_pipeline_resume(void);
bool player_save_pipeline_drain(uint64_t timeout_msec);
player_save_pipeline_health player_save_pipeline_health_copy(void);
size_t player_save_pipeline_dirty_count(void);
bool player_save_pipeline_is_nonterminal_type(int save_intent);
void player_save_pipeline_reset_for_tests(void);

#endif
