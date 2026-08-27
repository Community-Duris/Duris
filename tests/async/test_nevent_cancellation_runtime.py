#!/usr/bin/env python3
"""Behavioral cancellation and scheduler-accounting invariants under ASan/UBSan."""

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "new_events.c"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static mm_ds test_pool = {};
static int release_calls = 0;
static int payload_frees = 0;
static int typed_payload_destroys = 0;
static int link_removals = 0;
P_index mob_index = nullptr;

void debug(const char *, ...)
{
}

void logit(const char *, const char *, ...)
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

void __free(void *memory, const char *, int)
{
	payload_frees++;
	std::free(memory);
}

void mm_release(mm_ds *pool, void *)
{
	release_calls++;
	pool->objs_used--;
}

void remove_link(P_char, char_link_data *link)
{
	link_removals++;
	event_broken(link);
}

static void callback(P_char, P_char, P_obj, void *)
{
}

struct typed_payload
{
	std::vector<int> path;
	~typed_payload()
	{
		typed_payload_destroys++;
	}
};

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static long wheel_count()
{
	long count = 0;
	for (int bucket = 0; bucket < PULSES_IN_TICK; bucket++)
	{
		P_nevent previous = nullptr;
		for (P_nevent event = ne_schedule[bucket]; event; event = event->next_sched)
		{
			require(event->prev_sched == previous, 100);
			require(event->element == static_cast<unsigned int>(bucket), 101);
			previous = event;
			count++;
		}
		require(ne_schedule_tail[bucket] == previous, 102);
	}
	return count;
}

static void require_balanced(long expected_events, long expected_debt, int code)
{
	long tracked_debt = 0;
	for (const auto &entry : nevent_deferred_due_counts)
		tracked_debt += entry.second;
	require(wheel_count() == expected_events, code);
	require(ne_event_counter == expected_events, code + 1);
	require(static_cast<long>(test_pool.objs_used) == expected_events, code + 2);
	require(nevent_catchup_debt == expected_debt, code + 3);
	require(tracked_debt == expected_debt, code + 4);
	if (expected_debt == 0)
		require(nevent_catchup_debt_estimated_us == 0, code + 5);
}

static void reset_scheduler()
{
	nevent_bind_game_thread();
	std::memset(ne_schedule, 0, sizeof(ne_schedule));
	std::memset(ne_schedule_tail, 0, sizeof(ne_schedule_tail));
	current_nevent = nullptr;
	ne_event_counter = 0;
	ne_event_sequence = 0;
	nevent_catchup_debt = 0;
	nevent_catchup_remaining = 0;
	nevent_catchup_debt_estimated_us = 0;
	nevent_deferred_due_counts.clear();
	nevent_pending_cancellations.clear();
	std::memset(&test_pool, 0, sizeof(test_pool));
	ne_dead_event_pool = &test_pool;
	release_calls = 0;
	payload_frees = 0;
	typed_payload_destroys = 0;
	link_removals = 0;
}

static void schedule_event(P_nevent event, unsigned int bucket, P_char ch = nullptr,
			   P_obj obj = nullptr, P_char victim = nullptr,
			   char_link_data *link = nullptr, unsigned int deferrals = 0)
{
	std::memset(event, 0, sizeof(*event));
	event->ch = ch;
	event->obj = obj;
	event->victim = victim;
	event->cld = link;
	event->func = callback;
	event->due_tick = 0;
	event->element = bucket;
	event->deferral_count = 0;
	event->deferred_cost_us = 0;
	event->sequence = ++ne_event_sequence;
	event->lifecycle_state = NEVENT_LIFECYCLE_ACTIVE;
	event->prev_sched = ne_schedule_tail[bucket];
	if (ne_schedule_tail[bucket])
		ne_schedule_tail[bucket]->next_sched = event;
	else
		ne_schedule[bucket] = event;
	ne_schedule_tail[bucket] = event;
	if (ch)
	{
		event->prev_char_nev = ch->nevents_tail;
		if (ch->nevents_tail)
			ch->nevents_tail->next_char_nev = event;
		else
			ch->nevents = event;
		ch->nevents_tail = event;
	}
	if (obj)
	{
		event->prev_obj_nev = obj->nevents_tail;
		if (obj->nevents_tail)
			obj->nevents_tail->next_obj_nev = event;
		else
			obj->nevents = event;
		obj->nevents_tail = event;
	}
	ne_event_counter++;
	test_pool.objs_used++;
	if (deferrals > 0)
	{
		nevent_register_deferred(event);
		event->deferral_count = deferrals;
	}
}

static void test_immediate_and_idempotent_cancel()
{
	reset_scheduler();
	nevent_data events[3] = {};
	for (nevent_data &event : events)
		schedule_event(&event, 17);
	events[1].data = std::malloc(8);
	events[1].data_destroy = nevent_destroy_raw_payload;

	const nevent_handle middle = nevent_handle_from_event(&events[1]);
	require(nevent_cancel(middle) == nevent_cancel_result::canceled, 1);
	require(nevent_cancel(middle) == nevent_cancel_result::already_inactive, 2);
	require(payload_frees == 1 && release_calls == 1, 3);
	require(ne_schedule[17] == &events[0] && ne_schedule_tail[17] == &events[2], 4);
	require(events[0].next_sched == &events[2] && events[2].prev_sched == &events[0], 5);
	require_balanced(2, 0, 6);

	require(nevent_cancel(nevent_handle_from_event(&events[0])) ==
			nevent_cancel_result::canceled,
		10);
	require(nevent_cancel(nevent_handle_from_event(&events[2])) ==
			nevent_cancel_result::canceled,
		11);
	require_balanced(0, 0, 12);
	require(nevent_cancel({nullptr, 0}) == nevent_cancel_result::invalid_handle, 16);
}

static void test_typed_payload_destruction()
{
	reset_scheduler();
	nevent_data event = {};
	schedule_event(&event, 18);
	auto *payload = new typed_payload;
	payload->path = { 1, 2, 3, 4 };
	event.data = payload;
	event.data_destroy = [](void *data) { delete static_cast<typed_payload *>(data); };

	require(nevent_cancel(nevent_handle_from_event(&event)) ==
			nevent_cancel_result::canceled,
		80);
	require(typed_payload_destroys == 1, 81);
	require_balanced(0, 0, 82);
}

static void test_lookup_excluding_current()
{
	reset_scheduler();
	char_data owner = {};
	nevent_data events[2] = {};
	schedule_event(&events[0], 61, &owner);
	current_nevent = &events[0];
	require(get_scheduled(&owner, callback) == &events[0], 90);
	require(get_scheduled_excluding_current(&owner, callback) == nullptr, 91);

	schedule_event(&events[1], 62, &owner);
	require(get_scheduled_excluding_current(&owner, callback) == &events[1], 92);
	current_nevent = nullptr;
	require(nevent_cancel(nevent_handle_from_event(&events[0])) ==
			nevent_cancel_result::canceled,
		93);
	require(nevent_cancel(nevent_handle_from_event(&events[1])) ==
			nevent_cancel_result::canceled,
		94);
	require_balanced(0, 0, 95);
}

static void test_debt_and_stale_handle()
{
	reset_scheduler();
	nevent_data event = {};
	schedule_event(&event, 22, nullptr, nullptr, nullptr, nullptr, 4);
	const nevent_handle old_handle = nevent_handle_from_event(&event);
	require_balanced(1, 1, 20);
	require(nevent_cancel(old_handle) == nevent_cancel_result::canceled, 24);
	require_balanced(0, 0, 25);

	schedule_event(&event, 23);
	require(nevent_cancel(old_handle) == nevent_cancel_result::stale_handle, 29);
	require_balanced(1, 0, 30);
	require(nevent_cancel(nevent_handle_from_event(&event)) ==
			nevent_cancel_result::canceled,
		34);
	require_balanced(0, 0, 35);
}

static void test_active_iteration_cancel()
{
	reset_scheduler();
	nevent_data events[3] = {};
	for (nevent_data &event : events)
		schedule_event(&event, 31, nullptr, nullptr, nullptr, nullptr, 1);
	current_nevent = &events[0];

	for (nevent_data &event : events)
	{
		const nevent_handle handle = nevent_handle_from_event(&event);
		require(nevent_cancel(handle) == nevent_cancel_result::deferred, 40);
		require(nevent_cancel(handle) == nevent_cancel_result::already_inactive, 41);
	}
	require(nevent_pending_cancellations.size() == 3, 42);
	require_balanced(3, 3, 43);

	require(nevent_destroy(&events[0]), 47);
	current_nevent = nullptr;
	nevent_process_pending_cancellations();
	require(nevent_pending_cancellations.empty(), 48);
	require(release_calls == 3, 49);
	require_balanced(0, 0, 50);
}

static void test_owner_and_victim_cleanup()
{
	reset_scheduler();
	char_data owner = {};
	char_data victim = {};
	obj_data object = {};
	char_link_data link = {};
	link.type = LNK_EVENT;
	link.linking = &owner;
	link.linked = &victim;
	nevent_data event = {};
	schedule_event(&event, 41, &owner, &object, &victim, &link);

	require(nevent_cancel(nevent_handle_from_event(&event)) ==
			nevent_cancel_result::canceled,
		60);
	require(owner.nevents == nullptr && owner.nevents_tail == nullptr &&
			object.nevents == nullptr && object.nevents_tail == nullptr,
		61);
	require(event.ch == nullptr && event.obj == nullptr && event.victim == nullptr &&
			event.cld == nullptr,
		62);
	require(link_removals == 1, 63);
	require_balanced(0, 0, 64);

	std::memset(&owner, 0, sizeof(owner));
	std::memset(&victim, 0, sizeof(victim));
	std::memset(&link, 0, sizeof(link));
	link.type = LNK_EVENT;
	link.linking = &owner;
	link.linked = &victim;
	schedule_event(&event, 42, &owner, nullptr, &victim, &link);
	event_broken(&link);
	require(owner.nevents == nullptr && owner.nevents_tail == nullptr &&
			event.victim == nullptr && event.cld == nullptr,
		68);
	require_balanced(0, 0, 69);
}

static void test_bulk_disarm()
{
	reset_scheduler();
	char_data owner = {};
	obj_data object = {};
	nevent_data events[3] = {};
	schedule_event(&events[0], 51, &owner);
	schedule_event(&events[1], 52, &owner);
	schedule_event(&events[2], 53, nullptr, &object);
	disarm_char_nevents(&owner, nullptr);
	require(owner.nevents == nullptr && owner.nevents_tail == nullptr, 73);
	require_balanced(1, 0, 74);
	disarm_obj_nevents(&object, nullptr);
	require(object.nevents == nullptr && object.nevents_tail == nullptr, 78);
	require_balanced(0, 0, 79);
}

int main()
{
	test_immediate_and_idempotent_cancel();
	test_debt_and_stale_handle();
	test_active_iteration_cancel();
	test_owner_and_victim_cleanup();
	test_bulk_disarm();
	test_typed_payload_destruction();
	test_lookup_excluding_current();
	std::puts("nevent cancellation runtime invariants passed");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-nevent-cancel-") as directory:
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
mobact = (SRC / "mobact.c").read_text(encoding="ascii")
assert "void clear_nevent" not in source
assert "clear_nevent(" not in mobact
assert "nevent_cancel(nevent_handle_from_event(e1))" in mobact

print("nevent cancellation runtime test passed under ASan/UBSan")
