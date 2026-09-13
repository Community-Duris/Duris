#include "telemetry/telemetry_activity.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

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

struct activity_b_golden_interval
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_time_window window;
	telemetry_duration_usec duration_usec;
	telemetry_interval_category category;
	telemetry_activity_context context;
	telemetry_context_quality context_quality;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

struct activity_b_golden_fact
{
	const char *id;
	telemetry_session_enter enter;
	telemetry_activity_evidence evidence[2];
	telemetry_activity_context_snapshot context[2];
	telemetry_connection_transition transitions[2];
	telemetry_monotonic_usec pulses[3];
	telemetry_utc_usec pulse_utc[3];
	std::uint8_t pulse_count;
	activity_b_golden_interval intervals[4];
	std::uint8_t interval_count;
	telemetry_cumulative_counters cumulative;
	telemetry_connection_id final_connection;
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

constexpr telemetry_dimensions make_dimensions(std::uint16_t level_band, std::uint16_t class_id,
					       std::uint16_t race_id, std::uint16_t faction_id,
					       std::int32_t zone_vnum, std::uint32_t group_size)
{
	return { level_band, class_id, race_id, faction_id, zone_vnum, group_size };
}

constexpr telemetry_session_enter
make_enter(telemetry_session_ref session, telemetry_connection_id connection,
	   telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	   telemetry_dimensions dimensions, telemetry_config_id config_id,
	   std::uint32_t classifier_version, std::uint32_t policy_version,
	   telemetry_quality_mask quality_flags)
{
	telemetry_session_enter enter{};
	enter.session = session;
	enter.connection = connection;
	enter.at_monotonic_usec = at_monotonic_usec;
	enter.at_utc_usec = at_utc_usec;
	enter.dimensions = dimensions;
	enter.config_id = config_id;
	enter.classifier_version = classifier_version;
	enter.policy_version = policy_version;
	enter.quality_flags = quality_flags;
	return enter;
}

constexpr telemetry_activity_evidence
make_evidence(telemetry_session_ref session, telemetry_connection_id connection,
	      telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	      telemetry_activity_evidence_kind kind, telemetry_quality_mask quality_flags = 0U)
{
	telemetry_activity_evidence evidence{};
	evidence.session = session;
	evidence.connection = connection;
	evidence.at_monotonic_usec = at_monotonic_usec;
	evidence.at_utc_usec = at_utc_usec;
	evidence.kind = kind;
	evidence.quality_flags = quality_flags;
	return evidence;
}

constexpr telemetry_activity_context_snapshot
make_context(telemetry_session_ref session, telemetry_connection_id connection,
	     telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
	     telemetry_dimensions dimensions, std::uint32_t context_flags,
	     telemetry_config_id config_id, std::uint32_t classifier_version,
	     std::uint32_t policy_version, telemetry_quality_mask quality_flags = 0U)
{
	telemetry_activity_context_snapshot snapshot{};
	snapshot.session = session;
	snapshot.connection = connection;
	snapshot.at_monotonic_usec = at_monotonic_usec;
	snapshot.at_utc_usec = at_utc_usec;
	snapshot.dimensions = dimensions;
	snapshot.context_flags = context_flags;
	snapshot.config_id = config_id;
	snapshot.classifier_version = classifier_version;
	snapshot.policy_version = policy_version;
	snapshot.quality_flags = quality_flags;
	return snapshot;
}

constexpr telemetry_connection_transition
make_transition(telemetry_session_ref session, telemetry_connection_id connection,
		telemetry_monotonic_usec at_monotonic_usec, telemetry_utc_usec at_utc_usec,
		telemetry_connection_transition_kind kind,
		telemetry_quality_mask quality_flags = 0U)
{
	telemetry_connection_transition transition{};
	transition.session = session;
	transition.connection = connection;
	transition.at_monotonic_usec = at_monotonic_usec;
	transition.at_utc_usec = at_utc_usec;
	transition.kind = kind;
	transition.quality_flags = quality_flags;
	return transition;
}

constexpr activity_b_golden_interval
make_expected_interval(telemetry_session_ref session, telemetry_connection_id connection,
		       telemetry_time_window window, telemetry_duration_usec duration_usec,
		       telemetry_interval_category category, telemetry_activity_context context,
		       telemetry_context_quality context_quality, telemetry_dimensions dimensions,
		       telemetry_config_id config_id, std::uint32_t classifier_version,
		       std::uint32_t policy_version, telemetry_quality_mask quality_flags)
{
	return { session,	 connection,	  window,     duration_usec, category,
		 context,	 context_quality, dimensions, config_id,     classifier_version,
		 policy_version, quality_flags };
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

#include "telemetry_activity_b_golden_fixture.inc"

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
	telemetry_activity_state state;
};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry activity B golden failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool producer_equal(const telemetry_producer_id &left, const telemetry_producer_id &right) noexcept
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool session_equal(const telemetry_session_ref &left, const telemetry_session_ref &right) noexcept
{
	return producer_equal(left.id.producer, right.id.producer) &&
	       left.id.session_seq == right.id.session_seq && left.subject_id == right.subject_id &&
	       left.pid == right.pid && left.season_id == right.season_id &&
	       left.environment_id == right.environment_id;
}

bool connection_equal(const telemetry_connection_id &left,
		      const telemetry_connection_id &right) noexcept
{
	return producer_equal(left.producer, right.producer) &&
	       left.connection_seq == right.connection_seq;
}

bool dimensions_equal(const telemetry_dimensions &left, const telemetry_dimensions &right) noexcept
{
	return left.level_band == right.level_band && left.class_id == right.class_id &&
	       left.race_id == right.race_id && left.faction_id == right.faction_id &&
	       left.zone_vnum == right.zone_vnum && left.group_size == right.group_size;
}

bool counters_equal(const telemetry_cumulative_counters &left,
		    const telemetry_cumulative_counters &right) noexcept
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
	if (sink->count >= sizeof(sink->records) / sizeof(sink->records[0]))
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

telemetry_config_snapshot make_config(telemetry_config_id config_id,
				      telemetry_utc_usec effective_utc_usec)
{
	telemetry_config_snapshot config{};
	config.schema_version = TELEMETRY_SCHEMA_VERSION;
	config.config_id = config_id;
	config.revision = 1U;
	config.build_version = 100U;
	config.content_version = 200U;
	config.property_version = 300U;
	config.classifier_version = 400U;
	config.policy_version = 500U;
	config.season_id = 7U;
	config.environment_id = 8U;
	config.fingerprint[0] = static_cast<std::uint8_t>(config_id & 0xffU);
	config.effective_utc_usec = effective_utc_usec;
	config.interval_usec = 100U;
	config.checkpoint_interval_usec = 100U;
	config.active_window_usec = 1000U;
	config.context_segments_per_minute = 8U;
	config.pulse_slot_count = 1U;
	config.backend = telemetry_storage_backend::sql;
	config.enabled = 1U;
	return config;
}

void init_runtime(golden_runtime &runtime, const activity_b_golden_fact &fact)
{
	runtime = {};
	runtime.clock = { 0U, TELEMETRY_UTC_UNKNOWN, true };
	runtime.keys = { fact.enter.connection.producer, 1U };
	telemetry_activity_state_config config{};
	config.max_slots = 4U;
	config.pulse_slot_count = 1U;
	config.context_segments_per_minute = 8U;
	config.interval_usec = 100U;
	config.active_window_usec = 1000U;
	config.detached_retire_after_usec = 1000U;
	config.producer = fact.enter.connection.producer;
	config.clock = { clock_now, &runtime.clock };
	config.sink = { sink_emit, &runtime.sink };
	config.key_allocator = { next_key, &runtime.keys };
	CHECK(telemetry_activity_state_init(&runtime.state, &config) ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_publish_config(
		      &runtime.state, make_config(fact.enter.config_id, fact.enter.at_utc_usec))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_enter(&runtime.state, fact.enter).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(&runtime.state, fact.evidence[0]).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(&runtime.state, fact.context[0]).outcome ==
	      telemetry_activity_outcome::accepted);
}

std::size_t interval_count(const finite_sink &sink)
{
	std::size_t count = 0U;
	for (std::size_t index = 0U; index < sink.count; ++index)
		if (sink.records[index].header.kind == telemetry_record_kind::interval)
			++count;
	return count;
}

const telemetry_record *interval_at(const finite_sink &sink, std::size_t occurrence)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
	{
		if (sink.records[index].header.kind != telemetry_record_kind::interval)
			continue;
		if (occurrence == 0U)
			return &sink.records[index];
		--occurrence;
	}
	return nullptr;
}

void compare_interval(const telemetry_record &record, const activity_b_golden_interval &expected)
{
	CHECK(record.header.kind == telemetry_record_kind::interval);
	const telemetry_interval_payload &actual = record.payload.interval;
	CHECK(session_equal(actual.session, expected.session));
	CHECK(connection_equal(actual.connection, expected.connection));
	CHECK(actual.window.start_monotonic_usec == expected.window.start_monotonic_usec);
	CHECK(actual.window.end_monotonic_usec == expected.window.end_monotonic_usec);
	CHECK(actual.window.start_utc_usec == expected.window.start_utc_usec);
	CHECK(actual.window.end_utc_usec == expected.window.end_utc_usec);
	CHECK(actual.duration_usec == expected.duration_usec);
	CHECK(actual.category == expected.category);
	CHECK(actual.context == expected.context);
	CHECK(actual.context_quality == expected.context_quality);
	CHECK(dimensions_equal(actual.dimensions, expected.dimensions));
	CHECK(actual.config_id == expected.config_id);
	CHECK(actual.classifier_version == expected.classifier_version);
	CHECK(actual.policy_version == expected.policy_version);
	CHECK(actual.quality_flags == expected.quality_flags);
}

void check_records_valid(const finite_sink &sink)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
		CHECK(telemetry_record_is_valid(sink.records[index]));
}

void run_fixture(const activity_b_golden_fact &fact)
{
	golden_runtime runtime{};
	init_runtime(runtime, fact);
	for (std::size_t pulse = 0U; pulse < fact.pulse_count; ++pulse)
	{
		if (pulse == 1U &&
		    fact.transitions[0].kind != telemetry_connection_transition_kind::attached)
		{
			CHECK(telemetry_activity_state_transition(&runtime.state,
								  fact.transitions[0])
				      .outcome == telemetry_activity_outcome::accepted);
		}
		if (pulse == 2U)
		{
			CHECK(telemetry_activity_state_transition(&runtime.state,
								  fact.transitions[1])
				      .outcome == telemetry_activity_outcome::accepted);
			CHECK(telemetry_activity_state_record_evidence(&runtime.state,
								       fact.evidence[1])
				      .outcome == telemetry_activity_outcome::accepted);
			CHECK(telemetry_activity_state_update_context(&runtime.state,
								      fact.context[1])
				      .outcome == telemetry_activity_outcome::accepted);
		}
		telemetry_counter_update deltas[TELEMETRY_ACTIVITY_STATE_MAX_SLOTS]{};
		telemetry_activity_pulse_request request{};
		request.deltas = deltas;
		request.delta_capacity = TELEMETRY_ACTIVITY_STATE_MAX_SLOTS;
		request.now_monotonic_usec = fact.pulses[pulse];
		request.occurrence_utc_usec = fact.pulse_utc[pulse];
		request.slot = 0U;
		if (pulse + 1U == fact.pulse_count &&
		    fact.transitions[0].kind == telemetry_connection_transition_kind::detached)
		{
			const telemetry_activity_result result = telemetry_activity_state_flush_at(
				&runtime.state, fact.enter.session, fact.pulses[pulse],
				fact.pulse_utc[pulse]);
			CHECK(result.outcome == telemetry_activity_outcome::accepted);
		}
		else
		{
			const telemetry_activity_pulse_result result =
				telemetry_activity_state_pulse(&runtime.state, request);
			CHECK(result.outcome == telemetry_activity_outcome::accepted);
		}
	}
	CHECK(interval_count(runtime.sink) == fact.interval_count);
	for (std::size_t index = 0U; index < fact.interval_count; ++index)
		compare_interval(*interval_at(runtime.sink, index), fact.intervals[index]);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&runtime.state, fact.enter.session, &view));
	CHECK(counters_equal(view.cumulative, fact.cumulative));
	CHECK(connection_equal(view.connection, fact.final_connection));
	check_records_valid(runtime.sink);
}

} // namespace

int main()
{
	for (std::size_t index = 0U; index < ACTIVITY_B_GOLDEN_FACT_COUNT; ++index)
		run_fixture(ACTIVITY_B_GOLDEN_FACTS[index]);
	std::puts("telemetry activity B golden harness passed");
	return 0;
}
