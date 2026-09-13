#include "telemetry/telemetry_session.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

/* The direct fixture executable must remain allocation-free like the service. */
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

enum class golden_call_kind : std::uint8_t
{
	enter,
	counter,
	transition,
	checkpoint,
	handoff_copy,
	resume,
};

struct golden_call
{
	golden_call_kind kind;
	std::uint8_t process;
	telemetry_session_enter enter;
	telemetry_counter_update counter;
	telemetry_connection_transition transition;
	telemetry_session_ref session;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
};

struct golden_expected
{
	telemetry_session_ref session;
	telemetry_connection_id final_connection;
	telemetry_cumulative_counters cumulative;
	telemetry_producer_id old_producer;
	telemetry_producer_id new_producer;
	telemetry_cumulative_counters handoff_cumulative;
	telemetry_checkpoint_revision handoff_revision;
	telemetry_checkpoint_revision final_revision;
	std::uint8_t has_handoff;
};

struct golden_fixture
{
	const char *id;
	const golden_call *calls;
	std::size_t call_count;
	golden_expected expected;
};

constexpr telemetry_producer_id make_producer(std::uint64_t boot_id, std::uint64_t process_id)
{
	return { boot_id, process_id };
}

constexpr telemetry_session_ref make_session(std::uint64_t boot_id, std::uint64_t process_id,
					     std::uint64_t session_seq, std::uint64_t subject_id,
					     telemetry_pid pid, std::uint64_t season_id,
					     std::uint64_t environment_id)
{
	return { { make_producer(boot_id, process_id), session_seq },
		 subject_id,
		 pid,
		 season_id,
		 environment_id };
}

constexpr telemetry_connection_id make_connection(std::uint64_t boot_id, std::uint64_t process_id,
						  std::uint64_t connection_seq)
{
	return { make_producer(boot_id, process_id), connection_seq };
}

constexpr telemetry_cumulative_counters
make_cumulative(telemetry_duration_usec connected_usec, telemetry_duration_usec active_usec,
		telemetry_duration_usec idle_usec, telemetry_duration_usec unknown_usec,
		telemetry_duration_usec resident_usec, telemetry_duration_usec linkdead_usec)
{
	return {
		connected_usec, active_usec, idle_usec, unknown_usec, resident_usec, linkdead_usec
	};
}

constexpr telemetry_dimensions make_dimensions(std::uint16_t level_band, std::uint16_t class_id,
					       std::uint16_t race_id, std::uint16_t faction_id,
					       std::int32_t zone_vnum, std::uint32_t group_size)
{
	return { level_band, class_id, race_id, faction_id, zone_vnum, group_size };
}

constexpr telemetry_session_enter
make_enter(std::uint64_t session_boot_id, std::uint64_t session_process_id,
	   std::uint64_t session_seq, std::uint64_t subject_id, telemetry_pid pid,
	   std::uint64_t season_id, std::uint64_t environment_id, std::uint64_t connection_boot_id,
	   std::uint64_t connection_process_id, std::uint64_t connection_seq,
	   telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	   std::uint16_t level_band, std::uint16_t class_id, std::uint16_t race_id,
	   std::uint16_t faction_id, std::int32_t zone_vnum, std::uint32_t group_size,
	   telemetry_config_id config_id, std::uint32_t classifier_version,
	   std::uint32_t policy_version, telemetry_quality_mask quality_flags)
{
	telemetry_session_enter enter{};
	enter.session = make_session(session_boot_id, session_process_id, session_seq, subject_id,
				     pid, season_id, environment_id);
	enter.connection =
		make_connection(connection_boot_id, connection_process_id, connection_seq);
	enter.at_monotonic_usec = at_monotonic_usec;
	enter.at_utc_usec = at_utc_usec;
	enter.dimensions =
		make_dimensions(level_band, class_id, race_id, faction_id, zone_vnum, group_size);
	enter.config_id = config_id;
	enter.classifier_version = classifier_version;
	enter.policy_version = policy_version;
	enter.quality_flags = quality_flags;
	return enter;
}

constexpr telemetry_counter_update make_counter(
	std::uint64_t session_boot_id, std::uint64_t session_process_id, std::uint64_t session_seq,
	std::uint64_t subject_id, telemetry_pid pid, std::uint64_t season_id,
	std::uint64_t environment_id, std::uint64_t connection_boot_id,
	std::uint64_t connection_process_id, std::uint64_t connection_seq,
	telemetry_monotonic_usec at_monotonic_usec, telemetry_duration_usec connected_delta_usec,
	telemetry_duration_usec active_delta_usec, telemetry_duration_usec idle_delta_usec,
	telemetry_duration_usec unknown_delta_usec, telemetry_duration_usec resident_delta_usec,
	telemetry_duration_usec linkdead_delta_usec, telemetry_quality_mask quality_flags)
{
	telemetry_counter_update update{};
	update.session = make_session(session_boot_id, session_process_id, session_seq, subject_id,
				      pid, season_id, environment_id);
	update.connection =
		make_connection(connection_boot_id, connection_process_id, connection_seq);
	update.at_monotonic_usec = at_monotonic_usec;
	update.connected_delta_usec = connected_delta_usec;
	update.active_delta_usec = active_delta_usec;
	update.idle_delta_usec = idle_delta_usec;
	update.unknown_delta_usec = unknown_delta_usec;
	update.resident_delta_usec = resident_delta_usec;
	update.linkdead_delta_usec = linkdead_delta_usec;
	update.quality_flags = quality_flags;
	return update;
}

constexpr telemetry_connection_transition
make_transition(std::uint64_t session_boot_id, std::uint64_t session_process_id,
		std::uint64_t session_seq, std::uint64_t subject_id, telemetry_pid pid,
		std::uint64_t season_id, std::uint64_t environment_id,
		std::uint64_t connection_boot_id, std::uint64_t connection_process_id,
		std::uint64_t connection_seq, telemetry_monotonic_usec at_monotonic_usec,
		telemetry_utc_usec at_utc_usec, telemetry_connection_transition_kind kind,
		telemetry_quality_mask quality_flags)
{
	telemetry_connection_transition transition{};
	transition.session = make_session(session_boot_id, session_process_id, session_seq,
					  subject_id, pid, season_id, environment_id);
	transition.connection =
		make_connection(connection_boot_id, connection_process_id, connection_seq);
	transition.at_monotonic_usec = at_monotonic_usec;
	transition.at_utc_usec = at_utc_usec;
	transition.kind = kind;
	transition.quality_flags = quality_flags;
	return transition;
}

constexpr golden_call make_enter_call(std::uint8_t process, telemetry_session_enter enter)
{
	return { golden_call_kind::enter, process, enter, {}, {}, {}, 0U, TELEMETRY_UTC_UNKNOWN };
}

constexpr golden_call make_counter_call(std::uint8_t process, telemetry_counter_update counter)
{
	return {
		golden_call_kind::counter, process, {}, counter, {}, {}, 0U, TELEMETRY_UTC_UNKNOWN
	};
}

constexpr golden_call make_transition_call(std::uint8_t process,
					   telemetry_connection_transition transition)
{
	return { golden_call_kind::transition, process, {}, {}, transition, {}, 0U,
		 TELEMETRY_UTC_UNKNOWN };
}

constexpr golden_call make_checkpoint_call(std::uint8_t process, telemetry_session_ref session,
					   telemetry_monotonic_usec at_monotonic_usec,
					   telemetry_utc_usec at_utc_usec)
{
	return { golden_call_kind::checkpoint,
		 process,
		 {},
		 {},
		 {},
		 session,
		 at_monotonic_usec,
		 at_utc_usec };
}

constexpr golden_call make_handoff_call(std::uint8_t process, telemetry_session_ref session,
					telemetry_monotonic_usec at_monotonic_usec,
					telemetry_utc_usec at_utc_usec)
{
	return { golden_call_kind::handoff_copy,
		 process,
		 {},
		 {},
		 {},
		 session,
		 at_monotonic_usec,
		 at_utc_usec };
}

constexpr golden_call make_resume_call(std::uint8_t process, telemetry_session_enter enter)
{
	return { golden_call_kind::resume, process, enter, {}, {}, {}, 0U, TELEMETRY_UTC_UNKNOWN };
}

#include "telemetry_session_state_golden_fixture.inc"

struct test_clock
{
	telemetry_monotonic_usec monotonic_usec;
	telemetry_utc_usec utc_usec;
	bool available;
};

struct finite_sink
{
	telemetry_record records[128];
	std::size_t count;
	std::size_t capacity;
};

struct key_space
{
	telemetry_producer_id producer;
	telemetry_record_sequence next_sequence;
};

struct golden_runtime
{
	test_clock clock;
	finite_sink sink;
	key_space keys;
	telemetry_session_state states[2];
	telemetry_session_handoff handoff;
	std::uint8_t initialized[2];
};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry golden harness failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool producer_equal(const telemetry_producer_id &left, const telemetry_producer_id &right)
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool session_equal(const telemetry_session_ref &left, const telemetry_session_ref &right)
{
	return producer_equal(left.id.producer, right.id.producer) &&
	       left.id.session_seq == right.id.session_seq && left.subject_id == right.subject_id &&
	       left.pid == right.pid && left.season_id == right.season_id &&
	       left.environment_id == right.environment_id;
}

bool connection_equal(const telemetry_connection_id &left, const telemetry_connection_id &right)
{
	return producer_equal(left.producer, right.producer) &&
	       left.connection_seq == right.connection_seq;
}

bool counters_equal(const telemetry_cumulative_counters &left,
		    const telemetry_cumulative_counters &right)
{
	return left.connected_usec == right.connected_usec &&
	       left.active_usec == right.active_usec && left.idle_usec == right.idle_usec &&
	       left.unknown_usec == right.unknown_usec &&
	       left.resident_usec == right.resident_usec &&
	       left.linkdead_usec == right.linkdead_usec;
}

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
	if (sink->count >= sink->capacity)
		return false;
	sink->records[sink->count++] = *record;
	return true;
}

bool next_key(void *context, telemetry_record_kind, telemetry_record_key *key) noexcept
{
	auto *space = static_cast<key_space *>(context);
	if (space->next_sequence == 0U)
		return false;
	*key = { space->producer, space->next_sequence };
	++space->next_sequence;
	return true;
}

void set_clock(golden_runtime &runtime, telemetry_monotonic_usec monotonic_usec,
	       telemetry_utc_usec utc_usec)
{
	runtime.clock = { monotonic_usec, utc_usec, true };
}

void init_process(golden_runtime &runtime, std::uint8_t process,
		  const telemetry_producer_id &producer)
{
	runtime.clock = { 0U, TELEMETRY_UTC_UNKNOWN, true };
	runtime.sink = {};
	runtime.sink.capacity = sizeof(runtime.sink.records) / sizeof(runtime.sink.records[0]);
	runtime.keys = { producer, 1U };
	telemetry_session_state_config config{};
	config.max_slots = 4U;
	config.detached_retire_after_usec = 1000U;
	config.producer = producer;
	config.clock = { clock_now, &runtime.clock };
	config.sink = { sink_emit, &runtime.sink };
	config.key_allocator = { next_key, &runtime.keys };
	CHECK(telemetry_session_state_init(&runtime.states[process], &config) ==
	      telemetry_session_state_outcome::accepted);
	runtime.initialized[process] = 1U;
}

const telemetry_record *last_checkpoint(const finite_sink &sink)
{
	const telemetry_record *found = nullptr;
	for (std::size_t index = 0U; index < sink.count; ++index)
		if (sink.records[index].header.kind == telemetry_record_kind::session_checkpoint)
			found = &sink.records[index];
	return found;
}

void check_records_valid(const finite_sink &sink)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
		CHECK(telemetry_record_is_valid(sink.records[index]));
}

void run_fixture(const golden_fixture &fixture)
{
	CHECK(fixture.call_count > 0U);
	CHECK(fixture.calls[0].kind == golden_call_kind::enter);
	golden_runtime runtime{};
	init_process(runtime, 0U, fixture.calls[0].enter.connection.producer);
	bool saw_handoff = false;
	bool saw_checkpoint = false;
	std::uint8_t last_process = 0U;

	for (std::size_t index = 0U; index < fixture.call_count; ++index)
	{
		const golden_call &call = fixture.calls[index];
		CHECK(call.process < 2U);
		if (runtime.initialized[call.process] == 0U)
		{
			CHECK(call.kind == golden_call_kind::resume);
			init_process(runtime, call.process, call.enter.connection.producer);
		}
		telemetry_session_state &state = runtime.states[call.process];
		telemetry_session_state_result result{};
		switch (call.kind)
		{
		case golden_call_kind::enter:
			result = telemetry_session_state_enter(&state, call.enter);
			break;
		case golden_call_kind::counter:
			result = telemetry_session_state_update_counters(&state, call.counter);
			break;
		case golden_call_kind::transition:
			result = telemetry_session_state_transition(&state, call.transition);
			break;
		case golden_call_kind::checkpoint:
			set_clock(runtime, call.at_monotonic_usec, call.at_utc_usec);
			result = telemetry_session_state_checkpoint(&state, call.session);
			saw_checkpoint = true;
			break;
		case golden_call_kind::handoff_copy:
			set_clock(runtime, call.at_monotonic_usec, call.at_utc_usec);
			result = telemetry_session_state_handoff_copy(&state, call.session,
								      &runtime.handoff);
			CHECK(fixture.expected.has_handoff != 0U);
			CHECK(session_equal(runtime.handoff.session, fixture.expected.session));
			CHECK(producer_equal(runtime.handoff.previous_producer,
					     fixture.expected.old_producer));
			CHECK(runtime.handoff.last_checkpoint_revision ==
			      fixture.expected.handoff_revision);
			CHECK(counters_equal(runtime.handoff.cumulative,
					     fixture.expected.handoff_cumulative));
			saw_handoff = true;
			break;
		case golden_call_kind::resume:
		{
			telemetry_session_resume resume{};
			resume.handoff = runtime.handoff;
			resume.entry = call.enter;
			result = telemetry_session_state_resume(&state, resume);
			break;
		}
		}
		CHECK(result.outcome == telemetry_session_state_outcome::accepted);
		last_process = call.process;
	}

	CHECK(saw_handoff == (fixture.expected.has_handoff != 0U));
	CHECK(saw_checkpoint);
	telemetry_session_state_view view{};
	CHECK(telemetry_session_state_copy_view(&runtime.states[last_process],
						fixture.expected.session, &view));
	CHECK(session_equal(view.session, fixture.expected.session));
	CHECK(connection_equal(view.connection, fixture.expected.final_connection));
	CHECK(counters_equal(view.cumulative, fixture.expected.cumulative));
	CHECK(view.last_allocated_revision == fixture.expected.final_revision);
	const telemetry_record *checkpoint = last_checkpoint(runtime.sink);
	CHECK(checkpoint != nullptr);
	CHECK(session_equal(checkpoint->payload.checkpoint.session, fixture.expected.session));
	CHECK(connection_equal(checkpoint->payload.checkpoint.connection,
			       fixture.expected.final_connection));
	CHECK(counters_equal(checkpoint->payload.checkpoint.cumulative,
			     fixture.expected.cumulative));
	CHECK(checkpoint->payload.checkpoint.revision == fixture.expected.final_revision);
	CHECK(producer_equal(checkpoint->header.key.producer,
			     last_process == 0U ? fixture.expected.old_producer :
						  fixture.expected.new_producer));
	check_records_valid(runtime.sink);
}

} // namespace

int main()
{
	for (std::size_t index = 0U; index < GOLDEN_FIXTURE_COUNT; ++index)
		run_fixture(GOLDEN_FIXTURES[index]);
	std::puts("telemetry session state golden harness passed");
	return 0;
}
