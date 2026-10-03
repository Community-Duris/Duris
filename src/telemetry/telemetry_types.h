#ifndef DURIS_TELEMETRY_TYPES_H
#define DURIS_TELEMETRY_TYPES_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

/*
 * Public telemetry contract, schema 1.
 *
 * These declarations are deliberately value-only.  A record is copied into
 * the bounded transport queue; it never owns a player, connection, string,
 * JSON document, SQL handle, or callback.  The SQL repository assigns its
 * ingestion identity separately; the producer identity and record sequence
 * below are the replay identity and must survive retries unchanged.
 */

inline constexpr std::uint16_t TELEMETRY_SCHEMA_VERSION = 1U;
inline constexpr std::size_t TELEMETRY_RECORD_MAX_BYTES = 512U;
inline constexpr std::size_t TELEMETRY_CONFIG_FINGERPRINT_BYTES = 32U;

/*
 * Candidate first-release budgets from the design review.  They are bounded
 * defaults/proposals, not measured acceptance claims.  Changing them is a
 * contract decision because queue and batch owners must agree on the limits.
 */
inline constexpr std::size_t TELEMETRY_QUEUE_CAPACITY_PROPOSAL = 8192U;
inline constexpr std::size_t TELEMETRY_CONTROL_RESERVE_PROPOSAL = 128U;
inline constexpr std::size_t TELEMETRY_BATCH_MAX_RECORDS_PROPOSAL = 128U;
inline constexpr std::size_t TELEMETRY_BATCH_MAX_BYTES_PROPOSAL = 64U * 1024U;
inline constexpr std::uint64_t TELEMETRY_INTERVAL_USEC_PROPOSAL = 60'000'000ULL;
inline constexpr std::uint64_t TELEMETRY_FLUSH_OLDEST_USEC_PROPOSAL = 2'000'000ULL;
inline constexpr std::uint64_t TELEMETRY_FLUSH_OLDEST_USEC_MAX_PROPOSAL = 3'600'000'000ULL;
inline constexpr std::uint32_t TELEMETRY_CONTEXT_SEGMENTS_PER_MINUTE_PROPOSAL = 8U;

using telemetry_id = std::uint64_t;
using telemetry_sequence = std::uint64_t;
using telemetry_record_sequence = telemetry_sequence;
using telemetry_connection_sequence = telemetry_sequence;
using telemetry_session_sequence = telemetry_sequence;
using telemetry_revision = std::uint64_t;
using telemetry_checkpoint_revision = telemetry_revision;
using telemetry_config_revision = telemetry_revision;
using telemetry_monotonic_usec = std::uint64_t;
using telemetry_duration_usec = std::uint64_t;
using telemetry_utc_usec = std::int64_t;
using telemetry_subject_id = telemetry_id;
using telemetry_season_id = telemetry_id;
using telemetry_environment_id = telemetry_id;
using telemetry_config_id = telemetry_id;
using telemetry_pid = std::int32_t;
using telemetry_quality_mask = std::uint32_t;

inline constexpr telemetry_id TELEMETRY_UNKNOWN_ID = 0U;
inline constexpr telemetry_pid TELEMETRY_UNKNOWN_PID = -1;
inline constexpr telemetry_utc_usec TELEMETRY_UTC_UNKNOWN =
	std::numeric_limits<telemetry_utc_usec>::min();

/* The active union member selected by telemetry_record_header::kind. */
enum class telemetry_record_kind : std::uint8_t
{
	invalid = 0,
	interval = 1,
	session_lifecycle = 2,
	session_checkpoint = 3,
	coverage_gap = 4,
	configuration = 5,
	progression = 6,
	encounter = 7,
	combat_summary = 8,
	ownership = 9,
};

/* Observed authenticated descriptor ownership; preparation alone emits no fact. */
enum class telemetry_ownership_source : std::uint8_t
{
	authenticated_login = 1,
	reconnect = 2,
	copyover = 3,
	ownership_changed = 4,
	unavailable = 5,
};

/* Progression facts keep XP arithmetic separate from level-threshold use. */
enum class telemetry_progression_kind : std::uint8_t
{
	unknown = 0,
	experience_observed = 1,
	level_advanced = 2,
	level_lost = 3,
};

/* Values 1-10 mirror the existing gain_exp source constants. */
enum class telemetry_progression_source : std::uint8_t
{
	unknown = 0,
	damage = 1,
	healing = 2,
	kill = 3,
	death = 4,
	quest = 5,
	resurrect = 6,
	melee = 7,
	world_quest = 8,
	tanking = 9,
	boon = 10,
	administration = 11,
	system = 12,
};

enum class telemetry_progression_reason : std::uint8_t
{
	unknown = 0,
	earned = 1,
	death_loss = 2,
	resurrection = 3,
	level_threshold = 4,
	administration = 5,
	system_adjustment = 6,
};

/* H emits observed_mutable. Later recovery/ledger work may promote facts. */
enum class telemetry_progression_observation_status : std::uint8_t
{
	unknown = 0,
	observed_mutable = 1,
	recovered_checkpoint = 2,
	durable_reconciled = 3,
};

/* Encounter facts are append-only lifecycle/roster observations.  A run's
 * terminal outcome is distinct from expected reward credit and from each
 * participant's observed time. */
enum class telemetry_encounter_mode : std::uint8_t
{
	unknown = 0,
	pve = 1,
	pvp = 2,
	mixed = 3,
};

enum class telemetry_encounter_event_kind : std::uint8_t
{
	start = 1,
	participant_join = 2,
	participant_leave = 3,
	close = 4,
	participant_summary = 5,
};

enum class telemetry_encounter_outcome : std::uint8_t
{
	unknown = 0,
	success = 1,
	failure = 2,
	death = 3,
	flee = 4,
	withdrawal = 5,
	abandonment = 6,
	timeout = 7,
	copyover = 8,
	shutdown = 9,
	unknown_close = 10,
};

/* Combat rows retain actor identity without retaining a game pointer. */
enum class telemetry_combat_actor_kind : std::uint8_t
{
	unknown = 0,
	player = 1,
	pet = 2,
	npc = 3,
};

/* Lifecycle records describe a logical session or an explicit socket edge. */
enum class telemetry_lifecycle_kind : std::uint8_t
{
	unknown = 0,
	session_entered = 1,
	session_exited = 2,
	connection_attached = 3,
	connection_detached = 4,
};

enum class telemetry_session_end_reason : std::uint8_t
{
	unknown = 0,
	logout = 1,
	/* Logical session unload after link loss; not a mere connection detach. */
	disconnect = 2,
	shutdown = 3,
	copyover = 4,
	process_restart = 5,
};

/* A connection can change while the logical session remains stable. */
enum class telemetry_connection_transition_kind : std::uint8_t
{
	attached = 1,
	detached = 2,
	copyover_resumed = 3,
};

/*
 * Exactly one category owns an interval's duration.  Active and idle are
 * connected time; unknown is connected time whose activity classifier was
 * unavailable.  A context-segment cap uses overflow_unknown context and
 * overflow quality, but preserves connected_active/connected_idle whenever
 * activity evidence remains known.  resident_linkdead is resident time after
 * detachment and uses an all-zero connection id.  Categories are not additive
 * counters inside one interval.
 */
enum class telemetry_interval_category : std::uint8_t
{
	unknown = 0,
	connected_idle = 1,
	connected_active = 2,
	resident_linkdead = 3,
};

/* Context is descriptive and is never used to infer duration quality. */
enum class telemetry_activity_context : std::uint8_t
{
	unknown = 0,
	none = 1,
	combat = 2,
	travel = 3,
	social = 4,
	crafting = 5,
	administration = 6,
	other = 7,
	overflow_unknown = 8,
};

enum class telemetry_context_quality : std::uint8_t
{
	unknown = 0,
	observed = 1,
	partial = 2,
	overflow = 3,
	unavailable = 4,
};

/* A gap is explicit evidence of incomplete observation, never a zero fact. */
enum class telemetry_gap_reason : std::uint8_t
{
	unknown = 0,
	detail_queue_drop = 1,
	control_queue_drop = 2,
	sequence_gap = 3,
	telemetry_disabled = 4,
	unclosed_tail = 5,
	clock_discontinuity = 6,
	record_quarantined = 7,
};

/*
 * SQL is the only initial sink.  flatfile_disabled is an intentional status:
 * flat-file authority does not write a hidden telemetry file and must report
 * telemetry as disabled/unsupported rather than as healthy zero activity.
 */
enum class telemetry_storage_backend : std::uint8_t
{
	sql = 1,
	flatfile_disabled = 2,
};

/*
 * Configuration identity is copied into the typed stream before intervals
 * refer to it.  The fingerprint is an opaque, fixed-width digest; it is not
 * a pointer or a variable-size document. Full representation validation lives
 * in this header so both standalone config and tagged-record validation use
 * the same bounds/backend rules without an include cycle.
 */
struct telemetry_config_snapshot
{
	std::uint16_t schema_version;
	std::uint16_t reserved;
	telemetry_config_id config_id;
	telemetry_config_revision revision;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t property_version;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_season_id season_id;
	telemetry_environment_id environment_id;
	std::uint8_t fingerprint[TELEMETRY_CONFIG_FINGERPRINT_BYTES];
	telemetry_utc_usec effective_utc_usec;
	telemetry_duration_usec interval_usec;
	telemetry_duration_usec checkpoint_interval_usec;
	telemetry_duration_usec active_window_usec;
	std::uint32_t context_segments_per_minute;
	std::uint16_t pulse_slot_count;
	telemetry_storage_backend backend;
	std::uint8_t enabled;
};

enum class telemetry_health_state : std::uint8_t
{
	disabled = 0,
	starting = 1,
	healthy = 2,
	degraded = 3,
	stopping = 4,
	stopped = 5,
	circuit_open = 6,
};

/*
 * Failure classes are intentionally stable, compact diagnostic values. SQL
 * text and record payloads never cross the repository boundary as health
 * data. commit_ambiguous remains distinct because only exact replay can
 * resolve whether the transaction committed.
 */
enum class telemetry_failure_class : std::uint8_t
{
	none = 0,
	transient_connection = 1,
	transient_transaction = 2,
	transient_internal = 3,
	commit_ambiguous = 4,
	invalid_record = 5,
	permanent_schema = 6,
	permanent_permission = 7,
	permanent_repository = 8,
};

enum class telemetry_disabled_reason : std::uint8_t
{
	none = 0,
	configured_off = 1,
	flatfile_authority = 2,
	unsupported_schema = 3,
	not_initialized = 4,
};

/* The SQL repository holds its advisory lock for the lifetime of the owned
 * writer connection.  This compact state is safe to expose; lock names and
 * connection details remain private to the repository. */
enum class telemetry_advisory_lock_state : std::uint8_t
{
	not_applicable = 0,
	unavailable = 1,
	held = 2,
};

/* Queue admission is RAM retention, not durable acknowledgement. */
enum class telemetry_queue_admission : std::uint8_t
{
	accepted_detail = 0,
	accepted_control_reserve = 1,
	rejected_invalid = 2,
	rejected_disabled = 3,
	rejected_stopping = 4,
	rejected_detail_full = 5,
	rejected_control_full = 6,
	rejected_oversize = 7,
	rejected_circuit_open = 8,
	/* Worker startup has not yet validated the storage contract. */
	rejected_not_ready = 9,
};

/*
 * Per-record repository result.  duplicate_identical and checkpoint_older
 * are idempotent/no-op outcomes; duplicate_conflict is a data-integrity
 * failure.  retryable_failure and commit_ambiguous retain the immutable batch
 * for retry/reconciliation rather than generating a new record key.
 */
enum class telemetry_apply_outcome : std::uint8_t
{
	applied = 0,
	duplicate_identical = 1,
	checkpoint_older = 2,
	rejected_invalid = 3,
	duplicate_conflict = 4,
	retryable_failure = 5,
	commit_ambiguous = 6,
	unavailable = 7,
	disabled = 8,
	quarantined_invalid = 9,
	permanent_failure = 10,
};

enum class telemetry_batch_outcome : std::uint8_t
{
	committed = 0,
	committed_with_rejections = 1,
	invalid_batch = 2,
	retryable_failure = 3,
	commit_ambiguous = 4,
	unavailable = 5,
	disabled = 6,
	permanent_failure = 7,
};

/* Observation-quality flags are separate from repository/queue health. */
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_NONE = 0U;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_CONTEXT_UNKNOWN = 1U << 0;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_CONTEXT_OVERFLOW = 1U << 1;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_DIMENSION_UNKNOWN = 1U << 2;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_SEQUENCE_GAP = 1U << 3;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_QUEUE_DROP = 1U << 4;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_DISABLED = 1U << 5;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_UNCLOSED_TAIL = 1U << 6;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_CLOCK_DISCONTINUITY = 1U << 7;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_LATE = 1U << 8;
inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_CARDINALITY_OVERFLOW = 1U << 9;

/* Compact combat context; values are intentionally not an open-ended map. */
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_NONE = 0U;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_SPELL = 1U << 0;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_MELEE = 1U << 1;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_PVP = 1U << 2;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_PET = 1U << 3;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_NPC = 1U << 4;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_CONTROL = 1U << 5;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_TANKING = 1U << 6;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_SELF = 1U << 7;
inline constexpr std::uint32_t TELEMETRY_COMBAT_MODIFIER_KNOWN =
	TELEMETRY_COMBAT_MODIFIER_SPELL | TELEMETRY_COMBAT_MODIFIER_MELEE |
	TELEMETRY_COMBAT_MODIFIER_PVP | TELEMETRY_COMBAT_MODIFIER_PET |
	TELEMETRY_COMBAT_MODIFIER_NPC | TELEMETRY_COMBAT_MODIFIER_CONTROL |
	TELEMETRY_COMBAT_MODIFIER_TANKING | TELEMETRY_COMBAT_MODIFIER_SELF;

inline constexpr std::uint16_t TELEMETRY_COMBAT_SUMMARY_MAX_PARTICIPANTS = 64U;
inline constexpr std::uint16_t TELEMETRY_COMBAT_SUMMARY_MAX_UNIQUE_PLAYERS = 64U;

/* These flags describe bounded modifier paths, not arbitrary property maps. */
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_NONE = 0U;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_RESTED = 1U << 0;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_WELLRESTED = 1U << 1;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_OVER_LEVEL_CAP = 1U << 2;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_DIFFICULTY_EARNED = 1U << 3;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_DIFFICULTY_DEATH = 1U << 4;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_PVP = 1U << 5;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_FINAL_CAP = 1U << 6;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_RACE = 1U << 7;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_VICTIM = 1U << 8;
inline constexpr std::uint32_t TELEMETRY_PROGRESSION_MODIFIER_KNOWN =
	TELEMETRY_PROGRESSION_MODIFIER_RESTED | TELEMETRY_PROGRESSION_MODIFIER_WELLRESTED |
	TELEMETRY_PROGRESSION_MODIFIER_OVER_LEVEL_CAP |
	TELEMETRY_PROGRESSION_MODIFIER_DIFFICULTY_EARNED |
	TELEMETRY_PROGRESSION_MODIFIER_DIFFICULTY_DEATH | TELEMETRY_PROGRESSION_MODIFIER_PVP |
	TELEMETRY_PROGRESSION_MODIFIER_FINAL_CAP | TELEMETRY_PROGRESSION_MODIFIER_RACE |
	TELEMETRY_PROGRESSION_MODIFIER_VICTIM;

/*
 * A producer identity is allocated once for a boot/process incarnation.  An
 * OS PID alone is not sufficient because it can be reused after a restart.
 */
struct telemetry_producer_id
{
	telemetry_id boot_id;
	telemetry_id process_id;
};

struct telemetry_encounter_id
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
};

/* Live group identity is scoped to its observing producer. The high bit
 * separates formal groups from positive signed character-PID solo keys. */
inline constexpr telemetry_id TELEMETRY_GROUP_GENERATION_TAG = 1ULL << 63U;
struct telemetry_group_generation
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
	/* Zero with a valid lifetime means the observed roster revision exhausted. */
	std::uint16_t revision = 0U;
};

struct telemetry_encounter_source
{
	telemetry_environment_id environment_id;
	telemetry_season_id season_id;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	std::int32_t zone_vnum;
	telemetry_id group_key;
};

struct telemetry_encounter_participant
{
	telemetry_subject_id subject_id;
	telemetry_pid pid;
};

/* Stable replay identity: (producer_id, record_seq).  Retries keep both. */
struct telemetry_record_key
{
	telemetry_producer_id producer;
	telemetry_record_sequence record_seq;
};

/* A connection segment can change while the logical session remains stable. */
struct telemetry_connection_id
{
	telemetry_producer_id producer;
	telemetry_connection_sequence connection_seq;
};

/* A session sequence is not a connection sequence and is not an OS PID. */
struct telemetry_session_id
{
	telemetry_producer_id producer;
	telemetry_session_sequence session_seq;
};

/*
 * The subject/season/environment scope is copied into lifecycle, interval,
 * and checkpoint values.  session.id is the stable logical session key;
 * subject_id and season_id are immutable scope captured at session entry.
 * environment_id is always nonzero; unknown dimensions are represented by
 * their quality flags, not by weakening deployment identity.
 */
struct telemetry_session_ref
{
	telemetry_session_id id;
	telemetry_subject_id subject_id;
	telemetry_pid pid;
	telemetry_season_id season_id;
	telemetry_environment_id environment_id;
};

/*
 * Explicit socket transitions are not logical session enter/exit facts.
 * Reconnect keeps session.id and allocates a new connection id.  A successful
 * copyover_resumed keeps the original producer in session.id, while the new
 * process uses a fresh current producer/header identity, resets its monotonic
 * anchor, and creates a fresh connection identity; it does not fabricate
 * downtime.
 */
struct telemetry_connection_transition
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_connection_transition_kind kind;
	std::uint8_t reserved[3];
	telemetry_quality_mask quality_flags;
};

/* Monotonic endpoints define duration; UTC endpoints are reporting labels. */
struct telemetry_time_window
{
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec end_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_utc_usec end_utc_usec;
};

/* Numeric dimensions are captured at observation time, never joined live. */
struct telemetry_dimensions
{
	std::uint16_t level_band;
	std::uint16_t class_id;
	std::uint16_t race_id;
	std::uint16_t faction_id;
	std::int32_t zone_vnum;
	std::uint32_t group_size;
};

/* Numeric widths bound mapped IDs; #263 owns existence/version mapping checks. */
constexpr bool telemetry_dimensions_are_valid(const telemetry_dimensions &dimensions) noexcept
{
	return dimensions.zone_vnum >= -1;
}

constexpr bool telemetry_dimensions_are_unknown(const telemetry_dimensions &dimensions) noexcept
{
	return dimensions.level_band == 0U && dimensions.class_id == 0U &&
	       dimensions.race_id == 0U && dimensions.faction_id == 0U &&
	       dimensions.zone_vnum == -1 && dimensions.group_size == 0U;
}

/* Checkpoint values are absolute cumulative totals, not deltas. */
struct telemetry_cumulative_counters
{
	telemetry_duration_usec connected_usec;
	telemetry_duration_usec active_usec;
	telemetry_duration_usec idle_usec;
	telemetry_duration_usec unknown_usec;
	telemetry_duration_usec resident_usec;
	telemetry_duration_usec linkdead_usec;
};

/* Last observed account context, never an authentication credential or a new
 * process clock anchor. Source zero is absent (including legacy wire input).
 * Unavailable/overflow retains an explicit unknown boundary. */
struct telemetry_ownership_handoff
{
	std::uint64_t account_token;
	telemetry_utc_usec observed_at_utc_usec;
	telemetry_quality_mask quality_flags;
	telemetry_ownership_source source;
	std::uint8_t reserved[3];
};

/* Bounded copyover state; no monotonic timestamp crosses process incarnations.
 * Import retains totals and starts from a new observation's clock anchor.
 * Descriptors may hold this value while initial writer qualification completes. */
struct telemetry_session_handoff
{
	telemetry_session_ref session;
	telemetry_producer_id previous_producer;
	telemetry_checkpoint_revision last_checkpoint_revision;
	telemetry_cumulative_counters cumulative;
	telemetry_quality_mask quality_flags;
	telemetry_ownership_handoff ownership;
};

/* Common immutable record metadata. occurrence_utc_usec is not ingestion time. */
struct telemetry_record_header
{
	std::uint16_t schema_version;
	telemetry_record_kind kind;
	std::uint8_t reserved;
	telemetry_record_key key;
	telemetry_utc_usec occurrence_utc_usec;
};

/*
 * An immutable sealed [start,end) interval with one exclusive category.
 * resident_linkdead is emitted after detach with an all-zero connection;
 * connected categories require a complete nonzero connection.  If a context
 * cap overflows, context/dimensions become unknown with overflow quality, but
 * the activity category remains active or idle when that evidence is known.
 */
struct telemetry_interval_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_time_window window;
	telemetry_duration_usec duration_usec;
	telemetry_interval_category category;
	telemetry_activity_context context;
	telemetry_context_quality context_quality;
	std::uint8_t reserved;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/*
 * Lifecycle facts do not themselves claim duration.  connection_attached and
 * connection_detached are explicit socket transitions; detached references
 * the closing nonzero connection.  end_reason::disconnect is reserved for
 * logical session unload after link loss, not for the detach event itself.
 */
struct telemetry_session_lifecycle_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_lifecycle_kind lifecycle;
	telemetry_session_end_reason end_reason;
	std::uint16_t reserved;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/*
 * A checkpoint carries absolute session totals and a per-session revision.
 * Repository apply advances only when revision is newer.  It can recover a
 * later total after an earlier checkpoint was dropped, but cannot recreate
 * the missing interval's historical dimensions or coverage.  A detached
 * resident checkpoint may use an all-zero connection id; partial ids are not
 * valid.
 */
struct telemetry_session_checkpoint_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_checkpoint_revision revision;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_cumulative_counters cumulative;
	telemetry_config_id config_id;
	telemetry_quality_mask quality_flags;
};

/*
 * Progression is an immutable observed fact, not an XP authority.  XP fields
 * are signed and record the requested input, post-modifier integer candidate,
 * and actual change at the storage boundary.  Level threshold use has its own
 * kind and threshold field, so reports never infer a reward/loss from it.
 */
struct telemetry_progression_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_progression_kind kind;
	telemetry_progression_source source;
	telemetry_progression_reason reason;
	telemetry_progression_observation_status observation_status;
	std::uint32_t modifier_flags;
	std::int64_t requested_xp;
	std::int64_t computed_xp;
	std::int64_t applied_xp;
	std::int64_t before_exp;
	std::int64_t after_exp;
	std::uint16_t before_level;
	std::uint16_t after_level;
	std::uint32_t reserved;
	std::uint64_t threshold_xp;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/*
 * Gap/disabled records are control records.  A zero session reference denotes
 * a process-wide gap.  Missing sequence bounds are zero when only duration or
 * disabled status is known.
 */
struct telemetry_coverage_gap_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_gap_reason reason;
	std::uint8_t reserved[3];
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec end_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_utc_usec end_utc_usec;
	telemetry_record_sequence first_missing_record_seq;
	telemetry_record_sequence last_missing_record_seq;
	telemetry_duration_usec duration_usec;
	std::uint64_t dropped_records;
	telemetry_quality_mask quality_flags;
};

/* Configuration is a control fact on the same bounded typed stream. */
struct telemetry_configuration_payload
{
	telemetry_config_snapshot config;
};

/*
 * Encounter facts share the tagged interval stream.  A close row carries the
 * run elapsed duration and each participant_summary row carries one player's
 * observed contribution; expected reward-credit membership is a separate
 * count and is never used as the participation denominator.
 */
struct telemetry_encounter_payload
{
	telemetry_encounter_id encounter;
	telemetry_encounter_event_kind kind;
	telemetry_encounter_mode mode;
	telemetry_encounter_outcome outcome;
	std::uint8_t reserved;
	std::uint16_t revision;
	telemetry_encounter_source source;
	telemetry_encounter_participant participant;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_duration_usec elapsed_usec;
	telemetry_duration_usec participant_usec;
	std::uint16_t participant_count;
	std::uint16_t expected_credit_count;
	telemetry_quality_mask quality_flags;
};

/* One bounded contribution row is emitted per retained actor at close. */
struct telemetry_combat_summary_payload
{
	telemetry_encounter_id encounter;
	telemetry_encounter_source source;
	telemetry_encounter_mode mode;
	telemetry_encounter_outcome outcome;
	telemetry_combat_actor_kind actor_kind;
	std::uint8_t reserved;
	std::uint16_t revision;
	telemetry_id actor_id;
	telemetry_pid actor_pid;
	telemetry_subject_id owner_subject_id;
	std::uint16_t unique_player_count;
	std::uint16_t participant_count;
	std::uint16_t dropped_participant_count;
	std::uint16_t power_band;
	std::uint16_t opponent_power_band;
	std::uint16_t opponent_count;
	std::uint32_t modifier_flags;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec end_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_utc_usec end_utc_usec;
	telemetry_duration_usec damage_dealt;
	telemetry_duration_usec damage_taken;
	telemetry_duration_usec healing_attempted;
	telemetry_duration_usec effective_healing;
	telemetry_duration_usec overhealing;
	std::uint64_t control_applications;
	std::uint64_t casting_attempts;
	std::uint64_t casting_completions;
	std::uint64_t casting_aborts;
	telemetry_duration_usec casting_elapsed_usec;
	telemetry_duration_usec tanking_usec;
	telemetry_quality_mask quality_flags;
};

struct telemetry_ownership_payload
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_id account_token;
	telemetry_ownership_source source;
	std::uint8_t reserved[7];
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/* Shared battle values have a separate actor/episode domain. Their durable
 * tagged-record activation is qualified with the additive writer contract. */
inline constexpr std::uint16_t TELEMETRY_BATTLE_DEFINITION_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION = 1U;
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_ACTORS = 64U;
inline constexpr std::uint32_t TELEMETRY_BATTLE_MAX_MUTATION_FACTS = 4096U;
inline constexpr std::size_t TELEMETRY_BATTLE_MAX_FACTS_PER_MUTATION = 5U;
inline constexpr telemetry_id TELEMETRY_BATTLE_NPC_GENERATION_TAG = 1ULL << 63U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_COMBAT = 1U << 0U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_SUPPORT = 1U << 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE = 1U << 2U;

struct telemetry_combat_actor_ref
{
	telemetry_id actor_id;
	telemetry_pid actor_pid;
	telemetry_subject_id owner_subject_id;
	telemetry_combat_actor_kind kind;
	std::uint8_t reserved[3];
	std::uint16_t power_band;
};

struct telemetry_battle_id
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
};

struct telemetry_battle_actor_key
{
	telemetry_id id;
	telemetry_combat_actor_kind kind;
};

struct telemetry_battle_actor_context
{
	telemetry_combat_actor_ref actor;
	telemetry_encounter_id encounter;
	telemetry_session_id session;
	telemetry_dimensions dimensions;
	telemetry_id group_key;
	std::uint16_t group_revision;
	std::uint16_t context_version;
	telemetry_quality_mask quality_flags;
};

enum class telemetry_battle_relation : std::uint8_t
{
	hostile = 1,
	support = 2,
	group_presence = 3,
};

enum class telemetry_battle_side_status : std::uint8_t
{
	qualified_observed_graph = 1,
	ambiguous = 2,
	partial = 3,
};

enum class telemetry_battle_fact_kind : std::uint8_t
{
	start = 1,
	actor_context = 2,
	relation = 3,
	cut = 4,
	merge_alias = 5,
	actor_summary = 6,
	close = 7,
};

enum class telemetry_battle_close_reason : std::uint8_t
{
	unknown = 0,
	inactivity = 1,
	copyover = 2,
	shutdown = 3,
};

/* Per-actor presence is distinct from input-derived activity, unique owner
 * characters, controllers and canonical reward-credit membership. */
struct telemetry_battle_effort
{
	telemetry_duration_usec present_usec;
	telemetry_duration_usec contributor_usec;
	telemetry_duration_usec pve_usec;
	telemetry_duration_usec pvp_usec;
	telemetry_duration_usec mixed_usec;
	telemetry_duration_usec unknown_mode_usec;
	telemetry_duration_usec outnumbered_owner_usec;
	telemetry_duration_usec unknown_side_usec;
};

struct telemetry_battle_fact
{
	telemetry_battle_id battle;
	telemetry_battle_id related_battle;
	telemetry_encounter_source scope;
	telemetry_revision revision;
	std::uint32_t fact_sequence;
	std::uint16_t fact_index;
	std::uint16_t fact_count;
	std::uint16_t definition_version;
	telemetry_battle_fact_kind kind;
	telemetry_battle_relation relation;
	telemetry_battle_side_status side_status;
	telemetry_encounter_mode mode;
	telemetry_battle_close_reason close_reason;
	std::uint8_t active;
	std::uint8_t side;
	std::uint8_t end_censored;
	std::uint16_t roles;
	std::uint16_t actor_count;
	std::uint16_t active_actor_count;
	std::uint16_t observed_owner_count;
	std::uint32_t dropped_actor_count;
	telemetry_battle_actor_context actor;
	telemetry_battle_actor_key related_actor;
	telemetry_battle_effort effort;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_monotonic_usec observed_through_monotonic_usec;
	telemetry_monotonic_usec last_engagement_monotonic_usec;
	telemetry_duration_usec inactivity_grace_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_utc_usec observed_through_utc_usec;
	telemetry_quality_mask quality_flags;
};

union telemetry_record_payload
{
	telemetry_interval_payload interval;
	telemetry_session_lifecycle_payload lifecycle;
	telemetry_session_checkpoint_payload checkpoint;
	telemetry_coverage_gap_payload gap;
	telemetry_configuration_payload configuration;
	telemetry_progression_payload progression;
	telemetry_encounter_payload encounter;
	telemetry_combat_summary_payload combat_summary;
	telemetry_ownership_payload ownership;
};

/* Fixed-size tagged value.  The active payload is selected by header.kind. */
struct telemetry_record
{
	telemetry_record_header header;
	telemetry_record_payload payload;
};

/* Common health is a copied snapshot; counters are monotonic/saturating. */
struct telemetry_health_snapshot
{
	telemetry_health_state state;
	telemetry_storage_backend backend;
	telemetry_disabled_reason disabled_reason;
	telemetry_failure_class last_failure_class;
	std::uint16_t schema_version;
	std::uint16_t reserved2;
	std::uint32_t last_error_code;
	std::uint32_t queue_capacity;
	telemetry_producer_id producer;
	telemetry_record_sequence last_admitted_record_seq;
	telemetry_record_sequence last_committed_record_seq;
	telemetry_record_sequence inflight_first_record_seq;
	telemetry_record_sequence inflight_last_record_seq;
	std::uint64_t inflight_record_kind_mask;
	telemetry_duration_usec retry_backoff_remaining_usec;
	std::uint64_t queue_depth;
	std::uint64_t queue_high_water;
	std::uint64_t admitted_detail;
	std::uint64_t admitted_control;
	std::uint64_t dropped_detail;
	std::uint64_t dropped_control;
	std::uint64_t applied_records;
	std::uint64_t duplicate_records;
	std::uint64_t stale_checkpoint_records;
	std::uint64_t invalid_records;
	std::uint64_t conflict_records;
	std::uint64_t retryable_failures;
	std::uint64_t ambiguous_commits;
	std::uint64_t sequence_gap_count;
	std::uint64_t unclosed_tail_count;
	telemetry_duration_usec attributable_duration_usec;
	telemetry_duration_usec unknown_duration_usec;
	telemetry_monotonic_usec last_success_monotonic_usec;
	telemetry_monotonic_usec last_failure_monotonic_usec;
	telemetry_producer_id last_failure_producer;
	telemetry_record_sequence last_failure_first_record_seq;
	telemetry_record_sequence last_failure_last_record_seq;
	std::uint64_t last_failure_record_kind_mask;
	std::uint64_t quarantined_records;
	std::uint64_t circuit_open_count;
	std::uint32_t last_failure_retry_attempts;
	std::uint32_t inflight_retry_attempts;
	std::uint32_t repository_retry_attempts;
	telemetry_advisory_lock_state advisory_lock_state;
	std::uint8_t inflight_active;
	std::uint8_t reserved4[2];
};

constexpr bool telemetry_record_kind_is_valid(telemetry_record_kind kind) noexcept
{
	return kind == telemetry_record_kind::interval ||
	       kind == telemetry_record_kind::session_lifecycle ||
	       kind == telemetry_record_kind::session_checkpoint ||
	       kind == telemetry_record_kind::coverage_gap ||
	       kind == telemetry_record_kind::configuration ||
	       kind == telemetry_record_kind::progression ||
	       kind == telemetry_record_kind::encounter ||
	       kind == telemetry_record_kind::combat_summary ||
	       kind == telemetry_record_kind::ownership;
}

constexpr bool telemetry_record_kind_is_control(telemetry_record_kind kind) noexcept
{
	return kind == telemetry_record_kind::session_lifecycle ||
	       kind == telemetry_record_kind::session_checkpoint ||
	       kind == telemetry_record_kind::coverage_gap ||
	       kind == telemetry_record_kind::configuration ||
	       kind == telemetry_record_kind::encounter ||
	       kind == telemetry_record_kind::combat_summary ||
	       kind == telemetry_record_kind::ownership;
}

constexpr bool telemetry_lifecycle_kind_is_valid(telemetry_lifecycle_kind kind) noexcept
{
	return kind == telemetry_lifecycle_kind::session_entered ||
	       kind == telemetry_lifecycle_kind::session_exited ||
	       kind == telemetry_lifecycle_kind::connection_attached ||
	       kind == telemetry_lifecycle_kind::connection_detached;
}

constexpr bool telemetry_session_end_reason_is_valid(telemetry_session_end_reason reason) noexcept
{
	return reason == telemetry_session_end_reason::unknown ||
	       reason == telemetry_session_end_reason::logout ||
	       reason == telemetry_session_end_reason::disconnect ||
	       reason == telemetry_session_end_reason::shutdown ||
	       reason == telemetry_session_end_reason::copyover ||
	       reason == telemetry_session_end_reason::process_restart;
}

constexpr bool
telemetry_connection_transition_kind_is_valid(telemetry_connection_transition_kind kind) noexcept
{
	return kind == telemetry_connection_transition_kind::attached ||
	       kind == telemetry_connection_transition_kind::detached ||
	       kind == telemetry_connection_transition_kind::copyover_resumed;
}

constexpr bool telemetry_interval_category_is_valid(telemetry_interval_category category) noexcept
{
	return category == telemetry_interval_category::unknown ||
	       category == telemetry_interval_category::connected_idle ||
	       category == telemetry_interval_category::connected_active ||
	       category == telemetry_interval_category::resident_linkdead;
}

constexpr bool telemetry_activity_context_is_valid(telemetry_activity_context context) noexcept
{
	return context == telemetry_activity_context::unknown ||
	       context == telemetry_activity_context::none ||
	       context == telemetry_activity_context::combat ||
	       context == telemetry_activity_context::travel ||
	       context == telemetry_activity_context::social ||
	       context == telemetry_activity_context::crafting ||
	       context == telemetry_activity_context::administration ||
	       context == telemetry_activity_context::other ||
	       context == telemetry_activity_context::overflow_unknown;
}

constexpr bool telemetry_context_quality_is_valid(telemetry_context_quality quality) noexcept
{
	return quality == telemetry_context_quality::unknown ||
	       quality == telemetry_context_quality::observed ||
	       quality == telemetry_context_quality::partial ||
	       quality == telemetry_context_quality::overflow ||
	       quality == telemetry_context_quality::unavailable;
}

constexpr bool telemetry_gap_reason_is_valid(telemetry_gap_reason reason) noexcept
{
	return reason == telemetry_gap_reason::detail_queue_drop ||
	       reason == telemetry_gap_reason::control_queue_drop ||
	       reason == telemetry_gap_reason::sequence_gap ||
	       reason == telemetry_gap_reason::telemetry_disabled ||
	       reason == telemetry_gap_reason::unclosed_tail ||
	       reason == telemetry_gap_reason::clock_discontinuity;
}

constexpr bool telemetry_storage_backend_is_valid(telemetry_storage_backend backend) noexcept
{
	return backend == telemetry_storage_backend::sql ||
	       backend == telemetry_storage_backend::flatfile_disabled;
}

constexpr bool telemetry_progression_kind_is_valid(telemetry_progression_kind kind) noexcept
{
	return kind == telemetry_progression_kind::experience_observed ||
	       kind == telemetry_progression_kind::level_advanced ||
	       kind == telemetry_progression_kind::level_lost;
}

constexpr bool telemetry_progression_source_is_valid(telemetry_progression_source source) noexcept
{
	return source == telemetry_progression_source::unknown ||
	       source == telemetry_progression_source::damage ||
	       source == telemetry_progression_source::healing ||
	       source == telemetry_progression_source::kill ||
	       source == telemetry_progression_source::death ||
	       source == telemetry_progression_source::quest ||
	       source == telemetry_progression_source::resurrect ||
	       source == telemetry_progression_source::melee ||
	       source == telemetry_progression_source::world_quest ||
	       source == telemetry_progression_source::tanking ||
	       source == telemetry_progression_source::boon ||
	       source == telemetry_progression_source::administration ||
	       source == telemetry_progression_source::system;
}

constexpr bool telemetry_progression_reason_is_valid(telemetry_progression_reason reason) noexcept
{
	return reason == telemetry_progression_reason::unknown ||
	       reason == telemetry_progression_reason::earned ||
	       reason == telemetry_progression_reason::death_loss ||
	       reason == telemetry_progression_reason::resurrection ||
	       reason == telemetry_progression_reason::level_threshold ||
	       reason == telemetry_progression_reason::administration ||
	       reason == telemetry_progression_reason::system_adjustment;
}

constexpr bool telemetry_progression_observation_status_is_valid(
	telemetry_progression_observation_status status) noexcept
{
	return status == telemetry_progression_observation_status::observed_mutable ||
	       status == telemetry_progression_observation_status::recovered_checkpoint ||
	       status == telemetry_progression_observation_status::durable_reconciled;
}

constexpr bool telemetry_progression_modifier_flags_are_valid(std::uint32_t flags) noexcept
{
	return (flags & ~TELEMETRY_PROGRESSION_MODIFIER_KNOWN) == 0U;
}

inline constexpr telemetry_quality_mask TELEMETRY_QUALITY_KNOWN =
	TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_CONTEXT_OVERFLOW |
	TELEMETRY_QUALITY_DIMENSION_UNKNOWN | TELEMETRY_QUALITY_SEQUENCE_GAP |
	TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_DISABLED |
	TELEMETRY_QUALITY_UNCLOSED_TAIL | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY |
	TELEMETRY_QUALITY_LATE | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW;

constexpr bool telemetry_quality_mask_is_valid(telemetry_quality_mask quality) noexcept
{
	return (quality & ~TELEMETRY_QUALITY_KNOWN) == 0U;
}

constexpr bool
telemetry_ownership_handoff_is_zero(const telemetry_ownership_handoff &value) noexcept
{
	return value.account_token == 0U && value.observed_at_utc_usec == 0 &&
	       value.quality_flags == 0U && static_cast<std::uint8_t>(value.source) == 0U &&
	       value.reserved[0] == 0U && value.reserved[1] == 0U && value.reserved[2] == 0U;
}

constexpr bool
telemetry_ownership_handoff_is_valid(const telemetry_ownership_handoff &value) noexcept
{
	if (telemetry_ownership_handoff_is_zero(value))
		return true;
	if (!telemetry_quality_mask_is_valid(value.quality_flags) || value.reserved[0] != 0U ||
	    value.reserved[1] != 0U || value.reserved[2] != 0U)
		return false;
	if (value.source == telemetry_ownership_source::unavailable)
		return value.account_token == 0U;
	return value.account_token != 0U &&
	       (value.source == telemetry_ownership_source::authenticated_login ||
		value.source == telemetry_ownership_source::reconnect ||
		value.source == telemetry_ownership_source::copyover ||
		value.source == telemetry_ownership_source::ownership_changed);
}

constexpr bool telemetry_producer_id_is_zero(const telemetry_producer_id &id) noexcept
{
	return id.boot_id == TELEMETRY_UNKNOWN_ID && id.process_id == TELEMETRY_UNKNOWN_ID;
}

constexpr bool telemetry_producer_id_is_valid(const telemetry_producer_id &id) noexcept
{
	return id.boot_id != TELEMETRY_UNKNOWN_ID && id.process_id != TELEMETRY_UNKNOWN_ID;
}

constexpr bool telemetry_encounter_mode_is_valid(telemetry_encounter_mode mode) noexcept
{
	return mode == telemetry_encounter_mode::unknown || mode == telemetry_encounter_mode::pve ||
	       mode == telemetry_encounter_mode::pvp || mode == telemetry_encounter_mode::mixed;
}

constexpr bool telemetry_encounter_event_kind_is_valid(telemetry_encounter_event_kind kind) noexcept
{
	return kind == telemetry_encounter_event_kind::start ||
	       kind == telemetry_encounter_event_kind::participant_join ||
	       kind == telemetry_encounter_event_kind::participant_leave ||
	       kind == telemetry_encounter_event_kind::close ||
	       kind == telemetry_encounter_event_kind::participant_summary;
}

constexpr bool telemetry_encounter_outcome_is_valid(telemetry_encounter_outcome outcome) noexcept
{
	return outcome == telemetry_encounter_outcome::unknown ||
	       outcome == telemetry_encounter_outcome::success ||
	       outcome == telemetry_encounter_outcome::failure ||
	       outcome == telemetry_encounter_outcome::death ||
	       outcome == telemetry_encounter_outcome::flee ||
	       outcome == telemetry_encounter_outcome::withdrawal ||
	       outcome == telemetry_encounter_outcome::abandonment ||
	       outcome == telemetry_encounter_outcome::timeout ||
	       outcome == telemetry_encounter_outcome::copyover ||
	       outcome == telemetry_encounter_outcome::shutdown ||
	       outcome == telemetry_encounter_outcome::unknown_close;
}

constexpr bool telemetry_combat_actor_kind_is_valid(telemetry_combat_actor_kind kind) noexcept
{
	return kind == telemetry_combat_actor_kind::player ||
	       kind == telemetry_combat_actor_kind::pet || kind == telemetry_combat_actor_kind::npc;
}

constexpr bool telemetry_combat_modifier_flags_are_valid(std::uint32_t flags) noexcept
{
	return (flags & ~TELEMETRY_COMBAT_MODIFIER_KNOWN) == 0U;
}

constexpr bool telemetry_encounter_id_is_valid(const telemetry_encounter_id &id) noexcept
{
	return telemetry_producer_id_is_valid(id.producer) && id.sequence != 0U;
}

constexpr bool
telemetry_encounter_source_is_valid(const telemetry_encounter_source &source) noexcept
{
	return source.environment_id != 0U && source.season_id != 0U && source.config_id != 0U &&
	       source.classifier_version != 0U && source.policy_version != 0U &&
	       source.zone_vnum >= -1;
}

constexpr bool telemetry_encounter_participant_is_valid(
	const telemetry_encounter_participant &participant) noexcept
{
	return participant.subject_id != 0U && participant.pid > 0;
}

constexpr bool telemetry_record_key_is_valid(const telemetry_record_key &key) noexcept
{
	return telemetry_producer_id_is_valid(key.producer) && key.record_seq != 0U;
}

constexpr bool telemetry_session_id_is_zero(const telemetry_session_id &id) noexcept
{
	return telemetry_producer_id_is_zero(id.producer) && id.session_seq == 0U;
}

constexpr bool telemetry_session_id_is_valid(const telemetry_session_id &id) noexcept
{
	return telemetry_producer_id_is_valid(id.producer) && id.session_seq != 0U;
}

constexpr bool telemetry_connection_id_is_zero(const telemetry_connection_id &id) noexcept
{
	return telemetry_producer_id_is_zero(id.producer) && id.connection_seq == 0U;
}

constexpr bool telemetry_connection_id_is_valid(const telemetry_connection_id &id) noexcept
{
	return telemetry_producer_id_is_valid(id.producer) && id.connection_seq != 0U;
}

/* A detached resident may carry no connection, but a partial id is invalid. */
constexpr bool telemetry_connection_reference_is_valid(const telemetry_connection_id &id) noexcept
{
	return telemetry_connection_id_is_zero(id) || telemetry_connection_id_is_valid(id);
}

constexpr bool telemetry_session_ref_is_zero(const telemetry_session_ref &ref) noexcept
{
	return telemetry_session_id_is_zero(ref.id) && ref.subject_id == 0U && ref.pid == 0 &&
	       ref.season_id == 0U && ref.environment_id == 0U;
}

constexpr bool telemetry_session_ref_is_valid(const telemetry_session_ref &ref) noexcept
{
	return telemetry_session_id_is_valid(ref.id) && ref.subject_id != TELEMETRY_UNKNOWN_ID &&
	       ref.pid > 0 && ref.season_id != TELEMETRY_UNKNOWN_ID &&
	       ref.environment_id != TELEMETRY_UNKNOWN_ID;
}

/* Presence/shape only. #263 hashes once; #261 verifies canonical SHA-256 on the
 * worker before materialization. Enqueue does not hash or certify authenticity. */
constexpr bool telemetry_config_fingerprint_is_valid(
	const std::uint8_t (&fingerprint)[TELEMETRY_CONFIG_FINGERPRINT_BYTES]) noexcept
{
	for (const std::uint8_t byte : fingerprint)
		if (byte != 0U)
			return true;
	return false;
}

inline constexpr std::uint32_t TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL = 64U;
inline constexpr std::uint16_t TELEMETRY_CONFIG_PULSE_SLOT_COUNT_MAX_PROPOSAL = 256U;
inline constexpr telemetry_duration_usec TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL = 3'600'000'000ULL;
inline constexpr telemetry_duration_usec TELEMETRY_CHECKPOINT_INTERVAL_USEC_MAX_PROPOSAL =
	3'600'000'000ULL;
inline constexpr telemetry_duration_usec TELEMETRY_ACTIVE_WINDOW_USEC_PROPOSAL = 300'000'000ULL;
inline constexpr telemetry_duration_usec TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL =
	3'600'000'000ULL;
/* Descriptive aliases make the config ownership of the one-hour cap clear. */
inline constexpr telemetry_duration_usec TELEMETRY_CONFIG_ACTIVE_WINDOW_USEC_MAX_PROPOSAL =
	TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL;

/* Validation distinguishes a healthy SQL config from intentional disablement. */
enum class telemetry_config_validation : std::uint8_t
{
	valid_sql = 0,
	valid_disabled = 1,
	valid_flatfile_disabled = 2,
	unsupported_schema = 3,
	missing_identity = 4,
	invalid_interval = 5,
	invalid_checkpoint_interval = 6,
	invalid_segment_cap = 7,
	invalid_pulse_slots = 8,
	invalid_backend = 9,
	invalid_reserved = 10,
	invalid_versions = 11,
	invalid_fingerprint = 12,
	invalid_active_window = 13,
};

constexpr bool telemetry_config_is_enabled(const telemetry_config_snapshot &config) noexcept
{
	return config.enabled == 1U;
}

constexpr telemetry_config_validation
telemetry_config_validate(const telemetry_config_snapshot &config) noexcept
{
	if (config.schema_version != TELEMETRY_SCHEMA_VERSION)
		return telemetry_config_validation::unsupported_schema;
	if (config.reserved != 0U)
		return telemetry_config_validation::invalid_reserved;
	if (config.config_id == TELEMETRY_UNKNOWN_ID || config.revision == 0U ||
	    config.season_id == TELEMETRY_UNKNOWN_ID ||
	    config.environment_id == TELEMETRY_UNKNOWN_ID)
		return telemetry_config_validation::missing_identity;
	if (config.build_version == 0U || config.content_version == 0U ||
	    config.property_version == 0U || config.classifier_version == 0U ||
	    config.policy_version == 0U)
		return telemetry_config_validation::invalid_versions;
	if (!telemetry_config_fingerprint_is_valid(config.fingerprint))
		return telemetry_config_validation::invalid_fingerprint;
	if (config.enabled > 1U)
		return telemetry_config_validation::invalid_backend;
	if (!telemetry_storage_backend_is_valid(config.backend))
		return telemetry_config_validation::invalid_backend;
	if (config.backend == telemetry_storage_backend::flatfile_disabled && config.enabled != 0U)
		return telemetry_config_validation::invalid_backend;
	/* Disabled configs may leave unused values at zero, but never out of range. */
	if (config.interval_usec > TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_interval;
	if (config.checkpoint_interval_usec > TELEMETRY_CHECKPOINT_INTERVAL_USEC_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_checkpoint_interval;
	if (config.active_window_usec > TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_active_window;
	if (config.context_segments_per_minute > TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_segment_cap;
	if (config.pulse_slot_count > TELEMETRY_CONFIG_PULSE_SLOT_COUNT_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_pulse_slots;
	if (config.enabled == 0U)
		return config.backend == telemetry_storage_backend::flatfile_disabled ?
			       telemetry_config_validation::valid_flatfile_disabled :
			       telemetry_config_validation::valid_disabled;
	if (config.interval_usec == 0U)
		return telemetry_config_validation::invalid_interval;
	if (config.checkpoint_interval_usec == 0U)
		return telemetry_config_validation::invalid_checkpoint_interval;
	if (config.active_window_usec == 0U ||
	    config.active_window_usec > TELEMETRY_ACTIVE_WINDOW_USEC_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_active_window;
	if (config.context_segments_per_minute == 0U ||
	    config.context_segments_per_minute > TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_segment_cap;
	if (config.pulse_slot_count == 0U ||
	    config.pulse_slot_count > TELEMETRY_CONFIG_PULSE_SLOT_COUNT_MAX_PROPOSAL)
		return telemetry_config_validation::invalid_pulse_slots;
	return telemetry_config_validation::valid_sql;
}

constexpr bool telemetry_config_is_valid(const telemetry_config_snapshot &config) noexcept
{
	const telemetry_config_validation result = telemetry_config_validate(config);
	return result == telemetry_config_validation::valid_sql ||
	       result == telemetry_config_validation::valid_disabled ||
	       result == telemetry_config_validation::valid_flatfile_disabled;
}

constexpr bool
telemetry_config_snapshot_identity_is_valid(const telemetry_config_snapshot &config) noexcept
{
	return config.schema_version == TELEMETRY_SCHEMA_VERSION && config.reserved == 0U &&
	       config.config_id != TELEMETRY_UNKNOWN_ID && config.revision != 0U &&
	       config.build_version != 0U && config.content_version != 0U &&
	       config.property_version != 0U && config.classifier_version != 0U &&
	       config.policy_version != 0U && config.season_id != TELEMETRY_UNKNOWN_ID &&
	       config.environment_id != TELEMETRY_UNKNOWN_ID &&
	       telemetry_config_fingerprint_is_valid(config.fingerprint) &&
	       telemetry_storage_backend_is_valid(config.backend) && config.enabled <= 1U;
}

/* All subtraction is preceded by an order check, so uint64_t cannot wrap. */
constexpr bool telemetry_duration_subtract_if_possible(telemetry_duration_usec minuend,
						       telemetry_duration_usec subtrahend,
						       telemetry_duration_usec &difference) noexcept
{
	if (subtrahend > minuend)
		return false;
	difference = minuend - subtrahend;
	return true;
}

constexpr bool telemetry_record_time_is_valid(const telemetry_time_window &window) noexcept
{
	return window.end_monotonic_usec > window.start_monotonic_usec;
}

constexpr bool telemetry_time_window_is_valid(const telemetry_time_window &window) noexcept
{
	return telemetry_record_time_is_valid(window);
}

constexpr bool telemetry_duration_matches_window(const telemetry_time_window &window,
						 telemetry_duration_usec duration) noexcept
{
	return duration != 0U && telemetry_record_time_is_valid(window) &&
	       window.end_monotonic_usec - window.start_monotonic_usec == duration;
}

constexpr bool
telemetry_cumulative_counters_are_valid(const telemetry_cumulative_counters &counters) noexcept
{
	telemetry_duration_usec remaining = 0U;
	if (!telemetry_duration_subtract_if_possible(counters.connected_usec, counters.active_usec,
						     remaining))
		return false;
	if (!telemetry_duration_subtract_if_possible(remaining, counters.idle_usec, remaining))
		return false;
	if (counters.unknown_usec != remaining)
		return false;
	if (!telemetry_duration_subtract_if_possible(counters.resident_usec,
						     counters.connected_usec, remaining))
		return false;
	return counters.linkdead_usec == remaining;
}

constexpr bool telemetry_record_header_is_valid(const telemetry_record_header &header) noexcept
{
	return header.schema_version == TELEMETRY_SCHEMA_VERSION && header.reserved == 0U &&
	       telemetry_record_kind_is_valid(header.kind) &&
	       telemetry_record_key_is_valid(header.key);
}

constexpr bool
telemetry_connection_transition_is_valid(const telemetry_connection_transition &transition) noexcept
{
	if (!telemetry_session_ref_is_valid(transition.session) ||
	    !telemetry_connection_id_is_valid(transition.connection) ||
	    !telemetry_connection_transition_kind_is_valid(transition.kind) ||
	    transition.reserved[0] != 0U || transition.reserved[1] != 0U ||
	    transition.reserved[2] != 0U ||
	    !telemetry_quality_mask_is_valid(transition.quality_flags))
		return false;
	if (transition.kind == telemetry_connection_transition_kind::copyover_resumed &&
	    transition.connection.producer.boot_id == transition.session.id.producer.boot_id &&
	    transition.connection.producer.process_id == transition.session.id.producer.process_id)
		return false;
	return true;
}

constexpr bool
telemetry_interval_connection_is_valid(const telemetry_interval_payload &interval) noexcept
{
	if (interval.category == telemetry_interval_category::resident_linkdead)
		return telemetry_connection_id_is_zero(interval.connection);
	return telemetry_connection_id_is_valid(interval.connection);
}

constexpr bool
telemetry_interval_payload_is_valid(const telemetry_interval_payload &interval) noexcept
{
	if (!telemetry_session_ref_is_valid(interval.session) ||
	    !telemetry_interval_connection_is_valid(interval) ||
	    !telemetry_duration_matches_window(interval.window, interval.duration_usec) ||
	    !telemetry_interval_category_is_valid(interval.category) ||
	    !telemetry_activity_context_is_valid(interval.context) ||
	    !telemetry_context_quality_is_valid(interval.context_quality) ||
	    interval.reserved != 0U || !telemetry_dimensions_are_valid(interval.dimensions) ||
	    interval.config_id == TELEMETRY_UNKNOWN_ID || interval.classifier_version == 0U ||
	    interval.policy_version == 0U ||
	    !telemetry_quality_mask_is_valid(interval.quality_flags))
		return false;
	if (interval.context == telemetry_activity_context::overflow_unknown)
		return telemetry_dimensions_are_unknown(interval.dimensions) &&
		       interval.context_quality == telemetry_context_quality::overflow &&
		       (interval.quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW) != 0U &&
		       (interval.quality_flags & TELEMETRY_QUALITY_DIMENSION_UNKNOWN) != 0U;
	return true;
}

constexpr bool telemetry_session_lifecycle_payload_is_valid(
	const telemetry_session_lifecycle_payload &lifecycle) noexcept
{
	if (!telemetry_session_ref_is_valid(lifecycle.session) ||
	    !telemetry_lifecycle_kind_is_valid(lifecycle.lifecycle) ||
	    !telemetry_session_end_reason_is_valid(lifecycle.end_reason) ||
	    lifecycle.reserved != 0U || !telemetry_dimensions_are_valid(lifecycle.dimensions) ||
	    lifecycle.config_id == TELEMETRY_UNKNOWN_ID || lifecycle.classifier_version == 0U ||
	    lifecycle.policy_version == 0U ||
	    !telemetry_quality_mask_is_valid(lifecycle.quality_flags))
		return false;
	if (lifecycle.lifecycle == telemetry_lifecycle_kind::connection_attached ||
	    lifecycle.lifecycle == telemetry_lifecycle_kind::connection_detached)
		return telemetry_connection_id_is_valid(lifecycle.connection) &&
		       lifecycle.end_reason == telemetry_session_end_reason::unknown;
	if (lifecycle.lifecycle == telemetry_lifecycle_kind::session_entered)
		return telemetry_connection_id_is_valid(lifecycle.connection) &&
		       lifecycle.end_reason == telemetry_session_end_reason::unknown;
	return telemetry_connection_reference_is_valid(lifecycle.connection) &&
	       lifecycle.end_reason != telemetry_session_end_reason::unknown;
}

constexpr bool telemetry_session_checkpoint_payload_is_valid(
	const telemetry_session_checkpoint_payload &checkpoint) noexcept
{
	return telemetry_session_ref_is_valid(checkpoint.session) &&
	       telemetry_connection_reference_is_valid(checkpoint.connection) &&
	       checkpoint.revision != 0U &&
	       telemetry_cumulative_counters_are_valid(checkpoint.cumulative) &&
	       checkpoint.config_id != TELEMETRY_UNKNOWN_ID &&
	       telemetry_quality_mask_is_valid(checkpoint.quality_flags);
}

/* Compare a signed storage delta without ever overflowing signed arithmetic. */
constexpr bool telemetry_signed_delta_is_valid(std::int64_t before, std::int64_t after,
					       std::int64_t applied) noexcept
{
	if (after >= before)
		return applied >= 0 && static_cast<std::uint64_t>(applied) ==
					       static_cast<std::uint64_t>(after) -
						       static_cast<std::uint64_t>(before);
	return applied < 0 &&
	       static_cast<std::uint64_t>(0U) - static_cast<std::uint64_t>(applied) ==
		       static_cast<std::uint64_t>(before) - static_cast<std::uint64_t>(after);
}

constexpr bool
telemetry_progression_payload_is_valid(const telemetry_progression_payload &progression) noexcept
{
	if (!telemetry_session_ref_is_valid(progression.session) ||
	    !telemetry_connection_reference_is_valid(progression.connection) ||
	    !telemetry_progression_kind_is_valid(progression.kind) ||
	    !telemetry_progression_source_is_valid(progression.source) ||
	    !telemetry_progression_reason_is_valid(progression.reason) ||
	    !telemetry_progression_observation_status_is_valid(progression.observation_status) ||
	    !telemetry_progression_modifier_flags_are_valid(progression.modifier_flags) ||
	    progression.reserved != 0U || !telemetry_dimensions_are_valid(progression.dimensions) ||
	    progression.config_id == TELEMETRY_UNKNOWN_ID || progression.classifier_version == 0U ||
	    progression.policy_version == 0U ||
	    !telemetry_quality_mask_is_valid(progression.quality_flags))
		return false;
	if (progression.kind == telemetry_progression_kind::experience_observed)
	{
		if (progression.before_level != progression.after_level ||
		    progression.threshold_xp != 0U)
			return false;
		/* The storage-boundary delta is the only authoritative observed change. */
		return telemetry_signed_delta_is_valid(
			progression.before_exp, progression.after_exp, progression.applied_xp);
	}
	if (progression.before_level == progression.after_level || progression.applied_xp != 0U ||
	    progression.requested_xp != 0 || progression.computed_xp != 0 ||
	    progression.before_exp != 0 || progression.after_exp != 0)
		return false;
	if (progression.kind == telemetry_progression_kind::level_advanced)
		return progression.after_level > progression.before_level;
	return progression.kind == telemetry_progression_kind::level_lost &&
	       progression.after_level < progression.before_level;
}

constexpr bool
telemetry_coverage_gap_payload_is_valid(const telemetry_coverage_gap_payload &gap) noexcept
{
	if ((!telemetry_session_ref_is_zero(gap.session) &&
	     !telemetry_session_ref_is_valid(gap.session)) ||
	    (!telemetry_connection_id_is_zero(gap.connection) &&
	     !telemetry_connection_id_is_valid(gap.connection)) ||
	    !telemetry_gap_reason_is_valid(gap.reason) || gap.reserved[0] != 0U ||
	    gap.reserved[1] != 0U || gap.reserved[2] != 0U ||
	    !telemetry_quality_mask_is_valid(gap.quality_flags) ||
	    gap.end_monotonic_usec < gap.start_monotonic_usec)
		return false;
	if ((gap.first_missing_record_seq == 0U) != (gap.last_missing_record_seq == 0U) ||
	    gap.last_missing_record_seq < gap.first_missing_record_seq)
		return false;
	if (gap.duration_usec == 0U)
		return true;
	return gap.end_monotonic_usec > gap.start_monotonic_usec &&
	       gap.end_monotonic_usec - gap.start_monotonic_usec == gap.duration_usec;
}

constexpr bool telemetry_configuration_payload_is_valid(
	const telemetry_configuration_payload &configuration) noexcept
{
	return telemetry_config_is_valid(configuration.config);
}

constexpr bool
telemetry_encounter_payload_is_valid(const telemetry_encounter_payload &encounter) noexcept
{
	if (!telemetry_encounter_id_is_valid(encounter.encounter) ||
	    !telemetry_encounter_event_kind_is_valid(encounter.kind) ||
	    !telemetry_encounter_mode_is_valid(encounter.mode) ||
	    !telemetry_encounter_outcome_is_valid(encounter.outcome) || encounter.reserved != 0U ||
	    encounter.revision == 0U || !telemetry_encounter_source_is_valid(encounter.source) ||
	    encounter.source.zone_vnum < -1 ||
	    !telemetry_quality_mask_is_valid(encounter.quality_flags) ||
	    encounter.at_monotonic_usec < encounter.start_monotonic_usec)
		return false;
	const bool participant_event =
		encounter.kind == telemetry_encounter_event_kind::participant_join ||
		encounter.kind == telemetry_encounter_event_kind::participant_leave ||
		encounter.kind == telemetry_encounter_event_kind::participant_summary;
	if (participant_event != telemetry_encounter_participant_is_valid(encounter.participant))
		return false;
	if (encounter.kind == telemetry_encounter_event_kind::start &&
	    encounter.outcome != telemetry_encounter_outcome::unknown)
		return false;
	if (encounter.kind == telemetry_encounter_event_kind::participant_leave &&
	    encounter.outcome == telemetry_encounter_outcome::unknown)
		return false;
	if ((encounter.kind == telemetry_encounter_event_kind::close ||
	     encounter.kind == telemetry_encounter_event_kind::participant_summary) &&
	    encounter.outcome == telemetry_encounter_outcome::unknown)
		return false;
	if (encounter.kind == telemetry_encounter_event_kind::close)
	{
		if (encounter.participant.subject_id != 0U || encounter.participant.pid != 0 ||
		    encounter.elapsed_usec !=
			    encounter.at_monotonic_usec - encounter.start_monotonic_usec)
			return false;
	}
	else if (encounter.kind == telemetry_encounter_event_kind::start ||
		 encounter.kind == telemetry_encounter_event_kind::participant_join)
	{
		if (encounter.elapsed_usec != 0U || encounter.participant_usec != 0U)
			return false;
	}
	return true;
}

constexpr bool
telemetry_combat_summary_payload_is_valid(const telemetry_combat_summary_payload &summary) noexcept
{
	if (!telemetry_encounter_id_is_valid(summary.encounter) ||
	    !telemetry_encounter_source_is_valid(summary.source) ||
	    !telemetry_encounter_mode_is_valid(summary.mode) ||
	    summary.mode == telemetry_encounter_mode::unknown ||
	    !telemetry_encounter_outcome_is_valid(summary.outcome) ||
	    summary.outcome == telemetry_encounter_outcome::unknown ||
	    !telemetry_combat_actor_kind_is_valid(summary.actor_kind) || summary.reserved != 0U ||
	    summary.revision == 0U || summary.actor_id == TELEMETRY_UNKNOWN_ID ||
	    !telemetry_combat_modifier_flags_are_valid(summary.modifier_flags) ||
	    !telemetry_quality_mask_is_valid(summary.quality_flags) ||
	    summary.source.zone_vnum < -1 ||
	    summary.end_monotonic_usec < summary.start_monotonic_usec ||
	    summary.unique_player_count > TELEMETRY_COMBAT_SUMMARY_MAX_UNIQUE_PLAYERS ||
	    summary.participant_count > TELEMETRY_COMBAT_SUMMARY_MAX_PARTICIPANTS ||
	    summary.effective_healing > summary.healing_attempted ||
	    summary.casting_completions > summary.casting_attempts ||
	    summary.casting_aborts > summary.casting_attempts - summary.casting_completions)
		return false;
	if (summary.actor_kind == telemetry_combat_actor_kind::player)
		return summary.actor_pid > 0 && summary.owner_subject_id == summary.actor_id;
	if (summary.actor_kind == telemetry_combat_actor_kind::pet)
		return summary.actor_pid == TELEMETRY_UNKNOWN_PID && summary.owner_subject_id != 0U;
	return summary.actor_pid == TELEMETRY_UNKNOWN_PID && summary.owner_subject_id == 0U;
}

constexpr bool
telemetry_ownership_payload_is_valid(const telemetry_ownership_payload &value) noexcept
{
	if (!telemetry_session_ref_is_valid(value.session) ||
	    !telemetry_connection_id_is_valid(value.connection) ||
	    !telemetry_dimensions_are_valid(value.dimensions) || !value.config_id ||
	    !value.classifier_version || !value.policy_version ||
	    !telemetry_quality_mask_is_valid(value.quality_flags))
		return false;
	for (auto byte : value.reserved)
		if (byte != 0U)
			return false;
	if (value.source == telemetry_ownership_source::unavailable)
		return value.account_token == TELEMETRY_UNKNOWN_ID;
	return value.account_token != TELEMETRY_UNKNOWN_ID &&
	       (value.source == telemetry_ownership_source::authenticated_login ||
		value.source == telemetry_ownership_source::reconnect ||
		value.source == telemetry_ownership_source::copyover ||
		value.source == telemetry_ownership_source::ownership_changed);
}

constexpr bool telemetry_combat_actor_ref_is_valid(const telemetry_combat_actor_ref &actor) noexcept
{
	if (!telemetry_combat_actor_kind_is_valid(actor.kind) || actor.actor_id == 0U ||
	    actor.reserved[0] != 0U || actor.reserved[1] != 0U || actor.reserved[2] != 0U)
		return false;
	if (actor.kind == telemetry_combat_actor_kind::player)
		return actor.actor_pid > 0 && actor.owner_subject_id == actor.actor_id;
	if (actor.kind == telemetry_combat_actor_kind::pet)
		return actor.actor_pid == TELEMETRY_UNKNOWN_PID && actor.owner_subject_id != 0U;
	return actor.actor_pid == TELEMETRY_UNKNOWN_PID && actor.owner_subject_id == 0U;
}

constexpr bool
telemetry_battle_actor_context_is_valid(const telemetry_battle_actor_context &context) noexcept
{
	if (!telemetry_combat_actor_ref_is_valid(context.actor) ||
	    !telemetry_quality_mask_is_valid(context.quality_flags) ||
	    !telemetry_dimensions_are_valid(context.dimensions) ||
	    context.actor.owner_subject_id >
		    static_cast<telemetry_subject_id>(std::numeric_limits<telemetry_pid>::max()) ||
	    context.context_version != TELEMETRY_BATTLE_ACTOR_CONTEXT_VERSION)
		return false;
	if (context.actor.kind == telemetry_combat_actor_kind::player)
	{
		if (context.actor.actor_id != static_cast<telemetry_id>(context.actor.actor_pid))
			return false;
	}
	else if ((context.actor.actor_id & TELEMETRY_BATTLE_NPC_GENERATION_TAG) == 0U ||
		 (context.actor.actor_id & ~TELEMETRY_BATTLE_NPC_GENERATION_TAG) == 0U)
		return false;
	if ((context.group_key & TELEMETRY_GROUP_GENERATION_TAG) != 0U &&
	    ((context.group_key & ~TELEMETRY_GROUP_GENERATION_TAG) == 0U ||
	     context.group_revision == 0U))
		return false;
	if ((context.group_key & TELEMETRY_GROUP_GENERATION_TAG) == 0U &&
	    context.group_revision != 0U)
		return false;
	const bool empty_encounter = context.encounter.producer.boot_id == 0U &&
				     context.encounter.producer.process_id == 0U &&
				     context.encounter.sequence == 0U;
	const bool empty_session = context.session.producer.boot_id == 0U &&
				   context.session.producer.process_id == 0U &&
				   context.session.session_seq == 0U;
	return (empty_encounter || telemetry_encounter_id_is_valid(context.encounter)) &&
	       (empty_session || (context.actor.kind == telemetry_combat_actor_kind::player &&
				  telemetry_session_id_is_valid(context.session)));
}

constexpr bool telemetry_battle_id_is_zero(telemetry_battle_id id) noexcept
{
	return id.producer.boot_id == 0U && id.producer.process_id == 0U && id.sequence == 0U;
}

constexpr bool telemetry_battle_id_is_valid(telemetry_battle_id id) noexcept
{
	return telemetry_producer_id_is_valid(id.producer) && id.sequence != 0U;
}

constexpr bool telemetry_battle_actor_key_is_valid(telemetry_battle_actor_key actor) noexcept
{
	if (!telemetry_combat_actor_kind_is_valid(actor.kind))
		return false;
	if (actor.kind == telemetry_combat_actor_kind::player)
		return actor.id > 0U &&
		       actor.id <=
			       static_cast<telemetry_id>(std::numeric_limits<telemetry_pid>::max());
	return (actor.id & TELEMETRY_BATTLE_NPC_GENERATION_TAG) != 0U &&
	       (actor.id & ~TELEMETRY_BATTLE_NPC_GENERATION_TAG) != 0U;
}

constexpr bool
telemetry_battle_actor_context_is_zero(const telemetry_battle_actor_context &context) noexcept
{
	return context.actor.actor_id == 0U && context.actor.actor_pid == 0 &&
	       context.actor.owner_subject_id == 0U &&
	       static_cast<std::uint8_t>(context.actor.kind) == 0U &&
	       context.actor.reserved[0] == 0U && context.actor.reserved[1] == 0U &&
	       context.actor.reserved[2] == 0U && context.actor.power_band == 0U &&
	       context.encounter.producer.boot_id == 0U &&
	       context.encounter.producer.process_id == 0U && context.encounter.sequence == 0U &&
	       context.session.producer.boot_id == 0U &&
	       context.session.producer.process_id == 0U && context.session.session_seq == 0U &&
	       context.dimensions.level_band == 0U && context.dimensions.class_id == 0U &&
	       context.dimensions.race_id == 0U && context.dimensions.faction_id == 0U &&
	       context.dimensions.zone_vnum == 0 && context.dimensions.group_size == 0U &&
	       context.group_key == 0U && context.group_revision == 0U &&
	       context.context_version == 0U && context.quality_flags == 0U;
}

constexpr bool telemetry_battle_effort_is_valid(const telemetry_battle_effort &effort) noexcept
{
	telemetry_duration_usec total = 0U;
	const telemetry_duration_usec modes[] = { effort.pve_usec, effort.pvp_usec,
						  effort.mixed_usec, effort.unknown_mode_usec };
	for (auto value : modes)
	{
		if (value > std::numeric_limits<telemetry_duration_usec>::max() - total)
			return false;
		total += value;
	}
	return total == effort.present_usec && effort.contributor_usec <= effort.present_usec &&
	       effort.outnumbered_owner_usec <= effort.pvp_usec &&
	       effort.unknown_side_usec <= effort.present_usec;
}

/* One intrinsic fact is not proof that its mutation packet or battle is complete.
 * Complete-packet validation and source/linkage coverage remain mandatory. */
constexpr bool telemetry_battle_fact_is_valid(const telemetry_battle_fact &fact) noexcept
{
	if (!telemetry_battle_id_is_valid(fact.battle) ||
	    fact.definition_version != TELEMETRY_BATTLE_DEFINITION_VERSION ||
	    !telemetry_encounter_source_is_valid(fact.scope) || fact.scope.group_key != 0U ||
	    fact.scope.zone_vnum != -1 || fact.revision == 0U || fact.fact_sequence == 0U ||
	    fact.fact_count == 0U || fact.fact_index >= fact.fact_count ||
	    fact.fact_sequence <= fact.fact_index ||
	    fact.kind < telemetry_battle_fact_kind::start ||
	    fact.kind > telemetry_battle_fact_kind::close ||
	    fact.side_status < telemetry_battle_side_status::qualified_observed_graph ||
	    fact.side_status > telemetry_battle_side_status::partial ||
	    !telemetry_encounter_mode_is_valid(fact.mode) ||
	    !telemetry_quality_mask_is_valid(fact.quality_flags) || fact.actor_count < 2U ||
	    fact.actor_count > TELEMETRY_BATTLE_MAX_ACTORS ||
	    fact.active_actor_count > fact.actor_count ||
	    fact.observed_owner_count > fact.active_actor_count ||
	    fact.last_engagement_monotonic_usec < fact.start_monotonic_usec ||
	    fact.observed_through_monotonic_usec < fact.last_engagement_monotonic_usec ||
	    fact.at_monotonic_usec < fact.observed_through_monotonic_usec ||
	    fact.inactivity_grace_usec == 0U || fact.active > 1U || fact.side > 2U ||
	    fact.end_censored > 1U ||
	    (fact.roles & ~(TELEMETRY_BATTLE_ROLE_COMBAT | TELEMETRY_BATTLE_ROLE_SUPPORT |
			    TELEMETRY_BATTLE_ROLE_GROUP_PRESENCE)) != 0U ||
	    !telemetry_battle_effort_is_valid(fact.effort) ||
	    fact.effort.present_usec >
		    fact.observed_through_monotonic_usec - fact.start_monotonic_usec)
		return false;
	const bool terminal = fact.kind == telemetry_battle_fact_kind::actor_summary ||
			      fact.kind == telemetry_battle_fact_kind::close;
	if (terminal)
	{
		if (fact.close_reason < telemetry_battle_close_reason::inactivity ||
		    fact.close_reason > telemetry_battle_close_reason::shutdown ||
		    fact.end_censored != 1U ||
		    (fact.quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) == 0U ||
		    fact.fact_count != fact.actor_count + 1U ||
		    (fact.kind == telemetry_battle_fact_kind::close ?
			     fact.fact_index != fact.actor_count :
			     fact.fact_index >= fact.actor_count))
			return false;
	}
	else if (fact.close_reason != telemetry_battle_close_reason::unknown ||
		 fact.end_censored != 0U ||
		 fact.fact_count > TELEMETRY_BATTLE_MAX_FACTS_PER_MUTATION)
		return false;
	const bool actor_fact = fact.kind == telemetry_battle_fact_kind::actor_context ||
				fact.kind == telemetry_battle_fact_kind::relation ||
				fact.kind == telemetry_battle_fact_kind::actor_summary;
	if (actor_fact)
	{
		if (!telemetry_battle_actor_context_is_valid(fact.actor) || fact.roles == 0U ||
		    (fact.actor.quality_flags & ~fact.quality_flags) != 0U ||
		    (!fact.active && fact.side != 0U) ||
		    (fact.side_status != telemetry_battle_side_status::qualified_observed_graph &&
		     fact.side != 0U) ||
		    (fact.actor.encounter.sequence != 0U &&
		     (fact.actor.encounter.producer.boot_id != fact.battle.producer.boot_id ||
		      fact.actor.encounter.producer.process_id !=
			      fact.battle.producer.process_id)) ||
		    ((fact.roles &
		      (TELEMETRY_BATTLE_ROLE_COMBAT | TELEMETRY_BATTLE_ROLE_SUPPORT)) == 0U &&
		     fact.effort.contributor_usec != 0U))
			return false;
	}
	else if (!telemetry_battle_actor_context_is_zero(fact.actor) || fact.roles != 0U ||
		 fact.active != 0U || fact.side != 0U || fact.effort.present_usec != 0U)
		return false;
	const bool alias = fact.kind == telemetry_battle_fact_kind::merge_alias;
	if (alias)
	{
		if (!telemetry_battle_id_is_valid(fact.related_battle) ||
		    fact.related_battle.producer.boot_id != fact.battle.producer.boot_id ||
		    fact.related_battle.producer.process_id != fact.battle.producer.process_id ||
		    fact.related_battle.sequence <= fact.battle.sequence || fact.fact_index != 0U)
			return false;
	}
	else if (!telemetry_battle_id_is_zero(fact.related_battle))
		return false;
	if (fact.kind == telemetry_battle_fact_kind::relation)
	{
		if (fact.relation < telemetry_battle_relation::hostile ||
		    fact.relation > telemetry_battle_relation::group_presence ||
		    !telemetry_battle_actor_key_is_valid(fact.related_actor) || !fact.active)
			return false;
	}
	else if (static_cast<std::uint8_t>(fact.relation) != 0U || fact.related_actor.id != 0U ||
		 static_cast<std::uint8_t>(fact.related_actor.kind) != 0U)
		return false;
	if (fact.kind == telemetry_battle_fact_kind::start &&
	    (fact.fact_index != 0U || fact.revision != 1U || fact.fact_count != 5U))
		return false;
	if (fact.kind == telemetry_battle_fact_kind::cut && fact.fact_index + 1U != fact.fact_count)
		return false;
	if (fact.kind == telemetry_battle_fact_kind::actor_context &&
	    fact.fact_index + 1U >= fact.fact_count)
		return false;
	if (fact.dropped_actor_count != 0U &&
	    (fact.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW) == 0U)
		return false;
	constexpr telemetry_quality_mask incomplete_graph =
		TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW |
		TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	if ((fact.quality_flags & incomplete_graph) != 0U &&
	    fact.side_status == telemetry_battle_side_status::qualified_observed_graph)
		return false;
	if (fact.kind == telemetry_battle_fact_kind::relation &&
	    fact.relation != telemetry_battle_relation::support &&
	    fact.actor.actor.actor_id == fact.related_actor.id)
		return false;
	if (terminal && (fact.close_reason == telemetry_battle_close_reason::inactivity ?
				 fact.at_monotonic_usec - fact.last_engagement_monotonic_usec <
					 fact.inactivity_grace_usec :
				 fact.observed_through_monotonic_usec != fact.at_monotonic_usec ||
					 fact.observed_through_utc_usec != fact.at_utc_usec))
		return false;
	return true;
}

/* The switch reads only the union member selected by header.kind. */
constexpr bool telemetry_record_is_valid(const telemetry_record &record) noexcept
{
	if (!telemetry_record_header_is_valid(record.header))
		return false;
	switch (record.header.kind)
	{
	case telemetry_record_kind::interval:
		return telemetry_interval_payload_is_valid(record.payload.interval);
	case telemetry_record_kind::session_lifecycle:
		return telemetry_session_lifecycle_payload_is_valid(record.payload.lifecycle);
	case telemetry_record_kind::session_checkpoint:
		return telemetry_session_checkpoint_payload_is_valid(record.payload.checkpoint);
	case telemetry_record_kind::coverage_gap:
		return telemetry_coverage_gap_payload_is_valid(record.payload.gap);
	case telemetry_record_kind::configuration:
		return telemetry_configuration_payload_is_valid(record.payload.configuration);
	case telemetry_record_kind::progression:
		return telemetry_progression_payload_is_valid(record.payload.progression);
	case telemetry_record_kind::encounter:
		return telemetry_encounter_payload_is_valid(record.payload.encounter);
	case telemetry_record_kind::combat_summary:
		return telemetry_combat_summary_payload_is_valid(record.payload.combat_summary);
	case telemetry_record_kind::ownership:
		return telemetry_ownership_payload_is_valid(record.payload.ownership) &&
		       record.header.key.producer.boot_id ==
			       record.payload.ownership.connection.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.ownership.connection.producer.process_id &&
		       record.header.occurrence_utc_usec == record.payload.ownership.at_utc_usec;
	case telemetry_record_kind::invalid:
		break;
	}
	return false;
}

static_assert(std::is_trivially_copyable_v<telemetry_record>);
static_assert(std::is_trivially_copyable_v<telemetry_ownership_payload>);
static_assert(std::is_standard_layout_v<telemetry_record>);
static_assert(std::is_trivially_copyable_v<telemetry_config_snapshot>);
static_assert(std::is_standard_layout_v<telemetry_config_snapshot>);
static_assert(sizeof(telemetry_config_snapshot) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(std::is_trivially_copyable_v<telemetry_connection_transition>);
static_assert(std::is_trivially_copyable_v<telemetry_health_snapshot>);
static_assert(std::is_trivially_copyable_v<telemetry_progression_payload>);
static_assert(std::is_standard_layout_v<telemetry_progression_payload>);
static_assert(sizeof(telemetry_progression_payload) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(std::is_trivially_copyable_v<telemetry_encounter_payload>);
static_assert(std::is_standard_layout_v<telemetry_encounter_payload>);
static_assert(sizeof(telemetry_encounter_payload) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(std::is_trivially_copyable_v<telemetry_combat_summary_payload>);
static_assert(std::is_standard_layout_v<telemetry_combat_summary_payload>);
static_assert(sizeof(telemetry_combat_summary_payload) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(sizeof(telemetry_record) <= TELEMETRY_RECORD_MAX_BYTES,
	      "telemetry_record must remain within the fixed queue record bound");

#endif
