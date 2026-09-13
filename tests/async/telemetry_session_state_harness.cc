#include "telemetry/telemetry_session.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

/* The state machine must not allocate, even when exercised as a standalone binary. */
void *operator new(std::size_t)
{
	std::abort();
}
void *operator new[](std::size_t)
{
	std::abort();
}
void operator delete(void *) noexcept {}
void operator delete[](void *) noexcept {}
void operator delete(void *, std::size_t) noexcept {}
void operator delete[](void *, std::size_t) noexcept {}

namespace
{

struct test_clock
{
	telemetry_monotonic_usec monotonic_usec;
	telemetry_utc_usec utc_usec;
	bool available;
};

struct finite_sink
{
	telemetry_record records[512];
	std::size_t count;
	std::size_t capacity;
	bool reject;
};

struct key_space
{
	telemetry_producer_id producer;
	telemetry_record_sequence next_sequence;
	bool fail;
};

static test_clock clock_state{};
static finite_sink sink_state{};
static key_space key_state{};
static telemetry_session_state session_state{};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry session harness failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool clock_now(void *context, telemetry_monotonic_usec *monotonic_usec,
	       telemetry_utc_usec *utc_usec) noexcept
{
	auto *clock = static_cast<test_clock *>(context);
	if (!clock->available)
		return false;
	*monotonic_usec = clock->monotonic_usec;
	*utc_usec = clock->utc_usec;
	return true;
}

bool sink_emit(void *context, const telemetry_record *record) noexcept
{
	auto *sink = static_cast<finite_sink *>(context);
	if (sink->reject || sink->count >= sink->capacity)
		return false;
	sink->records[sink->count++] = *record;
	return true;
}

bool next_key(void *context, telemetry_record_kind, telemetry_record_key *key) noexcept
{
	auto *space = static_cast<key_space *>(context);
	if (space->fail || space->next_sequence == 0U || space->next_sequence == UINT64_MAX)
		return false;
	*key = { space->producer, space->next_sequence };
	++space->next_sequence;
	return true;
}

void set_clock(telemetry_monotonic_usec monotonic_usec, telemetry_utc_usec utc_usec)
{
	clock_state.monotonic_usec = monotonic_usec;
	clock_state.utc_usec = utc_usec;
	clock_state.available = true;
}

telemetry_session_enter make_enter(telemetry_producer_id producer,
				   telemetry_session_sequence session_sequence,
				   telemetry_connection_sequence connection_sequence,
				   telemetry_monotonic_usec monotonic_usec,
				   telemetry_utc_usec utc_usec)
{
	telemetry_session_enter enter{};
	enter.session = { { producer, session_sequence }, 9001U, 42, 7U, 8U };
	enter.connection = { producer, connection_sequence };
	enter.at_monotonic_usec = monotonic_usec;
	enter.at_utc_usec = utc_usec;
	enter.dimensions = { 1U, 1U, 1U, 1U, 100, 1U };
	enter.config_id = 100U;
	enter.classifier_version = 400U;
	enter.policy_version = 500U;
	return enter;
}

telemetry_counter_update
make_counter(const telemetry_session_ref &session, const telemetry_connection_id &connection,
	     telemetry_monotonic_usec monotonic_usec, telemetry_duration_usec connected_usec,
	     telemetry_duration_usec active_usec, telemetry_duration_usec idle_usec,
	     telemetry_duration_usec unknown_usec)
{
	telemetry_counter_update update{};
	update.session = session;
	update.connection = connection;
	update.at_monotonic_usec = monotonic_usec;
	update.connected_delta_usec = connected_usec;
	update.active_delta_usec = active_usec;
	update.idle_delta_usec = idle_usec;
	update.unknown_delta_usec = unknown_usec;
	update.resident_delta_usec = connected_usec;
	return update;
}

telemetry_connection_transition make_transition(const telemetry_session_ref &session,
						const telemetry_connection_id &connection,
						telemetry_monotonic_usec monotonic_usec,
						telemetry_utc_usec utc_usec,
						telemetry_connection_transition_kind kind)
{
	telemetry_connection_transition transition{};
	transition.session = session;
	transition.connection = connection;
	transition.at_monotonic_usec = monotonic_usec;
	transition.at_utc_usec = utc_usec;
	transition.kind = kind;
	return transition;
}

telemetry_session_exit make_exit(const telemetry_session_ref &session,
				 const telemetry_connection_id &connection,
				 telemetry_monotonic_usec monotonic_usec,
				 telemetry_utc_usec utc_usec)
{
	telemetry_session_exit exit{};
	exit.session = session;
	exit.connection = connection;
	exit.at_monotonic_usec = monotonic_usec;
	exit.at_utc_usec = utc_usec;
	exit.reason = telemetry_session_end_reason::logout;
	return exit;
}

void reset_fixture(telemetry_producer_id producer, std::uint16_t capacity,
		   telemetry_duration_usec retire_after_usec)
{
	clock_state = { 0U, TELEMETRY_UTC_UNKNOWN, true };
	sink_state = {};
	sink_state.capacity = sizeof(sink_state.records) / sizeof(sink_state.records[0]);
	key_state = { producer, 1U, false };
	telemetry_session_state_config config{};
	config.max_slots = capacity;
	config.detached_retire_after_usec = retire_after_usec;
	config.producer = producer;
	config.clock = { clock_now, &clock_state };
	config.sink = { sink_emit, &sink_state };
	config.key_allocator = { next_key, &key_state };
	CHECK(telemetry_session_state_init(&session_state, &config) ==
	      telemetry_session_state_outcome::accepted);
}

void check_records_valid()
{
	for (std::size_t index = 0; index < sink_state.count; ++index)
		CHECK(telemetry_record_is_valid(sink_state.records[index]));
}

const telemetry_record *find_record(telemetry_record_kind kind, std::size_t occurrence)
{
	for (std::size_t index = 0; index < sink_state.count; ++index)
	{
		if (sink_state.records[index].header.kind != kind)
			continue;
		if (occurrence == 0U)
			return &sink_state.records[index];
		--occurrence;
	}
	return nullptr;
}

std::size_t count_lifecycle(telemetry_lifecycle_kind lifecycle)
{
	std::size_t count = 0U;
	for (std::size_t index = 0; index < sink_state.count; ++index)
		if (sink_state.records[index].header.kind ==
			    telemetry_record_kind::session_lifecycle &&
		    sink_state.records[index].payload.lifecycle.lifecycle == lifecycle)
			++count;
	return count;
}

void test_normal_checkpoint_and_shared_allocator()
{
	const telemetry_producer_id producer = { 101U, 201U };
	reset_fixture(producer, 4U, 1000U);
	const telemetry_session_enter enter = make_enter(producer, 1U, 1U, 1000U, 1000000);
	telemetry_session_state_result result =
		telemetry_session_state_enter(&session_state, enter);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(sink_state.count == 1U);

	/* #266 detail asks the same allocator for key 2; control resumes at key 3. */
	telemetry_record_key detail_key{};
	CHECK(next_key(&key_state, telemetry_record_kind::interval, &detail_key));
	CHECK(detail_key.record_seq == 2U);
	const telemetry_counter_update update =
		make_counter(enter.session, enter.connection, 1100U, 100U, 100U, 0U, 0U);
	result = telemetry_session_state_update_counters(&session_state, update);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	set_clock(1100U, 1000100);
	result = telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == 1U);
	CHECK(result.cumulative.connected_usec == 100U);
	CHECK(result.cumulative.active_usec == 100U);
	CHECK(sink_state.count == 2U);
	const telemetry_record *checkpoint =
		find_record(telemetry_record_kind::session_checkpoint, 0U);
	CHECK(checkpoint != nullptr);
	CHECK(checkpoint->header.key.record_seq == 3U);
	CHECK(checkpoint->payload.checkpoint.revision == 1U);
	CHECK(checkpoint->payload.checkpoint.cumulative.connected_usec == 100U);

	const std::size_t before_duplicate = sink_state.count;
	result = telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::idempotent);
	CHECK(sink_state.count == before_duplicate);
	check_records_valid();
}

void test_detach_reconnect_exit_and_idempotency()
{
	const telemetry_producer_id producer = { 108U, 208U };
	reset_fixture(producer, 4U, 1000U);
	const telemetry_session_enter enter = make_enter(producer, 1U, 1U, 100U, 8000100);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   200U, 100U, 100U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	const telemetry_connection_transition detach =
		make_transition(enter.session, enter.connection, 200U, 8000200,
				telemetry_connection_transition_kind::detached);
	CHECK(telemetry_session_state_transition(&session_state, detach).outcome ==
	      telemetry_session_state_outcome::accepted);
	const std::size_t after_detach = sink_state.count;
	CHECK(telemetry_session_state_transition(&session_state, detach).outcome ==
	      telemetry_session_state_outcome::idempotent);
	CHECK(sink_state.count == after_detach);

	set_clock(300U, 8000300);
	const telemetry_connection_id connection_two = { producer, 2U };
	const telemetry_connection_transition attach =
		make_transition(enter.session, connection_two, 300U, 8000300,
				telemetry_connection_transition_kind::attached);
	CHECK(telemetry_session_state_transition(&session_state, attach).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_transition(&session_state, attach).outcome ==
	      telemetry_session_state_outcome::idempotent);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, connection_two,
								   350U, 50U, 0U, 50U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	set_clock(350U, 8000350);
	telemetry_session_state_result result =
		telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	telemetry_session_state_view view{};
	CHECK(telemetry_session_state_copy_view(&session_state, enter.session, &view));
	CHECK(view.cumulative.connected_usec == 150U);
	CHECK(view.cumulative.active_usec == 100U);
	CHECK(view.cumulative.idle_usec == 50U);
	CHECK(view.cumulative.unknown_usec == 0U);
	CHECK(view.cumulative.resident_usec == 250U);
	CHECK(view.cumulative.linkdead_usec == 100U);
	CHECK(view.last_allocated_revision == 1U);
	CHECK(count_lifecycle(telemetry_lifecycle_kind::connection_detached) == 1U);
	CHECK(count_lifecycle(telemetry_lifecycle_kind::connection_attached) == 1U);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, connection_two,
								   360U, 10U, 0U, 10U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	set_clock(360U, 8000200);
	result = telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK((result.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U);

	const telemetry_session_exit exit = make_exit(enter.session, connection_two, 400U, 8000400);
	result = telemetry_session_state_exit(&session_state, exit);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == 3U);
	const std::size_t after_exit = sink_state.count;
	CHECK(telemetry_session_state_exit(&session_state, exit).outcome ==
	      telemetry_session_state_outcome::idempotent);
	CHECK(sink_state.count == after_exit);
	CHECK(count_lifecycle(telemetry_lifecycle_kind::session_exited) == 1U);
	CHECK(telemetry_session_state_copy_view(&session_state, enter.session, &view));
	CHECK(view.closed != 0U);
	check_records_valid();
}

void test_dropped_control_recovers_absolute_total()
{
	const telemetry_producer_id producer = { 105U, 205U };
	reset_fixture(producer, 2U, 1000U);
	const telemetry_session_enter enter = make_enter(producer, 1U, 1U, 100U, 5000100);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   140U, 40U, 40U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	// Flush the classifier through the checkpoint cut before publishing it.
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   200U, 60U, 60U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	sink_state.capacity = sink_state.count;
	set_clock(200U, 5000200);
	telemetry_session_state_result result =
		telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::sink_rejected);
	CHECK(result.revision == 1U);
	const std::size_t after_drop = sink_state.count;
	sink_state.capacity = sizeof(sink_state.records) / sizeof(sink_state.records[0]);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   220U, 20U, 0U, 20U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	set_clock(220U, 5000220);
	result = telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == 2U);
	CHECK(result.cumulative.connected_usec == 120U);
	CHECK(result.cumulative.active_usec == 100U);
	CHECK(result.cumulative.idle_usec == 20U);
	CHECK(sink_state.count == after_drop + 2U);
	const telemetry_record *gap = find_record(telemetry_record_kind::coverage_gap, 0U);
	CHECK(gap != nullptr);
	CHECK(gap->payload.gap.reason == telemetry_gap_reason::control_queue_drop);
	CHECK(gap->payload.gap.dropped_records == 1U);
	CHECK(gap->payload.gap.duration_usec == 0U);
	const telemetry_record *checkpoint =
		find_record(telemetry_record_kind::session_checkpoint, 0U);
	CHECK(checkpoint != nullptr);
	CHECK(checkpoint->payload.checkpoint.revision == 2U);
	CHECK(checkpoint->payload.checkpoint.cumulative.connected_usec == 120U);
	const telemetry_session_state_stats stats =
		telemetry_session_state_stats_copy(&session_state);
	CHECK(stats.dropped_control_total == 1U);
	check_records_valid();
}

void test_copyover_uses_new_clock_and_producer()
{
	const telemetry_producer_id old_producer = { 801U, 1U };
	reset_fixture(old_producer, 2U, 1000U);
	const telemetry_session_enter old_enter = make_enter(old_producer, 9U, 1U, 100U, 9000100);
	CHECK(telemetry_session_state_enter(&session_state, old_enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(
		      &session_state, make_counter(old_enter.session, old_enter.connection, 200U,
						   100U, 100U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	set_clock(200U, 9000200);
	CHECK(telemetry_session_state_checkpoint(&session_state, old_enter.session).revision == 1U);
	telemetry_session_handoff handoff{};
	set_clock(200U, 9000200);
	CHECK(telemetry_session_state_handoff_copy(&session_state, old_enter.session, &handoff)
		      .outcome == telemetry_session_state_outcome::accepted);
	CHECK(handoff.previous_producer.boot_id == 801U);
	CHECK(handoff.last_checkpoint_revision == 1U);
	CHECK(handoff.cumulative.connected_usec == 100U);

	const telemetry_producer_id new_producer = { 802U, 1U };
	reset_fixture(new_producer, 2U, 1000U);
	telemetry_session_resume resume{};
	resume.handoff = handoff;
	resume.entry = make_enter(new_producer, 9U, 1U, 10U, 9000210);
	resume.entry.session = old_enter.session;
	telemetry_session_state_result result =
		telemetry_session_state_resume(&session_state, resume);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(sink_state.count == 1U);
	CHECK(sink_state.records[0].payload.lifecycle.lifecycle ==
	      telemetry_lifecycle_kind::connection_attached);
	CHECK(sink_state.records[0].payload.lifecycle.session.id.producer.boot_id == 801U);
	CHECK(sink_state.records[0].payload.lifecycle.connection.producer.boot_id == 802U);
	CHECK(sink_state.records[0].header.key.producer.boot_id == 802U);
	CHECK(telemetry_session_state_update_counters(
		      &session_state, make_counter(resume.entry.session, resume.entry.connection,
						   60U, 50U, 50U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	set_clock(60U, 9000260);
	result = telemetry_session_state_checkpoint(&session_state, resume.entry.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == 2U);
	CHECK(result.cumulative.connected_usec == 150U);
	CHECK(result.cumulative.active_usec == 150U);
	CHECK(result.cumulative.resident_usec == 150U);
	telemetry_session_state_view view{};
	CHECK(telemetry_session_state_copy_view(&session_state, resume.entry.session, &view));
	CHECK(view.cumulative.connected_usec == 150U);
	CHECK(view.last_allocated_revision == 2U);
	CHECK(sink_state.count == 2U);
	check_records_valid();
}

void test_absent_handoff_is_unclosed_without_tail()
{
	const telemetry_producer_id producer = { 903U, 1U };
	reset_fixture(producer, 2U, 1000U);
	telemetry_session_resume resume{};
	resume.entry = make_enter(producer, 1U, 1U, 10U, 10000300);
	telemetry_session_state_result result =
		telemetry_session_state_resume(&session_state, resume);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted_unclosed_recovery);
	CHECK(sink_state.count == 2U);
	const telemetry_record *gap = find_record(telemetry_record_kind::coverage_gap, 0U);
	CHECK(gap != nullptr);
	CHECK(gap->payload.gap.reason == telemetry_gap_reason::unclosed_tail);
	CHECK(gap->payload.gap.duration_usec == 0U);
	CHECK(gap->payload.gap.dropped_records == 0U);
	CHECK((gap->payload.gap.quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U);
	CHECK(telemetry_session_state_update_counters(
		      &session_state, make_counter(resume.entry.session, resume.entry.connection,
						   20U, 10U, 10U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	const std::size_t before_duplicate = sink_state.count;
	CHECK(telemetry_session_state_resume(&session_state, resume).outcome ==
	      telemetry_session_state_outcome::idempotent);
	CHECK(sink_state.count == before_duplicate);
	check_records_valid();
}

void test_capacity_retirement_and_failures()
{
	const telemetry_producer_id producer = { 910U, 1U };
	reset_fixture(producer, 1U, 50U);
	const telemetry_session_enter first = make_enter(producer, 1U, 1U, 0U, 10000000);
	CHECK(telemetry_session_state_enter(&session_state, first).outcome ==
	      telemetry_session_state_outcome::accepted);
	const std::size_t after_enter = sink_state.count;
	CHECK(telemetry_session_state_enter(&session_state, first).outcome ==
	      telemetry_session_state_outcome::idempotent);
	CHECK(sink_state.count == after_enter);
	const telemetry_connection_transition detach =
		make_transition(first.session, first.connection, 100U, 10000100,
				telemetry_connection_transition_kind::detached);
	CHECK(telemetry_session_state_transition(&session_state, detach).outcome ==
	      telemetry_session_state_outcome::accepted);
	const telemetry_session_enter second = make_enter(producer, 2U, 2U, 149U, 10000149);
	CHECK(telemetry_session_state_enter(&session_state, second).outcome ==
	      telemetry_session_state_outcome::capacity_full);
	telemetry_session_state_result result = telemetry_session_state_enter(
		&session_state, make_enter(producer, 2U, 2U, 150U, 10000150));
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.slots_retired == 1U);
	telemetry_session_state_stats stats = telemetry_session_state_stats_copy(&session_state);
	CHECK(stats.capacity == 1U);
	CHECK(stats.used_slots == 1U);
	CHECK(stats.resident_slots == 1U);
	CHECK(stats.retired_session_total == 1U);
	CHECK(stats.unclosed_tail_total == 1U);
	const telemetry_session_ref second_session = second.session;
	const telemetry_connection_id second_connection = second.connection;
	CHECK(telemetry_session_state_exit(
		      &session_state, make_exit(second_session, second_connection, 160U, 10000160))
		      .outcome == telemetry_session_state_outcome::accepted);
	stats = telemetry_session_state_stats_copy(&session_state);
	CHECK(stats.closed_slots == 1U);
	const telemetry_session_enter third = make_enter(producer, 3U, 3U, 200U, 10000200);
	CHECK(telemetry_session_state_enter(&session_state, third).outcome ==
	      telemetry_session_state_outcome::accepted);
	stats = telemetry_session_state_stats_copy(&session_state);
	CHECK(stats.closed_slots == 0U);
	CHECK(stats.used_slots == 1U);

	clock_state.available = false;
	result = telemetry_session_state_checkpoint(&session_state, third.session);
	CHECK(result.outcome == telemetry_session_state_outcome::clock_unavailable);
	clock_state.available = true;
	key_state.fail = true;
	set_clock(200U, 10000200);
	result = telemetry_session_state_checkpoint(&session_state, third.session);
	CHECK(result.outcome == telemetry_session_state_outcome::allocator_exhausted);
	CHECK(result.revision == 0U);
	key_state.fail = false;
	result = telemetry_session_state_checkpoint(&session_state, third.session);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == 1U);
	check_records_valid();
}

void test_numeric_boundaries()
{
	const telemetry_producer_id producer = { 911U, 1U };
	reset_fixture(producer, 2U, 1000U);
	const telemetry_session_enter enter = make_enter(producer, 1U, 1U, 0U, 11000000);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	session_state.slots[0].last_allocated_revision = UINT64_MAX;
	set_clock(0U, 11000000);
	telemetry_session_state_result result =
		telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(result.outcome == telemetry_session_state_outcome::revision_exhausted);
	CHECK(telemetry_session_state_stats_copy(&session_state).revision_exhaustion_total == 1U);

	session_state.slots[0].last_allocated_revision = 0U;
	session_state.slots[0].cumulative = { UINT64_MAX, UINT64_MAX, 0U, 0U, UINT64_MAX, 0U };
	telemetry_session_state_view before{};
	CHECK(telemetry_session_state_copy_view(&session_state, enter.session, &before));
	result = telemetry_session_state_update_counters(
		&session_state, make_counter(enter.session, enter.connection, 1U, 1U, 1U, 0U, 0U));
	CHECK(result.outcome == telemetry_session_state_outcome::invalid);
	telemetry_session_state_view after{};
	CHECK(telemetry_session_state_copy_view(&session_state, enter.session, &after));
	CHECK(after.cumulative.connected_usec == before.cumulative.connected_usec);
	CHECK(after.cumulative.active_usec == before.cumulative.active_usec);

	telemetry_session_state_config bad{};
	bad.max_slots = static_cast<std::uint16_t>(TELEMETRY_SESSION_STATE_MAX_SLOTS + 1U);
	bad.detached_retire_after_usec = 1U;
	bad.producer = producer;
	bad.clock = { clock_now, &clock_state };
	bad.sink = { sink_emit, &sink_state };
	bad.key_allocator = { next_key, &key_state };
	CHECK(telemetry_session_state_init(&session_state, &bad) ==
	      telemetry_session_state_outcome::invalid);
	check_records_valid();
}

void test_counter_time_authority_and_atomicity()
{
	const telemetry_producer_id producer = { 912U, 1U };
	reset_fixture(producer, 2U, 1000U);
	const auto enter = make_enter(producer, 1U, 1U, 0U, 1000);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	auto inflated = make_counter(enter.session, enter.connection, 10U, 100U, 100U, 0U, 0U);
	CHECK(telemetry_session_state_update_counters(&session_state, inflated).outcome ==
	      telemetry_session_state_outcome::invalid);
	CHECK(session_state.slots[0].cumulative.resident_usec == 0U);
	auto valid = make_counter(enter.session, enter.connection, 10U, 10U, 10U, 0U, 0U);
	CHECK(telemetry_session_state_update_counters(&session_state, valid).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(&session_state, valid).outcome ==
	      telemetry_session_state_outcome::idempotent);
	auto conflict = make_counter(enter.session, enter.connection, 10U, 10U, 0U, 10U, 0U);
	CHECK(telemetry_session_state_update_counters(&session_state, conflict).outcome ==
	      telemetry_session_state_outcome::invalid);
	set_clock(20U, 1020);
	auto checkpoint = telemetry_session_state_checkpoint(&session_state, enter.session);
	CHECK(checkpoint.cumulative.connected_usec == 20U);
	CHECK(checkpoint.cumulative.active_usec == 10U);
	CHECK(checkpoint.cumulative.unknown_usec == 10U);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   25U, 15U, 15U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::invalid);

	reset_fixture(producer, 2U, 1000U);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	auto &slot = session_state.slots[0];
	slot.cumulative = { 1U, 1U, 0U, 0U, UINT64_MAX, UINT64_MAX - 1U };
	CHECK(telemetry_cumulative_counters_are_valid(slot.cumulative));
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   1U, 1U, 1U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::invalid);
	CHECK(slot.cumulative.connected_usec == 1U);
	CHECK(slot.cumulative.active_usec == 1U);
	CHECK(slot.cumulative.resident_usec == UINT64_MAX);
	CHECK(slot.last_observed_monotonic_usec == 0U);
}

void test_loss_scope_and_closed_checkpoint()
{
	const telemetry_producer_id producer = { 913U, 1U };
	reset_fixture(producer, 2U, 1000U);
	const auto first = make_enter(producer, 1U, 1U, 0U, 1000);
	const auto second = make_enter(producer, 2U, 2U, 0U, 1000);
	sink_state.reject = true;
	CHECK(telemetry_session_state_enter(&session_state, first).outcome ==
	      telemetry_session_state_outcome::sink_rejected);
	CHECK(telemetry_session_state_enter(&session_state, second).outcome ==
	      telemetry_session_state_outcome::sink_rejected);
	sink_state.reject = false;
	set_clock(10U, 1010);
	auto checkpoint = telemetry_session_state_checkpoint(&session_state, first.session);
	CHECK(checkpoint.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP);
	const auto *gap = find_record(telemetry_record_kind::coverage_gap, 0U);
	CHECK(gap != nullptr);
	CHECK(telemetry_session_ref_is_zero(gap->payload.gap.session));
	CHECK(telemetry_connection_id_is_zero(gap->payload.gap.connection));
	CHECK(telemetry_session_state_exit(&session_state,
					   make_exit(first.session, first.connection, 10U, 1010))
		      .outcome == telemetry_session_state_outcome::accepted);
	const auto count = sink_state.count;
	set_clock(20U, 1020);
	CHECK(telemetry_session_state_checkpoint(&session_state, first.session).outcome ==
	      telemetry_session_state_outcome::invalid);
	CHECK(sink_state.count == count);
	check_records_valid();
}

void test_connection_identity_replay()
{
	const telemetry_producer_id producer = { 914U, 1U };
	reset_fixture(producer, 1U, 1000U);
	const auto enter = make_enter(producer, 1U, 1U, 0U, 1000);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	const telemetry_connection_id second = { producer, 2U };
	CHECK(telemetry_session_state_transition(
		      &session_state,
		      make_transition(enter.session, enter.connection, 10U, 1010,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_transition(
		      &session_state,
		      make_transition(enter.session, second, 20U, 1020,
				      telemetry_connection_transition_kind::attached))
		      .outcome == telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_transition(
		      &session_state,
		      make_transition(enter.session, second, 30U, 1030,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_session_state_outcome::accepted);
	const auto count = sink_state.count;
	CHECK(telemetry_session_state_transition(
		      &session_state,
		      make_transition(enter.session, enter.connection, 40U, 1040,
				      telemetry_connection_transition_kind::attached))
		      .outcome == telemetry_session_state_outcome::invalid);
	CHECK(sink_state.count == count);
	CHECK(session_state.slots[0].connected == 0U);
	const telemetry_connection_id third = { producer, 3U };
	const auto attach = make_transition(enter.session, third, 40U, 1040,
					    telemetry_connection_transition_kind::attached);
	CHECK(telemetry_session_state_transition(&session_state, attach).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_transition(&session_state, attach).outcome ==
	      telemetry_session_state_outcome::idempotent);
	auto conflicting = attach;
	conflicting.at_monotonic_usec++;
	CHECK(telemetry_session_state_transition(&session_state, conflicting).outcome ==
	      telemetry_session_state_outcome::invalid);
	CHECK(telemetry_session_state_exit(&session_state,
					   make_exit(enter.session, third, 50U, 1050))
		      .outcome == telemetry_session_state_outcome::accepted);
	const auto newer = make_enter(producer, 2U, 4U, 60U, 1060);
	CHECK(telemetry_session_state_enter(&session_state, newer).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::invalid);
	check_records_valid();
}

void test_resume_conflicts()
{
	const telemetry_producer_id producer = { 916U, 1U };
	reset_fixture(producer, 2U, 1000U);
	telemetry_session_resume resume{};
	resume.entry = make_enter(producer, 1U, 1U, 0U, 1000);
	resume.entry.session.id.producer = { 915U, 1U };
	resume.handoff.session = resume.entry.session;
	resume.handoff.previous_producer = { 915U, 1U };
	resume.handoff.cumulative = { 10U, 10U, 0U, 0U, 10U, 0U };
	resume.handoff.last_checkpoint_revision = 1U;
	CHECK(telemetry_session_state_resume(&session_state, resume).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_resume(&session_state, resume).outcome ==
	      telemetry_session_state_outcome::idempotent);
	const auto count = sink_state.count;
	for (unsigned variant = 0U; variant < 4U; ++variant)
	{
		auto conflict = resume;
		if (variant == 0U)
			conflict.handoff.cumulative = { 20U, 20U, 0U, 0U, 20U, 0U };
		if (variant == 1U)
			conflict.handoff.last_checkpoint_revision++;
		if (variant == 2U)
			conflict.handoff.previous_producer.boot_id = 914U;
		if (variant == 3U)
		{
			conflict.handoff.quality_flags = TELEMETRY_QUALITY_QUEUE_DROP;
			conflict.entry.quality_flags = TELEMETRY_QUALITY_QUEUE_DROP;
		}
		CHECK(telemetry_session_state_resume(&session_state, conflict).outcome ==
		      telemetry_session_state_outcome::invalid);
	}
	CHECK(sink_state.count == count);
	CHECK(session_state.slots[0].cumulative.connected_usec == 10U);
	CHECK(session_state.slots[0].last_allocated_revision == 1U);
	reset_fixture(producer, 2U, 1000U);
	resume.entry.session.id.producer = producer;
	resume.handoff.session = resume.entry.session;
	CHECK(telemetry_session_state_resume(&session_state, resume).outcome ==
	      telemetry_session_state_outcome::invalid);
	CHECK(sink_state.count == 0U);
}

void test_dropped_control_copyover_gate()
{
	const telemetry_producer_id producer = { 917U, 1U };
	reset_fixture(producer, 2U, 1000U);
	const auto enter = make_enter(producer, 1U, 1U, 0U, 1000);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	set_clock(10U, 1010);
	sink_state.reject = true;
	CHECK(telemetry_session_state_checkpoint(&session_state, enter.session).outcome ==
	      telemetry_session_state_outcome::sink_rejected);
	telemetry_session_handoff handoff{};
	const auto before = session_state.pending_control_drops;
	CHECK(telemetry_session_state_handoff_copy(&session_state, enter.session, &handoff)
		      .outcome == telemetry_session_state_outcome::sink_rejected);
	CHECK(telemetry_session_ref_is_zero(handoff.session));
	CHECK(session_state.pending_control_drops >= before);
	sink_state.reject = false;
	const auto missing = session_state.pending_control_drops;
	CHECK(telemetry_session_state_handoff_copy(&session_state, enter.session, &handoff)
		      .outcome == telemetry_session_state_outcome::accepted);
	CHECK(session_state.pending_control_drops == 0U);
	const auto *gap = find_record(telemetry_record_kind::coverage_gap, 0U);
	CHECK(gap && gap->payload.gap.dropped_records == missing);
	CHECK(gap->payload.gap.reason == telemetry_gap_reason::control_queue_drop);
	CHECK(handoff.last_checkpoint_revision == 1U);
	CHECK(handoff.cumulative.connected_usec == 10U);
	CHECK(handoff.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP);
	check_records_valid();
	reset_fixture({ 918U, 1U }, 2U, 1000U);
	telemetry_session_resume resume{};
	resume.handoff = handoff;
	resume.entry = make_enter({ 918U, 1U }, 1U, 1U, 0U, 9000);
	resume.entry.session = handoff.session;
	resume.entry.quality_flags = handoff.quality_flags;
	CHECK(telemetry_session_state_resume(&session_state, resume).outcome ==
	      telemetry_session_state_outcome::accepted);
	set_clock(5U, 9005);
	const auto checkpoint = telemetry_session_state_checkpoint(&session_state, handoff.session);
	CHECK(checkpoint.cumulative.connected_usec == 15U);
	CHECK(checkpoint.revision == 2U);
	CHECK(checkpoint.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP);
	check_records_valid();
}

void test_utc_jumps_and_midnight()
{
	const telemetry_producer_id producer = { 919U, 1U };
	const telemetry_utc_usec midnight = INT64_C(86400000000);
	reset_fixture(producer, 2U, 1000U);
	const auto enter = make_enter(producer, 1U, 1U, 0U, midnight - 10);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(&session_state,
						      make_counter(enter.session, enter.connection,
								   20U, 20U, 20U, 0U, 0U))
		      .outcome == telemetry_session_state_outcome::accepted);
	// A classifier update has no UTC sample: compare against the last paired cut.
	auto cp = telemetry_session_state_checkpoint_at(&session_state, enter.session, 20U,
							midnight + 10);
	CHECK(cp.cumulative.connected_usec == 20U);
	CHECK((cp.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) == 0U);
	cp = telemetry_session_state_checkpoint_at(&session_state, enter.session, 30U,
						   midnight + 10000020);
	CHECK(cp.cumulative.connected_usec == 30U);
	CHECK(cp.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY);
	reset_fixture(producer, 2U, 1000U);
	CHECK(telemetry_session_state_enter(&session_state, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	cp = telemetry_session_state_checkpoint_at(&session_state, enter.session, 10U,
						   midnight - 100);
	CHECK(cp.cumulative.connected_usec == 10U);
	CHECK(cp.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY);
	reset_fixture(producer, 2U, 1000U);
	auto extreme = enter;
	extreme.at_utc_usec = INT64_MIN + 1;
	CHECK(telemetry_session_state_enter(&session_state, extreme).outcome ==
	      telemetry_session_state_outcome::accepted);
	cp = telemetry_session_state_checkpoint_at(&session_state, enter.session, 1U, INT64_MAX);
	CHECK(cp.cumulative.connected_usec == 1U);
	CHECK(cp.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY);
	check_records_valid();
}

} // namespace

int main()
{
	test_utc_jumps_and_midnight();
	test_connection_identity_replay();
	test_resume_conflicts();
	test_dropped_control_copyover_gate();
	test_counter_time_authority_and_atomicity();
	test_loss_scope_and_closed_checkpoint();
	test_normal_checkpoint_and_shared_allocator();
	test_detach_reconnect_exit_and_idempotency();
	test_dropped_control_recovers_absolute_total();
	test_copyover_uses_new_clock_and_producer();
	test_absent_handoff_is_unclosed_without_tail();
	test_capacity_retirement_and_failures();
	test_numeric_boundaries();
	std::puts("telemetry session state harness passed");
	return 0;
}
