#ifndef DURIS_TELEMETRY_CONFIG_PRIVATE_H
#define DURIS_TELEMETRY_CONFIG_PRIVATE_H

#include "telemetry/telemetry_config.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

/*
 * Private #263 boundary.  The public snapshot remains the only value that can
 * cross the telemetry transport.  This header contains the fixed-size capture,
 * publication and test seams used by #265 and the focused service tests.
 */
inline constexpr std::size_t TELEMETRY_CONFIG_CANONICAL_BYTES = 70U;
inline constexpr std::size_t TELEMETRY_CONFIG_PENDING_MAX = 4U;
inline constexpr std::size_t TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX = 64U;
inline constexpr std::size_t TELEMETRY_CONFIG_PROPERTY_MAX = 19U;
inline constexpr std::size_t TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES =
	TELEMETRY_CONFIG_FINGERPRINT_BYTES;
inline constexpr std::uint16_t TELEMETRY_CONFIG_PROPERTY_SCHEMA_VERSION = 1U;

/* The registry is reviewed source identity, not a dump of duris.properties. */
enum class telemetry_config_property_role : std::uint8_t
{
	effective = 0,
	hardcoded_effective = 1,
	unavailable_context = 2,
	maintained_but_unused = 3,
};

enum class telemetry_config_property_kind : std::uint8_t
{
	float32 = 0,
	uint32 = 1,
	boolean = 2,
	unknown = 3,
};

enum class telemetry_config_property_source : std::uint8_t
{
	declared_default = 0,
	loaded_property = 1,
	hardcoded = 2,
	unavailable = 3,
	excluded_unused = 4,
};

struct telemetry_config_property_definition
{
	std::uint16_t id;
	telemetry_config_property_role role;
	telemetry_config_property_kind kind;
	const char *stable_name;
	const char *source_key;
	std::uint32_t default_value;
};

/* Values are typed bits, never a string or a raw property object. */
struct telemetry_config_property_entry
{
	std::uint16_t id;
	std::uint8_t role;
	std::uint8_t kind;
	std::uint8_t source;
	std::uint8_t reserved[3];
	std::uint32_t value;
};

struct telemetry_config_property_snapshot
{
	std::uint16_t schema_version;
	std::uint16_t count;
	std::uint32_t property_version;
	std::uint32_t reserved;
	std::uint8_t effective_digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES];
	std::uint32_t stable_namespace;
	std::uint32_t stable_version;
	telemetry_config_property_entry entries[TELEMETRY_CONFIG_PROPERTY_MAX];
};

struct telemetry_config_property_catalog_resolution
{
	std::uint32_t property_version;
	std::uint32_t stable_namespace;
	std::uint32_t stable_version;
	std::uint8_t digest[TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES];
};

struct telemetry_config_property_catalog
{
	/* The owner supplies a preloaded durable/reviewed catalog.  A false result
	 * is refusal, never permission to allocate a process-local version. */
	using resolve_function = bool (*)(void *, const telemetry_config_property_snapshot *,
					  const std::uint8_t *,
					  telemetry_config_property_catalog_resolution *) noexcept;

	resolve_function resolve;
	void *context;
};

struct telemetry_config_property_reader
{
	/* The callback must return the effective float32 value after the game's
	 * get_property conversion, including its declared fallback. */
	using read_function = bool (*)(void *, const char *, float *) noexcept;

	read_function read;
	void *context;
};

enum class telemetry_config_property_capture_mode : std::uint8_t
{
	require_reader = 0,
	declared_defaults = 1,
};

struct telemetry_config_property_capture
{
	telemetry_config_property_reader reader;
	telemetry_config_property_capture_mode mode;
	std::uint8_t reserved[7];
	telemetry_config_property_catalog catalog;
};

/* All fields are supplied by the configuration owner; #263 allocates no
 * revision or property counter. Enabled property_version comes from the reviewed
 * catalog resolution of the full effective typed allowlist digest. */
struct telemetry_config_capture_input
{
	std::uint16_t schema_version;
	std::uint16_t reserved;
	telemetry_config_revision revision;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_season_id season_id;
	telemetry_environment_id environment_id;
	telemetry_utc_usec effective_utc_usec;
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec checkpoint_interval_usec;
	telemetry_duration_usec active_window_usec;
	std::uint32_t context_segments_per_minute;
	std::uint16_t pulse_slot_count;
	telemetry_storage_backend backend;
	std::uint8_t enabled;
	telemetry_config_property_capture properties;
};

enum class telemetry_config_build_outcome : std::uint8_t
{
	built = 0,
	invalid_input = 1,
	missing_property_context = 2,
	property_registry_invalid = 3,
	property_catalog_unavailable = 4,
};

const telemetry_config_property_definition *
telemetry_config_property_registry(std::size_t *count) noexcept;
bool telemetry_config_property_name_is_allowlisted(const char *name) noexcept;

telemetry_config_property_snapshot telemetry_config_property_snapshot_defaults(void) noexcept;
telemetry_config_build_outcome
telemetry_config_property_snapshot_capture(const telemetry_config_property_capture *capture,
					   telemetry_config_property_snapshot *snapshot) noexcept;

std::size_t telemetry_config_canonical_encode(const telemetry_config_snapshot &config,
					      std::uint8_t *bytes, std::size_t capacity) noexcept;
bool telemetry_config_compute_fingerprint(const telemetry_config_snapshot &config,
					  std::uint8_t *fingerprint, std::size_t capacity) noexcept;
telemetry_config_id telemetry_config_id_from_fingerprint(const std::uint8_t *fingerprint,
							 std::size_t size) noexcept;

telemetry_config_build_outcome
telemetry_config_snapshot_build(const telemetry_config_capture_input *input,
				telemetry_config_property_snapshot *properties,
				telemetry_config_snapshot *snapshot) noexcept;

struct telemetry_config_sink
{
	using emit_function = bool (*)(void *, const telemetry_record *) noexcept;

	emit_function emit;
	void *context;
};

struct telemetry_config_key_allocator
{
	using next_function = bool (*)(void *, telemetry_record_kind,
				       telemetry_record_key *) noexcept;

	next_function next;
	void *context;
};

enum class telemetry_config_state_outcome : std::uint8_t
{
	uninitialized = 0,
	accepted = 1,
	unchanged = 2,
	disabled = 3,
	flatfile_disabled = 4,
	sink_rejected = 5,
	allocator_exhausted = 6,
	capacity_full = 7,
	stale_revision = 8,
	identity_collision = 9,
	identity_registry_full = 10,
	invalid = 11,
	missing_context = 12,
	revision_conflict = 13,
};

struct telemetry_config_state_result
{
	telemetry_config_state_outcome outcome;
	telemetry_config_validation status;
	std::uint8_t visible;
	std::uint8_t admission_attempted;
	std::uint8_t admission_accepted;
	std::uint8_t reserved;
	std::uint16_t pending_count;
	std::uint16_t reserved2;
	telemetry_record_key record_key;
	telemetry_config_snapshot snapshot;
};

struct telemetry_config_state_config
{
	telemetry_config_sink sink;
	telemetry_config_key_allocator key_allocator;
};

struct telemetry_config_pending_entry
{
	std::uint8_t used;
	std::uint8_t has_key;
	std::uint8_t reserved[6];
	telemetry_config_snapshot snapshot;
	telemetry_config_snapshot canonical;
	telemetry_record_key key;
	telemetry_record record;
};

struct telemetry_config_identity_entry
{
	std::uint8_t used;
	std::uint8_t admitted;
	std::uint8_t reserved[6];
	telemetry_config_id config_id;
	std::uint8_t fingerprint[TELEMETRY_CONFIG_FINGERPRINT_BYTES];
	telemetry_config_snapshot canonical;
};

struct telemetry_config_state
{
	std::uint8_t initialized;
	std::uint8_t has_admitted;
	std::uint8_t visible;
	std::uint8_t reload_requested;
	std::uint16_t pending_count;
	std::uint16_t reserved;
	telemetry_config_revision highest_revision;
	telemetry_config_revision visibility_floor_revision;
	telemetry_config_snapshot admitted;
	telemetry_config_sink sink;
	telemetry_config_key_allocator key_allocator;
	std::uint64_t publication_attempts;
	std::uint64_t publication_accepts;
	std::uint64_t publication_rejections;
	telemetry_config_pending_entry pending[TELEMETRY_CONFIG_PENDING_MAX];
	telemetry_config_identity_entry identities[TELEMETRY_CONFIG_IDENTITY_REGISTRY_MAX];
};

struct telemetry_config_state_stats
{
	std::uint16_t capacity;
	std::uint16_t pending_count;
	std::uint8_t initialized;
	std::uint8_t has_admitted;
	std::uint8_t visible;
	std::uint8_t reload_requested;
	std::uint64_t publication_attempts;
	std::uint64_t publication_accepts;
	std::uint64_t publication_rejections;
};

static_assert(std::is_trivially_copyable_v<telemetry_config_capture_input>);
static_assert(std::is_trivially_copyable_v<telemetry_config_property_snapshot>);
static_assert(std::is_trivially_copyable_v<telemetry_config_state>);
static_assert(std::is_standard_layout_v<telemetry_config_state>);

telemetry_config_state_outcome
telemetry_config_state_init(telemetry_config_state *state,
			    const telemetry_config_state_config *config) noexcept;
void telemetry_config_state_reset(telemetry_config_state *state) noexcept;
/* Seed original immutable row metadata before the first publication, after the
 * owner resolves it off-thread. Never marks a control record as admitted. */
telemetry_config_state_outcome
telemetry_config_state_seed_identity(telemetry_config_state *state,
				     const telemetry_config_snapshot &canonical) noexcept;
telemetry_config_state_result
telemetry_config_state_publish(telemetry_config_state *state,
			       telemetry_config_snapshot snapshot) noexcept;
telemetry_config_snapshot
telemetry_config_state_snapshot_copy(const telemetry_config_state *state) noexcept;
telemetry_config_validation
telemetry_config_state_status(const telemetry_config_state *state) noexcept;
telemetry_config_state_stats
telemetry_config_state_stats_copy(const telemetry_config_state *state) noexcept;

/* Registered through the parent-owned telemetry_config_reload.h seam.  The
 * observer invalidates attribution and sets a bit; capture/publication stays an explicit bounded call
 * by the game-thread owner. */
void telemetry_config_reload_observer(void *context) noexcept;
bool telemetry_config_reload_requested(const telemetry_config_state *state) noexcept;
void telemetry_config_reload_request_clear(telemetry_config_state *state) noexcept;

/* #265 binds the process-wide public functions after its transport/key owner is
 * ready.  Until then the public functions return an unavailable status. */
telemetry_config_state_outcome
telemetry_config_global_init(const telemetry_config_state_config *config) noexcept;
void telemetry_config_global_reset(void) noexcept;
/* Borrowed game-thread-only state used by the public facade and reload seam. */
telemetry_config_state *telemetry_config_global_state(void) noexcept;

#endif
