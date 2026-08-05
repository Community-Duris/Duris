#include "test_persistence.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <chrono>
#include <unistd.h>

#include "persistence_queue.h"

void wizlog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}

namespace
{
struct capture_state
{
	std::vector<std::string> lines;
	int fail_first_n = 0;
	int call_count = 0;
	int sleep_us = 0;
};

static int capture_writer(const char *line, void *context)
{
	auto *state = static_cast<capture_state *>(context);
	state->call_count++;
	if (state->fail_first_n > 0)
	{
		state->fail_first_n--;
		return 0;
	}

	state->lines.emplace_back(line ? line : "");
	if (state->sleep_us > 0)
		usleep(state->sleep_us);
	return 1;
}

struct identity_race_state
{
	int calls = 0;
};

static int identity_race_writer(const char *line, void *context)
{
	auto *state = static_cast<identity_race_state *>(context);
	state->calls++;
	if (state->calls == 1)
	{
		char removed[PERSISTENCE_EVENT_MAX_LEN];
		if (!persistence_scalar_event_queue_dequeue(removed, sizeof(removed)))
			return 0;
		if (!persistence_scalar_event_queue_enqueue(line))
			return 0;
	}
	return 1;
}

static bool wait_for_scalar_queue_empty(int timeout_ms)
{
	for (int i = 0; i < timeout_ms; ++i)
	{
		if (persistence_scalar_event_queue_pending() == 0)
			return true;
		usleep(1000);
	}
	return persistence_scalar_event_queue_pending() == 0;
}

static bool wait_for_item_queue_empty(int timeout_ms)
{
	for (int i = 0; i < timeout_ms; ++i)
	{
		if (persistence_item_event_queue_pending() == 0)
			return true;
		usleep(1000);
	}
	return persistence_item_event_queue_pending() == 0;
}

static bool wait_for_large_queue_empty(int timeout_ms)
{
	for (int i = 0; i < timeout_ms; ++i)
	{
		if (persistence_large_event_queue_pending() == 0)
			return true;
		usleep(1000);
	}
	return persistence_large_event_queue_pending() == 0;
}

static bool expect(bool cond, const char *message)
{
	if (!cond)
		fprintf(stderr, "[persistence-test] %s\n", message);
	return cond;
}

template <typename StartFn, typename StopFn, typename RunningFn, typename StuckFn,
          typename HeartbeatSetFn, typename EnqueueFn, typename PendingFn,
          typename ResetFn, typename WaitEmptyFn>
static bool test_worker_slow_write_not_stuck_case(const char *worker_name,
                                                  const char *event_name,
                                                  StartFn start_fn,
                                                  StopFn stop_fn,
                                                  RunningFn running_fn,
                                                  StuckFn stuck_fn,
                                                  HeartbeatSetFn heartbeat_set_fn,
                                                  EnqueueFn enqueue_fn,
                                                  PendingFn pending_fn,
                                                  ResetFn reset_fn,
                                                  WaitEmptyFn wait_empty_fn)
{
	capture_state state;
	state.sleep_us = 200000;

	stop_fn(0);
	reset_fn();

	if (!expect(start_fn(capture_writer, &state), "failed to start worker for slow-write stale-heartbeat test"))
		return false;
	if (!expect(enqueue_fn(event_name), "enqueue should succeed for slow-write stale-heartbeat test"))
	{
		stop_fn(0);
		return false;
	}

	usleep(50000);
	heartbeat_set_fn(time(NULL) - (PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS + 10));
	if (!expect(!stuck_fn(PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS),
	            "slow in-write worker should not be flagged stuck by stale heartbeat"))
	{
		stop_fn(0);
		return false;
	}

	stop_fn(1);
	return expect(state.lines.size() == 1, "slow-write test should persist exactly one event") &&
	       expect(state.lines[0] == event_name, "slow-write test should round-trip the queued event") &&
	       expect(pending_fn() == 0, "slow-write test should leave no queued events") &&
	       expect(!running_fn(), "worker should not be running after slow-write stop");
}

template <typename StartFn, typename StopFn, typename RunningFn, typename HeartbeatSetFn,
          typename EnqueueFn, typename PendingFn, typename ResetFn, typename WaitEmptyFn>
static bool test_worker_bounded_stop_timeout_case(const char *worker_name,
                                                  const char *event_name,
                                                  StartFn start_fn,
                                                  StopFn stop_fn,
                                                  RunningFn running_fn,
                                                  HeartbeatSetFn heartbeat_set_fn,
                                                  EnqueueFn enqueue_fn,
                                                  PendingFn pending_fn,
                                                  ResetFn reset_fn,
                                                  WaitEmptyFn wait_empty_fn)
{
	capture_state state;
	state.sleep_us = 3000000;

	stop_fn(0);
	reset_fn();

	if (!expect(start_fn(capture_writer, &state), "failed to start worker for bounded-stop test"))
		return false;
	if (!expect(enqueue_fn(event_name), "enqueue should succeed for bounded-stop test"))
	{
		stop_fn(0);
		return false;
	}

	usleep(50000);
	heartbeat_set_fn(time(NULL) - (PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS + 10));
	auto start = std::chrono::steady_clock::now();
	stop_fn(0);
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	if (!expect(wait_empty_fn(8000), "bounded-stop test queue did not drain"))
		return false;

	return expect(elapsed_ms >= 1500 && elapsed_ms < 6000,
	              "bounded-stop test should return before the writer finishes but not hang forever") &&
	       expect(state.lines.size() == 1, "bounded-stop test should persist exactly one event") &&
	       expect(state.lines[0] == event_name, "bounded-stop test should round-trip the queued event") &&
	       expect(pending_fn() == 0, "bounded-stop test should leave no queued events") &&
	       expect(!running_fn(), "worker should not be running after bounded stop");
}

static bool test_queue_flood_scalar_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();

	capture_state state;
	state.sleep_us = 200;
	if (!expect(persistence_scalar_event_worker_start(capture_writer, &state), "failed to start scalar worker"))
		return false;

	constexpr int kEvents = 5000;
	char line[64];
	for (int i = 0; i < kEvents; ++i)
	{
		snprintf(line, sizeof(line), "scalar-event-%d", i);
		if (!expect(persistence_scalar_event_queue_enqueue(line), "enqueue should succeed while worker is active"))
		{
			persistence_scalar_event_worker_stop(0);
			return false;
		}
	}

	if (!expect(wait_for_scalar_queue_empty(8000), "scalar queue did not drain in time"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	persistence_scalar_event_worker_stop(1);

	return expect(static_cast<int>(state.lines.size()) == kEvents, "worker did not persist every queued scalar event") &&
	       expect(persistence_scalar_event_queue_pending() == 0, "scalar queue should be empty after drain") &&
	       expect(persistence_scalar_event_queue_dropped() == 0, "scalar queue should not drop events in the flood test") &&
	       expect(!persistence_scalar_event_worker_running(), "scalar worker should not be running after stop") &&
	       expect(persistence_scalar_event_worker_write_failures() == 0, "scalar worker should not report write failures in flood test");
}

static bool test_worker_scalar_fallback_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();

	capture_state state;
	state.fail_first_n = 3;
	if (!expect(persistence_scalar_event_worker_start(capture_writer, &state), "failed to start scalar worker for retry test"))
		return false;

	if (!expect(persistence_scalar_event_queue_enqueue("retry-me"), "enqueue should succeed for retry test"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_scalar_queue_empty(8000), "scalar queue did not recover after transient writer failures"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	persistence_scalar_event_worker_stop(1);

	return expect(state.call_count >= 4, "writer should have been retried after transient failures") &&
	       expect(state.lines.size() == 1, "retry test should persist the scalar event exactly once") &&
	       expect(persistence_scalar_event_worker_write_failures() >= 3, "retry test should report the simulated write failures") &&
	       expect(persistence_scalar_event_worker_written() >= 1, "retry test should count the successful write") &&
	       expect(persistence_scalar_event_queue_pending() == 0, "retry test should leave no queued scalar events");
}

static bool test_worker_scalar_fifo_after_retry_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();

	capture_state state;
	state.fail_first_n = 2;
	if (!expect(persistence_scalar_event_worker_start(capture_writer, &state), "failed to start scalar worker for FIFO retry test"))
		return false;

	if (!expect(persistence_scalar_event_queue_enqueue("first"), "enqueue first scalar event failed"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}
	if (!expect(persistence_scalar_event_queue_enqueue("second"), "enqueue second scalar event failed"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_scalar_queue_empty(8000), "scalar queue did not drain in FIFO retry test"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	persistence_scalar_event_worker_stop(1);

	return expect(state.lines.size() == 2, "FIFO retry test should persist two scalar events") &&
	       expect(state.lines[0] == "first", "first scalar event should persist first") &&
	       expect(state.lines[1] == "second", "second scalar event should persist second") &&
	       expect(persistence_scalar_event_worker_write_failures() >= 2, "FIFO retry test should count transient failures") &&
	       expect(persistence_scalar_event_queue_pending() == 0, "FIFO retry test should leave no queued scalar events");
}

static bool test_worker_scalar_stale_heartbeat_shutdown_fallback_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();

	capture_state state;
	state.sleep_us = 200000;
	if (!expect(persistence_scalar_event_worker_start(capture_writer, &state), "failed to start scalar worker for stale-heartbeat stop test"))
		return false;

	if (!expect(persistence_scalar_event_queue_enqueue("stale-heartbeat-stop"), "enqueue should succeed for stale-heartbeat stop test"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	usleep(50000);
	persistence_scalar_event_worker_heartbeat_set(time(NULL) - (PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS + 10));

	if (!expect(persistence_scalar_event_worker_stuck(PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS), "stale-heartbeat helper should report a stuck worker before stop"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}

	auto start = std::chrono::steady_clock::now();
	persistence_scalar_event_worker_stop(0);
	auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	usleep(300000);

	return expect(elapsed_ms < 150, "stale-heartbeat stop should not block on a wedged worker") &&
	       expect(!persistence_scalar_event_worker_running(), "scalar worker should be marked stopped after stale-heartbeat fallback") &&
	       expect(state.lines.size() == 1, "stale-heartbeat stop should not lose the queued event") &&
	       expect(state.lines[0] == "stale-heartbeat-stop", "stale-heartbeat stop should persist the queued scalar event") &&
	       expect(persistence_scalar_event_queue_pending() == 0, "stale-heartbeat stop should leave the scalar queue empty");
}

static bool test_worker_item_fifo_impl()
{
	persistence_item_event_worker_stop(0);
	persistence_item_event_queue_reset();

	capture_state state;
	if (!expect(persistence_item_event_worker_start(capture_writer, &state), "failed to start item worker"))
		return false;

	if (!expect(persistence_item_event_queue_enqueue("item-one"), "enqueue first item event failed"))
	{
		persistence_item_event_worker_stop(0);
		return false;
	}
	if (!expect(persistence_item_event_queue_enqueue("item-two"), "enqueue second item event failed"))
	{
		persistence_item_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_item_queue_empty(8000), "item queue did not drain"))
	{
		persistence_item_event_worker_stop(0);
		return false;
	}

	persistence_item_event_worker_stop(1);

	return expect(state.lines.size() == 2, "item worker should persist both events") &&
	       expect(state.lines[0] == "item-one", "item queue should preserve FIFO order for first event") &&
	       expect(state.lines[1] == "item-two", "item queue should preserve FIFO order for second event") &&
	       expect(persistence_item_event_queue_pending() == 0, "item queue should be empty after drain");
}

static bool test_queue_rejects_oversize_scalar_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();
	persistence_large_event_worker_stop(0);
	persistence_large_event_queue_reset();

	capture_state state;
	if (!expect(persistence_large_event_worker_start(capture_writer, &state), "failed to start large worker for oversize scalar route test"))
		return false;

	std::string payload(PERSISTENCE_EVENT_MAX_LEN + 64, 'S');
	payload.replace(0, 32, "oversize-scalar-event");
	if (!expect(persistence_scalar_event_queue_enqueue(payload.c_str()), "oversize scalar payload should route to large queue"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_large_queue_empty(8000), "oversize scalar payload did not drain through large queue"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	persistence_large_event_worker_stop(1);

	return expect(state.lines.size() == 1, "oversize scalar payload should persist exactly once") &&
	       expect(state.lines[0] == payload, "oversize scalar payload should round-trip through large queue") &&
	       expect(persistence_scalar_event_queue_pending() == 0, "oversize scalar payload should not remain in scalar queue") &&
	       expect(persistence_large_event_queue_pending() == 0, "oversize scalar payload should not remain in large queue") &&
	       expect(persistence_scalar_event_queue_dropped() == 0, "oversize scalar payload should not be dropped") &&
	       expect(persistence_large_event_queue_dropped() == 0, "oversize scalar payload should not be dropped from large queue");
}

static bool test_queue_rejects_oversize_item_impl()
{
	persistence_item_event_worker_stop(0);
	persistence_item_event_queue_reset();
	persistence_large_event_worker_stop(0);
	persistence_large_event_queue_reset();

	capture_state state;
	if (!expect(persistence_large_event_worker_start(capture_writer, &state), "failed to start large worker for oversize item route test"))
		return false;

	std::string payload(PERSISTENCE_EVENT_MAX_LEN + 64, 'I');
	payload.replace(0, 30, "oversize-item-event");
	if (!expect(persistence_item_event_queue_enqueue(payload.c_str()), "oversize item payload should route to large queue"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_large_queue_empty(8000), "oversize item payload did not drain through large queue"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	persistence_large_event_worker_stop(1);

	return expect(state.lines.size() == 1, "oversize item payload should persist exactly once") &&
	       expect(state.lines[0] == payload, "oversize item payload should round-trip through large queue") &&
	       expect(persistence_item_event_queue_pending() == 0, "oversize item payload should not remain in item queue") &&
	       expect(persistence_large_event_queue_pending() == 0, "oversize item payload should not remain in large queue") &&
	       expect(persistence_item_event_queue_dropped() == 0, "oversize item payload should not be dropped") &&
	       expect(persistence_large_event_queue_dropped() == 0, "oversize item payload should not be dropped from large queue");
}

static bool test_worker_generation_identity_impl()
{
	persistence_scalar_event_worker_stop(0);
	persistence_scalar_event_queue_reset();

	identity_race_state state;
	if (!expect(persistence_scalar_event_worker_start(identity_race_writer, &state),
	            "failed to start scalar worker for generation identity test"))
		return false;
	if (!expect(persistence_scalar_event_queue_enqueue("identical-event"),
	            "failed to enqueue generation identity event"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}
	if (!expect(wait_for_scalar_queue_empty(8000),
	            "generation identity queue did not drain"))
	{
		persistence_scalar_event_worker_stop(0);
		return false;
	}
	persistence_scalar_event_worker_stop(1);

	return expect(state.calls == 2,
	              "replacement event with identical text must be written separately") &&
	       expect(persistence_scalar_event_queue_pending() == 0,
	              "generation identity test should leave no queued events");
}

static bool test_worker_large_roundtrip_impl()
{
	persistence_large_event_worker_stop(0);
	persistence_large_event_queue_reset();

	capture_state state;
	if (!expect(persistence_large_event_worker_start(capture_writer, &state), "failed to start large worker"))
		return false;

	std::string payload(4096, 'L');
	payload.replace(0, 18, "large-event-roundtrip");
	if (!expect(persistence_large_event_queue_enqueue(payload.c_str()), "enqueue large payload failed"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	if (!expect(wait_for_large_queue_empty(8000), "large queue did not drain"))
	{
		persistence_large_event_worker_stop(0);
		return false;
	}

	persistence_large_event_worker_stop(1);

	return expect(state.lines.size() == 1, "large worker should persist exactly one payload") &&
	       expect(state.lines[0] == payload, "large payload should round-trip byte-for-byte") &&
	       expect(persistence_large_event_queue_pending() == 0, "large queue should be empty after drain") &&
	       expect(persistence_large_event_queue_dropped() == 0, "large queue should not drop the round-trip payload") &&
	       expect(!persistence_large_event_worker_running(), "large worker should not be running after stop");
}

struct suite_case
{
	const char *name;
	bool (*fn)();
};

static const suite_case kCases[] =
{
	{"queue_flood_scalar", test_queue_flood_scalar_impl},
	{"queue_routes_oversize_scalar_to_large", test_queue_rejects_oversize_scalar_impl},
	{"queue_routes_oversize_item_to_large", test_queue_rejects_oversize_item_impl},
	{"worker_scalar_fallback", test_worker_scalar_fallback_impl},
	{"worker_scalar_fifo_after_retry", test_worker_scalar_fifo_after_retry_impl},
	{"worker_generation_identity", test_worker_generation_identity_impl},
	{"worker_scalar_stale_heartbeat_shutdown_fallback", test_worker_scalar_stale_heartbeat_shutdown_fallback_impl},
	{"worker_item_fifo", test_worker_item_fifo_impl},
	{"worker_large_roundtrip", test_worker_large_roundtrip_impl},
};

static int g_run_count = 0;
static int g_pass_count = 0;
static int g_fail_count = 0;

} // namespace

void test_persistence_reset(void)
{
	g_run_count = 0;
	g_pass_count = 0;
	g_fail_count = 0;
}

int test_persistence_run_one(const char *name)
{
	if (!name || !*name)
		return 0;

	for (const auto &test_case : kCases)
	{
		if (strcmp(test_case.name, name) == 0)
		{
			g_run_count++;
			const bool ok = test_case.fn();
			if (ok)
				g_pass_count++;
			else
				g_fail_count++;
			fprintf(stderr, "[persistence-test] %s: %s\n", test_case.name, ok ? "PASS" : "FAIL");
			return ok ? 1 : 0;
		}
	}

	fprintf(stderr, "[persistence-test] unknown test: %s\n", name);
	return 0;
}

void test_persistence_run_all(void)
{
	for (const auto &test_case : kCases)
		test_persistence_run_one(test_case.name);
}

void test_persistence_print_summary(void)
{
	fprintf(stderr,
	        "[persistence-test] summary: run=%d pass=%d fail=%d\n",
	        g_run_count,
	        g_pass_count,
	        g_fail_count);
}

#ifdef TEST_PERSISTENCE
int test_persistence_queue_flood_scalar(void)
{
	return test_queue_flood_scalar_impl() ? 1 : 0;
}

int test_persistence_worker_scalar_fallback(void)
{
	return test_worker_scalar_fallback_impl() ? 1 : 0;
}

int test_persistence_worker_scalar_fifo_after_retry(void)
{
	return test_worker_scalar_fifo_after_retry_impl() ? 1 : 0;
}

int test_persistence_worker_scalar_stale_heartbeat_shutdown_fallback(void)
{
	return test_worker_scalar_stale_heartbeat_shutdown_fallback_impl() ? 1 : 0;
}

int test_persistence_worker_item_fifo(void)
{
	return test_worker_item_fifo_impl() ? 1 : 0;
}

int test_persistence_worker_large_roundtrip(void)
{
	return test_worker_large_roundtrip_impl() ? 1 : 0;
}
#endif

#ifdef TEST_PERSISTENCE_STANDALONE
int main()
{
	test_persistence_reset();
	test_persistence_run_all();
	test_persistence_print_summary();
	return g_fail_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif
