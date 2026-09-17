#ifndef DURIS_TELEMETRY_RUNTIME_H
#define DURIS_TELEMETRY_RUNTIME_H

#include "telemetry/telemetry_config.h"
#include "telemetry/telemetry_config_private.h"
#include "telemetry/telemetry_progression.h"
#include "telemetry/telemetry_types.h"

#include <cstdint>
#include <type_traits>

/*
 * Thread/lifetime contract:
 * - The runtime initializer starts one private worker.  The lifecycle owner must
 *   call shutdown off gameplay hooks; shutdown requests stop, performs the
 *   deadline-bounded stop request.  If the worker does not finish by the
 *   deadline, shutdown returns stopping and retains every binding and state for
 *   telemetry_runtime_final_reap().  No worker is detached and no game hook
 *   joins it.  final_reap joins the worker and only then closes the private
 *   repository connection; it may block and is an off-game-thread lifecycle
 *   operation.
 * - Runtime capture, config publication, and enqueue are game-thread bounded
 *   value copies only: no SQL, heap-allocation wait, blocking wait, or worker
 *   join is permitted.  The transport owns a record only after admission.
 * - transport_pulse/drain and repository apply/config are worker-only bounded
 *   operations.  Health APIs return synchronized cached copies; they do not
 *   query SQL on callers.
 * - request_stop is nonblocking.  The repository connector must honor the
 *   worker's cancellation/close contract; a deadline does not preempt a
 *   connector call that violates that contract.  The timed shutdown request
 *   never calls repository teardown synchronously; final_reap owns that
 *   possibly blocking step after the worker is joined.
 */
enum class telemetry_runtime_outcome : std::uint8_t
{
	accepted = 0,
	disabled = 1,
	not_initialized = 2,
	invalid = 3,
	queue_full = 4,
	stopping = 5,
	flatfile_disabled = 6,
};

struct telemetry_capture_result
{
	telemetry_runtime_outcome outcome;
	telemetry_queue_admission admission;
	std::uint16_t records_emitted;
	std::uint16_t records_dropped;
	telemetry_record_key first_record;
	telemetry_record_key last_record;
	telemetry_quality_mask quality_flags;
};

struct telemetry_runtime_options
{
	telemetry_config_snapshot config;
	telemetry_producer_id producer;
	/* Optional owner-supplied effective-property boundary.  When present, the
	 * runtime rebuilds the typed snapshot at bootstrap and after the
	 * post-apply_properties reload notification.  A missing boundary is kept
	 * for explicit prebuilt test/owner snapshots and never fabricates a
	 * property version. */
	telemetry_config_property_capture property_capture;
	std::uint8_t property_capture_enabled;
	std::uint8_t reserved[7];
};

struct char_data;
struct descriptor_data;

/* Typed, value-only evidence accepted from gameplay hooks.  The enum values
 * intentionally mirror telemetry_activity_evidence_kind without including the
 * activity service header here (telemetry_session.h includes this header). */
enum class telemetry_runtime_evidence_kind : std::uint8_t
{
	player_action = 1,
	movement = 2,
	interaction = 3,
	communication = 4,
	combat_participation = 5,
	automatic_combat = 6,
	linkdead = 7,
};

struct telemetry_runtime_evidence
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_runtime_evidence_kind kind;
	std::uint8_t reserved[3];
	telemetry_quality_mask quality_flags;
};

/* Session identity is supplied by the owner; runtime does not inspect it. */
struct telemetry_session_enter
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_dimensions dimensions;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/* Bounded copyover state; no monotonic timestamp crosses process incarnations.
 * Export after accounting through the handoff cut. Revision is the last allocated
 * checkpoint revision (including dropped checkpoints), zero if none was sealed.
 * Import retains cumulative totals and starts from entry's NEW clock anchor.
 * This is observational state; failure never vetoes gameplay copyover. */
struct telemetry_session_handoff
{
	telemetry_session_ref session;
	telemetry_producer_id previous_producer;
	telemetry_checkpoint_revision last_checkpoint_revision;
	telemetry_cumulative_counters cumulative;
	telemetry_quality_mask quality_flags;
};

struct telemetry_handoff_result
{
	telemetry_runtime_outcome outcome;
	telemetry_session_handoff handoff;
};

struct telemetry_session_resume
{
	telemetry_session_handoff handoff;
	telemetry_session_enter entry;
};

struct telemetry_session_exit
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_session_end_reason reason;
	std::uint8_t reserved[3];
	telemetry_quality_mask quality_flags;
};

/*
 * Each delta is elapsed time since the preceding update.  Active, idle, and
 * unknown sum to connected_delta_usec; connected and linkdead sum to
 * resident_delta_usec.  A detached resident may use an all-zero connection
 * id for linkdead-only updates.  The runtime later emits absolute totals.
 */
struct telemetry_counter_update
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_duration_usec connected_delta_usec;
	telemetry_duration_usec active_delta_usec;
	telemetry_duration_usec idle_delta_usec;
	telemetry_duration_usec unknown_delta_usec;
	telemetry_duration_usec resident_delta_usec;
	telemetry_duration_usec linkdead_delta_usec;
	telemetry_quality_mask quality_flags;
};

/* A meaningful dimension/classifier change seals the prior interval. */
struct telemetry_context_update
{
	telemetry_session_ref session;
	telemetry_connection_id connection;
	telemetry_monotonic_usec at_monotonic_usec;
	telemetry_utc_usec at_utc_usec;
	telemetry_dimensions dimensions;
	telemetry_interval_category category;
	telemetry_activity_context context;
	telemetry_context_quality context_quality;
	std::uint8_t reserved;
	telemetry_config_id config_id;
	std::uint32_t classifier_version;
	std::uint32_t policy_version;
	telemetry_quality_mask quality_flags;
};

/* slot is used for staggered cohorts; UTC is reporting time, not duration. */
struct telemetry_pulse_request
{
	telemetry_monotonic_usec now_monotonic_usec;
	telemetry_utc_usec occurrence_utc_usec;
	std::uint16_t slot;
	std::uint16_t reserved;
};

struct telemetry_pulse_result
{
	telemetry_runtime_outcome outcome;
	std::uint8_t reserved[3];
	std::uint32_t sessions_considered;
	std::uint32_t intervals_sealed;
	std::uint32_t checkpoints_sealed;
	std::uint32_t lifecycle_records;
	std::uint32_t records_dropped;
	telemetry_quality_mask quality_flags;
};

struct telemetry_shutdown_request
{
	/* Absolute monotonic deadline; no promise is made that SQL can be forced
	 * to finish before it.  final_flush is best effort and never blocks gameplay. */
	telemetry_monotonic_usec deadline_monotonic_usec;
	std::uint8_t final_flush;
	std::uint8_t reserved[7];
};

constexpr bool
telemetry_runtime_evidence_is_valid(const telemetry_runtime_evidence &evidence) noexcept
{
	return telemetry_session_ref_is_valid(evidence.session) &&
	       telemetry_connection_reference_is_valid(evidence.connection) &&
	       static_cast<std::uint8_t>(evidence.kind) >=
		       static_cast<std::uint8_t>(telemetry_runtime_evidence_kind::player_action) &&
	       static_cast<std::uint8_t>(evidence.kind) <=
		       static_cast<std::uint8_t>(telemetry_runtime_evidence_kind::linkdead) &&
	       evidence.reserved[0] == 0U && evidence.reserved[1] == 0U &&
	       evidence.reserved[2] == 0U &&
	       telemetry_quality_mask_is_valid(evidence.quality_flags) &&
	       (evidence.kind != telemetry_runtime_evidence_kind::linkdead ||
		telemetry_connection_id_is_zero(evidence.connection));
}

constexpr bool telemetry_session_enter_is_valid(const telemetry_session_enter &enter) noexcept
{
	return telemetry_session_ref_is_valid(enter.session) &&
	       telemetry_connection_id_is_valid(enter.connection) && enter.config_id != 0U &&
	       enter.classifier_version != 0U && enter.policy_version != 0U &&
	       telemetry_dimensions_are_valid(enter.dimensions) &&
	       telemetry_quality_mask_is_valid(enter.quality_flags);
}

constexpr bool telemetry_session_resume_is_valid(const telemetry_session_resume &resume) noexcept
{
	const auto &prior = resume.handoff;
	const auto &entry = resume.entry;
	const auto &a = prior.session;
	const auto &b = entry.session;
	return telemetry_session_enter_is_valid(entry) && telemetry_session_ref_is_valid(a) &&
	       a.id.producer.boot_id == b.id.producer.boot_id &&
	       a.id.producer.process_id == b.id.producer.process_id &&
	       a.id.session_seq == b.id.session_seq && a.subject_id == b.subject_id &&
	       a.pid == b.pid && a.season_id == b.season_id &&
	       a.environment_id == b.environment_id &&
	       telemetry_producer_id_is_valid(prior.previous_producer) &&
	       (prior.previous_producer.boot_id != entry.connection.producer.boot_id ||
		prior.previous_producer.process_id != entry.connection.producer.process_id) &&
	       prior.last_checkpoint_revision <
		       std::numeric_limits<telemetry_checkpoint_revision>::max() &&
	       telemetry_cumulative_counters_are_valid(prior.cumulative) &&
	       telemetry_quality_mask_is_valid(prior.quality_flags) &&
	       (entry.quality_flags & prior.quality_flags) == prior.quality_flags;
}

constexpr bool telemetry_session_exit_is_valid(const telemetry_session_exit &exit) noexcept
{
	return telemetry_session_ref_is_valid(exit.session) &&
	       telemetry_connection_reference_is_valid(exit.connection) &&
	       telemetry_session_end_reason_is_valid(exit.reason) &&
	       exit.reason != telemetry_session_end_reason::unknown && exit.reserved[0] == 0U &&
	       exit.reserved[1] == 0U && exit.reserved[2] == 0U &&
	       telemetry_quality_mask_is_valid(exit.quality_flags);
}

constexpr bool telemetry_counter_update_is_valid(const telemetry_counter_update &update) noexcept
{
	if (!telemetry_session_ref_is_valid(update.session) ||
	    !telemetry_connection_reference_is_valid(update.connection))
		return false;
	telemetry_cumulative_counters deltas = {
		update.connected_delta_usec, update.active_delta_usec,	 update.idle_delta_usec,
		update.unknown_delta_usec,   update.resident_delta_usec, update.linkdead_delta_usec
	};
	if (!telemetry_cumulative_counters_are_valid(deltas) ||
	    !telemetry_quality_mask_is_valid(update.quality_flags))
		return false;
	if (telemetry_connection_id_is_zero(update.connection))
		return update.connected_delta_usec == 0U && update.active_delta_usec == 0U &&
		       update.idle_delta_usec == 0U && update.unknown_delta_usec == 0U;
	return update.linkdead_delta_usec == 0U &&
	       update.resident_delta_usec == update.connected_delta_usec;
}

constexpr bool telemetry_context_update_is_valid(const telemetry_context_update &update) noexcept
{
	if (!telemetry_dimensions_are_valid(update.dimensions))
		return false;
	if (update.context == telemetry_activity_context::overflow_unknown &&
	    (!telemetry_dimensions_are_unknown(update.dimensions) ||
	     update.context_quality != telemetry_context_quality::overflow ||
	     (update.quality_flags & TELEMETRY_QUALITY_CONTEXT_OVERFLOW) == 0U ||
	     (update.quality_flags & TELEMETRY_QUALITY_DIMENSION_UNKNOWN) == 0U))
		return false;
	return telemetry_session_ref_is_valid(update.session) &&
	       telemetry_connection_id_is_valid(update.connection) &&
	       telemetry_interval_category_is_valid(update.category) &&
	       update.category != telemetry_interval_category::resident_linkdead &&
	       telemetry_activity_context_is_valid(update.context) &&
	       telemetry_context_quality_is_valid(update.context_quality) &&
	       update.reserved == 0U && update.config_id != 0U && update.classifier_version != 0U &&
	       update.policy_version != 0U && telemetry_quality_mask_is_valid(update.quality_flags);
}

constexpr bool telemetry_pulse_request_is_valid(const telemetry_pulse_request &pulse) noexcept
{
	return pulse.reserved == 0U;
}

constexpr bool
telemetry_shutdown_request_is_valid(const telemetry_shutdown_request &request) noexcept
{
	for (const std::uint8_t byte : request.reserved)
		if (byte != 0U)
			return false;
	return request.final_flush <= 1U;
}

telemetry_runtime_outcome telemetry_runtime_init(telemetry_runtime_options options);
/* Default-off bootstrap with independently random nonzero incarnation tokens.
 * No time/PID fallback on entropy failure. Enabled incarnations cannot reuse a
 * producer pair after final reap; request new options for a new lifetime. */
telemetry_runtime_options telemetry_runtime_default_options(void);
/* Opt-in environment source. Invalid or unsupported values fail closed to the
 * same disabled options as the default bootstrap. */
telemetry_runtime_options telemetry_runtime_options_from_environment(void);
telemetry_producer_id telemetry_runtime_producer_copy(void);
bool telemetry_runtime_next_connection(telemetry_connection_id *connection) noexcept;
bool telemetry_runtime_next_session(telemetry_session_id *session) noexcept;
bool telemetry_runtime_now(telemetry_monotonic_usec *monotonic_usec,
			   telemetry_utc_usec *utc_usec) noexcept;

/*
 * Value-only server-boundary adapters.  They keep gameplay hooks free of
 * session/classifier state-machine construction and never make telemetry
 * availability a prerequisite for the caller's teardown path.
 */
telemetry_capture_result telemetry_runtime_game_enter(struct char_data *character,
						      struct descriptor_data *descriptor);
/* Captures current traced dimensions and publishes a bounded context update.
 * Call after game_enter or copyover resume; no SQL or filesystem work occurs. */
telemetry_capture_result telemetry_runtime_game_context(struct char_data *character,
							struct descriptor_data *descriptor);
/* Copyover adapters preserve the logical session key while allocating a new
 * connection in the replacement process. A null/zero handoff is an explicit
 * absent handoff and starts a new session with an unclosed-tail marker. */
telemetry_handoff_result telemetry_runtime_game_handoff_copy(struct char_data *character);
telemetry_capture_result
telemetry_runtime_game_session_resume(struct char_data *character,
				      struct descriptor_data *descriptor,
				      const telemetry_session_handoff *handoff);
telemetry_capture_result
telemetry_runtime_game_connection_transition(struct char_data *character,
					     struct descriptor_data *descriptor,
					     telemetry_connection_transition_kind kind);
telemetry_capture_result telemetry_runtime_game_session_exit(struct char_data *character,
							     struct descriptor_data *descriptor,
							     telemetry_session_end_reason reason);
telemetry_capture_result telemetry_runtime_game_evidence(struct char_data *character,
								 struct descriptor_data *descriptor,
								 telemetry_runtime_evidence_kind kind);
/* Captures an XP storage or level transition snapshot with current session,
 * connection, dimensions and effective config copied by the runtime. */
telemetry_capture_result telemetry_runtime_game_progression(
	struct char_data *character, struct descriptor_data *descriptor,
	telemetry_progression_observation observation);
std::uint16_t telemetry_runtime_pulse_slot_count(void) noexcept;
telemetry_capture_result telemetry_runtime_session_enter(telemetry_session_enter enter);
/* Copyover owner only, after capturing its complete handoff batch while normal
 * game-loop admissions are paused. Waits at most 250ms for worker durability;
 * never stops/joins the writer. On failure serialize absent handoffs. Queue
 * emptiness alone is insufficient: in-flight/rejected records are checked. */
telemetry_runtime_outcome telemetry_runtime_flush_for_copyover(telemetry_monotonic_usec deadline);
/* Game-thread bounded export; accepted is a candidate until the batch flush. */
telemetry_handoff_result telemetry_runtime_session_handoff_copy(telemetry_session_ref session);
/* Seeds an absent session or carries a valid copyover handoff and emits
 * connection_attached, NOT session_entered. It preserves revision/counters/
 * quality, uses the new anchor, never counts handoff downtime, and does not
 * require a second attach afterwards. */
telemetry_capture_result telemetry_runtime_session_resume(telemetry_session_resume resume);
/* Explicit attach/detach on an already initialized session, never logical exit.
 * copyover_resumed is emitted by session_resume; calling this alone cannot seed
 * missing session state and must return invalid for an absent runtime session. */
telemetry_capture_result
telemetry_runtime_connection_transition(telemetry_connection_transition transition);
/* Runtime classifier delivery only: the cut and category totals must match
 * the owned activity state. Independent elapsed-time producers are rejected. */
telemetry_capture_result telemetry_runtime_update_counters(telemetry_counter_update update);
telemetry_capture_result telemetry_runtime_record_evidence(telemetry_runtime_evidence evidence);
telemetry_capture_result telemetry_runtime_update_context(telemetry_context_update update);
telemetry_pulse_result telemetry_runtime_pulse(telemetry_pulse_request pulse);
telemetry_capture_result telemetry_runtime_session_exit(telemetry_session_exit exit);
/*
 * The config owner uses the same bounded control-reserve queue as other
 * control records.  If admission fails, no interval may claim that config_id;
 * capture remains unknown/degraded until a configuration record is accepted.
 */
telemetry_capture_result telemetry_config_publish(telemetry_config_snapshot config);
/* Requests stop and waits at most until request.deadline for the worker to
 * report done. accepted means the worker reached done before that deadline;
 * the worker remains joinable until final_reap. stopping retains all state. */
telemetry_runtime_outcome telemetry_runtime_shutdown(telemetry_shutdown_request request);
/* Mandatory lifecycle completion after shutdown; joins/reaps the worker and
 * performs repository/transport teardown. This may block on an in-flight
 * repository callback and must run off gameplay hooks, before global SQL
 * teardown. Idempotent after completion. */
telemetry_runtime_outcome telemetry_runtime_final_reap(void);
telemetry_health_snapshot telemetry_runtime_health_copy(void);

static_assert(std::is_trivially_copyable_v<telemetry_session_handoff>);
static_assert(std::is_trivially_copyable_v<telemetry_session_resume>);
static_assert(sizeof(telemetry_session_resume) <= TELEMETRY_RECORD_MAX_BYTES);
static_assert(std::is_trivially_copyable_v<telemetry_capture_result>);
static_assert(std::is_trivially_copyable_v<telemetry_runtime_options>);
static_assert(std::is_trivially_copyable_v<telemetry_counter_update>);
static_assert(std::is_standard_layout_v<telemetry_counter_update>);

#endif
