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
static room_data test_world[1] = {};
P_room world = test_world;
extern const int top_of_world = 0;

static mm_ds test_pool = {};
static std::vector<std::pair<int, unsigned long long>> fired;
static std::vector<int> fired_this_tick;
static bool oracle_enabled = false;
static int failure_code = 0;
static unsigned long long fake_clock_ns = 0;
static int unbounded_warnings = 0;
static int invalid_config_warnings = 0;

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

void logit(const char *, const char *format, ...)
{
	if (format && std::strstr(format, "intentionally unbounded"))
		unbounded_warnings++;
	if (format && std::strstr(format, "allowed 0.."))
		invalid_config_warnings++;
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
DEFINE_LABEL_CALLBACK(event_spellcast)
DEFINE_LABEL_CALLBACK(event_memorize)
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
	require(record->expected_tick == ULLONG_MAX || ne_event_tick == record->expected_tick, 21);
	fired.emplace_back(record->id, ne_event_tick);
	fired_this_tick.push_back(record->id);
}

void event_wait(P_char ch, P_char victim, P_obj obj, void *data)
{
	record_callback(ch, victim, obj, data);
}

void event_ward_regen(P_char ch, P_char victim, P_obj obj, void *data)
{
	record_callback(ch, victim, obj, data);
}

static void add_record(int id, int delay, unsigned long long expected_tick)
{
	record_payload record = { id, expected_tick };
	add_event(record_callback, delay, nullptr, nullptr, nullptr, 0, &record, sizeof(record));
	if (oracle_enabled)
		oracle.push({ expected_tick, id });
}

static void add_player_record(P_char player, event_func_type callback, int id, int delay = 0)
{
	record_payload record = { id, ULLONG_MAX };
	add_event(callback, delay, player, nullptr, nullptr, 0, &record, sizeof(record));
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
	nevent_catchup_debt_estimated_us = 0;
	nevent_deferred_due_counts.clear();
	nevent_pending_cancellations.clear();
	pulse = 0;
	after_events_call = FALSE;
	fake_clock_ns = 0;
	fired.clear();
	fired_this_tick.clear();
	oracle = {};
	oracle_enabled = false;
	unbounded_warnings = 0;
	invalid_config_warnings = 0;
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
				add_event(noop_callback, 0, nullptr, nullptr, nullptr, 0, nullptr, 0);
		}
		P_nevent cursor = ne_schedule[0];
		for (int position = 0; position < target_position; ++position)
			cursor = cursor->next_sched;
		require(cursor && static_cast<record_payload *>(cursor->data)->id ==
					 2000 + target_position,
			81);
		run_one_heartbeat();
		require(fired.size() == 1 && fired[0].second == 0, 82);
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

static void test_priority_order(bool enabled)
{
	reset_scheduler();
	char_data player = {};
	player.specials.position = STAT_NORMAL | POS_STANDING;
	add_record(6000, 0, ULLONG_MAX);
	add_player_record(&player, event_wait, 6001);
	add_player_record(&player, event_ward_regen, 6002);
	ne_events();
	const std::vector<int> expected =
		enabled ? std::vector<int>{ 6001, 6002, 6000 } :
			  std::vector<int>{ 6000, 6001, 6002 };
	require(fired_this_tick == expected, 110);
	require(player.nevents == nullptr, 111);
	nevent_advance_tick();
	require_balanced(112);
}

static void test_bounded_normal_aging()
{
	reset_scheduler();
	char_data player = {};
	player.specials.position = STAT_NORMAL | POS_STANDING;
	add_record(7000, 0, ULLONG_MAX);
	for (int id = 7001; id <= 7004; ++id)
		add_player_record(&player, event_wait, id);
	for (int tick = 0; tick <= static_cast<int>(NEVENT_NORMAL_AGING_DEFERRALS); ++tick)
		run_one_heartbeat();
	auto normal = std::find(fired.begin(), fired.end(),
				std::pair<int, unsigned long long>{ 7000,
								       NEVENT_NORMAL_AGING_DEFERRALS });
	require(normal != fired.end(), 115);
	cancel_all_events();
	require_balanced(116);

	reset_scheduler();
	player = {};
	player.specials.position = STAT_NORMAL | POS_STANDING;
	add_record(7100, 0, ULLONG_MAX);
	add_player_record(&player, event_wait, 7101);
	run_one_heartbeat();
	add_player_record(&player, event_wait, 7102);
	run_one_heartbeat();
	normal = std::find(fired.begin(), fired.end(),
			   std::pair<int, unsigned long long>{ 7100, 1 });
	require(normal != fired.end(), 117);
	cancel_all_events();
	require_balanced(118);
}

static void test_catchup_convergence()
{
	reset_scheduler();
	nevent_avg_callback_us = 100;
	for (int id = 8000; id < 8006; ++id)
		add_record(id, 0, ULLONG_MAX);
	ne_events();
	require(nevent_catchup_debt == 4, 140);
	require(nevent_catchup_debt_estimated_us ==
			static_cast<unsigned long long>(4 * nevent_avg_callback_us),
		141);
	require(nevent_oldest_deferred_due_tick() == 0, 142);
	nevent_advance_tick();

	int next_id = 8100;
	int repayment_ticks = 0;
	while (nevent_catchup_debt > 0)
	{
		require(repayment_ticks++ < 8, 143);
		const long debt_before = nevent_catchup_debt;
		add_record(next_id++, 0, ULLONG_MAX);
		add_record(next_id++, 0, ULLONG_MAX);
		ne_events();
		require(nevent_catchup_debt < debt_before, 144);
		require(nevent_catchup_extension_us == 0, 145);
		nevent_advance_tick();
	}
	require(repayment_ticks <= NEVENT_CATCHUP_WINDOW_PULSES, 146);
	require(nevent_catchup_debt_estimated_us == 0 &&
			nevent_deferred_due_counts.empty(),
		147);
	require_balanced(148);
}

static void test_unbounded_warning()
{
	reset_scheduler();
	ne_events();
	nevent_advance_tick();
	ne_events();
	nevent_advance_tick();
	require(unbounded_warnings == 1, 150);
}

static void test_schedule_results_and_lookup()
{
	reset_scheduler();
	char_data dead_owner = {};
	dead_owner.in_room = 0;
	dead_owner.specials.position = STAT_DEAD;
	char_data victim = {};
	victim.in_room = 0;
	victim.specials.position = STAT_NORMAL | POS_STANDING;
	record_payload payload = { 9000, ULLONG_MAX };

	auto result = add_event(nullptr, 0, nullptr, nullptr, nullptr, 0, nullptr, 0);
	require(result.status == nevent_schedule_status::null_callback && !result.handle.event, 170);
	result = add_event(record_callback, -1, nullptr, nullptr, nullptr, 0, &payload,
			   sizeof(payload));
	require(result.status == nevent_schedule_status::negative_delay, 171);
	result = add_event(record_callback, 0, &dead_owner, nullptr, nullptr, 0, &payload,
			   sizeof(payload));
	require(result.status == nevent_schedule_status::dead_owner, 172);
	result = add_event(record_callback, 0, nullptr, &victim, nullptr, 0, &payload,
			   sizeof(payload));
	require(result.status == nevent_schedule_status::victim_without_owner, 173);
	result = add_event(record_callback, 0, nullptr, nullptr, nullptr, 0,
			   static_cast<const void *>(nullptr), sizeof(payload));
	require(result.status == nevent_schedule_status::invalid_payload, 174);
	ne_event_sequence = ULLONG_MAX;
	result = add_event(record_callback, 0, nullptr, nullptr, nullptr, 0, &payload,
			   sizeof(payload));
	require(result.status == nevent_schedule_status::sequence_exhausted, 175);
	ne_event_sequence = 0;
	require_balanced(176);

	auto late = add_event(record_callback, 300, nullptr, nullptr, nullptr, 0, &payload,
			      sizeof(payload));
	payload.id = 9001;
	auto early = add_event(record_callback, 1, nullptr, nullptr, nullptr, 0, &payload,
			       sizeof(payload));
	payload.id = 9002;
	auto middle = add_event(record_callback, 299, nullptr, nullptr, nullptr, 0, &payload,
				sizeof(payload));
	require(late && early && middle, 177);
	require(nevent_find_next(record_callback).sequence == early.handle.sequence, 178);
	require(get_scheduled(record_callback) == early.handle.event, 179);
	cancel_all_events();
	require_balanced(180);

	reset_scheduler();
	char_data player = {};
	player.specials.position = STAT_NORMAL | POS_STANDING;
	add_player_record(&player, event_wait, 9010, 10);
	add_player_record(&player, event_wait, 9011, 2);
	const nevent_handle owner_first = nevent_find_next(&player, event_wait);
	require(owner_first.event && owner_first.event->due_tick == 2, 181);
	P_nevent owner_second = get_next_scheduled_char(owner_first.event, event_wait);
	require(owner_second && owner_second->due_tick == 10, 182);
	current_nevent = owner_first.event;
	require(nevent_find_next_excluding_current(&player, event_wait).event == owner_second, 183);
	current_nevent = nullptr;
	cancel_all_events();
	require_balanced(184);

	reset_scheduler();
	payload = { 9020, ULLONG_MAX };
	auto existing = add_event(record_callback, 10, nullptr, nullptr, nullptr, 0, &payload,
				  sizeof(payload));
	payload.id = 9021;
	auto rejected = nevent_replace(existing.handle, record_callback, -1, nullptr, nullptr,
				       nullptr, 0, &payload, sizeof(payload));
	require(rejected.status == nevent_schedule_status::negative_delay && ne_event_counter == 1,
		185);
	require(nevent_find_next(record_callback).sequence == existing.handle.sequence, 186);
	auto replacement = nevent_replace(existing.handle, record_callback, 2, nullptr, nullptr,
					  nullptr, 0, &payload, sizeof(payload));
	require(replacement && ne_event_counter == 1, 187);
	require(nevent_find_next(record_callback).sequence == replacement.handle.sequence, 188);
	auto invalid = nevent_replace({ nullptr, 0 }, record_callback, 1, nullptr, nullptr, nullptr, 0,
				      &payload, sizeof(payload));
	require(invalid.status == nevent_schedule_status::invalid_replace_target &&
			ne_event_counter == 1,
		189);
	while (ne_event_counter > 0)
		run_one_heartbeat();
	require(fired.size() == 1 && fired[0] == std::pair<int, unsigned long long>{ 9021, 2 }, 190);
}

static void test_invalid_config_fallback()
{
	require(nevent_budget_usec() == NEVENT_BUDGET_USEC_DEFAULT, 155);
	require(invalid_config_warnings == 1, 156);
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

int main(int argc, char **argv)
{
	require(argc == 2, 160);
	if (std::strcmp(argv[1], "common") == 0)
	{
		test_boundary_matrix();
		test_current_bucket_positions();
		test_shared_bucket_revolutions();
		test_reschedule_apis();
		test_randomized_oracle();
	}
	else if (std::strcmp(argv[1], "priority-off") == 0)
		test_priority_order(false);
	else if (std::strcmp(argv[1], "priority-on") == 0)
		test_priority_order(true);
	else if (std::strcmp(argv[1], "aging") == 0)
		test_bounded_normal_aging();
	else if (std::strcmp(argv[1], "catchup") == 0)
		test_catchup_convergence();
	else if (std::strcmp(argv[1], "unbounded") == 0)
		test_unbounded_warning();
	else if (std::strcmp(argv[1], "invalid-config") == 0)
		test_invalid_config_fallback();
	else if (std::strcmp(argv[1], "api") == 0)
		test_schedule_results_and_lookup();
	else
		require(false, 161);
	std::printf("nevent scheduler runtime mode passed: %s\n", argv[1]);
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
    base_environment = os.environ.copy()
    base_environment.update(
        {
            "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
            "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1",
            "DURIS_NEVENT_ANALYTICS": "0",
            "DURIS_NEVENT_BUDGET_USEC": "0",
            "DURIS_NEVENT_MAX_CALLBACKS": "0",
            "DURIS_NEVENT_CATCHUP_MAX_EXTENSION_USEC": "0",
            "DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS": "0",
            "DURIS_NEVENT_PLAYER_PRIORITY": "0",
        }
    )

    def run_mode(mode: str, **overrides: str) -> None:
        environment = base_environment.copy()
        environment.update(overrides)
        subprocess.run([str(binary), mode], check=True, env=environment)

    run_mode("common")
    run_mode("priority-off")
    run_mode("priority-on", DURIS_NEVENT_PLAYER_PRIORITY="1")
    run_mode(
        "aging",
        DURIS_NEVENT_PLAYER_PRIORITY="1",
        DURIS_NEVENT_MAX_CALLBACKS="1",
    )
    run_mode(
        "catchup",
        DURIS_NEVENT_MAX_CALLBACKS="2",
        DURIS_NEVENT_CATCHUP_MAX_EXTRA_CALLBACKS="1",
    )
    run_mode("unbounded")
    run_mode("invalid-config", DURIS_NEVENT_BUDGET_USEC="9" * 100)
    run_mode("api")

source = (SRC / "new_events.c").read_text(encoding="ascii")
comm = (SRC / "comm.c").read_text(encoding="ascii")
assert "--(current_nevent->timer)" not in source
assert "event->timer" not in source
assert "current_nevent->due_tick > ne_event_tick" in source
assert "pass_sequence = ne_event_sequence" in source
assert "nevent_sorts_before" in source
assert "NEVENT_NORMAL_AGING_DEFERRALS" in source
assert "nevent_schedule_status" in source
assert "nevent_find_next" in source
assert "nevent_advance_tick();" in comm

print("nevent scheduler timing, priority, aging, and catch-up passed under ASan/UBSan")
