#include "telemetry/telemetry_encounter.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace
{

struct sink_fixture
{
	telemetry_encounter_event events[512];
	std::size_t count;
	bool accept;
};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry encounter harness failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

bool emit_event(void *context, const telemetry_encounter_event &event) noexcept
{
	auto *sink = static_cast<sink_fixture *>(context);
	if (!sink->accept || sink->count == sizeof(sink->events) / sizeof(sink->events[0]))
		return false;
	CHECK(telemetry_encounter_payload_is_valid(event));
	sink->events[sink->count++] = event;
	return true;
}

telemetry_encounter_id id(telemetry_producer_id producer, telemetry_sequence sequence)
{
	return { producer, sequence };
}

telemetry_encounter_participant participant(telemetry_pid pid)
{
	return { static_cast<telemetry_subject_id>(pid), pid };
}

telemetry_encounter_source source(telemetry_id group_key)
{
	return { 11U, 22U, 33U, 4U, 5U, 700, group_key };
}

std::size_t count_kind(const sink_fixture &sink, telemetry_encounter_event_kind kind)
{
	std::size_t count = 0U;
	for (std::size_t index = 0U; index < sink.count; ++index)
		if (sink.events[index].kind == kind)
			++count;
	return count;
}

void test_roster_effort_and_idempotent_close()
{
	const telemetry_producer_id producer = { 101U, 202U };
	telemetry_encounter_state state{};
	telemetry_encounter_state_init(&state);
	sink_fixture sink{ {}, 0U, true };
	const auto run = id(producer, 1U);
	const auto run_source = source(900U);
	CHECK(telemetry_encounter_begin(&state, run, run_source, telemetry_encounter_mode::pve,
					participant(1), 0U, 1U, emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::accepted);
	for (telemetry_pid pid = 2; pid <= 10; ++pid)
	{
		const auto result = telemetry_encounter_begin(
			&state, id(producer, static_cast<telemetry_sequence>(pid)), run_source,
			telemetry_encounter_mode::pve, participant(pid), 0U, 1U, emit_event, &sink);
		CHECK(result.outcome == telemetry_encounter_update_outcome::joined_existing);
	}
	CHECK(telemetry_encounter_leave(&state, participant(1),
					telemetry_encounter_outcome::withdrawal,
					1'800'000'000ULL, 2U, emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::accepted);
	CHECK(telemetry_encounter_begin(&state, id(producer, 12U), run_source,
					telemetry_encounter_mode::pve, participant(11),
					1'800'000'000ULL, 2U, emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::joined_existing);
	const auto closed = telemetry_encounter_close(
		&state, run, telemetry_encounter_outcome::success, 10U, 3'600'000'000ULL, 3U,
		emit_event, &sink);
	CHECK(closed.outcome == telemetry_encounter_update_outcome::accepted);
	CHECK(count_kind(sink, telemetry_encounter_event_kind::close) == 1U);
	CHECK(count_kind(sink, telemetry_encounter_event_kind::participant_summary) == 11U);
	std::uint64_t participant_total = 0U;
	for (std::size_t index = 0U; index < sink.count; ++index)
	{
		const auto &event = sink.events[index];
		if (event.kind == telemetry_encounter_event_kind::close)
		{
			CHECK(event.elapsed_usec == 3'600'000'000ULL);
			CHECK(event.expected_credit_count == 10U);
			CHECK(event.participant_count == 11U);
		}
		if (event.kind == telemetry_encounter_event_kind::participant_summary)
			participant_total += event.participant_usec;
	}
	CHECK(participant_total == 36'000'000'000ULL);
	const std::size_t before_duplicate = sink.count;
	CHECK(telemetry_encounter_close(&state, run, telemetry_encounter_outcome::success, 10U,
					3'600'000'000ULL, 3U, emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::idempotent);
	CHECK(sink.count == before_duplicate);
	CHECK(telemetry_encounter_close(&state, run, telemetry_encounter_outcome::failure, 10U,
					3'600'000'000ULL, 3U, emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::duplicate_conflict);
}

void test_terminal_outcomes_remain_observable()
{
	const telemetry_producer_id producer = { 303U, 404U };
	const telemetry_encounter_outcome outcomes[] = {
		telemetry_encounter_outcome::success,
		telemetry_encounter_outcome::failure,
		telemetry_encounter_outcome::withdrawal,
		telemetry_encounter_outcome::death,
	};
	for (std::size_t index = 0U; index < sizeof(outcomes) / sizeof(outcomes[0]); ++index)
	{
		telemetry_encounter_state state{};
		telemetry_encounter_state_init(&state);
		sink_fixture sink{ {}, 0U, true };
		const auto run = id(producer, static_cast<telemetry_sequence>(index + 1U));
		CHECK(telemetry_encounter_begin(&state, run, source(1000U + index),
					telemetry_encounter_mode::mixed, participant(50 + index),
					10U, 1U, emit_event, &sink)
			      .outcome == telemetry_encounter_update_outcome::accepted);
		CHECK(telemetry_encounter_close(&state, run, outcomes[index], 0U, 20U, 2U,
						emit_event, &sink)
			      .outcome == telemetry_encounter_update_outcome::accepted);
		CHECK(sink.events[sink.count - 2U].kind == telemetry_encounter_event_kind::close);
		CHECK(sink.events[sink.count - 2U].outcome == outcomes[index]);
	}
}

void test_bounded_state_and_quality()
{
	static_assert(TELEMETRY_ENCOUNTER_MAX_ACTIVE == 128U);
	static_assert(TELEMETRY_ENCOUNTER_MAX_PARTICIPANTS == 64U);
	static_assert(TELEMETRY_ENCOUNTER_MAX_EVENTS_PER_RUN == 256U);
	const telemetry_producer_id producer = { 505U, 606U };
	telemetry_encounter_state state{};
	telemetry_encounter_state_init(&state);
	sink_fixture sink{ {}, 0U, true };
	for (std::size_t index = 0U; index < TELEMETRY_ENCOUNTER_MAX_ACTIVE; ++index)
	{
		const auto result = telemetry_encounter_begin(
			&state, id(producer, index + 1U), source(2000U + index),
			telemetry_encounter_mode::pve, participant(static_cast<telemetry_pid>(1000 + index)),
			0U, 1U, emit_event, &sink);
		CHECK(result.outcome == telemetry_encounter_update_outcome::accepted);
	}
	CHECK(telemetry_encounter_begin(&state, id(producer, 1000U), source(9999U),
					telemetry_encounter_mode::pve, participant(5000), 0U, 1U,
					emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::capacity_full);

	telemetry_encounter_state participants{};
	telemetry_encounter_state_init(&participants);
	CHECK(telemetry_encounter_begin(&participants, id(producer, 2000U), source(3000U),
					telemetry_encounter_mode::pve, participant(1), 0U, 1U,
					emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::accepted);
	for (telemetry_pid pid = 2; pid <= 64; ++pid)
		CHECK(telemetry_encounter_begin(&participants, id(producer, 2000U + pid),
					 source(3000U), telemetry_encounter_mode::pve, participant(pid),
					 1U, 1U, emit_event, &sink)
			      .outcome == telemetry_encounter_update_outcome::joined_existing);
	CHECK(telemetry_encounter_begin(&participants, id(producer, 3000U), source(3000U),
					telemetry_encounter_mode::pve, participant(65), 1U, 1U,
					emit_event, &sink)
		      .outcome == telemetry_encounter_update_outcome::capacity_full);

	telemetry_encounter_state events{};
	telemetry_encounter_state_init(&events);
	sink_fixture event_sink{ {}, 0U, true };
	CHECK(telemetry_encounter_begin(&events, id(producer, 4000U), source(4000U),
					telemetry_encounter_mode::pve, participant(1), 0U, 1U,
					emit_event, &event_sink)
		      .outcome == telemetry_encounter_update_outcome::accepted);
	CHECK(telemetry_encounter_begin(&events, id(producer, 4001U), source(4000U),
					telemetry_encounter_mode::pve, participant(2), 1U, 1U,
					emit_event, &event_sink)
		      .outcome == telemetry_encounter_update_outcome::joined_existing);
	for (unsigned index = 0U; index <= 126U; ++index)
	{
		const auto left = telemetry_encounter_leave(
			&events, participant(1), telemetry_encounter_outcome::withdrawal,
			2U + index * 2U, 1U, emit_event, &event_sink);
		CHECK(left.outcome == telemetry_encounter_update_outcome::accepted);
		const auto joined = telemetry_encounter_begin(
			&events, id(producer, 5000U + index), source(4000U), telemetry_encounter_mode::pve,
			participant(1), 3U + index * 2U, 1U, emit_event, &event_sink);
		if (index == 126U)
		{
			CHECK((joined.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
			break;
		}
		CHECK(joined.outcome == telemetry_encounter_update_outcome::joined_existing);
	}

	telemetry_encounter_state rejected{};
	telemetry_encounter_state_init(&rejected);
	sink_fixture rejected_sink{ {}, 0U, false };
	const auto result = telemetry_encounter_begin(
		&rejected, id(producer, 6000U), source(5000U), telemetry_encounter_mode::pvp,
		participant(1), 0U, 1U, emit_event, &rejected_sink);
	CHECK(result.outcome == telemetry_encounter_update_outcome::sink_rejected);
	CHECK((result.quality_flags & TELEMETRY_QUALITY_QUEUE_DROP) != 0U);
}

} // namespace

int main()
{
	test_roster_effort_and_idempotent_close();
	test_terminal_outcomes_remain_observable();
	test_bounded_state_and_quality();
	std::puts("telemetry encounter bounded lifecycle harness passed");
	return 0;
}
