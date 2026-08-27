#!/usr/bin/env python3
"""Deterministic absolute-due scheduler behavior under ASan/UBSan."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#define clock_gettime nevent_test_clock_gettime
#include "new_events.c"
#undef clock_gettime

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

int pulse = 0;
bool after_events_call = FALSE;
P_char character_list = nullptr;
P_index mob_index = nullptr;
P_index obj_index = nullptr;
P_room world = nullptr;
extern const int top_of_world = 0;

static mm_ds test_pool = {};
static std::vector<std::pair<int, unsigned long long>> fired;
static std::vector<int> fired_this_tick;
static bool oracle_enabled = false;
static int failure_code = 0;
static unsigned long long fake_clock_ns = 0;

struct oracle_record
{
	unsigned long long due_tick;
	int id;
};

struct oracle_later
{
	bool operator()(const oracle_record &left, const oracle_record &right) const
	{
		if (left.due_tick != right.due_tick)
			return left.due_tick > right.due_tick;
		return left.id > right.id;
	}
};

static std::priority_queue<oracle_record, std::vector<oracle_record>, oracle_later> oracle;

static void require(bool condition, int code)
{
	if (!condition)
	{
		failure_code = code;
		std::fprintf(stderr, "scheduler harness failure %d at tick %llu\n", code,
			     ne_event_tick);
		std::exit(code);
	}
}

extern "C" int nevent_test_clock_gettime(clockid_t, struct timespec *value) noexcept
{
	value->tv_sec = static_cast<time_t>(fake_clock_ns / 1000000000ULL);
	value->tv_nsec = static_cast<long>(fake_clock_ns % 1000000000ULL);
	fake_clock_ns += 1000;
	return 0;
}

void debug(const char *, ...)
{
}

void logit(const char *, const char *, ...)
{
}

void statuslog(int, const char *, ...)
{
}

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

void *__malloc(size_t size, const char *, const char *, int)
{
	return std::malloc(size);
}

void __free(void *memory, const char *, int)
{
	std::free(memory);
}

void *_mm_get(mm_ds *pool, const char *, int)
{
	pool->objs_used++;
	return std::calloc(1, sizeof(nevent_data));
}

void mm_release(mm_ds *pool, void *memory)
{
	require(pool->objs_used > 0, 10);
	pool->objs_used--;
	std::free(memory);
}

char_link_data *link_char(P_char, P_char, ush_int)
{
	return nullptr;
}

P_char get_linked_char(P_char, ush_int)
{
	return nullptr;
}

void remove_link(P_char, char_link_data *)
{
}

const char *event_name_registry_lookup(const void *)
{
	return nullptr;
}

void release_mob_mem(P_char, P_char, P_obj, void *)
{
}

#define DEFINE_LABEL_CALLBACK(name) void name(P_char, P_char, P_obj, void *) {}
DEFINE_LABEL_CALLBACK(event_hit_regen)
DEFINE_LABEL_CALLBACK(event_mana_regen)
DEFINE_LABEL_CALLBACK(event_move_regen)
DEFINE_LABEL_CALLBACK(event_ward_regen)
DEFINE_LABEL_CALLBACK(event_spellcast)
DEFINE_LABEL_CALLBACK(event_memorize)
DEFINE_LABEL_CALLBACK(event_wait)
DEFINE_LABEL_CALLBACK(event_balance_affects)
DEFINE_LABEL_CALLBACK(event_mob_mundane)
DEFINE_LABEL_CALLBACK(event_reset_zone)
DEFINE_LABEL_CALLBACK(event_autosave)
#undef DEFINE_LABEL_CALLBACK

struct record_payload
{
	int id;
	unsigned long long expected_tick;
};

struct child_payload
{
	int id;
	int delay;
};

static void record_callback(P_char, P_char, P_obj, void *data)
{
	auto *record = static_cast<record_payload *>(data);
	require(record != nullptr, 20);
	require(ne_event_tick == record->expected_tick, 21);
	fired.emplace_back(record->id, ne_event_tick);
	fired_this_tick.push_back(record->id);
}

static void add_record(int id, int delay, unsigned long long expected_tick)
{
	record_payload record = { id, expected_tick };
	add_event(record_callback, delay, nullptr, nullptr, nullptr, 0, &record, sizeof(record));
	if (oracle_enabled)
		oracle.push({ expected_tick, id });
}

static void child_callback(P_char, P_char, P_obj, void *data)
{
	auto *child = static_cast<child_payload *>(data);
	require(child != nullptr, 22);
	const unsigned long long expected_tick =
		ne_event_tick + static_cast<unsigned long long>(std::max(child->delay, 1));
	add_record(child->id, child->delay, expected_tick);
}

static void noop_callback(P_char, P_char, P_obj, void *)
{
}

static long wheel_count()
{
	long count = 0;
	for (int bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
	{
		P_nevent previous = nullptr;
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			require(event->prev_sched == previous, 30);
			require(event->element == static_cast<unsigned int>(bucket), 31);
			require(event->lifecycle_state == NEVENT_LIFECYCLE_ACTIVE ||
					event->lifecycle_state == NEVENT_LIFECYCLE_CANCEL_PENDING,
				32);
			previous = event;
			count++;
		}
		require(ne_schedule_tail[bucket] == previous, 33);
	}
	return count;
}

static void require_balanced(int code)
{
	const long count = wheel_count();
	require(count == ne_event_counter, code);
	require(count == static_cast<long>(test_pool.objs_used), code + 1);
	require(pulse == static_cast<int>(ne_event_tick % PULSES_IN_TICK), code + 2);
}

static void reset_scheduler()
{
	require(test_pool.objs_used == 0, 40);
	std::memset(ne_schedule, 0, sizeof(ne_schedule));
	std::memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));
	std::memset(&test_pool, 0, sizeof(test_pool));
	std::memset(&nevent_analytics, 0, sizeof(nevent_analytics));
	current_nevent = nullptr;
	ne_dead_event_pool = &test_pool;
	ne_event_counter = 0;
	ne_event_tick = 0;
	ne_event_sequence = 0;
	nevent_catchup_debt = 0;
	nevent_catchup_remaining = 0;
	nevent_catchup_quota = 0;
	nevent_catchup_extension_us = 0;
	nevent_catchup_extra_callbacks = 0;
	nevent_pending_cancellations.clear();
	pulse = 0;
	after_events_call = FALSE;
	fake_clock_ns = 0;
	fired.clear();
	fired_this_tick.clear();
	oracle = {};
	oracle_enabled = false;
}

static void run_one_heartbeat()
{
	fired_this_tick.clear();
	ne_events();
	require_balanced(50);
	nevent_advance_tick();
	require_balanced(53);
}

enum class schedule_phase
{
	before,
	during,
	after
};

static void test_boundary_delay(schedule_phase phase, int delay, int case_id)
{
	reset_scheduler();
	const unsigned long long expected_tick =
		phase == schedule_phase::before ? static_cast<unsigned long long>(delay) :
					       static_cast<unsigned long long>(std::max(delay, 1));

	if (phase == schedule_phase::before)
	{
		add_record(case_id, delay, expected_tick);
	}
	else if (phase == schedule_phase::during)
	{
		child_payload child = { case_id, delay };
		add_event(child_callback, 0, nullptr, nullptr, nullptr, 0, &child, sizeof(child));
		fired_this_tick.clear();
		ne_events();
		require(fired.empty(), 60);
		P_nevent event = get_scheduled(record_callback);
		require(event && event->due_tick == expected_tick, 61);
		require(ne_event_time(event) == static_cast<int>(expected_tick), 62);
		require_balanced(63);
		nevent_advance_tick();
	}
	else
	{
		fired_this_tick.clear();
		ne_events();
		add_record(case_id, delay, expected_tick);
		P_nevent event = get_scheduled(record_callback);
		require(event && event->due_tick == expected_tick, 66);
		require(ne_event_time(event) == static_cast<int>(expected_tick), 67);
		require_balanced(68);
		nevent_advance_tick();
	}

	if (phase == schedule_phase::before)
	{
		P_nevent event = get_scheduled(record_callback);
		require(event && event->due_tick == expected_tick, 70);
		require(ne_event_time(event) == delay, 71);
	}

	while (fired.empty())
	{
		require(ne_event_tick <= expected_tick, 72);
		run_one_heartbeat();
	}
	require(fired.size() == 1 && fired[0].first == case_id &&
			fired[0].second == expected_tick,
		73);
	require_balanced(74);
}

static void test_boundary_matrix()
{
	const int delays[] = { 0, 1, 299, 300, 301, 599, 600, 601 };
	int case_id = 1000;
	for (schedule_phase phase :
	     { schedule_phase::before, schedule_phase::during, schedule_phase::after })
		for (int delay : delays)
			test_boundary_delay(phase, delay, case_id++);
}

static void cancel_all_events()
{
	for (int bucket = 0; bucket < PULSES_IN_TICK; ++bucket)
		while (ne_schedule[bucket])
			require(nevent_cancel(nevent_handle_from_event(ne_schedule[bucket])) ==
					nevent_cancel_result::canceled,
				80);
}

static void test_current_bucket_positions()
{
	for (int target_position = 0; target_position < 3; ++target_position)
	{
		reset_scheduler();
		for (int position = 0; position < 3; ++position)
		{
			if (position == target_position)
				add_record(2000 + position, 0, 0);
			else
				add_record(2100 + position, position == 0 ? 300 : 600,
					   position == 0 ? 300 : 600);
		}
		P_nevent cursor = ne_schedule[0];
		for (int position = 0; position < target_position; ++position)
			cursor = cursor->next_sched;
		require(cursor && static_cast<record_payload *>(cursor->data)->id ==
					 2000 + target_position,
			81);
		run_one_heartbeat();
		require(fired.size() == 1 && fired[0].second == 0, 82);
		cancel_all_events();
		require_balanced(83);
	}

	for (int parent_position = 0; parent_position < 3; ++parent_position)
	{
		reset_scheduler();
		for (int position = 0; position < 3; ++position)
		{
			if (position == parent_position)
			{
				child_payload child = { 2200 + position, 300 };
				add_event(child_callback, 0, nullptr, nullptr, nullptr, 0, &child,
					  sizeof(child));
			}
			else
				add_event(noop_callback, 0, nullptr, nullptr, nullptr, 0, nullptr, 0);
		}
		run_one_heartbeat();
		require(fired.empty(), 84);
		P_nevent child = get_scheduled(record_callback);
		require(child && child->due_tick == 300, 85);
		while (ne_event_counter > 0)
			run_one_heartbeat();
		require(fired.size() == 1 && fired[0].second == 300, 86);
	}
}

static void test_shared_bucket_revolutions()
{
	reset_scheduler();
	add_record(3000, 0, 0);
	add_record(3001, 300, 300);
	add_record(3002, 600, 600);
	require(ne_schedule[0] && ne_schedule[0]->next_sched &&
			ne_schedule[0]->next_sched->next_sched,
		90);
	while (ne_event_counter > 0)
		run_one_heartbeat();
	const std::vector<std::pair<int, unsigned long long>> expected = {
		{ 3000, 0 }, { 3001, 300 }, { 3002, 600 }
	};
	require(fired == expected, 91);
}

static void test_reschedule_apis()
{
	reset_scheduler();
	add_record(4000, 600, 5);
	P_nevent event = get_scheduled(record_callback);
	require(event && event->due_tick == 600, 100);
	const nevent_handle handle = nevent_handle_from_event(event);
	require(nevent_reschedule_after(handle, 300) && event->due_tick == 300, 101);
	require(nevent_advance_by(handle, 100) && event->due_tick == 200, 102);
	require(nevent_reschedule_at(handle, 5) && event->due_tick == 5, 103);
	require(ne_event_time(event) == 5, 104);
	while (ne_event_counter > 0)
		run_one_heartbeat();
	require(fired.size() == 1 && fired[0].second == 5, 105);

	reset_scheduler();
	add_record(4001, 300, 1);
	event = get_scheduled(record_callback);
	fired_this_tick.clear();
	ne_events();
	require(nevent_reschedule_at(nevent_handle_from_event(event), 0), 106);
	require(event->due_tick == 1 && event->element == 1, 107);
	nevent_advance_tick();
	run_one_heartbeat();
	require(fired.size() == 1 && fired[0].second == 1, 108);
}

static unsigned int random_state = 0x6d2b79f5U;

static unsigned int next_random()
{
	random_state = random_state * 1664525U + 1013904223U;
	return random_state;
}

static int random_delay()
{
	static const int boundaries[] = { 0, 1, 299, 300, 301, 599, 600, 601 };
	if ((next_random() & 3U) == 0)
		return boundaries[next_random() % (sizeof(boundaries) / sizeof(boundaries[0]))];
	return static_cast<int>(next_random() % 901U);
}

static void validate_oracle_tick()
{
	std::vector<int> expected_ids;
	while (!oracle.empty() && oracle.top().due_tick <= ne_event_tick)
	{
		require(oracle.top().due_tick == ne_event_tick, 120);
		expected_ids.push_back(oracle.top().id);
		oracle.pop();
	}
	std::sort(expected_ids.begin(), expected_ids.end());
	std::sort(fired_this_tick.begin(), fired_this_tick.end());
	require(expected_ids == fired_this_tick, 121);
}

static void test_randomized_oracle()
{
	reset_scheduler();
	oracle_enabled = true;
	random_state = 0x6d2b79f5U;
	int next_id = 5000;
	const int generation_ticks = 1200;

	for (int generated_tick = 0; generated_tick < generation_ticks; ++generated_tick)
	{
		const int before_count = static_cast<int>(next_random() % 4U);
		for (int index = 0; index < before_count; ++index)
		{
			const int delay = random_delay();
			add_record(next_id++, delay, ne_event_tick + delay);
		}
		if ((next_random() % 5U) == 0)
		{
			child_payload child = { next_id++, random_delay() };
			add_event(child_callback, 0, nullptr, nullptr, nullptr, 0, &child,
				  sizeof(child));
		}

		fired_this_tick.clear();
		ne_events();
		validate_oracle_tick();

		const int after_count = static_cast<int>(next_random() % 3U);
		for (int index = 0; index < after_count; ++index)
		{
			const int delay = random_delay();
			add_record(next_id++, delay,
				   ne_event_tick + static_cast<unsigned long long>(std::max(delay, 1)));
		}
		require_balanced(122);
		nevent_advance_tick();
	}

	int drain_ticks = 0;
	while (ne_event_counter > 0)
	{
		require(drain_ticks++ < 1000, 125);
		fired_this_tick.clear();
		ne_events();
		validate_oracle_tick();
		require_balanced(126);
		nevent_advance_tick();
	}
	require(oracle.empty(), 129);
	require_balanced(130);
}

int main()
{
	setenv("DURIS_NEVENT_BUDGET_USEC", "0", 1);
	setenv("DURIS_NEVENT_MAX_CALLBACKS", "0", 1);
	setenv("DURIS_NEVENT_ANALYTICS", "0", 1);
	setenv("DURIS_NEVENT_PLAYER_PRIORITY", "0", 1);
	test_boundary_matrix();
	test_current_bucket_positions();
	test_shared_bucket_revolutions();
	test_reschedule_apis();
	test_randomized_oracle();
	std::puts("nevent absolute-due scheduler runtime matrix passed");
	return failure_code;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-nevent-scheduler-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-ffunction-sections",
            "-fdata-sections",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{SRC}",
            str(harness),
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        check=True,
    )
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary)], check=True, env=environment)

source = (SRC / "new_events.c").read_text(encoding="ascii")
comm = (SRC / "comm.c").read_text(encoding="ascii")
assert "--(current_nevent->timer)" not in source
assert "event->timer" not in source
assert "current_nevent->due_tick > ne_event_tick" in source
assert "pass_sequence = ne_event_sequence" in source
assert "nevent_advance_tick();" in comm

print("nevent scheduler runtime test passed under ASan/UBSan")
