#ifndef GAMEPLAY_READ_STATE_H
#define GAMEPLAY_READ_STATE_H

#include <cstddef>
#include <cstdint>

constexpr size_t GAMEPLAY_READ_RECENT_DURABLE_MAX = 20;
constexpr size_t GAMEPLAY_READ_RECENT_PENDING_MAX = 128;
constexpr size_t GAMEPLAY_READ_RECENT_MAX =
	GAMEPLAY_READ_RECENT_DURABLE_MAX + GAMEPLAY_READ_RECENT_PENDING_MAX;
constexpr size_t GAMEPLAY_READ_COMPLETED_ZONE_MAX = 1024;
constexpr int64_t GAMEPLAY_READ_RECENT_WINDOW_SECONDS = 60 * 60;

enum class gameplay_read_status : uint8_t
{
	unavailable,
	ready,
};

struct gameplay_recent_death
{
	int64_t occurred_at;
	uint64_t token;
	bool provisional;
};

struct gameplay_read_state
{
	gameplay_read_status status;
	size_t recent_death_count;
	gameplay_recent_death recent_deaths[GAMEPLAY_READ_RECENT_MAX];
	size_t completed_zone_count;
	int32_t completed_zones[GAMEPLAY_READ_COMPLETED_ZONE_MAX];
	uint64_t next_token;
};

void gameplay_read_state_reset(gameplay_read_state *state);
bool gameplay_read_state_publish(gameplay_read_state *state, const int64_t *recent_deaths,
				 size_t recent_death_count, const int32_t *completed_zones,
				 size_t completed_zone_count);
size_t gameplay_read_state_recent_count(const gameplay_read_state *state, int64_t now);
bool gameplay_read_state_heaven_seconds(const gameplay_read_state *state, int64_t now,
					int base_seconds, int *seconds);
uint64_t gameplay_read_state_add_provisional(gameplay_read_state *state, int64_t occurred_at);
bool gameplay_read_state_finish_provisional(gameplay_read_state *state, uint64_t token,
					    bool committed);
bool gameplay_read_state_zone_completed(const gameplay_read_state *state, int32_t zone_number);
bool gameplay_read_state_add_completed_zone(gameplay_read_state *state, int32_t zone_number);

#endif
