#include "telemetry/telemetry_progression.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

/* A gameplay-boundary fact must not allocate while it is being captured. */
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

struct sink_fixture
{
	telemetry_record records[16];
	std::size_t count;
	bool accept;
};

struct key_fixture
{
	telemetry_producer_id producer;
	telemetry_record_sequence next_sequence;
	bool fail;
};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry progression harness failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool emit_record(void *context, const telemetry_record *record) noexcept
{
	auto *sink = static_cast<sink_fixture *>(context);
	if (!sink->accept || record == nullptr ||
	    sink->count == sizeof(sink->records) / sizeof(sink->records[0]))
		return false;
	sink->records[sink->count++] = *record;
	return true;
}

bool next_key(void *context, telemetry_record_kind kind, telemetry_record_key *key) noexcept
{
	(void)kind;
	auto *fixture = static_cast<key_fixture *>(context);
	if (fixture->fail || key == nullptr || fixture->next_sequence == 0U)
		return false;
	*key = { fixture->producer, fixture->next_sequence };
	++fixture->next_sequence;
	return true;
}

telemetry_progression_state_config state_config(sink_fixture &sink, key_fixture &keys)
{
	return { keys.producer, { emit_record, &sink }, { next_key, &keys } };
}

int main_impl()
{
	const telemetry_producer_id producer = { 100U, 200U };
	const telemetry_session_ref session = { { producer, 7U }, 800U, 900, 3U, 4U };
	const telemetry_connection_id connection = { producer, 9U };
	const telemetry_dimensions dimensions = { 10U, 2U, 3U, 4U, 500, 1U };

	sink_fixture sink{};
	sink.accept = true;
	key_fixture keys{ producer, 1U, false };
	telemetry_progression_state state{};
	const auto config = state_config(sink, keys);
	CHECK(telemetry_progression_state_init(&state, &config) ==
	      telemetry_progression_outcome::accepted);

	const auto earned = telemetry_progression_make_experience(
		telemetry_progression_source::kill, telemetry_progression_reason::earned, 100, 150,
		1000, 1125, 10U,
		TELEMETRY_PROGRESSION_MODIFIER_RESTED | TELEMETRY_PROGRESSION_MODIFIER_FINAL_CAP,
		TELEMETRY_QUALITY_NONE);
	CHECK(earned.applied_xp == 125);
	CHECK(telemetry_progression_observation_is_valid(earned));
	const auto earned_result = telemetry_progression_state_record(
		&state, session, connection, 1234U, 1'700'000, dimensions, 55U, 6U, 7U, earned);
	CHECK(earned_result.outcome == telemetry_progression_outcome::accepted);
	CHECK(earned_result.record.record_seq == 1U);
	CHECK(sink.count == 1U);
	CHECK(telemetry_record_is_valid(sink.records[0]));
	CHECK(sink.records[0].header.kind == telemetry_record_kind::progression);
	CHECK(sink.records[0].payload.progression.kind ==
	      telemetry_progression_kind::experience_observed);
	CHECK(sink.records[0].payload.progression.source == telemetry_progression_source::kill);
	CHECK(sink.records[0].payload.progression.reason == telemetry_progression_reason::earned);
	CHECK(sink.records[0].payload.progression.applied_xp == 125);
	CHECK(sink.records[0].payload.progression.observation_status ==
	      telemetry_progression_observation_status::observed_mutable);

	const auto death = telemetry_progression_make_experience(
		telemetry_progression_source::death, telemetry_progression_reason::death_loss, -30,
		-30, 1000, 970, 10U, TELEMETRY_PROGRESSION_MODIFIER_DIFFICULTY_DEATH,
		TELEMETRY_QUALITY_NONE);
	CHECK(death.applied_xp == -30);
	CHECK(telemetry_progression_observation_is_valid(death));
	CHECK(telemetry_progression_state_record(&state, session, connection, 1235U, 1'700'001,
						 dimensions, 55U, 6U, 7U, death)
		      .outcome == telemetry_progression_outcome::accepted);

	const auto level_up = telemetry_progression_make_level_transition(
		telemetry_progression_kind::level_advanced, telemetry_progression_source::system,
		telemetry_progression_reason::level_threshold, 10U, 11U, 900U,
		TELEMETRY_PROGRESSION_MODIFIER_NONE, TELEMETRY_QUALITY_NONE);
	CHECK(telemetry_progression_observation_is_valid(level_up));
	CHECK(level_up.applied_xp == 0);
	CHECK(level_up.before_exp == 0 && level_up.after_exp == 0);
	CHECK(telemetry_progression_state_record(&state, session, connection, 1236U, 1'700'002,
						 dimensions, 55U, 6U, 7U, level_up)
		      .outcome == telemetry_progression_outcome::accepted);
	CHECK(sink.count == 3U);
	CHECK(sink.records[2].payload.progression.threshold_xp == 900U);
	CHECK(sink.records[2].payload.progression.requested_xp == 0);
	CHECK(sink.records[2].payload.progression.computed_xp == 0);

	const auto level_down = telemetry_progression_make_level_transition(
		telemetry_progression_kind::level_lost, telemetry_progression_source::death,
		telemetry_progression_reason::death_loss, 11U, 10U, 900U,
		TELEMETRY_PROGRESSION_MODIFIER_NONE, TELEMETRY_QUALITY_NONE);
	CHECK(telemetry_progression_observation_is_valid(level_down));
	CHECK(telemetry_progression_state_record(&state, session, connection, 1237U, 1'700'003,
						 dimensions, 55U, 6U, 7U, level_down)
		      .outcome == telemetry_progression_outcome::accepted);

	auto invalid = earned;
	invalid.applied_xp = 126;
	CHECK(!telemetry_progression_observation_is_valid(invalid));
	CHECK(telemetry_progression_state_record(&state, session, connection, 1238U, 1'700'004,
						 dimensions, 55U, 6U, 7U, invalid)
		      .outcome == telemetry_progression_outcome::invalid);
	CHECK(sink.count == 4U);

	const auto extreme = telemetry_progression_make_experience(
		telemetry_progression_source::system,
		telemetry_progression_reason::system_adjustment,
		std::numeric_limits<std::int64_t>::max(), 0,
		std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max(),
		10U, TELEMETRY_PROGRESSION_MODIFIER_NONE, TELEMETRY_QUALITY_NONE);
	CHECK(!telemetry_progression_observation_is_valid(extreme));

	sink.accept = false;
	const auto rejected = telemetry_progression_state_record(
		&state, session, connection, 1239U, 1'700'005, dimensions, 55U, 6U, 7U, earned);
	CHECK(rejected.outcome == telemetry_progression_outcome::sink_rejected);
	CHECK((rejected.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
	CHECK(rejected.record.record_seq == 5U);
	CHECK(sink.count == 4U);

	sink.accept = true;
	keys.fail = true;
	const auto exhausted = telemetry_progression_state_record(
		&state, session, connection, 1240U, 1'700'006, dimensions, 55U, 6U, 7U, earned);
	CHECK(exhausted.outcome == telemetry_progression_outcome::allocator_exhausted);
	CHECK((exhausted.quality_flags & TELEMETRY_QUALITY_SEQUENCE_GAP) != 0U);
	CHECK(state.accepted_total == 4U);
	CHECK(state.dropped_total == 2U);

	return 0;
}

} // namespace

int main()
{
	const int result = main_impl();
	std::puts("telemetry progression fact and arithmetic harness passed");
	return result;
}
