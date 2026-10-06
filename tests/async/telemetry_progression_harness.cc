#include "telemetry/telemetry_progression.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

telemetry_progression_context_observation progression_context()
{
	telemetry_progression_context_observation v{};
	v.producer = { 100U, 200U };
	v.sequence = 35U;
	v.session = { { v.producer, 7U }, 800U, 900, 3U, 4U };
	v.connection = { v.producer, 9U };
	v.source_record = { v.producer, 30U };
	v.ownership_record = { v.producer, 3U };
	v.account_token = 77U;
	v.start_monotonic_usec = 1'234U;
	v.at_monotonic_usec = 1'234U;
	v.start_utc_usec = 1'700'000;
	v.at_utc_usec = 1'700'000;
	auto &c = v.context;
	c.config_id = 55U;
	c.next_threshold_xp = 4'000U;
	c.current_exp = 1'125;
	c.primary_class_mask = 0x8000'0001U;
	c.secondary_class_mask = 0x4000'0002U;
	c.build_version = 17U;
	c.content_version = 19U;
	c.classifier_version = 6U;
	c.policy_version = 7U;
	c.flags = TELEMETRY_PCTX_ALIVE | TELEMETRY_PCTX_AFFECTS_COMPLETE |
		  TELEMETRY_PCTX_AUTOMATIC_RESTED | TELEMETRY_PCTX_RESTED_PRESENT |
		  TELEMETRY_PCTX_SELECTION_KNOWN | TELEMETRY_PCTX_APPLICATION_KNOWN |
		  TELEMETRY_PCTX_ASSISTANCE_KNOWN | TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN |
		  TELEMETRY_PCTX_THRESHOLD_KNOWN | TELEMETRY_PCTX_BUILD_KNOWN |
		  TELEMETRY_PCTX_GROUP_ROSTER_KNOWN | TELEMETRY_PCTX_STORAGE_GATE_KNOWN |
		  TELEMETRY_PCTX_STORAGE_GATE_PASSED;
	for (std::size_t index = 0U; index < TELEMETRY_PROGRESSION_CONTEXT_STATS; ++index)
	{
		c.base_stats[index] = static_cast<std::int16_t>(-100 - index);
		c.effective_stats[index] = static_cast<std::int16_t>(200 + index);
	}
	c.formal_group_size = 3U;
	c.current_level = 10U;
	c.threshold_level = 11U;
	c.threshold_catalog_version = 1U;
	c.specialization = 2U;
	c.race = 3U;
	c.faction = 4U;
	c.eligible_group_size = 2U;
	c.highest_group_level = 25U;
	c.version = TELEMETRY_PROGRESSION_CONTEXT_VERSION;
	c.rested_selection = telemetry_progression_rested_selection::rested;
	c.rested_application = telemetry_progression_rested_application::rested;
	c.assistance = telemetry_progression_assistance::group_kill_share;
	v.starting_level = 10U;
	v.source_inventory_version = TELEMETRY_PROGRESSION_SOURCE_INVENTORY_VERSION;
	v.boundary = telemetry_progression_context_boundary::experience;
	v.source_kind = telemetry_record_kind::progression;
	return v;
}

telemetry_progression_configuration_snapshot progression_configuration()
{
	using kind = telemetry_progression_configuration_value_kind;
	telemetry_progression_configuration_snapshot v{};
	v.config_id = 55U;
	v.build_version = 2U;
	v.content_version = 3U;
	v.classifier_version = 6U;
	v.policy_version = 7U;
	v.version = TELEMETRY_PROGRESSION_CONFIGURATION_VERSION;
	v.source_inventory_version = TELEMETRY_PROGRESSION_SOURCE_INVENTORY_VERSION;
	auto append = [&v](std::uint16_t id, kind type, std::uint64_t bits)
	{
		auto &entry = v.values[v.count++];
		entry.id = id;
		entry.kind = type;
		entry.bits = bits;
	};
	for (std::uint16_t id = 1U; id <= 62U; ++id)
		append(id, kind::signed_integer, id * 2'000U);
	for (std::uint16_t id = 100U; id <= 161U; ++id)
		append(id, kind::float32_bits, 0x3f800000U);
	for (std::uint16_t id = 200U; id <= 300U; ++id)
		append(id, kind::float32_bits, 0x3f800000U);
	for (std::uint16_t id = 400U; id <= 500U; ++id)
		append(id, kind::float32_bits, 0x3f800000U);
	append(600U, kind::signed_integer, 3'000'000U);
	for (std::uint16_t id = 601U; id <= 603U; ++id)
		append(id, kind::float64_bits, 0x3ff0000000000000ULL);
	for (std::uint16_t id = 604U; id <= 606U; ++id)
		append(id, kind::signed_integer, 15U);
	append(607U, kind::float32_bits, 0x3dcccccdU);
	append(608U, kind::signed_integer, 1U);
	append(609U, kind::signed_integer, 0U);
	append(610U, kind::signed_integer, 20U);
	return v;
}

telemetry_progression_configuration_observation
configuration_chunk(std::uint16_t index, const telemetry_progression_configuration_snapshot &source,
		    const std::uint8_t *digest)
{
	telemetry_progression_configuration_observation v{};
	v.producer = { 11U, 22U };
	v.sequence = 7U;
	v.root_record_seq = 5U;
	v.config_id = source.config_id;
	v.environment_id = 8U;
	v.season_id = 7U;
	v.at_monotonic_usec = 100U;
	v.at_utc_usec = 1'000'000;
	for (std::size_t word = 0U; word < 4U; ++word)
		for (std::size_t byte = 0U; byte < 8U; ++byte)
			v.digest_words[word] = (v.digest_words[word] << 8U) |
					       digest[word * 8U + byte];
	v.build_version = source.build_version;
	v.content_version = source.content_version;
	v.classifier_version = source.classifier_version;
	v.policy_version = source.policy_version;
	v.total_values = source.count;
	v.chunk_index = index;
	v.chunk_count = 15U;
	v.version = source.version;
	v.source_inventory_version = source.source_inventory_version;
	const auto first = index * TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	for (std::size_t ordinal = first;
	     ordinal < source.count &&
	     v.value_count < TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	     ++ordinal)
	{
		const auto target = v.value_count++;
		v.value_ids[target] = source.values[ordinal].id;
		v.value_kinds[target] = source.values[ordinal].kind;
		v.value_bits[target] = source.values[ordinal].bits;
	}
	return v;
}

void configuration_chunks_contract()
{
	const auto source = progression_configuration();
	std::uint8_t digest[32U]{};
	CHECK(telemetry_progression_configuration_digest(source, digest, sizeof(digest)));
	for (std::uint16_t index = 0U; index < 15U; ++index)
	{
		const auto v = configuration_chunk(index, source, digest);
		CHECK(telemetry_progression_configuration_observation_is_valid(v));
		std::uint8_t wire[TELEMETRY_PROGRESSION_CONFIGURATION_WIRE_BYTES]{};
		CHECK(telemetry_progression_configuration_observation_encode(v, wire,
									     sizeof(wire)));
		telemetry_progression_configuration_observation restored{};
		CHECK(telemetry_progression_configuration_observation_decode(wire, sizeof(wire),
									     &restored));
		CHECK(restored.value_count == (index == 14U ? 1U : 24U));
		CHECK(restored.root_record_seq == 5U && restored.chunk_index == index);
		CHECK(!telemetry_progression_configuration_observation_decode(
			wire, sizeof(wire) - 1U, &restored));
		CHECK(restored.sequence == 0U && restored.value_count == 0U);
		auto invalid = v;
		invalid.reserved = 1U;
		CHECK(!telemetry_progression_configuration_observation_is_valid(invalid));
		invalid = v;
		invalid.value_count -= 1U;
		CHECK(!telemetry_progression_configuration_observation_is_valid(invalid));
	}
	std::printf("PASS: exact XP configuration chunks: 15 / %zu native / %zu wire bytes\n",
		    sizeof(telemetry_progression_configuration_observation),
		    TELEMETRY_PROGRESSION_CONFIGURATION_WIRE_BYTES);
}

int verify_configuration_chunk(const char *text)
{
	std::uint8_t wire[TELEMETRY_PROGRESSION_CONFIGURATION_WIRE_BYTES]{};
	if (!text || std::strlen(text) != 2U * sizeof(wire))
		return 2;
	for (std::size_t index = 0U; index < sizeof(wire); ++index)
	{
		unsigned value = 0U;
		if (std::sscanf(text + index * 2U, "%2x", &value) != 1)
			return 2;
		wire[index] = static_cast<std::uint8_t>(value);
	}
	telemetry_progression_configuration_observation restored{};
	const bool valid = telemetry_progression_configuration_observation_decode(
		wire, sizeof(wire), &restored);
	if (!valid)
		CHECK(restored.sequence == 0U && restored.value_count == 0U);
	std::puts(valid ? "1" : "0");
	return 0;
}

void configuration_contract()
{
	const auto v = progression_configuration();
	CHECK(telemetry_progression_configuration_is_valid(v));
	std::uint8_t digest[32U]{}, changed[32U]{};
	std::uint8_t wire[TELEMETRY_PROGRESSION_CONFIGURATION_CANONICAL_BYTES]{};
	CHECK(telemetry_progression_configuration_encode(v, wire, sizeof(wire)));
	telemetry_progression_configuration_snapshot restored{};
	CHECK(telemetry_progression_configuration_decode(wire, sizeof(wire), v.config_id,
							 &restored));
	CHECK(telemetry_progression_configuration_is_valid(restored));
	CHECK(restored.values[336].bits == 20U && restored.config_id == v.config_id);
	CHECK(!telemetry_progression_configuration_decode(wire, sizeof(wire) - 1U, v.config_id,
							  &restored));
	CHECK(restored.count == 0U && restored.config_id == 0U);
	CHECK(telemetry_progression_configuration_digest(v, digest, sizeof(digest)));
	auto other = v;
	other.config_id += 1U;
	CHECK(telemetry_progression_configuration_digest(other, changed, sizeof(changed)));
	CHECK(std::memcmp(digest, changed, sizeof(digest)) == 0);
	other.content_version += 1U;
	CHECK(telemetry_progression_configuration_digest(other, changed, sizeof(changed)));
	CHECK(std::memcmp(digest, changed, sizeof(digest)) != 0);
	other = v;
	other.values[1].bits += 1U;
	CHECK(telemetry_progression_configuration_digest(other, changed, sizeof(changed)));
	CHECK(std::memcmp(digest, changed, sizeof(digest)) != 0);
	other = v;
	other.values[0].id = 2U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.values[62].bits = 0x7f800000U;
	CHECK(!telemetry_progression_configuration_digest(other, changed, sizeof(changed)));
	for (const auto byte : changed)
		CHECK(byte == 0U);
	other.values[62].bits = 0x7fc00000U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.values[327].bits = 0x7ff0000000000000ULL;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.values[v.count].bits = 1U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.values[0].reserved[4] = 1U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.count -= 1U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	other = v;
	other.version += 1U;
	CHECK(!telemetry_progression_configuration_is_valid(other));
	std::printf(
		"PASS: XP configuration inventory: %u exact values / %zu bytes; complete source and finite typed digest\n",
		v.count, sizeof(v));
}

void context_contract()
{
	const auto v = progression_context();
	CHECK(telemetry_progression_context_observation_is_valid(v));
	std::uint8_t wire[TELEMETRY_PROGRESSION_CONTEXT_WIRE_BYTES]{};
	CHECK(telemetry_progression_context_observation_encode(v, wire, sizeof(wire)));
	telemetry_progression_context_observation restored{};
	CHECK(telemetry_progression_context_observation_decode(wire, sizeof(wire), &restored));
	CHECK(telemetry_progression_context_observation_equal(v, restored));
	CHECK(!telemetry_progression_context_observation_decode(wire, sizeof(wire) - 1U,
								&restored));
	CHECK(restored.sequence == 0U && restored.context.version == 0U);
	auto invalid = v;
	invalid.context.reserved = 1U;
	CHECK(!telemetry_progression_context_observation_is_valid(invalid));
	invalid = v;
	invalid.reserved = 1U;
	CHECK(!telemetry_progression_context_observation_is_valid(invalid));
	CHECK(!telemetry_progression_context_observation_equal(v, invalid));
	CHECK(!telemetry_progression_context_observation_encode(invalid, wire, sizeof(wire)));
	std::printf("PASS: progression context value contract: %zu native / %zu wire bytes\n",
		    sizeof(v), sizeof(wire));
}

int verify_context(const char *text)
{
	std::uint8_t wire[TELEMETRY_PROGRESSION_CONTEXT_WIRE_BYTES]{};
	if (!text || std::strlen(text) != 2U * sizeof(wire))
		return 2;
	for (std::size_t index = 0U; index < sizeof(wire); ++index)
	{
		unsigned value = 0U;
		if (std::sscanf(text + index * 2U, "%2x", &value) != 1)
			return 2;
		wire[index] = static_cast<std::uint8_t>(value);
	}
	telemetry_progression_context_observation restored{};
	const bool valid =
		telemetry_progression_context_observation_decode(wire, sizeof(wire), &restored);
	if (!valid)
		CHECK(restored.context.version == 0U && restored.sequence == 0U);
	std::puts(valid ? "1" : "0");
	return 0;
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

int main(int argc, char **argv)
{
	if (argc == 3 && std::strcmp(argv[1], "--verify-context") == 0)
		return verify_context(argv[2]);
	if (argc == 2 && std::strcmp(argv[1], "--export-context") == 0)
	{
		std::uint8_t wire[TELEMETRY_PROGRESSION_CONTEXT_WIRE_BYTES]{};
		CHECK(telemetry_progression_context_observation_encode(progression_context(), wire,
								       sizeof(wire)));
		for (const auto value : wire)
			std::printf("%02x", static_cast<unsigned>(value));
		std::puts("");
		return 0;
	}
	if (argc == 2 && std::strcmp(argv[1], "--export-configuration") == 0)
	{
		const auto v = progression_configuration();
		std::uint8_t digest[32U]{};
		CHECK(telemetry_progression_configuration_digest(v, digest, sizeof(digest)));
		for (const auto byte : digest)
			std::printf("%02x", static_cast<unsigned>(byte));
		std::puts("");
		std::uint8_t wire[TELEMETRY_PROGRESSION_CONFIGURATION_CANONICAL_BYTES]{};
		CHECK(telemetry_progression_configuration_encode(v, wire, sizeof(wire)));
		for (const auto byte : wire)
			std::printf("%02x", static_cast<unsigned>(byte));
		std::puts("");
		return 0;
	}
	if (argc == 3 && std::strcmp(argv[1], "--verify-configuration-chunk") == 0)
		return verify_configuration_chunk(argv[2]);
	if (argc == 2 && std::strcmp(argv[1], "--export-configuration-chunks") == 0)
	{
		const auto source = progression_configuration();
		std::uint8_t digest[32U]{};
		CHECK(telemetry_progression_configuration_digest(source, digest, sizeof(digest)));
		for (std::uint16_t index = 0U; index < 15U; ++index)
		{
			const auto v = configuration_chunk(index, source, digest);
			std::uint8_t wire[TELEMETRY_PROGRESSION_CONFIGURATION_WIRE_BYTES]{};
			CHECK(telemetry_progression_configuration_observation_encode(v, wire,
										     sizeof(wire)));
			for (const auto byte : wire)
				std::printf("%02x", static_cast<unsigned>(byte));
			std::puts("");
		}
		return 0;
	}
	configuration_chunks_contract();
	configuration_contract();
	context_contract();
	const int result = main_impl();
	std::puts("telemetry progression fact and arithmetic harness passed");
	return result;
}
