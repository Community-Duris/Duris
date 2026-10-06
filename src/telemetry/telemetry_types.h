#ifndef DURIS_TELEMETRY_TYPES_H
#define DURIS_TELEMETRY_TYPES_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
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
	battle = 10,
	battle_contribution = 11,
	battle_build = 12,
	control = 13,
	battle_result = 14,
	progression_context = 15,
	progression_configuration = 16,
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

/* Independently versioned progression context. The sealed kind-6 amount
 * payload above remains the XP observation, never an authoritative award.
 * Selection, application, group presence and actual assistance gates are
 * distinct. A point does not imply continuously valid exposure. */
inline constexpr std::uint16_t TELEMETRY_PROGRESSION_CONTEXT_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_PROGRESSION_SOURCE_INVENTORY_VERSION = 1U;
inline constexpr std::size_t TELEMETRY_PROGRESSION_CONTEXT_STATS = 10U;
inline constexpr std::size_t TELEMETRY_PROGRESSION_CONTEXT_MAX_AFFECTS = 64U;

enum class telemetry_progression_rested_selection : std::uint8_t
{
	unknown = 0,
	none = 1,
	rested = 2,
	wellrested = 3,
};

enum class telemetry_progression_rested_application : std::uint8_t
{
	unknown = 0,
	resurrection_exempt = 1,
	none = 2,
	rested = 3,
	wellrested = 4,
};

enum class telemetry_progression_assistance : std::uint8_t
{
	unknown = 0,
	solo_kill_share = 1,
	group_kill_share = 2,
	self_healing = 3,
	group_healing = 4,
	group_tanking = 5,
	group_tank_support = 6,
	no_assistance_gate = 7,
};

enum class telemetry_progression_context_boundary : std::uint8_t
{
	baseline = 1,
	exposure = 2,
	experience = 3,
	level = 4,
	lifecycle_cut = 5,
	unknown = 6,
};

inline constexpr std::uint32_t TELEMETRY_PCTX_AFFECTS_COMPLETE = 1U << 0;
inline constexpr std::uint32_t TELEMETRY_PCTX_AUTOMATIC_RESTED = 1U << 1;
inline constexpr std::uint32_t TELEMETRY_PCTX_RESTED_PRESENT = 1U << 2;
inline constexpr std::uint32_t TELEMETRY_PCTX_WELLRESTED_PRESENT = 1U << 3;
inline constexpr std::uint32_t TELEMETRY_PCTX_RESTED_STAFF = 1U << 4;
inline constexpr std::uint32_t TELEMETRY_PCTX_WELLRESTED_STAFF = 1U << 5;
inline constexpr std::uint32_t TELEMETRY_PCTX_SELECTION_KNOWN = 1U << 6;
inline constexpr std::uint32_t TELEMETRY_PCTX_APPLICATION_KNOWN = 1U << 7;
inline constexpr std::uint32_t TELEMETRY_PCTX_ASSISTANCE_KNOWN = 1U << 8;
inline constexpr std::uint32_t TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN = 1U << 9;
inline constexpr std::uint32_t TELEMETRY_PCTX_THRESHOLD_KNOWN = 1U << 10;
inline constexpr std::uint32_t TELEMETRY_PCTX_BUILD_KNOWN = 1U << 11;
inline constexpr std::uint32_t TELEMETRY_PCTX_GROUP_ROSTER_KNOWN = 1U << 12;
inline constexpr std::uint32_t TELEMETRY_PCTX_STORAGE_GATE_KNOWN = 1U << 13;
inline constexpr std::uint32_t TELEMETRY_PCTX_STORAGE_GATE_PASSED = 1U << 14;
inline constexpr std::uint32_t TELEMETRY_PCTX_CONTIGUOUS_EXPOSURE = 1U << 15;
inline constexpr std::uint32_t TELEMETRY_PCTX_ALIVE = 1U << 16;
inline constexpr std::uint32_t TELEMETRY_PCTX_CONFIG_CATALOG_KNOWN = 1U << 17;
inline constexpr std::uint32_t TELEMETRY_PCTX_DECISION_POLICY_KNOWN = 1U << 18;
inline constexpr std::uint32_t TELEMETRY_PCTX_HARDCORE = 1U << 19;
inline constexpr std::uint32_t TELEMETRY_PCTX_HARDCORE_BYPASS = 1U << 20;
inline constexpr std::uint32_t TELEMETRY_PCTX_FLAGS = (1U << 21) - 1U;

/* Independent XP configuration inventory. Entries are actual cached/native
 * values, not a property-file dump. This value is captured only at bootstrap or
 * after property consumers update; it never traverses a character or writes XP.
 * Definition 1 enumerates all threshold/race/modifier slots plus live XP gates.
 * Original configuration/property catalogs retain their sealed meanings. */
inline constexpr std::uint16_t TELEMETRY_PROGRESSION_CONFIGURATION_VERSION = 1U;
inline constexpr std::size_t TELEMETRY_PROGRESSION_CONFIGURATION_VALUES = 337U;
inline constexpr std::size_t TELEMETRY_PROGRESSION_CONFIGURATION_MAX_VALUES = 384U;
inline constexpr std::size_t TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES = 24U;

enum class telemetry_progression_configuration_value_kind : std::uint8_t
{
	unsigned_integer = 1,
	signed_integer = 2,
	float32_bits = 3,
	float64_bits = 4,
};

struct telemetry_progression_configuration_value
{
	std::uint64_t bits;
	std::uint16_t id;
	telemetry_progression_configuration_value_kind kind;
	std::uint8_t reserved[5];
};

struct telemetry_progression_configuration_snapshot
{
	telemetry_config_id config_id;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	std::uint16_t version;
	std::uint16_t source_inventory_version;
	std::uint16_t count;
	std::uint16_t reserved;
	telemetry_progression_configuration_value
		values[TELEMETRY_PROGRESSION_CONFIGURATION_MAX_VALUES];
};

static_assert(std::is_trivially_copyable_v<telemetry_progression_configuration_snapshot>);
static_assert(std::is_standard_layout_v<telemetry_progression_configuration_snapshot>);
static_assert(sizeof(telemetry_progression_configuration_snapshot) <= 8U * 1024U);

/* One exact chunk of the independent cold XP inventory. The first raw receipt
 * is its immutable root. Chunks do not make a complete inventory until retained
 * reconciliation verifies every source ordinal, version and canonical digest. */
struct telemetry_progression_configuration_observation
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
	telemetry_record_sequence root_record_seq;
	telemetry_config_id config_id;
	telemetry_environment_id environment_id;
	telemetry_season_id season_id;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	std::uint64_t digest_words[4];
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	std::uint16_t total_values;
	std::uint16_t chunk_index;
	std::uint16_t chunk_count;
	std::uint16_t value_count;
	std::uint16_t version;
	std::uint16_t source_inventory_version;
	std::uint32_t reserved;
	std::uint64_t value_bits[TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES];
	std::uint16_t value_ids[TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES];
	telemetry_progression_configuration_value_kind
		value_kinds[TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES];
};

static_assert(std::is_trivially_copyable_v<telemetry_progression_configuration_observation>);
static_assert(std::is_standard_layout_v<telemetry_progression_configuration_observation>);
static_assert(sizeof(telemetry_progression_configuration_observation) <= 440U);

struct telemetry_progression_context_snapshot
{
	telemetry_config_id config_id;
	/* Relative to this observation's producer; publication resolves the exact
	 * complete retained catalog before comparing its effective digest. */
	telemetry_record_sequence configuration_record_seq;
	std::uint64_t configuration_digest_words[4];
	std::int32_t decision_level_cap;
	std::int32_t decision_good_assistance_gap;
	std::int32_t decision_evil_assistance_gap;
	std::int32_t decision_max_exp_level;
	std::uint64_t next_threshold_xp;
	std::int64_t current_exp;
	std::uint32_t primary_class_mask;
	std::uint32_t secondary_class_mask;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
	std::uint32_t flags;
	/* Str,Dex,Agi,Con,Pow,Int,Wis,Cha,Kar,Luk, as in the native build reader. */
	std::int16_t base_stats[TELEMETRY_PROGRESSION_CONTEXT_STATS];
	std::int16_t effective_stats[TELEMETRY_PROGRESSION_CONTEXT_STATS];
	std::uint32_t formal_group_size;
	std::uint16_t current_level;
	std::uint16_t threshold_level;
	std::uint16_t threshold_catalog_version;
	std::uint16_t specialization;
	std::uint16_t race;
	std::uint16_t faction;
	std::uint16_t eligible_group_size;
	std::uint16_t highest_group_level;
	std::uint16_t assistance_level;
	std::uint16_t version;
	telemetry_progression_rested_selection rested_selection;
	telemetry_progression_rested_application rested_application;
	telemetry_progression_assistance assistance;
	std::uint8_t reserved;
};

struct telemetry_progression_context_observation
{
	telemetry_producer_id producer;
	telemetry_sequence sequence;
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_record_key source_record;
	telemetry_record_key ownership_record;
	telemetry_id account_token;
	telemetry_monotonic_usec start_monotonic_usec;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_progression_context_snapshot context;
	std::uint16_t starting_level;
	std::uint16_t source_inventory_version;
	telemetry_progression_context_boundary boundary;
	telemetry_record_kind source_kind;
	std::uint16_t reserved;
};

static_assert(std::is_trivially_copyable_v<telemetry_progression_context_snapshot>);
static_assert(std::is_standard_layout_v<telemetry_progression_context_snapshot>);
static_assert(std::is_trivially_copyable_v<telemetry_progression_context_observation>);
static_assert(std::is_standard_layout_v<telemetry_progression_context_observation>);
static_assert(sizeof(telemetry_progression_context_observation) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

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

inline constexpr std::uint16_t TELEMETRY_BATTLE_CONTRIBUTION_VERSION = 1U;
inline constexpr std::uint32_t TELEMETRY_BC_DAMAGE = 1U;
inline constexpr std::uint32_t TELEMETRY_BC_HEALING = 2U;
inline constexpr std::uint32_t TELEMETRY_BC_CONTROL = 4U;
inline constexpr std::uint32_t TELEMETRY_BC_CASTING = 8U;
inline constexpr std::uint32_t TELEMETRY_BC_ENGAGEMENT = 16U;
inline constexpr std::uint32_t TELEMETRY_BC_METRICS = 31U;
/* Exact association reference and native context, supplied by the owning
 * collector. Availability names reviewed producer families, not complete
 * historical coverage. Missing families have unknown, not measured-zero, totals. */
struct telemetry_battle_contribution_context
{
	telemetry_battle_id battle;
	telemetry_encounter_source scope;
	telemetry_battle_actor_context actor;
	telemetry_revision association_revision;
	std::uint32_t association_fact_sequence;
	std::uint32_t available_metrics;
	telemetry_battle_side_status side_status;
	telemetry_encounter_mode mode;
	std::uint8_t side;
	std::uint8_t reserved;
	telemetry_quality_mask quality_flags;
};

struct telemetry_battle_contribution_counters
{
	std::uint64_t damage_dealt;
	std::uint64_t damage_taken;
	std::uint64_t healing_attempted;
	std::uint64_t effective_healing;
	std::uint64_t overhealing;
	std::uint64_t healing_received;
	std::uint64_t control_applications;
	std::uint64_t control_received;
	std::uint64_t casting_attempts;
	std::uint64_t casting_completions;
	std::uint64_t casting_aborts;
	std::uint64_t casting_unresolved;
	telemetry_duration_usec casting_elapsed_usec;
	/* Observed opponent-link duration; it does not establish incoming pressure
	 * or prevention and must not be relabeled as tanking. */
	telemetry_duration_usec engaged_target_usec;
};

enum class telemetry_battle_contribution_end : std::uint8_t
{
	context_changed = 1,
	actor_left = 2,
	battle_ended = 3,
	source_gap = 4,
};

struct telemetry_battle_contribution_cut
{
	telemetry_monotonic_usec observed_usec;
	telemetry_utc_usec observed_utc_usec;
	telemetry_monotonic_usec decision_usec;
	telemetry_utc_usec decision_utc_usec;
};

/* One sealed disjoint segment, emitted once. Its process-wide segment sequence
 * is independent of battle/actor identity and the later transport receipt.
 * A battle-ID/context change seals the old totals; no cumulative amount is
 * copied into the new segment. Alias resolution is the publisher's job. */
struct telemetry_battle_contribution_payload
{
	telemetry_battle_contribution_context context;
	telemetry_sequence sequence;
	telemetry_revision last_association_revision;
	std::uint32_t last_association_fact_sequence;
	std::uint32_t modifier_flags;
	telemetry_monotonic_usec start_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_battle_contribution_cut cut;
	telemetry_battle_contribution_counters counters;
	telemetry_quality_mask quality_flags;
	std::uint16_t definition_version;
	telemetry_battle_contribution_end end_reason;
	std::uint8_t reserved;
};

/* Compact observed build context is a separate domain from relationship facts
 * and contribution amounts. Native affect flag banks are omitted: this family
 * keeps selected traversal counts and their explicit source uncertainty. */
inline constexpr std::uint16_t TELEMETRY_BATTLE_BUILD_DEFINITION_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_BUILD_CONTEXT_VERSION = 1U;

enum telemetry_battle_build_available : std::uint32_t
{
	TELEMETRY_BUILD_BASE = 1U << 0U,
	TELEMETRY_BUILD_EFFECTIVE = 1U << 1U,
	TELEMETRY_BUILD_RESOURCES = 1U << 2U,
	TELEMETRY_BUILD_SAVING_MODIFIERS = 1U << 3U,
	TELEMETRY_BUILD_EFFECTIVE_FLAGS = 1U << 4U,
	TELEMETRY_BUILD_FIXED_EQUIPMENT = 1U << 5U,
	TELEMETRY_BUILD_LEARNED_EPICS = 1U << 6U,
	TELEMETRY_BUILD_LISTED_AFFECTS = 1U << 7U,
	TELEMETRY_BUILD_ARENA_ROOM = 1U << 8U,
	TELEMETRY_BUILD_ARENA_ROSTER = 1U << 9U,
};

enum telemetry_battle_build_quality : std::uint32_t
{
	TELEMETRY_BUILD_EPICS_UNAVAILABLE = 1U << 0U,
	TELEMETRY_BUILD_EQUIPMENT_INVALID = 1U << 1U,
	TELEMETRY_BUILD_AFFECTS_TRUNCATED = 1U << 2U,
	TELEMETRY_BUILD_AFFECTS_CYCLIC = 1U << 3U,
	TELEMETRY_BUILD_ARENA_UNAVAILABLE = 1U << 4U,
	TELEMETRY_BUILD_ARENA_INVALID = 1U << 5U,
	TELEMETRY_BUILD_ROOM_UNAVAILABLE = 1U << 6U,
	TELEMETRY_BUILD_SUPPORT_ORIGIN_UNKNOWN = 1U << 7U,
};

enum class telemetry_battle_build_status : std::uint8_t
{
	snapshot = 1,
	unavailable = 2,
};

enum class telemetry_battle_build_boundary : std::uint8_t
{
	actor_entry = 1,
	actor_changed = 2,
	configuration_changed = 3,
	periodic_sample = 4,
	source_resumed = 5,
	rate_limit = 6,
	source_unavailable = 7,
	configuration_unavailable = 8,
};

struct telemetry_battle_build_observation
{
	telemetry_battle_id battle;
	telemetry_environment_id environment_id;
	telemetry_season_id season_id;
	telemetry_config_id config_id;
	telemetry_id actor_id;
	telemetry_sequence sequence;
	telemetry_revision association_revision;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	std::uint64_t effective_flags[5];
	std::uint64_t equipment_flags[5];
	std::uint8_t equipment_digest[32];
	std::uint8_t epic_digest[32];
	std::uint32_t primary_class_mask;
	std::uint32_t secondary_class_mask;
	std::uint32_t build_version;
	std::uint32_t content_version;
	std::uint32_t available;
	std::uint32_t context_quality;
	telemetry_quality_mask quality_flags;
	std::uint32_t association_fact_sequence;
	std::int32_t base_resources[4];
	std::int32_t effective_resources[4];
	std::int32_t current_resources[4];
	std::int32_t base_combat[3];
	std::int32_t effective_combat[3];
	std::int32_t equipment_modifiers[5];
	std::int32_t arena_player_flags;
	std::int16_t base_stats[10];
	std::int16_t effective_stats[10];
	std::uint16_t definition_version;
	std::uint16_t native_context_version;
	std::uint16_t level;
	std::uint16_t race;
	std::uint16_t faction;
	std::uint16_t epic_catalog_skills;
	std::uint16_t epic_learned_skills;
	std::uint16_t affect_nodes;
	std::uint16_t offensive_modifier_nodes;
	std::uint16_t armor_modifier_nodes;
	std::uint16_t resource_modifier_nodes;
	std::uint16_t unapplied_nodes;
	telemetry_combat_actor_kind actor_kind;
	std::uint8_t specialization;
	std::int8_t saving_modifiers[5];
	std::uint8_t equipment_counts[7];
	std::uint8_t affects_complete;
	std::uint8_t arena_membership;
	std::uint8_t arena_room;
	std::uint8_t arena_enabled;
	std::uint8_t arena_type;
	std::uint8_t arena_stage;
	std::uint8_t arena_team;
	telemetry_battle_build_boundary boundary;
	telemetry_battle_build_status status;
};

static_assert(std::is_trivially_copyable_v<telemetry_battle_build_observation>);
static_assert(std::is_standard_layout_v<telemetry_battle_build_observation>);
static_assert(sizeof(telemetry_battle_build_observation) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

/* Reviewed control results and target-state intervals are a separate value
 * domain. Application operations and elapsed target time have different units;
 * a target interval never assigns its duration to a guessed caster. */
inline constexpr std::uint16_t TELEMETRY_CONTROL_DEFINITION_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_PRODUCER_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_STATE_MASK = 255U;

enum class telemetry_control_family : std::uint8_t
{
	unknown = 0,
	blindness = 1,
	stun = 2,
	major_paralysis = 3,
	minor_paralysis = 4,
	slow = 5,
	sleep = 6,
	silence = 7,
	entangle = 8,
};

enum class telemetry_control_result : std::uint8_t
{
	unknown = 0,
	applied = 1,
	saved = 2,
	resisted = 3,
	immune = 4,
	already_present = 5,
	movement_protection = 6,
	target_protected = 7,
	source_ineligible = 8,
	target_ineligible = 9,
	location_ineligible = 10,
	level_ineligible = 11,
	class_ineligible = 12,
	percentage_rejected = 13,
	unclassified_rejection = 14,
};

enum class telemetry_control_kind : std::uint8_t
{
	resolution = 1,
	state_entry = 2,
	state_interval = 3,
	source_gap = 4,
};

enum class telemetry_control_boundary : std::uint8_t
{
	actor_entry = 1,
	state_changed = 2,
	periodic_observation = 3,
	context_changed = 4,
	actor_left = 5,
	battle_ended = 6,
	source_unavailable = 7,
	configuration_unavailable = 8,
	clock_discontinuity = 9,
	capacity_refused = 10,
	attempt_resolved = 11,
};

inline constexpr std::uint16_t TELEMETRY_CONTROL_TRUSTED_SOURCE = 1U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_NEGATIVE_LEVEL = 2U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_SAVE_BYPASSED = 4U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_SLEEP_REFRESH = 8U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_HALF_STUN = 16U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_SELF = 32U;
inline constexpr std::uint16_t TELEMETRY_CONTROL_FLAGS = 63U;

/* The session and native group/dimensions are observations, not authenticated
 * account/controller authority. The optional association reference is resolved
 * against complete retained battle packets by the publisher. */
struct telemetry_control_actor_context
{
	telemetry_combat_actor_ref actor;
	telemetry_session_id session;
	telemetry_dimensions dimensions;
	telemetry_id group_key;
	std::uint16_t group_revision;
	std::uint16_t context_version;
	telemetry_quality_mask quality_flags;
};

struct telemetry_control_association
{
	telemetry_sequence battle_sequence;
	telemetry_revision revision;
	std::uint32_t fact_sequence;
};

struct telemetry_control_observation
{
	telemetry_producer_id producer;
	telemetry_encounter_source scope;
	telemetry_sequence sequence;
	telemetry_sequence previous_state_sequence;
	telemetry_control_actor_context source;
	telemetry_control_actor_context target;
	telemetry_control_association source_association;
	telemetry_control_association target_association;
	telemetry_revision last_target_association_revision;
	std::uint32_t last_target_association_fact_sequence;
	telemetry_monotonic_usec start_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_monotonic_usec at_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_monotonic_usec decision_usec;
	telemetry_utc_usec decision_utc_usec;
	std::uint32_t build_version;
	std::uint32_t content_version;
	telemetry_quality_mask quality_flags;
	/* Native scheduler ticks, not measured microseconds; never a duration
	 * denominator. Negative/zero accepted declarations retain their value. */
	std::int32_t configured_ticks;
	std::uint16_t definition_version;
	std::uint16_t producer_version;
	std::uint16_t flags;
	std::uint16_t before_mask;
	std::uint16_t after_mask;
	std::uint16_t state_available;
	std::uint16_t duration_coverage;
	telemetry_control_kind kind;
	telemetry_control_family family;
	telemetry_control_result result;
	telemetry_control_boundary boundary;
};

static_assert(std::is_trivially_copyable_v<telemetry_control_observation>);
static_assert(std::is_standard_layout_v<telemetry_control_observation>);
static_assert(sizeof(telemetry_control_observation) + sizeof(telemetry_record_header) <=
	      TELEMETRY_RECORD_MAX_BYTES);

/* Independently versioned evidence about a participant or an exact committed
 * objective. Neither a death nor a room departure declares a battle winner. */
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_DEFINITION_VERSION = 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_PRODUCER_VERSION = 1U;
inline constexpr telemetry_duration_usec TELEMETRY_BATTLE_ESCAPE_MIN_USEC = 30'000'000ULL;

enum class telemetry_battle_result_kind : std::uint8_t
{
	death_observed = 1,
	flee_movement = 2,
	withdrawal = 3,
	escape_observed = 4,
	objective_requested = 5,
	objective_committed = 6,
	unresolved = 7,
	censored = 8,
};

enum class telemetry_battle_result_authority : std::uint8_t
{
	native_death = 1,
	accepted_flee = 2,
	accepted_retreat = 3,
	accepted_disengage = 4,
	bounded_escape_watch = 5,
	zone_touch_submit = 6,
	zone_touch_receipt = 7,
	zone_touch_outbox = 8,
	lifecycle = 9,
};

enum class telemetry_battle_result_reason : std::uint8_t
{
	none = 0,
	reengaged = 1,
	not_alive = 2,
	session_lost = 3,
	copyover = 4,
	shutdown = 5,
	clock_unknown = 6,
	source_unknown = 7,
	capacity_refused = 8,
	configuration_unknown = 9,
	actor_unknown = 10,
	operation_pending = 11,
	movement_refused = 12,
	reincarnated = 13,
	receipt_unknown = 14,
};

inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_TRUSTED = 1U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_ARENA = 2U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_MOVED = 4U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_NO_OPPONENT = 8U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_SAME_SESSION = 16U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_WATCH_COMPLETE = 32U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_RESET_REQUESTED = 64U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_RECORD_ZONE = 128U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_RECOVERED = 256U;
inline constexpr std::uint16_t TELEMETRY_BATTLE_RESULT_FLAGS = 511U;

struct telemetry_battle_result_observation
{
	telemetry_producer_id producer;
	telemetry_encounter_source scope;
	telemetry_sequence sequence;
	telemetry_sequence parent_sequence;
	telemetry_control_actor_context source;
	telemetry_control_actor_context target;
	telemetry_control_association source_association;
	telemetry_control_association target_association;
	/* Context is retained before teardown; at is the accepted observation. An
	 * escape confirmation starts at its exact preceding movement observation. */
	telemetry_monotonic_usec start_usec;
	telemetry_utc_usec start_utc_usec;
	telemetry_monotonic_usec at_usec;
	telemetry_utc_usec at_utc_usec;
	std::uint8_t operation_id[16];
	std::uint64_t source_object_uid;
	telemetry_duration_usec proof_window_usec;
	std::uint32_t build_version;
	std::uint32_t content_version;
	telemetry_quality_mask quality_flags;
	std::int32_t credited_zone_vnum;
	std::int32_t from_room_vnum;
	std::int32_t to_room_vnum;
	std::uint16_t participant_count;
	std::uint16_t source_payload_version;
	std::uint16_t definition_version;
	std::uint16_t producer_version;
	std::uint16_t flags;
	telemetry_battle_result_kind kind;
	telemetry_battle_result_authority authority;
	telemetry_battle_result_reason reason;
	std::uint8_t reserved[3];
};

union telemetry_record_payload
{
	telemetry_interval_payload interval;
	telemetry_session_lifecycle_payload lifecycle;
	telemetry_session_checkpoint_payload checkpoint;
	telemetry_coverage_gap_payload gap;
	telemetry_configuration_payload configuration;
	telemetry_progression_payload progression;
	telemetry_progression_context_observation progression_context;
	telemetry_progression_configuration_observation progression_configuration;
	telemetry_encounter_payload encounter;
	telemetry_combat_summary_payload combat_summary;
	telemetry_ownership_payload ownership;
	telemetry_battle_fact battle;
	telemetry_battle_contribution_payload battle_contribution;
	telemetry_battle_build_observation battle_build;
	telemetry_control_observation control;
	telemetry_battle_result_observation battle_result;
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
	       kind == telemetry_record_kind::ownership || kind == telemetry_record_kind::battle ||
	       kind == telemetry_record_kind::battle_contribution ||
	       kind == telemetry_record_kind::battle_build ||
	       kind == telemetry_record_kind::control ||
	       kind == telemetry_record_kind::battle_result ||
	       kind == telemetry_record_kind::progression_context ||
	       kind == telemetry_record_kind::progression_configuration;
}

/* Per-attempt/state control detail does not consume the lifecycle reserve. */
constexpr bool telemetry_record_kind_is_control(telemetry_record_kind kind) noexcept
{
	return kind == telemetry_record_kind::session_lifecycle ||
	       kind == telemetry_record_kind::session_checkpoint ||
	       kind == telemetry_record_kind::coverage_gap ||
	       kind == telemetry_record_kind::configuration ||
	       kind == telemetry_record_kind::encounter ||
	       kind == telemetry_record_kind::combat_summary ||
	       kind == telemetry_record_kind::ownership || kind == telemetry_record_kind::battle ||
	       kind == telemetry_record_kind::battle_contribution ||
	       kind == telemetry_record_kind::battle_build ||
	       kind == telemetry_record_kind::progression_configuration;
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

constexpr bool telemetry_battle_contribution_context_is_valid(
	const telemetry_battle_contribution_context &c) noexcept
{
	if (!telemetry_battle_id_is_valid(c.battle) ||
	    !telemetry_encounter_source_is_valid(c.scope) || c.scope.group_key ||
	    c.scope.zone_vnum != -1 || !telemetry_battle_actor_context_is_valid(c.actor) ||
	    !c.association_revision || !c.association_fact_sequence ||
	    (c.available_metrics & ~TELEMETRY_BC_METRICS) ||
	    c.side_status < telemetry_battle_side_status::qualified_observed_graph ||
	    c.side_status > telemetry_battle_side_status::partial ||
	    !telemetry_encounter_mode_is_valid(c.mode) || c.side > 2U || c.reserved ||
	    !telemetry_quality_mask_is_valid(c.quality_flags) ||
	    (c.actor.quality_flags & ~c.quality_flags))
		return false;
	if (c.side_status == telemetry_battle_side_status::qualified_observed_graph)
	{
		constexpr auto incomplete_graph =
			TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_CARDINALITY_OVERFLOW |
			TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
		if (!c.side || (c.quality_flags & incomplete_graph))
			return false;
	}
	else if (c.side)
		return false;
	return c.actor.encounter.sequence == 0U ||
	       (c.actor.encounter.producer.boot_id == c.battle.producer.boot_id &&
		c.actor.encounter.producer.process_id == c.battle.producer.process_id);
}

constexpr bool telemetry_battle_contribution_payload_is_valid(
	const telemetry_battle_contribution_payload &v) noexcept
{
	if (!telemetry_battle_contribution_context_is_valid(v.context) || !v.sequence ||
	    v.definition_version != TELEMETRY_BATTLE_CONTRIBUTION_VERSION || v.reserved ||
	    (v.end_reason < telemetry_battle_contribution_end::context_changed ||
	     v.end_reason > telemetry_battle_contribution_end::source_gap) ||
	    (v.cut.decision_usec < v.cut.observed_usec) || v.start_usec > v.cut.observed_usec ||
	    v.last_association_revision < v.context.association_revision ||
	    v.last_association_fact_sequence < v.context.association_fact_sequence ||
	    (v.last_association_fact_sequence == v.context.association_fact_sequence &&
	     v.last_association_revision != v.context.association_revision) ||
	    !telemetry_combat_modifier_flags_are_valid(v.modifier_flags) ||
	    !telemetry_quality_mask_is_valid(v.quality_flags) ||
	    (v.context.quality_flags & ~v.quality_flags))
		return false;
	const auto &x = v.counters;
	const auto mask = v.context.available_metrics;
	if (!(mask & TELEMETRY_BC_DAMAGE) && (x.damage_dealt || x.damage_taken))
		return false;
	if (!(mask & TELEMETRY_BC_HEALING) &&
	    (x.healing_attempted || x.effective_healing || x.overhealing || x.healing_received))
		return false;
	if (!(mask & TELEMETRY_BC_CONTROL) && (x.control_applications || x.control_received))
		return false;
	if (!(mask & TELEMETRY_BC_CASTING) &&
	    (x.casting_attempts || x.casting_completions || x.casting_aborts ||
	     x.casting_unresolved || x.casting_elapsed_usec))
		return false;
	if (!(mask & TELEMETRY_BC_ENGAGEMENT) && x.engaged_target_usec)
		return false;
	if (x.effective_healing > x.healing_attempted || x.overhealing > x.healing_attempted ||
	    (!(v.quality_flags & TELEMETRY_QUALITY_CARDINALITY_OVERFLOW) &&
	     x.overhealing != x.healing_attempted - x.effective_healing) ||
	    x.casting_completions > x.casting_attempts ||
	    x.casting_aborts > x.casting_attempts - x.casting_completions ||
	    x.casting_unresolved != x.casting_attempts - x.casting_completions - x.casting_aborts ||
	    x.casting_unresolved > 1U || (!x.casting_attempts && x.casting_elapsed_usec) ||
	    (x.casting_unresolved && !(v.quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL)) ||
	    (x.control_applications && !(v.modifier_flags & TELEMETRY_COMBAT_MODIFIER_CONTROL)) ||
	    x.casting_elapsed_usec > v.cut.observed_usec - v.start_usec ||
	    x.engaged_target_usec > v.cut.observed_usec - v.start_usec)
		return false;
	const bool utc_reversed = (v.start_utc_usec != TELEMETRY_UTC_UNKNOWN &&
				   v.cut.observed_utc_usec != TELEMETRY_UTC_UNKNOWN &&
				   v.cut.observed_utc_usec < v.start_utc_usec) ||
				  (v.cut.observed_utc_usec != TELEMETRY_UTC_UNKNOWN &&
				   v.cut.decision_utc_usec != TELEMETRY_UTC_UNKNOWN &&
				   v.cut.decision_utc_usec < v.cut.observed_utc_usec);
	return (!utc_reversed || (v.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)) &&
	       (v.end_reason != telemetry_battle_contribution_end::source_gap ||
		(v.quality_flags &
		 (TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP)) ==
			(TELEMETRY_QUALITY_CONTEXT_UNKNOWN | TELEMETRY_QUALITY_QUEUE_DROP));
}

namespace telemetry_battle_build_detail
{
using observation = telemetry_battle_build_observation;

template <typename T, std::size_t Size> constexpr bool zero(const T (&values)[Size]) noexcept
{
	for (auto value : values)
		if (value != 0)
			return false;
	return true;
}

constexpr bool metadata(std::string_view name) noexcept
{
	return name.starts_with("bctx_battle_") || name == "bctx_environment_id" ||
	       name == "bctx_season_id" || name == "bctx_config_id" || name == "bctx_actor_id" ||
	       name == "bctx_actor_kind" || name == "bctx_sequence" ||
	       name.starts_with("bctx_association_") || name.starts_with("bctx_at_") ||
	       name == "bctx_definition_version" || name == "bctx_native_context_version" ||
	       name == "bctx_boundary" || name == "bctx_status" || name == "bctx_build_version" ||
	       name == "bctx_content_version" || name == "bctx_quality_flags" ||
	       name == "bctx_context_quality";
}

constexpr bool empty_profile(const observation &v) noexcept
{
#define TELEMETRY_BUILD_FIELD(name, member, width, signed_value) \
	if constexpr (!metadata(#name))                          \
		if (static_cast<std::uint64_t>(v.member) != 0U)  \
			return false;
#define TELEMETRY_BUILD_BYTES(name, member, width) \
	if (!zero(v.member))                       \
		return false;
#include "telemetry/telemetry_battle_build_fields.inc"
#undef TELEMETRY_BUILD_FIELD
#undef TELEMETRY_BUILD_BYTES
	return true;
}

} // namespace telemetry_battle_build_detail

constexpr bool
telemetry_battle_build_observation_is_valid(const telemetry_battle_build_observation &v) noexcept
{
	using namespace telemetry_battle_build_detail;
	if (!telemetry_battle_id_is_valid(v.battle) || v.environment_id == 0U ||
	    v.season_id == 0U || v.sequence == 0U || v.association_revision == 0U ||
	    v.association_fact_sequence == 0U ||
	    !telemetry_battle_actor_key_is_valid({ v.actor_id, v.actor_kind }) ||
	    v.definition_version != TELEMETRY_BATTLE_BUILD_DEFINITION_VERSION ||
	    v.native_context_version != TELEMETRY_BATTLE_BUILD_CONTEXT_VERSION ||
	    !telemetry_quality_mask_is_valid(v.quality_flags) ||
	    (v.context_quality & ~255U) != 0U || (v.available & ~1023U) != 0U ||
	    v.boundary < telemetry_battle_build_boundary::actor_entry ||
	    v.boundary > telemetry_battle_build_boundary::configuration_unavailable)
		return false;
	if (v.boundary == telemetry_battle_build_boundary::configuration_unavailable)
	{
		if (v.config_id != 0U || v.build_version != 0U || v.content_version != 0U)
			return false;
	}
	else if (v.config_id == 0U || v.build_version == 0U || v.content_version == 0U)
		return false;
	if (v.status == telemetry_battle_build_status::unavailable)
		return v.boundary >= telemetry_battle_build_boundary::rate_limit &&
		       (v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) && empty_profile(v);
	if (v.status != telemetry_battle_build_status::snapshot || v.available == 0U ||
	    v.boundary > telemetry_battle_build_boundary::source_resumed ||
	    !(v.context_quality & TELEMETRY_BUILD_SUPPORT_ORIGIN_UNKNOWN))
		return false;
	if (!(v.available & TELEMETRY_BUILD_BASE) &&
	    (!zero(v.base_stats) || !zero(v.base_resources) || !zero(v.base_combat)))
		return false;
	if (!(v.available & TELEMETRY_BUILD_EFFECTIVE) &&
	    (!zero(v.effective_stats) || !zero(v.effective_resources) || !zero(v.effective_combat)))
		return false;
	if (!(v.available & TELEMETRY_BUILD_RESOURCES) && !zero(v.current_resources))
		return false;
	if (!(v.available & TELEMETRY_BUILD_SAVING_MODIFIERS) && !zero(v.saving_modifiers))
		return false;
	if (!(v.available & TELEMETRY_BUILD_EFFECTIVE_FLAGS) && !zero(v.effective_flags))
		return false;
	if (!(v.available & TELEMETRY_BUILD_FIXED_EQUIPMENT))
	{
		if (!zero(v.equipment_counts) || !zero(v.equipment_modifiers) ||
		    !zero(v.equipment_flags) || !zero(v.equipment_digest))
			return false;
	}
	else if (v.equipment_counts[0] > 43U || v.equipment_counts[6] > v.equipment_counts[0] ||
		 v.equipment_counts[1] + v.equipment_counts[2] + v.equipment_counts[3] +
				 v.equipment_counts[4] + v.equipment_counts[5] !=
			 v.equipment_counts[0] ||
		 zero(v.equipment_digest) ||
		 (v.context_quality & TELEMETRY_BUILD_EQUIPMENT_INVALID))
		return false;
	if (!(v.available & TELEMETRY_BUILD_LEARNED_EPICS))
	{
		if (v.epic_catalog_skills != 0U || v.epic_learned_skills != 0U ||
		    !zero(v.epic_digest))
			return false;
	}
	else if (v.actor_kind != telemetry_combat_actor_kind::player ||
		 v.epic_catalog_skills == 0U || v.epic_catalog_skills > 309U ||
		 v.epic_learned_skills > v.epic_catalog_skills || zero(v.epic_digest) ||
		 (v.context_quality & TELEMETRY_BUILD_EPICS_UNAVAILABLE))
		return false;
	if (!(v.available & TELEMETRY_BUILD_LISTED_AFFECTS))
	{
		if (v.affect_nodes != 0U || v.offensive_modifier_nodes != 0U ||
		    v.armor_modifier_nodes != 0U || v.resource_modifier_nodes != 0U ||
		    v.unapplied_nodes != 0U || v.affects_complete != 0U)
			return false;
	}
	else
	{
		const auto partial = v.context_quality & (TELEMETRY_BUILD_AFFECTS_TRUNCATED |
							  TELEMETRY_BUILD_AFFECTS_CYCLIC);
		if (v.affect_nodes > 64U || v.unapplied_nodes > v.affect_nodes ||
		    v.offensive_modifier_nodes + v.armor_modifier_nodes +
				    v.resource_modifier_nodes >
			    v.affect_nodes - v.unapplied_nodes ||
		    v.affects_complete > 1U || (v.affects_complete && partial) ||
		    (!v.affects_complete && !partial) ||
		    ((v.context_quality & TELEMETRY_BUILD_AFFECTS_TRUNCATED) &&
		     v.affect_nodes != 64U) ||
		    ((v.context_quality & TELEMETRY_BUILD_AFFECTS_CYCLIC) && v.affect_nodes == 0U))
			return false;
	}
	if (v.arena_room > 1U ||
	    (!(v.available & TELEMETRY_BUILD_ARENA_ROOM) && v.arena_room != 0U) ||
	    ((v.available & TELEMETRY_BUILD_ARENA_ROOM) &&
	     (v.context_quality & TELEMETRY_BUILD_ROOM_UNAVAILABLE)) ||
	    v.arena_enabled > 1U || v.arena_type > 5U || v.arena_stage > 5U)
		return false;
	if (v.available & TELEMETRY_BUILD_ARENA_ROSTER)
	{
		if (v.context_quality &
		    (TELEMETRY_BUILD_ARENA_INVALID | TELEMETRY_BUILD_ARENA_UNAVAILABLE))
			return false;
		if (v.arena_membership == 1U)
			return v.arena_team == 0U && v.arena_player_flags == 0;
		return v.arena_membership == 2U &&
		       v.actor_kind == telemetry_combat_actor_kind::player && v.arena_team >= 1U &&
		       v.arena_team <= 3U;
	}
	return (v.arena_membership == 0U ||
		(v.arena_membership == 3U &&
		 (v.context_quality & TELEMETRY_BUILD_ARENA_INVALID))) &&
	       v.arena_team == 0U && v.arena_player_flags == 0;
}

namespace telemetry_control_validation_detail
{
using actor_context = telemetry_control_actor_context;
using association = telemetry_control_association;
using kind = telemetry_control_kind;
using family = telemetry_control_family;
using result = telemetry_control_result;
using boundary = telemetry_control_boundary;
constexpr bool actor_valid(const actor_context &v) noexcept
{
	telemetry_battle_actor_context a{};
	a.actor = v.actor;
	a.session = v.session;
	a.dimensions = v.dimensions;
	a.group_key = v.group_key;
	a.group_revision = v.group_revision;
	a.context_version = v.context_version;
	a.quality_flags = v.quality_flags;
	return telemetry_battle_actor_context_is_valid(a);
}

constexpr bool actor_zero(const actor_context &v) noexcept
{
	telemetry_battle_actor_context a{};
	a.actor = v.actor;
	a.session = v.session;
	a.dimensions = v.dimensions;
	a.group_key = v.group_key;
	a.group_revision = v.group_revision;
	a.context_version = v.context_version;
	a.quality_flags = v.quality_flags;
	return telemetry_battle_actor_context_is_zero(a);
}

constexpr bool association_valid(const association &v) noexcept
{
	return (v.battle_sequence == 0U && v.revision == 0U && v.fact_sequence == 0U) ||
	       (v.battle_sequence != 0U && v.revision != 0U && v.fact_sequence != 0U);
}

constexpr bool association_zero(const association &v) noexcept
{
	return v.battle_sequence == 0U && v.revision == 0U && v.fact_sequence == 0U;
}

} // namespace telemetry_control_validation_detail

constexpr bool
telemetry_control_observation_is_valid(const telemetry_control_observation &v) noexcept
{
	using namespace telemetry_control_validation_detail;
	const bool configuration_unknown = v.kind == kind::source_gap &&
					   v.boundary == boundary::configuration_unavailable;
	if (configuration_unknown ? v.scope.config_id != 0U || v.scope.classifier_version != 0U ||
					    v.scope.policy_version != 0U || v.build_version != 0U ||
					    v.content_version != 0U :
				    !telemetry_encounter_source_is_valid(v.scope) ||
					    v.build_version == 0U || v.content_version == 0U)
		return false;
	if (!telemetry_producer_id_is_valid(v.producer) || v.sequence == 0U ||
	    v.scope.environment_id == 0U || v.scope.season_id == 0U || v.scope.group_key != 0U ||
	    v.scope.zone_vnum != -1 ||
	    v.definition_version != TELEMETRY_CONTROL_DEFINITION_VERSION ||
	    v.producer_version != TELEMETRY_CONTROL_PRODUCER_VERSION || !actor_valid(v.target) ||
	    !association_valid(v.source_association) || !association_valid(v.target_association) ||
	    !telemetry_quality_mask_is_valid(v.quality_flags) ||
	    ((v.source.quality_flags | v.target.quality_flags) & ~v.quality_flags) != 0U ||
	    (v.flags & ~TELEMETRY_CONTROL_FLAGS) != 0U ||
	    ((v.before_mask | v.after_mask | v.state_available | v.duration_coverage) &
	     ~TELEMETRY_CONTROL_STATE_MASK) != 0U ||
	    v.start_usec > v.at_usec || v.at_usec > v.decision_usec ||
	    v.previous_state_sequence >= v.sequence || v.boundary < boundary::actor_entry ||
	    v.boundary > boundary::attempt_resolved)
		return false;
	if (association_zero(v.target_association) ?
		    v.last_target_association_revision != 0U ||
			    v.last_target_association_fact_sequence != 0U :
		    v.last_target_association_revision < v.target_association.revision ||
			    v.last_target_association_fact_sequence <
				    v.target_association.fact_sequence ||
			    (v.last_target_association_fact_sequence ==
				     v.target_association.fact_sequence &&
			     v.last_target_association_revision != v.target_association.revision))
		return false;
	if ((v.start_utc_usec != TELEMETRY_UTC_UNKNOWN && v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     v.at_utc_usec < v.start_utc_usec) ||
	    (v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	     v.decision_utc_usec != TELEMETRY_UTC_UNKNOWN && v.decision_utc_usec < v.at_utc_usec))
		if (!(v.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY))
			return false;
	if (v.kind == kind::resolution)
	{
		const bool self = v.source.actor.actor_id == v.target.actor.actor_id &&
				  v.source.actor.kind == v.target.actor.kind;
		if (!actor_valid(v.source) || v.previous_state_sequence != 0U ||
		    v.boundary != boundary::attempt_resolved || v.start_usec != v.at_usec ||
		    v.at_usec != v.decision_usec || v.start_utc_usec != v.at_utc_usec ||
		    v.at_utc_usec != v.decision_utc_usec || v.family < family::blindness ||
		    v.family > family::entangle || v.result < result::applied ||
		    v.result > result::unclassified_rejection ||
		    v.state_available != TELEMETRY_CONTROL_STATE_MASK ||
		    v.duration_coverage != 0U ||
		    v.last_target_association_revision != v.target_association.revision ||
		    v.last_target_association_fact_sequence != v.target_association.fact_sequence ||
		    static_cast<bool>(v.flags & TELEMETRY_CONTROL_SELF) != self)
			return false;
		if (v.result != result::applied)
			return v.configured_ticks == 0 &&
			       (v.flags &
				(TELEMETRY_CONTROL_SAVE_BYPASSED | TELEMETRY_CONTROL_SLEEP_REFRESH |
				 TELEMETRY_CONTROL_HALF_STUN)) == 0U;
		const auto effect = v.family == family::entangle ?
					    136U :
					    1U << (static_cast<unsigned>(v.family) - 1U);
		return (v.after_mask & effect) != 0U &&
		       (!(v.flags & TELEMETRY_CONTROL_SLEEP_REFRESH) ||
			(v.family == family::sleep && (v.before_mask & 32U))) &&
		       (!(v.flags & TELEMETRY_CONTROL_HALF_STUN) || v.family == family::stun) &&
		       (!(v.flags & TELEMETRY_CONTROL_SAVE_BYPASSED) || v.family == family::stun ||
			v.family == family::major_paralysis || v.family == family::sleep);
	}
	if (!actor_zero(v.source) || !association_zero(v.source_association) ||
	    v.family != family::unknown || v.result != result::unknown || v.flags != 0U ||
	    v.configured_ticks != 0 || v.boundary == boundary::attempt_resolved)
		return false;
	if (v.kind == kind::source_gap)
		return v.state_available == 0U && v.duration_coverage == 0U &&
		       v.before_mask == 0U && v.after_mask == 0U &&
		       (v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) != 0U &&
		       v.boundary >= boundary::source_unavailable;
	if (v.state_available != TELEMETRY_CONTROL_STATE_MASK ||
	    (v.duration_coverage != TELEMETRY_CONTROL_STATE_MASK &&
	     !(v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN)) ||
	    association_zero(v.target_association))
		return false;
	if (v.kind == kind::state_entry)
		return v.previous_state_sequence == 0U && v.start_usec == v.at_usec &&
		       v.at_usec == v.decision_usec && v.start_utc_usec == v.at_utc_usec &&
		       v.at_utc_usec == v.decision_utc_usec && v.before_mask == v.after_mask &&
		       v.last_target_association_revision == v.target_association.revision &&
		       v.last_target_association_fact_sequence ==
			       v.target_association.fact_sequence &&
		       (v.boundary == boundary::actor_entry ||
			v.boundary == boundary::context_changed);
	return v.kind == kind::state_interval && v.previous_state_sequence != 0U &&
	       v.boundary >= boundary::state_changed && v.boundary <= boundary::capacity_refused &&
	       (v.at_usec == v.decision_usec ||
		(v.quality_flags & TELEMETRY_QUALITY_UNCLOSED_TAIL) != 0U);
}

/* The switch reads only the union member selected by header.kind. */
constexpr bool
telemetry_battle_result_observation_is_valid(const telemetry_battle_result_observation &v) noexcept
{
	using kind = telemetry_battle_result_kind;
	using authority = telemetry_battle_result_authority;
	using reason = telemetry_battle_result_reason;
	using telemetry_control_validation_detail::actor_valid;
	using telemetry_control_validation_detail::actor_zero;
	using telemetry_control_validation_detail::association_valid;
	using telemetry_control_validation_detail::association_zero;
	const bool uncertain = v.kind == kind::unresolved || v.kind == kind::censored;
	const bool configuration_unknown = uncertain && v.reason == reason::configuration_unknown;
	if (configuration_unknown ? v.scope.config_id != 0U || v.scope.classifier_version != 0U ||
					    v.scope.policy_version != 0U || v.build_version != 0U ||
					    v.content_version != 0U :
				    !telemetry_encounter_source_is_valid(v.scope) ||
					    !v.build_version || !v.content_version)
		return false;
	if (!telemetry_producer_id_is_valid(v.producer) || !v.sequence ||
	    v.parent_sequence >= v.sequence || !v.scope.environment_id || !v.scope.season_id ||
	    v.scope.zone_vnum != -1 || v.scope.group_key ||
	    v.definition_version != TELEMETRY_BATTLE_RESULT_DEFINITION_VERSION ||
	    v.producer_version != TELEMETRY_BATTLE_RESULT_PRODUCER_VERSION ||
	    !actor_valid(v.target) || (!actor_zero(v.source) && !actor_valid(v.source)) ||
	    !association_valid(v.source_association) || !association_valid(v.target_association) ||
	    (actor_zero(v.source) && !association_zero(v.source_association)) ||
	    !telemetry_quality_mask_is_valid(v.quality_flags) ||
	    ((v.source.quality_flags | v.target.quality_flags) & ~v.quality_flags) ||
	    (v.flags & ~TELEMETRY_BATTLE_RESULT_FLAGS) || v.start_usec > v.at_usec ||
	    v.kind < kind::death_observed || v.kind > kind::censored ||
	    v.authority < authority::native_death || v.authority > authority::lifecycle ||
	    v.reason > reason::receipt_unknown || v.reserved[0] || v.reserved[1] || v.reserved[2] ||
	    v.credited_zone_vnum < -1 || v.from_room_vnum < -1 || v.to_room_vnum < -1)
		return false;
	if (v.start_utc_usec != TELEMETRY_UTC_UNKNOWN && v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    v.at_utc_usec < v.start_utc_usec &&
	    !(v.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY))
		return false;
	bool operation = false;
	for (auto byte : v.operation_id)
		operation |= byte != 0U;
	const bool objective =
		v.kind == kind::objective_requested || v.kind == kind::objective_committed ||
		(v.kind == kind::unresolved && v.authority >= authority::zone_touch_submit &&
		 v.authority <= authority::zone_touch_outbox);
	const auto objective_flags = TELEMETRY_BATTLE_RESULT_RESET_REQUESTED |
				     TELEMETRY_BATTLE_RESULT_RECORD_ZONE |
				     TELEMETRY_BATTLE_RESULT_RECOVERED;
	if (objective)
		return operation && v.credited_zone_vnum >= 0 && v.participant_count >= 1U &&
		       v.participant_count <= 15U && actor_zero(v.source) &&
		       v.parent_sequence == 0U &&
		       v.target.actor.kind == telemetry_combat_actor_kind::player &&
		       v.proof_window_usec == 0U &&
		       (uncertain ? v.reason != reason::none &&
					    (v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) :
				    v.reason == reason::none) &&
		       v.from_room_vnum == -1 && v.to_room_vnum == -1 &&
		       (v.flags & ~(objective_flags | TELEMETRY_BATTLE_RESULT_TRUSTED)) == 0U &&
		       ((v.source_payload_version == 1U && !v.source_object_uid &&
			 (v.flags & TELEMETRY_BATTLE_RESULT_RECORD_ZONE)) ||
			(v.source_payload_version == 2U && v.source_object_uid)) &&
		       (v.kind == kind::unresolved ||
			(v.kind == kind::objective_requested ?
				 v.authority == authority::zone_touch_submit &&
					 !(v.flags & TELEMETRY_BATTLE_RESULT_RECOVERED) :
				 v.authority == authority::zone_touch_receipt ||
					 v.authority == authority::zone_touch_outbox));
	if (operation || v.source_object_uid || v.participant_count || v.source_payload_version ||
	    v.credited_zone_vnum != -1 || (v.flags & objective_flags))
		return false;
	if (uncertain)
		return v.reason != reason::none &&
		       (v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) &&
		       v.proof_window_usec == 0U &&
		       !(v.flags & (TELEMETRY_BATTLE_RESULT_WATCH_COMPLETE |
				    TELEMETRY_BATTLE_RESULT_SAME_SESSION |
				    TELEMETRY_BATTLE_RESULT_NO_OPPONENT)) &&
		       (v.kind != kind::censored || v.parent_sequence != 0U);
	if (v.reason != reason::none)
		return false;
	if (v.kind == kind::death_observed)
		return v.authority == authority::native_death && v.parent_sequence == 0U &&
		       v.proof_window_usec == 0U &&
		       (v.flags &
			~(TELEMETRY_BATTLE_RESULT_TRUSTED | TELEMETRY_BATTLE_RESULT_ARENA)) == 0U &&
		       v.from_room_vnum == -1 && v.to_room_vnum == -1;
	if (!actor_zero(v.source) || !association_zero(v.source_association) ||
	    (v.flags & TELEMETRY_BATTLE_RESULT_ARENA))
		return false;
	const bool moved = (v.flags & TELEMETRY_BATTLE_RESULT_MOVED) && v.from_room_vnum >= 0 &&
			   v.to_room_vnum >= 0 && v.from_room_vnum != v.to_room_vnum;
	if (v.kind == kind::escape_observed)
	{
		const auto proof = TELEMETRY_BATTLE_RESULT_MOVED |
				   TELEMETRY_BATTLE_RESULT_NO_OPPONENT |
				   TELEMETRY_BATTLE_RESULT_SAME_SESSION |
				   TELEMETRY_BATTLE_RESULT_WATCH_COMPLETE;
		return v.authority == authority::bounded_escape_watch && v.parent_sequence != 0U &&
		       moved && v.proof_window_usec == TELEMETRY_BATTLE_ESCAPE_MIN_USEC &&
		       v.at_usec - v.start_usec >= v.proof_window_usec &&
		       (v.flags & proof) == proof &&
		       v.target.actor.kind == telemetry_combat_actor_kind::player &&
		       !association_zero(v.target_association) &&
		       telemetry_session_id_is_valid(v.target.session) &&
		       v.target.session.producer.boot_id == v.producer.boot_id &&
		       v.target.session.producer.process_id == v.producer.process_id;
	}
	if (v.parent_sequence || v.proof_window_usec ||
	    (v.flags & ~(TELEMETRY_BATTLE_RESULT_TRUSTED | TELEMETRY_BATTLE_RESULT_MOVED)))
		return false;
	return (v.kind == kind::flee_movement && v.authority == authority::accepted_flee &&
		moved) ||
	       (v.kind == kind::withdrawal &&
		((v.authority == authority::accepted_retreat && moved) ||
		 (v.authority == authority::accepted_disengage &&
		  !(v.flags & TELEMETRY_BATTLE_RESULT_MOVED) && v.from_room_vnum >= 0 &&
		  v.from_room_vnum == v.to_room_vnum)));
}

constexpr std::uint16_t telemetry_progression_configuration_source_id(std::size_t index) noexcept
{
	return index < 62U  ? index + 1U :
	       index < 124U ? index - 62U + 100U :
	       index < 225U ? index - 124U + 200U :
	       index < 326U ? index - 225U + 400U :
	       index < 337U ? index - 326U + 600U :
			      0U;
}

constexpr bool telemetry_progression_configuration_value_is_valid(
	std::uint16_t id, telemetry_progression_configuration_value_kind type,
	std::uint64_t bits) noexcept
{
	using kind = telemetry_progression_configuration_value_kind;
	if ((id >= 1U && id <= 62U) || id == 600U || (id >= 604U && id <= 606U) ||
	    (id >= 608U && id <= 610U))
		return type == kind::signed_integer;
	if ((id >= 100U && id <= 161U) || (id >= 200U && id <= 300U) ||
	    (id >= 400U && id <= 500U) || id == 607U)
		return type == kind::float32_bits && bits <= UINT32_MAX &&
		       (bits & 0x7f800000U) != 0x7f800000U;
	return id >= 601U && id <= 603U && type == kind::float64_bits &&
	       (bits & 0x7ff0000000000000ULL) != 0x7ff0000000000000ULL;
}

constexpr bool telemetry_progression_configuration_observation_is_valid(
	const telemetry_progression_configuration_observation &v) noexcept
{
	constexpr auto chunks = (TELEMETRY_PROGRESSION_CONFIGURATION_VALUES +
				 TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES - 1U) /
				TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	if (!telemetry_producer_id_is_valid(v.producer) || !v.sequence || !v.root_record_seq ||
	    !v.config_id || !v.environment_id || !v.season_id || !v.build_version ||
	    !v.content_version || !v.classifier_version || !v.policy_version || v.reserved ||
	    v.version != TELEMETRY_PROGRESSION_CONFIGURATION_VERSION ||
	    v.source_inventory_version != TELEMETRY_PROGRESSION_SOURCE_INVENTORY_VERSION ||
	    v.total_values != TELEMETRY_PROGRESSION_CONFIGURATION_VALUES ||
	    v.chunk_count != chunks || v.chunk_index >= chunks ||
	    !(v.digest_words[0] || v.digest_words[1] || v.digest_words[2] || v.digest_words[3]))
		return false;
	const auto first = v.chunk_index * TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	const auto available = TELEMETRY_PROGRESSION_CONFIGURATION_VALUES - first;
	const auto count = available < TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES ?
				   available :
				   TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	if (v.value_count != count)
		return false;
	for (std::size_t index = 0U; index < TELEMETRY_PROGRESSION_CONFIGURATION_CHUNK_VALUES;
	     ++index)
	{
		if (index < count)
		{
			if (v.value_ids[index] !=
				    telemetry_progression_configuration_source_id(first + index) ||
			    !telemetry_progression_configuration_value_is_valid(
				    v.value_ids[index], v.value_kinds[index], v.value_bits[index]))
				return false;
		}
		else if (v.value_ids[index] || v.value_bits[index] ||
			 static_cast<std::uint8_t>(v.value_kinds[index]))
			return false;
	}
	return true;
}

constexpr bool telemetry_progression_optional_key(const telemetry_record_key &key,
						  const telemetry_producer_id &producer) noexcept
{
	return (key.producer.boot_id == 0U && key.producer.process_id == 0U &&
		key.record_seq == 0U) ||
	       (telemetry_record_key_is_valid(key) &&
		(key.producer.boot_id == producer.boot_id &&
		 key.producer.process_id == producer.process_id));
}

constexpr bool telemetry_progression_key_zero(const telemetry_record_key &key) noexcept
{
	return key.producer.boot_id == 0U && key.producer.process_id == 0U && key.record_seq == 0U;
}

constexpr bool telemetry_progression_context_snapshot_is_valid(
	const telemetry_progression_context_snapshot &v) noexcept
{
	using selection = telemetry_progression_rested_selection;
	using application = telemetry_progression_rested_application;
	using assistance = telemetry_progression_assistance;
	if (v.version != TELEMETRY_PROGRESSION_CONTEXT_VERSION || v.reserved != 0U ||
	    v.current_level == 0U || (v.flags & ~TELEMETRY_PCTX_FLAGS) != 0U ||
	    !telemetry_quality_mask_is_valid(v.quality_flags))
		return false;
	const bool configured = v.config_id && v.build_version && v.content_version &&
				v.classifier_version && v.policy_version;
	if (!configured &&
	    (v.config_id || v.build_version || v.content_version || v.classifier_version ||
	     v.policy_version || !(v.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN)))
		return false;
	if (((v.flags & TELEMETRY_PCTX_RESTED_STAFF) &&
	     !(v.flags & TELEMETRY_PCTX_RESTED_PRESENT)) ||
	    ((v.flags & TELEMETRY_PCTX_WELLRESTED_STAFF) &&
	     !(v.flags & TELEMETRY_PCTX_WELLRESTED_PRESENT)))
		return false;
	if (v.flags & TELEMETRY_PCTX_SELECTION_KNOWN)
	{
		if (v.rested_selection < selection::none ||
		    v.rested_selection > selection::wellrested)
			return false;
		if (v.flags & TELEMETRY_PCTX_AFFECTS_COMPLETE)
		{
			const bool automatic = v.flags & TELEMETRY_PCTX_AUTOMATIC_RESTED;
			const bool alive = v.flags & TELEMETRY_PCTX_ALIVE;
			const bool well = (v.flags & TELEMETRY_PCTX_WELLRESTED_PRESENT) &&
					  (automatic ||
					   (alive && (v.flags & TELEMETRY_PCTX_WELLRESTED_STAFF)));
			const bool rested =
				(v.flags & TELEMETRY_PCTX_RESTED_PRESENT) &&
				(automatic || (alive && (v.flags & TELEMETRY_PCTX_RESTED_STAFF)));
			if (v.rested_selection != (well	  ? selection::wellrested :
						   rested ? selection::rested :
							    selection::none))
				return false;
		}
	}
	else if (v.rested_selection != selection::unknown)
		return false;
	if (v.flags & TELEMETRY_PCTX_APPLICATION_KNOWN)
	{
		if (v.rested_application < application::resurrection_exempt ||
		    v.rested_application > application::wellrested)
			return false;
		if ((v.flags & TELEMETRY_PCTX_SELECTION_KNOWN) &&
		    v.rested_application != application::resurrection_exempt &&
		    static_cast<unsigned>(v.rested_application) !=
			    static_cast<unsigned>(v.rested_selection) + 1U)
			return false;
	}
	else if (v.rested_application != application::unknown)
		return false;
	if (v.flags & TELEMETRY_PCTX_THRESHOLD_KNOWN)
	{
		if (!configured || !v.next_threshold_xp || v.threshold_catalog_version != 1U ||
		    v.current_level == std::numeric_limits<std::uint16_t>::max() ||
		    v.threshold_level != v.current_level + 1U)
			return false;
	}
	else if (v.next_threshold_xp || v.threshold_level || v.threshold_catalog_version)
		return false;
	if (!(v.flags & TELEMETRY_PCTX_BUILD_KNOWN))
	{
		if (v.primary_class_mask || v.secondary_class_mask || v.specialization || v.race ||
		    v.faction)
			return false;
		for (std::size_t index = 0U; index < TELEMETRY_PROGRESSION_CONTEXT_STATS; ++index)
			if (v.base_stats[index] || v.effective_stats[index])
				return false;
	}
	if (v.flags & TELEMETRY_PCTX_GROUP_ROSTER_KNOWN)
	{
		if (!v.formal_group_size)
			return false;
	}
	else if (v.formal_group_size)
		return false;
	const bool has_digest = v.configuration_digest_words[0] ||
				v.configuration_digest_words[1] ||
				v.configuration_digest_words[2] || v.configuration_digest_words[3];
	if (v.flags & TELEMETRY_PCTX_CONFIG_CATALOG_KNOWN)
	{
		if (!configured || !v.configuration_record_seq || !has_digest)
			return false;
	}
	else if (v.configuration_record_seq || has_digest)
		return false;
	if (!(v.flags & TELEMETRY_PCTX_DECISION_POLICY_KNOWN) &&
	    (v.decision_level_cap || v.decision_good_assistance_gap ||
	     v.decision_evil_assistance_gap || v.decision_max_exp_level ||
	     (v.flags & TELEMETRY_PCTX_HARDCORE_BYPASS)))
		return false;
	if ((v.flags & TELEMETRY_PCTX_HARDCORE_BYPASS) && !(v.flags & TELEMETRY_PCTX_HARDCORE))
		return false;
	if ((v.flags & TELEMETRY_PCTX_STORAGE_GATE_PASSED) &&
	    !(v.flags & TELEMETRY_PCTX_STORAGE_GATE_KNOWN))
		return false;
	if (v.flags & TELEMETRY_PCTX_ASSISTANCE_KNOWN)
	{
		if (v.assistance < assistance::solo_kill_share ||
		    v.assistance > assistance::no_assistance_gate)
			return false;
		if (v.assistance == assistance::solo_kill_share &&
		    (!(v.flags & TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN) ||
		     v.eligible_group_size != 1U || v.highest_group_level != v.current_level))
			return false;
		if (v.assistance == assistance::group_kill_share &&
		    (!(v.flags & TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN) ||
		     !v.eligible_group_size || v.highest_group_level < v.current_level))
			return false;
		if (v.assistance == assistance::group_tanking &&
		    (!(v.flags & TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN) ||
		     v.eligible_group_size < 2U || v.highest_group_level))
			return false;
	}
	else if (v.assistance != assistance::unknown ||
		 (v.flags & TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN) || v.assistance_level)
		return false;
	if (!(v.flags & TELEMETRY_PCTX_GROUP_ELIGIBILITY_KNOWN) &&
	    (v.eligible_group_size || v.highest_group_level))
		return false;
	return true;
}

constexpr bool telemetry_progression_context_observation_is_valid(
	const telemetry_progression_context_observation &v) noexcept
{
	using boundary = telemetry_progression_context_boundary;
	if (!telemetry_producer_id_is_valid(v.producer) || !v.sequence ||
	    !telemetry_session_ref_is_valid(v.session) ||
	    !telemetry_connection_reference_is_valid(v.connection) ||
	    !telemetry_progression_optional_key(v.source_record, v.producer) ||
	    !telemetry_progression_optional_key(v.ownership_record, v.producer) ||
	    (v.account_token && telemetry_progression_key_zero(v.ownership_record)) ||
	    v.source_inventory_version != TELEMETRY_PROGRESSION_SOURCE_INVENTORY_VERSION ||
	    v.reserved || !telemetry_progression_context_snapshot_is_valid(v.context) ||
	    v.start_monotonic_usec > v.at_monotonic_usec || v.boundary < boundary::baseline ||
	    v.boundary > boundary::unknown)
		return false;
	if (!telemetry_connection_id_is_zero(v.connection) &&
	    (v.connection.producer.boot_id != v.producer.boot_id ||
	     v.connection.producer.process_id != v.producer.process_id))
		return false;
	if (v.start_utc_usec != TELEMETRY_UTC_UNKNOWN && v.at_utc_usec != TELEMETRY_UTC_UNKNOWN &&
	    v.start_utc_usec > v.at_utc_usec &&
	    !(v.context.quality_flags & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY))
		return false;
	const bool has_source = !telemetry_progression_key_zero(v.source_record);
	if (has_source != (v.source_kind != telemetry_record_kind::invalid))
		return false;
	if (v.boundary == boundary::baseline && has_source)
		return false;
	if (v.boundary == boundary::exposure && (v.source_kind != telemetry_record_kind::interval ||
						 v.start_monotonic_usec == v.at_monotonic_usec))
		return false;
	if ((v.boundary == boundary::experience || v.boundary == boundary::level) &&
	    v.source_kind != telemetry_record_kind::progression)
		return false;
	if (v.boundary == boundary::lifecycle_cut &&
	    v.source_kind != telemetry_record_kind::session_lifecycle)
		return false;
	if (v.source_kind != telemetry_record_kind::invalid &&
	    v.source_kind != telemetry_record_kind::interval &&
	    v.source_kind != telemetry_record_kind::progression &&
	    v.source_kind != telemetry_record_kind::session_lifecycle)
		return false;
	if ((v.context.flags & TELEMETRY_PCTX_CONTIGUOUS_EXPOSURE) &&
	    v.boundary != boundary::exposure)
		return false;
	if ((v.context.flags &
	     (TELEMETRY_PCTX_APPLICATION_KNOWN | TELEMETRY_PCTX_STORAGE_GATE_KNOWN |
	      TELEMETRY_PCTX_DECISION_POLICY_KNOWN)) &&
	    v.boundary != boundary::experience)
		return false;
	return v.boundary != boundary::unknown ||
	       ((v.context.quality_flags & TELEMETRY_QUALITY_CONTEXT_UNKNOWN) &&
		!(v.context.flags & TELEMETRY_PCTX_CONTIGUOUS_EXPOSURE));
}

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
	case telemetry_record_kind::battle:
		return telemetry_battle_fact_is_valid(record.payload.battle) &&
		       record.header.key.producer.boot_id ==
			       record.payload.battle.battle.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.battle.battle.producer.process_id &&
		       record.header.occurrence_utc_usec == record.payload.battle.at_utc_usec;
	case telemetry_record_kind::battle_contribution:
		return telemetry_battle_contribution_payload_is_valid(
			       record.payload.battle_contribution) &&
		       record.header.key.producer.boot_id ==
			       record.payload.battle_contribution.context.battle.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.battle_contribution.context.battle.producer
				       .process_id &&
		       record.header.occurrence_utc_usec ==
			       record.payload.battle_contribution.cut.decision_utc_usec;
	case telemetry_record_kind::battle_build:
		return telemetry_battle_build_observation_is_valid(record.payload.battle_build) &&
		       record.header.key.producer.boot_id ==
			       record.payload.battle_build.battle.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.battle_build.battle.producer.process_id &&
		       record.header.occurrence_utc_usec == record.payload.battle_build.at_utc_usec;
	case telemetry_record_kind::control:
		return telemetry_control_observation_is_valid(record.payload.control) &&
		       record.header.key.producer.boot_id ==
			       record.payload.control.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.control.producer.process_id &&
		       record.header.occurrence_utc_usec ==
			       record.payload.control.decision_utc_usec;
	case telemetry_record_kind::battle_result:
		return telemetry_battle_result_observation_is_valid(record.payload.battle_result) &&
		       record.header.key.producer.boot_id ==
			       record.payload.battle_result.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.battle_result.producer.process_id &&
		       record.header.occurrence_utc_usec ==
			       record.payload.battle_result.at_utc_usec;
	case telemetry_record_kind::progression_context:
		return telemetry_progression_context_observation_is_valid(
			       record.payload.progression_context) &&
		       record.header.key.producer.boot_id ==
			       record.payload.progression_context.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.progression_context.producer.process_id &&
		       record.header.occurrence_utc_usec ==
			       record.payload.progression_context.at_utc_usec;
	case telemetry_record_kind::progression_configuration:
		return telemetry_progression_configuration_observation_is_valid(
			       record.payload.progression_configuration) &&
		       record.header.key.producer.boot_id ==
			       record.payload.progression_configuration.producer.boot_id &&
		       record.header.key.producer.process_id ==
			       record.payload.progression_configuration.producer.process_id &&
		       record.header.occurrence_utc_usec ==
			       record.payload.progression_configuration.at_utc_usec &&
		       (record.payload.progression_configuration.chunk_index == 0U ?
				record.header.key.record_seq ==
					record.payload.progression_configuration.root_record_seq :
				record.header.key.record_seq >
					record.payload.progression_configuration.root_record_seq);
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
