#include "telemetry/telemetry_activity.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

/* The pure module must not allocate on a gameplay capture path. */
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

constexpr telemetry_utc_usec DAY_USEC = 86'400'000'000LL;

struct test_clock
{
	telemetry_monotonic_usec monotonic_usec;
	telemetry_utc_usec utc_usec;
	bool available;
};

struct finite_sink
{
	telemetry_record records[1024];
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

struct runtime
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
		std::fprintf(stderr, "telemetry activity harness failure at line %d: %s\n", line,
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
	if (space->fail || space->next_sequence == 0U ||
	    space->next_sequence == std::numeric_limits<telemetry_record_sequence>::max())
		return false;
	*key = { space->producer, space->next_sequence };
	++space->next_sequence;
	return true;
}

telemetry_config_snapshot make_config(telemetry_config_id id,
				      telemetry_duration_usec interval_usec = 60U,
				      telemetry_duration_usec active_window_usec = 300U,
				      std::uint32_t segments = 8U)
{
	telemetry_config_snapshot config{};
	config.schema_version = TELEMETRY_SCHEMA_VERSION;
	config.config_id = id;
	config.revision = id;
	config.build_version = 100U;
	config.content_version = 200U;
	config.property_version = 300U + static_cast<std::uint32_t>(id);
	config.classifier_version = 400U;
	config.policy_version = 500U;
	config.season_id = 7U;
	config.environment_id = 8U;
	config.fingerprint[0] = static_cast<std::uint8_t>(id == 0U ? 1U : id);
	config.effective_utc_usec = 1'000'000;
	config.interval_usec = interval_usec;
	config.checkpoint_interval_usec = interval_usec;
	config.active_window_usec = active_window_usec;
	config.context_segments_per_minute = segments;
	config.pulse_slot_count = 1U;
	config.backend = telemetry_storage_backend::sql;
	config.enabled = 1U;
	return config;
}

telemetry_session_enter make_enter(telemetry_producer_id producer,
				   telemetry_session_sequence session_sequence,
				   telemetry_connection_sequence connection_sequence,
				   telemetry_monotonic_usec monotonic_usec,
				   telemetry_utc_usec utc_usec, telemetry_config_id config_id)
{
	telemetry_session_enter enter{};
	enter.session = { { producer, session_sequence },
			  9000U + session_sequence,
			  40 + static_cast<telemetry_pid>(session_sequence),
			  7U,
			  8U };
	enter.connection = { producer, connection_sequence };
	enter.at_monotonic_usec = monotonic_usec;
	enter.at_utc_usec = utc_usec;
	enter.dimensions = { 1U, 1U, 1U, 1U, 100, 1U };
	enter.config_id = config_id;
	enter.classifier_version = 400U;
	enter.policy_version = 500U;
	return enter;
}

telemetry_activity_evidence make_evidence(const telemetry_session_ref &session,
					  const telemetry_connection_id &connection,
					  telemetry_monotonic_usec monotonic_usec,
					  telemetry_utc_usec utc_usec,
					  telemetry_activity_evidence_kind kind)
{
	telemetry_activity_evidence evidence{};
	evidence.session = session;
	evidence.connection = connection;
	evidence.at_monotonic_usec = monotonic_usec;
	evidence.at_utc_usec = utc_usec;
	evidence.kind = kind;
	return evidence;
}

telemetry_activity_context_snapshot
make_context(const telemetry_session_ref &session, const telemetry_connection_id &connection,
	     telemetry_monotonic_usec monotonic_usec, telemetry_utc_usec utc_usec,
	     std::uint32_t flags, telemetry_config_id config_id, std::int32_t zone = 100,
	     std::uint16_t level_band = 1U)
{
	telemetry_activity_context_snapshot snapshot{};
	snapshot.session = session;
	snapshot.connection = connection;
	snapshot.at_monotonic_usec = monotonic_usec;
	snapshot.at_utc_usec = utc_usec;
	snapshot.dimensions = { level_band, 1U, 1U, 1U, zone, 1U };
	snapshot.context_flags = flags;
	snapshot.config_id = config_id;
	snapshot.classifier_version = 400U;
	snapshot.policy_version = 500U;
	return snapshot;
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

void set_clock(runtime &fixture, telemetry_monotonic_usec monotonic_usec,
	       telemetry_utc_usec utc_usec)
{
	fixture.clock = { monotonic_usec, utc_usec, true };
}

void reset_runtime(runtime &fixture, telemetry_producer_id producer, std::uint16_t max_slots = 4U,
		   telemetry_duration_usec interval_usec = 60U,
		   telemetry_duration_usec active_window_usec = 300U, std::uint32_t segments = 8U,
		   telemetry_duration_usec retire_after_usec = 3'600'000'000ULL)
{
	fixture = {};
	fixture.clock = { 0U, TELEMETRY_UTC_UNKNOWN, true };
	fixture.sink.capacity = sizeof(fixture.sink.records) / sizeof(fixture.sink.records[0]);
	fixture.keys = { producer, 1U, false };
	telemetry_activity_state_config config{};
	config.max_slots = max_slots;
	config.context_segments_per_minute = segments;
	config.interval_usec = interval_usec;
	config.active_window_usec = active_window_usec;
	config.detached_retire_after_usec = retire_after_usec;
	config.producer = producer;
	config.clock = { clock_now, &fixture.clock };
	config.sink = { sink_emit, &fixture.sink };
	config.key_allocator = { next_key, &fixture.keys };
	CHECK(telemetry_activity_state_init(&fixture.state, &config) ==
	      telemetry_activity_outcome::accepted);
}

telemetry_session_enter start_session(runtime &fixture, telemetry_config_id config_id = 100U,
				      telemetry_monotonic_usec monotonic_usec = 0U,
				      telemetry_utc_usec utc_usec = 1'000'000)
{
	const telemetry_session_enter enter =
		make_enter(fixture.keys.producer, 1U, 1U, monotonic_usec, utc_usec, config_id);
	CHECK(telemetry_activity_state_publish_config(
		      &fixture.state, make_config(config_id, fixture.state.interval_usec,
						  fixture.state.active_window_usec,
						  fixture.state.context_segments_per_minute))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_enter(&fixture.state, enter).outcome ==
	      telemetry_activity_outcome::accepted);
	return enter;
}

telemetry_activity_pulse_result pulse_at(runtime &fixture, telemetry_monotonic_usec monotonic_usec,
					 telemetry_utc_usec utc_usec,
					 telemetry_counter_update *deltas = nullptr,
					 std::uint16_t delta_capacity = 0U)
{
	telemetry_activity_pulse_request request{};
	request.now_monotonic_usec = monotonic_usec;
	request.occurrence_utc_usec = utc_usec;
	request.slot = 0U;
	telemetry_counter_update scratch[TELEMETRY_ACTIVITY_STATE_MAX_SLOTS]{};
	request.deltas = deltas != nullptr ? deltas : scratch;
	request.delta_capacity = deltas != nullptr ? delta_capacity :
						     TELEMETRY_ACTIVITY_STATE_MAX_SLOTS;
	return telemetry_activity_state_pulse(&fixture.state, request);
}

std::size_t count_kind(const finite_sink &sink, telemetry_record_kind kind)
{
	std::size_t count = 0U;
	for (std::size_t index = 0U; index < sink.count; ++index)
		if (sink.records[index].header.kind == kind)
			++count;
	return count;
}

std::size_t count_intervals(const finite_sink &sink)
{
	return count_kind(sink, telemetry_record_kind::interval);
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

const telemetry_record *record_kind_at(const finite_sink &sink, telemetry_record_kind kind,
				       std::size_t occurrence)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
	{
		if (sink.records[index].header.kind != kind)
			continue;
		if (occurrence == 0U)
			return &sink.records[index];
		--occurrence;
	}
	return nullptr;
}

void check_records_valid(const finite_sink &sink)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
		CHECK(telemetry_record_is_valid(sink.records[index]));
}

void test_active_idle_deadline_and_deltas()
{
	runtime fixture{};
	reset_runtime(fixture, { 101U, 201U }, 4U, 60U, 100U);
	const auto enter = start_session(fixture);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 0U, 1'000'000,
						   TELEMETRY_ACTIVITY_CONTEXT_NONE, 100U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	telemetry_counter_update deltas[4]{};
	const auto first_pulse = pulse_at(fixture, 100U, 1'000'100, deltas, 4U);
	CHECK(first_pulse.outcome == telemetry_activity_outcome::accepted);
	CHECK(first_pulse.intervals_sealed == 2U);
	CHECK(first_pulse.deltas_written == 1U);
	CHECK(telemetry_counter_update_is_valid(deltas[0]));
	CHECK(deltas[0].connected_delta_usec == 100U);
	CHECK(deltas[0].active_delta_usec == 100U);
	CHECK(deltas[0].idle_delta_usec == 0U);
	CHECK(count_intervals(fixture.sink) == 2U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.category ==
	      telemetry_interval_category::connected_active);
	CHECK(interval_at(fixture.sink, 1U)->payload.interval.window.start_monotonic_usec == 60U);
	CHECK(interval_at(fixture.sink, 1U)->payload.interval.window.end_monotonic_usec == 100U);
	const auto second_pulse = pulse_at(fixture, 160U, 1'000'160, deltas, 4U);
	CHECK(second_pulse.intervals_sealed == 1U);
	CHECK(deltas[0].idle_delta_usec == 60U);
	CHECK(fixture.state.slots[0].mode == telemetry_activity_accounting_mode::idle);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.cumulative.connected_usec == 160U);
	CHECK(view.cumulative.active_usec == 100U);
	CHECK(view.cumulative.idle_usec == 60U);
	check_records_valid(fixture.sink);
}

void test_explicit_evidence_cases_and_replay()
{
	runtime fixture{};
	reset_runtime(fixture, { 102U, 202U }, 4U, 50U, 100U);
	const auto enter = start_session(fixture);
	CHECK(pulse_at(fixture, 50U, 1'000'050).intervals_sealed == 1U);
	const auto *unknown = interval_at(fixture.sink, 0U);
	CHECK(unknown != nullptr);
	CHECK(unknown->payload.interval.category == telemetry_interval_category::unknown);
	CHECK(unknown->payload.interval.context == telemetry_activity_context::unknown);
	auto following = make_evidence(enter.session, enter.connection, 50U, 1'000'050,
				       telemetry_activity_evidence_kind::following);
	CHECK(telemetry_activity_state_record_evidence(&fixture.state, following).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(&fixture.state, following).outcome ==
	      telemetry_activity_outcome::idempotent);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, enter.connection, 60U, 1'000'060,
						    telemetry_activity_evidence_kind::reading))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, enter.connection, 49U, 1'000'049,
						    telemetry_activity_evidence_kind::reading))
		      .outcome == telemetry_activity_outcome::invalid);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 49U, 1'000'049,
						   TELEMETRY_ACTIVITY_CONTEXT_TRAVEL, 100U))
		      .outcome == telemetry_activity_outcome::invalid);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.category == telemetry_interval_category::connected_active);
	const auto deadline = view.active_deadline_monotonic_usec;
	CHECK(deadline == 160U);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, enter.connection, 70U, 1'000'070,
						    telemetry_activity_evidence_kind::recovery))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 80U, 1'000'080,
				    telemetry_activity_evidence_kind::automatic_combat))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.active_deadline_monotonic_usec == 160U);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, enter.connection, 90U, 1'000'090,
						    telemetry_activity_evidence_kind::spam))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 100U, 1'000'100,
				    telemetry_activity_evidence_kind::afk))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.category == telemetry_interval_category::connected_idle);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 100U, 1'000'100,
						   TELEMETRY_ACTIVITY_CONTEXT_COMBAT |
							   TELEMETRY_ACTIVITY_CONTEXT_SOCIAL,
						   100U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context == telemetry_activity_context::combat);
	CHECK(view.context_quality == telemetry_context_quality::observed);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 110U, 1'000'110,
				    telemetry_activity_evidence_kind::social))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.category == telemetry_interval_category::connected_active);
	check_records_valid(fixture.sink);
}

void test_context_precedence_cap_and_minute_reset()
{
	runtime fixture{};
	reset_runtime(fixture, { 103U, 203U }, 2U, 60U, 1000U, 3U);
	CHECK(telemetry_activity_context_resolve(TELEMETRY_ACTIVITY_CONTEXT_SOCIAL |
						 TELEMETRY_ACTIVITY_CONTEXT_COMBAT) ==
	      telemetry_activity_context::combat);
	CHECK(telemetry_activity_context_resolve(TELEMETRY_ACTIVITY_CONTEXT_OTHER |
						 TELEMETRY_ACTIVITY_CONTEXT_CRAFTING) ==
	      telemetry_activity_context::crafting);
	const auto enter = start_session(fixture);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 0U, 1'000'000,
						   TELEMETRY_ACTIVITY_CONTEXT_NONE, 100U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 10U, 1'000'010,
						   TELEMETRY_ACTIVITY_CONTEXT_COMBAT, 100U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 20U, 1'000'020,
						   TELEMETRY_ACTIVITY_CONTEXT_TRAVEL, 100U))
		      .outcome == telemetry_activity_outcome::accepted);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context_overflow != 0U);
	CHECK(view.context == telemetry_activity_context::overflow_unknown);
	CHECK(view.category == telemetry_interval_category::connected_active);
	CHECK((view.quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW) != 0U);
	CHECK(view.context_segments_used <= 3U);
	CHECK(telemetry_activity_state_publish_config(&fixture.state, make_config(101U)).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 30U, 1'000'030,
						   TELEMETRY_ACTIVITY_CONTEXT_SOCIAL, 101U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context_overflow != 0U);
	CHECK(view.config_id == 101U);
	CHECK(fixture.state.slots[0].segment_cap == 3U);
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, enter.connection, 50U, 1'000'050,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::accepted_degraded);
	const telemetry_connection_id second_connection = { fixture.keys.producer, 2U };
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, second_connection, 60U, 1'000'060,
				      telemetry_connection_transition_kind::attached))
		      .outcome == telemetry_activity_outcome::accepted_degraded);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context_overflow != 0U);
	CHECK(view.connection.connection_seq == 2U);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, second_connection, 61U, 1'000'061,
						   TELEMETRY_ACTIVITY_CONTEXT_OTHER, 101U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context_overflow != 0U);
	CHECK(view.context == telemetry_activity_context::overflow_unknown);
	const auto minute_update = telemetry_activity_state_update_context(
		&fixture.state,
		make_context(enter.session, second_connection, TELEMETRY_ACTIVITY_MINUTE_USEC + 61U,
			     1'000'000 + TELEMETRY_ACTIVITY_MINUTE_USEC + 61U,
			     TELEMETRY_ACTIVITY_CONTEXT_NONE, 101U));
	CHECK(minute_update.outcome == telemetry_activity_outcome::accepted_degraded);
	CHECK(minute_update.quality_flags & TELEMETRY_QUALITY_LATE);
	CHECK(minute_update.intervals_sealed <= TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION);
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.context_overflow == 0U);
	CHECK(view.context == telemetry_activity_context::none);
	CHECK(telemetry_activity_state_stats_copy(&fixture.state).context_overflow_total == 1U);
	check_records_valid(fixture.sink);
}

void test_linkdead_accounting_and_reconnect()
{
	runtime fixture{};
	reset_runtime(fixture, { 104U, 204U }, 2U, 60U, 300U);
	const auto enter = start_session(fixture);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::combat_participation))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, enter.connection, 100U, 1'000'100,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, telemetry_connection_id{}, 100U, 1'000'100,
				    telemetry_activity_evidence_kind::linkdead))
		      .outcome == telemetry_activity_outcome::accepted);
	const telemetry_connection_id connection_two = { fixture.keys.producer, 2U };
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, connection_two, 200U, 1'000'200,
				      telemetry_connection_transition_kind::attached))
		      .outcome == telemetry_activity_outcome::accepted);
	const auto *linkdead = interval_at(fixture.sink, 2U);
	CHECK(linkdead != nullptr);
	CHECK(linkdead->payload.interval.category ==
	      telemetry_interval_category::resident_linkdead);
	CHECK(telemetry_connection_id_is_zero(linkdead->payload.interval.connection));
	CHECK(linkdead->payload.interval.duration_usec == 60U);
	const auto *linkdead_tail = interval_at(fixture.sink, 3U);
	CHECK(linkdead_tail != nullptr);
	CHECK(linkdead_tail->payload.interval.category ==
	      telemetry_interval_category::resident_linkdead);
	CHECK(linkdead_tail->payload.interval.duration_usec == 40U);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, connection_two, 200U, 1'000'200,
						    telemetry_activity_evidence_kind::afk))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(pulse_at(fixture, 260U, 1'000'260).intervals_sealed == 1U);
	const auto *idle = interval_at(fixture.sink, 4U);
	CHECK(idle != nullptr);
	CHECK(idle->payload.interval.category == telemetry_interval_category::connected_idle);
	CHECK(idle->payload.interval.connection.connection_seq == 2U);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.cumulative.connected_usec == 160U);
	CHECK(view.cumulative.active_usec == 100U);
	CHECK(view.cumulative.idle_usec == 60U);
	CHECK(view.cumulative.resident_usec == 260U);
	CHECK(view.cumulative.linkdead_usec == 100U);
	check_records_valid(fixture.sink);
}

void test_drop_gap_and_total_recovery()
{
	runtime fixture{};
	reset_runtime(fixture, { 105U, 205U }, 2U, 60U, 300U);
	const auto enter = start_session(fixture);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	fixture.sink.capacity = fixture.sink.count;
	telemetry_counter_update deltas[4]{};
	const auto dropped = pulse_at(fixture, 60U, 1'000'060, deltas, 4U);
	CHECK(dropped.outcome == telemetry_activity_outcome::sink_rejected);
	CHECK(dropped.deltas_written == 1U);
	CHECK(deltas[0].connected_delta_usec == 60U);
	CHECK(deltas[0].active_delta_usec == 60U);
	CHECK(dropped.records_dropped == 1U);
	fixture.sink.capacity = sizeof(fixture.sink.records) / sizeof(fixture.sink.records[0]);
	const auto recovered = pulse_at(fixture, 120U, 1'000'120, deltas, 4U);
	CHECK(recovered.outcome == telemetry_activity_outcome::accepted);
	const auto *gap = record_kind_at(fixture.sink, telemetry_record_kind::coverage_gap, 0U);
	CHECK(gap != nullptr);
	CHECK(gap->payload.gap.reason == telemetry_gap_reason::detail_queue_drop);
	CHECK(gap->payload.gap.duration_usec == 60U);
	CHECK(gap->payload.gap.dropped_records == 1U);
	CHECK(gap->payload.gap.first_missing_record_seq != 0U);
	CHECK(gap->payload.gap.first_missing_record_seq ==
	      gap->payload.gap.last_missing_record_seq);
	CHECK(count_intervals(fixture.sink) == 1U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.window.start_monotonic_usec == 60U);
	CHECK(deltas[0].connected_delta_usec == 60U);
	fixture.keys.fail = true;
	const auto allocator_drop = pulse_at(fixture, 180U, 1'000'180, deltas, 4U);
	CHECK(allocator_drop.outcome == telemetry_activity_outcome::allocator_exhausted);
	CHECK(allocator_drop.deltas_dropped == 0U);
	CHECK(deltas[0].connected_delta_usec == 60U);
	fixture.keys.fail = false;
	CHECK(pulse_at(fixture, 240U, 1'000'240, deltas, 4U).deltas_written == 1U);
	CHECK(telemetry_activity_state_stats_copy(&fixture.state).dropped_detail_total == 2U);
	check_records_valid(fixture.sink);
}

void test_config_publication_gate()
{
	runtime fixture{};
	reset_runtime(fixture, { 106U, 206U }, 2U, 60U, 300U);
	const auto enter = make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 600U);
	CHECK(telemetry_activity_state_enter(&fixture.state, enter).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(pulse_at(fixture, 60U, 1'000'060).outcome ==
	      telemetry_activity_outcome::accepted_degraded);
	CHECK(count_intervals(fixture.sink) == 0U);
	telemetry_activity_state_view view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.state, enter.session, &view));
	CHECK(view.cumulative.unknown_usec == 60U);
	CHECK(view.detail_ready == 0U);
	const auto publish =
		telemetry_activity_state_publish_config(&fixture.state, make_config(600U));
	CHECK(publish.outcome == telemetry_activity_outcome::accepted);
	const auto *disabled_gap =
		record_kind_at(fixture.sink, telemetry_record_kind::coverage_gap, 0U);
	CHECK(disabled_gap != nullptr);
	CHECK(disabled_gap->payload.gap.reason == telemetry_gap_reason::telemetry_disabled);
	CHECK(disabled_gap->payload.gap.duration_usec == 60U);
	const auto config_pulse = pulse_at(fixture, 120U, 1'000'120);
	CHECK(config_pulse.outcome == telemetry_activity_outcome::accepted_degraded);
	CHECK(count_intervals(fixture.sink) == 0U);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 120U, 1'000'120,
						   TELEMETRY_ACTIVITY_CONTEXT_NONE, 600U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(pulse_at(fixture, 180U, 1'000'180).outcome == telemetry_activity_outcome::accepted);
	CHECK(count_intervals(fixture.sink) == 1U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.window.start_monotonic_usec == 120U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.config_id == 600U);
	check_records_valid(fixture.sink);
}

void test_midnight_and_clock_jump()
{
	runtime fixture{};
	reset_runtime(fixture, { 107U, 207U }, 2U, 100U, 1000U);
	const auto enter = start_session(fixture, 700U, 0U, DAY_USEC - 50);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, DAY_USEC - 50,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(pulse_at(fixture, 100U, DAY_USEC + 50).intervals_sealed == 2U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.duration_usec == 50U);
	CHECK(interval_at(fixture.sink, 1U)->payload.interval.duration_usec == 50U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.window.end_utc_usec == DAY_USEC);
	CHECK(interval_at(fixture.sink, 1U)->payload.interval.window.start_utc_usec == DAY_USEC);
	auto jump = make_evidence(enter.session, enter.connection, 110U, DAY_USEC - 1000,
				  telemetry_activity_evidence_kind::communication);
	CHECK(telemetry_activity_state_record_evidence(&fixture.state, jump).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(pulse_at(fixture, 200U, DAY_USEC - 910).intervals_sealed == 1U);
	const auto *ambiguous = interval_at(fixture.sink, 2U);
	CHECK(ambiguous != nullptr);
	CHECK((ambiguous->payload.interval.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) !=
	      0U);
	CHECK(ambiguous->payload.interval.window.start_utc_usec == TELEMETRY_UTC_UNKNOWN);
	CHECK(ambiguous->payload.interval.window.end_utc_usec == TELEMETRY_UTC_UNKNOWN);
	CHECK(telemetry_activity_state_stats_copy(&fixture.state).clock_discontinuity_total == 1U);
	check_records_valid(fixture.sink);
}

void test_capacity_numeric_bounds_and_clock_injection()
{
	runtime fixture{};
	reset_runtime(fixture, { 108U, 208U }, 1U, 60U, 300U, 8U, 50U);
	const auto first = start_session(fixture, 800U);
	const auto second = make_enter(fixture.keys.producer, 2U, 2U, 0U, 1'000'000, 800U);
	CHECK(telemetry_activity_state_enter(&fixture.state, second).outcome ==
	      telemetry_activity_outcome::capacity_full);
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(first.session, first.connection, 10U, 1'000'010,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::accepted);
	set_clock(fixture, 59U, 1'000'059);
	CHECK(telemetry_activity_state_retire_expired(&fixture.state).outcome ==
	      telemetry_activity_outcome::idempotent);
	set_clock(fixture, 60U, 1'000'060);
	const auto entered = telemetry_activity_state_enter(&fixture.state, second);
	CHECK(entered.outcome == telemetry_activity_outcome::accepted);
	CHECK(entered.slots_retired == 1U);
	CHECK(telemetry_activity_state_stats_copy(&fixture.state).retired_session_total == 1U);
	fixture.clock.available = false;
	CHECK(telemetry_activity_state_flush(&fixture.state, second.session).outcome ==
	      telemetry_activity_outcome::clock_unavailable);
	telemetry_activity_state_config bad{};
	bad.max_slots = 1U;
	bad.interval_usec = 1U;
	bad.active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL + 1U;
	bad.context_segments_per_minute = 1U;
	bad.producer = fixture.keys.producer;
	bad.clock = { clock_now, &fixture.clock };
	bad.sink = { sink_emit, &fixture.sink };
	bad.key_allocator = { next_key, &fixture.keys };
	CHECK(telemetry_activity_state_init(&fixture.state, &bad) ==
	      telemetry_activity_outcome::invalid);
	bad.active_window_usec = 1U;
	bad.interval_usec = TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL + 1U;
	CHECK(telemetry_activity_state_init(&fixture.state, &bad) ==
	      telemetry_activity_outcome::invalid);
	CHECK(telemetry_activity_state_exit(
		      &fixture.state, make_exit(second.session, second.connection, 1U, 1'000'001U))
		      .outcome == telemetry_activity_outcome::accepted);
	const auto third = make_enter(fixture.keys.producer, 3U, 3U, 1U, 1'000'001U, 800U);
	CHECK(telemetry_activity_state_enter(&fixture.state, third).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_stats_copy(&fixture.state).closed_slots == 0U);
	check_records_valid(fixture.sink);
}

void test_rejected_calls_and_closed_resident_accounting()
{
	runtime fixture{};
	reset_runtime(fixture, { 109U, 209U }, 1U);
	const auto enter = start_session(fixture, 900U);
	telemetry_activity_state_config bad{};
	bad.max_slots = 1U;
	bad.pulse_slot_count = 2U;
	bad.producer = fixture.keys.producer;
	bad.clock = { clock_now, &fixture.clock };
	bad.sink = { sink_emit, &fixture.sink };
	bad.key_allocator = { next_key, &fixture.keys };
	CHECK(telemetry_activity_state_init(&fixture.state, &bad) ==
	      telemetry_activity_outcome::invalid);
	CHECK(fixture.state.initialized == 1U);
	CHECK(fixture.state.used_slots == 1U);
	const auto count = fixture.sink.count;
	const telemetry_connection_id wrong_connection = { fixture.keys.producer, 99U };
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, wrong_connection, 10U, 1,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::invalid);
	CHECK(fixture.state.clock_discontinuity_total == 0U);
	CHECK(fixture.state.slots[0].utc_discontinuity_pending == 0U);
	CHECK(fixture.sink.count == count);
	CHECK(telemetry_activity_state_transition(
		      &fixture.state,
		      make_transition(enter.session, enter.connection, 10U, 1'000'010,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_exit(&fixture.state,
					    make_exit(enter.session, {}, 20U, 1'000'020))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(fixture.state.resident_slots == 0U);
	CHECK(fixture.state.closed_slots == 1U);
	const auto resident = fixture.state.slots[0].cumulative.resident_usec;
	const auto after_exit = fixture.sink.count;
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state, make_evidence(enter.session, {}, 30U, 1'000'030,
						    telemetry_activity_evidence_kind::linkdead))
		      .outcome == telemetry_activity_outcome::invalid);
	CHECK(fixture.state.slots[0].cumulative.resident_usec == resident);
	CHECK(fixture.sink.count == after_exit);
}

void test_numeric_failure_keeps_frontier()
{
	runtime fixture{};
	reset_runtime(fixture, { 110U, 210U }, 1U);
	const auto enter = start_session(fixture);
	auto &slot = fixture.state.slots[0];
	slot.cumulative = { UINT64_MAX, 0U, 0U, UINT64_MAX, UINT64_MAX, 0U };
	const auto count = fixture.sink.count;
	const auto result =
		telemetry_activity_state_flush_at(&fixture.state, enter.session, 1U, 1'000'001);
	CHECK(result.outcome == telemetry_activity_outcome::invalid);
	CHECK(result.has_delta == 0U);
	CHECK(slot.interval_start_monotonic_usec == 0U);
	CHECK(slot.last_observed_monotonic_usec == 0U);
	CHECK(slot.cumulative.connected_usec == UINT64_MAX);
	CHECK(fixture.sink.count == count);
	telemetry_counter_update delta{};
	telemetry_activity_pulse_request request{};
	request.now_monotonic_usec = 60U;
	request.occurrence_utc_usec = 1'000'060;
	request.deltas = &delta;
	request.delta_capacity = 1U;
	CHECK(telemetry_activity_state_pulse(&fixture.state, request).outcome ==
	      telemetry_activity_outcome::invalid);
	CHECK(slot.interval_start_monotonic_usec == 0U);
	CHECK(slot.last_observed_monotonic_usec == 0U);
}

void test_late_tail_preserves_deadline_and_avoids_cross_day_detail()
{
	runtime fixture{};
	reset_runtime(fixture, { 111U, 211U }, 1U, 10U, 75U, 64U);
	const auto enter = start_session(fixture, 100U, 0U, DAY_USEC - 80);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, DAY_USEC - 80,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	const auto result = telemetry_activity_state_flush_at(&fixture.state, enter.session, 100U,
							      DAY_USEC + 20);
	CHECK(result.has_delta == 1U);
	CHECK(result.delta.active_delta_usec == 75U);
	CHECK(result.delta.idle_delta_usec == 25U);
	CHECK(result.delta.resident_delta_usec == 100U);
	CHECK(result.intervals_sealed <= TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION);
	CHECK(result.quality_flags & TELEMETRY_QUALITY_LATE);
	for (std::size_t i = 0; i < fixture.sink.count; ++i)
	{
		const auto &record = fixture.sink.records[i];
		if (record.header.kind != telemetry_record_kind::interval)
			continue;
		const auto &window = record.payload.interval.window;
		CHECK(window.start_utc_usec == TELEMETRY_UTC_UNKNOWN ||
		      window.end_utc_usec <= DAY_USEC || window.start_utc_usec >= DAY_USEC);
	}
	CHECK(fixture.state.pending_gap.pending != 0U ||
	      count_kind(fixture.sink, telemetry_record_kind::coverage_gap) != 0U);
	check_records_valid(fixture.sink);
}

void test_context_churn_has_hard_record_cap()
{
	runtime fixture{};
	reset_runtime(fixture, { 112U, 212U }, 1U, 60'000'000U, 300'000'000U, 3U);
	const auto enter = start_session(fixture);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.state,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	for (unsigned i = 1; i <= 2; ++i)
		(void)telemetry_activity_state_update_context(
			&fixture.state,
			make_context(enter.session, enter.connection, i, 1'000'000 + i,
				     TELEMETRY_ACTIVITY_CONTEXT_TRAVEL, 100U, 100 + i));
	CHECK(fixture.state.slots[0].context_overflow == 1U);
	const auto count = count_intervals(fixture.sink);
	for (unsigned i = 3; i <= 100; ++i)
		(void)telemetry_activity_state_update_context(
			&fixture.state,
			make_context(enter.session, enter.connection, i, 1'000'000 + i,
				     TELEMETRY_ACTIVITY_CONTEXT_TRAVEL, 100U, 100 + i));
	CHECK(count_intervals(fixture.sink) == count);
	(void)telemetry_activity_state_flush_at(&fixture.state, enter.session, 200U, 1'000'200);
	CHECK(count_intervals(fixture.sink) <= 3U);
	// More forced flushes cannot exceed the same minute's record budget.
	(void)telemetry_activity_state_flush_at(&fixture.state, enter.session, 300U, 1'000'300);
	CHECK(count_intervals(fixture.sink) <= 3U);
	CHECK(fixture.state.slots[0].cumulative.active_usec == 300U);
	CHECK(fixture.state.slots[0].cumulative.unknown_usec == 0U);
	check_records_valid(fixture.sink);
}

void test_pulse_buffer_rejection_does_not_consume_counters()
{
	runtime fixture{};
	reset_runtime(fixture, { 113U, 213U }, 2U);
	const auto enter = start_session(fixture);
	telemetry_activity_pulse_request request{};
	request.now_monotonic_usec = 60U;
	request.occurrence_utc_usec = 1'000'060;
	const auto before = fixture.sink.count;
	CHECK(telemetry_activity_state_pulse(&fixture.state, request).outcome ==
	      telemetry_activity_outcome::invalid);
	CHECK(fixture.sink.count == before);
	CHECK(fixture.state.slots[0].cumulative.resident_usec == 0U);
	CHECK(fixture.state.slots[0].interval_start_monotonic_usec == 0U);
	telemetry_counter_update delta{};
	request.deltas = &delta;
	request.delta_capacity = 1U;
	const auto result = telemetry_activity_state_pulse(&fixture.state, request);
	CHECK(result.outcome == telemetry_activity_outcome::accepted);
	CHECK(result.deltas_written == 1U);
	CHECK(delta.session.id.session_seq == enter.session.id.session_seq);
	CHECK(delta.resident_delta_usec == 60U);
}

void test_config_scope_and_versions_do_not_cross()
{
	runtime fixture{};
	reset_runtime(fixture, { 114U, 214U }, 2U);
	CHECK(telemetry_activity_state_publish_config(&fixture.state, make_config(100U)).outcome ==
	      telemetry_activity_outcome::accepted);
	const auto enter = make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 100U);
	for (unsigned variant = 0; variant < 4; ++variant)
	{
		auto invalid = enter;
		if (variant == 0)
			invalid.session.environment_id++;
		if (variant == 1)
			invalid.session.season_id++;
		if (variant == 2)
			invalid.classifier_version++;
		if (variant == 3)
			invalid.policy_version++;
		CHECK(telemetry_activity_state_enter(&fixture.state, invalid).outcome ==
		      telemetry_activity_outcome::invalid);
		CHECK(fixture.state.used_slots == 0U);
	}
	CHECK(telemetry_activity_state_enter(&fixture.state, enter).outcome ==
	      telemetry_activity_outcome::accepted);
	auto foreign = make_config(101U);
	foreign.environment_id++;
	CHECK(telemetry_activity_state_publish_config(&fixture.state, foreign).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state, make_context(enter.session, enter.connection, 10U, 1'000'010,
						   TELEMETRY_ACTIVITY_CONTEXT_OTHER, 101U))
		      .outcome == telemetry_activity_outcome::invalid);
	CHECK(fixture.state.slots[0].config_id == 100U);
	CHECK(fixture.state.slots[0].interval_start_monotonic_usec == 0U);
}

void test_disabled_zero_config_rollover_and_recovery()
{
	runtime fixture{};
	reset_runtime(fixture, { 115U, 215U }, 1U);
	auto disabled = make_config(100U);
	disabled.enabled = 0U;
	disabled.context_segments_per_minute = 0U;
	disabled.interval_usec = 0U;
	disabled.active_window_usec = 0U;
	disabled.pulse_slot_count = 0U;
	CHECK(telemetry_config_is_valid(disabled));
	CHECK(telemetry_activity_state_publish_config(&fixture.state, disabled).outcome ==
	      telemetry_activity_outcome::accepted);
	constexpr auto start = TELEMETRY_ACTIVITY_MINUTE_USEC - 1U;
	const auto enter =
		make_enter(fixture.keys.producer, 1U, 1U, start, 1'000'000 + start, 100U);
	CHECK(telemetry_activity_state_enter(&fixture.state, enter).outcome ==
	      telemetry_activity_outcome::accepted);
	for (unsigned i = 1U; i <= 20U; ++i)
	{
		const auto at = TELEMETRY_ACTIVITY_MINUTE_USEC + i;
		const auto result = telemetry_activity_state_flush_at(&fixture.state, enter.session,
								      at, 1'000'000 + at);
		CHECK(result.outcome == telemetry_activity_outcome::accepted_degraded);
		CHECK(fixture.state.pending_gap.reason == telemetry_gap_reason::telemetry_disabled);
		CHECK(fixture.state.slots[0].segment_cap ==
		      fixture.state.context_segments_per_minute);
		CHECK(fixture.state.slots[0].context_overflow == 0U);
	}
	constexpr auto end = TELEMETRY_ACTIVITY_MINUTE_USEC + 1'000U;
	CHECK(telemetry_activity_state_flush_at(&fixture.state, enter.session, end, 1'000'000 + end)
		      .outcome == telemetry_activity_outcome::accepted_degraded);
	CHECK(fixture.state.slots[0].cumulative.unknown_usec == end - start);
	CHECK(count_intervals(fixture.sink) == 0U);
	CHECK(telemetry_activity_state_publish_config(&fixture.state, make_config(101U)).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_update_context(
		      &fixture.state,
		      make_context(enter.session, enter.connection, end, 1'000'000 + end,
				   TELEMETRY_ACTIVITY_CONTEXT_OTHER, 101U))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_flush_at(&fixture.state, enter.session, end + 1U,
						1'000'001 + end)
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(count_intervals(fixture.sink) == 1U);
	CHECK(interval_at(fixture.sink, 0U)->payload.interval.config_id == 101U);
	bool saw_gap = false;
	for (std::size_t i = 0; i < fixture.sink.count; ++i)
	{
		const auto &record = fixture.sink.records[i];
		if (record.header.kind == telemetry_record_kind::coverage_gap)
		{
			saw_gap = true;
			CHECK(record.payload.gap.reason ==
			      telemetry_gap_reason::telemetry_disabled);
			CHECK(record.payload.gap.quality_flags & TELEMETRY_QUALITY_DISABLED);
		}
	}
	CHECK(saw_gap);
}

} // namespace

int main()
{
	test_disabled_zero_config_rollover_and_recovery();
	test_config_scope_and_versions_do_not_cross();
	test_pulse_buffer_rejection_does_not_consume_counters();
	test_numeric_failure_keeps_frontier();
	test_late_tail_preserves_deadline_and_avoids_cross_day_detail();
	test_context_churn_has_hard_record_cap();
	test_rejected_calls_and_closed_resident_accounting();
	test_active_idle_deadline_and_deltas();
	test_explicit_evidence_cases_and_replay();
	test_context_precedence_cap_and_minute_reset();
	test_linkdead_accounting_and_reconnect();
	test_drop_gap_and_total_recovery();
	test_config_publication_gate();
	test_midnight_and_clock_jump();
	test_capacity_numeric_bounds_and_clock_injection();
	std::puts("telemetry activity state harness passed");
	return 0;
}
