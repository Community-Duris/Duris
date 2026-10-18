#include "economy/economic_event_outbox.h"
#include "player/player_save_worker.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
std::vector<persistence_job_write_fn> submitted_jobs;
uint64_t last_submitted_owner = 0;
persistence_job_kind last_submitted_kind = persistence_job_kind::player;
size_t last_submitted_bytes = 0;
} // namespace

// Stubs for standalone test compilation
void logit(const char * /*filename*/, const char * /*format*/, ...)
{
}

player_save_submit_result persistence_writer_submit(persistence_job_kind kind, uint64_t owner,
						    size_t bytes, persistence_job_write_fn write)
{
	last_submitted_kind = kind;
	last_submitted_owner = owner;
	last_submitted_bytes = bytes;
	submitted_jobs.push_back(std::move(write));
	return player_save_submit_result::accepted;
}

static void test_basic_record_and_drain()
{
	economic_event_outbox_reset_for_tests();

	economic_event_record(101, EconomicAction::WALLET_ADD, 1500, 3001, 0, "test_wallet_add");
	economic_event_record(101, EconomicAction::SHOP_BUY, -250, 3001, 1045, "test_shop_buy");
	economic_event_record(202, EconomicAction::MOB_LOOT, 75, 4100, 5020, "test_mob_loot");

	EconomicOutboxHealth health = economic_event_outbox_health();
	assert(health.queue_depth == 3);
	assert(health.total_recorded == 3);
	assert(health.total_drained == 0);
	assert(health.total_dropped == 0);
	assert(health.unclassified_inflows == 0);
	assert(health.unclassified_outflows == 0);

	std::vector<EconomicEvent> batch;
	size_t drained = economic_event_drain(&batch, 2);
	assert(drained == 2);
	assert(batch.size() == 2);

	assert(batch[0].event_id == 1);
	assert(batch[0].actor_pid == 101);
	assert(batch[0].action == EconomicAction::WALLET_ADD);
	assert(batch[0].copper_delta == 1500);
	assert(batch[0].target_id == 3001);
	assert(std::strcmp(batch[0].reason, "test_wallet_add") == 0);
	assert(batch[0].timestamp_usec > 0);

	assert(batch[1].event_id == 2);
	assert(batch[1].actor_pid == 101);
	assert(batch[1].action == EconomicAction::SHOP_BUY);
	assert(batch[1].copper_delta == -250);

	health = economic_event_outbox_health();
	assert(health.queue_depth == 1);
	assert(health.total_drained == 2);

	batch.clear();
	drained = economic_event_drain(&batch, 10);
	assert(drained == 1);
	assert(batch[0].event_id == 3);
	assert(batch[0].actor_pid == 202);
	assert(batch[0].copper_delta == 75);

	health = economic_event_outbox_health();
	assert(health.queue_depth == 0);
	assert(health.total_drained == 3);

	printf("test_basic_record_and_drain passed\n");
}

static void test_resilient_open_world_fallback()
{
	economic_event_outbox_reset_for_tests();

	// Unknown action with positive delta -> UNCLASSIFIED_INFLOW
	economic_event_record(303, EconomicAction::UNKNOWN, 50000, 100, 0, "mystery_reward");

	// Unknown action with negative delta -> UNCLASSIFIED_OUTFLOW
	economic_event_record(303, EconomicAction::UNKNOWN, -12000, 100, 0, "mystery_sink");

	// Explicit UNCLASSIFIED_INFLOW
	economic_event_record(404, EconomicAction::UNCLASSIFIED_INFLOW, 100, 0, 0, "unclassified_in");

	// Explicit UNCLASSIFIED_OUTFLOW
	economic_event_record(404, EconomicAction::UNCLASSIFIED_OUTFLOW, -50, 0, 0, "unclassified_out");

	EconomicOutboxHealth health = economic_event_outbox_health();
	assert(health.queue_depth == 4);
	assert(health.total_recorded == 4);
	assert(health.unclassified_inflows == 2);
	assert(health.unclassified_outflows == 2);

	std::vector<EconomicEvent> batch;
	size_t drained = economic_event_drain(&batch, 10);
	assert(drained == 4);

	assert(batch[0].action == EconomicAction::UNCLASSIFIED_INFLOW);
	assert(batch[0].copper_delta == 50000);

	assert(batch[1].action == EconomicAction::UNCLASSIFIED_OUTFLOW);
	assert(batch[1].copper_delta == -12000);

	assert(batch[2].action == EconomicAction::UNCLASSIFIED_INFLOW);
	assert(batch[3].action == EconomicAction::UNCLASSIFIED_OUTFLOW);

	printf("test_resilient_open_world_fallback passed\n");
}

static void test_outbox_flush_submits_to_writer()
{
	economic_event_outbox_reset_for_tests();
	submitted_jobs.clear();

	economic_event_record(505, EconomicAction::AUCTION_PAYOUT, 800000, 0, 999, "auction_payout");
	economic_event_record(505, EconomicAction::BANK_DEPOSIT, -800000, 0, 0, "bank_deposit");

	economic_event_flush_outbox();

	assert(submitted_jobs.size() == 1);
	assert(last_submitted_kind == persistence_job_kind::economy);
	assert(last_submitted_owner > 0);
	assert(last_submitted_bytes == 2 * sizeof(EconomicEvent));

	// Execute the submitted persistence closure
	player_save_apply_result res = submitted_jobs[0]();
	assert(res.outcome == player_save_apply_outcome::applied);

	EconomicOutboxHealth health = economic_event_outbox_health();
	assert(health.queue_depth == 0);
	assert(health.total_drained == 2);

	printf("test_outbox_flush_submits_to_writer passed\n");
}

static void test_ring_buffer_capacity_and_drop()
{
	economic_event_outbox_reset_for_tests();

	// Fill to capacity (16384)
	for (size_t i = 0; i < 16384; ++i)
	{
		economic_event_record(1, EconomicAction::WALLET_ADD, 1, 0, 0, "fill");
	}

	EconomicOutboxHealth health = economic_event_outbox_health();
	assert(health.queue_depth == 16384);
	assert(health.total_recorded == 16384);
	assert(health.total_dropped == 0);

	// Add 5 more events, which must drop the 5 oldest without blocking or failing
	for (size_t i = 0; i < 5; ++i)
	{
		economic_event_record(2, EconomicAction::WALLET_SUB, -1, 0, 0, "overflow");
	}

	health = economic_event_outbox_health();
	assert(health.queue_depth == 16384);
	assert(health.total_recorded == 16389);
	assert(health.total_dropped == 5);

	std::vector<EconomicEvent> batch;
	economic_event_drain(&batch, 1);
	assert(batch.size() == 1);
	// First 5 events (IDs 1..5) were dropped, so oldest remaining is event_id == 6
	assert(batch[0].event_id == 6);

	printf("test_ring_buffer_capacity_and_drop passed\n");
}

int main()
{
	test_basic_record_and_drain();
	test_resilient_open_world_fallback();
	test_outbox_flush_submits_to_writer();
	test_ring_buffer_capacity_and_drop();
	printf("ALL ECONOMIC EVENT OUTBOX TESTS PASSED\n");
	return 0;
}
