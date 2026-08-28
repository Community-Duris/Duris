#!/usr/bin/env python3
"""Periodic nevent registry runtime and migration contracts."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "prototypes.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

unsigned long long ne_event_tick = 0;

static unsigned long long next_sequence = 0;
static std::vector<P_nevent> events;
static int callback_runs = 0;
static int callback_mode = 0;

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

bool nevent_require_game_thread(const char *)
{
	return true;
}

bool nevent_handle_is_active(nevent_handle handle)
{
	return handle.event && handle.sequence && handle.event->sequence == handle.sequence &&
	       handle.event->lifecycle_state == 1;
}

nevent_schedule_result add_event(event_func callback, int delay, P_char, P_char, P_obj, int,
				 const void *, int)
{
	P_nevent event = new nevent_data{};
	event->func = callback;
	event->due_tick = ne_event_tick + static_cast<unsigned long long>(delay);
	event->sequence = ++next_sequence;
	event->lifecycle_state = 1;
	events.push_back(event);
	return { nevent_schedule_status::scheduled, { event, event->sequence } };
}

nevent_cancel_result nevent_cancel(nevent_handle handle)
{
	if (!nevent_handle_is_active(handle))
		return nevent_cancel_result::stale_handle;
	handle.event->lifecycle_state = 4;
	return nevent_cancel_result::canceled;
}

void logit(const char *, const char *, ...)
{
}

void panic_corruption(const char *, const char *, ...)
{
	std::exit(200);
}

static void periodic_callback(P_char, P_char, P_obj, void *)
{
	callback_runs++;
	if (callback_mode == 1 && callback_runs == 1)
		nevent_periodic_retry_after(3, "transient database failure");
	else if (callback_mode == 1 && callback_runs == 2)
		nevent_periodic_next_after(7);
	else if (callback_mode == 2 && callback_runs == 1)
		nevent_periodic_continue_after(1);
}

static void second_callback(P_char, P_char, P_obj, void *)
{
}

static P_nevent only_active_event()
{
	P_nevent found = nullptr;
	for (P_nevent event : events)
		if (event->lifecycle_state == 1)
		{
			require(found == nullptr, 201);
			found = event;
		}
	require(found != nullptr, 202);
	return found;
}

static size_t active_event_count()
{
	size_t count = 0;
	for (P_nevent event : events)
		if (event->lifecycle_state == 1)
			count++;
	return count;
}

static void execute(P_nevent event, unsigned long long tick)
{
	require(event && event->lifecycle_state == 1, 203);
	ne_event_tick = tick;
	const bool periodic = nevent_periodic_begin(event);
	require(periodic, 204);
	event->func(nullptr, nullptr, nullptr, nullptr);
	nevent_periodic_complete(event);
	event->lifecycle_state = 4;
}

static nevent_periodic_health health()
{
	nevent_periodic_health item = {};
	require(nevent_periodic_copy_health(&item, 1) == 1, 205);
	return item;
}

static void reset()
{
	for (P_nevent event : events)
		delete event;
	events.clear();
	ne_event_tick = 0;
	callback_runs = 0;
	callback_mode = 0;
	nevent_periodic_reset();
}

static void test_unique_fixed_delay_job()
{
	reset();
	require(nevent_periodic_register("checkpoint", periodic_callback, 5, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::registered,
		1);
	require(active_event_count() == 1, 2);
	require(nevent_periodic_register("checkpoint", periodic_callback, 5, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::duplicate_suppressed,
		3);
	require(active_event_count() == 1, 4);
	require(nevent_periodic_register("checkpoint", periodic_callback, 5, 11,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::invalid_definition,
		5);
	require(nevent_periodic_register("alias", periodic_callback, 5, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::invalid_definition,
		6);

	P_nevent first = only_active_event();
	require(first->periodic_job_id == 1 && first->due_tick == 5, 7);
	require(nevent_periodic_event_is_valid(first), 8);
	execute(first, 5);
	require(active_event_count() == 1, 9);
	nevent_periodic_health item = health();
	require(item.armed && item.next_due_tick == 15 && item.total_runs == 1, 10);
	require(item.has_succeeded && item.last_success_tick == 5, 11);
	require(item.duplicates_suppressed == 1 && item.callback_failures == 0, 12);
	require(nevent_periodic_integrity_errors(false) == 0, 13);
}

static void test_retry_and_success_override()
{
	reset();
	callback_mode = 1;
	require(nevent_periodic_register("artifact", periodic_callback, 2, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::registered,
		20);
	execute(only_active_event(), 2);
	nevent_periodic_health item = health();
	require(item.next_due_tick == 5 && item.callback_failures == 1 &&
			item.consecutive_failures == 1,
		21);
	require(std::strcmp(item.last_failure, "transient database failure") == 0, 22);
	execute(only_active_event(), 5);
	item = health();
	require(item.next_due_tick == 12 && item.total_runs == 2, 23);
	require(item.has_succeeded && item.last_success_tick == 5 &&
			item.consecutive_failures == 0,
		24);
}

static void test_fixed_rate_and_missed_runs()
{
	reset();
	require(nevent_periodic_register("clock", periodic_callback, 5, 10,
					 nevent_periodic_policy::fixed_rate, true) ==
			nevent_periodic_result::registered,
		30);
	execute(only_active_event(), 9);
	require(health().next_due_tick == 15, 31);
	execute(only_active_event(), 36);
	const nevent_periodic_health item = health();
	require(item.next_due_tick == 45 && item.missed_runs == 2, 32);
}

static void test_continuation_is_not_a_completed_run()
{
	reset();
	callback_mode = 2;
	require(nevent_periodic_register("sliced", periodic_callback, 2, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::registered,
		35);
	execute(only_active_event(), 2);
	nevent_periodic_health item = health();
	require(item.next_due_tick == 3 && item.total_runs == 1 && item.completed_runs == 0 &&
			item.continuation_slices == 1 && !item.has_succeeded,
		36);
	execute(only_active_event(), 3);
	item = health();
	require(item.next_due_tick == 13 && item.total_runs == 2 && item.completed_runs == 1 &&
			item.continuation_slices == 1 && item.last_success_tick == 3,
		37);
}

static void test_watchdog_rearms_missing_successor()
{
	reset();
	require(nevent_periodic_register("watchdog", periodic_callback, 20, 10,
					 nevent_periodic_policy::fixed_delay, true) ==
			nevent_periodic_result::registered,
		40);
	P_nevent missing = only_active_event();
	missing->lifecycle_state = 4;
	ne_event_tick = 3;
	nevent_periodic_watchdog();
	require(active_event_count() == 1 && only_active_event()->due_tick == 20, 41);
	require(health().watchdog_rearms == 1, 42);
	nevent_periodic_watchdog();
	require(active_event_count() == 1 && health().watchdog_rearms == 1, 43);

	missing = only_active_event();
	missing->lifecycle_state = 4;
	ne_event_tick = 25;
	nevent_periodic_watchdog();
	const nevent_periodic_health item = health();
	require(active_event_count() == 1 && only_active_event()->due_tick == 26, 44);
	require(item.watchdog_rearms == 2 && item.missed_runs == 1, 45);
}

static void test_disabled_job_enablement()
{
	reset();
	require(nevent_periodic_register("conditional", second_callback, 5, 10,
					 nevent_periodic_policy::fixed_delay, false) ==
			nevent_periodic_result::registered,
		50);
	require(active_event_count() == 0 && !health().enabled, 51);
	require(nevent_periodic_set_enabled("conditional", true, 3) ==
			nevent_periodic_result::enabled,
		52);
	require(only_active_event()->due_tick == 3, 53);
	require(nevent_periodic_set_enabled("conditional", true, 3) ==
			nevent_periodic_result::duplicate_suppressed,
		54);
	require(nevent_periodic_set_enabled("conditional", false, 3) ==
			nevent_periodic_result::disabled,
		55);
	nevent_periodic_watchdog();
	const nevent_periodic_summary summary = nevent_periodic_summary_copy();
	require(active_event_count() == 0 && summary.registered == 1 && summary.enabled == 0 &&
			summary.unhealthy == 0,
		56);
}

int main()
{
	test_unique_fixed_delay_job();
	test_retry_and_success_override();
	test_fixed_rate_and_missed_runs();
	test_continuation_is_not_a_completed_run();
	test_watchdog_rearms_missing_successor();
	test_disabled_job_enablement();
	reset();
	return 0;
}
'''


def function_body(source: str, signature: str) -> str:
    start = source.rindex(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


with tempfile.TemporaryDirectory(prefix="duris-nevent-periodic-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            f"-I{SRC}",
            str(harness),
            str(SRC / "nevent_periodic.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary)], check=True, env=environment)

events = (SRC / "new_events.c").read_text(encoding="utf-8")
periodic = (SRC / "nevent_periodic.c").read_text(encoding="utf-8")
redis = (SRC / "redis.c").read_text(encoding="utf-8")
artifact = (SRC / "artifact.c").read_text(encoding="utf-8")
weather = (SRC / "weather.c").read_text(encoding="utf-8")
handler = (SRC / "handler.c").read_text(encoding="utf-8")
outposts = (SRC / "outposts.c").read_text(encoding="utf-8")
drannak = (SRC / "drannak.c").read_text(encoding="utf-8")
comm = (SRC / "comm.c").read_text(encoding="utf-8")
makefile = (SRC / "Makefile").read_text(encoding="utf-8")
donation = (SRC / "redis_donation_runtime.c").read_text(encoding="utf-8")
checkpoint = (SRC / "persistence_checkpoint.c").read_text(encoding="utf-8")

for key in (
    "game-clock",
    "astral-clock",
    "generic-character-sweep",
    "artifact-bind",
    "artifact-wars",
    "artifact-expiry",
    "outpost-upkeep",
    "surname-update",
    "dirty-player-checkpoint",
    "donation-message-poll",
    "world-state-save",
):
    assert f'"{key}"' in events

for source, signature, callback in (
    (weather, "void event_another_hour", "event_another_hour"),
    (weather, "void event_astral_clock", "event_astral_clock"),
    (handler, "void generic_char_event", "generic_char_event"),
    (outposts, "void event_outposts_upkeep", "event_outposts_upkeep"),
    (drannak, "void event_update_surnames", "event_update_surnames"),
    (checkpoint, "void event_flush_dirty_players", "event_flush_dirty_players"),
    (redis, "void event_save_world_state", "event_save_world_state"),
    (donation, "void event_check_donation_messages", "event_check_donation_messages"),
    (artifact, "void event_artifact_check_poof_sql", "event_artifact_check_poof_sql"),
    (artifact, "void event_artifact_wars_sql", "event_artifact_wars_sql"),
    (artifact, "void event_artifact_check_bind_sql", "event_artifact_check_bind_sql"),
):
    assert f"add_event({callback}" not in function_body(source, signature)

assert "nevent_periodic_retry_after(ARTIFACT_MAINTENANCE_RETRY_DELAY" in artifact
assert "nevent_periodic_next_after(number(300, 600) * WAIT_SEC);" in drannak
assert "nevent_periodic_next_after(world_state_interval * WAIT_SEC);" in redis
assert 'nevent_periodic_set_enabled("world-state-save", true' in comm
assert "nevent_periodic_begin(current_nevent)" in events
assert "nevent_periodic_complete(current_nevent)" in events
assert "nevent_periodic_watchdog();" in events
assert "world events" not in periodic.lower()
assert "nevent_periodic.o" in makefile

print("periodic nevent registry passed under ASan/UBSan")
