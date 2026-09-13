#include "telemetry/telemetry_activity.h"
#include "telemetry/telemetry_session.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

/* The integration proof must remain allocation-free like the two services. */
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

struct record_sink
{
	telemetry_record records[256];
	std::size_t count;
	std::size_t capacity;
	bool reject;
};

/* One allocator is deliberately borrowed by both #266 and #264. */
struct shared_key_allocator
{
	telemetry_producer_id producer;
	telemetry_record_sequence next_sequence;
	bool fail;
};

struct integration_fixture
{
	test_clock clock;
	record_sink activity_sink;
	record_sink session_sink;
	shared_key_allocator keys;
	telemetry_activity_state activity;
	telemetry_session_state session;
};

bool check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
		return false;
	}
	return true;
}

#define CHECK(expression)                                        \
	do                                                       \
	{                                                        \
		if (!check((expression), #expression, __LINE__)) \
			return false;                            \
	} while (false)

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

bool counters_equal(const telemetry_cumulative_counters &left,
		    const telemetry_cumulative_counters &right) noexcept
{
	return left.connected_usec == right.connected_usec &&
	       left.active_usec == right.active_usec && left.idle_usec == right.idle_usec &&
	       left.unknown_usec == right.unknown_usec &&
	       left.resident_usec == right.resident_usec &&
	       left.linkdead_usec == right.linkdead_usec;
}

bool activity_views_equal(const telemetry_activity_state_view &left,
			  const telemetry_activity_state_view &right) noexcept
{
	return session_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       counters_equal(left.cumulative, right.cumulative) &&
	       left.dimensions.level_band == right.dimensions.level_band &&
	       left.dimensions.class_id == right.dimensions.class_id &&
	       left.dimensions.race_id == right.dimensions.race_id &&
	       left.dimensions.faction_id == right.dimensions.faction_id &&
	       left.dimensions.zone_vnum == right.dimensions.zone_vnum &&
	       left.dimensions.group_size == right.dimensions.group_size &&
	       left.config_id == right.config_id &&
	       left.classifier_version == right.classifier_version &&
	       left.policy_version == right.policy_version &&
	       left.interval_start_monotonic_usec == right.interval_start_monotonic_usec &&
	       left.active_deadline_monotonic_usec == right.active_deadline_monotonic_usec &&
	       left.context_segments_used == right.context_segments_used &&
	       left.category == right.category && left.context == right.context &&
	       left.context_quality == right.context_quality &&
	       left.quality_flags == right.quality_flags && left.connected == right.connected &&
	       left.closed == right.closed && left.detail_ready == right.detail_ready &&
	       left.context_overflow == right.context_overflow;
}

bool activity_stats_equal(const telemetry_activity_state_stats &left,
			  const telemetry_activity_state_stats &right) noexcept
{
	return left.capacity == right.capacity && left.used_slots == right.used_slots &&
	       left.resident_slots == right.resident_slots &&
	       left.closed_slots == right.closed_slots &&
	       left.admitted_config_total == right.admitted_config_total &&
	       left.accepted_detail_total == right.accepted_detail_total &&
	       left.dropped_detail_total == right.dropped_detail_total &&
	       left.accepted_control_total == right.accepted_control_total &&
	       left.dropped_control_total == right.dropped_control_total &&
	       left.suppressed_detail_total == right.suppressed_detail_total &&
	       left.context_overflow_total == right.context_overflow_total &&
	       left.clock_discontinuity_total == right.clock_discontinuity_total &&
	       left.retired_session_total == right.retired_session_total;
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
	auto *sink = static_cast<record_sink *>(context);
	if (sink->reject || sink->count >= sink->capacity)
		return false;
	sink->records[sink->count++] = *record;
	return true;
}

bool next_key(void *context, telemetry_record_kind, telemetry_record_key *key) noexcept
{
	auto *allocator = static_cast<shared_key_allocator *>(context);
	if (allocator->fail || allocator->next_sequence == 0U ||
	    allocator->next_sequence == std::numeric_limits<telemetry_record_sequence>::max())
		return false;
	*key = { allocator->producer, allocator->next_sequence };
	++allocator->next_sequence;
	return true;
}

telemetry_config_snapshot make_config(telemetry_config_id config_id,
				      telemetry_duration_usec interval_usec,
				      telemetry_duration_usec active_window_usec)
{
	telemetry_config_snapshot config{};
	config.schema_version = TELEMETRY_SCHEMA_VERSION;
	config.config_id = config_id;
	config.revision = config_id;
	config.build_version = 100U;
	config.content_version = 200U;
	config.property_version = 300U + static_cast<std::uint32_t>(config_id);
	config.classifier_version = 400U;
	config.policy_version = 500U;
	config.season_id = 7U;
	config.environment_id = 8U;
	config.fingerprint[0] = static_cast<std::uint8_t>(config_id & 0xffU);
	config.effective_utc_usec = 1'000'000;
	config.interval_usec = interval_usec;
	config.checkpoint_interval_usec = interval_usec;
	config.active_window_usec = active_window_usec;
	config.context_segments_per_minute = 8U;
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

bool init_fixture(integration_fixture &fixture, telemetry_producer_id producer, bool publish_config,
		  telemetry_duration_usec interval_usec = 100U,
		  telemetry_duration_usec active_window_usec = 1'000U)
{
	fixture = {};
	fixture.clock = { 0U, 1'000'000, true };
	fixture.activity_sink.capacity =
		sizeof(fixture.activity_sink.records) / sizeof(fixture.activity_sink.records[0]);
	fixture.session_sink.capacity =
		sizeof(fixture.session_sink.records) / sizeof(fixture.session_sink.records[0]);
	fixture.keys = { producer, 1U, false };

	telemetry_activity_state_config activity_config{};
	activity_config.max_slots = 2U;
	activity_config.pulse_slot_count = 1U;
	activity_config.context_segments_per_minute = 8U;
	activity_config.interval_usec = interval_usec;
	activity_config.active_window_usec = active_window_usec;
	activity_config.detached_retire_after_usec = 1'000U;
	activity_config.producer = producer;
	activity_config.clock = { clock_now, &fixture.clock };
	activity_config.sink = { sink_emit, &fixture.activity_sink };
	activity_config.key_allocator = { next_key, &fixture.keys };
	CHECK(telemetry_activity_state_init(&fixture.activity, &activity_config) ==
	      telemetry_activity_outcome::accepted);

	telemetry_session_state_config session_config{};
	session_config.max_slots = 2U;
	session_config.detached_retire_after_usec = 1'000U;
	session_config.producer = producer;
	session_config.clock = { clock_now, &fixture.clock };
	session_config.sink = { sink_emit, &fixture.session_sink };
	session_config.key_allocator = { next_key, &fixture.keys };
	CHECK(telemetry_session_state_init(&fixture.session, &session_config) ==
	      telemetry_session_state_outcome::accepted);

	if (publish_config)
	{
		CHECK(telemetry_activity_state_publish_config(&fixture.activity,
							      make_config(700U, interval_usec,
									  active_window_usec))
			      .outcome == telemetry_activity_outcome::accepted);
	}
	return true;
}

bool enter_both(integration_fixture &fixture, const telemetry_session_enter &enter)
{
	CHECK(telemetry_activity_state_enter(&fixture.activity, enter).outcome ==
	      telemetry_activity_outcome::accepted);
	CHECK(telemetry_session_state_enter(&fixture.session, enter).outcome ==
	      telemetry_session_state_outcome::accepted);
	return true;
}

bool transfer_delta(integration_fixture &fixture, const telemetry_activity_result &result)
{
	CHECK(result.has_delta != 0U);
	CHECK(telemetry_counter_update_is_valid(result.delta));
	CHECK(telemetry_session_state_update_counters(&fixture.session, result.delta).outcome ==
	      telemetry_session_state_outcome::accepted);
	return true;
}

bool checkpoint(integration_fixture &fixture, telemetry_session_ref session,
		telemetry_monotonic_usec monotonic_usec, telemetry_utc_usec utc_usec,
		telemetry_checkpoint_revision revision)
{
	const telemetry_session_state_result result = telemetry_session_state_checkpoint_at(
		&fixture.session, session, monotonic_usec, utc_usec);
	CHECK(result.outcome == telemetry_session_state_outcome::accepted);
	CHECK(result.revision == revision);
	return true;
}

telemetry_activity_pulse_result pulse_at(integration_fixture &fixture,
					 telemetry_monotonic_usec monotonic_usec,
					 telemetry_utc_usec utc_usec,
					 telemetry_counter_update *deltas,
					 std::uint16_t delta_capacity)
{
	telemetry_activity_pulse_request request{};
	request.now_monotonic_usec = monotonic_usec;
	request.occurrence_utc_usec = utc_usec;
	request.slot = 0U;
	request.deltas = deltas;
	request.delta_capacity = delta_capacity;
	return telemetry_activity_state_pulse(&fixture.activity, request);
}

std::size_t count_kind(const record_sink &sink, telemetry_record_kind kind)
{
	std::size_t count = 0U;
	for (std::size_t index = 0U; index < sink.count; ++index)
		if (sink.records[index].header.kind == kind)
			++count;
	return count;
}

bool check_records(const record_sink &sink, telemetry_producer_id producer)
{
	for (std::size_t index = 0U; index < sink.count; ++index)
	{
		CHECK(telemetry_record_is_valid(sink.records[index]));
		CHECK(producer_equal(sink.records[index].header.key.producer, producer));
	}
	return true;
}

bool check_shared_key_stream(const integration_fixture &fixture)
{
	CHECK(check_records(fixture.activity_sink, fixture.keys.producer));
	CHECK(check_records(fixture.session_sink, fixture.keys.producer));
	for (std::size_t left = 0U; left < fixture.activity_sink.count; ++left)
	{
		for (std::size_t right = left + 1U; right < fixture.activity_sink.count; ++right)
			CHECK(fixture.activity_sink.records[left].header.key.record_seq !=
			      fixture.activity_sink.records[right].header.key.record_seq);
		for (std::size_t right = 0U; right < fixture.session_sink.count; ++right)
			CHECK(fixture.activity_sink.records[left].header.key.record_seq !=
			      fixture.session_sink.records[right].header.key.record_seq);
	}
	for (std::size_t left = 0U; left < fixture.session_sink.count; ++left)
		for (std::size_t right = left + 1U; right < fixture.session_sink.count; ++right)
			CHECK(fixture.session_sink.records[left].header.key.record_seq !=
			      fixture.session_sink.records[right].header.key.record_seq);
	return true;
}

bool reconcile_one(integration_fixture &fixture, telemetry_session_ref session,
		   telemetry_connection_id expected_connection,
		   telemetry_cumulative_counters expected_totals)
{
	telemetry_activity_state_view activity_view{};
	telemetry_session_state_view session_view{};
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, session, &activity_view));
	CHECK(telemetry_session_state_copy_view(&fixture.session, session, &session_view));
	CHECK(connection_equal(activity_view.connection, expected_connection));
	CHECK(connection_equal(session_view.connection, expected_connection));
	CHECK(counters_equal(activity_view.cumulative, expected_totals));
	CHECK(counters_equal(session_view.cumulative, expected_totals));
	CHECK(counters_equal(activity_view.cumulative, session_view.cumulative));
	return true;
}

bool test_connected_transfer_and_rejected_detail()
{
	integration_fixture fixture{};
	CHECK(init_fixture(fixture, { 266U, 264U }, true));
	const telemetry_session_enter enter =
		make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 700U);
	CHECK(enter_both(fixture, enter));
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.activity,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);

	const telemetry_activity_result first =
		telemetry_activity_state_flush_at(&fixture.activity, enter.session, 60U, 1'000'060);
	CHECK(first.outcome == telemetry_activity_outcome::accepted);
	CHECK(first.intervals_sealed == 1U);
	CHECK(first.has_delta != 0U);
	CHECK(first.delta.at_monotonic_usec == 60U);
	CHECK(first.delta.connected_delta_usec == 60U);
	CHECK(first.delta.active_delta_usec == 60U);
	CHECK(first.delta.idle_delta_usec == 0U);
	CHECK(first.delta.unknown_delta_usec == 0U);
	CHECK(first.delta.resident_delta_usec == 60U);
	CHECK(first.delta.linkdead_delta_usec == 0U);
	CHECK(transfer_delta(fixture, first));
	CHECK(checkpoint(fixture, enter.session, 60U, 1'000'060, 1U));

	/* A rejected interval still carries its typed delta to #264. */
	fixture.activity_sink.reject = true;
	const telemetry_activity_result rejected = telemetry_activity_state_flush_at(
		&fixture.activity, enter.session, 120U, 1'000'120);
	CHECK(rejected.outcome == telemetry_activity_outcome::sink_rejected);
	CHECK(rejected.records_dropped == 1U);
	CHECK(rejected.has_delta != 0U);
	CHECK(rejected.delta.at_monotonic_usec == 120U);
	CHECK(rejected.delta.connected_delta_usec == 60U);
	CHECK(rejected.delta.active_delta_usec == 60U);
	CHECK(rejected.delta.resident_delta_usec == 60U);
	CHECK(transfer_delta(fixture, rejected));
	CHECK(checkpoint(fixture, enter.session, 120U, 1'000'120, 2U));
	fixture.activity_sink.reject = false;

	const telemetry_cumulative_counters expected = { 120U, 120U, 0U, 0U, 120U, 0U };
	CHECK(reconcile_one(fixture, enter.session, enter.connection, expected));
	const telemetry_activity_state_stats stats =
		telemetry_activity_state_stats_copy(&fixture.activity);
	CHECK(stats.accepted_detail_total == 1U);
	CHECK(stats.dropped_detail_total == 1U);
	CHECK(stats.suppressed_detail_total == 0U);
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::interval) == 1U);
	const auto recovered = telemetry_activity_state_flush_at(&fixture.activity, enter.session,
								 180U, 1'000'180);
	CHECK(recovered.outcome == telemetry_activity_outcome::accepted);
	CHECK(transfer_delta(fixture, recovered));
	CHECK(checkpoint(fixture, enter.session, 180U, 1'000'180, 3U));
	const telemetry_cumulative_counters recovered_totals = { 180U, 180U, 0U, 0U, 180U, 0U };
	CHECK(reconcile_one(fixture, enter.session, enter.connection, recovered_totals));
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::coverage_gap) == 1U);
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::interval) == 2U);
	CHECK(check_shared_key_stream(fixture));
	return true;
}

bool test_detach_linkdead_flush_and_reconnect()
{
	integration_fixture fixture{};
	CHECK(init_fixture(fixture, { 267U, 264U }, true));
	const telemetry_session_enter enter =
		make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 700U);
	CHECK(enter_both(fixture, enter));
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.activity,
		      make_evidence(enter.session, enter.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::combat_participation))
		      .outcome == telemetry_activity_outcome::accepted);

	/* The detach-frontier delta is delivered before S records its detach. */
	const telemetry_activity_result detach_delta = telemetry_activity_state_flush_at(
		&fixture.activity, enter.session, 100U, 1'000'100);
	CHECK(detach_delta.outcome == telemetry_activity_outcome::accepted);
	CHECK(detach_delta.has_delta != 0U);
	CHECK(detach_delta.delta.connected_delta_usec == 100U);
	CHECK(detach_delta.delta.active_delta_usec == 100U);
	CHECK(detach_delta.delta.resident_delta_usec == 100U);
	CHECK(transfer_delta(fixture, detach_delta));
	CHECK(telemetry_activity_state_transition(
		      &fixture.activity,
		      make_transition(enter.session, enter.connection, 100U, 1'000'100,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_session_state_transition(
		      &fixture.session,
		      make_transition(enter.session, enter.connection, 100U, 1'000'100,
				      telemetry_connection_transition_kind::detached))
		      .outcome == telemetry_session_state_outcome::accepted);

	const telemetry_activity_result linkdead_flush = telemetry_activity_state_flush_at(
		&fixture.activity, enter.session, 160U, 1'000'160);
	CHECK(linkdead_flush.outcome == telemetry_activity_outcome::accepted);
	CHECK(linkdead_flush.has_delta != 0U);
	CHECK(telemetry_connection_id_is_zero(linkdead_flush.delta.connection));
	CHECK(linkdead_flush.delta.connected_delta_usec == 0U);
	CHECK(linkdead_flush.delta.active_delta_usec == 0U);
	CHECK(linkdead_flush.delta.resident_delta_usec == 60U);
	CHECK(linkdead_flush.delta.linkdead_delta_usec == 60U);
	CHECK(transfer_delta(fixture, linkdead_flush));
	CHECK(checkpoint(fixture, enter.session, 160U, 1'000'160, 1U));
	telemetry_session_state_view detached_view{};
	CHECK(telemetry_session_state_copy_view(&fixture.session, enter.session, &detached_view));
	CHECK(telemetry_connection_id_is_zero(detached_view.connection));
	CHECK(detached_view.cumulative.linkdead_usec == 60U);

	/* Reconnect seals the remaining linkdead frontier before S attaches. */
	const telemetry_connection_id connection_two = { fixture.keys.producer, 2U };
	const telemetry_activity_result reconnect_delta = telemetry_activity_state_transition(
		&fixture.activity, make_transition(enter.session, connection_two, 200U, 1'000'200,
						   telemetry_connection_transition_kind::attached));
	CHECK(reconnect_delta.outcome == telemetry_activity_outcome::accepted);
	CHECK(reconnect_delta.has_delta != 0U);
	CHECK(telemetry_connection_id_is_zero(reconnect_delta.delta.connection));
	CHECK(reconnect_delta.delta.resident_delta_usec == 40U);
	CHECK(reconnect_delta.delta.linkdead_delta_usec == 40U);
	CHECK(transfer_delta(fixture, reconnect_delta));
	CHECK(telemetry_session_state_transition(
		      &fixture.session,
		      make_transition(enter.session, connection_two, 200U, 1'000'200,
				      telemetry_connection_transition_kind::attached))
		      .outcome == telemetry_session_state_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.activity,
		      make_evidence(enter.session, connection_two, 200U, 1'000'200,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);

	const telemetry_activity_result post_reconnect = telemetry_activity_state_flush_at(
		&fixture.activity, enter.session, 260U, 1'000'260);
	CHECK(post_reconnect.outcome == telemetry_activity_outcome::accepted);
	CHECK(post_reconnect.has_delta != 0U);
	CHECK(connection_equal(post_reconnect.delta.connection, connection_two));
	CHECK(post_reconnect.delta.connected_delta_usec == 60U);
	CHECK(post_reconnect.delta.active_delta_usec == 60U);
	CHECK(post_reconnect.delta.resident_delta_usec == 60U);
	CHECK(transfer_delta(fixture, post_reconnect));
	CHECK(checkpoint(fixture, enter.session, 260U, 1'000'260, 2U));

	const telemetry_cumulative_counters expected = { 160U, 160U, 0U, 0U, 260U, 100U };
	CHECK(reconcile_one(fixture, enter.session, connection_two, expected));
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::interval) == 4U);
	CHECK(check_shared_key_stream(fixture));
	return true;
}

bool test_config_unavailable_preserves_counters()
{
	integration_fixture fixture{};
	CHECK(init_fixture(fixture, { 268U, 264U }, false));
	const telemetry_session_enter enter =
		make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 999U);
	CHECK(enter_both(fixture, enter));
	telemetry_activity_state_view before{};
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, enter.session, &before));
	CHECK(before.detail_ready == 0U);

	const telemetry_activity_result unavailable =
		telemetry_activity_state_flush_at(&fixture.activity, enter.session, 60U, 1'000'060);
	CHECK(unavailable.outcome == telemetry_activity_outcome::accepted_degraded);
	CHECK(unavailable.has_delta != 0U);
	CHECK(unavailable.intervals_sealed == 1U);
	CHECK(unavailable.records_dropped == 1U);
	CHECK(unavailable.delta.connected_delta_usec == 60U);
	CHECK(unavailable.delta.active_delta_usec == 0U);
	CHECK(unavailable.delta.unknown_delta_usec == 60U);
	CHECK(unavailable.delta.resident_delta_usec == 60U);
	CHECK(transfer_delta(fixture, unavailable));
	CHECK(checkpoint(fixture, enter.session, 60U, 1'000'060, 1U));

	const telemetry_activity_state_stats stats =
		telemetry_activity_state_stats_copy(&fixture.activity);
	CHECK(stats.admitted_config_total == 0U);
	CHECK(stats.accepted_detail_total == 0U);
	CHECK(stats.dropped_detail_total == 0U);
	CHECK(stats.suppressed_detail_total == 1U);
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::interval) == 0U);
	const telemetry_cumulative_counters expected = { 60U, 0U, 0U, 60U, 60U, 0U };
	CHECK(reconcile_one(fixture, enter.session, enter.connection, expected));
	CHECK(check_shared_key_stream(fixture));
	return true;
}

bool test_pulse_capacity_atomic_retry()
{
	integration_fixture fixture{};
	CHECK(init_fixture(fixture, { 269U, 264U }, true));
	const telemetry_session_enter first =
		make_enter(fixture.keys.producer, 1U, 1U, 0U, 1'000'000, 700U);
	const telemetry_session_enter second =
		make_enter(fixture.keys.producer, 2U, 2U, 0U, 1'000'000, 700U);
	CHECK(enter_both(fixture, first));
	CHECK(enter_both(fixture, second));
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.activity,
		      make_evidence(first.session, first.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);
	CHECK(telemetry_activity_state_record_evidence(
		      &fixture.activity,
		      make_evidence(second.session, second.connection, 0U, 1'000'000,
				    telemetry_activity_evidence_kind::player_action))
		      .outcome == telemetry_activity_outcome::accepted);

	telemetry_activity_state_view before_first{};
	telemetry_activity_state_view before_second{};
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, first.session, &before_first));
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, second.session,
						 &before_second));
	const telemetry_activity_state_stats before_stats =
		telemetry_activity_state_stats_copy(&fixture.activity);
	const std::size_t before_records = fixture.activity_sink.count;
	const telemetry_record_sequence before_sequence = fixture.keys.next_sequence;
	telemetry_counter_update insufficient[1]{};
	insufficient[0].at_monotonic_usec = 777U;

	const telemetry_activity_pulse_result rejected =
		pulse_at(fixture, 100U, 1'000'100, insufficient, 1U);
	telemetry_activity_state_view after_first{};
	telemetry_activity_state_view after_second{};
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, first.session, &after_first));
	CHECK(telemetry_activity_state_copy_view(&fixture.activity, second.session, &after_second));
	const telemetry_activity_state_stats after_stats =
		telemetry_activity_state_stats_copy(&fixture.activity);

	CHECK(rejected.outcome == telemetry_activity_outcome::invalid);
	CHECK(rejected.intervals_sealed == 0U);
	CHECK(rejected.records_attempted == 0U);
	CHECK(rejected.records_accepted == 0U);
	CHECK(rejected.records_dropped == 0U);
	CHECK(rejected.deltas_written == 0U);
	CHECK(rejected.deltas_dropped == 0U);
	CHECK(rejected.quality_flags == TELEMETRY_QUALITY_NONE);
	CHECK(activity_views_equal(before_first, after_first));
	CHECK(activity_views_equal(before_second, after_second));
	CHECK(activity_stats_equal(before_stats, after_stats));
	CHECK(fixture.activity_sink.count == before_records);
	CHECK(fixture.keys.next_sequence == before_sequence);
	CHECK(insufficient[0].at_monotonic_usec == 777U);

	telemetry_counter_update sufficient[2]{};
	const telemetry_activity_pulse_result retried =
		pulse_at(fixture, 100U, 1'000'100, sufficient, 2U);
	CHECK(retried.outcome == telemetry_activity_outcome::accepted);
	CHECK(retried.sessions_considered == 2U);
	CHECK(retried.intervals_sealed == 2U);
	CHECK(retried.deltas_written == 2U);
	CHECK(retried.deltas_dropped == 0U);
	CHECK(telemetry_counter_update_is_valid(sufficient[0]));
	CHECK(telemetry_counter_update_is_valid(sufficient[1]));
	CHECK(telemetry_session_state_update_counters(&fixture.session, sufficient[0]).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(telemetry_session_state_update_counters(&fixture.session, sufficient[1]).outcome ==
	      telemetry_session_state_outcome::accepted);
	CHECK(checkpoint(fixture, first.session, 100U, 1'000'100, 1U));
	CHECK(checkpoint(fixture, second.session, 100U, 1'000'100, 1U));

	const telemetry_cumulative_counters expected = { 100U, 100U, 0U, 0U, 100U, 0U };
	CHECK(reconcile_one(fixture, first.session, first.connection, expected));
	CHECK(reconcile_one(fixture, second.session, second.connection, expected));
	CHECK(count_kind(fixture.activity_sink, telemetry_record_kind::interval) == 2U);
	CHECK(check_shared_key_stream(fixture));
	return true;
}

} // namespace

int main()
{
	if (!test_connected_transfer_and_rejected_detail() ||
	    !test_detach_linkdead_flush_and_reconnect() ||
	    !test_config_unavailable_preserves_counters() || !test_pulse_capacity_atomic_retry())
		return 1;
	std::puts("telemetry activity/session integration harness passed");
	return 0;
}
