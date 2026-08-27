#ifndef PLAYER_SAVE_PIPELINE_H
#define PLAYER_SAVE_PIPELINE_H

#include "player_revision_state.h"

#include <cstddef>
#include <cstdint>

struct char_data;
typedef struct char_data *P_char;

constexpr size_t PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS = 128;
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
	bool initialized;
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
void player_save_pipeline_pulse(void);
player_save_pipeline_health player_save_pipeline_health_copy(void);
size_t player_save_pipeline_dirty_count(void);
bool player_save_pipeline_is_nonterminal_type(int save_intent);
void player_save_pipeline_reset_for_tests(void);

#endif
