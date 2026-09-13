#include "telemetry/telemetry_activity_private.h"

#include <limits>

namespace
{

constexpr telemetry_record_key ZERO_RECORD_KEY = {};
constexpr telemetry_session_ref ZERO_SESSION_REF = {};
constexpr telemetry_connection_id ZERO_CONNECTION_ID = {};
constexpr telemetry_dimensions UNKNOWN_DIMENSIONS = { 0U, 0U, 0U, 0U, -1, 0U };
constexpr telemetry_duration_usec CLOCK_SKEW_TOLERANCE_USEC = 1'000'000ULL;

bool producer_equal(const telemetry_producer_id &left, const telemetry_producer_id &right) noexcept
{
	return left.boot_id == right.boot_id && left.process_id == right.process_id;
}

bool session_id_equal(const telemetry_session_id &left, const telemetry_session_id &right) noexcept
{
	return producer_equal(left.producer, right.producer) &&
	       left.session_seq == right.session_seq;
}

bool session_equal(const telemetry_session_ref &left, const telemetry_session_ref &right) noexcept
{
	return session_id_equal(left.id, right.id) && left.subject_id == right.subject_id &&
	       left.pid == right.pid && left.season_id == right.season_id &&
	       left.environment_id == right.environment_id;
}

bool connection_equal(const telemetry_connection_id &left,
		      const telemetry_connection_id &right) noexcept
{
	return producer_equal(left.producer, right.producer) &&
	       left.connection_seq == right.connection_seq;
}

bool dimensions_equal(const telemetry_dimensions &left, const telemetry_dimensions &right) noexcept
{
	return left.level_band == right.level_band && left.class_id == right.class_id &&
	       left.race_id == right.race_id && left.faction_id == right.faction_id &&
	       left.zone_vnum == right.zone_vnum && left.group_size == right.group_size;
}

bool evidence_equal(const telemetry_activity_evidence &left,
		    const telemetry_activity_evidence &right) noexcept
{
	return session_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec && left.kind == right.kind &&
	       left.reserved[0] == right.reserved[0] && left.reserved[1] == right.reserved[1] &&
	       left.reserved[2] == right.reserved[2] && left.quality_flags == right.quality_flags;
}

bool context_equal(const telemetry_activity_context_snapshot &left,
		   const telemetry_activity_context_snapshot &right) noexcept
{
	return session_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec &&
	       dimensions_equal(left.dimensions, right.dimensions) &&
	       left.context_flags == right.context_flags && left.config_id == right.config_id &&
	       left.classifier_version == right.classifier_version &&
	       left.policy_version == right.policy_version &&
	       left.quality_flags == right.quality_flags;
}

bool transition_equal(const telemetry_connection_transition &left,
		      const telemetry_connection_transition &right) noexcept
{
	return session_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec && left.kind == right.kind &&
	       left.reserved[0] == right.reserved[0] && left.reserved[1] == right.reserved[1] &&
	       left.reserved[2] == right.reserved[2] && left.quality_flags == right.quality_flags;
}

bool exit_equal(const telemetry_session_exit &left, const telemetry_session_exit &right) noexcept
{
	return session_equal(left.session, right.session) &&
	       connection_equal(left.connection, right.connection) &&
	       left.at_monotonic_usec == right.at_monotonic_usec &&
	       left.at_utc_usec == right.at_utc_usec && left.reason == right.reason &&
	       left.reserved[0] == right.reserved[0] && left.reserved[1] == right.reserved[1] &&
	       left.reserved[2] == right.reserved[2] && left.quality_flags == right.quality_flags;
}

bool config_equal(const telemetry_config_snapshot &left,
		  const telemetry_config_snapshot &right) noexcept
{
	if (left.schema_version != right.schema_version || left.reserved != right.reserved ||
	    left.config_id != right.config_id || left.revision != right.revision ||
	    left.build_version != right.build_version ||
	    left.content_version != right.content_version ||
	    left.property_version != right.property_version ||
	    left.classifier_version != right.classifier_version ||
	    left.policy_version != right.policy_version || left.season_id != right.season_id ||
	    left.environment_id != right.environment_id ||
	    left.effective_utc_usec != right.effective_utc_usec ||
	    left.interval_usec != right.interval_usec ||
	    left.checkpoint_interval_usec != right.checkpoint_interval_usec ||
	    left.active_window_usec != right.active_window_usec ||
	    left.context_segments_per_minute != right.context_segments_per_minute ||
	    left.pulse_slot_count != right.pulse_slot_count || left.backend != right.backend ||
	    left.enabled != right.enabled)
		return false;
	for (std::size_t index = 0U; index < TELEMETRY_CONFIG_FINGERPRINT_BYTES; ++index)
		if (left.fingerprint[index] != right.fingerprint[index])
			return false;
	return true;
}

bool add_u64(std::uint64_t &target, std::uint64_t amount) noexcept
{
	if (amount > std::numeric_limits<std::uint64_t>::max() - target)
		return false;
	target += amount;
	return true;
}

void saturating_increment(std::uint64_t &target) noexcept
{
	if (target != std::numeric_limits<std::uint64_t>::max())
		++target;
}

bool cumulative_add(telemetry_cumulative_counters &total,
		    const telemetry_cumulative_counters &delta) noexcept
{
	telemetry_cumulative_counters candidate = total;
	if (!add_u64(candidate.connected_usec, delta.connected_usec) ||
	    !add_u64(candidate.active_usec, delta.active_usec) ||
	    !add_u64(candidate.idle_usec, delta.idle_usec) ||
	    !add_u64(candidate.unknown_usec, delta.unknown_usec) ||
	    !add_u64(candidate.resident_usec, delta.resident_usec) ||
	    !add_u64(candidate.linkdead_usec, delta.linkdead_usec) ||
	    !telemetry_cumulative_counters_are_valid(candidate))
		return false;
	total = candidate;
	return true;
}

telemetry_activity_result result_with(telemetry_activity_outcome outcome) noexcept
{
	telemetry_activity_result result{};
	result.outcome = outcome;
	return result;
}

void result_note_record(telemetry_activity_result &result, const telemetry_record_key &key,
			bool accepted) noexcept
{
	if (result.records_attempted != std::numeric_limits<std::uint16_t>::max())
		++result.records_attempted;
	if (accepted)
	{
		if (result.records_accepted != std::numeric_limits<std::uint16_t>::max())
			++result.records_accepted;
	}
	else if (result.records_dropped != std::numeric_limits<std::uint16_t>::max())
		++result.records_dropped;
	(void)key;
}

void result_note_unallocated(telemetry_activity_result &result) noexcept
{
	if (result.records_dropped != std::numeric_limits<std::uint16_t>::max())
		++result.records_dropped;
}

void result_note_interval(telemetry_activity_result &result) noexcept
{
	if (result.intervals_sealed != std::numeric_limits<std::uint16_t>::max())
		++result.intervals_sealed;
}

void result_add_delta(telemetry_activity_result &result, const telemetry_session_ref &session,
		      const telemetry_connection_id &connection,
		      telemetry_monotonic_usec at_monotonic_usec,
		      const telemetry_cumulative_counters &delta) noexcept
{
	if (delta.connected_usec == 0U && delta.resident_usec == 0U)
		return;
	if (result.has_delta == 0U)
	{
		result.has_delta = 1U;
		result.delta = {};
		result.delta.session = session;
		result.delta.connection = connection;
	}
	else if (!session_equal(result.delta.session, session) ||
		 connection_equal(result.delta.connection, connection) == false)
	{
		/* A single operation is expected to cover one connection.  Do not merge
		 * unlike identities into a misleading typed delta. */
		result.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
		return;
	}
	(void)add_u64(result.delta.connected_delta_usec, delta.connected_usec);
	(void)add_u64(result.delta.active_delta_usec, delta.active_usec);
	(void)add_u64(result.delta.idle_delta_usec, delta.idle_usec);
	(void)add_u64(result.delta.unknown_delta_usec, delta.unknown_usec);
	(void)add_u64(result.delta.resident_delta_usec, delta.resident_usec);
	(void)add_u64(result.delta.linkdead_delta_usec, delta.linkdead_usec);
	result.delta.at_monotonic_usec = at_monotonic_usec;
	result.delta.quality_flags |= result.quality_flags;
}

void result_from_slot(telemetry_activity_result &result,
		      const telemetry_activity_slot &slot) noexcept
{
	result.cumulative = slot.cumulative;
	result.quality_flags |= slot.quality_flags;
	if (result.has_delta != 0U)
		result.delta.quality_flags |= result.quality_flags;
}

void set_failure(telemetry_activity_result &result, telemetry_activity_outcome outcome) noexcept
{
	if (outcome == telemetry_activity_outcome::allocator_exhausted ||
	    outcome == telemetry_activity_outcome::sink_rejected)
		result.outcome = outcome;
	else if (result.outcome == telemetry_activity_outcome::accepted ||
		 result.outcome == telemetry_activity_outcome::accepted_degraded ||
		 result.outcome == telemetry_activity_outcome::idempotent)
		result.outcome = outcome;
}

bool state_ready(const telemetry_activity_state *state) noexcept
{
	return state != nullptr && state->initialized != 0U && state->max_slots > 0U &&
	       state->max_slots <= TELEMETRY_ACTIVITY_STATE_MAX_SLOTS &&
	       state->pulse_slot_count > 0U && state->pulse_slot_count <= state->max_slots;
}

int find_slot(const telemetry_activity_state *state, const telemetry_session_ref &session) noexcept
{
	for (std::size_t index = 0U; index < state->max_slots; ++index)
	{
		const telemetry_activity_slot &slot = state->slots[index];
		if (slot.lifecycle == telemetry_activity_slot_lifecycle::empty)
			continue;
		if (!session_id_equal(slot.session.id, session.id))
			continue;
		return session_equal(slot.session, session) ? static_cast<int>(index) : -2;
	}
	return -1;
}

std::size_t find_empty_slot(const telemetry_activity_state *state) noexcept
{
	for (std::size_t index = 0U; index < state->max_slots; ++index)
		if (state->slots[index].lifecycle == telemetry_activity_slot_lifecycle::empty)
			return index;
	return state->max_slots;
}

std::size_t find_closed_slot(const telemetry_activity_state *state) noexcept
{
	for (std::size_t index = 0U; index < state->max_slots; ++index)
		if (state->slots[index].lifecycle == telemetry_activity_slot_lifecycle::closed)
			return index;
	return state->max_slots;
}

int find_config(const telemetry_activity_state *state, telemetry_config_id config_id) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ACTIVITY_CONFIG_MAX; ++index)
		if (state->configs[index].used != 0U &&
		    state->configs[index].snapshot.config_id == config_id)
			return static_cast<int>(index);
	return -1;
}

std::size_t find_free_config(const telemetry_activity_state *state) noexcept
{
	for (std::size_t index = 0U; index < TELEMETRY_ACTIVITY_CONFIG_MAX; ++index)
		if (state->configs[index].used == 0U)
			return index;
	return TELEMETRY_ACTIVITY_CONFIG_MAX;
}

bool config_detail_enabled(const telemetry_config_snapshot &config) noexcept
{
	return config.enabled == 1U && config.backend == telemetry_storage_backend::sql;
}

bool admitted_config(const telemetry_activity_state *state, telemetry_config_id config_id,
		     const telemetry_config_snapshot **snapshot) noexcept
{
	const int index = find_config(state, config_id);
	if (index < 0)
		return false;
	if (snapshot != nullptr)
		*snapshot = &state->configs[static_cast<std::size_t>(index)].snapshot;
	return true;
}

bool config_matches_session(const telemetry_config_snapshot &config,
			    const telemetry_session_ref &session, std::uint32_t classifier,
			    std::uint32_t policy) noexcept
{
	return config.environment_id == session.environment_id &&
	       config.season_id == session.season_id && config.classifier_version == classifier &&
	       config.policy_version == policy;
}

std::uint64_t bucket_for(const telemetry_activity_state *state,
			 telemetry_monotonic_usec monotonic_usec) noexcept
{
	if (monotonic_usec < state->monotonic_anchor_usec)
		return 0U;
	return (monotonic_usec - state->monotonic_anchor_usec) / TELEMETRY_ACTIVITY_MINUTE_USEC;
}

void prepare_bucket(const telemetry_activity_state *state, telemetry_activity_slot &slot,
		    telemetry_monotonic_usec monotonic_usec) noexcept
{
	const std::uint64_t bucket = bucket_for(state, monotonic_usec);
	if (slot.context_bucket == bucket)
		return;
	slot.context_bucket = bucket;
	slot.context_segments_used = 0U;
	slot.context_overflow = 0U;
	const telemetry_config_snapshot *config = nullptr;
	slot.segment_cap = admitted_config(state, slot.config_id, &config) &&
					   config->context_segments_per_minute != 0U ?
				   config->context_segments_per_minute :
				   state->context_segments_per_minute;
}

void clear_slot(telemetry_activity_state *state, std::size_t index) noexcept
{
	telemetry_activity_slot &slot = state->slots[index];
	if (slot.lifecycle == telemetry_activity_slot_lifecycle::resident &&
	    state->resident_slots != 0U)
		--state->resident_slots;
	if (slot.lifecycle == telemetry_activity_slot_lifecycle::closed &&
	    state->closed_slots != 0U)
		--state->closed_slots;
	if (slot.lifecycle != telemetry_activity_slot_lifecycle::empty && state->used_slots != 0U)
		--state->used_slots;
	slot = {};
}

void occupy_slot(telemetry_activity_state *state, std::size_t index) noexcept
{
	telemetry_activity_slot &slot = state->slots[index];
	state->slots[index].lifecycle = telemetry_activity_slot_lifecycle::resident;
	++state->used_slots;
	++state->resident_slots;
	state->last_connection_sequence = slot.connection.connection_seq;
}

void initialize_slot(telemetry_activity_state *state, telemetry_activity_slot &slot,
		     const telemetry_session_enter &enter) noexcept
{
	slot = {};
	slot.lifecycle = telemetry_activity_slot_lifecycle::resident;
	slot.mode = telemetry_activity_accounting_mode::unknown;
	slot.connected = 1U;
	slot.session = enter.session;
	slot.connection = enter.connection;
	slot.dimensions = enter.dimensions;
	slot.context = telemetry_activity_context::unknown;
	slot.context_quality = telemetry_context_quality::unknown;
	slot.config_id = enter.config_id;
	slot.classifier_version = enter.classifier_version;
	slot.policy_version = enter.policy_version;
	slot.quality_flags = enter.quality_flags;
	slot.interval_start_monotonic_usec = enter.at_monotonic_usec;
	slot.interval_start_utc_usec = enter.at_utc_usec;
	slot.last_observed_monotonic_usec = enter.at_monotonic_usec;
	slot.last_observed_utc_usec = enter.at_utc_usec;
	slot.has_utc_anchor = enter.at_utc_usec == TELEMETRY_UTC_UNKNOWN ? 0U : 1U;
	slot.context_bucket = bucket_for(state, enter.at_monotonic_usec);
	slot.segment_cap = state->context_segments_per_minute;
	slot.interval_usec = state->interval_usec;
	slot.active_window_usec = state->active_window_usec;
	slot.entry_snapshot = enter;
	const telemetry_config_snapshot *config = nullptr;
	if (admitted_config(state, enter.config_id, &config))
	{
		slot.detail_ready = config_detail_enabled(*config) ? 1U : 0U;
		slot.segment_cap = config->context_segments_per_minute != 0U ?
					   config->context_segments_per_minute :
					   state->context_segments_per_minute;
		slot.interval_usec = config->interval_usec != 0U ? config->interval_usec :
								   state->interval_usec;
		slot.active_window_usec = config->active_window_usec != 0U ?
						  config->active_window_usec :
						  state->active_window_usec;
	}
}

void note_pending_gap(telemetry_activity_state *state, const telemetry_session_ref *session,
		      const telemetry_connection_id *connection,
		      telemetry_monotonic_usec start_monotonic_usec,
		      telemetry_monotonic_usec end_monotonic_usec,
		      telemetry_utc_usec start_utc_usec, telemetry_utc_usec end_utc_usec,
		      telemetry_duration_usec duration_usec, bool has_time, bool has_key,
		      const telemetry_record_key *key, telemetry_gap_reason reason,
		      telemetry_quality_mask quality, bool count_record) noexcept
{
	const bool valid_scope = session != nullptr && telemetry_session_ref_is_valid(*session) &&
				 connection != nullptr &&
				 telemetry_connection_reference_is_valid(*connection);
	telemetry_activity_pending_gap &pending = state->pending_gap;
	if (pending.pending == 0U)
	{
		pending = {};
		pending.pending = 1U;
		pending.scoped = valid_scope ? 1U : 0U;
		pending.sequence_known =
			has_key && key != nullptr && telemetry_record_key_is_valid(*key) ? 1U : 0U;
		pending.duration_known = has_time ? 1U : 0U;
		pending.detail_loss = reason == telemetry_gap_reason::detail_queue_drop ? 1U : 0U;
		pending.control_loss = reason == telemetry_gap_reason::control_queue_drop ? 1U : 0U;
		pending.reason = reason;
		pending.session = valid_scope ? *session : ZERO_SESSION_REF;
		pending.connection = valid_scope ? *connection : ZERO_CONNECTION_ID;
		if (has_time)
		{
			pending.start_monotonic_usec = start_monotonic_usec;
			pending.end_monotonic_usec = end_monotonic_usec;
			pending.start_utc_usec = start_utc_usec;
			pending.end_utc_usec = end_utc_usec;
			pending.duration_usec = duration_usec;
		}
		if (pending.sequence_known != 0U)
		{
			pending.first_missing_record_seq = key->record_seq;
			pending.last_missing_record_seq = key->record_seq;
		}
	}
	else
	{
		const bool same_scope = pending.scoped != 0U && valid_scope &&
					session_equal(pending.session, *session) &&
					connection_equal(pending.connection, *connection);
		if (!same_scope)
		{
			pending.scoped = 0U;
			pending.session = ZERO_SESSION_REF;
			pending.connection = ZERO_CONNECTION_ID;
		}
		if (pending.detail_loss != 0U || reason == telemetry_gap_reason::detail_queue_drop)
		{
			pending.detail_loss = 1U;
			pending.reason = telemetry_gap_reason::detail_queue_drop;
		}
		else if (pending.control_loss != 0U ||
			 reason == telemetry_gap_reason::control_queue_drop)
		{
			pending.control_loss = 1U;
			pending.reason = telemetry_gap_reason::control_queue_drop;
		}
		else
			pending.reason = reason;
		if (!has_key || key == nullptr || !telemetry_record_key_is_valid(*key) ||
		    pending.sequence_known == 0U ||
		    pending.last_missing_record_seq ==
			    std::numeric_limits<telemetry_record_sequence>::max() ||
		    key->record_seq != pending.last_missing_record_seq + 1U)
		{
			pending.sequence_known = 0U;
			pending.first_missing_record_seq = 0U;
			pending.last_missing_record_seq = 0U;
		}
		else
			pending.last_missing_record_seq = key->record_seq;
		if (!has_time)
			pending.duration_known = 0U;
		else if (pending.duration_known == 0U)
			pending.duration_known = 0U;
		else if (!same_scope || pending.end_monotonic_usec != start_monotonic_usec ||
			 pending.end_utc_usec != start_utc_usec ||
			 duration_usec > std::numeric_limits<telemetry_duration_usec>::max() -
						 pending.duration_usec)
		{
			pending.duration_known = 0U;
			pending.duration_usec = 0U;
		}
		else
		{
			pending.end_monotonic_usec = end_monotonic_usec;
			pending.end_utc_usec = end_utc_usec;
			pending.duration_usec += duration_usec;
		}
	}
	if (count_record)
		saturating_increment(pending.dropped_records);
	pending.quality_flags |= quality;
	if (!has_key)
	{
		pending.sequence_known = 0U;
		pending.first_missing_record_seq = 0U;
		pending.last_missing_record_seq = 0U;
		pending.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
	}
}

void note_detail_loss(telemetry_activity_state *state, const telemetry_activity_slot &slot,
		      telemetry_monotonic_usec start_monotonic_usec,
		      telemetry_monotonic_usec end_monotonic_usec,
		      telemetry_utc_usec start_utc_usec, telemetry_utc_usec end_utc_usec,
		      telemetry_duration_usec duration_usec, const telemetry_record_key *key,
		      bool has_key, telemetry_quality_mask quality) noexcept
{
	const telemetry_connection_id &connection = slot.connected != 0U ? slot.connection :
									   ZERO_CONNECTION_ID;
	note_pending_gap(state, &slot.session, &connection, start_monotonic_usec,
			 end_monotonic_usec, start_utc_usec, end_utc_usec, duration_usec, true,
			 has_key, key, telemetry_gap_reason::detail_queue_drop,
			 quality | TELEMETRY_QUALITY_QUEUE_DROP, true);
}

void note_control_loss(telemetry_activity_state *state, const telemetry_session_ref *session,
		       const telemetry_connection_id *connection,
		       telemetry_quality_mask quality) noexcept
{
	note_pending_gap(state, session, connection, 0U, 0U, TELEMETRY_UTC_UNKNOWN,
			 TELEMETRY_UTC_UNKNOWN, 0U, false, false, nullptr,
			 telemetry_gap_reason::control_queue_drop,
			 quality | TELEMETRY_QUALITY_QUEUE_DROP | TELEMETRY_QUALITY_SEQUENCE_GAP,
			 true);
	saturating_increment(state->dropped_control_total);
}

bool allocate_key(telemetry_activity_state *state, telemetry_record_kind kind,
		  telemetry_record &record) noexcept
{
	telemetry_record_key key{};
	if (state->key_allocator.next == nullptr ||
	    !state->key_allocator.next(state->key_allocator.context, kind, &key) ||
	    !telemetry_record_key_is_valid(key) || !producer_equal(key.producer, state->producer))
		return false;
	record.header.key = key;
	return true;
}

bool emit_pending_gap(telemetry_activity_state *state, telemetry_activity_result &result) noexcept
{
	telemetry_activity_pending_gap &pending = state->pending_gap;
	if (pending.pending == 0U)
		return true;
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::coverage_gap;
	record.header.occurrence_utc_usec = pending.duration_known != 0U ? pending.end_utc_usec :
									   TELEMETRY_UTC_UNKNOWN;
	telemetry_coverage_gap_payload &gap = record.payload.gap;
	gap.session = pending.scoped != 0U ? pending.session : ZERO_SESSION_REF;
	gap.connection = pending.scoped != 0U ? pending.connection : ZERO_CONNECTION_ID;
	gap.reason = pending.reason;
	gap.start_monotonic_usec = pending.duration_known != 0U ? pending.start_monotonic_usec : 0U;
	gap.end_monotonic_usec = pending.duration_known != 0U ? pending.end_monotonic_usec : 0U;
	gap.start_utc_usec = pending.duration_known != 0U ? pending.start_utc_usec :
							    TELEMETRY_UTC_UNKNOWN;
	gap.end_utc_usec = pending.duration_known != 0U ? pending.end_utc_usec :
							  TELEMETRY_UTC_UNKNOWN;
	gap.first_missing_record_seq =
		pending.sequence_known != 0U ? pending.first_missing_record_seq : 0U;
	gap.last_missing_record_seq =
		pending.sequence_known != 0U ? pending.last_missing_record_seq : 0U;
	gap.duration_usec = pending.duration_known != 0U ? pending.duration_usec : 0U;
	gap.dropped_records = pending.dropped_records;
	gap.quality_flags = pending.quality_flags;
	if (gap.reason == telemetry_gap_reason::telemetry_disabled)
		gap.quality_flags |= TELEMETRY_QUALITY_DISABLED | TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
	if (!allocate_key(state, record.header.kind, record))
	{
		pending.sequence_known = 0U;
		pending.first_missing_record_seq = 0U;
		pending.last_missing_record_seq = 0U;
		pending.control_loss = 1U;
		pending.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
		result_note_unallocated(result);
		set_failure(result, telemetry_activity_outcome::allocator_exhausted);
		saturating_increment(state->dropped_control_total);
		return false;
	}
	const bool accepted = state->sink.emit != nullptr &&
			      state->sink.emit(state->sink.context, &record);
	result_note_record(result, record.header.key, accepted);
	if (!accepted)
	{
		pending.sequence_known = 0U;
		pending.first_missing_record_seq = 0U;
		pending.last_missing_record_seq = 0U;
		pending.control_loss = 1U;
		pending.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP |
					 TELEMETRY_QUALITY_SEQUENCE_GAP;
		saturating_increment(pending.dropped_records);
		saturating_increment(state->dropped_control_total);
		set_failure(result, telemetry_activity_outcome::sink_rejected);
		return false;
	}
	pending = {};
	saturating_increment(state->accepted_control_total);
	return true;
}

telemetry_quality_mask utc_quality(const telemetry_activity_slot &slot,
				   telemetry_monotonic_usec monotonic_usec,
				   telemetry_utc_usec utc_usec) noexcept
{
	if (utc_usec == TELEMETRY_UTC_UNKNOWN || slot.has_utc_anchor == 0U ||
	    slot.last_observed_utc_usec == TELEMETRY_UTC_UNKNOWN)
		return TELEMETRY_QUALITY_NONE;
	if (monotonic_usec < slot.last_observed_monotonic_usec ||
	    utc_usec < slot.last_observed_utc_usec)
		return TELEMETRY_QUALITY_CLOCK_DISCONTINUITY;
	const telemetry_monotonic_usec monotonic_elapsed =
		monotonic_usec - slot.last_observed_monotonic_usec;
	std::uint64_t utc_elapsed = 0U;
	if (slot.last_observed_utc_usec >= 0)
		/* Both operands are nonnegative and ordered, so signed subtraction is safe. */
		utc_elapsed = static_cast<std::uint64_t>(utc_usec - slot.last_observed_utc_usec);
	else if (utc_usec < 0)
		/* Two negative ordered int64 values differ by at most INT64_MAX. */
		utc_elapsed = static_cast<std::uint64_t>(utc_usec - slot.last_observed_utc_usec);
	else
	{
		const std::uint64_t prior_magnitude =
			static_cast<std::uint64_t>(-(slot.last_observed_utc_usec + 1)) + 1U;
		const std::uint64_t after_magnitude = static_cast<std::uint64_t>(utc_usec);
		utc_elapsed = prior_magnitude > std::numeric_limits<std::uint64_t>::max() -
							after_magnitude ?
				      std::numeric_limits<std::uint64_t>::max() :
				      prior_magnitude + after_magnitude;
	}
	const std::uint64_t drift = utc_elapsed > monotonic_elapsed ?
					    utc_elapsed - monotonic_elapsed :
					    monotonic_elapsed - utc_elapsed;
	return drift > CLOCK_SKEW_TOLERANCE_USEC ? TELEMETRY_QUALITY_CLOCK_DISCONTINUITY :
						   TELEMETRY_QUALITY_NONE;
}

void observe_time(telemetry_activity_slot &slot, telemetry_monotonic_usec monotonic_usec,
		  telemetry_utc_usec utc_usec) noexcept
{
	slot.last_observed_monotonic_usec = monotonic_usec;
	slot.last_observed_utc_usec = utc_usec;
	slot.has_utc_anchor = utc_usec == TELEMETRY_UTC_UNKNOWN ? 0U : 1U;
}

bool next_interval_cut(const telemetry_activity_slot &slot, telemetry_monotonic_usec start,
		       telemetry_monotonic_usec end, telemetry_monotonic_usec &cut) noexcept
{
	if (slot.interval_usec == 0U)
		return false;
	telemetry_monotonic_usec candidate = 0U;
	if (!telemetry_activity_private::checked_monotonic_add(start, slot.interval_usec,
							       candidate) ||
	    candidate >= end)
		return false;
	cut = candidate;
	return true;
}

bool next_deadline_cut(const telemetry_activity_slot &slot, telemetry_monotonic_usec start,
		       telemetry_monotonic_usec end, telemetry_monotonic_usec &cut) noexcept
{
	if (slot.mode != telemetry_activity_accounting_mode::active ||
	    slot.active_deadline_monotonic_usec <= start ||
	    slot.active_deadline_monotonic_usec >= end)
		return false;
	cut = slot.active_deadline_monotonic_usec;
	return true;
}

bool next_minute_cut(const telemetry_activity_state *state, const telemetry_activity_slot &slot,
		     telemetry_monotonic_usec start, telemetry_monotonic_usec end,
		     telemetry_monotonic_usec &cut) noexcept
{
	const std::uint64_t start_bucket = bucket_for(state, start);
	const std::uint64_t end_bucket = bucket_for(state, end);
	if (start_bucket == end_bucket || start < state->monotonic_anchor_usec)
		return false;
	const telemetry_monotonic_usec relative = start - state->monotonic_anchor_usec;
	const std::uint64_t next_bucket = start_bucket + 1U;
	if (next_bucket >
	    std::numeric_limits<std::uint64_t>::max() / TELEMETRY_ACTIVITY_MINUTE_USEC)
		return false;
	const telemetry_monotonic_usec next_relative = next_bucket * TELEMETRY_ACTIVITY_MINUTE_USEC;
	if (next_relative >
	    std::numeric_limits<telemetry_monotonic_usec>::max() - state->monotonic_anchor_usec)
		return false;
	const telemetry_monotonic_usec boundary = state->monotonic_anchor_usec + next_relative;
	(void)slot;
	if (boundary <= start || boundary >= end || relative >= next_relative)
		return false;
	cut = boundary;
	return true;
}

bool next_midnight_cut(const telemetry_activity_slot &slot, telemetry_monotonic_usec start,
		       telemetry_monotonic_usec end, telemetry_utc_usec start_utc,
		       telemetry_utc_usec end_utc, telemetry_quality_mask quality,
		       telemetry_monotonic_usec &cut) noexcept
{
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U ||
	    start_utc == TELEMETRY_UTC_UNKNOWN || end_utc == TELEMETRY_UTC_UNKNOWN ||
	    end_utc <= start_utc)
		return false;
	constexpr telemetry_utc_usec DAY_USEC = 86'400'000'000LL;
	telemetry_utc_usec remainder = start_utc % DAY_USEC;
	if (remainder < 0)
		remainder += DAY_USEC;
	const telemetry_utc_usec until_midnight = DAY_USEC - remainder;
	if (start_utc > std::numeric_limits<telemetry_utc_usec>::max() - until_midnight)
		return false;
	const telemetry_utc_usec midnight = start_utc + until_midnight;
	if (end_utc < midnight)
		return false;
	const telemetry_duration_usec monotonic_offset =
		static_cast<telemetry_duration_usec>(until_midnight);
	if (monotonic_offset > std::numeric_limits<telemetry_monotonic_usec>::max() - start)
		return false;
	const telemetry_monotonic_usec boundary = start + monotonic_offset;
	if (boundary <= start || boundary >= end)
		return false;
	(void)slot;
	cut = boundary;
	return true;
}

bool utc_for_cut(const telemetry_activity_slot &slot, telemetry_monotonic_usec start,
		 telemetry_monotonic_usec cut, telemetry_monotonic_usec end,
		 telemetry_utc_usec start_utc, telemetry_utc_usec end_utc,
		 telemetry_quality_mask quality, telemetry_utc_usec &utc) noexcept
{
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U || slot.has_utc_anchor == 0U ||
	    start_utc == TELEMETRY_UTC_UNKNOWN)
	{
		utc = TELEMETRY_UTC_UNKNOWN;
		return false;
	}
	if (cut == end)
	{
		utc = end_utc;
		return utc != TELEMETRY_UTC_UNKNOWN;
	}
	const telemetry_duration_usec offset = cut - start;
	if (offset >
	    static_cast<telemetry_duration_usec>(std::numeric_limits<telemetry_utc_usec>::max()))
	{
		utc = TELEMETRY_UTC_UNKNOWN;
		return false;
	}
	const telemetry_utc_usec signed_offset = static_cast<telemetry_utc_usec>(offset);
	if (start_utc > std::numeric_limits<telemetry_utc_usec>::max() - signed_offset)
	{
		utc = TELEMETRY_UTC_UNKNOWN;
		return false;
	}
	utc = start_utc + signed_offset;
	return true;
}

telemetry_interval_category category_for(const telemetry_activity_slot &slot) noexcept
{
	if (slot.connected == 0U || slot.mode == telemetry_activity_accounting_mode::linkdead)
		return telemetry_interval_category::resident_linkdead;
	if (slot.mode == telemetry_activity_accounting_mode::active)
		return telemetry_interval_category::connected_active;
	if (slot.mode == telemetry_activity_accounting_mode::idle)
		return telemetry_interval_category::connected_idle;
	return telemetry_interval_category::unknown;
}

void add_piece_to_totals(telemetry_activity_slot &slot, telemetry_interval_category category,
			 telemetry_duration_usec duration,
			 telemetry_cumulative_counters &delta) noexcept
{
	if (category == telemetry_interval_category::resident_linkdead)
		delta = { 0U, 0U, 0U, 0U, duration, duration };
	else if (category == telemetry_interval_category::connected_active)
		delta = { duration, duration, 0U, 0U, duration, 0U };
	else if (category == telemetry_interval_category::connected_idle)
		delta = { duration, 0U, duration, 0U, duration, 0U };
	else
		delta = { duration, 0U, 0U, duration, duration, 0U };
	(void)slot;
}

void interval_context(const telemetry_activity_slot &slot, telemetry_activity_context &context,
		      telemetry_context_quality &context_quality, telemetry_dimensions &dimensions,
		      telemetry_quality_mask &quality) noexcept
{
	if (slot.connected == 0U || slot.mode == telemetry_activity_accounting_mode::linkdead)
	{
		context = telemetry_activity_context::unknown;
		context_quality = telemetry_context_quality::unavailable;
		dimensions = UNKNOWN_DIMENSIONS;
		quality |= TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
		return;
	}
	if (slot.context_overflow != 0U)
	{
		context = telemetry_activity_context::overflow_unknown;
		context_quality = telemetry_context_quality::overflow;
		dimensions = UNKNOWN_DIMENSIONS;
		quality |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW | TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
		return;
	}
	context = slot.context;
	context_quality = slot.context_quality;
	dimensions = slot.dimensions;
	if (context == telemetry_activity_context::unknown)
		quality |= TELEMETRY_QUALITY_CONTEXT_UNKNOWN;
}

telemetry_cumulative_counters delta_for_range(const telemetry_activity_slot &slot,
					      telemetry_monotonic_usec start,
					      telemetry_monotonic_usec end) noexcept
{
	const auto duration = end - start;
	if (slot.connected == 0U)
		return { 0U, 0U, 0U, 0U, duration, duration };
	if (slot.mode == telemetry_activity_accounting_mode::active)
	{
		const auto deadline = slot.active_deadline_monotonic_usec;
		const auto active_end = deadline < end ? deadline : end;
		const auto active = active_end > start ? active_end - start : 0U;
		return { duration, active, duration - active, 0U, duration, 0U };
	}
	if (slot.mode == telemetry_activity_accounting_mode::idle)
		return { duration, 0U, duration, 0U, duration, 0U };
	return { duration, 0U, 0U, duration, duration, 0U };
}

bool emit_interval_piece(telemetry_activity_state *state, telemetry_activity_slot &slot,
			 telemetry_monotonic_usec start, telemetry_monotonic_usec end,
			 telemetry_utc_usec start_utc, telemetry_utc_usec end_utc,
			 telemetry_quality_mask quality, telemetry_activity_result &result) noexcept
{
	if (end <= start)
		return true;
	const telemetry_duration_usec duration = end - start;
	const telemetry_interval_category category = category_for(slot);
	telemetry_cumulative_counters delta{};
	add_piece_to_totals(slot, category, duration, delta);
	if (!cumulative_add(slot.cumulative, delta))
	{
		set_failure(result, telemetry_activity_outcome::invalid);
		return false;
	}
	result_add_delta(result, slot.session,
			 category == telemetry_interval_category::resident_linkdead ?
				 ZERO_CONNECTION_ID :
				 slot.connection,
			 end, delta);
	result_note_interval(result);
	prepare_bucket(state, slot, start);
	// Suppressed detail is disabled, not context-budget loss. Preserve the
	// existing budget without charging intervals that cannot be emitted.
	if (slot.detail_ready == 0U)
	{
		saturating_increment(state->suppressed_detail_total);
		note_pending_gap(state, &slot.session,
				 category == telemetry_interval_category::resident_linkdead ?
					 &ZERO_CONNECTION_ID :
					 &slot.connection,
				 start, end, start_utc, end_utc, duration, true, false, nullptr,
				 telemetry_gap_reason::telemetry_disabled,
				 quality | TELEMETRY_QUALITY_DISABLED |
					 TELEMETRY_QUALITY_CONTEXT_UNKNOWN,
				 true);
		if (result.records_dropped != std::numeric_limits<std::uint16_t>::max())
			++result.records_dropped;
		if (result.outcome == telemetry_activity_outcome::accepted ||
		    result.outcome == telemetry_activity_outcome::accepted_degraded ||
		    result.outcome == telemetry_activity_outcome::idempotent ||
		    result.outcome == telemetry_activity_outcome::invalid)
			result.outcome = telemetry_activity_outcome::accepted_degraded;
		return true;
	}
	if (slot.context_segments_used >= slot.segment_cap)
	{
		// The cap includes the reserved overflow interval. Further category,
		// config or forced-flush boundaries keep counters, not extra rows.
		const auto loss_quality = quality | TELEMETRY_QUALITY_CONTEXT_OVERFLOW |
					  TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
		note_pending_gap(state, &slot.session, &slot.connection, start, end, start_utc,
				 end_utc, duration, true, false, nullptr,
				 telemetry_gap_reason::sequence_gap, loss_quality, true);
		result.quality_flags |= loss_quality;
		saturating_increment(state->suppressed_detail_total);
		result_note_unallocated(result);
		if (result.outcome != telemetry_activity_outcome::sink_rejected &&
		    result.outcome != telemetry_activity_outcome::allocator_exhausted)
			result.outcome = telemetry_activity_outcome::accepted_degraded;
		return true;
	}
	if (slot.context_overflow == 0U && slot.context_segments_used == slot.segment_cap - 1U)
	{
		slot.context_overflow = 1U;
		saturating_increment(state->context_overflow_total);
	}
	++slot.context_segments_used;
	if (!emit_pending_gap(state, result))
	{
		note_detail_loss(state, slot, start, end, start_utc, end_utc, duration, nullptr,
				 false, quality | TELEMETRY_QUALITY_SEQUENCE_GAP);
		return false;
	}
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::interval;
	record.header.occurrence_utc_usec = end_utc;
	telemetry_interval_payload &payload = record.payload.interval;
	payload.session = slot.session;
	payload.connection = category == telemetry_interval_category::resident_linkdead ?
				     ZERO_CONNECTION_ID :
				     slot.connection;
	payload.window = { start, end, start_utc, end_utc };
	payload.duration_usec = duration;
	payload.category = category;
	interval_context(slot, payload.context, payload.context_quality, payload.dimensions,
			 quality);
	payload.config_id = slot.config_id;
	payload.classifier_version = slot.classifier_version;
	payload.policy_version = slot.policy_version;
	payload.quality_flags = quality | slot.quality_flags;
	if (!allocate_key(state, record.header.kind, record))
	{
		note_detail_loss(state, slot, start, end, start_utc, end_utc, duration, nullptr,
				 false, payload.quality_flags | TELEMETRY_QUALITY_SEQUENCE_GAP);
		saturating_increment(state->dropped_detail_total);
		result_note_unallocated(result);
		set_failure(result, telemetry_activity_outcome::allocator_exhausted);
		return false;
	}
	const bool accepted = state->sink.emit != nullptr &&
			      state->sink.emit(state->sink.context, &record);
	result_note_record(result, record.header.key, accepted);
	if (!accepted)
	{
		note_detail_loss(state, slot, start, end, start_utc, end_utc, duration,
				 &record.header.key, true,
				 payload.quality_flags | TELEMETRY_QUALITY_QUEUE_DROP);
		saturating_increment(state->dropped_detail_total);
		set_failure(result, telemetry_activity_outcome::sink_rejected);
		return false;
	}
	saturating_increment(state->accepted_detail_total);
	return true;
}

bool seal_to(telemetry_activity_state *state, telemetry_activity_slot &slot,
	     telemetry_monotonic_usec end, telemetry_utc_usec end_utc,
	     telemetry_quality_mask quality, telemetry_activity_result &result) noexcept
{
	const telemetry_quality_mask effective_quality =
		quality |
		(slot.utc_discontinuity_pending != 0U ? TELEMETRY_QUALITY_CLOCK_DISCONTINUITY :
							TELEMETRY_QUALITY_NONE);
	if (!telemetry_activity_private::monotonic_range_is_valid(
		    slot.interval_start_monotonic_usec, end))
	{
		set_failure(result, telemetry_activity_outcome::invalid);
		return false;
	}
	if (end == slot.interval_start_monotonic_usec)
		return true;
	// Preflight the whole cut before records, counters or the frontier change.
	auto candidate_total = slot.cumulative;
	if (!cumulative_add(candidate_total,
			    delta_for_range(slot, slot.interval_start_monotonic_usec, end)))
	{
		result.outcome = telemetry_activity_outcome::invalid;
		return false;
	}
	telemetry_monotonic_usec start = slot.interval_start_monotonic_usec;
	telemetry_utc_usec start_utc = slot.interval_start_utc_usec;
	if ((effective_quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
		start_utc = TELEMETRY_UTC_UNKNOWN;
	std::uint32_t pieces = 0U;
	while (start < end)
	{
		telemetry_monotonic_usec cut = end;
		telemetry_monotonic_usec candidate = 0U;
		if (next_interval_cut(slot, start, end, candidate) && candidate < cut)
			cut = candidate;
		if (next_deadline_cut(slot, start, end, candidate) && candidate < cut)
			cut = candidate;
		if (next_minute_cut(state, slot, start, end, candidate) && candidate < cut)
			cut = candidate;
		if (next_midnight_cut(slot, start, end, start_utc, end_utc, effective_quality,
				      candidate) &&
		    candidate < cut)
			cut = candidate;
		if (pieces >= TELEMETRY_ACTIVITY_MAX_PIECES_PER_OPERATION - 1U && cut < end)
		{
			// Bound catch-up without pretending a cross-deadline/day tail is
			// one valid interval. Counters are exact; missing detail is a gap.
			const auto delta = delta_for_range(slot, start, end);
			if (!cumulative_add(slot.cumulative, delta))
			{
				result.outcome = telemetry_activity_outcome::invalid;
				return false;
			}
			result_add_delta(result, slot.session, slot.connection, end, delta);
			result_note_interval(result);
			const auto tail_quality = effective_quality | TELEMETRY_QUALITY_LATE |
						  TELEMETRY_QUALITY_CONTEXT_UNKNOWN |
						  (slot.detail_ready == 0U ?
							   TELEMETRY_QUALITY_DISABLED :
							   TELEMETRY_QUALITY_NONE);
			result.quality_flags |= tail_quality;
			note_pending_gap(state, &slot.session, &slot.connection, start, end,
					 TELEMETRY_UTC_UNKNOWN, TELEMETRY_UTC_UNKNOWN, end - start,
					 true, false, nullptr,
					 slot.detail_ready == 0U ?
						 telemetry_gap_reason::telemetry_disabled :
						 telemetry_gap_reason::sequence_gap,
					 tail_quality, true);
			saturating_increment(state->suppressed_detail_total);
			result_note_unallocated(result);
			if (result.outcome != telemetry_activity_outcome::sink_rejected &&
			    result.outcome != telemetry_activity_outcome::allocator_exhausted)
				result.outcome = telemetry_activity_outcome::accepted_degraded;
			(void)emit_pending_gap(state, result);
			start = end;
			break;
		}
		telemetry_quality_mask piece_quality = effective_quality;
		telemetry_utc_usec cut_utc = TELEMETRY_UTC_UNKNOWN;
		(void)utc_for_cut(slot, start, cut, end, start_utc, end_utc, piece_quality,
				  cut_utc);
		const telemetry_activity_accounting_mode mode_before = slot.mode;
		if (slot.mode == telemetry_activity_accounting_mode::active &&
		    slot.active_deadline_monotonic_usec <= start)
		{
			slot.mode = telemetry_activity_accounting_mode::idle;
			slot.active_deadline_monotonic_usec = 0U;
		}
		// Delivery loss keeps accounting; a numerical failure must not advance.
		if (!emit_interval_piece(state, slot, start, cut, start_utc, cut_utc, piece_quality,
					 result) &&
		    result.outcome == telemetry_activity_outcome::invalid)
			return false;
		start = cut;
		start_utc = cut_utc;
		slot.interval_start_monotonic_usec = start;
		slot.interval_start_utc_usec = start_utc;
		if (mode_before == telemetry_activity_accounting_mode::active &&
		    slot.active_deadline_monotonic_usec == start)
		{
			slot.mode = telemetry_activity_accounting_mode::idle;
			slot.active_deadline_monotonic_usec = 0U;
		}
		++pieces;
	}
	if (effective_quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY)
		slot.interval_start_utc_usec = end_utc;
	else if (start == end)
		slot.interval_start_utc_usec = end_utc;
	slot.interval_start_monotonic_usec = end;
	if (slot.mode == telemetry_activity_accounting_mode::active &&
	    slot.active_deadline_monotonic_usec != 0U && slot.active_deadline_monotonic_usec <= end)
	{
		slot.mode = telemetry_activity_accounting_mode::idle;
		slot.active_deadline_monotonic_usec = 0U;
	}
	slot.quality_flags |= quality;
	if ((effective_quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
		slot.utc_discontinuity_pending = 0U;
	return true;
}

bool sync_admitted_config(telemetry_activity_state *state, telemetry_activity_slot &slot,
			  telemetry_monotonic_usec at_monotonic_usec,
			  telemetry_utc_usec at_utc_usec, telemetry_quality_mask quality,
			  telemetry_activity_result &result) noexcept
{
	if (slot.detail_ready != 0U)
		return true;
	const telemetry_config_snapshot *config = nullptr;
	if (!admitted_config(state, slot.config_id, &config) || !config_detail_enabled(*config) ||
	    !config_matches_session(*config, slot.session, slot.classifier_version,
				    slot.policy_version))
		return true;
	if (!seal_to(state, slot, at_monotonic_usec, at_utc_usec, quality, result))
		return false;
	slot.detail_ready = 1U;
	slot.interval_usec = config->interval_usec != 0U ? config->interval_usec :
							   state->interval_usec;
	slot.active_window_usec = config->active_window_usec != 0U ? config->active_window_usec :
								     state->active_window_usec;
	const auto desired_cap = config->context_segments_per_minute != 0U ?
					 config->context_segments_per_minute :
					 state->context_segments_per_minute;
	slot.segment_cap = desired_cap < slot.segment_cap ? desired_cap : slot.segment_cap;
	slot.interval_start_monotonic_usec = at_monotonic_usec;
	slot.interval_start_utc_usec = at_utc_usec;
	return true;
}

bool active_evidence(telemetry_activity_evidence_kind kind) noexcept
{
	return kind == telemetry_activity_evidence_kind::player_action ||
	       kind == telemetry_activity_evidence_kind::movement ||
	       kind == telemetry_activity_evidence_kind::interaction ||
	       kind == telemetry_activity_evidence_kind::communication ||
	       kind == telemetry_activity_evidence_kind::combat_participation ||
	       kind == telemetry_activity_evidence_kind::reading ||
	       kind == telemetry_activity_evidence_kind::social;
}

bool add_active_deadline(telemetry_monotonic_usec at, telemetry_duration_usec window,
			 telemetry_monotonic_usec &deadline) noexcept
{
	return telemetry_activity_private::checked_monotonic_add(at, window, deadline) ||
	       (deadline = std::numeric_limits<telemetry_monotonic_usec>::max(), true);
}

bool should_process_pulse(const telemetry_activity_state *state,
			  const telemetry_activity_slot &slot, telemetry_monotonic_usec now,
			  telemetry_utc_usec utc) noexcept
{
	if (now <= slot.interval_start_monotonic_usec)
		return false;
	if (now - slot.interval_start_monotonic_usec >= slot.interval_usec)
		return true;
	if (slot.mode == telemetry_activity_accounting_mode::active &&
	    slot.active_deadline_monotonic_usec != 0U && slot.active_deadline_monotonic_usec <= now)
		return true;
	if (bucket_for(state, slot.interval_start_monotonic_usec) != bucket_for(state, now))
		return true;
	if (next_midnight_cut(slot, slot.interval_start_monotonic_usec, now,
			      slot.interval_start_utc_usec, utc, TELEMETRY_QUALITY_NONE, now))
		return true;
	return slot.detail_ready == 0U && admitted_config(state, slot.config_id, nullptr);
}

telemetry_activity_result retire_one_at(telemetry_activity_state *state, std::size_t index,
					telemetry_utc_usec occurrence_utc_usec) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::retired);
	telemetry_activity_slot &slot = state->slots[index];
	(void)emit_pending_gap(state, result);
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::coverage_gap;
	record.header.occurrence_utc_usec = occurrence_utc_usec;
	telemetry_coverage_gap_payload &gap = record.payload.gap;
	gap.session = slot.session;
	gap.connection = ZERO_CONNECTION_ID;
	gap.reason = telemetry_gap_reason::unclosed_tail;
	gap.start_monotonic_usec = 0U;
	gap.end_monotonic_usec = 0U;
	gap.start_utc_usec = occurrence_utc_usec;
	gap.end_utc_usec = occurrence_utc_usec;
	gap.first_missing_record_seq = 0U;
	gap.last_missing_record_seq = 0U;
	gap.duration_usec = 0U;
	gap.dropped_records = 0U;
	gap.quality_flags = TELEMETRY_QUALITY_UNCLOSED_TAIL;
	if (!allocate_key(state, record.header.kind, record))
	{
		note_control_loss(state, &slot.session, &ZERO_CONNECTION_ID,
				  TELEMETRY_QUALITY_UNCLOSED_TAIL);
		result_note_unallocated(result);
		set_failure(result, telemetry_activity_outcome::allocator_exhausted);
	}
	else
	{
		const bool accepted = state->sink.emit != nullptr &&
				      state->sink.emit(state->sink.context, &record);
		result_note_record(result, record.header.key, accepted);
		if (accepted)
			saturating_increment(state->accepted_control_total);
		else
		{
			note_control_loss(state, &slot.session, &ZERO_CONNECTION_ID,
					  TELEMETRY_QUALITY_UNCLOSED_TAIL);
			set_failure(result, telemetry_activity_outcome::sink_rejected);
		}
	}
	clear_slot(state, index);
	saturating_increment(state->retired_session_total);
	if (result.slots_retired != std::numeric_limits<std::uint16_t>::max())
		++result.slots_retired;
	return result;
}

std::size_t find_expired_detached(const telemetry_activity_state *state,
				  telemetry_monotonic_usec now) noexcept
{
	std::size_t found = state->max_slots;
	telemetry_monotonic_usec oldest = std::numeric_limits<telemetry_monotonic_usec>::max();
	for (std::size_t index = 0U; index < state->max_slots; ++index)
	{
		const telemetry_activity_slot &slot = state->slots[index];
		if (slot.lifecycle != telemetry_activity_slot_lifecycle::resident ||
		    slot.connected != 0U || now < slot.detached_since_monotonic_usec ||
		    now - slot.detached_since_monotonic_usec < state->detached_retire_after_usec)
			continue;
		if (found == state->max_slots || slot.detached_since_monotonic_usec < oldest)
		{
			found = index;
			oldest = slot.detached_since_monotonic_usec;
		}
	}
	return found;
}

} // namespace

telemetry_activity_outcome
telemetry_activity_state_init(telemetry_activity_state *state,
			      const telemetry_activity_state_config *config) noexcept
{
	if (state == nullptr || config == nullptr || config->reserved != 0U ||
	    config->max_slots == 0U || config->max_slots > TELEMETRY_ACTIVITY_STATE_MAX_SLOTS ||
	    (config->pulse_slot_count > config->max_slots) ||
	    (config->context_segments_per_minute >
	     TELEMETRY_CONFIG_CONTEXT_SEGMENT_CAP_MAX_PROPOSAL) ||
	    config->interval_usec > TELEMETRY_INTERVAL_USEC_MAX_PROPOSAL ||
	    config->active_window_usec > telemetry_activity_private::CONFIGURED_WINDOW_MAX_USEC ||
	    config->detached_retire_after_usec >
		    telemetry_activity_private::CONFIGURED_WINDOW_MAX_USEC ||
	    !telemetry_producer_id_is_valid(config->producer) || config->clock.now == nullptr ||
	    config->sink.emit == nullptr || config->key_allocator.next == nullptr)
		return telemetry_activity_outcome::invalid;
	*state = {};
	state->max_slots = config->max_slots;
	state->pulse_slot_count = config->pulse_slot_count == 0U ? 1U : config->pulse_slot_count;
	state->context_segments_per_minute = config->context_segments_per_minute == 0U ?
						     TELEMETRY_ACTIVITY_SEGMENTS_DEFAULT :
						     config->context_segments_per_minute;
	state->interval_usec = config->interval_usec == 0U ?
				       TELEMETRY_ACTIVITY_INTERVAL_DEFAULT_USEC :
				       config->interval_usec;
	state->active_window_usec = config->active_window_usec == 0U ?
					    TELEMETRY_ACTIVITY_ACTIVE_WINDOW_DEFAULT_USEC :
					    config->active_window_usec;
	state->detached_retire_after_usec =
		config->detached_retire_after_usec == 0U ?
			telemetry_activity_private::CONFIGURED_WINDOW_MAX_USEC :
			config->detached_retire_after_usec;
	state->monotonic_anchor_usec = config->monotonic_anchor_usec;
	state->producer = config->producer;
	state->clock = config->clock;
	state->sink = config->sink;
	state->key_allocator = config->key_allocator;
	state->process_anchor_set = 1U;
	state->initialized = 1U;
	return telemetry_activity_outcome::accepted;
}

void telemetry_activity_state_reset(telemetry_activity_state *state) noexcept
{
	if (state != nullptr)
		*state = {};
}

telemetry_activity_result
telemetry_activity_state_publish_config(telemetry_activity_state *state,
					telemetry_config_snapshot config) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_config_is_valid(config))
		return result;
	const int existing = find_config(state, config.config_id);
	if (existing >= 0)
	{
		if (!config_equal(state->configs[static_cast<std::size_t>(existing)].snapshot,
				  config))
			return result;
		result.outcome = telemetry_activity_outcome::idempotent;
		return result;
	}
	const std::size_t index = find_free_config(state);
	if (index >= TELEMETRY_ACTIVITY_CONFIG_MAX)
	{
		result.outcome = telemetry_activity_outcome::config_capacity_full;
		return result;
	}
	if (!emit_pending_gap(state, result))
		return result;
	telemetry_record record{};
	record.header.schema_version = TELEMETRY_SCHEMA_VERSION;
	record.header.kind = telemetry_record_kind::configuration;
	record.header.occurrence_utc_usec = config.effective_utc_usec;
	record.payload.configuration.config = config;
	if (!allocate_key(state, record.header.kind, record))
	{
		note_control_loss(state, nullptr, nullptr, TELEMETRY_QUALITY_SEQUENCE_GAP);
		result_note_unallocated(result);
		set_failure(result, telemetry_activity_outcome::allocator_exhausted);
		return result;
	}
	const bool accepted = state->sink.emit != nullptr &&
			      state->sink.emit(state->sink.context, &record);
	result_note_record(result, record.header.key, accepted);
	if (!accepted)
	{
		note_control_loss(state, nullptr, nullptr, TELEMETRY_QUALITY_QUEUE_DROP);
		set_failure(result, telemetry_activity_outcome::sink_rejected);
		return result;
	}
	state->configs[index].used = 1U;
	state->configs[index].snapshot = config;
	saturating_increment(state->admitted_config_total);
	saturating_increment(state->accepted_control_total);
	result.outcome = telemetry_activity_outcome::accepted;
	return result;
}

bool telemetry_activity_state_config_is_admitted(const telemetry_activity_state *state,
						 telemetry_config_id config_id) noexcept
{
	return state_ready(state) && admitted_config(state, config_id, nullptr);
}

telemetry_activity_result telemetry_activity_state_enter(telemetry_activity_state *state,
							 telemetry_session_enter enter) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_enter_is_valid(enter) ||
	    !producer_equal(enter.connection.producer, state->producer))
		return result;
	const telemetry_config_snapshot *config = nullptr;
	if (admitted_config(state, enter.config_id, &config) &&
	    !config_matches_session(*config, enter.session, enter.classifier_version,
				    enter.policy_version))
		return result;
	const int existing = find_slot(state, enter.session);
	if (existing == -2)
		return result;
	if (existing >= 0)
	{
		const telemetry_activity_slot &slot =
			state->slots[static_cast<std::size_t>(existing)];
		if (session_equal(slot.session, enter.session) &&
		    slot.entry_snapshot.at_monotonic_usec == enter.at_monotonic_usec &&
		    connection_equal(slot.entry_snapshot.connection, enter.connection) &&
		    slot.entry_snapshot.at_utc_usec == enter.at_utc_usec &&
		    dimensions_equal(slot.entry_snapshot.dimensions, enter.dimensions) &&
		    slot.entry_snapshot.config_id == enter.config_id &&
		    slot.entry_snapshot.classifier_version == enter.classifier_version &&
		    slot.entry_snapshot.policy_version == enter.policy_version &&
		    slot.entry_snapshot.quality_flags == enter.quality_flags)
		{
			result.outcome = telemetry_activity_outcome::idempotent;
			result_from_slot(result, slot);
		}
		return result;
	}
	if (enter.connection.connection_seq <= state->last_connection_sequence)
		return result;
	std::size_t index = find_empty_slot(state);
	if (index >= state->max_slots)
		index = find_closed_slot(state);
	if (index >= state->max_slots)
	{
		telemetry_monotonic_usec reclaim_now = enter.at_monotonic_usec;
		telemetry_utc_usec reclaim_utc = enter.at_utc_usec;
		if (state->clock.now != nullptr)
		{
			telemetry_monotonic_usec clock_monotonic = 0U;
			telemetry_utc_usec clock_utc = TELEMETRY_UTC_UNKNOWN;
			if (state->clock.now(state->clock.context, &clock_monotonic, &clock_utc))
			{
				reclaim_now = clock_monotonic;
				reclaim_utc = clock_utc;
			}
		}
		index = find_expired_detached(state, reclaim_now);
		if (index >= state->max_slots)
		{
			result.outcome = telemetry_activity_outcome::capacity_full;
			return result;
		}
		telemetry_activity_result retired = retire_one_at(state, index, reclaim_utc);
		result = retired;
		index = find_empty_slot(state);
	}
	const bool retirement_failed = result.outcome ==
					       telemetry_activity_outcome::allocator_exhausted ||
				       result.outcome == telemetry_activity_outcome::sink_rejected;
	telemetry_activity_slot &slot = state->slots[index];
	if (slot.lifecycle == telemetry_activity_slot_lifecycle::closed)
		clear_slot(state, index);
	initialize_slot(state, slot, enter);
	occupy_slot(state, index);
	if (!retirement_failed)
		result.outcome = telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_result
telemetry_activity_state_record_evidence(telemetry_activity_state *state,
					 telemetry_activity_evidence evidence) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_activity_evidence_is_valid(evidence))
		return result;
	const int found = find_slot(state, evidence.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_activity_outcome::not_found) : result;
	telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.has_last_evidence != 0U && evidence_equal(slot.last_evidence, evidence))
	{
		result.outcome = telemetry_activity_outcome::idempotent;
		result_from_slot(result, slot);
		return result;
	}
	if (slot.lifecycle != telemetry_activity_slot_lifecycle::resident)
		return result;
	if (evidence.kind == telemetry_activity_evidence_kind::linkdead)
	{
		if (slot.connected != 0U ||
		    evidence.at_monotonic_usec < slot.last_observed_monotonic_usec)
			return result;
		const telemetry_quality_mask quality =
			utc_quality(slot, evidence.at_monotonic_usec, evidence.at_utc_usec) |
			evidence.quality_flags;
		if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
		{
			saturating_increment(state->clock_discontinuity_total);
			slot.utc_discontinuity_pending = 1U;
		}
		if (!seal_to(state, slot, evidence.at_monotonic_usec, evidence.at_utc_usec, quality,
			     result))
			return result;
		observe_time(slot, evidence.at_monotonic_usec, evidence.at_utc_usec);
		slot.last_evidence = evidence;
		slot.has_last_evidence = 1U;
		result.outcome = result.outcome == telemetry_activity_outcome::invalid ?
					 telemetry_activity_outcome::accepted :
					 result.outcome;
		result_from_slot(result, slot);
		return result;
	}
	if (slot.connected == 0U || !connection_equal(slot.connection, evidence.connection) ||
	    evidence.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	telemetry_quality_mask quality =
		utc_quality(slot, evidence.at_monotonic_usec, evidence.at_utc_usec) |
		evidence.quality_flags;
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
	{
		saturating_increment(state->clock_discontinuity_total);
		slot.utc_discontinuity_pending = 1U;
	}
	if (!sync_admitted_config(state, slot, evidence.at_monotonic_usec, evidence.at_utc_usec,
				  quality, result))
		return result;
	const bool active = active_evidence(evidence.kind);
	const bool force_idle = evidence.kind == telemetry_activity_evidence_kind::afk;
	const bool deadline_expired =
		slot.mode == telemetry_activity_accounting_mode::active &&
		(slot.active_deadline_monotonic_usec == 0U ||
		 evidence.at_monotonic_usec >= slot.active_deadline_monotonic_usec);
	const bool changes_mode =
		(active &&
		 (slot.mode != telemetry_activity_accounting_mode::active || deadline_expired)) ||
		(force_idle && slot.mode != telemetry_activity_accounting_mode::idle) ||
		deadline_expired;
	if (changes_mode && !seal_to(state, slot, evidence.at_monotonic_usec, evidence.at_utc_usec,
				     quality, result))
		return result;
	slot.quality_flags |= quality;
	if (active)
	{
		slot.mode = telemetry_activity_accounting_mode::active;
		(void)add_active_deadline(evidence.at_monotonic_usec, slot.active_window_usec,
					  slot.active_deadline_monotonic_usec);
	}
	else if (force_idle)
	{
		slot.mode = telemetry_activity_accounting_mode::idle;
		slot.active_deadline_monotonic_usec = 0U;
	}
	else if (deadline_expired)
	{
		slot.mode = telemetry_activity_accounting_mode::idle;
		slot.active_deadline_monotonic_usec = 0U;
	}
	observe_time(slot, evidence.at_monotonic_usec, evidence.at_utc_usec);
	slot.last_evidence = evidence;
	slot.has_last_evidence = 1U;
	if (slot.detail_ready == 0U)
		result.outcome =
			result.outcome == telemetry_activity_outcome::accepted_degraded ?
				result.outcome :
				(result.outcome == telemetry_activity_outcome::sink_rejected ||
						 result.outcome == telemetry_activity_outcome::
									   allocator_exhausted ?
					 result.outcome :
					 telemetry_activity_outcome::accepted_degraded);
	else if (result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_result
telemetry_activity_state_update_context(telemetry_activity_state *state,
					telemetry_activity_context_snapshot snapshot) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_activity_context_snapshot_is_valid(snapshot))
		return result;
	const int found = find_slot(state, snapshot.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_activity_outcome::not_found) : result;
	telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.connected == 0U || !connection_equal(slot.connection, snapshot.connection) ||
	    snapshot.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	if (slot.has_last_context != 0U && context_equal(slot.last_context, snapshot))
	{
		result.outcome = telemetry_activity_outcome::idempotent;
		result_from_slot(result, slot);
		return result;
	}
	const telemetry_config_snapshot *new_config = nullptr;
	if (admitted_config(state, snapshot.config_id, &new_config) &&
	    !config_matches_session(*new_config, snapshot.session, snapshot.classifier_version,
				    snapshot.policy_version))
		return result;
	telemetry_quality_mask quality =
		utc_quality(slot, snapshot.at_monotonic_usec, snapshot.at_utc_usec) |
		snapshot.quality_flags;
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
	{
		saturating_increment(state->clock_discontinuity_total);
		slot.utc_discontinuity_pending = 1U;
	}
	const telemetry_activity_context next_context =
		telemetry_activity_context_resolve(snapshot.context_flags);
	const telemetry_context_quality next_quality =
		next_context == telemetry_activity_context::unknown ?
			telemetry_context_quality::unknown :
			telemetry_context_quality::observed;
	const bool changed = next_context != slot.context || next_quality != slot.context_quality ||
			     !dimensions_equal(slot.dimensions, snapshot.dimensions) ||
			     slot.config_id != snapshot.config_id ||
			     slot.classifier_version != snapshot.classifier_version ||
			     slot.policy_version != snapshot.policy_version;
	const bool needs_config_sync = slot.detail_ready == 0U &&
				       admitted_config(state, slot.config_id, &new_config) &&
				       config_detail_enabled(*new_config);
	const bool identity_changed = slot.config_id != snapshot.config_id ||
				      slot.classifier_version != snapshot.classifier_version ||
				      slot.policy_version != snapshot.policy_version;
	const bool must_split =
		changed && (slot.context_overflow == 0U || identity_changed ||
			    bucket_for(state, snapshot.at_monotonic_usec) != slot.context_bucket);
	if ((must_split || needs_config_sync) && !seal_to(state, slot, snapshot.at_monotonic_usec,
							  snapshot.at_utc_usec, quality, result))
		return result;
	slot.quality_flags |= quality;
	if (changed)
	{
		prepare_bucket(state, slot, snapshot.at_monotonic_usec);
		if (slot.context_overflow == 0U &&
		    slot.context_segments_used >=
			    (slot.segment_cap > 0U ? slot.segment_cap - 1U : 0U))
		{
			slot.context_overflow = 1U;
			slot.quality_flags |= TELEMETRY_QUALITY_CONTEXT_OVERFLOW |
					      TELEMETRY_QUALITY_DIMENSION_UNKNOWN;
			saturating_increment(state->context_overflow_total);
		}
		slot.context = next_context;
		slot.context_quality = next_quality;
		slot.dimensions = snapshot.dimensions;
		slot.config_id = snapshot.config_id;
		slot.classifier_version = snapshot.classifier_version;
		slot.policy_version = snapshot.policy_version;
		const telemetry_config_snapshot *config = nullptr;
		if (admitted_config(state, snapshot.config_id, &config))
		{
			slot.detail_ready = config_detail_enabled(*config) ? 1U : 0U;
			slot.interval_usec = config->interval_usec != 0U ? config->interval_usec :
									   state->interval_usec;
			slot.active_window_usec = config->active_window_usec != 0U ?
							  config->active_window_usec :
							  state->active_window_usec;
			const auto desired_cap = config->context_segments_per_minute != 0U ?
							 config->context_segments_per_minute :
							 state->context_segments_per_minute;
			slot.segment_cap = desired_cap < slot.segment_cap ? desired_cap :
									    slot.segment_cap;
		}
		else
		{
			slot.detail_ready = 0U;
			slot.interval_usec = state->interval_usec;
			slot.active_window_usec = state->active_window_usec;
			const auto desired_cap = state->context_segments_per_minute;
			slot.segment_cap = desired_cap < slot.segment_cap ? desired_cap :
									    slot.segment_cap;
		}
	}
	observe_time(slot, snapshot.at_monotonic_usec, snapshot.at_utc_usec);
	slot.last_context = snapshot;
	slot.has_last_context = 1U;
	if (slot.detail_ready == 0U)
		result.outcome = result.outcome == telemetry_activity_outcome::accepted_degraded ?
					 result.outcome :
					 telemetry_activity_outcome::accepted_degraded;
	else if (result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_result
telemetry_activity_state_transition(telemetry_activity_state *state,
				    telemetry_connection_transition transition) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_connection_transition_is_valid(transition) ||
	    transition.kind == telemetry_connection_transition_kind::copyover_resumed ||
	    !producer_equal(transition.connection.producer, state->producer))
		return result;
	const int found = find_slot(state, transition.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_activity_outcome::not_found) : result;
	telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.has_last_transition != 0U && transition_equal(slot.last_transition, transition))
	{
		result.outcome = telemetry_activity_outcome::idempotent;
		result_from_slot(result, slot);
		return result;
	}
	if (slot.lifecycle == telemetry_activity_slot_lifecycle::closed)
		return result;
	if (transition.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	// Reject stale identities before recording clock quality or touching state.
	if (transition.kind == telemetry_connection_transition_kind::detached)
	{
		if (slot.connected == 0U ||
		    !connection_equal(slot.connection, transition.connection))
			return result;
	}
	else if (slot.connected != 0U ||
		 transition.connection.connection_seq <= state->last_connection_sequence)
		return result;
	telemetry_quality_mask quality =
		utc_quality(slot, transition.at_monotonic_usec, transition.at_utc_usec) |
		transition.quality_flags;
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
	{
		saturating_increment(state->clock_discontinuity_total);
		slot.utc_discontinuity_pending = 1U;
	}
	if (transition.kind == telemetry_connection_transition_kind::detached)
	{
		if (!seal_to(state, slot, transition.at_monotonic_usec, transition.at_utc_usec,
			     quality, result))
			return result;
		slot.connection = ZERO_CONNECTION_ID;
		slot.connected = 0U;
		slot.mode = telemetry_activity_accounting_mode::linkdead;
		slot.active_deadline_monotonic_usec = 0U;
		slot.context = telemetry_activity_context::unknown;
		slot.context_quality = telemetry_context_quality::unavailable;
		slot.dimensions = UNKNOWN_DIMENSIONS;
		slot.interval_start_monotonic_usec = transition.at_monotonic_usec;
		slot.interval_start_utc_usec = transition.at_utc_usec;
		slot.detached_since_monotonic_usec = transition.at_monotonic_usec;
	}
	else
	{
		if (!seal_to(state, slot, transition.at_monotonic_usec, transition.at_utc_usec,
			     quality, result))
			return result;
		state->last_connection_sequence = transition.connection.connection_seq;
		slot.connection = transition.connection;
		slot.connected = 1U;
		slot.mode = telemetry_activity_accounting_mode::unknown;
		slot.active_deadline_monotonic_usec = 0U;
		slot.context = telemetry_activity_context::unknown;
		slot.context_quality = telemetry_context_quality::unknown;
		slot.dimensions = UNKNOWN_DIMENSIONS;
		slot.interval_start_monotonic_usec = transition.at_monotonic_usec;
		slot.interval_start_utc_usec = transition.at_utc_usec;
		slot.detached_since_monotonic_usec = 0U;
	}
	slot.quality_flags |= quality;
	observe_time(slot, transition.at_monotonic_usec, transition.at_utc_usec);
	slot.last_transition = transition;
	slot.has_last_transition = 1U;
	if (result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = slot.detail_ready == 0U ?
					 telemetry_activity_outcome::accepted_degraded :
					 telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_result
telemetry_activity_state_flush_at(telemetry_activity_state *state, telemetry_session_ref session,
				  telemetry_monotonic_usec at_monotonic_usec,
				  telemetry_utc_usec at_utc_usec) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_ref_is_valid(session))
		return result;
	const int found = find_slot(state, session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_activity_outcome::not_found) : result;
	telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle != telemetry_activity_slot_lifecycle::resident ||
	    at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	telemetry_quality_mask quality = utc_quality(slot, at_monotonic_usec, at_utc_usec);
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
	{
		saturating_increment(state->clock_discontinuity_total);
		slot.utc_discontinuity_pending = 1U;
	}
	if (!sync_admitted_config(state, slot, at_monotonic_usec, at_utc_usec, quality, result) ||
	    !seal_to(state, slot, at_monotonic_usec, at_utc_usec, quality, result))
		return result;
	observe_time(slot, at_monotonic_usec, at_utc_usec);
	if (result.intervals_sealed == 0U && result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = telemetry_activity_outcome::idempotent;
	else if (result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = slot.detail_ready == 0U ?
					 telemetry_activity_outcome::accepted_degraded :
					 telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_result telemetry_activity_state_flush(telemetry_activity_state *state,
							 telemetry_session_ref session) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state))
		return result;
	telemetry_monotonic_usec monotonic_usec = 0U;
	telemetry_utc_usec utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (state->clock.now == nullptr ||
	    !state->clock.now(state->clock.context, &monotonic_usec, &utc_usec))
	{
		result.outcome = telemetry_activity_outcome::clock_unavailable;
		return result;
	}
	return telemetry_activity_state_flush_at(state, session, monotonic_usec, utc_usec);
}

telemetry_activity_result telemetry_activity_state_exit(telemetry_activity_state *state,
							telemetry_session_exit exit) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state) || !telemetry_session_exit_is_valid(exit))
		return result;
	const int found = find_slot(state, exit.session);
	if (found < 0)
		return found == -1 ? result_with(telemetry_activity_outcome::not_found) : result;
	telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	if (slot.lifecycle == telemetry_activity_slot_lifecycle::closed)
	{
		if (slot.has_last_exit != 0U && exit_equal(slot.last_exit, exit))
		{
			result.outcome = telemetry_activity_outcome::idempotent;
			result_from_slot(result, slot);
		}
		return result;
	}
	const telemetry_connection_id expected = slot.connected != 0U ? slot.connection :
									ZERO_CONNECTION_ID;
	if (!connection_equal(expected, exit.connection) ||
	    exit.at_monotonic_usec < slot.last_observed_monotonic_usec)
		return result;
	telemetry_quality_mask quality =
		utc_quality(slot, exit.at_monotonic_usec, exit.at_utc_usec) | exit.quality_flags;
	if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
	{
		saturating_increment(state->clock_discontinuity_total);
		slot.utc_discontinuity_pending = 1U;
	}
	if (!sync_admitted_config(state, slot, exit.at_monotonic_usec, exit.at_utc_usec, quality,
				  result) ||
	    !seal_to(state, slot, exit.at_monotonic_usec, exit.at_utc_usec, quality, result))
		return result;
	observe_time(slot, exit.at_monotonic_usec, exit.at_utc_usec);
	slot.last_exit = exit;
	slot.has_last_exit = 1U;
	if (state->resident_slots != 0U)
		--state->resident_slots;
	slot.lifecycle = telemetry_activity_slot_lifecycle::closed;
	slot.connected = 0U;
	slot.connection = ZERO_CONNECTION_ID;
	slot.mode = telemetry_activity_accounting_mode::unknown;
	slot.active_deadline_monotonic_usec = 0U;
	if (state->closed_slots != std::numeric_limits<std::uint16_t>::max())
		++state->closed_slots;
	if (result.outcome == telemetry_activity_outcome::invalid)
		result.outcome = slot.detail_ready == 0U ?
					 telemetry_activity_outcome::accepted_degraded :
					 telemetry_activity_outcome::accepted;
	result_from_slot(result, slot);
	return result;
}

telemetry_activity_pulse_result
telemetry_activity_state_pulse(telemetry_activity_state *state,
			       telemetry_activity_pulse_request request) noexcept
{
	telemetry_activity_pulse_result pulse{};
	pulse.outcome = telemetry_activity_outcome::invalid;
	if (!state_ready(state) || !telemetry_activity_pulse_request_is_valid(request) ||
	    request.slot >= state->pulse_slot_count)
		return pulse;
	// Reserve caller-owned delivery slots BEFORE consuming any elapsed time.
	// Otherwise #264 receives a later noncontiguous delta it cannot apply.
	std::uint16_t required = 0U;
	for (std::size_t index = 0U; index < state->max_slots; ++index)
		if (state->slots[index].lifecycle == telemetry_activity_slot_lifecycle::resident &&
		    index % state->pulse_slot_count == request.slot)
			++required;
	if (required != 0U && (request.deltas == nullptr || request.delta_capacity < required))
		return pulse;
	pulse.outcome = telemetry_activity_outcome::idempotent;
	bool any_invalid = false;
	for (std::size_t index = 0U; index < state->max_slots; ++index)
	{
		telemetry_activity_slot &slot = state->slots[index];
		if (slot.lifecycle != telemetry_activity_slot_lifecycle::resident ||
		    index % state->pulse_slot_count != request.slot)
			continue;
		if (pulse.sessions_considered != std::numeric_limits<std::uint16_t>::max())
			++pulse.sessions_considered;
		if (request.now_monotonic_usec < slot.last_observed_monotonic_usec)
		{
			any_invalid = true;
			pulse.quality_flags |= TELEMETRY_QUALITY_SEQUENCE_GAP;
			continue;
		}
		if (!should_process_pulse(state, slot, request.now_monotonic_usec,
					  request.occurrence_utc_usec))
			continue;
		telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
		telemetry_quality_mask quality =
			utc_quality(slot, request.now_monotonic_usec, request.occurrence_utc_usec);
		if ((quality & TELEMETRY_QUALITY_CLOCK_DISCONTINUITY) != 0U)
		{
			saturating_increment(state->clock_discontinuity_total);
			slot.utc_discontinuity_pending = 1U;
		}
		const bool sealed = sync_admitted_config(state, slot, request.now_monotonic_usec,
							 request.occurrence_utc_usec, quality,
							 result) &&
				    seal_to(state, slot, request.now_monotonic_usec,
					    request.occurrence_utc_usec, quality, result);
		if (!sealed)
		{
			set_failure(result, telemetry_activity_outcome::invalid);
			any_invalid = true;
		}
		else if (result.outcome == telemetry_activity_outcome::invalid)
			result.outcome = slot.detail_ready == 0U ?
						 telemetry_activity_outcome::accepted_degraded :
						 telemetry_activity_outcome::accepted;
		if (sealed)
			observe_time(slot, request.now_monotonic_usec, request.occurrence_utc_usec);
		result_from_slot(result, slot);
		if (result.has_delta != 0U)
		{
			if (request.deltas != nullptr &&
			    pulse.deltas_written < request.delta_capacity)
				request.deltas[pulse.deltas_written++] = result.delta;
			else
			{
				if (pulse.deltas_dropped !=
				    std::numeric_limits<std::uint16_t>::max())
					++pulse.deltas_dropped;
				pulse.quality_flags |= TELEMETRY_QUALITY_QUEUE_DROP |
						       TELEMETRY_QUALITY_SEQUENCE_GAP;
			}
		}
		if (pulse.intervals_sealed >
		    std::numeric_limits<std::uint16_t>::max() - result.intervals_sealed)
			pulse.intervals_sealed = std::numeric_limits<std::uint16_t>::max();
		else
			pulse.intervals_sealed += result.intervals_sealed;
		if (pulse.records_attempted >
		    std::numeric_limits<std::uint16_t>::max() - result.records_attempted)
			pulse.records_attempted = std::numeric_limits<std::uint16_t>::max();
		else
			pulse.records_attempted += result.records_attempted;
		if (pulse.records_accepted >
		    std::numeric_limits<std::uint16_t>::max() - result.records_accepted)
			pulse.records_accepted = std::numeric_limits<std::uint16_t>::max();
		else
			pulse.records_accepted += result.records_accepted;
		if (pulse.records_dropped >
		    std::numeric_limits<std::uint16_t>::max() - result.records_dropped)
			pulse.records_dropped = std::numeric_limits<std::uint16_t>::max();
		else
			pulse.records_dropped += result.records_dropped;
		pulse.quality_flags |= result.quality_flags;
		if (result.intervals_sealed != 0U || result.has_delta != 0U)
			pulse.outcome = result.outcome;
	}
	if (any_invalid)
		pulse.outcome = telemetry_activity_outcome::invalid;
	return pulse;
}

telemetry_activity_result
telemetry_activity_state_retire_expired(telemetry_activity_state *state) noexcept
{
	telemetry_activity_result result = result_with(telemetry_activity_outcome::invalid);
	if (!state_ready(state))
		return result;
	telemetry_monotonic_usec now_monotonic_usec = 0U;
	telemetry_utc_usec now_utc_usec = TELEMETRY_UTC_UNKNOWN;
	if (state->clock.now == nullptr ||
	    !state->clock.now(state->clock.context, &now_monotonic_usec, &now_utc_usec))
	{
		result.outcome = telemetry_activity_outcome::clock_unavailable;
		return result;
	}
	result.outcome = telemetry_activity_outcome::idempotent;
	for (std::size_t count = 0U; count < state->max_slots; ++count)
	{
		const std::size_t index = find_expired_detached(state, now_monotonic_usec);
		if (index >= state->max_slots)
			break;
		telemetry_activity_result retired = retire_one_at(state, index, now_utc_usec);
		if (result.records_attempted >
		    std::numeric_limits<std::uint16_t>::max() - retired.records_attempted)
			result.records_attempted = std::numeric_limits<std::uint16_t>::max();
		else
			result.records_attempted += retired.records_attempted;
		if (result.records_accepted >
		    std::numeric_limits<std::uint16_t>::max() - retired.records_accepted)
			result.records_accepted = std::numeric_limits<std::uint16_t>::max();
		else
			result.records_accepted += retired.records_accepted;
		if (result.records_dropped >
		    std::numeric_limits<std::uint16_t>::max() - retired.records_dropped)
			result.records_dropped = std::numeric_limits<std::uint16_t>::max();
		else
			result.records_dropped += retired.records_dropped;
		if (result.slots_retired >
		    std::numeric_limits<std::uint16_t>::max() - retired.slots_retired)
			result.slots_retired = std::numeric_limits<std::uint16_t>::max();
		else
			result.slots_retired += retired.slots_retired;
		result.quality_flags |= retired.quality_flags;
		result.outcome = retired.outcome;
	}
	return result;
}

bool telemetry_activity_state_copy_view(const telemetry_activity_state *state,
					telemetry_session_ref session,
					telemetry_activity_state_view *view) noexcept
{
	if (view != nullptr)
		*view = {};
	if (!state_ready(state) || view == nullptr)
		return false;
	const int found = find_slot(state, session);
	if (found < 0)
		return false;
	const telemetry_activity_slot &slot = state->slots[static_cast<std::size_t>(found)];
	view->session = slot.session;
	view->connection = slot.connected != 0U ? slot.connection : ZERO_CONNECTION_ID;
	view->cumulative = slot.cumulative;
	view->dimensions = slot.dimensions;
	view->config_id = slot.config_id;
	view->classifier_version = slot.classifier_version;
	view->policy_version = slot.policy_version;
	view->interval_start_monotonic_usec = slot.interval_start_monotonic_usec;
	view->active_deadline_monotonic_usec = slot.active_deadline_monotonic_usec;
	view->context_segments_used = slot.context_segments_used;
	view->category = category_for(slot);
	view->context = slot.context_overflow != 0U ? telemetry_activity_context::overflow_unknown :
						      slot.context;
	view->context_quality = slot.context_overflow != 0U ? telemetry_context_quality::overflow :
							      slot.context_quality;
	view->quality_flags = slot.quality_flags;
	view->connected = slot.connected;
	view->closed = slot.lifecycle == telemetry_activity_slot_lifecycle::closed ? 1U : 0U;
	view->detail_ready = slot.detail_ready;
	view->context_overflow = slot.context_overflow;
	return true;
}

telemetry_activity_state_stats
telemetry_activity_state_stats_copy(const telemetry_activity_state *state) noexcept
{
	telemetry_activity_state_stats stats{};
	if (!state_ready(state))
		return stats;
	stats.capacity = state->max_slots;
	stats.used_slots = state->used_slots;
	stats.resident_slots = state->resident_slots;
	stats.closed_slots = state->closed_slots;
	stats.admitted_config_total = state->admitted_config_total;
	stats.accepted_detail_total = state->accepted_detail_total;
	stats.dropped_detail_total = state->dropped_detail_total;
	stats.accepted_control_total = state->accepted_control_total;
	stats.dropped_control_total = state->dropped_control_total;
	stats.suppressed_detail_total = state->suppressed_detail_total;
	stats.context_overflow_total = state->context_overflow_total;
	stats.clock_discontinuity_total = state->clock_discontinuity_total;
	stats.retired_session_total = state->retired_session_total;
	return stats;
}
