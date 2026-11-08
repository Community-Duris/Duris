#ifndef ECONOMIC_EVENT_OUTBOX_H
#define ECONOMIC_EVENT_OUTBOX_H

#include <cstddef>
#include <cstdint>
#include <vector>

enum class EconomicAction : uint16_t
{
	UNKNOWN = 0,
	WALLET_ADD = 1,
	WALLET_SUB = 2,
	SHOP_BUY = 3,
	SHOP_SELL = 4,
	MOB_LOOT = 5,
	DEATH_LOSS = 6,
	BANK_DEPOSIT = 7,
	BANK_WITHDRAW = 8,
	AUCTION_BID = 9,
	AUCTION_PAYOUT = 10,
	TRADE_GIVE = 11,
	SPLIT_COINS = 12,
	BOON_REWARD = 13,
	MINING_FIND = 14,
	CRAFTING_COST = 15,
	STAFF_FORCE = 16,
	UNCLASSIFIED_INFLOW = 998,
	UNCLASSIFIED_OUTFLOW = 999
};

struct EconomicEvent
{
	uint64_t event_id = 0;
	uint64_t timestamp_usec = 0;
	uint32_t actor_pid = 0;
	EconomicAction action = EconomicAction::UNKNOWN;
	int64_t copper_delta = 0;
	uint32_t target_id = 0;
	uint32_t context_vnum = 0;
	char reason[32] = {};
};

struct EconomicOutboxHealth
{
	size_t queue_depth = 0;
	uint64_t total_recorded = 0;
	uint64_t total_drained = 0;
	uint64_t total_dropped = 0;
	uint64_t unclassified_inflows = 0;
	uint64_t unclassified_outflows = 0;
};

// Records an economic event into the thread-safe ring buffer outbox.
// Any unmapped/unknown action automatically falls back to UNCLASSIFIED_INFLOW or UNCLASSIFIED_OUTFLOW.
// Never blocks, never panics, never fails gameplay.
void economic_event_record(uint32_t actor_pid, EconomicAction action, int64_t copper_delta,
			   uint32_t target_id, uint32_t context_vnum, const char *reason);

// Drains up to max_events from the ring buffer into the provided batch vector.
// Returns the number of events drained.
size_t economic_event_drain(std::vector<EconomicEvent> *batch_out, size_t max_events);

// Returns a copy of the outbox health metrics.
EconomicOutboxHealth economic_event_outbox_health(void);

// Flushes drained outbox events to the background persistence writer.
// Called during periodic persistence pulse.
void economic_event_flush_outbox(void);

// Reset outbox state (for unit / regression tests).
void economic_event_outbox_reset_for_tests(void);

#endif
