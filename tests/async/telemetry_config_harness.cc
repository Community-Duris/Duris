#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_runtime.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <openssl/sha.h>

namespace
{

constexpr const char *GOLDEN_FINGERPRINT =
	"71daf2a6e0a9faa4f200f855b88426d6cac93b9b3cead7d66046ac0263e0daf7";
constexpr std::uint32_t GOLDEN_PROPERTY_VERSION = 1867102185U;
constexpr telemetry_config_id GOLDEN_CONFIG_ID = 8204136469756508836ULL;

struct property_values
{
	float observe;
	float max_payout_factor;
	float payout_factor;
	float alignment_mod;
	float minimum_alignment;
	std::size_t calls;
	std::size_t frequency_calls;
};

struct reviewed_property_catalog
{
	struct entry
	{
		std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES];
		std::uint32_t property_version;
		std::uint32_t stable_namespace;
		std::uint32_t stable_version;
	};
	entry entries[8];
	std::size_t count;
	bool reject_version_collision;
};

struct finite_sink
{
	bool accept;
	telemetry_record records[16];
	std::size_t count;
};

struct key_space
{
	telemetry_producer_id producer;
	telemetry_record_sequence next_sequence;
	std::size_t calls;
};

void check(bool condition, const char *expression, int line)
{
	if (!condition)
	{
		std::fprintf(stderr, "telemetry config focused failure at line %d: %s\n", line,
			     expression);
		std::abort();
	}
}

#define CHECK(expression) check((expression), #expression, __LINE__)

void catalog_add(reviewed_property_catalog &catalog,
		 const telemetry_config_property_snapshot &snapshot,
		 std::uint32_t property_version = 0U)
{
	CHECK(catalog.count < sizeof(catalog.entries) / sizeof(catalog.entries[0]));
	auto &entry = catalog.entries[catalog.count++];
	std::memcpy(entry.digest, snapshot.effective_digest, sizeof(entry.digest));
	entry.property_version = property_version == 0U ? snapshot.property_version :
							  property_version;
	entry.stable_namespace = 1U;
	entry.stable_version = 1U;
}

bool resolve_reviewed_catalog(void *context, const telemetry_config_property_snapshot *snapshot,
			      const std::uint8_t *digest,
			      telemetry_config_property_catalog_resolution *resolution) noexcept
{
	if (context == nullptr || snapshot == nullptr || digest == nullptr || resolution == nullptr)
		return false;
	auto &catalog = *static_cast<reviewed_property_catalog *>(context);
	for (std::size_t index = 0U; index < catalog.count; ++index)
	{
		const auto &entry = catalog.entries[index];
		if (std::memcmp(entry.digest, digest, sizeof(entry.digest)) != 0)
			continue;
		if (catalog.reject_version_collision)
			for (std::size_t other = 0U; other < catalog.count; ++other)
				if (other != index &&
				    catalog.entries[other].property_version ==
					    entry.property_version &&
				    std::memcmp(catalog.entries[other].digest, digest,
						sizeof(entry.digest)) != 0)
					return false;
		*resolution = {};
		resolution->property_version = entry.property_version;
		resolution->stable_namespace = entry.stable_namespace;
		resolution->stable_version = entry.stable_version;
		std::memcpy(resolution->digest, entry.digest, sizeof(resolution->digest));
		return snapshot->property_version != 0U;
	}
	return false;
}

bool property_read(void *context, const char *key, float *value) noexcept
{
	auto *properties = static_cast<property_values *>(context);
	++properties->calls;
	if (std::strncmp(key, "epic.freqMod.", 13U) == 0)
	{
		++properties->frequency_calls;
		return false;
	}
	if (std::strcmp(key, "exp.zoneTrophy.observe") == 0)
		*value = properties->observe;
	else if (std::strcmp(key, "epic.touch.maxPayoutFactor") == 0)
		*value = properties->max_payout_factor;
	else if (std::strcmp(key, "epic.touch.PayoutFactor") == 0)
		*value = properties->payout_factor;
	else if (std::strcmp(key, "epic.zone.alignmentMod") == 0)
		*value = properties->alignment_mod;
	else if (std::strcmp(key, "epic.alignment.minPercentage") == 0)
		*value = properties->minimum_alignment;
	else
		return false;
	return true;
}

float float_from_bits(std::uint32_t bits) noexcept
{
	float value = 0.0F;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

telemetry_config_property_capture make_capture(property_values &values,
					       reviewed_property_catalog *catalog = nullptr)
{
	telemetry_config_property_capture capture{};
	capture.reader = { property_read, &values };
	capture.mode = telemetry_config_property_capture_mode::require_reader;
	if (catalog != nullptr)
		capture.catalog = { resolve_reviewed_catalog, catalog };
	return capture;
}

telemetry_config_capture_input
make_input(telemetry_config_revision revision, telemetry_utc_usec effective_utc_usec,
	   std::uint32_t policy_version, telemetry_config_property_capture properties,
	   std::uint8_t enabled = 1U,
	   telemetry_storage_backend backend = telemetry_storage_backend::sql)
{
	telemetry_config_capture_input input{};
	input.schema_version = TELEMETRY_SCHEMA_VERSION;
	input.revision = revision;
	input.build_version = 0x01020304U;
	input.content_version = 0x05060708U;
	input.classifier_version = 0x11121314U;
	input.policy_version = policy_version;
	input.season_id = 0x0102030405060708ULL;
	input.environment_id = 0x1112131415161718ULL;
	input.effective_utc_usec = effective_utc_usec;
	input.interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
	input.checkpoint_interval_usec = 120'000'000ULL;
	input.active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL;
	input.context_segments_per_minute = TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL;
	input.pulse_slot_count = 16U;
	input.backend = backend;
	input.enabled = enabled;
	input.properties = properties;
	return input;
}

telemetry_config_snapshot build_snapshot(const telemetry_config_capture_input &input,
					 telemetry_config_property_snapshot *properties = nullptr)
{
	telemetry_config_snapshot snapshot{};
	const telemetry_config_build_outcome outcome =
		telemetry_config_snapshot_build(&input, properties, &snapshot);
	CHECK(outcome == telemetry_config_build_outcome::built);
	CHECK(telemetry_config_is_valid(snapshot));
	return snapshot;
}

bool sink_emit(void *context, const telemetry_record *record) noexcept
{
	auto *sink = static_cast<finite_sink *>(context);
	if (!sink->accept || sink->count >= sizeof(sink->records) / sizeof(sink->records[0]))
		return false;
	sink->records[sink->count++] = *record;
	return true;
}

bool next_key(void *context, telemetry_record_kind kind, telemetry_record_key *key) noexcept
{
	auto *space = static_cast<key_space *>(context);
	CHECK(kind == telemetry_record_kind::configuration);
	++space->calls;
	if (space->next_sequence == 0U)
		return false;
	*key = { space->producer, space->next_sequence++ };
	return true;
}

telemetry_config_state_config state_config(finite_sink *sink, key_space *keys)
{
	telemetry_config_state_config config{};
	config.sink = { sink_emit, sink };
	config.key_allocator = { next_key, keys };
	return config;
}

std::uint32_t reference_append(std::uint8_t *bytes, std::size_t offset, std::uint64_t value,
			       unsigned int width)
{
	for (unsigned int index = width; index != 0U; --index)
		bytes[offset + width - index] =
			static_cast<std::uint8_t>((value >> ((index - 1U) * 8U)) & 0xFFU);
	return static_cast<std::uint32_t>(offset + width);
}

void reference_fingerprint(const telemetry_config_snapshot &config,
			   std::uint8_t fingerprint[TELEMETRY_CONFIG_FINGERPRINT_BYTES])
{
	std::uint8_t bytes[TELEMETRY_CONFIG_CANONICAL_BYTES]{};
	std::size_t offset = 0U;
	offset = reference_append(bytes, offset, config.schema_version, 2U);
	offset = reference_append(bytes, offset, config.build_version, 4U);
	offset = reference_append(bytes, offset, config.content_version, 4U);
	offset = reference_append(bytes, offset, config.property_version, 4U);
	offset = reference_append(bytes, offset, config.classifier_version, 4U);
	offset = reference_append(bytes, offset, config.policy_version, 4U);
	offset = reference_append(bytes, offset, config.season_id, 8U);
	offset = reference_append(bytes, offset, config.environment_id, 8U);
	offset = reference_append(bytes, offset, config.interval_usec, 8U);
	offset = reference_append(bytes, offset, config.checkpoint_interval_usec, 8U);
	offset = reference_append(bytes, offset, config.active_window_usec, 8U);
	offset = reference_append(bytes, offset, config.context_segments_per_minute, 4U);
	offset = reference_append(bytes, offset, config.pulse_slot_count, 2U);
	offset = reference_append(bytes, offset, static_cast<std::uint8_t>(config.backend), 1U);
	offset = reference_append(bytes, offset, config.enabled, 1U);
	CHECK(offset == TELEMETRY_CONFIG_CANONICAL_BYTES);
	CHECK(SHA256(bytes, sizeof(bytes), fingerprint) != nullptr);
}

void bytes_hex(const std::uint8_t *bytes, std::size_t size, char *output, std::size_t capacity)
{
	static constexpr char HEX[] = "0123456789abcdef";
	CHECK(capacity >= size * 2U + 1U);
	for (std::size_t index = 0U; index < size; ++index)
	{
		output[index * 2U] = HEX[bytes[index] >> 4U];
		output[index * 2U + 1U] = HEX[bytes[index] & 0x0FU];
	}
	output[size * 2U] = '\0';
}

const telemetry_config_property_definition *find_definition(const char *name)
{
	std::size_t count = 0U;
	const auto *registry = telemetry_config_property_registry(&count);
	for (std::size_t index = 0U; index < count; ++index)
		if (std::strcmp(registry[index].stable_name, name) == 0)
			return &registry[index];
	return nullptr;
}

void test_registry_and_capture(property_values &values, telemetry_config_snapshot &golden,
			       telemetry_config_property_snapshot &golden_properties,
			       reviewed_property_catalog &catalog)
{
	std::size_t count = 0U;
	const auto *registry = telemetry_config_property_registry(&count);
	CHECK(registry != nullptr);
	CHECK(count == TELEMETRY_CONFIG_PROPERTY_MAX);
	CHECK(find_definition("rested.xp_multiplier") != nullptr);
	CHECK(find_definition("wellrested.xp_multiplier") != nullptr);
	CHECK(find_definition("trophy.exp.zoneTrophy.observe") != nullptr);
	CHECK(find_definition("payout.epic.touch.PayoutFactor") != nullptr);
	CHECK(find_definition("unused.epic.freqMod.tick.waitSecs") != nullptr);
	CHECK(!telemetry_config_property_name_is_allowlisted("DURISWEB_SECRET"));
	CHECK(!telemetry_config_property_name_is_allowlisted("DB_PASSWD"));
	CHECK(telemetry_config_property_name_is_allowlisted("epic.zone.alignmentMod"));

	telemetry_config_property_capture defaults{};
	defaults.mode = telemetry_config_property_capture_mode::declared_defaults;
	telemetry_config_property_snapshot default_snapshot{};
	CHECK(telemetry_config_property_snapshot_capture(&defaults, &default_snapshot) ==
	      telemetry_config_build_outcome::built);
	CHECK(default_snapshot.property_version != 0U);
	CHECK(values.frequency_calls == 0U);

	property_values probe = values;
	const telemetry_config_property_capture probe_capture = make_capture(probe);
	telemetry_config_property_snapshot probe_snapshot{};
	CHECK(telemetry_config_property_snapshot_capture(&probe_capture, &probe_snapshot) ==
	      telemetry_config_build_outcome::built);
	catalog_add(catalog, probe_snapshot);
	telemetry_config_property_capture live = make_capture(values, &catalog);
	CHECK(telemetry_config_property_snapshot_capture(&live, &golden_properties) ==
	      telemetry_config_build_outcome::built);
	CHECK(golden_properties.property_version != default_snapshot.property_version);
	CHECK(values.frequency_calls == 0U);
	CHECK(values.calls == 5U);
	property_values conversion = values;
	conversion.observe = 1.9f;
	telemetry_config_property_capture conversion_capture{};
	conversion_capture.reader = { property_read, &conversion };
	conversion_capture.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_property_snapshot conversion_snapshot{};
	CHECK(telemetry_config_property_snapshot_capture(&conversion_capture,
							 &conversion_snapshot) ==
	      telemetry_config_build_outcome::built);
	CHECK(conversion_snapshot.entries[3].value == 1U);
	for (std::size_t index = 0U; index < count; ++index)
	{
		if (registry[index].role == telemetry_config_property_role::maintained_but_unused)
			CHECK(golden_properties.entries[index].source ==
			      static_cast<std::uint8_t>(
				      telemetry_config_property_source::excluded_unused));
	}

	const telemetry_config_capture_input input = make_input(1U, 123456789LL, 0x15161718U, live);
	golden = build_snapshot(input, &golden_properties);
	CHECK(golden.property_version == golden_properties.property_version);
	CHECK(golden.property_version == GOLDEN_PROPERTY_VERSION);
	CHECK(golden.config_id == GOLDEN_CONFIG_ID);
	CHECK(golden.config_id != 0U);
	std::uint8_t canonical[TELEMETRY_CONFIG_CANONICAL_BYTES]{};
	CHECK(telemetry_config_canonical_encode(golden, canonical, sizeof(canonical)) ==
	      TELEMETRY_CONFIG_CANONICAL_BYTES);
	std::uint8_t reference[TELEMETRY_CONFIG_FINGERPRINT_BYTES]{};
	reference_fingerprint(golden, reference);
	CHECK(std::memcmp(reference, golden.fingerprint, sizeof(reference)) == 0);

	char actual_hex[TELEMETRY_CONFIG_FINGERPRINT_BYTES * 2U + 1U]{};
	bytes_hex(golden.fingerprint, sizeof(golden.fingerprint), actual_hex, sizeof(actual_hex));
	if (GOLDEN_FINGERPRINT[0] != '\0')
		CHECK(std::strcmp(actual_hex, GOLDEN_FINGERPRINT) == 0);
	std::fprintf(stdout, "golden fingerprint=%s property_version=%u config_id=%llu\n",
		     actual_hex, golden.property_version,
		     static_cast<unsigned long long>(golden.config_id));

	telemetry_config_capture_input missing_input = input;
	missing_input.properties.reader = {};
	missing_input.properties.mode = telemetry_config_property_capture_mode::require_reader;
	telemetry_config_snapshot missing_snapshot{};
	CHECK(telemetry_config_snapshot_build(&missing_input, nullptr, &missing_snapshot) ==
	      telemetry_config_build_outcome::missing_property_context);
	CHECK(missing_snapshot.config_id == 0U);
	telemetry_config_capture_input unresolved_input = input;
	unresolved_input.properties = make_capture(values);
	telemetry_config_snapshot unresolved_snapshot{};
	CHECK(telemetry_config_snapshot_build(&unresolved_input, nullptr, &unresolved_snapshot) ==
	      telemetry_config_build_outcome::property_catalog_unavailable);

	property_values collision_a = values;
	property_values collision_b = values;
	collision_a.payout_factor = float_from_bits(0x3F00908CU);
	collision_b.payout_factor = float_from_bits(0x3F01C902U);
	const telemetry_config_property_capture collision_a_capture = make_capture(collision_a);
	const telemetry_config_property_capture collision_b_capture = make_capture(collision_b);
	telemetry_config_property_snapshot collision_a_snapshot{};
	telemetry_config_property_snapshot collision_b_snapshot{};
	CHECK(telemetry_config_property_snapshot_capture(&collision_a_capture,
							 &collision_a_snapshot) ==
	      telemetry_config_build_outcome::built);
	CHECK(telemetry_config_property_snapshot_capture(&collision_b_capture,
							 &collision_b_snapshot) ==
	      telemetry_config_build_outcome::built);
	CHECK(std::memcmp(collision_a_snapshot.effective_digest,
			  collision_b_snapshot.effective_digest,
			  TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES) != 0);
	reviewed_property_catalog collision_catalog{};
	catalog_add(collision_catalog, collision_a_snapshot, 1994669249U);
	catalog_add(collision_catalog, collision_b_snapshot, 1994669249U);
	collision_catalog.reject_version_collision = true;
	telemetry_config_capture_input collision_input = input;
	collision_input.properties = make_capture(collision_a, &collision_catalog);
	telemetry_config_snapshot collision_result{};
	CHECK(telemetry_config_snapshot_build(&collision_input, nullptr, &collision_result) ==
	      telemetry_config_build_outcome::property_registry_invalid);
}

void test_canonical_identity(const telemetry_config_snapshot &golden,
			     const telemetry_config_capture_input &input,
			     telemetry_config_property_snapshot &properties)
{
	telemetry_config_capture_input later = input;
	later.revision = 99U;
	later.effective_utc_usec = 987654321LL;
	const telemetry_config_snapshot same_effective = build_snapshot(later, &properties);
	CHECK(same_effective.config_id == golden.config_id);
	CHECK(std::memcmp(same_effective.fingerprint, golden.fingerprint,
			  TELEMETRY_CONFIG_FINGERPRINT_BYTES) == 0);
	CHECK(same_effective.revision != golden.revision);
	CHECK(same_effective.effective_utc_usec != golden.effective_utc_usec);

	property_values changed{ 0.0f, 10.0f, 1.25f, 0.2f, 0.15f, 0U, 0U };
	auto *catalog = static_cast<reviewed_property_catalog *>(input.properties.catalog.context);
	property_values changed_probe_values = changed;
	const telemetry_config_property_capture changed_probe_capture =
		make_capture(changed_probe_values);
	telemetry_config_property_snapshot changed_probe{};
	CHECK(telemetry_config_property_snapshot_capture(&changed_probe_capture, &changed_probe) ==
	      telemetry_config_build_outcome::built);
	catalog_add(*catalog, changed_probe);
	telemetry_config_property_capture changed_capture = make_capture(changed, catalog);
	telemetry_config_capture_input changed_input = input;
	changed_input.revision = 2U;
	changed_input.properties = changed_capture;
	const telemetry_config_snapshot changed_snapshot =
		build_snapshot(changed_input, &properties);
	CHECK(changed_snapshot.property_version != golden.property_version);
	CHECK(changed_snapshot.config_id != golden.config_id);
	CHECK(std::memcmp(changed_snapshot.fingerprint, golden.fingerprint,
			  TELEMETRY_CONFIG_FINGERPRINT_BYTES) != 0);
}

void test_state_impl(const telemetry_config_snapshot &golden,
		     const telemetry_config_snapshot &changed,
		     const telemetry_config_snapshot &same_effective)
{
	finite_sink sink{ true, {}, 0U };
	key_space keys{ { 0x10101010ULL, 0x20202020ULL }, 1U, 0U };
	const telemetry_config_state_config configuration = state_config(&sink, &keys);
	telemetry_config_state state{};
	CHECK(telemetry_config_state_init(&state, &configuration) ==
	      telemetry_config_state_outcome::accepted);
	telemetry_config_state_result result = telemetry_config_state_publish(&state, golden);
	CHECK(result.outcome == telemetry_config_state_outcome::accepted);
	CHECK(result.admission_accepted == 1U);
	CHECK(result.record_key.record_seq == 1U);
	CHECK(sink.count == 1U);
	CHECK(telemetry_record_is_valid(sink.records[0]));
	CHECK(telemetry_config_state_status(&state) == telemetry_config_validation::valid_sql);
	CHECK(telemetry_config_state_snapshot_copy(&state).config_id == golden.config_id);

	result = telemetry_config_state_publish(&state, changed);
	CHECK(result.outcome == telemetry_config_state_outcome::accepted);
	CHECK(result.record_key.record_seq == 2U);
	CHECK(sink.count == 2U);
	CHECK(telemetry_config_state_snapshot_copy(&state).config_id == changed.config_id);

	result = telemetry_config_state_publish(&state, same_effective);
	CHECK(result.outcome == telemetry_config_state_outcome::accepted);
	CHECK(result.snapshot.revision == same_effective.revision);
	CHECK(sink.count == 3U);
	CHECK(sink.records[2].payload.configuration.config.revision == golden.revision);
	CHECK(sink.records[2].payload.configuration.config.effective_utc_usec ==
	      golden.effective_utc_usec);
	CHECK(sink.records[2].header.occurrence_utc_usec == golden.effective_utc_usec);

	telemetry_config_state conflict_state{};
	CHECK(telemetry_config_state_init(&conflict_state, &configuration) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_publish(&conflict_state, golden).outcome ==
	      telemetry_config_state_outcome::accepted);
	telemetry_config_snapshot same_revision = changed;
	same_revision.revision = golden.revision;
	CHECK(telemetry_config_compute_fingerprint(same_revision, same_revision.fingerprint,
						   sizeof(same_revision.fingerprint)));
	same_revision.config_id = telemetry_config_id_from_fingerprint(
		same_revision.fingerprint, sizeof(same_revision.fingerprint));
	CHECK(telemetry_config_state_publish(&conflict_state, same_revision).outcome ==
	      telemetry_config_state_outcome::revision_conflict);
	CHECK(telemetry_config_state_snapshot_copy(&conflict_state).config_id == 0U);

	telemetry_config_snapshot collision = changed;
	collision.config_id = golden.config_id;
	result = telemetry_config_state_publish(&state, collision);
	CHECK(result.outcome == telemetry_config_state_outcome::identity_collision);
	telemetry_config_snapshot remapped = golden;
	remapped.config_id = golden.config_id + 1U;
	result = telemetry_config_state_publish(&state, remapped);
	CHECK(result.outcome == telemetry_config_state_outcome::identity_collision);

	finite_sink retry_sink{ false, {}, 0U };
	key_space retry_keys{ { 0x30303030ULL, 0x40404040ULL }, 7U, 0U };
	const telemetry_config_state_config retry_configuration =
		state_config(&retry_sink, &retry_keys);
	telemetry_config_state retry_state{};
	CHECK(telemetry_config_state_init(&retry_state, &retry_configuration) ==
	      telemetry_config_state_outcome::accepted);
	result = telemetry_config_state_publish(&retry_state, golden);
	CHECK(result.outcome == telemetry_config_state_outcome::sink_rejected);
	CHECK(telemetry_config_state_snapshot_copy(&retry_state).config_id == 0U);
	CHECK(telemetry_config_state_status(&retry_state) ==
	      telemetry_config_validation::missing_identity);
	CHECK(telemetry_config_state_stats_copy(&retry_state).pending_count == 1U);
	CHECK(retry_keys.calls == 1U);
	result = telemetry_config_state_publish(&retry_state, golden);
	CHECK(result.outcome == telemetry_config_state_outcome::sink_rejected);
	CHECK(retry_keys.calls == 1U);
	retry_sink.accept = true;
	result = telemetry_config_state_publish(&retry_state, golden);
	CHECK(result.outcome == telemetry_config_state_outcome::accepted);
	CHECK(result.record_key.record_seq == 7U);
	CHECK(retry_keys.calls == 1U);
	CHECK(telemetry_config_state_snapshot_copy(&retry_state).config_id == golden.config_id);

	finite_sink transition_sink{ true, {}, 0U };
	key_space transition_keys{ { 0x50505050ULL, 0x60606060ULL }, 1U, 0U };
	const telemetry_config_state_config transition_configuration =
		state_config(&transition_sink, &transition_keys);
	telemetry_config_state transition_state{};
	CHECK(telemetry_config_state_init(&transition_state, &transition_configuration) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_publish(&transition_state, golden).outcome ==
	      telemetry_config_state_outcome::accepted);
	transition_sink.accept = false;
	CHECK(telemetry_config_state_publish(&transition_state, changed).outcome ==
	      telemetry_config_state_outcome::sink_rejected);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == 0U);
	transition_sink.accept = true;
	CHECK(telemetry_config_state_publish(&transition_state, changed).outcome ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id ==
	      changed.config_id);
	telemetry_config_snapshot invalid = changed;
	invalid.fingerprint[0] ^= 0xFFU;
	CHECK(telemetry_config_state_publish(&transition_state, invalid).outcome ==
	      telemetry_config_state_outcome::invalid);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == 0U);
	CHECK(telemetry_config_state_publish(&transition_state, changed).outcome ==
	      telemetry_config_state_outcome::unchanged);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == 0U);
	telemetry_config_snapshot next = changed;
	next.revision = 3U;
	next.policy_version += 1U;
	CHECK(telemetry_config_compute_fingerprint(next, next.fingerprint,
						   sizeof(next.fingerprint)));
	next.config_id =
		telemetry_config_id_from_fingerprint(next.fingerprint, sizeof(next.fingerprint));
	CHECK(telemetry_config_state_publish(&transition_state, next).outcome ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == next.config_id);
	telemetry_config_snapshot collision_after_invalid = next;
	collision_after_invalid.config_id = golden.config_id;
	CHECK(telemetry_config_state_publish(&transition_state, collision_after_invalid).outcome ==
	      telemetry_config_state_outcome::identity_collision);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == 0U);
	CHECK(telemetry_config_state_publish(&transition_state, next).outcome ==
	      telemetry_config_state_outcome::unchanged);
	CHECK(telemetry_config_state_snapshot_copy(&transition_state).config_id == 0U);

	telemetry_config_state bounded{};
	finite_sink bounded_sink{ false, {}, 0U };
	key_space bounded_keys{ { 0x70707070ULL, 0x80808080ULL }, 1U, 0U };
	const telemetry_config_state_config bounded_configuration =
		state_config(&bounded_sink, &bounded_keys);
	CHECK(telemetry_config_state_init(&bounded, &bounded_configuration) ==
	      telemetry_config_state_outcome::accepted);
	telemetry_config_snapshot candidates[5] = { golden, changed, changed, changed, changed };
	for (std::size_t index = 1U; index < 5U; ++index)
	{
		candidates[index] = changed;
		candidates[index].revision = static_cast<telemetry_config_revision>(index + 1U);
		candidates[index].policy_version += static_cast<std::uint32_t>(index);
		CHECK(telemetry_config_compute_fingerprint(candidates[index],
							   candidates[index].fingerprint,
							   sizeof(candidates[index].fingerprint)));
		candidates[index].config_id = telemetry_config_id_from_fingerprint(
			candidates[index].fingerprint, sizeof(candidates[index].fingerprint));
		CHECK(telemetry_config_is_valid(candidates[index]));
	}
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_PENDING_MAX; ++index)
		CHECK(telemetry_config_state_publish(&bounded, candidates[index]).outcome ==
		      telemetry_config_state_outcome::sink_rejected);
	CHECK(telemetry_config_state_stats_copy(&bounded).pending_count ==
	      TELEMETRY_CONFIG_PENDING_MAX);
	CHECK(telemetry_config_state_publish(&bounded, candidates[4]).outcome ==
	      telemetry_config_state_outcome::capacity_full);
	CHECK(telemetry_config_state_stats_copy(&bounded).pending_count ==
	      TELEMETRY_CONFIG_PENDING_MAX);
	CHECK(telemetry_config_state_snapshot_copy(&bounded).config_id == 0U);
	bounded_sink.accept = true;
	CHECK(telemetry_config_state_publish(&bounded, candidates[3]).outcome ==
	      telemetry_config_state_outcome::accepted);
	CHECK(bounded_sink.records[0].payload.configuration.config.config_id == golden.config_id);
	for (std::size_t index = 0U; index < 3U; ++index)
	{
		CHECK(telemetry_config_state_publish(&bounded, candidates[3]).outcome ==
		      telemetry_config_state_outcome::accepted);
		CHECK(bounded_sink.records[index + 1U].payload.configuration.config.config_id ==
		      candidates[index + 1U].config_id);
	}
	CHECK(telemetry_config_state_snapshot_copy(&bounded).config_id == 0U);

	telemetry_config_state reload_state{};
	CHECK(telemetry_config_state_init(&reload_state, &configuration) ==
	      telemetry_config_state_outcome::accepted);
	CHECK(!telemetry_config_reload_requested(&reload_state));
	CHECK(telemetry_config_state_publish(&reload_state, golden).outcome ==
	      telemetry_config_state_outcome::accepted);
	telemetry_config_reload_observer(&reload_state);
	CHECK(telemetry_config_reload_requested(&reload_state));
	CHECK(telemetry_config_state_snapshot_copy(&reload_state).config_id == 0U);
	CHECK(telemetry_config_state_publish(&reload_state, golden).outcome ==
	      telemetry_config_state_outcome::unchanged);
	CHECK(telemetry_config_state_snapshot_copy(&reload_state).config_id == 0U);
	CHECK(telemetry_config_state_publish(&reload_state, changed).outcome ==
	      telemetry_config_state_outcome::accepted);
	CHECK(telemetry_config_state_snapshot_copy(&reload_state).config_id == changed.config_id);
	telemetry_config_reload_request_clear(&reload_state);
	CHECK(!telemetry_config_reload_requested(&reload_state));
}

void test_disabled_and_public(const telemetry_config_capture_input &base_input,
			      telemetry_config_property_snapshot &properties)
{
	telemetry_config_capture_input disabled_input = base_input;
	disabled_input.revision = 50U;
	disabled_input.enabled = 0U;
	const telemetry_config_snapshot disabled = build_snapshot(disabled_input, &properties);
	finite_sink no_sink{ false, {}, 0U };
	key_space no_keys{ { 0x90909090ULL, 0xA0A0A0A0ULL }, 1U, 0U };
	telemetry_config_state_config configuration = state_config(&no_sink, &no_keys);
	configuration.sink = {};
	configuration.key_allocator = {};
	telemetry_config_state state{};
	CHECK(telemetry_config_state_init(&state, &configuration) ==
	      telemetry_config_state_outcome::accepted);
	telemetry_config_state_result result = telemetry_config_state_publish(&state, disabled);
	CHECK(result.outcome == telemetry_config_state_outcome::disabled);
	CHECK(telemetry_config_state_snapshot_copy(&state).enabled == 0U);
	CHECK(telemetry_config_state_status(&state) == telemetry_config_validation::valid_disabled);
	CHECK(no_sink.count == 0U);
	CHECK(telemetry_config_state_publish(&state, disabled).outcome ==
	      telemetry_config_state_outcome::disabled);
	CHECK(no_sink.count == 0U);

	telemetry_config_capture_input flatfile_input = disabled_input;
	flatfile_input.revision = 51U;
	flatfile_input.backend = telemetry_storage_backend::flatfile_disabled;
	const telemetry_config_snapshot flatfile = build_snapshot(flatfile_input, &properties);
	result = telemetry_config_state_publish(&state, flatfile);
	CHECK(result.outcome == telemetry_config_state_outcome::flatfile_disabled);
	CHECK(telemetry_config_state_status(&state) ==
	      telemetry_config_validation::valid_flatfile_disabled);
	CHECK(telemetry_config_state_publish(&state, flatfile).outcome ==
	      telemetry_config_state_outcome::flatfile_disabled);

	finite_sink global_sink{ true, {}, 0U };
	key_space global_keys{ { 0xB0B0B0B0ULL, 0xC0C0C0C0ULL }, 1U, 0U };
	const telemetry_config_state_config global_configuration =
		state_config(&global_sink, &global_keys);
	CHECK(telemetry_config_global_init(&global_configuration) ==
	      telemetry_config_state_outcome::accepted);
	const telemetry_config_snapshot enabled = build_snapshot(base_input, &properties);
	const telemetry_capture_result enabled_result = telemetry_config_publish(enabled);
	CHECK(enabled_result.outcome == telemetry_runtime_outcome::accepted);
	CHECK(enabled_result.records_emitted == 1U);
	CHECK(global_sink.count == 1U);
	CHECK(telemetry_config_snapshot_copy().config_id == enabled.config_id);
	CHECK(telemetry_config_status() == telemetry_config_validation::valid_sql);
	const telemetry_capture_result public_result = telemetry_config_publish(disabled);
	CHECK(public_result.outcome == telemetry_runtime_outcome::disabled);
	CHECK(telemetry_config_snapshot_copy().enabled == 0U);
	CHECK(telemetry_config_status() == telemetry_config_validation::valid_disabled);
	telemetry_config_global_reset();
	CHECK(telemetry_config_snapshot_copy().config_id == 0U);
	CHECK(telemetry_config_status() == telemetry_config_validation::missing_identity);
}

} // namespace

int main()
{
	property_values values{ 0.0f, 10.0f, 1.0f, 0.2f, 0.15f, 0U, 0U };
	reviewed_property_catalog catalog{};
	telemetry_config_property_snapshot properties{};
	telemetry_config_snapshot golden{};
	test_registry_and_capture(values, golden, properties, catalog);
	const telemetry_config_property_capture live = make_capture(values, &catalog);
	const telemetry_config_capture_input base_input =
		make_input(1U, 123456789LL, 0x15161718U, live);
	test_canonical_identity(golden, base_input, properties);

	property_values changed_values{ 0.0f, 10.0f, 1.25f, 0.2f, 0.15f, 0U, 0U };
	property_values changed_probe_values = changed_values;
	const telemetry_config_property_capture changed_probe_capture =
		make_capture(changed_probe_values);
	telemetry_config_property_snapshot changed_probe{};
	CHECK(telemetry_config_property_snapshot_capture(&changed_probe_capture, &changed_probe) ==
	      telemetry_config_build_outcome::built);
	catalog_add(catalog, changed_probe);
	const telemetry_config_property_capture changed_live =
		make_capture(changed_values, &catalog);
	const telemetry_config_snapshot changed =
		build_snapshot(make_input(2U, 123456790LL, 0x15161718U, changed_live), &properties);
	CHECK(changed.config_id != golden.config_id);
	const telemetry_config_snapshot same_effective =
		build_snapshot(make_input(99U, 987654321LL, 0x15161718U, live), &properties);
	test_state_impl(golden, changed, same_effective);
	test_disabled_and_public(base_input, properties);
	std::fprintf(stdout, "telemetry config focused checks passed\n");
	return 0;
}
