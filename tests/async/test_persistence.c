#include "test_persistence.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
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

static bool wait_for_queue_empty(int timeout_ms)
{
	for (int i = 0; i < timeout_ms; ++i)
	{
		if (persistence_scalar_event_queue_pending() == 0)
			return true;
		usleep(1000);
	}
	return persistence_scalar_event_queue_pending() == 0;
}

static bool expect(bool cond, const char *message)
{
	if (!cond)
		fprintf(stderr, "[persistence-test] %s\n", message);
	return cond;
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

	if (!expect(wait_for_queue_empty(8000), "scalar queue did not drain in time"))
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

	if (!expect(wait_for_queue_empty(8000), "scalar queue did not recover after transient writer failures"))
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

struct suite_case
{
	const char *name;
	bool (*fn)();
};

static const suite_case kCases[] =
{
	{"queue_flood_scalar", test_queue_flood_scalar_impl},
	{"worker_scalar_fallback", test_worker_scalar_fallback_impl},
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
