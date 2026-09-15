#include "telemetry/telemetry_runtime.h"
#include "telemetry/telemetry_activity.h"
#include "core/structs.h"
#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_config_reload.h"
#include "telemetry/telemetry_repository.h"
#include "telemetry/telemetry_session.h"
#include "telemetry/telemetry_transport.h"
#include "telemetry/telemetry_transport_private.h"
#include "core/defines.h"

#include <openssl/rand.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <thread>
#include <unistd.h>

extern P_room world;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern int top_of_world;
/* The standalone runtime harnesses do not link properties.c.  Production has
 * the strong game implementation; an absent weak symbol makes bootstrap fail
 * closed instead of manufacturing an effective value. */
extern float get_property(const char *, double) __attribute__((weak));

namespace
{

constexpr std::uint16_t MAX_STATE_SLOTS =
	static_cast<std::uint16_t>(TELEMETRY_SESSION_STATE_MAX_SLOTS);
constexpr std::uint32_t QUEUE_CAPACITY =
	static_cast<std::uint32_t>(TELEMETRY_QUEUE_CAPACITY_PROPOSAL);
constexpr std::uint32_t CONTROL_RESERVE =
	static_cast<std::uint32_t>(TELEMETRY_CONTROL_RESERVE_PROPOSAL);
constexpr std::uint16_t MAX_BATCH_RECORDS =
	static_cast<std::uint16_t>(TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL);
constexpr std::uint32_t MAX_BATCH_BYTES =
	static_cast<std::uint32_t>(TELEMETRY_BATCH_MAX_BYTES_PROPOSAL);
constexpr telemetry_duration_usec FLUSH_OLDEST_USEC = TELEMETRY_FLUSH_OLDEST_USEC_PROPOSAL;
constexpr telemetry_duration_usec RETIRE_AFTER_USEC = TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL;
constexpr telemetry_duration_usec WORKER_WAIT_USEC = 50'000U;
constexpr std::uint16_t GAME_GROUP_MAX_NODES = 256U;
constexpr std::size_t REVIEWED_PROPERTY_CATALOG_MAX = 64U;
constexpr std::size_t REVIEWED_PROPERTY_DIGEST_TEXT_BYTES =
	TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES * 2U;

struct reviewed_property_catalog_entry
{
	std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES];
	std::uint32_t property_version;
	std::uint32_t stable_namespace;
	std::uint32_t stable_version;
};

struct reviewed_property_catalog
{
	reviewed_property_catalog_entry entries[REVIEWED_PROPERTY_CATALOG_MAX];
	std::uint16_t count;
	std::uint8_t loaded;
	std::uint8_t reserved;
};

reviewed_property_catalog REVIEWED_CATALOG{};

bool ascii_space(char value) noexcept
{
	return value == ' ' || value == '	' || value == '\r' || value == '\n' ||
	       value == '\f' || value == '\v';
}

int hex_digit(char value) noexcept
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

bool parse_property_digest(const char *text,
			   std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES]) noexcept
{
	if (text == nullptr || digest == nullptr ||
	    std::strlen(text) != REVIEWED_PROPERTY_DIGEST_TEXT_BYTES)
		return false;
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES; ++index)
	{
		const int high = hex_digit(text[index * 2U]);
		const int low = hex_digit(text[index * 2U + 1U]);
		if (high < 0 || low < 0)
			return false;
		digest[index] = static_cast<std::uint8_t>((high << 4) | low);
	}
	return true;
}

bool load_reviewed_property_catalog(const char *path) noexcept
{
	REVIEWED_CATALOG = {};
	if (path == nullptr || *path == '\0')
		return false;
	FILE *file = std::fopen(path, "r");
	if (file == nullptr)
		return false;

	bool valid = true;
	char line[256]{};
	while (valid && std::fgets(line, sizeof(line), file) != nullptr)
	{
		const char *first = line;
		while (*first != '\0' && ascii_space(*first))
			++first;
		if (*first == '\0' || *first == '#')
			continue;

		char digest_text[REVIEWED_PROPERTY_DIGEST_TEXT_BYTES + 1U]{};
		unsigned long long property_version = 0U;
		unsigned long long stable_namespace = 0U;
		unsigned long long stable_version = 0U;
		char extra = '\0';
		std::uint8_t parsed_digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES]{};
		const int fields = std::sscanf(line, " %64s %llu %llu %llu %c", digest_text,
					       &property_version, &stable_namespace,
					       &stable_version, &extra);
		if (fields != 4 || !parse_property_digest(digest_text, parsed_digest) ||
		    property_version == 0U ||
		    property_version > std::numeric_limits<std::uint32_t>::max() ||
		    stable_namespace == 0U ||
		    stable_namespace > std::numeric_limits<std::uint32_t>::max() ||
		    stable_version == 0U ||
		    stable_version > std::numeric_limits<std::uint32_t>::max() ||
		    REVIEWED_CATALOG.count >= REVIEWED_PROPERTY_CATALOG_MAX)
		{
			valid = false;
			break;
		}
		reviewed_property_catalog_entry candidate{};
		std::memcpy(candidate.digest, parsed_digest, sizeof(candidate.digest));
		candidate.property_version = static_cast<std::uint32_t>(property_version);
		candidate.stable_namespace = static_cast<std::uint32_t>(stable_namespace);
		candidate.stable_version = static_cast<std::uint32_t>(stable_version);
		for (std::size_t index = 0U; index < REVIEWED_CATALOG.count; ++index)
		{
			const auto &prior = REVIEWED_CATALOG.entries[index];
			if (std::memcmp(prior.digest, candidate.digest, sizeof(candidate.digest)) ==
				    0 ||
			    prior.property_version == candidate.property_version)
			{
				/* One full digest has one mapping, and a public version may
				 * never alias a different full digest. */
				valid = false;
				break;
			}
		}
		if (valid)
			REVIEWED_CATALOG.entries[REVIEWED_CATALOG.count++] = candidate;
	}
	if (std::ferror(file) != 0)
		valid = false;
	std::fclose(file);
	if (!valid || REVIEWED_CATALOG.count == 0U)
	{
		REVIEWED_CATALOG = {};
		return false;
	}
	REVIEWED_CATALOG.loaded = 1U;
	return true;
}

bool reviewed_property_catalog_resolve(
	void *context, const telemetry_config_property_snapshot *snapshot,
	const std::uint8_t *digest,
	telemetry_config_property_catalog_resolution *resolution) noexcept
{
	if (context == nullptr || snapshot == nullptr || digest == nullptr || resolution == nullptr)
		return false;
	const auto *catalog = static_cast<const reviewed_property_catalog *>(context);
	if (catalog->loaded == 0U)
		return false;
	for (std::size_t index = 0U; index < catalog->count; ++index)
	{
		const auto &entry = catalog->entries[index];
		if (std::memcmp(entry.digest, digest, sizeof(entry.digest)) != 0)
			continue;
		*resolution = {};
		resolution->property_version = entry.property_version;
		resolution->stable_namespace = entry.stable_namespace;
		resolution->stable_version = entry.stable_version;
		std::memcpy(resolution->digest, entry.digest, sizeof(resolution->digest));
		return true;
	}
	return false;
}

struct runtime_state
{
	bool initialized = false;
	bool enabled = false;
	bool transport_started = false;
	bool shutdown_pending = false;
	bool has_last_health = false;
	telemetry_health_snapshot last_health{};
	telemetry_producer_id producer{};
	telemetry_config_snapshot config{};
	telemetry_config_property_capture property_capture{};
	telemetry_config_capture_input capture_input{};
	bool property_capture_enabled = false;
	bool config_available = false;
	telemetry_config_revision reload_revision = 0U;
	telemetry_season_id session_scope_season_id = TELEMETRY_UNKNOWN_ID;
	telemetry_environment_id session_scope_environment_id = TELEMETRY_UNKNOWN_ID;
	telemetry_record_sequence next_record_sequence = 1U;
	telemetry_connection_sequence next_connection_sequence = 1U;
	telemetry_session_sequence next_session_sequence = 1U;
	telemetry_session_state session{};
	telemetry_activity_state activity{};
	std::thread worker{};
	std::atomic<bool> worker_stop{ false };
	std::atomic<bool> worker_done{ false };
	std::atomic<bool> worker_final_flush{ false };
	std::atomic<telemetry_monotonic_usec> worker_deadline{ 0U };
	std::atomic<std::uint64_t> wake_generation{ 0U };
	std::atomic<std::uint64_t> flush_requested{ 0U };
	std::atomic<std::uint64_t> flush_completed{ 0U };
	std::atomic<telemetry_monotonic_usec> flush_deadline{ 0U };
	std::atomic<bool> flush_durable{ false };
	std::condition_variable wake_condition{};
	std::mutex wake_mutex{};
};

runtime_state R;
// Process-lifetime ledger, deliberately NOT reset by final_reap. Reject rather
// than evict an old identity or restart its attempted record sequence at one.
telemetry_producer_id used_producers[256]{};
std::size_t used_producer_count = 0U;

bool reserve_runtime_producer(telemetry_producer_id producer) noexcept
{
	for (std::size_t index = 0U; index < used_producer_count; ++index)
		if (used_producers[index].boot_id == producer.boot_id &&
		    used_producers[index].process_id == producer.process_id)
			return false;
	if (used_producer_count == sizeof(used_producers) / sizeof(used_producers[0]))
		return false;
	used_producers[used_producer_count++] = producer;
	return true;
}

struct worker_done_guard
{
	~worker_done_guard() noexcept
	{
		R.worker_done.store(true, std::memory_order_release);
		R.wake_condition.notify_all();
	}
};

bool session_equal(const telemetry_session_ref &left, const telemetry_session_ref &right) noexcept
{
	return left.id.producer.boot_id == right.id.producer.boot_id &&
	       left.id.producer.process_id == right.id.producer.process_id &&
	       left.id.session_seq == right.id.session_seq && left.subject_id == right.subject_id &&
	       left.pid == right.pid && left.season_id == right.season_id &&
	       left.environment_id == right.environment_id;
}

bool handoff_is_zero(const telemetry_session_handoff &handoff) noexcept
{
	return telemetry_session_ref_is_zero(handoff.session) &&
	       telemetry_producer_id_is_zero(handoff.previous_producer) &&
	       handoff.last_checkpoint_revision == 0U && handoff.cumulative.connected_usec == 0U &&
	       handoff.cumulative.active_usec == 0U && handoff.cumulative.idle_usec == 0U &&
	       handoff.cumulative.unknown_usec == 0U && handoff.cumulative.resident_usec == 0U &&
	       handoff.cumulative.linkdead_usec == 0U &&
	       handoff.quality_flags == TELEMETRY_QUALITY_NONE;
}

char ascii_lower(char value) noexcept
{
	return value >= 'A' && value <= 'Z' ? static_cast<char>(value + ('a' - 'A')) : value;
}

bool environment_text_is(const char *value, const char *expected) noexcept
{
	if (value == nullptr || expected == nullptr)
		return false;
	while (*value != '\0' && *expected != '\0')
	{
		if (ascii_lower(*value) != ascii_lower(*expected))
			return false;
		++value;
		++expected;
	}
	return *value == '\0' && *expected == '\0';
}

template <typename T, typename Minimum, typename Maximum>
bool environment_unsigned(const char *name, T default_value, Minimum minimum, Maximum maximum,
			  T *value) noexcept
{
	if (value == nullptr)
		return false;
	const char *text = std::getenv(name);
	if (text == nullptr || *text == '\0')
	{
		*value = default_value;
		return true;
	}
	if (*text == '-')
		return false;
	errno = 0;
	char *end = nullptr;
	const unsigned long long parsed = std::strtoull(text, &end, 10);
	if (errno == ERANGE || end == text || end == nullptr || *end != '\0' ||
	    parsed < static_cast<unsigned long long>(minimum) ||
	    parsed > static_cast<unsigned long long>(maximum))
		return false;
	*value = static_cast<T>(parsed);
	return true;
}

bool environment_backend(telemetry_storage_backend default_backend,
			 telemetry_storage_backend *backend) noexcept
{
	if (backend == nullptr)
		return false;
	const char *text = std::getenv("TELEMETRY_BACKEND");
	if (text == nullptr || *text == '\0')
	{
		*backend = default_backend;
		return true;
	}
	if (environment_text_is(text, "sql"))
		*backend = telemetry_storage_backend::sql;
	else if (environment_text_is(text, "flatfile_disabled") ||
		 environment_text_is(text, "disabled") || environment_text_is(text, "off"))
		*backend = telemetry_storage_backend::flatfile_disabled;
	else
		return false;
	return true;
}

bool environment_enabled(bool *enabled) noexcept
{
	if (enabled == nullptr)
		return false;
	const char *text = std::getenv("TELEMETRY_ENABLED");
	if (text == nullptr || *text == '\0' || environment_text_is(text, "false") ||
	    environment_text_is(text, "off") || environment_text_is(text, "0") ||
	    environment_text_is(text, "no"))
	{
		*enabled = false;
		return true;
	}
	if (environment_text_is(text, "true") || environment_text_is(text, "on") ||
	    environment_text_is(text, "1") || environment_text_is(text, "yes"))
	{
		*enabled = true;
		return true;
	}
	return false;
}

bool production_property_reader(void *, const char *key, float *value) noexcept
{
	if (key == nullptr || value == nullptr || get_property == nullptr)
		return false;
	std::size_t count = 0U;
	const telemetry_config_property_definition *registry =
		telemetry_config_property_registry(&count);
	if (registry == nullptr)
		return false;
	for (std::size_t index = 0U; index < count; ++index)
	{
		const auto &definition = registry[index];
		if (definition.role != telemetry_config_property_role::effective ||
		    definition.source_key == nullptr ||
		    std::strcmp(definition.source_key, key) != 0)
			continue;
		float declared_default = 0.0F;
		if (definition.kind == telemetry_config_property_kind::float32)
			std::memcpy(&declared_default, &definition.default_value,
				    sizeof(declared_default));
		else
			declared_default = static_cast<float>(definition.default_value);
		/* This calls the game's normal double overload, so absent properties
		 * receive the same declared fallback as gameplay. */
		*value = get_property(definition.source_key, static_cast<double>(declared_default));
		return true;
	}
	return false;
}

telemetry_config_capture_input
capture_input_for(const telemetry_config_snapshot &config,
		  const telemetry_config_property_capture &properties) noexcept
{
	telemetry_config_capture_input input{};
	input.schema_version = config.schema_version;
	input.revision = config.revision;
	input.build_version = config.build_version;
	input.content_version = config.content_version;
	input.classifier_version = config.classifier_version;
	input.policy_version = config.policy_version;
	input.season_id = config.season_id;
	input.environment_id = config.environment_id;
	input.effective_utc_usec = config.effective_utc_usec;
	input.interval_usec = config.interval_usec;
	input.checkpoint_interval_usec = config.checkpoint_interval_usec;
	input.active_window_usec = config.active_window_usec;
	input.context_segments_per_minute = config.context_segments_per_minute;
	input.pulse_slot_count = config.pulse_slot_count;
	input.backend = config.backend;
	input.enabled = config.enabled;
	input.properties = properties;
	return input;
}

bool build_effective_snapshot(const telemetry_config_snapshot &base,
			      const telemetry_config_property_capture &properties,
			      telemetry_config_revision revision,
			      telemetry_utc_usec effective_utc_usec,
			      telemetry_config_property_snapshot *property_snapshot,
			      telemetry_config_snapshot *snapshot) noexcept
{
	if (property_snapshot == nullptr || snapshot == nullptr)
		return false;
	telemetry_config_capture_input input = capture_input_for(base, properties);
	input.revision = revision;
	input.effective_utc_usec = effective_utc_usec;
	return telemetry_config_snapshot_build(&input, property_snapshot, snapshot) ==
	       telemetry_config_build_outcome::built;
}

void refresh_config_identity(telemetry_config_snapshot &config) noexcept
{
	std::uint8_t fingerprint[TELEMETRY_CONFIG_FINGERPRINT_BYTES]{};
	if (!telemetry_config_compute_fingerprint(config, fingerprint, sizeof(fingerprint)))
		return;
	std::memcpy(config.fingerprint, fingerprint, sizeof(config.fingerprint));
	config.config_id = telemetry_config_id_from_fingerprint(config.fingerprint,
								sizeof(config.fingerprint));
}

telemetry_config_snapshot disabled_config(telemetry_config_snapshot config) noexcept
{
	config.enabled = 0U;
	if (config.property_version == 0U)
		config.property_version = 1U;
	refresh_config_identity(config);
	return config;
}

bool saturating_add_u16(std::uint16_t &value, std::uint16_t amount) noexcept
{
	if (amount > std::numeric_limits<std::uint16_t>::max() - value)
	{
		value = std::numeric_limits<std::uint16_t>::max();
		return false;
	}
	value = static_cast<std::uint16_t>(value + amount);
	return true;
}

bool saturating_add_u32(std::uint32_t &value, std::uint32_t amount) noexcept
{
	if (amount > std::numeric_limits<std::uint32_t>::max() - value)
	{
		value = std::numeric_limits<std::uint32_t>::max();
		return false;
	}
	value += amount;
	return true;
}

void wake_worker() noexcept
{
	R.wake_generation.fetch_add(1U, std::memory_order_release);
	R.wake_condition.notify_one();
}

bool production_clock_now(void *, telemetry_monotonic_usec *monotonic_usec,
			  telemetry_utc_usec *utc_usec) noexcept
{
	if (monotonic_usec == nullptr || utc_usec == nullptr)
		return false;
	try
	{
		const auto monotonic = std::chrono::duration_cast<std::chrono::microseconds>(
					       std::chrono::steady_clock::now().time_since_epoch())
					       .count();
		const auto utc = std::chrono::duration_cast<std::chrono::microseconds>(
					 std::chrono::system_clock::now().time_since_epoch())
					 .count();
		if (monotonic < 0 || utc < std::numeric_limits<telemetry_utc_usec>::min() ||
		    utc > std::numeric_limits<telemetry_utc_usec>::max())
			return false;
		*monotonic_usec = static_cast<telemetry_monotonic_usec>(monotonic);
		*utc_usec = static_cast<telemetry_utc_usec>(utc);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool production_monotonic_now(telemetry_monotonic_usec *monotonic_usec) noexcept
{
	telemetry_utc_usec ignored_utc = TELEMETRY_UTC_UNKNOWN;
	return production_clock_now(nullptr, monotonic_usec, &ignored_utc);
}

telemetry_runtime_outcome wait_for_copyover_flush(
	std::uint64_t generation, telemetry_monotonic_usec deadline,
	bool (*monotonic_now)(telemetry_monotonic_usec *) noexcept = production_monotonic_now)
{
	telemetry_monotonic_usec now = 0U;
	for (;;)
	{
		// Snapshot acknowledgement BEFORE sampling time: a descheduled caller
		// must not accept a late publication using an earlier clock reading.
		const auto completed = R.flush_completed.load(std::memory_order_acquire);
		const bool durable = R.flush_durable.load(std::memory_order_relaxed);
		if (!monotonic_now(&now) || now >= deadline)
			return telemetry_runtime_outcome::queue_full;
		if (completed == generation)
			return durable ? telemetry_runtime_outcome::accepted :
					 telemetry_runtime_outcome::queue_full;
		std::this_thread::sleep_for(std::chrono::microseconds(1'000U));
	}
}

bool worker_finished_by(telemetry_monotonic_usec deadline) noexcept
{
	if (!R.worker.joinable() || R.worker_done.load(std::memory_order_acquire))
		return true;
	for (;;)
	{
		telemetry_monotonic_usec now = 0U;
		if (!production_monotonic_now(&now) || now >= deadline)
			return false;
		std::unique_lock<std::mutex> lock(R.wake_mutex);
		if (R.worker_done.load(std::memory_order_acquire))
			return true;
		R.wake_condition.wait_for(
			lock, std::chrono::microseconds(deadline - now),
			[] { return R.worker_done.load(std::memory_order_acquire); });
		if (R.worker_done.load(std::memory_order_acquire))
			return true;
	}
}

bool allocate_record_key(void *, telemetry_record_kind kind, telemetry_record_key *key) noexcept
{
	(void)kind;
	if (key == nullptr || !telemetry_producer_id_is_valid(R.producer) ||
	    R.next_record_sequence == 0U)
		return false;
	*key = { R.producer, R.next_record_sequence };
	if (R.next_record_sequence == std::numeric_limits<telemetry_record_sequence>::max())
		R.next_record_sequence = 0U;
	else
		++R.next_record_sequence;
	return true;
}

bool allocate_connection_id(void *, telemetry_connection_id *id) noexcept
{
	if (id == nullptr || !telemetry_producer_id_is_valid(R.producer) ||
	    R.next_connection_sequence == 0U)
		return false;
	*id = { R.producer, R.next_connection_sequence };
	if (R.next_connection_sequence == std::numeric_limits<telemetry_connection_sequence>::max())
		R.next_connection_sequence = 0U;
	else
		++R.next_connection_sequence;
	return true;
}

bool enqueue_record(void *, const telemetry_record *record) noexcept
{
	if (record == nullptr)
		return false;
	const telemetry_enqueue_result result = telemetry_transport_enqueue(*record);
	const bool accepted = result.admission == telemetry_queue_admission::accepted_detail ||
			      result.admission ==
				      telemetry_queue_admission::accepted_control_reserve;
	if (accepted)
		wake_worker();
	return accepted;
}

telemetry_transport_config transport_config_for(const telemetry_config_snapshot &config) noexcept
{
	telemetry_transport_config transport{};
	transport.backend = config.backend;
	transport.schema_version = TELEMETRY_SCHEMA_VERSION;
	transport.queue_capacity = QUEUE_CAPACITY;
	transport.control_reserve = CONTROL_RESERVE;
	transport.max_batch_records = MAX_BATCH_RECORDS;
	transport.max_batch_bytes = MAX_BATCH_BYTES;
	transport.flush_oldest_after_usec = FLUSH_OLDEST_USEC;
	transport.fresh_producer = R.producer;
	return transport;
}

bool activity_outcome_success(telemetry_activity_outcome outcome) noexcept
{
	return outcome == telemetry_activity_outcome::accepted ||
	       outcome == telemetry_activity_outcome::accepted_degraded ||
	       outcome == telemetry_activity_outcome::idempotent ||
	       outcome == telemetry_activity_outcome::retired;
}

bool session_outcome_success(telemetry_session_state_outcome outcome) noexcept
{
	return outcome == telemetry_session_state_outcome::accepted ||
	       outcome == telemetry_session_state_outcome::accepted_unclosed_recovery ||
	       outcome == telemetry_session_state_outcome::idempotent ||
	       outcome == telemetry_session_state_outcome::retired;
}

bool map_runtime_evidence_kind(telemetry_runtime_evidence_kind runtime_kind,
			       telemetry_activity_evidence_kind *activity_kind) noexcept;
telemetry_capture_result game_capture_invalid() noexcept;

telemetry_capture_result capture_from_activity(const telemetry_activity_result &source) noexcept
{
	telemetry_capture_result result{};
	result.outcome = telemetry_runtime_outcome::invalid;
	result.admission = telemetry_queue_admission::rejected_invalid;
	result.records_emitted = source.records_accepted;
	result.records_dropped = source.records_dropped;
	result.quality_flags = source.quality_flags;
	if (source.records_accepted != 0U)
		result.admission = telemetry_queue_admission::accepted_detail;
	if (source.outcome == telemetry_activity_outcome::sink_rejected ||
	    source.outcome == telemetry_activity_outcome::allocator_exhausted ||
	    source.outcome == telemetry_activity_outcome::capacity_full ||
	    source.outcome == telemetry_activity_outcome::config_capacity_full)
	{
		result.outcome = telemetry_runtime_outcome::queue_full;
		if (source.records_accepted == 0U)
			result.admission = telemetry_queue_admission::rejected_detail_full;
	}
	else if (source.outcome == telemetry_activity_outcome::not_found ||
		 source.outcome == telemetry_activity_outcome::invalid ||
		 source.outcome == telemetry_activity_outcome::clock_unavailable)
		result.outcome = telemetry_runtime_outcome::invalid;
	else if (activity_outcome_success(source.outcome))
		result.outcome = telemetry_runtime_outcome::accepted;
	else
		result.outcome = telemetry_runtime_outcome::invalid;
	return result;
}

telemetry_capture_result capture_from_session(const telemetry_session_state_result &source) noexcept
{
	telemetry_capture_result result{};
	result.outcome = telemetry_runtime_outcome::invalid;
	result.admission = telemetry_queue_admission::rejected_invalid;
	result.records_emitted = source.records_accepted;
	result.records_dropped = source.records_dropped;
	result.quality_flags = source.quality_flags;
	if (source.records_accepted != 0U)
		result.admission = telemetry_queue_admission::accepted_control_reserve;
	if (source.outcome == telemetry_session_state_outcome::sink_rejected ||
	    source.outcome == telemetry_session_state_outcome::allocator_exhausted ||
	    source.outcome == telemetry_session_state_outcome::capacity_full)
	{
		result.outcome = telemetry_runtime_outcome::queue_full;
		if (source.records_accepted == 0U)
			result.admission = telemetry_queue_admission::rejected_control_full;
	}
	else if (source.outcome == telemetry_session_state_outcome::not_found ||
		 source.outcome == telemetry_session_state_outcome::invalid ||
		 source.outcome == telemetry_session_state_outcome::clock_unavailable)
		result.outcome = telemetry_runtime_outcome::invalid;
	else if (session_outcome_success(source.outcome))
		result.outcome = telemetry_runtime_outcome::accepted;
	else
		result.outcome = telemetry_runtime_outcome::invalid;
	return result;
}

telemetry_capture_result disabled_capture(telemetry_runtime_outcome outcome) noexcept
{
	telemetry_capture_result result{};
	result.outcome = outcome;
	result.admission = outcome == telemetry_runtime_outcome::stopping ?
				   telemetry_queue_admission::rejected_stopping :
				   telemetry_queue_admission::rejected_disabled;
	result.quality_flags = TELEMETRY_QUALITY_DISABLED;
	return result;
}

void merge_capture(telemetry_capture_result &target,
		   const telemetry_capture_result &source) noexcept
{
	const bool target_empty = target.records_emitted == 0U && target.records_dropped == 0U &&
				  target.first_record.record_seq == 0U &&
				  target.last_record.record_seq == 0U;
	if (target_empty)
		target = source;
	else
	{
		(void)saturating_add_u16(target.records_emitted, source.records_emitted);
		(void)saturating_add_u16(target.records_dropped, source.records_dropped);
		target.quality_flags |= source.quality_flags;
		if (target.first_record.record_seq == 0U)
			target.first_record = source.first_record;
		if (source.last_record.record_seq != 0U)
			target.last_record = source.last_record;
		if (source.outcome == telemetry_runtime_outcome::queue_full)
			target.outcome = telemetry_runtime_outcome::queue_full;
		else if (target.outcome == telemetry_runtime_outcome::accepted &&
			 source.outcome != telemetry_runtime_outcome::accepted)
			target.outcome = source.outcome;
		if (target.admission == telemetry_queue_admission::rejected_invalid &&
		    source.admission != telemetry_queue_admission::rejected_invalid)
			target.admission = source.admission;
	}
}

void fill_capture_keys(telemetry_capture_result &capture, const telemetry_record_key &first,
		       const telemetry_record_key &last) noexcept
{
	capture.first_record = first;
	capture.last_record = last;
}

bool publish_activity_config(const telemetry_config_snapshot &config) noexcept
{
	if (!config.enabled || config.backend != telemetry_storage_backend::sql)
		return false;
	if (telemetry_activity_state_config_is_admitted(&R.activity, config.config_id))
		return true;
	const telemetry_activity_result result =
		telemetry_activity_state_publish_config(&R.activity, config);
	return result.outcome == telemetry_activity_outcome::accepted ||
	       result.outcome == telemetry_activity_outcome::idempotent;
}

bool adopt_visible_config(const telemetry_config_snapshot &visible) noexcept
{
	if (!telemetry_config_is_valid(visible) || visible.enabled == 0U)
		return false;
	R.config = visible;
	R.config_available = true;
	if (!publish_activity_config(visible))
		return false;
	return telemetry_activity_state_config_is_admitted(&R.activity, visible.config_id);
}

bool retry_pending_config(telemetry_config_state *state) noexcept
{
	if (state == nullptr)
		return false;
	for (std::size_t attempt = 0U;
	     attempt < TELEMETRY_CONFIG_PENDING_MAX && state->pending_count != 0U; ++attempt)
	{
		const telemetry_config_snapshot pending = state->pending[0].snapshot;
		const telemetry_capture_result retried = telemetry_config_publish(pending);
		const telemetry_config_snapshot visible = telemetry_config_snapshot_copy();
		if (adopt_visible_config(visible))
		{
			telemetry_config_reload_request_clear(state);
			R.reload_revision = 0U;
			return true;
		}
		if (retried.outcome != telemetry_runtime_outcome::accepted)
			return false;
	}
	return false;
}

bool capture_reloaded_config(telemetry_config_state *state) noexcept
{
	if (state == nullptr || state->initialized == 0U ||
	    !telemetry_config_reload_requested(state))
		return false;
	if (retry_pending_config(state))
		return true;
	if (state->pending_count != 0U || !R.property_capture_enabled)
		return false;

	if (R.reload_revision == 0U || R.reload_revision <= state->highest_revision)
	{
		if (state->highest_revision ==
		    std::numeric_limits<telemetry_config_revision>::max())
			return false;
		R.reload_revision = state->highest_revision + 1U;
	}
	telemetry_monotonic_usec ignored_monotonic = 0U;
	telemetry_utc_usec effective_utc = R.capture_input.effective_utc_usec;
	if (production_clock_now(nullptr, &ignored_monotonic, &effective_utc))
		R.capture_input.effective_utc_usec = effective_utc;
	telemetry_config_capture_input input = R.capture_input;
	input.revision = R.reload_revision;
	input.effective_utc_usec = effective_utc;
	telemetry_config_property_snapshot properties{};
	telemetry_config_snapshot candidate{};
	if (telemetry_config_snapshot_build(&input, &properties, &candidate) !=
	    telemetry_config_build_outcome::built)
		return false;

	const telemetry_capture_result published = telemetry_config_publish(candidate);
	const telemetry_config_snapshot visible = telemetry_config_snapshot_copy();
	if (adopt_visible_config(visible))
	{
		telemetry_config_reload_request_clear(state);
		R.reload_revision = 0U;
		return true;
	}
	/* A rejected sink leaves the exact candidate in the bounded pending FIFO.
	 * Capacity/identity/revision rejection establishes a newer floor for the
	 * next attempt; it must not be retried with the same rejected revision. */
	if (published.outcome != telemetry_runtime_outcome::queue_full ||
	    state->pending_count == 0U)
		R.reload_revision = 0U;
	return false;
}

bool ensure_current_config() noexcept
{
	if (!R.enabled || R.shutdown_pending)
		return false;
	telemetry_config_state *state = telemetry_config_global_state();
	if (state == nullptr || state->initialized == 0U)
		return false;
	if (telemetry_config_reload_requested(state))
		return capture_reloaded_config(state);

	telemetry_config_snapshot visible = telemetry_config_snapshot_copy();
	if (!telemetry_config_is_valid(visible) || visible.enabled == 0U)
		return retry_pending_config(state);
	return adopt_visible_config(visible);
}

void reload_observer(void *context) noexcept
{
	telemetry_config_state *state = static_cast<telemetry_config_state *>(context);
	telemetry_config_reload_observer(state);
	/* R.config is an admitted effective snapshot, never a fallback for a
	 * post-reload capture.  Keep session scope separately so detach/exit/
	 * handoff can still identify the logical session while attribution is
	 * honestly unavailable. */
	R.config = {};
	R.config_available = false;
	R.reload_revision = 0U;
	(void)capture_reloaded_config(state);
}

bool all_admitted_records_durable() noexcept
{
	const auto health = telemetry_transport_health_copy();
	const auto limit = std::numeric_limits<std::uint64_t>::max();
	// A drained queue can contain permanently rejected records. Never mistake
	// those, a disabled writer, or saturating accounting for durability.
	if (health.backend != telemetry_storage_backend::sql || health.invalid_records != 0U ||
	    health.conflict_records != 0U ||
	    health.admitted_detail >= limit - health.admitted_control ||
	    health.applied_records >= limit - health.duplicate_records)
		return false;
	const auto applied = health.applied_records + health.duplicate_records;
	if (applied >= limit - health.stale_checkpoint_records)
		return false;
	return health.admitted_detail + health.admitted_control ==
	       applied + health.stale_checkpoint_records;
}

void worker_loop() noexcept
{
	worker_done_guard done;
	for (;;)
	{
		if (R.worker_stop.load(std::memory_order_acquire))
			break;
		telemetry_monotonic_usec now = 0U;
		if (!production_monotonic_now(&now))
			now = 0U;
		const auto flush = R.flush_requested.load(std::memory_order_acquire);
		if (flush != R.flush_completed.load(std::memory_order_acquire))
		{
			const auto deadline = R.flush_deadline.load(std::memory_order_acquire);
			telemetry_transport_drain_result drained{};
			drained.outcome = telemetry_transport_outcome::deadline_reached;
			if (production_monotonic_now(&now) && now < deadline)
				drained = telemetry_transport_drain_until(deadline);
			const bool durable = drained.pending == 0U &&
					     drained.outcome ==
						     telemetry_transport_outcome::drained &&
					     all_admitted_records_durable();
			// A synchronous repository callback can outlive the deadline even
			// when transport reports a fully drained queue. Check after both
			// the callback and accounting, against this generation's deadline.
			const bool expired = !production_monotonic_now(&now) || now >= deadline;
			if (drained.pending == 0U || expired)
			{
				R.flush_durable.store(durable && !expired,
						      std::memory_order_relaxed);
				R.flush_completed.store(flush, std::memory_order_release);
			}
		}
		else
			(void)telemetry_transport_pulse(now);
		const std::uint64_t observed_generation =
			R.wake_generation.load(std::memory_order_acquire);
		std::unique_lock<std::mutex> lock(R.wake_mutex);
		R.wake_condition.wait_for(
			lock, std::chrono::microseconds(WORKER_WAIT_USEC),
			[&]
			{
				return R.worker_stop.load(std::memory_order_acquire) ||
				       R.wake_generation.load(std::memory_order_acquire) !=
					       observed_generation;
			});
	}

	if (!R.worker_final_flush.load(std::memory_order_acquire))
		return;
	const telemetry_monotonic_usec deadline = R.worker_deadline.load(std::memory_order_acquire);
	if (deadline == 0U)
		return;
	for (;;)
	{
		telemetry_monotonic_usec now = 0U;
		if (!production_monotonic_now(&now))
			return;
		if (now > deadline)
			return;
		const telemetry_transport_drain_result drained =
			telemetry_transport_drain_until(deadline);
		if (drained.pending == 0U ||
		    drained.outcome == telemetry_transport_outcome::drained ||
		    drained.outcome == telemetry_transport_outcome::flatfile_disabled)
			return;
		std::this_thread::sleep_for(std::chrono::microseconds(1'000U));
	}
}

telemetry_health_snapshot fallback_health() noexcept
{
	telemetry_health_snapshot health{};
	health.schema_version = TELEMETRY_SCHEMA_VERSION;
	health.state = telemetry_health_state::disabled;
	health.backend = R.config.backend;
	health.disabled_reason = R.config.backend == telemetry_storage_backend::flatfile_disabled ?
					 telemetry_disabled_reason::flatfile_authority :
					 telemetry_disabled_reason::not_initialized;
	return health;
}

bool valid_runtime_options(const telemetry_runtime_options &options) noexcept
{
	return telemetry_producer_id_is_valid(options.producer) &&
	       telemetry_config_is_valid(options.config);
}

telemetry_capture_result capture_not_ready() noexcept
{
	if (!R.initialized)
		return disabled_capture(telemetry_runtime_outcome::not_initialized);
	if (!R.enabled)
		return disabled_capture(
			R.config.backend == telemetry_storage_backend::flatfile_disabled ?
				telemetry_runtime_outcome::flatfile_disabled :
				telemetry_runtime_outcome::disabled);
	return disabled_capture(telemetry_runtime_outcome::stopping);
}

telemetry_activity_context context_to_activity(telemetry_activity_context context) noexcept
{
	return context;
}

std::uint32_t context_flags_for(telemetry_activity_context context) noexcept
{
	switch (context)
	{
	case telemetry_activity_context::combat:
		return TELEMETRY_ACTIVITY_CONTEXT_COMBAT;
	case telemetry_activity_context::travel:
		return TELEMETRY_ACTIVITY_CONTEXT_TRAVEL;
	case telemetry_activity_context::crafting:
		return TELEMETRY_ACTIVITY_CONTEXT_CRAFTING;
	case telemetry_activity_context::social:
		return TELEMETRY_ACTIVITY_CONTEXT_SOCIAL;
	case telemetry_activity_context::administration:
		return TELEMETRY_ACTIVITY_CONTEXT_ADMINISTRATION;
	case telemetry_activity_context::other:
		return TELEMETRY_ACTIVITY_CONTEXT_OTHER;
	case telemetry_activity_context::none:
		return TELEMETRY_ACTIVITY_CONTEXT_NONE;
	case telemetry_activity_context::unknown:
	case telemetry_activity_context::overflow_unknown:
		return TELEMETRY_ACTIVITY_CONTEXT_UNKNOWN;
	}
	return TELEMETRY_ACTIVITY_CONTEXT_UNKNOWN;
}

void seed_activity_resume(const telemetry_session_resume &resume) noexcept
{
	for (std::size_t index = 0U; index < R.activity.max_slots; ++index)
	{
		telemetry_activity_slot &slot = R.activity.slots[index];
		if (slot.lifecycle == telemetry_activity_slot_lifecycle::empty ||
		    !session_equal(slot.session, resume.entry.session))
			continue;
		slot.cumulative = resume.handoff.cumulative;
		slot.quality_flags |= resume.handoff.quality_flags | resume.entry.quality_flags;
		slot.entry_snapshot = resume.entry;
		slot.connection = resume.entry.connection;
		slot.connected = 1U;
		slot.interval_start_monotonic_usec = resume.entry.at_monotonic_usec;
		slot.interval_start_utc_usec = resume.entry.at_utc_usec;
		slot.last_observed_monotonic_usec = resume.entry.at_monotonic_usec;
		slot.last_observed_utc_usec = resume.entry.at_utc_usec;
		slot.detached_since_monotonic_usec = 0U;
		return;
	}
}

telemetry_capture_result
capture_with_delta(const telemetry_activity_result &activity_result) noexcept
{
	telemetry_capture_result result = capture_from_activity(activity_result);
	if (activity_result.has_delta != 0U)
	{
		const telemetry_capture_result delivered =
			telemetry_runtime_update_counters(activity_result.delta);
		merge_capture(result, delivered);
	}
	return result;
}

void add_pulse_capture(telemetry_pulse_result &pulse, const telemetry_capture_result &capture,
		       bool lifecycle) noexcept
{
	(void)saturating_add_u32(pulse.records_dropped, capture.records_dropped);
	if (lifecycle)
		(void)saturating_add_u32(pulse.lifecycle_records, capture.records_emitted);
	pulse.quality_flags |= capture.quality_flags;
}

} // namespace

telemetry_runtime_options telemetry_runtime_default_options(void)
{
	telemetry_runtime_options options{};
	const auto now_utc = std::chrono::duration_cast<std::chrono::microseconds>(
				     std::chrono::system_clock::now().time_since_epoch())
				     .count();
	// Independent nonzero incarnation tokens, not time/PID-derived identifiers.
	// Entropy failure leaves an invalid producer and fails telemetry bootstrap
	// closed; no weak fallback or per-event randomness is permitted.
	for (unsigned attempt = 0U; attempt < 4U; ++attempt)
	{
		if (RAND_bytes(reinterpret_cast<unsigned char *>(&options.producer),
			       static_cast<int>(sizeof(options.producer))) != 1)
		{
			options.producer = {};
			break;
		}
		if (telemetry_producer_id_is_valid(options.producer))
			break;
		options.producer = {};
	}
	options.config.schema_version = TELEMETRY_SCHEMA_VERSION;
	options.config.revision = 1U;
	options.config.build_version = 1U;
	options.config.content_version = 1U;
	options.config.property_version = 1U;
	options.config.classifier_version = 1U;
	options.config.policy_version = 1U;
	options.config.season_id = 1U;
	options.config.environment_id = 1U;
	options.config.effective_utc_usec = now_utc;
	options.config.backend = telemetry_storage_backend::sql;
	options.config.enabled = 0U;
#ifdef __NO_MYSQL__
	options.config.backend = telemetry_storage_backend::flatfile_disabled;
#endif
	(void)telemetry_config_compute_fingerprint(options.config, options.config.fingerprint,
						   sizeof(options.config.fingerprint));
	options.config.config_id = telemetry_config_id_from_fingerprint(
		options.config.fingerprint, sizeof(options.config.fingerprint));
	return options;
}

telemetry_runtime_options telemetry_runtime_options_from_environment(void)
{
	telemetry_runtime_options options = telemetry_runtime_default_options();
	bool enabled = false;
	if (!environment_enabled(&enabled) || !enabled)
		return options;

	telemetry_storage_backend backend = options.config.backend;
	if (!environment_backend(backend, &backend))
		return options;

	telemetry_config_revision revision = 1U;
	std::uint32_t build_version = 1U;
	std::uint32_t content_version = 1U;
	std::uint32_t classifier_version = 1U;
	std::uint32_t policy_version = 1U;
	telemetry_season_id season_id = 1U;
	telemetry_environment_id environment_id = 1U;
	telemetry_duration_usec interval_usec = TELEMETRY_INTERVAL_USEC_PROPOSAL;
	telemetry_duration_usec checkpoint_interval_usec = 300'000'000U;
	telemetry_duration_usec active_window_usec = TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL;
	std::uint32_t context_segments_per_minute = TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL;
	std::uint16_t pulse_slot_count = 1U;
	if (!environment_unsigned("TELEMETRY_CONFIG_REVISION", revision, 1U,
				  std::numeric_limits<telemetry_config_revision>::max(),
				  &revision) ||
	    !environment_unsigned("TELEMETRY_BUILD_VERSION", build_version, 1U,
				  std::numeric_limits<std::uint32_t>::max(), &build_version) ||
	    !environment_unsigned("TELEMETRY_CONTENT_VERSION", content_version, 1U,
				  std::numeric_limits<std::uint32_t>::max(), &content_version) ||
	    !environment_unsigned("TELEMETRY_CLASSIFIER_VERSION", classifier_version, 1U,
				  std::numeric_limits<std::uint32_t>::max(), &classifier_version) ||
	    !environment_unsigned("TELEMETRY_POLICY_VERSION", policy_version, 1U,
				  std::numeric_limits<std::uint32_t>::max(), &policy_version) ||
	    !environment_unsigned("TELEMETRY_SEASON_ID", season_id, 1U,
				  std::numeric_limits<telemetry_season_id>::max(), &season_id) ||
	    !environment_unsigned("TELEMETRY_ENVIRONMENT_ID", environment_id, 1U,
				  std::numeric_limits<telemetry_environment_id>::max(),
				  &environment_id) ||
	    !environment_unsigned("TELEMETRY_INTERVAL_USEC", interval_usec, 1U,
				  TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL, &interval_usec) ||
	    !environment_unsigned("TELEMETRY_CHECKPOINT_INTERVAL_USEC", checkpoint_interval_usec,
				  1U, TELEMETRY_CHECKPOINT_INTERVAL_USEC_MAX_PROPOSAL,
				  &checkpoint_interval_usec) ||
	    !environment_unsigned("TELEMETRY_ACTIVE_WINDOW_USEC", active_window_usec, 1U,
				  TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL, &active_window_usec) ||
	    !environment_unsigned("TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE",
				  context_segments_per_minute, 1U,
				  TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL,
				  &context_segments_per_minute) ||
	    !environment_unsigned("TELEMETRY_PULSE_SLOT_COUNT", pulse_slot_count, 1U,
				  TELEMETRY_CONFIG_PULSE_SLOT_COUNT_MAX_PROPOSAL,
				  &pulse_slot_count))
		return options;

	options.config.revision = revision;
	options.config.build_version = build_version;
	options.config.content_version = content_version;
	options.config.classifier_version = classifier_version;
	options.config.policy_version = policy_version;
	options.config.season_id = season_id;
	options.config.environment_id = environment_id;
	options.config.interval_usec = interval_usec;
	options.config.checkpoint_interval_usec = checkpoint_interval_usec;
	options.config.active_window_usec = active_window_usec;
	options.config.context_segments_per_minute = context_segments_per_minute;
	options.config.pulse_slot_count = pulse_slot_count;
	options.config.backend = backend;
	options.config.enabled = backend == telemetry_storage_backend::sql ? 1U : 0U;
#ifdef __NO_MYSQL__
	options.config.backend = telemetry_storage_backend::flatfile_disabled;
	options.config.enabled = 0U;
#endif
	if (options.config.enabled == 0U)
	{
		refresh_config_identity(options.config);
		return options;
	}

	/* Enabled SQL telemetry is admitted only with a reviewed, preloaded full
	 * digest catalog.  A missing/invalid file is a telemetry-only fail-closed
	 * result; the caller still boots ordinary gameplay. */
	if (!load_reviewed_property_catalog(std::getenv("TELEMETRY_PROPERTY_CATALOG_FILE")))
		return telemetry_runtime_default_options();
	options.property_capture.reader = { production_property_reader, nullptr };
	options.property_capture.mode = telemetry_config_property_capture_mode::require_reader;
	options.property_capture.catalog = { reviewed_property_catalog_resolve, &REVIEWED_CATALOG };
	options.property_capture_enabled = 1U;
	telemetry_config_property_snapshot properties{};
	telemetry_config_snapshot effective{};
	if (!build_effective_snapshot(options.config, options.property_capture,
				      options.config.revision, options.config.effective_utc_usec,
				      &properties, &effective))
		return telemetry_runtime_default_options();
	options.config = effective;
	return options;
}

telemetry_runtime_outcome telemetry_runtime_init(telemetry_runtime_options options)
{
	if (R.initialized || !valid_runtime_options(options))
		return telemetry_runtime_outcome::invalid;
	if (options.config.enabled != 0U &&
	    options.config.backend == telemetry_storage_backend::sql &&
	    options.property_capture_enabled != 0U)
	{
		telemetry_config_property_snapshot properties{};
		telemetry_config_snapshot effective{};
		if (!build_effective_snapshot(
			    options.config, options.property_capture, options.config.revision,
			    options.config.effective_utc_usec, &properties, &effective))
		{
			/* Telemetry context is optional.  A missing reader/catalog disables
			 * only this runtime and never gates normal gameplay startup. */
			options.config = disabled_config(options.config);
			options.property_capture_enabled = 0U;
		}
		else
			options.config = effective;
	}
	if (options.config.enabled != 0U &&
	    options.config.backend == telemetry_storage_backend::sql &&
	    !reserve_runtime_producer(options.producer))
		return telemetry_runtime_outcome::invalid;
	R.producer = options.producer;
	R.config = options.config;
	R.property_capture = options.property_capture;
	R.property_capture_enabled = options.property_capture_enabled != 0U;
	R.capture_input = capture_input_for(options.config, options.property_capture);
	R.config_available = false;
	R.reload_revision = 0U;
	R.session_scope_season_id = options.config.season_id;
	R.session_scope_environment_id = options.config.environment_id;
	R.shutdown_pending = false;
	R.next_record_sequence = 1U;
	R.next_connection_sequence = 1U;
	R.next_session_sequence = 1U;
	R.worker_stop.store(false, std::memory_order_release);
	R.worker_done.store(false, std::memory_order_release);
	R.worker_final_flush.store(false, std::memory_order_release);
	R.worker_deadline.store(0U, std::memory_order_release);
	R.wake_generation.store(0U, std::memory_order_release);
	R.flush_requested.store(0U, std::memory_order_release);
	R.flush_completed.store(0U, std::memory_order_release);
	R.flush_deadline.store(0U, std::memory_order_release);
	R.flush_durable.store(false, std::memory_order_release);

	telemetry_monotonic_usec monotonic_anchor = 0U;
	telemetry_utc_usec utc_anchor = TELEMETRY_UTC_UNKNOWN;
	(void)production_clock_now(nullptr, &monotonic_anchor, &utc_anchor);
	const telemetry_session_state_config session_config = {
		MAX_STATE_SLOTS,
		0U,
		RETIRE_AFTER_USEC,
		R.producer,
		{ production_clock_now, nullptr },
		{ enqueue_record, nullptr },
		{ allocate_record_key, nullptr },
	};
	if (telemetry_session_state_init(&R.session, &session_config) !=
	    telemetry_session_state_outcome::accepted)
		return telemetry_runtime_outcome::invalid;

	telemetry_activity_state_config activity_config{};
	activity_config.max_slots = MAX_STATE_SLOTS;
	activity_config.pulse_slot_count = options.config.pulse_slot_count;
	activity_config.context_segments_per_minute = options.config.context_segments_per_minute;
	activity_config.interval_usec = options.config.interval_usec;
	activity_config.active_window_usec = options.config.active_window_usec;
	activity_config.detached_retire_after_usec = RETIRE_AFTER_USEC;
	activity_config.monotonic_anchor_usec = monotonic_anchor;
	activity_config.producer = R.producer;
	activity_config.clock = { production_clock_now, nullptr };
	activity_config.sink = { enqueue_record, nullptr };
	activity_config.key_allocator = { allocate_record_key, nullptr };
	if (telemetry_activity_state_init(&R.activity, &activity_config) !=
	    telemetry_activity_outcome::accepted)
	{
		telemetry_session_state_reset(&R.session);
		return telemetry_runtime_outcome::invalid;
	}

	const telemetry_config_state_config config_state_config = {
		{ enqueue_record, nullptr },
		{ allocate_record_key, nullptr },
	};
	if (telemetry_config_global_init(&config_state_config) !=
	    telemetry_config_state_outcome::accepted)
	{
		telemetry_activity_state_reset(&R.activity);
		telemetry_session_state_reset(&R.session);
		return telemetry_runtime_outcome::invalid;
	}
	if (telemetry_config_state_seed_identity(telemetry_config_global_state(), R.config) !=
	    telemetry_config_state_outcome::accepted)
	{
		telemetry_config_global_reset();
		telemetry_activity_state_reset(&R.activity);
		telemetry_session_state_reset(&R.session);
		return telemetry_runtime_outcome::invalid;
	}

	R.enabled = options.config.enabled != 0U &&
		    options.config.backend == telemetry_storage_backend::sql;
	if (R.enabled)
	{
		const telemetry_transport_outcome transport =
			telemetry_transport_init(transport_config_for(options.config));
		if (transport != telemetry_transport_outcome::started)
		{
			telemetry_config_global_reset();
			telemetry_activity_state_reset(&R.activity);
			telemetry_session_state_reset(&R.session);
			return transport == telemetry_transport_outcome::flatfile_disabled ?
				       telemetry_runtime_outcome::flatfile_disabled :
				       telemetry_runtime_outcome::invalid;
		}
		R.transport_started = true;
	}

	R.initialized = true;
	(void)telemetry_config_reload_register(reload_observer, telemetry_config_global_state());
	const telemetry_capture_result published = telemetry_config_publish(R.config);
	if (R.enabled && published.outcome == telemetry_runtime_outcome::accepted)
		(void)adopt_visible_config(telemetry_config_snapshot_copy());

	if (R.enabled)
	{
		try
		{
			R.worker = std::thread(worker_loop);
		}
		catch (...)
		{
			R.initialized = false;
			(void)telemetry_config_reload_unregister(reload_observer,
								 telemetry_config_global_state());
			telemetry_transport_request_stop();
			telemetry_transport_repository_shutdown_for_owner();
			telemetry_transport_shutdown();
			telemetry_config_global_reset();
			telemetry_activity_state_reset(&R.activity);
			telemetry_session_state_reset(&R.session);
			R.enabled = false;
			R.transport_started = false;
			return telemetry_runtime_outcome::invalid;
		}
	}
	if (!R.enabled)
	{
		R.last_health = fallback_health();
		R.has_last_health = true;
		return R.config.backend == telemetry_storage_backend::flatfile_disabled ?
			       telemetry_runtime_outcome::flatfile_disabled :
			       telemetry_runtime_outcome::disabled;
	}
	return published.outcome == telemetry_runtime_outcome::invalid ?
		       telemetry_runtime_outcome::queue_full :
		       telemetry_runtime_outcome::accepted;
}

telemetry_capture_result telemetry_runtime_session_enter(telemetry_session_enter enter)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_session_enter_is_valid(enter) || !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::queue_full);
	if (!telemetry_activity_state_config_is_admitted(&R.activity, enter.config_id))
		return disabled_capture(telemetry_runtime_outcome::queue_full);
	const telemetry_activity_result activity =
		telemetry_activity_state_enter(&R.activity, enter);
	telemetry_capture_result result = capture_from_activity(activity);
	const telemetry_session_state_result session =
		telemetry_session_state_enter(&R.session, enter);
	const telemetry_capture_result session_capture = capture_from_session(session);
	merge_capture(result, session_capture);
	fill_capture_keys(result, result.first_record,
			  session_capture.last_record.record_seq != 0U ?
				  session_capture.last_record :
				  result.last_record);
	return result;
}

telemetry_runtime_outcome telemetry_runtime_flush_for_copyover(telemetry_monotonic_usec deadline)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready().outcome;
	telemetry_monotonic_usec now = 0U;
	if (!production_monotonic_now(&now) || deadline <= now)
		return telemetry_runtime_outcome::queue_full;
	// The game-loop owner has captured all handoffs and admits no more work
	// during this bounded wait. No SQL, worker join, or permanent stop here.
	constexpr telemetry_duration_usec maximum_wait = 250'000U;
	if (deadline - now > maximum_wait)
		deadline = now + maximum_wait;
	const auto previous = R.flush_requested.load(std::memory_order_relaxed);
	if (previous == std::numeric_limits<std::uint64_t>::max())
		return telemetry_runtime_outcome::queue_full;
	const auto generation = previous + 1U;
	R.flush_deadline.store(deadline, std::memory_order_relaxed);
	R.flush_requested.store(generation, std::memory_order_release);
	wake_worker();
	return wait_for_copyover_flush(generation, deadline);
}

telemetry_handoff_result telemetry_runtime_session_handoff_copy(telemetry_session_ref session)
{
	telemetry_handoff_result result{};
	result.outcome = !R.initialized	    ? telemetry_runtime_outcome::not_initialized :
			 !R.enabled	    ? telemetry_runtime_outcome::disabled :
			 R.shutdown_pending ? telemetry_runtime_outcome::stopping :
					      telemetry_runtime_outcome::invalid;
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return result;
	if (!telemetry_session_ref_is_valid(session) || !ensure_current_config())
	{
		result.outcome = telemetry_runtime_outcome::queue_full;
		return result;
	}
	telemetry_monotonic_usec monotonic_usec = 0U;
	telemetry_utc_usec utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (!production_clock_now(nullptr, &monotonic_usec, &utc_usec))
		return result;
	// Activity deltas and session counters must stop at the same observation.
	// Taking another clock cut would invalidate continued play after failed exec.
	const telemetry_activity_result flushed =
		telemetry_activity_state_flush_at(&R.activity, session, monotonic_usec, utc_usec);
	const telemetry_capture_result captured = capture_with_delta(flushed);
	if (captured.outcome != telemetry_runtime_outcome::accepted)
	{
		result.outcome = captured.outcome;
		return result;
	}
	const telemetry_session_state_result handoff_result =
		telemetry_session_state_handoff_copy_at(&R.session, session, &result.handoff,
							monotonic_usec, utc_usec);
	if (handoff_result.outcome == telemetry_session_state_outcome::accepted)
		result.outcome = telemetry_runtime_outcome::accepted;
	else if (handoff_result.outcome == telemetry_session_state_outcome::sink_rejected ||
		 handoff_result.outcome == telemetry_session_state_outcome::allocator_exhausted)
		result.outcome = telemetry_runtime_outcome::queue_full;
	return result;
}

telemetry_capture_result telemetry_runtime_session_resume(telemetry_session_resume resume)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	const bool absent_handoff = handoff_is_zero(resume.handoff);
	if ((!absent_handoff && !telemetry_session_resume_is_valid(resume)) ||
	    (absent_handoff && !telemetry_session_enter_is_valid(resume.entry)) ||
	    !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::invalid);
	if (!telemetry_activity_state_config_is_admitted(&R.activity, resume.entry.config_id))
		return disabled_capture(telemetry_runtime_outcome::queue_full);
	const telemetry_activity_result activity =
		telemetry_activity_state_enter(&R.activity, resume.entry);
	telemetry_capture_result result = capture_from_activity(activity);
	if (activity.outcome == telemetry_activity_outcome::invalid ||
	    activity.outcome == telemetry_activity_outcome::capacity_full)
		return result;
	seed_activity_resume(resume);
	const telemetry_session_state_result session =
		telemetry_session_state_resume(&R.session, resume);
	const telemetry_capture_result session_capture = capture_from_session(session);
	merge_capture(result, session_capture);
	return result;
}

telemetry_capture_result
telemetry_runtime_connection_transition(telemetry_connection_transition transition)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_connection_transition_is_valid(transition) ||
	    transition.kind == telemetry_connection_transition_kind::copyover_resumed ||
	    !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::invalid);
	const telemetry_activity_result activity =
		telemetry_activity_state_transition(&R.activity, transition);
	telemetry_capture_result result = capture_with_delta(activity);
	// Allocation failure can occur after the classifier commits the boundary.
	// Finish the matching session mutation, retaining the record-loss outcome.
	if (!activity_outcome_success(activity.outcome) &&
	    activity.outcome != telemetry_activity_outcome::sink_rejected &&
	    activity.outcome != telemetry_activity_outcome::allocator_exhausted)
		return result;
	const telemetry_session_state_result session =
		telemetry_session_state_transition(&R.session, transition);
	merge_capture(result, capture_from_session(session));
	return result;
}

telemetry_capture_result telemetry_runtime_update_counters(telemetry_counter_update update)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_counter_update_is_valid(update))
		return disabled_capture(telemetry_runtime_outcome::invalid);
	if (!ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::queue_full);
	// The runtime owns both state machines: a second counter producer must not
	// move the session frontier beyond the classifier or replace its categories.
	telemetry_activity_state_view activity{};
	telemetry_session_state_view session{};
	if (!telemetry_activity_state_copy_view(&R.activity, update.session, &activity) ||
	    !telemetry_session_state_copy_view(&R.session, update.session, &session) ||
	    update.at_monotonic_usec != activity.interval_start_monotonic_usec)
		return disabled_capture(telemetry_runtime_outcome::invalid);
	const auto matches = [](telemetry_duration_usec classified,
				telemetry_duration_usec accounted,
				telemetry_duration_usec delta) noexcept
	{ return classified >= accounted && classified - accounted == delta; };
	const bool caught_up =
		activity.cumulative.connected_usec == session.cumulative.connected_usec &&
		activity.cumulative.active_usec == session.cumulative.active_usec &&
		activity.cumulative.idle_usec == session.cumulative.idle_usec &&
		activity.cumulative.unknown_usec == session.cumulative.unknown_usec &&
		activity.cumulative.resident_usec == session.cumulative.resident_usec &&
		activity.cumulative.linkdead_usec == session.cumulative.linkdead_usec;
	const bool exact_delta =
		matches(activity.cumulative.connected_usec, session.cumulative.connected_usec,
			update.connected_delta_usec) &&
		matches(activity.cumulative.active_usec, session.cumulative.active_usec,
			update.active_delta_usec) &&
		matches(activity.cumulative.idle_usec, session.cumulative.idle_usec,
			update.idle_delta_usec) &&
		matches(activity.cumulative.unknown_usec, session.cumulative.unknown_usec,
			update.unknown_delta_usec) &&
		matches(activity.cumulative.resident_usec, session.cumulative.resident_usec,
			update.resident_delta_usec) &&
		matches(activity.cumulative.linkdead_usec, session.cumulative.linkdead_usec,
			update.linkdead_delta_usec);
	// When already caught up, let #264 validate an exact idempotent replay.
	if (!caught_up && !exact_delta)
		return disabled_capture(telemetry_runtime_outcome::invalid);
	return capture_from_session(telemetry_session_state_update_counters(&R.session, update));
}

telemetry_capture_result telemetry_runtime_record_evidence(telemetry_runtime_evidence evidence)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_runtime_evidence_is_valid(evidence) || !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::invalid);
	telemetry_activity_evidence activity_evidence{};
	activity_evidence.session = evidence.session;
	activity_evidence.connection = evidence.connection;
	activity_evidence.at_monotonic_usec = evidence.at_monotonic_usec;
	activity_evidence.at_utc_usec = evidence.at_utc_usec;
	if (!map_runtime_evidence_kind(evidence.kind, &activity_evidence.kind))
		return game_capture_invalid();
	activity_evidence.quality_flags = evidence.quality_flags;
	return capture_with_delta(
		telemetry_activity_state_record_evidence(&R.activity, activity_evidence));
}

telemetry_capture_result telemetry_runtime_update_context(telemetry_context_update update)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_context_update_is_valid(update) || !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::invalid);
	telemetry_activity_context_snapshot snapshot{};
	snapshot.session = update.session;
	snapshot.connection = update.connection;
	snapshot.at_monotonic_usec = update.at_monotonic_usec;
	snapshot.at_utc_usec = update.at_utc_usec;
	snapshot.dimensions = update.dimensions;
	snapshot.context_flags = context_flags_for(context_to_activity(update.context));
	snapshot.config_id = update.config_id;
	snapshot.classifier_version = update.classifier_version;
	snapshot.policy_version = update.policy_version;
	snapshot.quality_flags = update.quality_flags;
	const telemetry_activity_result activity =
		telemetry_activity_state_update_context(&R.activity, snapshot);
	return capture_with_delta(activity);
}

telemetry_pulse_result telemetry_runtime_pulse(telemetry_pulse_request pulse)
{
	telemetry_pulse_result result{};
	result.outcome =
		!R.initialized ? telemetry_runtime_outcome::not_initialized :
		!R.enabled     ? (R.config.backend == telemetry_storage_backend::flatfile_disabled ?
					  telemetry_runtime_outcome::flatfile_disabled :
					  telemetry_runtime_outcome::disabled) :
		R.shutdown_pending ? telemetry_runtime_outcome::stopping :
				     telemetry_runtime_outcome::accepted;
	result.quality_flags = !R.enabled ? TELEMETRY_QUALITY_DISABLED : TELEMETRY_QUALITY_NONE;
	if (!R.initialized || !R.enabled || R.shutdown_pending ||
	    !telemetry_pulse_request_is_valid(pulse))
		return result;
	if (!ensure_current_config())
	{
		result.outcome = telemetry_runtime_outcome::queue_full;
		result.quality_flags |= TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
		return result;
	}

	telemetry_counter_update deltas[TELEMETRY_ACTIVITY_STATE_MAX_SLOTS]{};
	telemetry_activity_pulse_request request{};
	request.now_monotonic_usec = pulse.now_monotonic_usec;
	request.occurrence_utc_usec = pulse.occurrence_utc_usec;
	request.slot = pulse.slot;
	request.deltas = deltas;
	request.delta_capacity = static_cast<std::uint16_t>(TELEMETRY_ACTIVITY_STATE_MAX_SLOTS);
	const telemetry_activity_pulse_result activity =
		telemetry_activity_state_pulse(&R.activity, request);
	result.sessions_considered = activity.sessions_considered;
	result.intervals_sealed = activity.intervals_sealed;
	result.records_dropped = activity.records_dropped;
	result.quality_flags |= activity.quality_flags;
	for (std::uint16_t index = 0U; index < activity.deltas_written; ++index)
	{
		const telemetry_capture_result delivered =
			telemetry_runtime_update_counters(deltas[index]);
		(void)saturating_add_u32(result.records_dropped, delivered.records_dropped);
		result.quality_flags |= delivered.quality_flags;
		if (delivered.outcome != telemetry_runtime_outcome::accepted)
		{
			result.outcome = delivered.outcome;
			result.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
		}
	}
	for (std::size_t index = 0U; index < R.session.max_slots; ++index)
	{
		telemetry_session_slot &slot = R.session.slots[index];
		if (slot.lifecycle != telemetry_session_slot_lifecycle::resident ||
		    R.config.checkpoint_interval_usec == 0U)
			continue;
		const telemetry_monotonic_usec anchor =
			slot.last_checkpoint_monotonic_usec != 0U ?
				slot.last_checkpoint_monotonic_usec :
				slot.entry_monotonic_usec;
		// Only checkpoint time already delivered by the classifier. Other
		// staggered buckets have not observed this pulse; accounting them to
		// pulse.now would make their next activity delta noncontiguous.
		const auto observed = slot.last_observed_monotonic_usec;
		if (observed < anchor || observed - anchor < R.config.checkpoint_interval_usec)
			continue;
		const auto observed_utc = slot.last_utc_monotonic_usec == observed ?
						  slot.last_observed_utc_usec :
						  TELEMETRY_UTC_UNKNOWN;
		const telemetry_session_state_result checkpoint =
			telemetry_session_state_checkpoint_at(&R.session, slot.session, observed,
							      observed_utc);
		if (session_outcome_success(checkpoint.outcome))
			++result.checkpoints_sealed;
		add_pulse_capture(result, capture_from_session(checkpoint), true);
	}
	const telemetry_activity_result activity_retired =
		telemetry_activity_state_retire_expired(&R.activity);
	add_pulse_capture(result, capture_from_activity(activity_retired), false);
	const telemetry_session_state_result session_retired =
		telemetry_session_state_retire_expired(&R.session);
	add_pulse_capture(result, capture_from_session(session_retired), true);
	if (activity.outcome == telemetry_activity_outcome::invalid ||
	    activity_retired.outcome == telemetry_activity_outcome::invalid ||
	    session_retired.outcome == telemetry_session_state_outcome::invalid)
		result.outcome = telemetry_runtime_outcome::invalid;
	return result;
}

telemetry_capture_result telemetry_runtime_session_exit(telemetry_session_exit exit)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return capture_not_ready();
	if (!telemetry_session_exit_is_valid(exit) || !ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::invalid);
	const telemetry_activity_result activity = telemetry_activity_state_exit(&R.activity, exit);
	telemetry_capture_result result = capture_with_delta(activity);
	// Allocation failure can occur after the classifier commits the boundary.
	// Finish the matching session mutation, retaining the record-loss outcome.
	if (!activity_outcome_success(activity.outcome) &&
	    activity.outcome != telemetry_activity_outcome::sink_rejected &&
	    activity.outcome != telemetry_activity_outcome::allocator_exhausted)
		return result;
	const telemetry_session_state_result session =
		telemetry_session_state_exit(&R.session, exit);
	merge_capture(result, capture_from_session(session));
	return result;
}

telemetry_runtime_outcome telemetry_runtime_shutdown(telemetry_shutdown_request request)
{
	if (!telemetry_shutdown_request_is_valid(request))
		return telemetry_runtime_outcome::invalid;
	if (!R.initialized)
		return telemetry_runtime_outcome::not_initialized;
	if (!R.enabled)
	{
		R.last_health = fallback_health();
		R.has_last_health = true;
		(void)telemetry_config_reload_unregister(reload_observer,
							 telemetry_config_global_state());
		telemetry_config_global_reset();
		telemetry_activity_state_reset(&R.activity);
		telemetry_session_state_reset(&R.session);
		R.initialized = false;
		R.shutdown_pending = false;
		return telemetry_runtime_outcome::accepted;
	}

	if (!R.shutdown_pending)
	{
		R.shutdown_pending = true;
		R.worker_final_flush.store(request.final_flush != 0U, std::memory_order_release);
		R.worker_deadline.store(request.deadline_monotonic_usec, std::memory_order_release);
		R.worker_stop.store(true, std::memory_order_release);
		/* This only closes admission and publishes stop state. Repository
		 * callbacks remain owned by the worker/final-reap sequence. */
		(void)telemetry_transport_request_stop();
		wake_worker();
	}
	return worker_finished_by(R.worker_deadline.load(std::memory_order_acquire)) ?
		       telemetry_runtime_outcome::accepted :
		       telemetry_runtime_outcome::stopping;
}

telemetry_runtime_outcome telemetry_runtime_final_reap(void)
{
	if (!R.initialized)
		return telemetry_runtime_outcome::accepted;
	if (!R.enabled)
		return telemetry_runtime_outcome::invalid;
	if (!R.shutdown_pending)
		return telemetry_runtime_outcome::invalid;

	/* This is deliberately the possibly blocking lifecycle boundary. The
	 * worker owns the borrowed repository binding until this join completes. */
	if (R.worker.joinable())
		R.worker.join();
	telemetry_transport_repository_shutdown_for_owner();
	R.last_health = telemetry_transport_health_copy();
	telemetry_transport_shutdown();
	R.last_health = telemetry_transport_health_copy();
	R.has_last_health = true;
	(void)telemetry_config_reload_unregister(reload_observer, telemetry_config_global_state());
	telemetry_config_global_reset();
	telemetry_activity_state_reset(&R.activity);
	telemetry_session_state_reset(&R.session);
	R.initialized = false;
	R.enabled = false;
	R.transport_started = false;
	R.shutdown_pending = false;
	return telemetry_runtime_outcome::accepted;
}

telemetry_health_snapshot telemetry_runtime_health_copy(void)
{
	if (R.transport_started)
		return telemetry_transport_health_copy();
	if (R.has_last_health)
		return R.last_health;
	return fallback_health();
}

/* Small value-only seams used by gameplay glue. */
telemetry_producer_id telemetry_runtime_producer_copy(void)
{
	return R.producer;
}

bool telemetry_runtime_next_connection(telemetry_connection_id *connection) noexcept
{
	return R.initialized && R.enabled && !R.shutdown_pending &&
	       allocate_connection_id(nullptr, connection);
}

bool telemetry_runtime_next_session(telemetry_session_id *session) noexcept
{
	if (session == nullptr || !R.initialized || !R.enabled || R.shutdown_pending ||
	    !telemetry_producer_id_is_valid(R.producer) || R.next_session_sequence == 0U)
		return false;
	*session = { R.producer, R.next_session_sequence };
	if (R.next_session_sequence == std::numeric_limits<telemetry_session_sequence>::max())
		R.next_session_sequence = 0U;
	else
		++R.next_session_sequence;
	return true;
}

bool telemetry_runtime_now(telemetry_monotonic_usec *monotonic_usec,
			   telemetry_utc_usec *utc_usec) noexcept
{
	return R.initialized && production_clock_now(nullptr, monotonic_usec, utc_usec);
}

namespace
{

telemetry_capture_result game_capture_not_ready() noexcept
{
	return capture_not_ready();
}

telemetry_capture_result game_capture_invalid() noexcept
{
	telemetry_capture_result result{};
	result.outcome = telemetry_runtime_outcome::invalid;
	result.admission = telemetry_queue_admission::rejected_invalid;
	result.quality_flags = TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	return result;
}

bool game_session_ref(const struct char_data *character, telemetry_session_ref *session) noexcept
{
	if (character == nullptr || session == nullptr || character->only.pc == nullptr ||
	    character->only.pc->pid <= 0 || character->telemetry_session_sequence == 0U ||
	    character->telemetry_session_producer_boot_id == 0U ||
	    character->telemetry_session_producer_process_id == 0U)
		return false;
	*session = {};
	session->id.producer.boot_id = character->telemetry_session_producer_boot_id;
	session->id.producer.process_id = character->telemetry_session_producer_process_id;
	session->id.session_seq = character->telemetry_session_sequence;
	session->subject_id = static_cast<telemetry_subject_id>(character->only.pc->pid);
	session->pid = static_cast<telemetry_pid>(character->only.pc->pid);
	session->season_id = R.session_scope_season_id;
	session->environment_id = R.session_scope_environment_id;
	return telemetry_session_ref_is_valid(*session);
}

bool game_connection_id(const struct descriptor_data *descriptor,
			telemetry_connection_id *connection) noexcept
{
	if (descriptor == nullptr || connection == nullptr ||
	    descriptor->telemetry_connection_sequence == 0U ||
	    descriptor->telemetry_connection_producer_boot_id == 0U ||
	    descriptor->telemetry_connection_producer_process_id == 0U)
		return false;
	*connection = {};
	connection->producer.boot_id = descriptor->telemetry_connection_producer_boot_id;
	connection->producer.process_id = descriptor->telemetry_connection_producer_process_id;
	connection->connection_seq = descriptor->telemetry_connection_sequence;
	return telemetry_connection_id_is_valid(*connection);
}

bool game_zone_vnum(const struct char_data *character, std::int32_t *zone_vnum) noexcept
{
	if (zone_vnum == nullptr)
		return false;
	*zone_vnum = -1;
	if (character == nullptr || world == nullptr || zone_table == nullptr ||
	    character->in_room < 0 || character->in_room > top_of_world || top_of_zone_table < 0)
		return false;
	const unsigned int zone_index = world[character->in_room].zone;
	if (zone_index > static_cast<unsigned int>(top_of_zone_table))
		return false;
	const int mapped_zone = zone_table[zone_index].number;
	if (mapped_zone < -1)
		return false;
	*zone_vnum = static_cast<std::int32_t>(mapped_zone);
	return true;
}

bool game_group_size(const struct char_data *character, std::uint32_t *group_size) noexcept
{
	if (character == nullptr || group_size == nullptr)
		return false;
	if (character->group == nullptr)
	{
		/* A character outside a group is a stable one-player cohort. */
		*group_size = 1U;
		return true;
	}
	std::uint32_t count = 0U;
	bool contains_character = false;
	std::uint16_t nodes = 0U;
	for (const struct group_list *member = character->group; member != nullptr;
	     member = member->next)
	{
		if (nodes == GAME_GROUP_MAX_NODES)
		{
			*group_size = 0U;
			return false;
		}
		++nodes;
		if (member->ch == character)
			contains_character = true;
		if (member->ch == nullptr || (member->ch->specials.act & ACT_ISNPC) != 0U ||
		    member->ch->only.pc == nullptr || member->ch->only.pc->pid <= 0)
			continue;
		if (count != std::numeric_limits<std::uint32_t>::max())
			++count;
	}
	if (!contains_character || count == 0U)
	{
		*group_size = 0U;
		return false;
	}
	*group_size = count;
	return true;
}

void game_dimensions(const struct char_data *character, telemetry_dimensions *dimensions,
		     telemetry_quality_mask *quality_flags) noexcept
{
	*dimensions = {};
	*dimensions = {
		static_cast<std::uint16_t>(
			character->player.level == 0U ? 0U : character->player.level / 5U + 1U),
		static_cast<std::uint16_t>(character->player.m_class),
		static_cast<std::uint16_t>(character->player.race),
		static_cast<std::uint16_t>(character->player.racewar),
		-1,
		0U,
	};
	bool dimensions_complete = character->player.level != 0U &&
				   character->player.m_class != 0U &&
				   character->player.race != 0U && character->player.racewar != 0U;
	if (!game_zone_vnum(character, &dimensions->zone_vnum))
		dimensions_complete = false;
	if (!game_group_size(character, &dimensions->group_size))
		dimensions_complete = false;
	if (!dimensions_complete)
		*quality_flags |= TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
}

bool game_time(telemetry_monotonic_usec *monotonic_usec, telemetry_utc_usec *utc_usec) noexcept
{
	return telemetry_runtime_now(monotonic_usec, utc_usec);
}

bool game_allocate_session(struct char_data *character) noexcept
{
	if (character == nullptr || character->only.pc == nullptr || character->only.pc->pid <= 0)
		return false;
	telemetry_session_id session{};
	if (!telemetry_runtime_next_session(&session))
		return false;
	character->telemetry_session_sequence = session.session_seq;
	character->telemetry_session_producer_boot_id = session.producer.boot_id;
	character->telemetry_session_producer_process_id = session.producer.process_id;
	return true;
}

bool game_allocate_connection(struct descriptor_data *descriptor) noexcept
{
	if (descriptor == nullptr)
		return false;
	telemetry_connection_id connection{};
	if (!telemetry_runtime_next_connection(&connection))
		return false;
	descriptor->telemetry_connection_sequence = connection.connection_seq;
	descriptor->telemetry_connection_producer_boot_id = connection.producer.boot_id;
	descriptor->telemetry_connection_producer_process_id = connection.producer.process_id;
	return true;
}

void game_clear_connection(struct descriptor_data *descriptor) noexcept
{
	if (descriptor == nullptr)
		return;
	descriptor->telemetry_connection_sequence = 0U;
	descriptor->telemetry_connection_producer_boot_id = 0U;
	descriptor->telemetry_connection_producer_process_id = 0U;
}

void game_clear_session(struct char_data *character) noexcept
{
	if (character == nullptr)
		return;
	character->telemetry_session_sequence = 0U;
	character->telemetry_session_producer_boot_id = 0U;
	character->telemetry_session_producer_process_id = 0U;
}

bool game_enter_payload(const struct char_data *character, const struct descriptor_data *descriptor,
			telemetry_session_enter *enter) noexcept
{
	if (enter == nullptr || !game_session_ref(character, &enter->session) ||
	    !game_connection_id(descriptor, &enter->connection))
		return false;
	if (!game_time(&enter->at_monotonic_usec, &enter->at_utc_usec))
		return false;
	enter->dimensions = {};
	enter->quality_flags = TELEMETRY_QUALITY_NONE;
	game_dimensions(character, &enter->dimensions, &enter->quality_flags);
	enter->config_id = R.config.config_id;
	enter->classifier_version = R.config.classifier_version;
	enter->policy_version = R.config.policy_version;
	return telemetry_session_enter_is_valid(*enter);
}

bool game_transition_payload(const struct char_data *character,
			     const struct descriptor_data *descriptor,
			     telemetry_connection_transition_kind kind,
			     telemetry_connection_transition *transition) noexcept
{
	if (transition == nullptr || !game_session_ref(character, &transition->session) ||
	    !game_time(&transition->at_monotonic_usec, &transition->at_utc_usec))
		return false;
	transition->connection = {};
	if (kind == telemetry_connection_transition_kind::attached)
	{
		if (!game_connection_id(descriptor, &transition->connection))
			return false;
	}
	else if (kind == telemetry_connection_transition_kind::detached)
	{
		if (!game_connection_id(descriptor, &transition->connection))
			return false;
	}
	else
		return false;
	transition->kind = kind;
	transition->reserved[0] = 0U;
	transition->reserved[1] = 0U;
	transition->reserved[2] = 0U;
	transition->quality_flags = TELEMETRY_QUALITY_NONE;
	return telemetry_connection_transition_is_valid(*transition);
}

bool game_exit_payload(const struct char_data *character, const struct descriptor_data *descriptor,
		       telemetry_session_end_reason reason, telemetry_session_exit *exit) noexcept
{
	if (exit == nullptr || !game_session_ref(character, &exit->session) ||
	    !game_time(&exit->at_monotonic_usec, &exit->at_utc_usec))
		return false;
	exit->connection = {};
	if (descriptor != nullptr && !game_connection_id(descriptor, &exit->connection))
		return false;
	exit->reason = reason;
	exit->reserved[0] = 0U;
	exit->reserved[1] = 0U;
	exit->reserved[2] = 0U;
	exit->quality_flags = descriptor == nullptr ? TELEMETRY_QUALITY_CONTEXT_UNKNOWN :
						      TELEMETRY_QUALITY_NONE;
	return telemetry_session_exit_is_valid(*exit);
}

bool map_runtime_evidence_kind(telemetry_runtime_evidence_kind runtime_kind,
			       telemetry_activity_evidence_kind *activity_kind) noexcept
{
	if (activity_kind == nullptr)
		return false;
	switch (runtime_kind)
	{
	case telemetry_runtime_evidence_kind::player_action:
		*activity_kind = telemetry_activity_evidence_kind::player_action;
		return true;
	case telemetry_runtime_evidence_kind::movement:
		*activity_kind = telemetry_activity_evidence_kind::movement;
		return true;
	case telemetry_runtime_evidence_kind::interaction:
		*activity_kind = telemetry_activity_evidence_kind::interaction;
		return true;
	case telemetry_runtime_evidence_kind::communication:
		*activity_kind = telemetry_activity_evidence_kind::communication;
		return true;
	case telemetry_runtime_evidence_kind::combat_participation:
		*activity_kind = telemetry_activity_evidence_kind::combat_participation;
		return true;
	case telemetry_runtime_evidence_kind::automatic_combat:
		*activity_kind = telemetry_activity_evidence_kind::automatic_combat;
		return true;
	case telemetry_runtime_evidence_kind::linkdead:
		/* The public runtime enum is intentionally compact; the canonical
		 * activity enum reserves 7-15 and assigns linkdead=16. */
		*activity_kind = telemetry_activity_evidence_kind::linkdead;
		return true;
	}
	return false;
}

bool game_evidence_payload(const struct char_data *character,
			   const struct descriptor_data *descriptor,
			   telemetry_runtime_evidence_kind kind,
			   telemetry_runtime_evidence *evidence) noexcept
{
	if (evidence == nullptr || !game_session_ref(character, &evidence->session) ||
	    !game_time(&evidence->at_monotonic_usec, &evidence->at_utc_usec))
		return false;
	evidence->connection = {};
	if (kind != telemetry_runtime_evidence_kind::linkdead &&
	    !game_connection_id(descriptor, &evidence->connection))
		return false;
	evidence->kind = kind;
	evidence->reserved[0] = 0U;
	evidence->reserved[1] = 0U;
	evidence->reserved[2] = 0U;
	evidence->quality_flags = TELEMETRY_QUALITY_NONE;
	return telemetry_runtime_evidence_is_valid(*evidence);
}

} // namespace

telemetry_capture_result telemetry_runtime_game_enter(struct char_data *character,
						      struct descriptor_data *descriptor)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	if (!ensure_current_config())
		return disabled_capture(telemetry_runtime_outcome::queue_full);
	if (character == nullptr || descriptor == nullptr || character->only.pc == nullptr)
		return game_capture_invalid();
	if (character->telemetry_session_sequence != 0U)
	{
		if (!game_allocate_connection(descriptor))
			return game_capture_invalid();
		return telemetry_runtime_game_connection_transition(
			character, descriptor, telemetry_connection_transition_kind::attached);
	}
	if (!game_allocate_session(character))
		return game_capture_invalid();
	if (!game_allocate_connection(descriptor))
	{
		game_clear_session(character);
		return game_capture_invalid();
	}
	telemetry_session_enter enter{};
	if (!game_enter_payload(character, descriptor, &enter))
	{
		game_clear_connection(descriptor);
		game_clear_session(character);
		return game_capture_invalid();
	}
	return telemetry_runtime_session_enter(enter);
}

telemetry_handoff_result telemetry_runtime_game_handoff_copy(struct char_data *character)
{
	telemetry_handoff_result result{};
	result.outcome = !R.initialized	    ? telemetry_runtime_outcome::not_initialized :
			 !R.enabled	    ? telemetry_runtime_outcome::disabled :
			 R.shutdown_pending ? telemetry_runtime_outcome::stopping :
					      telemetry_runtime_outcome::invalid;
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return result;
	telemetry_session_ref session{};
	if (!game_session_ref(character, &session))
		return result;
	return telemetry_runtime_session_handoff_copy(session);
}

telemetry_capture_result
telemetry_runtime_game_session_resume(struct char_data *character,
				      struct descriptor_data *descriptor,
				      const telemetry_session_handoff *handoff)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	if (character == nullptr || descriptor == nullptr || character->only.pc == nullptr ||
	    !ensure_current_config())
		return game_capture_invalid();
	if (descriptor->telemetry_connection_sequence != 0U ||
	    descriptor->telemetry_connection_producer_boot_id != 0U ||
	    descriptor->telemetry_connection_producer_process_id != 0U)
		return game_capture_invalid();

	const telemetry_session_handoff supplied = handoff != nullptr ? *handoff :
									telemetry_session_handoff{};
	const bool absent = handoff == nullptr || handoff_is_zero(supplied);
	const std::uint64_t old_session_sequence = character->telemetry_session_sequence;
	const std::uint64_t old_session_boot_id = character->telemetry_session_producer_boot_id;
	const std::uint64_t old_session_process_id =
		character->telemetry_session_producer_process_id;
	const bool character_session_is_zero = old_session_sequence == 0U &&
					       old_session_boot_id == 0U &&
					       old_session_process_id == 0U;
	if (!character_session_is_zero &&
	    (absent || old_session_sequence != supplied.session.id.session_seq ||
	     old_session_boot_id != supplied.session.id.producer.boot_id ||
	     old_session_process_id != supplied.session.id.producer.process_id))
		return game_capture_invalid();
	if (!absent &&
	    (!telemetry_session_ref_is_valid(supplied.session) ||
	     supplied.session.subject_id !=
		     static_cast<telemetry_subject_id>(character->only.pc->pid) ||
	     supplied.session.pid != static_cast<telemetry_pid>(character->only.pc->pid) ||
	     supplied.session.season_id != R.session_scope_season_id ||
	     supplied.session.environment_id != R.session_scope_environment_id ||
	     !telemetry_producer_id_is_valid(supplied.previous_producer) ||
	     (supplied.previous_producer.boot_id == R.producer.boot_id &&
	      supplied.previous_producer.process_id == R.producer.process_id)))
		return game_capture_invalid();

	if (absent)
	{
		if (!game_allocate_session(character))
			return game_capture_invalid();
	}
	else
	{
		character->telemetry_session_sequence = supplied.session.id.session_seq;
		character->telemetry_session_producer_boot_id =
			supplied.session.id.producer.boot_id;
		character->telemetry_session_producer_process_id =
			supplied.session.id.producer.process_id;
	}
	if (!game_allocate_connection(descriptor))
	{
		character->telemetry_session_sequence = old_session_sequence;
		character->telemetry_session_producer_boot_id = old_session_boot_id;
		character->telemetry_session_producer_process_id = old_session_process_id;
		return game_capture_invalid();
	}

	telemetry_session_enter entry{};
	if (!game_enter_payload(character, descriptor, &entry))
	{
		game_clear_connection(descriptor);
		character->telemetry_session_sequence = old_session_sequence;
		character->telemetry_session_producer_boot_id = old_session_boot_id;
		character->telemetry_session_producer_process_id = old_session_process_id;
		return game_capture_invalid();
	}
	if (!absent)
		entry.quality_flags |= supplied.quality_flags;
	const telemetry_session_resume resume = { supplied, entry };
	const telemetry_capture_result result = telemetry_runtime_session_resume(resume);
	// Queue loss and failed state admission share queue_full. Preserve game IDs
	// only when a matching connected session actually exists; lifecycle queue
	// loss after slot installation must not erase an admitted session.
	telemetry_session_state_view view{};
	const bool admitted =
		telemetry_session_state_copy_view(&R.session, entry.session, &view) &&
		view.connected != 0U && view.closed == 0U &&
		view.connection.connection_seq == entry.connection.connection_seq &&
		view.connection.producer.boot_id == entry.connection.producer.boot_id &&
		view.connection.producer.process_id == entry.connection.producer.process_id;
	if (admitted && (result.outcome == telemetry_runtime_outcome::accepted ||
			 result.outcome == telemetry_runtime_outcome::queue_full))
		return result;
	game_clear_connection(descriptor);
	character->telemetry_session_sequence = old_session_sequence;
	character->telemetry_session_producer_boot_id = old_session_boot_id;
	character->telemetry_session_producer_process_id = old_session_process_id;
	return result;
}

telemetry_capture_result telemetry_runtime_game_context(struct char_data *character,
							struct descriptor_data *descriptor)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	if (character == nullptr || descriptor == nullptr || !ensure_current_config())
		return game_capture_invalid();
	telemetry_context_update update{};
	if (!game_session_ref(character, &update.session) ||
	    !game_connection_id(descriptor, &update.connection) ||
	    !game_time(&update.at_monotonic_usec, &update.at_utc_usec))
		return game_capture_invalid();
	update.dimensions = {};
	update.quality_flags = TELEMETRY_QUALITY_NONE;
	game_dimensions(character, &update.dimensions, &update.quality_flags);
	const bool any_dimension =
		update.dimensions.level_band != 0U || update.dimensions.class_id != 0U ||
		update.dimensions.race_id != 0U || update.dimensions.faction_id != 0U ||
		update.dimensions.zone_vnum >= 0 || update.dimensions.group_size != 0U;
	update.category = telemetry_interval_category::connected_idle;
	// Describe observed combat without claiming that automatic combat is
	// human activity. The classifier owns active/idle accounting separately.
	update.context = character->specials.fighting != nullptr ?
				 telemetry_activity_context::combat :
				 telemetry_activity_context::none;
	update.context_quality = update.quality_flags == TELEMETRY_QUALITY_NONE ?
					 telemetry_context_quality::observed :
				 any_dimension ? telemetry_context_quality::partial :
						 telemetry_context_quality::unavailable;
	update.reserved = 0U;
	update.config_id = R.config.config_id;
	update.classifier_version = R.config.classifier_version;
	update.policy_version = R.config.policy_version;
	telemetry_capture_result result = telemetry_runtime_update_context(update);
	if ((character->specials.act & PLR_AFK) != 0U)
	{
		// Map the game status to the existing classifier's force-idle evidence;
		// do not implement a second active-window policy in gameplay hooks.
		telemetry_activity_evidence evidence{};
		evidence.session = update.session;
		evidence.connection = update.connection;
		evidence.at_monotonic_usec = update.at_monotonic_usec;
		evidence.at_utc_usec = update.at_utc_usec;
		evidence.kind = telemetry_activity_evidence_kind::afk;
		merge_capture(result, capture_with_delta(telemetry_activity_state_record_evidence(
					      &R.activity, evidence)));
	}
	return result;
}

telemetry_capture_result
telemetry_runtime_game_connection_transition(struct char_data *character,
					     struct descriptor_data *descriptor,
					     telemetry_connection_transition_kind kind)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	if (character == nullptr || descriptor == nullptr)
		return game_capture_invalid();
	telemetry_connection_transition transition{};
	if (kind == telemetry_connection_transition_kind::attached)
	{
		if (descriptor->telemetry_connection_sequence == 0U &&
		    !game_allocate_connection(descriptor))
			return game_capture_invalid();
	}
	if (!game_transition_payload(character, descriptor, kind, &transition))
		return game_capture_invalid();
	const telemetry_capture_result result = telemetry_runtime_connection_transition(transition);
	if (kind == telemetry_connection_transition_kind::detached)
		game_clear_connection(descriptor);
	return result;
}

telemetry_capture_result telemetry_runtime_game_session_exit(struct char_data *character,
							     struct descriptor_data *descriptor,
							     telemetry_session_end_reason reason)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	if (character == nullptr)
		return game_capture_invalid();
	telemetry_session_exit exit{};
	if (!game_exit_payload(character, descriptor, reason, &exit))
		return game_capture_invalid();
	const telemetry_capture_result result = telemetry_runtime_session_exit(exit);
	game_clear_connection(descriptor);
	game_clear_session(character);
	return result;
}

telemetry_capture_result telemetry_runtime_game_evidence(struct char_data *character,
							 struct descriptor_data *descriptor,
							 telemetry_runtime_evidence_kind kind)
{
	if (!R.initialized || !R.enabled || R.shutdown_pending)
		return game_capture_not_ready();
	telemetry_runtime_evidence evidence{};
	if (!game_evidence_payload(character, descriptor, kind, &evidence))
		return game_capture_invalid();
	return telemetry_runtime_record_evidence(evidence);
}

std::uint16_t telemetry_runtime_pulse_slot_count(void) noexcept
{
	if (!R.initialized || !R.enabled || R.shutdown_pending || R.config.pulse_slot_count == 0U)
		return 1U;
	return R.config.pulse_slot_count;
}
